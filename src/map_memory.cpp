#include "map_memory.h"

template<typename T>
T lru_cache<T>::get( const tripoint &pos, const T &default_ ) const
{
    auto found = map.find( pos );
    if( found != map.end() ) {
        return found->second->second;
    }
    return default_;
}

template<typename T>
void lru_cache<T>::remove( const tripoint &pos )
{
    auto found = map.find( pos );
    if( found != map.end() ) {
        ordered_list.erase( found->second );
        map.erase( found );
    }
}

template<typename T>
void lru_cache<T>::insert( int limit, const tripoint &pos, const T &t )
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

template<typename T>
void lru_cache<T>::trim( int limit )
{
    while( map.size() > static_cast<size_t>( limit ) ) {
        map.erase( ordered_list.front().first );
        ordered_list.pop_front();
    }
}

template<typename T>
void lru_cache<T>::clear()
{
    map.clear();
    ordered_list.clear();
}

template<typename T>
const std::list<typename lru_cache<T>::Pair> &lru_cache<T>::list() const
{
    return ordered_list;
}

template class lru_cache<memorized_terrain_tile>;
template class lru_cache<long>;

static const memorized_terrain_tile default_tile{ "", 0, 0 };

memorized_terrain_tile map_memory::get_tile( const tripoint &pos ) const
{
    return tile_cache.get( pos, default_tile );
}

void map_memory::memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                                const int subtile, const int rotation )
{
    tile_cache.insert( limit, pos, memorized_terrain_tile{ ter, subtile, rotation } );
}

long map_memory::get_symbol( const tripoint &pos ) const
{
    return symbol_cache.get( pos, 0 );
}

void map_memory::memorize_symbol( int limit, const tripoint &pos, const long symbol )
{
    symbol_cache.insert( limit, pos, symbol );
}

void map_memory::clear_memorized_tile( const tripoint &pos )
{
    tile_cache.remove( pos );
    symbol_cache.remove( pos );
}
