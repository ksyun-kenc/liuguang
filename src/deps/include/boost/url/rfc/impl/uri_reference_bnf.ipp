//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_URI_REFERENCE_BNF_IPP
#define BOOST_URL_IMPL_URI_REFERENCE_BNF_IPP

#include <boost/url/rfc/uri_reference_bnf.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/hier_part_bnf.hpp>
#include <boost/url/rfc/relative_part_bnf.hpp>

namespace boost {
namespace urls {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    uri_reference_bnf& t)
{
    using bnf::parse;
    auto const start = it;

    // scheme ":"
    if(! parse(it, end, ec,
        t.scheme_part))
    {
        // rewind
        it = start;
        ec = {};

        // relative-ref
        relative_part_bnf t0;
        if(! parse(it, end, ec,t0))
            return false;

        t.has_authority =
            t0.has_authority;
        t.authority = t0.authority;
        t.path = t0.path;
    }
    else
    {
        // hier-part
        hier_part_bnf t0;
        if(! parse(it, end, ec, t0))
            return false;

        t.has_authority =
            t0.has_authority;
        t.authority = t0.authority;
        t.path = t0.path;
    }

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
