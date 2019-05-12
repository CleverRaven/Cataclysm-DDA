#pragma once
#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include <stddef.h>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <string>

#include "optional.h"
#include "string_id.h"
#include "type_id.h"

namespace catacurses
{
class window;
} // namespace catacurses
class JsonObject;
class nc_color;
struct tripoint;

struct construction {
        // Construction type category
        std::string category;
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

        /** Skill->skill level mapping. Can be empty. */
        std::map<skill_id, int> required_skills;
        requirement_id requirements;

        // Index in construction vector
        size_t id;
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
