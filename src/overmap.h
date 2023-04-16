#pragma once
#ifndef CATA_SRC_OVERMAP_H
#define CATA_SRC_OVERMAP_H

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "city.h"
#include "coordinates.h"
#include "cube_direction.h"
#include "enums.h"
#include "game_constants.h"
#include "mapgendata.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "mongroup.h"
#include "overmap_types.h" // IWYU pragma: keep
#include "point.h"
#include "rng.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class JsonOut;
class cata_path;
class character_id;
class npc;
class overmap_connection;
struct regional_settings;

namespace pf
{
template<typename Point>
struct directed_path;
} // namespace pf

struct om_note {
    std::string text;
    point_om_omt p;
    bool dangerous = false;
    int danger_radius = 0;
};

struct om_map_extra {
    map_extra_id id;
    point_om_omt p;
};

struct om_vehicle {
    tripoint_om_omt p; // overmap coordinates of tracked vehicle
    std::string name;
};

enum class radio_type : int {
    MESSAGE_BROADCAST,
    WEATHER_RADIO
};

extern std::map<radio_type, std::string> radio_type_names;

constexpr int RADIO_MIN_STRENGTH = 120;
constexpr int RADIO_MAX_STRENGTH = 360;

struct radio_tower {
    // local (to the containing overmap) submap coordinates
    point_om_sm pos;
    int strength;
    radio_type type;
    std::string message;
    int frequency;
    explicit radio_tower( const point_om_sm &p, int S = -1, const std::string &M = "",
                          radio_type T = radio_type::MESSAGE_BROADCAST ) :
        pos( p ), strength( S ), type( T ), message( M ) {
        frequency = rng( 0, INT_MAX );
    }
};

struct map_layer {
    cata::mdarray<oter_id, point_om_omt> terrain;
    cata::mdarray<bool, point_om_omt> visible;
    cata::mdarray<bool, point_om_omt> explored;
    std::vector<om_note> notes;
    std::vector<om_map_extra> extras;
};

struct om_special_sectors {
    std::vector<point_om_omt> sectors;
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
        explicit overmap_special_batch( const point_abs_om &origin ) : origin_overmap( origin ) {}
        overmap_special_batch( const point_abs_om &origin,
                               const std::vector<const overmap_special *> &specials ) :
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
        std::vector<overmap_special_placement>::const_iterator begin() const {
            return placements.begin();
        }
        std::vector<overmap_special_placement>::iterator end() {
            return placements.end();
        }
        std::vector<overmap_special_placement>::const_iterator end() const {
            return placements.end();
        }
        std::vector<overmap_special_placement>::iterator erase(
            std::vector<overmap_special_placement>::iterator pos ) {
            return placements.erase( pos );
        }
        bool empty() {
            return placements.empty();
        }

        point_abs_om get_origin() const {
            return origin_overmap;
        }

    private:
        std::vector<overmap_special_placement> placements;
        point_abs_om origin_overmap;
};

template<typename Tripoint>
struct pos_dir {
    Tripoint p;
    cube_direction dir;

    pos_dir opposite() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonArray &ja );

    bool operator==( const pos_dir &r ) const;
    bool operator<( const pos_dir &r ) const;
};

extern template struct pos_dir<tripoint_om_omt>;
extern template struct pos_dir<tripoint_rel_omt>;

using om_pos_dir = pos_dir<tripoint_om_omt>;
using rel_pos_dir = pos_dir<tripoint_rel_omt>;

namespace std
{
template<typename Tripoint>
struct hash<pos_dir<Tripoint>> {
    size_t operator()( const pos_dir<Tripoint> &p ) const {
        cata::tuple_hash h;
        return h( std::make_tuple( p.p, p.dir ) );
    }
};
} // namespace std

class overmap
{
    public:
        overmap( const overmap & ) = default;
        overmap( overmap && ) = default;
        explicit overmap( const point_abs_om &p );
        ~overmap();

        overmap &operator=( const overmap & ) = default;

        /**
         * Create content in the overmap.
         **/
        void populate( overmap_special_batch &enabled_specials );
        void populate();

        const point_abs_om &pos() const {
            return loc;
        }

        void save() const;

