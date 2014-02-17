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
            jo.get_string("id"), // "tr_beartrap"
            g->traps.size(),     // tr_beartrap
            jo.get_string("name"), // "bear trap"
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
    trapmap[new_trap->id] = new_trap->loadid;
    traps.push_back(new_trap);
}

// ...why? we're exiting
void game::release_traps()
{
 std::vector<trap*>::iterator it;
 for (it = traps.begin(); it != traps.end(); it++) {
  if (*it != NULL) {
   delete *it;
  }
 }
 traps.clear();
 trapmap.clear();
}

std::map<std::string, int> trapmap;

trap_id trapfind(const std::string id) {
    if( trapmap.find(id) == trapmap.end() ) {
         popup("Can't find trap %s",id.c_str());
         return 0;
    }
    return g->traps[trapmap[id]]->loadid;
};

//////////////////////////
// convenient int-lookup names for hard-coded functions
trap_id
 tr_null,
 tr_bubblewrap,
 tr_cot,
 tr_brazier,
 tr_funnel,
 tr_makeshift_funnel,
 tr_rollmat,
 tr_fur_rollmat,
 tr_beartrap,
 tr_beartrap_buried,
 tr_nailboard,
 tr_caltrops,
 tr_tripwire,
 tr_crossbow,
 tr_shotgun_2,
 tr_shotgun_1,
 tr_engine,
 tr_blade,
 tr_light_snare,
 tr_heavy_snare,
 tr_landmine,
 tr_landmine_buried,
 tr_telepad,
 tr_goo,
 tr_dissector,
 tr_sinkhole,
 tr_pit,
 tr_spike_pit,
 tr_lava,
 tr_portal,
 tr_ledge,
 tr_boobytrap,
 tr_temple_flood,
 tr_temple_toggle,
 tr_glow,
 tr_hum,
 tr_shadow,
 tr_drain,
 tr_snake;

void set_trap_ids() {
 tr_null = trapfind("tr_null");
 tr_bubblewrap = trapfind("tr_bubblewrap");
 tr_cot = trapfind("tr_cot");
 tr_brazier = trapfind("tr_brazier");
 tr_funnel = trapfind("tr_funnel");
 tr_makeshift_funnel = trapfind("tr_makeshift_funnel");
 tr_rollmat = trapfind("tr_rollmat");
 tr_fur_rollmat = trapfind("tr_fur_rollmat");
 tr_beartrap = trapfind("tr_beartrap");
 tr_beartrap_buried = trapfind("tr_beartrap_buried");
 tr_nailboard = trapfind("tr_nailboard");
 tr_caltrops = trapfind("tr_caltrops");
 tr_tripwire = trapfind("tr_tripwire");
 tr_crossbow = trapfind("tr_crossbow");
 tr_shotgun_2 = trapfind("tr_shotgun_2");
 tr_shotgun_1 = trapfind("tr_shotgun_1");
 tr_engine = trapfind("tr_engine");
 tr_blade = trapfind("tr_blade");
 tr_light_snare = trapfind("tr_light_snare");
 tr_heavy_snare = trapfind("tr_heavy_snare");
 tr_landmine = trapfind("tr_landmine");
 tr_landmine_buried = trapfind("tr_landmine_buried");
 tr_telepad = trapfind("tr_telepad");
 tr_goo = trapfind("tr_goo");
 tr_dissector = trapfind("tr_dissector");
 tr_sinkhole = trapfind("tr_sinkhole");
 tr_pit = trapfind("tr_pit");
 tr_spike_pit = trapfind("tr_spike_pit");
 tr_lava = trapfind("tr_lava");
 tr_portal = trapfind("tr_portal");
 tr_ledge = trapfind("tr_ledge");
 tr_boobytrap = trapfind("tr_boobytrap");
 tr_temple_flood = trapfind("tr_temple_flood");
 tr_temple_toggle = trapfind("tr_temple_toggle");
 tr_glow = trapfind("tr_glow");
 tr_hum = trapfind("tr_hum");
 tr_shadow = trapfind("tr_shadow");
 tr_drain = trapfind("tr_drain");
 tr_snake = trapfind("tr_snake");

    // Set ter_t.trap using ter_t.trap_id_str.
    for( std::vector<ter_t>::iterator terrain = terlist.begin();
         terrain != terlist.end(); ++terrain ) {
        if( terrain->trap_id_str.length() != 0 ) {
            terrain->trap = trapfind( terrain->trap_id_str );
        }
    }
}
