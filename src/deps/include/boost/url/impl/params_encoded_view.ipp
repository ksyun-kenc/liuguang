//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PARAMS_ENCODED_VIEW_IPP
#define BOOST_URL_IMPL_PARAMS_ENCODED_VIEW_IPP

#include <boost/url/params_encoded_view.hpp>
#include <boost/url/url.hpp>
#include <boost/url/rfc/query_bnf.hpp>
#include <boost/url/detail/pct_encoding.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {

void
params_encoded_view::
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

params_encoded_view::
iterator::
iterator(
    string_view s) noexcept
    : end_(s.data() + s.size())
    , p_(s.data())
{
    scan();
}

params_encoded_view::
iterator::
iterator(
    string_view s,
    int) noexcept
    : end_(s.data() + s.size())
    , p_(nullptr)
    , first_(false)
{
}

string_view
params_encoded_view::
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
params_encoded_view::
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
params_encoded_view::
iterator::
operator*() const ->
    value_type
{
    if(! first_)
    {
        if(nv_ > 0)
            return value_type{
                string_view(
                    p_ + 1, nk_ - 1),
                string_view(
                    p_ + nk_ + 1, nv_ - 1),
                true};
        return value_type{
            string_view(
                p_ + 1, nk_ - 1),
            string_view{},
            false};
    }
    if(nv_ > 0)
        return value_type{
            string_view(
                p_, nk_),
            string_view(
                p_ + nk_ + 1,
                nv_ - 1),
            true};
    return value_type{
        string_view(
            p_, nk_),
        string_view{},
        false};
}

bool
operator==(
    params_encoded_view::
        iterator a,
    params_encoded_view::
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
params_encoded_view::
at(string_view key) const ->
    string_view
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
    return {
        it.p_ + it.nk_ + 1,
        it.nv_ - 1 };
}

//--------------------------------------------
//
// Iterators
//
//--------------------------------------------

auto
params_encoded_view::
begin() const noexcept ->
    iterator
{
    return { s_ };
}

auto
params_encoded_view::
end() const noexcept ->
    iterator
{
    return { s_, 0 };
}

//------------------------------------------------
//
// Lookup
//
//------------------------------------------------

std::size_t
params_encoded_view::
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
params_encoded_view::
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
        // VFALCO need pct-encoded comparison
        if( key == from.encoded_key())
            break;
        ++from;
    }
    return from;
}

//------------------------------------------------
//
// Parsing
//
//------------------------------------------------

result<params_encoded_view>
parse_query_params(
    string_view s) noexcept
{
    error_code ec;
    query_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;
    return params_encoded_view(
        t.str, t.count);
}

} // urls
} // boost

#endif
