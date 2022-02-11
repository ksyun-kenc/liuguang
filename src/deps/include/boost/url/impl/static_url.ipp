//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_STATIC_URL_IPP
#define BOOST_URL_IMPL_STATIC_URL_IPP

#include <boost/url/url_view.hpp>
#include <boost/url/detail/except.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace urls {

static_url_base::
~static_url_base()
{
    // prevent url from
    // deallocating memory
    s_ = nullptr;
}

static_url_base::
static_url_base(
    char* buf,
    std::size_t cap) noexcept
    : url(buf, cap)
{
}

void
static_url_base::
copy(url const& u)
{
    this->url::copy(u);
}

void
static_url_base::
construct(string_view s)
{
    auto r = parse_uri_reference(s);
    if(r.has_error())
        detail::throw_invalid_argument(
            BOOST_CURRENT_LOCATION);
    this->url::operator=(r.value());
}

void
static_url_base::
copy(url_view const& u)
{
    this->url::copy(u);
}

url_view
static_url_base::
convert() const noexcept
{
    return url_view(*this);
}

char*
static_url_base::
allocate(std::size_t n)
{
    (void)n;
    // should never get here
    BOOST_ASSERT(
        n > capacity_in_bytes());
    detail::throw_invalid_argument(
        "alloc_impl",
        BOOST_CURRENT_LOCATION);
}

void
static_url_base::
deallocate(char*)
{
    // should never get here
    detail::throw_invalid_argument(
        "free_impl",
        BOOST_CURRENT_LOCATION);
}

} // urls
} // boost

#endif
