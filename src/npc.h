#pragma once
#ifndef CATA_SRC_NPC_H
#define CATA_SRC_NPC_H

#include <algorithm>
#include <array>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "auto_pickup.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "dialogue_chatbin.h"
#include "enums.h"
#include "faction.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "line.h"
#include "lru_cache.h"
#include "memory_fast.h"
#include "mission_companion.h"
#include "npc_attack.h"
#include "pimpl.h"
#include "point.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units_fwd.h"

class JsonObject;
class JsonOut;
class JsonValue;
class mission;
class monfaction;
class monster;
class npc_class;
class talker;
class vehicle;

namespace catacurses
{
class window;
}  // namespace catacurses
struct bionic_data;
struct mission_type;
struct overmap_location;
struct pathfinding_settings;

enum game_message_type : int;
class gun_mode;

using bionic_id = string_id<bionic_data>;
using npc_class_id = string_id<npc_class>;
using mission_type_id = string_id<mission_type>;
using mfaction_id = int_id<monfaction>;
using overmap_location_str_id = string_id<overmap_location>;
using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<drop_location>;

void parse_tags( std::string &phrase, const Character &u, const Character &me,
                 const itype_id &item_type = itype_id::NULL_ID() );

/*
 * Talk:   Trust midlow->high, fear low->mid, need doesn't matter
 * Trade:  Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill:   Trust low->midlow, fear low->midlow, need low
 * Flee:   Trust low, fear mid->high, need low
 */

// Attitude is how we feel about the player, what we do around them
enum npc_attitude : int {
    NPCATT_NULL = 0, // Don't care/ignoring player The places this is assigned is on shelter NPC generation, and when you order a NPC to stay in a location, and after talking to a NPC that wanted to talk to you.
    NPCATT_TALK,  // Move to and talk to player
    NPCATT_LEGACY_1,
    NPCATT_FOLLOW,  // Follow the player
    NPCATT_LEGACY_2,
    NPCATT_LEAD,  // Lead the player, wait for them if they're behind
    NPCATT_WAIT,  // Waiting for the player
    NPCATT_LEGACY_6,
    NPCATT_MUG,  // Mug the player
    NPCATT_WAIT_FOR_LEAVE, // Attack the player if our patience runs out
    NPCATT_KILL,  // Kill the player
    NPCATT_FLEE,  // Get away from the player; deprecated
    NPCATT_LEGACY_3,
    NPCATT_HEAL,  // Get to the player and heal them

    NPCATT_LEGACY_4,
    NPCATT_LEGACY_5,
    NPCATT_ACTIVITY, // Perform a mission activity
    NPCATT_FLEE_TEMP, // Get away from the player for a while
    NPCATT_RECOVER_GOODS, // Chase the player to demand stolen goods back
    NPCATT_END
};

std::string npc_attitude_id( npc_attitude );
std::string npc_attitude_name( npc_attitude );

// Attitudes are grouped by overall behavior towards player
enum class attitude_group : int {
    neutral = 0, // Doesn't particularly mind the player
    hostile, // Not necessarily attacking, but also mugging, exploiting etc.
    fearful, // Run
    friendly // Follow, defend, listen
};

// jobs assigned to an NPC when they are stationed at a basecamp.
class job_data
{
    private:
        std::map<activity_id, int> task_priorities = {
            { activity_id( "ACT_MULTIPLE_BUTCHER" ), 0 },
            { activity_id( "ACT_MULTIPLE_CONSTRUCTION" ), 0 },
            { activity_id( "ACT_VEHICLE_REPAIR" ), 0 },
            { activity_id( "ACT_VEHICLE_DECONSTRUCTION" ), 0 },
            { activity_id( "ACT_MULTIPLE_FARM" ), 0 },
            { activity_id( "ACT_MULTIPLE_CHOP_TREES" ), 0 },
            { activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ), 0 },
            { activity_id( "ACT_MULTIPLE_FISH" ), 0 },
            { activity_id( "ACT_MOVE_LOOT" ), 0 },
            { activity_id( "ACT_TIDY_UP" ), 0 },
            { activity_id( "ACT_MULTIPLE_DIS" ), 0}
        };
    public:
        bool set_task_priority( const activity_id &task, int new_priority );
        void clear_all_priorities();
        bool has_job() const;
        int get_priority_of_job( const activity_id &req_job ) const;
        std::vector<activity_id> get_prioritised_vector() const;
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonValue &jv );
};

enum npc_mission : int {
    NPC_MISSION_NULL = 0, // Nothing in particular
    NPC_MISSION_LEGACY_1,
    NPC_MISSION_SHELTER, // Stay in shelter, introduce player to game
    NPC_MISSION_SHOPKEEP, // Stay still unless combat or something and sell stuff

    NPC_MISSION_LEGACY_2,
    NPC_MISSION_LEGACY_3,

    NPC_MISSION_GUARD_ALLY, // Assigns an allied NPC to guard a position
    NPC_MISSION_GUARD, // Assigns an non-allied NPC to remain in place
    NPC_MISSION_GUARD_PATROL, // Assigns a non-allied NPC to guard and investigate
    NPC_MISSION_ACTIVITY, // Perform a player_activity until it is complete
    NPC_MISSION_TRAVELLING
};

struct npc_companion_mission {
    mission_id miss_id;
    tripoint_abs_omt position;
    std::string role_id;
    std::optional<tripoint_abs_omt> destination;
};

std::string npc_class_name( const npc_class_id & );
std::string npc_class_name_str( const npc_class_id & );

