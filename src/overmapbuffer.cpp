#include <stdlib.h>

#include "overmapbuffer.h"
#include "game.h"
#include "monster.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>

overmapbuffer overmap_buffer;

/** Mathematical modulo (only for positive m): 0 <= result < m */
inline int modulo(int v, int m);
inline int divide(int v, int m);
inline int divide(int v, int m, int &r);

overmapbuffer::overmapbuffer()
: last_requested_overmap( nullptr )
{
}

std::string overmapbuffer::terrain_filename(int const x, int const y)
{
    std::stringstream filename;

    filename << world_generator->active_world->world_path << "/";
    filename << "o." << x << "." << y;

    return filename.str();
}

std::string overmapbuffer::player_filename(int const x, int const y)
{
    std::stringstream filename;

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
        if( mg.population <= 0 ) {
            new_overmap.zg.erase( it++ );
            continue;
        }
        // Inside the bounds of the overmap?
        if( mg.posx >= 0 && mg.posy >= 0 && mg.posx < OMAPX * 2 && mg.posy < OMAPY * 2 ) {
            ++it;
            continue;
        }
        point smabs( mg.posx + new_overmap.pos().x * OMAPX * 2,
                     mg.posy + new_overmap.pos().y * OMAPY * 2 );
        point omp = sm_to_om_remain( smabs );
        if( !has( omp.x, omp.y ) ) {
            // Don't generate new overmaps, as this can be called from the
            // overmap-generating code.
            ++it;
            continue;
        }
        overmap &om = get( omp.x, omp.y );
        mg.posx = smabs.x;
        mg.posy = smabs.y;
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
    overmap &om = get_om_global(x, y);
    return om.get_settings(x, y, z);
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
    // Check if the overmap exist on disk,
    std::ifstream tmp(terrain_filename( x, y ).c_str(), std::ios::in);
    if(tmp.is_open()) {
        // File exists, load it normally (the get function
        // indirectly call overmap::open to do so).
        tmp.close();
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

bool overmapbuffer::has(int x, int y)
{
    return get_existing(x, y) != NULL;
}

overmap *overmapbuffer::get_existing_om_global(int &x, int &y)
{
    const point om_pos = omt_to_om_remain(x, y);
    return get_existing(om_pos.x, om_pos.y);
}

overmap &overmapbuffer::get_om_global(int &x, int &y)
{
    const point om_pos = omt_to_om_remain(x, y);
    return get(om_pos.x, om_pos.y);
}

overmap *overmapbuffer::get_existing_om_global(const point& p)
{
    const point om_pos = omt_to_om_copy(p);
    return get_existing(om_pos.x, om_pos.y);
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

bool overmapbuffer::has_npc(int x, int y, int const z)
{
    auto const om = get_existing_om_global(x, y);
    if (!om) {
        return false;
    }

    for (auto const &npc : om->npcs) {
        if (npc->global_omt_location() == tripoint(x, y, z)) {
            return true;
        }
    }

    return false;
}

bool overmapbuffer::has_vehicle(int x, int y, int z, bool require_pda)
{
    if (z) {
        return false;
    }

    // if the player is not carrying a PDA then he cannot see the vehicle.
    if (require_pda && !g->u.has_pda()) {
        return false;
    }

    auto const om = get_existing_om_global(x, y);
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

std::vector<om_vehicle> overmapbuffer::get_vehicle(int x, int y, int z, bool require_pda)
{
    std::vector<om_vehicle> result;
    if( z != 0 ) {
        return result;
    }
    // if the player is not carrying a PDA then he cannot see the vehicle.
    if( require_pda && !g->u.has_pda() ) {
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
        if( mg.population <= 0 ) {
            continue;
        }
        result.push_back( &mg );
    }
    return result;
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

overmap &overmapbuffer::get_om_global(const point& p)
{
    const point om_pos = omt_to_om_copy(p);
    return get(om_pos.x, om_pos.y);
}

oter_id& overmapbuffer::ter(int x, int y, int z) {
    overmap &om = get_om_global(x, y);
    return om.ter(x, y, z);
}

bool overmapbuffer::reveal(const point &center, int radius, int z)
{
    bool result = false;
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            if(!seen(center.x + i, center.y + j, z)) {
                result = true;
                set_seen(center.x + i, center.y + j, z, true);
            }
        }
    }
    return result;
}

bool overmapbuffer::check_ot_type(const std::string& type, int x, int y, int z)
{
    overmap& om = get_om_global(x, y);
    return om.check_ot_type(type, x, y, z);
}

point overmapbuffer::find_closest(const tripoint& origin, const std::string& type, int& dist, bool must_be_seen)
{
    int max = (dist == 0 ? OMAPX : dist);
    const int z = origin.z;
    // expanding box
    for (dist = 0; dist <= max; dist++) {
        // each edge length is 2*dist-2, because corners belong to one edge
        // south is +y, north is -y
        for (int i = 0; i < dist*2-1; i++) {
            //start at northwest, scan north edge
            int x = origin.x - dist + i;
            int y = origin.y - dist;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return point(x, y);
                }
            }

            //start at southeast, scan south
            x = origin.x + dist - i;
            y = origin.y + dist;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return point(x, y);
                }
            }

            //start at southwest, scan west
            x = origin.x - dist;
            y = origin.y + dist - i;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return point(x, y);
                }
            }

            //start at northeast, scan east
            x = origin.x + dist;
            y = origin.y - dist + i;
            if (check_ot_type(type, x, y, z)) {
                if (!must_be_seen || seen(x, y, z)) {
                    return point(x, y);
                }
            }
        }
    }
    dist = -1;
    return overmap::invalid_point;
}

