#pragma once
#ifndef ARRAY_VIEW_H
#define ARRAY_VIEW_H
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <iterator>
#include <initializer_list>
#include <memory>

template< class DataT >
class array_view
{
    public:
        using value_type             = DataT;
        using pointer                = value_type *;
        using const_pointer          = value_type const*;
        using reference              = value_type &;
        using const_reference        = value_type const&;
        using const_iterator         = value_type const*;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using size_type              = size_t;
        enum constant : size_type { npos = static_cast<size_type>( -1 ) };

        // Constructors
        constexpr array_view() noexcept = default;
        constexpr array_view( array_view const & ) noexcept = default;
        array_view &operator=( array_view const & ) noexcept = default;

        // construct view from c arrays
        template<size_type N>
        constexpr array_view( value_type const( &s )[N] ) noexcept :
            _data( s ), _size( N )
        {}

        // construct view from begin and and end pointers
        constexpr array_view( value_type const *s, value_type const *e ) noexcept :
            _data( s ), _size( static_cast<size_type>( e - s ) )
        {}

        // construct view from pointer and size
        constexpr array_view( value_type const *s, size_type cnt ) noexcept :
            _data( s ), _size( cnt )
        {}

        // construct view from initializer lists
        constexpr array_view( std::initializer_list<value_type> l ) noexcept :
            _data( l.begin() ), _size( l.size() )
        {}

        // construct view to single value
        constexpr array_view( const_reference v ) noexcept :
            _data( std::addressof( v ) ), _size( 1 )
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
            class = decltype( size_type( std::declval<T>().size() ) ),
            class = decltype( pointer( std::declval<T>().data() + std::declval<T>().size() ) )
            >
        constexpr array_view( T const &s ) noexcept :
            _data( s.data() ), _size( s.size() )
        {
            static_assert(sizeof(*s.data()) == sizeof(value_type), "Container data would get sliced");
        }

        // Iterators
        constexpr const_iterator begin() const noexcept {
            return _data;
        }
        constexpr const_iterator cbegin() const noexcept {
            return begin();
        }
        constexpr const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator( end() );
        }

        constexpr const_iterator end() const noexcept {
            return _data + _size;
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
            if( idx >= size() ) {
                out_of_range();
            }
            return begin()[idx];
        }

        // invalid when empty() == true
        constexpr const_reference front() const noexcept {
            return begin()[0];
        }

        // invalid when empty() == true
        constexpr const_reference back() const noexcept {
            return begin()[size() - 1];
        }

        constexpr const_pointer data() const noexcept {
            return begin();
        }

        // Capacity
        constexpr size_type size() const noexcept {
            return _size;
        }

        constexpr bool empty() const noexcept {
            return size() == 0;
        }

        constexpr explicit operator bool() const noexcept {
            return !empty();
        }

        // Modifiers
        // removes first n elements,
        void remove_prefix( size_type n ) noexcept {
            n = std::min( n, size() );
            _data += n;
            _size -= n;
        }

        // removes last n elements,
        void remove_suffix( size_type n ) noexcept {
            n = std::min( n, size() );
            _size -= n;
        }

        void swap( array_view &v ) noexcept {
            std::swap( _data, v._data );
            std::swap( _size, v._size );
        }

        // Operations
        // makes substr of range  [pos, pos+rcount), unlike standard, doesnt throw. just returns empty
        array_view substr( size_type pos = 0, size_type n = npos ) const noexcept {
            if( pos > size() ) {
                return array_view{};
            }
            if( n == npos || pos + n >= size() ) {
                n = size() - pos;
            }
            return array_view{ data() + pos, n };
        }
    protected:
        [[noreturn]]
        static void out_of_range() {
            throw std::out_of_range( "Out of bounds access in string_view" );
        }

        // Data
        const_pointer _data = nullptr;
        size_type     _size = 0;
};

#endif

