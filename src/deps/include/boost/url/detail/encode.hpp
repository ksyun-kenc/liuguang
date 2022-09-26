//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_ENCODE_HPP
#define BOOST_URL_DETAIL_ENCODE_HPP

#include <boost/url/encode_opts.hpp>
#include <boost/url/pct_string_view.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <cstdlib>

namespace boost {
namespace urls {
namespace detail {

//------------------------------------------------

// checked encode
//
// The destination range is enforced
// to ensure no buffer overruns

template<class FwdIt, class CharSet>
std::size_t
encoded_size_impl(
    FwdIt it,
    FwdIt const end,
    encode_opts const& opt,
    CharSet const& allowed) noexcept
{
    std::size_t n = 0;
    if(! opt.space_to_plus)
    {
        while (it != end)
        {
            if (allowed(*it))
                n += 1;
            else
                n += 3;
            ++it;
        }
        return n;
    }
    // If you are converting space
    // to plus, then space should
    // be in the list of reserved
    // characters!
    BOOST_ASSERT(!allowed(' '));
    while (it != end)
    {
        if (*it == ' ')
            ++n;
        else if (allowed(*it))
            ++n;
        else
            n += 3;
        ++it;
    }
    return n;
}

template<
    class FwdIt, 
    class CharSet>
std::size_t
encode_impl(
    char* dest,
    char const* const end,
    FwdIt p,
    FwdIt last,
    encode_opts const& opt,
    CharSet const& allowed)
{
    // Can't have % in charset
    BOOST_ASSERT(! allowed('%'));

    static constexpr char lo[] =
        "0123456789abcdef";
    static constexpr char hi[] =
        "0123456789ABCDEF";
    char const* const hex =
        opt.lower_case ? lo : hi;
    auto const encode = [hex](
        char*& dest,
        unsigned char c) noexcept
    {
        *dest++ = '%';
        *dest++ = hex[c>>4];
        *dest++ = hex[c&0xf];
    };

    auto const dest0 = dest;
    auto const end3 = end - 3;
    if(! opt.space_to_plus)
    {
        // VFALCO we could in theory
        // optimize this with another
        // loop up to safe_last, where
        // safe_last = it + (last/it)/3,
        // and increment safe_last by
        // 2 every time we output an
        // unescaped character

        while(p != last)
        {
            if(allowed(*p))
            {
                if(dest == end)
                    return dest - dest0;
                *dest++ = *p++;
                continue;
            }
            if(dest > end3)
                return dest - dest0;
            encode(dest, *p++);
        }
        return dest - dest0;
    }
    // If you are converting space
    // to plus, then space should
    // be in the list of reserved
    // characters!
    BOOST_ASSERT(! allowed(' '));

    // VFALCO Same note as above
    while(p != last)
    {
        if(allowed(*p))
        {
            if(dest == end)
                return dest - dest0;
            *dest++ = *p++;
            continue;
        }
        if(*p == ' ')
        {
            if(dest == end)
                return dest - dest0;
            *dest++ = '+';
            ++p;
            continue;
        }
        if(dest > end3)
            return dest - dest0;
        encode(dest, *p++);
    }
    return dest - dest0;
}

//------------------------------------------------

// unchecked encode

template<class CharSet>
std::size_t
encode_unchecked(
    char* dest,
    char const* end,
    char const* it,
    char const* const last,
    encode_opts const& opt,
    CharSet const& allowed)
{
    BOOST_ASSERT(! allowed('%'));
    BOOST_ASSERT(
        ! opt.space_to_plus ||
        ! allowed(' '));
    static constexpr char lo[] =
        "0123456789abcdef";
    static constexpr char hi[] =
        "0123456789ABCDEF";
    char const* const hex =
        opt.lower_case ? lo : hi;
    auto const encode = [end](
        char*& dest,
        unsigned char c,
        char const* hex) noexcept
    {
        (void)end;
        *dest++ = '%';
        BOOST_ASSERT(dest != end);
        *dest++ = hex[c>>4];
        BOOST_ASSERT(dest != end);
        *dest++ = hex[c&0xf];
    };
    (void)end;
    auto const dest0 = dest;
    if(! opt.space_to_plus)
    {
        while(it != last)
        {
            BOOST_ASSERT(dest != end);
            if(allowed(*it))
                *dest++ = *it++;
            else
                encode(dest, *it++, hex);
        }
    }
    else
    {
        while(it != last)
        {
            BOOST_ASSERT(dest != end);
            if(allowed(*it))
            {
                *dest++ = *it++;
            }
            else if(*it == ' ')
            {
                *dest++ = '+';
                ++it;
            }
            else
            {
                encode(dest, *it++, hex);
            }
        }
    }
    return dest - dest0;
}

template<class CharSet>
std::size_t
encode_unchecked(
    char* dest,
    char const* const end,
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed)
{
    return encode_unchecked(
        dest,
        end,
        s.begin(),
        s.end(),
        opt,
        allowed);
}

//------------------------------------------------

// re-encode is to percent-encode a
// string that can already contain
// escapes. Characters not in the
// allowed set are escaped, and
// escapes are passed through unchanged.
//
template<class CharSet>
std::size_t
re_encoded_size_unchecked(
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed) noexcept
{
    std::size_t n = 0;
    auto const end = s.end();
    auto it = s.begin();
    if(opt.space_to_plus)
    {
        while(it != end)
        {
            if(*it != '%')
            {
                if( allowed(*it)
                    || *it == ' ')
                    n += 1; 
                else
                    n += 3;
                ++it;
            }
            else
            {
                BOOST_ASSERT(end - it >= 3);
                BOOST_ASSERT(
                    grammar::hexdig_value(
                        it[1]) >= 0);
                BOOST_ASSERT(
                    grammar::hexdig_value(
                        it[2]) >= 0);
                n += 3;
                it += 3;
            }
        }
    }
    else
    {
        while(it != end)
        {
            if(*it != '%')
            {
                if(allowed(*it))
                    n += 1; 
                else
                    n += 3;
                ++it;
            }
            else
            {
                BOOST_ASSERT(end - it >= 3);
                BOOST_ASSERT(
                    grammar::hexdig_value(
                        it[1]) >= 0);
                BOOST_ASSERT(
                    grammar::hexdig_value(
                        it[2]) >= 0);
                n += 3;
                it += 3;
            }
        }
    }
    return n;
}

// unchecked
// returns decoded size
template<class CharSet>
std::size_t
re_encode_unchecked(
    char*& dest_,
    char const* const end,
    string_view s,
    encode_opts const& opt,
    CharSet const& allowed) noexcept
{
    static constexpr char lo[] =
        "0123456789abcdef";
    static constexpr char hi[] =
        "0123456789ABCDEF";
    char const* const hex =
        opt.lower_case ? lo : hi;
    auto const encode = [end](
        char*& dest,
        unsigned char c,
        char const* hex) noexcept
    {
        (void)end;
        *dest++ = '%';
        BOOST_ASSERT(dest != end);
        *dest++ = hex[c>>4];
        BOOST_ASSERT(dest != end);
        *dest++ = hex[c&0xf];
    };
    (void)end;
    auto dest = dest_;
    auto const dest0 = dest;
    auto const last = s.end();
    std::size_t dn = 0;
    auto it = s.begin();
    if(opt.space_to_plus)
    {
        while(it != last)
        {
            BOOST_ASSERT(dest != end);
            if(*it != '%')
            {
                if(*it == ' ')
                {
                    *dest++ = '+';
                }
                else if(allowed(*it))
                {
                    *dest++ = *it;
                }
                else
                {
                    encode(dest, *it, hex);
                    dn += 2;
                }
                ++it;
            }
            else
            {
                *dest++ = *it++;
                BOOST_ASSERT(dest != end);
                *dest++ = *it++;
                BOOST_ASSERT(dest != end);
                *dest++ = *it++;
                dn += 2;
            }
        }
    }
    else
    {
        while(it != last)
        {
            BOOST_ASSERT(dest != end);
            if(*it != '%')
            {
                if(allowed(*it))
                {
                    *dest++ = *it;
                }
                else
                {
                    encode(dest, *it, hex);
                    dn += 2;
                }
                ++it;
            }
            else
            {
                *dest++ = *it++;
                BOOST_ASSERT(dest != end);
                *dest++ = *it++;
                BOOST_ASSERT(dest != end);
                *dest++ = *it++;
                dn += 2;
            }
        }
    }
    dest_ = dest;
    return dest - dest0 - dn;
}

} // detail
} // urls
} // boost

#endif
