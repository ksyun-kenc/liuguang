//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_SEGMENTS_ENCODED_VIEW_IPP
#define BOOST_URL_IMPL_SEGMENTS_ENCODED_VIEW_IPP

#include <boost/url/segments_view.hpp>
#include <boost/url/error.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/paths_bnf.hpp>
#include <boost/url/rfc/query_bnf.hpp>
#include <boost/url/detail/except.hpp>
#include <boost/url/detail/path.hpp>
#include <ostream>

namespace boost {
namespace urls {

//------------------------------------------------

segments_encoded_view::
iterator::
iterator(
    string_view s,
    std::size_t nseg) noexcept
    : begin_(s.data())
    , pos_(s.data())
    , next_(s.data())
    , end_(s.data() + s.size())
{
    if(nseg == 0)
    {
        next_ = nullptr;
        return;
    }
    auto const n =
        detail::path_prefix(s);
    begin_ += n;
    next_ += n;
    pos_ += n;
    auto const i = string_view(
        begin_, s.size() - n
            ).find_first_of('/');
    if(i != string_view::npos)
        next_ += i;
    else
        next_ = end_;
    s_ = string_view(
        pos_, next_ - pos_);
}

segments_encoded_view::
iterator::
iterator(
    string_view s,
    std::size_t nseg,
    int) noexcept
    : i_(nseg)
    , begin_(s.data())
    , pos_(s.data() + s.size())
    , end_(s.data() + s.size())
{
    auto const n =
        detail::path_prefix(s);
    begin_ += n;
}

auto
segments_encoded_view::
iterator::
operator++() noexcept ->
    iterator&
{
    using bnf::parse;
    using bnf_t =
        path_rootless_bnf;
    BOOST_ASSERT(next_ != nullptr);
    ++i_;
    pos_ = next_;
    error_code ec;
    // "/" segment
    pct_encoded_str t;
    bnf_t::increment(
        next_, end_, ec, t);
    if(ec == error::end)
    {
        next_ = nullptr;
        return *this;
    }
    BOOST_ASSERT(! ec);
    s_ = t.str;
    return *this;
}

auto
segments_encoded_view::
iterator::
operator--() noexcept ->
    iterator&
{
    using bnf::parse;
    using bnf_t =
        path_rootless_bnf;
    BOOST_ASSERT(i_ != 0);
    --i_;
    if(i_ == 0)
    {
        next_ = pos_;
        pos_ = begin_;
        s_ = string_view(
            pos_, next_ - pos_);
        return *this;
    }
    error_code ec;
    while(--pos_ != begin_)
    {
        if(*pos_ != '/')
            continue;
        // "/" segment
        next_ = pos_;
        pct_encoded_str t;
        bnf_t::increment(next_,
            end_, ec, t);
        BOOST_ASSERT(! ec);
        s_ = t.str;
        return *this;
    }
    next_ = pos_;
    if(*next_ == '/')
    {
        // "/" segment
        pct_encoded_str t;
        bnf_t::increment(next_,
            end_, ec, t);
        BOOST_ASSERT(! ec);
        s_ = t.str;
    }
    else
    {
        // segment-nz
        pct_encoded_str t;
        bnf_t::begin(next_,
            end_, ec, t);
        BOOST_ASSERT(! ec);
        s_ = t.str;
    }
    return *this;
}

//------------------------------------------------

segments_encoded_view::
segments_encoded_view(
    string_view s,
    std::size_t nseg) noexcept
    : s_(s)
    , n_(nseg)
{
}

//------------------------------------------------
//
// Iterators
//
//------------------------------------------------

auto
segments_encoded_view::
begin() const noexcept ->
    iterator
{
    return iterator(s_, n_);
}

auto
segments_encoded_view::
end() const noexcept ->
    iterator
{
    return iterator(s_, n_, 0);
}

//------------------------------------------------
//
// Friends
//
//------------------------------------------------

std::ostream&
operator<<(
    std::ostream& os,
    segments_encoded_view const& pv)
{
    os << pv.s_;
    return os;
}

//------------------------------------------------

result<segments_encoded_view>
parse_path(string_view s) noexcept
{
    if(s.empty())
        return segments_encoded_view();
    if(s[0] == '/')
        return parse_path_abempty(s);
    return parse_path_rootless(s);
}

result<segments_encoded_view>
parse_path_abempty(
    string_view s) noexcept
{
    error_code ec;
    path_abempty_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;
    return segments_encoded_view(
        t.str, detail::path_segments(
            t.str, t.count));
}

result<segments_encoded_view>
parse_path_absolute(
    string_view s) noexcept
{
    error_code ec;
    path_absolute_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;
    return segments_encoded_view(
        t.str, detail::path_segments(
            t.str, t.count));
}

result<segments_encoded_view>
parse_path_noscheme(
    string_view s) noexcept
{
    error_code ec;
    path_noscheme_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;
    return segments_encoded_view(
        t.str, detail::path_segments(
            t.str, t.count));
}

result<segments_encoded_view>
parse_path_rootless(
    string_view s) noexcept
{
    error_code ec;
    path_rootless_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;
    return segments_encoded_view(
        t.str, detail::path_segments(
            t.str, t.count));
}

} // urls
} // boost

#endif
