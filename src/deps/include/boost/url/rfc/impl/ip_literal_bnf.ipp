//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_IP_LITERAL_BNF_IPP
#define BOOST_URL_IMPL_IP_LITERAL_BNF_IPP

#include <boost/url/rfc/ip_literal_bnf.hpp>
#include <boost/url/ipv6_address.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/ipv_future_bnf.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    ip_literal_bnf& t)
{
    using bnf::parse;
    // '['
    if(! parse(it, end, ec, '['))
        return false;
    if(it == end)
    {
        // expected address
        ec = BOOST_URL_ERR(
            error::incomplete);
        return false;
    }
    if(*it != 'v')
    {
        // IPv6address
        if(! parse(it, end, ec,
                t.ipv6, ']'))
            return false;
        t.is_ipv6 = true;
    }
    else
    {
        // IPvFuture
        ipv_future_bnf p;
        if(! parse(it, end, ec,
                p, ']'))
            return false;
        t.is_ipv6 = false;
        t.ipvfuture = p.str;
    }
    return true;
}

} // urls
} // boost

#endif
