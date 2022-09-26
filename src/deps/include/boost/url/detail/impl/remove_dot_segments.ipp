//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_IMPL_REMOVE_DOT_SEGMENTS_IPP
#define BOOST_URL_DETAIL_IMPL_REMOVE_DOT_SEGMENTS_IPP

#include <boost/url/detail/remove_dot_segments.hpp>
#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>
#include <cstring>

namespace boost {
namespace urls {
namespace detail {

std::size_t
remove_dot_segments(
    char* dest0,
    char const* end,
    string_view s) noexcept
{
    // 1. The input buffer `s` is initialized with
    // the now-appended path components and the
    // output buffer `dest0` is initialized to
    // the empty string.
    char* dest = dest0;

    // Step 2 is a loop through 5 production rules:
    // https://www.rfc-editor.org/rfc/rfc3986#section-5.2.4
    //
    // There are no transitions between all rules,
    // which enables some optimizations.
    //
    // Initial:
    // - Rule A: handle initial dots
    // If the input buffer begins with a
    // prefix of "../" or "./", then remove
    // that prefix from the input buffer.
    // Rule A can only happen at the beginning.
    // Errata 4547: Keep "../" in the beginning
    // https://www.rfc-editor.org/errata/eid4547
    //
    // Then:
    // - Rule D: ignore a final ".." or "."
    // if the input buffer consists only  of "."
    // or "..", then remove that from the input
    // buffer.
    // Rule D can only happen after Rule A because:
    // - B and C write "/" to the input
    // - E writes "/" to input or returns
    //
    // Then:
    // - Rule B: ignore ".": write "/" to the input
    // - Rule C: apply "..": remove seg and write "/"
    // - Rule E: copy complete segment
    auto append =
        [](char*& first, char const* last, string_view in)
    {
        // append `in` to `dest`
        BOOST_ASSERT(in.size() <= std::size_t(last - first));
        std::memmove(first, in.data(), in.size());
        first += in.size();
        ignore_unused(last);
    };

    auto dot_starts_with = [](
        string_view str, string_view dots, std::size_t& n)
    {
        // starts_with for encoded/decoded dots
        // or decoded otherwise. return how many
        // chars in str match the dots
        n = 0;
        for (char c: dots)
        {
            if (str.empty())
            {
                n = 0;
                return false;
            }
            else if (str.starts_with(c))
            {
                str.remove_prefix(1);
                ++n;
            }
            else if (str.size() > 2 &&
                     str[0] == '%' &&
                     str[1] == '2' &&
                     (str[2] == 'e' ||
                      str[2] == 'E'))
            {
                str.remove_prefix(3);
                n += 3;
            }
            else
            {
                n = 0;
                return false;
            }
        }
        return true;
    };

    auto dot_equal = [&dot_starts_with](
        string_view str, string_view dots)
    {
        std::size_t n = 0;
        dot_starts_with(str, dots, n);
        return n == str.size();
    };

    // Rule A
    std::size_t n;
    while (!s.empty())
    {
        if (dot_starts_with(s, "../", n))
        {
            // Errata 4547
            append(dest, end, "../");
            s.remove_prefix(n);
            continue;
        }
        else if (!dot_starts_with(s, "./", n))
        {
            break;
        }
        s.remove_prefix(n);
    }

    // Rule D
    if( dot_equal(s, "."))
    {
        s = {};
    }
    else if( dot_equal(s, "..") )
    {
        // Errata 4547
        append(dest, end, "..");
        s = {};
    }

    // 2. While the input buffer is not empty,
    // loop as follows:
    while (!s.empty())
    {
        // Rule B
        if (dot_starts_with(s, "/./", n))
        {
            s.remove_prefix(n - 1);
            continue;
        }
        if (dot_equal(s, "/."))
        {
            // We can't remove "." from a string_view
            // So what we do here is equivalent to
            // replacing s with '/' as required
            // in Rule B and executing the next
            // iteration, which would append this
            // '/' to  the output, as required by
            // Rule E
            append(dest, end, s.substr(0, 1));
            s = {};
            break;
        }

        // Rule C
        if (dot_starts_with(s, "/../", n))
        {
            std::size_t p = string_view(
                dest0, dest - dest0).find_last_of('/');
            if (p != string_view::npos)
            {
                // output has multiple segments
                // "erase" [p, end] if not "/.."
                string_view last_seg(dest0 + p, dest - (dest0 + p));
                if (!dot_equal(last_seg, "/.."))
                    dest = dest0 + p;
                else
                    append(dest, end, "/..");
            }
            else if (dest0 != dest)
            {
                // one segment in the output
                dest = dest0;
                s.remove_prefix(1);
            }
            else
            {
                // output is empty
                append(dest, end, "/..");
            }
            s.remove_prefix(n-1);
            continue;
        }
        if (dot_equal(s, "/.."))
        {
            std::size_t p = string_view(
                dest0, dest - dest0).find_last_of('/');
            if (p != string_view::npos)
            {
                // erase [p, end]
                dest = dest0 + p;
                append(dest, end, "/");
            }
            else if (dest0 != dest)
            {
                dest = dest0;
            }
            else
            {
                append(dest, end, "/..");
            }
            s = {};
            break;
        }

        // Rule E
        std::size_t p = s.find_first_of('/', 1);
        if (p != string_view::npos)
        {
            append(dest, end, s.substr(0, p));
            s.remove_prefix(p);
        }
        else
        {
            append(dest, end, s);
            s = {};
        }
    }

    // 3. Finally, the output buffer is set
    // as the result of remove_dot_segments,
    // and we return its size
    return dest - dest0;
}

char
path_pop_back( string_view& s )
{
    if (s.size() < 3 ||
        *std::prev(s.end(), 3) != '%')
    {
        char c = s.back();
        s.remove_suffix(1);
        return c;
    }
    char c = 0;
    detail::decode_unchecked(
        &c, &c + 1, s.substr(s.size() - 3));
    if (c != '/')
    {
        s.remove_suffix(3);
        return c;
    }
    c = s.back();
    s.remove_suffix(1);
    return c;
};

void
pop_last_segment(
    string_view& s,
    string_view& c,
    std::size_t& level,
    bool r) noexcept
{
    c = {};
    std::size_t n = 0;
    while (!s.empty())
    {
        // B.  if the input buffer begins with a
        // prefix of "/./" or "/.", where "." is
        // a complete path segment, then replace
        // that prefix with "/" in the input
        // buffer; otherwise,
        n = detail::path_ends_with(s, "/./");
        if (n)
        {
            c = s.substr(s.size() - n);
            s.remove_suffix(n);
            continue;
        }
        n = detail::path_ends_with(s, "/.");
        if (n)
        {
            c = s.substr(s.size() - n, 1);
            s.remove_suffix(n);
            continue;
        }

        // C. if the input buffer begins with a
        // prefix of "/../" or "/..", where ".."
        // is a complete path segment, then
        // replace that prefix with "/" in the
        // input buffer and remove the last
        // segment and its preceding "/"
        // (if any) from the output buffer
        // otherwise,
        n = detail::path_ends_with(s, "/../");
        if (n)
        {
            c = s.substr(s.size() - n);
            s.remove_suffix(n);
            ++level;
            continue;
        }
        n = detail::path_ends_with(s, "/..");
        if (n)
        {
            c = s.substr(s.size() - n);
            s.remove_suffix(n);
            ++level;
            continue;
        }

        // E.  move the first path segment in the
        // input buffer to the end of the output
        // buffer, including the initial "/"
        // character (if any) and any subsequent
        // characters up to, but not including,
        // the next "/" character or the end of
        // the input buffer.
        std::size_t p = s.size() > 1
            ? s.find_last_of('/', s.size() - 2)
            : string_view::npos;
        if (p != string_view::npos)
        {
            c = s.substr(p + 1);
            s.remove_suffix(c.size());
        }
        else
        {
            c = s;
            s = {};
        }

        if (level == 0)
            return;
        if (!s.empty())
            --level;
    }
    // we still need to skip n_skip + 1
    // but the string is empty
    if (r && level)
    {
        c = "/";
        level = 0;
        return;
    }
    else if (level)
    {
        if (c.empty())
            c = "/..";
        else
            c = "/../";
        --level;
        return;
    }
    c = {};
}


int
normalized_path_compare(
    string_view s0_init,
    string_view s1_init,
    bool r0,
    bool r1) noexcept
{
    // Pseudocode:
    // Execute remove_dot_segments iterations in reverse:
    // - keep track of number of elements
    // - keep track of normalized size
    // Iterate the both path segments in reverse again:
    // - use normalized size to identify the
    //   positions we are comparing

    // 1. The input buffer is initialized with
    // the now-appended path components and the
    // output buffer is initialized to the empty
    // string.
    // - These memory allocations are logically removed.

    // Step 2 is a loop through 5 production rules
    // There are no transitions between all rules,
    // which enables some optimizations, such as
    // ignoring the prefix rules before applying
    // other rules.
    auto remove_prefix =
        []( string_view& s )
    {
        // A.  If the input buffer begins with a
        // prefix of "../" or "./", then remove
        // that prefix from the input buffer,
        // otherwise,
        // Rule A can only happen at the beginning:
        // - B and C write "/" to the output
        // - D can only happen at the end
        // - E leaves "/" or happens at the end
        std::size_t out = 0;
        std::size_t n = 0;
        while (!s.empty())
        {
            n = detail::path_starts_with(s, "../");
            if (n)
            {
                out += 3;
                s.remove_prefix(n);
                continue;
            }
            n = detail::path_starts_with(s, "./");
            if (n)
            {
                s.remove_prefix(n);
                continue;
            }
            break;
        }

        // D.  if the input buffer consists only
        // of "." or "..", then remove that from
        // the input buffer; otherwise, ...
        // Rule D can only happen after A is consumed:
        // - B and C write "/" to the output
        // - D can only happen at the end
        // - E leaves "/" or happens at the end
        if (detail::compare_encoded(s, ".") == 0)
            s = {};
        else if (detail::compare_encoded(s, "..") == 0)
        {
            out += 2;
            s = {};
        }
        return out;
    };
    string_view s0 = s0_init;
    string_view s1 = s1_init;
    std::size_t s0_prefix_n = remove_prefix(s0);
    std::size_t s1_prefix_n = remove_prefix(s1);

    // number of decoded bytes in a path segment
    auto path_decoded_bytes =
        []( string_view s )
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
            char c = 0;
            detail::decode_unchecked(
                &c,
                &c + 1,
                string_view(it, 3));
            if (c != '/')
                it += 3;
            else
                ++it;
            ++n;
        }
        return n;
    };