        /**
         * @return The (local) overmap terrain coordinates of a randomly
         * chosen place on the overmap with the specific overmap terrain.
         * Returns @ref invalid_tripoint if no suitable place has been found.
         */
        tripoint_om_omt find_random_omt( const std::pair<std::string, ot_match_type> &target,
                                         std::optional<city> target_city = std::nullopt ) const;
        tripoint_om_omt find_random_omt( const std::string &omt_base_type,
                                         ot_match_type match_type = ot_match_type::type,
                                         std::optional<city> target_city = std::nullopt ) const {
            return find_random_omt( std::make_pair( omt_base_type, match_type ), std::move( target_city ) );
        }
        /**
         * Return a vector containing the absolute coordinates of
         * every matching terrain on the current z level of the current overmap.
         * @returns A vector of terrain coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching terrain is found.
         */
        std::vector<point_abs_omt> find_terrain( const std::string &term, int zlevel ) const;

        void ter_set( const tripoint_om_omt &p, const oter_id &id );
        // ter has bounds checking, and returns ot_null when out of bounds.
        const oter_id &ter( const tripoint_om_omt &p ) const;
        // ter_unsafe is UB when out of bounds.
        const oter_id &ter_unsafe( const tripoint_om_omt &p ) const;
        std::optional<mapgen_arguments> *mapgen_args( const tripoint_om_omt & );
        std::string *join_used_at( const om_pos_dir & );
        std::vector<oter_id> predecessors( const tripoint_om_omt & );
        void set_seen( const tripoint_om_omt &p, bool val );
        bool seen( const tripoint_om_omt &p ) const;
        bool &explored( const tripoint_om_omt &p );
        bool is_explored( const tripoint_om_omt &p ) const;

        bool has_note( const tripoint_om_omt &p ) const;
        bool is_marked_dangerous( const tripoint_om_omt &p ) const;
        const std::string &note( const tripoint_om_omt &p ) const;
        void add_note( const tripoint_om_omt &p, std::string message );
        void delete_note( const tripoint_om_omt &p );
        void mark_note_dangerous( const tripoint_om_omt &p, int radius, bool is_dangerous );

        bool has_extra( const tripoint_om_omt &p ) const;
        const map_extra_id &extra( const tripoint_om_omt &p ) const;
        void add_extra( const tripoint_om_omt &p, const map_extra_id &id );
        void add_extra_note( const tripoint_om_omt &p );
        void delete_extra( const tripoint_om_omt &p );

        /**
         * Getter for overmap scents.
         * @returns a reference to a scent_trace from the requested location.
         */
        const scent_trace &scent_at( const tripoint_abs_omt &loc ) const;
        /**
         * Setter for overmap scents, stores the provided scent at the provided location.
         */
        void set_scent( const tripoint_abs_omt &loc, const scent_trace &new_scent );

        /**
         * @returns Whether @param p is within desired bounds of the overmap
         * @param clearance Minimal distance from the edges of the overmap
         */
        static bool inbounds( const tripoint_om_omt &p, int clearance = 0 );
        static bool inbounds( const point_om_omt &p, int clearance = 0 ) {
            return inbounds( tripoint_om_omt( p, 0 ), clearance );
        }
        /**
         * Dummy value, used to indicate that a point returned by a function is invalid.
         */
        static constexpr tripoint_abs_omt invalid_tripoint{ tripoint_min };
        /**
         * Return a vector containing the absolute coordinates of
         * every matching note on the current z level of the current overmap.
         * @returns A vector of note coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching notes are found.
         */
        std::vector<point_abs_omt> find_notes( int z, const std::string &text );
        /**
         * Return a vector containing the absolute coordinates of
         * every matching map extra on the current z level of the current overmap.
         * @returns A vector of map extra coordinates (absolute overmap terrain
         * coordinates), or empty vector if no matching map extras are found.
         */
        std::vector<point_abs_omt> find_extras( int z, const std::string &text );

        /**
         * Returns whether or not the location has been generated (e.g. mapgen has run).
         * @param loc Location to check.
         * @returns True if param @loc has been generated.
         */
        bool is_omt_generated( const tripoint_om_omt &loc ) const;

        /** Returns the (0, 0) corner of the overmap in the global coordinates. */
        point_abs_omt global_base_point() const;

        // TODO: Should depend on coordinates
        const regional_settings &get_settings() const {
            return *settings;
        }

        void clear_mon_groups();
        void clear_overmap_special_placements();
        void clear_cities();
        void clear_connections_out();
        void place_special_forced( const overmap_special_id &special_id, const tripoint_om_omt &p,
                                   om_direction::type dir );
    private:
        std::multimap<tripoint_om_sm, mongroup> zg; // NOLINT(cata-serialize)
    public:
        /** Unit test enablers to check if a given mongroup is present. */
        bool mongroup_check( const mongroup &candidate ) const;
        bool monster_check( const std::pair<tripoint_om_sm, monster> &candidate ) const;

