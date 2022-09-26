//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_DECODE_HPP
#define BOOST_URL_DETAIL_DECODE_HPP

#include <boost/url/decode_opts.hpp>
#include <boost/url/error_types.hpp>
#include <boost/url/string_view.hpp>
#include <cstdlib>

namespace boost {
namespace urls {
namespace detail {

template<
    class CharSet>
result<std::size_t>
validate_encoding(
    string_view s,
    decode_opts const& opt,
    CharSet const& allowed) noexcept;

BOOST_URL_DECL
result<std::size_t>
validate_encoding(
    string_view s,
    decode_opts const& opt = {}) noexcept;

template<
    class CharSet>
result<std::size_t>
decode(
    char* dest,
    char const* end,
    string_view s,
    decode_opts const& opt,
    CharSet const& allowed) noexcept;

BOOST_URL_DECL
result<std::size_t>
decode(
    char* dest,
    char const* end,
    string_view s,
    decode_opts const& opt = {}) noexcept;

BOOST_URL_DECL
std::size_t
decode_bytes_unchecked(
    string_view s) noexcept;

BOOST_URL_DECL
std::size_t
decode_unchecked(
    char* dest,
    char const* end,
    string_view s,
    decode_opts const& opt = {}) noexcept;

BOOST_URL_DECL
char
decode_one(
    char const* it) noexcept;

} // detail
} // urls
} // boost

#include <boost/url/detail/impl/decode.hpp>

#endif
