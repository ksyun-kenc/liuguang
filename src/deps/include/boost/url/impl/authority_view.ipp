//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_IMPL_AUTHORITY_VIEW_IPP
#define BOOST_URL_IMPL_AUTHORITY_VIEW_IPP

#include <boost/url/authority_view.hpp>
#include <boost/url/error.hpp>
#include <boost/url/detail/over_allocator.hpp>
#include <boost/url/bnf/parse.hpp>
#include <boost/url/rfc/authority_bnf.hpp>
#include <boost/url/rfc/host_bnf.hpp>
#include <boost/url/rfc/pct_encoded_bnf.hpp>
#include <array>
#include <ostream>

namespace boost {
namespace urls {

struct authority_view::shared_impl :
    authority_view
{
    virtual
    ~shared_impl()
    {
    }

    shared_impl(
        authority_view const& u) noexcept
        : authority_view(u, reinterpret_cast<
            char const*>(this + 1))
    {
    }
};

authority_view::
authority_view(
    char const* cs) noexcept
    : cs_(cs)
{
}

authority_view::
authority_view(
    authority_view const& u,
    char const* cs) noexcept
    : authority_view(u)
{
    cs_ = cs;
}

//------------------------------------------------

authority_view::
~authority_view()
{
}

authority_view::
authority_view() noexcept = default;

authority_view::
authority_view(
    authority_view const&) noexcept = default;

authority_view&
authority_view::
operator=(
    authority_view const&) noexcept = default;

//------------------------------------------------
//
// Observers
//
//------------------------------------------------

std::shared_ptr<
    authority_view const>
authority_view::
collect() const
{
    using T = shared_impl;
    using Alloc = std::allocator<char>;
    Alloc a;
    auto p = std::allocate_shared<T>(
        detail::over_allocator<T, Alloc>(
            size(), a), *this);
    std::memcpy(
        reinterpret_cast<char*>(
            p.get() + 1), data(), size());
    return p;
}

//----------------------------------------------------------
//
// Authority
//
//----------------------------------------------------------

// userinfo

bool
authority_view::
has_userinfo() const noexcept
{
    auto n = len(id_pass);
    if(n == 0)
        return false;
    BOOST_ASSERT(get(
        id_pass).ends_with('@'));
    return true;
}

string_view
authority_view::
encoded_userinfo() const noexcept
{
    auto s = get(
        id_user, id_host);
    if(s.empty())
        return s;
    if(s.empty())
        return s;
    BOOST_ASSERT(
        s.ends_with('@'));
    s.remove_suffix(1);
    return s;
}

bool
authority_view::
has_password() const noexcept
{
    auto const n = len(id_pass);
    if(n > 1)
    {
        BOOST_ASSERT(get(id_pass
            ).starts_with(':'));
        BOOST_ASSERT(get(id_pass
            ).ends_with('@'));
        return true;
    }
    BOOST_ASSERT(n == 0 || get(
        id_pass).ends_with('@'));
    return false;
}

string_view
authority_view::
encoded_password() const noexcept
{
    auto s = get(id_pass);
    switch(s.size())
    {
    case 1:
        BOOST_ASSERT(
            s.starts_with('@'));
        BOOST_FALLTHROUGH;
    case 0:
        return s.substr(0, 0);
    default:
        break;
    }
    BOOST_ASSERT(
        s.ends_with('@'));
    BOOST_ASSERT(
        s.starts_with(':'));
    return s.substr(1,
        s.size() - 2);
}

// host

string_view
authority_view::
encoded_host() const noexcept
{
    return get(id_host);
}

urls::ipv4_address
authority_view::
ipv4_address() const noexcept
{
    if(host_type_ !=
        urls::host_type::ipv4)
        return {};
    std::array<
        unsigned char, 4> bytes;
    std::memcpy(
        &bytes[0],
        &ip_addr_[0], 4);
    return urls::ipv4_address(
        bytes);
}

urls::ipv6_address
authority_view::
ipv6_address() const noexcept
{
    if(host_type_ ==
        urls::host_type::ipv6)
    {
        std::array<
            unsigned char, 16> bytes;
        std::memcpy(
            &bytes[0],
            &ip_addr_[0], 16);
        return urls::ipv6_address(
            bytes);
    }
    return urls::ipv6_address();
}

string_view
authority_view::
ipv_future() const noexcept
{
    if(host_type_ ==
        urls::host_type::ipvfuture)
        return get(id_host);
    return {};
}

// port

bool
authority_view::
has_port() const noexcept
{
    auto const n = len(id_port);
    if(n == 0)
        return false;
    BOOST_ASSERT(
        get(id_port).starts_with(':'));
    return true;
}

string_view
authority_view::
port() const noexcept
{
    auto s = get(id_port);
    if(s.empty())
        return s;
    BOOST_ASSERT(has_port());
    return s.substr(1);
}

std::uint16_t
authority_view::
port_number() const noexcept
{
    BOOST_ASSERT(
        has_port() ||
        port_number_ == 0);
    return port_number_;
}

string_view
authority_view::
encoded_host_and_port() const noexcept
{
    return get(id_host, id_end);
}

//------------------------------------------------
//
// Parsing
//
//------------------------------------------------

void
authority_view::
apply(
    host_bnf const& t) noexcept
{
    host_type_ = t.host_type;
    switch(t.host_type)
    {
    default:
    case urls::host_type::none:
    {
        break;
    }
    case urls::host_type::name:
    {
        decoded_[id_host] =
            t.name.decoded_size;
        break;
    }
    case urls::host_type::ipv4:
    {
        auto const bytes =
            t.ipv4.to_bytes();
        std::memcpy(
            &ip_addr_[0],
            bytes.data(), 4);
        break;
    }
    case urls::host_type::ipv6:
    {
        auto const bytes =
            t.ipv6.to_bytes();
        std::memcpy(
            &ip_addr_[0],
            bytes.data(), 16);
        break;
    }
    case urls::host_type::ipvfuture:
    {
        break;
    }
    }

    if(t.host_type !=
        urls::host_type::none)
    {
        set_size(
            id_host,
            t.host_part.size());
    }
}

void
authority_view::
apply(
    authority_bnf const& t) noexcept
{
    if(t.has_userinfo)
    {
        auto const& u = t.userinfo;

        set_size(
            id_user,
            u.user.str.size());
        decoded_[id_user] = u.user.decoded_size;

        if(u.has_password)
        {
            // leading ':' for password,
            // trailing '@' for userinfo
            set_size(
                id_pass,
                u.password.str.size() + 2);
            decoded_[id_pass] =
                u.password.decoded_size;
        }
        else
        {
            // trailing '@' for userinfo
            set_size(id_pass, 1);
            decoded_[id_pass] = 0;
        }
    }
    else
    {
        set_size(id_user, 0);
        decoded_[id_user] = 0;
    }

    // host
    apply(t.host);

    // port
    if(t.port.has_port)
    {
        set_size(
            id_port,
            t.port.port.size() + 1);

        if(t.port.has_number)
            port_number_ =
                t.port.port_number;
    }
}

//------------------------------------------------
//
// Parsing
//
//------------------------------------------------

result<authority_view>
parse_authority(
    string_view s) noexcept
{
    if(s.size() > authority_view::max_size())
        detail::throw_length_error(
            "authority_view::max_size exceeded",
            BOOST_CURRENT_LOCATION);

    error_code ec;
    authority_bnf t;
    if(! bnf::parse_string(s, ec, t))
        return ec;

    authority_view a(s.data());

    // authority
    a.apply(t);

    return a;
}

//------------------------------------------------

std::ostream&
operator<<(
    std::ostream& os,
    authority_view const& a)
{
    os << a.encoded_authority();
    return os;
}

} // urls
} // boost

#endif
