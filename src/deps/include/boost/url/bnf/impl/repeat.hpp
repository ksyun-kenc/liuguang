//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_IMPL_REPEAT_HPP
#define BOOST_URL_BNF_IMPL_REPEAT_HPP

#include <boost/url/error.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {
namespace bnf {

namespace detail {

template<class T>
bool
parse_repeat(
    char const*& it,
    char const* const end,
    error_code& ec,
    std::size_t N,
    std::size_t M,
    std::size_t& n)
{
    T v;
    n = 0;
    for(;;)
    {
        auto it1 = it;
        if(! parse(
            it1, end, ec, v))
            break;
        ++n;
        it = it1;
        if(n == M)
            break;
    }
    if(n < N)
    {
        // too few
        ec = BOOST_URL_ERR(
            error::syntax);
        return false;
    }
    return true;
}

} // detail

template<
    class T,
    std::size_t N,
    std::size_t M>
bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    repeat<T, N, M> const& t)
{
    auto start = it;
    std::size_t n;
    if(! detail::parse_repeat<T>(
        it, end, ec, N, M, n))
        return false;
    t.v = string_view(
        start, it - start);
    return true;
}

} // bnf
} // urls
} // boost

#endif
