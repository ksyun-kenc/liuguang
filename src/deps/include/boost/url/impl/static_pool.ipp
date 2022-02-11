//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_STATIC_POOL_IPP
#define BOOST_URL_IMPL_STATIC_POOL_IPP

#include <boost/url/static_pool.hpp>
#include <boost/url/detail/except.hpp>

namespace boost {
namespace urls {

void**
basic_static_pool::
table() noexcept
{
    return reinterpret_cast<
        void**>(begin_);
}

void**
basic_static_pool::
find(void* p) noexcept
{
    auto t = table();
    auto const end =
        t + n_;
    (void)end;
    BOOST_ASSERT(t != end);
    while(*t != p)
    {
        ++t;
        BOOST_ASSERT(
            t != end);
    }
    return t;
}

basic_static_pool::
~basic_static_pool()
{
    BOOST_ASSERT(n_ == 0);
    BOOST_ASSERT(top_ == end_);
}

void*
basic_static_pool::
allocate(
    std::size_t bytes,
    std::size_t align)
{
    static constexpr auto S =
        sizeof(void*);
    bytes = alignment::align_up(
        bytes, align);
    auto p = reinterpret_cast<char*>(
        reinterpret_cast<std::uintptr_t>(
            top_ - bytes) & ~(align - 1));
    auto low = begin_ + S * (n_ + 1);
    if(p < low)
        detail::throw_bad_alloc(
            BOOST_CURRENT_LOCATION);
    top_ = p;
    table()[n_] = p;
    ++n_;
    return p;
}

void
basic_static_pool::
deallocate(
    void* p,
    std::size_t bytes,
    std::size_t align) noexcept
{
    bytes = alignment::align_up(
        bytes, align);
    BOOST_ASSERT(n_ > 0);
    if(p != top_)
    {
        auto t = find(p);
        *t = reinterpret_cast<void*>(
            reinterpret_cast<
                std::uintptr_t>(*t) | 1);
        return;
    }
    --n_;
    top_ += bytes;
    while(n_ > 0)
    {
        auto i = reinterpret_cast<
            std::uintptr_t>(table()[n_ - 1]);
        if((i & 1) == 0)
        {
            // not free
            top_ = reinterpret_cast<
                char*>(i);
            return;
        }
        --n_;
    }
    top_ = end_;
}

} // urls
} // boost

#endif
