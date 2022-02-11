//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_PARAMS_HPP
#define BOOST_URL_PARAMS_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/params_value_type.hpp>
#include <boost/url/string.hpp>
#include <boost/url/detail/parts_base.hpp>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
class url;
class params_encoded;
#endif

/** A container referencing a random-access range of modifiable, percent-decoded query parameters.
*/
class params
    : private detail::parts_base
{
    friend class url;
    friend class params_encoded;

    url* u_ = nullptr;
    string_value::allocator a_;

    template<class Allocator>
    params(
        url& u,
        Allocator const& a);

public:
#ifdef BOOST_URL_DOCS
    /** A random-access iterator referencing parameters in a url query.

        Dereferencing this iterator returns the
        query parameter as a read-only structure
        containing strings allocated using the
        container's allocator.
    */
    using iterator = __see_below__;

#else
    class iterator;
#endif

    /** A read-only structure representing a decoded query parameter.

        Objects of this type are returned when
        accessing elements of the query parameters
        container. The strings returned are
        read-only copies. Changes to the query
        parameters are be made through the
        container's non-const member functions.
    */
    /**@{*/
    class reference
    {
        friend class params;
        friend class iterator;

        BOOST_URL_DECL
        reference(
            char const* s,
            std::size_t nk,
            std::size_t nv,
            string_value::allocator a);

    public:
        /** The query key.

            This string, which may be empty,
            holds the query key with percent-decoding
            applied.
        */
        string_value key;

        /** The query value.

            This string, which may be empty,
            holds the query value with percent-decoding
            applied. The field @ref has_value indicates
            if the query value is defined.
        */
        string_value value;

        /** True if the query parameter has a value.

            This field is true if the query parameter
            has a value, even if that value is empty.
            An empty value is distinguished from
            having no value.
        */
        bool has_value;
    };
    using const_reference = reference;
    /**@}*/

    /** A type which can represent a parameter as a value

        This type allows for making a copy of
        a query parameter where ownership is
        retained in the copy.
    */
    using value_type = params_value_type;

    /** An unsigned integer type
    */
    using size_type = std::size_t;

    /** A signed integer type
    */
    using difference_type = std::ptrdiff_t;

    //--------------------------------------------
    //
    // Members
    //
    //--------------------------------------------

    inline
    params&
    operator=(std::initializer_list<
        value_type> init);

    inline
    void
    assign(std::initializer_list<
        value_type> init);

    template<class FwdIt>
    void
    assign(FwdIt first, FwdIt last);

private:
    template<class FwdIt>
    void
    assign(FwdIt first, FwdIt last,
        std::forward_iterator_tag);

    template<class FwdIt>
    void
    assign(FwdIt first, FwdIt last,
        std::input_iterator_tag) = delete;
public:

    //--------------------------------------------
    //
    // Element Access
    //
    //--------------------------------------------

    inline
    reference
    at(std::size_t pos) const;

    BOOST_URL_DECL
    string_value
    at(string_view key) const;

    template<class Key>
#ifdef BOOST_URL_DOCS
    string_value
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        string_value>::type
#endif
    at(Key const& key) const;

    BOOST_URL_DECL
    reference
    operator[](
        std::size_t pos) const;

    inline
    reference
    front() const;

    inline
    reference
    back() const;

    //--------------------------------------------
    //
    // Iterators
    //
    //--------------------------------------------

    inline
    iterator
    begin() const noexcept;

    inline
    iterator
    end() const noexcept;

    //--------------------------------------------
    //
    // Capacity
    //
    //--------------------------------------------

    inline
    bool
    empty() const noexcept;

    inline
    std::size_t
    size() const noexcept;

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    inline
    void
    clear() noexcept;

    //--------------------------------------------

    inline
    iterator
    insert(
        iterator before,
        value_type const& v);

    inline
    iterator
    insert(
        iterator before,
        std::initializer_list<
            value_type> init);

    template<class FwdIt>
    iterator
    insert(
        iterator before,
        FwdIt first,
        FwdIt last);

private:
    template<class FwdIt>
    iterator
    insert(
        iterator before,
        FwdIt first,
        FwdIt last,
        std::forward_iterator_tag);

    template<class FwdIt>
    iterator
    insert(
        iterator before,
        FwdIt first,
        FwdIt last,
        std::input_iterator_tag) = delete;
public:

    //--------------------------------------------

    inline
    iterator
    replace(
        iterator pos,
        value_type const& value);

    template<class FwdIt>
    iterator
    replace(
        iterator from,
        iterator to,
        FwdIt first,
        FwdIt last);

    inline
    iterator
    replace(
        iterator from,
        iterator to,
        std::initializer_list<
            value_type> init);

    BOOST_URL_DECL
    iterator
    remove_value(
        iterator pos);

    BOOST_URL_DECL
    iterator
    replace_value(
        iterator pos,
        string_view value);

    template<class Value>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Value>::value,
        iterator>::type
#endif
    replace_value(
        iterator pos,
        Value const& value);

    //--------------------------------------------

    inline
    iterator
    emplace_at(
        iterator pos,
        string_view key,
        string_view value);

    template<
        class Key,
        class Value>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value &&
        is_stringlike<Value>::value,
        iterator>::type
#endif
    emplace_at(
        iterator pos,
        Key const& key,
        Value const& value);

    inline
    iterator
    emplace_at(
        iterator pos,
        string_view key);

    template<
        class Key>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
#endif
    emplace_at(
        iterator pos,
        Key const& key);

    inline
    iterator
    emplace_before(
        iterator before,
        string_view key,
        string_view value);

    template<
        class Key,
        class Value>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value &&
        is_stringlike<Value>::value,
        iterator>::type
#endif
    emplace_before(
        iterator before,
        Key const& key,
        Value const& value);

    inline
    iterator
    emplace_before(
        iterator before,
        string_view key);

    template<class Key>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
#endif
    emplace_before(
        iterator before,
        Key const& key);

    //--------------------------------------------

    inline
    iterator
    erase(iterator pos);

    BOOST_URL_DECL
    iterator
    erase(
        iterator first,
        iterator last);

    BOOST_URL_DECL
    std::size_t
    erase(string_view key) noexcept;

    template<class Key>
#ifdef BOOST_URL_DOCS
    std::size_t
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        std::size_t>::type
#endif
    erase(Key const& key) noexcept;

    //--------------------------------------------

    inline
    iterator
    emplace_back(
        string_view key,
        string_view value);

    template<
        class Key,
        class Value>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value &&
        is_stringlike<Value>::value,
        iterator>::type
#endif
    emplace_back(
        Key const& key,
        Value const& value);

    template<class Key>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
#endif
    emplace_back(
        Key const& key);

    inline
    void
    push_back(
        value_type const& value);

    inline
    void
    pop_back() noexcept;

    //--------------------------------------------
    //
    // Lookup
    //
    //--------------------------------------------

    BOOST_URL_DECL
    std::size_t
    count(string_view key) const noexcept;

    template<class Key>
#ifdef BOOST_URL_DOCS
    std::size_t
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        std::size_t>::type
#endif
    count(Key const& key) const noexcept;

    inline
    iterator
    find(string_view key) const noexcept;

    template<class Key>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
#endif
    find(Key const& key) const noexcept;

    /** Search [from, end), from==end is valid
    */
    BOOST_URL_DECL
    iterator
    find(
        iterator from,
        string_view key) const noexcept;

    template<class Key>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        iterator>::type
#endif
    find(
        iterator from,
        Key const& key) const noexcept;

    inline
    bool
    contains(string_view key) const noexcept;

    template<class Key>
#ifdef BOOST_URL_DOCS
    bool
#else
    typename std::enable_if<
        is_stringlike<Key>::value,
        bool>::type
#endif
    contains(Key const& key) const noexcept;
};

} // urls
} // boost

// VFALCO This include is at the bottom of
// url.hpp because of a circular dependency
//#include <boost/url/impl/params.hpp>

#endif
