//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_URL_HPP
#define BOOST_URL_URL_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/ipv4_address.hpp>
#include <boost/url/ipv6_address.hpp>
#include <boost/url/params.hpp>
#include <boost/url/params_encoded.hpp>
#include <boost/url/scheme.hpp>
#include <boost/url/segments.hpp>
#include <boost/url/segments_encoded.hpp>
#include <boost/url/url_view.hpp>
#include <boost/url/detail/any_path_iter.hpp>
#include <boost/url/detail/any_query_iter.hpp>
#include <boost/url/detail/pct_encoding.hpp>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
class url_view;
namespace detail {
struct any_path_iter;
}
#endif

/** A modifiable container for a URL.

    Objects of this type hold URLs which may be
    inspected and modified. The derived class
    is responsible for providing storage.

    The underlying string stored in the container
    is always null-terminated.

    @par Exception Safety

    @li Functions marked `noexcept` provide the
    no-throw guarantee, otherwise:

    @li Functions which throw offer the strong
    exception safety guarantee.

    @par BNF
    @code
    URI-reference = URI / relative-ref

    URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

    relative-ref  = relative-part [ "?" query ] [ "#" fragment ]

    absolute-URI  = scheme ":" hier-part [ "?" query ]
    @endcode

    @par Specification
    @li <a href="https://tools.ietf.org/html/rfc3986"
        >Uniform Resource Identifier (URI): Generic Syntax (rfc3986)</a>

    @see
        @ref parse_absolute_uri,
        @ref parse_relative_ref,
        @ref parse_uri,
        @ref parse_uri_reference,
        @ref resolve.
*/
class BOOST_SYMBOL_VISIBLE url
    : public url_view
{
    friend class urls::segments;
    friend class urls::params;
    friend class segments_encoded;
    friend class params_encoded;

#ifndef BOOST_URL_DOCS
protected:
#endif
    char* s_ = nullptr;
    std::size_t cap_ = 0;

    BOOST_URL_DECL
    url(char* buf,
        std::size_t cap) noexcept;

    BOOST_URL_DECL
    void
    copy(url_view const& u);

    BOOST_URL_DECL
    virtual
    char*
    allocate(
        std::size_t new_cap);

    BOOST_URL_DECL
    virtual
    void
    deallocate(char* s);

public:
    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Destructor

        Any params, segments, or iterators
        which reference this object are
        invalidated.
    */
    BOOST_URL_DECL
    virtual
    ~url();

    /** Constructor

        Default constructed urls contain
        a zero-length string. This matches
        the grammar for a relative-ref with
        an empty path and no query or
        fragment.

        @par BNF
        @code
        relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-4.2"
            >4.2. Relative Reference (rfc3986)</a>
    */
    BOOST_URL_DECL
    url() noexcept;

    /** Constructor

        This function performs a move-construction
        from `u`. After the move. the state of `u`
        will be as-if default constructed.

        @param Exception Safety
        Throws nothing.

        @param u The url to construct from.
    */
    BOOST_URL_DECL
    url(url&& u) noexcept;

    /** Constructor

        This function constructs a copy of `u`.

        @param u The url to construct from.
    */
    BOOST_URL_DECL
    url(url const& u);

    /** Constructor

        This function constructs a copy of `u`.

        @param u The url to construct from.
    */
    BOOST_URL_DECL
    url(url_view const& u);

    /** Assignment

        This function performs a move-assignment
        from `u`. After the move. the state of `u`
        will be as-if default constructed.

        @param Exception Safety
        Throws nothing.

        @param u The url to assign from.
    */
    BOOST_URL_DECL
    url&
    operator=(url&& u) noexcept;

    /** Assignment

        This function assigns a copy of `u`
        to `*this`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param u The url to copy.
    */
    url&
    operator=(url const& u)
    {
        *this = u.base();
        return *this;
    }

    /** Assignment

        This function assigns a copy of `u`
        to `*this`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param u The url to copy.
    */
    BOOST_URL_DECL
    url&
    operator=(url_view const& u);

    /** Construct from a string

        This function constructs a URL from
        the string `s`, which must contain a
        valid URI or <em>relative-ref</em> or
        else an exception is thrown.

        @par BNF
        @code
        URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

        relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        @endcode

        @throw std::invalid_argument parse error.

        @param s The string to parse.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-4.1"
            >4.1. URI Reference</a>
    */
    BOOST_URL_DECL
    url(string_view s);

    //--------------------------------------------

    /** Return the encoded URL as a null-terminated string
    */
    char const*
    c_str() const noexcept
    {
        return cs_;
    }

    /** Returns the total number of bytes currently available to the container
    */
    std::size_t
    capacity_in_bytes() const noexcept
    {
        return cap_;
    }

    /** Adjust the capacity without changing the size

        This function adjusts the capacity
        of the container in bytes, without
        affecting the current contents.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @throw bad_alloc Allocation failure

        @par bytes The number of bytes to reserve.
    */
    BOOST_URL_DECL
    void
    reserve_bytes(
        std::size_t bytes);

    /** Clear the contents.
    
        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_URL_DECL
    void
    clear() noexcept;

    //--------------------------------------------
    //
    // Scheme
    //
    //--------------------------------------------

private:
    void set_scheme_impl(string_view s, urls::scheme id);
public:

    /** Remove the scheme

        This function removes the scheme if it
        is present.

        @par BNF
        @code
        URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.1">
            3.1. Scheme (rfc3986)</a>

        @see @ref set_scheme.
    */
    BOOST_URL_DECL
    url&
    remove_scheme() noexcept;

    /** Set the scheme

        This function sets the scheme to the specified
        string, which must contain a valid scheme
        without a trailing colon (':'). If the scheme
        is invalid, an exception is thrown.

        @par Example
        @code
        url u = parse_uri( "http://www.example.com" );

        u.set_scheme( "https" );         // u is now "https://www.example.com"

        assert( u.string() == "https://www.example.com" );

        u.set_scheme( "1forall");       // throws, invalid scheme
        @endcode

        @par BNF
        @code
        scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The scheme to set.

        @throw std::invalid_argument invalid scheme.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.1">
            3.1. Scheme (rfc3986)</a>

        @see
            @ref remove_scheme.
    */
    BOOST_URL_DECL
    url&
    set_scheme(string_view s);

    /** Set the scheme

        This function sets the scheme to the specified
        known @ref urls::scheme id, which may not be
        @ref scheme::unknown or else an exception is
        thrown. If the id is @ref scheme::none, this
        function behaves as if @ref remove_scheme
        were called.

        @par Example
        @code
        url u;
        u.set_scheme( scheme::http );           // produces "http:"
        u.set_scheme( scheme::none );           // produces ""
        u.set_scheme( scheme::unknown);         // throws, invalid scheme
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param id The scheme to set.

        @throw std::invalid_argument invalid scheme.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.1">
            3.1. Scheme (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    set_scheme(urls::scheme id);

    //--------------------------------------------
    //
    // Authority
    //
    //--------------------------------------------

private:
    char* set_user_impl(std::size_t n);
public:

    /** Remove the user

        If a user is present, it is removed. If the
        user was the only component present in the
        userinfo, then the userinfo is removed without
        removing the authority.

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1">
            3.2.1. User Information (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    remove_user() noexcept;

    /** Set the user.

        The user is set to the specified string,
        replacing any previous user. If a userinfo
        was not present it is added, even if the
        user string is empty. The resulting URL
        will have an authority if it did not have
        one previously.

        Any special or reserved characters in the
        string are automatically percent-encoded.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set. This string may
        contain any characters, including nulls.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1">
            3.2.1. User Information (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    set_user(
        string_view s);

    /** Set the user.

        The user is set to the specified string,
        replacing any previous user. If a userinfo
        was not present it is added, even if the
        user string is empty. The resulting URL
        will have an authority if it did not have
        one previously.

        The string must be a valid percent-encoded
        string for the user field, otherwise an
        exception is thrown.

        @par BNF
        @code
        user          = *( unreserved / pct-encoded / sub-delims )
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.
    */
    BOOST_URL_DECL
    url&
    set_encoded_user(
        string_view s);

private:
    char* set_password_impl(std::size_t n);
public:

    /** Remove the password
    */
    BOOST_URL_DECL
    url&
    remove_password() noexcept;

    /** Set the password.

        This function sets the password to the specified
        string, replacing any previous password:

        @li If the string is empty, the password is
        cleared, and the first occurring colon (':') is
        removed from the userinfo if present, otherwise

        @li If ths string is not empty then the password
        is set to the new string.
        Any special or reserved characters in the
        string are automatically percent-encoded.
        If the URL previously did not have an authority
        (@ref has_authority returns `false`), a double
        slash ("//") is prepended to the userinfo.

        @par Exception Safety

        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set. This string may
        contain any characters, including nulls.
    */
    BOOST_URL_DECL
    url&
    set_password(
        string_view s);

    /** Set the password.

        The password is set to the encoded string `s`,
        replacing any previous password:

        @li If the string is empty, the password is
        cleared, and the first occurring colon (':') is
        removed from the userinfo if present, otherwise

        @li If ths string is not empty then the password
        is set to the new string.
        If the URL previously did not have an authority
        (@ref has_authority returns `false`), a double
        slash ("//") is prepended to the userinfo.
        The string must meet the syntactic requirements
        of <em>password</em> otherwise an exception is
        thrown.

        @par ANBF
        @code
        password      = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety

        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.
    */
    BOOST_URL_DECL
    url&
    set_encoded_password(
        string_view s);

private:
    char* set_userinfo_impl(std::size_t n);
public:

    /** Remove the userinfo
    */
    BOOST_URL_DECL
    url&
    remove_userinfo() noexcept;

    /** Set the userinfo.

        Sets the userinfo of the URL to the given
        encoded string:

        @li If the string is empty, the userinfo is
        cleared, else

        @li If the string is not empty, then the userinfo
        is set to the given string. The user is set to
        the characters up to the first colon if any,
        while the password is set to the remaining
        characters if any.
        If the URL previously did not have an authority
        (@ref has_authority returns `false`), a double
        slash ("//") is prepended to the userinfo.
        The string must meet the syntactic requirements
        of <em>userinfo</em> otherwise an exception is
        thrown.

        @par BNF
        @code
        userinfo      = [ [ user ] [ ':' password ] ]

        user          = *( unreserved / pct-encoded / sub-delims )
        password      = *( unreserved / pct-encoded / sub-delims / ":" )
        @endcode

        @par Exception Safety

        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.
    */
    BOOST_URL_DECL
    url&
    set_encoded_userinfo(
        string_view s);

    BOOST_URL_DECL
    url&
    set_userinfo(
        string_view s);

    //--------------------------------------------

private:
    char* set_host_impl(std::size_t n);
public:

    /** Set the host

        The host is set to the specified IPv4,
        address, replacing any previous host. If
        an authority was not present, it is added.

        @par Postconditions
        @code
        this->host_type() == host_type::ipv4 && this->ipv4_address() == addr
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2">
            3.2.2. Host (rfc3986)</a>

        @param addr The address to set.
    */
#ifdef BOOST_URL_DOCS
    url& set_host( ipv4_address const& addr );
#else
    BOOST_URL_DECL
    url&
    set_host(
        urls::ipv4_address const& addr);
#endif

    /** Set the host

        The host is set to the specified IPv6,
        address, replacing any previous host.
        If an authority did not
        previously exist it is added by prepending
        a double slash ("//") at the beginning of
        the URL or after the scheme if a scheme is
        present.

        @par Postconditions
        @code
        this->host_type() == host_type::ipv6 && this->ipv6_address() == addr
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2">
            3.2.2. Host (rfc3986)</a>

        @param addr The address to set.
    */
#ifdef BOOST_URL_DOCS
    url& set_host( urls::ipv6_address const& addr );
#else
    BOOST_URL_DECL
    url&
    set_host(
        urls::ipv6_address const& addr);
#endif

    /** Set the host

        The host is set to the specified plain
        string, subject to the following rules:

        @li If the string is a valid IPv4 address,
        the address is parsed and the host is set
        as if an instance of the equivalent
        @ref urls::ipv4_address were passed instead.
        In this case @ref url::host_type will return
        @ref host_type::ipv4. Otherwise,

        @li The plain string is percent-encoded and
        the result is set as the reg-name for the
        host. In this case @ref url::host_type will
        return @ref host_type::name.

        In all cases, if an authority did not
        previously exist it is added by prepending
        a double slash ("//") at the beginning of
        the URL or after the scheme if a scheme is
        present.

        @par Postconditions
        @code
        this->encoded_host() == s
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2">
            3.2.2. Host (rfc3986)</a>

        @param s The string to set.
    */
    BOOST_URL_DECL
    url&
    set_host(
        string_view s);

    /** Set the host

        The host is set to the specified percent-
        encoded string, subject to the following
        rules:

        @li If the string is a valid IPv4 address,
        the address is parsed and the host is set
        as if an instance of the equivalent
        @ref urls::ipv4_address were passed instead. In
        this case @ref url::host_type will return
        @ref host_type::ipv4. Or,

        @li If the string is a valid IPv6 address
        enclosed in square brackets ('[' and ']'),
        the address is parsed and the host is set
        as if an instance of the equivalent
        @ref urls::ipv6_address were passed instead. In
        this case @ref url::host_type will return
        @ref host_type::ipv4. Or,

        @li If the string is a valid IPvFuture address
        enclosed in square brackets ('[' and ']'),
        the address is parsed and the host is set
        to the specified string. In this case
        @ref url::host_type will return
        @ref host_type::ipvfuture. Or,

        @li If the string is a valid percent-encoded
        string with no characters from the reserved
        character set, then it is set as the encoded
        host name. In this case @ref url::host_type
        will return @ref host_type::name. Otherwise,

        @li If the string does not contain a valid
        percent-encoding for the host field, an
        exception is thrown.

        In all cases, if an authority did not
        previously exist it is added by prepending
        a double slash ("//") at the beginning of
        the URL or after the scheme if a scheme is
        present.
        
        @par Postconditions
        @code
        this->encoded_host() == s
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2">
            3.2.2. Host (rfc3986)</a>

        @param s The string to set.

        @throw std::invalid_argument the percent-encoding is invalid
    */
    BOOST_URL_DECL
    url&
    set_encoded_host(string_view s);

private:
    char* set_port_impl(std::size_t n);
public:

    /** Remove the port

        If a port is present, it is removed.
        The remainder of the authority component
        is left unchanged including the leading
        double slash ("//").

        @par Postconditions
        @code
        this->has_port() == false && this->port_number() == 0 && this->port() == ""
        @endcode

        @par Exception Safety
        Throws nothing.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3">
            3.2.3. Port (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    remove_port() noexcept;

    /** Set the port

        The port of the URL is set to the specified
        integer, replacing any previous port.
        If an authority did not
        previously exist it is added by prepending
        a double slash ("//") at the beginning of
        the URL or after the scheme if a scheme is
        present.

        @par Postconditions
        @code
        this->has_port() == true && this->port_number() == n && this->port() == std::to_string(n)
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3">
            3.2.3. Port (rfc3986)</a>

        @param n The port number to set.
    */
    BOOST_URL_DECL
    url&
    set_port(std::uint16_t n);

    /** Set the port

        The port of the URL is set to the specified
        string, replacing any previous port. The string
        must meet the syntactic requirements for PORT,
        which consists only of digits. The string
        may be empty. In this case the port is still
        defined, however it is the empty string. To
        remove the port call @ref remove_port.
        If an authority did not
        previously exist it is added by prepending
        a double slash ("//") at the beginning of
        the URL or after the scheme if a scheme is
        present.

        @par Postconditions
        @code
        this->has_port() == true && this->port_number() == n && this->port() == std::to_string(n)
        @endcode

        @par BNF
        @code
        port          = *DIGIT
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.3">
            3.2.3. Port (rfc3986)</a>

        @param s The port string to set.

        @throw std::invalid_argument `s` is not a valid port string.
    */
    BOOST_URL_DECL
    url&
    set_port(string_view s);

    //--------------------------------------------

    /** Remove the authority

        The full authority component is removed
        if present, which includes the leading
        double slashes ("//"), the userinfo,
        the host, and the port.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2">
            3.2. Authority (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    remove_authority() noexcept;

    /** Set the authority

        This function sets the authority component
        to the specified encoded string. If a
        component was present it is replaced.
        Otherwise, the authority is added
        including leading double slashes ("//").

        The encoded string must be a valid
        authority or else an exception is thrown.

        @par BNF
        @code
        authority     = [ userinfo "@" ] host [ ":" port ]

        userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
        host          = IP-literal / IPv4address / reg-name
        port          = *DIGIT
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2">
            3.2. Authority (rfc3986)</a>

        @param s The authority string to set.

        @throw std::invalid_argument `s` is not a valid authority.
    */
    BOOST_URL_DECL
    url&
    set_encoded_authority(
        string_view s);

    //--------------------------------------------

    /** Remove the origin component

        The origin consists of the everything from the
        beginning of the URL up to but not including
        the path.

        @par Exception Safety
        Throws nothing.
    */
    BOOST_URL_DECL
    url&
    remove_origin() noexcept;

    //--------------------------------------------
    //
    // Path
    //
    //--------------------------------------------

private:
    pos_t
    segment(
        std::size_t i) const noexcept;

    char*
    edit_segments(
        std::size_t i0,
        std::size_t i1,
        std::size_t n,
        std::size_t nseg);

    BOOST_URL_DECL
    void
    edit_segments(
        std::size_t i0,
        std::size_t i1,
        detail::any_path_iter&& it0,
        detail::any_path_iter&& it1,
        int abs_hint = -1);
public:

    /** Set whether the path is absolute.

        This modifies the path as needed to
        make it absolute or relative.

        @return true on success.
    */
    BOOST_URL_DECL
    bool
    set_path_absolute(bool absolute);

    /** Set the path.

        This function validates the given percent-encoded
        path according to the allowed grammar based
        on the existing contents of the URL. If the
        path is valid, the old path is replaced with
        the new path. Otherwise, an exception is
        thrown. The requirements for `s` are thus:

        @li If `s` matches <em>path-empty</em>, that is
        if `s.empty() == true`, the path is valid. Else,

        @li If an authority is present (@ref has_authority
        returns `true`), the path syntax must match
        <em>path-abempty</em>. Else, if there is no
        authority then:

        @li If the new path starts with a forward
        slash ('/'), the path syntax must match
        <em>path-absolute</em>. Else, if the
        path is rootless (does not start with '/'),
        then:

        @li If a scheme is present, the path syntax
        must match <em>path-rootless</em>, otherwise

        @li The path syntax must match <em>path-noscheme</em>.

        @par BNF
        @code
        path-abempty  = *( "/" segment )
        path-absolute = "/" [ segment-nz *( "/" segment ) ]
        path-noscheme = segment-nz-nc *( "/" segment )
        path-rootless = segment-nz *( "/" segment )
        path-empty    = 0<pchar>
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.

        @throws std::invalid_argument invalid path.

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3"
            >3.3. Path (rfc3986)</a>
    */
    BOOST_URL_DECL
    url&
    set_encoded_path(
        string_view s);

    BOOST_URL_DECL
    url&
    set_path(
        string_view s);

    BOOST_URL_DECL
    segments_encoded
    encoded_segments() noexcept;

    template<class Allocator =
        std::allocator<char>>
    urls::segments
    segments(Allocator const& = {}) noexcept;

    //--------------------------------------------
    //
    // Query
    //
    //--------------------------------------------

private:
    struct raw_param
    {
        std::size_t pos;
        std::size_t nk;
        std::size_t nv;
    };

    raw_param
    param(
        std::size_t i) const noexcept;

    char*
    edit_params(
        std::size_t i0,
        std::size_t i1,
        std::size_t n,
        std::size_t nparam);

    BOOST_URL_DECL
    void
    edit_params(
        std::size_t i0,
        std::size_t i1,
        detail::any_query_iter&& it0,
        detail::any_query_iter&& it1,
        bool set_hint = false);
public:

    /** Remove the query.

        If a query is present (@ref has_query
        returns `true`), then the query is
        removed including the leading `?`.

        @par Exception Safety
        Throws nothing.

        @see
            @ref has_query,
            @ref set_encoded_query,
            @ref set_query.
    */
    BOOST_URL_DECL
    url&
    remove_query() noexcept;

    /** Set the query.

        Sets the query of the URL to the specified
        encoded string. The string must contain a
        valid percent-encoding or else an exception
        is thrown. When this function returns,
        @ref has_query will return `true`.

        @par BNF
        @code
        query           = *( pchar / "/" / "?" )
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.

        @throws std::invalid_argument bad encoding.

        @see
            @ref has_query,
            @ref remove_query,
            @ref set_query.
    */
    BOOST_URL_DECL
    url&
    set_encoded_query(
        string_view s);

    /** Set the query.

        Sets the query of the URL to the specified
        plain string. Any reserved characters in the
        string will be automatically percent-encoded.
        When this function returns, @ref has_query
        will return `true`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set. This string may
        contain any characters, including nulls.

        @see
            @ref has_query,
            @ref remove_query,
            @ref set_encoded_query.
    */
    BOOST_URL_DECL
    url&
    set_query(
        string_view s);

    /** Return the query parameters as a random-access range of percent-encoded strings.
    */
    urls::params_encoded
    encoded_params() noexcept
    {
        return urls::params_encoded(*this);
    }

    /** Return the query parameters as a random-access range of strings with percent-decoding applied.
    */
    template<class Allocator =
        std::allocator<char>>
    urls::params
    params(Allocator const& a = {})
    {
        return urls::params(*this, a);
    }

    //--------------------------------------------
    //
    // Fragment
    //
    //--------------------------------------------

private:
    char* set_fragment_impl(std::size_t n);
public:

    /** Remove the fragment.

        If a fragment is present (@ref has_fragment
        returns `true`), then the fragment is
        removed including the leading `#`.

        @par Exception Safety
        Throws nothing.

        @see
            @ref has_fragment,
            @ref set_encoded_fragment,
            @ref set_fragment.
    */
    BOOST_URL_DECL
    url&
    remove_fragment() noexcept;

    /** Set the fragment.

        Sets the fragment of the URL to the specified
        encoded string. The string must contain a
        valid percent-encoding or else an exception
        is thrown. When this function returns,
        @ref has_fragment will return `true`.

        @par BNF
        @code
        fragment      = *( pchar / "/" / "?" )
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set.

        @throws std::invalid_argument bad encoding.

        @see
            @ref has_fragment,
            @ref remove_fragment,
            @ref set_fragment.
    */
    BOOST_URL_DECL
    url&
    set_encoded_fragment(
        string_view s);

    /** Set the fragment.

        Sets the fragment of the URL to the specified
        plain string. Any reserved characters in the
        string will be automatically percent-encoded.
        When this function returns, @ref has_fragment
        will return `true`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param s The string to set. This string may
        contain any characters, including nulls.

        @see
            @ref has_fragment,
            @ref remove_fragment,
            @ref set_encoded_fragment.
    */
    BOOST_URL_DECL
    url&
    set_fragment(
        string_view s);

    //--------------------------------------------
    //
    // Normalization
    //
    //--------------------------------------------

    /** Normalize everything.
    */
    BOOST_URL_DECL
    url&
    normalize();

    //--------------------------------------------
    //
    // Resolution
    //
    //--------------------------------------------

    friend
    inline
    void
    resolve(
        url_view const& base,
        url_view const& ref,
        url& dest,
        error_code& ec);

private:
    //--------------------------------------------
    //
    // implementation
    //
    //--------------------------------------------

    friend class static_url_base;

    void check_invariants() const noexcept;
    void build_tab() noexcept;

    void
    ensure_space(
        std::size_t nchar,
        std::size_t nseg,
        std::size_t nparam);

    char*
    resize_impl(
        int id,
        std::size_t new_size);

    char*
    resize_impl(
        int first,
        int last,
        std::size_t new_size);

    BOOST_URL_DECL
    bool
    resolve(
        url_view const& base,
        url_view const& ref,
        error_code& ec);
};

