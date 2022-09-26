//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_ANY_SEGMENTS_ITER_IPP
#define BOOST_URL_DETAIL_IMPL_ANY_SEGMENTS_ITER_IPP

#include <boost/url/detail/any_segments_iter.hpp>
#include <boost/url/string_view.hpp>
#include <boost/url/decode.hpp>
#include <boost/url/encode.hpp>
#include <boost/url/rfc/pchars.hpp>

namespace boost {
namespace urls {
namespace detail {

//------------------------------------------------
//
// path_iter
//
//------------------------------------------------

path_iter::
path_iter(
    string_view s_) noexcept
    : any_segments_iter(s_)
{
    rewind();
}

void
path_iter::
increment() noexcept
{
    pos_ = next_;
    if(pos_ == s.size())
    {
        pos_ = string_view::npos;
        return;
    }
    // skip '/'
    ++pos_;
    auto const end =
        s.data() + s.size();
    auto const p0 =
        s.data() + pos_;
    auto p = p0;
    while(p != end)
    {
        if(*p == '/')
        {
            next_ = p - s.data();
            return;
        }
        ++p;
    }
    next_ = s.size();
}

void
path_iter::
rewind() noexcept
{
    pos_ = 0;
    auto p0 = s.data();
    auto const end = p0 + s.size();
    if(p0 != end)
    {
        // skip leading '/'
        // of absolute-path
        if(*p0 == '/')
        {
            ++p0;
            ++pos_;
        }
        auto p = p0;
        while(p != end)
        {
            if(*p == '/')
                break;
            ++p;
        }
        front = string_view(
            p0, p - p0);
        next_ = p - s.data();
    }
    else
    {
        pos_ = string_view::npos;
        front = { p0, 0 };
    }
}

bool
path_iter::
measure(
    std::size_t& n) noexcept
{
    if(pos_ == string_view::npos)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        s.substr(
            pos_,
            next_ - pos_),
        opt,
        pchars);
    increment();
    return true;
}

void
path_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(pos_ !=
        string_view::npos);
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode(
        dest,
        end,
        s.substr(
            pos_,
            next_ - pos_),
        opt,
        pchars);
    increment();
}

//------------------------------------------------
//
// path_encoded_iter
//
//------------------------------------------------

path_encoded_iter::
path_encoded_iter(
    pct_string_view s) noexcept
    : path_iter(s)
{
}

bool
path_encoded_iter::
measure(
    std::size_t& n) noexcept
{
    if(pos_ == string_view::npos)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += detail::re_encoded_size_unchecked(
        s.substr(
            pos_,
            next_ - pos_),
        opt,
        pchars);
    increment();
    return true;
}

void
path_encoded_iter::
copy(
    char*& dest,
    char const* end) noexcept
{
    BOOST_ASSERT(pos_ !=
        string_view::npos);
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        s.substr(
            pos_,
            next_ - pos_),
        opt,
        pchars);
    increment();
}

//------------------------------------------------
//
// segment_iter
//
//------------------------------------------------

segment_iter::
segment_iter(
    string_view s_) noexcept
    : any_segments_iter(s_)
{
    front = s;
}

void
segment_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
segment_iter::
measure(
    std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        s,
        opt,
        pchars);
    at_end_ = true;
    return true;
}

void
segment_iter::
copy(char*& dest, char const* end) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode(
        dest,
        end,
        s,
        opt,
        pchars);
}

//------------------------------------------------
//
// segments_iter_base
//
//------------------------------------------------

void
segments_iter_base::
measure_impl(
    std::size_t& n,
    string_view s) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    n += encoded_size(
        s,
        opt,
        pchars);
}

void
segments_iter_base::
copy_impl(
    char*& dest,
    char const* end,
    string_view s) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    dest += encode(
        dest,
        end,
        s,
        opt,
        pchars);
}

//------------------------------------------------
//
// segment_encoded_iter
//
//------------------------------------------------

segment_encoded_iter::
segment_encoded_iter(
    pct_string_view const& s_) noexcept
    : any_segments_iter(s_)
{
    front = s;
}

void
segment_encoded_iter::
rewind() noexcept
{
    at_end_ = false;
}

bool
segment_encoded_iter::
measure(
    std::size_t& n) noexcept
{
    if(at_end_)
        return false;
    encode_opts opt;
    opt.space_to_plus = false;
    n += detail::re_encoded_size_unchecked(
        s,
        opt,
        pchars);
    at_end_ = true;
    return true;
}

void
segment_encoded_iter::
copy(char*& dest, char const* end) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        s,
        opt,
        pchars);
}

//------------------------------------------------
//
// segments_encoded_iter_base
//
//------------------------------------------------

void
segments_encoded_iter_base::
measure_impl(
    std::size_t& n,
    string_view s) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    n += detail::re_encoded_size_unchecked(
        s,
        opt,
        pchars);
}

void
segments_encoded_iter_base::
copy_impl(
    char*& dest,
    char const* end,
    string_view s) noexcept
{
    encode_opts opt;
    opt.space_to_plus = false;
    detail::re_encode_unchecked(
        dest,
        end,
        s,
        opt,
        pchars);
}

} // detail
} // urls
} // boost

#endif
