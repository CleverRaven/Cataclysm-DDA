#ifndef _NPC_H_
#define _NPC_H_

#include "player.h"
#include "monster.h"
#include "overmap.h"
#include "faction.h"
#include <vector>
#include <string>
#include <sstream>

#define NPC_LOW_VALUE       5
#define NPC_HI_VALUE        8
#define NPC_VERY_HI_VALUE  15
#define NPC_DANGER_LEVEL   10
#define NPC_DANGER_VERY_LOW 5

class item;
class overmap;
class player;

void parse_tags(std::string &phrase, player *u, npc *me);

/*
 * Talk:   Trust midlow->high, fear low->mid, need doesn't matter
 * Trade:  Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill:   Trust low->midlow, fear low->midlow, need low
 * Flee:   Trust low, fear mid->high, need low
 */

// Attitude is how we feel about the player, what we do around them
enum npc_attitude {
 NPCATT_NULL = 0,	// Don't care/ignoring player
 NPCATT_TALK,		// Move to and talk to player
 NPCATT_TRADE,		// Move to and trade with player
 NPCATT_FOLLOW,		// Follow the player
 NPCATT_FOLLOW_RUN,	// Follow the player, don't shoot monsters
 NPCATT_LEAD,		// Lead the player, wait for them if they're behind
 NPCATT_WAIT,		// Waiting for the player
 NPCATT_DEFEND,		// Kill monsters that threaten the player
 NPCATT_MUG,		// Mug the player
 NPCATT_WAIT_FOR_LEAVE,	// Attack the player if our patience runs out
 NPCATT_KILL,		// Kill the player
 NPCATT_FLEE,		// Get away from the player
 NPCATT_SLAVE,		// Following the player under duress
 NPCATT_HEAL,		// Get to the player and heal them

 NPCATT_MISSING,	// Special; missing NPC as part of mission
 NPCATT_KIDNAPPED,	// Special; kidnapped NPC as part of mission
 NPCATT_MAX
};

std::string npc_attitude_name(npc_attitude);

enum npc_mission {
 NPC_MISSION_NULL = 0,	// Nothing in particular
 NPC_MISSION_RESCUE_U,	// Find the player and aid them
 NPC_MISSION_SHELTER,	// Stay in shelter, introduce player to game
 NPC_MISSION_SHOPKEEP,	// Stay still unless combat or something and sell stuff

 NPC_MISSION_MISSING,	// Special; following player to finish mission
 NPC_MISSION_KIDNAPPED,	// Special; was kidnapped, to be rescued by player
 NUM_NPC_MISSIONS
};

//std::string npc_mission_name(npc_mission);

enum npc_class {
 NC_NONE,
 NC_SHOPKEEP,	// Found in towns.  Stays in his shop mostly.
 NC_HACKER,	// Weak in combat but has hacking skills and equipment
 NC_DOCTOR,	// Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER,	// Roaming trader, journeying between towns.
 NC_NINJA,	// Specializes in unarmed combat, carries few items
 NC_COWBOY,	// Gunslinger and survivalist
 NC_SCIENTIST,	// Uses intelligence-based skills and high-tech items
 NC_BOUNTY_HUNTER, // Resourceful and well-armored
 NC_MAX
};

std::string npc_class_name(npc_class);

enum npc_action {
 npc_undecided = 0,
 npc_pause, //1
 npc_reload, npc_sleep, // 2, 3
 npc_pickup, // 4
 npc_escape_item, npc_wield_melee, npc_wield_loaded_gun, npc_wield_empty_gun,
  npc_heal, npc_use_painkiller, npc_eat, npc_drop_items, // 5 - 12
 npc_flee, npc_melee, npc_shoot, npc_shoot_burst, npc_alt_attack, // 13 - 17
 npc_look_for_player, npc_heal_player, npc_follow_player, npc_talk_to_player,
  npc_mug_player, // 18 - 22
 npc_goto_destination, npc_avoid_friendly_fire, // 23, 24
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

enum npc_favor_type {
 FAVOR_NULL,
 FAVOR_GENERAL,	// We owe you... a favor?
 FAVOR_CASH,	// We owe cash (or goods of equivalent value)
 FAVOR_ITEM,	// We owe a specific item
 FAVOR_TRAINING,// We owe skill or style training
 NUM_FAVOR_TYPES
};

struct npc_favor
{
 npc_favor_type type;
 int value;
 itype_id item_id;
 skill skill_id;

