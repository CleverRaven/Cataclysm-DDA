#include "overmapbuffer.h"
#include "coordinate_conversions.h"
#include "overmap_types.h"
#include "overmap.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "monster.h"
#include "mongroup.h"
#include "simple_pathfinding.h"
#include "worldfactory.h"
#include "catacharset.h"
#include "npc.h"
#include "vehicle.h"
#include "filesystem.h"
#include "cata_utility.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdlib.h>

overmapbuffer overmap_buffer;

overmapbuffer::overmapbuffer()
: last_requested_overmap( nullptr )
{
}

std::string overmapbuffer::terrain_filename(int const x, int const y)
{
    std::ostringstream filename;

    filename << world_generator->active_world->world_path << "/";
    filename << "o." << x << "." << y;

    return filename.str();
}

std::string overmapbuffer::player_filename(int const x, int const y)
{
    std::ostringstream filename;

    filename << world_generator->active_world->world_path << "/" << base64_encode(
                 g->u.name) << ".seen." << x << "." << y;

    return filename.str();
}

overmap &overmapbuffer::get( const int x, const int y )
{
    point const p {x, y};

    if (last_requested_overmap && last_requested_overmap->pos() == p) {
        return *last_requested_overmap;
    }

    auto const it = overmaps.find( p );
    if( it != overmaps.end() ) {
        return *(last_requested_overmap = it->second.get());
    }

    // That constructor loads an existing overmap or creates a new one.
    std::unique_ptr<overmap> new_om( new overmap( x, y ) );
    overmap &result = *new_om;
    overmaps[ new_om->pos() ] = std::move( new_om );
    // Note: fix_mongroups might load other overmaps, so overmaps.back() is not
    // necessarily the overmap at (x,y)
    fix_mongroups( result );

    last_requested_overmap = &result;
    return result;
}

void overmapbuffer::fix_mongroups(overmap &new_overmap)
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
    last_requested_overmap = NULL;
}

const regional_settings& overmapbuffer::get_settings(int x, int y, int z)
{
    (void)z;
    overmap &om = get_om_global(x, y);
    return om.get_settings();
}

void overmapbuffer::add_note(int x, int y, int z, const std::string& message)
{
    overmap &om = get_om_global(x, y);
    om.add_note(x, y, z, message);
}

void overmapbuffer::delete_note(int x, int y, int z)
{
    if (has_note(x, y, z)) {
        overmap &om = get_om_global(x, y);
        om.delete_note(x, y, z);
    }
}

overmap *overmapbuffer::get_existing(int x, int y)
{
    point const p {x, y};

    if( last_requested_overmap && last_requested_overmap->pos() == p ) {
        return last_requested_overmap;
    }
    auto const it = overmaps.find( p );
    if( it != overmaps.end() ) {
        return last_requested_overmap = it->second.get();
    }
    if (known_non_existing.count(p) > 0) {
        // This overmap does not exist on disk (this has already been
        // checked in a previous call of this function).
        return NULL;
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
    known_non_existing.insert(p);
    return NULL;
}

bool overmapbuffer::has( int x, int y )
{
    return get_existing( x, y ) != NULL;
}

overmap &overmapbuffer::get_om_global( int &x, int &y )
{
    const point om_pos = omt_to_om_remain( x, y );
    return get( om_pos.x, om_pos.y );
}

overmap &overmapbuffer::get_om_global( const point& p )
{
    const point om_pos = omt_to_om_copy( p );
    return get( om_pos.x, om_pos.y );
}

overmap &overmapbuffer::get_om_global( const tripoint& p )
{
    const point om_pos = omt_to_om_copy( { p.x, p.y } );
    return get( om_pos.x, om_pos.y );
}

overmap *overmapbuffer::get_existing_om_global( int &x, int &y )
{
    const point om_pos = omt_to_om_remain( x, y );
    return get_existing( om_pos.x, om_pos.y );
}

overmap *overmapbuffer::get_existing_om_global( const point& p )
{
    const point om_pos = omt_to_om_copy( p );
    return get_existing( om_pos.x, om_pos.y );
}

overmap *overmapbuffer::get_existing_om_global( const tripoint& p )
{
    const tripoint om_pos = omt_to_om_copy( p );
    return get_existing( om_pos.x, om_pos.y );
}

bool overmapbuffer::has_note(int x, int y, int z)
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && om->has_note(x, y, z);
}