std::vector<point> overmapbuffer::find_all(const tripoint& origin, const std::string& type, int dist, bool must_be_seen)
{
    std::vector<point> result;
    int max = (dist == 0 ? OMAPX : dist);
    for (dist = 0; dist <= max; dist++) {
        for (int x = origin.x - dist; x <= origin.x + dist; x++) {
            for (int y = origin.y - dist; y <= origin.y + dist; y++) {
                if (must_be_seen && !seen(x, y, origin.z)) {
                    continue;
                }
                if (check_ot_type(type, x, y, origin.z)) {
                    result.push_back(point(x, y));
                }
            }
        }
    }
    return result;
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

std::vector<npc*> overmapbuffer::get_npcs_near_player(int radius)
{
    tripoint plpos = g->om_global_location();
    // get_npcs_near needs submap coordinates
    omt_to_sm(plpos.x, plpos.y);
    return get_npcs_near(plpos.x, plpos.y, plpos.z, radius);
}

std::vector<overmap*> overmapbuffer::get_overmaps_near( point const location, int const radius )
{
    // Grab the corners of a square around the target location at distance radius.
    // Convert to overmap coordinates and iterate from the minimum to the maximum.
    const point upper_left = sm_to_om_copy( point( location.x - radius, location.y - radius ) );
    const point lower_right = sm_to_om_copy( point( location.x + radius, location.y + radius ) );

    std::vector<point> distinct_corners;
    auto const dx = lower_right.x - upper_left.x;
    auto const dy = lower_right.y - upper_left.y;
    distinct_corners.reserve(std::max(1, dx*dy));

    for( int x = upper_left.x; x <= lower_right.x; x++ ) {
        for( int y = upper_left.y; y <= lower_right.y; y++ ) {
            distinct_corners.emplace_back(x, y);
        }
    }

    std::sort(begin(distinct_corners), end(distinct_corners));
    auto const last = std::unique(begin(distinct_corners), end(distinct_corners));

    std::vector<overmap*> nearby_overmaps;
    nearby_overmaps.reserve(distance(begin(distinct_corners), last));

    // Grab references to the overmaps at those coordinates, but only if they exist.
    // Might use this to drive creation of these overmaps at some point if we want to
    // more agressively expand the created overmaps.
    for (auto const &overmap_origin : distinct_corners) {
        auto const nearby_overmap = get_existing(overmap_origin.x, overmap_origin.y);
        if (nearby_overmap) {
            nearby_overmaps.emplace_back(nearby_overmap);
        }
    }

    return nearby_overmaps;
}

std::vector<npc*> overmapbuffer::get_npcs_near(int x, int y, int z, int radius)
{
    std::vector<npc*> result;
    for( auto &it : get_overmaps_near( point( x, y ), radius ) ) {
        for( auto &elem : it->npcs ) {
            npc *p = elem;
            // Global position of NPC, in submap coordiantes
            const tripoint pos = p->global_sm_location();
            if (pos.z != z) {
                continue;
            }
            const int npc_offset = square_dist(x, y, pos.x, pos.y);
            if (npc_offset <= radius) {
                result.push_back(p);
            }
        }
    }
    return result;
}

std::vector<npc*> overmapbuffer::get_npcs_near_omt(int x, int y, int z, int radius)
{
    std::vector<npc*> result;
    for( auto &it : get_overmaps_near( omt_to_sm_copy( x, y ), radius ) ) {
        for( auto &elem : it->npcs ) {
            npc *p = elem;
            // Global position of NPC, in submap coordiantes
            tripoint pos = p->global_omt_location();
            if (pos.z != z) {
                continue;
            }
            const int npc_offset = square_dist(x, y, pos.x, pos.y);
            if (npc_offset <= radius) {
                result.push_back(p);
            }
        }
    }
    return result;
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
        const point local = g->m.getlocal( ms.x, ms.y );
        assert( g->m.inbounds( local.x, local.y ) );
        this_monster.spawn( local.x, local.y );
        g->add_zombie( this_monster );
    } );
    om.monster_map.erase( current_submap_loc );
}

