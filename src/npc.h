#ifndef _NPC_H_
#define _NPC_H_

#include "player.h"
#include "monster.h"
#include "overmap.h"
#include "faction.h"
#include "json.h"

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
 NPCATT_NULL = 0, // Don't care/ignoring player The places this is assigned is on shelter NPC generation, and when you order a NPC to stay in a location, and after talking to a NPC that wanted to talk to you.
 NPCATT_TALK,  // Move to and talk to player
 NPCATT_TRADE,  // Move to and trade with player
 NPCATT_FOLLOW,  // Follow the player
 NPCATT_FOLLOW_RUN, // Follow the player, don't shoot monsters
 NPCATT_LEAD,  // Lead the player, wait for them if they're behind
 NPCATT_WAIT,  // Waiting for the player
 NPCATT_DEFEND,  // Kill monsters that threaten the player
 NPCATT_MUG,  // Mug the player
 NPCATT_WAIT_FOR_LEAVE, // Attack the player if our patience runs out
 NPCATT_KILL,  // Kill the player
 NPCATT_FLEE,  // Get away from the player
 NPCATT_SLAVE,  // Following the player under duress
 NPCATT_HEAL,  // Get to the player and heal them

 NPCATT_MISSING, // Special; missing NPC as part of mission
 NPCATT_KIDNAPPED, // Special; kidnapped NPC as part of mission
 NPCATT_MAX
};

std::string npc_attitude_name(npc_attitude);

enum npc_mission {
 NPC_MISSION_NULL = 0, // Nothing in particular
 NPC_MISSION_RESCUE_U, // Find the player and aid them
 NPC_MISSION_SHELTER, // Stay in shelter, introduce player to game
 NPC_MISSION_SHOPKEEP, // Stay still unless combat or something and sell stuff

 NPC_MISSION_MISSING, // Special; following player to finish mission
 NPC_MISSION_KIDNAPPED, // Special; was kidnapped, to be rescued by player

 NPC_MISSION_BASE, // Base Mission: unassigned (Might be used for assigning a npc to stay in a location).

 NUM_NPC_MISSIONS
};

//std::string npc_mission_name(npc_mission);

enum npc_class {
 NC_NONE,
 NC_SHOPKEEP, // Found in towns.  Stays in his shop mostly.
 NC_HACKER, // Weak in combat but has hacking skills and equipment
 NC_DOCTOR, // Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER, // Roaming trader, journeying between towns.
 NC_NINJA, // Specializes in unarmed combat, carries few items
 NC_COWBOY, // Gunslinger and survivalist
 NC_SCIENTIST, // Uses intelligence-based skills and high-tech items
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
 npc_look_for_player, npc_heal_player, npc_follow_player, npc_follow_embarked,
 npc_talk_to_player, npc_mug_player, // 18 - 23
 npc_goto_destination, npc_avoid_friendly_fire, // 24, 25
 npc_base_idle, // 26
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
 FAVOR_GENERAL, // We owe you... a favor?
 FAVOR_CASH, // We owe cash (or goods of equivalent value)
 FAVOR_ITEM, // We owe a specific item
 FAVOR_TRAINING,// We owe skill or style training
 NUM_FAVOR_TYPES
};

struct npc_favor : public JsonSerializer, public JsonDeserializer
{
    npc_favor_type type;
    int value;
    itype_id item_id;
    Skill *skill;

    npc_favor() {
        type = FAVOR_NULL;
        value = 0;
        item_id = "null";
        skill = NULL;
    };

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);
};

struct npc_personality : public JsonSerializer, public JsonDeserializer
{
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

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);
};

struct npc_opinion : public JsonSerializer, public JsonDeserializer
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

 npc_opinion(const npc_opinion &copy): JsonSerializer(), JsonDeserializer()
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

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

 void load_legacy(std::stringstream &info);
};

enum combat_engagement {
 ENGAGE_NONE = 0,
 ENGAGE_CLOSE,
 ENGAGE_WEAK,
 ENGAGE_HIT,
 ENGAGE_ALL
};

struct npc_combat_rules : public JsonSerializer, public JsonDeserializer
{
 combat_engagement engagement;
 bool use_guns;
 bool use_grenades;
 bool use_silent;

