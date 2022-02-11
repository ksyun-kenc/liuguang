//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_STATIC_POOL_HPP
#define BOOST_URL_IMPL_STATIC_POOL_HPP

namespace boost {
namespace urls {

template<class T>
class basic_static_pool::
    allocator_type
{
    basic_static_pool* pool_ = nullptr;
   
    template<class U>
    friend class allocator_type;

public:
    using is_always_equal = std::false_type;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using const_pointer = T const*;
    using const_reference = T const&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
 
    template<class U>
    struct rebind
    {
        using other = allocator_type<U>;
    };

#ifndef BOOST_URL_NO_GCC_4_2_WORKAROUND
    // libg++ basic_string requires Allocator to be DefaultConstructible
    // https://code.woboq.org/firebird/include/c++/4.8.2/bits/basic_string.tcc.html#82
    allocator_type() = default;
#endif

    template<class U>
    allocator_type(
        allocator_type<U> const& other) noexcept
        : pool_(other.pool_)
    {
    }

    explicit
    allocator_type(
        basic_static_pool& pool)
        : pool_(&pool)
    {
    }

    pointer
    allocate(size_type n)
    {
        return reinterpret_cast<T*>(
            pool_->allocate(
                n * sizeof(T), alignof(T)));
    }

    void
    deallocate(
        pointer p,
        size_type n) noexcept
    {
        pool_->deallocate(p,
            n * sizeof(T), alignof(T));
    }

    template<class U>
    bool
    operator==(allocator_type<
        U> const& other) const noexcept
    {
        return pool_ == other.pool_;
    }

    template<class U>
    bool
    operator!=(allocator_type<
        U> const& other) const noexcept
    {
        return pool_ != other.pool_;
    }
};

auto
basic_static_pool::
allocator() noexcept ->
    allocator_type<char>
{
    return allocator_type<char>(*this);
}

template<class... Args>
auto
basic_static_pool::
make_string(Args&&... args) ->
    string_type
{
    return std::basic_string<
        char, std::char_traits<char>,
            allocator_type<char>>(
        std::forward<Args>(args)...,
            allocator());
}

} // urls
} // boost

#endif
