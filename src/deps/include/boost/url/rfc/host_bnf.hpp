//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_HOST_BNF_HPP
#define BOOST_URL_RFC_HOST_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/host_type.hpp>
#include <boost/url/pct_encoding_types.hpp>
#include <boost/url/string.hpp>
#include <boost/url/ipv4_address.hpp>
#include <boost/url/ipv6_address.hpp>

namespace boost {
namespace urls {

/** BNF for host

    @par BNF
    @code
    host          = IP-literal / IPv4address / reg-name
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
*/
struct host_bnf
{
    urls::host_type host_type =
        urls::host_type::none;
    pct_encoded_str name;
    ipv4_address ipv4;
    ipv6_address ipv6;
    string_view ipvfuture;
    string_view host_part;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        host_bnf& t);
};

} // urls
} // boost

#endif
