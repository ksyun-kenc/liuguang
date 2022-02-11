 //
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_IMPL_ANY_PATH_ITER_IPP
#define BOOST_URL_DETAIL_IMPL_ANY_PATH_ITER_IPP

#include <boost/url/detail/any_path_iter.hpp>
#include <boost/url/string.hpp>
#include <boost/url/rfc/charsets.hpp>
#include <boost/url/detail/pct_encoding.hpp>

namespace boost {
namespace urls {
namespace detail {

any_path_iter::
~any_path_iter() noexcept = default;

//------------------------------------------------

void
enc_path_iter::
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
    auto pos = s.find_first_of('/');
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

enc_path_iter::
enc_path_iter(
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
    if(s.starts_with('/'))
        s.remove_prefix(1);
    pos = s.find_first_of('/');
    p_ = s.data();
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
    front = { p_, n_ };
}

bool
enc_path_iter::
measure(
    std::size_t& n,
    error_code& ec) noexcept
{
    if(! p_)
        return false;
    string_view s(p_, n_);
    urls::validate_pct_encoding(
        s, ec, pchars);
    if(ec.failed())
        return false;
    n += s.size();
    increment();
    return true;
}

void
enc_path_iter::
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
plain_path_iter::
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
    auto pos = s.find_first_of('/');
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
}

plain_path_iter::
plain_path_iter(
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
    if(s.starts_with('/'))
        s.remove_prefix(1);
    pos = s.find_first_of('/');
    p_ = s.data();
    if(pos != string_view::npos)
        n_ = pos;
    else
        n_ = s.size();
    front = { p_, n_ };
}

bool
plain_path_iter::
measure(
    std::size_t& n,
    error_code&) noexcept
{
    if(! p_)
        return false;
    string_view s(p_, n_);
    n += urls::pct_encode_bytes(
        s, pchars);
    increment();
    return true;
}

void
plain_path_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(p_ != nullptr);
    dest += pct_encode(
        dest, end,
        string_view(p_, n_),
        pchars);
    increment();
}

//------------------------------------------------

bool
enc_segs_iter_base::
measure_impl(
    string_view s,
    std::size_t& n,
    error_code& ec) noexcept
{
    urls::validate_pct_encoding(
        s, ec, pchars);
    if(ec.failed())
        return false;
    n += s.size();
    return true;
}

void
enc_segs_iter_base::
copy_impl(
    string_view s,
    char*& dest,
    char const* end) noexcept
{
    (void)end;
    BOOST_ASSERT(static_cast<
        std::size_t>(end - dest) >=
            s.size());
    if(! s.empty())
    {
        std::memcpy(dest,
            s.data(), s.size());
        dest += s.size();
    }
}

//------------------------------------------------

void
plain_segs_iter_base::
measure_impl(
    string_view s,
    std::size_t& n) noexcept
{
    n += pct_encode_bytes(
        s, pchars);
}

void
plain_segs_iter_base::
copy_impl(
    string_view s,
    char*& dest,
    char const* end) noexcept
{
    dest += pct_encode(
        dest, end, s, pchars);
}

} // detail
} // urls
} // boost

#endif
