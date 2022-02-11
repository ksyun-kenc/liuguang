//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_SEGMENTS_VIEW_HPP
#define BOOST_URL_SEGMENTS_VIEW_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/string.hpp>
#include <cstddef>
#include <iosfwd>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DECL
class url_view;
class segments_encoded_view;
#endif

/** A bidirectional range of read-only path segment strings with percent-decoding applied.

    @see
        @ref segments_encoded_view.
*/
class segments_view
{
    string_view s_;
    std::size_t n_ = 0;
    string_value::allocator a_;

    friend class url_view;
    friend class segments_encoded_view;

    template<class Allocator>
    segments_view(
        string_view s,
        std::size_t n,
        Allocator const& a);

public:
#ifdef BOOST_URL_DOCS
    /** An iterator to a read-only decoded path segment.
    */
    using iterator = __see_below__;
#else
    class iterator;
#endif

    /** The type of value returned when dereferencing an iterator.
    */
    using value_type = string_value;

    /** The type of value returned when dereferencing an iterator.
    */
    using reference = string_value;

    /** The type of value returned when dereferencing an iterator.
    */
    using const_reference = string_value;

    /** The unsigned integer type used to represent size.
    */
    using size_type = std::size_t;

    /** The signed integer type used to represent differences.
    */
    using difference_type = std::ptrdiff_t;

    //--------------------------------------------
    //
    // Members
    //
    //--------------------------------------------

    /** Constructor

        Default constructed views represent an
        empty path.
    */
    inline
    segments_view() noexcept;

    /** Returns true if this contains an absolute path.

        Absolute paths always start with a
        forward slash ('/').
    */
    inline
    bool
    is_absolute() const noexcept;

    //--------------------------------------------
    //
    // Element Access
    //
    //--------------------------------------------

    /** Return the first element.
    */
    inline
    string_value
    front() const noexcept;

    /** Return the last element.
    */
    inline
    string_value
    back() const noexcept;

    //--------------------------------------------
    //
    // Iterators
    //
    //--------------------------------------------

    /** Return an iterator to the beginning.
    */
    BOOST_URL_DECL
    iterator
    begin() const noexcept;

    /** Return an iterator to the end.
    */
    BOOST_URL_DECL
    iterator
    end() const noexcept;

    //--------------------------------------------
    //
    // Capacity
    //
    //--------------------------------------------

    /** Return true if the range contains no elements
    */
    inline
    bool
    empty() const noexcept;

    /** Return the number of elements in the range
    */
    inline
    std::size_t
    size() const noexcept;

    //--------------------------------------------
    //
    // Friends
    //
    //--------------------------------------------

    BOOST_URL_DECL friend std::ostream&
        operator<<(std::ostream& os,
            segments_view const& pv);
};

//----------------------------------------------------------

/** Format the object to an output stream
*/
BOOST_URL_DECL
std::ostream&
operator<<(
    std::ostream& os,
    segments_view const& vw);

} // urls
} // boost

#include <boost/url/impl/segments_view.hpp>

#endif
