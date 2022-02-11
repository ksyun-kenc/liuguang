//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PARAMS_VIEW_IPP
#define BOOST_URL_IMPL_PARAMS_VIEW_IPP

#include <boost/url/params_view.hpp>
#include <boost/url/url.hpp>
#include <boost/url/detail/pct_encoding.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {

params_view::
value_type::
~value_type() noexcept = default;

params_view::
value_type::
value_type() noexcept = default;

params_view::
value_type::
value_type(
    value_type const& other) noexcept = default;

auto
params_view::
value_type::
operator=(
    value_type const& other) noexcept ->
        value_type& = default;

params_view::
value_type::
value_type(
    char const* s,
    std::size_t nk,
    std::size_t const nv,
    string_value::allocator a)
{
    if(nk + nv == 0)
    {
        has_value = false;
        return;
    }
    if(nv > 0)
    {
        // value
        BOOST_ASSERT(s[nk] == '=');
        has_value = true;
        string_view ev{
            s + nk + 1, nv - 1 };
        auto n =
            pct_decode_bytes_unchecked(ev);
        char *dest;
        value = a.make_string_value(
            n, dest);
        pct_decode_unchecked(
            dest, dest + n, ev);
    }
    else
    {
        has_value = false;
    }
    // key
    string_view ek{s, nk};
    auto n =
        pct_decode_bytes_unchecked(ek);
    char* dest;
    key = a.make_string_value(n, dest);
    pct_decode_unchecked(
        dest, dest + nk, ek);
}

//------------------------------------------------

void
params_view::
iterator::
scan() noexcept
{
    string_view s(p_, end_ - p_);
    std::size_t i;
    if(! first_)
    {
        BOOST_ASSERT(
            s.starts_with('&'));
        i = s.find_first_of('&', 1);
    }
    else
    {
        i = s.find_first_of('&');
    }
    if( i == string_view::npos)
        i = s.size();
    nk_ = string_view(
        p_, i).find_first_of('=');
    if(nk_ != string_view::npos)
    {
        nv_ = i - nk_;
    }
    else
    {
        // has_value==false
        nk_ = i;
        nv_ = 0;
    }
}

params_view::
iterator::
iterator(
    string_view s,
    string_value::allocator a) noexcept
    : end_(s.data() + s.size())
    , p_(s.data())
    , a_(a)
{
    scan();
}

params_view::
iterator::
iterator(
    string_view s,
    int,
    string_value::allocator a) noexcept
    : end_(s.data() + s.size())
    , p_(nullptr)
    , a_(a)
    , first_(false)
{
}

string_view
params_view::
iterator::
encoded_key() const noexcept
{
    BOOST_ASSERT(p_ != nullptr);
    if(! first_)
        return string_view(
            p_ + 1, nk_ - 1);
    return string_view{ p_, nk_ };
}

auto
params_view::
iterator::
operator++() noexcept ->
    iterator&
{
    BOOST_ASSERT(p_ != nullptr);
    first_ = false;
    p_ += nk_ + nv_;
    if(p_ == end_)
    {
        p_ = nullptr;
        nk_ = 0;
        nv_ = 0;
        return *this;
    }
    scan();
    return *this;
}

auto
params_view::
iterator::
operator*() const ->
    value_type
{
    if(! first_)
        return value_type(
            p_ + 1, nk_ - 1,
                nv_, a_);
    return value_type(
        p_, nk_, nv_, a_);
}

bool
operator==(
    params_view::
        iterator a,
    params_view::
        iterator b) noexcept
{
    BOOST_ASSERT(a.end_ == b.end_);
    return
        a.p_ == b.p_ &&
        a.first_ == b.first_;
}

//------------------------------------------------
//
// Element Access
//
//------------------------------------------------

auto
params_view::
at(string_view key) const ->
    string_value
{
    auto it = find(key);
    for(;;)
    {
        if(it == end())
            detail::throw_out_of_range(
                BOOST_CURRENT_LOCATION);
        if(it.nv_ != 0)
            break;
        ++it;
        it = find(it, key);
    }
    string_view ev{
        it.p_ + it.nk_ + 1,
        it.nv_ - 1 };
    auto n =
        pct_decode_bytes_unchecked(ev);
    char *dest;
    auto s = a_.make_string_value(n, dest);
    pct_decode_unchecked(
        dest, dest + n, ev);
    return s;
}

//------------------------------------------------
//
// Iterators
//
//------------------------------------------------

auto
params_view::
begin() const noexcept ->
    iterator
{
    return { s_, a_ };
}

auto
params_view::
end() const noexcept ->
    iterator
{
    return { s_, 0, a_ };
}

//------------------------------------------------
//
// Lookup
//
//------------------------------------------------

std::size_t
params_view::
count(string_view key) const noexcept
{
    std::size_t n = 0;
    auto it = find(key);
    auto const end_ = end();
    while(it != end_)
    {
        ++n;
        ++it;
        it = find(it, key);
    }
    return n;
}

auto
params_view::
find(
    iterator from,
    string_view key) const noexcept ->
        iterator
{
    BOOST_ASSERT(from.end_ ==
        s_.data() + s_.size());

    auto const end_ = end();
    while(from != end_)
    {
        if( detail::key_equal_encoded(
            key,
            from.encoded_key()))
            break;
        ++from;
    }
    return from;
}

} // urls
} // boost

#endif
