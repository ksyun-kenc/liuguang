//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_ANY_PARAMS_ITER_IPP
#define BOOST_URL_DETAIL_IMPL_ANY_PARAMS_ITER_IPP

#include <boost/url/detail/any_params_iter.hpp>
#include <boost/url/string_view.hpp>
#include <boost/url/decode.hpp>
#include <boost/url/rfc/detail/charsets.hpp>

namespace boost {
namespace urls {
namespace detail {

/*
    When a string is transformed into a range of
    params, the empty string becomes ambiguous:
    it can be an empty range, or a range with
    one param. The value `not_empty` is used on
    construction to inform the transformation
    that the empty string should be treated as
    a one-element range. This simplifies
    edit_params().
*/

//------------------------------------------------
//
// any_params_iter
//
//------------------------------------------------

any_params_iter::
~any_params_iter() noexcept = default;

//------------------------------------------------
//
// query_iter
//
//------------------------------------------------

query_iter::
query_iter(
    string_view s,
    bool ne) noexcept
    : any_params_iter(
        s.empty() && ! ne, s)
{
    rewind();
}

void
query_iter::
rewind() noexcept
{
    if(empty)
    {
        at_end_ = true;
        return;
    }
    p_ = s0.begin();
    if(! s0.empty())
    {
        auto pos =
            s0.find_first_of('&');
        if(pos != string_view::npos)
            n_ = pos;
        else
            n_ = s0.size();
    }
    else
    {
        n_ = 0;
    }
    at_end_ = false;
}

bool
query_iter::
measure(
    std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        string_view(p_, n_),
        opt,
        query_chars);
    increment();
    return true;
}

void
query_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(! at_end_);
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode_unchecked(
        dest,
        end,
        string_view(p_, n_),
        opt,
        query_chars);
    increment();
}

void
query_iter::
increment() noexcept
{
    p_ += n_;
    if(p_ == s0.end())
    {
        at_end_ = true;
        return;
    }
    ++p_;
    string_view s(p_, s0.end() - p_);
    auto pos = s.find_first_of('&');
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

//------------------------------------------------
//
// param_iter
//
//------------------------------------------------

param_iter::
param_iter(
    param_view const& p) noexcept
    : any_params_iter(
        false,
        p.key,
        p.value)
    , has_value_(p.has_value)
{
}

void
param_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
param_iter::
measure(std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        s0,
        opt,
        detail::param_key_chars);
    if(has_value_)
    {
        ++n; // '='
        n += encoded_size(
            s1,
            opt,
            detail::param_value_chars);
    }
    at_end_ = true;
    return true;
}

void
param_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(! at_end_);
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode(
        dest,
        end,
        s0,
        opt,
        detail::param_key_chars);
    if(has_value_)
    {
        *dest++ = '=';
        dest += encode(
            dest,
            end,
            s1,
            opt,
            detail::param_value_chars);
    }
}

//------------------------------------------------
//
// params_iter_base
//
//------------------------------------------------

void
params_iter_base::
measure_impl(
    std::size_t& n,
    param_view const& p) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        p.key,
        opt,
        detail::param_key_chars);
    if(p.has_value)
    {
        ++n; // '='
        n += encoded_size(
            p.value,
            opt,
            detail::param_value_chars);
    }
}

void
params_iter_base::
copy_impl(
    char*& dest,
    char const* end,
    param_view const& p) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode(
        dest,
        end,
        p.key,
        opt,
        detail::param_key_chars);
    if(p.has_value)
    {
        *dest++ = '=';
        dest += encode(
            dest,
            end,
            p.value,
            opt,
            detail::param_value_chars);
    }
}

//------------------------------------------------
//
// param_encoded_iter
//
//------------------------------------------------

param_encoded_iter::
param_encoded_iter(
    param_pct_view const& p) noexcept
    : any_params_iter(
        false,
        p.key,
        p.value)
    , has_value_(p.has_value)
{
}

void
param_encoded_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
param_encoded_iter::
measure(std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += detail::re_encoded_size_unchecked(
        s0,
        opt,
        detail::param_key_chars);
    if(has_value_)
        n += detail::re_encoded_size_unchecked(
            s1,
            opt,
            detail::param_value_chars
                ) + 1; // for '='
    at_end_ = true;
    return true;
}

void
param_encoded_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        s0,
        opt,
        detail::param_key_chars);
    if(has_value_)
    {
        *dest++ = '=';
        detail::re_encode_unchecked(
            dest,
            end,
            s1,
            opt,
            detail::param_value_chars);
    }
}


//------------------------------------------------
//
// params_encoded_iter_base
//
//------------------------------------------------

void
params_encoded_iter_base::
measure_impl(
    std::size_t& n,
    param_view const& p) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    n += detail::re_encoded_size_unchecked(
        p.key,
        opt,
        detail::param_key_chars);
    if(p.has_value)
        n += detail::re_encoded_size_unchecked(
            p.value,
            opt,
            detail::param_value_chars
                ) + 1; // for '='
}

void
params_encoded_iter_base::
copy_impl(
    char*& dest,
    char const* end,
    param_view const& p) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        p.key,
        opt,
        detail::param_key_chars);
    if(p.has_value)
    {
        *dest++ = '=';
        detail::re_encode_unchecked(
            dest,
            end,
            p.value,
            opt,
            detail::param_value_chars);
    }
}

//------------------------------------------------
//
// param_value_iter
//
//------------------------------------------------

void
param_value_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
param_value_iter::
measure(
    std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    n += nk_; // skip key
    if(has_value_)
    {
        encode_opts opt;
        opt.space_to_plus = false;
        n += encoded_size(
            s0,
            opt,
            detail::param_value_chars
                ) + 1; // for '='
    }
    at_end_ = true;
    return true;
}

void
param_value_iter::
copy(char*& it, char const* end) noexcept
{
    it += nk_; // skip key
    if(! has_value_)
        return;
    *it++ = '=';
    encode_opts opt;
    opt.space_to_plus = false;
    it += encode(
        it,
        end,
        s0,
        opt,
        detail::param_value_chars);
}

//------------------------------------------------
//
// param_encoded_value_iter
//
//------------------------------------------------

void
param_encoded_value_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
param_encoded_value_iter::
measure(
    std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    n += nk_; // skip key
    if(has_value_)
    {
        encode_opts opt;
        opt.space_to_plus = false;
        n += detail::re_encoded_size_unchecked(
            s0,
            opt,
            detail::param_value_chars
                ) + 1; // for '='
    }
    at_end_ = true;
    return true;
}

void
param_encoded_value_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    dest += nk_; // skip key
    if(! has_value_)
        return;
    *dest++ = '=';
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        s0,
        opt,
        detail::param_value_chars);
}

} // detail
} // urls
} // boost

#endif
