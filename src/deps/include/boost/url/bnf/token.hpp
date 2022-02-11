//
// Copyright (c) 2016-2019 Damian Jarek (damian dot jarek93 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_URL_BNF_TOKEN_HPP
#define BOOST_URL_BNF_TOKEN_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/string.hpp>

namespace boost {
namespace urls {
namespace bnf {

#ifndef BOOST_URL_DOCS
namespace detail {
template<class CharSet>
struct token;
} // detail
#endif

/** BNF for a series of characters in a char set
*/
template<class CharSet>
detail::token<CharSet>
token(
    CharSet const& cs,
    string_view& t) noexcept;

} // bnf
} // urls
} // boost

#include <boost/url/bnf/impl/token.hpp>

#endif