#pragma once
#ifndef CATA_SRC_FIELD_TYPE_H
#define CATA_SRC_FIELD_TYPE_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
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
#include "effect_source.h"
#include "enums.h"
#include "mapdata.h"
#include "map_field.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
template <typename E> struct enum_traits;

class field_entry;
struct field_proc_data;

enum class description_affix : int {
    DESCRIPTION_AFFIX_IN,
    DESCRIPTION_AFFIX_COVERED_IN,
    DESCRIPTION_AFFIX_ON,
    DESCRIPTION_AFFIX_UNDER,
    DESCRIPTION_AFFIX_ILLUMINATED_BY,
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

template<>
struct enum_traits<description_affix> {
    static constexpr description_affix last = description_affix::DESCRIPTION_AFFIX_NUM;
};

generic_factory<field_type> &get_all_field_types();

struct field_immunity_data {
    std::vector<json_character_flag> immunity_data_flags;
    std::vector<std::pair<body_part_type::type, int>> immunity_data_body_part_env_resistance;
    std::vector < std::pair<body_part_type::type, flag_id>> immunity_data_part_item_flags;
    std::vector < std::pair<body_part_type::type, flag_id>> immunity_data_part_item_flags_any;
};

struct field_effect {
    efftype_id id;
    std::vector<std::pair<efftype_id, mod_id>> src;
    time_duration min_duration = 0_seconds;
    time_duration max_duration = 0_seconds;
    int intensity = 0;
    bodypart_str_id bp;
    field_immunity_data immunity_data;
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
        return effect( effect_source::empty(), &id.obj(), get_duration(), bp, false, intensity,
                       start_time );
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
    int concentration = 0;
    int convection_temperature_mod = 0;
    int scent_neutralization = 0;
    std::vector<field_effect> field_effects;
};

// NOLINTNEXTLINE(cata-static-int_id-constants)
const field_type_id INVALID_FIELD_TYPE_ID = field_type_id( -1 );
extern const field_type_str_id fd_null;
extern const field_type_str_id fd_acid;
extern const field_type_str_id fd_acid_vent;
extern const field_type_str_id fd_bile;
extern const field_type_str_id fd_blood;
extern const field_type_str_id fd_blood_insect;
extern const field_type_str_id fd_blood_invertebrate;
extern const field_type_str_id fd_blood_veggy;
extern const field_type_str_id fd_churned_earth;
extern const field_type_str_id fd_construction_site;
extern const field_type_str_id fd_cold_air2;
extern const field_type_str_id fd_cold_air3;
extern const field_type_str_id fd_cold_air4;
extern const field_type_str_id fd_dazzling;
extern const field_type_str_id fd_electricity;
extern const field_type_str_id fd_electricity_unlit;
extern const field_type_str_id fd_extinguisher;
extern const field_type_str_id fd_fatigue;
extern const field_type_str_id fd_fire;
extern const field_type_str_id fd_fire_vent;
extern const field_type_str_id fd_flame_burst;
extern const field_type_str_id fd_fungal_haze;
extern const field_type_str_id fd_fungicidal_gas;
extern const field_type_str_id fd_gas_vent;
extern const field_type_str_id fd_gibs_flesh;
extern const field_type_str_id fd_gibs_insect;
extern const field_type_str_id fd_gibs_invertebrate;
extern const field_type_str_id fd_gibs_veggy;
extern const field_type_str_id fd_hot_air1;
extern const field_type_str_id fd_hot_air2;
extern const field_type_str_id fd_hot_air3;
extern const field_type_str_id fd_hot_air4;
extern const field_type_str_id fd_incendiary;
extern const field_type_str_id fd_insecticidal_gas;
extern const field_type_str_id fd_laser;
extern const field_type_str_id fd_last_known;
extern const field_type_str_id fd_nuke_gas;
extern const field_type_str_id fd_plasma;
extern const field_type_str_id fd_push_items;
extern const field_type_str_id fd_relax_gas;
extern const field_type_str_id fd_sap;
extern const field_type_str_id fd_shock_vent;
extern const field_type_str_id fd_slime;
extern const field_type_str_id fd_sludge;
extern const field_type_str_id fd_smoke;
extern const field_type_str_id fd_smoke_vent;
extern const field_type_str_id fd_tear_gas;
extern const field_type_str_id fd_tindalos_rift;
extern const field_type_str_id fd_toxic_gas;
extern const field_type_str_id fd_web;

struct field_type;

struct field_type {
    public:
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;

        // Used by generic_factory
        field_type_str_id id;
        std::vector<std::pair<field_type_str_id, mod_id>> src;
        bool was_loaded = false;

        int legacy_enum_id = -1;

        std::vector<field_intensity_level> intensity_levels;

        time_duration underwater_age_speedup = 0_turns;
        time_duration outdoor_age_speedup = 0_turns;
        int decay_amount_factor = 0;
        int percent_spread = 0;
        int apply_slime_factor = 0;
        time_duration gas_absorption_factor = 0_turns;
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
        field_immunity_data immunity_data;

        std::set<mtype_id> immune_mtypes;

        int priority = 0;
        time_duration half_life = 0_turns;
        phase_id phase = phase_id::PNULL;
        bool accelerated_decay = false;
        bool display_items = true;
        bool display_field = false;
        bool legacy_make_rubble = false;
        field_type_str_id wandering_field;
        std::string looks_like;

        bool mopsafe = false;

        bool decrease_intensity_on_contact = false;

    private:
        // any_of( intensity_levels::dangerous )
        bool dangerous = false;
        // all_of( intensity_levels::transparent )
        bool transparent = false;

        std::vector<map_field_processing::FieldProcessorPtr> processors;

    public:
        const field_intensity_level &get_intensity_level( int level = 0 ) const;
        std::string get_name( int level = 0 ) const {
            return get_intensity_level( level ).name.translated();
        }
        std::string get_symbol( int level = 0 ) const {
            return utf32_to_utf8( get_intensity_level( level ).symbol );
        }
        // any_of( intensity_levels::dangerous )
        inline bool is_dangerous() const {
            return dangerous;
        }
        // all_of( intensity_levels::transparent )
        inline bool is_transparent() const {
            return transparent;
        }
        inline int get_max_intensity() const {
            return intensity_levels.size();
        }
        bool gas_can_spread() const {
            return phase == phase_id::GAS && percent_spread > 0;
        }
        const std::vector<map_field_processing::FieldProcessorPtr> &get_processors() const {
            return processors;
        }

        static size_t count();
};

namespace field_types
{

void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

void load_immunity( const JsonObject &jid, field_immunity_data &fd );

const std::vector<field_type> &get_all();
field_type get_field_type_by_legacy_enum( int legacy_enum_id );

} // namespace field_types

#endif // CATA_SRC_FIELD_TYPE_H