        // TODO: make private
        std::vector<radio_tower> radios;
        std::map<int, om_vehicle> vehicles;
        std::vector<basecamp> camps;
        std::vector<city> cities;
        std::map<string_id<overmap_connection>, std::vector<tripoint_om_omt>> connections_out;
        std::optional<basecamp *> find_camp( const point_abs_omt &p );
        /// Adds the npc to the contained list of npcs ( @ref npcs ).
        void insert_npc( const shared_ptr_fast<npc> &who );
        /// Removes the npc and returns it ( or returns nullptr if not found ).
        shared_ptr_fast<npc> erase_npc( const character_id &id );
        shared_ptr_fast<npc> find_npc( const character_id &id ) const;
        shared_ptr_fast<npc> find_npc_by_unique_id( const std::string &id ) const;
        const std::vector<shared_ptr_fast<npc>> &get_npcs() const {
            return npcs;
        }
        std::vector<shared_ptr_fast<npc>> get_npcs( const std::function<bool( const npc & )>
                                       &predicate )
                                       const;

    private:
        friend class overmapbuffer;

        std::vector<shared_ptr_fast<npc>> npcs;

        // A fake boolean that's returned for out-of-bounds calls to
        // overmap::seen and overmap::explored
        bool nullbool = false; // NOLINT(cata-serialize)
        point_abs_om loc; // NOLINT(cata-serialize)

        std::array<map_layer, OVERMAP_LAYERS> layer;
        std::unordered_map<tripoint_abs_omt, scent_trace> scents;

        // Records the locations where a given overmap special was placed, which
        // can be used after placement to lookup whether a given location was created
        // as part of a special.
        std::unordered_map<tripoint_om_omt, overmap_special_id> overmap_special_placements;
        // Records location where mongroups are not allowed to spawn during worldgen.
        // Reconstructed on load, so need not be serialized.
        std::unordered_set<tripoint_om_omt> safe_at_worldgen; // NOLINT(cata-serialize)

        // For oter_ts with the requires_predecessor flag, we need to store the
        // predecessor terrains so they can be used for mapgen later
        std::unordered_map<tripoint_om_omt, std::vector<oter_id>> predecessors_;

        // Records mapgen parameters required at the overmap special level
        // These are lazily evaluated; empty optional means that they have yet
        // to be evaluated.
        cata::colony<std::optional<mapgen_arguments>> mapgen_arg_storage;
        std::unordered_map<tripoint_om_omt, std::optional<mapgen_arguments> *> mapgen_args_index;

        // Records the joins that were chosen during placement of a mutable
        // special, so that it can be queried later by mapgen
        std::unordered_map<om_pos_dir, std::string> joins_used;

        const regional_settings *settings;

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
        std::unordered_multimap<tripoint_om_sm, monster> monster_map;

        // parse data in an opened overmap file
        void unserialize( const cata_path &file_name, std::istream &fin );
        void unserialize( const JsonObject &jsobj );
        // parse data in an opened omap file
        void unserialize_omap( const JsonValue &jsin, const cata_path &json_path );
        // Parse per-player overmap view data.
        void unserialize_view( const cata_path &file_name, std::istream &fin );
        void unserialize_view( const JsonObject &jsobj );
        // Save data in an opened overmap file
        void serialize( std::ostream &fout ) const;
        // Save per-player overmap view data.
        void serialize_view( std::ostream &fout ) const;
    private:
        void generate( const overmap *north, const overmap *east,
                       const overmap *south, const overmap *west,
                       overmap_special_batch &enabled_specials );
        bool generate_sub( int z );
        bool generate_over( int z );
        // Check and put bridgeheads
        void generate_bridgeheads( const std::vector<point_om_omt> &bridge_points );

        const city &get_nearest_city( const tripoint_om_omt &p ) const;

        void signal_hordes( const tripoint_rel_sm &p, int sig_power );
        void process_mongroups();
        void move_hordes();

        //nemesis movement for "hunted" trait
        void signal_nemesis( const tripoint_abs_sm & );
        void move_nemesis();
        void place_nemesis( const tripoint_abs_omt & );
        bool remove_nemesis(); // returns true if nemesis found and removed

        // Overall terrain
        void place_river( const point_om_omt &pa, const point_om_omt &pb );
        void place_forests();
        void place_lakes();
        void place_rivers( const overmap *north, const overmap *east, const overmap *south,
                           const overmap *west );
        void place_swamps();
        void place_forest_trails();
        void place_forest_trailheads();

        void place_roads( const overmap *north, const overmap *east, const overmap *south,
                          const overmap *west );