const std::string& overmapbuffer::note(int x, int y, int z)
{
    const overmap *om = get_existing_om_global(x, y);
    if (om == NULL) {
        static const std::string empty_string;
        return empty_string;
    }
    return om->note(x, y, z);
}

bool overmapbuffer::is_explored(int x, int y, int z)
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && om->is_explored(x, y, z);
}

void overmapbuffer::toggle_explored(int x, int y, int z)
{
    overmap &om = get_om_global(x, y);
    om.explored(x, y, z) = !om.explored(x, y, z);
}

bool overmapbuffer::has_horde(int const x, int const y, int const z) {
    for (auto const &m : overmap_buffer.monsters_at(x, y, z)) {
        if (m->horde) {
            return true;
        }
    }

    return false;
}

bool overmapbuffer::has_vehicle( int x, int y, int z )
{
    if (z) {
        return false;
    }

    overmap const *const om = get_existing_om_global(x, y);
    if (!om) {
        return false;
    }

    for (auto const &v : om->vehicles) {
        if (v.second.x == x && v.second.y == y) {
            return true;
        }
    }

    return false;;
}

std::vector<om_vehicle> overmapbuffer::get_vehicle( int x, int y, int z )
{
    std::vector<om_vehicle> result;
    if( z != 0 ) {
        return result;
    }
    overmap *om = get_existing_om_global(x, y);
    if( om == nullptr ) {
        return result;
    }
    for( const auto &ov : om->vehicles ) {
        if ( ov.second.x == x && ov.second.y == y ) {
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

std::vector<mongroup*> overmapbuffer::monsters_at(int x, int y, int z)
{
    // (x,y) are overmap terrain coordinates, they spawn 2x2 submaps,
    // but monster groups are defined with submap coordinates.
    std::vector<mongroup *> result, tmp;
    tmp = groups_at( x * 2, y * 2 , z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2, y * 2 + 1, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2 + 1, y * 2 + 1, z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    tmp = groups_at( x * 2 + 1, y * 2 , z );
    result.insert( result.end(), tmp.begin(), tmp.end() );
    return result;
}

std::vector<mongroup*> overmapbuffer::groups_at(int x, int y, int z)
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
    const point new_msp = veh->real_global_pos();
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
    const point omt = ms_to_omt_copy( veh->real_global_pos() );
    overmap &om = get_om_global( omt );
    om.vehicles.erase( veh->om_id );
}

void overmapbuffer::add_vehicle( vehicle *veh )
{
    point omt = ms_to_omt_copy( veh->real_global_pos() );
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

bool overmapbuffer::seen(int x, int y, int z)
{
    const overmap *om = get_existing_om_global(x, y);
    return (om != NULL) && const_cast<overmap*>(om)->seen(x, y, z);
}

void overmapbuffer::set_seen(int x, int y, int z, bool seen)
{
    overmap &om = get_om_global(x, y);
    om.seen(x, y, z) = seen;
}

oter_id& overmapbuffer::ter(int x, int y, int z) {
    overmap &om = get_om_global(x, y);
    return om.ter(x, y, z);
}

bool overmapbuffer::reveal(const point &center, int radius, int z)
{
    return reveal( tripoint( center, z ), radius );
}

bool overmapbuffer::reveal( const tripoint &center, int radius )
{
    bool result = false;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if(!seen(center.x + i, center.y + j, center.z)) {
                result = true;
                set_seen(center.x + i, center.y + j, center.z, true);
            }
        }
    }
    return result;
}

bool overmapbuffer::reveal_route( const tripoint &source, const tripoint &dest, int radius )
{
    static const int RADIUS = 4;            // Maximal radius of search (in overmaps)
    static const int OX = RADIUS * OMAPX;   // half-width of the area to search in
    static const int OY = RADIUS * OMAPY;   // half-height of the area to search in

    const tripoint start( OX, OY, source.z );   // Local source - center of the local area
    const tripoint base( source - start );      // To convert local coordinates to global ones
    const tripoint finish( dest - base );       // Local destination - relative to source

    const auto estimate = [ this, &base, &finish ]( const pf::node &, const pf::node &cur ) {
        int res = 0;
        int omx = base.x + cur.x;
        int omy = base.y + cur.y;

        const auto &oter = get_om_global( omx, omy ).get_ter( omx, omy, base.z );

        if( !is_ot_type( "road", oter ) && !is_ot_type ( "bridge", oter ) && !is_ot_type( "hiway", oter ) ) {
            if( is_river( oter ) ) {
                return -1; // Can't walk on water
            }
            // Allow going slightly off-road to overcome small obstacles (e.g. craters),
            // but heavily penalize that to make roads preferable
            res += 250;
        }

        res += std::abs( finish.x - cur.x ) +
               std::abs( finish.y - cur.y );

        return res;
    };

    const auto path = pf::find_path( point( start.x, start.y ), point( finish.x, finish.y ), 2*OX, 2*OY, estimate );

    if( path.empty() ) {
        return false;
    }

    for( const auto &node : path ) {
        reveal( base + tripoint( node.x, node.y, base.z ), radius );
    }

    return true;
}

bool overmapbuffer::check_ot_type(const std::string& type, int x, int y, int z)
{
    overmap& om = get_om_global(x, y);
    return om.check_ot_type(type, x, y, z);
}

tripoint overmapbuffer::find_closest(const tripoint& origin, const std::string& type, int const radius, bool must_be_seen)
{
    int max = (radius == 0 ? OMAPX : radius);
    const int z = origin.z;
    // expanding box
    for( int dist = 0; dist <= max; dist++) {
        // each edge length is 2*dist-2, because corners belong to one edge
        // south is +y, north is -y
        for (int i = 0; i < dist*2-1; i++) {
            //start at northwest, scan north edge
            int x = origin.x - dist + i;
            int y = origin.y - dist;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return tripoint( x, y, z );
                }
            }

            //start at southeast, scan south
            x = origin.x + dist - i;
            y = origin.y + dist;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return tripoint( x, y, z );
                }
            }

            //start at southwest, scan west
            x = origin.x - dist;
            y = origin.y + dist - i;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return tripoint( x, y, z );
                }
            }

            //start at northeast, scan east
            x = origin.x + dist;
            y = origin.y - dist + i;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return tripoint( x, y, z );
                }
            }
        }
    }
    return overmap::invalid_tripoint;
}

