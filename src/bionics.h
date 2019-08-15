#pragma once
#ifndef BIONICS_H
#define BIONICS_H

#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <utility>

#include "bodypart.h"
#include "calendar.h"
#include "string_id.h"
#include "type_id.h"

class player;
class JsonObject;
class JsonIn;
class JsonOut;
using itype_id = std::string;

struct bionic_data {
    bionic_data();

    std::string name;
    std::string description;
    /** Power cost on activation */
    int power_activate = 0;
    /** Power cost on deactivation */
    int power_deactivate = 0;
    /** Power cost over time, does nothing without a non-zero charge_time */
    int power_over_time = 0;
    /** How often a bionic draws power while active in turns */
    int charge_time = 0;
    /** Power bank size **/
    int capacity = 0;

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
    /**Amount of environemental protection offered by this bionic*/
    std::map<body_part, size_t> env_protec;
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
    int         charge  = 0;
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

    int get_quality( const quality_id &quality ) const;

    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

// A simpler wrapper to allow forward declarations of it. std::vector can not
// be forward declared without a *definition* of bionic, but this wrapper can.
class bionic_collection : public std::vector<bionic>
{
};

void check_bionics();
void finalize_bionics();
void reset_bionics();
void load_bionic( JsonObject &jsobj ); // load a bionic from JSON
char get_free_invlet( player &p );
std::string list_occupied_bps( const bionic_id &bio_id, const std::string &intro,
                               bool each_bp_on_new_line = true );

int bionic_manip_cos( float adjusted_skill, bool autodoc, int bionic_difficulty );

#endif
