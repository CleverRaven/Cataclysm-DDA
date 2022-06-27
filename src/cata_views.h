#pragma once
#ifndef CATA_SRC_CATA_VIEWS_H
#define CATA_SRC_CATA_VIEWS_H

#include <functional>

namespace cata
{
namespace views
{

/*
 * A lazily-evaluated transformation on an STL container for zero-copy iteration.
 *
 * Example:
 *     std::vector<int> vec{ 1, 2, 3, 4, 5 };
 *     auto square = []( const int &x ) -> int { return x * x; };
 *     for( auto i : cata::views::transform<std::vector<int>, int>( vec, square ) ) {
 *         // You get i = { 1, 4, 9, 16, 25 } in this ranged-based for loop,
 *         // but avoids constructing a temporary std::vector<int> and copy over all elements
 *     }
 *
 * You can even transform elements into a different type:
 *     std::vector<intr> vec{ 1, 2, 3, 4, 5 };
 *     auto stringify = []( const int &x ) -> std::string {
 *         return string_format( _( "Square of %d is %d." ), x, x * x );
 *     };
 *     for( auto s : cata::views::transform<std::vector<int>, std::string>( vec, stringify ) ) {
 *         you.add_msg_if_player( s );
 *     }
 *
 * typename A is the type of the STL *container*, e.g. std::vector<mutable_overmap_placement_rule_piece>
 * typename B is the type of *element* after transformation
 */
template<typename A, typename B>
class transform
{
    private:
        using fun_t = std::function<B( const typename A::value_type & )>;

        fun_t fun;
        typename A::const_iterator it_begin, it_end;

        class iterator
        {
            private:
                typename A::const_iterator it;
                const fun_t &fun;
            public:
                explicit iterator( const typename A::const_iterator &it, const fun_t &fun )
                    : it( it ), fun( fun ) {}
                bool operator != ( const iterator &rhs ) const {
                    return this->it != rhs.it;
                }
                const iterator &operator++() {
                    ++it;
                    return *this;
                }
                B operator*() const {
                    return fun( *it );
                }

                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = B;
                using pointer = value_type*;
                using reference = value_type&;
        };
    public:
        explicit transform( const A &container, const fun_t &fun ) : fun( fun ),
            it_begin( std::begin( container ) ), it_end( std::end( container ) ) {}
        explicit transform( const typename A::const_iterator &it_begin,
                            const typename A::const_iterator &it_end,
                            const fun_t &fun )
            : fun( fun ), it_begin( it_begin ), it_end( it_end ) {}
        iterator begin() {
            return iterator( it_begin, fun );
        }
        iterator end() {
            return iterator( it_end, fun );
        }
};
} // namespace views
} // namespace cata

#endif // CATA_SRC_CATA_VIEWS_H
