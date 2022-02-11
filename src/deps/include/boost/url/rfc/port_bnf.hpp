//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_PORT_BNF_HPP
#define BOOST_URL_RFC_PORT_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <cstdint>

namespace boost {
namespace urls {

/** BNF for port

    @par BNF
    @code
    port          = *DIGIT
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
*/
struct port_bnf
{
    using number_type =
        std::uint16_t;

    string_view str;
    std::uint16_t number;
    bool has_number = false;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        port_bnf& t);
};

/** BNF for port-part

    @par BNF
    @code
    port-part       = [ ":" port ]

    port            = *DIGIT
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.2
*/
struct port_part_bnf
{
    using number_type =
        std::uint16_t;

    bool has_port = false;
    string_view port;
    bool has_number = false;
    std::uint16_t port_number = 0;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        port_part_bnf& t);
};

} // urls
} // boost

#endif
