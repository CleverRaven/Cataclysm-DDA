/**
* For creature, pathfinding, and move cost functions
*/

#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_assert.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "game_constants.h"
#include "line.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "mapgen_primitives.h"
#include "maptile_fwd.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "point.h"
#include "rng.h"
#include "submap.h"
#include "trap.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"

static const efftype_id effect_fake_common_cold( "fake_common_cold" );
static const efftype_id effect_fake_flu( "fake_flu" );
static const efftype_id effect_pet( "pet" );

static const species_id species_FERAL( "FERAL" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

void map::kill_creature( const tripoint_bub_ms &p, bool remove_corpse )
{
    Creature *tmp_critter = get_creature_tracker().creature_at( get_abs( p ), true );
    if( tmp_critter && !tmp_critter->is_avatar() ) {
        if( remove_corpse ) {
            if( monster *mon = tmp_critter->as_monster(); mon != nullptr ) {
                mon->death_drops = false;
                mon->no_corpse_quiet = true;

            } else if( npc *npc = tmp_critter->as_npc(); npc != nullptr ) {
                npc->death_drops = false;
                npc->quiet_death = true;
            }
        }
        tmp_critter->die( this, nullptr );
    }
}

int map::move_cost( const tripoint_bub_ms &p, const vehicle *ignored_vehicle,
                    const bool ignore_fields ) const
{
    // To save all of the bound checks and submaps fetching, we extract it
    // here instead of using furn(), field_at() and ter().
    if( !inbounds( p ) ) {
        return 0;
    }
    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        return 0;
    }

    field static nofield;

    const furn_t &furniture = current_submap->get_furn( l ).obj();
    const ter_t &terrain = current_submap->get_ter( l ).obj();
    const field &field = current_submap->get_field( l );
    const optional_vpart_position vp = veh_at( p );
    vehicle *const veh = ( !vp || &vp->vehicle() == ignored_vehicle ) ? nullptr : &vp->vehicle();
    const int part = veh ? vp->part_index() : -1;

    return move_cost_internal( furniture, terrain,
                               ( !ignore_fields ? field : nofield ), veh, part );
}

bool map::impassable( const tripoint_bub_ms &p ) const
{
    return !passable( p );
}

bool map::passable( const tripoint_bub_ms &p ) const
{
    return move_cost( p ) != 0;
}

bool map::passable_through( const tripoint_bub_ms &p ) const
{
    return passable( p ) && ( has_floor( p ) || has_floor_or_support( p ) );
}

bool map::passable_skip_fields( const tripoint_bub_ms &p ) const
{
    return move_cost( p, static_cast<const vehicle *>( nullptr ), true ) != 0;
}

int map::move_cost_ter_furn( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Tried process terrain at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        return 0;
    }

    const int tercost = current_submap->get_ter( l ).obj().movecost;
    if( tercost == 0 ) {
        return 0;
    }

    const int furncost = current_submap->get_furn( l ).obj().movecost;
    if( furncost < 0 ) {
        return 0;
    }

    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

bool map::impassable_ter_furn( const tripoint_bub_ms &p ) const
{
    return !passable_ter_furn( p );
}

bool map::passable_ter_furn( const tripoint_bub_ms &p ) const
{
    return move_cost_ter_furn( p ) != 0;
}

int map::combined_movecost( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                            const vehicle *ignored_vehicle,
                            const int modifier, const bool flying, const bool via_ramp, const bool ignore_fields ) const
{
    static constexpr std::array<int, 4> mults = { 0, 50, 71, 100 };
    const int cost1 = move_cost( from, ignored_vehicle, ignore_fields );
    const int cost2 = move_cost( to, ignored_vehicle, ignore_fields );
    // Multiply cost depending on the number of differing axes
    // 0 if all axes are equal, 100% if only 1 differs, 141% for 2, 200% for 3
    size_t match = trigdist ? ( from.x() != to.x() ) + ( from.y() != to.y() ) +
                   ( from.z() != to.z() ) : 1;
    if( flying || from.z() == to.z() ) {
        return ( cost1 + cost2 + modifier ) * mults[match] / 2;
    }

    // Inter-z-level movement by foot (not flying)
    if( !valid_move( from, to, false, via_ramp ) ) {
        return 0;
    }

    // TODO: Penalize for using stairs
    return ( cost1 + cost2 + modifier ) * mults[match] / 2;
}