enum npc_action : int;

enum npc_need {
    need_none,
    need_ammo, need_weapon, need_gun,
    need_food, need_drink, need_safety,
    num_needs
};

// TODO: Turn the personality struct into a vector/map?
enum npc_personality_type : int {
    NPE_AGGRESSION,
    NPE_BRAVERY,
    NPE_COLLECTOR,
    NPE_ALTRUISM,
    NUM_NPE
};

struct npc_personality {
    // All values should be in the -10 to 10 range.
    int8_t aggression;
    int8_t bravery;
    int8_t collector;
    int8_t altruism;
    npc_personality() {
        aggression = 0;
        bravery    = 0;
        collector  = 0;
        altruism   = 0;
    }

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

struct npc_opinion {
    int trust;
    int fear;
    int value;
    int anger;
    int owed; // Positive when the npc owes the player. Negative if player owes them.
    int sold; // Total value of goods sold/donated by player to the npc. Cannot be negative.

    npc_opinion() {
        trust = 0;
        fear  = 0;
        value = 0;
        anger = 0;
        owed  = 0;
        sold = 0;
    }

    npc_opinion &operator+=( const npc_opinion &rhs ) {
        trust += rhs.trust;
        fear  += rhs.fear;
        value += rhs.value;
        anger += rhs.anger;
        owed  += rhs.owed;
        sold  += rhs.sold;
        return *this;
    }

