#include "lru_cache.h"

#include <cstddef>
#include <iterator>

#include "map_memory.h"
#include "point.h"

template<typename Key, typename Value>
Value lru_cache<Key, Value>::get( const Key &pos, const Value &default_ ) const
{
    auto found = map.find( pos );
    if( found != map.end() ) {
        return found->second->second;
    }
    return default_;
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::remove( const Key &pos )
{
    auto found = map.find( pos );
    if( found != map.end() ) {
        ordered_list.erase( found->second );
        map.erase( found );
    }
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::insert( int limit, const Key &pos, const Value &t )
{
    auto found = map.find( pos );

    if( found == map.end() ) {
        // Need new entry in map.  Make the new list entry and point to it.
        ordered_list.emplace_back( pos, t );
        map[pos] = std::prev( ordered_list.end() );
        trim( limit );
    } else {
        // Splice existing entry to the back.  Does not invalidate the
        // iterator, so no need to change the map.
        auto list_iterator = found->second;
        ordered_list.splice( ordered_list.end(), ordered_list, list_iterator );
        // Update the moved item
        list_iterator->second = t;
    }
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::trim( int limit )
{
    while( map.size() > static_cast<size_t>( limit ) ) {
        map.erase( ordered_list.front().first );
        ordered_list.pop_front();
    }
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::clear()
{
    map.clear();
    ordered_list.clear();
}

template<typename Key, typename Value>
const std::list<typename lru_cache<Key, Value>::Pair> &lru_cache<Key, Value>::list() const
{
    return ordered_list;
}

// explicit template initialization for lru_cache of all types
template class lru_cache<tripoint, memorized_terrain_tile>;
template class lru_cache<tripoint, int>;
template class lru_cache<point, char>;
