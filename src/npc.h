#ifndef NPC_H
#define NPC_H

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
enum game_message_type : int;

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
 NC_SOLDIER,        // Well equiped and trained combatant, good with rifles and melee
 NC_BARTENDER,      // Stocks alcohol
 NC_JUNK_SHOPKEEP,  // Stocks wide range of items...
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
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
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
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
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

 npc_opinion& operator+= ( const npc_opinion &rhs )
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
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
};

enum combat_engagement {
 ENGAGE_NONE = 0,
 ENGAGE_CLOSE,
 ENGAGE_WEAK,
 ENGAGE_HIT,
 ENGAGE_ALL
};

struct npc_follower_rules : public JsonSerializer, public JsonDeserializer
{
    combat_engagement engagement;
    bool use_guns;
    bool use_grenades;
    bool use_silent;

    bool allow_pick_up;
    bool allow_bash;
    bool allow_sleep;

    npc_follower_rules()
    {
        engagement = ENGAGE_ALL;
        use_guns = true;
        use_grenades = true;
        use_silent = false;

        allow_pick_up = true;
        allow_bash = true;
        allow_sleep = false;
    };

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
};

// DO NOT USE! This is old, use strings as talk topic instead, e.g. "TALK_AGREE_FOLLOW" instead of
// TALK_AGREE_FOLLOW. There is also convert_talk_topic which can convert the enumeration values to
// the new string values (used to load old saves).
enum talk_topic_enum {
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

 TALK_OLD_GUARD_SOLDIER,//98, Generic Old Guard

 TALK_OLD_GUARD_NEC_CPT,//99, Main mission source in Necropolis
 TALK_OLD_GUARD_NEC_CPT_GOAL,
 TALK_OLD_GUARD_NEC_CPT_VAULT,

 TALK_OLD_GUARD_NEC_COMMO,//102, Mission source/destination in Necropolis
 TALK_OLD_GUARD_NEC_COMMO_GOAL,
 TALK_OLD_GUARD_NEC_COMMO_FREQ,

 TALK_RANCH_FOREMAN,//105, Mission source/critical to building up the ranch camp
 TALK_RANCH_FOREMAN_PROSPECTUS,
 TALK_RANCH_FOREMAN_OUTPOST,
 TALK_RANCH_FOREMAN_REFUGEES,
 TALK_RANCH_FOREMAN_JOB,

 TALK_RANCH_CONSTRUCTION_1,//110

 TALK_RANCH_CONSTRUCTION_2,//111
 TALK_RANCH_CONSTRUCTION_2_JOB,
 TALK_RANCH_CONSTRUCTION_2_HIRE,

 TALK_RANCH_WOODCUTTER,//114
 TALK_RANCH_WOODCUTTER_JOB,
 TALK_RANCH_WOODCUTTER_HIRE,

 TALK_RANCH_WOODCUTTER_2,//117
 TALK_RANCH_WOODCUTTER_2_JOB,
 TALK_RANCH_WOODCUTTER_2_HIRE,

 TALK_RANCH_FARMER_1,//120
 TALK_RANCH_FARMER_1_JOB,
 TALK_RANCH_FARMER_1_HIRE,

 TALK_RANCH_FARMER_2,//123
 TALK_RANCH_FARMER_2_JOB,
 TALK_RANCH_FARMER_2_HIRE,

 TALK_RANCH_CROP_OVERSEER,//126
 TALK_RANCH_CROP_OVERSEER_JOB,

 TALK_RANCH_ILL_1,//128
 TALK_RANCH_ILL_1_JOB,
 TALK_RANCH_ILL_1_HIRE,
 TALK_RANCH_ILL_1_SICK,

 TALK_RANCH_NURSE,//132
 TALK_RANCH_NURSE_JOB,
 TALK_RANCH_NURSE_HIRE,
 TALK_RANCH_NURSE_AID,
 TALK_RANCH_NURSE_AID_DONE,

 TALK_RANCH_DOCTOR,//137

 TALK_RANCH_SCRAPPER,//138
 TALK_RANCH_SCRAPPER_JOB,
 TALK_RANCH_SCRAPPER_HIRE,

 TALK_RANCH_SCAVENGER_1,//141
 TALK_RANCH_SCAVENGER_1_JOB,
 TALK_RANCH_SCAVENGER_1_HIRE,

 TALK_RANCH_BARKEEP,//144
 TALK_RANCH_BARKEEP_JOB,
 TALK_RANCH_BARKEEP_INFORMATION,
 TALK_RANCH_BARKEEP_TAP,

 TALK_RANCH_BARBER,//148
 TALK_RANCH_BARBER_JOB,
 TALK_RANCH_BARBER_HIRE,
 TALK_RANCH_BARBER_CUT,