bool map::valid_move( const tripoint_bub_ms &from, const tripoint_bub_ms &to,
                      const bool bash, const bool flying, const bool via_ramp ) const
{
    // Used to account for the fact that older versions of GCC can trip on the if statement here.
    cata_assert( to.z() > std::numeric_limits<int>::min() );
    // Note: no need to check inbounds here, because maptile_at will do that
    // If oob tile is supplied, the maptile_at will be an unpassable "null" tile
    if( std::abs( from.x() - to.x() ) > 1 || std::abs( from.y() - to.y() ) > 1 ||
        std::abs( from.z() - to.z() ) > 1 ) {
        return false;
    }

    if( from.z() == to.z() ) {
        // But here we need to, to prevent bashing critters
        return passable( to ) || ( bash && inbounds( to ) );
    } else if( !zlevels ) {
        return false;
    }

    const bool going_up = from.z() < to.z();

    const tripoint_bub_ms &up_p = going_up ? to : from;
    const tripoint_bub_ms &down_p = going_up ? from : to;

    const const_maptile up = maptile_at( up_p );
    const ter_t &up_ter = up.get_ter_t();
    if( up_ter.id.is_null() ) {
        return false;
    }
    // Checking for ledge is a workaround for the case when mapgen doesn't
    // actually make a valid ledge drop location with zlevels on, this forces
    // at least one zlevel drop and if down_ter is impassable it's probably
    // inside a wall, we could workaround that further but it's unnecessary.
    const bool up_is_ledge = is_open_air( up_p );

    if( up_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    const const_maptile down = maptile_at( down_p );
    const ter_t &down_ter = down.get_ter_t();
    if( down_ter.id.is_null() ) {
        return false;
    }

    if( !up_is_ledge && down_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    if( has_floor( up_p ) && !up_ter.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) &&
        !up_is_ledge && !via_ramp ) {
        // Can't move from up to down
        if( std::abs( from.x() - to.x() ) == 1 || std::abs( from.y() - to.y() ) == 1 ) {
            // Break the move into two - vertical then horizontal
            tripoint_bub_ms midpoint( down_p.xy(), up_p.z() );
            return valid_move( down_p, midpoint, bash, flying, via_ramp ) &&
                   valid_move( midpoint, up_p, bash, flying, via_ramp );
        }
        return false;
    }

    if( !flying && !down_ter.has_flag( ter_furn_flag::TFLAG_GOES_UP ) &&
        !down_ter.has_flag( ter_furn_flag::TFLAG_SWIMMABLE ) &&
        !down_ter.has_flag( ter_furn_flag::TFLAG_RAMP ) &&
        !up_is_ledge && !via_ramp ) {
        // Can't safely reach the lower tile
        return false;
    }

    if( bash ) {
        return true;
    }

    int part_up;
    const vehicle *veh_up = veh_at_internal( up_p, part_up );
    if( veh_up != nullptr ) {
        // TODO: Hatches below the vehicle, passable frames
        return false;
    }

    int part_down;
    const vehicle *veh_down = veh_at_internal( down_p, part_down );
    if( veh_down != nullptr && veh_down->roof_at_part( part_down ) >= 0 ) {
        // TODO: OPEN (and only open) hatches from above
        return false;
    }

    // Currently only furniture can block movement if everything else is OK
    // TODO: Vehicles with boards in the given spot
    return up.get_furn_t().movecost >= 0;
}

// End of move cost

// This method tries a bunch of initial offsets for the line to try and find a clear one.
// Basically it does, "Find a line from any point in the source that ends up in the target square".
std::vector<tripoint_bub_ms> map::find_clear_path( const tripoint_bub_ms &source,
        const tripoint_bub_ms &destination, bool empty_on_fail ) const
{
    // TODO: Push this junk down into the Bresenham method, it's already doing it.
    const point d( destination.xy().raw() - source.xy().raw() );
    const point a( std::abs( d.x ) * 2, std::abs( d.y ) * 2 );
    const int dominant = std::max( a.x, a.y );
    const int minor = std::min( a.x, a.y );
    // This seems to be the method for finding the ideal start value for the error value.
    const int ideal_start_offset = minor - dominant / 2;
    const int start_sign = ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 );
    // Not totally sure of the derivation.
    const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
    for( int horizontal_offset = -1; horizontal_offset <= max_start_offset; ++horizontal_offset ) {
        int candidate_offset = horizontal_offset * ( start_sign == 0 ? 1 : start_sign );
        if( sees( source, destination, rl_dist( source, destination ),
                  candidate_offset, /*with_fields=*/true, /*allow_cached=*/false, los_trace::physical ) ) {
            return line_to( source, destination, candidate_offset, 0 );
        }
    }
    // If we couldn't find a clear LoS, depending on empty_on_fail
    // return either the ideal one or an empty vector.
    if( empty_on_fail ) {
        return std::vector<tripoint_bub_ms>();
    } else {
        return line_to( source, destination, ideal_start_offset, 0 );
    }
}