    // Calculate the normalized size
    auto norm_bytes =
        [&path_decoded_bytes]
        ( string_view p,
          bool r)
    {
        string_view c;
        std::size_t s{0};
        std::size_t n{0};
        do
        {
            pop_last_segment(p, c, s, r);
            n += path_decoded_bytes(c);
        }
        while (!c.empty());
        return n;
    };
    std::size_t s0n = norm_bytes(
        s0,
        r0);
    if (!r0)
        s0n += s0_prefix_n;

    std::size_t s1n = norm_bytes(
        s1,
        r1);
    if (!r1)
        s1n += s1_prefix_n;

    // Remove child segments until last intersection
    s0 = s0_init;
    s1 = s1_init;
    string_view s0c;
    string_view s1c;
    std::size_t s0l = 0;
    std::size_t s1l = 0;
    std::size_t s0i = s0n;
    std::size_t s1i = s1n;
    pop_last_segment(
        s0, s0c, s0l,
        r0);
    pop_last_segment(
        s1, s1c, s1l,
        r1);

    // Consume incomparable segments
    auto pop_decoded_back =
        []( string_view& s )
    {
        if (s.size() < 3 ||
            *std::prev(s.end(), 3) != '%')
        {
            char c = s.back();
            s.remove_suffix(1);
            return c;
        }
        char c = 0;
        detail::decode_unchecked(
            &c, &c + 1, s.substr(s.size() - 3));
        if (c != '/')
        {
            s.remove_suffix(3);
            return c;
        }
        c = s.back();
        s.remove_suffix(1);
        return c;
    };

