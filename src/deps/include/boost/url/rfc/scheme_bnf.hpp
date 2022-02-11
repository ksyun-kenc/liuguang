//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_SCHEME_BNF_HPP
#define BOOST_URL_RFC_SCHEME_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/scheme.hpp>
#include <boost/url/string.hpp>

namespace boost {
namespace urls {

/** BNF for scheme

    @par BNF
    @code
    scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc3986#section-3.1"
        >3.1. Scheme (rfc3986)</a>
*/
struct scheme_bnf
{
    string_view scheme;
    urls::scheme scheme_id;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        scheme_bnf& t);
};

/** BNF for scheme-part

    @par BNF
    @code
    scheme-part     = scheme ":"

    scheme          = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-3.1
*/
struct scheme_part_bnf
{
    string_view scheme;
    urls::scheme scheme_id =
        urls::scheme::none;
    string_view scheme_part;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        scheme_part_bnf& t);
};

} // urls
} // boost

#endif