 npc_combat_rules()
 {
  engagement = ENGAGE_ALL;
  use_guns = true;
  use_grenades = true;
  use_silent = false;
 };

 void load_legacy(std::istream &data);

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);
};

enum talk_topic {
 TALK_NONE = 0, // Used to go back to last subject
 TALK_DONE, // Used to end the conversation
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

struct npc_chatbin : public JsonSerializer, public JsonDeserializer
{
 std::vector<int> missions;
 std::vector<int> missions_assigned;
 int mission_selected;
 int tempvalue; //No clue what this value does, but it is used all over the place. So it is NOT temp.
 Skill* skill;
 matype_id style;
 talk_topic first_topic;

 npc_chatbin()
 {
  mission_selected = -1;
  tempvalue = -1;
  skill = NULL;
  style = "";
  first_topic = TALK_NONE;
 }

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

 void load_legacy(std::stringstream &info);
};

class npc : public player
{
public:

 npc();
 //npc(npc& rhs);
 npc(const npc &rhs);
 virtual ~npc();
 virtual bool is_player() { return false; }
 virtual bool is_npc() { return true; }

 npc& operator= (const npc &rhs);

// Generating our stats, etc.
 void randomize(npc_class type = NC_NONE);
 void randomize_from_faction(faction *fac);
 void spawn_at(overmap *o, int posx, int posy, int omz);
 void place_near(int potentialX, int potentialY);
 Skill* best_skill();
 void starting_weapon();

// Save & load
 virtual void load_legacy(std::stringstream & dump);// Overloaded from player
 virtual void load_info(std::string data);// Overloaded from player
 virtual std::string save_info();

