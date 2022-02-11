//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_SEGMENTS_HPP
#define BOOST_URL_IMPL_SEGMENTS_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/pct_encoding.hpp>
#include <boost/url/string.hpp>
#include <boost/url/detail/any_path_iter.hpp>
#include <boost/url/detail/copied_strings.hpp>
#include <boost/url/detail/except.hpp>
#include <boost/url/detail/optional_allocator.hpp>
#include <boost/url/rfc/charsets.hpp>
#include <boost/assert.hpp>
#include <iterator>
#include <new>

namespace boost {
namespace urls {

//------------------------------------------------

class segments::iterator
{
    url const* u_ = nullptr;
    std::size_t i_ = 0;
    string_value::allocator a_;

    friend class segments;

    iterator(
        url const& u,
        std::size_t i,
        string_value::allocator
            const& a) noexcept
        : u_(&u)
        , i_(i)
        , a_(a)
    {
    }

public:
    using value_type = string_value;
    using reference = string_value;
    using pointer = void const*;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::random_access_iterator_tag;

    iterator() = default;

    iterator&
    operator++() noexcept
    {
        ++i_;
        return *this;
    }

    iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    iterator&
    operator--() noexcept
    {
        --i_;
        return *this;
    }

    iterator
    operator--(int) noexcept
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    BOOST_URL_DECL
    string_value
    operator*() const;

    friend
    bool
    operator==(
        iterator a,
        iterator b) noexcept
    {
        BOOST_ASSERT(a.u_ == b.u_);
        return a.u_ == b.u_ &&
            a.i_ == b.i_;
    }

    friend
    bool
    operator!=(
        iterator a,
        iterator b) noexcept
    {
        BOOST_ASSERT(a.u_ == b.u_);
        return a.u_ != b.u_ ||
            a.i_ != b.i_;
    }

    // LegacyRandomAccessIterator

    iterator&
    operator+=(ptrdiff_t n) noexcept
    {
        i_ += n;
        return *this;
    }

    friend
    iterator
    operator+(
        iterator it,
        ptrdiff_t n) noexcept
    {
        return { *it.u_, it.i_ + n, it.a_ };
    }

    friend
    iterator
    operator+(
        ptrdiff_t n,
        iterator it) noexcept
    {
        return { *it.u_, it.i_ + n, it.a_ };
    }

    iterator&
    operator-=(ptrdiff_t n) noexcept
    {
        i_ -= n;
        return *this;
    }

    friend
    iterator
    operator-(
        iterator it,
        ptrdiff_t n) noexcept
    {
        return { *it.u_, it.i_ - n, it.a_ };
    }

    friend
    std::ptrdiff_t
    operator-(
        iterator a,
        iterator b) noexcept
    {
        BOOST_ASSERT(a.u_ == b.u_);
        return static_cast<
            std::ptrdiff_t>(a.i_) - b.i_;
    }

    string_value
    operator[](ptrdiff_t n) const
    {
        return *(*this + n);
    }

    friend
    bool
    operator<(
        iterator a,
        iterator b)
    {
        BOOST_ASSERT(a.u_ == b.u_);
        return a.i_ < b.i_;
    }

    friend
    bool
    operator>(
        iterator a,
        iterator b)
    {
        return b < a;
    }

    friend
    bool
    operator>=(
        iterator a,
        iterator b)
    {
        return !(a < b);
    }

