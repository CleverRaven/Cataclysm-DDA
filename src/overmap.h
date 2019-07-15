#pragma once
#ifndef OVERMAP_H
#define OVERMAP_H

#include <cstdlib>
#include <algorithm>
#include <array>
#include <climits>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <iterator>
#include <utility>

#include "basecamp.h"
#include "game_constants.h"
#include "omdata.h"
#include "overmap_types.h" // IWYU pragma: keep
#include "regional_settings.h"
#include "enums.h"
#include "mongroup.h"
#include "optional.h"
#include "type_id.h"
#include "point.h"
#include "rng.h"
#include "string_id.h"

class npc;
class overmap_connection;
class JsonIn;
class JsonOut;
class monster;
class JsonObject;
class map_extra;

namespace pf
{
struct path;
} // namespace pf

struct city {
    // location of the city (in overmap terrain coordinates)
    point pos;
    int size;
    std::string name;
    city( const point &P = point_zero, const int S = -1 );
    city( const int X, const int Y, const int S ) : city( point( X, Y ), S ) {}

    operator bool() const {
        return size >= 0;
    }

    int get_distance_from( const tripoint &p ) const;
};

struct om_note {
    std::string text;
    int         x;
    int         y;
};

struct om_map_extra {
    string_id<map_extra> id;
    int                  x;
    int                  y;
};

struct om_vehicle {
    point p; // overmap coordinates of tracked vehicle
    std::string name;
};

enum radio_type {
    MESSAGE_BROADCAST,
    WEATHER_RADIO
};

extern std::map<enum radio_type, std::string> radio_type_names;

#define RADIO_MIN_STRENGTH 80
#define RADIO_MAX_STRENGTH 200

struct radio_tower {
    // local (to the containing overmap) submap coordinates
    int x;
    int y;
    int strength;
    radio_type type;
    std::string message;
    int frequency;
    radio_tower( int X = -1, int Y = -1, int S = -1, const std::string &M = "",
                 radio_type T = MESSAGE_BROADCAST ) :
        x( X ), y( Y ), strength( S ), type( T ), message( M ) {
        frequency = rng( 0, INT_MAX );
    }
};

struct map_layer {
    oter_id terrain[OMAPX][OMAPY];
    bool visible[OMAPX][OMAPY];
    bool explored[OMAPX][OMAPY];
    std::vector<om_note> notes;
    std::vector<om_map_extra> extras;
};

struct om_special_sectors {
    std::vector<point> sectors;
    int sector_width;
};

// Wrapper around an overmap special to track progress of placing specials.
struct overmap_special_placement {
    int instances_placed;
    const overmap_special *special_details;
};

// A batch of overmap specials to place.
class overmap_special_batch
{
    public:
        overmap_special_batch( const point &origin ) : origin_overmap( origin ) {}
        overmap_special_batch( const point &origin, const std::vector<const overmap_special *> &specials ) :
            origin_overmap( origin ) {
            std::transform( specials.begin(), specials.end(),
            std::back_inserter( placements ), []( const overmap_special * elem ) {
                return overmap_special_placement{ 0, elem };
            } );
        }

        // Wrapper methods that make overmap_special_batch act like
        // the underlying vector of overmap placements.
        std::vector<overmap_special_placement>::iterator begin() {
            return placements.begin();
        }
        std::vector<overmap_special_placement>::iterator end() {
            return placements.end();
        }
        std::vector<overmap_special_placement>::iterator erase(
            std::vector<overmap_special_placement>::iterator pos ) {
            return placements.erase( pos );
        }
        bool empty() {
            return placements.empty();
        }

        point get_origin() const {
            return origin_overmap;
        }

    private:
        std::vector<overmap_special_placement> placements;
        point origin_overmap;
};

class overmap
{
    public:
        overmap( const overmap & ) = default;
        overmap( overmap && ) = default;
        overmap( int x, int y );
        overmap( const point &p ) : overmap( p.x, p.y ) {}
        ~overmap();

        overmap &operator=( const overmap & ) = default;

        /**
         * Create content in the overmap.
         **/
        void populate( overmap_special_batch &enabled_specials );
        void populate();

        const point &pos() const {
            return loc;
        }

