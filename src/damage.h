#pragma once
#ifndef CATA_SRC_DAMAGE_H
#define CATA_SRC_DAMAGE_H

#include <array>
#include <map>
#include <vector>
#include <string>

#include "type_id.h"
#include "calendar.h"

class item;
class monster;
class JsonObject;
class JsonArray;
class JsonIn;

enum body_part : int;

enum damage_type : int {
    DT_NULL = 0, // null damage, doesn't exist
    DT_TRUE, // typeless damage, should always go through
    DT_BIOLOGICAL, // internal damage, like from smoke or poison
    DT_BASH, // bash damage
    DT_CUT, // cut damage
    DT_ACID, // corrosive damage, e.g. acid
    DT_STAB, // stabbing/piercing damage
    DT_HEAT, // e.g. fire, plasma
    DT_COLD, // e.g. heatdrain, cryogrenades
    DT_ELECTRIC, // e.g. electrical discharge
    DT_BULLET, // bullets and other fast moving projectiles
    NUM_DT
};

struct damage_unit {
    damage_type type;
    float amount;
    float res_pen;
    float res_mult;
    float damage_multiplier;

    float unconditional_res_mult;
    float unconditional_damage_mult;

    damage_unit( damage_type dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
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
    static damage_instance physical( float bash, float cut, float stab, float arpen = 0.0f );
    damage_instance( damage_type dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
                     float dmg_mult = 1.0f, float unc_arpen_mult = 1.0f, float unc_dmg_mult = 1.0f );
    void mult_damage( double multiplier, bool pre_armor = false );
    float type_damage( damage_type dt ) const;
    float total_damage() const;
    void clear();
    bool empty() const;

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
    void add_damage( damage_type dt, float amt, float arpen = 0.0f, float arpen_mult = 1.0f,
                     float dmg_mult = 1.0f, float unc_arpen_mult = 1.0f, float unc_dmg_mult = 1.0f );
    void add( const damage_instance &added_di );
    void add( const damage_unit &added_du );
    /*@}*/

    void deserialize( JsonIn & );
};

class damage_over_time_data
{
    public:
        damage_type type;
        time_duration duration;
        std::vector<bodypart_str_id> bps;
        int amount;

        bool was_loaded;

        void load( const JsonObject &obj );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
};

struct dealt_damage_instance {
    std::array<int, NUM_DT> dealt_dams;
    bodypart_id bp_hit;

    dealt_damage_instance();
    void set_damage( damage_type dt, int amount );
    int type_damage( damage_type dt ) const;
    int total_damage() const;
};

struct resistances {
    std::array<float, NUM_DT> resist_vals;

    resistances();

    // If to_self is true, we want armor's own resistance, not one it provides to wearer
    resistances( const item &armor, bool to_self = false );
    resistances( monster &monster );
    void set_resist( damage_type dt, float amount );
    float type_resist( damage_type dt ) const;

    float get_effective_resist( const damage_unit &du ) const;

    resistances &operator+=( const resistances &other );
};

const std::map<std::string, damage_type> &get_dt_map();
damage_type dt_by_name( const std::string &name );
std::string name_by_dt( const damage_type &dt );

const skill_id &skill_by_dt( damage_type dt );

damage_instance load_damage_instance( const JsonObject &jo );
damage_instance load_damage_instance( const JsonArray &jarr );

damage_instance load_damage_instance_inherit( const JsonObject &jo, const damage_instance &parent );
damage_instance load_damage_instance_inherit( const JsonArray &jarr,
        const damage_instance &parent );

resistances load_resistances_instance( const JsonObject &jo );

// Returns damage or resistance data
// Handles some shorthands
std::array<float, NUM_DT> load_damage_array( const JsonObject &jo );

#endif // CATA_SRC_DAMAGE_H
