#pragma once
#ifndef CATA_SRC_BIONICS_H
#define CATA_SRC_BIONICS_H

#include <cstddef>
#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <vector>

#include "calendar.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "flat_set.h"
#include "item.h"
#include "magic.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

class Character;
class JsonObject;
class JsonOut;

enum class character_stat : char;

struct bionic_data {
    bionic_data();

    bionic_id id;
    std::vector<std::pair<bionic_id, mod_id>> src;

    translation name;
    translation description;

    std::optional<translation> cant_remove_reason;
    /** Power cost on activation */
    units::energy power_activate = 0_kJ;
    /** Power cost on deactivation */
    units::energy power_deactivate = 0_kJ;
    /** Power cost over time, does nothing without a non-zero charge_time */
    units::energy power_over_time = 0_kJ;
    /** Power cost when the bionic's special effect is triggered */
    units::energy power_trigger = 0_kJ;
    /** Amount of free energy the bionic generates each turn regardless of activation state*/
    units::energy power_trickle = 0_kJ;
    /** How often a bionic draws or produces power while active in turns */
    time_duration charge_time = 0_turns;
    /** Power bank size **/
    units::energy capacity = 0_kJ;
    /** If true multiples of this can be installed */
    bool dupes_allowed = false;
    /** Is true if a bionic is an active instead of a passive bionic */
    bool activated = false;
    /** Is true if a bionic is activated automatically on install */
    bool activated_on_install = false;
    /**
    * If true, this bionic is included with another.
    */
    bool included = false;
    /**Factor modifying weight capacity*/
    float weight_capacity_modifier = 1.0f;
    /**Bonus to weight capacity*/
    units::mass weight_capacity_bonus = 0_gram;
    /**Map of stats and their corresponding bonuses passively granted by a bionic*/
    std::map<character_stat, int> stat_bonus;
    /**This bionic draws power through a cable*/
    bool is_remote_fueled = false;
    /**Fuel types that can be used by this bionic*/
    std::vector<material_id> fuel_opts;
    /**Fraction of fuel energy converted to bionic power*/
    float fuel_efficiency = 0.0f;
    /**Fraction of fuel energy passively converted to bionic power*/
    float passive_fuel_efficiency = 0.0f;
    /**Fraction of coverage diminishing fuel_efficiency*/
    std::optional<float> coverage_power_gen_penalty;
    /**If true this bionic emits heat when producing power*/
    bool exothermic_power_gen = false;
    /**Type of field emitted by this bionic when it produces energy*/
    emit_id power_gen_emission = emit_id::NULL_ID();
    /**Amount of environmental protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> env_protec;

    /**Amount of bash protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> bash_protec;
    /**Amount of cut protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> cut_protec;
    /**Amount of bullet protection offered by this bionic*/
    std::map<bodypart_str_id, size_t> bullet_protec;

    float vitamin_absorb_mod = 1.0f;

    // Bonus or penalty to social checks (additive).  50 adds 50% to success, -25 subtracts 25%
    social_modifiers social_mods;
    /** whether to immediately close ui when activating - used for targeted bionics */
    bool activated_close_ui;
    /** whether to immediately close ui when deactivating - used for targeted bionics */
    bool deactivated_close_ui;
    /** effect_on_conditions triggered when this bionic is activated */
    std::vector<effect_on_condition_id> activated_eocs;
    /** effect_on_conditions triggered while this bionic is active */
    std::vector<effect_on_condition_id> processed_eocs;
    /** effect_on_conditions triggered when this bionic is deactivated */
    std::vector<effect_on_condition_id> deactivated_eocs;
    /** bionic enchantments */
    std::vector<enchantment_id> enchantments;
    /** kown martial arts styles */
    std::vector<matype_id> ma_styles;

    cata::value_ptr<fake_spell> spell_on_activate;

    /**
     * Proficiencies given on install (and removed on uninstall) of this bionic
     */
    std::vector<proficiency_id> proficiencies;
    /**
     * Body part slots used to install this bionic, mapped to the amount of space required.
     */
    std::map<bodypart_str_id, size_t> occupied_bodyparts;
    /**
     * Body part encumbered by this bionic, mapped to the amount of encumbrance caused.
     */
    std::map<bodypart_str_id, int> encumbrance;

    /**
    * Pseudo items and weapons this CBM spawns
    */
    std::vector<itype_id> passive_pseudo_items;
    std::vector<itype_id> toggled_pseudo_items;
    itype_id fake_weapon;
    std::set<json_character_flag> installable_weapon_flags;

