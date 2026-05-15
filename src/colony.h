#pragma once
#ifndef CATA_SRC_COLONY_H
#define CATA_SRC_COLONY_H

#include <cstddef>
#include <memory>

#include <plf/colony.h> // IWYU pragma: export

namespace cata
{
template<class T, class Allocator = std::allocator<T>>
using colony = plf::colony<T, Allocator>;

// plf::colony has no index<->iterator member helpers; use ADL friend
// advance/distance on the iterator (std::distance is not specialized for
// plf iterators and would fall back to a linear walk).
template<class T, class Allocator>
inline std::size_t get_index_from_iterator( const colony<T, Allocator> &c,
        typename colony<T, Allocator>::const_iterator it )
{
    return static_cast<std::size_t>( distance( c.cbegin(), it ) );
}

template<class T, class Allocator>
inline typename colony<T, Allocator>::iterator get_iterator_from_index(
    colony<T, Allocator> &c, std::size_t idx )
{
    typename colony<T, Allocator>::iterator it = c.begin();
    if( idx == 0 || c.empty() ) {
        return it;
    }
    advance( it, static_cast<typename colony<T, Allocator>::iterator::difference_type>( idx ) );
    return it;
}
} // namespace cata

#endif // CATA_SRC_COLONY_H
