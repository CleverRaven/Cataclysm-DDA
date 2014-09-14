#ifndef _OVERMAPBUFFER_H_
#define _OVERMAPBUFFER_H_

#include "enums.h"
#include "overmap.h"
#include <set>
#include <list>
#include <memory>
#include <unordered_map>

class monster;

/**
 * Coordinate systems used here are:
 * overmap (om): the position of an overmap. Each overmap stores
 * this as overmap::loc (access with overmap::pos()).
 * There is a unique overmap for each overmap coord.
 *
 * segment (seg): A segment is a unit of terrain saved to a directory.
 * Each segment contains 32x32 overmap terrains, and is used only for
 * saving/loading submaps, see mapbuffer.cpp.
 * Translation from omt to seg:
 * om.x /= 32
 * om.y /= 32
 * (with special handling for negative values).
 *
 * overmap terrain (omt): the position of a overmap terrain (oter_id).
 * Each overmap contains (OMAPX * OMAPY) overmap terrains.
 * Translation from omt to om:
 * om.x /= OMAPX
 * om.y /= OMAPY
 * (with special handling for negative values).
 *
 * Z-components are never considered and simply copied.
 *
 * submap (sm): each overmap terrain contains (2*2) submaps.
 * Translating from sm to omt coordinates:
 * sm.x /= 2
 * sm.y /= 2
 *
 * map square (ms): used by @ref map, each map square may contain a single
 * piece of furniture, it has a terrain (ter_t).
 * There are SEEX*SEEY map squares in each submap.
 *
 * The class provides static translation functions, named like this:
    static point <from>_to_<to>_copy(int x, int y);
    static point <from>_to_<to>_copy(const point& p);
    static tripoint <from>_to_<to>_copy(const tripoint& p);
    static void <from>_to_<to>(int &x, int &y);
    static void <from>_to_<to>(point& p);
    static void <from>_to_<to>(tripoint& p);
    static point <from>_to_<to>_remain(int &x, int &y);
    static point <from>_to_<to>_remain(point& p);
 * Functions ending with _copy return the translated coordinates,
 * other functions change the parameters itself and don't return anything.
 * Functions ending with _remain return teh translated coordinates and
 * store the remainder in the parameters.
 */
class overmapbuffer
{
public:
    overmapbuffer();

    static std::string terrain_filename(int const x, int const y);
    static std::string player_filename(int const x, int const y);

    /**
     * Uses overmap coordinates, that means x and y are directly
     * compared with the position of the overmap.
     */
    overmap &get( const int x, const int y );
    void save();
    void clear();

    /**
     * Uses global overmap terrain coordinates, creates the
     * overmap if needed.
     */
    oter_id& ter(int x, int y, int z);
    oter_id& ter(const tripoint& p) { return ter(p.x, p.y, p.z); }
    /**
     * Uses global overmap terrain coordinates.
     */
    bool has_note(int x, int y, int z);
    bool has_note(const tripoint& p) { return has_note(p.x, p.y, p.z); }
    const std::string& note(int x, int y, int z);
    const std::string& note(const tripoint& p) { return note(p.x, p.y, p.z); }
    void add_note(int x, int y, int z, const std::string& message);
    void add_note(const tripoint& p, const std::string& message) { add_note(p.x, p.y, p.z, message); }
    void delete_note(int x, int y, int z);
    void delete_note(const tripoint& p) { delete_note(p.x, p.y, p.z); }
    bool seen(int x, int y, int z);
    void set_seen(int x, int y, int z, bool seen = true);
    bool has_npc(int x, int y, int z);
    bool has_vehicle(int x, int y, int z, bool require_pda = true);
    std::vector<om_vehicle> get_vehicle(int x, int y, int z, bool require_pda = true);
    const regional_settings& get_settings(int x, int y, int z);
    bool is_safe(int x, int y, int z);
    bool is_safe(const tripoint& p) { return is_safe(p.x, p.y, p.z); }

    /**
     * Move the tracking mark of the given vehicle.
     * @param veh The vehicle whose tracking device is active and
     * that has been moved.
     * @param old_msp The previous position (before the movement) of the
     * vehicle. In map square coordinates (see vehicle::real_global_pos), it's
     * used to remove the vehicle from the old overmap if the new position is
     * on another overmap.
     */
    void move_vehicle(vehicle *veh, const point &old_msp);
    /**
     * Add the vehicle to be tracked in the overmap.
     */
    void add_vehicle(vehicle *veh);
    /**
     * Remove the vehicle from being tracked in the overmap.
     */
    void remove_vehicle(const vehicle *veh);
    /**
     * Get all npcs in a area with given radius around (x, y).
     * Only npcs on the given z-level are considered.
     * Uses square_dist for distance calculation.
     * x,y are submap coordinates.
     * @radius Maximal distance of npc from (x,y). If the npc
     * is at most this far away from (x,y) it will be returned.
     * A radius of 0 returns only those npcs that are on the
     * specifc submap.
     */
    std::vector<npc*> get_npcs_near(int x, int y, int z, int radius);
    /**
     * Uses overmap terrain coords, this also means radius is
     * in overmap terrain.
     * A radius of 0 returns all npcs that are on that specifc
     * overmap terrain tile.
     */
    std::vector<npc*> get_npcs_near_omt(int x, int y, int z, int radius);
    /**
     * Same as @ref get_npcs_near(int,int,int,int) but uses
     * player position as center.
     */
    std::vector<npc*> get_npcs_near_player(int radius);
    /**
     * Find the npc with the given ID.
     * Returns NULL if the npc could not be found.
     * Searches all loaded overmaps.
     */
    npc* find_npc(int id);
    /**
     * Find npc by id and if found, erase it from the npc list
     * and delete the npc object. This assumes that the npc is
     * already dead and not contained in game::active_npc anymore.
     */
    void remove_npc(int id);