 npc_favor() {
  type = FAVOR_NULL;
  value = 0;
  item_id = itm_null;
  skill_id = sk_null;
 };

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

struct npc_opinion
{
 int trust;
 int fear;
 int value;
 int anger;
 int owed;
 std::vector<npc_favor> favors;

 int total_owed() {
  int ret = owed;
  return ret;
 }

 npc_opinion() {
  trust = 0;
  fear  = 0;
  value = 0;
  anger = 0;
  owed = 0;
 };
 npc_opinion(signed char T, signed char F, signed char V, signed char A, int O):
             trust (T), fear (F), value (V), anger(A), owed (O) { };

 npc_opinion(npc_opinion &copy)
 {
  trust = copy.trust;
  fear = copy.fear;
  value = copy.value;
  anger = copy.anger;
  owed = copy.owed;
  favors.clear();
  for (int i = 0; i < copy.favors.size(); i++)
   favors.push_back( copy.favors[i] );
 };

 npc_opinion& operator+= (npc_opinion &rhs)
 {
  trust += rhs.trust;
  fear  += rhs.fear;
  value += rhs.value;
  anger += rhs.anger;
  owed  += rhs.owed;
  return *this;
 };

/*
 npc_opinion& operator+= (npc_opinion rhs)
 {
  trust += rhs.trust;
  fear  += rhs.fear;
  value += rhs.value;
  anger += rhs.anger;
  owed  += rhs.owed;
  return *this;
 };
*/

 npc_opinion& operator+ (npc_opinion &rhs)
 {
  return (npc_opinion(*this) += rhs);
 };

 std::string save_info()
 {
  std::stringstream ret;
  ret << trust << " " << fear << " " << value << " " << anger << " " << owed <<
         " " << favors.size();
  for (int i = 0; i < favors.size(); i++)
    ret << " " << int(favors[i].type) << " " << favors[i].value << " " <<
           favors[i].item_id << " " << favors[i].skill_id;
  return ret.str();
 }

 void load_info(std::stringstream &info)
 {
  int tmpsize;
  info >> trust >> fear >> value >> anger >> owed >> tmpsize;
  for (int i = 0; i < tmpsize; i++) {
   int tmptype, tmpitem, tmpskill;
   npc_favor tmpfavor;
   info >> tmptype >> tmpfavor.value >> tmpitem >> tmpskill;
   tmpfavor.type = npc_favor_type(tmptype);
   tmpfavor.item_id = itype_id(tmpitem);
   tmpfavor.skill_id = skill(tmpskill);
   favors.push_back(tmpfavor);
  }
 }
};

enum combat_engagement {
 ENGAGE_NONE = 0,
 ENGAGE_CLOSE,
 ENGAGE_WEAK,
 ENGAGE_HIT,
 ENGAGE_ALL
};

struct npc_combat_rules
{
 combat_engagement engagement;
 bool use_guns;
 bool use_grenades;

 npc_combat_rules()
 {
  engagement = ENGAGE_ALL;
  use_guns = true;
  use_grenades = true;
 };

 std::string save_info()
 {
  std::stringstream dump;
  dump << engagement << " " << use_guns << " " << use_grenades << " ";
  return dump.str();
 }