std::vector<tripoint_bub_ms> map::reachable_flood_steps( const tripoint_bub_ms &f, int range,
        const int cost_min, const int cost_max ) const
{
    struct pq_item {
        int dist;
        int ndx;
    };
    struct pq_item_comp {
        bool operator()( const pq_item &left, const pq_item &right ) {
            return left.dist > right.dist;
        }
    };
    using PQ_type = std::priority_queue< pq_item, std::vector<pq_item>, pq_item_comp>;

    // temp buffer for grid
    const int grid_dim = range * 2 + 1;
    const size_t grid_area = static_cast<size_t>( grid_dim ) * grid_dim;
    // init to -1 as "not visited yet"
    std::vector<int> t_grid( grid_area, -1 );
    const tripoint_rel_ms origin_offset = { range, range, 0 };
    const int initial_visit_distance = range * range; // Large unreachable value

    // Fill positions that are visitable with initial_visit_distance
    for( const tripoint_bub_ms &p : points_in_radius( f, range ) ) {
        const tripoint_bub_ms tp = { p.xy(), f.z()};
        const int tp_cost = move_cost( tp );
        // rejection conditions
        if( tp_cost < cost_min || tp_cost > cost_max || is_open_air( tp ) ) {
            continue;
        }
        // set initial cost for grid point
        tripoint_rel_ms origin_relative = tp - f;
        origin_relative += origin_offset;
        int ndx = origin_relative.x() + origin_relative.y() * grid_dim;
        t_grid[ndx] = initial_visit_distance;
    }

    auto gen_neighbors = []( const pq_item & elem, int grid_dim, std::array<pq_item, 8> &neighbors ) {
        // Up to 8 neighbors
        int new_cost = elem.dist + 1;
        // *INDENT-OFF*
        const std::array<int, 8> ox = {
            -1, 0, 1,
            -1,    1,
            -1, 0, 1
        };
        const std::array<int, 8> oy = {
            -1, -1, -1,
            0,      0,
            1,  1,  1
        };
        // *INDENT-ON*

        point e( elem.ndx % grid_dim, elem.ndx / grid_dim );
        for( int i = 0; i < 8; ++i ) {
            point n( e + point( ox[i], oy[i] ) );

            int ndx = n.x + n.y * grid_dim;
            neighbors[i] = { new_cost, ndx };
        }
    };

    PQ_type pq( pq_item_comp{} );
    pq_item first_item{ 0, range + range * grid_dim };
    pq.push( first_item );
    std::array<pq_item, 8> neighbor_elems;

    while( !pq.empty() ) {
        const pq_item item = pq.top();
        pq.pop();

        if( t_grid[item.ndx] == initial_visit_distance ) {
            t_grid[item.ndx] = item.dist;
            if( item.dist + 1 < range ) {
                gen_neighbors( item, grid_dim, neighbor_elems );
                for( pq_item neighbor_elem : neighbor_elems ) {
                    pq.push( neighbor_elem );
                }
            }
        }
    }
    std::vector<bool> o_grid( grid_area );
    int count = 0;
    for( int y = 0, ndx = 0; y < grid_dim; ++y ) {
        for( int x = 0; x < grid_dim; ++x, ++ndx ) {
            if( t_grid[ndx] != -1 && t_grid[ndx] < initial_visit_distance ) {
                // set self and neighbors to 1
                for( int y2 = y - 1; y2 <= y + 1; ++y2 ) {
                    for( int x2 = x - 1; x2 <= x + 1; ++x2 ) {
                        if( x2 >= 0 && x2 < grid_dim && y2 >= 0 && y2 < grid_dim ) {
                            const int index = x2 + ( y2 * grid_dim );
                            if( !o_grid[ index ] ) {
                                count++;
                                o_grid[ index ] = true;
                            }
                        }
                    }
                }
            }
        }
    }


    std::vector<tripoint_bub_ms> reachable_pts;
    if( count != 0 ) {
        reachable_pts.reserve( count );
        // Now go over again to pull out all of the reachable points
        for( int y = 0, ndx = 0; y < grid_dim; ++y ) {
            for( int x = 0; x < grid_dim; ++x, ++ndx ) {
                if( o_grid[ndx] ) {
                    tripoint_bub_ms t = f - origin_offset + tripoint{ x, y, 0 };
                    reachable_pts.push_back( t );
                }
            }
        }
    }
    return reachable_pts;
}

