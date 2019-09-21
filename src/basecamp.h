#pragma once
#ifndef BASECAMP_H
#define BASECAMP_H

#include <cstddef>
#include <list>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "inventory.h"
#include "point.h"

class JsonIn;
class JsonOut;
class npc;
class time_duration;

enum class farm_ops;
class item;
class map;
class recipe;
class mission_data;

struct expansion_data {
    std::string type;
    std::map<std::string, int> provides;
    std::map<std::string, int> in_progress;
    tripoint pos;
    // legacy camp level, replaced by provides map and set to -1
    int cur_level;

};

using npc_ptr = std::shared_ptr<npc>;
using comp_list = std::vector<npc_ptr>;
using Group_tag = std::string;
using itype_id = std::string;

namespace catacurses
{
class window;
} // namespace catacurses

namespace base_camps
{
const std::string base_dir = "[B]";
const std::string prefix = "faction_base_";
const std::string id = "FACTION_CAMP";
const int prefix_len = 13;
std::string faction_encode_short( const std::string &type );
std::string faction_encode_abs( const expansion_data &e, int number );
std::string faction_decode( const std::string &full_type );
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
    std::string name;
    bool avail = false;
    bool in_progress = false;
};

class basecamp
{
    public:
        basecamp();
        basecamp( const std::string &name_, const tripoint &omt_pos );
        basecamp( const std::string &name_, const tripoint &bb_pos_,
                  const std::vector<std::string> &directions_,
                  const std::map<std::string, expansion_data> &expansions_ );

        inline bool is_valid() const {
            return !name.empty() && omt_pos != tripoint_zero;
        }
        inline int board_x() const {
            return bb_pos.x;
        }
        inline int board_y() const {
            return bb_pos.y;
        }
        inline tripoint camp_omt_pos() const {
            return omt_pos;
        }
        inline const std::string &camp_name() const {
            return name;
        }
        std::string board_name() const;
        std::vector<std::string> directions;
        std::vector<tripoint> fortifications;
        std::string name;
        void faction_display( const catacurses::window &fac_w, const int width ) const;

        //change name of camp
        void set_name( const std::string &new_name );
        void query_new_name();
        void add_expansion( const std::string &terrain, const tripoint &new_pos );
        void add_expansion( const std::string &bldg, const tripoint &new_pos,
                            const std::string &dir );
        void define_camp( npc &p, const std::string &camp_type = "default" );

        std::string expansion_tab( const std::string &dir ) const;

        // upgrade levels
        bool has_provides( const std::string &req, const expansion_data &e_data, int level = 0 ) const;
        bool has_provides( const std::string &req, const std::string &dir = "all", int level = 0 ) const;
        void update_resources( const std::string &bldg );
        void update_provides( const std::string &bldg, expansion_data &e_data );
        void update_in_progress( const std::string &bldg, const std::string &dir );

        bool can_expand();
        /// Returns the name of the building the current building @ref dir upgrades into,
        /// "null" if there isn't one
        std::string next_upgrade( const std::string &dir, const int offset = 1 ) const;
        std::vector<basecamp_upgrade> available_upgrades( const std::string &dir );

        // camp utility functions
        int recruit_evaluation() const;
        int recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const;
        // confirm there is at least 1 loot destination and 1 unsorted loot zone in the camp
        bool validate_sort_points();
        // Validates the expansion data
        expansion_data parse_expansion( const std::string &terrain, const tripoint &new_pos );
        /**
         * Invokes the zone manager and validates that the necessary sort zones exist.
         */
        bool set_sort_points();

        // food utility
        /// Takes all the food from the camp_food zone and increases the faction
        /// food_supply
        bool distribute_food();

        // recipes, gathering, and craft support functions
        std::map<std::string, std::string> recipe_deck( const std::string &dir ) const;
        int recipe_batch_max( const recipe &making ) const;
        void form_crafting_inventory( const bool by_radio = false );
        void form_crafting_inventory( map &target_map );
        std::list<item> use_charges( const itype_id &fake_id, int &quantity );
        void consume_components( const recipe &making, int batch_size, bool by_radio = false );
        void consume_components( map &target_map, const recipe &making, int batch_size,
                                 bool by_radio = false );
        std::string get_gatherlist() const;
        /**
         * spawn items or corpses based on search attempts
         * @param skill skill level of the search
         * @param group_id name of the item_group that provides the items
         * @param attempts number of skill checks to make
         * @param difficulty a random number from 0 to difficulty is created for each attempt, and
         * if skill is higher, an item or corpse is spawned
         */
        void search_results( int skill, const Group_tag &group_id, int attempts, int difficulty,
                             bool by_radio = false );
        /**
         * spawn items or corpses based on search attempts
         * @param skill skill level of the search
         * @param task string to identify what types of corpses to provide ( _faction_camp_hunting
         * or _faction_camp_trapping )
         * @param attempts number of skill checks to make
         * @param difficulty a random number from 0 to difficulty is created for each attempt, and
         * if skill is higher, an item or corpse is spawned
         */
        void hunting_results( int skill, const std::string &task, int attempts, int difficulty,
                              bool by_radio = false );
        inline const tripoint &get_dumping_spot() const {
            return dumping_spot;
        }
        // dumping spot in absolute co-ords
        inline void set_dumping_spot( const tripoint &spot ) {
            dumping_spot = spot;
        }
        void place_results( item result, bool by_radio );

