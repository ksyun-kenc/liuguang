//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/http_proto
//

#ifndef BOOST_URL_DETAIL_COPIED_STRINGS_HPP
#define BOOST_URL_DETAIL_COPIED_STRINGS_HPP

#include <boost/url/string.hpp>

namespace boost {
namespace urls {
namespace detail {

// Makes copies of string_view parameters as
// needed when the storage for the parameters
// overlap the container being modified.
class basic_copied_strings
{
    struct dynamic_buf
    {
        dynamic_buf* next;
    };

    string_view s_;
    char* local_buf_;
    std::size_t local_remain_;
    dynamic_buf* dynamic_list_ = nullptr;

    bool
    is_overlapping(
        string_view s) const noexcept;

public:
    BOOST_URL_DECL
    ~basic_copied_strings();

    BOOST_URL_DECL
    basic_copied_strings(
        string_view s,
        char* local_buf,
        std::size_t local_size) noexcept;

    BOOST_URL_DECL
    string_view
    maybe_copy(
        string_view s);
};

class copied_strings
    : public basic_copied_strings
{
    char buf_[4096];

public:
    copied_strings(
        string_view s)
        : basic_copied_strings(
            s, buf_, sizeof(buf_))
    {
    }
};

} // detail
} // urls
} // boost

#endif
