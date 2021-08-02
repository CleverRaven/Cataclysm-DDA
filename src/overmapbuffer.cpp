#include "overmapbuffer.h"

#include <algorithm>
#include <climits>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <tuple>

#include "basecamp.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "common_types.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "line.h"
#include "map.h"
#include "memory_fast.h"
#include "mongroup.h"
#include "monster.h"
#include "npc.h"
#include "optional.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "overmap_types.h"
#include "path_info.h"
#include "point.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"

class map_extra;

overmapbuffer overmap_buffer;

overmapbuffer::overmapbuffer()
    : last_requested_overmap( nullptr )
{
}

const city_reference city_reference::invalid{ nullptr, tripoint_abs_sm(), -1 };

int city_reference::get_distance_from_bounds() const
{
    cata_assert( city != nullptr );
    return distance - omt_to_sm_copy( city->size );
}

int camp_reference::get_distance_from_bounds() const
{
    cata_assert( camp != nullptr );
    return distance - omt_to_sm_copy( 4 );
}

std::string overmapbuffer::terrain_filename( const point_abs_om &p )
{
    return string_format( "%s/o.%d.%d", PATH_INFO::world_base_save_path(), p.x(), p.y() );
}

std::string overmapbuffer::player_filename( const point_abs_om &p )
{
    return string_format( "%s.seen.%d.%d", PATH_INFO::player_base_save_path(), p.x(), p.y() );
}

overmap &overmapbuffer::get( const point_abs_om &p )
{
    if( last_requested_overmap != nullptr && last_requested_overmap->pos() == p ) {
        return *last_requested_overmap;
    }

    const auto it = overmaps.find( p );
    if( it != overmaps.end() ) {
        return *( last_requested_overmap = it->second.get() );
    }

    // That constructor loads an existing overmap or creates a new one.
    overmap &new_om = *( overmaps[ p ] = std::make_unique<overmap>( p ) );
    new_om.populate();
    // Note: fix_mongroups might load other overmaps, so overmaps.back() is not
    // necessarily the overmap at (x,y)
    fix_mongroups( new_om );
    fix_npcs( new_om );

    last_requested_overmap = &new_om;
    return new_om;
}

void overmapbuffer::create_custom_overmap( const point_abs_om &p, overmap_special_batch &specials )
{
    if( last_requested_overmap != nullptr ) {
        auto om_iter = overmaps.find( p );
        if( om_iter != overmaps.end() && om_iter->second.get() == last_requested_overmap ) {
            last_requested_overmap = nullptr;
        }
    }
    overmap &new_om = *( overmaps[ p ] = std::make_unique<overmap>( p ) );
    new_om.populate( specials );
}

void overmapbuffer::fix_mongroups( overmap &new_overmap )
{
    for( auto it = new_overmap.zg.begin(); it != new_overmap.zg.end(); ) {
        auto &mg = it->second;
        // spawn related code simply sets population to 0 when they have been
        // transformed into spawn points on a submap, the group can then be removed
        if( mg.empty() ) {
            new_overmap.zg.erase( it++ );
            continue;
        }
        // Inside the bounds of the overmap?
        if( mg.pos.x() >= 0 && mg.pos.y() >= 0 && mg.pos.x() < OMAPX * 2 &&
            mg.pos.y() < OMAPY * 2 ) {
            ++it;
            continue;
        }
        point_abs_sm smabs = project_combine( new_overmap.pos(), mg.pos.xy() );
        point_abs_om omp;
        point_om_sm sm_rem;
        std::tie( omp, sm_rem ) = project_remain<coords::om>( smabs );
        if( !has( omp ) ) {
            // Don't generate new overmaps, as this can be called from the
            // overmap-generating code.
            ++it;
            continue;
        }
        overmap &om = get( omp );
        mg.pos = tripoint_om_sm( sm_rem, mg.pos.z() );
        om.spawn_mon_group( mg );
        new_overmap.zg.erase( it++ );
    }
}

void overmapbuffer::fix_npcs( overmap &new_overmap )
{
    // First step: move all npcs that are located outside of the given overmap
    // into a separate container. After that loop, new_overmap.npcs is no
    // accessed anymore!
    decltype( overmap::npcs ) to_relocate;
    for( auto it = new_overmap.npcs.begin(); it != new_overmap.npcs.end(); ) {
        npc &np = **it;
        const tripoint_abs_omt npc_omt_pos = np.global_omt_location();
        const point_abs_om npc_om_pos = project_to<coords::om>( npc_omt_pos.xy() );
        const point_abs_om loc = new_overmap.pos();
        if( npc_om_pos == loc ) {
            // Nothing to do
            ++it;
            continue;
        }
        to_relocate.push_back( *it );
        it = new_overmap.npcs.erase( it );
    }
    // Second step: put them back where they belong. This step involves loading
    // new overmaps (via `get`), which does in turn call this function for the
    // newly loaded overmaps. This in turn may move NPCs from the second overmap
    // back into the first overmap. This messes up the iteration of it. The
    // iteration is therefore done in a separate step above (which does *not*
    // involve loading new overmaps).
    for( auto &ptr : to_relocate ) {
        npc &np = *ptr;
        const tripoint_abs_omt npc_omt_pos = np.global_omt_location();
        const point_abs_om npc_om_pos = project_to<coords::om>( npc_omt_pos.xy() );
        const point_abs_om loc = new_overmap.pos();
        if( !has( npc_om_pos ) ) {
            // This can't really happen without save editing
            // We have no sane option here, just place the NPC on the edge
            debugmsg( "NPC %s is out of bounds, on non-generated overmap %s",
                      np.name, loc.to_string() );
            point_abs_sm npc_sm = project_to<coords::sm>( npc_om_pos );
            point_abs_sm min = project_to<coords::sm>( loc );
            point_abs_sm max =
                project_to<coords::sm>( loc + point_south_east ) - point_south_east;
            npc_sm.x() = clamp( npc_sm.x(), min.x(), max.x() );
            npc_sm.y() = clamp( npc_sm.y(), min.y(), max.y() );
            // TODO: fix point types
            np.spawn_at_sm( tripoint_abs_sm( npc_sm, np.posz() ).raw() );
            new_overmap.npcs.push_back( ptr );
            continue;
        }

        // Simplest case: just move the pointer
        get( npc_om_pos ).insert_npc( ptr );
    }
}

void overmapbuffer::save()
{
    for( auto &omp : overmaps ) {
        // Note: this may throw io errors from std::ofstream
        omp.second->save();
    }
}

void overmapbuffer::clear()
{
    overmaps.clear();
    known_non_existing.clear();
    last_requested_overmap = nullptr;
}

const regional_settings &overmapbuffer::get_settings( const tripoint_abs_omt &p )
{
    overmap *om = get_om_global( p ).om;
    return om->get_settings();
}

