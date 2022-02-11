//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_IPV_FUTURE_BNF_HPP
#define BOOST_URL_RFC_IPV_FUTURE_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>

namespace boost {
namespace urls {

/** BNF for IPvFuture

    @par BNF
    @code
    IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
*/
struct ipv_future_bnf
{
    string_view str;
    string_view major;
    string_view minor;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        ipv_future_bnf& t);
};

} // urls
} // boost

#endif
