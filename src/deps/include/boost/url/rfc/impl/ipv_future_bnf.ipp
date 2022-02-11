//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_IPV_FUTURE_BNF_IPP
#define BOOST_URL_IMPL_IPV_FUTURE_BNF_IPP

#include <boost/url/rfc/ipv_future_bnf.hpp>
#include <boost/url/bnf/charset.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/bnf/token.hpp>
#include <boost/url/rfc/charsets.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    ipv_future_bnf& t)
{
    using namespace bnf;
    auto const start = it;
    static constexpr auto cs =
        unreserved_chars +
        subdelim_chars + ':';
    if(! parse(it, end, ec,
        'v',
        bnf::token(
            hexdig_chars, t.major),
        '.',
        bnf::token(cs, t.minor)))
        return false;
    if(t.major.empty())
    {
        // can't be empty
        ec = BOOST_URL_ERR(
            error::bad_empty_element);
        return false;
    }
    if(t.minor.empty())
    {
        // can't be empty
        ec = BOOST_URL_ERR(
            error::bad_empty_element);
        return false;
    }
    t.str = string_view(
        start, it - start);
    return true;
}

} // urls
} // boost

#endif
