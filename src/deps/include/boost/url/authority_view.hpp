//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_AUTHORITY_VIEW_HPP
#define BOOST_URL_AUTHORITY_VIEW_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/host_type.hpp>
#include <boost/url/ipv4_address.hpp>
#include <boost/url/ipv6_address.hpp>
#include <boost/url/pct_encoding.hpp>
#include <boost/url/detail/except.hpp>
#include <boost/assert.hpp>
#include <cstddef>
#include <iosfwd>
#include <utility>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
struct authority_bnf;
struct host_bnf;
#endif

/** A read-only view to an authority.

    Objects of this type represent a valid
    authority whose storage is managed externally.
    That is, it acts like a `std::string_view` in
    terms of ownership. Callers are responsible
    for ensuring that the lifetime of the
    underlying string extends until the
    `authority_view` is no longer in use.
    To construct from an existing string it is
    necessary to use one of the parsing
    functions. Each function parses against
    a particular URL grammar:

    @par Example
    @code
    authority_view a;

    a = parse_authority( "www.example.com:443" );
    @endcode

    @par BNF
    @code
    authority     = [ userinfo "@" ] host [ ":" port ]

    userinfo      = user [ ":" [ password ] ]

    user          = *( unreserved / pct-encoded / sub-delims )
    password      = *( unreserved / pct-encoded / sub-delims / ":" )

    host          = IP-literal / IPv4address / reg-name

    port          = *DIGIT
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2"
        >3.2. Authority (rfc3986)</a>

    @see
        @ref parse_authority.
*/
class BOOST_SYMBOL_VISIBLE
    authority_view
{
    enum
    {
        id_user = 0,
        id_pass,        // leading ':', trailing '@'
        id_host,
        id_port,        // leading ':'
        id_end          // one past the end
    };

    static
    constexpr
    pos_t zero_ = 0;

    static
    constexpr
    char const* const empty_ = "";

    char const* cs_ = empty_;
    pos_t offset_[id_end + 1] = {};
    pos_t decoded_[id_end] = {};
    unsigned char ip_addr_[16] = {};
    // VFALCO don't we need a bool?
    std::uint16_t port_number_ = 0;
    urls::host_type host_type_ =
        urls::host_type::none;

    struct shared_impl;

    inline pos_t len(int first,
        int last) const noexcept;
    inline void set_size(
        int id, pos_t n) noexcept;
    explicit authority_view(
        char const* cs) noexcept;
    authority_view(
        authority_view const& u,
            char const* cs) noexcept;

    // return offset of id
    BOOST_URL_CONSTEXPR
    auto
    offset(int id) const noexcept ->
        pos_t
    {
        return
            id == id_user ?
            zero_ : offset_[id - 1];
    }

    // return length of part
    BOOST_URL_CONSTEXPR
    auto
    len(int id) const noexcept ->
        pos_t
    {
        return
            offset(id + 1) -
            offset(id);
    }

    // return id as string
    BOOST_URL_CONSTEXPR
    string_view
    get(int id) const noexcept
    {
        return {
            cs_ + offset(id), len(id) };
    }

    // return [first, last) as string
    BOOST_URL_CONSTEXPR
    string_view
    get(int first,
        int last) const noexcept
    {
        return { cs_ + offset(first),
            offset(last) - offset(first) };
    }

public:
    /** The type of elements.
    */
    using value_type        = char;

    /** The type of pointers to elements.
    */
    using pointer           = char const*;

    /** The type of const pointers to elements.
    */
    using const_pointer     = char const*;

    /** The type of reference to elements.
    */
    using reference         = char const&;

    /** The type of const references to elements.
    */
    using const_reference   = char const&;

    /** The type of const iterator to elements.
    */
    using const_iterator    = char const*;

    /** The type of iterator to elements.
    */
    using iterator          = char const*;

    /** An unsigned integer type to represent sizes.
    */
    using size_type         = std::size_t;

    /** A signed integer type to represent differences.
    */
    using difference_type   = std::ptrdiff_t;

    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Destructor
    */
    BOOST_URL_DECL
    virtual ~authority_view();

    /** Constructor

        Default constructed authorities
        refer to a string with zero length,
        which is always valid. This matches
        the grammar for a zero-length host.

        @par Exception Safety
        Throws nothing.

        @par Specification
    */
    BOOST_URL_DECL
    authority_view() noexcept;

    BOOST_URL_DECL
    authority_view(
        authority_view const&) noexcept;

    BOOST_URL_DECL
    authority_view&
    operator=(
        authority_view const&) noexcept;

    //--------------------------------------------
    //
    // Capacity
    // Element Access
    //
    //--------------------------------------------

    /** Return the maximum number of characters allowed in the authority.

        This includes any null terminator, if present

        @par Exception Safety
        Throws nothing.
    */
    static
    constexpr
    std::size_t
    max_size() noexcept
    {
        return BOOST_URL_MAX_SIZE;
    }

    /** Return the number of characters in the authority.

        This function returns the number
        of characters in the authority, not
        including any null terminator.

        @par Exception Safety
        Throws nothing.
    */
    std::size_t
    size() const noexcept
    {
        return offset(id_end);
    }

    /** Return true if the contents are empty.

        @par Exception Safety
        Throws nothing.
    */
    bool
    empty() const noexcept
    {
        return size() == 0;
    }

    /** Return a pointer to the first character

        This function returns a pointer to the
        beginning of the view, which is not
        guaranteed to be null-terminated.

        @par Exception Safety
        Throws nothing.
    */
    char const*
    data() const noexcept
    {
        return cs_;
    }

    /** Access the specified character

        This function returns a reference to
        the character at the specified zero-based
        position. If `pos` is out of range, an
        exception if thrown.

        @param pos The zero-based character
        position to access.

        @throw std::out_of_range pos >= size()
    */
    char const&
    at(std::size_t pos) const
    {
        if(pos >= size())
            detail::throw_out_of_range(
                BOOST_CURRENT_LOCATION);
        return cs_[pos];
    }

    /** Access the specified character

        This function returns a reference to
        the character at the specified zero-based
        position. The behavior is undefined if
        `pos` is out of range.

        @par Preconditions
        @code
        pos < this->size()
        @endcode
    */
    char const&
    operator[](
        std::size_t pos) const noexcept
    {
        BOOST_ASSERT(pos < size());
        return cs_[pos];
    }

    /** Return an iterator to the beginning

        This function returns a constant iterator
        to the first character of the view, or
        one past the last element if the view is
        empty.
    */
    char const*
    begin() const noexcept
    {
        return data();
    }

    /** Return an iterator to the end

        This function returns a constant iterator to
        the character following the last character of
        the view. This character acts as a placeholder,
        attempting to access it results in undefined
        behavior.
    */
    char const*
    end() const noexcept
    {
        return data() + size();
    }

    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return a read-only copy of the authority, with shared lifetime.

        This function makes a copy of the storage
        pointed to by this, and attaches it to a
        new constant @ref authority_view returned in a
        shared pointer. The lifetime of the storage
        for the characters will extend for the
        lifetime of the shared object. This allows
        the new view to be copied and passed around
        after the original string buffer is destroyed.

        @par Example
        @code
        std::shared_ptr<authority const> sp;
        {
            std::string s( "user:pass@example.com:443" );
            authority_view a = parse_authority( s ).value();    // a references characters in s

            assert( a.data() == s.data() );                     // same buffer

            sp = a.collect();

            assert( sp->data() != a.data() );                   // different buffer
            assert( sp->encoded_authority() == s);              // same contents

            // s is destroyed and thus a
            // becomes invalid, but sp remains valid.
        }
        std::cout << *sp; // works
        @endcode
    */
    BOOST_URL_DECL
    std::shared_ptr<
        authority_view const>
    collect() const;

    /** Return the complete authority

        This function returns the authority
        as a percent-encoded string.

        @par Example
        @code
        assert( parse_authority( "www.example.com" ).value().encoded_authority() == "www.example.com" );
        @endcode

        @par BNF
        @code
        authority   = [ userinfo "@" ] host [ ":" port ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2"
            >3.2. Authority (rfc3986)</a>
    */
    string_view
    encoded_authority() const noexcept
    {
        return string_view(
            data(), size());
    }

    //--------------------------------------------

    /** Return true if this contains a userinfo

        This function returns true if this
        contains a userinfo.

        @par Example
        @code
        authority_view a = parse_authority( "user@example.com" ).value();

        assert( a.has_userinfo() == true );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        authority   = [ userinfo "@" ] host [ ":" port ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_userinfo,
            @ref userinfo.
    */
    BOOST_URL_DECL
    bool
    has_userinfo() const noexcept;

    /** Return the userinfo

        This function returns the userinfo
        as a percent-encoded string.

        @par Example
        @code
        assert( parse_authority( "user:pass@example.com" ).value().encoded_userinfo() == "user:pass" );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        authority   = [ userinfo "@" ] host [ ":" port ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref has_userinfo,
            @ref userinfo.
    */
    BOOST_URL_DECL
    string_view
    encoded_userinfo() const noexcept;

    /** Return the userinfo

        This function returns the userinfo as a
        string with percent-decoding applied, using
        the optionally specified allocator.

        @par Example
        @code
        authority_view a = parse_authority( "user:pass@example.com" ).value();

        assert( a.userinfo() == "user:pass" );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        authority   = [ userinfo "@" ] host [ ":" port ]
        @endcode

        @par Exception Safety
        Calls to allocate may throw.

        @param a An optional allocator the returned
        string will use. If this parameter is omitted,
        the default allocator is used.

        @return A @ref string_value using the
        specified allocator.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref has_userinfo,
            @ref encoded_userinfo.
    */
    template<
        class Allocator =
            std::allocator<char>>
    string_value
    userinfo(
        Allocator const& a = {}) const
    {
        pct_decode_opts opt;
        opt.plus_to_space = false;
        return pct_decode_unchecked(
            encoded_userinfo(), opt, a);
    }

    //--------------------------------------------

    /** Return the user

        This function returns the user portion of
        the userinfo as a percent-encoded string.

        @par Example
        @code
        authority_view a = parse_authority( "user:pass@example.com" ).value();

        assert( a.encoded_user() == "user" );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        user        = *( unreserved / pct-encoded / sub-delims )
        password    = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_password,
            @ref has_password,
            @ref password,
            @ref user.
    */
    string_view
    encoded_user() const noexcept
    {
        return get(id_user);
    }

    /** Return the user

        This function returns the user portion of
        the userinfo as a string with percent-decoding
        applied, using the optionally specified
        allocator.

        @par Example
        @code
        assert( parse_authority( "user:pass@example.com" ).value().user() == "user" );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        user        = *( unreserved / pct-encoded / sub-delims )
        password    = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Calls to allocate may throw.

        @param a An optional allocator the returned
        string will use. If this parameter is omitted,
        the default allocator is used.

        @return A @ref string_value using the
        specified allocator.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_password,
            @ref encoded_user,
            @ref has_password,
            @ref password.
    */
    template<
        class Allocator =
            std::allocator<char>>
    string_value
    user(
        Allocator const& a = {}) const
    {
        pct_decode_opts opt;
        opt.plus_to_space = false;
        return pct_decode_unchecked(
            encoded_user(), opt, a);
    }

    /** Return true if this contains a password

        This function returns true if the userinfo
        contains a password (which may be empty).

        @par Example
        @code
        assert( parse_authority( "user@example.com" ).value().has_password() == false );

        assert( parse_authority( "user:pass@example.com" ).value().has_password() == true );

        assert( parse_authority( ":@" ).value().has_password() == true );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        user        = *( unreserved / pct-encoded / sub-delims )
        password    = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_password,
            @ref encoded_user,
            @ref password,
            @ref user.
    */
    BOOST_URL_DECL
    bool
    has_password() const noexcept;

    /** Return the password

        This function returns the password portion
        of the userinfo as a percent-encoded string.

        @par Example
        @code
        authority_view a = parse_authority( "user:pass@example.com" ).value();

        assert( a.encoded_user() == "user" );
        @endcode

        @par BNF
        @code
        userinfo    = user [ ":" [ password ] ]

        user        = *( unreserved / pct-encoded / sub-delims )
        password    = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_password,
            @ref has_password,
            @ref password,
            @ref user.
    */
    BOOST_URL_DECL
    string_view
    encoded_password() const noexcept;

    /** Return the password

        This function returns the password from the
        userinfo with percent-decoding applied,
        using the optionally specified allocator.

        @par Exception Safety
        Calls to allocate may throw.

        @param a An allocator to use for the string.
        If this parameter is omitted, the default
        allocator is used, and the return type of
        the function becomes `std::string`.

        @return A @ref string_value using the
        specified allocator.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
            >3.2.1. User Information (rfc3986)</a>

        @see
            @ref encoded_password,
            @ref encoded_user,
            @ref has_password,
            @ref password.
    */
    template<
        class Allocator =
            std::allocator<char>>
    string_value
    password(
        Allocator const& a = {}) const
    {
        pct_decode_opts opt;
        opt.plus_to_space = false;
        return pct_decode_unchecked(
            encoded_password(), opt, a);
    }

    //--------------------------------------------

    /** Return the type of host present, if any

        This function returns a
            @ref urls::host_type
        constant representing the type of
        host this contains, which may be
        @ref urls::host_type::none.

        @par Example
        @code
        assert( authority_view().host_type() == host_type::none );

        assert( parse_authority( "example.com" ).value().host_type() == host_type::name );

        assert( parse_authority( "192.168.0.1" ).value().host_type() == host_type::ipv4 );
        @endcode

        @par BNF
        @code
        host        = IP-literal / IPv4address / reg-name

        IP-literal  = "[" ( IPv6address / IPvFuture  ) "]"

        IPvFuture   = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref has_port,
            @ref host,
            @ref port,
            @ref port_number.
    */
    urls::host_type
    host_type() const noexcept
    {
        return host_type_;
    }

    /** Return the host

        This function returns the host portion of
        the authority as a percent-encoded string,

        @par Example
        @code
        assert( parse_authority( "" ).value().encoded_host() == "" );

        assert( parse_authority( "http://example.com" ).value().encoded_host() == "example.com" );

        assert( parse_authority( "http://192.168.0.1" ).value().encoded_host() == "192.168.0.1" );
        @endcode

        @par BNF
        @code
        host        = IP-literal / IPv4address / reg-name

        IP-literal  = "[" ( IPv6address / IPvFuture  ) "]"

        IPvFuture   = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

        reg-name    = *( unreserved / pct-encoded / "-" / ".")
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>

        @see
            @ref has_port,
            @ref host,
            @ref host_type,
            @ref port,
            @ref port_number.
    */
    BOOST_URL_DECL
    string_view
    encoded_host() const noexcept;

    /** Return the host

        This function returns the host portion of
        the authority as a string with percent-decoding
        applied, using the optionally specified
        allocator.

        @par Example
        @code
        assert( parse_authority( "" ).value().host() == "" );

        assert( parse_authority( "example.com" ).value().host() == "example.com" );

        assert( parse_authority( "192.168.0.1" ).value().host() == "192.168.0.1" );
        @endcode

        @par BNF
        @code
        host        = IP-literal / IPv4address / reg-name

        IP-literal  = "[" ( IPv6address / IPvFuture  ) "]"

        IPvFuture   = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

        reg-name    = *( unreserved / pct-encoded / "-" / ".")
        @endcode

        @par Exception Safety
        Calls to allocate may throw.

        @param a An optional allocator the returned
        string will use. If this parameter is omitted,
        the default allocator is used, and the return
        type of the function becomes `std::string`.

        @return A @ref string_value using the
        specified allocator.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref encoded_host_and_port
            @ref has_port,
            @ref host_type,
            @ref port,
            @ref port_number.
    */
    template<
        class Allocator =
            std::allocator<char>>
    string_value
    host(
        Allocator const& a = {}) const
    {
        auto const s0 =
            encoded_host();
        if(host_type_ !=
            urls::host_type::name)
        {
            // no decoding
            return string_value(s0, a);
        }
        pct_decode_opts opt;
        opt.plus_to_space = false;
        return pct_decode_unchecked(
            s0, opt, a, decoded_[id_host]);
    }

    /** Return the host as an IPv4 address

        If @ref host_type equals @ref urls::host_type::ipv4,
        this function returns the corresponding
        @ref ipv4_address
        of the host if it exists, otherwise it
        returns the unspecified address which
        is equal to "0.0.0.0".

        @par BNF
        @code
        IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet

        dec-octet   = DIGIT                 ; 0-9
                    / %x31-39 DIGIT         ; 10-99
                    / "1" 2DIGIT            ; 100-199
                    / "2" %x30-34 DIGIT     ; 200-249
                    / "25" %x30-35          ; 250-255
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>

        @see
            @ref ipv4_address.
    */
    BOOST_URL_DECL
    urls::ipv4_address
    ipv4_address() const noexcept;

    /** Return the host as an IPv6 address

        If @ref host_type equals
        @ref urls::host_type::ipv6, this
        function returns the corresponding
        @ref ipv6_address of the host if it
        exists, otherwise it returns the
        unspecified address which is equal
        to "0:0:0:0:0:0:0:0".

        @par BNF
        @code
        IPv6address =                            6( h16 ":" ) ls32
                    /                       "::" 5( h16 ":" ) ls32
                    / [               h16 ] "::" 4( h16 ":" ) ls32
                    / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                    / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                    / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                    / [ *4( h16 ":" ) h16 ] "::"              ls32
                    / [ *5( h16 ":" ) h16 ] "::"              h16
                    / [ *6( h16 ":" ) h16 ] "::"

        ls32        = ( h16 ":" h16 ) / IPv4address
                    ; least-significant 32 bits of address

        h16         = 1*4HEXDIG
                    ; 16 bits of address represented in hexadecimal
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>

        @see
            @ref ipv6_address.
    */
    BOOST_URL_DECL
    urls::ipv6_address
    ipv6_address() const noexcept;

    /** Return the host as an IPvFuture string

        If @ref host_type equals
        @ref urls::host_type::ipvfuture, this
        function returns a string representing
        the address. Otherwise it returns the
        empty string.

        @par BNF
        @code
        IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2"
            >3.2.2. Host (rfc3986)</a>
    */
    BOOST_URL_DECL
    string_view
    ipv_future() const noexcept;

    /** Return true if the URL contains a port

        This function returns true if the
        authority contains a port.

        @par BNF
        @code
        authority   = [ userinfo "@" ] host [ ":" port ]

        port        = *DIGIT
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3"
            >3.2.3. Port (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref encoded_host_and_port
            @ref host,
            @ref host_type,
            @ref port,
            @ref port_number.
    */
    BOOST_URL_DECL
    bool
    has_port() const noexcept;

    /** Return the port

        This function returns the port specified
        in the authority, or an empty string if
        there is no port.

        @par BNF
        @code
        port        = *DIGIT
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3"
            >3.2.3. Port (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref encoded_host_and_port
            @ref has_port,
            @ref host,
            @ref host_type,
            @ref port_number.
    */
    BOOST_URL_DECL
    string_view
    port() const noexcept;

    /** Return the port as an intege

        This function returns the port as an
        integer if the authority specifies
        a port and the number can be represented.
        Otherwise it returns zero.

        @par BNF
        @code
        port        = *DIGIT
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3"
            >3.2.3. Port (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref encoded_host_and_port
            @ref has_port,
            @ref host,
            @ref host_type,
            @ref port,
    */
    BOOST_URL_DECL
    std::uint16_t
    port_number() const noexcept;

    /** Return the host and port

        This function returns the host and
        port of the authority as a single
        percent-encoded string.

        @par BNF
        @code
        authority   = [ userinfo "@" ] host [ ":" port ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3"
            >3.2.3. Port (rfc3986)</a>

        @see
            @ref encoded_host,
            @ref encoded_host_and_port
            @ref has_port,
            @ref host,
            @ref host_type,
            @ref port,
    */
    BOOST_URL_DECL
    string_view
    encoded_host_and_port() const noexcept;

    //--------------------------------------------
    //
    // Parsing
    //
    //--------------------------------------------

    BOOST_URL_DECL friend result<authority_view>
        parse_authority(string_view s) noexcept;

private:
    void apply(host_bnf const& h) noexcept;
    void apply(authority_bnf const& t) noexcept;
};