    /**
     * Mutations/trait that are removed upon installing this CBM.
     * E.g. enhanced optic bionic may cancel HYPEROPIC trait.
     */
    std::vector<trait_id> canceled_mutations;
    /**
     * Mutations/traits that prevent installing this CBM
     */
    std::set<trait_id> mutation_conflicts;

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
     * Bionics that are incompatible with this bionic and will be
     * deactivated automatically when this bionic is activated.
     */
    std::vector<bionic_id> autodeactivated_bionics;

    /**
     * Id of another bionic which this bionic can upgrade.
     */
    bionic_id upgraded_bionic;
    /**
     * Upgrades available for this bionic (opposite to @ref upgraded_bionic).
     */
    std::set<bionic_id> available_upgrades;

    /**Requirement to bionic installation*/
    requirement_id installation_requirement;

    cata::flat_set<json_character_flag> flags;
    cata::flat_set<json_character_flag> active_flags;
    cata::flat_set<json_character_flag> inactive_flags;
    bool has_flag( const json_character_flag &flag ) const;
    bool has_active_flag( const json_character_flag &flag ) const;
    bool has_inactive_flag( const json_character_flag &flag ) const;

    itype_id itype() const;

    bool was_loaded = false;
    void load( const JsonObject &obj, const std::string &src );
    static void load_bionic( const JsonObject &jo, const std::string &src );
    static const std::vector<bionic_data> &get_all();
    static void check_bionic_consistency();

    static std::map<bionic_id, bionic_id> migrations;
    static void load_bionic_migration( const JsonObject &jo, const std::string & );
};

struct bionic {

        using bionic_uid = unsigned int;

        bionic_id id;
        time_duration         charge_timer  = 0_turns;
        char        invlet  = 'a';
        bool        powered = false;
        /* An amount of time during which this bionic has been rendered inoperative. */
        time_duration        incapacitated_time;

        bionic() : bionic( bionic_id( "bio_batteries" ), 'a', 0 ) { }
        bionic( bionic_id pid, char pinvlet, bionic_uid pbionic_uid,
                bionic_uid pparent_uid = 0 ) : id( pid ), invlet( pinvlet ),
            incapacitated_time( 0_turns ), uid( pbionic_uid ), parent_uid( pparent_uid ) {
            initialize_pseudo_items( true );
        }

        const bionic_data &info() const {
            return *id;
        }

        void set_flag( const std::string &flag );
        void remove_flag( const std::string &flag );
        bool has_flag( const std::string &flag ) const;

        int get_quality( const quality_id &quality ) const;
        item get_weapon() const;
        void set_weapon( const item &new_weapon );
        bool install_weapon( const item &new_weapon, bool skip_checks = false );
        std::optional<item> uninstall_weapon();
        bool has_weapon() const;
        bool can_install_weapon() const;
        bool can_install_weapon( const item &new_weapon ) const;
        unsigned int get_uid() const;
        void set_uid( bionic_uid new_uid );
        bool is_included() const;
        bionic_uid get_parent_uid() const;
        void set_parent_uid( bionic_uid new_uid );

        std::vector<const item *> get_available_pseudo_items( bool include_weapon = true ) const;

        bool is_this_fuel_powered( const material_id &this_fuel ) const;
        void toggle_safe_fuel_mod();

        void set_safe_fuel_thresh( float val );
        float get_safe_fuel_thresh() const;
        bool is_safe_fuel_on() const;
        bool activate_spell( Character &caster ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
    private:
        // generic bionic specific flags
        cata::flat_set<std::string> bionic_tags;
        float safe_fuel_threshold = 1.0f;
        item weapon;
        std::vector<item> toggled_pseudo_items; // NOLINT(cata-serialize)
        std::vector<item> passive_pseudo_items; // NOLINT(cata-serialize)
        bionic_uid uid;
        bionic_uid parent_uid;
        void initialize_pseudo_items( bool create_weapon = false );
        void update_weapon_flags();
};

// A simpler wrapper to allow forward declarations of it. std::vector can not
// be forward declared without a *definition* of bionic, but this wrapper can.
class bionic_collection : public std::vector<bionic>
{
};

/**List of bodyparts occupied by a bionic*/
std::vector<bodypart_id> get_occupied_bodyparts( const bionic_id &bid );

void reset_bionics();

char get_free_invlet( Character &p );
std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               bool each_bp_on_new_line = true );

int bionic_success_chance( bool autodoc, int skill_level, int difficulty, const Character &target );
int bionic_manip_cos( float adjusted_skill, int bionic_difficulty );

std::vector<bionic_id> bionics_cancelling_trait( const std::vector<bionic_id> &bios,
        const trait_id &tid );

#endif // CATA_SRC_BIONICS_H
