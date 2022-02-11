//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_PCT_ENCODING_HPP
#define BOOST_URL_DETAIL_PCT_ENCODING_HPP

#include <boost/url/pct_encoding_types.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/url/bnf/charset.hpp>
#include <boost/assert.hpp>
#include <memory>

namespace boost {
namespace urls {
namespace detail {

/** Return true if plain equals a decoded percent-encoded string

    This function compares a plain key to a
    percent-encoded string. The comparison is
    made as if the key were percent-encoded.

    @param plain_key The key to use for comparison.

    @param encoded The percent-encoded string to
        compare to.
*/
BOOST_URL_DECL
bool
key_equal_encoded(
    string_view plain_key,
    pct_encoded_str encoded) noexcept;

BOOST_URL_DECL
bool
key_equal_encoded(
    string_view plain_key,
    string_view encoded) noexcept;

} // detail
} // urls
} // boost

#endif
