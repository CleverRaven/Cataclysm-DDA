#pragma once
#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <utility>

#include "item.h"
#include "optional.h"
#include "string_id.h"
#include "type_id.h"

class inventory;
class player;

namespace catacurses
{
class window;
} // namespace catacurses
class JsonObject;
class nc_color;
struct tripoint;

struct partial_con {
    int counter = 0;
    std::list<item> components = {};
    size_t id = 0;
};

struct build_reqs {
    std::map<skill_id, int> skills;
    std::map<requirement_id, int> reqs;
    int time = 0;
};

struct construction {
        // Construction type category
        construction_category_id category;
        // How the action is displayed to the player
        std::string description;
        // Additional note displayed along with construction requirements.
        std::string pre_note;
        // Beginning terrain for construction
        std::string pre_terrain;
        // Final terrain after construction
        std::string post_terrain;

        // Item group of byproducts created by the construction on success.
        cata::optional<std::string> byproduct_item_group;

        // Flags beginning terrain must have
        std::set<std::string> pre_flags;

        // Post construction flags
        std::set<std::string> post_flags;

        /** Skill->skill level mapping. Can be empty. */
        std::map<skill_id, int> required_skills;
        // the requirements specified by "using"
        std::vector<std::pair<requirement_id, int>> reqs_using;
        requirement_id requirements;

        // Index in construction vector
        size_t id;

        // Time in moves
        int time;

        // If true, the requirements are generated during finalization
        bool vehicle_start;

        // Custom constructibility check
        std::function<bool( const tripoint & )> pre_special;
        // Custom after-effects
        std::function<void( const tripoint & )> post_special;
        // Custom error message display
        std::function<void( const tripoint & )> explain_failure;
        // Whether it's furniture or terrain
        bool pre_is_furniture;
        // Whether it's furniture or terrain
        bool post_is_furniture;

        // NPC assistance adjusted
        int adjusted_time() const;
        int print_time( const catacurses::window &w, int ypos, int xpos, int width, nc_color col ) const;
        std::vector<std::string> get_folded_time_string( int width ) const;
        // Result of construction scaling option
        float time_scale() const;

        // make the construction available for selection
        bool on_display = true;
    private:
        std::string get_time_string() const;
};

const std::vector<construction> &get_constructions();

//! Set all constructions to take the specified time.
void standardize_construction_times( int time );

void load_construction( JsonObject &jo );
void reset_constructions();
int construction_menu( bool blueprint );
void complete_construction( player *p );
bool can_construct( const construction &con, const tripoint &p );
bool player_can_build( player &p, const inventory &inv, const construction &con );
void check_constructions();
void finalize_constructions();

void get_build_reqs_for_furn_ter_ids( const std::pair<std::map<ter_id, int>,
                                      std::map<furn_id, int>> &changed_ids,
                                      build_reqs &total_reqs );
#endif
