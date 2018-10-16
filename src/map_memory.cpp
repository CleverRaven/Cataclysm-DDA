#include "map_memory.h"

#include "game_constants.h"

#include <algorithm>

memorized_terrain_tile map_memory::get_memorized_terrain( const tripoint &pos ) const
{
    if( memorized_terrain.find( pos ) != memorized_terrain.end() ) {
        return memorized_terrain.at( pos );
    }
    return { "", 0, 0 };
}

void map_memory::memorize_tile( const tripoint &pos, const std::string &ter, const int subtile,
                                const int rotation )
{
    memorized_terrain_tmp[pos] = { ter, subtile, rotation };
}

void map_memory::finalize_tile_memory( size_t max_submaps )
{
    memorize_tiles( memorized_terrain_tmp, max_submaps );
    memorized_terrain_tmp.clear();
}

void map_memory::memorize_tiles( const std::map<tripoint, memorized_terrain_tile> &tiles,
                                 const size_t max_submaps )
{
    std::set<tripoint> submaps;
    for( auto i : tiles ) {
        const tripoint &p = i.first;
        submaps.insert( { p.x / SEEX, p.y / SEEY, p.z } );
        memorized_terrain[p] = i.second;
    }

    update_submap_memory( submaps, max_submaps );
}

void map_memory::update_submap_memory( const std::set<tripoint> &submaps, const size_t max_submaps )
{
    std::set<tripoint> erase;
    for( auto i : submaps ) {
        std::vector<tripoint>::iterator position = std::find( memorized_submaps.begin(),
                memorized_submaps.end(), i );
        if( position != memorized_submaps.end() ) {
            memorized_submaps.erase( position );
        }
        memorized_submaps.push_back( i );
    }

    while( memorized_submaps.size() > max_submaps ) {
        erase.insert( memorized_submaps.front() );
        memorized_submaps.erase( memorized_submaps.begin() );
    }
    if( !erase.empty() ) {
        clear_submap_memory( erase );
    }
}

void map_memory::clear_submap_memory( const std::set<tripoint> &erase )
{
    for( auto it = memorized_terrain.cbegin(); it != memorized_terrain.cend(); ) {
        bool delete_this = false;
        for( auto i : erase ) {
            if( ( it->first.x / SEEX == i.x ) && ( it->first.y / SEEY == i.y ) && ( it->first.z == i.z ) ) {
                delete_this = true;
                break;
            }
        }
        if( delete_this ) {
            memorized_terrain.erase( it++ );
        } else {
            ++it;
        }
    }
    for( auto it = memorized_terrain_curses.cbegin(); it != memorized_terrain_curses.cend(); ) {
        bool delete_this = false;
        for( auto i : erase ) {
            if( ( it->first.x / SEEX == i.x ) && ( it->first.y / SEEY == i.y ) && ( it->first.z == i.z ) ) {
                delete_this = true;
                break;
            }
        }
        if( delete_this ) {
            memorized_terrain_curses.erase( it++ );
        } else {
            ++it;
        }
    }
}

void map_memory::memorize_terrain_symbol( const tripoint &pos, const long symbol )
{
    memorized_terrain_curses_tmp[pos] = symbol;
}

void map_memory::finalize_terrain_memory_curses( const size_t max_submaps )
{
    memorize_terrain_symbols( memorized_terrain_curses_tmp, max_submaps );
    memorized_terrain_curses_tmp.clear();
}

void map_memory::memorize_terrain_symbols( const std::map<tripoint, long> &tiles,
        size_t max_submaps )
{
    std::set<tripoint> submaps;
    for( auto i : tiles ) {
        const tripoint &p = i.first;
        submaps.insert( { p.x / SEEX, p.y / SEEY, p.z } );
        memorized_terrain_curses[p] = i.second;
    }

    update_submap_memory( submaps, max_submaps );
}

long map_memory::get_memorized_terrain_curses( const tripoint &pos ) const
{
    if( memorized_terrain_curses.find( pos ) != memorized_terrain_curses.end() ) {
        return memorized_terrain_curses.at( pos );
    }
    return 0;
}
