//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_AUTHORITY_VIEW_HPP
#define BOOST_URL_IMPL_AUTHORITY_VIEW_HPP

namespace boost {
namespace urls {

// return length of [first, last)
auto
authority_view::
len(
    int first,
    int last) const noexcept ->
        pos_t
{
    BOOST_ASSERT(first <= last);
    BOOST_ASSERT(last <= id_end);
    return offset(last) - offset(first);
}

// change id to size n
void
authority_view::
set_size(
    int id,
    pos_t n) noexcept
{
    auto d = n - len(id);
    for(auto i = id;
        i <= id_end; ++i)
        offset_[i] += d;
}

} // urls
} // boost

#endif