    npc_opinion operator+( const npc_opinion &rhs ) {
        return npc_opinion( *this ) += rhs;
    }

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

enum class combat_engagement : int {
    NONE = 0,
    CLOSE,
    WEAK,
    HIT,
    ALL,
    FREE_FIRE,
    NO_MOVE
};
const std::unordered_map<std::string, combat_engagement> combat_engagement_strs = { {
        { "ENGAGE_NONE", combat_engagement::NONE },
        { "ENGAGE_CLOSE", combat_engagement::CLOSE },
        { "ENGAGE_WEAK", combat_engagement::WEAK },
        { "ENGAGE_HIT", combat_engagement::HIT },
        { "ENGAGE_ALL", combat_engagement::ALL },
        { "ENGAGE_FREE_FIRE", combat_engagement::FREE_FIRE },
        { "ENGAGE_NO_MOVE", combat_engagement::NO_MOVE }
    }
};

enum class combat_skills : int {
    ALL = 0,
    NO_GENERAL,
    WEAPONS_ONLY
};

enum class aim_rule : int {
    // Aim some
    WHEN_CONVENIENT = 0,
    // No concern for ammo efficiency
    SPRAY,
    // Aim when possible, then shoot
    PRECISE,
    // If you can't aim, don't shoot
    STRICTLY_PRECISE
};
const std::unordered_map<std::string, aim_rule> aim_rule_strs = { {
        { "AIM_WHEN_CONVENIENT", aim_rule::WHEN_CONVENIENT },
        { "AIM_SPRAY", aim_rule::SPRAY },
        { "AIM_PRECISE", aim_rule::PRECISE },
        { "AIM_STRICTLY_PRECISE", aim_rule::STRICTLY_PRECISE }
    }
};

// How much CBM power should remain before attempting to recharge, values are percents of power
enum class cbm_recharge_rule : int {
    CBM_RECHARGE_ALL = 90,
    CBM_RECHARGE_MOST = 75,
    CBM_RECHARGE_SOME = 50,
    CBM_RECHARGE_LITTLE = 25,
    CBM_RECHARGE_NONE = 10
};
const std::unordered_map<std::string, cbm_recharge_rule> cbm_recharge_strs = { {
        { "CBM_RECHARGE_ALL", cbm_recharge_rule::CBM_RECHARGE_ALL },
        { "CBM_RECHARGE_MOST", cbm_recharge_rule::CBM_RECHARGE_MOST },
        { "CBM_RECHARGE_SOME", cbm_recharge_rule::CBM_RECHARGE_SOME },
        { "CBM_RECHARGE_LITTLE", cbm_recharge_rule::CBM_RECHARGE_LITTLE },
        { "CBM_RECHARGE_NONE", cbm_recharge_rule::CBM_RECHARGE_NONE }
    }
};

// How much CBM power to reserve for defense, values are percents of total power
enum class cbm_reserve_rule : int {
    CBM_RESERVE_ALL = 100,
    CBM_RESERVE_MOST = 75,
    CBM_RESERVE_SOME = 50,
    CBM_RESERVE_LITTLE = 25,
    CBM_RESERVE_NONE = 0
};
const std::unordered_map<std::string, cbm_reserve_rule> cbm_reserve_strs = { {
        { "CBM_RESERVE_ALL", cbm_reserve_rule::CBM_RESERVE_ALL },
        { "CBM_RESERVE_MOST", cbm_reserve_rule::CBM_RESERVE_MOST },
        { "CBM_RESERVE_SOME", cbm_reserve_rule::CBM_RESERVE_SOME },
        { "CBM_RESERVE_LITTLE", cbm_reserve_rule::CBM_RESERVE_LITTLE },
        { "CBM_RESERVE_NONE", cbm_reserve_rule::CBM_RESERVE_NONE }
    }
};

enum class ally_rule : int {
    DEFAULT = 0,
    use_guns = 1,
    use_grenades = 2,
    use_silent = 4,
    avoid_friendly_fire = 8,
    allow_pick_up = 16,
    allow_bash = 32,
    allow_sleep = 64,
    allow_complain = 128,
    allow_pulp = 256,
    close_doors = 512,
    follow_close = 1024,
    avoid_doors = 2048,
    hold_the_line = 4096,
    ignore_noise = 8192,
    forbid_engage = 16384,
    follow_distance_2 = 32768
};

struct ally_rule_data {
    ally_rule rule;
    std::string rule_true_text;
    std::string rule_false_text;
};

const std::unordered_map<std::string, ally_rule_data> ally_rule_strs = { {
        {
            "use_guns", {
                ally_rule::use_guns,
                "<ally_rule_use_guns_true_text>",
                "<ally_rule_use_guns_false_text>"
            }
        },
        {
            "use_grenades", {
                ally_rule::use_grenades,
                "<ally_rule_use_grenades_true_text>",
                "<ally_rule_use_grenades_false_text>"
            }
        },
        {
            "use_silent", {
                ally_rule::use_silent,
                "<ally_rule_use_silent_true_text>",
                "<ally_rule_use_silent_false_text>"
            }
        },
        {
            "avoid_friendly_fire", {
                ally_rule::avoid_friendly_fire,
                "<ally_rule_avoid_friendly_fire_true_text>",
                "<ally_rule_avoid_friendly_fire_false_text>"
            }
        },
        {
            "allow_pick_up", {
                ally_rule::allow_pick_up,
                "<ally_rule_allow_pick_up_true_text>",
                "<ally_rule_allow_pick_up_false_text>"
            }
        },
        {
            "allow_bash", {
                ally_rule::allow_bash,
                "<ally_rule_allow_bash_true_text>",
                "<ally_rule_allow_bash_false_text>"
            }
        },
        {
            "allow_sleep", {
                ally_rule::allow_sleep,
                "<ally_rule_allow_sleep_true_text>",
                "<ally_rule_allow_sleep_false_text>"
            }
        },
        {
            "allow_complain", {
                ally_rule::allow_complain,
                "<ally_rule_allow_complain_true_text>",
                "<ally_rule_allow_complain_false_text>"
            }
        },
        {
            "allow_pulp", {
                ally_rule::allow_pulp,
                "<ally_rule_allow_pulp_true_text>",
                "<ally_rule_allow_pulp_false_text>"
            }
        },
        {
            "close_doors", {
                ally_rule::close_doors,
                "<ally_rule_close_doors_true_text>",
                "<ally_rule_close_doors_false_text>"
            }
        },
        {
            "follow_close", {
                ally_rule::follow_close,
                "<ally_rule_follow_close_true_text>",
                "<ally_rule_follow_close_false_text>"
            }
        },
        {
            "avoid_doors", {
                ally_rule::avoid_doors,
                "<ally_rule_avoid_doors_true_text>",
                "<ally_rule_avoid_doors_false_text>"
            }
        },
        {
            "hold_the_line", {
                ally_rule::hold_the_line,
                "<ally_rule_hold_the_line_true_text>",
                "<ally_rule_hold_the_line_false_text>"
            }
        },
        {
            "ignore_noise", {
                ally_rule::ignore_noise,
                "<ally_rule_ignore_noise_true_text>",
                "<ally_rule_ignore_noise_false_text>"
            }
        },
        {
            "forbid_engage", {
                ally_rule::forbid_engage,
                "<ally_rule_forbid_engage_true_text>",
                "<ally_rule_forbid_engage_false_text>"
            }
        },
        {
            "follow_distance_2", {
                ally_rule::follow_distance_2,
                "<ally_rule_follow_distance_2_true_text>",
                "<ally_rule_follow_distance_2_false_text>"
            }
        }
    }
};

struct npc_follower_rules {
    combat_engagement engagement;
    aim_rule aim = aim_rule::WHEN_CONVENIENT;
    cbm_recharge_rule cbm_recharge = cbm_recharge_rule::CBM_RECHARGE_SOME;
    cbm_reserve_rule cbm_reserve = cbm_reserve_rule::CBM_RESERVE_SOME;
    ally_rule flags = ally_rule::DEFAULT; // NOLINT(cata-serialize)
    ally_rule override_enable = ally_rule::DEFAULT; // NOLINT(cata-serialize)
    ally_rule overrides = ally_rule::DEFAULT; // NOLINT(cata-serialize)

    pimpl<auto_pickup::npc_settings> pickup_whitelist;

    npc_follower_rules();

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );

    bool has_flag( ally_rule test, bool check_override = true ) const;
    void set_flag( ally_rule setit );
    void clear_flag( ally_rule clearit );
    void toggle_flag( ally_rule toggle );
    void set_specific_override_state( ally_rule, bool state );
    void toggle_specific_override_state( ally_rule rule, bool state );
    bool has_override_enable( ally_rule test ) const;
    void enable_override( ally_rule setit );
    void disable_override( ally_rule clearit );
    bool has_override( ally_rule test ) const;
    void set_override( ally_rule setit );
    void clear_override( ally_rule clearit );

