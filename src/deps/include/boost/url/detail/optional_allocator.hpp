//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_OPTIONAL_ALLOCATOR_HPP
#define BOOST_URL_DETAIL_OPTIONAL_ALLOCATOR_HPP

#include <new>

namespace boost {
namespace urls {
namespace detail {

// VFALCO This is so we can make
// iterators default-constructible
template<class Allocator>
class optional_allocator
{
    char buf_[sizeof(Allocator)];
    bool has_value_ = false;

public:
    ~optional_allocator()
    {
        if(has_value_)
            (*(*this)).~Allocator();
    }

    optional_allocator() = default;

    explicit
    optional_allocator(
        Allocator const& a) noexcept
        : has_value_(true)
    {
        ::new(buf_) Allocator(a);
    }

    optional_allocator(
        optional_allocator const& other) noexcept
        : has_value_(other.has_value_)
    {
        if(has_value_)
            ::new(buf_) Allocator(*other);
    }

    optional_allocator&
    operator=(optional_allocator const& other
        ) noexcept
    {
        if(has_value_)
            (*(*this)).~Allocator();
        has_value_ = other.has_value_;
        if(has_value_)
            ::new(buf_) Allocator(*other);
        return *this;
    }

    Allocator const&
    operator*() const noexcept
    {
        return *reinterpret_cast<
            Allocator const*>(buf_);
    }
};

} // detail
} // urls
} // boost

#endif