void overmapbuffer::add_note( const tripoint_abs_omt &p, const std::string &message )
{
    overmap_with_local_coords om_loc = get_om_global( p );
    om_loc.om->add_note( om_loc.local, message );
}

void overmapbuffer::delete_note( const tripoint_abs_omt &p )
{
    if( has_note( p ) ) {
        overmap_with_local_coords om_loc = get_om_global( p );
        om_loc.om->delete_note( om_loc.local );
    }
}

void overmapbuffer::mark_note_dangerous( const tripoint_abs_omt &p, int radius, bool is_dangerous )
{
    if( has_note( p ) ) {
        overmap_with_local_coords om_loc = get_om_global( p );
        om_loc.om->mark_note_dangerous( om_loc.local, radius, is_dangerous );
    }
}

void overmapbuffer::add_extra( const tripoint_abs_omt &p, const string_id<map_extra> &id )
{
    overmap_with_local_coords om_loc = get_om_global( p );
    om_loc.om->add_extra( om_loc.local, id );
}

void overmapbuffer::delete_extra( const tripoint_abs_omt &p )
{
    if( has_extra( p ) ) {
        overmap_with_local_coords om_loc = get_om_global( p );
        om_loc.om->delete_extra( om_loc.local );
    }
}

overmap *overmapbuffer::get_existing( const point_abs_om &p )
{
    if( last_requested_overmap && last_requested_overmap->pos() == p ) {
        return last_requested_overmap;
    }
    const auto it = overmaps.find( p );
    if( it != overmaps.end() ) {
        return last_requested_overmap = it->second.get();
    }
    if( known_non_existing.count( p ) > 0 ) {
        // This overmap does not exist on disk (this has already been
        // checked in a previous call of this function).
        return nullptr;
    }
    if( file_exist( terrain_filename( p ) ) ) {
        // File exists, load it normally (the get function
        // indirectly call overmap::open to do so).
        return &get( p );
    }
    // File does not exist (or not readable which is essentially
    // the same for our usage). A second call of this function with
    // the same coordinates will not check the file system, and
    // return early.
    // If the overmap had been created in the mean time, the previous
    // loop would have found and returned it.
    known_non_existing.insert( p );
    return nullptr;
}

bool overmapbuffer::has( const point_abs_om &p )
{
    return get_existing( p ) != nullptr;
}

overmap_with_local_coords
overmapbuffer::get_om_global( const point_abs_omt &p )
{
    return get_om_global( tripoint_abs_omt( p, 0 ) );
}

overmap_with_local_coords
overmapbuffer::get_om_global( const tripoint_abs_omt &p )
{
    point_abs_om om_pos;
    point_om_omt local;
    std::tie( om_pos, local ) = project_remain<coords::om>( p.xy() );
    overmap *om = &get( om_pos );
    return { om, tripoint_om_omt( local, p.z() ) };
}

overmap_with_local_coords
overmapbuffer::get_existing_om_global( const point_abs_omt &p )
{
    return get_existing_om_global( tripoint_abs_omt( p, 0 ) );
}

overmap_with_local_coords
overmapbuffer::get_existing_om_global( const tripoint_abs_omt &p )
{
    point_abs_om om_pos;
    point_om_omt local;
    std::tie( om_pos, local ) = project_remain<coords::om>( p.xy() );
    overmap *om = get_existing( om_pos );
    if( om == nullptr ) {
        return overmap_with_local_coords{ nullptr, tripoint_om_omt() };
    }

    return overmap_with_local_coords{ om, tripoint_om_omt( local, p.z() ) };
}

bool overmapbuffer::is_omt_generated( const tripoint_abs_omt &loc )
{
    if( overmap_with_local_coords om_loc = get_existing_om_global( loc ) ) {
        return om_loc.om->is_omt_generated( om_loc.local );
    }

    // If the overmap doesn't exist, then for sure the local mapgen
    // hasn't happened.
    return false;
}

bool overmapbuffer::has_note( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->has_note( om_loc.local );
    }
    return false;
}

bool overmapbuffer::is_marked_dangerous( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->is_marked_dangerous( om_loc.local );
    }
    return false;
}

const std::string &overmapbuffer::note( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->note( om_loc.local );
    }
    static const std::string empty_string;
    return empty_string;
}

bool overmapbuffer::has_extra( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->has_extra( om_loc.local );
    }
    return false;
}

const string_id<map_extra> &overmapbuffer::extra( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->extra( om_loc.local );
    }
    static const string_id<map_extra> id;
    return id;
}

bool overmapbuffer::is_explored( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->is_explored( om_loc.local );
    }
    return false;
}

void overmapbuffer::toggle_explored( const tripoint_abs_omt &p )
{
    const overmap_with_local_coords om_loc = get_om_global( p );
    om_loc.om->explored( om_loc.local ) = !om_loc.om->explored( om_loc.local );
}

bool overmapbuffer::has_horde( const tripoint_abs_omt &p )
{
    for( const auto &m : overmap_buffer.monsters_at( p ) ) {
        if( m->horde ) {
            return true;
        }
    }

    return false;
}

int overmapbuffer::get_horde_size( const tripoint_abs_omt &p )
{
    int horde_size = 0;
    for( const auto &m : overmap_buffer.monsters_at( p ) ) {
        if( m->horde ) {
            if( !m->monsters.empty() ) {
                horde_size += m->monsters.size();
            } else {
                // We don't know how large this will actually be, because
                // population "1" can still result in a zombie pack.
                // So we double the population as an estimate to make
                // hordes more likely to be visible on the overmap.
                horde_size += m->population * 2;
            }
        }
    }

    return horde_size;
}

bool overmapbuffer::has_camp( const tripoint_abs_omt &p )
{
    if( p.z() ) {
        return false;
    }

    const overmap_with_local_coords om_loc = get_existing_om_global( p );
    if( !om_loc ) {
        return false;
    }

    for( const auto &v : om_loc.om->camps ) {
        if( v.camp_omt_pos().xy() == p.xy() ) {
            return true;
        }
    }

    return false;
}

bool overmapbuffer::has_vehicle( const tripoint_abs_omt &p )
{
    if( p.z() ) {
        return false;
    }

    const overmap_with_local_coords om_loc = get_existing_om_global( p );
    if( !om_loc ) {
        return false;
    }

    for( const auto &v : om_loc.om->vehicles ) {
        if( v.second.p == om_loc.local.xy() ) {
            return true;
        }
    }

    return false;
}

std::vector<om_vehicle> overmapbuffer::get_vehicle( const tripoint_abs_omt &p )
{
    std::vector<om_vehicle> result;
    if( p.z() != 0 ) {
        return result;
    }
    const overmap_with_local_coords om_loc = get_existing_om_global( p );
    if( !om_loc ) {
        return result;
    }
    for( const auto &ov : om_loc.om->vehicles ) {
        if( ov.second.p == om_loc.local.xy() ) {
            result.push_back( ov.second );
        }
    }
    return result;
}

