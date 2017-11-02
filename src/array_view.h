#pragma once
#ifndef ARRAY_VIEW_H
#define ARRAY_VIEW_H
#include <stdexcept>
#include <iosfwd>
#include <utility>
#include <type_traits>
#include <iterator>
#include <initializer_list>

template< class DataT >
class array_view
{
    protected:
        // Data
        DataT const *_begin = nullptr;
        DataT const *_end   = nullptr;

    public:
        // Types
        using value_type             = DataT;
        using pointer                = value_type *;
        using const_pointer          = value_type const*;
        using reference              = value_type &;
        using const_reference        = value_type const&;
        using const_iterator         = value_type const*;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type              = unsigned int;
        enum constant : size_type { npos = static_cast<size_type>( -1 ) }; 

        // Constructors
        constexpr array_view() noexcept = default;
        constexpr array_view( array_view const & ) noexcept = default;
        array_view &operator=( array_view const & ) noexcept = default;

        template<size_type N>
        constexpr array_view( value_type const( &s )[N] ) noexcept :
            _begin(s), _end(s + N)
        {}

        constexpr array_view( value_type const *s, value_type const *e ) noexcept :
            _begin(s), _end(e)
        {}

        constexpr array_view( value_type const *s, size_type cnt ) noexcept :
            _begin(s), _end(s + cnt)
        {}

        /** Allows initializer lists to be used
         *
         * void foo(array_view<int>);
         * foo({1, 2, 3, 4});
         */
        constexpr array_view( std::initializer_list<value_type> l) noexcept :
            _begin(l.begin()), _end(l.end())
        {}

        /** Allow any type where .data() and .data() + .size() can be converted
         *  into valid pointers
         *
         * In otherwords, any kind of container storing contiguous memory,
         * std::vector, std::array, std::string,
         */
        template <
            class T,
            class = decltype( pointer( std::declval<T>().data() ) ),
            class = decltype( pointer( std::declval<T>().data() + std::declval<T>().size() ) )
            >
        constexpr array_view( T const &s ) noexcept :
            _begin(s.data()), _end(s.data() + s.size())
        {}

        // Iterators
        constexpr const_iterator begin() const noexcept {
            return _begin;
        }
        constexpr const_iterator cbegin() const noexcept {
            return begin();
        }
        constexpr const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator( end() );
        }

        constexpr const_iterator end() const noexcept {
            return _end;
        }
        constexpr const_iterator cend() const noexcept {
            return end();
        }
        constexpr const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator( begin() );
        }

        // Accessors
        constexpr const_reference operator[]( size_type idx ) const noexcept {
            return begin()[idx];
        }

        const_reference at( size_type idx ) const {
            if( begin() + idx >= end() ) {
                out_of_range();
            }
            return begin()[idx];
        }

        constexpr const_reference front() const noexcept {
            return begin()[0];
        }

        constexpr const_reference back() const noexcept {
            return end()[-1];
        }

        constexpr const_pointer data() const noexcept {
            return begin();
        }

        // Capacity
        constexpr size_type size() const noexcept {
            return static_cast<size_type>(end() - begin());
        }

        constexpr bool empty() const noexcept {
            return begin() == end();
        }

        constexpr explicit operator bool() const noexcept {
            return ! empty();
        }

        // Modifiers
        void remove_prefix( size_type n ) noexcept {
            if( ( _begin += n ) > end() ) {
                _begin = end();
            }
        }

        void remove_suffix( size_type n ) noexcept {
            if( ( _end -= n ) < begin() ) {
                _end = begin();
            }
        }

        void swap( array_view &v ) noexcept {
            std::swap( _begin, v._begin );
            std::swap( _end,   v._end );
        }

        // Operations
        array_view substr( size_type pos = 0, size_type count = npos ) const {
            if( pos > size() ) {
                out_of_range();
            }
            return {
                begin() + pos,
                (count != npos) ? std::min( count, size() - pos ) : size() - pos
            };
        }

        size_type find( const_reference val ) const {
            for( size_type i = 0; i < size(); ++i ) {
                if( (*this)[i] == val) {
                    return val;
                }
            }
            return npos;
        }

    protected:
        [[noreturn]]
        static void out_of_range() {
            throw std::out_of_range( "Out of bounds access in string_view" );
        }
};

#endif

