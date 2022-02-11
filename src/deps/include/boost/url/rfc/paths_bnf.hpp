//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_PATHS_BNF_HPP
#define BOOST_URL_RFC_PATHS_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/pct_encoding_types.hpp>
#include <boost/url/bnf/range.hpp>
#include <cstddef>

namespace boost {
namespace urls {

/** Information about a parsed path
*/
struct parsed_path
{
    /** The encoded string representing the path
    */
    string_view path;

    /** The number of segments in the path
    */
    std::size_t count = 0;
};

//------------------------------------------------

struct segment_bnf
{
    pct_encoded_str& v;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        segment_bnf const& t);
};

//------------------------------------------------

struct segment_nz_bnf
{
    pct_encoded_str& v;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        segment_nz_bnf const& t);
};

//------------------------------------------------

struct segment_nz_nc_bnf
{
    pct_encoded_str& v;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        segment_nz_nc_bnf const& t);
};

//------------------------------------------------

/** BNF for path-abempty grammar

    @par BNF
    @code
    path-abempty  = *( "/" segment )
    @endcode
*/
struct path_abempty_bnf : bnf::range
{
    using value_type =
        pct_encoded_str;

    path_abempty_bnf()
        : bnf::range(this)
    {
    }

    BOOST_URL_DECL
    static
    bool
    begin(
        char const*& it,
        char const* end,
        error_code& ec,
        pct_encoded_str& t) noexcept;

    BOOST_URL_DECL
    static
    bool
    increment(
        char const*& it,
        char const* end,
        error_code& ec,
        pct_encoded_str& t) noexcept;
};

//------------------------------------------------

/** BNF for path-absolute grammar.

    @par BNF
    @code
    path-absolute = "/" [ segment-nz *( "/" segment ) ]
    @endcode
*/
struct path_absolute_bnf : bnf::range
{
    using value_type =
        pct_encoded_str;

    path_absolute_bnf()
        : bnf::range(this)
    {
    }

    BOOST_URL_DECL
    static
    bool
    begin(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;

    BOOST_URL_DECL
    static
    bool
    increment(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;
};

//------------------------------------------------

/** BNF for path-noscheme grammar.

    @par BNF
    @code
    path-noscheme = segment-nz-nc *( "/" segment )
    @endcode
*/
struct path_noscheme_bnf : bnf::range
{
    using value_type =
        pct_encoded_str;

    path_noscheme_bnf()
        : bnf::range(this)
    {
    }

    BOOST_URL_DECL
    static
    bool
    begin(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;

    BOOST_URL_DECL
    static
    bool
    increment(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;
};

//------------------------------------------------

/** BNF for path-rootless grammar.

    @par BNF
    @code
    path-rootless = segment-nz *( "/" segment )
    @endcode
*/

struct path_rootless_bnf : bnf::range
{
    using value_type =
        pct_encoded_str;

    path_rootless_bnf()
        : bnf::range(this)
    {
    }

    BOOST_URL_DECL
    static
    bool
    begin(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;

    BOOST_URL_DECL
    static
    bool
    increment(
        char const*& it,
        char const* const end,
        error_code& ec,
        pct_encoded_str& t) noexcept;
};

} // urls
} // boost

#endif