void overmapbuffer::signal_hordes( const tripoint_abs_sm &center, const int sig_power )
{
    const int radius = sig_power;
    for( auto &om : get_overmaps_near( center, radius ) ) {
        const point_abs_sm abs_pos_om = project_to<coords::sm>( om->pos() );
        const tripoint_rel_sm rel_pos = center - abs_pos_om;
        // overmap::signal_hordes expects a coordinate relative to the overmap, this is easier
        // for processing as the monster group stores is location as relative coordinates, too.
        om->signal_hordes( rel_pos, sig_power );
    }
}

void overmapbuffer::process_mongroups()
{
    // arbitrary radius to include nearby overmaps (aside from the current one)
    const int radius = MAPSIZE * 2;
    // TODO: fix point types
    const tripoint_abs_sm center( get_player_character().global_sm_location() );
    for( auto &om : get_overmaps_near( center, radius ) ) {
        om->process_mongroups();
    }
}

void overmapbuffer::move_hordes()
{
    // arbitrary radius to include nearby overmaps (aside from the current one)
    const int radius = MAPSIZE * 2;
    // TODO: fix point types
    const tripoint_abs_sm center( get_player_character().global_sm_location() );
    for( auto &om : get_overmaps_near( center, radius ) ) {
        om->move_hordes();
    }
}

std::vector<mongroup *> overmapbuffer::monsters_at( const tripoint_abs_omt &p )
{
    // (x,y) are overmap terrain coordinates, they spawn 2x2 submaps,
    // but monster groups are defined with submap coordinates.
    tripoint_abs_sm p_sm = project_to<coords::sm>( p );
    std::vector<mongroup *> result;
    for( const point &offset : std::array<point, 4> { { { point_zero }, { point_south }, { point_east }, { point_south_east } } } ) {
        std::vector<mongroup *> tmp = groups_at( p_sm + offset );
        result.insert( result.end(), tmp.begin(), tmp.end() );
    }
    return result;
}

std::vector<mongroup *> overmapbuffer::groups_at( const tripoint_abs_sm &p )
{
    std::vector<mongroup *> result;
    point_om_sm sm_within_om;
    point_abs_om omp;
    std::tie( omp, sm_within_om ) = project_remain<coords::om>( p.xy() );
    if( !has( omp ) ) {
        return result;
    }
    overmap &om = get( omp );
    auto groups_range = om.zg.equal_range( tripoint_om_sm( sm_within_om, p.z() ) );
    for( auto it = groups_range.first; it != groups_range.second; ++it ) {
        mongroup &mg = it->second;
        if( mg.empty() ) {
            continue;
        }
        result.push_back( &mg );
    }
    return result;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
std::array<std::array<scent_trace, 3>, 3> overmapbuffer::scents_near( const tripoint_abs_omt
        &origin )
{
    std::array<std::array<scent_trace, 3>, 3> found_traces;

    for( int x = -1; x <= 1 ; ++x ) {
        for( int y = -1; y <= 1; ++y ) {
            tripoint_abs_omt iter = origin + point( x, y );
            found_traces[x + 1][y + 1] = scent_at( iter );
        }
    }

    return found_traces;
}
#pragma GCC diagnostic pop

scent_trace overmapbuffer::scent_at( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->scent_at( p );
    }
    return scent_trace();
}

void overmapbuffer::set_scent( const tripoint_abs_omt &loc, int strength )
{
    const overmap_with_local_coords om_loc = get_om_global( loc );
    scent_trace new_scent( calendar::turn, strength );
    om_loc.om->set_scent( loc, new_scent );
}

void overmapbuffer::move_vehicle( vehicle *veh, const point_abs_ms &old_msp )
{
    // TODO: fix point types
    const point_abs_ms new_msp( get_map().getabs( veh->global_pos3().xy() ) );
    const point_abs_omt old_omt = project_to<coords::omt>( old_msp );
    const point_abs_omt new_omt = project_to<coords::omt>( new_msp );
    const overmap_with_local_coords old_om_loc = get_om_global( old_omt );
    const overmap_with_local_coords new_om_loc = get_om_global( new_omt );
    if( old_om_loc.om == new_om_loc.om ) {
        new_om_loc.om->vehicles[veh->om_id].p = new_om_loc.local.xy();
    } else {
        old_om_loc.om->vehicles.erase( veh->om_id );
        add_vehicle( veh );
    }
}

void overmapbuffer::remove_camp( const basecamp &camp )
{
    const point_abs_omt omt = camp.camp_omt_pos().xy();
    const overmap_with_local_coords om_loc = get_om_global( omt );
    std::vector<basecamp> &camps = om_loc.om->camps;
    for( auto it = camps.begin(); it != camps.end(); ++it ) {
        if( it->camp_omt_pos().xy() == omt ) {
            camps.erase( it );
            return;
        }
    }
}

void overmapbuffer::remove_vehicle( const vehicle *veh )
{
    // TODO: fix point types
    const point_abs_omt omt( ms_to_omt_copy( get_map().getabs( veh->global_pos3().xy() ) ) );
    const overmap_with_local_coords om_loc = get_om_global( omt );
    om_loc.om->vehicles.erase( veh->om_id );
}

void overmapbuffer::add_vehicle( vehicle *veh )
{
    const point abs_pos = get_map().getabs( veh->global_pos3().xy() );
    // TODO: fix point types
    const point_abs_omt omt( ms_to_omt_copy( abs_pos ) );
    const overmap_with_local_coords om_loc = get_om_global( omt );
    int id = om_loc.om->vehicles.size() + 1;
    // this *should* be unique but just in case
    while( om_loc.om->vehicles.count( id ) > 0 ) {
        id++;
    }
    om_vehicle &tracked_veh = om_loc.om->vehicles[id];
    tracked_veh.p = om_loc.local.xy();
    tracked_veh.name = veh->name;
    veh->om_id = id;
}

void overmapbuffer::add_camp( const basecamp &camp )
{
    const point_abs_omt omt = camp.camp_omt_pos().xy();
    const overmap_with_local_coords om_loc = get_om_global( omt );
    om_loc.om->camps.push_back( camp );
}

bool overmapbuffer::seen( const tripoint_abs_omt &p )
{
    if( const overmap_with_local_coords om_loc = get_existing_om_global( p ) ) {
        return om_loc.om->seen( om_loc.local );
    }
    return false;
}

void overmapbuffer::set_seen( const tripoint_abs_omt &p, bool seen )
{
    const overmap_with_local_coords om_loc = get_om_global( p );
    om_loc.om->seen( om_loc.local ) = seen;
}

const oter_id &overmapbuffer::ter( const tripoint_abs_omt &p )
{
    const overmap_with_local_coords om_loc = get_om_global( p );
    return om_loc.om->ter( om_loc.local );
}

