#pragma once
#ifndef CATA_SRC_BASECAMP_H
#define CATA_SRC_BASECAMP_H

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "coordinates.h"
#include "craft_command.h"
#include "game_inventory.h"
#include "inventory.h"
#include "map.h"
#include "mission_companion.h"
#include "point.h"
#include "requirements.h"
#include "type_id.h"

class JsonObject;
class JsonOut;
class character_id;
class npc;
class time_duration;
class zone_data;
enum class farm_ops : int;
class item;
class recipe;

const int work_day_hours = 10;
const int work_day_rest_hours = 8;
const int work_day_idle_hours = 6;

struct expansion_data {
    std::string type;
    std::vector<itype_id> available_pseudo_items;
    std::map<std::string, int> provides;
    std::map<std::string, int> in_progress;
    tripoint_abs_omt pos;
    // legacy camp level, replaced by provides map and set to -1
    int cur_level = 0;

};

using npc_ptr = shared_ptr_fast<npc>;
using comp_list = std::vector<npc_ptr>;

namespace catacurses
{
class window;
} // namespace catacurses

namespace base_camps
{

enum tab_mode : int {
    TAB_MAIN,
    TAB_N,
    TAB_NE,
    TAB_E,
    TAB_SE,
    TAB_S,
    TAB_SW,
    TAB_W,
    TAB_NW
};

struct direction_data {
    // used for composing mission ids
    std::string id;
    // tab order
    tab_mode tab_order;
    // such as [B], [NW], etc
    translation bracket_abbr;
    // MAIN, [NW], etc
    translation tab_title;
};

// base_dir and the eight directional points
extern const std::map<point, direction_data> all_directions;

point direction_from_id( const std::string &id );

constexpr point base_dir;
const std::string prefix = "faction_base_";
const std::string id = "FACTION_CAMP";
const int prefix_len = 13;
std::string faction_encode_short( const std::string &type );
std::string faction_encode_abs( const expansion_data &e, int number );
std::string faction_decode( std::string_view full_type );
time_duration to_workdays( const time_duration &work_time );
int max_upgrade_by_type( const std::string &type );
} // namespace base_camps

// camp resource structures
struct basecamp_resource {
    itype_id fake_id;
    itype_id ammo_id;
    int available = 0;
    int consumed = 0;
};

struct basecamp_fuel {
    itype_id ammo_id;
    int available = 0;
};

struct basecamp_upgrade {
    std::string bldg;
    mapgen_arguments args;
    translation name;
    bool avail = false;
    bool in_progress = false;
};

struct expansion_salt_water_pipe_segment {
    tripoint_abs_omt point;
    bool started;
    bool finished;
};

struct expansion_salt_water_pipe {
    point expansion;
    point connection_direction;
    std::vector<expansion_salt_water_pipe_segment> segments;
};

class basecamp_map
{
        friend basecamp;
    private:
        std::unique_ptr<map> map_;
    public:
        basecamp_map() = default;
        basecamp_map( const basecamp_map & );
        basecamp_map &operator=( const basecamp_map & );
};

class basecamp
{
    public:
        basecamp();
        basecamp( const std::string &name_, const tripoint_abs_omt &omt_pos );
        basecamp( const std::string &name_, const tripoint &bb_pos_,
                  const std::vector<point> &directions_,
                  const std::map<point, expansion_data> &expansions_ );
        inline bool is_valid() const {
            return !name.empty() && omt_pos != tripoint_abs_omt();
        }
        inline int board_x() const {
            return bb_pos.x;
        }
        inline int board_y() const {
            return bb_pos.y;
        }
        inline tripoint_abs_omt camp_omt_pos() const {
            return omt_pos;
        }
        inline const std::string &camp_name() const {
            return name;
        }
        tripoint get_bb_pos() const {
            return bb_pos;
        }
        void validate_bb_pos( const tripoint &new_abs_pos ) {
            if( bb_pos == tripoint_zero ) {
                bb_pos = new_abs_pos;
            }
        }
        void set_bb_pos( const tripoint &new_abs_pos ) {
            bb_pos = new_abs_pos;
        }
        void set_by_radio( bool access_by_radio );

        std::string board_name() const;
        std::vector<point> directions; // NOLINT(cata-serialize)
        std::vector<std::vector<ui_mission_id>> hidden_missions;
        std::vector<tripoint_abs_omt> fortifications;
        std::vector<expansion_salt_water_pipe *> salt_water_pipes;
        std::string name;
        void faction_display( const catacurses::window &fac_w, int width ) const;

