#ifndef BIONICS_H
#define BIONICS_H

#include "json.h"
#include <string>
#include <map>

/* Thought: Perhaps a HUD bionic that changes the display of the game?
 * Showing more information or something. */

typedef std::string bionic_id;

class bionic_data
{
    public:
        bionic_data(std::string nname, bool ps, bool tog, int pac, int pad, int pot,
                    int ct, std::string desc, bool fault);

        std::string name;
        bool power_source;
        /** Is true if a bionic is an active instead of a passive bionic */
        bool activated;
        /** If true, then the bionic only has a function when activated, else it causes
         *  it's effect every turn. */
        bool toggled;
        /** Power cost on activation */
        int power_activate;
        /** Power cost on deactivation */
        int power_deactivate;
        /** Power cost over time, does nothing without a non-zero charge_time */
        int power_over_time;
        /** How often a bionic draws power while active in turns */
        int charge_time;
        std::string description;
        /** True if a bionic is a "faulty" bionic */
        bool faulty;
};

struct bionic : public JsonSerializer, public JsonDeserializer {
    bionic_id id;
    char invlet;
    bool powered;
    int charge;
    bionic() : id("bio_batteries")
    {
        invlet = 'a';
        powered = false;
        charge = 0;
    }
    bionic(bionic_id pid, char pinvlet) : id(pid)
    {
        id = pid;
        invlet = pinvlet;
        powered = false;
        charge = 0;
    };
    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const
    {
        json.start_object();
        json.member("id", id);
        json.member("invlet", (int)invlet);
        json.member("powered", powered);
        json.member("charge", charge);
        json.end_object();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin)
    {
        JsonObject jo = jsin.get_object();
        id = jo.get_string("id");
        invlet = jo.get_int("invlet");
        powered = jo.get_bool("powered");
        charge = jo.get_int("charge");
    }
};

extern std::map<bionic_id, bionic_data *> bionics;
extern std::vector<bionic_id> faulty_bionics;
extern std::vector<bionic_id> power_source_bionics;
extern std::vector<bionic_id> unpowered_bionics;

void draw_exam_window(WINDOW *win, int border_line, bool examination);
void reset_bionics();
void load_bionic(JsonObject &jsobj); // load a bionic from JSON

#endif
