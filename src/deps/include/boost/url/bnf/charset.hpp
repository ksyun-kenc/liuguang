//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_CHARSET_HPP
#define BOOST_URL_BNF_CHARSET_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/bnf/detail/charset.hpp>
#include <boost/type_traits/make_void.hpp>
#include <boost/static_assert.hpp>
#include <cstdint>
#include <type_traits>

// Credit to Peter Dimov for ideas regarding
// SIMD constexpr, and character set masks.

namespace boost {
namespace urls {
namespace bnf {

/** Alias for `std::true_type` if T satisfies <em>CharSet</em>.

    This metafunction determines if the
    type `T` meets these requirements of
    <em>CharSet</em>:

    @li An instance of `T` is invocable
    with this equivalent function signature:
    @code
    bool T::operator()( char ) const;
    @endcode

    @par Example
    Use with `enable_if` on the return value:
    @code
    template< class CharSet >
    typename std::enable_if< is_charset<T>::value >::type
    func( CharSet const& cs );
    @endcode

    @tparam T the type to check.
*/
#ifdef BOOST_URL_DOCS
template<class T>
using is_charset = __see_below__;
#else
template<class T, class = void>
struct is_charset : std::false_type {};

template<class T>
struct is_charset<T, boost::void_t<
    decltype(
    std::declval<bool&>() =
        std::declval<T const&>().operator()(
            std::declval<char>())
            ) > > : std::true_type
{
};
#endif

//------------------------------------------------

/** A character set based on a constexpr lookup table.

    Objects of this type are invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
class lut_chars
{
    std::uint64_t mask_[4] = {};

    constexpr
    static
    std::uint64_t
    lo(char c) noexcept
    {
        return static_cast<
            unsigned char>(c) & 3;
    }

    constexpr
    static
    std::uint64_t
    hi(char c) noexcept
    {
        return 1ULL << (static_cast<
            unsigned char>(c) >> 2);
    }

    constexpr
    static
    lut_chars
    construct(
        char const* s) noexcept
    {
        return *s
            ? lut_chars(*s) +
                construct(s+1)
            : lut_chars();
    }

    constexpr
    static
    lut_chars
    construct(
        unsigned char ch,
        bool b) noexcept
    {
        return b
            ? lut_chars(ch)
            : lut_chars();
    }

    template<class Pred>
    constexpr
    static
    lut_chars
    construct(
        Pred pred,
        unsigned char ch) noexcept
    {
        return ch == 255
            ? construct(ch, pred(ch))
            : construct(ch, pred(ch)) +
                construct(pred, ch + 1);
    }

    constexpr
    lut_chars() = default;

    constexpr
    lut_chars(
        std::uint64_t m0,
        std::uint64_t m1,
        std::uint64_t m2,
        std::uint64_t m3) noexcept
        : mask_{ m0, m1, m2, m3 }
    {
    }

public:
    /** Constructor.

        This function constructs a character
        set which has as a single member,
        the character `ch`.

        @par Example
        @code
        constexpr lut_chars asterisk( '*' );
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @param ch A character.
    */
    constexpr
    lut_chars(char ch) noexcept
        : mask_ {
            lo(ch) == 0 ? hi(ch) : 0,
            lo(ch) == 1 ? hi(ch) : 0,
            lo(ch) == 2 ? hi(ch) : 0,
            lo(ch) == 3 ? hi(ch) : 0 }
    {
    }

    /** Constructor.

        This function constructs a character
        set which has as members, all of the
        characters present in the null-terminated
        string `s`.

        @par Example
        @code
        constexpr lut_chars digits = "0123456789";
        @endcode

        @par Complexity
        Linear in `::strlen(s)`, or constant
        if `s` is a constant expression.

        @par Exception Safety
        Throws nothing.

        @param s A null-terminated string.
    */
    constexpr
    lut_chars(char const* s) noexcept
        : lut_chars(construct(s))
    {
    }

    /** Constructor.

        This function constructs a character
        set which has as members, every value
        of `char ch` for which the expression
        `pred(ch)` returns `true`.

        @par Example
        @code
        struct is_digit
        {
            constexpr bool
            operator()(char c ) const noexcept
            {
                return c >= '0' && c <= '9';
            }
        };

        constexpr lut_chars digits( is_digit{} );
        @endcode

        @par Complexity
        Linear in `pred`, or constant if
        `pred(ch)` is a constant expression.

        @par Exception Safety
        Throws nothing.

        @param pred The function object to
        use for determining membership in
        the character set.
    */
    template<class Pred
#ifndef BOOST_URL_DOCS
        ,class = typename std::enable_if<
            is_charset<Pred>::value &&
        ! std::is_same<Pred, lut_chars>::value>::type
#endif
    >
    constexpr
    lut_chars(Pred const& pred) noexcept
        : lut_chars(
            construct(pred, 0))
    {
    }

