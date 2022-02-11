//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_IMPL_PARSE_HPP
#define BOOST_URL_BNF_IMPL_PARSE_HPP

#include <boost/url/detail/except.hpp>
#include <utility>

namespace boost {
namespace urls {
namespace bnf {

bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    char ch) noexcept
{
    if(it == end)
    {
        // end of input
        ec = BOOST_URL_ERR(
            error::incomplete);
        return false;
    }
    if(*it != ch)
    {
        // expected ch
        ec = BOOST_URL_ERR(
            error::wrong_char_literal);
        return false;
    }
    ++it;
    return true;
}
 
template<class T>
bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    optional<T>& t)
{
    t.emplace();
    if(parse(it, end, ec, *t))
        return true;
    ec = {};
    t.reset();
    return true;
}

bool
parse(
    char const*&,
    char const*,
    error_code&,
    optional<char> const&) = delete;

template<
    class T0,
    class T1,
    class... Tn>
bool
parse(
    char const*& it,
    char const* const end,
    error_code& ec,
    T0&& t0,
    T1&& t1,
    Tn&&... tn)
{
    if(! parse(
        it, end, ec,
        std::forward<T0>(t0)))
        return false;
    if(! parse(
        it, end, ec,
        std::forward<T1>(t1),
        std::forward<Tn>(tn)...))
        return false;
    return true;
}

template<
    class T0,
    class... Tn>
bool
parse_string(
    string_view s,
    error_code& ec,
    T0&& t0,
    Tn&&... tn)
{
    auto it = s.data();
    auto const end =
        it + s.size();
    if(! parse(it, end, ec,
        std::forward<T0>(t0),
        std::forward<Tn>(tn)...))
        return false;
    if(it != end)
    {
        // input not consumed fully
        ec = BOOST_URL_ERR(
            error::leftover_input);
        return false;
    }
    return true;
}

template<
    class T0,
    class... Tn>
void
parse_string(
    string_view s,
    T0&& t0,
    Tn&&... tn)
{
    error_code ec;
    if(parse_string(s, ec,
        std::forward<T0>(t0),
        std::forward<Tn>(tn)...))
    {
        BOOST_ASSERT(! ec.failed());
        return;
    }
    urls::detail::throw_system_error(
        ec, BOOST_CURRENT_LOCATION);
}

} // bnf
} // urls
} // boost

#endif