bool map::clear_path( const tripoint_bub_ms &f, const tripoint_bub_ms &t, const int range,
                      const int cost_min, const int cost_max ) const
{
    if( std::abs( f.z() - t.z() ) > fov_3d_z_range ) {
        return false;
    }

    // Ugly `if` for now
    if( f.z() == t.z() ) {
        if( ( range >= 0 && range < rl_dist( f.xy(), t.xy() ) ) ||
            !inbounds( t ) ) {
            return false; // Out of range!
        }
        bool is_clear = true;
        bresenham( f.xy(), t.xy(), 0,
        [this, &is_clear, cost_min, cost_max, &t]( const point_bub_ms & new_point ) {
            // Exit before checking the last square, it's still reachable even if it is an obstacle.
            if( new_point.x() == t.x() && new_point.y() == t.y() ) {
                return false;
            }

            const int cost = this->move_cost( new_point );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
            return true;
        } );
        return is_clear;
    }

    if( ( range >= 0 && range < rl_dist( f, t ) ) ||
        !inbounds( t ) ) {
        return false; // Out of range!
    }
    bool is_clear = true;
    tripoint_bub_ms last_point = f;
    bresenham( f, t, 0, 0,
    [this, &is_clear, cost_min, cost_max, t, &last_point]( const tripoint_bub_ms & new_point ) {
        // Exit before checking the last square, it's still reachable even if it is an obstacle.
        if( new_point == t ) {
            return false;
        }

        // We have to check a weird case where the move is both vertical and horizontal
        if( new_point.z() == last_point.z() ) {
            const int cost = move_cost( new_point );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
        } else {
            bool this_clear = false;
            const int max_z = std::max( new_point.z(), last_point.z() );
            if( !has_floor_or_support( { point_bub_ms( new_point.xy() ), max_z } ) ) {
                const int cost = move_cost( tripoint_bub_ms{ new_point.xy(), last_point.z()} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear && has_floor_or_support( { point_bub_ms( last_point.xy() ), max_z } ) ) {
                const int cost = move_cost( tripoint_bub_ms{ last_point.xy(), new_point.z()} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear ) {
                is_clear = false;
                return false;
            }
        }

        last_point = new_point;
        return true;
    } );
    return is_clear;
}

void map::spawn_monsters_submap_group( const tripoint_rel_sm &gp, mongroup &group,
                                       bool ignore_sight )
{
    Character &player_character = get_player_character();
    const int s_range = std::min( HALF_MAPSIZE_X,
                                  player_character.sight_range( g->light_level( player_character.posz() ) ) );
    int pop = group.population;
    std::vector<tripoint_bub_ms> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        if( gp.x() == 0 || gp.y() == 0 || gp.x() + 1 == MAPSIZE || gp.y() + 1 == MAPSIZE ) {
            ignore_sight = true;
        }
    }

    if( gp.z() != player_character.posz() ) {
        // Note: this is only OK because 3D vision isn't a thing yet. 3D vision is a thing! Is this still OK?
        ignore_sight = true;
    }

    const auto allow_on_terrain = [&]( const tripoint_bub_ms & p ) {
        // TODO: flying creatures should be allowed to spawn without a floor,
        // but the new creature is created *after* determining the terrain, so
        // we can't check for it here.
        return passable( p ) && has_floor_or_water( p );
    };

    // If the submap is uniform, we can skip many checks
    const submap *current_submap = get_submap_at_grid( gp );
    if( current_submap == nullptr ) {
        debugmsg( "Tried spawn monster group at (%d,%d,%d) but the submap is not loaded", gp.x(), gp.y(),
                  gp.z() );
        return;
    }
    bool ignore_terrain_checks = false;
    bool ignore_inside_checks = gp.z() < 0;
    if( current_submap->is_uniform() ) {
        const tripoint_bub_ms upper_left{ SEEX * gp.x(), SEEY * gp.y(), gp.z()};
        if( !allow_on_terrain( upper_left ) ||
            ( !ignore_inside_checks && has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, upper_left ) ) ) {
            const tripoint_abs_ms glp = get_abs( tripoint_bub_ms( gp.x() * SEEX, gp.y() * SEEY, gp.z() ) );
            dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                             " at uniform submap " << gp.x() << "," << gp.y() << "," << gp.z() <<
                             " global " << glp.x() << "," << glp.y() << "," << glp.z();
            return;
        }

        ignore_terrain_checks = true;
        ignore_inside_checks = true;
    }

    creature_tracker &creatures = get_creature_tracker();
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            point_bub_ms f( x + SEEX * gp.x(), y + SEEY * gp.y() );
            tripoint_bub_ms fp{ f, gp.z()};
            if( creatures.creature_at( fp ) != nullptr ) {
                continue; // there is already some creature
            }

            if( !ignore_terrain_checks && !allow_on_terrain( fp ) ) {
                continue; // solid area, impassable
            }

            if( !ignore_sight && sees( player_character.pos_bub(), fp, s_range ) ) {
                continue; // monster must spawn outside the viewing range of the player
            }

            if( !ignore_inside_checks && has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, fp ) ) {
                continue; // monster must spawn outside.
            }

            locations.push_back( fp );
        }
    }

    if( locations.empty() ) {
        // TODO: what now? there is no possible place to spawn monsters, most
        // likely because the player can see all the places.
        const tripoint_abs_ms glp = get_abs( tripoint_bub_ms( gp.x() * SEEX, gp.y() * SEEY, gp.z() ) );
        dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                         " at " << gp.x() << "," << gp.y() << "," << gp.z() <<
                         " global " << glp.x() << "," << glp.y() << "," << glp.z();
        // Just kill the group. It's not like we're removing existing monsters
        // Unless it's a horde - then don't kill it and let it spawn behind a tree or smoke cloud
        if( !group.horde ) {
            group.clear();
        }

        return;
    }

    if( pop ) {
        // Populate the group from its population variable.
        for( int m = 0; m < pop; m++ ) {
            std::vector<MonsterGroupResult> spawn_details =
                MonsterGroupManager::GetResultFromGroup( group.type, &pop );
            for( const MonsterGroupResult &mgr : spawn_details ) {
                if( !mgr.id ) {
                    continue;
                }
                monster tmp( mgr.id );

                // If a monster came from a horde population, configure them to always be willing to rejoin a horde.
                if( group.horde ) {
                    tmp.set_horde_attraction( MHA_ALWAYS );
                }
                for( int i = 0; i < mgr.pack_size; i++ ) {
                    group.monsters.push_back( tmp );
                }
            }
        }
    }

    // Find horde's target submap
    for( monster &tmp : group.monsters ) {
        for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
            const tripoint_bub_ms local_pos = random_entry_removed( locations );
            const tripoint_abs_ms abs_pos = get_abs( local_pos );
            if( !tmp.can_move_to( local_pos ) ) {
                continue; // target can not contain the monster
            }
            if( group.horde ) {
                // Give monster a random point near horde's expected destination
                const tripoint_sm_ms pos_in_sm( rng( 0, SEEX ), rng( 0, SEEY ), local_pos.z() );
                const tripoint_abs_ms rand_dest = project_combine( group.target, pos_in_sm );

                // The monster will wander to the rand_dest for several turns which is determined by the distance and interest
                const int turns = rl_dist( abs_pos, rand_dest ) + group.interest;
                tmp.wander_to( rand_dest, turns );
                add_msg_debug( debugmode::DF_MAP, "%s targeting %s", tmp.disp_name(),
                               tmp.wander_pos.to_string_writable() );
            }

            monster *const placed = g->place_critter_at( make_shared_fast<monster>( tmp ),
                                    local_pos );
            if( placed ) {
                placed->on_load();
            }
            break;
        }
    }
    // indicates the group is empty, and can be removed later
    group.clear();
}

