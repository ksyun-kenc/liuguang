//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_USERINFO_BNF_HPP
#define BOOST_URL_RFC_USERINFO_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/pct_encoding_types.hpp>
#include <boost/url/string.hpp>

namespace boost {
namespace urls {

/** BNF for userinfo

    @par BNF
    @code
    userinfo    = user [ ":" [ password ] ]

    user        = *( unreserved / pct-encoded / sub-delims )
    password    = *( unreserved / pct-encoded / sub-delims / ":" )
    @endcode

    @par Specification
    <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1"
        >3.2.1. User Information (3986)</a>
*/
struct userinfo_bnf
{
    pct_encoded_str user;
    string_view     user_part;
    bool            has_password = false;
    pct_encoded_str password;
    string_view     password_part;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        userinfo_bnf& t);
};

} // urls
} // boost

#endif
