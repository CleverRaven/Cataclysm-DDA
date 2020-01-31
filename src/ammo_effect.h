#pragma once
#ifndef AMMO_EFFECT_H
#define AMMO_EFFECT_H

#include <vector>
#include <string>

#include "explosion.h"
#include "field_type.h"
#include "type_id.h"

class JsonObject;

struct ammo_effect {
    public:
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

    public:
        field_type_id aoe_field_type = fd_null;
        /** used during JSON loading only */
        std::string aoe_field_type_name = "fd_null";
        int aoe_intensity_min = 0;
        int aoe_intensity_max = 0;
        int aoe_radius = 1;
        int aoe_radius_z = 0;
        int aoe_chance = 100;
        int aoe_size = 0;
        explosion_data aoe_explosion_data;
        bool aoe_check_passable = false;

        bool aoe_check_sees = false;
        int aoe_check_sees_radius = 0;
        bool do_flashbang = false;
        bool do_emp_blast = false;

        field_type_id trail_field_type = fd_null;
        /** used during JSON loading only */
        std::string trail_field_type_name = "fd_null";
        int trail_intensity_min = 0;
        int trail_intensity_max = 0;
        int trail_chance = 100;

    public:
        // Used by generic_factory
        string_id<ammo_effect> id;
        bool was_loaded = false;

    public:
        static size_t count();
};

namespace ammo_effects
{

void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<ammo_effect> &get_all();

} // namespace ammo_effects

extern ammo_effect_id AE_NULL;

#endif