        //change name of camp
        void set_name( const std::string &new_name );
        void query_new_name( bool force = false );
        void abandon_camp();
        void scan_pseudo_items();
        void add_expansion( const std::string &terrain, const tripoint_abs_omt &new_pos );
        void add_expansion( const std::string &bldg, const tripoint_abs_omt &new_pos,
                            const point &dir );
        void define_camp( const tripoint_abs_omt &p, std::string_view camp_type );

        std::string expansion_tab( const point &dir ) const;
        // check whether the point is the part of camp
        bool point_within_camp( const tripoint_abs_omt &p ) const;
        // upgrade levels
        bool has_provides( const std::string &req, const expansion_data &e_data, int level = 0 ) const;
        bool has_provides( const std::string &req, const std::optional<point> &dir = std::nullopt,
                           int level = 0 ) const;
        void update_resources( const std::string &bldg );
        void update_provides( const std::string &bldg, expansion_data &e_data );
        void update_in_progress( const std::string &bldg, const point &dir );

        /// Returns the name of the building the current building @ref dir upgrades into,
        /// "null" if there isn't one
        std::string next_upgrade( const point &dir, int offset = 1 ) const;
        std::vector<basecamp_upgrade> available_upgrades( const point &dir );

        // camp utility functions
        int recruit_evaluation() const;
        int recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const;
        // confirm there is at least 1 loot destination and 1 unsorted loot zone in the camp
        bool validate_sort_points();
        // Validates the expansion data
        expansion_data parse_expansion( std::string_view terrain,
                                        const tripoint_abs_omt &new_pos );
        /**
         * Invokes the zone manager and validates that the necessary sort zones exist.
         */
        bool set_sort_points();

        // food utility
        /// Takes all the food from the camp_food zone and increases the faction
        /// food_supply
        bool distribute_food();
        std::string name_display_of( const mission_id &miss_id );
        void handle_hide_mission( const point &dir );
        void handle_reveal_mission( const point &dir );
        bool has_water() const;

        // recipes, gathering, and craft support functions
        // from a direction
        std::map<recipe_id, translation> recipe_deck( const point &dir ) const;
        // from a building
        std::map<recipe_id, translation> recipe_deck( const std::string &bldg ) const;
        int recipe_batch_max( const recipe &making ) const;
        void form_crafting_inventory();
        void form_crafting_inventory( map &target_map );
        std::list<item> use_charges( const itype_id &fake_id, int &quantity );
        /**
         * spawn items or corpses based on search attempts
         * @param skill skill level of the search
         * @param group_id name of the item_group that provides the items
         * @param attempts number of skill checks to make
         * @param difficulty a random number from 0 to difficulty is created for each attempt, and
         * if skill is higher, an item or corpse is spawned
         */
        void search_results( int skill, const item_group_id &, int attempts, int difficulty );
        /**
         * spawn items or corpses based on search attempts
         * @param skill skill level of the search
         * @param task string to identify what types of corpses to provide ( _faction_camp_hunting
         * or _faction_camp_trapping )
         * @param attempts number of skill checks to make
         * @param difficulty a random number from 0 to difficulty is created for each attempt, and
         * if skill is higher, an item or corpse is spawned
         */
        void hunting_results( int skill, const mission_id &miss_id, int attempts, int difficulty );
        inline const tripoint_abs_ms &get_dumping_spot() const {
            return dumping_spot;
        }
        inline const std::vector<tripoint_abs_ms> &get_liquid_dumping_spot() const {
            return liquid_dumping_spots;
        }
        // dumping spot in absolute co-ords
        inline void set_dumping_spot( const tripoint_abs_ms &spot ) {
            dumping_spot = spot;
        }
        inline void set_liquid_dumping_spot( const std::vector<tripoint_abs_ms> &liquid_dumps ) {
            // Nowhere qualified to dump liquid? Dump it wherever everything else goes.
            if( liquid_dumps.empty() ) {
                liquid_dumping_spots.clear();
                liquid_dumping_spots.emplace_back( dumping_spot );
                return;
            } //else
            liquid_dumping_spots.clear();
            liquid_dumping_spots = liquid_dumps;
        }
        void place_results( const item &result );

        // mission description functions
        void add_available_recipes( mission_data &mission_key, mission_kind kind, const point &dir,
                                    const std::map<recipe_id, translation> &craft_recipes );

