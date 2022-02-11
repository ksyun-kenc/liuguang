//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_RANGE_HPP
#define BOOST_URL_BNF_RANGE_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/make_void.hpp>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace boost {
namespace urls {
namespace bnf {

/** Alias for `std::true_type` if T satisfies __Range__
*/
#ifdef BOOST_URL_DOCS
template<class T>
using is_range = __see_below__;
#else
template<class T, class = void>
struct is_range : std::false_type {};

template<class T>
struct is_range<T, boost::void_t<
    decltype(typename T::value_type()), // default ctor
    decltype(
    std::declval<bool&>() =
        T::begin(
            std::declval<char const*&>(),
            std::declval<char const*>(),
            std::declval<error_code&>(),
            std::declval<typename
                std::add_lvalue_reference<
                    typename T::value_type>::type>()),
    std::declval<bool&>() =
        T::increment(
            std::declval<char const*&>(),
            std::declval<char const*>(),
            std::declval<error_code&>(),
            std::declval<typename
                std::add_lvalue_reference<
                    typename T::value_type>::type>())
        ) > >
    : std::true_type
{
};
#endif

//------------------------------------------------

class range
{
    bool(*fp_)(
        char const*&,
        char const*,
        error_code&,
        range&);

    template<class T>
    static
    bool
    parse_impl(
        char const*& it,
        char const* end,
        error_code& ec,
        range& t);

protected:
    template<class T>
    explicit
    range(T const*) noexcept;

public:
    string_view str;
    std::size_t count;

    template<class T>
    class iterator;

    range() noexcept
        : fp_(nullptr)
    {
    }

    friend
    bool
    parse(
        char const*& it,
        char const* end,
        error_code& ec,
        range& t);
};

} // bnf
} // urls
} // boost

#include <boost/url/bnf/impl/range.hpp>

#endif