    /**
     * Find all places with the specific overmap terrain type.
     * The function only searches on the z-level indicated by
     * origin.
     * This function may greate a new overmap if needed.
     * @param dist The maximum search distance.
     * If 0, OMAPX is used.
     * @param must_be_seen If true, only terrain seen by the player
     * should be searched.
     */
    std::vector<point> find_all(const tripoint& origin, const std::string& type,
        int dist, bool must_be_seen);

    /**
     * Mark a square area around center on z-level z
     * as seen.
     * center is in absolute overmap terrain coords.
     * @param radius The half size of the square to make visible.
     * A value of 0 makes only center visible, radius 1 makes a
     * square 3x3 visible.
     * @return true if something has actually been revealed.
     */
    bool reveal(const point &center, int radius, int z);
    /**
     * Returns the closest point of terrain type.
     * This function may greate a new overmap if needed.
     * @param dist This gets set to the distance between the origin
     * and the returned point.
     * You can give dist an initial value, which will be used as
     * the maximum distance for point to be searched.
     * A value of 0 will search an entire overmap area.
     * @returns If no matching tile can be found,
     * overmap::invalid_point is returned.
     * @param origin uses overmap terrain coordinates.
     * @param must_be_seen If true, only terrain seen by the player
     * should be searched.
     */
    point find_closest(const tripoint& origin, const std::string& type, int& dist, bool must_be_seen);

    /* These 4 functions return the overmap that contains the given
     * overmap terrain coordinate.
     * get_existing_om_global will not create a new overmap and
     * instead return NULL, if the requested overmap does not yet exist.
     * get_om_global creates a new overmap if needed.
     *
     * The parameters x and y will be cropped to be local to the
     * returned overmap, the parameter p will not be changed.
     */
    overmap* get_existing_om_global(int& x, int& y);
    overmap* get_existing_om_global(const point& p);
    overmap& get_om_global(int& x, int& y);
    overmap& get_om_global(const point& p);
    /**
     * (x,y) are global overmap coordinates (same as @ref get).
     * @returns true if the buffer has a overmap with
     * the given coordinates.
     */
    bool has(int x, int y);
    /**
     * Get an existing overmap, does not create a new one
     * and may return NULL if the requested overmap does not
     * exist.
     * (x,y) are global overmap coordinates (same as @ref get).
     */
    overmap *get_existing(int x, int y);

    typedef std::pair<point, std::string> t_point_with_note;
    typedef std::vector<t_point_with_note> t_notes_vector;
    t_notes_vector get_all_notes(int z) {
        return get_notes(z, NULL); // NULL => don't filter notes
    }
    t_notes_vector find_notes(int z, const std::string& pattern) {
        return get_notes(z, &pattern); // filter with pattern
    }
    // hordes -- this uses overmap terrain coordinates!
    std::vector<mongroup*> monsters_at(int x, int y, int z);
    /**
     * Monster groups at (x,y,z) - absolute submap coordinates.
     * Groups with no population are not included.
     */
    std::vector<mongroup*> groups_at(int x, int y, int z);

    /**
     * Spawn monsters from the overmap onto the main map (game::m).
     * (x,y,z) is an absolute *submap* coordinate.
     */
    void spawn_monster(const int x, const int y, const int z);
    /**
     * Despawn the monster back onto the overmap. The monsters position
     * (monster::pos()) is interpreted as relative to the main map.
     */
    void despawn_monster(const monster &critter);

