#pragma once
#ifndef BASECAMP_H
#define BASECAMP_H

#include <memory>
#include <vector>
#include <map>
#include <string>

#include "enums.h"
#include "mission_companion.h"

class JsonIn;
class JsonOut;
class npc;
class time_duration;
enum class farm_ops;
class item;
class recipe;
class inventory;

struct expansion_data {
    std::string type;
    int cur_level;
    tripoint pos;
};

using npc_ptr = std::shared_ptr<npc>;
using comp_list = std::vector<npc_ptr>;

class basecamp
{
    public:
        basecamp();
        basecamp( const std::string &name_, const tripoint &pos_ );
        basecamp( const std::string &name_, const tripoint &bb_pos_, const tripoint &pos_,
                  std::vector<tripoint> sort_points_, std::vector<std::string> directions_,
                  std::map<std::string, expansion_data> expansions_ );

        inline bool is_valid() const {
            return !name.empty() && pos != tripoint_zero;
        }
        inline int board_x() const {
            return bb_pos.x;
        }
        inline int board_y() const {
            return bb_pos.y;
        }
        tripoint camp_pos() const {
            return pos;
        }
        inline const std::string &camp_name() const {
            return name;
        }
        std::string board_name() const;
        std::vector<tripoint> sort_points;
        std::vector<std::string> directions;

        void add_expansion( const std::string &terrain, const tripoint &new_pos );
        void define_camp( npc &p );
        void define_camp( const tripoint &omt_pos );

        std::string expansion_tab( const std::string &dir ) const;

        // upgrade levels
        bool has_level( const std::string &type, int min_level, const std::string &dir ) const;
        bool any_has_level( const std::string &type, int min_level ) const;
        bool can_expand() const;
        /// Returns the name of the building the current building @ref dir upgrades into, "null" if there isn't one
        const std::string next_upgrade( const std::string &dir ) const;
        /// Improve the camp tile to the next level and pushes the camp manager onto his correct position in case he moved
        bool om_upgrade( npc &comp, const std::string &next_upgrade, const tripoint &upos );

        // camp utility functions
        int recruit_evaluation() const;
        int recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const;
        void validate_sort_points();
        /**
         * Sets the location of the sorting piles used above.
         * @param reset_pts reverts all previous points to defaults.
         * @param choose_pts let the player review and choose new sort points
         */
        bool set_sort_points( bool reset_pts, bool choose_pts );

        // food utility
        /// Takes all the food from the point set in set_sort_pts() and increases the faction
        /// food_supply
        bool distribute_food();

        // recipes and craft support functions
        std::map<std::string, std::string> recipe_deck( const std::string &dir ) const;
        int recipe_batch_max( const recipe &making, const inventory &total_inv ) const;
        void craft_construction( const tripoint &omt_pos, const std::string &cmri, const std::string &cur_id, 
                    const std::string &cur_dir, const std::string &type, const std::string &miss_id );
        const std::string get_gatherlist() const;

        // mission description functions
        std::string recruit_description( int npc_count );

        // main mission start/return dispatch function
        bool handle_mission( npc &p, const std::string &miss_id, const std::string &miss_dir );
        bool handle_mission( const tripoint &omt_pos, const std::string &cmri,
                    const std::string &miss_id, const std::string &miss_dir );

        // mission start functions
        /// generic mission start function that wraps individual mission
        npc_ptr start_mission( const tripoint &omt_pos, const std::string &cmri, const std::string &miss_id,
                               time_duration duration, bool must_feed, const std::string &desc, bool group,
                               const std::vector<item *> &equipment,
                               const std::string &skill_tested, int skill_level );
        void start_upgrade( const tripoint &omt_pos, const std::string &cmri, const std::string &bldg, const std::string &key );
        /// Called when a companion is sent to cut logs
        void start_cut_logs( const tripoint &omt_pos );
        void start_clearcut( const tripoint &omt_pos );
        void start_setup_hide_site( const tripoint &omt_pos );
        void start_relay_hide_site( const tripoint &omt_pos );
        /// Called when a compansion is sent to start fortifications
        void start_fortifications( std::string &bldg_exp, const tripoint &omt_pos );
        void start_combat_mission( const std::string &miss, const tripoint &omt_pos, const std::string &cmri );
        /// Called when a companion starts a chop shop @ref task mission
        bool start_garage_chop( const tripoint &omt_pos, const std::string &cmri, const std::string dir, const tripoint &omt_tgt );
        void start_farm_op( const tripoint &omt_pos, const std::string &cmri, const std::string &dir, const tripoint &omt_tgt, farm_ops op );

        // mission functions
        void camp_missions( mission_data &mission_key, basecamp *bcp );
        bool handle_camp_mission( mission_entry &cur_key );
        /// called to select a companion to return to the base
        npc_ptr companion_choose_return( const std::string &miss_id, time_duration min_duration );
        comp_list companion_list( const std::string mission_id );
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
        /// Choose which expansion you should start, called when a survey mission is completed
        bool survey_return();
        bool menial_return();
        /// Called when a companion completes a gathering @ref task mission
        bool gathering_return( const std::string &task, time_duration min_time );
        void recruit_return( const std::string &task, int score );
        /**
        * Perform any mix of the three farm tasks.
        * @param p NPC companion
        * @param task
        * @param omt_trg the overmap pos3 of the farm_ops
        * @param ops whether to plow, plant, or harvest
        */
        bool farm_return( const std::string &task, const tripoint &omt_trg, farm_ops op );
        void fortifications_return();
        void combat_mission_return( const std::string &miss );

        // Save/load
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
        void load_data( const std::string &data );
    private:
        std::string name;
        // location of the camp in the overmap
        tripoint pos;
        // location of associated bulletin board
        tripoint bb_pos;
        std::map<std::string, expansion_data> expansions;
};

#endif