    friend
    bool
    operator<=(
        iterator a,
        iterator b)
    {
        return !(a > b);
    }
};

//------------------------------------------------
//
// Members
//
//------------------------------------------------

template<class Allocator>
segments::
segments(
    url& u,
    Allocator const& a)
    : u_(&u)
    , a_(a)
{
}

bool
segments::
is_absolute() const noexcept
{
    return
        u_->len(id_path) != 0 &&
        u_->s_[u_->offset(id_path)] == '/';
}

template<class String>
auto
segments::
operator=(
    std::initializer_list<String> init) ->
    typename std::enable_if<
        is_stringlike<String>::value,
        segments&>::type
{
    assign(init);
    return *this;
}

template<class String>
auto
segments::
assign(
    std::initializer_list<String> init) ->
    typename std::enable_if<
        is_stringlike<String>::value,
        void>::type
{
    assign(init.begin(), init.end());
}

template<class FwdIt>
auto
segments::
assign(FwdIt first, FwdIt last) ->
    typename std::enable_if<
        is_stringlike<typename
            std::iterator_traits<
                FwdIt>::value_type>::value,
        void>::type
{
    u_->edit_segments(
        0,
        size(),
        detail::make_plain_segs_iter(
            first, last),
        detail::make_plain_segs_iter(
            first, last));
}

//------------------------------------------------
//
// Element Access
//
//------------------------------------------------

auto
segments::
at(std::size_t i) const ->
    string_value
{
    if(i >= size())
        detail::throw_out_of_range(
            BOOST_CURRENT_LOCATION);
    return (*this)[i];
}

auto
segments::
front() const ->
    string_value
{
    BOOST_ASSERT(! empty());
    return (*this)[0];
}

auto
segments::
back() const ->
    string_value
{
    BOOST_ASSERT(! empty());
    return (*this)[size() - 1];
}

//------------------------------------------------
//
// Iterators
//
//------------------------------------------------

auto
segments::
begin() const noexcept ->
    iterator
{
    return iterator(
        *u_, 0, a_);
}

auto
segments::
end() const noexcept ->
    iterator
{
    return iterator(
        *u_, size(), a_);
}

//------------------------------------------------
//
// Capacity
//
//------------------------------------------------

bool
segments::
empty() const noexcept
{
    return size() == 0;
}

std::size_t
segments::
size() const noexcept
{
    return u_->nseg_;
}

//------------------------------------------------
//
// Modifiers
//
//------------------------------------------------

void
segments::
clear() noexcept
{
    erase(begin(), end());
}

//------------------------------------------------

template<class String>
auto
segments::
insert(
    iterator before,
    String const& s) ->
        typename std::enable_if<
            is_stringlike<String>::value,
            iterator>::type
{
    return insert(before,
        to_string_view(s));
}

template<class String>
auto
segments::
insert(
    iterator before,
    std::initializer_list<String> init) ->
        typename std::enable_if<
            is_stringlike<String>::value,
            iterator>::type
{
    return insert(before,
        init.begin(), init.end());
}

template<class FwdIt>
auto
segments::
insert(
    iterator before,
    FwdIt first,
    FwdIt last) ->
        typename std::enable_if<
            is_stringlike<typename
                std::iterator_traits<
                    FwdIt>::value_type>::value,
            iterator>::type
{
    return insert(before, first, last,
        typename std::iterator_traits<
            FwdIt>::iterator_category{});
}

template<class FwdIt>
auto
segments::
insert(
    iterator before,
    FwdIt first,
    FwdIt last,
    std::forward_iterator_tag) ->
        iterator
{
    u_->edit_segments(
        before.i_,
        before.i_,
        detail::make_plain_segs_iter(
            first, last),
        detail::make_plain_segs_iter(
            first, last));
    return { *u_, before.i_, a_ };
}

//------------------------------------------------

auto
segments::
replace(
    iterator pos,
    string_view s) ->
        iterator
{
    return replace(
        pos, pos + 1,
            &s, &s + 1);
}

template<class String>
auto
segments::
replace(
    iterator pos,
    String const& s) ->
        typename std::enable_if<
            is_stringlike<String>::value,
            iterator>::type
{
    return replace(pos,
        to_string_view(s));
}

template<class String>
auto
segments::
replace(
    iterator from,
    iterator to,
    std::initializer_list<String> init) ->
        typename std::enable_if<
            is_stringlike<String>::value,
            iterator>::type
{
    return replace(
        from,
        to,
        init.begin(),
        init.end());
}

auto
segments::
replace(
    iterator from,
    iterator to,
    std::initializer_list<
        string_view> init) ->
    iterator
{
    return replace(
        from,
        to,
        init.begin(),
        init.end());
}

template<class FwdIt>
auto
segments::
replace(
    iterator from,
    iterator to,
    FwdIt first,
    FwdIt last) ->
        typename std::enable_if<
            is_stringlike<typename
                std::iterator_traits<
                    FwdIt>::value_type>::value,
            iterator>::type
{
    BOOST_ASSERT(from.u_ == u_);
    BOOST_ASSERT(to.u_ == u_);
    u_->edit_segments(
        from.i_,
        to.i_,
        detail::make_plain_segs_iter(
            first, last),
        detail::make_plain_segs_iter(
            first, last));
    return from;
}

//------------------------------------------------

auto
segments::
erase(
    iterator pos) noexcept ->
        iterator
{
    return erase(pos, pos + 1);
}

//------------------------------------------------

void
segments::
push_back(
    string_view s)
{
    insert(end(), s);
}

template<class String>
auto
segments::
push_back(String const& s) ->
    typename std::enable_if<
        is_stringlike<String>::value,
        void>::type
{
    return push_back(
        to_string_view(s));
}

void
segments::
pop_back() noexcept
{
    erase(end() - 1);
}

} // urls
} // boost

#endif
