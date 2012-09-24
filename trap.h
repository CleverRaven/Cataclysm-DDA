#ifndef _TRAP_H_
#define _TRAP_H_

#include "color.h"
#include "monster.h"
#include "itype.h"
#include <string>

enum trap_id {
 tr_null,
 tr_bubblewrap,
 tr_beartrap,
 tr_beartrap_buried,
 tr_snare,
 tr_nailboard,
 tr_tripwire,
 tr_crossbow,
 tr_shotgun_2,
 tr_shotgun_1,
 tr_engine,
 tr_blade,
 tr_landmine,
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
 void none		(game *g, int x, int y) { };
 void bubble		(game *g, int x, int y);
 void beartrap		(game *g, int x, int y);
 void snare		(game *g, int x, int y) { };
 void board		(game *g, int x, int y);
 void tripwire		(game *g, int x, int y);
 void crossbow		(game *g, int x, int y);
 void shotgun		(game *g, int x, int y);
 void blade		(game *g, int x, int y);
 void landmine		(game *g, int x, int y);
 void telepad		(game *g, int x, int y);
 void goo		(game *g, int x, int y);
 void dissector		(game *g, int x, int y);
 void sinkhole		(game *g, int x, int y);
 void pit		(game *g, int x, int y);
 void pit_spikes	(game *g, int x, int y);
 void lava		(game *g, int x, int y);
 void portal		(game *g, int x, int y) { };
 void ledge		(game *g, int x, int y);
 void boobytrap		(game *g, int x, int y);
 void temple_flood	(game *g, int x, int y);
 void temple_toggle	(game *g, int x, int y);
 void glow		(game *g, int x, int y);
 void hum		(game *g, int x, int y);
 void shadow		(game *g, int x, int y);
 void drain		(game *g, int x, int y);
 void snake		(game *g, int x, int y);
};

struct trapfuncm {
 void none	(game *g, monster *z, int x, int y) { };
 void bubble	(game *g, monster *z, int x, int y);
 void beartrap	(game *g, monster *z, int x, int y);
 void board	(game *g, monster *z, int x, int y);
 void tripwire	(game *g, monster *z, int x, int y);
 void crossbow	(game *g, monster *z, int x, int y);
 void shotgun	(game *g, monster *z, int x, int y);
 void blade	(game *g, monster *z, int x, int y);
 void snare	(game *g, monster *z, int x, int y) { };
 void landmine	(game *g, monster *z, int x, int y);
 void telepad	(game *g, monster *z, int x, int y);
 void goo	(game *g, monster *z, int x, int y);
 void dissector	(game *g, monster *z, int x, int y);
 void sinkhole	(game *g, monster *z, int x, int y) { };
 void pit	(game *g, monster *z, int x, int y);
 void pit_spikes(game *g, monster *z, int x, int y);
 void lava	(game *g, monster *z, int x, int y);
 void portal	(game *g, monster *z, int x, int y) { };
 void ledge	(game *g, monster *z, int x, int y);
 void boobytrap (game *g, monster *z, int x, int y);
 void glow	(game *g, monster *z, int x, int y);
 void hum	(game *g, monster *z, int x, int y);
 void drain	(game *g, monster *z, int x, int y);
 void snake	(game *g, monster *z, int x, int y);
};

struct trap {
 int id;
 char sym;
 nc_color color;
 std::string name;
 
 int visibility;// 1 to ??, affects detection
 int avoidance;	// 0 to ??, affects avoidance
 int difficulty; // 0 to ??, difficulty of assembly & disassembly
 std::vector<itype_id> components;	// For disassembly?
 
// You stepped on it
 void (trapfunc::*act)(game *, int x, int y);
// Monster stepped on it
 void (trapfuncm::*actm)(game *, monster *, int x, int y);
 
 trap(int pid, char psym, nc_color pcolor, std::string pname,
      int pvisibility, int pavoidance, int pdifficulty, 
      void (trapfunc::*pact)(game *, int x, int y),
      void (trapfuncm::*pactm)(game *, monster *, int x, int y), ...) {
  id = pid;
  sym = psym;
  color = pcolor;
  name = pname;
  visibility = pvisibility;
  avoidance = pavoidance;
  difficulty = pdifficulty;
  act = pact;
  actm = pactm;
  va_list ap;
  va_start(ap, pactm);
  itype_id tmp;
  while ((tmp = (itype_id)va_arg(ap, int)))
   components.push_back(tmp);
  va_end(ap);
 };
};

#endif