//----------------------------------------------------------

/** Resolve a URL reference against a base URL

    This function attempts to resolve a URL
    reference `ref` against the base URL `base`
    which must satisfy the <em>absolute-URI</em>
    grammar.
    This process is defined in detail in
    rfc3986 (see below).
    The result of the resolution is placed
    into `dest`.
    If an error occurs, the contents of
    `dest` is unspecified.

    @par BNF
    @code
    absolute-URI  = scheme ":" hier-part [ "?" query ]
    @endcode

    @par Exception Safety
    Basic guarantee.
    Calls to allocate may throw.

    @param base The base URL to resolve against.

    @param ref The URL reference to resolve.

    @param dest The container where the result
    is written, upon success.

    @param ec Set to the error if any occurred.

    @par Specification
    <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-5"
        >5. Reference Resolution (rfc3986)</a>

    @see
        @ref url,
        @ref url_view.
*/
inline
void
resolve(
    url_view const& base,
    url_view const& ref,
    url& dest,
    error_code& ec)
{
    BOOST_ASSERT(&dest != &base);
    BOOST_ASSERT(&dest != &ref);
    dest.resolve(base, ref, ec);
}

/** Format the encoded url to the output stream
*/
BOOST_URL_DECL
std::ostream&
operator<<(std::ostream& os, url const& u);

} // urls
} // boost

#include <boost/url/impl/params.hpp>
#include <boost/url/impl/params_encoded.hpp>
#include <boost/url/impl/segments.hpp>
#include <boost/url/impl/segments_encoded.hpp>
#include <boost/url/impl/url.hpp>

#endif
