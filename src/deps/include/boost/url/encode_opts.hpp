//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2022 Alan Freitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/url
//

#ifndef BOOST_URL_ENCODE_OPTS_HPP
#define BOOST_URL_ENCODE_OPTS_HPP

namespace boost {
namespace urls {

/** Options for applying percent-encoding to strings.

    Instances of this type may be provided
    to percent-encoding algorithms to
    customize their behavior.

    @see
        @ref encode,
        @ref encoded_size.
*/
struct encode_opts
{
    /** True if space (SP, ' ') encodes into PLUS ('+').

        @par Specification
        @li <a href="https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1">
            application/x-www-form-urlencoded (w3.org)</a>
    */
    bool space_to_plus = false;

    /** True if lower-cased hex digits should be used
    */
    bool lower_case = false;
};

} // urls
} // boost

#endif
