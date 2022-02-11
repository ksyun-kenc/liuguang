//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_DETAIL_DEC_OCTET_HPP
#define BOOST_URL_RFC_DETAIL_DEC_OCTET_HPP

#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/charsets.hpp>

namespace boost {
namespace urls {
namespace detail {

struct dec_octet
{
    unsigned char& v;

    friend
    bool
    parse(
        char const*& it,
        char const* const end,
        error_code& ec,
        dec_octet const& t)
    {
        if(it == end)
        {
            // expected DIGIT
            ec = BOOST_URL_ERR(
                error::incomplete);
            return false;
        }
        if(! bnf::digit_chars(*it))
        {
            // not a digit
            ec = BOOST_URL_ERR(
                error::bad_digit);
            return false;
        }
        unsigned v = *it - '0';
        ++it;
        if(it == end)
        {
            t.v = static_cast<
                std::uint8_t>(v);
            ec = {};
            return true;
        }
        if(! bnf::digit_chars(*it))
        {
            t.v = static_cast<
                std::uint8_t>(v);
            ec = {};
            return true;
        }
        if(v == 0)
        {
            // bad leading '0'
            ec = BOOST_URL_ERR(
                error::bad_leading_zero);
            return false;
        }
        v = (10 * v) + *it - '0';
        ++it;
        if(it == end)
        {
            t.v = static_cast<
                std::uint8_t>(v);
            ec = {};
            return true;
        }
        if(! bnf::digit_chars(*it))
        {
            t.v = static_cast<
                std::uint8_t>(v);
            ec = {};
            return true;
        }
        if(v > 25)
        {
            // out of range
            ec = BOOST_URL_ERR(
                error::bad_octet);
            return false;
        }
        v = (10 * v) + *it - '0';
        if(v > 255)
        {
            // out of range
            ec = BOOST_URL_ERR(
                error::bad_octet);
            return false;
        }
        ++it;
        t.v = static_cast<
            std::uint8_t>(v);
        ec = {};
        return true;
    }
};

} // detail
} // urls
} // boost

#endif