    void set_danger_overrides();
    void clear_overrides();
};

struct dangerous_sound {
    tripoint abs_pos;
    sounds::sound_t type = sounds::sound_t::LAST;
    int volume = 0;
};

constexpr std::array<direction, 8> npc_threat_dir = {
    direction::NORTHWEST, direction::NORTH, direction::NORTHEAST, direction::EAST,
    direction::SOUTHEAST, direction::SOUTH, direction::SOUTHWEST, direction::WEST
};

struct healing_options {
    bool bandage = false;
    bool disinfect = false;
    bool bleed = false;
    bool bite = false;
    bool infect = false;
    void clear_all();
    void set_all();
    bool any_true() const;
    bool all_false() const;
};

// Data relevant only for this action
struct npc_short_term_cache {
    float danger = 0.0f;
    float total_danger = 0.0f;
    float danger_assessment = 0.0f;
    // Use weak_ptr to avoid circular references between Creatures
    weak_ptr_fast<Creature> target;
    // target is hostile, ally is for aiding actions
    weak_ptr_fast<Creature> ally;
    healing_options can_heal;
    // map of positions / type / volume of suspicious sounds
    std::vector<dangerous_sound> sound_alerts;
    // current sound position being investigated
    tripoint s_abs_pos;
    // number of times we haven't moved when investigating a sound
    int stuck = 0;
    // Position to return to guarding
    std::optional<tripoint> guard_pos;
    double my_weapon_value = 0;

    npc_attack_rating current_attack_evaluation;
    std::shared_ptr<npc_attack> current_attack;

    // Use weak_ptr to avoid circular references between Creatures
    // attitude of creatures the npc can see
    std::vector<weak_ptr_fast<Creature>> hostile_guys;
    std::vector<weak_ptr_fast<Creature>> neutral_guys;
    std::vector<weak_ptr_fast<Creature>> friends;
    std::vector<sphere> dangerous_explosives;
    std::map<direction, float> threat_map;
    // Cache of locations the NPC has searched recently in npc::find_item()
    lru_cache<tripoint, int> searched_tiles;
    // returns the value of the distance between a friendly creature and the closest enemy to that
    // friendly creature.
    // returns nullopt if not applicable
    std::optional<int> closest_enemy_to_friendly_distance() const;
};

struct npc_need_goal_cache {
    tripoint_abs_omt goal;
    tripoint_abs_omt omt_loc;
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

// Function for conversion of legacy topics, defined in savegame_legacy.cpp
std::string convert_talk_topic( talk_topic_enum old_value );

class npc_template;

class npc : public Character
{
    public:

        npc();
        npc( const npc & ) = delete;
        npc( npc && ) noexcept( map_is_noexcept );
        npc &operator=( const npc & ) = delete;
        npc &operator=( npc && ) noexcept( list_is_noexcept );
        ~npc() override;

        bool is_avatar() const override {
            return false;
        }
        bool is_npc() const override {
            return true;
        }
        npc *as_npc() override {
            return this;
        }
        const npc *as_npc() const override {
            return this;
        }
        void load_npc_template( const string_id<npc_template> &ident );
        void npc_dismount();
        weak_ptr_fast<monster> chosen_mount;
        // Generating our stats, etc.
        void randomize( const npc_class_id &type = npc_class_id::NULL_ID() );
        void randomize_from_faction( faction *fac );
        void apply_ownership_to_inv();
        void learn_ma_styles_from_traits();
        // Faction version number
        int get_faction_ver() const;
        void set_faction_ver( int new_version );
        bool has_faction_relationship( const Character &you,
                                       npc_factions::relationship flag ) const;
        void set_fac( const faction_id &id );
        faction *get_faction() const override;
        faction_id get_fac_id() const;
        /**
         * Spawns the NPC on a random square within the given OMT.
         * @param p global omt coordinates.
         */
        void spawn_at_omt( const tripoint_abs_omt &p );
        /**
         * Spawns the NPC on the specified map square.
         */
        void spawn_at_precise( const tripoint_abs_ms &p );
        /**
         * Places the NPC on the @ref map. This update its
         * pos values to fit the current offset of
         * map (g->levx, g->levy).
         * If the square on the map where the NPC would go is not empty
         * a spiral search for an empty square around it is performed.
         */
        void place_on_map();
        /**
         * See @ref dialogue_chatbin::add_new_mission
         */
        void add_new_mission( mission *miss );
        void update_missions_target( character_id old_character, character_id new_character );
        std::pair<skill_id, int> best_combat_skill( combat_skills subset ) const;
        void starting_weapon( const npc_class_id &type );

        // Save & load
        void deserialize( const JsonObject &data ) override;
        void serialize( JsonOut &json ) const override;

        // Display
        nc_color basic_symbol_color() const override;
        int print_info( const catacurses::window &w, int line, int vLines, int column ) const override;
        std::string opinion_text() const;
        int faction_display( const catacurses::window &fac_w, int width ) const;
        std::string describe_mission() const;
        std::string name_and_activity() const;
        std::string name_and_maybe_activity() const override;
        /// Returns current status (Sleeping, Guarding, In Combat, etc.), or current activity
        std::string get_current_status() const;
        /// Returns the current activity name (reading, disassembling, etc.), or "nothing"
        std::string get_current_activity() const;