        void populate_connections_out_from_neighbors( const overmap *north, const overmap *east,
                const overmap *south, const overmap *west );

        // City Building
        overmap_special_id pick_random_building_to_place( int town_dist ) const;

        void place_cities();
        void place_building( const tripoint_om_omt &p, om_direction::type dir, const city &town );

        void build_city_street( const overmap_connection &connection, const point_om_omt &p, int cs,
                                om_direction::type dir, const city &town, int block_width = 2 );
        bool build_lab( const tripoint_om_omt &p, int s, std::vector<point_om_omt> *lab_train_points,
                        const std::string &prefix, int train_odds );
        bool build_slimepit( const tripoint_om_omt &origin, int s );
        void place_ravines();

        // Connection laying
        pf::directed_path<point_om_omt> lay_out_connection(
            const overmap_connection &connection, const point_om_omt &source,
            const point_om_omt &dest, int z, bool must_be_unexplored ) const;
        pf::directed_path<point_om_omt> lay_out_street(
            const overmap_connection &connection, const point_om_omt &source,
            om_direction::type dir, size_t len ) const;
    public:
        void build_connection(
            const overmap_connection &connection, const pf::directed_path<point_om_omt> &path, int z,
            cube_direction initial_dir = cube_direction::last );
        void build_connection( const point_om_omt &source, const point_om_omt &dest, int z,
                               const overmap_connection &connection, bool must_be_unexplored,
                               cube_direction initial_dir = cube_direction::last );
        void connect_closest_points( const std::vector<point_om_omt> &points, int z,
                                     const overmap_connection &connection );
        // Polishing
        bool check_ot( const std::string &otype, ot_match_type match_type,
                       const tripoint_om_omt &p ) const;
        bool check_overmap_special_type( const overmap_special_id &id,
                                         const tripoint_om_omt &location ) const;
        std::optional<overmap_special_id> overmap_special_at( const tripoint_om_omt &p ) const;
        void chip_rock( const tripoint_om_omt &p );

        void polish_river();
        void good_river( const tripoint_om_omt &p );

        om_direction::type random_special_rotation( const overmap_special &special,
                const tripoint_om_omt &p, bool must_be_unexplored ) const;

        bool can_place_special( const overmap_special &special, const tripoint_om_omt &p,
                                om_direction::type dir, bool must_be_unexplored ) const;

        std::vector<tripoint_om_omt> place_special(
            const overmap_special &special, const tripoint_om_omt &p, om_direction::type dir,
            const city &cit, bool must_be_unexplored, bool force );
    private:
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
                                  om_special_sectors &sectors, bool place_optional, bool must_be_unexplored );

        /**
         * Attempts to place specials within a sector.
         * @param enabled_specials vector of objects that track specials being placed.
         * @param sector sector identifies the location where specials are being placed.
         * @param place_optional restricts attempting to place specials that have met their minimum count in the first pass.
         */
        bool place_special_attempt(
            overmap_special_batch &enabled_specials, const point_om_omt &sector, int sector_width,
            bool place_optional, bool must_be_unexplored );

        void place_mongroups();
        void place_radios();

        void add_mon_group( const mongroup &group );
        void add_mon_group( const mongroup &group, int radius );
        // Spawns a new mongroup (to be called by worldgen code)
        void spawn_mon_group( const mongroup &group, int radius );

        void load_monster_groups( const JsonArray &jsin );
        void load_legacy_monstergroups( const JsonArray &jsin );
        void save_monster_groups( JsonOut &jo ) const;
    public:
        static void load_oter_id_migration( const JsonObject &jo );
        static void reset_oter_id_migrations();
        static bool is_oter_id_obsolete( const std::string &oterid );
        void migrate_oter_ids( const std::unordered_map<tripoint_om_omt, std::string> &points );
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
                  ot_match_type match_type );

/**
* Gets a collection of sectors and their width for usage in placing overmap specials.
* @param sector_width used to divide the OMAPX by OMAPY map into sectors.
*/
om_special_sectors get_sectors( int sector_width );

/**
* Returns the string of oter without any directional suffix
*/
std::string oter_no_dir( const oter_id &oter );

/**
* Return 0, 1, 2, 3 respectively if the suffix is _north, _west, _south, _east
* Return 0 if there's no suffix
*/
int oter_get_rotation( const oter_id &oter );

/**
* Return the directional suffix or "" if there isn't one.
*/
std::string oter_get_rotation_string( const oter_id &oter );
#endif // CATA_SRC_OVERMAP_H
