#pragma once
#ifndef STRING_VIEW_H
#define STRING_VIEW_H
#include "array_view.h"
#include <string>
#include <stdexcept>
#include <iosfwd>

template< class CharT, class Traits = std::char_traits<CharT> >
class basic_string_view;

using string_view  = basic_string_view< char >;
using wstring_view = basic_string_view< wchar_t >;

template< class CharT, class Traits >
class basic_string_view : public array_view<CharT>
{
        using base = array_view<CharT>;
    protected:
        using base::_begin;
        using base::_end;
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

        constexpr basic_string_view( CharT const *s ) noexcept :
            base( s, s + Traits::length( s ) )
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
        constexpr basic_string_view substr( size_type pos = 0, size_type count = npos ) const {
            return base::substr( pos, count );
        }

        size_type find( const_reference val, size_type pos = 0 ) const noexcept {
            auto p = Traits::find( begin() + pos, size() - pos, val );
            if ( p == nullptr ) {
                return npos;
            }
            return static_cast<size_type>( p - begin() );
        }

        size_type find( basic_string_view v, size_type pos = 0 ) const {
            size_type s = pos;
            while( s < size() && ( s = find( v[0], s ) ) != npos ) {
                if( compare( s, v.size(), v ) == 0 ) {
                    return s;
                }
                ++s;
            }
            return npos;
        }

        // Conversions to std::string.
        template<class Alloc>
        std::basic_string<CharT, Traits, Alloc> to_string() const {
            return std::basic_string<CharT, Traits, Alloc>(_begin, size());
        }

        template<class Alloc> friend
        std::basic_string<CharT, Traits, Alloc> to_string( basic_string_view v ) {
            return std::basic_string<CharT, Traits, Alloc>(v._begin, v.size());
        }

        template<class Alloc>
        explicit operator std::basic_string<CharT, Traits, Alloc>() const {
            return to_string<Alloc>();
        }

        // string comparison
        int compare( basic_string_view v ) const noexcept {
            const size_type rlen = std::min( v.size(), size() );
            const int cmp = Traits::compare( data(), v.data(), rlen );
            if( cmp < 0 || cmp > 0 ) {
                return cmp;
            }
            return
                ( size() < v.size() ) ? -1 :
                ( size() > v.size() ) ? +1 : 0;
        }

        int compare( size_type p1, size_type c1, basic_string_view v ) const {
            return substr( p1, c1 ).compare( v );
        }

        int compare( size_type p1, size_type c1, basic_string_view v,
                     size_type p2, size_type c2 ) const {
            return substr( p1, c1 ).compare( v.substr( p2, c2 ) );
        }
};

#define MAKE_STRING_VIEW_OPERATOR( OP ) \
    template<class charT, class traits> \
    constexpr bool operator OP(basic_string_view<charT, traits> x, \
                               basic_string_view<charT, traits> y ) noexcept \
    { \
        return x.compare(y) OP 0; \
    } \
    template<class charT, class traits> \
    constexpr bool operator OP(basic_string_view<charT, traits> x, \
                               std::basic_string<charT, traits> const& y ) noexcept \
    { \
        return x.compare(basic_string_view<charT, traits>(y)) OP 0; \
    } \
    template<class charT, class traits> \
    constexpr bool operator OP(std::basic_string<charT, traits> const& x, \
                               basic_string_view<charT, traits> y ) noexcept \
    { \
        return basic_string_view<charT, traits>(x).compare(y) OP 0; \
    } \
    template<class charT, class traits> \
    constexpr bool operator OP(basic_string_view<charT, traits> x, \
                               charT const* y) noexcept \
    { \
        return x.compare(basic_string_view<charT, traits>(y)) OP 0; \
    } \
    template<class charT, class traits> \
    constexpr bool operator OP(charT const* x, \
                               basic_string_view<charT, traits> y ) noexcept \
    { \
        return basic_string_view<charT, traits>(x).compare(y) OP 0; \
    }

MAKE_STRING_VIEW_OPERATOR( == )
MAKE_STRING_VIEW_OPERATOR( != )
MAKE_STRING_VIEW_OPERATOR( < )
MAKE_STRING_VIEW_OPERATOR( > )
MAKE_STRING_VIEW_OPERATOR( <= )
MAKE_STRING_VIEW_OPERATOR( >= )

#undef MAKE_STRING_VIEW_OPERATOR

template<class charT, class traits>
std::basic_ostream<charT, traits> &
operator<<( std::basic_ostream<charT, traits> &os,
            basic_string_view<charT, traits> v )
{
    return os.write( v.data(), v.size() );
}

#endif

