//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_STRING_HPP
#define BOOST_URL_IMPL_STRING_HPP

#include <boost/url/detail/over_allocator.hpp>

namespace boost {
namespace urls {

struct string_value::base
{
    std::size_t refs = 1;
    virtual void destroy() noexcept = 0;
};

template<class Allocator>
auto
string_value::
construct(
    std::size_t n,
    Allocator const& a,
    char*& dest) ->
        base*
{
    class impl;

    using allocator_type =
        detail::over_allocator<
            impl, Allocator>;

    class impl : public base
    {
        allocator_type a_;

    public:
        ~impl()
        {
        }

        explicit
        impl(
            allocator_type const& a)
            : a_(a)
        {
        }

        void
        destroy() noexcept override
        {
            auto a(a_);
            a.deallocate(this, 1);
        }
    };

    if(n == 0)
    {
        dest = nullptr;
        return nullptr;
    }
    allocator_type al(n, a);
    auto p = ::new(
        al.allocate(1)) impl(al);
    dest = reinterpret_cast<
        char*>(p + 1);
    static_cast<string_view&>(
        *this) = { dest, n };
    return p;
}

string_value::
~string_value()
{
    if( p_ &&
        --p_->refs == 0)
        p_->destroy();
}

template<class Allocator>
string_value::
string_value(
    std::size_t n,
    Allocator const& a,
    char*& dest)
    : p_(construct(n, a, dest))
{
}

template<class Allocator>
string_value::
string_value(
    string_view s,
    Allocator const& a)
{
    char* dest;
    p_ = construct(
        s.size(), a, dest);
    std::memcpy(dest,
        s.data(), s.size());
}

string_value::
string_value(
    string_value const& other) noexcept
    : string_view(other)
    , p_(other.p_)
{
    if(p_)
        ++p_->refs;
}

string_value&
string_value::
operator=(
    string_value const& other) & noexcept
{
    if( p_ &&
        --p_->refs == 0)
        p_->destroy();
    p_ = other.p_;
    if(p_)
        ++p_->refs;
    static_cast<string_view&>(
        *this) = other;
    return *this;
}

//------------------------------------------------

class string_value::allocator
{
    struct base
    {
        std::size_t refs = 1;

        virtual
        ~base()
        {
        }

        virtual
        string_value
        alloc(
            std::size_t n,
            char*& dest) = 0;

        virtual
        void
        destroy() noexcept = 0;
    };

    base* p_ = nullptr;

public:
    ~allocator()
    {
        if( p_ &&
            --p_->refs == 0)
            p_->destroy();
    }

    allocator() = default;

    allocator(
        allocator const& other) noexcept
        : p_(other.p_)
    {
        if(p_)
            ++p_->refs;
    }

    allocator&
    operator=(
        allocator const& other) noexcept
    {
        if(other.p_)
            ++other.p_->refs;
        if( p_ &&
            --p_->refs == 0)
            p_->destroy();
        p_ = other.p_;
        return *this;
    }

    template<class Allocator>
    explicit
    allocator(Allocator const& a)
    {
        class impl;

        using allocator_type = typename
            detail::allocator_traits<
                Allocator>::template
                    rebind_alloc<impl>;

        class impl : public base
        {
            allocator_type a_;

        public:
            ~impl()
            {
            }

            impl(allocator_type const& a)
                : a_(a)
            {
            }

            string_value
            alloc(
                std::size_t n,
                char*& dest) override
            {
                return string_value(
                    n, a_, dest);
            }

            void
            destroy() noexcept override
            {
                auto a(a_);
                a.deallocate(this, 1);
            }
        };

        allocator_type al(a);
        p_ = ::new(al.allocate(1)) impl(al);
    }

    string_value
    make_string_value(
        std::size_t n,
        char*& dest) const
    {
        return p_->alloc(n, dest);
    }
};

} // urls
} // boost

#endif
