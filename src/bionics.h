#ifndef BIONICS_H
#define BIONICS_H

#include "json.h"
#include <string>

class player;

struct quality;
using quality_id = string_id<quality>;

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
    std::map<body_part, size_t> occupied_bodyparts;
    /**
     * Fake item created for crafting with this bionic available.
     * Also the item used for gun bionics.
     */
    std::string fake_item;
    /**
     * Mutations/trait that are removed upon installing this CBM.
     * E.g. enhanced optic bionic may cancel HYPEROPIC trait.
     */
    std::vector<std::string> canceled_mutations;
    /**
     * Additional bionics that are installed automatically when this
     * bionic is installed. This can be used to install several bionics
     * from one CBM item, which is useful as each of those can be
     * activated independently.
     */
    std::vector<std::string> included_bionics;
    bool is_included( const std::string &id ) const;
};

bionic_data const &bionic_info( std::string const &id );

struct bionic : public JsonSerializer, public JsonDeserializer {
    std::string id;
    int         charge  = 0;
    char        invlet  = 'a';
    bool        powered = false;

    bionic()
        : id( "bio_batteries" ) { }
    bionic( std::string pid, char pinvlet )
        : id( std::move( pid ) ), invlet( pinvlet ) { }

    bionic_data const &info() const {
        return bionic_info( id );
    }

    int get_quality( const quality_id &quality ) const;

    using JsonSerializer::serialize;
    void serialize( JsonOut &json ) const override;
    using JsonDeserializer::deserialize;
    void deserialize( JsonIn &jsin ) override;
};

void check_bionics();
void reset_bionics();
void load_bionic( JsonObject &jsobj ); // load a bionic from JSON
bool is_valid_bionic( std::string const &id );
char get_free_invlet( player &p );
std::string list_occupied_bps( const std::string &bio_id, const std::string &intro,
                               const bool each_bp_on_new_line = true );

#endif