void overmapbuffer::ter_set( const tripoint_abs_omt &p, const oter_id &id )
{
    const overmap_with_local_coords om_loc = get_om_global( p );
    return om_loc.om->ter_set( om_loc.local, id );
}

bool overmapbuffer::reveal( const point_abs_omt &center, int radius, int z )
{
    return reveal( tripoint_abs_omt( center, z ), radius );
}

bool overmapbuffer::reveal( const tripoint_abs_omt &center, int radius )
{
    return reveal( center, radius, []( const oter_id & ) {
        return true;
    } );
}

bool overmapbuffer::reveal( const tripoint_abs_omt &center, int radius,
                            const std::function<bool( const oter_id & )> &filter )
{
    int radius_squared = radius * radius;
    bool result = false;
    for( int i = -radius; i <= radius; i++ ) {
        for( int j = -radius; j <= radius; j++ ) {
            const tripoint_abs_omt p = center + point( i, j );
            if( seen( p ) ) {
                continue;
            }
            if( trigdist && i * i + j * j > radius_squared ) {
                continue;
            }
            if( !filter( ter( p ) ) ) {
                continue;
            }
            result = true;
            set_seen( p, true );
        }
    }
    return result;
}

std::vector<tripoint_abs_omt> overmapbuffer::get_npc_path( const tripoint_abs_omt &src,
        const tripoint_abs_omt &dest )
{
    path_type ptype;
    return get_npc_path( src, dest, ptype );
}

std::vector<tripoint_abs_omt> overmapbuffer::get_npc_path(
    const tripoint_abs_omt &src, const tripoint_abs_omt &dest, path_type &ptype )
{
    std::vector<tripoint_abs_omt> path;
    static const int RADIUS = 4;            // Maximal radius of search (in overmaps)
    static const point_rel_omt O( RADIUS * OMAPX,
                                  RADIUS * OMAPY );   // half-height of the area to search in
    if( src == overmap::invalid_tripoint || dest == overmap::invalid_tripoint ) {
        return path;
    }

    // Local source - center of the local area
    const point_rel_omt start( O );
    // To convert local coordinates to global ones
    const tripoint_abs_omt base = src - start;
    // Local destination - relative to base
    const point_rel_omt finish = ( dest - base ).xy();

    const auto get_ter_at = [&]( const point_rel_omt & p ) {
        return ter( base + p );
    };
    const auto estimate =
    [&]( const pf::node<point_rel_omt> &cur, const pf::node<point_rel_omt> * ) {
        const tripoint_abs_omt convert_result = base + tripoint_rel_omt( cur.pos, 0 );
        if( ptype.only_known_by_player && !seen( convert_result ) ) {
            return pf::rejected;
        }
        int res = 0;
        const oter_id oter = get_ter_at( cur.pos );
        int travel_cost = static_cast<int>( oter->get_travel_cost() );
        if( ptype.avoid_danger && is_marked_dangerous( convert_result ) ) {
            return pf::rejected;
        }
        if( ptype.only_road && ( !is_ot_match( "road", oter, ot_match_type::type ) &&
                                 !is_ot_match( "bridge", oter, ot_match_type::type ) &&
                                 !is_ot_match( "road_nesw_manhole", oter, ot_match_type::type ) ) ) {
            return pf::rejected;
        }
        if( ptype.only_water && !is_river_or_lake( oter ) ) {
            return pf::rejected;
        }
        if( ptype.only_air && ( !is_ot_match( "open_air", oter, ot_match_type::type ) ) ) {
            return pf::rejected;
        }
        if( is_ot_match( "empty_rock", oter, ot_match_type::type ) ) {
            return pf::rejected;
        } else if( is_ot_match( "open_air", oter, ot_match_type::type ) ) {
            if( ptype.only_air ) {
                travel_cost += 1;
            } else {
                return pf::rejected;
            }
        } else if( is_ot_match( "forest", oter, ot_match_type::type ) ) {
            travel_cost = 10;
        } else if( is_ot_match( "forest_water", oter, ot_match_type::type ) ) {
            travel_cost = 15;
        } else if( is_ot_match( "road", oter, ot_match_type::type ) ||
                   is_ot_match( "bridge", oter, ot_match_type::type ) ||
                   is_ot_match( "road_nesw_manhole", oter, ot_match_type::type ) ) {
            travel_cost = 1;
        } else if( is_river_or_lake( oter ) ) {
            if( ptype.amphibious || ptype.only_water ) {
                travel_cost = 1;
            } else {
                return pf::rejected;
            }
        }
        res += travel_cost;
        res += manhattan_dist( finish, cur.pos );

        return res;
    };
    pf::path<point_rel_omt> route = pf::find_path( start, finish, 2 * O, estimate );
    for( auto node : route.nodes ) {
        tripoint_abs_omt convert_result = base + tripoint_rel_omt( node.pos, 0 );
        convert_result.z() = base.z();
        path.push_back( convert_result );
    }
    return path;
}

bool overmapbuffer::reveal_route( const tripoint_abs_omt &source, const tripoint_abs_omt &dest,
                                  int radius, bool road_only )
{
    static const int RADIUS = 4;            // Maximal radius of search (in overmaps)
    static const point_rel_omt O( RADIUS * OMAPX,
                                  RADIUS * OMAPY );   // half-height of the area to search in

    if( source == overmap::invalid_tripoint || dest == overmap::invalid_tripoint ) {
        return false;
    }

    // Local source - center of the local area
    const point_rel_omt start( O );
    // To convert local coordinates to global ones
    const tripoint_abs_omt base = source - start;
    // Local destination - relative to base
    const point_rel_omt finish = ( dest - base ).xy();

    const auto get_ter_at = [&]( const point_rel_omt & p ) {
        return ter( base + p );
    };

    const oter_id oter = get_ter_at( start );
    const auto connection = overmap_connections::guess_for( oter );

    if( !connection ) {
        return false;
    }

    const auto estimate =
    [&]( const pf::node<point_rel_omt> &cur, const pf::node<point_rel_omt> * ) {
        int res = 0;

        const oter_id oter = get_ter_at( cur.pos );

        if( !connection->has( oter ) ) {
            if( road_only ) {
                return pf::rejected;
            }

            if( is_river( oter ) ) {
                return pf::rejected; // Can't walk on water
            }
            // Allow going slightly off-road to overcome small obstacles (e.g. craters),
            // but heavily penalize that to make roads preferable
            res += 250;
        }

        res += manhattan_dist( finish, cur.pos );

        return res;
    };

    const auto path = pf::find_path( start, finish, 2 * O, estimate );

    for( const auto &node : path.nodes ) {
        reveal( base + node.pos, radius );
    }
    return !path.nodes.empty();
}

bool overmapbuffer::check_ot_existing( const std::string &type, ot_match_type match_type,
                                       const tripoint_abs_omt &loc )
{
    const overmap_with_local_coords om_loc = get_existing_om_global( loc );
    if( !om_loc ) {
        return false;
    }
    return om_loc.om->check_ot( type, match_type, om_loc.local );
}