//------------------------------------------------

/** Parse an authority

    This function parses a string according to
    the authority grammar below, and returns an
    @ref authority_view referencing the string.
    Ownership of the string is not transferred;
    the caller is responsible for ensuring that
    the lifetime of the string extends until the
    view is no longer being accessed.

    @par BNF
    @code
    authority     = [ userinfo "@" ] host [ ":" port ]

    userinfo      = user [ ":" [ password ] ]

    user          = *( unreserved / pct-encoded / sub-delims )
    password      = *( unreserved / pct-encoded / sub-delims / ":" )

    host          = IP-literal / IPv4address / reg-name

    port          = *DIGIT
    @endcode

    @par Exception Safety
    Throws nothing.

    @return A view to the parsed authority

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2"
        >3.2. Authority (rfc3986)</a>

    @see
        @ref authority_view.
*/
BOOST_URL_DECL
result<authority_view>
parse_authority(
    string_view s) noexcept;

//------------------------------------------------

/** Format the encoded authority to the output stream

    This function serializes the encoded URL
    to the output stream.

    @par Example
    @code
    url_view u = parse_uri( "http://www.example.com/index.htm" );

    std::cout << u << std::endl;
    @endcode

    @return A reference to the output stream, for chaining

    @param os The output stream to write to

    @param u The URL to write
*/
BOOST_URL_DECL
std::ostream&
operator<<(
    std::ostream& os,
    authority_view const& a);

} // urls
} // boost

#include <boost/url/impl/authority_view.hpp>

#endif
