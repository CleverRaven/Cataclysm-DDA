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
template<typename T, typename Compare = transparent_less_than>
class flat_set : private Compare
{
    private:
        using Data = std::vector<T>;
    public:
        using value_type = typename Data::value_type;
        using const_reference = typename Data::const_reference;
        using const_pointer = typename Data::const_pointer;
        using key_type = value_type;
        using key_compare = Compare;
        using value_compare = Compare;
        using size_type = typename Data::size_type;
        using difference_type = typename Data::difference_type;
        using iterator = typename Data::iterator;
        using const_iterator = typename Data::const_iterator;
        using reverse_iterator = typename Data::reverse_iterator;
        using const_reverse_iterator = typename Data::const_reverse_iterator;

        flat_set() = default;
        flat_set( const key_compare &kc ) : Compare( kc ) {}
        template<typename InputIt>
        flat_set( InputIt first, InputIt last ) : data( first, last ) {
            sort_data();
        }
        template<typename InputIt>
        flat_set( InputIt first, InputIt last, const key_compare &kc ) :
            Compare( kc ), data( first, last ) {
            sort_data();
        }

        const key_compare &key_comp() const {
            return *this;
        }
        const value_compare &value_comp() const {
            return *this;
        }

        size_type size() const {
            return data.size();
        }
        constexpr size_type max_size() const {
            return data.max_size();
        }
        bool empty() const {
            return data.empty();
        }

        iterator begin() {
            return data.begin();
        }
        const_iterator begin() const {
            return data.begin();
        }
        iterator end() {
            return data.end();
        }
        const_iterator end() const {
            return data.end();
        }

        reverse_iterator rbegin() {
            return data.rbegin();
        }
        const_reverse_iterator rbegin() const {
            return data.rbegin();
        }
        reverse_iterator rend() {
            return data.rend();
        }
        const_reverse_iterator rend() const {
            return data.rend();
        }

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
            return { data.insert( at, value ), true };
        }
        std::pair<iterator, bool> insert( value_type &&value ) {
            auto at = lower_bound( value );
            if( at != end() && *at == value ) {
                return { at, false };
            }
            return { data.insert( at, std::move( value ) ), true };
        }

        template<typename InputIt>
        void insert( InputIt first, InputIt last ) {
            /// @todo could be faster when inserting only a few elements
            data.insert( data.end(), first, last );
            sort_data();
        }

        size_type erase( const value_type &value ) {
            auto at = find( value );
            if( at != end() ) {
                erase( at );
                return 1;
            }
            return 0;
        }
        iterator erase( const_iterator at ) {
            return data.erase( at );
        }
        iterator erase( const_iterator first, const_iterator last ) {
            return data.erase( first, last );
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
            data.erase( new_end, end() );
        }

        Data data;
};

}

#endif // CATA_FLAT_SET
