//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_RELATIVE_REF_BNF_HPP
#define BOOST_URL_RFC_RELATIVE_REF_BNF_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/rfc/fragment_bnf.hpp>
#include <boost/url/rfc/query_bnf.hpp>
#include <boost/url/rfc/relative_part_bnf.hpp>

namespace boost {
namespace urls {

/** BNF for relative-ref

    @par BNF
    @code
    relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc3986#section-4.2
*/
struct relative_ref_bnf
{
    relative_part_bnf   relative_part;
    query_part_bnf      query_part;
    fragment_part_bnf   fragment_part;

    BOOST_URL_DECL
    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        relative_ref_bnf& t);
};

} // urls
} // boost

#endif
