#pragma once
#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include <functional>
#include <map>
#include <set>
#include <vector>

#include "optional.h"
#include "string_id.h"

namespace catacurses
{
class window;
} // namespace catacurses
class JsonObject;
class nc_color;
class Skill;
struct requirement_data;
struct tripoint;

using skill_id = string_id<Skill>;
using requirement_id = string_id<requirement_data>;

struct construction {
        std::string category; //Construction type category
        std::string description; // how the action is displayed to the player
        std::string pre_note; // Additional note displayed along with construction requirements.
        std::string pre_terrain; // beginning terrain for construction
        std::string post_terrain;// final terrain after construction

        // Item group of byproducts created by the construction on success.
        cata::optional<std::string> byproduct_item_group;

        std::set<std::string> pre_flags; // flags beginning terrain must have

        /** Skill->skill level mapping. Can be empty. */
        std::map<skill_id, int> required_skills;
        requirement_id requirements;

        size_t id; // Index in construction vector
        int time;

        // If true, the requirements are generated during finalization
        bool vehicle_start;

        std::function<bool( const tripoint & )> pre_special; // custom constructibility check
        std::function<void( const tripoint & )> post_special; // custom after-effects
        std::function<void( const tripoint & )> explain_failure; // Custom error message display

        bool pre_is_furniture; // whether it's furniture or terrain
        bool post_is_furniture; // whether it's furniture or terrain

        int adjusted_time() const; // NPC assistance adjusted
        int print_time( const catacurses::window &w, int ypos, int xpos, int width, nc_color col ) const;
        std::vector<std::string> get_folded_time_string( int width ) const;
        float time_scale() const; //result of construction scaling option
    private:
        std::string get_time_string() const;
};

//! Set all constructions to take the specified time.
void standardize_construction_times( int time );

void load_construction( JsonObject &jo );
void reset_constructions();
void construction_menu();
void complete_construction();
void check_constructions();
void finalize_constructions();

#endif
