#ifndef _MISSION_H_
#define _MISSION_H_

#include <vector>
#include <string>
#include "itype.h"

struct mission;
class game;

enum mission_id {
 MISSION_REACH_SAFETY,
 MISSION_FIND_FAMILY,
 MISSION_FOLLOW_TO_SAFETY,
 MISSION_MEET_FACTION,
 MISSION_GET_JELLY,
 MISSION_GET_JELLY_IGNT,
 MISSION_FIND_NPC,
 MISSION_RESCUE_KIDNAPPED,
 NUM_MISSION_IDS
};

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

struct mission_place {	// Return true if [posx,posy] is valid in overmap
 bool never		(game *g, int posx, int posy) { return false; }
 bool always		(game *g, int posx, int posy) { return true;  }
 bool danger		(game *g, int posx, int posy);
 bool get_jelly		(game *g, int posx, int posy);
 bool lost_npc		(game *g, int posx, int posy);
 bool kidnap_victim	(game *g, int posx, int posy);
};

/* mission_start functions are first run when a mission is accepted; this
 * initializes the mission's key values, like the target and description.
 * These functions are also run once a turn for each active mission, to check
 * if the current goal has been reached.  At that point they either start the
 * goal, or run the appropriate mission_end function.
 */
struct mission_start {
 void standard		(game *, mission *);
 void danger		(game *, mission *);
 void find_family	(game *, mission *);
 void get_jelly		(game *, mission *);
 void get_jelly_ignt	(game *, mission *);
 void lost_npc		(game *, mission *);
 void kidnap_victim	(game *, mission *);
};

struct mission_end {	// These functions are run when a mission ends
 void standard		(game *){}; // Nothing special happens
 void find_family	(game *){};
 void join_faction	(game *){}; // Offer to join GOODFAC
 void get_jelly		(game *){}; // Cure whoever was sick
 void get_jelly_ignt	(game *){}; // ^, plus teach about royal jelly
 void lost_npc		(game *){};
 void kidnap_victim	(game *){};
 void demo		(game *){};
};

struct mission_type {
 std::string name;	// The name the mission is given in menus
 mission_goal goal;	// The basic goal type
 int difficulty;	// Difficulty; TODO: come up with a scale
 int deadline_low, deadline_high; // Low and high deadlines (turn numbers)

 std::vector<mission_origin> origins;	// Points of origin
 std::vector<std::string> intros;

 std::vector<itype_id> items;

 bool (mission_place::*place)(game *g, int x, int y);
 void (mission_start::*start)(game *g, mission *);
 void (mission_end  ::*end  )(game *g);

 mission_type(std::string NAME, mission_goal GOAL, int DIF,
              bool (mission_place::*PLACE)(game *, int x, int y),
              void (mission_start::*START)(game *, mission *),
              void (mission_end::*END)(game *)) :
  name (NAME), goal (GOAL), difficulty (DIF), place (PLACE), start (START),
  end (END) { deadline_low = 0; deadline_high = 0; };

 mission create(game *g); // Create a mission based on this template
};

struct mission {
 mission_type *type;
 std::string description; // Basic descriptive text
 point target;	// Marked on the player's map.  (-1,-1) for none
 int deadline;	// Turn number
 itype_id item_id;	// Item that needs to be found (or whatever)
 int count;		// How many of that item
 int npc_id;		// ID of a related npc
 int good_fac_id, bad_fac_id;	// IDs of the protagonist/antagonist factions

 std::string name();
};

#endif
