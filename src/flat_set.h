#pragma once
#ifndef CATA_FLAT_SET
#define CATA_FLAT_SET

#include <vector>

namespace cata
{

struct transparent_less_than {
    using is_transparent = void;

    template<typename T, typename U>
    constexpr auto operator()( T &&lhs, U &&rhs ) const
    -> decltype( std::forward<T>( lhs ) < std::forward<U>( rhs ) ) {
        return std::forward<T>( lhs ) < std::forward<U>( rhs );
    }
};

/**
 * @brief A SimpleAssociativeContainer implemented as a sorted vector.
 *
 * O(n) insertion, O(log(n)) lookup, only one allocation at any given time.
 */
template<typename T, typename Compare = transparent_less_than, typename Data = std::vector<T>>
class flat_set : private Compare, Data
{
    public:
        using typename Data::value_type;
        using typename Data::const_reference;
        using typename Data::const_pointer;
        using key_type = value_type;
        using key_compare = Compare;
        using value_compare = Compare;
        using typename Data::size_type;
        using typename Data::difference_type;
        using typename Data::iterator;
        using typename Data::const_iterator;
        using typename Data::reverse_iterator;
        using typename Data::const_reverse_iterator;

        flat_set() = default;
        flat_set( const key_compare &kc ) : Compare( kc ) {}
        template<typename InputIt>
        flat_set( InputIt first, InputIt last ) : Data( first, last ) {
            sort_data();
        }
        template<typename InputIt>
        flat_set( InputIt first, InputIt last, const key_compare &kc ) :
            Compare( kc ), Data( first, last ) {
            sort_data();
        }

        const key_compare &key_comp() const {
            return *this;
        }
        const value_compare &value_comp() const {
            return *this;
        }

        using Data::size;
        using Data::max_size;
        using Data::empty;

        using Data::reserve;
        using Data::capacity;
        using Data::shrink_to_fit;

        using Data::begin;
        using Data::end;
        using Data::rbegin;
        using Data::rend;
        using Data::cbegin;
        using Data::cend;
        using Data::crbegin;
        using Data::crend;

        iterator lower_bound( const T &t ) {
            return std::lower_bound( begin(), end(), t, key_comp() );
        }
        const_iterator lower_bound( const T &t ) const {
            return std::lower_bound( begin(), end(), t, key_comp() );
        }
        iterator upper_bound( const T &t ) {
            return std::upper_bound( begin(), end(), t, key_comp() );
        }
        const_iterator upper_bound( const T &t ) const {
            return std::upper_bound( begin(), end(), t, key_comp() );
        }
        std::pair<iterator, iterator> equal_range( const T &t ) {
            return { lower_bound( t ), upper_bound( t ) };
        }
        std::pair<const_iterator, const_iterator> equal_range( const T &t ) const {
            return { lower_bound( t ), upper_bound( t ) };
        }

        iterator find( const value_type &value ) {
            return find_impl( *this, value );
        }
        const_iterator find( const value_type &value ) const {
            return find_impl( *this, value );
        }
        size_type count( const T &t ) const {
            auto at = lower_bound( t );
            return at != end() && *at == t;
        }

        std::pair<iterator, bool> insert( iterator, const value_type &value ) {
            /// @todo Use insertion hint
            return insert( value );
        }
        std::pair<iterator, bool> insert( iterator, value_type &&value ) {
            /// @todo Use insertion hint
            return insert( std::move( value ) );
        }
        std::pair<iterator, bool> insert( const value_type &value ) {
            auto at = lower_bound( value );
            if( at != end() && *at == value ) {
                return { at, false };
            }
            return { Data::insert( at, value ), true };
        }
        std::pair<iterator, bool> insert( value_type &&value ) {
            auto at = lower_bound( value );
            if( at != end() && *at == value ) {
                return { at, false };
            }
            return { Data::insert( at, std::move( value ) ), true };
        }

        template<typename InputIt>
        void insert( InputIt first, InputIt last ) {
            /// @todo could be faster when inserting only a few elements
            Data::insert( end(), first, last );
            sort_data();
        }

        using Data::clear;
        using Data::erase;
        size_type erase( const value_type &value ) {
            auto at = find( value );
            if( at != end() ) {
                erase( at );
                return 1;
            }
            return 0;
        }

        friend void swap( flat_set &l, flat_set &r ) {
            using std::swap;
            swap( static_cast<Compare &>( l ), static_cast<Compare &>( r ) );
            swap( static_cast<Data &>( l ), static_cast<Data &>( r ) );
        }
    private:
        template<typename FlatSet>
        static auto find_impl( FlatSet &s, const value_type &value ) -> decltype( s.end() ) {
            auto at = s.lower_bound( value );
            if( at != s.end() && *at == value ) {
                return at;
            }
            return s.end();
        }
        void sort_data() {
            std::sort( begin(), end(), key_comp() );
            auto new_end = std::unique( begin(), end() );
            Data::erase( new_end, end() );
        }
};

}

#endif // CATA_FLAT_SET
