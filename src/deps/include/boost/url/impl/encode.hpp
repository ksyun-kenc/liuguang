//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_IMPL_ENCODE_HPP
#define BOOST_URL_IMPL_ENCODE_HPP

#include <boost/url/detail/encode.hpp>
#include <boost/url/detail/except.hpp>
#include <boost/url/encode_opts.hpp>
#include <boost/url/grammar/charset.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <boost/url/grammar/type_traits.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

namespace boost {
namespace urls {

//------------------------------------------------

template<class CharSet>
std::size_t
encoded_size(
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed) noexcept
{
/*  If you get a compile error here, it
    means that the value you passed does
    not meet the requirements stated in
    the documentation.
*/
    static_assert(
        grammar::is_charset<CharSet>::value,
        "Type requirements not met");

    return detail::encoded_size_impl(
        s.data(),
        s.data() + s.size(),
        opt,
        allowed);
}

//------------------------------------------------

template<class CharSet>
std::size_t
encode(
    char* dest,
    char const* const end,
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed)
{
/*  If you get a compile error here, it
    means that the value you passed does
    not meet the requirements stated in
    the documentation.
*/
    static_assert(
        grammar::is_charset<CharSet>::value,
        "Type requirements not met");

    return detail::encode_impl(
        dest,
        end,
        s.data(),
        s.data() + s.size(),
        opt,
        allowed);
}

//------------------------------------------------

template<
    class CharSet,
    class Allocator>
std::basic_string<char,
    std::char_traits<char>,
        Allocator>
encode_to_string(
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed,
    Allocator const& a)
{
    // CharSet must satisfy is_charset
    BOOST_STATIC_ASSERT(
        grammar::is_charset<CharSet>::value);

    std::basic_string<
        char,
        std::char_traits<char>,
        Allocator> r(a);
    if(s.empty())
        return r;
    auto const n =
        encoded_size(s, opt, allowed);
    r.resize(n);
    detail::encode_unchecked(
        &r[0], &r[0] + n, s, opt, allowed);
    return r;
}

} // urls
} // boost

#endif
