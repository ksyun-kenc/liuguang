//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PARAMS_IPP
#define BOOST_URL_IMPL_PARAMS_IPP

#include <boost/url/params.hpp>
#include <boost/url/url.hpp>
#include <boost/url/detail/pct_encoding.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {

params::
reference::
reference(
    char const* const s,
    std::size_t const nk,
    std::size_t const nv,
    string_value::allocator a)
{
    if(nv > 0)
    {
        // value
        BOOST_ASSERT(s[nk] == '=');
        has_value = true;
        string_view ev{
            s + nk + 1, nv - 1 };
        auto n = pct_decode_bytes_unchecked(ev);
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
    BOOST_ASSERT(nk > 0);
    BOOST_ASSERT(
        s[0] == '?' || s[0] == '&');
    string_view ek{s + 1, nk - 1};
    auto n =
        pct_decode_bytes_unchecked(ek);
    char* dest;
    key = a.make_string_value(n, dest);
    pct_decode_unchecked(
        dest, dest + nk, ek);
}

auto
params::
operator[](
    std::size_t pos) const ->
        reference
{
    BOOST_ASSERT(
        pos < u_->nparam_);
    auto const r =
        u_->param(pos);
    return reference(
        u_->s_ + r.pos,
            r.nk, r.nv, a_);
}

//------------------------------------------------

auto
params::
iterator::
operator*() const ->
    reference
{
    BOOST_ASSERT(i_ <
        u_->nparam_);
    auto const r =
        u_->param(i_);
    return reference(
        u_->s_ + r.pos,
        r.nk,
        r.nv,
        a_);
}

//------------------------------------------------
//
// Element Access
//
//------------------------------------------------

auto
params::
at(string_view key) const ->
    string_value
{
    url::raw_param r;
    auto it = find(key);
    for(;;)
    {
        if(it == end())
            detail::throw_out_of_range(
                BOOST_CURRENT_LOCATION);
        r = u_->param(it.i_);
        if(r.nv != 0)
            break;
        ++it;
        it = find(it, key);
    }
    string_view ev{
        u_->s_ + r.pos +
            r.nk + 1,
        r.nv - 1 };
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
// Modifiers
//
//------------------------------------------------

auto
params::
remove_value(
    iterator pos) ->
        iterator
{
    BOOST_ASSERT(pos.u_ == u_);
    auto r = u_->param(pos.i_);
    value_type v{
        string_view{
            u_->s_ + r.pos + 1,
            r.nk - 1},
        string_view{},
        false};
    u_->edit_params(
        pos.i_,
        pos.i_ + 1,
        detail::make_enc_params_iter(
            &v, &v + 1),
        detail::make_enc_params_iter(
            &v, &v + 1));
    return pos;
}

auto
params::
replace_value(
    iterator pos,
    string_view value) ->
        iterator
{
    (void)pos;
    (void)value;
    // VFALCO TODO
    return {};
}

auto
params::
erase(
    iterator first,
    iterator last) ->
        iterator
{
    BOOST_ASSERT(first.u_ == u_);
    BOOST_ASSERT(last.u_ == u_);
    string_view s;
    u_->edit_params(
        first.i_,
        last.i_,
        detail::enc_query_iter(s),
        detail::enc_query_iter(s));
    return first;
}

std::size_t
params::
erase(string_view key) noexcept
{
    std::size_t n = 0;
    auto it = find(key);
    while(it != end())
    {
        ++n;
        it = erase(it);
        it = find(it, key);
    }
    return n;
}

//------------------------------------------------
//
// Lookup
//
//------------------------------------------------

std::size_t
params::
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
params::
find(
    iterator from,
    string_view key) const noexcept ->
        iterator
{
    BOOST_ASSERT(from.u_ == u_);
    auto const end_ = end();
    while(from != end_)
    {
        auto r = u_->param(
            from.i_);
        if( detail::key_equal_encoded(
            key, string_view(u_->s_ +
            r.pos + 1, r.nk - 1)))
            break;
        ++from;
    }
    return from;
}

} // urls
} // boost

#endif