        // Interaction with the player
        void form_opinion( const Character &you );
        npc_opinion get_opinion_values( const Character &you ) const;
        std::string pick_talk_topic( const Character &u );
        std::string const &get_specified_talk_topic( std::string const &topic_id );
        float character_danger( const Character &u ) const;
        float vehicle_danger( int radius ) const;
        void pretend_fire( npc *source, int shots, item &gun ); // fake ranged attack for hallucination
        // True if our anger is at least equal to...
        bool turned_hostile() const;
        // ... this value!
        int hostile_anger_level() const;
        // Called if the player attacks us
        void make_angry();
        /*
        * Angers and makes the NPC consider the creature an attacker
        * if the creature is a player and the NPC is not already hostile
        * towards the player.
        */
        void on_attacked( const Creature &attacker );
        int assigned_missions_value();
        /**
         * @return Skills of which this NPC has a higher level than the given player. In other
         * words: skills this NPC could teach the player.
         */
        std::vector<skill_id> skills_offered_to( const Character &you ) const;
        /**
         * Proficiencies we know that the character doesn't
         */
        std::vector<proficiency_id> proficiencies_offered_to( const Character &guy ) const;
        /**
         * Martial art styles that we known, but the player p doesn't.
         */
        std::vector<matype_id> styles_offered_to( const Character &you ) const;
        /**
         * Spells that the NPC knows but that the player p doesn't.
         * not const because get_spell isn't const and both this and p call it
         */
        std::vector<spell_id> spells_offered_to( Character &you );
        // State checks
        // We want to kill/mug/etc the player
        bool is_enemy() const;
        // Traveling w/ player (whether as a friend or a slave)
        bool is_following() const;
        bool is_obeying( const Character &p ) const;

        // true if the NPC isn't actually real
        bool is_hallucination() const override {
            return hallucination;
        }

        // true if the NPC produces electrical radiation
        // TODO: make this way less hard coded
        bool is_electrical() const override {
            // only beep on Rubik for now
            return has_trait( trait_id( "EXODII_BODY_1" ) );
        }

        // Ally of or traveling with p
        bool is_friendly( const Character &p ) const;
        // Leading the player
        bool is_leader() const;
        // Leading, following, or waiting for the player
        bool is_walking_with() const;
        // In the same faction
        bool is_ally( const Character &p ) const;
        // Is an ally of the player
        bool is_player_ally() const;
        // Isn't moving
        bool is_stationary( bool include_guards = true ) const;
        // Has a guard mission
        bool is_guarding() const;
        // Has a guard patrol mission
        bool is_patrolling() const;
        bool within_boundaries_of_camp() const;
        /** is performing a player_activity */
        bool has_player_activity() const;
        bool is_travelling() const;
        /** Trusts you a lot. */
        bool is_minion() const;
        /** Is enemy or will turn into one (can't be convinced not to attack). */
        bool guaranteed_hostile() const;
        Attitude attitude_to( const Creature &other ) const override;
        /* player allies that become guaranteed hostile should mutiny first */
        void mutiny();

        /** For mutant NPCs. Returns how monsters perceive said NPC. Doesn't imply NPC sees them the same. */
        mfaction_id get_monster_faction() const override;
        // What happens when the player makes a request
        // How closely do we follow the player?
        int follow_distance() const;

        // Re-roll the inventory of a shopkeeper
        void shop_restock();
        bool is_shopkeeper() const;
        // Use and assessment of items
        // The minimum value to want to pick up an item
        int minimum_item_value() const;
        // Find the worst value in our inventory
        void update_worst_item_value();
        double value( const item &it ) const;
        double value( const item &it, double market_price ) const;
        faction_price_rule const *get_price_rules( item const &it ) const;
        bool wear_if_wanted( const item &it, std::string &reason );
        bool can_read( const item &book, std::vector<std::string> &fail_reasons );
        time_duration time_to_read( const item &book, const Character &reader ) const;
        void do_npc_read();
        void stow_item( item &it );
        bool wield( item &it ) override;
        void drop( const drop_locations &what, const tripoint &target,
                   bool stash ) override;
        bool adjust_worn();
        bool has_healing_item( healing_options try_to_fix );
        healing_options patient_assessment( const Character &c );
        healing_options has_healing_options();
        healing_options has_healing_options( healing_options try_to_fix );
        item &get_healing_item( healing_options try_to_fix, bool first_best = false );
        bool has_painkiller();
        bool took_painkiller() const;
        void use_painkiller();
        void activate_item( item &it );
        bool has_identified( const itype_id & ) const override {
            return true;
        }
        void identify( const item & ) override {
            // Do nothing
        }
        /**
         * Is the item safe or does the NPC trust you enough?
         * Is not recursive, only checks the item that is the parameter.
         * to check contents, call this on the items inside the item.
         */
        bool will_accept_from_player( const item &it ) const;

        bool wants_to_sell( const item_location &it ) const;
        ret_val<void> wants_to_sell( const item_location &it, int at_price ) const;
        bool wants_to_buy( const item &it ) const;
        ret_val<void> wants_to_buy( const item &/*it*/, int at_price ) const;

        bool will_exchange_items_freely() const;
        int max_credit_extended() const;
        int max_willing_to_owe() const;

        // AI helpers
        void regen_ai_cache();
        const Creature *current_target() const;
        Creature *current_target();
        const Creature *current_ally() const;
        Creature *current_ally();
        tripoint good_escape_direction( bool include_pos = true );

        // Interaction and assessment of the world around us
        float danger_assessment() const;
        // Our guess at how much damage we can deal
        float average_damage_dealt();
        bool bravery_check( int diff ) const;
        bool emergency() const;
        bool emergency( float danger ) const;
        bool is_active() const;
        template<typename ...Args>
        void say( const char *const line, Args &&... args ) const {
            return say( string_format( line, std::forward<Args>( args )... ) );
        }
        void say( const std::string &line, sounds::sound_t spriority = sounds::sound_t::speech ) const;
        void decide_needs();
        void reboot();
        void die( Creature *killer ) override;
        bool is_dead() const;
        // How well we smash terrain (not corpses!)
        int smash_ability() const;

