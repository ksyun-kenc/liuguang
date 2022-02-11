//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PARAMS_ENCODED_VIEW_HPP
#define BOOST_URL_IMPL_PARAMS_ENCODED_VIEW_HPP

#include <boost/url/detail/except.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {

//------------------------------------------------

class params_encoded_view::iterator
{
    char const* end_ = nullptr;
    char const* p_ = nullptr;
    std::size_t nk_ = 0;
    std::size_t nv_ = 0;
    bool first_ = true;

    friend class params_encoded_view;

    void scan() noexcept;

    iterator(
        string_view s) noexcept;

    // end
    iterator(
        string_view s,
        int) noexcept;

    string_view
    encoded_key() const noexcept;

public:
    using value_type = params_value_type;
    using reference = params_value_type;
    using pointer = void const*;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::forward_iterator_tag;

    iterator() = default;

    BOOST_URL_DECL
    iterator&
    operator++() noexcept;

    iterator
    operator++(int) noexcept
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    BOOST_URL_DECL
    value_type
    operator*() const;

    BOOST_URL_DECL
    friend
    bool
    operator==(
        iterator a,
        iterator b) noexcept;

    friend
    bool
    operator!=(
        iterator a,
        iterator b) noexcept
    {
        return !(a == b);
    }
};

//------------------------------------------------
//
// Members
//
//------------------------------------------------

params_encoded_view::
params_encoded_view(
    string_view s,
    std::size_t n) noexcept
    : s_(s)
    , n_(n)
{
}

template<class Allocator>
params_view
params_encoded_view::
decoded(Allocator const& alloc) const
{
    return params_view(s_, n_, alloc);
}

//------------------------------------------------
//
// Element Access
//
//------------------------------------------------

template<class Key>
auto
params_encoded_view::
at(Key const& key) const ->
    typename std::enable_if<
        is_stringlike<Key>::value,
        string_view>::type
{
    return at(to_string_view(key));
}

//------------------------------------------------
//
// Capacity
//
//------------------------------------------------

bool
params_encoded_view::
empty() const noexcept
{
    return n_ == 0;
}

std::size_t
params_encoded_view::
size() const noexcept
{
    return n_;
}

//------------------------------------------------
//
// Lookup
//
//------------------------------------------------

template<class Key>
auto
params_encoded_view::
count(Key const& key) const noexcept ->
    typename std::enable_if<
        is_stringlike<Key>::value,
        std::size_t>::type
{
    return count(to_string_view(key));
}

auto
params_encoded_view::
find(string_view key) const noexcept ->
    iterator
{
    return find(begin(), key);
}

template<class Key>
auto
params_encoded_view::
find(Key const& key) const noexcept ->
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
{
    return find(to_string_view(key));
}

template<class Key>
auto
params_encoded_view::
find(
    iterator from,
    Key const& key) const noexcept ->
        typename std::enable_if<
            is_stringlike<Key>::value,
            iterator>::type
{
    return find(from,
        to_string_view(key));
}

bool
params_encoded_view::
contains(string_view key) const noexcept
{
    return find(key) != end();
}

template<class Key>
auto
params_encoded_view::
contains(
    Key const& key) const noexcept ->
    typename std::enable_if<
        is_stringlike<Key>::value,
        bool>::type
{
    return contains(to_string_view(key));
}

} // urls
} // boost

#endif
