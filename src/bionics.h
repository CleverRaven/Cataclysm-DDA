#pragma once
#ifndef CATA_SRC_BIONICS_H
#define CATA_SRC_BIONICS_H

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "flat_set.h"
#include "optional.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

class JsonIn;
class JsonObject;
class JsonOut;
class player;

using itype_id = std::string;

struct bionic_data {
    bionic_data();

    translation name;
    translation description;
    /** Power cost on activation */
    units::energy power_activate = 0_kJ;
    /** Power cost on deactivation */
    units::energy power_deactivate = 0_kJ;
    /** Power cost over time, does nothing without a non-zero charge_time */
    units::energy power_over_time = 0_kJ;
    /** How often a bionic draws or produces power while active in turns */
    int charge_time = 0;
    /** Power bank size **/
    units::energy capacity = 0_kJ;

    /** True if a bionic can be used by an NPC and installed on them */
    bool npc_usable = false;
    /** True if a bionic is a "faulty" bionic */
    bool faulty = false;
    bool power_source = false;
    /** Is true if a bionic is an active instead of a passive bionic */
    bool activated = false;
    /** If true, then the bionic only has a function when activated, else it causes
     *  it's effect every turn.
     */
    bool toggled = false;
    /**
     * If true, this bionic is a gun bionic and activating it will fire it.
     * Prevents all other activation effects.
     */
    bool gun_bionic = false;
    /**
     * If true, this bionic is a weapon bionic and activating it will
     * create (or destroy) bionic's fake_item in user's hands.
     * Prevents all other activation effects.
     */
    bool weapon_bionic = false;
    /**
     * If true, this bionic can provide power to powered armor.
     */
    bool armor_interface = false;
    /**
    * If true, this bionic won't provide a warning if the player tries to sleep while it's active.
    */
    bool sleep_friendly = false;
    /**
    * If true, this bionic can't be incapacitated by electrical attacks.
    */
    bool shockproof = false;
    /**
    * If true, this bionic is included with another.
    */
    bool included = false;
    /**Factor modifiying weight capacity*/
    float weight_capacity_modifier = 0.0f;
    /**Bonus to weight capacity*/
    units::mass weight_capacity_bonus = 0_gram;
    /**Map of stats and their corresponding bonuses passively granted by a bionic*/
    std::map<Character::stat, int> stat_bonus;
    /**This bionic draws power through a cable*/
    bool is_remote_fueled = false;
    /**Fuel types that can be used by this bionic*/
    std::vector<itype_id> fuel_opts;
    /**How much fuel this bionic can hold*/
    int fuel_capacity = 0;
    /**Fraction of fuel energy converted to bionic power*/
    float fuel_efficiency = 0.0f;
    /**Fraction of fuel energy passively converted to bionic power*/
    float passive_fuel_efficiency = 0.0f;
    /**Fraction of coverage diminishing fuel_efficiency*/
    cata::optional<float> coverage_power_gen_penalty;
    /**If true this bionic emits heat when producing power*/
    bool exothermic_power_gen = false;
    /**Type of field emitted by this bionic when it produces energy*/
    emit_id power_gen_emission = emit_id::NULL_ID();
    /**Amount of environemental protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> env_protec;

    /**Amount of bash protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> bash_protec;
    /**Amount of cut protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> cut_protec;
    /**Amount of bullet protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> bullet_protec;

    /** bionic enchantments */
    std::vector<enchantment_id> enchantments;

    /**
     * Body part slots used to install this bionic, mapped to the amount of space required.
     */
    std::map<body_part, size_t> occupied_bodyparts;
    /**
     * Body part encumbered by this bionic, mapped to the amount of encumbrance caused.
     */
    std::map<body_part, int> encumbrance;
    /**
     * Fake item created for crafting with this bionic available.
     * Also the item used for gun bionics.
     */
    std::string fake_item;
    /**
     * Mutations/trait that are removed upon installing this CBM.
     * E.g. enhanced optic bionic may cancel HYPEROPIC trait.
     */
    std::vector<trait_id> canceled_mutations;

    /**
     * The spells you learn when you install this bionic, and what level you learn them at.
     */
    std::map<spell_id, int> learned_spells;

    /**
     * Additional bionics that are installed automatically when this
     * bionic is installed. This can be used to install several bionics
     * from one CBM item, which is useful as each of those can be
     * activated independently.
     */
    std::vector<bionic_id> included_bionics;

    /**
     * Id of another bionic which this bionic can upgrade.
     */
    bionic_id upgraded_bionic;
    /**
     * Upgrades available for this bionic (opposite to @ref upgraded_bionic).
     */
    std::set<bionic_id> available_upgrades;

    bool is_included( const bionic_id &id ) const;
};

struct bionic {
        bionic_id id;
        int         charge_timer  = 0;
        char        invlet  = 'a';
        bool        powered = false;
        /* Ammunition actually loaded in this bionic gun in deactivated state */
        itype_id    ammo_loaded = "null";
        /* Ammount of ammo actually held inside by this bionic gun in deactivated state */
        unsigned int         ammo_count = 0;
        /* An amount of time during which this bionic has been rendered inoperative. */
        time_duration        incapacitated_time;
        bionic()
            : id( "bio_batteries" ), incapacitated_time( 0_turns ) {
        }
        bionic( bionic_id pid, char pinvlet )
            : id( std::move( pid ) ), invlet( pinvlet ), incapacitated_time( 0_turns ) { }

        const bionic_data &info() const {
            return *id;
        }

        void set_flag( const std::string &flag );
        void remove_flag( const std::string &flag );
        bool has_flag( const std::string &flag ) const;

        int get_quality( const quality_id &quality ) const;

        bool is_this_fuel_powered( const itype_id &this_fuel ) const;
        void toggle_safe_fuel_mod();
        void toggle_auto_start_mod();

        void set_auto_start_thresh( float val );
        float get_auto_start_thresh() const;
        bool is_auto_start_on() const;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
    private:
        // generic bionic specific flags
        cata::flat_set<std::string> bionic_tags;
        float auto_start_threshold = -1.0;
};

// A simpler wrapper to allow forward declarations of it. std::vector can not
// be forward declared without a *definition* of bionic, but this wrapper can.
class bionic_collection : public std::vector<bionic>
{
};

/**List of bodyparts occupied by a bionic*/
std::vector<body_part> get_occupied_bodyparts( const bionic_id &bid );

void check_bionics();
void finalize_bionics();
void reset_bionics();
// load a bionic from JSON
void load_bionic( const JsonObject &jsobj );
char get_free_invlet( player &p );
std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               bool each_bp_on_new_line = true );

int bionic_manip_cos( float adjusted_skill, int bionic_difficulty );

std::vector<bionic_id> bionics_cancelling_trait( const std::vector<bionic_id> &bios,
        const trait_id &tid );

#endif // CATA_SRC_BIONICS_H