 TALK_RANCH_STOCKS_BANDAGES,

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
    /**
     * Add a new mission to the available missions (@ref missions). For compatibility it silently
     * ignores null pointers passed to it.
     */
    void add_new_mission( mission *miss );
    /**
     * Check that assigned missions are still assigned if not move them back to the
     * unassigned vector. This is called directly before talking.
     */
    void check_missions();
    /**
     * Missions that the NPC can give out. All missions in this vector should be unassigned,
     * when given out, they should be moved to @ref missions_assigned.
     */
    std::vector<mission*> missions;
    /**
     * Mission that have been assigned by this NPC to a player character.
     */
    std::vector<mission*> missions_assigned;
    /**
     * The mission (if any) that we talk about right now. Can be null. Should be one of the
     * missions in @ref missions or @ref missions_assigned.
     */
    mission *mission_selected = nullptr;
    /**
     * The skill this NPC offers to train.
     */
    const Skill* skill = nullptr;
    /**
     * The martial art style this NPC offers to train.
     */
    matype_id style;
    std::string first_topic = "TALK_NONE";

    npc_chatbin() = default;

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
};

class npc;
struct epilogue;

typedef std::map<std::string, npc> npc_map;
typedef std::map<std::string, epilogue> epilogue_map;

class npc : public player
{
public:

 npc();
 npc(const npc &) = default;
 npc(npc &&) = default;
 npc &operator=(const npc &) = default;
 npc &operator=(npc &&) = default;
 virtual ~npc();
 virtual bool is_player() const override { return false; }
 virtual bool is_npc() const override { return true; }

 static void load_npc(JsonObject &jsobj);
 npc* find_npc(std::string ident);
 void load_npc_template(std::string ident);

// Generating our stats, etc.
 void randomize(npc_class type = NC_NONE);
 void randomize_from_faction(faction *fac);
 void set_fac(std::string fac_name);
    /**
     * Set @ref mapx and @ref mapx and @ref pos.
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
    /**
     * See @ref npc_chatbin::add_new_mission
     */
    void add_new_mission( mission *miss );

 const Skill* best_skill() const;
 void starting_weapon(npc_class type);

// Save & load
    virtual void load_info(std::string data) override;// Overloaded from player
    virtual std::string save_info() const override;

    using player::deserialize;
    virtual void deserialize(JsonIn &jsin) override;
    using player::serialize;
    virtual void serialize(JsonOut &jsout) const override;

// Display
    virtual nc_color basic_symbol_color() const override;
    virtual nc_color symbol_color() const override;
 int print_info(WINDOW* w, int vStart, int vLines, int column) const override;
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
    std::string pick_talk_topic(player *u);
 int  player_danger(player *u) const; // Comparable to monsters
 int vehicle_danger(int radius) const;
 bool turned_hostile() const; // True if our anger is at least equal to...
 int hostile_anger_level() const; // ... this value!
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p) const;
 int assigned_missions_value();
 std::vector<const Skill*> skills_offered_to(const player &p); // Skills that're higher
    /**
     * Martial art styles that we known, but the player p doesn't.
     */
    std::vector<matype_id> styles_offered_to( const player &p ) const;
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
 int  speed_estimate( const Creature& ) const; // Estimate of a target's speed, usually player


// Dialogue and bartering--see npctalk.cpp
 void talk_to_u();
// Bartering - select items we're willing to buy/sell and set prices
// Prices are later modified by g->u's barter skill; see dialogue.cpp
    struct item_pricing {
        item *itm;
        int price;
        // Whether this is selected for trading, init_buying and init_selling initialize
        // this to `false`.
        bool selected;
    };
// returns prices for items in `you`
    std::vector<item_pricing> init_buying( inventory& you );
// returns prices and items in the inventory of this NPC
    std::vector<item_pricing> init_selling();
// Re-roll the inventory of a shopkeeper
 void shop_restock();
// Use and assessment of items
 int  minimum_item_value(); // The minimum value to want to pick up an item
 void update_worst_item_value(); // Find the worst value in our inventory
 int  value(const item &it);
    bool wear_if_wanted( const item &it );
    virtual bool wield( item& it ) override;
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
    void say( const std::string line, ...) const;
    void decide_needs();
    void die(Creature* killer) override;
    bool is_dead() const;
    int smash_ability() const; // How well we smash terrain (not corpses!)
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
 /**
  * Check if this NPC is blocking movement from the given position
  */
 bool is_blocking_position( const tripoint &p );
 bool wont_hit_friend(  const tripoint &p , int position = -1 );
 bool need_to_reload(); // Wielding a gun that is empty
 bool enough_time_to_reload(int target, item &gun);

