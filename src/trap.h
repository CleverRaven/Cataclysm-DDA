#ifndef _TRAP_H_
#define _TRAP_H_

#include "color.h"
#include "monster.h"
#include "itype.h"
#include "json.h"
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
    void none           (int ,  int) {};
    void bubble         (int x, int y);
    void beartrap       (int x, int y);
    void snare_light    (int x, int y);
    void snare_heavy    (int x, int y);
    void board          (int x, int y);
    void caltrops       (int x, int y);
    void tripwire       (int x, int y);
    void crossbow       (int x, int y);
    void shotgun        (int x, int y);
    void blade          (int x, int y);
    void landmine       (int x, int y);
    void telepad        (int x, int y);
    void goo            (int x, int y);
    void dissector      (int x, int y);
    void sinkhole       (int x, int y);
    void pit            (int x, int y);
    void pit_spikes     (int x, int y);
    void lava           (int x, int y);
    void portal         (int x, int y);
    void ledge          (int x, int y);
    void boobytrap      (int x, int y);
    void temple_flood   (int x, int y);
    void temple_toggle  (int x, int y);
    void glow           (int x, int y);
    void hum            (int x, int y);
    void shadow         (int x, int y);
    void drain          (int x, int y);
    void snake          (int x, int y);
};

struct trapfuncm {
    void none           (monster *,  int ,  int ) {};
    void bubble         (monster *z, int x, int y);
    void cot            (monster *z, int x, int y);
    void beartrap       (monster *z, int x, int y);
    void board          (monster *z, int x, int y);
    void caltrops       (monster *z, int x, int y);
    void tripwire       (monster *z, int x, int y);
    void crossbow       (monster *z, int x, int y);
    void shotgun        (monster *z, int x, int y);
    void blade          (monster *z, int x, int y);
    void snare_light    (monster *z, int x, int y);
    void snare_heavy    (monster *z, int x, int y);
    void landmine       (monster *z, int x, int y);
    void telepad        (monster *z, int x, int y);
    void goo            (monster *z, int x, int y);
    void dissector      (monster *z, int x, int y);
    void sinkhole       (monster *z, int x, int y);
    void pit            (monster *z, int x, int y);
    void pit_spikes     (monster *z, int x, int y);
    void lava           (monster *z, int x, int y);
    void portal         (monster *z, int x, int y);
    void ledge          (monster *z, int x, int y);
    void boobytrap      (monster *z, int x, int y);
    void glow           (monster *z, int x, int y);
    void hum            (monster *z, int x, int y);
    void drain          (monster *z, int x, int y);
    void snake          (monster *z, int x, int y);
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
 void (trapfunc::*act)(int x, int y);
// Monster stepped on it
 void (trapfuncm::*actm)(monster *, int x, int y);
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

    trap(int pid, std::string /*string_id*/, std::string pname, nc_color pcolor,
            char psym, int pvisibility, int pavoidance, int pdifficulty,
            void (trapfunc::*pact)(int x, int y),
            void (trapfuncm::*pactm)(monster *, int x, int y),
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

typedef void (trapfunc::*trap_function)(int, int);
typedef void (trapfuncm::*trap_function_mon)(monster *, int, int);
trap_function trap_function_from_string(std::string function_name);
trap_function_mon trap_function_mon_from_string(std::string function_name);

#endif
