//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_REPEAT_HPP
#define BOOST_URL_BNF_REPEAT_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/static_assert.hpp>
#include <cstddef>

namespace boost {
namespace urls {
namespace bnf {

/** BNF for a repeating number of elements

    This rule defines a sequence containing
    at least n and at most m of Element.

    @par BNF
    @code
    sequence        =  <n>*<m>element

    *<m>element     => <0>*<m>element
    <n>*element     => <n>*<inf.>element
    *element        => <0>*<inf.>element
    <n>element      => <n>*<n>element
    [ element ]     => *1( element )
    @endcode

    @see
        https://datatracker.ietf.org/doc/html/rfc5234#section-3.6

    @tparam T The element type to repeat
    @tparam N The minimum number of repetitions, which may be zero
    @tparam M The maximum number of repetitions.
*/
template<
    class T,
    std::size_t N = 0,
    std::size_t M = std::size_t(-1)>
struct repeat
{
    BOOST_STATIC_ASSERT(M > 0);
    BOOST_STATIC_ASSERT(M >= N);

    string_view& v;

    template<
        class T_,
        std::size_t N_,
        std::size_t M_>
    friend
    bool
    parse(
        char const*& it,
        char const* end,
        error_code& ec,
        repeat<T_, N_, M_> const& t);
};

} // bnf
} // urls
} // boost

#include <boost/url/bnf/impl/repeat.hpp>

#endif