        void save() const;

        /**
         * @return The (local) overmap terrain coordinates of a randomly
         * chosen place on the overmap with the specific overmap terrain.
         * Returns @ref invalid_tripoint if no suitable place has been found.
         */
        tripoint find_random_omt( const std::string &omt_base_type ) const;
        /**
         * Return a vector containing the absolute coordinates of
         * every matching terrain on the current z level of the current overmap.
         * @returns A vector of terrain coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching terrain is found.
         */
        std::vector<point> find_terrain( const std::string &term, int zlevel );

        oter_id &ter( const int x, const int y, const int z );
        oter_id &ter( const tripoint &p );
        const oter_id get_ter( const int x, const int y, const int z ) const;
        const oter_id get_ter( const tripoint &p ) const;
        bool &seen( int x, int y, int z );
        bool &seen( const tripoint &p ) {
            return seen( p.x, p.y, p.z );
        }
        bool seen( int x, int y, int z ) const;
        bool seen( const tripoint &p ) const {
            return seen( p.x, p.y, p.z );
        }
        bool &explored( int x, int y, int z );
        bool &explored( const tripoint &p ) {
            return explored( p.x, p.y, p.z );
        }
        bool is_explored( const int x, const int y, const int z ) const;
        bool is_explored( const tripoint &p ) const {
            return is_explored( p.x, p.y, p.z );
        }

        bool has_note( int x, int y, int z ) const;
        bool has_note( const tripoint &p ) const {
            return has_note( p.x, p.y, p.z );
        }
        const std::string &note( int x, int y, int z ) const;
        const std::string &note( const tripoint &p ) const {
            return note( p.x, p.y, p.z );
        }
        void add_note( int x, int y, int z, std::string message );
        void add_note( const tripoint &p, std::string message ) {
            add_note( p.x, p.y, p.z, message );
        }
        void delete_note( int x, int y, int z );
        void delete_note( const tripoint &p ) {
            delete_note( p.x, p.y, p.z );
        }

        bool has_extra( int x, int y, int z ) const;
        bool has_extra( const tripoint &p ) const {
            return has_extra( p.x, p.y, p.z );
        }
        const string_id<map_extra> &extra( int x, int y, int z ) const;
        const string_id<map_extra> &extra( const tripoint &p ) const {
            return extra( p.x, p.y, p.z );
        }
        void add_extra( int x, int y, int z, const string_id<map_extra> &id );
        void add_extra( const tripoint &p, const string_id<map_extra> &id ) {
            add_extra( p.x, p.y, p.z, id );
        }
        void delete_extra( int x, int y, int z );
        void delete_extra( const tripoint &p ) {
            delete_extra( p.x, p.y, p.z );
        }

        /**
         * Getter for overmap scents.
         * @returns a reference to a scent_trace from the requested location.
         */
        const scent_trace &scent_at( const tripoint &loc ) const;
        /**
         * Setter for overmap scents, stores the provided scent at the provided location.
         */
        void set_scent( const tripoint &loc, const scent_trace &new_scent );

        /**
         * @returns Whether @param p is within desired bounds of the overmap
         * @param clearance Minimal distance from the edges of the overmap
         */
        static bool inbounds( const tripoint &p, int clearance = 0 );
        static bool inbounds( const point &p, int clearance = 0 ) {
            return inbounds( tripoint( p, 0 ), clearance );
        }
        /**
         * Dummy value, used to indicate that a point returned by a function is invalid.
         */
        static constexpr tripoint invalid_tripoint = tripoint_min;
        /**
         * Return a vector containing the absolute coordinates of
         * every matching note on the current z level of the current overmap.
         * @returns A vector of note coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching notes are found.
         */
        std::vector<point> find_notes( const int z, const std::string &text );
        /**
         * Return a vector containing the absolute coordinates of
         * every matching map extra on the current z level of the current overmap.
         * @returns A vector of map extra coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching map extras are found.
         */
        std::vector<point> find_extras( const int z, const std::string &text );

        /**
         * Returns whether or not the location has been generated (e.g. mapgen has run).
         * @param loc Location to check.
         * @returns True if param @loc has been generated.
         */
        bool is_omt_generated( const tripoint &loc ) const;

