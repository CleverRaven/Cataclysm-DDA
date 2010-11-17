#ifndef _NPC_H_
#define _NPC_H_

#include "player.h"
#include "monster.h"
#include "item.h"
#include "overmap.h"
#include "faction.h"
#include <vector>
#include <string>

#define NPC_LOW_VALUE      5
#define NPC_HI_VALUE       8
#define NPC_VERY_HI_VALUE 15

class item;
class overmap;

/*
 * Talk: Trust midlow->high, fear low->mid, need doesn't matter
 * Trade: Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill: Trust low->midlow, fear low->midlow, need low
 * Flee: Trust low, fear mid->high, need low
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
 NPCATT_MAX
};

enum npc_mission {
 MISSION_NULL = 0,	// Nothing in particular
 MISSION_RESCUE_U,	// Find the player and aid them
 MISSION_SHOPKEEP,	// Stay still unless combat or something and sell stuff
 NUM_MISSIONS
};

enum npc_class {
 NC_NONE,
 NC_SHOPKEEP,	// Found in towns.  Stays in his shop mostly.
 NC_DOCTOR,	// Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER,	// Roaming trader, journeying between towns.
 NC_NINJA,
 NC_COWBOY,
 NC_SCIENTIST,
 NC_BOUNTY_HUNTER,
 NC_MAX
};

enum npc_action {
 npc_pause = 0,
 npc_reload,
 npc_pickup,
 npc_flee_monster, npc_melee_monster, npc_shoot_monster, npc_alt_attack_monster,
 npc_look_for_player,
 npc_heal_player, npc_follow_player, npc_talk_to_player,
 npc_flee_player, npc_mug_player, npc_melee_player, npc_shoot_player,
  npc_alt_attack_player,
 npc_goto_destination,
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
  bravery = 0;
  collector = 0;
  altruism = 0;
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
 virtual void load_info(std::string data);
 virtual std::string save_info();

 void randomize(game *g, npc_class type = NC_NONE);
 void randomize_from_fact(game *g, faction *fac);
 void make_shopkeep(game *g, oter_id type);
 void set_name();
 void spawn_at(overmap *o, int posx, int posy);
 skill best_skill();
 void starting_weapon(game *g);
 bool wear_if_wanted(item it);

 void perform_mission(game *g);
 int  minutes_to_u(game *g);
 bool fac_has_value(faction_value value);
 bool fac_has_job(faction_job job);

 void form_opinion(player *u);
 void decide_needs();
 void talk_to_u(game *g);
 void say(game *g, std::string line);
 void init_selling(std::vector<int> &indices, std::vector<int> &prices);
 void init_buying(std::vector<item> you, std::vector<int> &indices,
                  std::vector<int> &prices);
 int  value(item &it);

 bool is_friend();
 bool is_following();
 bool is_enemy();

 int  danger_assessment(game *g);
 bool bravery_check(int diff);
 void told_to_help(game *g);
 void told_to_wait(game *g);
 void told_to_leave(game *g);

// Display
 void draw(WINDOW* w, int plx, int ply, bool inv);
 void print_info(WINDOW* w);
// Movement and combat
 void shift(int sx, int sy); 	// Shifts the npc to the appropriate submap
			     	// Updates current pos AND our plans

// The following are defined in npcmove.cpp
 void move(game *g); // Actual movement; depends on target and attitude
 int confident_range();	// Range at which we have 50% chance of a shot hitting
 bool wont_shoot_friend(game *g);// Confident that we won't shoot a friend
 monster* choose_monster_target(game *g);// Most often, the closest to us
 bool want_to_attack_player(game *g);
 int  follow_distance();	// How closely do we follow the player?
 bool can_reload();
 bool saw_player_recently();	// Do we have an idea of where u are?
 bool has_destination();	// Do we have a long-term destination?
 bool alt_attack_available();	// Do we have grenades, molotov, etc?
 npc_action method_of_attacking_player(game *g);
 npc_action method_of_attacking_monster(game *g);
 npc_action long_term_goal_action(game *g);
 void set_destination(game *g);	// Pick a place to go
 void go_to_destination(game *g);

 bool can_move_to	(game *g, int x, int y);
 void move_to		(game *g, int x, int y);
 void move_to_next_in_path(game *g);
 void move_away_from	(game *g, int x, int y);
 void move_pause	();
 void melee_monster(game *g, monster *m);
 void alt_attack(game *g, monster *m, player *p);
 void find_items(game *g);
 void pickup_items(game *g);
 void melee_player(game *g, player &foe);
 void alt_attack_player(game *g, player &foe);
 void heal_player(game *g, player &patient);
 void mug_player(game *g, player &mark);
 void look_for_player(game *g);
// The preceding are in npcmove.cpp

 void die(game *g, bool your_fault = false);

 monster *target;	// Current monster we want to kill
 npc_attitude attitude;	// What we want to do to the player
 int wandx, wandy, wandf;	// Location of heard sound, etc.

// Location:
 int omx, omy, omz;	// Which overmap (e.g., o.0.0.0)
 int mapx, mapy;// Which square in that overmap (e.g., m.0.0)
 int plx, ply, plt;// Where we last saw the player, timeout to forgetting
// int itx, ity;	// The square containing an item we want
 int goalx, goaly;// Which mapx:mapy square we want to get to

 bool fetching_item;

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