std::vector<tripoint> overmapbuffer::find_all( const tripoint& origin, const std::string& type,
                                               int dist, bool must_be_seen )
{
    std::vector<tripoint> result;
    // dist == 0 means search a whole overmap diameter.
    dist = dist ? dist : OMAPX;
    for (int x = origin.x - dist; x <= origin.x + dist; x++) {
        for (int y = origin.y - dist; y <= origin.y + dist; y++) {
            if (must_be_seen && !seen(x, y, origin.z)) {
                continue;
            }
            if (check_ot_type(type, x, y, origin.z)) {
                result.push_back( tripoint( x, y, origin.z ) );
            }
        }
    }
    return result;
}

tripoint overmapbuffer::find_random( const tripoint &origin, const std::string &type,
                                     int dist, bool must_be_seen )
{
    return random_entry( find_all( origin, type, dist, must_be_seen ), overmap::invalid_tripoint );
}

npc* overmapbuffer::find_npc(int id) {
    for( auto &it : overmaps ) {
        for( auto &elem : it.second->npcs ) {
            if( elem->getID() == id ) {
                return elem;
            }
        }
    }
    return NULL;
}

void overmapbuffer::remove_npc(int id)
{
    for( auto &it : overmaps ) {
        for (size_t i = 0; i < it.second->npcs.size(); i++) {
            npc *p = it.second->npcs[i];
            if (p->getID() == id) {
                if( !p->is_dead() ) {
                    debugmsg("overmapbuffer::remove_npc: NPC (%d) is not dead.", id);
                }
                it.second->npcs.erase(it.second->npcs.begin() + i);
                delete p;
                return;
            }
        }
    }
    debugmsg("overmapbuffer::remove_npc: NPC (%d) not found.", id);
}

