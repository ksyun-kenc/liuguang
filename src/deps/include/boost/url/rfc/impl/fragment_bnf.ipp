//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_FRAGMENT_BNF_IPP
#define BOOST_URL_IMPL_FRAGMENT_BNF_IPP

#include <boost/url/rfc/fragment_bnf.hpp>
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
    fragment_bnf const& t)
{
    using bnf::parse;
    return parse(it, end, ec,
        pct_encoded_bnf(
        fragment_chars, t.v));
}

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    fragment_part_bnf& t)
{
    using bnf::parse;
    if( it == end ||
        *it != '#')
    {
        t.has_fragment = false;
        ec = {};
        return true;
    }
    auto start = it;
    ++it;
    if(! parse(it, end, ec, 
        fragment_bnf{t.fragment}))
        return false;
    t.has_fragment = true;
    t.fragment_part = string_view(
        start, it - start);
    return true;
}

} // urls
} // boost

#endif
