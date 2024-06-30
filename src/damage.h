#pragma once
#ifndef CATA_SRC_DAMAGE_H
#define CATA_SRC_DAMAGE_H

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "flat_set.h"
#include "translation.h"
#include "type_id.h"
#include "units.h"

class Creature;
class JsonArray;
class JsonObject;
class JsonOut;
class JsonValue;
class item;
class monster;

struct damage_type {
    damage_type_id id;
    translation name;
    std::vector<effect_on_condition_id> onhit_eocs;
    std::vector<effect_on_condition_id> ondamage_eocs;
    skill_id skill = skill_id::NULL_ID();
    std::pair<damage_type_id, float> derived_from = { damage_type_id(), 0.0f };
    cata::flat_set<std::string> immune_flags;
    cata::flat_set<std::string> mon_immune_flags;
    nc_color magic_color;
    bool melee_only = false;
    bool physical = false;
    bool mon_difficulty = false;
    bool no_resist = false;
    bool edged = false;
    bool env = false;
    bool material_required = false;
    bool was_loaded = false;

    // Applies damage type on-hit effects
    void onhit_effects( Creature *source, Creature *target ) const;

    // Applies damage type on-damage effects
    void ondamage_effects( Creature *source, Creature *target,
                           bodypart_str_id bp, double total_damage = 0.0,
                           double damage_taken = 0.0 ) const;

    static void load_damage_types( const JsonObject &jo, const std::string &src );
    static void reset();
    static void check();
    void load( const JsonObject &jo, std::string_view );
    static const std::vector<damage_type> &get_all();
};

struct damage_info_order {
    enum class info_disp : int {
        NONE,
        BASIC,
        DETAILED
    };
    enum class info_type : int {
        NONE = 0,
        BIO,
        PROT,
        PET,
        MELEE,
        ABLATE
    };
    struct damage_info_order_entry {
        int order = -1;
        bool show_type = false;
        void load( const JsonObject &jo, std::string_view member );
    };
    damage_info_order_id id;
    damage_type_id dmg_type = damage_type_id::NULL_ID();
    info_disp info_display = info_disp::DETAILED;
    translation verb;
    damage_info_order_entry bionic_info;
    damage_info_order_entry protection_info;
    damage_info_order_entry pet_prot_info;
    damage_info_order_entry melee_combat_info;
    damage_info_order_entry ablative_info;
    bool was_loaded = false;

    static void load_damage_info_orders( const JsonObject &jo, const std::string &src );
    static void reset();
    static void finalize_all();
    void finalize();
    void load( const JsonObject &jo, std::string_view src );
    static const std::vector<damage_info_order> &get_all();
    static const std::vector<damage_info_order> &get_all( info_type sort_by );
};

struct barrel_desc {
    units::length barrel_length;
    float amount;

    barrel_desc();
    barrel_desc( units::length bl, float amt ) : barrel_length( bl ), amount( amt ) {}
    void deserialize( const JsonObject &jo );
};

struct damage_unit {
    damage_type_id type;
    float amount;
    float res_pen;
    float res_mult;
    float damage_multiplier;

    float unconditional_res_mult;
    float unconditional_damage_mult;

    std::vector<barrel_desc> barrels;

    damage_unit( const damage_type_id &dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
                 float dmg_mult = 1.0f, float unc_arpen_mult = 1.0f, float unc_dmg_mult = 1.0f ) :
        type( dt ), amount( amt ), res_pen( arpen ), res_mult( arpen_mult ), damage_multiplier( dmg_mult ),
        unconditional_res_mult( unc_arpen_mult ), unconditional_damage_mult( unc_dmg_mult ) { }

    bool operator==( const damage_unit &other ) const;
};