    /** Return true if ch is in the character set.

        This function returns true if the
        character `ch` is in the set, otherwise
        it returns false.

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @param ch The character to test.
    */
    constexpr
    bool
    operator()(
        unsigned char ch) const noexcept
    {
        return mask_[lo(ch)] & hi(ch);
    }

    /** Return the union of two character sets.

        This function returns a new character
        set which contains all of the characters
        in `cs0` as well as all of the characters
        in `cs`.

        @par Example
        This creates a character set which
        includes all letters and numbers
        @code
        constexpr lut_chars alpha_chars(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz");

        constexpr lut_chars alnum_chars = alpha_chars + "0123456789";
        @endcode

        @par Complexity
        Constant.

        @return The new character set.

        @param cs0 A character to join
        
        @param cs1 A character to join
    */
    friend
    constexpr
    lut_chars
    operator+(
        lut_chars const& cs0,
        lut_chars const& cs1) noexcept
    {
        return lut_chars(
            cs0.mask_[0] | cs1.mask_[0],
            cs0.mask_[1] | cs1.mask_[1],
            cs0.mask_[2] | cs1.mask_[2],
            cs0.mask_[3] | cs1.mask_[3]);
    }

    /** Return a new character set by subtracting

        This function returns a new character
        set which is formed from all of the
        characters in `cs0` which are not in `cs`.

        @par Example
        This statement declares a character set
        containing all the lowercase letters
        which are not vowels:
        @code
        constexpr lut_chars consonants = lut_chars("abcdefghijklmnopqrstuvwxyz") - "aeiou";
        @endcode

        @par Complexity
        Constant.

        @return The new character set.

        @param cs0 A character set to join.
        
        @param cs1 A character set to join.
    */
    friend
    constexpr
    lut_chars
    operator-(
        lut_chars const& cs0,
        lut_chars const& cs1) noexcept
    {
        return lut_chars(
            cs0.mask_[0] & ~cs1.mask_[0],
            cs0.mask_[1] & ~cs1.mask_[1],
            cs0.mask_[2] & ~cs1.mask_[2],
            cs0.mask_[3] & ~cs1.mask_[3]);
    }

    /** Return a new character set which is the complement of another character set.

        This function returns a new character
        set which contains all of the characters
        that are not in `*this`.

        @par Example
        This statement declares a character set
        containing everything but vowels:
        @code
        constexpr lut_chars not_vowels = ~lut_chars( "aAeEiIoOuU" );
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Throws nothing.

        @return The new character set.
    */
    constexpr
    lut_chars
    operator~() const noexcept
    {
        return lut_chars(
            ~mask_[0],
            ~mask_[1],
            ~mask_[2],
            ~mask_[3]
        );
    }

#ifndef BOOST_URL_DOCS
#ifdef BOOST_URL_USE_SSE2
    char const*
    find_if(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_pred(
            *this, first, last);
    }

