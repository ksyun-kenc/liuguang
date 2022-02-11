//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_PCT_ENCODING_TYPES_HPP
#define BOOST_URL_PCT_ENCODING_TYPES_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/string.hpp>
#include <cstddef>

namespace boost {
namespace urls {

/** A valid percent-encoded string
*/
struct pct_encoded_str
{
    /** A string holding the encoded characters
    */
    string_view str;

    /** The number of bytes needed to hold the decoded string
    */
    std::size_t decoded_size = 0;
};

} // urls
} // boost

#endif
