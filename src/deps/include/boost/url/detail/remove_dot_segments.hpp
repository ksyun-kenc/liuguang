//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_REMOVE_DOT_SEGMENTS_HPP
#define BOOST_URL_DETAIL_REMOVE_DOT_SEGMENTS_HPP

#include <boost/url/string_view.hpp>
#include <boost/url/detail/normalize.hpp>
#include <cstdint>

namespace boost {
namespace urls {
namespace detail {

BOOST_URL_DECL
std::size_t
remove_dot_segments(
    char* dest,
    char const* end,
    string_view s) noexcept;

void
pop_last_segment(
    string_view& s,
    string_view& c,
    std::size_t& level,
    bool r) noexcept;

char
path_pop_back( string_view& s );

int
normalized_path_compare(
    string_view lhs,
    string_view rhs,
    bool remove_unmatched_lhs,
    bool remove_unmatched_rhs) noexcept;

void
normalized_path_digest(
    string_view s,
    bool remove_unmatched,
    fnv_1a& hasher) noexcept;

} // detail
} // urls
} // boost

#endif
