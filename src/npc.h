#ifndef NPC_H
#define NPC_H

#include "messages.h"
#include "player.h"
#include "faction.h"
#include "json.h"

#include <vector>
#include <string>
#include <map>

#define NPC_LOW_VALUE       5
#define NPC_HI_VALUE        8
#define NPC_VERY_HI_VALUE  15
#define NPC_DANGER_LEVEL   10
#define NPC_DANGER_VERY_LOW 5

class item;
class overmap;
class player;
class field_entry;

void parse_tags(std::string &phrase, const player *u, const npc *me);

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
 NPC_MISSION_GUARD, // Similar to Base Mission, for use outside of camps

 NUM_NPC_MISSIONS
};

//std::string npc_mission_name(npc_mission);

enum npc_class {
 NC_NONE,
 NC_EVAC_SHOPKEEP,  // Found in the Evacuation Center, unique, has more goods than he should be able to carry
 NC_SHOPKEEP,       // Found in towns.  Stays in his shop mostly.
 NC_HACKER,         // Weak in combat but has hacking skills and equipment
 NC_DOCTOR,         // Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER,         // Roaming trader, journeying between towns.
 NC_NINJA,          // Specializes in unarmed combat, carries few items
 NC_COWBOY,         // Gunslinger and survivalist
 NC_SCIENTIST,      // Uses intelligence-based skills and high-tech items
 NC_BOUNTY_HUNTER,  // Resourceful and well-armored
 NC_THUG,           // Moderate melee skills and poor equipment
 NC_SCAVENGER,      // Good with pistols light weapons
 NC_ARSONIST,       // Evacuation Center, restocks moltovs and anarcist type stuff
 NC_HUNTER,         // Survivor type good with bow or rifle
 NC_MAX
};