void map::spawn_monsters_submap( const tripoint_rel_sm &gp, bool ignore_sight, bool spawn_nonlocal )
{
    const tripoint_abs_sm submap_pos( gp + abs_sub.xy() );
    // Load unloaded monsters
    overmap_buffer.spawn_monster( submap_pos, spawn_nonlocal );
    // Only spawn new monsters after existing monsters are loaded.
    std::vector<mongroup *> groups = overmap_buffer.groups_at( submap_pos );
    for( mongroup *&mgp : groups ) {
        spawn_monsters_submap_group( gp, *mgp, ignore_sight );
    }

    submap *const current_submap = get_submap_at_grid( gp );
    if( current_submap == nullptr ) {
        debugmsg( "Tried spawn monsters at (%d,%d,%d) but the submap is not loaded", gp.x(), gp.y(),
                  gp.z() );
        return;
    }
    const tripoint_bub_ms gp_ms = rebase_bub( coords::project_to<coords::ms>( gp ) );

    // The list of spawns on the submap might be updated while we are iterating it.
    // For example, `monster::on_load` -> `monster::try_reproduce` calls `map::add_spawn`.
    // Therefore, this intentionally uses old-school indexed for-loop with re-check against `.size()` each step.
    // NOLINTNEXTLINE(modernize-loop-convert)
    for( size_t sp_i = 0; sp_i < current_submap->spawns.size(); ++sp_i ) {
        const spawn_point i = current_submap->spawns[sp_i]; // intentional copy
        const tripoint_bub_ms center = gp_ms + rebase_rel( i.pos );
        const tripoint_range<tripoint_bub_ms> points = points_in_radius( center, 3 );

        for( int j = 0; j < i.count; j++ ) {
            monster tmp( i.type );

            if( tmp.type->in_species( species_FERAL ) ) {
                if( one_in( 100 ) ) {
                    // Same chances and duration of flu vs. cold as for the player.
                    if( one_in( 6 ) ) {
                        tmp.add_effect( effect_fake_flu, rng( 3_days, 10_days ) );
                    } else {
                        tmp.add_effect( effect_fake_common_cold, rng( 1_days, 14_days ) );
                    }
                }
            }

            if( i.mission_id > 0 ) {
                mission *found_mission = mission::find( i.mission_id );
                if( found_mission != nullptr ) {
                    tmp.mission_ids = { i.mission_id };
                    if( found_mission->get_type().goal == MGOAL_KILL_MONSTERS ) {
                        found_mission->register_kill_needed();
                    }
                }
            }
            if( i.name.has_value() ) {
                tmp.unique_name = i.name.value();
            }
            if( i.friendly ) {
                tmp.friendly = -1;
                tmp.add_effect( effect_pet, 1_turns, true );
            }
            if( !i.data.ammo.empty() ) {
                for( std::pair<itype_id, jmapgen_int> ap : i.data.ammo ) {
                    tmp.ammo.emplace( ap.first, ap.second.get() );
                }
            } else {
                tmp.ammo = tmp.type->starting_ammo;
            }

            // This can fail, but there isn't much we can do about it if it does. We could output some
            // kind of info message, but, again, it's unlikely to result in anything that can be acted on.
            g->place_critter_at_or_within( make_shared_fast<monster>( tmp ), this, center, points );
        }
    }
    current_submap->spawns.clear();
}