        /** Returns the (0, 0) corner of the overmap in the global coordinates. */
        point global_base_point() const;

        // TODO: Should depend on coordinates
        const regional_settings &get_settings() const {
            return settings;
        }

        void clear_mon_groups();
    private:
        std::multimap<tripoint, mongroup> zg;
    public:
        /** Unit test enablers to check if a given mongroup is present. */
        bool mongroup_check( const mongroup &candidate ) const;
        bool monster_check( const std::pair<tripoint, monster> &candidate ) const;

        // TODO: make private
        std::vector<radio_tower> radios;
        std::map<int, om_vehicle> vehicles;
        std::vector<basecamp> camps;
        std::vector<city> cities;
        std::vector<city> roads_out;
        cata::optional<basecamp *> find_camp( const int x, const int y );
        cata::optional<basecamp *> find_camp( const point &p ) {
            return find_camp( p.x, p.y );
        }
        /// Adds the npc to the contained list of npcs ( @ref npcs ).
        void insert_npc( std::shared_ptr<npc> who );
        /// Removes the npc and returns it ( or returns nullptr if not found ).
        std::shared_ptr<npc> erase_npc( const int id );

        void for_each_npc( const std::function<void( npc & )> &callback );
        void for_each_npc( const std::function<void( const npc & )> &callback ) const;

        std::shared_ptr<npc> find_npc( int id ) const;

        const std::vector<std::shared_ptr<npc>> &get_npcs() const {
            return npcs;
        }
        std::vector<std::shared_ptr<npc>> get_npcs( const std::function<bool( const npc & )> &predicate )
                                       const;

    private:
        friend class overmapbuffer;

        std::vector<std::shared_ptr<npc>> npcs;

        bool nullbool = false;
        point loc = point_zero;

        std::array<map_layer, OVERMAP_LAYERS> layer;
        std::unordered_map<tripoint, scent_trace> scents;

        // Records the locations where a given overmap special was placed, which
        // can be used after placement to lookup whether a given location was created
        // as part of a special.
        std::unordered_map<tripoint, overmap_special_id> overmap_special_placements;

        regional_settings settings;

        oter_id get_default_terrain( int z ) const;

        // Initialize
        void init_layers();
        // open existing overmap, or generate a new one
        void open( overmap_special_batch &enabled_specials );
    public:

        /**
         * When monsters despawn during map-shifting they will be added here.
         * map::spawn_monsters will load them and place them into the reality bubble
         * (adding it to the creature tracker and putting it onto the map).
         * This stores each submap worth of monsters in a different bucket of the multimap.
         */
        std::unordered_multimap<tripoint, monster> monster_map;

        // parse data in an opened overmap file
        void unserialize( std::istream &fin );
        // Parse per-player overmap view data.
        void unserialize_view( std::istream &fin );
        // Save data in an opened overmap file
        void serialize( std::ostream &fin ) const;
        // Save per-player overmap view data.
        void serialize_view( std::ostream &fin ) const;
    private:
        void generate( const overmap *north, const overmap *east,
                       const overmap *south, const overmap *west,
                       overmap_special_batch &enabled_specials );
        bool generate_sub( const int z );

        const city &get_nearest_city( const tripoint &p ) const;

        void signal_hordes( const tripoint &p, int sig_power );
        void process_mongroups();
        void move_hordes();

        static bool obsolete_terrain( const std::string &ter );
        void convert_terrain( const std::unordered_map<tripoint, std::string> &needs_conversion );

        // Overall terrain
        void place_river( point pa, point pb );
        void place_forests();
        void place_lakes();
        void place_rivers( const overmap *north, const overmap *east, const overmap *south,
                           const overmap *west );
        void place_swamps();
        void place_forest_trails();
        void place_forest_trailheads();

        void place_roads( const overmap *north, const overmap *east, const overmap *south,
                          const overmap *west );

        // City Building
        overmap_special_id pick_random_building_to_place( int town_dist ) const;

        void place_cities();
        void place_building( const tripoint &p, om_direction::type dir, const city &town );

