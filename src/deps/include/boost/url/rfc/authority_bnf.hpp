//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_AUTHORITY_BNF_HPP
#define BOOST_URL_RFC_AUTHORITY_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/pct_encoding_types.hpp>
#include <boost/url/rfc/host_bnf.hpp>
#include <boost/url/rfc/port_bnf.hpp>
#include <boost/url/rfc/userinfo_bnf.hpp>

namespace boost {
namespace urls {

/** BNF for authority

    @par BNF
    @code
    authority   = [ userinfo "@" ] host [ ":" port ]
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2"
        >3.2. Authority (rfc3986)</a>
*/
struct authority_bnf
{
    // userinfo
    bool has_userinfo = false;
    userinfo_bnf userinfo;

    // host
    host_bnf host;

    // port
    port_part_bnf port;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        authority_bnf& t);
};

} // urls
} // boost

#endif
