#pragma once
#ifndef NPC_H
#define NPC_H

#include "player.h"
#include "faction.h"
#include "pimpl.h"
#include "calendar.h"

#include <vector>
#include <string>
#include <map>
#include <memory>

class JsonObject;
class JsonIn;
class JsonOut;
class item;
class overmap;
class player;
class field_entry;
class npc_class;
class auto_pickup;
class monfaction;
struct mission_type;
struct npc_companion_mission;
struct overmap_location;

enum game_message_type : int;
class gun_mode;
using npc_class_id = string_id<npc_class>;
using mission_type_id = string_id<mission_type>;
using mfaction_id = int_id<monfaction>;
using overmap_location_str_id = string_id<overmap_location>;

void parse_tags( std::string &phrase, const player &u, const player &me );

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
    NPCATT_FLEE,  // Get away from the player
    NPCATT_LEGACY_3,
    NPCATT_HEAL,  // Get to the player and heal them

    NPCATT_LEGACY_4,
    NPCATT_LEGACY_5
};

std::string npc_attitude_name( npc_attitude );

// Attitudes are grouped by overall behavior towards player
enum class attitude_group : int {
    neutral = 0, // Doesn't particularly mind the player
    hostile, // Not necessarily attacking, but also mugging, exploiting etc.
    fearful, // Run
    friendly // Follow, defend, listen
};

enum npc_mission : int {
    NPC_MISSION_NULL = 0, // Nothing in particular
    NPC_MISSION_LEGACY_1,
    NPC_MISSION_SHELTER, // Stay in shelter, introduce player to game
    NPC_MISSION_SHOPKEEP, // Stay still unless combat or something and sell stuff

    NPC_MISSION_LEGACY_2,
    NPC_MISSION_LEGACY_3,

    NPC_MISSION_BASE, // Base Mission: unassigned (Might be used for assigning a npc to stay in a location).
    NPC_MISSION_GUARD, // Similar to Base Mission, for use outside of camps
};

struct npc_companion_mission {
    std::string mission_id;
    tripoint position;
    std::string role_id;
};

std::string npc_class_name( const npc_class_id & );
std::string npc_class_name_str( const npc_class_id & );

enum npc_action : int;

enum npc_need {
    need_none,
    need_ammo, need_weapon, need_gun,
    need_food, need_drink,
    num_needs
};

