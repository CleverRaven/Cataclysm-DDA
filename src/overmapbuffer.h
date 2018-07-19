#pragma once
#ifndef OVERMAPBUFFER_H
#define OVERMAPBUFFER_H

#include "enums.h"
#include "int_id.h"
#include "overmap_types.h"

#include <set>
#include <list>
#include <memory>
#include <vector>
#include <unordered_map>

struct mongroup;
class monster;
class npc;
struct om_vehicle;

struct oter_t;
using oter_id = int_id<oter_t>;

class overmap;
class overmap_special;
class overmap_special_batch;
struct radio_tower;
struct regional_settings;
class vehicle;

struct radio_tower_reference {
    /** The radio tower itself, points into @ref overmap::radios */
    radio_tower *tower;
    /** The global absolute position of the tower (in submap coordinates) */
    point abs_sm_pos;
    /** Perceived signal strength (tower output strength minus distance) */
    int signal_strength;
    operator bool() const {
        return tower != nullptr;
    }
};

struct city_reference {
    static const city_reference invalid;
    /** The city itself, points into @ref overmap::cities */
    const struct city *city;
    /** The global absolute position of the city (in submap coordinates!) */
    tripoint abs_sm_pos;
    /** Distance to center of the search */
    int distance;

    operator bool() const {
        return city != nullptr;
    }

    int get_distance_from_bounds() const;
};

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
    void create_custom_overmap( int const x, int const y, overmap_special_batch &specials );

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
    bool is_explored(int x, int y, int z);
    void toggle_explored(int x, int y, int z);
    bool seen(int x, int y, int z);
    void set_seen(int x, int y, int z, bool seen = true);
    bool has_vehicle( int x, int y, int z );
    bool has_horde(int x, int y, int z);
    int get_horde_size(int x, int y, int z);
    std::vector<om_vehicle> get_vehicle( int x, int y, int z );
    const regional_settings& get_settings(int x, int y, int z);

    /**
     * Accessors for horde introspection into overmaps.
     * Probably also useful for NPC overmap-scale navigation.
     */
    /**
     * Returns the 3x3 array of scent values surrounding the origin point.
     * @param origin is in world-global omt coordinates.
     */
    std::array<std::array<scent_trace, 3>, 3> scents_near( const tripoint &origin );
    /**
     * Method to retrieve the scent at a given location.
     **/
    scent_trace scent_at( const tripoint &pos );
    /**
     * Method to set a scent trace.
     * @param loc is in world-global omt coordinates.
     * @param strength sets the intensity of the scent trace,
     *     used for determining if a monster can detect the scent.
     */
    void set_scent( const tripoint &loc, int strength );
    /**
     * Check for any dangerous monster groups at the global overmap terrain coordinates.
     * If there are any, it's not safe.
     */
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
     * @param x,y,z are submap coordinates.
     * @param radius Maximal distance of npc from (x,y). If the npc
     * is at most this far away from (x,y) it will be returned.
     * A radius of 0 returns only those npcs that are on the
     * specific submap.
     */
    std::vector<std::shared_ptr<npc>> get_npcs_near( int x, int y, int z, int radius );
        /**
         * Get all (currently loaded!) npcs that have a companion
         * mission set.
         */
        std::vector<std::shared_ptr<npc>> get_companion_mission_npcs();
    /**
     * Uses overmap terrain coordinates, this also means radius is
     * in overmap terrain.
     * A radius of 0 returns all npcs that are on that specific
     * overmap terrain tile.
     */
    std::vector<std::shared_ptr<npc>> get_npcs_near_omt( int x, int y, int z, int radius );
    /**
     * Same as @ref get_npcs_near(int,int,int,int) but uses
     * player position as center.
     */
    std::vector<std::shared_ptr<npc>> get_npcs_near_player( int radius );
    /**
     * Find the npc with the given ID.
     * Returns NULL if the npc could not be found.
     * Searches all loaded overmaps.
     */
    std::shared_ptr<npc> find_npc( int id );
    /**
     * Find npc by id and if found, erase it from the npc list
     * and return it ( or return nullptr if not found ).
     */
    std::shared_ptr<npc> remove_npc( int id );
        /**
         * Adds the npc to an overmap ( based on the npcs current location )
         * and stores it there. The overmap takes ownership of the pointer.
         */
        void insert_npc( std::shared_ptr<npc> who );

    /**
     * Find all places with the specific overmap terrain type.
     * The function only searches on the z-level indicated by
     * origin.
     * This function may create a new overmap if needed.
     * @param origin Location of search
     * @param type Terrain type to search for
     * @param dist The maximum search distance.
     * If 0, OMAPX is used.
     * @param must_be_seen If true, only terrain seen by the player
     * should be searched.
     */
    std::vector<tripoint> find_all(const tripoint& origin, const std::string& type,
        int dist, bool must_be_seen);

    /**
     * Returns a random point of specific terrain type among those found in certain search radius.
     * This function may create new overmaps if needed.
     * @param type Type of terrain to search for
     * @param dist The maximal radius of the area to search for the desired terrain.
     * A value of 0 will search an area equal to 4 entire overmaps.
     * @returns If no matching tile can be found @ref overmap::invalid_tripoint is returned.
     * @param origin uses overmap terrain coordinates.
     * @param must_be_seen If true, only terrain seen by the player
     * should be searched.
     */
    tripoint find_random(const tripoint& origin, const std::string& type,
        int dist, bool must_be_seen);

    /**
     * Mark a square area around center on Z-level z
     * as seen.
     * @param center is in absolute overmap terrain coordinates.
     * @param radius The half size of the square to make visible.
     * A value of 0 makes only center visible, radius 1 makes a
     * square 3x3 visible.
     * @param z Z level to make area on
     * @return true if something has actually been revealed.
     */
    bool reveal(const point &center, int radius, int z);
    bool reveal( const tripoint &center, int radius );

    bool reveal_route( const tripoint &source, const tripoint &dest, int radius = 0, bool road_only = false );
    /**
     * Returns the closest point of terrain type.
     * This function may create new overmaps if needed.
     * @param type Type of terrain to look for
     * @param radius The maximal radius of the area to search for the desired terrain.
     * A value of 0 will search an area equal to 4 entire overmaps.
     * @returns If no matching tile can be found @ref overmap::invalid_tripoint is returned.
     * @param origin uses overmap terrain coordinates.
     * @param must_be_seen If true, only terrain seen by the player
     * should be searched.
     */
    tripoint find_closest(const tripoint& origin, const std::string& type, int radius, bool must_be_seen);

    /* These 4 functions return the overmap that contains the given
     * overmap terrain coordinate.
     * get_existing_om_global will not create a new overmap and
     * instead return NULL, if the requested overmap does not yet exist.
     * get_om_global creates a new overmap if needed.
     *
     * The parameters x and y will be cropped to be local to the
     * returned overmap, the parameter p will not be changed.
     */
    overmap* get_existing_om_global( int& x, int& y );
    overmap* get_existing_om_global( const point& p );
    overmap* get_existing_om_global( const tripoint& p );
    overmap& get_om_global( int& x, int& y );
    overmap& get_om_global( const point& p );
    overmap& get_om_global( const tripoint& p );
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
    overmap *get_existing( int x, int y );

    typedef std::pair<point, std::string> t_point_with_note;
    typedef std::vector<t_point_with_note> t_notes_vector;
    t_notes_vector get_all_notes(int z) {
        return get_notes(z, NULL); // NULL => don't filter notes
    }
    t_notes_vector find_notes(int z, const std::string& pattern) {
        return get_notes(z, &pattern); // filter with pattern
    }
    /**
     * Signal nearby hordes to move to given location.
     * @param center The origin of the signal, hordes (that recognize the signal) want to go
     * to there. In global submap coordinates.
     * @param sig_power The signal strength, higher values means it visible farther away.
     */
    void signal_hordes( const tripoint &center, int sig_power );
    /**
     * Process nearby monstergroups (dying mostly).
     */
    void process_mongroups();
    /**
     * Let hordes move a step. Note that this may move monster groups inside the reality bubble,
     * therefor you should probably call @ref map::spawn_monsters to spawn them.
     */
    void move_hordes();
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
    /**
     * Find radio station with given frequency, search an unspecified area around
     * the current player location.
     * If no matching tower has been found, it returns an object with the tower pointer set
     * to null.
     */
    radio_tower_reference find_radio_station( int frequency );
    /**
     * Find all radio stations that can be received around the current player location.
     * All entries in the returned vector are valid (have a valid tower pointer).
     */
    std::vector<radio_tower_reference> find_all_radio_stations();
    /**
     * Find all cities within the specified @ref radius.
     * Result is sorted by proximity to @ref location in ascending order.
     */
    std::vector<city_reference> get_cities_near( const tripoint &location, int radius );
    /**
     * Find the closest city. If no city is close, returns an object with city set to nullptr.
     * @param center The center of the search, the distance for determining the closest city is
     * calculated as distance to this point. In global submap coordinates!
     */
    city_reference closest_city( const tripoint &center );

    city_reference closest_known_city( const tripoint &center );

    std::string get_description_at( const tripoint &where );

