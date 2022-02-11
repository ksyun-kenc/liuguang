//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_SEGMENTS_ENCODED_VIEW_HPP
#define BOOST_URL_SEGMENTS_ENCODED_VIEW_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/segments_view.hpp>
#include <boost/url/string.hpp>
#include <iosfwd>
#include <utility>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DECL
class url_view;
#endif

/** A bidirectional range of read-only encoded path segment strings.

    Objects of this type represent an iterable
    range of path segments, where each segment
    is represented by a percent-encoded string.

    Dereferenced iterators return string views
    into the underlying character buffer.

    Ownership of the underlying characters is
    not transferred; the character buffer used
    to construct the container must remain
    valid for as long as the container exists.

    A view of encoded segments in a URL's path
    can be obtained by calling
        @ref url_view::encoded_segments.
    Alternatively, to obtain encoded segments
    from a path stored in a string call one of
    the parsing functions (see below).

    @par Examples

    A path string is parsed into encoded
    segments, then each segment is printed to
    standard output:

    @code
    segments_encoded_view sev = parse_path( "/path/to/file.txt" ).value();

    for( auto it = sev.begin(); it != sev.end(); ++it )
        std::cout << *it << std::endl;
    @endcode

    A URL containing a path is parsed, then a
    view to the encoded segments is obtained
    and formatted to standard output:

    @code
    url_view u = parse_uri( "http://example.com/path/to/file.txt" ).value();

    segments_encoded_view sev = u.encoded_segments();

    std::cout << sev << std::endl;
    @endcode

    @par Complexity

    Iterator increment or decrement runs in
    linear time on the size of the segment.
    All other operations run in constant time.
    No operations allocate memory.

    @see
        @ref parse_path,
        @ref parse_path_abempty,
        @ref parse_path_absolute,
        @ref parse_path_noscheme,
        @ref parse_path_rootless,
        @ref segments_view.
*/
class segments_encoded_view
{
    string_view s_;
    std::size_t n_;

    friend class url_view;

    BOOST_URL_DECL
    segments_encoded_view(
        string_view s,
        std::size_t n) noexcept;

public:
#ifdef BOOST_URL_DOCS
    /** A read-only bidirectional iterator to an encoded path segment.
    */
    using iterator = __see_below__;
#else
    class iterator;
#endif

    /** The type of value returned when dereferencing an iterator.
    */
    using value_type = string_view;

    /** The type of value returned when dereferencing an iterator.
    */
    using reference = string_view;

    /** The type of value returned when dereferencing an iterator.
    */
    using const_reference = string_view;

    /** An unsigned integer type used to represent size.
    */
    using size_type = std::size_t;

    /** A signed integer type used to represent differences.
    */
    using difference_type = std::ptrdiff_t;

    //--------------------------------------------
    //
    // Members
    //
    //--------------------------------------------

    /** Constructor

        A default-constructed instance will be
        an empty range.
    */
    inline
    segments_encoded_view() noexcept;

    /** Return a view of this container as percent-decoded segments

        This function returns a new view over the
        same underlying character buffer where each
        segment is returned as a @ref string_value
        with percent-decoding applied using the
        optionally specified allocator.

        The decoded view does not take ownership of
        the underlying character buffer; the caller
        is still responsible for ensuring that the
        buffer remains valid until all views which
        reference it are destroyed.

        @par Example
        @code
        segments_encoded_view sev = parse_path( "/%70%61%74%68/%74%6f/%66%69%6c%65%2e%74%78%74" ).value();

        segments_view sv = sev.decoded();

        std::stringstream ss;

        ss << sv.front() << "/../" << sv.back();

        assert( ss.string() == "path/../file.txt" );
        @endcode

        @par Exceptions
        Calls to allocate may throw.

        @return A view to decoded path segments.

        @param alloc The allocator the returned
        view will use for all string storage. If
        this parameter is ommitted, the default
        allocator will be used.
    */
    template<class Allocator = std::allocator<char> >
    segments_view
    decoded(Allocator const& alloc = {}) const;

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
    string_view
    front() const noexcept;

    /** Return the last element.
    */
    inline
    string_view
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

    BOOST_URL_DECL friend std::ostream&
        operator<<(std::ostream& os,
            segments_encoded_view const& pv);

    BOOST_URL_DECL friend
        result<segments_encoded_view>
        parse_path(string_view s) noexcept;

