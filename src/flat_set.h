#pragma once
#ifndef CATA_FLAT_SET
#define CATA_FLAT_SET

#include <algorithm>
#include <vector>

namespace cata
{

struct transparent_less_than {
    using is_transparent = void;

    template<typename T, typename U>
    constexpr auto operator()( T &&lhs, U &&rhs ) const noexcept
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
    private:
        template<typename Cmp, typename Sfinae, typename = void>
        struct has_is_transparent {};
        template<typename Cmp, typename Sfinae>
        struct has_is_transparent<Cmp, Sfinae, typename Cmp::is_transparent> {
            using type = void;
        };

    public:
        using typename Data::value_type;
        using typename Data::const_reference;
        using typename Data::const_pointer;
        using key_type = value_type;
        using key_compare = Compare;
        using value_compare = Compare;
        using typename Data::size_type;
        using typename Data::difference_type;
        using typename Data::const_iterator;
        using iterator = const_iterator;
        using typename Data::const_reverse_iterator;
        using reverse_iterator = const_reverse_iterator;

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
        flat_set( std::initializer_list<value_type> init ) : Data( init ) {
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

        iterator begin() const {
            return cbegin();
        }
        iterator end() const {
            return cend();
        }
        reverse_iterator rbegin() const {
            return crbegin();
        }
        reverse_iterator rend() const {
            return crend();
        }
        using Data::cbegin;
        using Data::cend;
        using Data::crbegin;
        using Data::crend;

        const_reference operator[]( size_type i ) const {
            return Data::operator[]( i );
        }

        const_iterator lower_bound( const T &t ) const {
            return std::lower_bound( begin(), end(), t, key_comp() );
        }
        template<typename K, typename = typename has_is_transparent<Compare, K>::type>
        const_iterator lower_bound( const K &k ) const {
            return std::lower_bound( begin(), end(), k, key_comp() );
        }
        const_iterator upper_bound( const T &t ) const {
            return std::upper_bound( begin(), end(), t, key_comp() );
        }
        template<typename K, typename = typename has_is_transparent<Compare, K>::type>
        const_iterator upper_bound( const K &k ) const {
            return std::upper_bound( begin(), end(), k, key_comp() );
        }
        std::pair<const_iterator, const_iterator> equal_range( const T &t ) const {
            return { lower_bound( t ), upper_bound( t ) };
        }
        template<typename K, typename = typename has_is_transparent<Compare, K>::type>
        std::pair<const_iterator, const_iterator> equal_range( const K &k ) const {
            return { lower_bound( k ), upper_bound( k ) };
        }

        const_iterator find( const value_type &value ) const {
            auto at = lower_bound( value );
            if( at != end() && *at == value ) {
                return at;
            }
            return end();
        }
        template<typename K, typename = typename has_is_transparent<Compare, K>::type>
        const_iterator find( const K &k ) const {
            auto at = lower_bound( k );
            if( at != end() && *at == k ) {
                return at;
            }
            return end();
        }
        size_type count( const T &t ) const {
            auto at = lower_bound( t );
            return at != end() && *at == t;
        }
        template<typename K, typename = typename has_is_transparent<Compare, K>::type>
        size_type count( const K &k ) const {
            auto at = lower_bound( k );
            return at != end() && *at == k;
        }

        iterator insert( iterator, const value_type &value ) {
            /// TODO: Use insertion hint
            return insert( value ).first;
        }
        iterator insert( iterator, value_type &&value ) {
            /// TODO: Use insertion hint
            return insert( std::move( value ) ).first;
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
            /// TODO: could be faster when inserting only a few elements
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
#define FLAT_SET_OPERATOR( op ) \
    friend bool operator op( const flat_set &l, const flat_set &r ) { \
        return l.data() op r.data(); \
    }
        FLAT_SET_OPERATOR( == )
        FLAT_SET_OPERATOR( != )
        FLAT_SET_OPERATOR( < )
        FLAT_SET_OPERATOR( <= )
        FLAT_SET_OPERATOR( > )
        FLAT_SET_OPERATOR( >= )
#undef FLAT_SET_OPERATOR
    private:
        const Data &data() const {
            return *this;
        }
        void sort_data() {
            std::sort( Data::begin(), Data::end(), key_comp() );
            auto new_end = std::unique( Data::begin(), Data::end() );
            Data::erase( new_end, end() );
        }
};

} // namespace cata

#endif // CATA_FLAT_SET
