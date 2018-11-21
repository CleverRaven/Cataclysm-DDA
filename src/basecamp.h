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
                  std::map<const std::string, expansion_data> expansions_ );

        inline bool is_valid() const {
            return !name.empty() && pos != tripoint( 0, 0, 0 );
        }
        inline int board_x() const {
            return bb_pos.x;
        }
        inline int board_y() const {
            return bb_pos.y;
        }
        inline std::string const &camp_name() const {
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

        // recipes and craft support functions
        std::map<std::string, std::string> recipe_deck( const std::string &dir ) const;
        const std::string get_gatherlist() const;

        // Save/load
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
        void load_data( std::string const &data );

    private:
        std::string name;
        // location of the camp in the overmap
        tripoint pos;
        // location of associated bulletin board
        tripoint bb_pos;
        std::map<const std::string, expansion_data> expansions;
};

#endif
