//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_STRING_HPP
#define BOOST_URL_STRING_HPP

#include <boost/url/detail/config.hpp>
#include <boost/type_traits/make_void.hpp>
#include <boost/utility/string_view.hpp>
#include <memory>
#include <string>
#ifndef BOOST_NO_CXX17_HDR_STRING_VIEW
# include <string_view>
# define BOOST_URL_HAS_STRING_VIEW
#endif
#include <type_traits>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
// this is private
using pos_t = std::size_t;
#endif

/** The type of string_view used by the library

    String views are used to pass character
    buffers into or out of functions. Ownership
    of the underlying character buffer is not
    transferred; the caller is responsible for
    ensuring that the lifetime of the object
    owning the character buffer extends until
    the string view is no longer referenced.
*/
typedef boost::string_view string_view;

/** Alias for `std::true_type` if a `T` can be converted to a string_view

    This metafunction is an alias for `std::true_type` if

    @code
    std::is_convertible< T, string_view > == true
    @endcode

    or, if the following expressions are valid
    where `t` has type `T const`:

    @code
    char const* p = t.data();

    std::size_t n = t.size();
    @endcode

    Otherwise, it is an alias for `std::false_type`.

    @par Examples

    This function causes a compile time error if
    `T` does not satisfy `is_stringlike`:
    @code
    template< typename T>
    void f( T const & )
    {
        static_assert( is_stringlike< T >::value, "T must be stringlike" );
    }
    @endcode

    The following function uses
    <a href="https://en.cppreference.com/w/cpp/language/sfinae">
    SFINAE</a> on the return type to enable itself only
    if `T` satisfies `is_stringlike`:

    @code
    // return type is void
    template< class T >
    typename std::enable_if< is_stringlike< T >::value >::type
    f( T const& t );
    @endcode

    The function @ref to_string_view is used to
    generically convert an instance of a type
    `T` to `string_view` when such a conversion
    is possible. This allows the library to
    interoperate with various string-like types.

    @see @ref to_string_view
*/
#ifdef BOOST_URL_DOCS
template<class T>
using is_stringlike = __see_below__;
#else
template<class T, class = void>
struct is_stringlike :
    std::is_convertible<T, string_view>
{
};

template<class T>
struct is_stringlike<T, boost::void_t<
    decltype(
    std::declval<char const*&>() =
        std::declval<T const>().data(),
    std::declval<std::size_t&>() =
        std::declval<T const>().size()
            ) > > : std::true_type
{
};
#endif

/** Return a string_view constructed from a T

    This function constructs a string view from
    the generic value `t`. The function
    participates in overload resolution only
    if `is_stringlike<T>::value == true`.

    @return A string view representing `t`.

    @param t The value to construct from.
*/
#ifdef BOOST_URL_DOCS
template<class T>
string_view
to_string_view(T const& t) noexcept;
#else
template<class T>
typename std::enable_if<
    std::is_convertible<
        T, string_view>::value,
    string_view>::type
to_string_view(
    T const& t) noexcept
{
    return t;
}

template<class T>
typename std::enable_if<
    is_stringlike<T>::value &&
    ! std::is_convertible<
        T, string_view>::value,
    string_view>::type
to_string_view(
    T const& t) noexcept
{
    return { t.data(), t.size() };
}
#endif

//------------------------------------------------

/** A read-only, reference counted string

    Objects of this type represent read-only
    strings with ownership of the character
    buffer. They are reference counted which
    makes copies cheap. The type is derived
    from @ref string_view which provides
    compatibility with strings in terms of
    comparisons and converisons. However,
    care must be exercised; undefined
    behavior results if the string_view
    portion of the object is modified
    directly, for example by calling
    `remove_suffix` or `operator=`.

    Slicing however, is supported, as
    copies of the `string_view` portion
    of the object are valid and remain
    valid for the lifetime of the oriignal
    object.
*/
class string_value : public string_view
{
    struct base;

    base* p_ = nullptr;

    template<class Allocator>
    base*
    construct(
        std::size_t n,
        Allocator const& a,
        char*& dest);

public:
    class allocator;

    inline
    ~string_value();

    string_value() = default;

    template<class Allocator>
    string_value(
        std::size_t n,
        Allocator const& a,
        char*& dest);

    template< class Allocator =
        std::allocator<char> >
    explicit
    string_value(
        string_view s,
        Allocator const& a = {});

    inline
    string_value(
        string_value const& other) noexcept;

    inline
    string_value&
    operator=(string_value const& other) & noexcept;
};

} // urls
} // boost

#include <boost/url/impl/string.hpp>

#endif
