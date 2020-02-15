#pragma once
#ifndef MOVE_MODE_H
#define MOVE_MODE_H

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

#include "type_id.h"
#include "string_id.h"
#include "translations.h"
#include "color.h"

class JsonObject;

struct move_mode {
    public:
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

    public:
        // Used by generic_factory
        move_mode_str_id id;
        bool was_loaded = false;

    public:
        translation name;

        bool can_be_cycled_to;
        bool can_haul;
        bool usable_while_mounted;
        float speed;
        std::string display_colour;
        std::string display_character;
        int minimum_required_legs;
        int cost_to_enter_or_leave_stance;
        translation flavor_text;
        translation flavor_text_mount;
        translation flavor_text_mech;
        bool cancels_activities;
        float stamina_burn_multiplier;
        float volume_multiplier;
        float silhouette_size;

        nc_color nc_display_colour;
        const char *display_character_ptr;

        static size_t count();
};

namespace move_modes
{
void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<move_mode> &get_all();

void set_move_mode_ids();

} // namespace move_modes

extern std::list<move_mode> move_mode_cycle_list;

extern move_mode_id MM_NULL,
       MM_WALK,
       MM_RUN,
       MM_CROUCH;
#endif // MOVE_MODE_H
