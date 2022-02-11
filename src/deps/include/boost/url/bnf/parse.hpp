//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_BNF_PARSE_HPP
#define BOOST_URL_BNF_PARSE_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/error.hpp>
#include <boost/url/string.hpp>
#include <boost/type_traits/make_void.hpp>
#include <type_traits>

#include <boost/optional.hpp>

namespace boost {
namespace urls {
namespace bnf {

/** @brief Parse a literal character

   This function parses the string defined by the first chars in the
   range of chars [@ref it, @ref end) as a literal character.

   The function returns `true` and resets @ref ec when the input is
   parsed successfully. The initial character in the range is immediately
   consumed and ignored.

   If the input cannot be parsed into the literal BNF literal,
   the function returns `false` and sets the error number in @ref ec to the
   appropriate error number defined in @ref boost::urls::error.

   @par Example
   @code
   std::string str = "Hello.cpp";
   auto it = str.data();
   auto const end = it + str.size();
   if (bnf::parse(it, end, ec, 'H')) {
       std::cout << "str begins with 'H'" << std::endl;
   }
   @endcode

   @par Exception Safety
   Calls to other `parse` overloads may throw, although they should
   prefer setting @ref ec instead.

   @return `true` if the first char in the range matches @ref ch successfully,
   `false` otherwise

   @param[in,out] it An iterator to the first element in the range.
   At the end of this function, @ref it points to one past the last
   element parsed in the range.

   @param[in] end An iterator to one past the last element in the range

   @param[out] ec The error code this function should set @ref if it fails

   @param[out] ch A literal character
*/
inline
bool
parse(
    char const*& it,
    char const* end,
    error_code& ec,
    char ch) noexcept;

/** @brief Parse an optional element

   This function parses the string defined by the first chars in the
   range of chars [@ref it, @ref end) as an optional element @ref t.
   It uses the underlying @ref parse function for elements of type @ref T
   to create an element of type @ref optional<T>.

   When the input is parsed successfully, the function returns `true`,
   resets @ref ec, an sets @ref t to valid element. If the input cannot
   be parsed into an element, the function returns `false`, sets the error
   number in @ref ec to the appropriate error number defined in
   @ref boost::urls::error, and resets the optional @ref t.

   @par Exception Safety
   Throws nothing.

   @return `true` if the range initial chars match an element of type @ref T
   successfully, `false` otherwise

   @param[in,out] it An iterator to the first element in the range
   At the end of this function, @ref it points to one past the last
   element parsed in the range.

   @param[in] end An iterator to one past the last element in the range

   @param[out] ec The error code this function should set if it fails

   @param[out] ch A optional element of type @ref T
*/
template<class T>
bool
parse(
    char const*& it,
    char const* end,
    error_code& ec,
    optional<T>& t);

/** @brief Parse a sequence of elements

   This function parses the string defined by the first chars in the
   range of chars [@ref it, @ref end) into the Backus–Naur form (BNF)
   elements [arg_0, arg_1, ..., arg_n] in @ref args.

   For each type Arg_i in [Arg_0, Arg_1, ..., Arg_n] in @ref Args, a
   corresponding function overload
   `bool parse(char const*& it, char const* const end, error_code&, Arg_i const& t)`
   should be defined to consume the input from [@ref it, @ref end) into `arg_i`.

   This `parse` function is defined for `char`s so that it matches the BNF
   string literals in [@ref it, @ref end).

   The function returns `true` and resets @ref ec when the input is
   parsed successfully. If the input cannot be parsed into the BNF elements,
   the function returns `false` and sets the error number in @ref ec to the
   appropriate error number defined in @ref boost::urls::error.

   @par Example
   @code
   #include <iostream>
   #include <string>

   #include <boost/url/bnf/char_set.hpp>
   #include <boost/url/bnf/parse.hpp>
   #include <boost/url/error.hpp>

   namespace boost {
   namespace urls {
   namespace detail {
   // BNF element to parse a non-empty sequence of alpha chars ([a-zA-Z]+)
   struct alpha_string_bnf {
     // reference to a std::string where we should store the results
     std::string &v;

     // parsing an alpha_string_bnf element
     friend bool parse(char const *&it, char const *end, error_code &ec,
                       alpha_string_bnf &&t) {
       // is_alpha functor
       bnf::alpha_chars is_alpha;

       // check if empty
       if (it == end) {
         ec = error::incomplete;
         return false;
       }

       // check if not alpha at all
       if (!is_alpha(*it)) {
         ec = error::bad_alpha;
         return false;
       }
       t.v.push_back(*it);
       ++it;

       // consume more alphas
       while (it != end && is_alpha(*it)) {
         t.v.push_back(*it);
         ++it;
       }
       ec = error::success;
       return true;
     }
   };
   } // namespace detail
   } // namespace urls
   } // namespace boost

   int main() {
     using namespace boost::urls;

     // Input
     std::string str = "Hello.cpp";
     auto it = str.data();
     auto const end = it + str.size();

     // Output
     std::string alpha_l;
     std::string alpha_r;
     error_code ec;

     // Parse two alpha strings separated by a '.'
     if (bnf::parse(it, end, ec, detail::alpha_string_bnf{alpha_l}, '.',
                    detail::alpha_string_bnf{alpha_r})) {
       std::cout << alpha_l << std::endl;
       std::cout << alpha_r << std::endl;
     }
     return 0;
   }
   @endcode

   @par Exception Safety
     Calls to `parse` overloads may throw, although they should
     prefer setting @ref ec instead.

   @see https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form

        @ref boost::urls::error

   @return `true` if the range initial chars match the elements successfully,
   `false` otherwise

   @param[in,out] it An iterator to the first element in the range.
   At the end of this function, @ref it points to one past the last
   element parsed in the range.

   @param[in] end An iterator to one past the last element in the range

   @param[out] ec The error code this function should set if it fails

   @param[out] args BNF elements to be parsed

*/
#ifdef BOOST_URL_DOCS
template<class... Args>
bool parse( char const*& it, char const* end, Args&&... args);
#else
template<
    class T0,
    class T1,
    class... Tn>
bool
parse(
    char const*& it,
    char const* end,
    error_code& ec,
    T0&& t0,
    T1&& t1,
    Tn&&... tn);
#endif

/** @brief Parse a complete string

   This function parses a complete string @ref s into the Backus–Naur form (BNF)
   elements [@ref t0, @ref t1, ...]. If the string is not completely consumed,
   @ref ec is set to @ref error::leftover_input.

   The function returns `true` and resets @ref ec when the input is
   parsed successfully into the elements [@ref t0, @ref t1, ...].

   If the input cannot be parsed into the BNF elements, the function returns
   `false` and sets the error number in @ref ec to the appropriate error
   number defined in @ref boost::urls::error.

   @par Exception Safety
     Calls to `parse` overloads may throw, although they should
     prefer setting @ref ec instead.

   @see @ref boost::urls::error

   @return `true` if the string matches the BNF elements successfully and all
   chars are consumed, `false` otherwise

   @param[in] s The input string this function should parse

   @param[out] ec The error code this function should set if it fails

   @param[out] t0 T0 1st BNF element

   @param[out] t1 T1 2nd BNF element

   @param[out] tn ...Tn [3rd, 4th, ...] BNF elements

*/
template<
    class T0,
    class... Tn>
bool
parse_string(
    string_view s,
    error_code& ec,
    T0&& t0,
    Tn&&... tn);

/** @brief Parse a complete string and throw on failure

   This function parses a complete string @ref s into the Backus–Naur form (BNF)
   elements [@ref t0, @ref t1, ...]. If the string is not completely consumed,
   an error is thrown.

   The function returns `true`when the input is parsed successfully into the
   elements [@ref t0, @ref t1, ...]. If the input cannot be parsed into the
   BNF elements, the function throw a @ref system_error.

   @par Exception Safety
     Exceptions thrown on invalid input.

   @see @ref boost::urls::error

   @param[in] s The input string this function should parse

   @param[out] t0 T0 1st BNF element

   @param[out] t1 T1 2nd BNF element

   @param[out] tn ...Tn [3rd, 4th, ...] BNF elements

   @throw system_error @ref s cannot be parsed into [@ref t0, @ref t1, ...]
*/
template<
    class T0,
    class... Tn>
void
parse_string(
    string_view s,
    T0&& t0,
    Tn&&... tn);

} // bnf
} // urls
} // boost

#include <boost/url/bnf/impl/parse.hpp>

#endif
