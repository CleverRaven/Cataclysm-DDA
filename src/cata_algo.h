#pragma once
#ifndef CATA_ALGO_H
#define CATA_ALGO_H

#include <algorithm>
#include <vector>

namespace algo
{

/**
 * @brief Sort elements of a sequence by their rating (the smaller the better).
 *
 * @param  begin           An input/output iterator.
 * @param  end             An input/output iterator.
 * @param  rating_func     A functor which calculates ratings (the outcome
 *                         must be comparable using '<' operator).
 *
 * The rating is calculated only once per each element.
 */
template<typename Iterator, typename RatingFunction>
void sort_by_rating( Iterator begin, Iterator end, RatingFunction rating_func )
{
    using value_t = typename Iterator::value_type;
    using pair_t = std::pair<value_t, decltype( rating_func( *begin ) )>;

    std::vector<pair_t> rated_entries;
    rated_entries.reserve( std::distance( begin, end ) );

    std::transform( begin, end, std::back_inserter( rated_entries ),
    [ &rating_func ]( const value_t &elem ) {
        return std::make_pair( elem, rating_func( elem ) );
    } );

    std::sort( rated_entries.begin(), rated_entries.end(), []( const pair_t &lhs, const pair_t &rhs ) {
        return lhs.second < rhs.second;
    } );

    std::transform( rated_entries.begin(), rated_entries.end(), begin, []( const pair_t &elem ) {
        return elem.first;
    } );
}

} // namespace algo

#endif // CATA_ALGO_H