        /*
         *  CBM management functions
         */
        void activate_combat_cbms();
        void deactivate_combat_cbms();
        // returns true if fuel resources are consumed
        bool recharge_cbm();
        // power is below the requested levels
        bool wants_to_recharge_cbm();
        // has power available to use offensive CBMs
        bool can_use_offensive_cbm() const;
        // return false if not present or can't be activated; true if present and already active
        // or if the call activates it
        bool use_bionic_by_id( const bionic_id &cbm_id, bool eff_only = false );
        // return false if not present, can't be activated, or is already active; returns true if
        // present and the call activates it
        bool activate_bionic_by_id( const bionic_id &cbm_id, bool eff_only = false );
        bool deactivate_bionic_by_id( const bionic_id &cbm_id, bool eff_only = false );
        // in bionics.cpp
        // can't use bionics::activate because it calls plfire directly
        void discharge_cbm_weapon();
        // check if an NPC has a bionic weapon and activate it if possible
        void check_or_use_weapon_cbm( const bionic_id &cbm_id );

        /*
         * Item management functions. These are meant for during and after combat/danger.
         */
        void activate_combat_items();
        void deactivate_combat_items();

        /*
         * Prepare for combat, such as enabling combat items or CBMs.
         */
        void prepare_for_combat();

        /*
         * Perform any cleanup upon no more present danger, such as disabling combat CBMs or items.
         */
        void cleanup_on_no_danger();

        // complain about a specific issue if enough time has passed
        // @param issue string identifier of the issue
        // @param dur time duration between complaints
        // @param force true if the complaint should happen even if not enough time has elapsed since last complaint
        // @param speech words of this complaint
        bool complain_about( const std::string &issue, const time_duration &dur,
                             const std::string &speech, bool force = false,
                             sounds::sound_t priority = sounds::sound_t::speech );
        // wrapper for complain_about that warns about a specific type of threat, with
        // different warnings for hostile or friendly NPCs and hostile NPCs always complaining
        void warn_about( const std::string &type, const time_duration &d = 10_minutes,
                         const std::string &name = "", int range = -1, const tripoint &danger_pos = tripoint_zero );
        // return snippet strings by given range
        std::string distance_string( int range ) const;

        // Finds something to complain about and complains. Returns if complained.
        bool complain();

        int calc_spell_training_cost( bool knows, int difficulty, int level ) const;

        void handle_sound( sounds::sound_t priority, const std::string &description,
                           int heard_volume, const tripoint &spos );

        void witness_thievery( item *it );

        /* shift() works much like monster::shift(), and is called when the player moves
         * from one submap to an adjacent submap.  It updates our position (shifting by
         * 12 tiles), as well as our plans.
         */
        void shift( const point &s );

        // Movement; the following are defined in npcmove.cpp
        void move(); // Picks an action & a target and calls execute_action
        void execute_action( npc_action action ); // Performs action
        void process_turn() override;

        using Character::invoke_item;
        bool invoke_item( item *, const tripoint &pt, int pre_obtain_moves ) override;
        bool invoke_item( item *used, const std::string &method ) override;
        bool invoke_item( item * ) override;

        /** rates how dangerous a target is from 0 (harmless) to 1 (max danger) */
        float evaluate_enemy( const Creature &target ) const;

        void assess_danger();
        bool is_safe() const;
        // Functions which choose an action for a particular goal
        npc_action method_of_fleeing();
        npc_action method_of_attack();
        // among the different attack methods the npc has available, what's the best one in the current situation?
        // picks among melee, guns, spells, etc.
        // updates the ai_cache
        void evaluate_best_weapon( const Creature *target );

        static std::array<std::pair<std::string, overmap_location_str_id>, npc_need::num_needs> need_data;

        static std::string get_need_str_id( const npc_need &need );

        static overmap_location_str_id get_location_for( const npc_need &need );

        npc_action address_needs();
        npc_action address_needs( float danger );
        npc_action address_player();
        npc_action long_term_goal_action();
        // Returns true if did something and we should end turn
        bool scan_new_items();
        // Returns true if did wield it
        bool wield_better_weapon();

        // Helper functions for ranged combat
        // Multiplier for acceptable angle of inaccuracy
        double confidence_mult() const;
        int confident_shoot_range( const item &it, int at_recoil ) const;
        int confident_gun_mode_range( const gun_mode &gun, int at_recoil ) const;
        int confident_throw_range( const item &, Creature * ) const;
        void invalidate_range_cache();
        bool wont_hit_friend( const tripoint &tar, const item &it, bool throwing ) const;
        bool enough_time_to_reload( const item &gun ) const;
        /** Can reload currently wielded gun? */
        bool can_reload_current();
        /** Has a gun or magazine that can be reloaded */
        item_location find_reloadable();
        /** Finds ammo the NPC could use to reload a given object */
        item_location find_usable_ammo( const item_location &weap );
        item_location find_usable_ammo( const item_location &weap ) const;

        bool dispose_item( item_location &&obj, const std::string &prompt = std::string() ) override;

        void update_cardio_acc() override {};

        void aim( const Target_attributes &target_attributes );
        void do_reload( const item_location &it );

        // Physical movement from one tile to the next
        /**
         * Tries to find path to p. If it can, updates path to it.
         * @param p Destination of pathing
         * @param no_bashing Don't allow pathing through tiles that require bashing.
         * @param force If there is no valid path, empty the current path.
         * @returns If it updated the path.
         */
        bool update_path( const tripoint &p, bool no_bashing = false, bool force = true );
        bool can_open_door( const tripoint &p, bool inside ) const;
        bool can_move_to( const tripoint &p, bool no_bashing = false ) const;

