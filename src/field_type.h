#pragma once
#ifndef CATA_SRC_FIELD_TYPE_H
#define CATA_SRC_FIELD_TYPE_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "effect.h"
#include "enums.h"
#include "mapdata.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename E> struct enum_traits;

enum class description_affix : int {
    DESCRIPTION_AFFIX_IN,
    DESCRIPTION_AFFIX_COVERED_IN,
    DESCRIPTION_AFFIX_ON,
    DESCRIPTION_AFFIX_UNDER,
    DESCRIPTION_AFFIX_ILLUMINTED_BY,
    DESCRIPTION_AFFIX_NUM
};

namespace std
{
template <>
struct hash<description_affix> {
    std::size_t operator()( const description_affix &k ) const noexcept {
        return static_cast<size_t>( k );
    }
};
} // namespace std

static const std::unordered_map<description_affix, std::string> description_affixes = {
    { description_affix::DESCRIPTION_AFFIX_IN, translate_marker( " in %s" ) },
    { description_affix::DESCRIPTION_AFFIX_COVERED_IN, translate_marker( " covered in %s" ) },
    { description_affix::DESCRIPTION_AFFIX_ON, translate_marker( " on %s" ) },
    { description_affix::DESCRIPTION_AFFIX_UNDER, translate_marker( " under %s" ) },
    { description_affix::DESCRIPTION_AFFIX_ILLUMINTED_BY, translate_marker( " in %s" ) },
};

template<>
struct enum_traits<description_affix> {
    static constexpr description_affix last = description_affix::DESCRIPTION_AFFIX_NUM;
};

struct field_effect {
    efftype_id id;
    time_duration min_duration = 0_seconds;
    time_duration max_duration = 0_seconds;
    int intensity = 0;
    body_part bp = num_bp;
    bool is_environmental = true;
    bool immune_in_vehicle  = false;
    bool immune_inside_vehicle  = false;
    bool immune_outside_vehicle = false;
    int chance_in_vehicle = 0;
    int chance_inside_vehicle = 0;
    int chance_outside_vehicle = 0;
    game_message_type env_message_type = m_neutral;
    translation message;
    translation message_npc;
    time_duration get_duration() const {
        return rng( min_duration, max_duration );
    }
    std::string get_message() const {
        return message.translated();
    }
    std::string get_message_npc() const {
        return message_npc.translated();
    }
    effect get_effect( const time_point &start_time = calendar::turn ) const {
        return effect( &id.obj(), get_duration(), bp, false, intensity, start_time );
    }
};

struct field_intensity_level {
    translation name;
    uint32_t symbol = PERCENT_SIGN_UNICODE;
    nc_color color = c_white;
    bool dangerous = false;
    bool transparent = true;
    int move_cost = 0;
    int extra_radiation_min = 0;
    int extra_radiation_max = 0;
    int radiation_hurt_damage_min = 0;
    int radiation_hurt_damage_max = 0;
    translation radiation_hurt_message;
    int intensity_upgrade_chance = 0;
    time_duration intensity_upgrade_duration = 0_turns;
    int monster_spawn_chance = 0;
    int monster_spawn_count = 0;
    int monster_spawn_radius = 0;
    mongroup_id monster_spawn_group;
    float light_emitted = 0.0f;
    float local_light_override = -1.0f;
    float translucency = 0.0f;
    int convection_temperature_mod = 0;
    int scent_neutralization = 0;
    std::vector<field_effect> field_effects;
};

struct field_type {
    public:
        void load( const JsonObject &jo, const std::string &src );
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
        description_affix desc_affix = description_affix::DESCRIPTION_AFFIX_NUM;
        map_bash_info bash_info;

        // chance, issue, duration, speech
        std::tuple<int, std::string, time_duration, std::string> npc_complain_data;

        std::vector<trait_id> immunity_data_traits;
        std::vector<std::pair<body_part, int>> immunity_data_body_part_env_resistance;
        std::set<mtype_id> immune_mtypes;

        int priority = 0;
        time_duration half_life = 0_turns;
        phase_id phase = phase_id::PNULL;
        bool accelerated_decay = false;
        bool display_items = true;
        bool display_field = false;
        field_type_id wandering_field;
        std::string looks_like;

    public:
        const field_intensity_level &get_intensity_level( int level = 0 ) const;
        std::string get_name( int level = 0 ) const {
            return get_intensity_level( level ).name.translated();
        }
        uint32_t get_codepoint( int level = 0 ) const {
            return get_intensity_level( level ).symbol;
        }
        std::string get_symbol( int level = 0 ) const {
            return utf32_to_utf8( get_intensity_level( level ).symbol );
        }
        nc_color get_color( int level = 0 ) const {
            return get_intensity_level( level ).color;
        }
        bool get_dangerous( int level = 0 ) const {
            return get_intensity_level( level ).dangerous;
        }
        bool get_transparent( int level = 0 ) const {
            return get_intensity_level( level ).transparent;
        }
        int get_move_cost( int level = 0 ) const {
            return get_intensity_level( level ).move_cost;
        }
        int get_extra_radiation_min( int level = 0 ) const {
            return get_intensity_level( level ).extra_radiation_min;
        }
        int get_extra_radiation_max( int level = 0 ) const {
            return get_intensity_level( level ).extra_radiation_max;
        }
        int get_radiation_hurt_damage_min( int level = 0 ) const {
            return get_intensity_level( level ).radiation_hurt_damage_min;
        }
        int get_radiation_hurt_damage_max( int level = 0 ) const {
            return get_intensity_level( level ).radiation_hurt_damage_max;
        }
        std::string get_radiation_hurt_message( int level = 0 ) const {
            return get_intensity_level( level ).radiation_hurt_message.translated();
        }
        int get_intensity_upgrade_chance( int level = 0 ) const {
            return get_intensity_level( level ).intensity_upgrade_chance;
        }
        time_duration get_intensity_upgrade_duration( int level = 0 ) const {
            return get_intensity_level( level ).intensity_upgrade_duration;
        }
        int get_monster_spawn_chance( int level = 0 ) const {
            return get_intensity_level( level ).monster_spawn_chance;
        }
        int get_monster_spawn_count( int level = 0 ) const {
            return get_intensity_level( level ).monster_spawn_count;
        }
        int get_monster_spawn_radius( int level = 0 ) const {
            return get_intensity_level( level ).monster_spawn_radius;
        }
        mongroup_id get_monster_spawn_group( int level = 0 ) const {
            return get_intensity_level( level ).monster_spawn_group;
        }
        float get_light_emitted( int level = 0 ) const {
            return get_intensity_level( level ).light_emitted;
        }
        float get_local_light_override( int level = 0 )const {
            return get_intensity_level( level ).local_light_override;
        }
        float get_translucency( int level = 0 ) const {
            return get_intensity_level( level ).translucency;
        }
        int get_convection_temperature_mod( int level = 0 ) const {
            return get_intensity_level( level ).convection_temperature_mod;
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

void load( const JsonObject &jo, const std::string &src );
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
       fd_insecticidal_gas,
       fd_smoke_vent,
       fd_tindalos_rift
       ;

#endif // CATA_SRC_FIELD_TYPE_H