// Physical movement from one tile to the next
 void update_path( const tripoint &p, bool no_bashing = false );
 bool can_move_to( const tripoint &p, bool no_bashing = false ) const;
 void move_to    ( const tripoint &p, bool no_bashing = false );
 void move_to_next(); // Next in <path>
 void avoid_friendly_fire(int target); // Maneuver so we won't shoot u
 void move_away_from( const tripoint &p, bool no_bashing = false );
 void move_pause(); // Same as if the player pressed '.'

// Item discovery and fetching
 void find_item  (); // Look around and pick an item
 void pick_up_item (); // Move to, or grab, our targeted item
 void drop_items (int weight, int volume); // Drop wgt and vol
 npc_action scan_new_items(int target);

// Combat functions and player interaction functions
    Creature *get_target( int target ) const;
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
 virtual void add_msg_if_npc(const char* msg, ...) const override;
 virtual void add_msg_player_or_npc(const char* player_str, const char* npc_str, ...) const override;
 virtual void add_msg_if_npc(game_message_type type, const char* msg, ...) const override;
 virtual void add_msg_player_or_npc(game_message_type type, const char* player_str, const char* npc_str, ...) const override;
 virtual void add_msg_if_player(const char *, ...) const override{};
 virtual void add_msg_if_player(game_message_type, const char *, ...) const override{};
 virtual void add_memorial_log(const char*, const char*, ...) override {};
 virtual void add_miss_reason(const char *, unsigned int) {};
    virtual void add_msg_player_or_say( const char *, const char *, ... ) const override;
    virtual void add_msg_player_or_say( game_message_type, const char *, const char *, ... ) const override;

// The preceding are in npcmove.cpp

    bool query_yn( const char *mes, ... ) const override;



// #############   VALUES   ################

 npc_attitude attitude; // What we want to do to the player
 npc_class myclass; // What's our archetype?
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
    int mapx, mapy;
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
     *     pos.z)
     * (Expressed in map squares, the system that @ref map uses.)
     * Any of om, map, pos can be in any range.
     * For active NPCs pos would be in the valid range required by
     * the map. But pos, map, and om can be changed without the NPC
     * actual moving as long as the position stays the same:
     * posx += SEEX; mapx -= 1;
     * This does not change the global position of the NPC.
     */
    tripoint global_square_location() const override;
    tripoint last_player_seen_pos; // Where we last saw the player
    int last_seen_player_turn; // Timeout to forgetting
    tripoint wanted_item_pos; // The square containing an item we want
    tripoint guard_pos;  // These are the local coordinates that a guard will return to inside of their goal tripoint
    /**
     * Global overmap terrain coordinate, where we want to get to
     * if no goal exist, this is no_goal_point.
     */
    tripoint goal;

    tripoint wander_pos; // Not actually used (should be: wander there when you hear a sound)
    int wander_time;

 int restock;
 bool fetching_item;
 bool has_new_items; // If true, we have something new and should re-equip
 int  worst_item_value; // The value of our least-wanted item

 std::vector<tripoint> path; // Our movement plans

// Personality & other defining characteristics
 std::string fac_id; // A temp variable used to inform the game which faction to link
 faction *my_fac;
 std::string companion_mission;
 int companion_mission_time;
 npc_mission mission;
 npc_personality personality;
 npc_opinion op_of_u;
 npc_chatbin chatbin;
 int patience; // Used when we expect the player to leave the area
    npc_follower_rules rules;
 bool marked_for_death; // If true, we die as soon as we respawn!
 bool hit_by_player;
 std::vector<npc_need> needs;
 unsigned flags : NF_MAX;
 // Dummy point that indicates that the goal is invalid.
 static const tripoint no_goal_point;

    int last_updated;
    /**
     * Do some cleanup and caching as npc is being unloaded from map.
     */
    void on_unload();
    /**
     * Retroactively update npc.
     */
    void on_load();

    protected:
        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);

private:
    void setID (int id);
    bool dead;  // If true, we need to be cleaned up

    bool is_dangerous_field( const field_entry &fld ) const;
    bool sees_dangerous_field( const tripoint &p ) const;
    bool could_move_onto( const tripoint &p ) const;
};

struct epilogue {
    epilogue();

    std::string id; //Unique name for declaring an ending for a given individual
    std::string group; //Male/female (dog/cyborg/mutant... whatever you want)
    bool is_unique; //If true, will not occur in random endings
    //The lines you with to draw
    std::vector<std::string> lines;

    static epilogue_map _all_epilogue;

    static void load_epilogue(JsonObject &jsobj);
    epilogue* find_epilogue(std::string ident);
    void random_by_group(std::string group, std::string name);
};
#endif
