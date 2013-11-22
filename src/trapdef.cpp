#include <vector>
#include "game.h"

void game::load_trap(JsonObject &jo)
{
    std::vector<std::string> drops;
    if(jo.has_member("drops")) {
        JsonArray drops_list = jo.get_array("drops");
        while(drops_list.has_more()) {
            drops.push_back(drops_list.next_string());
        }
    }

    trap *new_trap = new trap(
            jo.get_int("legacy_id"),
            jo.get_string("id"),
            jo.get_string("name"),
            color_from_string(jo.get_string("color")),
            jo.get_string("symbol").at(0),
            jo.get_int("visibility"),
            jo.get_int("avoidance"),
            jo.get_int("difficulty"),
            trap_function_from_string(jo.get_string("player_action")),
            trap_function_mon_from_string(jo.get_string("monster_action")),
            drops
    );
    new_trap->benign = jo.get_bool("benign", false);
    new_trap->funnel_radius_mm = jo.get_int("funnel_radius", 0);
    traps.push_back(new_trap);
}

void game::release_traps()
{
 std::vector<trap*>::iterator it;
 for (it = traps.begin(); it != traps.end(); it++) {
  if (*it != NULL) {
   delete *it;
  }
 }
 traps.clear();
}
