#ifndef _NPC_H_
#define _NPC_H_

#include "player.h"
#include "monster.h"
#include "item.h"
#include "overmap.h"
#include "faction.h"
#include <vector>
#include <string>

#define NPC_LOW_VALUE       5
#define NPC_HI_VALUE        8
#define NPC_VERY_HI_VALUE  15
#define NPC_DANGER_LEVEL   10
#define NPC_DANGER_VERY_LOW 5

class item;
class overmap;

void parse_tags(std::string &phrase, player *u, npc *me);

/*
 * Talk:   Trust midlow->high, fear low->mid, need doesn't matter
 * Trade:  Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill:   Trust low->midlow, fear low->midlow, need low
 * Flee:   Trust low, fear mid->high, need low
 */

enum npc_attitude {
 NPCATT_NULL = 0,	// Don't care/ignoring player
 NPCATT_TALK,		// Move to and talk to player
 NPCATT_TRADE,		// Move to and trade with player
 NPCATT_FOLLOW,		// Follow the player
 NPCATT_FOLLOW_RUN,	// Follow the player, don't shoot monsters
 NPCATT_WAIT,		// Waiting for the player
 NPCATT_DEFEND,		// Kill monsters that threaten the player
 NPCATT_MUG,		// Mug the player
 NPCATT_KILL,		// Kill the player
 NPCATT_FLEE,		// Get away from the player
 NPCATT_SLAVE,		// Following the player under duress
 NPCATT_HEAL,		// Get to the player and heal them

 NPCATT_MISSING,	// Special; missing NPC as part of mission
 NPCATT_KIDNAPPED,	// Special; kidnapped NPC as part of mission
 NPCATT_MAX
};

enum npc_mission {
 NPC_MISSION_NULL = 0,	// Nothing in particular
 NPC_MISSION_RESCUE_U,	// Find the player and aid them
 NPC_MISSION_SHOPKEEP,	// Stay still unless combat or something and sell stuff

 NPC_MISSION_MISSING,	// Special; following player to finish mission
 NPC_MISSION_KIDNAPPED,	// Special; was kidnapped, to be rescued by player
 NUM_NPC_MISSIONS
};

enum npc_class {
 NC_NONE,
 NC_SHOPKEEP,	// Found in towns.  Stays in his shop mostly.
 NC_DOCTOR,	// Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER,	// Roaming trader, journeying between towns.
 NC_NINJA,	// Specializes in unarmed combat, carries few items
 NC_COWBOY,	// Gunslinger and survivalist
 NC_SCIENTIST,	// Uses intelligence-based skills and high-tech items
 NC_BOUNTY_HUNTER, // Resourceful and well-armored
 NC_MAX
};

enum npc_action {
 npc_undecided = 0,
 npc_pause,
 npc_reload, npc_sleep,
 npc_pickup,
 npc_escape_item, npc_wield_melee, npc_wield_loaded_gun, npc_wield_empty_gun,
  npc_heal, npc_use_painkiller, npc_eat, npc_drop_items,
 npc_flee, npc_melee, npc_shoot, npc_shoot_burst, npc_alt_attack,
 npc_look_for_player, npc_heal_player, npc_follow_player, npc_talk_to_player,
  npc_mug_player,
 npc_goto_destination, npc_avoid_friendly_fire,
 num_npc_actions
};

enum npc_need {
 need_none,
 need_ammo, need_weapon, need_gun,
 need_food, need_drink,
 num_needs
};

enum npc_flag {
 NF_NULL,
// Items desired
 NF_FOOD_HOARDER,
 NF_DRUGGIE,
 NF_TECHNOPHILE,
 NF_BOOKWORM,
 NF_MAX
};

struct npc_personality {
// All values should be in the -10 to 10 range.
 signed char aggression;
 signed char bravery;
 signed char collector;
 signed char altruism;
 npc_personality() {
  aggression = 0;
  bravery    = 0;
  collector  = 0;
  altruism   = 0;
 };
};

struct npc_opinion {
 signed char trust;
 signed char fear;
 signed char value;
 npc_opinion() {
  trust = 0;
  fear  = 0;
  value = 0;
 };
};

class npc : public player {

public:

 npc();
 ~npc();
 virtual bool is_npc() { return true; }


// Generating our stats, etc.
 void randomize(game *g, npc_class type = NC_NONE);
 void randomize_from_faction(game *g, faction *fac);
 void make_shopkeep(game *g, oter_id type);
 void pick_name(); // Picks a name from NAMES_*
 void spawn_at(overmap *o, int posx, int posy);
 skill best_skill();
 void starting_weapon(game *g);


// Save & load
 virtual void load_info(std::string data);// Overloaded from player::load_info()
 virtual std::string save_info();


// Display
 void draw(WINDOW* w, int plx, int ply, bool inv);
 void print_info(WINDOW* w);


// Goal / mission functions
 void pick_long_term_goal(game *g);
 void perform_mission(game *g);
 int  minutes_to_u(game *g); // Time in minutes it takes to reach player
 bool fac_has_value(faction_value value);
 bool fac_has_job(faction_job job);


// Interaction with the player
 void form_opinion(player *u);
 int  player_danger(player *u); // Comparable to monsters
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p);
// State checks
 bool is_enemy(); // We want to kill/mug/etc the player
 bool is_following(); // Traveling w/ player (whether as a friend or a slave)
 bool is_friend(); // Allies with the player
 bool is_defending(); // Putting the player's safety ahead of ours
