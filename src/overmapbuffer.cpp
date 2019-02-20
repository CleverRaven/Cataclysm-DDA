#include "overmapbuffer.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "mongroup.h"
#include "monster.h"
#include "npc.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "overmap_types.h"
#include "simple_pathfinding.h"
#include "string_formatter.h"
#include "vehicle.h"

overmapbuffer overmap_buffer;

overmapbuffer::overmapbuffer()
    : last_requested_overmap( nullptr )
{
}

const city_reference city_reference::invalid{ nullptr, tripoint(), -1 };

int city_reference::get_distance_from_bounds() const
{
    assert( city != nullptr );
    return distance - omt_to_sm_copy( city->size );
}

std::string overmapbuffer::terrain_filename( const int x, const int y )
{
    std::ostringstream filename;

    filename << g->get_world_base_save_path() << "/";
    filename << "o." << x << "." << y;

    return filename.str();
}

std::string overmapbuffer::player_filename( const int x, const int y )
{
    std::ostringstream filename;

    filename << g->get_player_base_save_path() << ".seen." << x << "." << y;

    return filename.str();
}

overmap &overmapbuffer::get( const int x, const int y )
{
    const point p { x, y };

    if( last_requested_overmap != nullptr && last_requested_overmap->pos() == p ) {
        return *last_requested_overmap;
    }

    const auto it = overmaps.find( p );
    if( it != overmaps.end() ) {
        return *( last_requested_overmap = it->second.get() );
    }

    // That constructor loads an existing overmap or creates a new one.
    overmap *new_om = new overmap( x, y );
    overmaps[ p ] = std::unique_ptr<overmap>( new_om );
    new_om->populate();
    // Note: fix_mongroups might load other overmaps, so overmaps.back() is not
    // necessarily the overmap at (x,y)
    fix_mongroups( *new_om );
    fix_npcs( *new_om );

    last_requested_overmap = new_om;
    return *new_om;
}

