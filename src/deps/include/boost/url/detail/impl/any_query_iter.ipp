//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_IMPL_ANY_QUERY_ITER_IPP
#define BOOST_URL_DETAIL_IMPL_ANY_QUERY_ITER_IPP

#include <boost/url/detail/any_query_iter.hpp>
#include <boost/url/string.hpp>
#include <boost/url/rfc/charsets.hpp>

namespace boost {
namespace urls {
namespace detail {

any_query_iter::
~any_query_iter() noexcept = default;

//------------------------------------------------

void
enc_query_iter::
increment() noexcept
{
    p_ += n_;
    if(p_ == end_)
    {
        p_ = nullptr;
        return;
    }
    ++p_;
    string_view s(p_, end_ - p_);
    auto pos = s.find_first_of('&');
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

enc_query_iter::
enc_query_iter(
    string_view s) noexcept
    : end_(s.data() + s.size())
{
    if(s.empty())
    {
        n_ = 0;
        p_ = nullptr;
        return;
    }
    std::size_t pos;
    pos = s.find_first_of('&');
    p_ = s.data();
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

bool
enc_query_iter::
measure(
    std::size_t& n,
    error_code& ec) noexcept
{
    if(! p_)
        return false;
    string_view s(p_, n_);
    static auto constexpr cs =
        pchars + '/' + '?';
    urls::validate_pct_encoding(
        s, ec, cs);
    if(ec.failed())
        return false;
    n += s.size();
    increment();
    return true;
}

void
enc_query_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    (void)end;
    BOOST_ASSERT(static_cast<
        std::size_t>(
            end - dest) >= n_);
    BOOST_ASSERT(p_ != nullptr);
    if(n_ > 0)
    {
        std::memcpy(
            dest, p_, n_);
        dest += n_;
    }
    increment();
}

//------------------------------------------------

void
plain_query_iter::
increment() noexcept
{
    p_ += n_;
    if(p_ == end_)
    {
        p_ = nullptr;
        return;
    }
    ++p_;
    string_view s(p_, end_ - p_);
    auto pos = s.find_first_of('&');
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

plain_query_iter::
plain_query_iter(
    string_view s) noexcept
    : end_(s.data() + s.size())
{
    if(s.empty())
    {
        n_ = 0;
        p_ = nullptr;
        return;
    }
    std::size_t pos;
    pos = s.find_first_of('&');
    p_ = s.data();
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

bool
plain_query_iter::
measure(
    std::size_t& n,
    error_code&) noexcept
{
    if(! p_)
        return false;
    string_view s(p_, n_);
    static auto constexpr cs =
        pchars + '/' + '?';
    n += urls::pct_encode_bytes(
        s, cs);
    increment();
    return true;
}

void
plain_query_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(p_ != nullptr);
    static auto constexpr cs =
        pchars + '/' + '?';
    dest += pct_encode(
        dest, end,
        string_view(p_, n_),
        cs);
    increment();
}

//------------------------------------------------

bool
enc_params_iter_base::
measure_impl(
    string_view key,
    string_view const* value,
    std::size_t& n,
    error_code& ec) noexcept
{
    static constexpr auto cs =
        pchars + '/' + '?';
    pct_decode_opts opt;
    opt.plus_to_space = true;
    validate_pct_encoding(
        key, ec, cs, opt);
    if(ec.failed())
        return false;
    n += key.size();
    if(value)
    {
        validate_pct_encoding(
            *value, ec, cs, opt);
        if(ec.failed())
            return false;
        n += 1 + value->size();
    }
    return ! ec.failed();
}

void
enc_params_iter_base::
copy_impl(
    string_view key,
    string_view const* value,
    char*& dest,
    char const* end) noexcept
{
    // avoid self-copy
    if( key.data() != dest &&
        key.data() != nullptr)
    {
        std::size_t n =
            key.size();
        BOOST_ASSERT(
            end - n >= dest);
        std::memcpy(dest,
            key.data(), n);
        dest += n;
    }
    if(value)
    {
        BOOST_ASSERT(
            end - 1 >= dest);
        *dest++ = '=';
        std::size_t n =
            value->size();
        BOOST_ASSERT(
            end - n >= dest);
        if(n > 0)
        {
            std::memcpy(dest,
                value->data(), n);
            dest += n;
        }
    }
}

//------------------------------------------------

void
plain_params_iter_base::
measure_impl(
    string_view key,
    string_view const* value,
    std::size_t& n) noexcept
{
    static constexpr auto cs =
        pchars + '/' + '?';
    n += pct_encode_bytes(key, cs);
    if(value)
    {
        ++n; // '='
        n += pct_encode_bytes(
            *value, cs);
    }
}

void
plain_params_iter_base::
copy_impl(
    string_view key,
    string_view const* value,
    char*& dest,
    char const* end) noexcept
{
    static constexpr auto cs =
        pchars + '/' + '?';
    dest += pct_encode(
        dest, end, key, cs);
    if(value)
    {
        *dest++ = '=';
        dest += pct_encode(
            dest, end, *value, cs);
    }
}

} // detail
} // urls
} // boost

#endif
