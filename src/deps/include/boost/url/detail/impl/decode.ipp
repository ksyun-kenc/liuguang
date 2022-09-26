//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_DECODE_IPP
#define BOOST_URL_DETAIL_IMPL_DECODE_IPP

#include <boost/url/detail/decode.hpp>
#include <boost/url/grammar/charset.hpp>
#include <memory>

namespace boost {
namespace urls {
namespace detail {

result<std::size_t>
validate_encoding(
    string_view s,
    std::true_type) noexcept
{
    const auto is_safe = [](char c)
    {
        return c != '%';
    };

    std::size_t pcts = 0;
    char const* it = s.data();
    char const* end = it + s.size();
    it = grammar::find_if_not(it, end, is_safe);
    while (it != end)
    {
        if (end - it < 3)
        {
            // missing HEXDIG
            BOOST_URL_RETURN_EC(
                error::missing_pct_hexdig);
        }
        if (!grammar::hexdig_chars(it[1]) ||
            !grammar::hexdig_chars(it[2]))
        {
            // expected HEXDIG
            BOOST_URL_RETURN_EC(
                error::bad_pct_hexdig);
        }
        it += 3;
        ++pcts;
        it = grammar::find_if_not(it, end, is_safe);
    }
    return s.size() - pcts * 2;
}

result<std::size_t>
validate_encoding(
    string_view s,
    std::false_type) noexcept
{
    const auto is_safe = [](char c)
    {
        return c != '%' && c != '\0';
    };

    std::size_t pcts = 0;
    char const* it = s.data();
    char const* end = it + s.size();
    it = grammar::find_if_not(it, end, is_safe);
    while (it != end)
    {
        if (*it == '\0')
        {
            // null in input
            BOOST_URL_RETURN_EC(
                error::illegal_null);
        }
        if (end - it < 3)
        {
            // missing HEXDIG
            BOOST_URL_RETURN_EC(
                error::missing_pct_hexdig);
        }
        if (!grammar::hexdig_chars(it[1]) ||
            !grammar::hexdig_chars(it[2]))
        {
            // expected HEXDIG
            BOOST_URL_RETURN_EC(
                error::bad_pct_hexdig);
        }
        if (it[1] == '0' &&
            it[2] == '0')
        {
            // null in input
            BOOST_URL_RETURN_EC(
                error::illegal_null);
        }
        it += 3;
        ++pcts;
        it = grammar::find_if_not(it, end, is_safe);
    }
    return s.size() - pcts * 2;
}

result<std::size_t>
validate_encoding(
    string_view s,
    decode_opts const& opt) noexcept
{
    if (opt.allow_null)
        return detail::validate_encoding(
            s, std::true_type{});
    else
       return detail::validate_encoding(
            s, std::false_type{});
}

result<std::size_t>
decode(
    char* dest,
    char const* end,
    string_view s,
    decode_opts const& opt) noexcept
{
    auto const rn =
        detail::validate_encoding(s, opt);
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

std::size_t
decode_bytes_unchecked(
    string_view s) noexcept
{
    auto p = s.begin();
    auto const end = s.end();
    std::size_t dn = 0;
    if(s.size() >= 3)
    {
        auto const safe_end = end - 2;
        while(p < safe_end)
        {
            if(*p != '%')
                p += 1;
            else
                p += 3;
            ++dn;
        }
    }
    dn += end - p;
    return dn;
}

char
decode_one(
    char const* const it) noexcept
{
    auto d0 = grammar::hexdig_value(it[0]);
    auto d1 = grammar::hexdig_value(it[1]);
    return static_cast<char>(
        ((static_cast<
            unsigned char>(d0) << 4) +
        (static_cast<
            unsigned char>(d1))));
}

std::size_t
decode_unchecked(
    char* const dest0,
    char const* end,
    string_view s,
    decode_opts const& opt) noexcept
{
    auto it = s.data();
    auto const last = it + s.size();
    auto dest = dest0;

    if(opt.plus_to_space)
    {
        while(it != last)
        {
            if(dest == end)
            {
                // dest too small
                return dest - dest0;
            }
            if(*it == '+')
            {
                // plus to space
                *dest++ = ' ';
                ++it;
                continue;
            }
            if(*it == '%')
            {
                // escaped
                ++it;
                if(last - it < 2)
                {
                    // missing input,
                    // initialize output
                    std::memset(dest,
                        0, end - dest);
                    return dest - dest0;
                }
                *dest++ = decode_one(it);
                it += 2;
                continue;
            }
            // unescaped
            *dest++ = *it++;
        }
        return dest - dest0;
    }

    while(it != last)
    {
        if(dest == end)
        {
            // dest too small
            return dest - dest0;
        }
        if(*it == '%')
        {
            // escaped
            ++it;
            if(last - it < 2)
            {
                // missing input,
                // initialize output
                std::memset(dest,
                    0, end - dest);
                return dest - dest0;
            }
            *dest++ = decode_one(it);
            it += 2;
            continue;
        }
        // unescaped
        *dest++ = *it++;
    }
    return dest - dest0;
}

} // detail
} // urls
} // boost

#endif