bool overmapbuffer::check_overmap_special_type_existing(
    const overmap_special_id &id, const tripoint_abs_omt &loc )
{
    const overmap_with_local_coords om_loc = get_existing_om_global( loc );
    if( !om_loc ) {
        return false;
    }
    return om_loc.om->check_overmap_special_type( id, om_loc.local );
}

bool overmapbuffer::check_ot( const std::string &type, ot_match_type match_type,
                              const tripoint_abs_omt &p )
{
    const overmap_with_local_coords om_loc = get_om_global( p );
    return om_loc.om->check_ot( type, match_type, om_loc.local );
}

bool overmapbuffer::check_overmap_special_type( const overmap_special_id &id,
        const tripoint_abs_omt &loc )
{
    const overmap_with_local_coords om_loc = get_om_global( loc );
    return om_loc.om->check_overmap_special_type( id, om_loc.local );
}

static omt_find_params assign_params(
    const std::string &type, int const radius, bool must_be_seen,
    ot_match_type match_type, bool existing_overmaps_only,
    const cata::optional<overmap_special_id> &om_special )
{
    omt_find_params params;
    std::vector<std::pair<std::string, ot_match_type>> temp_types;
    std::pair<std::string, ot_match_type> temp_pair;
    temp_pair.first = type;
    temp_pair.second = match_type;
    temp_types.push_back( temp_pair );
    params.types = temp_types;
    params.search_range = radius;
    params.must_see = must_be_seen;
    params.existing_only = existing_overmaps_only;
    params.om_special = om_special;
    return params;
}

bool overmapbuffer::is_findable_location( const tripoint_abs_omt &location,
        const omt_find_params &params )
{
    bool type_matches = false;
    if( params.existing_only ) {
        for( const std::pair<std::string, ot_match_type> &elem : params.types ) {
            type_matches = check_ot_existing( elem.first, elem.second, location );
            if( type_matches ) {
                break;
            }
        }
    } else {
        for( const std::pair<std::string, ot_match_type> &elem : params.types ) {
            type_matches = check_ot( elem.first, elem.second, location );
            if( type_matches ) {
                break;
            }
        }
    }
    if( !type_matches ) {
        return false;
    }

    if( params.must_see && !seen( location ) ) {
        return false;
    }
    if( params.cant_see && seen( location ) ) {
        return false;
    }

    if( params.om_special ) {
        bool meets_om_special = false;
        if( params.existing_only ) {
            meets_om_special = check_overmap_special_type_existing( *params.om_special, location );
        } else {
            meets_om_special = check_overmap_special_type( *params.om_special, location );
        }
        if( !meets_om_special ) {
            return false;
        }
    }

    return true;
}

tripoint_abs_omt overmapbuffer::find_closest(
    const tripoint_abs_omt &origin, const std::string &type, int const radius, bool must_be_seen,
    ot_match_type match_type, bool existing_overmaps_only,
    const cata::optional<overmap_special_id> &om_special )
{
    const omt_find_params params = assign_params( type, radius, must_be_seen, match_type,
                                   existing_overmaps_only, om_special );
    return find_closest( origin, params );
}

tripoint_abs_omt overmapbuffer::find_closest( const tripoint_abs_omt &origin,
        const omt_find_params &params )
{
    // Check the origin before searching adjacent tiles!
    if( params.min_distance == 0 && is_findable_location( origin, params ) ) {
        return origin;
    }

    // By default search overmaps within a radius of 4,
    // i.e. C = current overmap, X = overmaps searched:
    // XXXXXXXXX
    // XXXXXXXXX
    // XXXXXXXXX
    // XXXXXXXXX
    // XXXXCXXXX
    // XXXXXXXXX
    // XXXXXXXXX
    // XXXXXXXXX
    // XXXXXXXXX
    //
    // See overmap::place_specials for how we attempt to insure specials are placed within this
    // range.  The actual number is 5 because 1 covers the current overmap,
    // and each additional one expends the search to the next concentric circle of overmaps.
    const int min_dist = params.min_distance;
    const int max_dist = params.search_range ? params.search_range : OMAPX * 5;

    std::vector<tripoint_abs_omt> result;
    cata::optional<int> found_dist;

    for( const point_abs_omt &loc_xy : closest_points_first( origin.xy(), min_dist, max_dist ) ) {
        const int dist_xy = square_dist( origin.xy(), loc_xy );

        if( found_dist && *found_dist < dist_xy ) {
            break;
        }

        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
            const tripoint_abs_omt loc( loc_xy, z );
            const int dist = square_dist( origin, loc );

            if( found_dist && *found_dist < dist ) {
                continue;
            }

            if( is_findable_location( loc, params ) ) {
                found_dist = dist;
                result.push_back( loc );
            }
        }
    }

    return random_entry( result, overmap::invalid_tripoint );
}

std::vector<tripoint_abs_omt> overmapbuffer::find_all( const tripoint_abs_omt &origin,
        const omt_find_params &params )
{
    std::vector<tripoint_abs_omt> result;
    // dist == 0 means search a whole overmap diameter.
    const int min_dist = params.min_distance;
    const int max_dist = params.search_range ? params.search_range : OMAPX;

    for( const tripoint_abs_omt &loc : closest_points_first( origin, min_dist, max_dist ) ) {
        if( is_findable_location( loc, params ) ) {
            result.push_back( loc );
        }
    }

    return result;
}

std::vector<tripoint_abs_omt> overmapbuffer::find_all(
    const tripoint_abs_omt &origin, const std::string &type, int dist, bool must_be_seen,
    ot_match_type match_type, bool existing_overmaps_only,
    const cata::optional<overmap_special_id> &om_special )
{
    const omt_find_params params = assign_params( type, dist, must_be_seen, match_type,
                                   existing_overmaps_only, om_special );
    return find_all( origin, params );
}

tripoint_abs_omt overmapbuffer::find_random( const tripoint_abs_omt &origin,
        const omt_find_params &params )
{
    return random_entry( find_all( origin, params ), overmap::invalid_tripoint );
}

tripoint_abs_omt overmapbuffer::find_random(
    const tripoint_abs_omt &origin, const std::string &type, int dist, bool must_be_seen,
    ot_match_type match_type, bool existing_overmaps_only,
    const cata::optional<overmap_special_id> &om_special )
{
    const omt_find_params params = assign_params( type, dist, must_be_seen, match_type,
                                   existing_overmaps_only, om_special );
    return find_random( origin, params );
}

shared_ptr_fast<npc> overmapbuffer::find_npc( character_id id )
{
    for( auto &it : overmaps ) {
        if( auto p = it.second->find_npc( id ) ) {
            return p;
        }
    }
    return nullptr;
}