void overmapbuffer::despawn_monster(const monster &critter)
{
    // Get absolute coordinates of the monster in map squares, translate to submap position
    point sm = ms_to_sm_copy( g->m.getabs( critter.posx(), critter.posy() ) );
    // Get the overmap coordinates and get the overmap, sm is now local to that overmap
    const point omp = sm_to_om_remain( sm );
    overmap &om = get( omp.x, omp.y );
    // Store the monster using coordinates local to the overmap.
    // TODO: with Z-levels this should probably be taken from the critter
    om.monster_map.insert( std::make_pair( tripoint(sm.x, sm.y, g->levz), critter ) );
}

extern bool lcmatch(const std::string& text, const std::string& pattern);
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
    overmap &om = get_om_global(x, y);
    return om.is_safe(x, y, z);
}

inline int modulo(int v, int m) {
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

inline int divide(int v, int m) {
    if (v >= 0) {
        return v / m;
    }
    return (v - m + 1) / m;
}

inline int divide(int v, int m, int &r) {
    const int result = divide(v, m);
    r = v - result * m;
    return result;
}

point overmapbuffer::omt_to_om_copy(int x, int y) {
    return point(divide(x, OMAPX), divide(y, OMAPY));
}

tripoint overmapbuffer::omt_to_om_copy(const tripoint& p) {
    return tripoint(divide(p.x, OMAPX), divide(p.y, OMAPY), p.z);
}

void overmapbuffer::omt_to_om(int &x, int &y) {
    x = divide(x, OMAPX);
    y = divide(y, OMAPY);
}

point overmapbuffer::omt_to_om_remain(int &x, int &y) {
    return point(divide(x, OMAPX, x), divide(y, OMAPY, y));
}



point overmapbuffer::sm_to_omt_copy(int x, int y) {
    return point(divide(x, 2), divide(y, 2));
}

tripoint overmapbuffer::sm_to_omt_copy(const tripoint& p) {
    return tripoint(divide(p.x, 2), divide(p.y, 2), p.z);
}

void overmapbuffer::sm_to_omt(int &x, int &y) {
    x = divide(x, 2);
    y = divide(y, 2);
}

point overmapbuffer::sm_to_omt_remain(int &x, int &y) {
    return point(divide(x, 2, x), divide(y, 2, y));
}



point overmapbuffer::sm_to_om_copy(int x, int y) {
    return point(divide(x, 2 * OMAPX), divide(y, 2 * OMAPY));
}

tripoint overmapbuffer::sm_to_om_copy(const tripoint& p) {
    return tripoint(divide(p.x, 2 * OMAPX), divide(p.y, 2 * OMAPY), p.z);
}

void overmapbuffer::sm_to_om(int &x, int &y) {
    x = divide(x, 2 * OMAPX);
    y = divide(y, 2 * OMAPY);
}

point overmapbuffer::sm_to_om_remain(int &x, int &y) {
    return point(divide(x, 2 * OMAPX, x), divide(y, 2 * OMAPY, y));
}



point overmapbuffer::omt_to_sm_copy(int x, int y) {
    return point(x * 2, y * 2);
}

tripoint overmapbuffer::omt_to_sm_copy(const tripoint& p) {
    return tripoint(p.x * 2, p.y * 2, p.z);
}

void overmapbuffer::omt_to_sm(int &x, int &y) {
    x *= 2;
    y *= 2;
}



point overmapbuffer::om_to_sm_copy(int x, int y) {
    return point(x * 2 * OMAPX, y * 2 * OMAPX);
}

tripoint overmapbuffer::om_to_sm_copy(const tripoint& p) {
    return tripoint(p.x * 2 * OMAPX, p.y * 2 * OMAPX, p.z);
}

void overmapbuffer::om_to_sm(int &x, int &y) {
    x *= 2 * OMAPX;
    y *= 2 * OMAPY;
}



point overmapbuffer::ms_to_sm_copy(int x, int y) {
    return point(divide(x, SEEX), divide(y, SEEY));
}

tripoint overmapbuffer::ms_to_sm_copy(const tripoint& p) {
    return tripoint(divide(p.x, SEEX), divide(p.y, SEEY), p.z);
}

void overmapbuffer::ms_to_sm(int &x, int &y) {
    x = divide(x, SEEX);
    y = divide(y, SEEY);
}

point overmapbuffer::ms_to_sm_remain(int &x, int &y) {
    return point(divide(x, SEEX, x), divide(y, SEEY, y));
}



point overmapbuffer::sm_to_ms_copy(int x, int y) {
    return point(x * SEEX, y * SEEY);
}

tripoint overmapbuffer::sm_to_ms_copy(const tripoint& p) {
    return tripoint(p.x * SEEX, p.y * SEEY, p.z);
}

void overmapbuffer::sm_to_ms(int &x, int &y) {
    x *= SEEX;
    y *= SEEY;
}



point overmapbuffer::ms_to_omt_copy(int x, int y) {
    return point(divide(x, SEEX * 2), divide(y, SEEY * 2));
}

tripoint overmapbuffer::ms_to_omt_copy(const tripoint& p) {
    return tripoint(divide(p.x, SEEX * 2), divide(p.y, SEEY * 2), p.z);
}

void overmapbuffer::ms_to_omt(int &x, int &y) {
    x = divide(x, SEEX * 2);
    y = divide(y, SEEY * 2);
}

point overmapbuffer::ms_to_omt_remain(int &x, int &y) {
    return point(divide(x, SEEX * 2, x), divide(y, SEEY * 2, y));
}



tripoint overmapbuffer::omt_to_seg_copy(const tripoint& p) {
    return tripoint(divide(p.x, 32), divide(p.y, 32), p.z);
}
