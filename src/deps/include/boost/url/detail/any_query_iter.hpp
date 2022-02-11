//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_ANY_QUERY_ITER_HPP
#define BOOST_URL_DETAIL_ANY_QUERY_ITER_HPP

#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/url/params_value_type.hpp>
#include <cstddef>

namespace boost {
namespace urls {
namespace detail {

struct BOOST_SYMBOL_VISIBLE
    any_query_iter
{
    BOOST_URL_DECL
    virtual
    ~any_query_iter() noexcept = 0;

    virtual
    bool
    measure(
        std::size_t& n,
        error_code& ec) noexcept = 0;

    virtual
    void
    copy(
        char*& dest,
        char const* end) noexcept = 0;
};

//------------------------------------------------

// iterates params in an
// encoded query string
class enc_query_iter
    : public any_query_iter
{
    char const* p_;
    char const* end_;
    std::size_t n_;

    void
    increment() noexcept;

public:
    explicit
    enc_query_iter(
        string_view s) noexcept;

    bool
    measure(
        std::size_t& n,
        error_code& ec) noexcept override;

    void
    copy(
        char*& dest,
        char const* end) noexcept override;
};

//------------------------------------------------

// iterates params in a
// plain query string
class plain_query_iter
    : public any_query_iter
{
    char const* p_;
    char const* end_;
    std::size_t n_;

    void
    increment() noexcept;

public:
    explicit
    plain_query_iter(
        string_view s) noexcept;

    bool
    measure(
        std::size_t& n,
        error_code& ec) noexcept override;

    void
    copy(
        char*& dest,
        char const* end) noexcept override;
};

//------------------------------------------------

class enc_params_iter_base
{
protected:
    BOOST_URL_DECL
    static
    bool
    measure_impl(
        string_view key,
        string_view const* value,
        std::size_t& n,
        error_code& ec) noexcept;

    BOOST_URL_DECL
    static
    void
    copy_impl(
        string_view key,
        string_view const* value,
        char*& dest,
        char const* end) noexcept;
};

// iterates params in an
// encoded params range
template<class FwdIt>
class enc_params_iter
    : public any_query_iter
    , public enc_params_iter_base
{
    FwdIt it_;
    FwdIt end_;

public:
    enc_params_iter(
        FwdIt first,
        FwdIt last) noexcept
        : it_(first)
        , end_(last)
    {
    }

    bool
    measure(
        std::size_t& n,
        error_code& ec
            ) noexcept override
    {
        if(it_ == end_)
            return false;
        params_value_type v(*it_++);
        if(v.has_value)
            measure_impl(v.key,
                &v.value, n, ec);
        else
            measure_impl(v.key,
                nullptr, n, ec);
        return ! ec.failed();
    }

    void
    copy(
        char*& dest,
        char const* end
            ) noexcept override
    {
        params_value_type v(*it_++);
        if(v.has_value)
            copy_impl(v.key,
                &v.value, dest, end);
        else
            copy_impl(v.key,
                nullptr, dest, end);
    }
};

//------------------------------------------------

class plain_params_iter_base
{
protected:
    BOOST_URL_DECL
    static
    void
    measure_impl(
        string_view key,
        string_view const* value,
        std::size_t& n) noexcept;

    BOOST_URL_DECL
    static
    void
    copy_impl(
        string_view key,
        string_view const* value,
        char*& dest,
        char const* end) noexcept;
};

// iterates params in a
// decoded params range
template<class FwdIt>
class plain_params_iter
    : public any_query_iter
    , public plain_params_iter_base
{
    FwdIt it_;
    FwdIt end_;

public:
    plain_params_iter(
        FwdIt first,
        FwdIt last) noexcept
        : it_(first)
        , end_(last)
    {
    }

    bool
    measure(
        std::size_t& n,
        error_code&
            ) noexcept override;

    void
    copy(
        char*& dest,
        char const* end
            ) noexcept override;
};

//------------------------------------------------

template<class FwdIt>
enc_params_iter<FwdIt>
make_enc_params_iter(
    FwdIt first, FwdIt last)
{
    return enc_params_iter<FwdIt>(
        first, last);
}

template<class FwdIt>
plain_params_iter<FwdIt>
make_plain_params_iter(
    FwdIt first, FwdIt last)
{
    return plain_params_iter<FwdIt>(
        first, last);
}

} // detail
} // urls
} // boost

// VFALCO This include is at the bottom of
// url.hpp because of a circular dependency
#include <boost/url/detail/impl/any_query_iter.hpp>

#endif
