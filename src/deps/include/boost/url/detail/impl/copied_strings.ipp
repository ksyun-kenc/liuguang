//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/http_proto
//

#ifndef BOOST_URL_DETAIL_COPIED_STRINGS_IPP
#define BOOST_URL_DETAIL_COPIED_STRINGS_IPP

#include <boost/url/detail/copied_strings.hpp>
#include <functional>

namespace boost {
namespace urls {
namespace detail {

bool
basic_copied_strings::
is_overlapping(
    string_view s) const noexcept
{
    auto const b1 = s_.data();
    auto const e1 = b1 + s_.size();
    auto const b2 = s.data();
    auto const e2 = b2 + s.size();
    auto const less_equal =
        std::less_equal<char const*>();
    if(less_equal(e1, b2))
        return false;
    if(less_equal(e2, b1))
        return false;
    return true;
}

basic_copied_strings::
~basic_copied_strings()
{
    while(dynamic_list_)
    {
        auto p = dynamic_list_;
        dynamic_list_ =
            dynamic_list_->next;
        delete[] p;
    }
}

basic_copied_strings::
basic_copied_strings(
    string_view s,
    char* local_buf,
    std::size_t local_size) noexcept
    : s_(s)
    , local_buf_(local_buf)
    , local_remain_(local_size)
{
}

string_view
basic_copied_strings::
maybe_copy(
    string_view s)
{
    if(! is_overlapping(s))
        return s;
    if(local_remain_ >= s.size())
    {
        std::memcpy(local_buf_,
            s.data(), s.size());
        s = string_view(
            local_buf_, s.size());
        local_buf_ += s.size();
        local_remain_ -= s.size();
        return s;
    }
    auto const n =
        sizeof(dynamic_buf);
    auto p = new dynamic_buf[1 +
        sizeof(n) * ((s.size() +
            sizeof(n) - 1) /
                sizeof(n))];
    std::memcpy(p + 1,
        s.data(), s.size());
    s = string_view(reinterpret_cast<
        char const*>(p + 1), s.size());
    p->next = dynamic_list_;
    dynamic_list_ = p;
    return s;
}

} // detail
} // urls
} // boost

#endif