        // nomove is used to resolve recursive invocation
        void move_to( const tripoint &p, bool no_bashing = false, std::set<tripoint> *nomove = nullptr );
        // Next in <path>
        void move_to_next();
        // Maneuver so we won't shoot u
        void avoid_friendly_fire();
        void escape_explosion();
        // nomove is used to resolve recursive invocation
        void move_away_from( const tripoint &p, bool no_bash_atk = false,
                             std::set<tripoint> *nomove = nullptr );
        void move_away_from( const std::vector<sphere> &spheres, bool no_bashing = false );
        // workers at camp relaxing/wandering
        void worker_downtime();
        bool find_job_to_perform();
        // Same as if the player pressed '.'
        void move_pause();

        void set_movement_mode( const move_mode_id &mode ) override;

        const pathfinding_settings &get_pathfinding_settings() const override;
        const pathfinding_settings &get_pathfinding_settings( bool no_bashing ) const;
        std::set<tripoint> get_path_avoid() const override;

        // Item discovery and fetching

        // Comment on item seen
        void see_item_say_smth( const itype_id &object, const std::string &smth );
        // Look around and pick an item
        void find_item();
        // Move to, or grab, our targeted item
        void pick_up_item();
        /** Picks up items and returns a list of them. */
        std::list<item> pick_up_item_map( const tripoint &where );
        std::list<item> pick_up_item_vehicle( vehicle &veh, int part_index );

        bool has_item_whitelist() const;
        bool item_name_whitelisted( const std::string &to_match );
        bool item_whitelisted( const item &it );

        /** Returns true if it finds one. */
        bool find_corpse_to_pulp();
        /** Returns true if it handles the turn. */
        bool do_pulp();
        /** perform a player activity, returning true if it took up the turn */
        bool do_player_activity();

        // Combat functions and player interaction functions
        // Returns true if did something
        bool alt_attack();
        void heal_player( Character &patient );
        void heal_self();
        void pretend_heal( Character &patient, item used ); // healing action of hallucinations
        void mug_player( Character &mark );
        void look_for_player( const Character &sought );
        // Do we have an idea of where u are?
        bool saw_player_recently() const;
        /** Returns true if food was consumed, false otherwise. */
        bool consume_food();
        bool consume_food_from_camp();
        int get_thirst() const override;

        // Movement on the overmap scale
        // Do we have a long-term destination?
        bool has_omt_destination() const;
        // Pick a place to go
        void set_omt_destination();
        // Move there; on the micro scale
        void go_to_omt_destination();
        // We made it!
        void reach_omt_destination();

        void guard_current_pos();

        // Message related stuff
        using Character::add_msg_if_npc;
        void add_msg_if_npc( const std::string &msg ) const override;
        void add_msg_if_npc( const game_message_params &params, const std::string &msg ) const override;
        using Character::add_msg_debug_if_npc;
        void add_msg_debug_if_npc( debugmode::debug_filter type, const std::string &msg ) const override;
        using Character::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        void add_msg_player_or_npc( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using Character::add_msg_debug_player_or_npc;
        void add_msg_debug_player_or_npc( debugmode::debug_filter type, const std::string &player_msg,
                                          const std::string &npc_msg ) const override;
        using Character::add_msg_if_player;
        void add_msg_if_player( const std::string &/*msg*/ ) const override {}
        void add_msg_if_player( const game_message_params &/*type*/,
                                const std::string &/*msg*/ ) const override {}
        using Character::add_msg_debug_if_player;
        void add_msg_debug_if_player( debugmode::debug_filter /*type*/,
                                      const std::string &/*msg*/ ) const override {}
        using Character::add_msg_player_or_say;
        void add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &npc_speech ) const override;
        void add_msg_player_or_say( const game_message_params &params, const std::string &player_msg,
                                    const std::string &npc_speech ) const override;

        // The preceding are in npcmove.cpp

        bool query_yn( const std::string &mes ) const override;

        std::string extended_description() const override;
        std::string get_epilogue() const;

        std::pair<std::string, nc_color> hp_description() const;

        // Note: NPCs use a different speed rating than players
        // Because they can't run yet
        float speed_rating() const override;
        /**
         * Note: this places NPC on a given position in CURRENT MAP coordinates.
         * Do not use when placing a NPC in mapgen.
         */
        void travel_overmap( const tripoint_abs_omt &pos );
        npc_attitude get_attitude() const override;
        void set_attitude( npc_attitude new_attitude );
        void set_mission( npc_mission new_mission );
        bool has_activity() const;
        bool has_job() const {
            return job.has_job();
        }
        npc_attitude get_previous_attitude();
        npc_mission get_previous_mission() const;
        void revert_after_activity();

        // #############   VALUES   ################
        activity_id current_activity_id = activity_id::NULL_ID();
        npc_class_id myclass; // What's our archetype?
        npc_class_id idz; // actual npc template used
        // A temp variable used to link to the correct mission
        std::vector<mission_type_id> miss_ids;
        std::optional<tripoint_abs_omt> assigned_camp = std::nullopt;

        // accessors to ai_cache functions
        const std::vector<weak_ptr_fast<Creature>> &get_cached_friends() const;
        std::optional<int> closest_enemy_to_friendly_distance() const;

    private:
        npc_attitude attitude = NPCATT_NULL; // What we want to do to the player
        npc_attitude previous_attitude = NPCATT_NULL;
        bool known_to_u = false; // Does the player know this NPC?
        // Type of complaint->last time we complained about this type
        std::map<std::string, time_point> complaints;

