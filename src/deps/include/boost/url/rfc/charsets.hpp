//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_RFC_CHARSETS_HPP
#define BOOST_URL_RFC_CHARSETS_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/bnf/charset.hpp>
#include <cstdint>

namespace boost {
namespace urls {

/** Character set for unreserved

    @par BNF
    @code
    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    @endcode
*/
constexpr
bnf::lut_chars
unreserved_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "-._~";

/** Character set for gen-delims

    @par BNF
    @code
    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    @endcode
*/
constexpr
bnf::lut_chars
gen_delim_chars = ":/?#[]@";

/** Character set for sub-delims

    @par BNF
    @code
    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
                  / "*" / "+" / "," / ";" / "="
    @endcode
*/
constexpr
bnf::lut_chars
subdelim_chars = "!$&()*+,;=\x27";

/** Character set for pchar

    @par BNF
    @code
    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    @endcode
*/
constexpr
bnf::lut_chars
pchars = unreserved_chars + subdelim_chars + ':' + '@';

/** Character set for query

    @par BNF
    @code
    query         = *( pchar / "/" / "?" )
    @endcode
*/
constexpr
bnf::lut_chars
query_chars = pchars + '/' + '?';

/** Character set for fragment

    @par BNF
    @code
    fragment      = *( pchar / "/" / "?" )
    @endcode
*/
constexpr
bnf::lut_chars
fragment_chars = pchars + '/' + '?';

} // urls
} // boost

#endif