void overmapbuffer::hide_npc(int id)
{
    for( auto &it : overmaps ) {
        for (size_t i = 0; i < it.second->npcs.size(); i++) {
            npc *p = it.second->npcs[i];
            if (p->getID() == id) {
                it.second->npcs.erase(it.second->npcs.begin() + i);
                return;
            }
        }
    }
    debugmsg("overmapbuffer::hide_npc: NPC (%d) not found.", id);
}

std::vector<npc*> overmapbuffer::get_npcs_near_player(int radius)
{
    tripoint plpos = g->u.global_omt_location();
    // get_npcs_near needs submap coordinates
    omt_to_sm(plpos.x, plpos.y);
    // INT_MIN is a (a bit ugly) way to inform get_npcs_near not to filter by z-level
    const int zpos = g->m.has_zlevels() ? INT_MIN : plpos.z;
    return get_npcs_near( plpos.x, plpos.y, zpos, radius );
}

std::vector<overmap*> overmapbuffer::get_overmaps_near( tripoint const &location, int const radius )
{
    // Grab the corners of a square around the target location at distance radius.
    // Convert to overmap coordinates and iterate from the minimum to the maximum.
    point const start = sm_to_om_copy(location.x - radius, location.y - radius);
    point const end = sm_to_om_copy(location.x + radius, location.y + radius);
    point const offset = end - start;

    std::vector<overmap*> result;
    result.reserve( ( offset.x + 1 ) * ( offset.y + 1 ) );

    for (int x = start.x; x <= end.x; ++x) {
        for (int y = start.y; y <= end.y; ++y) {
            if (auto const existing_om = get_existing(x, y)) {
                result.emplace_back(existing_om);
            }
        }
    }

    return result;
}

std::vector<overmap *> overmapbuffer::get_overmaps_near( const point &p, const int radius )
{
    return get_overmaps_near( tripoint( p.x, p.y, 0 ), radius );
}

// If z == INT_MIN, allow all z-levels
std::vector<npc*> overmapbuffer::get_npcs_near(int x, int y, int z, int radius)
{
    std::vector<npc*> result;
    tripoint p{ x, y, z };
    for( auto &it : get_overmaps_near( p, radius ) ) {
        for( auto &np : it->npcs ) {
            // Global position of NPC, in submap coordiantes
            const tripoint pos = np->global_sm_location();
            if( z != INT_MIN && pos.z != z ) {
                continue;
            }
            const int npc_offset = square_dist( x, y, pos.x, pos.y );
            if (npc_offset <= radius) {
                result.push_back( np );
            }
        }
    }
    return result;
}

// If z == INT_MIN, allow all z-levels
std::vector<npc*> overmapbuffer::get_npcs_near_omt(int x, int y, int z, int radius)
{
    std::vector<npc*> result;
    for( auto &it : get_overmaps_near( omt_to_sm_copy( x, y ), radius ) ) {
        for( auto &np : it->npcs ) {
            // Global position of NPC, in submap coordiantes
            tripoint pos = np->global_omt_location();
            if( z != INT_MIN && pos.z != z) {
                continue;
            }
            const int npc_offset = square_dist( x, y, pos.x, pos.y );
            if (npc_offset <= radius) {
                result.push_back(np);
            }
        }
    }
    return result;
}

