#ifndef _TRAP_H_
#define _TRAP_H_

#include "color.h"
#include "monster.h"
#include "itype.h"
#include <string>

/*
  On altering any entries in this enum please add or remove the appropriate entry to the trap_names array in tile_id_data.h
*/
enum trap_id {
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
 tr_snare,
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
 tr_snake,
 num_trap_types
};

struct trap;

struct trapfunc {
    void none           (game *g, int x, int y) { };
    void bubble         (game *g, int x, int y);
    void beartrap       (game *g, int x, int y);
    void snare_light    (game *g, int x, int y);
    void snare_heavy    (game *g, int x, int y);
    void snare          (game *g, int x, int y) { };
    void board          (game *g, int x, int y);
    void caltrops       (game *g, int x, int y);
    void tripwire       (game *g, int x, int y);
    void crossbow       (game *g, int x, int y);
    void shotgun        (game *g, int x, int y);
    void blade          (game *g, int x, int y);
    void landmine       (game *g, int x, int y);
    void telepad        (game *g, int x, int y);
    void goo            (game *g, int x, int y);
    void dissector      (game *g, int x, int y);
    void sinkhole       (game *g, int x, int y);
    void pit            (game *g, int x, int y);
    void pit_spikes     (game *g, int x, int y);
    void lava           (game *g, int x, int y);
    void portal         (game *g, int x, int y) { };
    void ledge          (game *g, int x, int y);
    void boobytrap      (game *g, int x, int y);
    void temple_flood   (game *g, int x, int y);
    void temple_toggle  (game *g, int x, int y);
    void glow           (game *g, int x, int y);
    void hum            (game *g, int x, int y);
    void shadow         (game *g, int x, int y);
    void drain          (game *g, int x, int y);
    void snake          (game *g, int x, int y);
};

struct trapfuncm {
    void none           (game *g, monster *z, int x, int y) { };
    void bubble         (game *g, monster *z, int x, int y);
    void cot            (game *g, monster *z, int x, int y);
    void beartrap       (game *g, monster *z, int x, int y);
    void board          (game *g, monster *z, int x, int y);
    void caltrops       (game *g, monster *z, int x, int y);
    void tripwire       (game *g, monster *z, int x, int y);
    void crossbow       (game *g, monster *z, int x, int y);
    void shotgun        (game *g, monster *z, int x, int y);
    void blade          (game *g, monster *z, int x, int y);
    void snare_light    (game *g, monster *z, int x, int y);
    void snare_heavy    (game *g, monster *z, int x, int y);
    void snare          (game *g, monster *z, int x, int y) { };
    void landmine       (game *g, monster *z, int x, int y);
    void telepad        (game *g, monster *z, int x, int y);
    void goo            (game *g, monster *z, int x, int y);
    void dissector      (game *g, monster *z, int x, int y);
    void sinkhole       (game *g, monster *z, int x, int y) { };
    void pit            (game *g, monster *z, int x, int y);
    void pit_spikes     (game *g, monster *z, int x, int y);
    void lava           (game *g, monster *z, int x, int y);
    void portal         (game *g, monster *z, int x, int y) { };
    void ledge          (game *g, monster *z, int x, int y);
    void boobytrap      (game *g, monster *z, int x, int y);
    void glow           (game *g, monster *z, int x, int y);
    void hum            (game *g, monster *z, int x, int y);
    void drain          (game *g, monster *z, int x, int y);
    void snake          (game *g, monster *z, int x, int y);
};

struct trap {
 int id;
 long sym;
 nc_color color;
 std::string name;

 int visibility; // 1 to ??, affects detection
 int avoidance;  // 0 to ??, affects avoidance
 int difficulty; // 0 to ??, difficulty of assembly & disassembly
 std::vector<itype_id> components; // For disassembly?

// You stepped on it
 void (trapfunc::*act)(game *, int x, int y);
// Monster stepped on it
 void (trapfuncm::*actm)(game *, monster *, int x, int y);
// Type of trap
 bool is_benign();
 bool benign;

 // non-generic numbers for special cases
 int funnel_radius_mm;
 double funnel_turns_per_charge( double rain_depth_mm_per_hour ) const;
 /* pending jsonize
 std::set<std::string> flags
 std::string id;
 */

 trap(int pid, std::string string_id, std::string pname, nc_color pcolor, char psym,
      int pvisibility, int pavoidance, int pdifficulty,
      void (trapfunc::*pact)(game *, int x, int y),
      void (trapfuncm::*pactm)(game *, monster *, int x, int y),
      std::vector<std::string> keys) {
  //string_id is ignored at the moment, will later replace the id 
  id = pid;
  sym = psym;
  color = pcolor;
  name = pname;
  visibility = pvisibility;
  avoidance = pavoidance;
  difficulty = pdifficulty;
  act = pact;
  actm = pactm;

  components.insert(components.end(), keys.begin(), keys.end());

  // It's a traaaap! So default;
  benign = false;
  // Traps are not typically funnels
  funnel_radius_mm = 0;
 };
};

trap_id trap_id_from_string(std::string trap_name);

#endif
