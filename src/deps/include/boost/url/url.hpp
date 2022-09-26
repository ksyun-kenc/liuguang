//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_URL_HPP
#define BOOST_URL_URL_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/url_base.hpp>
#include <boost/assert.hpp>
#include <utility>

namespace boost {
namespace urls {

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
    : public url_base
{
    friend std::hash<url>;

    using url_view_base::digest;

public:
    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Destructor

        Any params, segments, or iterators which
        reference this object are invalidated.
        The underlying character buffer is
        destroyed, invalidating all references
        to it.
    */
    BOOST_URL_DECL
    virtual ~url();

    /** Constructor

        Default constructed urls contain
        a zero-length string. This matches
        the grammar for a relative-ref with
        an empty path and no query or
        fragment.

        @par Example
        @code
        url u;
        @endcode

        @par Postconditions
        @code
        this->empty() == true
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @par BNF
        @code
        relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        @endcode

        @par Specification
        <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-4.2"
            >4.2. Relative Reference (rfc3986)</a>
    */
    BOOST_URL_DECL
    url() noexcept;

    /** Constructor

        This function constructs a URL from
        the string `s`, which must contain a
        valid <em>URI</em> or <em>relative-ref</em>
        or else an exception is thrown.
        The new url retains ownership by
        allocating a copy of the passed string.

        @par Example
        @code
        url u( "https://www.example.com" );
        @endcode

        @par Postconditions
        @code
        this->buffer().data() != s.data()
        @endcode

        @par Complexity
        Linear in `s.size()`.

        @par Exception Safety
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @throw system_error
        The input does not contain a valid url.

        @param s The string to parse.

        @par BNF
        @code
        URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

        relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        @endcode

        @par Specification
        @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-4.1"
            >4.1. URI Reference</a>
    */
    BOOST_URL_DECL
    explicit
    url(string_view s);

    /** Constructor

        The contents of `u` are transferred
        to the newly constructed object,
        which includes the underlying
        character buffer.
        After construction, the moved-from
        object will be as if default constructed.

        @par Postconditions
        @code
        u.empty() == true
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @param u The url to move from.
    */
    BOOST_URL_DECL
    url(url&& u) noexcept;

    /** Constructor

        The newly constructed object will
        contain a copy of `u`.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer().data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @throw std::length_error `u.size() > max_size()`.

        @param u The url to copy.
    */
    url(url_view_base const& u)
    {
        copy(u);
    }

    /** Constructor

        The newly constructed object will
        contain a copy of `u`.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer().data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @throw std::length_error `u.size() > max_size()`.

        @param u The url to copy.
    */
    url(url const& u)
        : url(static_cast<
            url_view_base const&>(u))
    {
    }

    /** Assignment

        The contents of `u` are transferred to
        `this`, including the underlying
        character buffer. The previous contents
        of `this` are destroyed.
        After assignment, the moved-from
        object will be as if default constructed.

        @par Postconditions
        @code
        u.empty() == true
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @param u The url to assign from.
    */
    BOOST_URL_DECL
    url&
    operator=(url&& u) noexcept;

    /** Assignment

        The contents of `u` are copied and
        the previous contents of `this` are
        destroyed.
        Capacity is preserved, or increases.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer().data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @throw std::length_error `u.size() > max_size()`.

        @param u The url to copy.
    */
    url&
    operator=(
        url_view_base const& u)
    {
        copy(u);
        return *this;
    }

    /** Assignment

        The contents of `u` are copied and
        the previous contents of `this` are
        destroyed.
        Capacity is preserved, or increases.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer().data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.

        @param u The url to copy.
    */
    url&
    operator=(url const& u)
    {
        return (*this)=static_cast<
            url_view_base const&>(u);
    }

    //--------------------------------------------

    /** Swap the contents.

        Exchanges the contents of this url with another
        url. All views, iterators and references remain valid.

        If `this == &other`, this function call has no effect.

        @par Example
        @code
        url u1( "https://www.example.com" );
        url u2( "https://www.boost.org" );
        u1.swap(u2);
        assert(u1 == "https://www.boost.org" );
        assert(u2 == "https://www.example.com" );
        @endcode

        @par Complexity
        Constant

        @par Exception Safety
        Throws nothing.

        @param other The object to swap with

    */
    BOOST_URL_DECL
    void
    swap(url& other) noexcept;

    /** Swap

        Exchanges the contents of `v0` with another `v1`.
        All views, iterators and references remain
        valid.

        If `&v0 == &v1`, this function call has no effect.

        @par Example
        @code
        url u1( "https://www.example.com" );
        url u2( "https://www.boost.org" );
        std::swap(u1, u2);
        assert(u1 == "https://www.boost.org" );
        assert(u2 == "https://www.example.com" );
        @endcode

        @par Effects
        @code
        v0.swap( v1 );
        @endcode

        @par Complexity
        Constant

        @par Exception Safety
        Throws nothing

        @param v0, v1 The objects to swap

        @see
            @ref url::swap
    */
    friend
    void
    swap(url& v0, url& v1) noexcept
    {
        v0.swap(v1);
    }

private:
    char* allocate(std::size_t);
    void deallocate(char* s);

    BOOST_URL_DECL void clear_impl() noexcept override;
    BOOST_URL_DECL void reserve_impl(std::size_t, op_t&) override;
    BOOST_URL_DECL void cleanup(op_t&) override;
};

} // urls
} // boost

//------------------------------------------------

// std::hash specialization
#ifndef BOOST_URL_DOCS
namespace std {
template<>
struct hash< ::boost::urls::url >
{
    hash() = default;
    hash(hash const&) = default;
    hash& operator=(hash const&) = default;

    explicit
    hash(std::size_t salt) noexcept
        : salt_(salt)
    {
    }

    std::size_t
    operator()(::boost::urls::url const& u) const noexcept
    {
        return u.digest(salt_);
    }

private:
    std::size_t salt_ = 0;
};
} // std
#endif

#endif