    while (s0i != s1i)
    {
        // Consume more child segments
        if (s0c.empty())
            pop_last_segment(
                s0, s0c, s0l, r0);
        if (s1c.empty())
            pop_last_segment(
                s1, s1c, s1l, r1);

        // Remove incomparable suffix
        while (
            !s0c.empty() &&
            !s1c.empty())
        {
            if (s1i > s0i)
            {
                pop_decoded_back(s1c);
                --s1i;
                continue;
            }
            else if (s0i > s1i)
            {
                pop_decoded_back(s0c);
                --s0i;
                continue;
            }
            break;
        }
    }

    int cmp = 0;
    BOOST_ASSERT(s0i == s1i);
    while (s0i > 0)
    {
        // Consume more child segments
        if (s0c.empty())
            pop_last_segment(
                s0, s0c, s0l, r0);
        if (s1c.empty())
            pop_last_segment(
                s1, s1c, s1l, r1);

        // Compare intersection
        while (
            !s0c.empty() &&
            !s1c.empty())
        {
            BOOST_ASSERT(s0i == s1i);
            char c0 = pop_decoded_back(s0c);
            char c1 = pop_decoded_back(s1c);
            if (c0 < c1)
                cmp = -1;
            else if (c1 < c0)
                cmp = 1;
            --s0i;
            --s1i;
        }
    }

    if (cmp != 0)
        return cmp;
    if (s0n == s1n )
        return 0;
    if (s0n < s1n )
        return -1;
    return 1;
}

void
normalized_path_digest(
    string_view s,
    bool remove_unmatched,
    fnv_1a& hasher) noexcept
{
    string_view child;
    std::size_t level = 0;
    do
    {
        pop_last_segment(
            s, child, level, remove_unmatched);
        while (!child.empty())
        {
            char c = path_pop_back(child);
            hasher.put(c);
        }
    }
    while (!s.empty());
}

} // detail
} // urls
} // boost

#endif