void map::spawn_monsters( bool ignore_sight, bool spawn_nonlocal )
{
    const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    tripoint_rel_sm gp;
    int &gx = gp.x();
    int &gy = gp.y();
    int &gz = gp.z();
    for( gz = zmin; gz <= zmax; gz++ ) {
        for( gx = 0; gx < my_MAPSIZE; gx++ ) {
            for( gy = 0; gy < my_MAPSIZE; gy++ ) {
                spawn_monsters_submap( gp, ignore_sight, spawn_nonlocal );
            }
        }
    }
}

void map::clear_spawns()
{
    for( submap *&smap : grid ) {
        smap->spawns.clear();
    }
}

std::list<Creature *> map::get_creatures_in_radius( const tripoint_bub_ms &center, size_t radius,
        size_t radiusz ) const
{
    creature_tracker &creatures = get_creature_tracker();
    std::list<Creature *> creature_list;
    for( const tripoint_bub_ms &loc : points_in_radius( center, radius, radiusz ) ) {
        Creature *tmp_critter = creatures.creature_at( loc );
        if( tmp_critter != nullptr ) {
            creature_list.push_back( tmp_critter );
        }

    }
    return creature_list;
}

std::list<Creature *> map::get_creatures_in_radius_circ( const tripoint_bub_ms &center,
        size_t radius, size_t radiusz ) const
{
    creature_tracker &creatures = get_creature_tracker();
    std::list<Creature *> creature_list;
    for( const tripoint_bub_ms &loc : points_in_radius_circ( center, radius, radiusz ) ) {
        Creature *tmp_critter = creatures.creature_at( loc );
        if( tmp_critter != nullptr ) {
            creature_list.push_back( tmp_critter );
        }

    }
    return creature_list;
}