        npc_short_term_cache ai_cache;

        std::map<npc_need, npc_need_goal_cache> goal_cache;
    public:
        const std::shared_ptr<npc_attack> &get_current_attack() const {
            return ai_cache.current_attack;
        }

        const npc_attack_rating &get_current_attack_evaluation() const {
            return ai_cache.current_attack_evaluation;
        }

        // Where we last saw the player
        std::optional<tripoint_abs_ms> last_player_seen_pos;
        int last_seen_player_turn = 0; // Timeout to forgetting
        tripoint wanted_item_pos; // The square containing an item we want
        // These are the coordinates that a guard will return to inside of their goal tripoint
        std::optional<tripoint_abs_ms> guard_pos;
        // This is the spot the NPC wants to move to to sit and relax.
        std::optional<tripoint_abs_ms> chair_pos;
        std::optional<tripoint_abs_omt> base_location; // our faction base location in OMT coords.
        /**
         * Global overmap terrain coordinate, where we want to get to
         * if no goal exist, this is no_goal_point.
         */
        tripoint_abs_omt goal;
        // Spot to wander off to when idle.
        std::optional<tripoint_abs_ms> wander_pos;
        item *known_stolen_item = nullptr; // the item that the NPC wants the player to drop or barter for.
        // Location of the corpse we'd like to pulp (if any).
        std::optional<tripoint_abs_ms> pulp_location;
        time_point restock;
        bool fetching_item = false;
        bool has_new_items = false; // If true, we have something new and should re-equip
        int  worst_item_value = 0; // The value of our least-wanted item

        std::vector<tripoint> path; // Our movement plans

        // Personality & other defining characteristics
        std::string companion_mission_role_id; //Set mission source or squad leader for a patrol
        //Mission leader use to determine item sorting, patrols use for points
        std::vector<tripoint_abs_omt> companion_mission_points;
        time_point companion_mission_time; //When you left for ongoing/repeating missions
        time_point
        companion_mission_time_ret; //When you are expected to return for calculated/variable mission returns
        inventory companion_mission_inv; //Inventory that is added and dropped on mission
        npc_mission mission = NPC_MISSION_NULL;
        npc_mission previous_mission = NPC_MISSION_NULL;
        npc_personality personality;
        npc_opinion op_of_u;
        dialogue_chatbin chatbin;
        int patience = 0; // Used when we expect the player to leave the area
        npc_follower_rules rules;
        bool marked_for_death = false; // If true, we die as soon as we respawn!
        bool hit_by_player = false;
        bool hallucination = false; // If true, NPC is an hallucination
        std::vector<npc_need> needs;
        std::optional<int> confident_range_cache;
        // Dummy point that indicates that the goal is invalid.
        static constexpr tripoint_abs_omt no_goal_point{ tripoint_min };
        job_data job;
        /**
         * Do some cleanup and caching as npc is being unloaded from map.
         */
        void on_unload();
        /**
         * Retroactively update npc.
         */
        void on_load();
        /**
         * Update body, but throttled.
         */
        void npc_update_body();

        bool get_known_to_u() const;

        void set_known_to_u( bool known );

        /// Set up (start) a companion mission.
        void set_companion_mission( npc &p, const mission_id &miss_id );
        void set_companion_mission( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                                    const mission_id &miss_id );
        void set_companion_mission(
            const tripoint_abs_omt &omt_pos, const std::string &role_id,
            const mission_id &miss_id, const tripoint_abs_omt &destination );
        /// Unset a companion mission. Precondition: `!has_companion_mission()`
        void reset_companion_mission();
        std::optional<tripoint_abs_omt> get_mission_destination() const;
        bool has_companion_mission() const;
        npc_companion_mission get_companion_mission() const;
        attitude_group get_attitude_group( npc_attitude att ) const;
        void set_unique_id( const std::string &id );
        std::string get_unique_id() const;
    protected:
        void store( JsonOut &json ) const;
        void load( const JsonObject &data );

        void on_move( const tripoint_abs_ms &old_pos ) override;
    private:
        // the weapon we're actually holding when using bionic fake guns
        item real_weapon;
        // the index of the bionics for the fake gun;
        int cbm_weapon_index = -1;

        bool dead = false;  // If true, we need to be cleaned up

        bool sees_dangerous_field( const tripoint &p ) const;
        bool could_move_onto( const tripoint &p ) const;

        std::vector<sphere> find_dangerous_explosives() const;

        npc_companion_mission comp_mission;

        std::string unique_id;
};

/** An NPC with standard stats */
class standard_npc : public npc
{
    public:
        explicit standard_npc( const std::string &name = "",
                               const tripoint &pos = tripoint( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 ),
                               const std::vector<std::string> &clothing = {},
                               int sk_lvl = 4, int s_str = 8, int s_dex = 8, int s_int = 8, int s_per = 8 );
};

// instances of this can be accessed via string_id<npc_template>.
class npc_template
{
    public:
        npc_template() = default;

        npc guy;
        translation name_unique;
        translation name_suffix;
        enum class gender : int {
            random,
            male,
            female
        };
        gender gender_override = gender::random;

        static void load( const JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

std::ostream &operator<< ( std::ostream &os, const npc_need &need );

/** Opens a menu and allows player to select a friendly NPC. */
npc *pick_follower();
std::unique_ptr<talker> get_talker_for( npc &guy );
std::unique_ptr<talker> get_talker_for( npc *guy );
#endif // CATA_SRC_NPC_H