        std::string recruit_description( int npc_count ) const;
        /// Provides a "guess" for some of the things your gatherers will return with
        /// to upgrade the camp
        std::string gathering_description();
        /// Returns a string for the number of plants that are harvestable, plots ready to plant,
        /// and ground that needs tilling
        std::string farm_description( const tripoint_abs_omt &farm_pos, size_t &plots_count,
                                      farm_ops operation );
        /// Returns the description of a camp crafting options. converts fire charges to charcoal,
        /// allows dark crafting
        std::string craft_description( const recipe_id &itm );

        // main mission description collection
        void get_available_missions( mission_data &mission_key, map &here );
        void get_available_missions_by_dir( mission_data &mission_key, const point &dir );
        void choose_new_leader();
        // available companion list manipulation
        void reset_camp_workers();
        comp_list get_mission_workers( const mission_id &miss_id, bool contains = false );
        // main mission start/return dispatch function
        bool handle_mission( const ui_mission_id &miss_id );

        // mission start functions
        /// generic mission start function that wraps individual mission
        npc_ptr start_mission( const mission_id &miss_id, time_duration duration,
                               bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment,
                               const skill_id &skill_tested, int skill_level, float exertion_level );
        npc_ptr start_mission( const mission_id &miss_id, time_duration duration,
                               bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment, float exertion_level,
                               const std::map<skill_id, int> &required_skills = {} );
        comp_list start_multi_mission( const mission_id &miss_id,
                                       bool must_feed, const std::string &desc,
                                       // const std::vector<item*>& equipment, //  No support for extracting equipment from recipes currently..
                                       const skill_id &skill_tested, int skill_level );
        comp_list start_multi_mission( const mission_id &miss_id,
                                       bool must_feed, const std::string &desc,
                                       //  const std::vector<item*>& equipment, //  No support for extracting equipment from recipes currently..
                                       const std::map<skill_id, int> &required_skills = {} );
        void start_upgrade( const mission_id &miss_id );
        std::string om_upgrade_description( const std::string &bldg, const mapgen_arguments &,
                                            bool trunc = false ) const;
        void start_menial_labor();
        void worker_assignment_ui();
        void job_assignment_ui();
        void start_crafting( const std::string &type, const mission_id &miss_id );

        /// Called when a companion is sent to cut logs
        void start_cut_logs( const mission_id &miss_id, float exertion_level );
        void start_clearcut( const mission_id &miss_id, float exertion_level );
        void start_setup_hide_site( const mission_id &miss_id, float exertion_level );
        void start_relay_hide_site( const mission_id &miss_id, float exertion_level );
        /// Called when a companion is sent to start fortifications
        void start_fortifications( const mission_id &miss_id, float exertion_level );
        /// Called when a companion is sent to start digging down salt water pipes
        bool common_salt_water_pipe_construction( const mission_id &miss_id,
                expansion_salt_water_pipe *pipe,
                int segment_number ); //  Code factored out from the two following operation, not intended to be used elsewhere.
        void start_salt_water_pipe( const mission_id &miss_id );
        void continue_salt_water_pipe( const mission_id &miss_id );
        void start_combat_mission( const mission_id &miss_id, float exertion_level );
        void start_farm_op( const tripoint_abs_omt &omt_tgt, const mission_id &miss_id,
                            float exertion_level );
        ///Display items listed in @ref equipment to let the player pick what to give the departing
        ///NPC, loops until quit or empty.
        drop_locations give_basecamp_equipment( inventory_filter_preset &preset, const std::string &title,
                                                const std::string &column_title, const std::string &msg_empty ) const;
        drop_locations give_equipment( Character *pc, const inventory_filter_preset &preset,
                                       const std::string &msg, const std::string &title, units::volume &total_volume,
                                       units::mass &total_mass );
        drop_locations get_equipment( tinymap *target_bay, const tripoint &target, Character *pc,
                                      const inventory_filter_preset &preset,
                                      const std::string &msg, const std::string &title, units::volume &total_volume,
                                      units::mass &total_mass );

        // mission return functions
        /// called to select a companion to return to the base
        npc_ptr companion_choose_return( const mission_id &miss_id, time_duration min_duration );
        npc_ptr companion_crafting_choose_return( const mission_id &miss_id );
        /// called with a companion @ref comp who is not the camp manager, finishes updating their
        /// skills, consuming food, and returning them to the base.
        void finish_return( npc &comp, bool fixed_time, const std::string &return_msg,
                            const std::string &skill, int difficulty, bool cancel = false );
        /// a wrapper function for @ref companion_choose_return and @ref finish_return
        npc_ptr mission_return( const mission_id &miss_id, time_duration min_duration,
                                bool fixed_time, const std::string &return_msg,
                                const std::string &skill, int difficulty );
        npc_ptr crafting_mission_return( const mission_id &miss_id, const std::string &return_msg,
                                         const std::string &skill, int difficulty );
        /// select a companion for any mission to return to base
        npc_ptr emergency_recall( const mission_id &miss_id );

