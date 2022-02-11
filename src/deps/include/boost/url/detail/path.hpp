//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_PATH_HPP
#define BOOST_URL_DETAIL_PATH_HPP

#include <boost/url/string.hpp>

namespace boost {
namespace urls {
namespace detail {

// Return the number of characters at
// the front of the path that are reserved
inline
std::size_t
path_prefix(
    string_view s) noexcept
{
    switch(s.size())
    {
    case 0:
        return 0;

    case 1:
        if(s[0] == '/')
            return 1;
        return 0;

    case 2:
        if(s[0] == '/')
            return 1;
        if( s[0] == '.' &&
            s[1] == '/')
            return 2;
        return 0;

    default:
        if(s[0] == '/')
        {
            if( s[1] == '.' &&
                s[2] == '/')
                return 3;
            return 1;
        }
        if( s[0] == '.' &&
            s[1] == '/')
            return 2;
        break;
    }
    return 0;
}

// returns the number of adjusted
// segments based on the malleable prefix.
inline
std::size_t
path_segments(
    string_view s,
    std::size_t nseg) noexcept
{
    switch(s.size())
    {
    case 0:
        BOOST_ASSERT(nseg == 0);
        return 0;

    case 1:
        BOOST_ASSERT(nseg == 1);
        if(s[0] == '/')
            return 0;
        return 1;

    case 2:
        if(s[0] == '/')
            return nseg;
        if( s[0] == '.' &&
            s[1] == '/')
        {
            BOOST_ASSERT(nseg > 1);
            return nseg - 1;
        }
        return nseg;

    default:
        if(s[0] == '/')
        {
            if( s[1] == '.' &&
                s[2] == '/')
            {
                BOOST_ASSERT(nseg > 1);
                return nseg - 1;
            }
            return nseg;
        }
        if( s[0] == '.' &&
            s[1] == '/')
        {
            BOOST_ASSERT(nseg > 1);
            return nseg - 1;
        }
        break;
    }
    return nseg;
}

// Trim reserved characters from
// the front of the path.
inline
string_view
clean_path(
    string_view s) noexcept
{
    s.remove_prefix(
        path_prefix(s));
    return s;
}

} // detail
} // urls
} // boost

#endif