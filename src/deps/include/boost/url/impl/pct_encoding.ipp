//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_PCT_ENCODING_IPP
#define BOOST_URL_IMPL_PCT_ENCODING_IPP

#include <boost/url/pct_encoding.hpp>
#include <boost/url/bnf/charset.hpp>
#include <memory>

namespace boost {
namespace urls {

std::size_t
pct_decode_bytes_unchecked(
    string_view s) noexcept
{
    auto it = s.data();
    auto const end =
        it + s.size();
    std::size_t n = 0;
    while(it < end)
    {
        if(*it != '%')
        {
            // unescaped
            ++it;
            ++n;
            continue;
        }
        if(end - it < 3)
            return n;
        it += 3;
        ++n;
    }
    return n;
}

std::size_t
pct_decode_unchecked(
    char* const dest0,
    char const* end,
    string_view s,
    pct_decode_opts const& opt) noexcept
{
    auto const decode_hex = [](
        char const* it)
    {
        auto const d0 =
            bnf::hexdig_value(it[0]);
        auto const d1 =
            bnf::hexdig_value(it[1]);
        return static_cast<char>(
            ((static_cast<
                unsigned char>(d0) << 4) +
            (static_cast<
                unsigned char>(d1))));
    };
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
                *dest++ = decode_hex(it);
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
            *dest++ = decode_hex(it);
            it += 2;
            continue;
        }
        // unescaped
        *dest++ = *it++;
    }
    return dest - dest0;
}

} // urls
} // boost

#endif