 void load_info(std::istream &data)
 {
  int tmpen;
  data >> tmpen >> use_guns >> use_grenades;
  engagement = combat_engagement(tmpen);
 }
};

enum talk_topic {
 TALK_NONE = 0,	// Used to go back to last subject
 TALK_DONE,	// Used to end the conversation
 TALK_MISSION_LIST, // List available missions. Intentionally placed above START
 TALK_MISSION_LIST_ASSIGNED, // Same, but for assigned missions.

 TALK_MISSION_START, // NOT USED; start of mission topics
 TALK_MISSION_DESCRIBE, // Describe a mission
 TALK_MISSION_OFFER, // Offer a mission
 TALK_MISSION_ACCEPTED,
 TALK_MISSION_REJECTED,
 TALK_MISSION_ADVICE,
 TALK_MISSION_INQUIRE,
 TALK_MISSION_SUCCESS,
 TALK_MISSION_SUCCESS_LIE, // Lie caught!
 TALK_MISSION_FAILURE,
 TALK_MISSION_END, // NOT USED: end of mission topics

 TALK_MISSION_REWARD, // Intentionally placed below END

 TALK_SHELTER,
 TALK_SHELTER_PLANS,
 TALK_SHARE_EQUIPMENT,
 TALK_GIVE_EQUIPMENT,
 TALK_DENY_EQUIPMENT,

 TALK_TRAIN,
 TALK_TRAIN_START,
 TALK_TRAIN_FORCE,

 TALK_SUGGEST_FOLLOW,
 TALK_AGREE_FOLLOW,
 TALK_DENY_FOLLOW,

 TALK_SHOPKEEP,

 TALK_LEADER,
 TALK_LEAVE,
 TALK_PLAYER_LEADS,
 TALK_LEADER_STAYS,
 TALK_HOW_MUCH_FURTHER,

 TALK_FRIEND,
 TALK_COMBAT_COMMANDS,
 TALK_COMBAT_ENGAGEMENT,

 TALK_STRANGER_NEUTRAL,
 TALK_STRANGER_WARY,
 TALK_STRANGER_SCARED,
 TALK_STRANGER_FRIENDLY,
 TALK_STRANGER_AGGRESSIVE,
 TALK_MUG,

 TALK_DESCRIBE_MISSION,

 TALK_WEAPON_DROPPED,
 TALK_DEMAND_LEAVE,

 TALK_SIZE_UP,
 TALK_LOOK_AT,
 TALK_OPINION,

 NUM_TALK_TOPICS
};

struct npc_chatbin
{
 std::vector<int> missions;
 std::vector<int> missions_assigned;
 int mission_selected;
 int tempvalue;
 talk_topic first_topic;

 npc_chatbin()
 {
  mission_selected = -1;
  tempvalue = -1;
  first_topic = TALK_NONE;
 }

 std::string save_info()
 {
  std::stringstream ret;
  ret << first_topic << " " << mission_selected << " " << tempvalue << " " <<
          missions.size() << " " << missions_assigned.size();
  for (int i = 0; i < missions.size(); i++)
   ret << " " << missions[i];
  for (int i = 0; i < missions_assigned.size(); i++)
   ret << " " << missions_assigned[i];
  return ret.str();
 }

 void load_info(std::stringstream &info)
 {
  int tmpsize_miss, tmpsize_assigned, tmptopic;
  info >> tmptopic >> mission_selected >> tempvalue >> tmpsize_miss >>
          tmpsize_assigned;
  first_topic = talk_topic(tmptopic);
  for (int i = 0; i < tmpsize_miss; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions.push_back(tmpmiss);
  }
  for (int i = 0; i < tmpsize_assigned; i++) {
   int tmpmiss;
   info >> tmpmiss;
   missions_assigned.push_back(tmpmiss);
  }
 }
};

class npc : public player {

public:

 npc();
 //npc(npc& rhs);
 npc(const npc &rhs);
 ~npc();
 virtual bool is_npc() { return true; }

