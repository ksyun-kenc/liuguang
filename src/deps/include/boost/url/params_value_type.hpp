//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_PARAMS_VALUE_TYPE_HPP
#define BOOST_URL_PARAMS_VALUE_TYPE_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/string.hpp>

namespace boost {
namespace urls {

struct params_value_type
{
    string_view key;
    string_view value;
    bool has_value = false;

#ifdef BOOST_NO_CXX14_AGGREGATE_NSDMI
    constexpr
    params_value_type(
        string_view key_ = {},
        string_view value_ = {},
        bool has_value_ = false ) noexcept
        : key(key_)
        , value(value_)
        , has_value(has_value_)
    {
    }
#endif
};

} // urls
} // boost

#endif