pathfinding_cache::pathfinding_cache()
{
    dirty = true;
}

pathfinding_cache &map::get_pathfinding_cache( int zlev ) const
{
    return *pathfinding_caches[zlev + OVERMAP_DEPTH];
}

void map::set_pathfinding_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_pathfinding_cache( zlev ).dirty = true;
    }
}

void map::set_pathfinding_cache_dirty( const tripoint_bub_ms &p )
{
    if( inbounds( p ) ) {
        get_pathfinding_cache( p.z() ).dirty_points.insert( p.xy() );
    }
}

const pathfinding_cache &map::get_pathfinding_cache_ref( int zlev ) const
{
    if( !inbounds_z( zlev ) ) {
        debugmsg( "Tried to get pathfinding cache for out of bounds z-level %d", zlev );
        return *pathfinding_caches[ OVERMAP_DEPTH ];
    }
    pathfinding_cache &cache = get_pathfinding_cache( zlev );
    if( cache.dirty || !cache.dirty_points.empty() ) {
        update_pathfinding_cache( zlev );
    }

    return cache;
}

void map::update_pathfinding_cache( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return;
    }
    pathfinding_cache &cache = get_pathfinding_cache( p.z() );
    PathfindingFlags cur_value = PathfindingFlag::Ground;

    const_maptile tile = maptile_at_internal( p );

    const ter_t &terrain = tile.get_ter_t();
    const furn_t &furniture = tile.get_furn_t();
    const field &field = tile.get_field();
    int part;
    const vehicle *veh = veh_at_internal( p, part );

    const int cost = move_cost_internal( furniture, terrain, field, veh, part );

    if( cost > 2 ) {
        cur_value |= PathfindingFlag::Slow;
    } else if( cost <= 0 ) {
        cur_value |= PathfindingFlag::Obstacle;
        if( terrain.has_flag( ter_furn_flag::TFLAG_CLIMBABLE ) ) {
            cur_value |= PathfindingFlag::Climbable;
        }
    }

    if( veh != nullptr ) {
        cur_value |= PathfindingFlag::Vehicle;
    }

    for( const auto &fld : tile.get_field() ) {
        const field_entry &cur = fld.second;
        if( cur.is_dangerous() ) {
            cur_value |= PathfindingFlag::DangerousField;
        }
    }

    if( ( !tile.get_trap_t().is_benign() || !terrain.trap.obj().is_benign() ) &&
        !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::DangerousTrap;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) && !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::Air;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_GOES_UP ) ) {
        cur_value |= PathfindingFlag::GoesUp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ) {
        cur_value |= PathfindingFlag::GoesDown;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_RAMP ) ||
        terrain.has_flag( ter_furn_flag::TFLAG_RAMP_UP ) ) {
        cur_value |= PathfindingFlag::GoesUp | PathfindingFlag::RampUp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN ) ) {
        cur_value |= PathfindingFlag::GoesDown | PathfindingFlag::RampDown;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_SHARP ) && !this->has_vehicle_floor( p ) ) {
        cur_value |= PathfindingFlag::Sharp;
    }

    if( terrain.has_flag( ter_furn_flag::TFLAG_SMALL_PASSAGE ) ) {
        cur_value |= ( PathfindingFlag::RestrictLarge | PathfindingFlag::RestrictHuge );
    }

    cache.special[p.x()][p.y()] = cur_value;
}

void map::update_pathfinding_cache( int zlev ) const
{
    pathfinding_cache &cache = get_pathfinding_cache( zlev );

    if( cache.dirty ) {
        const int size = getmapsize();
        for( int x = 0; x < size * SEEX; ++x ) {
            for( int y = 0; y < size * SEEX; ++y ) {
                update_pathfinding_cache( { x, y, zlev } );
            }
        }
        cache.dirty = false;
    } else {
        for( const point_bub_ms &p : cache.dirty_points ) {
            update_pathfinding_cache( { p, zlev } );
        }
    }
    cache.dirty_points.clear();
}
