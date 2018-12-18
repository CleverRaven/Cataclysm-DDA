#pragma once
#ifndef BASECAMP_H
#define BASECAMP_H

#include "enums.h"

#include <vector>
#include <map>
#include <string>

class JsonIn;
class JsonOut;
class npc;

struct expansion_data {
    std::string type;
    int cur_level;
    tripoint pos;
};

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
        /// Takes all the food from the point set in set_sort_pts() and increases the faction food_supply
        bool distribute_food();

        // recipes and craft support functions
        std::map<std::string, std::string> recipe_deck( const std::string &dir ) const;
        void craft_construction( npc &p, const std::string &cur_id, const std::string &cur_dir,
                                 const std::string &type, const std::string &miss_id ) const;
        const std::string get_gatherlist() const;

        // mission return functions
        std::string recruit_start( int npc_count );
        // mission return functions
        /// Called to close upgrade missions, @ref miss is the name of the mission id and @ref dir is the direction of the location to be upgraded
        bool upgrade_return( npc &p, const std::string &dir, const std::string &miss );
        /// Choose which expansion you should start, called when a survey mission is completed
        bool survey_return( npc &p );
        bool menial_return( npc &p );

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
