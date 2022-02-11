//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_EXCEPT_HPP
#define BOOST_URL_DETAIL_EXCEPT_HPP

#include <boost/url/error.hpp>
#include <boost/assert/source_location.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace boost {
namespace urls {
namespace detail {

BOOST_URL_DECL void BOOST_NORETURN throw_bad_alloc(source_location const& loc);
BOOST_URL_DECL void BOOST_NORETURN throw_invalid_argument(source_location const& loc);
BOOST_URL_DECL void BOOST_NORETURN throw_invalid_argument(char const* what, source_location const& loc);
BOOST_URL_DECL void BOOST_NORETURN throw_length_error(char const* what, source_location const& loc);
BOOST_URL_DECL void BOOST_NORETURN throw_out_of_range(source_location const& loc);
BOOST_URL_DECL void BOOST_NORETURN throw_system_error(error_code const& ec, source_location const& loc);
//BOOST_URL_DECL void BOOST_NORETURN throw_system_error(error e, source_location const& loc);

inline
void
maybe_throw(
    error_code const& ec,
    source_location const& loc)
{
    if(ec.failed())
        throw_system_error(ec, loc);
}

} // detail
} // urls
} // boost

#endif
