#pragma once
#ifndef FIELD_TYPE_H
#define FIELD_TYPE_H

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <string>

#include "bodypart.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "enums.h"
#include "type_id.h"
#include "string_id.h"
#include "translations.h"

class JsonObject;

enum phase_id : int;
enum body_part : int;

struct field_intensity_level {
    std::string name;
    uint32_t symbol = PERCENT_SIGN_UNICODE;
    nc_color color = c_white;
    bool dangerous = false;
    bool transparent = true;
    int move_cost = 0;
    int extra_radiation_min = 0;
    int extra_radiation_max = 0;
    float light_emitted = 0.0f;
    float translucency = 0.0f;
    int convection_temperature_mod = 0.0f;
};

struct field_type {
    public:
        void load( JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

    public:
        // Used by generic_factory
        field_type_str_id id;
        bool was_loaded = false;

        // Used only during loading
        std::string wandering_field_id = "fd_null";

    public:
        int legacy_enum_id = -1;

        std::vector<field_intensity_level> intensity_levels;

        time_duration underwater_age_speedup = 0_turns;
        time_duration outdoor_age_speedup = 0_turns;
        int decay_amount_factor = 0;
        int percent_spread = 0;
        int apply_slime_factor = 0;
        int gas_absorption_factor = 0;
        bool is_splattering = false;
        bool dirty_transparency_cache = false;
        bool has_fire = false;
        bool has_acid = false;
        bool has_elec = false;
        bool has_fume = false;

        // chance, issue, duration, speech
        std::tuple<int, std::string, time_duration, std::string> npc_complain_data;

        std::vector<trait_id> immunity_data_traits;
        std::vector<std::pair<body_part, int>> immunity_data_body_part_env_resistance;

        int priority = 0;
        time_duration half_life = 0_turns;
        phase_id phase = PNULL;
        bool accelerated_decay = false;
        bool display_items = true;
        bool display_field = false;
        field_type_id wandering_field;

    public:
        std::string get_name( int level = 0 ) const {
            return _( intensity_levels[level].name );
        }
        uint32_t get_codepoint( int level = 0 ) const {
            return intensity_levels[level].symbol;
        }
        std::string get_symbol( int level = 0 ) const {
            return utf32_to_utf8( intensity_levels[level].symbol );
        }
        nc_color get_color( int level = 0 ) const {
            return intensity_levels[level].color;
        }
        bool get_dangerous( int level = 0 ) const {
            return intensity_levels[level].dangerous;
        }
        bool get_transparent( int level = 0 ) const {
            return intensity_levels[level].transparent;
        }
        int get_move_cost( int level = 0 ) const {
            return intensity_levels[level].move_cost;
        }
        int get_extra_radiation_min( int level = 0 ) const {
            return intensity_levels[level].extra_radiation_min;
        }
        int get_extra_radiation_max( int level = 0 ) const {
            return intensity_levels[level].extra_radiation_max;
        }
        float get_light_emitted( int level = 0 ) const {
            return intensity_levels[level].light_emitted;
        }
        float get_translucency( int level = 0 ) const {
            return intensity_levels[level].translucency;
        }
        int get_convection_temperature_mod( int level = 0 ) const {
            return intensity_levels[level].convection_temperature_mod;
        }

        bool is_dangerous() const {
            return std::any_of( intensity_levels.begin(), intensity_levels.end(),
            []( const field_intensity_level & elem ) {
                return elem.dangerous;
            } );
        }
        bool is_transparent() const {
            return std::all_of( intensity_levels.begin(), intensity_levels.end(),
            []( const field_intensity_level & elem ) {
                return elem.transparent;
            } );
        }
        int get_max_intensity() const {
            return intensity_levels.size();
        }

        static size_t count();
};

namespace field_types
{

void load( JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<field_type> &get_all();
void set_field_type_ids();
field_type get_field_type_by_legacy_enum( int legacy_enum_id );

} // namespace field_types

extern field_type_id fd_null,
       fd_blood,
       fd_bile,
       fd_gibs_flesh,
       fd_gibs_veggy,
       fd_web,
       fd_slime,
       fd_acid,
       fd_sap,
       fd_sludge,
       fd_fire,
       fd_rubble,
       fd_smoke,
       fd_toxic_gas,
       fd_tear_gas,
       fd_nuke_gas,
       fd_gas_vent,
       fd_fire_vent,
       fd_flame_burst,
       fd_electricity,
       fd_fatigue,
       fd_push_items,
       fd_shock_vent,
       fd_acid_vent,
       fd_plasma,
       fd_laser,
       fd_spotlight,
       fd_dazzling,
       fd_blood_veggy,
       fd_blood_insect,
       fd_blood_invertebrate,
       fd_gibs_insect,
       fd_gibs_invertebrate,
       fd_cigsmoke,
       fd_weedsmoke,
       fd_cracksmoke,
       fd_methsmoke,
       fd_bees,
       fd_incendiary,
       fd_relax_gas,
       fd_fungal_haze,
       fd_cold_air1,
       fd_cold_air2,
       fd_cold_air3,
       fd_cold_air4,
       fd_hot_air1,
       fd_hot_air2,
       fd_hot_air3,
       fd_hot_air4,
       fd_fungicidal_gas,
       fd_smoke_vent
       ;

#endif