        /// Called to close upgrade missions, @ref miss is the name of the mission id
        /// and @ref dir is the direction of the location to be upgraded
        bool upgrade_return( const mission_id &miss_id );

        /// Choose which expansion slot to check for field conversion
        bool survey_field_return( const mission_id &miss_id );

        /// Choose which expansion you should start, called when a survey mission is completed
        bool survey_return( const mission_id &miss_id );
        bool menial_return( const mission_id &miss_id );
        /// Called when a companion completes a gathering @ref task mission
        bool gathering_return( const mission_id &miss_id, time_duration min_time );
        void recruit_return( const mission_id &miss_id, int score );
        /**
        * Perform any mix of the three farm tasks.
        * @param task
        * @param omt_tgt the overmap pos3 of the farm_ops
        * @param op whether to plow, plant, or harvest
        */
        bool farm_return( const mission_id &miss_id, const tripoint_abs_omt &omt_tgt );
        void fortifications_return( const mission_id &miss_id );
        bool salt_water_pipe_swamp_return( const mission_id &miss_id,
                                           const comp_list &npc_list );
        bool salt_water_pipe_return( const mission_id &miss_id,
                                     const comp_list &npc_list );

        void combat_mission_return( const mission_id &miss_id );
        void validate_assignees();
        void add_assignee( character_id id );
        void remove_assignee( character_id id );
        std::vector<npc_ptr> get_npcs_assigned();
        void hide_mission( ui_mission_id id );
        void reveal_mission( ui_mission_id id );
        bool is_hidden( ui_mission_id id );
        // Save/load
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        void load_data( const std::string &data );
        inline const std::vector<const zone_data * > &get_storage_zone() const {
            return storage_zones;
        }
        // dumping spot in absolute co-ords
        inline void set_storage_zone( const std::vector<const zone_data *> &zones ) {
            storage_zones = zones;
        }
        inline const std::unordered_set<tripoint_abs_ms> &get_storage_tiles() const {
            return src_set;
        }
        // dumping spot in absolute co-ords
        inline void set_storage_tiles( const std::unordered_set<tripoint_abs_ms> &tiles ) {
            src_set = tiles;
        }
        void form_storage_zones( map &here, const tripoint_abs_ms &abspos );
        map &get_camp_map();
        void unload_camp_map();
    private:
        friend class basecamp_action_components;

        // lazy re-evaluation of available camp resources
        void reset_camp_resources( map &here );
        void add_resource( const itype_id &camp_resource );
        // omt pos
        tripoint_abs_omt omt_pos;
        std::vector<npc_ptr> assigned_npcs; // NOLINT(cata-serialize)
        // location of associated bulletin board in abs coords
        tripoint bb_pos;
        std::map<point, expansion_data> expansions;
        comp_list camp_workers; // NOLINT(cata-serialize)
        basecamp_map camp_map; // NOLINT(cata-serialize)
        tripoint_abs_ms dumping_spot;
        // Tiles inside STORAGE-type zones that have LIQUIDCONT terrain
        std::vector<tripoint_abs_ms> liquid_dumping_spots;
        std::vector<const zone_data *> storage_zones; // NOLINT(cata-serialize)
        std::unordered_set<tripoint_abs_ms> src_set; // NOLINT(cata-serialize)
        std::set<itype_id> fuel_types; // NOLINT(cata-serialize)
        std::vector<basecamp_fuel> fuels; // NOLINT(cata-serialize)
        std::vector<basecamp_resource> resources; // NOLINT(cata-serialize)
        std::vector<std::vector<ui_mission_id>> temp_ui_mission_keys;   // NOLINT(cata-serialize)
        inventory _inv; // NOLINT(cata-serialize)
        bool by_radio = false; // NOLINT(cata-serialize)
};

class basecamp_action_components
{
    public:
        basecamp_action_components( const recipe &making, const mapgen_arguments &, int batch_size,
                                    basecamp & );

        // Returns true iff all necessary components were successfully chosen
        bool choose_components();
        void consume_components();
    private:
        const recipe &making_;
        const mapgen_arguments &args_;
        int batch_size_;
        basecamp &base_;
        std::vector<comp_selection<item_comp>> item_selections_;
        std::vector<comp_selection<tool_comp>> tool_selections_;
};

#endif // CATA_SRC_BASECAMP_H
