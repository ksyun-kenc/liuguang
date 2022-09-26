//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_STATIC_URL_HPP
#define BOOST_URL_STATIC_URL_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/url_base.hpp>
#include <boost/align/align_up.hpp>
#include <boost/static_assert.hpp>
#include <cstddef>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
template<std::size_t Capacity>
class static_url;
#endif

// VFALCO This class is for reducing
// the number of template instantiations,
// and keep definitions in the library

/** Common implementation for all static URLs

    This base class is used by the library
    to provide common functionality for
    static URLs. Users should not use this
    class directly. Instead, construct an
    instance of one of the containers
    or call a parsing function.

    @par Containers
        @li @ref url
        @li @ref url_view
        @li @ref static_url

    @par Parsing Functions
        @li @ref parse_absolute_uri
        @li @ref parse_origin_form
        @li @ref parse_relative_ref
        @li @ref parse_uri
        @li @ref parse_uri_reference
*/
class BOOST_SYMBOL_VISIBLE
    static_url_base
    : public url_base
{
    template<std::size_t>
    friend class static_url;

    ~static_url_base() = default;
    BOOST_URL_DECL static_url_base(
        char* buf, std::size_t cap) noexcept;
    BOOST_URL_DECL static_url_base(
        char* buf, std::size_t cap, string_view s);
    BOOST_URL_DECL void clear_impl() noexcept override;
    BOOST_URL_DECL void reserve_impl(std::size_t, op_t&) override;
    BOOST_URL_DECL void cleanup(op_t&) override;

    void
    copy(url_view_base const& u)
    {
        this->url_base::copy(u);
    }

};

//------------------------------------------------

/** A URL with fixed-capacity storage

    This container acts like @ref url,
    except that dynamic allocations are
    never performed. Instead, the capacity
    for the data comes from inline storage.

    @par Example
    @code
    static_url< 1024 > u( "https://www.example.com" );
    @endcode

    @par Invariants
    @code
    this->capacity() == Capacity
    @endcode

    @tparam Capacity The maximum capacity
    in characters, not including the
    null terminator.

    @see
        @ref url,
        @ref url_view.
*/
template<std::size_t Capacity>
class static_url
    : public static_url_base
{
    char buf_[Capacity + 1];

    friend std::hash<static_url>;
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
    ~static_url() = default;

    /** Constructor

        Default constructed urls contain
        a zero-length string. This matches
        the grammar for a relative-ref with
        an empty path and no query or
        fragment.

        @par Example
        @code
        static_url< 1024 > u;
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
    static_url() noexcept
        : static_url_base(
            buf_, sizeof(buf_))
    {
    }

    /** Constructor

        This function constructs a url from
        the string `s`, which must contain a
        valid <em>URI</em> or <em>relative-ref</em>
        or else an exception is thrown.
        The new url retains ownership by
        making a copy of the passed string.

        @par Example
        @code
        static_url< 1024 > u( "https://www.example.com" );
        @endcode

        @par Postconditions
        @code
        this->buffer().data() != s.data()
        @endcode

        @par Complexity
        Linear in `s.size()`.

        @par Exception Safety
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
    explicit
    static_url(
        string_view s)
        : static_url_base(
            buf_, sizeof(buf_), s)
    {
    }

    /** Constructor

        The newly constructed object will
        contain a copy of `u`.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer.data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Exception thrown if maximum size exceeded.

        @param u The url to copy.
    */
    static_url(
        static_url const& u) noexcept
        : static_url()
    {
        copy(u);
    }

    /** Constructor

        The newly constructed object will
        contain a copy of `u`.

        @par Postconditions
        @code
        this->buffer() == u.buffer() && this->buffer.data() != u.buffer().data()
        @endcode

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Exception thrown if capacity exceeded.

        @throw system_error
        Capacity would be exceeded.

        @param u The url to copy.
    */
    static_url(
        url_view_base const& u)
        : static_url()
    {
        copy(u);
    }

    /** Assignment

        The contents of `u` are copied and
        the previous contents of `this` are
        destroyed.

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Exception thrown if capacity exceeded.

        @throw system_error
        Capacity would be exceeded.

        @param u The url to copy.
    */
    static_url&
    operator=(
        static_url const& u)
    {
        if (this != &u)
            copy(u);
        return *this;
    }

    /** Assignment

        The contents of `u` are copied and
        the previous contents of `this` are
        destroyed.

        @par Complexity
        Linear in `u.size()`.

        @par Exception Safety
        Strong guarantee.
        Exception thrown if capacity exceeded.

        @throw system_error
        Capacity would be exceeded.

        @param u The url to copy.
    */
    static_url&
    operator=(
        url_view_base const& u)
    {
        copy(u);
        return *this;
    }
};

} // urls
} // boost

//------------------------------------------------

// std::hash specialization
#ifndef BOOST_URL_DOCS
namespace std {
template<std::size_t N>
struct hash< ::boost::urls::static_url<N> >
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
    operator()(::boost::urls::static_url<N> const& u) const noexcept
    {
        return u.digest(salt_);
    }

private:
    std::size_t salt_ = 0;
};
} // std
#endif

#endif