    char const*
    find_if_not(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_not_pred(
            *this, first, last);
    }
#endif
#endif
};

//------------------------------------------------

/** A character set containing all characters.

    This is an object invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @par BNF
    @code
    ALL         = %x00-FF
                ; all ASCII and high-ASCII
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
constexpr
#ifdef BOOST_URL_DOCS
__implementation_defined__
#else
struct
{
    constexpr
    bool
    operator()(char) const noexcept
    {
        return true;
    }

    constexpr
    char const*
    find_if(
        char const* first,
        char const*) const noexcept
    {
        return first;
    }

    constexpr
    char const*
    find_if_not(
        char const*,
        char const* last) const noexcept
    {
        return last;
    }
}
#endif
all_chars{};

//------------------------------------------------

/** A character set containing all letters and digits.

    This is an object invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @par BNF
    @code
    ALNUM       = ALPHA / DIGIT

    ALPHA       =  %x41-5A / %x61-7A
                ; A-Z / a-z

    DIGIT       =  %x30-39
                ; 0-9
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
constexpr
#ifdef BOOST_URL_DOCS
__implementation_defined__
#else
struct
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z');
    }

#ifdef BOOST_URL_USE_SSE2
    char const*
    find_if(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_pred(
            *this, first, last);
    }

    char const*
    find_if_not(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_not_pred(
            *this, first, last);
    }
#endif
}
#endif
alnum_chars{};

//------------------------------------------------

/** A CharSet containing the alpha characters

    This is an object invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @par BNF
    @code
    ALPHA       =  %x41-5A / %x61-7A
                ; A-Z / a-z
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
constexpr
#ifdef BOOST_URL_DOCS
__implementation_defined__
#else
struct
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z');
    }

#ifdef BOOST_URL_USE_SSE2
    char const*
    find_if(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_pred(
            *this, first, last);
    }

    char const*
    find_if_not(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_not_pred(
            *this, first, last);
    }
#endif
}
#endif
alpha_chars{};

//------------------------------------------------

/** A character set containing the decimal digits.

    This is an object invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @par BNF
    @code
    DIGIT       =  %x30-39
                ; 0-9
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
constexpr
#ifdef BOOST_URL_DOCS
__implementation_defined__
#else
struct
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return c >= '0' && c <= '9';
    }

#ifdef BOOST_URL_USE_SSE2
    char const*
    find_if(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_pred(
            *this, first, last);
    }

    char const*
    find_if_not(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_not_pred(
            *this, first, last);
    }
#endif
}
#endif
digit_chars{};

//------------------------------------------------

/** A character set containing the hexadecimal digits.

    This is an object invocable with
    the equivalent signature:

    @code
    bool( char ch ) const noexcept;
    @endcode

    The function object returns `true` when
    `ch` is a member of the character set,
    and `false` otherwise. The type of the
    function satisfies the <em>CharSet</em>
    requirements.

    @par BNF
    @code
    HEXDIG      = DIGIT
                / "A" / "B" / "C" / "D" / "E" / "F"
                / "a" / "b" / "c" / "d" / "e" / "f"
    @endcode

    @note The RFCs are inconsistent on the case
    sensitivity of hexadecimal digits. Existing
    uses suggest case-insensitivity is a de-facto
    standard.

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>
    @li <a href="https://datatracker.ietf.org/doc/html/rfc7230#section-1.2"
        >1.2. Syntax Notation (rfc7230)</a>
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5952#section-2.3"
        >2.3. Uppercase or Lowercase (rfc5952)</a>
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5952#section-4.3"
        >4.3. Lowercase (rfc5952)</a>

    @see
        @ref is_charset,
        @ref find_if,
        @ref find_if_not.
*/
constexpr
#ifdef BOOST_URL_DOCS
__implementation_defined__
#else
struct
{
    /** Return true if c is in the character set.
    */
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f');
    }

#ifdef BOOST_URL_USE_SSE2
    char const*
    find_if(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_pred(
            *this, first, last);
    }

    char const*
    find_if_not(
        char const* first,
        char const* last) const noexcept
    {
        return detail::find_if_not_pred(
            *this, first, last);
    }
#endif
}
#endif
hexdig_chars{};

// VFALCO We can declare
// these later if needed
//
//struct hexdig_upper_chars;
//struct hexdig_lower_chars;

/** Return the numeric value of a HEXDIG, or -1 if invalid

    This function returns the numeric value
    of a single hexadecimal digit, or -1 if
    `ch` is not a hexadecimal digit.
*/
constexpr
char
hexdig_value(char c) noexcept
{
    return
        (c >= '0' && c <= '9') ?
        c - '0' : (
            (c >= 'A' && c <= 'F') ?
            10 + c - 'A' : (
                (c >= 'a' && c <= 'f') ?
                10 + c - 'a' : - 1) );
}

//------------------------------------------------

/** Find the first character in the string that is in the set.

    @par Exception Safety
    Throws nothing.

    @return A pointer to the character if found,
    otherwise a pointer to `last`.

    @param first A pointer to the first character
    in the string to search.

    @param last A pointer to one past the last
    character in the string to search.

    @param cs The character set to use.

    @see
        @ref find_if_not.
*/
template<class CharSet>
char const*
find_if(
    char const* const first,
    char const* const last,
    CharSet const& cs) noexcept
{
    BOOST_STATIC_ASSERT(
        is_charset<CharSet>::value);
    return detail::find_if(first, last, cs,
        detail::has_find_if<CharSet>{});
}

/** Find the first character in the string that is not in CharSet

    @par Exception Safety
    Throws nothing.

    @return A pointer to the character if found,
    otherwise a pointer to `last`.

    @param first A pointer to the first character
    in the string to search.

    @param last A pointer to one past the last
    character in the string to search.

    @param cs The character set to use.

    @see
        @ref find_if_not.
*/
template<class CharSet>
char const*
find_if_not(
    char const* const first,
    char const* const last,
    CharSet const& cs) noexcept
{
    BOOST_STATIC_ASSERT(
        is_charset<CharSet>::value);
    return detail::find_if_not(first, last, cs,
        detail::has_find_if_not<CharSet>{});
}

} // bnf
} // urls
} // boost

#endif
