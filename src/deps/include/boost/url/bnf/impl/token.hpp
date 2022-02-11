//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_IMPL_TOKEN_HPP
#define BOOST_URL_BNF_IMPL_TOKEN_HPP

#include <boost/url/error.hpp>
#include <boost/url/bnf/charset.hpp>

namespace boost {
namespace urls {
namespace bnf {

namespace detail {

template<class CharSet>
struct token
{
    BOOST_STATIC_ASSERT(
        is_charset<
            CharSet>::value);

    CharSet const& cs_;
    string_view& s_;

    template<class CharSet_>
    friend
    bool
    parse(
        char const*& it,
        char const* end,
        error_code& ec,
        token<CharSet_> const& t) noexcept;
};

template<class CharSet>
bool
parse(
    char const*& it,
    char const* end,
    error_code& ec,
    token<CharSet> const& t) noexcept
{
    ec = {};
    auto const start = it;
    it = bnf::find_if_not(it, end, t.cs_);
    t.s_ = string_view(start, it - start);
    return true;
}

} // detail

template<class CharSet>
detail::token<CharSet>
token(
    CharSet const& cs,
    string_view& t) noexcept
{
    return detail::token<
        CharSet>{cs, t};
}

} // bnf
} // urls
} // boost

#endif