std::string npc_class_name(npc_class);
std::string npc_class_name_str(npc_class);

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
    const Skill* skill;

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
 TALK_GUARD, // End conversation, nothing to be said
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

 TALK_EVAC_MERCHANT, //17, Located in Refugee Center
 TALK_EVAC_MERCHANT_NEW,
 TALK_EVAC_MERCHANT_PLANS,
 TALK_EVAC_MERCHANT_PLANS2,
 TALK_EVAC_MERCHANT_PLANS3,
 TALK_EVAC_MERCHANT_WORLD,
 TALK_EVAC_MERCHANT_HORDES,
 TALK_EVAC_MERCHANT_PRIME_LOOT,
 TALK_EVAC_MERCHANT_ASK_JOIN,
 TALK_EVAC_MERCHANT_NO,
 TALK_EVAC_MERCHANT_HELL_NO,

 TALK_FREE_MERCHANT_STOCKS,//28, Located in Refugee Center
 TALK_FREE_MERCHANT_STOCKS_NEW,
 TALK_FREE_MERCHANT_STOCKS_WHY,
 TALK_FREE_MERCHANT_STOCKS_ALL,
 TALK_FREE_MERCHANT_STOCKS_JERKY,
 TALK_FREE_MERCHANT_STOCKS_CORNMEAL,
 TALK_FREE_MERCHANT_STOCKS_FLOUR,
 TALK_FREE_MERCHANT_STOCKS_SUGAR,
 TALK_FREE_MERCHANT_STOCKS_WINE,
 TALK_FREE_MERCHANT_STOCKS_BEER,
 TALK_FREE_MERCHANT_STOCKS_SMMEAT,
 TALK_FREE_MERCHANT_STOCKS_SMFISH,
 TALK_FREE_MERCHANT_STOCKS_OIL,
 TALK_FREE_MERCHANT_STOCKS_DELIVERED,

 TALK_EVAC_GUARD1,//42, Located in Refugee Center
 TALK_EVAC_GUARD1_PLACE,
 TALK_EVAC_GUARD1_GOVERNMENT,
 TALK_EVAC_GUARD1_TRADE,
 TALK_EVAC_GUARD1_JOIN,
 TALK_EVAC_GUARD1_JOIN2,
 TALK_EVAC_GUARD1_JOIN3,
 TALK_EVAC_GUARD1_ATTITUDE,
 TALK_EVAC_GUARD1_JOB,
 TALK_EVAC_GUARD1_OLDGUARD,
 TALK_EVAC_GUARD1_BYE,

 TALK_EVAC_GUARD2,//53, Located in Refugee Center
 TALK_EVAC_GUARD2_NEW,
 TALK_EVAC_GUARD2_RULES,
 TALK_EVAC_GUARD2_RULES_BASEMENT,
 TALK_EVAC_GUARD2_WHO,
 TALK_EVAC_GUARD2_TRADE,

 TALK_EVAC_GUARD3,//59, Located in Refugee Center
 TALK_EVAC_GUARD3_NEW,
 TALK_EVAC_GUARD3_RULES,
 TALK_EVAC_GUARD3_HIDE1,
 TALK_EVAC_GUARD3_HIDE2,
 TALK_EVAC_GUARD3_WASTE,
 TALK_EVAC_GUARD3_DEAD,
 TALK_EVAC_GUARD3_HOSTILE,
 TALK_EVAC_GUARD3_INSULT,

 TALK_EVAC_HUNTER,//68, Located in Refugee Center
 TALK_EVAC_HUNTER_SMELL,
 TALK_EVAC_HUNTER_DO,
 TALK_EVAC_HUNTER_LIFE,
 TALK_EVAC_HUNTER_HUNT,
 TALK_EVAC_HUNTER_SALE,
 TALK_EVAC_HUNTER_ADVICE,
 TALK_EVAC_HUNTER_BYE,

 TALK_OLD_GUARD_REP,//76, Located in Refugee Center
 TALK_OLD_GUARD_REP_NEW,
 TALK_OLD_GUARD_REP_NEW_DOING,
 TALK_OLD_GUARD_REP_NEW_DOWNSIDE,
 TALK_OLD_GUARD_REP_WORLD,
 TALK_OLD_GUARD_REP_WORLD_2NDFLEET,
 TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS,
 TALK_OLD_GUARD_REP_ASK_JOIN,

 TALK_ARSONIST,//84, Located in Refugee Center
 TALK_ARSONIST_NEW,
 TALK_ARSONIST_DOING,
 TALK_ARSONIST_DOING_REBAR,
 TALK_ARSONIST_WORLD,
 TALK_ARSONIST_WORLD_OPTIMISTIC,
 TALK_ARSONIST_JOIN,
 TALK_ARSONIST_MUTATION,
 TALK_ARSONIST_MUTATION_INSULT,

 TALK_SCAVENGER_MERC,//93, Located in Refugee Center
 TALK_SCAVENGER_MERC_NEW,
 TALK_SCAVENGER_MERC_TIPS,
 TALK_SCAVENGER_MERC_HIRE,
 TALK_SCAVENGER_MERC_HIRE_SUCCESS,

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
 TALK_FRIEND_GUARD,
 TALK_DENY_GUARD,
 TALK_DENY_TRAIN,
 TALK_DENY_PERSONAL,
 TALK_FRIEND_UNCOMFORTABLE,
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
 const Skill* skill;
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

class npc;

typedef std::map<std::string, npc> npc_map;

class npc : public player
{
public:

 npc();
 npc(const npc &) = default;
 npc(npc &&) = default;
 npc &operator=(const npc &) = default;
 npc &operator=(npc &&) = default;
 virtual ~npc();
 virtual bool is_player() const { return false; }
 virtual bool is_npc() const { return true; }

 static void load_npc(JsonObject &jsobj);
 npc* find_npc(std::string ident);
 void load_npc_template(std::string ident);

// Generating our stats, etc.
 void randomize(npc_class type = NC_NONE);
 void randomize_from_faction(faction *fac);
 void set_fac(std::string fac_name);
    /**
     * Set @ref mapx and @ref mapx and @ref mapz.
     * @param mx,my,mz are global submap coordinates.
     * This function also adds the npc object to the overmap.
     */
    void spawn_at(int mx, int my, int mz);
    /**
     * Calls @ref spawn_at, spawns in a random city in
     * the given overmap on z-level 0.
     */
    void spawn_at_random_city(overmap *o);
    /**
     * Places the NPC on the @ref map. This update its
     * posx,posy and mapx,mapy values to fit the current offset of
     * map (g->levx, g->levy).
     * If the square on the map where the NPC would go is not empty
     * a spiral search for an empty square around it is performed.
     */
    void place_on_map();
 const Skill* best_skill();
 void starting_weapon(npc_class type);

// Save & load
 virtual void load_legacy(std::stringstream & dump);// Overloaded from player
 virtual void load_info(std::string data);// Overloaded from player
 virtual std::string save_info();