    BOOST_URL_DECL friend
        result<segments_encoded_view>
        parse_path_abempty(string_view s) noexcept;

    BOOST_URL_DECL friend
        result<segments_encoded_view>
        parse_path_absolute(string_view s) noexcept;

    BOOST_URL_DECL friend
        result<segments_encoded_view>
        parse_path_noscheme(string_view s) noexcept;

    BOOST_URL_DECL friend
        result<segments_encoded_view>
        parse_path_rootless(string_view s) noexcept;
};

//----------------------------------------------------------

/** Format the object to an output stream
*/
BOOST_URL_DECL
std::ostream&
operator<<(
    std::ostream& os,
    segments_encoded_view const& vw);

//----------------------------------------------------------

/** Parse a string and return an encoded segment view

    This function parses the string and returns the
    corresponding path object if the string is valid,
    otherwise returns an error.

    @par BNF
    @code
    path          = [ "/" ] segment *( "/" segment )
    @endcode

    @par Exception Safety
    No-throw guarantee.

    @return A valid view on success, otherwise an
    error code.

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3">
        3.3. Path (rfc3986)</a>

    @see
        @ref parse_path,
        @ref parse_path_absolute,
        @ref parse_path_noscheme,
        @ref parse_path_rootless.
*/
BOOST_URL_DECL
result<segments_encoded_view>
parse_path(string_view s) noexcept;

/** Parse a string and return an encoded segment view

    This function parses the string and returns the
    corresponding path object if the string is valid,
    otherwise sets the error and returns an empty range.

    @par BNF
    @code
    path-abempty  = *( "/" segment )
    @endcode

    @par Exception Safety
    No-throw guarantee.

    @return A valid view on success, otherwise an
    error code.

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3">
        3.3. Path (rfc3986)</a>

    @see
        @ref parse_path,
        @ref parse_path_absolute,
        @ref parse_path_noscheme,
        @ref parse_path_rootless.
*/
BOOST_URL_DECL
result<segments_encoded_view>
parse_path_abempty(
    string_view s) noexcept;

/** Parse a string and return an encoded segment view

    This function parses the string and returns the
    corresponding path object if the string is valid,
    otherwise sets the error and returns an empty range.

    @par BNF
    @code
    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    @endcode

    @par Exception Safety
    No-throw guarantee.

    @return A valid view on success, otherwise an
    error code.

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3">
        3.3. Path (rfc3986)</a>

    @see
        @ref parse_path,
        @ref parse_path_abempty,
        @ref parse_path_noscheme,
        @ref parse_path_rootless.
*/
BOOST_URL_DECL
result<segments_encoded_view>
parse_path_absolute(
    string_view s) noexcept;

/** Parse a string and return an encoded segment view

    This function parses the string and returns the
    corresponding path object if the string is valid,
    otherwise sets the error and returns an empty range.

    @par BNF
    @code
    path-noscheme = segment-nz-nc *( "/" segment )

    segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
                    ; non-zero-length segment without any colon ":"
    @endcode

    @par Exception Safety
    No-throw guarantee.

    @return A valid view on success, otherwise an
    error code.

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3">
        3.3. Path (rfc3986)</a>

    @see
        @ref parse_path,
        @ref parse_path_abempty,
        @ref parse_path_absolute,
        @ref parse_path_rootless.
*/
BOOST_URL_DECL
result<segments_encoded_view>
parse_path_noscheme(
    string_view s) noexcept;

/** Parse a string and return an encoded segment view

    This function parses the string and returns the
    corresponding path object if the string is valid,
    otherwise sets the error and returns an empty range.

    @par BNF
    @code
    path-rootless = segment-nz *( "/" segment )

    segment-nz    = 1*pchar
    @endcode

    @par Exception Safety
    No-throw guarantee.

    @return A valid view on success, otherwise an
    error code.

    @param s The string to parse

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.3">
        3.3. Path (rfc3986)</a>

    @see
        @ref parse_path,
        @ref parse_path_abempty,
        @ref parse_path_absolute,
        @ref parse_path_noscheme.
*/
BOOST_URL_DECL
result<segments_encoded_view>
parse_path_rootless(
    string_view s) noexcept;

} // urls
} // boost

#include <boost/url/impl/segments_encoded_view.hpp>

#endif