    // overmap terrain to overmap
    static point omt_to_om_copy(int x, int y);
    static point omt_to_om_copy(const point& p) { return omt_to_om_copy(p.x, p.y); }
    static tripoint omt_to_om_copy(const tripoint& p);
    static void omt_to_om(int &x, int &y);
    static void omt_to_om(point& p) { omt_to_om(p.x, p.y); }
    static void omt_to_om(tripoint& p) { omt_to_om(p.x, p.y); }
    static point omt_to_om_remain(int &x, int &y);
    static point omt_to_om_remain(point& p) { return omt_to_om_remain(p.x, p.y); }
    // submap to overmap terrain
    static point sm_to_omt_copy(int x, int y);
    static point sm_to_omt_copy(const point& p) { return sm_to_omt_copy(p.x, p.y); }
    static tripoint sm_to_omt_copy(const tripoint& p);
    static void sm_to_omt(int &x, int &y);
    static void sm_to_omt(point& p) { sm_to_omt(p.x, p.y); }
    static void sm_to_omt(tripoint& p) { sm_to_omt(p.x, p.y); }
    static point sm_to_omt_remain(int &x, int &y);
    static point sm_to_omt_remain(point& p) { return sm_to_omt_remain(p.x, p.y); }
    // submap to overmap, basically: x / (OMAPX * 2)
    static point sm_to_om_copy(int x, int y);
    static point sm_to_om_copy(const point& p) { return sm_to_om_copy(p.x, p.y); }
    static tripoint sm_to_om_copy(const tripoint& p);
    static void sm_to_om(int &x, int &y);
    static void sm_to_om(point& p) { sm_to_om(p.x, p.y); }
    static void sm_to_om(tripoint& p) { sm_to_om(p.x, p.y); }
    static point sm_to_om_remain(int &x, int &y);
    static point sm_to_om_remain(point& p) { return sm_to_om_remain(p.x, p.y); }
    // overmap terrain to submap, basically: x *= 2
    static point omt_to_sm_copy(int x, int y);
    static point omt_to_sm_copy(const point& p) { return omt_to_sm_copy(p.x, p.y); }
    static tripoint omt_to_sm_copy(const tripoint& p);
    static void omt_to_sm(int &x, int &y);
    static void omt_to_sm(point& p) { omt_to_sm(p.x, p.y); }
    static void omt_to_sm(tripoint& p) { omt_to_sm(p.x, p.y); }
    // overmap to submap, basically: x *= 2 * OMAPX
    static point om_to_sm_copy(int x, int y);
    static point om_to_sm_copy(const point& p) { return om_to_sm_copy(p.x, p.y); }
    static tripoint om_to_sm_copy(const tripoint& p);
    static void om_to_sm(int &x, int &y);
    static void om_to_sm(point& p) { om_to_sm(p.x, p.y); }
    static void om_to_sm(tripoint& p) { om_to_sm(p.x, p.y); }
    // map squares to submap, basically: x /= SEEX
    static point ms_to_sm_copy(int x, int y);
    static point ms_to_sm_copy(const point& p) { return ms_to_sm_copy(p.x, p.y); }
    static tripoint ms_to_sm_copy(const tripoint& p);
    static void ms_to_sm(int &x, int &y);
    static void ms_to_sm(point& p) { ms_to_sm(p.x, p.y); }
    static void ms_to_sm(tripoint& p) { ms_to_sm(p.x, p.y); }
    static point ms_to_sm_remain(int &x, int &y);
    static point ms_to_sm_remain(point& p) { return ms_to_sm_remain(p.x, p.y); }
    // map squares to overmap terrain, basically: x /= SEEX * 2
    static point ms_to_omt_copy(int x, int y);
    static point ms_to_omt_copy(const point& p) { return ms_to_omt_copy(p.x, p.y); }
    static tripoint ms_to_omt_copy(const tripoint& p);
    static void ms_to_omt(int &x, int &y);
    static void ms_to_omt(point& p) { ms_to_omt(p.x, p.y); }
    static void ms_to_omt(tripoint& p) { ms_to_omt(p.x, p.y); }
    static point ms_to_omt_remain(int &x, int &y);
    static point ms_to_omt_remain(point& p) { return ms_to_omt_remain(p.x, p.y); }
    // overmap terrain to map segment.
    static tripoint omt_to_seg_copy(const tripoint& p);

private:
    std::unordered_map< point, std::unique_ptr< overmap > > overmaps;
    /**
     * Set of overmap coordinates of overmaps that are known
     * to not exist on disk. See @ref get_existing for usage.
     */
    mutable std::set<point> known_non_existing;
    // Cached result of previous call to overmapbuffer::get_existing
    overmap mutable * last_requested_overmap;

    /**
     * Get a list of notes in the (loaded) overmaps.
     * @param z only this specific z-level is search for notes.
     * @param pattern only notes that contain this pattern are returned.
     * If the pattern is NULL, every note matches.
     */
    t_notes_vector get_notes(int z, const std::string* pattern);
    /**
     * See overmap::check_ot_type, this uses global
     * overmap terrain coordinates.
     * This function may greate a new overmap if needed.
     */
    bool check_ot_type(const std::string& otype, int x, int y, int z);
    /**
     * Go thorough the monster groups of the overmap and move out-of-bounds
     * groups to the correct overmap (if it exists), also removes empty groups.
     */
    void fix_mongroups(overmap &new_overmap);
    /**
     * Retrieve overmaps that overlap the bounding box defined by the location and radius.
     */
    std::vector<overmap *> get_overmaps_near( point location, int radius );
};

extern overmapbuffer overmap_buffer;

#endif /* _OVERMAPBUFFER_H_ */