// What happens when the player makes a request
 void told_to_help(game *g);
 void told_to_wait(game *g);
 void told_to_leave(game *g);
 int  follow_distance();	// How closely do we follow the player?
 int  speed_estimate(int speed); // Estimate of a target's speed, usually player


// Dialogue and bartering--see npctalk.cpp
 void talk_to_u(game *g);
// Bartering - select items we're willing to buy/sell and set prices
// Prices are later modified by g->u's barter skill; see dialogue.cpp
// init_buying() fills <indices> with the indices of items in <you>
 void init_buying(std::vector<item> you, std::vector<int> &indices,
                  std::vector<int> &prices);
// init_selling() fills <indices> with the indices of items in our inventory
 void init_selling(std::vector<int> &indices, std::vector<int> &prices);


// Use and assessment of items
 int  minimum_item_value(); // The minimum value to want to pick up an item
 void update_worst_item_value(); // Find the worst value in our inventory
 int  value(item &it);
 bool wear_if_wanted(item it);
 virtual bool wield(game *g, int index);
 bool has_healing_item();
 bool has_painkiller();
 void use_painkiller(game *g);


// Interaction and assessment of the world around us
 int  danger_assessment(game *g);
 bool bravery_check(int diff);
 bool emergency(int danger);
 void say(game *g, std::string line, ...);
 void decide_needs();
 void die(game *g, bool your_fault = false);
/* shift() works much like monster::shift(), and is called when the player moves
 * from one submap to an adjacent submap.  It updates our position (shifting by
 * 12 tiles), as well as our plans.
 */
 void shift(int sx, int sy); 


// Movement; the following are defined in npcmove.cpp
 void move(game *g); // Picks an action & a target and calls execute_action
 void execute_action(game *g, npc_action action, int target); // Performs action

// Functions which choose an action for a particular goal
 void choose_monster_target(game *g, int &enemy, int &danger);
 npc_action method_of_fleeing	(game *g, int enemy);
 npc_action method_of_attack	(game *g, int enemy, int danger);
 npc_action address_needs	(game *g, int danger);
 npc_action address_player	(game *g);
 npc_action long_term_goal_action(game *g);
 bool alt_attack_available();	// Do we have grenades, molotov, etc?
 int  choose_escape_item(); // Returns index of our best escape aid

// Helper functions for ranged combat
 int  confident_range(int index = -1); // >= 50% chance to hit
 bool wont_hit_friend(game *g, int tarx, int tary, int index = -1);
 bool can_reload(); // Wielding a gun that is not fully loaded
 bool need_to_reload(); // Wielding a gun that is empty
 bool enough_time_to_reload(game *g, int target, item &gun);

// Physical movement from one tile to the next
 void update_path	(game *g, int x, int y);
 bool can_move_to	(game *g, int x, int y);
 void move_to		(game *g, int x, int y);
 void move_to_next	(game *g); // Next in <path>
 void avoid_friendly_fire(game *g, int target); // Maneuver so we won't shoot u
 void move_away_from	(game *g, int x, int y);
 void move_pause	(); // Same as if the player pressed '.'

// Item discovery and fetching
 void find_item		(game *g); // Look around and pick an item
 void pick_up_item	(game *g); // Move to, or grab, our targeted item
 void drop_items	(game *g, int weight, int volume); // Drop wgt and vol

// Combat functions and player interaction functions
 void melee_monster	(game *g, int target);
 void melee_player	(game *g, player &foe);
 void wield_best_melee	(game *g);
 void alt_attack	(game *g, int target);
 void use_escape_item	(game *g, int index, int target);
 void heal_player	(game *g, player &patient);
 void heal_self		(game *g);
 void take_painkiller	(game *g);
 void pick_and_eat	(game *g);
 void mug_player	(game *g, player &mark);
 void look_for_player	(game *g, player &sought);
 bool saw_player_recently();// Do we have an idea of where u are?

// Movement on the overmap scale
 bool has_destination();	// Do we have a long-term destination?
 void set_destination(game *g);	// Pick a place to go
 void go_to_destination(game *g);

// The preceding are in npcmove.cpp



// #############   VALUES   ################

 int id;	// A unique ID number, assigned by the game class
 npc_attitude attitude;	// What we want to do to the player
 int wandx, wandy, wandf;	// Location of heard sound, etc.

// Location:
 int omx, omy, omz;	// Which overmap (e.g., o.0.0.0)
 int mapx, mapy;// Which square in that overmap (e.g., m.0.0)
 int plx, ply, plt;// Where we last saw the player, timeout to forgetting
 int itx, ity;	// The square containing an item we want
 int goalx, goaly;// Which mapx:mapy square we want to get to

 bool fetching_item;
 int  worst_item_value; // The value of our least-wanted item

 std::vector<point> path;	// Our movement plans


// Personality & other defining characteristics
 int fac_id;	// A temp variable used to inform the game which faction to link
 faction *my_fac;
 npc_mission mission;
 npc_personality personality;
 npc_opinion op_of_u;
 std::vector<npc_need> needs;
 unsigned flags : NF_MAX;
};

#endif
