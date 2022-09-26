//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan de Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_DECODE_HPP
#define BOOST_URL_DETAIL_IMPL_DECODE_HPP

#include <boost/url/error.hpp>
#include <boost/url/grammar/charset.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <boost/url/grammar/type_traits.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

namespace boost {
namespace urls {
namespace detail {

template<class CharSet>
result<std::size_t>
validate_encoding(
    string_view s,
    decode_opts const& opt,
    CharSet const& allowed) noexcept
{
    // CharSet must satisfy is_charset
    BOOST_STATIC_ASSERT(
        grammar::is_charset<CharSet>::value);

    // we can't accept plus to space if '+' is not allowed
    BOOST_ASSERT(! opt.plus_to_space || allowed('+'));

    std::size_t n = 0;
    char const* it = s.data();
    char const* end = it + s.size();
    while(it != end)
    {
        if( ! opt.allow_null &&
            *it == '\0')
        {
            // null in input
            BOOST_URL_RETURN_EC(
                error::illegal_null);
        }
        if (*it != '%')
        {
            if(allowed(*it))
            {
                // unreserved
                ++n;
                ++it;
                continue;
            }
            // reserved character in input
            BOOST_URL_RETURN_EC(
                error::illegal_reserved_char);
        }
        else
        {
            // escaped
            ++it;
            if(end - it < 2)
            {
                // missing HEXDIG
                BOOST_URL_RETURN_EC(
                    error::missing_pct_hexdig);
            }
            auto d0 = grammar::hexdig_value(it[0]);
            auto d1 = grammar::hexdig_value(it[1]);
            if( d0 < 0 || d1 < 0)
            {
                // expected HEXDIG
                BOOST_URL_RETURN_EC(
                    error::bad_pct_hexdig);
            }
            it += 2;
            char const c = static_cast<char>(
                ((static_cast<
                    unsigned char>(d0) << 4) +
                (static_cast<
                    unsigned char>(d1))));
            if( ! opt.allow_null &&
                c == '\0')
            {
                // escaped null
                BOOST_URL_RETURN_EC(
                    error::illegal_null);
            }
            if( opt.non_normal_is_error &&
                allowed(c))
            {
                // escaped unreserved char
                BOOST_URL_RETURN_EC(
                    error::non_canonical);
            }
            ++n;
        }
    }
    BOOST_ASSERT(it == end);
    return n;
}

//------------------------------------------------

template<class CharSet>
result<std::size_t>
decode(
    char* dest,
    char const* end,
    string_view s,
    decode_opts const& opt,
    CharSet const& allowed) noexcept
{
    // CharSet must satisfy is_charset
    BOOST_STATIC_ASSERT(
        grammar::is_charset<CharSet>::value);

    auto const rn =
        detail::validate_encoding(
            s, opt, allowed);
    if( !rn )
        return rn;
    auto const n1 =
        detail::decode_unchecked(
            dest, end, s, opt);
    if(n1 < *rn)
    {
        return error::no_space;
    }
    return n1;
}

} // detail
} // urls
} // boost

#endif
