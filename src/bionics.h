#ifndef BIONICS_H
#define BIONICS_H

#include "json.h"
#include "output.h" // WINDOW
#include <string>

/* Thought: Perhaps a HUD bionic that changes the display of the game?
 * Showing more information or something. */

struct bionic_data {
    bionic_data() = default;
    bionic_data( std::string nname, bool ps, bool tog, int pac, int pad, int pot,
                 int ct, int cap, std::string desc, bool fault );

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
        *  it's effect every turn. */
    bool toggled = false;
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

    using JsonSerializer::serialize;
    void serialize( JsonOut &json ) const override;
    using JsonDeserializer::deserialize;
    void deserialize( JsonIn &jsin ) override;
};

void draw_exam_window( WINDOW *win, int border_line, bool examination );
void reset_bionics();
void load_bionic( JsonObject &jsobj ); // load a bionic from JSON
bool is_valid_bionic( std::string const &id );

#endif