cata::optional<basecamp *> overmapbuffer::find_camp( const point_abs_omt &p )
{
    for( auto &it : overmaps ) {
        const point_abs_omt p2( p );
        for( int x2 = p2.x() - 3; x2 < p2.x() + 3; x2++ ) {
            for( int y2 = p2.y() - 3; y2 < p2.y() + 3; y2++ ) {
                if( cata::optional<basecamp *> camp = it.second->find_camp( point_abs_omt( x2, y2 ) ) ) {
                    return camp;
                }
            }
        }
    }
    return cata::nullopt;
}

void overmapbuffer::insert_npc( const shared_ptr_fast<npc> &who )
{
    cata_assert( who );
    const tripoint_abs_omt npc_omt_pos = who->global_omt_location();
    const point_abs_om npc_om_pos = project_to<coords::om>( npc_omt_pos.xy() );
    get( npc_om_pos ).insert_npc( who );
}

shared_ptr_fast<npc> overmapbuffer::remove_npc( const character_id &id )
{
    for( auto &it : overmaps ) {
        if( const auto p = it.second->erase_npc( id ) ) {
            return p;
        }
    }
    debugmsg( "overmapbuffer::remove_npc: NPC (%d) not found.", id.get_value() );
    return nullptr;
}

std::vector<shared_ptr_fast<npc>> overmapbuffer::get_npcs_near_player( int radius )
{
    tripoint_abs_omt plpos_omt = get_player_character().global_omt_location();
    // get_npcs_near needs submap coordinates
    tripoint_abs_sm plpos = project_to<coords::sm>( plpos_omt );
    // INT_MIN is a (a bit ugly) way to inform get_npcs_near not to filter by z-level
    const int zpos = get_map().has_zlevels() ? INT_MIN : plpos.z();
    return get_npcs_near( tripoint_abs_sm( plpos.xy(), zpos ), radius );
}

std::vector<overmap *> overmapbuffer::get_overmaps_near( const tripoint_abs_sm &location,
        const int radius )
{
    // Grab the corners of a square around the target location at distance radius.
    // Convert to overmap coordinates and iterate from the minimum to the maximum.
    const point_abs_om start =
        project_to<coords::om>( location.xy() + point( -radius, -radius ) );
    const point_abs_om end =
        project_to<coords::om>( location.xy() + point( radius, radius ) );
    const point_rel_om offset = end - start;

    std::vector<overmap *> result;
    result.reserve( static_cast<size_t>( offset.x() + 1 ) * static_cast<size_t>( offset.y() + 1 ) );

    for( int x = start.x(); x <= end.x(); ++x ) {
        for( int y = start.y(); y <= end.y(); ++y ) {
            if( overmap *existing_om = get_existing( point_abs_om( x, y ) ) ) {
                result.emplace_back( existing_om );
            }
        }
    }

    // Sort the resulting overmaps so that the closest ones are first.
    const tripoint_abs_om center = project_to<coords::om>( location );
    std::sort( result.begin(), result.end(), [&center]( const overmap * lhs,
    const overmap * rhs ) {
        const tripoint_abs_om lhs_pos( lhs->pos(), 0 );
        const tripoint_abs_om rhs_pos( rhs->pos(), 0 );
        return trig_dist( center, lhs_pos ) < trig_dist( center, rhs_pos );
    } );

    return result;
}

std::vector<overmap *> overmapbuffer::get_overmaps_near( const point_abs_sm &p, const int radius )
{
    return get_overmaps_near( tripoint_abs_sm( p, 0 ), radius );
}

std::vector<shared_ptr_fast<npc>> overmapbuffer::get_companion_mission_npcs( int range )
{
    std::vector<shared_ptr_fast<npc>> available;
    // TODO: this is an arbitrary radius, replace with something sane.
    for( const auto &guy : get_npcs_near_player( range ) ) {
        if( guy->has_companion_mission() ) {
            available.push_back( guy );
        }
    }
    return available;
}

// If z == INT_MIN, allow all z-levels
std::vector<shared_ptr_fast<npc>> overmapbuffer::get_npcs_near( const tripoint_abs_sm &p,
                               int radius )
{
    std::vector<shared_ptr_fast<npc>> result;
    for( auto &it : get_overmaps_near( p.xy(), radius ) ) {
        auto temp = it->get_npcs( [&]( const npc & guy ) {
            // Global position of NPC, in submap coordinates
            // TODO: fix point types
            const tripoint_abs_sm pos( guy.global_sm_location() );
            if( p.z() != INT_MIN && pos.z() != p.z() ) {
                return false;
            }
            return square_dist( p.xy(), pos.xy() ) <= radius;
        } );
        result.insert( result.end(), temp.begin(), temp.end() );
    }
    return result;
}

// If z == INT_MIN, allow all z-levels
std::vector<shared_ptr_fast<npc>> overmapbuffer::get_npcs_near_omt( const tripoint_abs_omt &p,
                               int radius )
{
    std::vector<shared_ptr_fast<npc>> result;
    for( auto &it : get_overmaps_near( project_to<coords::sm>( p.xy() ), radius ) ) {
        auto temp = it->get_npcs( [&]( const npc & guy ) {
            // Global position of NPC, in submap coordinates
            tripoint_abs_omt pos = guy.global_omt_location();
            if( p.z() != INT_MIN && pos.z() != p.z() ) {
                return false;
            }
            return square_dist( p.xy(), pos.xy() ) <= radius;
        } );
        result.insert( result.end(), temp.begin(), temp.end() );
    }
    return result;
}

static radio_tower_reference create_radio_tower_reference( const overmap &om, radio_tower &t,
        const tripoint_abs_sm &center )
{
    // global submap coordinates, same as center is
    const point_abs_sm pos = project_combine( om.pos(), t.pos );
    const int strength = t.strength - rl_dist( tripoint_abs_sm( pos, 0 ), center );
    return radio_tower_reference{ &t, pos, strength };
}

radio_tower_reference overmapbuffer::find_radio_station( const int frequency )
{
    // TODO: fix point types
    const tripoint_abs_sm center( get_player_character().global_sm_location() );
    for( auto &om : get_overmaps_near( center, RADIO_MAX_STRENGTH ) ) {
        for( auto &tower : om->radios ) {
            const radio_tower_reference rref = create_radio_tower_reference( *om, tower, center );
            if( rref.signal_strength > 0 && tower.frequency == frequency ) {
                return rref;
            }
        }
    }
    return radio_tower_reference{ nullptr, point_abs_sm(), 0 };
}

std::vector<radio_tower_reference> overmapbuffer::find_all_radio_stations()
{
    std::vector<radio_tower_reference> result;
    // TODO: fix point types
    const tripoint_abs_sm center( get_player_character().global_sm_location() );
    // perceived signal strength is distance (in submaps) - signal strength, so towers
    // further than RADIO_MAX_STRENGTH submaps away can never be received at all.
    const int radius = RADIO_MAX_STRENGTH;
    for( auto &om : get_overmaps_near( center, radius ) ) {
        for( auto &tower : om->radios ) {
            const radio_tower_reference rref = create_radio_tower_reference( *om, tower, center );
            if( rref.signal_strength > 0 ) {
                result.push_back( rref );
            }
        }
    }
    return result;
}

