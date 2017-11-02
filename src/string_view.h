#pragma once
#ifndef STRING_VIEW_H
#define STRING_VIEW_H
#include "array_view.h"
#include <string>
#include <iosfwd>
#include <algorithm>

template< class CharT, class Traits = std::char_traits<CharT> >
class basic_string_view;

using string_view  = basic_string_view< char >;
using wstring_view = basic_string_view< wchar_t >;

template< class CharT, class Traits >
class basic_string_view : public array_view<CharT>
{
        using base = array_view<CharT>;
    public:
        using typename base::value_type;
        using typename base::pointer;
        using typename base::const_pointer;
        using typename base::reference;
        using typename base::const_reference;
        using typename base::const_iterator;
        using typename base::const_reverse_iterator;
        using typename base::size_type;
        using base::npos;

        // Constructors
        // Inherit common constructors
        using base::array_view;
        constexpr basic_string_view( base b ) noexcept : base( b ) {}
        constexpr basic_string_view( basic_string_view const &b ) noexcept = default;
        basic_string_view &operator=( basic_string_view const &b ) noexcept = default;

        // constuctor for char*s
        constexpr basic_string_view( const_pointer s ) noexcept :
            base( s, Traits::length( s ) )
        {}

        // Iterators
        using base::begin;
        using base::cbegin;
        using base::rbegin;
        using base::end;
        using base::cend;
        using base::rend;

        // Accessors
        using base::operator[];
        using base::at;
        using base::front;
        using base::back;
        using base::data;

        // Capacity
        using base::size;
        using base::empty;
        using base::operator bool;

        // Modifiers
        using base::remove_prefix;
        using base::remove_suffix;
        using base::swap;

        // Operations
        constexpr basic_string_view substr( size_type pos = 0, size_type count = npos ) const noexcept {
            return base::substr( pos, count );
        }

        size_type find( basic_string_view sv, size_type pos = 0 ) const noexcept {
            if( pos > size() ) {
                return npos;
            } else if( sv.empty() ) {
                return pos;
            }
            const_iterator it = std::search( begin() + pos, end(), sv.begin(), sv.end(), Traits::eq );
            return it == end() ? npos : static_cast<size_type>( std::distance( begin(), it ) );
        }

        // Conversions to std::string.
        template<class Alloc = std::string::allocator_type>
        std::basic_string<CharT, Traits, Alloc> to_string() const {
            return std::basic_string<CharT, Traits, Alloc>( data(), size() );
        }

        template<class Alloc = std::string::allocator_type> friend
        std::basic_string<CharT, Traits, Alloc> to_string( basic_string_view v ) {
            return std::basic_string<CharT, Traits, Alloc>( v.begin(), v.size() );
        }

        template<class Alloc>
        explicit operator std::basic_string<CharT, Traits, Alloc>() const {
            return to_string<Alloc>();
        }

        // string comparison
        int compare( basic_string_view v ) const noexcept {
            const int cmp = Traits::compare( data(), v.data(), std::min( size(), v.size() ) );
            return cmp != 0 ? cmp : ( size() == v.size() ? 0 : size() < v.size() ? -1 : 1 );
        }

        int compare( size_type p1, size_type c1, basic_string_view v ) const {
            return substr( p1, c1 ).compare( v );
        }

        int compare( size_type p1, size_type c1, basic_string_view v,
                     size_type p2, size_type c2 ) const {
            return substr( p1, c1 ).compare( v.substr( p2, c2 ) );
        }

    protected:
        using base::_data;
        using base::_size;
};

#define MAKE_STRING_VIEW_OPERATOR( OP ) \
    template<class charT, class traits> \
    constexpr bool operator OP(basic_string_view<charT, traits> x, \
                               basic_string_view<charT, traits> y ) noexcept \
    { \
        return x.compare(y) OP 0; \
    } \
    template< class T, \
              class charT, class traits, \
              class = decltype(basic_string_view<charT, traits>(std::declval<T>())) \
              > \
    constexpr bool operator OP( basic_string_view<charT, traits> lhs, T const& t ) noexcept \
    { \
        return lhs OP basic_string_view<charT, traits>(t); \
    } \
    template< class T, \
              class charT, class traits, \
              class = decltype(basic_string_view<charT, traits>(std::declval<T>())) \
              > \
    constexpr bool operator OP( T const& t, basic_string_view<charT, traits> rhs ) noexcept \
    { \
        return basic_string_view<charT, traits>(t) OP rhs; \
    } \

MAKE_STRING_VIEW_OPERATOR( == )
MAKE_STRING_VIEW_OPERATOR( != )
MAKE_STRING_VIEW_OPERATOR( < )
MAKE_STRING_VIEW_OPERATOR( > )
MAKE_STRING_VIEW_OPERATOR( <= )
MAKE_STRING_VIEW_OPERATOR( >= )

#undef MAKE_STRING_VIEW_OPERATOR

constexpr string_view operator""_sv( const char *s, size_t i )
{
    return {s, i};
}

template<class charT, class traits>
std::basic_ostream<charT, traits> &
operator<<( std::basic_ostream<charT, traits> &os,
            basic_string_view<charT, traits> v )
{
    return os.write( v.data(), static_cast<std::streamsize>( v.size() ) );
}

#endif

