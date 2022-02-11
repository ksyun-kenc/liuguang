//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_SEGMENTS_ENCODED_HPP
#define BOOST_URL_SEGMENTS_ENCODED_HPP

#include <boost/url/detail/config.hpp>
#include <boost/url/segments.hpp>
#include <boost/url/string.hpp>
#include <boost/url/detail/parts_base.hpp>
#include <initializer_list>
#include <iterator>

namespace boost {
namespace urls {

#ifndef BOOST_URL_DOCS
class url;
#endif

/** A reference-like container to modifiable URL segments

    This class implements a <em>RandomAccessContainer</em>
    representing the path segments in a @ref url as
    percent-encoded strings. Ownership of the segments
    is not transferred; the container references the
    buffer in the url. Therefore, the lifetime of the
    url must remain valid until this container no
    longer exists.

    Objects of this type are not constructed directly;
    Instead, call the corresponding non-const member
    function of @ref url to obtain an instance of
    the container:

    @par Example
    @code
    url u = parse_relative_ref( "/path/to/file.txt" );

    segments_encoded se = u.encoded_segments();

    for( segments_encoded::value_type s : se )
        std::cout << s << std::endl;
    @endcode

    The @ref reference and @ref const_reference
    nested types are defined as publicly accessible
    nested classes. They proxy the behavior of a
    reference to a percent-encoded string in the
    underlying URL. The primary use of these
    references is to provide l-values that can be
    returned from element-accessing operations.
    Any reads or writes which happen through a
    @ref reference or @ref const_reference
    potentially read or write the underlying
    @ref url.

    @see
        @ref url.
*/
class segments_encoded
    : private detail::parts_base
{
    url* u_ = nullptr;

    friend class url;

    inline
    explicit
    segments_encoded(
        url& u) noexcept;

public:
#ifdef BOOST_URL_DOCS
    /** A random-access iterator referencing segments in a url path

        When dereferenced, this iterator returns a
        proxy which allows conversion to stringlike
        types, assignments which change the underlying
        container, and comparisons.
    */
    using iterator = __see_below__;

#else
    class iterator;
#endif

    /** A type which can represent a segment as a value

        This type allows for making a copy of
        a segment where ownership is retained
        in the copy.
    */
    using value_type = std::string;

    using reference = string_view;

    using const_reference = string_view;

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

    /** Returns true if this contains an absolute path.

        Absolute paths always start with a
        forward slash ('/').
    */
    inline
    bool
    is_absolute() const noexcept;

    /** Return this container as percent-decoded segments
    */
    template<class Allocator = std::allocator<char> >
    segments
    decoded(Allocator const& alloc = {}) const;

    /** Replace the contents of the container

        This function replaces the contents
        with an initializer list  of
        percent-encoded strings.
        Each string must contain a valid
        percent-encoding or else an
        exception is thrown.
        The behavior is undefined any string
        refers to the contents of `*this`.
        All iterators and references to elements
        of the container are invalidated,
        including the @ref end iterator.

        @par Requires
        @code
        is_stringlike< String >::value == true
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/file.txt" );

        u.encoded_segments() = { "etc", "init.rc" };

        assert( u.encoded_path() == "/etc/init.rc") );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @param init An initializer list of strings.

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class String>
#ifdef BOOST_URL_DOCS
    segments_encoded&
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        segments_encoded&>::type
#endif
    operator=(std::initializer_list<String> init);

    /** Replace the contents of the container

        This function replaces the contents
        with an initializer list  of
        percent-encoded strings.
        Each string must contain a valid
        percent-encoding or else an
        exception is thrown.
        The behavior is undefined any string
        refers to the contents of `*this`.
        All iterators and references to elements
        of the container are invalidated,
        including the @ref end iterator.

        @par Requires
        @code
        is_stringlike< String >::value == true
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/file.txt" );

        u.encoded_segments().assign( { "etc", "init.rc" } );

        assert( u.encoded_path() == "/etc/init.rc") );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @param init An initializer list of strings.

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class String>
#ifdef BOOST_URL_DOCS
    void
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        void>::type
#endif
    assign(std::initializer_list<String> init);

    /** Replace the contents of the container

        This function replaces the contents
        with a range of percent-encoded
        strings.
        Each string must contain a valid
        percent-encoding or else an
        exception is thrown.
        The behavior is undefined if either
        argument is an iterator into `*this`.
        All iterators and references to elements
        of the container are invalidated,
        including the @ref end iterator.

        @par Requires
        @code
        is_stringlike< std::iterator_traits< FwdIt >::value_type >::value == true
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/file.txt" );

        segments_encoded se = u.encoded_segments();

        std::vector< std::string > v = { "etc", "init.rc" };

        se.insert( u.end() - 1, v.begin(), v.end() );

        assert( u.encoded_path() == "/etc/init.rc") );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @param first An iterator to the first
        element in the range

        @param last An iterator to one past the
        last element in the range

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class FwdIt>
#ifdef BOOST_URL_DOCS
    void
#else
    typename std::enable_if<
        is_stringlike<typename
            std::iterator_traits<
                FwdIt>::value_type>::value,
        void>::type
#endif
    assign(FwdIt first, FwdIt last);

    //--------------------------------------------
    //
    // Element Access
    //
    //--------------------------------------------

    /** Return an element with bounds checking

        This function returns a proxy reference
        to the i-th element. If i is greater than
        @ref size, an exception is thrown.

        @par Exception Safety
        Strong guarantee.
        Exception thrown on invalid parameter.

        @throws std::out_of_range `i >= size()`

        @return A proxy reference to the element.

        @param i The zero-based index of the
        element.
    */
    inline
    string_view
    at(std::size_t i) const;

    /** Return an element

        This function returns a proxy reference
        to the i-th element.

        @par Preconditions
        @code
        i < size()
        @endcode

        @par Exception Safety
        Strong guarantee.

        @return A proxy reference to the element.

        @param i The zero-based index of the
        element.
    */
    BOOST_URL_DECL
    string_view
    operator[](
        std::size_t i) const noexcept;

    /** Return the first element
    */
    inline
    string_view
    front() const noexcept;

    /** Return the last element
    */
    inline
    const_reference
    back() const noexcept;

    //--------------------------------------------
    //
    // Iterators
    //
    //--------------------------------------------

    /** Return an iterator to the beginning
    */
    inline
    iterator
    begin() const noexcept;

    /** Return an iterator to the end
    */
    inline
    iterator
    end() const noexcept;

    //--------------------------------------------
    //
    // Capacity
    //
    //--------------------------------------------

    /** Return true if the container is empty

        This function returns true if there are
        no elements in the container. That is, if
        the underlying path is the empty string.
    */
    inline
    bool
    empty() const noexcept;

    /** Return the number of elements in the container

        This function returns the number of
        elements in the underlying path. Empty
        segments count towards this total.

        @par Exception Safety
        Throws nothing.
    */
    inline
    std::size_t
    size() const noexcept;

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Remove the contents of the container

        This function removes all the segments
        from the container, leaving the
        underlying URL with an empty path.

        @par Postconditions
        @code
        empty() == true
        @endcode

        @par Exception Safety
        Throws nothing.
    */
    inline
    void
    clear() noexcept;

    //--------------------------------------------

    /** Insert an element

        This function inserts a segment specified
        by the percent-encoded string `s`, at the
        position preceding `before`.
        The string must contain a valid
        percent-encoding, or else an exception
        is thrown.
        All references and iterators starting
        from the newly inserted element and
        up to and including the last element
        and @ref end iterators are invalidated.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @return An iterator pointing to the
        inserted value.

        @param before An iterator before which the
        new element should be inserted.

        @param s A valid percent-encoded string
        to be inserted.

        @throw std::invalid_argument invalid percent-encoding
    */
    BOOST_URL_DECL
    iterator
    insert(
        iterator before,
        string_view s);

    /** Insert an element

        This function inserts a segment specified
        by the percent-encoded stringlike `s`,
        at the position preceding `before`.
        The stringlike must contain a valid
        percent-encoding, or else an exception
        is thrown.
        All references and iterators starting
        from the newly inserted element and
        up to and including the last element
        and @ref end iterators are invalidated.
        This function participates in overload
        resolution only if
        `is_stringlike<String>::value == true`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @return An iterator pointing to the
        inserted value.

        @param before An iterator before which the
        new element should be inserted.

        @param s The stringlike value to insert.

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class String>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        iterator>::type
#endif
    insert(
        iterator before,
        String const& s);

    /** Insert a range of segments

        This function inserts a range of
        percent-encoded strings passed as
        an initializer-list.
        Each string must contain a valid
        percent-encoding or else an exception
        is thrown.
        All references and iterators starting
        from the newly inserted elements and
        up to and including the last element
        and @ref end iterators are invalidated.

        @par Requires
        @code
        is_stringlike< String >::value == true
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/file.txt" );

        segments_encoded se = u.encoded_segments();

        se.insert( u.end() - 1, { "to", "the" } );

        assert( u.encoded_path() == "/path/to/the/file.txt") );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @return An iterator to one past the last
        newly inserted element or `before` if
        the range is empty.

        @param before An iterator before which the
        new elements should be inserted.

        @param init The initializer list containing
        percent-encoded segments to insert.

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class String>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        iterator>::type
#endif
    insert(
        iterator before,
        std::initializer_list<String> init);

    /** Insert a range of segments

        This function inserts a range
        of percent-encoded strings.
        Each string must contain a valid
        percent-encoding or else an
        exception is thrown.
        The behavior is undefined if either
        argument is an iterator into `this`.
        All references and iterators starting
        from the newly inserted elements and
        up to and including the last element
        and @ref end iterators are invalidated.

        @par Requires
        @code
        is_stringlike< std::iterator_traits< FwdIt >::value_type >::value == true
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/file.txt" );

        segments_encoded se = u.encoded_segments();

        std::vector< std::string > v = { "to", "the" };

        se.insert( u.end() - 1, v.begin(), v.end() );

        assert( u.encoded_path() == "/path/to/the/file.txt") );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @return An iterator to one past the last
        newly inserted element or `before` if
        the range is empty.

        @param before An iterator before which the
        new element should be inserted.

        @param first An iterator to the first
        element to insert.

        @param last An iterator to one past the
        last element to insert.

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class FwdIt>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<typename
            std::iterator_traits<
                FwdIt>::value_type>::value,
        iterator>::type
#endif
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
        std::input_iterator_tag) = delete;

    template<class FwdIt>
    iterator
    insert(
        iterator before,
        FwdIt first,
        FwdIt last,
        std::forward_iterator_tag);
public:

    //--------------------------------------------

    /** Erase an element

        This function erases the element pointed
        to by `pos`, which must be a valid
        iterator for the container.
        All references and iterators starting
        from pos and up to and including
        the last element and @ref end iterators
        are invalidated.

        @par Preconditions
        `pos` points to a valid element in
        this container.

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/file.txt" );

        segments_encoded se = u.encoded_segments();

        se.erase( se.begin() + 1 );

        assert( u.encoded_path() == "/path/file.txt" );
        @endcode

        @par Exception Safety
        Throws nothing.

        @return An iterator following
        the last element erased.

        @param pos An iterator to the
        element to erase.
    */
    inline
    iterator
    erase(
        iterator pos) noexcept;

    /** Erase a range of elements

        This function erases the elements
        in the range `[first, last)`, which
        must be a valid range in the container.
        All references and iterators starting
        from `first` and up to and including
        the last element and @ref end iterators
        are invalidated.

        @par Preconditions
        `[first, last)` is a valid range in
        this container.

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/the/file.txt" );

        segments_encoded se = u.encoded_segments();

        se.erase( se.begin() + 1, se.begin() + 3 );

        assert( u.encoded_path() == "/path/file.txt" );
        @endcode

        @return An iterator following
        the last element erased.

        @param first The beginning of the
        range to erase.

        @param last The end of the range
        to erase.

        @throw std::invalid_argument invalid percent-encoding
    */
    BOOST_URL_DECL
    iterator
    erase(
        iterator first,
        iterator last) noexcept;

    //--------------------------------------------

    inline
    iterator
    replace(
        iterator pos,
        string_view s);

    template<class String>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        iterator>::type
#endif
    replace(
        iterator pos,
        String const& s);

    template<class String>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        iterator>::type
#endif
    replace(
        iterator from,
        iterator to,
        std::initializer_list<String> init);

    inline
    iterator
    replace(
        iterator from,
        iterator to,
        std::initializer_list<
            string_view> init);

    template<class FwdIt>
#ifdef BOOST_URL_DOCS
    iterator
#else
    typename std::enable_if<
        is_stringlike<typename
            std::iterator_traits<
                FwdIt>::value_type>::value,
        iterator>::type
#endif
    replace(
        iterator from,
        iterator to,
        FwdIt first,
        FwdIt last);

    //--------------------------------------------

    /** Add an element to the end

        This function appends a segment
        containing the percent-encoded string
        `s` to the end of the container.
        The percent-encoding must be valid or
        else an exception is thrown.
        All @ref end iterators are invalidated.

        @par Example
        @code
        url u = parse_relative_uri( "/path/to" );

        u.segments_encoded().push_back( "file.txt" );

        assert( u.encoded_path() == "/path/to/file.txt" );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @param s The string to add

        @throw std::invalid_argument invalid percent-encoding
    */
    inline
    void
    push_back(
        string_view s);

    /** Add an element to the end

        This function appends a segment
        containing the percent-encoded stringlike
        `s` to the end of the container.
        The percent-encoding must be valid 
        or else an exception is thrown.
        All @ref end iterators are invalidated.
        The function participates in overload
        resolution only if
        `is_stringlike<String>::value == true`.

        @par Example
        @code
        url u = parse_relative_uri( "/path/to" );

        u.segments_encoded().push_back( "file.txt" );

        assert( u.encoded_path() == "/path/to/file.txt" );
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exceptions thrown on invalid input.

        @param s The stringlike to add

        @throw std::invalid_argument invalid percent-encoding
    */
    template<class String>
#ifdef BOOST_URL_DOCS
    void
#else
    typename std::enable_if<
        is_stringlike<String>::value,
        void>::type
#endif
    push_back(
        String const& s);

    /** Remove the last element

        This function removes the last element
        from the container, which must not be
        empty or else undefined behavior occurs.
        Iterators and references to
        the last element, as well as the
        @ref end iterator, are invalidated.

        @par Preconditions
        @code
        not empty()
        @endcode

        @par Example
        @code
        url u = parse_relative_uri( "/path/to/file.txt" );

        u.segments_encoded().pop_back();

        assert( u.encoded_path() == "/path/to" );
        @endcode

        @par Exception Safety
        Throws nothing.
    */
    inline
    void
    pop_back() noexcept;
};

} // urls
} // boost

// VFALCO This include is at the bottom of
// url.hpp because of a circular dependency
//#include <boost/url/impl/segments_encoded.hpp>

#endif