std::vector<camp_reference> overmapbuffer::get_camps_near( const tripoint_abs_sm &location,
        int radius )
{
    std::vector<camp_reference> result;
    for( overmap *om : get_overmaps_near( location, radius ) ) {
        result.reserve( result.size() + om->camps.size() );
        std::transform( om->camps.begin(), om->camps.end(), std::back_inserter( result ),
        [&]( basecamp & element ) {
            const tripoint_abs_omt camp_pt = element.camp_omt_pos();
            const tripoint_abs_sm camp_sm = project_to<coords::sm>( camp_pt );
            const int distance = rl_dist( camp_sm, location );

            return camp_reference{ &element, camp_sm, distance };
        } );
    }
    std::sort( result.begin(), result.end(), []( const camp_reference & lhs,
    const camp_reference & rhs ) {
        return lhs.get_distance_from_bounds() < rhs.get_distance_from_bounds();
    } );
    return result;
}

std::vector<shared_ptr_fast<npc>> overmapbuffer::get_overmap_npcs()
{
    std::vector<shared_ptr_fast<npc>> result;
    for( auto &om : overmaps ) {
        const overmap &overmap = *om.second;
        for( const auto &guy : overmap.npcs ) {
            result.push_back( guy );
        }
    }
    return result;
}

std::vector<city_reference> overmapbuffer::get_cities_near( const tripoint_abs_sm &location,
        int radius )
{
    std::vector<city_reference> result;

    for( overmap *om : get_overmaps_near( location, radius ) ) {
        result.reserve( result.size() + om->cities.size() );
        std::transform( om->cities.begin(), om->cities.end(), std::back_inserter( result ),
        [&]( city & element ) {
            const auto rel_pos_city = project_to<coords::sm>( element.pos );
            const auto abs_pos_city =
                tripoint_abs_sm( project_combine( om->pos(), rel_pos_city ), 0 );
            const int distance = rl_dist( abs_pos_city, location );

            return city_reference{ &element, abs_pos_city, distance };
        } );
    }

    std::sort( result.begin(), result.end(), []( const city_reference & lhs,
    const city_reference & rhs ) {
        return lhs.get_distance_from_bounds() < rhs.get_distance_from_bounds();
    } );

    return result;
}

city_reference overmapbuffer::closest_city( const tripoint_abs_sm &center )
{
    const auto cities = get_cities_near( center, omt_to_sm_copy( OMAPX ) );

    if( !cities.empty() ) {
        return cities.front();
    }

    return city_reference::invalid;
}

city_reference overmapbuffer::closest_known_city( const tripoint_abs_sm &center )
{
    const auto cities = get_cities_near( center, omt_to_sm_copy( OMAPX ) );
    const auto it = std::find_if( cities.begin(), cities.end(),
    [this]( const city_reference & elem ) {
        const tripoint_abs_omt p = project_to<coords::omt>( elem.abs_sm_pos );
        return seen( p );
    } );

    if( it != cities.end() ) {
        return *it;
    }

    return city_reference::invalid;
}

std::string overmapbuffer::get_description_at( const tripoint_abs_sm &where )
{
    const auto oter = ter( project_to<coords::omt>( where ) );
    const nc_color ter_color = oter->get_color();
    const std::string ter_name = colorize( oter->get_name(), ter_color );

    if( where.z() != 0 ) {
        return ter_name;
    }

    const city_reference closest_cref = closest_known_city( where );

    if( !closest_cref ) {
        return ter_name;
    }

    const auto &closest_city = *closest_cref.city;
    const std::string closest_city_name = colorize( closest_city.name, c_yellow );
    const direction dir = direction_from( closest_cref.abs_sm_pos, where );
    const std::string dir_name = colorize( direction_name( dir ), c_light_gray );

    const int sm_size = omt_to_sm_copy( closest_cref.city->size );
    const int sm_dist = closest_cref.distance;

    //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
    std::string format_string = pgettext( "terrain description", "%1$s %2$s from %3$s" );
    if( sm_dist <= 3 * sm_size / 4 ) {
        if( sm_size >= 16 ) {
            // The city is big enough to be split in districts.
            if( sm_dist <= sm_size / 4 ) {
                //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
                format_string = pgettext( "terrain description", "%1$s in central %3$s" );
            } else {
                //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
                format_string = pgettext( "terrain description", "%1$s in %2$s %3$s" );
            }
        } else {
            //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
            format_string = pgettext( "terrain description", "%1$s in %3$s" );
        }
    } else if( sm_dist <= sm_size ) {
        if( sm_size >= 8 ) {
            // The city is big enough to have outskirts.
            //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
            format_string = pgettext( "terrain description", "%1$s on the %2$s outskirts of %3$s" );
        } else {
            //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
            format_string = pgettext( "terrain description", "%1$s in %3$s" );
        }
    }

    return string_format( format_string, ter_name, dir_name, closest_city_name );
}

void overmapbuffer::spawn_monster( const tripoint_abs_sm &p )
{
    // Create a copy, so we can reuse x and y later
    point_abs_sm abs_sm = p.xy();
    point_om_sm sm;
    point_abs_om omp;
    std::tie( omp, sm ) = project_remain<coords::om>( abs_sm );
    overmap &om = get( omp );
    const tripoint_om_sm current_submap_loc( sm, p.z() );
    auto monster_bucket = om.monster_map.equal_range( current_submap_loc );
    std::for_each( monster_bucket.first, monster_bucket.second,
    [&]( std::pair<const tripoint_om_sm, monster> &monster_entry ) {
        monster &this_monster = monster_entry.second;
        // The absolute position in map squares, (x,y) is already global, but it's a
        // submap coordinate, so translate it and add the exact monster position on
        // the submap. modulo because the zombies position might be negative, as it
        // is stored *after* it has gone out of bounds during shifting. When reloading
        // we only need the part that tells where on the submap to put it.
        point ms( modulo( this_monster.posx(), SEEX ), modulo( this_monster.posy(), SEEY ) );
        cata_assert( ms.x >= 0 && ms.x < SEEX );
        cata_assert( ms.y >= 0 && ms.y < SEEX );
        // TODO: fix point types
        ms += project_to<coords::ms>( p.xy() ).raw();
        const map &here = get_map();
        // The monster position must be local to the main map when added to the game
        const tripoint local = tripoint( here.getlocal( ms ), p.z() );
        cata_assert( here.inbounds( local ) );
        monster *const placed = g->place_critter_around( make_shared_fast<monster>( this_monster ),
                                local, 0, true );
        if( placed ) {
            placed->on_load();
        }
    } );
    om.monster_map.erase( current_submap_loc );
}