    using player::deserialize;
    virtual void deserialize(JsonIn &jsin);
    using player::serialize;
    virtual void serialize(JsonOut &jsout) const override;

// Display
    virtual nc_color basic_symbol_color() const;
 int print_info(WINDOW* w, int vStart, int vLines, int column) const;
 std::string short_description() const;
 std::string opinion_text() const;

// Goal / mission functions
 void pick_long_term_goal();
 void perform_mission();
 int  minutes_to_u() const; // Time in minutes it takes to reach player
 bool fac_has_value(faction_value value);
 bool fac_has_job(faction_job job);

// Interaction with the player
 void form_opinion(player *u);
 talk_topic pick_talk_topic(player *u);
 int  player_danger(player *u) const; // Comparable to monsters
 int vehicle_danger(int radius) const;
 bool turned_hostile() const; // True if our anger is at least equal to...
 int hostile_anger_level() const; // ... this value!
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p) const;
 int assigned_missions_value();
 std::vector<const Skill*> skills_offered_to(player *p); // Skills that're higher
 std::vector<itype_id> styles_offered_to(player *p); // Martial Arts
// State checks
 bool is_enemy() const; // We want to kill/mug/etc the player
 bool is_following() const; // Traveling w/ player (whether as a friend or a slave)
 bool is_friend() const; // Allies with the player
 bool is_leader() const; // Leading the player
 bool is_defending() const; // Putting the player's safety ahead of ours
        Attitude attitude_to( const Creature &other ) const override;
// What happens when the player makes a request
 void told_to_help();
 void told_to_wait();
 void told_to_leave();
 int  follow_distance() const; // How closely do we follow the player?
 int  speed_estimate(int speed) const; // Estimate of a target's speed, usually player


// Dialogue and bartering--see npctalk.cpp
 void talk_to_u();
// Bartering - select items we're willing to buy/sell and set prices
// Prices are later modified by g->u's barter skill; see dialogue.cpp
// init_buying() fills <indices> with the indices of items in <you>
 void init_buying(inventory& you, std::vector<item*> &items,
                  std::vector<int> &prices);
// init_selling() fills <indices> with the indices of items in our inventory
 void init_selling(std::vector<item*> &items, std::vector<int> &prices);
// Re-roll the inventory of a shopkeeper
 void shop_restock();
// Use and assessment of items
 int  minimum_item_value(); // The minimum value to want to pick up an item
 void update_worst_item_value(); // Find the worst value in our inventory
 int  value(const item &it);
 bool wear_if_wanted(item it);
 virtual bool wield(item* it, bool);
 virtual bool wield(item* it);
 bool has_healing_item();
 bool has_painkiller();
 bool took_painkiller() const;
 void use_painkiller();
 void activate_item(int position);

// Interaction and assessment of the world around us
 int  danger_assessment();
 int  average_damage_dealt(); // Our guess at how much damage we can deal
 bool bravery_check(int diff);
 bool emergency(int danger);
 bool is_active() const;
 void say(std::string line, ...) const;
 void decide_needs();
 void die(Creature* killer);
 bool is_dead() const;
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
 int choose_escape_item(); // Returns item position of our best escape aid

// Helper functions for ranged combat
 int  confident_range(int position = -1); // >= 50% chance to hit
 bool wont_hit_friend(int tarx, int tary, int position = -1);
 bool can_reload(); // Wielding a gun that is not fully loaded
 bool need_to_reload(); // Wielding a gun that is empty
 bool enough_time_to_reload(int target, item &gun);