// @todo: Turn the personality struct into a vector/map?
enum npc_personality_type : int {
    NPE_AGGRESSION,
    NPE_BRAVERY,
    NPE_COLLECTOR,
    NPE_ALTRUISM,
    NUM_NPE
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

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct npc_opinion {
    int trust;
    int fear;
    int value;
    int anger;
    int owed;

    npc_opinion() {
        trust = 0;
        fear  = 0;
        value = 0;
        anger = 0;
        owed = 0;
    }

    npc_opinion( int T, int F, int V, int A, int O ) :
        trust( T ), fear( F ), value( V ), anger( A ), owed( O ) {
    }

    npc_opinion &operator+=( const npc_opinion &rhs ) {
        trust += rhs.trust;
        fear  += rhs.fear;
        value += rhs.value;
        anger += rhs.anger;
        owed  += rhs.owed;
        return *this;
    }

    npc_opinion operator+( const npc_opinion &rhs ) {
        return ( npc_opinion( *this ) += rhs );
    }

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

enum combat_engagement {
    ENGAGE_NONE = 0,
    ENGAGE_CLOSE,
    ENGAGE_WEAK,
    ENGAGE_HIT,
    ENGAGE_ALL,
    ENGAGE_NO_MOVE
};

enum aim_rule {
    // Aim some
    AIM_WHEN_CONVENIENT = 0,
    // No concern for ammo efficiency
    AIM_SPRAY,
    // Aim when possible, then shoot
    AIM_PRECISE,
    // If you can't aim, don't shoot
    AIM_STRICTLY_PRECISE
};


struct npc_follower_rules {
    combat_engagement engagement;
    aim_rule aim = AIM_WHEN_CONVENIENT;
    bool use_guns;
    bool use_grenades;
    bool use_silent;

    bool allow_pick_up;
    bool allow_bash;
    bool allow_sleep;
    bool allow_complain;
    bool allow_pulp;

    bool close_doors;

    pimpl<auto_pickup> pickup_whitelist;

    npc_follower_rules();

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

// Data relevant only for this action
struct npc_short_term_cache {
    float danger;
    float total_danger;
    float danger_assessment;
    std::shared_ptr<Creature> target;

    double my_weapon_value;

    std::vector<std::shared_ptr<Creature>> friends;
    std::vector<sphere> dangerous_explosives;
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

struct npc_chatbin {
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
    std::vector<mission *> missions;
    /**
     * Mission that have been assigned by this NPC to a player character.
     */
    std::vector<mission *> missions_assigned;
    /**
     * The mission (if any) that we talk about right now. Can be null. Should be one of the
     * missions in @ref missions or @ref missions_assigned.
     */
    mission *mission_selected = nullptr;
    /**
     * The skill this NPC offers to train.
     */
    skill_id skill = skill_id::NULL_ID();
    /**
     * The martial art style this NPC offers to train.
     */
    matype_id style;
    std::string first_topic = "TALK_NONE";

    npc_chatbin() = default;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

class npc;
class npc_template;
struct epilogue;

typedef std::map<std::string, epilogue> epilogue_map;

class npc : public player
{
    public:

        npc();
        npc( const npc & );
        npc( npc && );
        npc &operator=( const npc & );
        npc &operator=( npc && );
        ~npc() override;

        bool is_player() const override {
            return false;
        }
        bool is_npc() const override {
            return true;
        }

        void load_npc_template( const string_id<npc_template> &ident );

        // Generating our stats, etc.
        void randomize( const npc_class_id &type = npc_class_id::NULL_ID() );
        void randomize_from_faction( faction *fac );
        void set_fac( const string_id<faction> &id );
        /**
         * Set @ref submap_coords and @ref pos.
         * @param mx,my,mz are global submap coordinates.
         */
        void spawn_at_sm( int mx, int my, int mz );
        /**
         * As spawn_at, but also sets position within the submap.
         * Note: final submap may differ from submap_offset if @ref square has
         * x/y values outside [0, SEEX-1]/[0, SEEY-1] range.
         */
        void spawn_at_precise( const point &submap_offset, const tripoint &square );
        /**
         * Places the NPC on the @ref map. This update its
         * pos values to fit the current offset of
         * map (g->levx, g->levy).
         * If the square on the map where the NPC would go is not empty
         * a spiral search for an empty square around it is performed.
         */
        void place_on_map();
        /**
         * See @ref npc_chatbin::add_new_mission
         */
        void add_new_mission( mission *miss );
        skill_id best_skill() const;
        void starting_weapon( const npc_class_id &type );

        // Save & load
        void load_info( std::string data ) override; // Overloaded from player
        virtual std::string save_info() const override;

        void deserialize( JsonIn &jsin ) override;
        void serialize( JsonOut &jsout ) const override;

        // Display
        nc_color basic_symbol_color() const override;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;
        std::string short_description() const;
        std::string opinion_text() const;

        // Goal / mission functions
        void pick_long_term_goal();
        bool fac_has_value( faction_value value ) const;
        bool fac_has_job( faction_job job ) const;

        // Interaction with the player
        void form_opinion( const player &u );
        std::string pick_talk_topic( const player &u );
        float character_danger( const Character &u ) const;
        float vehicle_danger( int radius ) const;
        bool turned_hostile() const; // True if our anger is at least equal to...
        int hostile_anger_level() const; // ... this value!
        void make_angry(); // Called if the player attacks us
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
        std::vector<skill_id> skills_offered_to( const player &p ) const;
        /**
         * Martial art styles that we known, but the player p doesn't.
         */
        std::vector<matype_id> styles_offered_to( const player &p ) const;
        // State checks
        bool is_enemy() const; // We want to kill/mug/etc the player
        bool is_following() const; // Traveling w/ player (whether as a friend or a slave)
        bool is_friend() const; // Allies with the player
        bool is_leader() const; // Leading the player
        /** Standing in one spot, moving back if removed from it. */
        bool is_guarding() const;
        /** Trusts you a lot. */
        bool is_minion() const;
        /** Is enemy or will turn into one (can't be convinced not to attack). */
        bool guaranteed_hostile() const;
        Attitude attitude_to( const Creature &other ) const override;

        /** For mutant NPCs. Returns how monsters perceive said NPC. Doesn't imply NPC sees them the same. */
        mfaction_id get_monster_faction() const;
        // What happens when the player makes a request
        int  follow_distance() const; // How closely do we follow the player?


        // Dialogue and bartering--see npctalk.cpp
        void talk_to_u();
        // Re-roll the inventory of a shopkeeper
        void shop_restock();
        // Use and assessment of items
        int  minimum_item_value() const; // The minimum value to want to pick up an item
        void update_worst_item_value(); // Find the worst value in our inventory
        int value( const item &it ) const;
        int value( const item &it, int market_price ) const;
        bool wear_if_wanted( const item &it );
        bool wield( item &it ) override;
        bool adjust_worn();
        bool has_healing_item( bool bleed = false, bool bite = false, bool infect = false );
        item &get_healing_item( bool bleed = false, bool bite = false, bool infect = false,
                                bool first_best = false );
        bool has_painkiller();
        bool took_painkiller() const;
        void use_painkiller();
        void activate_item( int position );
        /** Is the item safe or does the NPC trust you enough? */
        bool will_accept_from_player( const item &it ) const;

        bool wants_to_sell( const item &it ) const;
        bool wants_to_sell( const item &it, int at_price, int market_price ) const;
        bool wants_to_buy( const item &it ) const;
        bool wants_to_buy( const item &it, int at_price, int market_price ) const;

        // AI helpers
        void regen_ai_cache();
        const Creature *current_target() const;
        Creature *current_target();

        // Interaction and assessment of the world around us
        float danger_assessment();
        float average_damage_dealt(); // Our guess at how much damage we can deal
        bool bravery_check( int diff );
        bool emergency() const;
        bool emergency( float danger ) const;
        bool is_active() const;
        template<typename ...Args>
        void say( const char *const line, Args &&... args ) const {
            return say( string_format( line, std::forward<Args>( args )... ) );
        }
        void say( const std::string &line ) const;
        void decide_needs();
        void die( Creature *killer ) override;
        bool is_dead() const;
        int smash_ability() const; // How well we smash terrain (not corpses!)
        bool complain(); // Finds something to complain about and complains. Returns if complained.
        /* shift() works much like monster::shift(), and is called when the player moves
         * from one submap to an adjacent submap.  It updates our position (shifting by
         * 12 tiles), as well as our plans.
         */
        void shift( int sx, int sy );


        // Movement; the following are defined in npcmove.cpp
        void move(); // Picks an action & a target and calls execute_action
        void execute_action( npc_action action ); // Performs action
        void process_turn() override;

        /** rates how dangerous a target is from 0 (harmless) to 1 (max danger) */
        float evaluate_enemy( const Creature &target ) const;

        void choose_target();
        void assess_danger();
        // Functions which choose an action for a particular goal
        npc_action method_of_fleeing();
        npc_action method_of_attack();

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
        bool wont_hit_friend( const tripoint &p, const item &it, bool throwing ) const;
        bool enough_time_to_reload( const item &gun ) const;
        /** Can reload currently wielded gun? */
        bool can_reload_current();
        /** Has a gun or magazine that can be reloaded */
        const item &find_reloadable() const;
        item &find_reloadable();
        /** Finds ammo the NPC could use to reload a given object */
        item_location find_usable_ammo( const item &weap );
        const item_location find_usable_ammo( const item &weap ) const;

        bool dispose_item( item_location &&obj, const std::string &prompt = std::string() ) override;

        void aim();
        void do_reload( item &what );

        // Physical movement from one tile to the next
        /**
         * Tries to find path to p. If it can, updates path to it.
         * @param p Destination of pathing
         * @param no_bashing Don't allow pathing through tiles that require bashing.
         * @param force If there is no valid path, empty the current path.
         * @returns If it updated the path.
         */
        bool update_path( const tripoint &p, bool no_bashing = false, bool force = true );
        bool can_move_to( const tripoint &p, bool no_bashing = false ) const;
        void move_to( const tripoint &p, bool no_bashing = false );
        void move_to_next(); // Next in <path>
        void avoid_friendly_fire(); // Maneuver so we won't shoot u
        void escape_explosion();
        void move_away_from( const tripoint &p, bool no_bashing = false );
        void move_away_from( const std::vector<sphere> &spheres, bool no_bashing = false );
        void move_pause(); // Same as if the player pressed '.'

        const pathfinding_settings &get_pathfinding_settings() const override;
        const pathfinding_settings &get_pathfinding_settings( bool no_bashing ) const;
        std::set<tripoint> get_path_avoid() const override;

        // Item discovery and fetching
        void find_item();   // Look around and pick an item
        void pick_up_item();  // Move to, or grab, our targeted item
        void drop_items( int weight, int volume ); // Drop wgt and vol

        /** Picks up items and returns a list of them. */
        std::list<item> pick_up_item_map( const tripoint &where );
        std::list<item> pick_up_item_vehicle( vehicle &veh, int part_index );

        bool has_item_whitelist() const;
        bool item_name_whitelisted( const std::string &name );
        bool item_whitelisted( const item &it );

        /** Returns true if it finds one. */
        bool find_corpse_to_pulp();
        /** Returns true if it handles the turn. */
        bool do_pulp();

        // Combat functions and player interaction functions
        void wield_best_melee();
        bool alt_attack(); // Returns true if did something
        void heal_player( player &patient );
        void heal_self();
        void take_painkiller();
        void mug_player( player &mark );
        void look_for_player( player &sought );
        bool saw_player_recently() const;// Do we have an idea of where u are?
        /** Returns true if food was consumed, false otherwise. */
        bool consume_food();

        // Movement on the overmap scale
        bool has_destination() const; // Do we have a long-term destination?
        void set_destination(); // Pick a place to go
        void go_to_destination(); // Move there; on the micro scale
        void reach_destination(); // We made it!

        void guard_current_pos();

        //message related stuff
        using player::add_msg_if_npc;
        void add_msg_if_npc( const std::string &msg ) const override;
        void add_msg_if_npc( game_message_type type, const std::string &msg ) const override;
        using player::add_msg_player_or_npc;
        void add_msg_player_or_npc( const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        void add_msg_player_or_npc( game_message_type type, const std::string &player_msg,
                                    const std::string &npc_msg ) const override;
        using player::add_msg_if_player;
        void add_msg_if_player( const std::string &/*msg*/ ) const override {}
        void add_msg_if_player( game_message_type /*type*/, const std::string &/*msg*/ ) const override {}
        using player::add_memorial_log;
        void add_memorial_log( const std::string &/*male_msg*/,
                               const std::string &/*female_msg*/ ) override {}
        using player::add_msg_player_or_say;
        void add_msg_player_or_say( const std::string &player_msg,
                                    const std::string &npc_speech ) const override;
        void add_msg_player_or_say( game_message_type type, const std::string &player_msg,
                                    const std::string &npc_speech ) const override;

        // The preceding are in npcmove.cpp

        bool query_yn( const std::string &mes ) const override;

        std::string extended_description() const override;

        // Note: NPCs use a different speed rating than players
        // Because they can't run yet
        float speed_rating() const override;

        /**
         * Note: this places NPC on a given position in CURRENT MAP coordinates.
         * Do not use when placing a NPC in mapgen.
         */
        void setpos( const tripoint &pos ) override;

        npc_attitude get_attitude() const;
        void set_attitude( npc_attitude new_attitude );

        // #############   VALUES   ################

        npc_class_id myclass; // What's our archetype?
        std::string idz; // A temp variable used to inform the game which npc json to use as a template
        mission_type_id miss_id; // A temp variable used to link to the correct mission

    private:

        npc_attitude attitude; // What we want to do to the player
        /**
         * Global submap coordinates of the submap containing the npc.
         * Use global_*_location to get the global position.
         * You should not change submap_coords directly, use pos instead,
         * @ref shift will update submap_coords and move the npc to a different
         * overmap if needed.
         * submap_coords defines the overmap the npc is stored on.
         */
        point submap_coords;
        // Type of complaint->last time we complained about this type
        std::map<std::string, time_point> complaints;

        npc_short_term_cache ai_cache;
    public:
        /**
         * Global position, expressed in map square coordinate system
         * (the most detailed coordinate system), used by the @ref map.
         *
         * The (global) position of an NPC is always:
         * point(
         *     submap_coords.x * SEEX + posx() % SEEX,
         *     submap_coords.y * SEEY + posy() % SEEY,
         *     pos.z)
         * (Expressed in map squares, the system that @ref map uses.)
         * Any of om, map, pos can be in any range.
         * For active NPCs pos would be in the valid range required by
         * the map. But pos, map, and om can be changed without the NPC
         * actual moving as long as the position stays the same:
         * pos() += SEEX; submap_coords.x -= 1;
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

        /**
         * Location and index of the corpse we'd like to pulp (if any).
         */
        tripoint pulp_location;

        time_point restock;
        bool fetching_item;
        bool has_new_items; // If true, we have something new and should re-equip
        int  worst_item_value; // The value of our least-wanted item

        std::vector<tripoint> path; // Our movement plans

        // Personality & other defining characteristics
        string_id<faction> fac_id; // A temp variable used to inform the game which faction to link
        faction *my_fac;

        std::string companion_mission_role_id; //Set mission source or squad leader for a patrol
        std::vector<tripoint>
        companion_mission_points; //Mission leader use to determine item sorting, patrols use for points
        time_point companion_mission_time; //When you left for ongoing/repeating missions
        time_point
        companion_mission_time_ret; //When you are expected to return for calculated/variable mission returns
        inventory companion_mission_inv; //Inventory that is added and dropped on mission
        npc_mission mission;
        npc_personality personality;
        npc_opinion op_of_u;
        npc_chatbin chatbin;
        int patience; // Used when we expect the player to leave the area
        npc_follower_rules rules;
        bool marked_for_death; // If true, we die as soon as we respawn!
        bool hit_by_player;
        std::vector<npc_need> needs;
        // Dummy point that indicates that the goal is invalid.
        static const tripoint no_goal_point;

        time_point last_updated;
        /**
         * Do some cleanup and caching as npc is being unloaded from map.
         */
        void on_unload();
        /**
         * Retroactively update npc.
         */
        void on_load();


        /// Set up (start) a companion mission.
        void set_companion_mission( npc &p, const std::string &id );
        /// Unset a companion mission. Precondition: `!has_companion_mission()`
        void reset_companion_mission();
        bool has_companion_mission() const;
        npc_companion_mission get_companion_mission() const;

    protected:
        void store( JsonOut &jsout ) const;
        void load( JsonObject &jsin );

    private:
        void setID( int id );
        bool dead;  // If true, we need to be cleaned up

        bool sees_dangerous_field( const tripoint &p ) const;
        bool could_move_onto( const tripoint &p ) const;

        std::vector<sphere> find_dangerous_explosives() const;

        npc_companion_mission comp_mission;
};

/** An NPC with standard stats */
class standard_npc : public npc
{
    public:
        standard_npc( const std::string &name = "", const std::vector<itype_id> &clothing = {},
                      int skill = 4, int s_str = 8, int s_dex = 8, int s_int = 8, int s_per = 8 );
};

// instances of this can be accessed via string_id<npc_template>.
class npc_template
{
    public:
        npc_template() : guy() {}

        npc guy;

        static void load( JsonObject &jsobj );
        static void reset();
        static void check_consistency();
};

struct epilogue {
    epilogue();

    std::string id; //Unique name for declaring an ending for a given individual
    std::string group; //Male/female (dog/cyborg/mutant... whatever you want)
    std::string text;

    static epilogue_map _all_epilogue;

    static void load_epilogue( JsonObject &jsobj );
    epilogue *find_epilogue( const std::string &ident );
    void random_by_group( std::string group );
};

std::ostream &operator<< ( std::ostream &os, const npc_need &need );

/** Opens a menu and allows player to select a friendly NPC. */
npc *pick_follower();

#endif