 npc& operator= (const npc &rhs);

// Generating our stats, etc.
 void randomize(game *g, npc_class type = NC_NONE);
 void randomize_from_faction(game *g, faction *fac);
 void make_shopkeep(game *g, oter_id type);
 void spawn_at(overmap *o, int posx, int posy);
 skill best_skill();
 void starting_weapon(game *g);


// Save & load
 virtual void load_info(game *g, std::string data);// Overloaded from player
 virtual std::string save_info();


// Display
 void draw(WINDOW* w, int plx, int ply, bool inv);
 void print_info(WINDOW* w);
 std::string short_description();
 std::string opinion_text();

// Goal / mission functions
 void pick_long_term_goal(game *g);
 void perform_mission(game *g);
 int  minutes_to_u(game *g); // Time in minutes it takes to reach player
 bool fac_has_value(faction_value value);
 bool fac_has_job(faction_job job);

// Interaction with the player
 void form_opinion(player *u);
 talk_topic pick_talk_topic(player *u);
 int  player_danger(player *u); // Comparable to monsters
 bool turned_hostile(); // True if our anger is at least equal to...
 int hostile_anger_level(); // ... this value!
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p);
 int assigned_missions_value(game *g);
 std::vector<skill> skills_offered_to(player *p); // Skills that're higher
 std::vector<itype_id> styles_offered_to(player *p); // Martial Arts
// State checks
 bool is_enemy(); // We want to kill/mug/etc the player
 bool is_following(); // Traveling w/ player (whether as a friend or a slave)
 bool is_friend(); // Allies with the player
 bool is_leader(); // Leading the player
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
 void init_buying(inventory you, std::vector<int> &indices,
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
 bool took_painkiller();
 void use_painkiller(game *g);
 void activate_item(game *g, int index);

// Interaction and assessment of the world around us
 int  danger_assessment(game *g);
 int  average_damage_dealt(); // Our guess at how much damage we can deal
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
 void choose_monster_target(game *g, int &enemy, int &danger,
                            int &total_danger);
 npc_action method_of_fleeing	(game *g, int enemy);
 npc_action method_of_attack	(game *g, int enemy, int danger);
 npc_action address_needs	(game *g, int danger);
 npc_action address_player	(game *g);
 npc_action long_term_goal_action(game *g);
 bool alt_attack_available(game *g);	// Do we have grenades, molotov, etc?
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
 npc_action scan_new_items(game *g, int target);

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
 void go_to_destination(game *g); // Move there; on the micro scale
 void reach_destination(game *g); // We made it!

// The preceding are in npcmove.cpp



// #############   VALUES   ################

 npc_attitude attitude;	// What we want to do to the player
 npc_class myclass; // What's our archetype?
 int wandx, wandy, wandf;	// Location of heard sound, etc.

// Location:
 int omx, omy, omz;	// Which overmap (e.g., o.0.0.0)
 int mapx, mapy;// Which square in that overmap (e.g., m.0.0)
 int plx, ply, plt;// Where we last saw the player, timeout to forgetting
 int itx, ity;	// The square containing an item we want
 int goalx, goaly;// Which mapx:mapy square we want to get to

 bool fetching_item;
 bool has_new_items; // If true, we have something new and should re-equip
 int  worst_item_value; // The value of our least-wanted item

 std::vector<point> path;	// Our movement plans


// Personality & other defining characteristics
 int fac_id;	// A temp variable used to inform the game which faction to link
 faction *my_fac;
 npc_mission mission;
 npc_personality personality;
 npc_opinion op_of_u;
 npc_chatbin chatbin;
 int patience; // Used when we expect the player to leave the area
 npc_combat_rules combat_rules;
 bool marked_for_death; // If true, we die as soon as we respawn!
 bool dead;		// If true, we need to be cleaned up
 std::vector<npc_need> needs;
 unsigned flags : NF_MAX;
};

#endif
