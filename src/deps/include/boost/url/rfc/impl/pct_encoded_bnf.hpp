//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PCT_ENCODED_BNF_HPP
#define BOOST_URL_IMPL_PCT_ENCODED_BNF_HPP

#include <boost/url/pct_encoding.hpp>
#include <boost/url/bnf/charset.hpp>

namespace boost {
namespace urls {

namespace detail {

template<class CharSet>
struct pct_encoded_bnf
{
    CharSet const& cs;
    pct_encoded_str& s;

    pct_encoded_bnf(
        CharSet const& cs_,
        pct_encoded_str& s_) noexcept
        : cs(cs_)
        , s(s_)
    {
    }
};

template<class CharSet>
bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    pct_encoded_bnf<
        CharSet> const& t) noexcept
{
    auto const start = it;
    // VFALCO TODO
    // opt.plus_to_space?
    std::size_t n = 0;
    char const* it0;
skip:
    it0 = it;
    it = bnf::find_if_not(
        it0, end, t.cs);
    n += it - it0;
    if(it == end)
        goto finish;
    if(*it != '%')
        goto finish;
    for(;;)
    {
        ++it;
        if(it == end)
        {
            // missing HEXDIG
            ec = BOOST_URL_ERR(
                error::missing_pct_hexdig);
            return false;
        }
        if(bnf::hexdig_value(*it) == -1)
        {
            // expected HEXDIG
            ec = BOOST_URL_ERR(
                error::bad_pct_hexdig);
            return false;
        }
        ++it;
        if(it == end)
        {
            // missing HEXDIG
            ec = BOOST_URL_ERR(
                error::missing_pct_hexdig);
            return false;
        }
        if(bnf::hexdig_value(*it) == -1)
        {
            // expected HEXDIG
            ec = BOOST_URL_ERR(
                error::bad_pct_hexdig);
            return false;
        }
        ++n;
        ++it;
        if(it == end)
            break;
        if(*it != '%')
            goto skip;
    }
finish:
    ec = {};
    t.s.str = string_view(
        start, it - start);
    t.s.decoded_size = n;
    return true;
}

} // detail

template<class CharSet>
detail::pct_encoded_bnf<CharSet>
pct_encoded_bnf(
    CharSet const& cs,
    pct_encoded_str& t) noexcept
{
    return detail::pct_encoded_bnf<
        CharSet>(cs, t);
}

} // urls
} // boost

#endif
