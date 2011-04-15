#ifndef _MISSION_H_
#define _MISSION_H_

#include <vector>
#include <string>
#include "game.h"

struct mission;

enum mission_origin {
 ORIGIN_NULL = 0,
 ORIGIN_GAME_START,	// Given when the game starts
 ORIGIN_OPENER_NPC,	// NPC comes up to you when the game starts
 ORIGIN_SECONDARY,	// Given when you complete another mission
 ORIGIN_BLACK_BOX,	// Found on a helicopter's black box
 ORIGIN_RADIO_TOWER,	// Given via radio signal
 ORIGIN_NPC_MISC,	// Given via any NPC
 ORIGIN_NPC_FACTION,	// Given by a NPC representing a faction
 ORIGIN_TOWN_BOARD,	// Given by a settlement's event board
 NUM_ORIGIN
};

enum mission_goal {
 MGOAL_NULL = 0,
 MGOAL_GO_TO,		// Reach a certain overmap tile
 MGOAL_FIND_ITEM,	// Find an item of a given type
 MGOAL_FIND_NPC,	// Find a given NPC
 MGOAL_ASSASSINATE,	// Kill a given NPC
 NUM_MGOAL
};

struct mis_place {	// Return true if [posx,posy] is valid in om
 bool never		(game *g, int posx, int posy) { return false; }
 bool always		(game *g, int posx, int posy) { return true;  }
 bool danger		(game *g, int posx, int posy);
 bool get_jelly		(game *g, int posx, int posy);
 bool lost_npc		(game *g, int posx, int posy);
 bool kidnap_victim	(game *g, int posx, int posy);
};

struct mis_start {	// These functions are run when a mission starts
 void standard		(game *, mission *);
 void get_jelly		(game *, mission *);
 void get_jelly_ignt	(game *, mission *);
 void lost_npc		(game *, mission *);
 void kidnap_victim	(game *, mission *);
};

struct mis_end {	// These functions are run when a mission ends
 void standard		(game *){}; // Nothing special happens
 void join_faction	(game *){}; // Offer to join GOODFAC
 void get_jelly		(game *){}; // Cure whoever was sick
 void get_jelly_ignt	(game *){}; // ^, plus teach about royal jelly
 void lost_npc		(game *){};
 void kidnap_victim	(game *){};
};

struct mission_type {
 std::string name;	// The name the mission is given in menus
 mission_goal goal;	// The basic goal type
 int difficulty;	// Difficulty; TODO: come up with a scale

 std::vector<mission_origin> origins;	// Points of origin
 std::vector<std::string> intros;

 std::vector<itype_id> items;

 bool (mis_place::*place)(game *g, int x, int y);
 void (mis_start::*start)(game *g, mission *miss);
 void (mis_end  ::*end  )(game *g);

 mission_type(std::string NAME, mission_goal GOAL, int DIF,
              bool (mis_place::*PLACE)(game *, int x, int y),
              void (mis_start::*START)(game *, mission *miss),
              void (mis_end::*END)(game *)) :
  name (NAME), goal (GOAL), difficulty (DIF), place (PLACE), start (START),
  end (END) {};
};

struct mission {
 mission_goal goal;
 point target;// Marked on map.  (-1,-1) for none
 int deadline;	// Turn number
 itype_id item_id;
 int count;
 int npc_id;	// ID of a related npc
 int good_fac_id, bad_fac_id;	// IDs of the protagonist/antagonist factions
};

#endif
