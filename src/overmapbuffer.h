#pragma once
#ifndef OVERMAPBUFFER_H
#define OVERMAPBUFFER_H

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <array>
#include <functional>
#include <string>
#include <utility>

#include "enums.h"
#include "omdata.h"
#include "overmap_types.h"
#include "optional.h"
#include "type_id.h"
#include "point.h"
#include "string_id.h"

class character_id;
struct mongroup;
class monster;
class npc;
struct om_vehicle;
class overmap_special_batch;
class overmap;
struct radio_tower;
struct regional_settings;
class vehicle;
class basecamp;
class map_extra;

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

struct camp_reference {
    static const camp_reference invalid;
    /** The camp itself, points into @ref overmap::camps */
    basecamp *camp;
    /** The global absolute position of the camp (in submap coordinates!) */
    tripoint abs_sm_pos;
    /** Distance to center of the search */
    int distance;

    operator bool() const {
        return camp != nullptr;
    }

    int get_distance_from_bounds() const;
};

struct overmap_with_local_coords {
    overmap *om;
    tripoint local;

    bool operator!() const {
        return !om;
    }

    explicit operator bool() const {
        return !!om;
    }
};

/*
 * Standard arguments for finding overmap terrain
 * @param origin Location of search
 * @param type Terrain type to search for
 * @param match_type Matching rule to use when finding the terrain type.
 * @param search_range The maximum search distance.  If 0, OMAPX is used.
 * @param min_distance Matches within min_distance are ignored.
 * @param must_see If true, only terrain seen by the player should be searched.
 * @param cant_see If true, only terrain not seen by the player should be searched
 * @param existing_overmaps_only If true, will restrict searches to existing overmaps only. This
 * is particularly useful if we want to attempt to add a missing overmap special to an existing
 * overmap rather than creating many overmaps in an attempt to find it.
 * @param om_special If set, the terrain must be part of the specified overmap special.
*/
struct omt_find_params {
    std::string type;
    ot_match_type match_type = ot_match_type::type;
    int search_range = 0;
    int min_distance = 0;
    bool must_see = false;
    bool cant_see = false;
    bool existing_only = false;
    cata::optional<overmap_special_id> om_special = cata::nullopt;
};

class overmapbuffer
{
    public:
        overmapbuffer();

        static std::string terrain_filename( const point & );
        static std::string player_filename( const point & );

        /**
         * Uses overmap coordinates, that means x and y are directly
         * compared with the position of the overmap.
         */
        overmap &get( const point & );
        void save();
        void clear();
        void create_custom_overmap( const point &, overmap_special_batch &specials );

        /**
         * Uses global overmap terrain coordinates, creates the
         * overmap if needed.
         */
        oter_id &ter( const tripoint &p );
        /**
         * Uses global overmap terrain coordinates.
         */
        bool has_note( const tripoint &p );
        const std::string &note( const tripoint &p );
        void add_note( const tripoint &, const std::string &message );
        void delete_note( const tripoint &p );
        bool has_extra( const tripoint &p );
        const string_id<map_extra> &extra( const tripoint &p );
        void add_extra( const tripoint &p, const string_id<map_extra> &id );
        void delete_extra( const tripoint &p );
        bool is_explored( const tripoint &p );
        void toggle_explored( const tripoint &p );
        bool seen( const tripoint &p );
        void set_seen( const tripoint &p, bool seen = true );
        bool has_camp( const tripoint &p );
        bool has_vehicle( const tripoint &p );
        bool has_horde( const tripoint &p );
        int get_horde_size( const tripoint &p );
        std::vector<om_vehicle> get_vehicle( const tripoint &p );
        const regional_settings &get_settings( const tripoint &p );
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
        bool is_safe( const tripoint &p );

        /**
         * Move the tracking mark of the given vehicle.
         * @param veh The vehicle whose tracking device is active and
         * that has been moved.
         * @param old_msp The previous position (before the movement) of the
         * vehicle. In map square coordinates (see vehicle::real_global_pos), it's
         * used to remove the vehicle from the old overmap if the new position is
         * on another overmap.
         */
        void move_vehicle( vehicle *veh, const point &old_msp );
        /**
         * Add the vehicle to be tracked in the overmap.
         */
        void add_vehicle( vehicle *veh );
        /**
         * Remove basecamp
         */
        void remove_camp( const basecamp &camp );
        /**
         * Remove the vehicle from being tracked in the overmap.
         */
        void remove_vehicle( const vehicle *veh );
        /**
         * Add Basecamp to overmapbuffer
         */
        void add_camp( const basecamp &camp );

