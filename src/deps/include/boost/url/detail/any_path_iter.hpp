//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_ANY_PATH_ITER_HPP
#define BOOST_URL_DETAIL_ANY_PATH_ITER_HPP

#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <cstddef>

namespace boost {
namespace urls {
namespace detail {

struct BOOST_SYMBOL_VISIBLE
    any_path_iter
{
    string_view front;

    BOOST_URL_DECL
    virtual
    ~any_path_iter() noexcept = 0;

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

// iterates segments in an
// encoded path string
class BOOST_SYMBOL_VISIBLE
    enc_path_iter
    : public any_path_iter
{
    std::size_t n_;
    char const* p_;
    char const* end_;

    void
    increment() noexcept;

public:
    explicit
    enc_path_iter(
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

// iterates segments in an
// plain path string
class BOOST_SYMBOL_VISIBLE
    plain_path_iter :
    public any_path_iter
{
    std::size_t n_;
    char const* p_;
    char const* end_;

    void
    increment() noexcept;

public:
    explicit
    plain_path_iter(
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

class enc_segs_iter_base
{
protected:
    BOOST_URL_DECL
    static
    bool
    measure_impl(
        string_view s,
        std::size_t& n,
        error_code& ec) noexcept;

    BOOST_URL_DECL
    static
    void
    copy_impl(
        string_view s,
        char*& dest,
        char const* end) noexcept;
};

// iterates segments in an
// encoded segment range
template<class FwdIt>
class enc_segs_iter
    : public any_path_iter
    , public enc_segs_iter_base
{
    FwdIt it_;
    FwdIt end_;

public:
    enc_segs_iter(
        FwdIt first,
        FwdIt last) noexcept
        : it_(first)
        , end_(last)
    {
        front = *first;
    }

    bool
    measure(
        std::size_t& n,
        error_code& ec
            ) noexcept override
    {
        if(it_ == end_)
            return false;
        if(! measure_impl(
                *it_, n, ec))
            return false;
        ++it_;
        return true;
    }

    void
    copy(
        char*& dest,
        char const* end
            ) noexcept override
    {
        copy_impl(*it_,
            dest, end);
        ++it_;
    }
};

//------------------------------------------------

class plain_segs_iter_base
{
protected:
    BOOST_URL_DECL
    static
    void
    measure_impl(
        string_view s,
        std::size_t& n) noexcept;

    BOOST_URL_DECL
    static
    void
    copy_impl(
        string_view s,
        char*& dest,
        char const* end) noexcept;
};

// iterates segments in a
// plain segment range
template<class FwdIt>
class plain_segs_iter
    : public any_path_iter
    , public plain_segs_iter_base
{
    FwdIt it_;
    FwdIt end_;

public:
    plain_segs_iter(
        FwdIt first,
        FwdIt last) noexcept
        : it_(first)
        , end_(last)
    {
        front = *first;
    }

    bool
    measure(
        std::size_t& n,
        error_code&
            ) noexcept override
    {
        if(it_ == end_)
            return false;
        measure_impl(*it_, n);
        ++it_;
        return true;
    }

    void
    copy(
        char*& dest,
        char const* end
            ) noexcept override
    {
        copy_impl(*it_,
            dest, end);
        ++it_;
    }
};

//------------------------------------------------

template<class FwdIt>
enc_segs_iter<FwdIt>
make_enc_segs_iter(
    FwdIt first, FwdIt last)
{
    return enc_segs_iter<FwdIt>(
        first, last);
}

template<class FwdIt>
plain_segs_iter<FwdIt>
make_plain_segs_iter(
    FwdIt first, FwdIt last)
{
    return plain_segs_iter<FwdIt>(
        first, last);
}

} // detail
} // urls
} // boost

#endif
