#pragma once
#ifndef CATA_SRC_AMMO_EFFECT_H
#define CATA_SRC_AMMO_EFFECT_H

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "explosion.h"
#include "magic.h"
#include "type_id.h"

class JsonObject;
struct ammo_effect;
template <typename T> class generic_factory;

generic_factory<ammo_effect> &get_all_ammo_effects();

struct on_hit_effect {
    efftype_id effect;
    time_duration duration;
    int intensity;
    bool need_touch_skin;

    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

struct aoe_effect {
    efftype_id effect;
    time_duration duration;
    int intensity_min;
    int intensity_max;
    int chance;
    int radius;

    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

struct aoe_field_effect {
    field_type_str_id field_type = field_type_str_id::NULL_ID();
    int intensity_min = 0;
    int intensity_max = 0;
    int radius = 1;
    int radius_z = 0;
    int chance = 100;
    int size = 0;
    bool check_passable = false;

    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

struct trail_field_effect {
    field_type_str_id field_type = field_type_str_id::NULL_ID();
    int intensity_min = 0;
    int intensity_max = 0;
    int chance = 100;

    void deserialize( const JsonObject &jo );
    bool was_loaded = false;
};

struct ammo_effect {
    public:
        void load( const JsonObject &jo, std::string_view src );
        void check() const;

        // x in 100
        int trigger_chance = 100;
        // apply_ammo_effects()
        bool do_flashbang = false;
        // apply_ammo_effects()
        bool do_emp_blast = false;
        // apply_ammo_effects()
        bool foamcrete_build = false;
        // if true, spell is castes no matter if creature was hit or not
        bool always_cast_spell = false;

        // apply_ammo_effects()
        explosion_data aoe_explosion_data;
        // apply_ammo_effects()
        std::vector<aoe_field_effect> aoe_field_types;
        // map::shoot()
        std::vector<trail_field_effect> trail_field_types;
        // apply_effects_nodamage()
        std::vector<on_hit_effect> on_hit_effects;
        // apply_effects_nodamage()
        std::vector<aoe_effect> aoe_effects;
        // apply_ammo_effects()
        std::vector<fake_spell> spell_data;
        // apply_ammo_effects()
        std::vector<effect_on_condition_id> eoc;

        // Used by generic_factory
        ammo_effect_str_id id;
        std::vector<std::pair<ammo_effect_str_id, mod_id>> src;
        bool was_loaded = false;

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

#endif // CATA_SRC_AMMO_EFFECT_H
