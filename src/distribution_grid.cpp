#include <unordered_set>

#include "distribution_grid.h"
#include "coordinate_conversions.h"
#include "active_tile_data.h"
#include "map.h"
#include "mapbuffer.h"
#include "map_iterator.h"
#include "submap.h"
#include "overmapbuffer.h"

distribution_grid::distribution_grid( const std::vector<tripoint> &global_submap_coords,
                                      mapbuffer &buffer ) :
    submap_coords( global_submap_coords ),
    mb( buffer )
{
    for( const tripoint &sm_coord : submap_coords ) {
        submap *sm = mb.lookup_submap( sm_coord );
        if( sm == nullptr ) {
            // Debugmsg already printed in mapbuffer.cpp
            return;
        }

        for( auto &active : sm->active_furniture ) {
            const tripoint ms_pos = sm_to_ms_copy( sm_coord );
            contents[sm_coord].emplace_back( active.first, ms_pos + active.first );
        }
    }
}

bool distribution_grid::empty() const
{
    return contents.empty();
}

distribution_grid::operator bool() const
{
    return !empty() && !submap_coords.empty();
}

void distribution_grid::update( time_point to )
{
    for( const std::pair<const tripoint, std::vector<tile_location>> &c : contents ) {
        submap *sm = mb.lookup_submap( c.first );
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

#include "vehicle.h"
int distribution_grid::mod_resource( int amt, bool recurse )
{
    std::vector<vehicle *> connected_vehicles;
    for( const std::pair<const tripoint, std::vector<tile_location>> &c : contents ) {
        for( const tile_location &loc : c.second ) {
            battery_tile *battery = active_tiles::furn_at<battery_tile>( loc.absolute );
            if( battery != nullptr ) {
                amt = battery->mod_resource( amt );
                if( amt == 0 ) {
                    return 0;
                }
                continue;
            }

            if( !recurse ) {
                continue;
            }

            vehicle_connector_tile *connector = active_tiles::furn_at<vehicle_connector_tile>( loc.absolute );
            if( connector != nullptr ) {
                for( const tripoint &veh_abs : connector->connected_vehicles ) {
                    vehicle *veh = vehicle::find_vehicle( veh_abs );
                    if( veh == nullptr ) {
                        // TODO: Disconnect
                        debugmsg( "lost vehicle at %d,%d,%d", veh_abs.x, veh_abs.y, veh_abs.z );
                        continue;
                    }
                    connected_vehicles.push_back( veh );
                }
            }
        }
    }

    // TODO: Giga ugly. We only charge the first vehicle to get it to use its recursive graph traversal because it's inaccessible from here due to being a template method
    if( !connected_vehicles.empty() ) {
        if( amt > 0 ) {
            amt = connected_vehicles.front()->charge_battery( amt, true );
        } else {
            amt = -connected_vehicles.front()->discharge_battery( -amt, true );
        }
    }

    return amt;
}

int distribution_grid::get_resource() const
{
    int res = 0;
    for( const std::pair<const tripoint, std::vector<tile_location>> &c : contents ) {
        for( const tile_location &loc : c.second ) {
            battery_tile *battery = active_tiles::furn_at<battery_tile>( loc.absolute );
            if( battery != nullptr ) {
                res += battery->stored;
                continue;
            }

            vehicle_connector_tile *connector = active_tiles::furn_at<vehicle_connector_tile>( loc.absolute );
            if( connector != nullptr ) {
                for( const tripoint &veh_abs : connector->connected_vehicles ) {
                    vehicle *veh = vehicle::find_vehicle( veh_abs );
                    if( veh == nullptr ) {
                        // TODO: Disconnect
                        debugmsg( "lost vehicle at %d,%d,%d", veh_abs.x, veh_abs.y, veh_abs.z );
                        continue;
                    }

                    res += veh->fuel_left( "battery", false );
                }
            }
        }
    }

    return res;
}

distribution_grid_tracker::distribution_grid_tracker()
    : distribution_grid_tracker( MAPBUFFER )
{}

distribution_grid_tracker::distribution_grid_tracker( mapbuffer &buffer )
    : mb( buffer )
{
}

void distribution_grid_tracker::make_distribution_grid_at( const tripoint &sm_pos )
{
    const std::set<tripoint> overmap_positions = overmap_buffer.electric_grid_at( sm_to_omt_copy(
                sm_pos ) );
    std::vector<tripoint> submap_positions;
    for( const tripoint &omp : overmap_positions ) {
        submap_positions.emplace_back( omt_to_sm_copy( omp ) + point( 0, 0 ) );
        submap_positions.emplace_back( omt_to_sm_copy( omp ) + point( 1, 0 ) );
        submap_positions.emplace_back( omt_to_sm_copy( omp ) + point( 0, 1 ) );
        submap_positions.emplace_back( omt_to_sm_copy( omp ) + point( 1, 1 ) );
    }
    shared_ptr_fast<distribution_grid> dist_grid = make_shared_fast<distribution_grid>
            ( submap_positions, mb );
    if( dist_grid->empty() ) {
        for( const tripoint &smp : submap_positions ) {
            parent_distribution_grids.erase( smp );
        }
        return;
    }
    for( const tripoint &smp : submap_positions ) {
        parent_distribution_grids[smp] = dist_grid;
    }
}

void distribution_grid_tracker::on_saved()
{
    parent_distribution_grids.clear();
    tripoint min_bounds = tripoint( bounds.p_min, -OVERMAP_DEPTH );
    tripoint max_bounds = tripoint( bounds.p_max, OVERMAP_HEIGHT );
    // TODO: Only those which existed before the save
    for( const tripoint &sm_pos : tripoint_range( min_bounds, max_bounds ) ) {
        if( parent_distribution_grids.find( sm_pos ) == parent_distribution_grids.end() ) {
            make_distribution_grid_at( sm_pos );
        }
    }
}

void distribution_grid_tracker::on_changed( const tripoint &p )
{
    tripoint sm_pos = ms_to_sm_copy( p );
    if( bounds.contains_half_open( sm_pos.xy() ) ) {
        // TODO: Don't rebuild, update
        make_distribution_grid_at( sm_pos );
    }

}

distribution_grid &distribution_grid_tracker::grid_at( const tripoint &p )
{
    // TODO: empty mapbuffer for this case
    static distribution_grid empty_grid( {}, MAPBUFFER );
    tripoint sm_pos = ms_to_sm_copy( p );
    // TODO: This should load a grid, not just expect to find loaded ones!
    auto iter = parent_distribution_grids.find( sm_pos );
    if( iter != parent_distribution_grids.end() ) {
        return *iter->second;
    }

    return empty_grid;
}

const distribution_grid &distribution_grid_tracker::grid_at( const tripoint &p ) const
{
    return const_cast<const distribution_grid &>(
               const_cast<distribution_grid_tracker *>( this )->grid_at( p ) );
}

void distribution_grid_tracker::update( time_point to )
{
    // TODO: Don't recalc this every update
    std::unordered_set<const distribution_grid *> updated;
    for( auto &pr : parent_distribution_grids ) {
        if( updated.count( pr.second.get() ) == 0 ) {
            updated.emplace( pr.second.get() );
            pr.second->update( to );
        }
    }
}

void distribution_grid_tracker::load( rectangle area )
{
    bounds = area;
    // TODO: Don't reload everything when not needed
    on_saved();
}

void distribution_grid_tracker::load( const map &m )
{
    load( rectangle( m.get_abs_sub().xy(),
                     m.get_abs_sub().xy() + point( m.getmapsize(), m.getmapsize() ) ) );
}