radio_tower_reference create_radio_tower_reference( overmap &om, radio_tower &t, const tripoint &center )
{
    // global submap coordinates, same as center is
    const point pos = point( t.x, t.y ) + om_to_sm_copy( om.pos() );
    const int strength = t.strength - rl_dist( tripoint( pos, 0 ), center );
    return radio_tower_reference{ &om, &t, pos, strength };
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
    return radio_tower_reference{ nullptr, nullptr, point( 0, 0 ), 0 };
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

city_reference overmapbuffer::closest_city( const tripoint &center )
{
    // a whole overmap (because it's in submap coordinates, OMAPX is overmap terrain coordinates)
    auto const radius = OMAPX * 2;
    // Starting with distance = INT_MAX, so the first city is already closer
    city_reference result{ nullptr, nullptr, tripoint( 0, 0, 0 ), INT_MAX };
    for( auto &om : get_overmaps_near( center, radius ) ) {
        const auto abs_pos_om = om_to_sm_copy( om->pos() );
        for( auto &city : om->cities ) {
            const auto rel_pos_city = omt_to_sm_copy( point( city.x, city.y ) );
            // TODO: Z-level cities. This 0 has to be here until mapgen understands non-0 zlev cities
            const auto abs_pos_city = tripoint( abs_pos_om + rel_pos_city, 0 );
            const auto distance = rl_dist( abs_pos_city, center );
            const city_reference cr{ om, &city, abs_pos_city, distance };
            if( distance < result.distance ) {
                result = cr;
            } else if( distance == result.distance && result.city->s < city.s ) {
                result = cr;
            }
        }
    }
    return result;
}

static int modulo(int v, int m) {
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

void overmapbuffer::spawn_monster(const int x, const int y, const int z)
{
    // Create a copy, so we can reuse x and y later
    point sm( x, y );
    const point omp = sm_to_om_remain( sm );
    overmap &om = get( omp.x, omp.y );
    const tripoint current_submap_loc( sm.x, sm.y, z );
    auto monster_bucket = om.monster_map.equal_range( current_submap_loc );
    std::for_each( monster_bucket.first, monster_bucket.second,
                   [&](std::pair<const tripoint, monster> &monster_entry ) {
        monster &this_monster = monster_entry.second;
        // The absolute position in map squares, (x,y) is already global, but it's a
        // submap coordinate, so translate it and add the exact monster position on
        // the submap. modulo because the zombies position might be negative, as it
        // is stored *after* it has gone out of bounds during shifting. When reloading
        // we only need the part that tells where on the sumap to put it.
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

void overmapbuffer::despawn_monster(const monster &critter)
{
    // Get absolute coordinates of the monster in map squares, translate to submap position
    tripoint sm = ms_to_sm_copy( g->m.getabs( critter.pos() ) );
    // Get the overmap coordinates and get the overmap, sm is now local to that overmap
    const point omp = sm_to_om_remain( sm.x, sm.y );
    overmap &om = get( omp.x, omp.y );
    // Store the monster using coordinates local to the overmap.
    om.monster_map.insert( std::make_pair( sm, critter ) );
}

overmapbuffer::t_notes_vector overmapbuffer::get_notes(int z, const std::string* pattern)
{
    t_notes_vector result;
    for( auto &it : overmaps ) {
        const overmap &om = *it.second;
        const int offset_x = om.pos().x * OMAPX;
        const int offset_y = om.pos().y * OMAPY;
        for (int i = 0; i < OMAPX; i++) {
            for (int j = 0; j < OMAPY; j++) {
                const std::string &note = om.note(i, j, z);
                if (note.empty()) {
                    continue;
                }
                if (pattern != NULL && lcmatch( note, *pattern ) ) {
                    // pattern not found in note text
                    continue;
                }
                result.push_back(t_point_with_note(
                    point(offset_x + i, offset_y + j),
                    om.note(i, j, z)
                ));
            }
        }
    }
    return result;
}

bool overmapbuffer::is_safe(int x, int y, int z)
{
    for( auto & mongrp : monsters_at( x, y, z ) ) {
        if( !mongrp->is_safe() ) {
            return false;
        }
    }
    return true;
}
