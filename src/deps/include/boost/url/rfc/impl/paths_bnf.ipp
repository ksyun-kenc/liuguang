//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_IMPL_PATHS_BNF_IPP
#define BOOST_URL_RFC_IMPL_PATHS_BNF_IPP

#include <boost/url/rfc/paths_bnf.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/charsets.hpp>
#include <boost/url/rfc/pct_encoded_bnf.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    segment_bnf const& t)
{
    using bnf::parse;
    return parse(it, end, ec,
        pct_encoded_bnf(pchars, t.v));
}

//------------------------------------------------

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    segment_nz_bnf const& t)
{
    using bnf::parse;
    auto const start = it;
    if(! parse(it, end, ec,
        pct_encoded_bnf(pchars, t.v)))
        return false;
    if(it == start)
    {
        // can't be empty
        ec = BOOST_URL_ERR(
            error::empty_path_segment);
        return false;
    }
    return true;
}

//------------------------------------------------

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    segment_nz_nc_bnf const& t)
{
    using bnf::parse;
    auto const start = it;
    static constexpr auto cs =
        pchars - ':';
    if(! parse(it, end, ec,
        pct_encoded_bnf(cs, t.v)))
        return false;
    if(it == start)
    {
        // can't be empty
        ec = BOOST_URL_ERR(
            error::empty_path_segment);
        return false;
    }
    return true;
}

//------------------------------------------------

bool
path_abempty_bnf::
begin(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    return increment(
        it, end, ec, t);
}

bool
path_abempty_bnf::
increment(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    auto const start = it;
    if(parse(it, end, ec,
        '/', segment_bnf{t}))
        return true;
    ec = BOOST_URL_ERR(
        error::end);
    it = start;
    return false;
}

//------------------------------------------------

bool
path_absolute_bnf::
begin(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    if(it == end)
    {
        // expected '/'
        ec = BOOST_URL_ERR(
            error::missing_path_segment);
        return false;
    }
    if(*it != '/')
    {
        // expected '/'
        ec = BOOST_URL_ERR(
            error::missing_path_separator);
        return false;
    }
    ++it;
    if(it == end)
        return true;
    if(*it == '/')
    {
        // can't begin with "//"
        ec = BOOST_URL_ERR(
            error::empty_path_segment);
        return false;
    }
    return parse(it, end, ec,
        segment_bnf{t});
}

bool
path_absolute_bnf::
increment(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    auto const start = it;
    if(parse(it, end, ec,
        '/', segment_bnf{t}))
        return true;
    ec = BOOST_URL_ERR(
        error::end);
    it = start;
    return false;
}

//------------------------------------------------

bool
path_noscheme_bnf::
begin(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    if(parse(it, end, ec,
        segment_nz_nc_bnf{t}))
        return true;
    // bad segment-nz-nc
    ec = BOOST_URL_ERR(
        error::bad_schemeless_path_segment);
    return false;
}

bool
path_noscheme_bnf::
increment(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    auto const start = it;
    if(parse(it, end, ec,
        '/', segment_bnf{t}))
        return true;
    ec = BOOST_URL_ERR(
        error::end);
    it = start;
    return false;
}


//------------------------------------------------

bool
path_rootless_bnf::
begin(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    return parse(it, end, ec,
        segment_nz_bnf{t});
}

bool
path_rootless_bnf::
increment(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_str& t) noexcept
{
    using bnf::parse;
    auto const start = it;
    if(parse(it, end, ec,
        '/', segment_bnf{t}))
        return true;
    ec = BOOST_URL_ERR(
        error::end);
    it = start;
    return false;
}

} // urls
} // boost

#endif
