//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_USERINFO_BNF_IPP
#define BOOST_URL_IMPL_USERINFO_BNF_IPP

#include <boost/url/rfc/userinfo_bnf.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/url/rfc/charsets.hpp>
#include <boost/url/rfc/pct_encoded_bnf.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    userinfo_bnf& t)
{
    using bnf::parse;
    auto start = it;
    {
        static constexpr auto cs =
            unreserved_chars +
            subdelim_chars;
        if(! parse(it, end, ec,
            pct_encoded_bnf(cs, t.user)))
            return false;
    }
    t.user_part = string_view(
        start, it - start);
    start = it;
    if( it != end &&
        *it == ':')
    {
        ++it;
        static constexpr auto cs =
            unreserved_chars +
            subdelim_chars + ':';
        if(! parse(it, end, ec,
            pct_encoded_bnf(cs,
                t.password)))
            return false;
        t.has_password = true;
        t.password_part = string_view(
            start, it - start);
    }
    else
    {
        t.has_password = false;
        t.password_part = {};
    }
    return true;
}

} // urls
} // boost

#endif