void overmapbuffer::create_custom_overmap( const int x, const int y,
        overmap_special_batch &specials )
{
    overmap *new_om = new overmap( x, y );
    if( last_requested_overmap != nullptr ) {
        auto om_iter = overmaps.find( new_om->pos() );
        if( om_iter != overmaps.end() && om_iter->second.get() == last_requested_overmap ) {
            last_requested_overmap = nullptr;
        }
    }
    overmaps[ new_om->pos() ] = std::unique_ptr<overmap>( new_om );
    new_om->populate( specials );
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
        if( mg.pos.x >= 0 && mg.pos.y >= 0 && mg.pos.x < OMAPX * 2 && mg.pos.y < OMAPY * 2 ) {
            ++it;
            continue;
        }
        point smabs( mg.pos.x + new_overmap.pos().x * OMAPX * 2,
                     mg.pos.y + new_overmap.pos().y * OMAPY * 2 );
        point omp = sm_to_om_remain( smabs );
        if( !has( omp.x, omp.y ) ) {
            // Don't generate new overmaps, as this can be called from the
            // overmap-generating code.
            ++it;
            continue;
        }
        overmap &om = get( omp.x, omp.y );
        mg.pos.x = smabs.x;
        mg.pos.y = smabs.y;
        om.add_mon_group( mg );
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
        const tripoint npc_omt_pos = np.global_omt_location();
        const point npc_om_pos = omt_to_om_copy( npc_omt_pos.x, npc_omt_pos.y );
        const point &loc = new_overmap.pos();
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
        const tripoint npc_omt_pos = np.global_omt_location();
        const point npc_om_pos = omt_to_om_copy( npc_omt_pos.x, npc_omt_pos.y );
        const point &loc = new_overmap.pos();
        if( !has( npc_om_pos.x, npc_om_pos.y ) ) {
            // This can't really happen without save editing
            // We have no sane option here, just place the NPC on the edge
            debugmsg( "NPC %s is out of bounds, on non-generated overmap %d,%d",
                      np.name.c_str(), loc.x, loc.y );
            point npc_sm = om_to_sm_copy( npc_om_pos );
            point min = om_to_sm_copy( loc );
            point max = om_to_sm_copy( loc + point( 1, 1 ) ) - point( 1, 1 );
            npc_sm.x = clamp( npc_sm.x, min.x, max.x );
            npc_sm.y = clamp( npc_sm.y, min.y, max.y );
            np.spawn_at_sm( npc_sm.x, npc_sm.y, np.posz() );
            new_overmap.npcs.push_back( ptr );
            continue;
        }

        // Simplest case: just move the pointer
        get( npc_om_pos.x, npc_om_pos.y ).insert_npc( ptr );
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

const regional_settings &overmapbuffer::get_settings( int x, int y, int z )
{
    ( void )z;
    overmap &om = get_om_global( x, y );
    return om.get_settings();
}

void overmapbuffer::add_note( int x, int y, int z, const std::string &message )
{
    overmap &om = get_om_global( x, y );
    om.add_note( x, y, z, message );
}

void overmapbuffer::delete_note( int x, int y, int z )
{
    if( has_note( x, y, z ) ) {
        overmap &om = get_om_global( x, y );
        om.delete_note( x, y, z );
    }
}

overmap *overmapbuffer::get_existing( int x, int y )
{
    const point p {x, y};

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
    if( file_exist( terrain_filename( x, y ) ) ) {
        // File exists, load it normally (the get function
        // indirectly call overmap::open to do so).
        return &get( x, y );
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

bool overmapbuffer::has( int x, int y )
{
    return get_existing( x, y ) != nullptr;
}

overmap &overmapbuffer::get_om_global( int &x, int &y )
{
    const point om_pos = omt_to_om_remain( x, y );
    return get( om_pos.x, om_pos.y );
}

overmap &overmapbuffer::get_om_global( const point &p )
{
    const point om_pos = omt_to_om_copy( p );
    return get( om_pos.x, om_pos.y );
}

overmap &overmapbuffer::get_om_global( const tripoint &p )
{
    const point om_pos = omt_to_om_copy( { p.x, p.y } );
    return get( om_pos.x, om_pos.y );
}

overmap_with_local_coordinates overmapbuffer::get_om_global_with_coordinates( const tripoint &p )
{
    int x = p.x;
    int y = p.y;
    const point om_pos = omt_to_om_remain( x, y );
    overmap *om = &get( om_pos.x, om_pos.y );
    return { om, tripoint( x, y, p.z ) };
}

overmap *overmapbuffer::get_existing_om_global( int &x, int &y )
{
    const point om_pos = omt_to_om_remain( x, y );
    return get_existing( om_pos.x, om_pos.y );
}

overmap *overmapbuffer::get_existing_om_global( const point &p )
{
    const point om_pos = omt_to_om_copy( p );
    return get_existing( om_pos.x, om_pos.y );
}

overmap *overmapbuffer::get_existing_om_global( const tripoint &p )
{
    const tripoint om_pos = omt_to_om_copy( p );
    return get_existing( om_pos.x, om_pos.y );
}

cata::optional<overmap_with_local_coordinates>
overmapbuffer::get_existing_om_global_with_coordinates(
    const tripoint &p )
{
    int x = p.x;
    int y = p.y;
    const point om_pos = omt_to_om_remain( x, y );
    overmap *om = get_existing( om_pos.x, om_pos.y );
    if( om == nullptr ) {
        return cata::nullopt;
    }

    return overmap_with_local_coordinates { om, tripoint( x, y, p.z ) };
}

bool overmapbuffer::is_omt_generated( const tripoint &loc )
{
    cata::optional<overmap_with_local_coordinates> om = get_existing_om_global_with_coordinates( loc );

    // If the overmap doesn't exist, then for sure the local mapgen
    // hasn't happened.
    if( !om ) {
        return false;
    }

    return om->overmap_pointer->is_omt_generated( om->coordinates );
}

bool overmapbuffer::has_note( int x, int y, int z )
{
    const overmap *om = get_existing_om_global( x, y );
    return ( om != nullptr ) && om->has_note( x, y, z );
}

const std::string &overmapbuffer::note( int x, int y, int z )
{
    const overmap *om = get_existing_om_global( x, y );
    if( om == nullptr ) {
        static const std::string empty_string;
        return empty_string;
    }
    return om->note( x, y, z );
}

bool overmapbuffer::is_explored( int x, int y, int z )
{
    const overmap *om = get_existing_om_global( x, y );
    return ( om != nullptr ) && om->is_explored( x, y, z );
}

void overmapbuffer::toggle_explored( int x, int y, int z )
{
    overmap &om = get_om_global( x, y );
    om.explored( x, y, z ) = !om.explored( x, y, z );
}

bool overmapbuffer::has_horde( const int x, const int y, const int z )
{
    for( const auto &m : overmap_buffer.monsters_at( x, y, z ) ) {
        if( m->horde ) {
            return true;
        }
    }

    return false;
}

int overmapbuffer::get_horde_size( const int x, const int y, const int z )
{
    int horde_size = 0;
    for( const auto &m : overmap_buffer.monsters_at( x, y, z ) ) {
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

bool overmapbuffer::has_vehicle( int x, int y, int z )
{
    if( z ) {
        return false;
    }

    const overmap *const om = get_existing_om_global( x, y );
    if( !om ) {
        return false;
    }

    for( const auto &v : om->vehicles ) {
        if( v.second.x == x && v.second.y == y ) {
            return true;
        }
    }

    return false;
}

std::vector<om_vehicle> overmapbuffer::get_vehicle( int x, int y, int z )
{
    std::vector<om_vehicle> result;
    if( z != 0 ) {
        return result;
    }
    overmap *om = get_existing_om_global( x, y );
    if( om == nullptr ) {
        return result;
    }
    for( const auto &ov : om->vehicles ) {
        if( ov.second.x == x && ov.second.y == y ) {
            result.push_back( ov.second );
        }
    }
    return result;
}

void overmapbuffer::signal_hordes( const tripoint &center, const int sig_power )
{
    const auto radius = sig_power;
    for( auto &om : get_overmaps_near( center, radius ) ) {
        const point abs_pos_om = om_to_sm_copy( om->pos() );
        const tripoint rel_pos( center.x - abs_pos_om.x, center.y - abs_pos_om.y, center.z );
        // overmap::signal_hordes expects a coordinate relative to the overmap, this is easier
        // for processing as the monster group stores is location as relative coordinates, too.
        om->signal_hordes( rel_pos, sig_power );
    }
}

void overmapbuffer::process_mongroups()
{
    // arbitrary radius to include nearby overmaps (aside from the current one)
    const auto radius = MAPSIZE * 2;
    const auto center = g->u.global_sm_location();
    for( auto &om : get_overmaps_near( center, radius ) ) {
        om->process_mongroups();
    }
}

void overmapbuffer::move_hordes()
{
    // arbitrary radius to include nearby overmaps (aside from the current one)
    const auto radius = MAPSIZE * 2;
    const auto center = g->u.global_sm_location();
    for( auto &om : get_overmaps_near( center, radius ) ) {
        om->move_hordes();
    }
}

std::vector<mongroup *> overmapbuffer::monsters_at( int x, int y, int z )
{
    // (x,y) are overmap terrain coordinates, they spawn 2x2 submaps,
    // but monster groups are defined with submap coordinates.
    std::vector<mongroup *> result;
    std::vector<mongroup *> tmp = groups_at( x * 2, y * 2, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2, y * 2 + 1, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2 + 1, y * 2 + 1, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2 + 1, y * 2, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    return result;
}

std::vector<mongroup *> overmapbuffer::groups_at( int x, int y, int z )
{
    std::vector<mongroup *> result;
    const point omp = sm_to_om_remain( x, y );
    if( !has( omp.x, omp.y ) ) {
        return result;
    }
    const tripoint dpos( x, y, z );
    overmap &om = get( omp.x, omp.y );
    for( auto it = om.zg.lower_bound( dpos ), end = om.zg.upper_bound( dpos ); it != end; ++it ) {
        auto &mg = it->second;
        if( mg.empty() ) {
            continue;
        }
        result.push_back( &mg );
    }
    return result;
}

std::array<std::array<scent_trace, 3>, 3> overmapbuffer::scents_near( const tripoint &origin )
{
    std::array<std::array<scent_trace, 3>, 3> found_traces;
    tripoint iter;
    int x;
    int y;

    for( x = 0, iter.x = origin.x - 1; x <= 2 ; ++iter.x, ++x ) {
        for( y = 0, iter.y = origin.y - 1; y <= 2; ++iter.y, ++y ) {
            found_traces[x][y] = scent_at( iter );
        }
    }

    return found_traces;
}

scent_trace overmapbuffer::scent_at( const tripoint &pos )
{
    overmap *found_omap = get_existing_om_global( pos );
    if( found_omap != nullptr ) {
        return found_omap->scent_at( pos );
    }
    return scent_trace();
}

void overmapbuffer::set_scent( const tripoint &loc, int strength )
{
    overmap &found_omap = get_om_global( loc );
    scent_trace new_scent( calendar::turn, strength );
    found_omap.set_scent( loc, new_scent );
}

void overmapbuffer::move_vehicle( vehicle *veh, const point &old_msp )
{
    const point new_msp = g->m.getabs( veh->global_pos3().x, veh->global_pos3().y );
    point old_omt = ms_to_omt_copy( old_msp );
    point new_omt = ms_to_omt_copy( new_msp );
    overmap &old_om = get_om_global( old_omt.x, old_omt.y );
    overmap &new_om = get_om_global( new_omt.x, new_omt.y );
    // *_omt is now local to the overmap, and it's in overmap terrain system
    if( &old_om == &new_om ) {
        new_om.vehicles[veh->om_id].x = new_omt.x;
        new_om.vehicles[veh->om_id].y = new_omt.y;
    } else {
        old_om.vehicles.erase( veh->om_id );
        add_vehicle( veh );
    }
}

void overmapbuffer::remove_vehicle( const vehicle *veh )
{
    const point omt = ms_to_omt_copy( g->m.getabs( veh->global_pos3().x, veh->global_pos3().y ) );
    overmap &om = get_om_global( omt );
    om.vehicles.erase( veh->om_id );
}

void overmapbuffer::add_vehicle( vehicle *veh )
{
    point omt = ms_to_omt_copy( g->m.getabs( veh->global_pos3().x, veh->global_pos3().y ) );
    overmap &om = get_om_global( omt.x, omt.y );
    int id = om.vehicles.size() + 1;
    // this *should* be unique but just in case
    while( om.vehicles.count( id ) > 0 ) {
        id++;
    }
    om_vehicle &tracked_veh = om.vehicles[id];
    tracked_veh.x = omt.x;
    tracked_veh.y = omt.y;
    tracked_veh.name = veh->name;
    veh->om_id = id;
}

bool overmapbuffer::seen( int x, int y, int z )
{
    const overmap *om = get_existing_om_global( x, y );
    return ( om != nullptr ) && const_cast<overmap *>( om )->seen( x, y, z );
}

void overmapbuffer::set_seen( int x, int y, int z, bool seen )
{
    overmap &om = get_om_global( x, y );
    om.seen( x, y, z ) = seen;
}

oter_id &overmapbuffer::ter( int x, int y, int z )
{
    overmap &om = get_om_global( x, y );
    return om.ter( x, y, z );
}

bool overmapbuffer::reveal( const point &center, int radius, int z )
{
    return reveal( tripoint( center, z ), radius );
}

bool overmapbuffer::reveal( const tripoint &center, int radius )
{
    bool result = false;
    for( int i = -radius; i <= radius; i++ ) {
        for( int j = -radius; j <= radius; j++ ) {
            if( !seen( center.x + i, center.y + j, center.z ) ) {
                result = true;
                set_seen( center.x + i, center.y + j, center.z, true );
            }
        }
    }
    return result;
}

bool overmapbuffer::reveal_route( const tripoint &source, const tripoint &dest, int radius,
                                  bool road_only )
{
    static const int RADIUS = 4;            // Maximal radius of search (in overmaps)
    static const int OX = RADIUS * OMAPX;   // half-width of the area to search in
    static const int OY = RADIUS * OMAPY;   // half-height of the area to search in

    if( source == overmap::invalid_tripoint || dest == overmap::invalid_tripoint ) {
        return false;
    }

    const tripoint start( OX, OY, source.z );   // Local source - center of the local area
    const tripoint base( source - start );      // To convert local coordinates to global ones
    const tripoint finish( dest - base );       // Local destination - relative to source

    const auto get_ter_at = [&]( int x, int y ) {
        x += base.x;
        y += base.y;
        const overmap &om = get_om_global( x, y );
        return om.get_ter( x, y, source.z );
    };

    const auto oter = get_ter_at( start.x, start.y ) ;
    const auto connection = overmap_connections::guess_for( oter );

    if( !connection ) {
        return false;
    }

    const auto estimate = [&]( const pf::node & cur, const pf::node * ) {
        int res = 0;

        const auto oter = get_ter_at( cur.x, cur.y );

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

        res += std::abs( finish.x - cur.x ) +
               std::abs( finish.y - cur.y );

        return res;
    };

    const auto path = pf::find_path( point( start.x, start.y ), point( finish.x, finish.y ), 2 * OX,
                                     2 * OY, estimate );

    for( const auto &node : path.nodes ) {
        reveal( base + tripoint( node.x, node.y, base.z ), radius );
    }

    return !path.nodes.empty();
}

bool overmapbuffer::check_ot_type_existing( const std::string &type, const tripoint &loc )
{
    const cata::optional<overmap_with_local_coordinates> om = get_existing_om_global_with_coordinates(
                loc );
    if( !om ) {
        return false;
    }
    return om->overmap_pointer->check_ot_type( type, om->coordinates.x, om->coordinates.y,
            om->coordinates.z );
}

bool overmapbuffer::check_ot_subtype_existing( const std::string &type, const tripoint &loc )
{
    const cata::optional<overmap_with_local_coordinates> om = get_existing_om_global_with_coordinates(
                loc );
    if( !om ) {
        return false;
    }
    return om->overmap_pointer->check_ot_subtype( type, om->coordinates.x, om->coordinates.y,
            om->coordinates.z );
}

bool overmapbuffer::check_overmap_special_type_existing( const overmap_special_id &id,
        const tripoint &loc )
{
    const cata::optional<overmap_with_local_coordinates> om = get_existing_om_global_with_coordinates(
                loc );
    if( !om ) {
        return false;
    }
    return om->overmap_pointer->check_overmap_special_type( id, om->coordinates );
}

bool overmapbuffer::check_ot_type( const std::string &type, int x, int y, int z )
{
    overmap &om = get_om_global( x, y );
    return om.check_ot_type( type, x, y, z );
}

bool overmapbuffer::check_ot_subtype( const std::string &type, int x, int y, int z )
{
    overmap &om = get_om_global( x, y );
    return om.check_ot_subtype( type, x, y, z );
}

bool overmapbuffer::check_overmap_special_type( const overmap_special_id &id, const tripoint &loc )
{
    const overmap_with_local_coordinates om = get_om_global_with_coordinates( loc );
    return om.overmap_pointer->check_overmap_special_type( id, om.coordinates );
}

bool overmapbuffer::is_findable_location( const tripoint &location, const std::string &type,
        bool must_be_seen, bool allow_subtype_matches,
        bool existing_overmaps_only, const cata::optional<overmap_special_id> &om_special )
{
    if( existing_overmaps_only ) {
        const bool type_matches = allow_subtype_matches
                                  ? check_ot_subtype_existing( type, location )
                                  : check_ot_type_existing( type, location );

        if( !type_matches ) {
            return false;
        }
    } else {
        const bool type_matches = allow_subtype_matches
                                  ? check_ot_subtype( type, location.x, location.y, location.z )
                                  : check_ot_type( type, location.x, location.y, location.z );

        if( !type_matches ) {
            return false;
        }
    }

    const bool meets_seen_criteria = !must_be_seen || seen( location.x, location.y, location.z );
    if( !meets_seen_criteria ) {
        return false;
    }

    if( existing_overmaps_only ) {
        const bool meets_om_special_criteria = !om_special ||
                                               check_overmap_special_type_existing( *om_special, location );
        if( !meets_om_special_criteria ) {
            return false;
        }
    } else {
        const bool meets_om_special_criteria = !om_special ||
                                               check_overmap_special_type( *om_special, location );
        if( !meets_om_special_criteria ) {
            return false;
        }
    }


    return true;
}

tripoint overmapbuffer::find_closest( const tripoint &origin, const std::string &type,
                                      int const radius, bool must_be_seen, bool allow_subtype_matches,
                                      bool existing_overmaps_only,
                                      const cata::optional<overmap_special_id> &om_special )
{
    // Check the origin before searching adjacent tiles!
    if( is_findable_location( origin, type, must_be_seen, allow_subtype_matches, existing_overmaps_only,
                              om_special ) ) {
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
    // See overmap::place_specials for how we attempt to insure specials are placed within this range.
    // The actual number is 5 because 1 covers the current overmap,
    // and each additional one expends the search to the next concentric circle of overmaps.

    int max = ( radius == 0 ? OMAPX * 5 : radius );
    // expanding box
    for( int dist = 0; dist <= max; dist++ ) {
        // each edge length is 2*dist-2, because corners belong to one edge
        // south is +y, north is -y
        for( int i = 0; i < dist * 2; i++ ) {
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                //start at northwest, scan north edge
                const tripoint n_loc( origin.x - dist + i, origin.y - dist, z );
                if( is_findable_location( n_loc, type, must_be_seen, allow_subtype_matches, existing_overmaps_only,
                                          om_special ) ) {
                    return n_loc;
                }

                //start at southeast, scan south
                const tripoint s_loc( origin.x + dist - i, origin.y + dist, z );
                if( is_findable_location( s_loc, type, must_be_seen, allow_subtype_matches, existing_overmaps_only,
                                          om_special ) ) {
                    return s_loc;
                }

                //start at southwest, scan west
                const tripoint w_loc( origin.x - dist, origin.y + dist - i, z );
                if( is_findable_location( w_loc, type, must_be_seen, allow_subtype_matches, existing_overmaps_only,
                                          om_special ) ) {
                    return w_loc;
                }

                //start at northeast, scan east
                const tripoint e_loc( origin.x + dist, origin.y - dist + i, z );
                if( is_findable_location( e_loc, type, must_be_seen, allow_subtype_matches, existing_overmaps_only,
                                          om_special ) ) {
                    return e_loc;
                }
            }
        }
    }
    return overmap::invalid_tripoint;
}

std::vector<tripoint> overmapbuffer::find_all( const tripoint &origin, const std::string &type,
        int dist, bool must_be_seen, bool allow_subtype_matches,
        bool existing_overmaps_only,
        const cata::optional<overmap_special_id> &om_special )
{
    std::vector<tripoint> result;
    // dist == 0 means search a whole overmap diameter.
    dist = dist ? dist : OMAPX;
    for( int x = origin.x - dist; x <= origin.x + dist; x++ ) {
        for( int y = origin.y - dist; y <= origin.y + dist; y++ ) {
            const tripoint search_loc( x, y, origin.z );
            if( is_findable_location( search_loc, type, must_be_seen, allow_subtype_matches,
                                      existing_overmaps_only, om_special ) ) {
                result.push_back( search_loc );
            }
        }
    }
    return result;
}

tripoint overmapbuffer::find_random( const tripoint &origin, const std::string &type,
                                     int dist, bool must_be_seen, bool allow_subtype_matches,
                                     bool existing_overmaps_only,
                                     const cata::optional<overmap_special_id> &om_special )
{
    return random_entry( find_all( origin, type, dist, must_be_seen, allow_subtype_matches,
                                   existing_overmaps_only,
                                   om_special ), overmap::invalid_tripoint );
}

std::shared_ptr<npc> overmapbuffer::find_npc( int id )
{
    for( auto &it : overmaps ) {
        if( auto p = it.second->find_npc( id ) ) {
            return p;
        }
    }
    return nullptr;
}

void overmapbuffer::insert_npc( const std::shared_ptr<npc> &who )
{
    assert( who );
    const tripoint npc_omt_pos = who->global_omt_location();
    const point npc_om_pos = omt_to_om_copy( npc_omt_pos.x, npc_omt_pos.y );
    get( npc_om_pos.x, npc_om_pos.y ).insert_npc( who );
}

std::shared_ptr<npc> overmapbuffer::remove_npc( const int id )
{
    for( auto &it : overmaps ) {
        if( const auto p = it.second->erase_npc( id ) ) {
            return p;
        }
    }
    debugmsg( "overmapbuffer::remove_npc: NPC (%d) not found.", id );
    return nullptr;
}

std::vector<std::shared_ptr<npc>> overmapbuffer::get_npcs_near_player( int radius )
{
    tripoint plpos = g->u.global_omt_location();
    // get_npcs_near needs submap coordinates
    omt_to_sm( plpos.x, plpos.y );
    // INT_MIN is a (a bit ugly) way to inform get_npcs_near not to filter by z-level
    const int zpos = g->m.has_zlevels() ? INT_MIN : plpos.z;
    return get_npcs_near( plpos.x, plpos.y, zpos, radius );
}

std::vector<overmap *> overmapbuffer::get_overmaps_near( const tripoint &location,
        const int radius )
{
    // Grab the corners of a square around the target location at distance radius.
    // Convert to overmap coordinates and iterate from the minimum to the maximum.
    const point start = sm_to_om_copy( location.x - radius, location.y - radius );
    const point end = sm_to_om_copy( location.x + radius, location.y + radius );
    const point offset = end - start;

    std::vector<overmap *> result;
    result.reserve( ( offset.x + 1 ) * ( offset.y + 1 ) );

    for( int x = start.x; x <= end.x; ++x ) {
        for( int y = start.y; y <= end.y; ++y ) {
            if( const auto existing_om = get_existing( x, y ) ) {
                result.emplace_back( existing_om );
            }
        }
    }

    // Sort the resulting overmaps so that the closest ones are first.
    const tripoint center = sm_to_om_copy( location );
    std::sort( result.begin(), result.end(), [&center]( const overmap * lhs,
    const overmap * rhs ) {
        const tripoint lhs_pos( lhs->pos(), 0 );
        const tripoint rhs_pos( rhs->pos(), 0 );
        return trig_dist( center, lhs_pos ) < trig_dist( center, rhs_pos );
    } );

    return result;
}

std::vector<overmap *> overmapbuffer::get_overmaps_near( const point &p, const int radius )
{
    return get_overmaps_near( tripoint( p.x, p.y, 0 ), radius );
}

std::vector<std::shared_ptr<npc>> overmapbuffer::get_companion_mission_npcs()
{
    std::vector<std::shared_ptr<npc>> available;
    //@todo: this is an arbitrary radius, replace with something sane.
    for( const auto &guy : get_npcs_near_player( 100 ) ) {
        if( guy->has_companion_mission() ) {
            available.push_back( guy );
        }
    }
    return available;
}

// If z == INT_MIN, allow all z-levels
std::vector<std::shared_ptr<npc>> overmapbuffer::get_npcs_near( int x, int y, int z, int radius )
{
    std::vector<std::shared_ptr<npc>> result;
    tripoint p{ x, y, z };
    for( auto &it : get_overmaps_near( p, radius ) ) {
        auto temp = it->get_npcs( [&]( const npc & guy ) {
            // Global position of NPC, in submap coordinates
            const tripoint pos = guy.global_sm_location();
            if( z != INT_MIN && pos.z != z ) {
                return false;
            }
            return square_dist( x, y, pos.x, pos.y ) <= radius;
        } );
        result.insert( result.end(), temp.begin(), temp.end() );
    }
    return result;
}

// If z == INT_MIN, allow all z-levels
std::vector<std::shared_ptr<npc>> overmapbuffer::get_npcs_near_omt( int x, int y, int z,
                               int radius )
{
    std::vector<std::shared_ptr<npc>> result;
    for( auto &it : get_overmaps_near( omt_to_sm_copy( x, y ), radius ) ) {
        auto temp = it->get_npcs( [&]( const npc & guy ) {
            // Global position of NPC, in submap coordinates
            tripoint pos = guy.global_omt_location();
            if( z != INT_MIN && pos.z != z ) {
                return false;
            }
            return square_dist( x, y, pos.x, pos.y ) <= radius;
        } );
        result.insert( result.end(), temp.begin(), temp.end() );
    }
    return result;
}

radio_tower_reference create_radio_tower_reference( overmap &om, radio_tower &t,
        const tripoint &center )
{
    // global submap coordinates, same as center is
    const point pos = point( t.x, t.y ) + om_to_sm_copy( om.pos() );
    const int strength = t.strength - rl_dist( tripoint( pos, 0 ), center );
    return radio_tower_reference{ &t, pos, strength };
}

radio_tower_reference overmapbuffer::find_radio_station( const int frequency )
{
    const auto center = g->u.global_sm_location();
    for( auto &om : get_overmaps_near( center, RADIO_MAX_STRENGTH ) ) {
        for( auto &tower : om->radios ) {
            const auto rref = create_radio_tower_reference( *om, tower, center );
            if( rref.signal_strength > 0 && tower.frequency == frequency ) {
                return rref;
            }
        }
    }
    return radio_tower_reference{ nullptr, point_zero, 0 };
}

std::vector<radio_tower_reference> overmapbuffer::find_all_radio_stations()
{
    std::vector<radio_tower_reference> result;
    const auto center = g->u.global_sm_location();
    // perceived signal strength is distance (in submaps) - signal strength, so towers
    // further than RADIO_MAX_STRENGTH submaps away can never be received at all.
    const int radius = RADIO_MAX_STRENGTH;
    for( auto &om : get_overmaps_near( center, radius ) ) {
        for( auto &tower : om->radios ) {
            const auto rref = create_radio_tower_reference( *om, tower, center );
            if( rref.signal_strength > 0 ) {
                result.push_back( rref );
            }
        }
    }
    return result;
}

std::vector<city_reference> overmapbuffer::get_cities_near( const tripoint &location, int radius )
{
    std::vector<city_reference> result;

    for( const auto om : get_overmaps_near( location, radius ) ) {
        const auto abs_pos_om = om_to_sm_copy( om->pos() );
        result.reserve( result.size() + om->cities.size() );
        std::transform( om->cities.begin(), om->cities.end(), std::back_inserter( result ),
        [&]( city & element ) {
            const auto rel_pos_city = omt_to_sm_copy( element.pos );
            const auto abs_pos_city = tripoint( rel_pos_city + abs_pos_om, 0 );
            const auto distance = rl_dist( abs_pos_city, location );

            return city_reference{ &element, abs_pos_city, distance };
        } );
    }

    std::sort( result.begin(), result.end(), []( const city_reference & lhs,
    const city_reference & rhs ) {
        return lhs.get_distance_from_bounds() < rhs.get_distance_from_bounds();
    } );

    return result;
}

city_reference overmapbuffer::closest_city( const tripoint &center )
{
    const auto cities = get_cities_near( center, omt_to_sm_copy( OMAPX ) );

    if( !cities.empty() ) {
        return cities.front();
    }

    return city_reference::invalid;
}

city_reference overmapbuffer::closest_known_city( const tripoint &center )
{
    const auto cities = get_cities_near( center, omt_to_sm_copy( OMAPX ) );
    const auto it = std::find_if( cities.begin(), cities.end(),
    [this]( const city_reference & elem ) {
        const tripoint p = sm_to_omt_copy( elem.abs_sm_pos );
        return seen( p.x, p.y, p.z );
    } );

    if( it != cities.end() ) {
        return *it;
    }

    return city_reference::invalid;
}

std::string overmapbuffer::get_description_at( const tripoint &where )
{
    const std::string ter_name = ter( sm_to_omt_copy( where ) )->get_name();

    if( where.z != 0 ) {
        return ter_name;
    }

    const auto closest_cref = closest_known_city( where );

    if( !closest_cref ) {
        return ter_name;
    }

    const auto &closest_city = *closest_cref.city;
    const direction dir = direction_from( closest_cref.abs_sm_pos, where );
    const std::string dir_name = direction_name( dir );

    const int sm_size = omt_to_sm_copy( closest_cref.city->size );
    const int sm_dist = closest_cref.distance;

    if( sm_dist <= 3 * sm_size / 4 ) {
        if( sm_size >= 16 ) {
            // The city is big enough to be split in districts.
            if( sm_dist <= sm_size / 4 ) {
                //~ First parameter is a terrain name, second parameter is a city name.
                return string_format( _( "%1$s in central %2$s" ), ter_name.c_str(), closest_city.name.c_str() );
            } else {
                //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
                return string_format( _( "%1$s in %2$s %3$s" ), ter_name.c_str(), dir_name.c_str(),
                                      closest_city.name.c_str() );
            }
        } else {
            //~ First parameter is a terrain name, second parameter is a city name.
            return string_format( _( "%1$s in %2$s" ), ter_name.c_str(), closest_city.name.c_str() );
        }
    } else if( sm_dist <= sm_size ) {
        if( sm_size >= 8 ) {
            // The city is big enough to have outskirts.
            //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
            return string_format( _( "%1$s on the %2$s outskirts of %3$s" ), ter_name.c_str(), dir_name.c_str(),
                                  closest_city.name.c_str() );
        } else {
            //~ First parameter is a terrain name, second parameter is a city name.
            return string_format( _( "%1$s in %2$s" ), ter_name.c_str(), closest_city.name.c_str() );
        }
    }

    //~ First parameter is a terrain name, second parameter is a direction, and third parameter is a city name.
    return string_format( _( "%1$s %2$s from %3$s" ), ter_name.c_str(), dir_name.c_str(),
                          closest_city.name.c_str() );
}

static int modulo( int v, int m )
{
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

void overmapbuffer::spawn_monster( const int x, const int y, const int z )
{
    // Create a copy, so we can reuse x and y later
    point sm( x, y );
    const point omp = sm_to_om_remain( sm );
    overmap &om = get( omp.x, omp.y );
    const tripoint current_submap_loc( sm.x, sm.y, z );
    auto monster_bucket = om.monster_map.equal_range( current_submap_loc );
    std::for_each( monster_bucket.first, monster_bucket.second,
    [&]( std::pair<const tripoint, monster> &monster_entry ) {
        monster &this_monster = monster_entry.second;
        // The absolute position in map squares, (x,y) is already global, but it's a
        // submap coordinate, so translate it and add the exact monster position on
        // the submap. modulo because the zombies position might be negative, as it
        // is stored *after* it has gone out of bounds during shifting. When reloading
        // we only need the part that tells where on the submap to put it.
        point ms( modulo( this_monster.posx(), SEEX ), modulo( this_monster.posy(), SEEY ) );
        assert( ms.x >= 0 && ms.x < SEEX );
        assert( ms.y >= 0 && ms.y < SEEX );
        ms.x += x * SEEX;
        ms.y += y * SEEY;
        // The monster position must be local to the main map when added via game::add_zombie
        const tripoint local = tripoint( g->m.getlocal( ms.x, ms.y ), z );
        assert( g->m.inbounds( local ) );
        this_monster.spawn( local );
        g->add_zombie( this_monster );
    } );
    om.monster_map.erase( current_submap_loc );
}

void overmapbuffer::despawn_monster( const monster &critter )
{
    // Get absolute coordinates of the monster in map squares, translate to submap position
    tripoint sm = ms_to_sm_copy( g->m.getabs( critter.pos() ) );
    // Get the overmap coordinates and get the overmap, sm is now local to that overmap
    const point omp = sm_to_om_remain( sm.x, sm.y );
    overmap &om = get( omp.x, omp.y );
    // Store the monster using coordinates local to the overmap.
    om.monster_map.insert( std::make_pair( sm, critter ) );
}

overmapbuffer::t_notes_vector overmapbuffer::get_notes( int z, const std::string *pattern )
{
    t_notes_vector result;
    for( auto &it : overmaps ) {
        const overmap &om = *it.second;
        const int offset_x = om.pos().x * OMAPX;
        const int offset_y = om.pos().y * OMAPY;
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                const std::string &note = om.note( i, j, z );
                if( note.empty() ) {
                    continue;
                }
                if( pattern != nullptr && lcmatch( note, *pattern ) ) {
                    // pattern not found in note text
                    continue;
                }
                result.push_back( t_point_with_note(
                                      point( offset_x + i, offset_y + j ),
                                      om.note( i, j, z )
                                  ) );
            }
        }
    }
    return result;
}

bool overmapbuffer::is_safe( int x, int y, int z )
{
    for( auto &mongrp : monsters_at( x, y, z ) ) {
        if( !mongrp->is_safe() ) {
            return false;
        }
    }
    return true;
}

bool overmapbuffer::place_special( const overmap_special &special, const tripoint &p,
                                   om_direction::type dir, const bool must_be_unexplored, const bool force )
{
    // get_om_global will transform these into overmap local coordinates, which
    // we'll then use with the overmap methods.
    int x = p.x;
    int y = p.y;
    overmap &om = get_om_global( x, y );
    const tripoint om_loc( x, y, p.z );

    // Get the closest city that is within the overmap because
    // all of the overmap generation functions only function within
    // the single overmap. If future generation is hoisted up to the
    // buffer to spawn overmaps, then this can also be changed accordingly.
    const city c = om.get_nearest_city( om_loc );

    bool placed = false;
    // Only place this special if we can actually place it per its criteria, or we're forcing
    // the placement, which is mostly a debug behavior, since a forced placement may not function
    // correctly (e.g. won't check correct underlying terrain).
    if( om.can_place_special( special, om_loc, dir, must_be_unexplored ) || force ) {
        om.place_special( special, om_loc, dir, c, must_be_unexplored, force );
        placed = true;
    }
    return placed;
}

bool overmapbuffer::place_special( const overmap_special_id &special_id, const tripoint &center,
                                   int radius )
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
    for( const auto &om : get_overmaps_near( omt_to_sm_copy( center ), omt_to_sm_copy( radius ) ) ) {

        // Build an overmap_special_batch for the special on this overmap.
        std::vector<const overmap_special *> specials;
        specials.push_back( &special );
        overmap_special_batch batch( om->pos(), specials );

        // Attempt to place the specials using our batch and sectors. We
        // require they be placed in unexplored terrain right now.
        om->place_specials_pass( batch, sectors, true, true );

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
