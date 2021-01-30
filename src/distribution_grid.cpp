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
    return !empty();
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

int distribution_grid::mod_resource( int amt )
{
    for( const std::pair<const tripoint, std::vector<tile_location>> &c : contents ) {
        if( amt == 0 ) {
            return 0;
        }
        submap *sm = mb.lookup_submap( c.first );
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
    for( const std::pair<const tripoint, std::vector<tile_location>> &c : contents ) {
        submap *sm = mb.lookup_submap( c.first );
        if( sm == nullptr ) {
            continue;
        }

        for( const tile_location &loc : c.second ) {
            res += sm->active_furniture[loc.on_submap]->get_resource();
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

#include "game.h"
#include "vehicle.h"
#include "avatar.h"

template <typename VehFunc, typename GridFunc, typename StartPoint>
int traverse_graph( StartPoint *start, int amount,
                    VehFunc veh_action, GridFunc grid_action )
{

    struct vehicle_or_grid {
        enum class type_t {
            vehicle,
            grid
        } type;

        vehicle *const veh;
        distribution_grid *const grid;

        vehicle_or_grid( vehicle *veh )
            : type( type_t::vehicle )
            , veh( veh )
        {}

        vehicle_or_grid( distribution_grid *grid )
            : type( type_t::grid )
            , grid( grid )
        {}

        bool operator==( const vehicle *veh ) const {
            return this->veh = veh;
        }

        bool operator==( const distribution_grid *grid ) const {
            return this->grid = grid;
        }
    };

    std::queue<vehicle_or_grid> connected_elements;
    std::set<vehicle_or_grid> visited_elements;
    connected_elements.emplace( start );

    auto process_vehicle = [&]( vehicle * veh ) {
        // Add this connected vehicle to the queue of vehicles to search next,
        // but only if we haven't seen this one before.
        if( visited_elements.count( veh ) < 1 ) {
            connected_elements.emplace( veh );

            amount = veh_action( veh, amount );
            g->u.add_msg_if_player( m_debug, "After remote veh %p, %d power", static_cast<void *>( veh ),
                                    amount );

            return amount < 1;
        }
    };

    auto process_grid = [&]( distribution_grid * grid ) {
        if( visited_elements.count( grid ) < 1 ) {
            connected_elements.emplace( grid );

            amount = grid_action( grid, amount );
            g->u.add_msg_if_player( m_debug, "After remote grid %p, %d power", static_cast<void *>( grid ),
                                    amount );

            return amount < 1;
        }
    };

    auto grid_tracker = get_distribution_grid_tracker();
    while( amount > 0 && !connected_elements.empty() ) {
        vehicle_or_grid current = connected_elements.front();

        visited_elements.insert( current );
        connected_elements.pop();

        if( current.type == vehicle_or_grid::type_t::vehicle ) {
            auto *current_veh  = current.veh;
            for( auto &p : current_veh->loose_parts ) {
                if( !current_veh->part_info( p ).has_flag( "POWER_TRANSFER" ) ) {
                    continue; // ignore loose parts that aren't power transfer cables
                }

                const tripoint target_pos = current_veh->parts[p].target.second;
                if( current_veh->parts[p].has_flag( vehicle_part::targets_grid ) ) {
                    auto target_grid = grid_tracker.grid_at( target_pos );
                    if( target_grid && process_grid( &target_grid ) ) {
                        break;
                    }
                } else {

                    auto target_veh = vehicle::find_vehicle( target_pos );
                    // Skip visited or unloadable vehicles
                    if( target_veh != nullptr && visited_elements.count( target_veh ) == 0 &&
                        process_vehicle( target_veh ) ) {
                        // No more charge to donate away.
                        break;
                    }
                }
            }
        } else {
            // Grids can only be connected to vehicles at the moment
            auto *current_grid = current.grid;
            for( auto &pr : current_grid->contents ) {
                const vehicle_connector_tile *connector = active_tiles::furn_at<vehicle_connector_tile>
                        ( pr.second.absolute );
                if( connector == nullptr ) {
                    continue;
                }

                for( const tripoint &target_pos : connector->connected_vehicles ) {
                    auto target_veh = vehicle::find_vehicle( target_pos );
                    // Skip visited or unloadable vehicles
                    if( target_veh != nullptr && visited_elements.count( target_veh ) == 0 &&
                        process_vehicle( target_veh ) ) {
                        // No more charge to donate away.
                        break;
                    }
                }
            }
        }
    }
    return amount;
}