// Physical movement from one tile to the next
 void update_path (int x, int y);
 bool can_move_to (int x, int y) const;
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
 void use_escape_item (int position);
 void heal_player (player &patient);
 void heal_self  ();
 void take_painkiller ();
 void pick_and_eat ();
 void mug_player (player &mark);
 void look_for_player (player &sought);
 bool saw_player_recently() const;// Do we have an idea of where u are?

// Movement on the overmap scale
 bool has_destination() const; // Do we have a long-term destination?
 void set_destination(); // Pick a place to go
 void go_to_destination(); // Move there; on the micro scale
 void reach_destination(); // We made it!

 //message related stuff
 virtual void add_msg_if_npc(const char* msg, ...) const;
 virtual void add_msg_player_or_npc(const char* player_str, const char* npc_str, ...) const;
 virtual void add_msg_if_npc(game_message_type type, const char* msg, ...) const;
 virtual void add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...) const;
 virtual void add_msg_if_player(const char *, ...) const{};
 virtual void add_msg_if_player(game_message_type, const char *, ...) const{};
 virtual void add_memorial_log(const char*, const char*, ...) {};
 virtual void add_miss_reason(const char *, unsigned int) {};

// The preceding are in npcmove.cpp



// #############   VALUES   ################

 npc_attitude attitude; // What we want to do to the player
 npc_class myclass; // What's our archetype?
 int wandx, wandy, wandf; // Location of heard sound, etc.
 std::string idz; // A temp variable used to inform the game which npc json to use as a template
 int miss_id; // A temp variable used to link to the correct mission

private:
    /**
     * Global submap coordinates of the npc (minus the position on the map:
     * posx,posy). Use global_*_location to get the global position.
     * You should not change mapx,mapy directly, use posx,posy instead,
     * @ref shift will update mapx,mapy and move the npc to a different
     * overmap if needed.
     * (mapx,mapy) defines the overmap the npc is stored on.
     */
    int mapx, mapy, mapz;
public:

    static npc_map _all_npc;
    /**
     * Global position, expressed in map square coordinate system
     * (the most detailed coordinate system), used by the @ref map.
     *
     * The (global) position of an NPC is always:
     * point(
     *     mapx * SEEX + posx,
     *     mapy * SEEY + posy,
     *     mapz)
     * (Expressed in map squares, the system that @ref map uses.)
     * Any of om, map, pos can be in any range.
     * For active NPCs pos would be in the valid range required by
     * the map. But pos, map, and om can be changed without the NPC
     * actual moving as long as the position stays the same:
     * posx += SEEX; mapx -= 1;
     * This does not change the global position of the NPC.
     */
    tripoint global_square_location() const;
    /**
     * Returns the location of the NPC in global submap coordinates.
     */
    tripoint global_sm_location() const;
    /**
     * Returns the location of the NPC in global overmap terrain coordinates.
     */
    tripoint global_omt_location() const;
 int plx, ply, plt;// Where we last saw the player, timeout to forgetting
 int itx, ity; // The square containing an item we want
 int guardx, guardy;  // These are the local coordinates that a guard will return to inside of their goal tripoint
    /**
     * Global overmap terrain coordinate, where we want to get to
     * if no goal exist, this is no_goal_point.
     */
    tripoint goal;
 int restock;
 bool fetching_item;
 bool has_new_items; // If true, we have something new and should re-equip
 int  worst_item_value; // The value of our least-wanted item

 std::vector<point> path; // Our movement plans

// Personality & other defining characteristics
 std::string fac_id; // A temp variable used to inform the game which faction to link
 faction *my_fac;
 npc_mission mission;
 npc_personality personality;
 npc_opinion op_of_u;
 npc_chatbin chatbin;
 int patience; // Used when we expect the player to leave the area
 npc_combat_rules combat_rules;
 bool marked_for_death; // If true, we die as soon as we respawn!
 bool hit_by_player;
 std::vector<npc_need> needs;
 unsigned flags : NF_MAX;
 // Dummy point that indicates that the goal is invalid.
 static const tripoint no_goal_point;

    protected:
        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);

private:
    void setID (int id);
    bool dead;  // If true, we need to be cleaned up

    bool is_dangerous_field( const field_entry &fld ) const;
    bool sees_dangerous_field( point p ) const;
    bool could_move_onto( point p ) const;
};

#endif
