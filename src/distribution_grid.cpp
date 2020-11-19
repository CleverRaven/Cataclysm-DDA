#include "distribution_grid.h"
#include "coordinate_conversions.h"
#include "active_tile_data.h"
#include "mapbuffer.h"
#include "submap.h"

distribution_grid::distribution_grid( const std::vector<tripoint> &global_submap_coords )
{
    for( const tripoint &sm_coord : global_submap_coords ) {
        submap *sm = MAPBUFFER.lookup_submap( sm_coord );
        if( sm == nullptr ) {
            // Debugmsg already printed in mapbuffer.cpp
            return;
        }

        const tripoint ms_pos = sm_to_ms_copy( sm_coord );
        for( auto &active : sm->active_furniture ) {
            contents[sm_coord].emplace_back( active.first, ms_pos + active.first );
        }
    }
}

bool distribution_grid::empty() const
{
    return contents.empty();
}

void distribution_grid::update( time_point to )
{
    for( const std::pair<tripoint, std::vector<tile_location>> &c : contents ) {
        submap *sm = MAPBUFFER.lookup_submap( c.first );
        if( sm == nullptr ) {
            return;
        }

        for( const tile_location &loc : c.second ) {
            auto &active = sm->active_furniture[loc.on_submap];
            if( !active ) {
                debugmsg( "No active furniture at %d,%d,%d",
                          loc.absolute.x, loc.absolute.y, loc.absolute.z );
                contents.clear();
                return;
            }
            active->update( to, loc.absolute, *this );
        }
    }
}

int distribution_grid::mod_resource( int amt )
{
    for( const std::pair<tripoint, std::vector<tile_location>> &c : contents ) {
        if( amt == 0 ) {
            return 0;
        }
        submap *sm = MAPBUFFER.lookup_submap( c.first );
        if( sm == nullptr ) {
            continue;
        }

        for( const tile_location &loc : c.second ) {
            amt = sm->active_furniture[loc.on_submap]->mod_resource( amt );
        }
    }

    return amt;
}

int distribution_grid::get_resource() const
{
    int res = 0;
    for( const std::pair<tripoint, std::vector<tile_location>> &c : contents ) {
        submap *sm = MAPBUFFER.lookup_submap( c.first );
        if( sm == nullptr ) {
            continue;
        }

        for( const tile_location &loc : c.second ) {
            res += sm->active_furniture[loc.on_submap]->get_resource();
        }
    }

    return res;
}
