//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_ABSOLUTE_URI_BNF_HPP
#define BOOST_URL_RFC_ABSOLUTE_URI_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/rfc/hier_part_bnf.hpp>
#include <boost/url/rfc/query_bnf.hpp>
#include <boost/url/rfc/scheme_bnf.hpp>

namespace boost {
namespace urls {

/** BNF for absolute-URI

    @par BNF
    @code
    absolute-URI    = scheme ":" hier-part [ "?" query ]

    hier-part       = "//" authority path-abempty
                    / path-absolute
                    / path-rootless
                    / path-empty
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-4.3"
        >4.3. Absolute URI (rfc3986)</a>
*/
struct absolute_uri_bnf
{
    scheme_part_bnf scheme_part;
    hier_part_bnf   hier_part;
    query_part_bnf  query_part;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        absolute_uri_bnf& t);
};

} // urls
} // boost

#endif
