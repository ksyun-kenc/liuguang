//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_ENCODE_HPP
#define BOOST_URL_ENCODE_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/encode_opts.hpp>
#include <boost/url/string_view.hpp>
#include <boost/url/grammar/all_chars.hpp>

namespace boost {
namespace urls {

/** Return the number of bytes needed to store a string with percent-encoding applied

    This function examines the characters in
    the string to determine the number of bytes
    necessary if the string were to be percent
    encoded using the given options and character
    set. No encoding is actually performed.

    @par Example 1
    Find the number of bytes needed to encode a string without transforming
    ' ' to '+'.
    @code
    encode_opts opt;
    opt.space_to_plus = false;
    std::size_t n = encoded_size( "My Stuff", pchars, opt );

    assert( n == 10 );
    @endcode

    @par Example 2
    Find the number of bytes needed to encode a string when transforming
    ' ' to '+'.
    @code
    encode_opts opt;
    opt.space_to_plus = true;
    std::size_t n = encoded_size( "My Stuff", opt, pchars );

    assert( n == 8 );
    @endcode

    @par Exception Safety
    Throws nothing.

    @return The number of bytes needed,
    excluding any null terminator.

    @param s The string to encode.

    @param opt The options for encoding. If
    this parameter is omitted, the default
    options will be used.

    @param allowed The set of characters
    allowed to appear unescaped.
    This type must satisfy the requirements
    of <em>CharSet</em>. If this parameter is
    omitted, then no characters are considered
    special. The character set is ignored if
    `opt.non_normal_is_error == false`.

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-2.1"
        >2.1. Percent-Encoding (rfc3986)</a>

    @see
        @ref decode,
        @ref encode,
        @ref encode_opts.
*/
template <class CharSet = grammar::all_chars_t>
std::size_t
encoded_size(
    string_view s,
    encode_opts const& opt = {},
    CharSet const& allowed = {}) noexcept;

/** Write a string with percent-encoding into a buffer.

    This function applies percent-encoding to
    the given plain string, by escaping all
    characters that are not in the specified
    <em>CharSet</em>.
    The output is written to the destination,
    and will be truncated if there is
    insufficient space.

    @par Example
    @code
    char *dest = new char[MAX_LENGTH];
    std::size_t encoded_size = encode( dest, dest + MAX_LENGTH,
            "Program Files", encode_opts{}, pchars );

    assert( encoded_size == 15 );
    assert( strncmp( "Program%20Files", dest, encoded_size ) == 0 );
    @endcode

    @par Exception Safety
    Throws nothing.

    @return `true` if the output was large
    enough to hold the entire result.

    @param[in, out] dest A pointer to the
    beginning of the output buffer. Upon
    return, the argument will be changed
    to one past the last character written.

    @param end A pointer to one past the end
    of the output buffer.

    @param s The string to encode.

    @param opt The options for encoding. If
    this parameter is omitted, the default
    options will be used.

    @param allowed The set of characters
    allowed to appear unescaped.
    This type must satisfy the requirements
    of <em>CharSet</em>. If this parameter is
    omitted, then no characters are considered
    special. The character set is ignored if
    `opt.non_normal_is_error == false`.

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-2.1"
        >2.1. Percent-Encoding (rfc3986)</a>

    @see
        @ref decode,
        @ref encode,
        @ref encoded_size.
*/
template<
    class CharSet =
        grammar::all_chars_t>
std::size_t
encode(
    char* dest,
    char const* end,
    string_view s,
    encode_opts const& opt = {},
    CharSet const& allowed = {});

/** Return a string with percent-encoding applied

    This function applies percent-encoding to
    the given plain string, by escaping all
    characters that are not in the specified
    <em>CharSet</em>.
    The result is returned as a
    `std::basic_string`, using the optionally
    specified allocator.

    @par Example
    @code
    encode_opts opt;
    opt.space_to_plus = true;
    std::string s = encode( "My Stuff", opt, pchars );

    assert( s == "My+Stuff" );
    @endcode

    @par Exception Safety
    Calls to allocate may throw.

    @return A `std::basic_string` holding the
    encoded string, using the specified
    allocator.

    @param s The string to encode.

    @param allowed The set of characters
    allowed to appear unescaped.
    This type must satisfy the requirements
    of <em>CharSet</em>. If this parameter is
    omitted, then no characters are considered
    special. The character set is ignored if
    `opt.non_normal_is_error == false`.

    @param opt The options for encoding. If
    this parameter is omitted, the default
    options will be used.

    @param a An optional allocator the returned
    string will use. If this parameter is omitted,
    the default allocator is used. In this case
    the return type will be `std::string`.

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-2.1"
        >2.1. Percent-Encoding (rfc3986)</a>

    @see
        @ref encode,
        @ref encoded_size,
        @ref encode_opts,
*/
template<
    class CharSet = grammar::all_chars_t,
    class Allocator =
        std::allocator<char> >
std::basic_string<char,
    std::char_traits<char>,
        Allocator>
encode_to_string(
    string_view s,
    encode_opts const& opt = {},
    CharSet const& allowed = {},
    Allocator const& a = {});

} // urls
} // boost

#include <boost/url/impl/encode.hpp>

#endif
