//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_URL_VIEW_HPP
#define BOOST_URL_IMPL_URL_VIEW_HPP

namespace boost {
namespace urls {

url_view&
url_view::
base() noexcept
{
    return *this;
}
    
url_view const&
url_view::
base() const noexcept
{
    return *this;
}

// return size of table in bytes
std::size_t
url_view::
table_bytes() const noexcept
{
    std::size_t n = 0;
    if(nseg_ > 1)
        n += nseg_ - 1;
    if(nparam_ > 1)
        n += nparam_ - 1;
    return n * sizeof(pos_t);
}

// return length of [first, last)
auto
url_view::
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
url_view::
set_size(
    int id,
    pos_t n) noexcept
{
    auto d = n - len(id);
    for(auto i = id + 1;
        i <= id_end; ++i)
        offset_[i] += d;
}

// trim id to size n,
// moving excess into id+1
void
url_view::
split(
    int id,
    std::size_t n) noexcept
{
    BOOST_ASSERT(id < id_end - 1);
    //BOOST_ASSERT(n <= len(id));
    offset_[id + 1] = offset(id) + n;
}

// add n to [first, last]
void
url_view::
adjust(
    int first,
    int last,
    std::size_t n) noexcept
{
    for(int i = first;
            i <= last; ++i)
        offset_[i] += n;
}

// set [first, last) offset
void
url_view::
collapse(
    int first,
    int last,
    std::size_t n) noexcept
{
    for(int i = first + 1;
            i < last; ++i)
        offset_[i] = n;
}

//------------------------------------------------
//
// Query
//
//------------------------------------------------

template<class Allocator>
params_view
url_view::
params(
    Allocator const& a) const noexcept
{
    auto s = get(id_query);
    if(s.empty())
        return params_view(
            s, 0, a);
    BOOST_ASSERT(s[0] == '?');
    s.remove_prefix(1);
    return params_view(
        s, nparam_, a);
}

} // urls
} // boost

#endif