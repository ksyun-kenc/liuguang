//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_SEGMENTS_ENCODED_VIEW_HPP
#define BOOST_URL_IMPL_SEGMENTS_ENCODED_VIEW_HPP

#include <boost/url/detail/except.hpp>
#include <boost/url/detail/pct_encoding.hpp>
#include <boost/url/rfc/paths_bnf.hpp>
#include <cstdint>

namespace boost {
namespace urls {

class segments_encoded_view::iterator
{
    std::size_t i_ = 0;
    string_view s_;
    char const* begin_ = nullptr;
    char const* pos_ = nullptr;
    char const* next_ = nullptr;
    char const* end_ = nullptr;

    friend segments_encoded_view;

    explicit
    iterator(
        string_view s,
        std::size_t nseg) noexcept;

    // end ctor
    iterator(
        string_view s,
        std::size_t nseg,
        int) noexcept;

public:
    using value_type = string_view;
    using reference = string_view;
    using pointer = void const*;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::bidirectional_iterator_tag;

    iterator() noexcept = default;
    iterator(
        iterator const&) noexcept = default;
    iterator& operator=(
        iterator const&) noexcept = default;

    value_type const&
    operator*() const noexcept
    {
        return s_;
    }

    bool
    operator==(
        iterator other) const noexcept
    {
        return
            next_ == other.next_ &&
            end_ == other.end_;
    }

    bool
    operator!=(
        iterator other) const noexcept
    {
        return
            next_ != other.next_ ||
            end_ != other.end_;
    }

    BOOST_URL_DECL
    iterator&
    operator++() noexcept;

    BOOST_URL_DECL
    iterator&
    operator--() noexcept;

    iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    iterator
    operator--(int) noexcept
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }
};

//------------------------------------------------
//
// Members
//
//------------------------------------------------

segments_encoded_view::
segments_encoded_view() noexcept
    : s_("")
    , n_(0)
{
}

template<class Allocator>
segments_view
segments_encoded_view::
decoded(Allocator const& alloc) const
{
    return segments_view(s_, n_, alloc);
}

bool
segments_encoded_view::
is_absolute() const noexcept
{
    return s_.starts_with('/');
}

//------------------------------------------------
//
// Element Access
//
//------------------------------------------------

string_view
segments_encoded_view::
front() const noexcept
{
    BOOST_ASSERT(! empty());
    return *begin();
}

string_view
segments_encoded_view::
back() const noexcept
{
    BOOST_ASSERT(! empty());
    return *--end();
}

//------------------------------------------------
//
// Capacity
//
//------------------------------------------------

bool
segments_encoded_view::
empty() const noexcept
{
    return size() == 0;
}

std::size_t
segments_encoded_view::
size() const noexcept
{
    return n_;
}

} // urls
} // boost

#endif
