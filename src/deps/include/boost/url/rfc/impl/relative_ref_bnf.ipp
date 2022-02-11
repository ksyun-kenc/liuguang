//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_RELATIVE_REF_BNF_IPP
#define BOOST_URL_IMPL_RELATIVE_REF_BNF_IPP

#include <boost/url/rfc/relative_ref_bnf.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/fragment_bnf.hpp>
#include <boost/url/rfc/query_bnf.hpp>
#include <boost/url/rfc/relative_part_bnf.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    relative_ref_bnf& t)
{
    using bnf::parse;

    // relative-part
    if(! parse(it, end, ec,
            t.relative_part))
        return false;

    // [ "?" query ]
    if(! parse(it, end, ec,
            t.query_part))
        return false;

    // [ "#" fragment ]
    if(! parse(it, end, ec,
            t.fragment_part))
        return false;

    return true;
}

} // urls
} // boost

#endif
