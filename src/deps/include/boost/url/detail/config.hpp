//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_CONFIG_HPP
#define BOOST_URL_DETAIL_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <limits.h>

#if CHAR_BIT != 8
# error unsupported platform
#endif

#if defined(BOOST_URL_DOCS)
# define BOOST_URL_DECL
#else
# if (defined(BOOST_URL_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)) && !defined(BOOST_URL_STATIC_LINK)
#  if defined(BOOST_URL_SOURCE)
#   define BOOST_URL_DECL  BOOST_SYMBOL_EXPORT
#   define BOOST_URL_BUILD_DLL
#  else
#   define BOOST_URL_DECL  BOOST_SYMBOL_IMPORT
#  endif
# endif // shared lib
# ifndef  BOOST_URL_DECL
#  define BOOST_URL_DECL
# endif
# if !defined(BOOST_URL_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_URL_NO_LIB)
#  define BOOST_LIB_NAME boost_url
#  if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_URL_DYN_LINK)
#   define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
# endif
#endif

#if ! defined(BOOST_URL_NO_SSE2) && \
    ! defined(BOOST_URL_USE_SSE2)
# if (defined(_M_IX86) && _M_IX86_FP == 2) || \
      defined(_M_X64) || defined(__SSE2__)
#  define BOOST_URL_USE_SSE2
# endif
#endif

// This macro is used for the limits
// test which sets the value lower,
// to exercise code coverage.
//
#ifndef BOOST_URL_MAX_SIZE
// we leave room for a null,
// and still fit in signed-32
#define BOOST_URL_MAX_SIZE 0x7ffffffe
#endif

#if BOOST_WORKAROUND( BOOST_GCC_VERSION, <= 72000 ) || \
    BOOST_WORKAROUND( BOOST_CLANG_VERSION, <= 35000 )
# define BOOST_URL_CONSTEXPR
#else
# define BOOST_URL_CONSTEXPR constexpr
#endif

// Add source location to error codes
#ifdef BOOST_URL_NO_SOURCE_LOCATION
# define BOOST_URL_ERR(ev) (ev)
#else
# define BOOST_URL_ERR(ev) (::boost::system::error_code( (ev), [] { \
         static constexpr auto loc(BOOST_CURRENT_LOCATION); \
         return &loc; }()))
#endif

#endif