        cata::optional<basecamp *> find_camp( const point &p );
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

        std::vector<std::shared_ptr<npc>> get_npcs_near( const tripoint &p, int radius );
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
        std::vector<std::shared_ptr<npc>> get_npcs_near_omt( const tripoint &p, int radius );
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
        std::shared_ptr<npc> find_npc( character_id id );
        /**
         * Get all NPCs active on the overmap
         */
        std::vector<std::shared_ptr<npc>> get_overmap_npcs();
        /**
         * Find npc by id and if found, erase it from the npc list
         * and return it ( or return nullptr if not found ).
         */
        std::shared_ptr<npc> remove_npc( character_id id );
        /**
         * Adds the npc to an overmap ( based on the npcs current location )
         * and stores it there. The overmap takes ownership of the pointer.
         */
        void insert_npc( const std::shared_ptr<npc> &who );

        /**
         * Find all places with the specific overmap terrain type.
         * The function only searches on the z-level indicated by
         * origin.
         * This function may create a new overmap if needed.
         * @param origin Location of search
         * see omt_find_params for definitions of the terms
         */
        std::vector<tripoint> find_all( const tripoint &origin, const omt_find_params &params );
        std::vector<tripoint> find_all( const tripoint &origin, const std::string &type,
                                        int dist, bool must_be_seen, ot_match_type match_type = ot_match_type::type,
                                        bool existing_overmaps_only = false,
                                        const cata::optional<overmap_special_id> &om_special = cata::nullopt );

        /**
         * Returns a random point of specific terrain type among those found in certain search
         * radius.
         * This function may create new overmaps if needed.
         * @param origin Location of search
         * see omt_find_params for definitions of the terms
         */
        tripoint find_random( const tripoint &origin, const omt_find_params &params );
        tripoint find_random( const tripoint &origin, const std::string &type,
                              int dist, bool must_be_seen, ot_match_type match_type = ot_match_type::type,
                              bool existing_overmaps_only = false,
                              const cata::optional<overmap_special_id> &om_special = cata::nullopt );
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
        bool reveal( const point &center, int radius, int z );
        bool reveal( const tripoint &center, int radius );
        bool reveal( const tripoint &center, int radius,
                     const std::function<bool( const oter_id & )> &filter );
        std::vector<tripoint> get_npc_path( const tripoint &src, const tripoint &dest,
                                            bool road_only = false );
        bool reveal_route( const tripoint &source, const tripoint &dest, int radius = 0,
                           bool road_only = false );
        /**
         * Returns the closest point of terrain type.
         * @param origin Location of search
         * see omt_find_params for definitions of the terms
         */
        tripoint find_closest( const tripoint &origin, const omt_find_params &params );
        tripoint find_closest( const tripoint &origin, const std::string &type, int radius,
                               bool must_be_seen, ot_match_type match_type = ot_match_type::type,
                               bool existing_overmaps_only = false,
                               const cata::optional<overmap_special_id> &om_special = cata::nullopt );

        /* These functions return the overmap that contains the given
         * overmap terrain coordinate, and the local coordinates of that point
         * within the overmap (for use with overmap APIs).
         * get_existing_om_global will not create a new overmap and
         * if the requested overmap does not yet exist it returns
         * { nullptr, tripoint_zero }.
         * get_om_global creates a new overmap if needed.
         */
        overmap_with_local_coords get_existing_om_global( const point &p );
        overmap_with_local_coords get_existing_om_global( const tripoint &p );
        overmap_with_local_coords get_om_global( const point &p );
        overmap_with_local_coords get_om_global( const tripoint &p );

        /**
         * Pass global overmap coordinates (same as @ref get).
         * @returns true if the buffer has a overmap with
         * the given coordinates.
         */
        bool has( const point &p );
        /**
         * Get an existing overmap, does not create a new one
         * and may return NULL if the requested overmap does not
         * exist.
         * (x,y) are global overmap coordinates (same as @ref get).
         */
        overmap *get_existing( const point &p );
        /**
         * Returns whether or not the location has been generated (e.g. mapgen has run).
         * @param loc is in world-global omt coordinates.
         * @returns True if the location has been generated.
         */
        bool is_omt_generated( const tripoint &loc );

