#pragma once
#ifndef CATA_SRC_STRING_ID_UTILS_H
#define CATA_SRC_STRING_ID_UTILS_H

#include "string_id.h"
#include <algorithm>
#include <vector>

// Helper methods that sort collections of string_ids lexicographically into a temporary vector
// Slow (relative), but convenient to use for UI purposes

/**
 * Returns vector sorted by lexicographic order of string_id
 * @param col collection that supports iterators of std::pair<string_id<T>, V>
 * @return sorted (using string_id::LexCmp by std::pair::first) `std::vector<std::pair<string_id<T>, V>>`
 */
template<typename Col,
         typename El = std::decay_t<decltype( *std::declval<const Col &>().begin() )>,
         typename K =  std::decay_t<typename El::first_type>,
         typename V = std::decay_t<typename El::second_type>,
         std::enable_if_t<std::is_same<K, string_id<typename K::value_type>>::value, int> = 0>
std::vector<std::pair<K, V>> sorted_lex( Col col )
{
    std::vector<std::pair<K, V>> ret;
    ret.insert( ret.begin(), col.begin(), col.end() );
    std::sort( ret.begin(), ret.end(), []( const auto & a, const auto & b ) {
        return typename K::LexCmp()( a.first, b.first );
    } );
    return ret;
}

/**
 * Returns vector sorted by lexicographic order of string_id
 * @param col collection that supports iterators of string_id<T>
 * @return sorted (using string_id::LexCmp) `std::vector<string_id<T>>`
 */
template<typename Col,
         typename El = std::decay_t<decltype( *std::declval<const Col &>().begin() )>,
         std::enable_if_t<std::is_same<El, string_id<typename El::value_type>>::value, int> = 0>
std::vector<El> sorted_lex( Col col )
{
    std::vector<El> ret;
    ret.insert( ret.begin(), col.begin(), col.end() );
    std::sort( ret.begin(), ret.end(), typename El::LexCmp() );
    return ret;
}

#endif // CATA_SRC_STRING_ID_UTILS_H
