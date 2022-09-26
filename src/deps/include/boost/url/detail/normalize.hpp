//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_DETAIL_NORMALIZED_HPP
#define BOOST_URL_DETAIL_NORMALIZED_HPP

#include <boost/url/string_view.hpp>

namespace boost {
namespace urls {
namespace detail {

class fnv_1a
{
public:
    using digest_type = std::size_t;

    static constexpr std::size_t const prime =
        sizeof(std::size_t) == 4 ?
        static_cast<std::size_t>(0x01000193UL) :
        static_cast<std::size_t>(0x100000001B3ULL);
    static constexpr std::size_t init_hash  =
        sizeof(std::size_t) == 4 ?
        static_cast<std::size_t>(0x811C9DC5UL) :
        static_cast<std::size_t>(0xcbf29ce484222325ULL);

    explicit
    fnv_1a(std::size_t salt) noexcept
    : h_(init_hash + salt)
    {
    }

    void
    put(char c) noexcept
    {
        h_ ^= c;
        h_ *= prime;
    }

    void
    put(string_view s) noexcept
    {
        for (char c: s)
        {
            put(c);
        }
    }

    digest_type
    digest() const noexcept
    {
        return h_;
    }

private:
    std::size_t h_;
};

void
pop_encoded_front(
    string_view& s,
    char& c,
    std::size_t& n) noexcept;

// compare two string_views as if they are both
// percent-decoded
int
compare_encoded(
    string_view lhs,
    string_view rhs) noexcept;

// digest a string_view as if it were
// percent-decoded
void
digest_encoded(
    string_view s,
    fnv_1a& hasher) noexcept;

void
digest(
    string_view s,
    fnv_1a& hasher) noexcept;

// check if string_view lhs starts with string_view
// rhs as if they are both percent-decoded. If
// lhs starts with rhs, return number of chars
// matched in the encoded string_view
std::size_t
path_starts_with(
    string_view lhs,
    string_view rhs) noexcept;

// check if string_view lhs ends with string_view
// rhs as if they are both percent-decoded. If
// lhs ends with rhs, return number of chars
// matched in the encoded string_view
std::size_t
path_ends_with(
    string_view lhs,
    string_view rhs) noexcept;

// compare two string_views as if they are both
// percent-decoded and lowercase
int
ci_compare_encoded(
    string_view lhs,
    string_view rhs) noexcept;

// digest a string_view as if it were decoded
// and lowercase
void
ci_digest_encoded(
    string_view s,
    fnv_1a& hasher) noexcept;

// compare two ascii string_views
int
compare(
    string_view lhs,
    string_view rhs) noexcept;

// compare two string_views as if they are both
// lowercase
int
ci_compare(
    string_view lhs,
    string_view rhs) noexcept;

// digest a string_view as if it were lowercase
void
ci_digest(
    string_view s,
    fnv_1a& hasher) noexcept;

} // detail
} // urls
} // boost

#endif
