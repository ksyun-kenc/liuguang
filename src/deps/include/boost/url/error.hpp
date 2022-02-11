//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_ERROR_HPP
#define BOOST_URL_ERROR_HPP

#include <boost/url/detail/config.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>
#include <boost/system/system_error.hpp>
#include <stdexcept>

namespace boost {
namespace urls {

/// The type of error category used by the library
using error_category = boost::system::error_category;

/// The type of error code used by the library
using error_code = boost::system::error_code;

/// The type of error condition used by the library
using error_condition = boost::system::error_condition;

/// The type of system error thrown by the library
using system_error = boost::system::system_error;

/** The type of result returned by library functions

    This is an alias template used as the return type
    for functions that can either return a container,
    or fail with an error code. This is a brief
    synopsis of the type:

    @par Declaration
    @code
    template< class T >
    class result
    {
    public:
        // Return true if the result contains an error
        constexpr bool has_error() const noexcept;

        // These return true if the result contains a value
        constexpr bool has_value() const noexcept;
        constexpr explicit operator bool() const noexcept;

        // Return the value or throw an exception if has_value()==false
        constexpr T& value();
        constexpr T& operator*();
        constexpr T const& value() const;
        constexpr T const& operator*() const;

        // Return the error, which is default constructed if has_error()==false
        constexpr error_code error() const noexcept;

        ...more
    @endcode

    @par Usage
    Given the function @ref parse_uri with this signature:
    @code
    result< url_view > parse_uri( string_view s ) noexcept;
    @endcode

    The following statement captures the value in a
    variable upon success, otherwise throws:
    @code
    url_view u = parse_uri( "http://example.com/path/to/file.txt" ).value();
    @endcode

    This statement captures the result in a local
    variable and inspects the error condition:
    @code
    result< url_view > r = parse_uri( "http://example.com/path/to/file.txt" );

    if( r.has_error() )
        std::cout << r.error();
    else
        std::cout << r.value();
    @endcode

    @note For a full synopsis of the type, please see
    the corresponding documentation in Boost.System.

    @tparam T The type of value held by the result.
*/
template<class T>
using result = boost::system::result<T, error_code>;

//----------------------------------------------------------

/** Error codes returned the library
*/
enum class error
{
    // VFALCO 3 space indent or
    // else Doxygen malfunctions

    /**
     * The operation completed successfully.
    */
    success = 0,

    /**
     * An unspecified syntax error was found.
    */
    syntax,

    /**
     * Bad alphabetic character.
    */
    bad_alpha,

    /**
     * Character is not a digit.
    */
    bad_digit,

    /**
     * A required element was empty.
    */
    bad_empty_element,

    /**
     * Bad HEXDIG
    */
    bad_hexdig,

    /**
     * Syntax error in IPv6 address.
    */
    bad_ipv6,

    /**
     * Bad leading zero in number.
    */
    bad_leading_zero,

    /**
     * The octet is out of range.
    */
    bad_octet,

    /**
     * Bad schemeless path segment.
    */
    bad_schemeless_path_segment,

    /** Bad empty element.
    */
    empty,

    /**
     * Illegal empty path segment.
    */
    empty_path_segment,

    /**
     * A list parser reached the end.
    */
    end,

    /**
     * Null encountered in pct-encoded.
    */
    illegal_null,

    /**
     * Illegal reserved character in encoded string.
    */
    illegal_reserved_char,

    /**
     * Incomplete input for grammar.

       This happens when the end of the input
       string is reached without fully matching
       the grammar.
    */
    incomplete,

    /**
     * Validation failed
    */
    invalid,

    /**
     * Leftover input remaining after match.
    */
    leftover_input,

    /**
     * Missing character literal.
    */
    missing_char_literal,

    /**
     * Missing path segment.
    */
    missing_path_segment,

    /**
     * A slash '/' was expected in the path segment.
    */
    missing_path_separator,

    /**
     * Missing words in IPv6 address.
    */
    missing_words,

    /**
     * A grammar element was not in canonical form.
    */
    non_canonical,

    /**
     * Wrong character literal.
    */
    wrong_char_literal,

    //--------------------------------------------

    /**
     * Bad hexadecimal digit.

       This error condition is fatal.
    */
    bad_pct_hexdig,

    /**
     * The percent-encoded sequence is incomplete.

       This error condition is fatal.
    */
    incomplete_pct_encoding,

    /**
     * Missing hexadecimal digit.

       This error condition is fatal.
    */
    missing_pct_hexdig,

    /**
     * No space in output buffer

       This error is returned when a provided
       output buffer was too small to hold
       the complete result of an algorithm.
    */
    no_space,

    /**
     * The URL is not a base URL
    */
    not_a_base
};

//------------------------------------------------

/** Error conditions returned by the library.
*/
enum class condition
{
    /**
     * A fatal error in syntax was encountered.

       This indicates that parsing cannot continue.
    */
    fatal = 1
};

} // urls
} // boost

#include <boost/url/impl/error.hpp>

#endif