void overmapbuffer::despawn_monster( const monster &critter )
{
    // Get absolute coordinates of the monster in map squares, translate to submap position
    // TODO: fix point types
    tripoint_abs_sm abs_sm( ms_to_sm_copy( get_map().getabs( critter.pos() ) ) );
    // Get the overmap coordinates and get the overmap, sm is now local to that overmap
    point_abs_om omp;
    tripoint_om_sm sm;
    std::tie( omp, sm ) = project_remain<coords::om>( abs_sm );
    overmap &om = get( omp );
    // Store the monster using coordinates local to the overmap.
    om.monster_map.insert( std::make_pair( sm, critter ) );
}

overmapbuffer::t_notes_vector overmapbuffer::get_notes( int z, const std::string *pattern )
{
    t_notes_vector result;
    for( auto &it : overmaps ) {
        const overmap &om = *it.second;
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                tripoint_om_omt p( i, j, z );
                const std::string &note = om.note( p );
                if( note.empty() ) {
                    continue;
                }
                if( pattern != nullptr && lcmatch( note, *pattern ) ) {
                    // pattern not found in note text
                    continue;
                }
                result.push_back( t_point_with_note(
                                      project_combine( om.pos(), p.xy() ),
                                      om.note( p )
                                  ) );
            }
        }
    }
    return result;
}

overmapbuffer::t_extras_vector overmapbuffer::get_extras( int z, const std::string *pattern )
{
    overmapbuffer::t_extras_vector result;
    for( auto &it : overmaps ) {
        const overmap &om = *it.second;
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                tripoint_om_omt p( i, j, z );
                const string_id<map_extra> &extra = om.extra( p );
                if( extra.is_null() ) {
                    continue;
                }
                const std::string &extra_text = extra.c_str();
                if( pattern != nullptr && lcmatch( extra_text, *pattern ) ) {
                    // pattern not found in note text
                    continue;
                }
                result.push_back( t_point_with_extra(
                                      project_combine( om.pos(), p.xy() ),
                                      om.extra( p )
                                  ) );
            }
        }
    }
    return result;
}

bool overmapbuffer::is_safe( const tripoint_abs_omt &p )
{
    for( auto &mongrp : monsters_at( p ) ) {
        if( !mongrp->is_safe() ) {
            return false;
        }
    }
    return true;
}

bool overmapbuffer::place_special(
    const overmap_special &special, const tripoint_abs_omt &p, om_direction::type dir,
    const bool must_be_unexplored, const bool force )
{
    const overmap_with_local_coords om_loc = get_om_global( p );

    bool placed = false;
    // Only place this special if we can actually place it per its criteria, or we're forcing
    // the placement, which is mostly a debug behavior, since a forced placement may not function
    // correctly (e.g. won't check correct underlying terrain).

    if( om_loc.om->can_place_special(
            special, om_loc.local, dir, must_be_unexplored ) || force ) {
        // Get the closest city that is within the overmap because
        // all of the overmap generation functions only function within
        // the single overmap. If future generation is hoisted up to the
        // buffer to spawn overmaps, then this can also be changed accordingly.
        const city c = om_loc.om->get_nearest_city( om_loc.local );
        om_loc.om->place_special( special, om_loc.local, dir, c,
                                  must_be_unexplored, force );
        placed = true;
    }
    return placed;
}

bool overmapbuffer::place_special( const overmap_special_id &special_id,
                                   const tripoint_abs_omt &center, int radius )
{
    // First find the requested special. If it doesn't exist, we're done here.
    bool found = false;
    overmap_special special;
    for( const auto &elem : overmap_specials::get_all() ) {
        if( elem.id == special_id ) {
            special = elem;
            found = true;
            break;
        }
    }
    if( !found ) {
        return false;
    }

    // Force our special to occur just once when we're spawning it here.
    special.occurrences.min = 1;
    special.occurrences.max = 1;

    // Figure out the longest side of the special for purposes of determining our sector size
    // when attempting placements.
    const auto calculate_longest_side = [&special]() {
        auto min_max_x = std::minmax_element( special.terrains.begin(),
                                              special.terrains.end(), []( const overmap_special_terrain & lhs,
        const overmap_special_terrain & rhs ) {
            return lhs.p.x < rhs.p.x;
        } );

        auto min_max_y = std::minmax_element( special.terrains.begin(),
                                              special.terrains.end(), []( const overmap_special_terrain & lhs,
        const overmap_special_terrain & rhs ) {
            return lhs.p.y < rhs.p.y;
        } );

        const int special_longest_side = std::max( min_max_x.second->p.x - min_max_x.first->p.x,
                                         min_max_y.second->p.y - min_max_y.first->p.y ) + 1;

        // If our longest side is greater than the OMSPEC_FREQ, just use that instead.
        return std::min( special_longest_side, OMSPEC_FREQ );
    };

    const int longest_side = calculate_longest_side();

    // Predefine our sectors to search in.
    om_special_sectors sectors = get_sectors( longest_side );

    // Get all of the overmaps within the defined radius of the center.
    for( const auto &om : get_overmaps_near(
             project_to<coords::sm>( center ), omt_to_sm_copy( radius ) ) ) {

        // Build an overmap_special_batch for the special on this overmap.
        std::vector<const overmap_special *> specials;
        specials.push_back( &special );
        overmap_special_batch batch( om->pos(), specials );

        // Filter the sectors to those which are in in range of our center point, so
        // that we don't end up creating specials in areas that are outside of our radius,
        // since the whole point is to create a special that is within the parameters.
        std::vector<point_om_omt> sector_points_in_range;
        std::copy_if( sectors.sectors.begin(), sectors.sectors.end(),
        std::back_inserter( sector_points_in_range ), [&]( point_om_omt & p ) {
            const point_abs_omt global_sector_point = project_combine( om->pos(), p );
            // We'll include this sector if it's within our radius. We reduce the radius by
            // the length of the longest side of our special so that we don't end up in a
            // scenario where one overmap terrain of the special is within the radius but the
            // rest of it is outside the radius (due to size, rotation, etc), which would
            // then result in us placing the special but then not finding it later if we
            // search using the same radius value we used in placing it.
            return square_dist( global_sector_point, center.xy() ) <= radius - longest_side;
        } );
        om_special_sectors sectors_in_range {sector_points_in_range, sectors.sector_width};

        // Attempt to place the specials using our batch and sectors. We
        // require they be placed in unexplored terrain right now.
        om->place_specials_pass( batch, sectors_in_range, true, true );

        // The place special pass will erase specials that have reached their
        // maximum number of instances so first check if its been erased.
        if( batch.empty() ) {
            return true;
        }

        // Hasn't been erased, so lets check placement counts.
        for( const auto &special_placement : batch ) {
            if( special_placement.instances_placed > 0 ) {
                // It was placed, lets get outta here.
                return true;
            }
        }
    }

    // If we got this far, we've failed to make the placement.
    return false;
}
