#ifndef _BIONICS_H_
#define _BIONICS_H_
#include "json.h"
#include <string>
#include <map>

/* Thought: Perhaps a HUD bionic that changes the display of the game?
 * Showing more information or something. */

typedef std::string bionic_id;

class bionic_data {
public:
     bionic_data(std::string new_name, bool new_power_source, bool new_activated,
              int new_power_cost, int new_charge_time, std::string new_description, bool new_faulty = false);

     std::string name;
     bool power_source;
     bool activated; // If true, then the below function only happens when
                     // activated; otherwise, it happens every turn
     int power_cost;
     int charge_time; // How long, when activated, between drawing power_cost
                      // If 0, it draws power once
     std::string description;
     bool faulty; // Whether or not the bionic is faulty
};

struct bionic : public JsonSerializer, public JsonDeserializer
{
 bionic_id id;
 char invlet;
 bool powered;
 int charge;
 bionic() : id("bio_batteries") {
  invlet = 'a';
  powered = false;
  charge = 0;
 }
 bionic(bionic_id pid, char pinvlet) : id(pid) {
  id = pid;
  invlet = pinvlet;
  powered = false;
  charge = 0;
 };
    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const {
        json.start_object();
        json.member("id", id);
        json.member("invlet", (int)invlet);
        json.member("powered", powered);
        json.member("charge", charge);
        json.end_object();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        id = jo.get_string("id");
        invlet = jo.get_int("invlet");
        powered = jo.get_bool("powered");
        charge = jo.get_int("charge");
    }
};

extern std::map<bionic_id, bionic_data*> bionics;
extern std::vector<bionic_id> faulty_bionics;
extern std::vector<bionic_id> power_source_bionics;
extern std::vector<bionic_id> unpowered_bionics;

void reset_bionics();
void load_bionic(JsonObject &jsobj); // load a bionic from JSON

#endif