        void build_city_street( const overmap_connection &connection, const point &p, int cs,
                                om_direction::type dir, const city &town, int block_width = 2 );
        bool build_lab( int x, int y, int z, int s, std::vector<point> *lab_train_points,
                        const std::string &prefix, int train_odds );
        void build_anthill( int x, int y, int z, int s );
        void build_tunnel( int x, int y, int z, int s, om_direction::type dir );
        bool build_slimepit( int x, int y, int z, int s );
        void build_mine( int x, int y, int z, int s );
        void place_rifts( const int z );

        // Connection laying
        pf::path lay_out_connection( const overmap_connection &connection, const point &source,
                                     const point &dest, int z, const bool must_be_unexplored ) const;
        pf::path lay_out_street( const overmap_connection &connection, const point &source,
                                 om_direction::type dir, size_t len ) const;

        void build_connection( const overmap_connection &connection, const pf::path &path, int z,
                               const om_direction::type &initial_dir = om_direction::type::invalid );
        void build_connection( const point &source, const point &dest, int z,
                               const overmap_connection &connection, const bool must_be_unexplored,
                               const om_direction::type &initial_dir = om_direction::type::invalid );
        void connect_closest_points( const std::vector<point> &points, int z,
                                     const overmap_connection &connection );
        // Polishing
        bool check_ot( const std::string &otype, ot_match_type match_type, int x, int y, int z ) const;
        bool check_ot( const std::string &otype, ot_match_type match_type, const tripoint &p ) const {
            return check_ot( otype, match_type, p.x, p.y, p.z );
        }
        bool check_overmap_special_type( const overmap_special_id &id, const tripoint &location ) const;
        void chip_rock( int x, int y, int z );

        void polish_river();
        void good_river( int x, int y, int z );

        om_direction::type random_special_rotation( const overmap_special &special,
                const tripoint &p, bool must_be_unexplored ) const;

        bool can_place_special( const overmap_special &special, const tripoint &p,
                                om_direction::type dir, const bool must_be_unexplored ) const;

        void place_special( const overmap_special &special, const tripoint &p, om_direction::type dir,
                            const city &cit, const bool must_be_unexplored, const bool force );
        /**
         * Iterate over the overmap and place the quota of specials.
         * If the stated minimums are not reached, it will spawn a new nearby overmap
         * and continue placing specials there.
         * @param enabled_specials specifies what specials to place, and tracks how many have been placed.
         **/
        void place_specials( overmap_special_batch &enabled_specials );
        /**
         * Walk over the overmap and attempt to place specials.
         * @param enabled_specials vector of objects that track specials being placed.
         * @param sectors sectors in which to attempt placement.
         * @param place_optional restricts attempting to place specials that have met their minimum count in the first pass.
         */
        void place_specials_pass( overmap_special_batch &enabled_specials,
                                  om_special_sectors &sectors, bool place_optional, const bool must_be_unexplored );

        /**
         * Attempts to place specials within a sector.
         * @param enabled_specials vector of objects that track specials being placed.
         * @param sector sector identifies the location where specials are being placed.
         * @param place_optional restricts attempting to place specials that have met their minimum count in the first pass.
         */
        bool place_special_attempt( overmap_special_batch &enabled_specials,
                                    const point &sector, const int sector_width, bool place_optional, const bool must_be_unexplored );

        void place_mongroups();
        void place_radios();

        void add_mon_group( const mongroup &group );

        void load_monster_groups( JsonIn &jo );
        void load_legacy_monstergroups( JsonIn &jo );
        void save_monster_groups( JsonOut &jo ) const;
    public:
        static void load_obsolete_terrains( JsonObject &jo );
};

bool is_river( const oter_id &ter );
bool is_river_or_lake( const oter_id &ter );

/**
* Determine if the provided name is a match with the provided overmap terrain
* based on the specified match type.
* @param name is the name we're looking for.
* @param oter is the overmap terrain id we're comparing our name with.
* @param match_type is the matching rule to use when comparing the two values.
*/
bool is_ot_match( const std::string &name, const oter_id &oter,
                  const ot_match_type match_type );

/**
* Gets a collection of sectors and their width for usage in placing overmap specials.
* @param sector_width used to divide the OMAPX by OMAPY map into sectors.
*/
om_special_sectors get_sectors( const int sector_width );

#endif
