//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_IMPL_RANGE_HPP
#define BOOST_URL_BNF_IMPL_RANGE_HPP

#include <boost/url/detail/except.hpp>

namespace boost {
namespace urls {
namespace bnf {

template<class T>
bool
range::
parse_impl(
    char const*& it,
    char const* end,
    error_code& ec,
    range& t)
{
    typename T::value_type t0;
    auto start = it;
    std::size_t n = 0;
    if(! T::begin(it, end, ec, t0))
    {
        if(ec == error::end)
            goto finish;
        if(ec.failed())
            return false;
    }
    for(;;)
    {
        ++n;
        if(! T::increment(
            it, end, ec, t0))
        {
            if(ec == error::end)
                break;
            if(ec.failed())
                return false;
        }
    }
finish:
    t.str = string_view(
        start, it - start);
    t.count = n;
    ec = {};
    return true;
}

template<class T>
range::
range(T const*) noexcept
    : fp_(&range::parse_impl<T>)
{
    // Type requirements not met!
    BOOST_STATIC_ASSERT(
        is_range<T>::value);
}

inline
bool
parse(
    char const*& it,
    char const* end,
    error_code& ec,
    range& t)
{
    return t.fp_(it, end, ec, t);
}

} // bnf
} // urls
} // boost

#endif
