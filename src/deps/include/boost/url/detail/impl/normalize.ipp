//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_NORMALIZE_IPP
#define BOOST_URL_DETAIL_IMPL_NORMALIZE_IPP

#include <boost/url/detail/normalize.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
namespace urls {
namespace detail {

void
pop_encoded_front(
    string_view& s,
    char& c,
    std::size_t& n) noexcept
{
    if(s.front() != '%')
    {
        c = s.front();
        s.remove_prefix(1);
    }
    else
    {
        detail::decode_unchecked(
            &c,
            &c + 1,
            s.substr(0, 3));
        s.remove_prefix(3);
    }
    ++n;
}

int
compare_encoded(
    string_view lhs,
    string_view rhs) noexcept
{
    std::size_t n0 = 0;
    std::size_t n1 = 0;
    char c0 = 0;
    char c1 = 0;
    while(
        !lhs.empty() &&
        !rhs.empty())
    {
        pop_encoded_front(lhs, c0, n0);
        pop_encoded_front(rhs, c1, n1);
        if (c0 < c1)
            return -1;
        if (c1 < c0)
            return 1;
    }
    n0 += detail::decode_bytes_unchecked(lhs);
    n1 += detail::decode_bytes_unchecked(rhs);
    if (n0 == n1)
        return 0;
    if (n0 < n1)
        return -1;
    return 1;
}

void
digest_encoded(
    string_view s,
    fnv_1a& hasher) noexcept
{
    char c = 0;
    std::size_t n = 0;
    while(!s.empty())
    {
        pop_encoded_front(s, c, n);
        hasher.put(c);
    }
}

int
ci_compare_encoded(
    string_view lhs,
    string_view rhs) noexcept
{
    std::size_t n0 = 0;
    std::size_t n1 = 0;
    char c0 = 0;
    char c1 = 0;
    while (
        !lhs.empty() &&
        !rhs.empty())
    {
        pop_encoded_front(lhs, c0, n0);
        pop_encoded_front(rhs, c1, n1);
        c0 = grammar::to_lower(c0);
        c1 = grammar::to_lower(c1);
        if (c0 < c1)
            return -1;
        if (c1 < c0)
            return 1;
    }
    n0 += detail::decode_bytes_unchecked(lhs);
    n1 += detail::decode_bytes_unchecked(rhs);
    if (n0 == n1)
        return 0;
    if (n0 < n1)
        return -1;
    return 1;
}

void
ci_digest_encoded(
    string_view s,
    fnv_1a& hasher) noexcept
{
    char c = 0;
    std::size_t n = 0;
    while(!s.empty())
    {
        pop_encoded_front(s, c, n);
        c = grammar::to_lower(c);
        hasher.put(c);
    }
}

int
compare(
    string_view lhs,
    string_view rhs) noexcept
{
    auto rlen = (std::min)(lhs.size(), rhs.size());
    for (std::size_t i = 0; i < rlen; ++i)
    {
        char c0 = lhs[i];
        char c1 = rhs[i];
        if (c0 < c1)
            return -1;
        if (c1 < c0)
            return 1;
    }
    if ( lhs.size() == rhs.size() )
        return 0;
    if ( lhs.size() < rhs.size() )
        return -1;
    return 1;
}

int
ci_compare(
    string_view lhs,
    string_view rhs) noexcept
{
    auto rlen = (std::min)(lhs.size(), rhs.size());
    for (std::size_t i = 0; i < rlen; ++i)
    {
        char c0 = grammar::to_lower(lhs[i]);
        char c1 = grammar::to_lower(rhs[i]);
        if (c0 < c1)
            return -1;
        if (c1 < c0)
            return 1;
    }
    if ( lhs.size() == rhs.size() )
        return 0;
    if ( lhs.size() < rhs.size() )
        return -1;
    return 1;
}

void
ci_digest(
    string_view s,
    fnv_1a& hasher) noexcept
{
    for (char c: s)
    {
        c = grammar::to_lower(c);
        hasher.put(c);
    }
}

std::size_t
path_starts_with(
    string_view lhs,
    string_view rhs) noexcept
{
    auto consume_one = [](
        string_view::iterator& it,
        char &c)
    {
        if(*it != '%')
        {
            c = *it;
            ++it;
            return;
        }
        detail::decode_unchecked(
            &c,
            &c + 1,
            string_view(it, 3));
        if (c != '/')
        {
            it += 3;
            return;
        }
        c = *it;
        ++it;
    };

    auto it0 = lhs.begin();
    auto it1 = rhs.begin();
    auto end0 = lhs.end();
    auto end1 = rhs.end();
    char c0 = 0;
    char c1 = 0;
    while (
        it0 < end0 &&
        it1 < end1)
    {
        consume_one(it0, c0);
        consume_one(it1, c1);
        if (c0 != c1)
            return 0;
    }
    if (it1 == end1)
        return it0 - lhs.begin();
    return 0;
}

std::size_t
path_ends_with(
    string_view lhs,
    string_view rhs) noexcept
{
    auto consume_last = [](
        string_view::iterator& it,
        string_view::iterator& end,
        char& c)
    {
        if ((end - it) < 3 ||
            *(std::prev(end, 3)) != '%')
        {
            c = *--end;
            return;
        }
        detail::decode_unchecked(
            &c,
            &c + 1,
            string_view(std::prev(
                end, 3), 3));
        if (c != '/')
        {
            end -= 3;
            return;
        }
        c = *--end;
    };

    auto it0 = lhs.begin();
    auto it1 = rhs.begin();
    auto end0 = lhs.end();
    auto end1 = rhs.end();
    char c0 = 0;
    char c1 = 0;
    while(
        it0 < end0 &&
        it1 < end1)
    {
        consume_last(it0, end0, c0);
        consume_last(it1, end1, c1);
        if (c0 != c1)
            return 0;
    }
    if (it1 == end1)
        return lhs.end() - end0;
    return 0;
}

} // detail
} // urls
} // boost

#endif
