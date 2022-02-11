//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_PARTS_BASE_HPP
#define BOOST_URL_DETAIL_PARTS_BASE_HPP

#include <boost/url/error.hpp>

namespace boost {
namespace urls {
namespace detail {

// mix-in to provide part
// constants and variables
struct parts_base
{
    enum
    {
        id_scheme = -1, // trailing ':'
        id_user,        // leading "//"
        id_pass,        // leading ':', trailing '@'
        id_host,
        id_port,        // leading ':'
        id_path,
        id_query,       // leading '?'
        id_frag,        // leading '#'
        id_end          // one past the end
    };

    static
    constexpr
    pos_t zero_ = 0;

    static
    constexpr
    char const* const empty_ = "";
};

} // detail
} // urls
} // boost

#endif