        using t_point_with_note = std::pair<point, std::string>;
        using t_notes_vector = std::vector<t_point_with_note>;
        t_notes_vector get_all_notes( int z ) {
            return get_notes( z, nullptr ); // NULL => don't filter notes
        }
        t_notes_vector find_notes( int z, const std::string &pattern ) {
            return get_notes( z, &pattern ); // filter with pattern
        }
        using t_point_with_extra = std::pair<point, string_id<map_extra>>;
        using t_extras_vector = std::vector<t_point_with_extra>;
        t_extras_vector get_all_extras( int z ) {
            return get_extras( z, nullptr ); // NULL => don't filter extras
        }
        t_extras_vector find_extras( int z, const std::string &pattern ) {
            return get_extras( z, &pattern ); // filter with pattern
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
         * therefore you should probably call @ref map::spawn_monsters to spawn them.
         */
        void move_hordes();
        // hordes -- this uses overmap terrain coordinates!
        std::vector<mongroup *> monsters_at( const tripoint &p );
        /**
         * Monster groups at p - absolute submap coordinates.
         * Groups with no population are not included.
         */
        std::vector<mongroup *> groups_at( const tripoint &p );

        /**
         * Spawn monsters from the overmap onto the main map (game::m).
         * p is an absolute *submap* coordinate.
         */
        void spawn_monster( const tripoint &p );
        /**
         * Despawn the monster back onto the overmap. The monsters position
         * (monster::pos()) is interpreted as relative to the main map.
         */
        void despawn_monster( const monster &critter );
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
        std::vector<camp_reference> get_camps_near( const tripoint &location, int radius );
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

        /**
         * Place the specified overmap special directly on the map using the provided location and rotation.
         * Intended to be used when you have a special in hand, the desired location and rotation are known
         * and the special should be directly placed rather than using the overmap's placement algorithm.
         * @param special The overmap special to place.
         * @param location The location to place the overmap special. Absolute overmap terrain coordinates.
         * @param dir The direction to rotate the overmap special before placement.
         * @param must_be_unexplored If true, will require that all of the terrains where the special would be
         * placed are unexplored.
         * @param force If true, placement will bypass the checks for valid placement.
         * @returns True if the special was placed, else false.
         */
        bool place_special( const overmap_special &special, const tripoint &p,
                            om_direction::type dir,
                            bool must_be_unexplored, bool force );
        /**
         * Place the specified overmap special using the overmap's placement algorithm. Intended to be used
         * when you have a special that you want placed but it should be placed similarly to as if it were
         * created during overmap generation.
         * @param special_id The id of overmap special to place.
         * @param center Used in conjunction with radius to search the specified and adjacent overmaps for
         * a valid placement location. Absolute overmap terrain coordinates.
         * @param radius Used in conjunction with center. Absolute overmap terrain units.
         * @returns True if the special was placed, else false.
         */
        bool place_special( const overmap_special_id &special_id, const tripoint &center, int radius );

    private:
        /**
         * Common function used by the find_closest/all/random to determine if the location is
         * findable based on the specified criteria.
         * @param location Location of search
         * see omt_find_params for definitions of the terms
         */
        bool is_findable_location( const tripoint &location, const omt_find_params &params );

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
        t_notes_vector get_notes( int z, const std::string *pattern );
        /**
         * Get a list of map extras in the (loaded) overmaps.
         * @param z only this specific z-level is search for map extras.
         * @param pattern only map extras that contain this pattern are returned.
         * If the pattern is NULL, every map extra matches.
         */
        t_extras_vector get_extras( int z, const std::string *pattern );
    public:
        /**
         * See overmap::check_ot, this uses global
         * overmap terrain coordinates.
         * This function may create a new overmap if needed.
         */
        bool check_ot( const std::string &otype, ot_match_type match_type, const tripoint &p );
        bool check_overmap_special_type( const overmap_special_id &id, const tripoint &loc );

        /**
        * These versions of the check_* methods will only check existing overmaps, and
        * return false if the overmap doesn't exist. They do not create new overmaps.
        */
        bool check_ot_existing( const std::string &otype, ot_match_type match_type, const tripoint &loc );
        bool check_overmap_special_type_existing( const overmap_special_id &id, const tripoint &loc );
    private:
        /**
         * Go thorough the monster groups of the overmap and move out-of-bounds
         * groups to the correct overmap (if it exists), also removes empty groups.
         */
        void fix_mongroups( overmap &new_overmap );
        /**
         * Moves out-of-bounds NPCs to the overmaps they should be in.
         */
        void fix_npcs( overmap &new_overmap );
        /**
         * Retrieve overmaps that overlap the bounding box defined by the location and radius.
         * The location is in absolute submap coordinates, the radius is in the same system.
         * The overmaps are returned sorted by distance from the provided location (closest first).
         */
        std::vector<overmap *> get_overmaps_near( const point &p, int radius );
        std::vector<overmap *> get_overmaps_near( const tripoint &location, int radius );
};

extern overmapbuffer overmap_buffer;

#endif