// a single atomic unit of damage from an attack. Can include multiple types
// of damage at different armor mitigation/penetration values
struct damage_instance {
    std::vector<damage_unit> damage_units;
    damage_instance();
    damage_instance( const damage_type_id &dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
                     float dmg_mult = 1.0f, float unc_arpen_mult = 1.0f, float unc_dmg_mult = 1.0f );
    void mult_damage( double multiplier, bool pre_armor = false );
    void mult_type_damage( double multiplier, const damage_type_id &dt );
    float type_damage( const damage_type_id &dt ) const;
    float type_arpen( const damage_type_id &dt ) const;
    float total_damage() const;
    void clear();
    bool empty() const;

    // Applies damage type on-hit effects for all damage units
    void onhit_effects( Creature *source, Creature *target ) const;

    // Applies damage type on-damage effects for all damage units
    void ondamage_effects( Creature *source, Creature *target, const damage_instance &premitigated,
                           bodypart_str_id bp ) const;

    // calculates damage taking barrel length into consideration for the amount
    damage_instance di_considering_length( units::length barrel_length ) const;

    std::vector<damage_unit>::iterator begin();
    std::vector<damage_unit>::const_iterator begin() const;
    std::vector<damage_unit>::iterator end();
    std::vector<damage_unit>::const_iterator end() const;

    bool operator==( const damage_instance &other ) const;

    /**
     * Adds damage to the instance.
     * If the damage type already exists in the instance, the old and new instance are normalized.
     * The normalization means that the effective damage can actually decrease (depending on target's armor).
     */
    /*@{*/
    void add_damage( const damage_type_id &dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
                     float dmg_mult = 1.0f, float unc_arpen_mult = 1.0f, float unc_dmg_mult = 1.0f );
    void add( const damage_instance &added_di );
    void add( const damage_unit &added_du );
    /*@}*/

    void deserialize( const JsonValue &jsin );
};

class damage_over_time_data
{
    public:
        damage_type_id type;
        time_duration duration;
        std::vector<bodypart_str_id> bps;
        int amount = 0;

        bool was_loaded = false;

        void load( const JsonObject &obj );

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jo );
};

struct dealt_damage_instance {
    std::map<damage_type_id, int> dealt_dams;
    bodypart_id bp_hit;
    std::string wp_hit;

    dealt_damage_instance();
    void set_damage( const damage_type_id &dt, int amount );
    int type_damage( const damage_type_id &dt ) const;
    int total_damage() const;
};

struct resistances {
    std::unordered_map<damage_type_id, float> resist_vals;

    resistances() = default;

    // If to_self is true, we want armor's own resistance, not one it provides to wearer
    explicit resistances( const item &armor, bool to_self = false, int roll = 0,
                          const bodypart_id &bp = bodypart_id() );
    explicit resistances( const item &armor, bool to_self, int roll, const sub_bodypart_id &bp );
    explicit resistances( monster &monster );
    void set_resist( const damage_type_id &dt, float amount );
    float type_resist( const damage_type_id &dt ) const;

    float get_effective_resist( const damage_unit &du ) const;

    resistances &operator+=( const resistances &other ) {
        for( const auto &dam : other.resist_vals ) {
            resist_vals[dam.first] += dam.second;
        }

        return *this;
    }
    bool operator==( const resistances &other );
    resistances operator*( float mod ) const;
    resistances operator/( float mod ) const;
};

damage_instance load_damage_instance( const JsonObject &jo );
damage_instance load_damage_instance( const JsonArray &jarr );

damage_instance load_damage_instance_inherit( const JsonObject &jo, const damage_instance &parent );
damage_instance load_damage_instance_inherit( const JsonArray &jarr,
        const damage_instance &parent );

resistances extend_resistances_instance( resistances ret, const JsonObject &jo );
resistances load_resistances_instance( const JsonObject &jo,
                                       const std::set<std::string> &ignored_keys = {} );

// Returns damage or resistance data
// Handles some shorthands
std::unordered_map<damage_type_id, float> load_damage_map( const JsonObject &jo,
        const std::set<std::string> &ignored_keys = {} );
void finalize_damage_map( std::unordered_map<damage_type_id, float> &damage_map,
                          bool force_derive = false,
                          float default_value = 0.0f );

#endif // CATA_SRC_DAMAGE_H