private:
    std::unordered_map< point, std::unique_ptr< overmap > > overmaps;
    /**
     * Set of overmap coordinates of overmaps that are known
     * to not exist on disk. See @ref get_existing for usage.
     */
    mutable std::set<point> known_non_existing;
    // Cached result of previous call to overmapbuffer::get_existing
    overmap mutable *last_requested_overmap;

    /**
     * Get a list of notes in the (loaded) overmaps.
     * @param z only this specific z-level is search for notes.
     * @param pattern only notes that contain this pattern are returned.
     * If the pattern is NULL, every note matches.
     */
    t_notes_vector get_notes(int z, const std::string* pattern);
public:
    /**
     * See overmap::check_ot_type, this uses global
     * overmap terrain coordinates.
     * This function may create a new overmap if needed.
     */
    bool check_ot_type(const std::string& otype, int x, int y, int z);
private:
    /**
     * Go thorough the monster groups of the overmap and move out-of-bounds
     * groups to the correct overmap (if it exists), also removes empty groups.
     */
    void fix_mongroups(overmap &new_overmap);
    /**
     * Moves out-of-bounds NPCs to the overmaps they should be in.
     */
    void fix_npcs( overmap &new_overmap );
    /**
     * Retrieve overmaps that overlap the bounding box defined by the location and radius.
     * The location is in absolute submap coordinates, the radius is in the same system.
     */
    std::vector<overmap *> get_overmaps_near( const point &location, int radius );
    std::vector<overmap *> get_overmaps_near( const tripoint &location, int radius );
};

extern overmapbuffer overmap_buffer;

#endif