    using player::deserialize;
    virtual void deserialize(JsonIn &jsin);
    using player::serialize;
    virtual void serialize(JsonOut &jsout, bool save_contents) const;

// Display
 void draw(WINDOW* w, int plx, int ply, bool inv);
 int print_info(WINDOW* w, int column = 1, int line = 6);
 std::string short_description();
 std::string opinion_text();

// Goal / mission functions
 void pick_long_term_goal();
 void perform_mission();
 int  minutes_to_u(); // Time in minutes it takes to reach player
 bool fac_has_value(faction_value value);
 bool fac_has_job(faction_job job);

// Interaction with the player
 void form_opinion(player *u);
 talk_topic pick_talk_topic(player *u);
 int  player_danger(player *u); // Comparable to monsters
 int vehicle_danger(int radius);
 bool turned_hostile(); // True if our anger is at least equal to...
 int hostile_anger_level(); // ... this value!
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p);
 int assigned_missions_value();
 std::vector<Skill*> skills_offered_to(player *p); // Skills that're higher
 std::vector<itype_id> styles_offered_to(player *p); // Martial Arts
// State checks
 bool is_enemy(); // We want to kill/mug/etc the player
 bool is_following(); // Traveling w/ player (whether as a friend or a slave)
 bool is_friend(); // Allies with the player
 bool is_leader(); // Leading the player
 bool is_defending(); // Putting the player's safety ahead of ours
// What happens when the player makes a request
 void told_to_help();
 void told_to_wait();
 void told_to_leave();
 int  follow_distance(); // How closely do we follow the player?
 int  speed_estimate(int speed); // Estimate of a target's speed, usually player


// Dialogue and bartering--see npctalk.cpp
 void talk_to_u();
// Bartering - select items we're willing to buy/sell and set prices
// Prices are later modified by g->u's barter skill; see dialogue.cpp
// init_buying() fills <indices> with the indices of items in <you>
 void init_buying(inventory& you, std::vector<item*> &items,
                  std::vector<int> &prices);
// init_selling() fills <indices> with the indices of items in our inventory
 void init_selling(std::vector<item*> &items, std::vector<int> &prices);


// Use and assessment of items
 int  minimum_item_value(); // The minimum value to want to pick up an item
 void update_worst_item_value(); // Find the worst value in our inventory
 int  value(const item &it);
 bool wear_if_wanted(item it);
 virtual bool wield(signed char invlet, bool);
 virtual bool wield(signed char invlet);
 bool has_healing_item();
 bool has_painkiller();
 bool took_painkiller();
 void use_painkiller();
 void activate_item(char invlet);

// Interaction and assessment of the world around us
 int  danger_assessment();
 int  average_damage_dealt(); // Our guess at how much damage we can deal
 bool bravery_check(int diff);
 bool emergency(int danger);
 bool is_active();
 void say(std::string line, ...);
 void decide_needs();
 void die(Creature* killer);
 void die(bool your_fault = false);
/* shift() works much like monster::shift(), and is called when the player moves
 * from one submap to an adjacent submap.  It updates our position (shifting by
 * 12 tiles), as well as our plans.
 */
 void shift(int sx, int sy);


// Movement; the following are defined in npcmove.cpp
 void move(); // Picks an action & a target and calls execute_action
 void execute_action(npc_action action, int target); // Performs action

// Functions which choose an action for a particular goal
 void choose_monster_target(int &enemy, int &danger,
                            int &total_danger);
 npc_action method_of_fleeing (int target);
 npc_action method_of_attack (int enemy, int danger);
 npc_action address_needs (int danger);
 npc_action address_player ();
 npc_action long_term_goal_action();
 bool alt_attack_available(); // Do we have grenades, molotov, etc?
 signed char  choose_escape_item(); // Returns index of our best escape aid

// Helper functions for ranged combat
 int  confident_range(char invlet = 0); // >= 50% chance to hit
 bool wont_hit_friend(int tarx, int tary, char invlet = 0);
 bool can_reload(); // Wielding a gun that is not fully loaded
 bool need_to_reload(); // Wielding a gun that is empty
 bool enough_time_to_reload(int target, item &gun);

// Physical movement from one tile to the next
 void update_path (int x, int y);
 bool can_move_to (int x, int y);
 void move_to  (int x, int y);
 void move_to_next (); // Next in <path>
 void avoid_friendly_fire(int target); // Maneuver so we won't shoot u
 void move_away_from (int x, int y);
 void move_pause (); // Same as if the player pressed '.'

// Item discovery and fetching
 void find_item  (); // Look around and pick an item
 void pick_up_item (); // Move to, or grab, our targeted item
 void drop_items (int weight, int volume); // Drop wgt and vol
 npc_action scan_new_items(int target);

// Combat functions and player interaction functions
 void melee_monster (int target);
 void melee_player (player &foe);
 void wield_best_melee ();
 void alt_attack (int target);
 void use_escape_item (signed char invlet);
 void heal_player (player &patient);
 void heal_self  ();
 void take_painkiller ();
 void pick_and_eat ();
 void mug_player (player &mark);
 void look_for_player (player &sought);
 bool saw_player_recently();// Do we have an idea of where u are?

// Movement on the overmap scale
 bool has_destination(); // Do we have a long-term destination?
 void set_destination(); // Pick a place to go
 void go_to_destination(); // Move there; on the micro scale
 void reach_destination(); // We made it!

// The preceding are in npcmove.cpp



// #############   VALUES   ################

 npc_attitude attitude; // What we want to do to the player
 npc_class myclass; // What's our archetype?
 int wandx, wandy, wandf; // Location of heard sound, etc.

// Location:
 int omx, omy, omz; // Which overmap (e.g., o.0.0.0)
 int mapx, mapy;// Which square in that overmap (e.g., m.0.0)
 int plx, ply, plt;// Where we last saw the player, timeout to forgetting
 int itx, ity; // The square containing an item we want
 int goalx, goaly, goalz;// Which mapx:mapy square we want to get to

 bool fetching_item;
 bool has_new_items; // If true, we have something new and should re-equip
 int  worst_item_value; // The value of our least-wanted item

 std::vector<point> path; // Our movement plans


// Personality & other defining characteristics
 int fac_id; // A temp variable used to inform the game which faction to link
 faction *my_fac;
 npc_mission mission;
 npc_personality personality;
 npc_opinion op_of_u;
 npc_chatbin chatbin;
 int patience; // Used when we expect the player to leave the area
 npc_combat_rules combat_rules;
 bool marked_for_death; // If true, we die as soon as we respawn!
 bool dead;  // If true, we need to be cleaned up
 bool hit_by_player;
 std::vector<npc_need> needs;
 unsigned flags : NF_MAX;
private:
    void setID (int id);
};

#endif