        // mission description functions
        void add_available_recipes( mission_data &mission_key, const std::string &dir,
                                    const std::map<std::string, std::string> &craft_recipes );

        std::string recruit_description( int npc_count );
        /// Provides a "guess" for some of the things your gatherers will return with
        /// to upgrade the camp
        std::string gathering_description( const std::string &bldg );
        /// Returns a string for the number of plants that are harvestable, plots ready to plany,
        /// and ground that needs tilling
        std::string farm_description( const tripoint &omt_pos, size_t &plots_count,
                                      farm_ops operation );
        /// Returns the description of a camp crafting options. converts fire charges to charcoal,
        /// allows dark crafting
        std::string craft_description( const std::string &itm );

        // main mission description collection
        void get_available_missions( mission_data &mission_key, bool by_radio = false );
        // available companion list manipulation
        void reset_camp_workers();
        comp_list get_mission_workers( const std::string &mission_id, bool contains = false );
        // main mission start/return dispatch function
        bool handle_mission( const std::string &miss_id, const std::string &miss_dir,
                             bool by_radio = false );

        // mission start functions
        /// generic mission start function that wraps individual mission
        npc_ptr start_mission( const std::string &miss_id, time_duration duration,
                               bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment,
                               const std::string &skill_tested, int skill_level );
        void start_upgrade( const std::string &bldg, const std::string &dir, const std::string &key,
                            bool by_radio );
        std::string om_upgrade_description( const std::string &bldg, bool trunc = false ) const;
        void start_menial_labor();
        void start_crafting( const std::string &cur_id, const std::string &cur_dir,
                             const std::string &type, const std::string &miss_id,
                             bool by_radio = false );

        /// Called when a companion is sent to cut logs
        void start_cut_logs();
        void start_clearcut();
        void start_setup_hide_site();
        void start_relay_hide_site();
        /// Called when a compansion is sent to start fortifications
        void start_fortifications( std::string &bldg_exp, bool by_radio );
        void start_combat_mission( const std::string &miss );
        /// Called when a companion starts a chop shop @ref task mission
        bool start_garage_chop( const std::string &dir, const tripoint &omt_tgt );
        void start_farm_op( const std::string &dir, const tripoint &omt_tgt, farm_ops op );
        ///Display items listed in @ref equipment to let the player pick what to give the departing
        ///NPC, loops until quit or empty.
        std::vector<item *> give_equipment( std::vector<item *> equipment, const std::string &msg );

        // mission return functions
        /// called to select a companion to return to the base
        npc_ptr companion_choose_return( const std::string &miss_id, time_duration min_duration );
        /// called with a companion @ref comp who is not the camp manager, finishes updating their
        /// skills, consuming food, and returning them to the base.
        void finish_return( npc &comp, bool fixed_time, const std::string &return_msg,
                            const std::string &skill, int difficulty );
        /// a wrapper function for @ref companion_choose_return and @ref finish_return
        npc_ptr mission_return( const std::string &miss_id, time_duration min_duration,
                                bool fixed_time, const std::string &return_msg,
                                const std::string &skill, int difficulty );
        /// Called to close upgrade missions, @ref miss is the name of the mission id
        /// and @ref dir is the direction of the location to be upgraded
        bool upgrade_return( const std::string &dir, const std::string &miss );
        /// As above, but with an explicit blueprint recipe to upgrade
        bool upgrade_return( const std::string &dir, const std::string &miss, const std::string &bldg );

        /// Choose which expansion you should start, called when a survey mission is completed
        bool survey_return();
        bool menial_return();
        /// Called when a companion completes a gathering @ref task mission
        bool gathering_return( const std::string &task, time_duration min_time,
                               bool by_radio = false );
        void recruit_return( const std::string &task, int score );
        /**
        * Perform any mix of the three farm tasks.
        * @param task
        * @param omt_trg the overmap pos3 of the farm_ops
        * @param op whether to plow, plant, or harvest
        */
        bool farm_return( const std::string &task, const tripoint &omt_trg, farm_ops op );
        void fortifications_return();

        void combat_mission_return( const std::string &miss );
        void validate_assignees();
        std::vector<npc_ptr> get_npcs_assigned();
        // Save/load
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
        void load_data( const std::string &data );
    private:
        // lazy re-evaluation of available camp resources
        void reset_camp_resources( bool by_radio = false );
        void add_resource( const itype_id &camp_resource );
        bool resources_updated = false;
        // omt pos
        tripoint omt_pos;
        std::vector<npc_ptr> assigned_npcs;
        // location of associated bulletin board
        tripoint bb_pos;
        std::map<std::string, expansion_data> expansions;
        comp_list camp_workers;
        tripoint dumping_spot;

        std::set<itype_id> fuel_types;
        std::vector<basecamp_fuel> fuels;
        std::vector<basecamp_resource> resources;
        static const int range = 20;
        inventory _inv;
};

#endif
