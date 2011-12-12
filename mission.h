#ifndef _MISSION_H_
#define _MISSION_H_

#include <vector>
#include <string>
#include "itype.h"
#include "texthash.h"
#include "npc.h"

struct mission;
class game;
enum talk_topic;

enum mission_id {
 MISSION_NULL,
 MISSION_GET_ANTIBIOTICS,
 MISSION_RESCUE_DOG,
 MISSION_KILL_ZOMBIE_MOM,
 NUM_MISSION_IDS
};

std::string mission_dialogue(mission_id id, talk_topic state);

enum mission_origin {
 ORIGIN_NULL = 0,
 ORIGIN_GAME_START,	// Given when the game starts
 ORIGIN_OPENER_NPC,	// NPC comes up to you when the game starts
 NUM_ORIGIN
};

enum mission_goal {
 MGOAL_NULL = 0,
 MGOAL_GO_TO,		// Reach a certain overmap tile
 MGOAL_FIND_ITEM,	// Find an item of a given type
 MGOAL_FIND_MONSTER,	// Find and retrieve a friendly monster
 MGOAL_FIND_NPC,	// Find a given NPC
 MGOAL_ASSASSINATE,	// Kill a given NPC
 MGOAL_KILL_MONSTER,	// Kill a particular hostile monster
 NUM_MGOAL
};

struct mission_place {	// Return true if [posx,posy] is valid in overmap
 bool never		(game *g, int posx, int posy) { return false; }
 bool always		(game *g, int posx, int posy) { return true;  }
 bool near_town		(game *g, int posx, int posy);
};

/* mission_start functions are first run when a mission is accepted; this
 * initializes the mission's key values, like the target and description.
 * These functions are also run once a turn for each active mission, to check
 * if the current goal has been reached.  At that point they either start the
 * goal, or run the appropriate mission_end function.
 */
struct mission_start {
 void standard		(game *, mission *); // Standard for its goal type
 void place_dog		(game *, mission *); // Put a dog in a house!
 void place_zombie_mom	(game *, mission *); // Put a zombie mom in a house!
};

struct mission_end {	// These functions are run when a mission ends
 void standard		(game *, mission *){}; // Nothing special happens
 void heal_infection	(game *, mission *);
};

struct mission_fail {
 void standard		(game *, mission *){}; // Nothing special happens
 void kill_npc		(game *, mission *); // Kill the NPC who assigned it!
};

struct mission_type {
 int id;		// Matches it to a mission_id above
 std::string name;	// The name the mission is given in menus
 mission_goal goal;	// The basic goal type
 int difficulty;	// Difficulty; TODO: come up with a scale
 int value;		// Value; determines rewards and such
 int deadline_low, deadline_high; // Low and high deadlines (turn numbers)
 bool urgent;	// If true, the NPC will press this mission!

 std::vector<mission_origin> origins;	// Points of origin
 itype_id item_id;

 bool (mission_place::*place)(game *g, int x, int y);
 void (mission_start::*start)(game *g, mission *);
 void (mission_end  ::*end  )(game *g, mission *);
 void (mission_fail ::*fail )(game *g, mission *);

 mission_type(int ID, std::string NAME, mission_goal GOAL, int DIF, int VAL,
              bool URGENT,
              bool (mission_place::*PLACE)(game *, int x, int y),
              void (mission_start::*START)(game *, mission *),
              void (mission_end  ::*END  )(game *, mission *),
              void (mission_fail ::*FAIL )(game *, mission *)) :
  id (ID), name (NAME), goal (GOAL), difficulty (DIF), value (VAL),
  urgent(URGENT), place (PLACE), start (START), end (END), fail (FAIL)
  { deadline_low = 0; deadline_high = 0; };

 mission create(game *g); // Create a mission based on this template
};

struct mission {
 mission_type *type;
 std::string description; // Basic descriptive text
 bool failed;		// True if we've failed it!
 int value;
 int uid;		// Unique ID number, used for referencing elsewhere
 point target;		// Marked on the player's map.  (-1,-1) for none
 itype_id item_id;	// Item that needs to be found (or whatever)
 int count;		// How many of that item
 int deadline;		// Turn number
 int npc_id;		// ID of a related npc
 int good_fac_id, bad_fac_id;	// IDs of the protagonist/antagonist factions
 int step;		// How much have we completed?
 text_hash text;

 std::string name();
 mission()
 {
  type = NULL;
  description = "";
  failed = false;
  value = 0;
  uid = -1;
  target = point(-1, -1);
  item_id = itm_null;
  count = 0;
  deadline = 0;
  npc_id = -1;
  good_fac_id = -1;
  bad_fac_id = -1;
  step = 0;
 }
};

#endif
