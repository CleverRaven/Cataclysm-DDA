#pragma once
#ifndef MISSION_H
#define MISSION_H

#include <functional>
#include <iosfwd>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include "calendar.h"
#include "enums.h"
#include "npc_favor.h"
#include "overmap.h"
#include "item_group.h"

class player;
class mission;
class game;
class npc;
class Creature;
class npc_class;
class JsonObject;
class JsonArray;
class JsonIn;
class JsonOut;
struct mission_type;
struct oter_type_t;
struct species_type;
struct mtype;

enum npc_mission : int;

using npc_class_id = string_id<npc_class>;
using mission_type_id = string_id<mission_type>;
using species_id = string_id<species_type>;
using mtype_id = string_id<mtype>;

namespace debug_menu
{
class mission_debug;
}

enum mission_origin {
    ORIGIN_NULL = 0,
    ORIGIN_GAME_START, // Given when the game starts
    ORIGIN_OPENER_NPC, // NPC comes up to you when the game starts
    ORIGIN_ANY_NPC,    // Any NPC
    ORIGIN_SECONDARY,  // Given at the end of another mission
    ORIGIN_COMPUTER,   // Taken after reading investigation provoking entries in computer terminal
    NUM_ORIGIN
};

enum mission_goal {
    MGOAL_NULL = 0,
    MGOAL_GO_TO,             // Reach a certain overmap tile
    MGOAL_GO_TO_TYPE,        // Instead of a point, go to an oter_type_id map tile like "hospital_entrance"
    MGOAL_FIND_ITEM,         // Find an item of a given type
    MGOAL_FIND_ANY_ITEM,     // Find an item tagged with this mission
    MGOAL_FIND_ITEM_GROUP,   // Find items that belong to a specific item group
    MGOAL_FIND_MONSTER,      // Find and retrieve a friendly monster
    MGOAL_FIND_NPC,          // Find a given NPC
    MGOAL_ASSASSINATE,       // Kill a given NPC
    MGOAL_KILL_MONSTER,      // Kill a particular hostile monster
    MGOAL_KILL_MONSTER_TYPE, // Kill a number of a given monster type
    MGOAL_RECRUIT_NPC,       // Recruit a given NPC
    MGOAL_RECRUIT_NPC_CLASS, // Recruit an NPC class
    MGOAL_COMPUTER_TOGGLE,   // Activating the correct terminal will complete the mission
    MGOAL_KILL_MONSTER_SPEC,  // Kill a number of monsters from a given species
    NUM_MGOAL
};
const std::unordered_map<std::string, mission_goal> mission_goal_strs = { {
        { "MGOAL_NULL", MGOAL_NULL },
        { "MGOAL_GO_TO", MGOAL_GO_TO },
        { "MGOAL_GO_TO_TYPE", MGOAL_GO_TO_TYPE },
        { "MGOAL_FIND_ITEM", MGOAL_FIND_ITEM },
        { "MGOAL_FIND_ITEM_GROUP", MGOAL_FIND_ITEM_GROUP },
        { "MGOAL_FIND_ANY_ITEM", MGOAL_FIND_ANY_ITEM },
        { "MGOAL_FIND_MONSTER", MGOAL_FIND_MONSTER },
        { "MGOAL_FIND_NPC", MGOAL_FIND_NPC },
        { "MGOAL_ASSASSINATE", MGOAL_ASSASSINATE },
        { "MGOAL_KILL_MONSTER", MGOAL_KILL_MONSTER },
        { "MGOAL_KILL_MONSTER_TYPE", MGOAL_KILL_MONSTER_TYPE },
        { "MGOAL_RECRUIT_NPC", MGOAL_RECRUIT_NPC },
        { "MGOAL_RECRUIT_NPC_CLASS", MGOAL_RECRUIT_NPC_CLASS },
        { "MGOAL_COMPUTER_TOGGLE", MGOAL_COMPUTER_TOGGLE },
        { "MGOAL_KILL_MONSTER_SPEC", MGOAL_KILL_MONSTER_SPEC }
    }
};

struct mission_place {
    // Return true if the place (global overmap terrain coordinate) is valid for starting a mission
    static bool never( const tripoint & ) {
        return false;
    }
    static bool always( const tripoint & ) {
        return true;
    }
    static bool near_town( const tripoint & );
};

struct mission_start_t {
    void set_reveal( const std::string &terrain );
    void set_reveal_any( JsonArray &ja );
    void set_assign_mission_target( JsonObject &jo );
    void load( JsonObject &jo );
    void apply( mission *miss ) const;
    std::vector<std::function<void( mission *miss )>> start_funcs;
};

/* mission_start functions are first run when a mission is accepted; this
 * initializes the mission's key values, like the target and description.
 */
struct mission_start {
    static void standard( mission * );           // Standard for its goal type
    static void join( mission * );               // NPC giving mission joins your party
    static void infect_npc( mission * );         // "infection", remove antibiotics
    static void need_drugs_npc( mission * );     // "need drugs" remove item
    static void place_dog( mission * );          // Put a dog in a house!
    static void place_zombie_mom( mission * );   // Put a zombie mom in a house!
    // Put a boss zombie in the refugee/evac center back bay
    static void place_zombie_bay( mission * );
    static void place_caravan_ambush( mission * ); // For Free Merchants mission
    static void place_bandit_cabin( mission * ); // For Old Guard mission
    static void place_informant( mission * );    // For Old Guard mission
    static void place_grabber( mission * );      // For Old Guard mission
    static void place_bandit_camp( mission * );  // For Old Guard mission
    static void place_jabberwock( mission * );   // Put a jabberwok in the woods nearby
    static void kill_20_nightmares( mission * ); // Kill 20 more regular nightmares
    static void kill_horde_master( mission * );  // Kill the master zombie at the center of the horde
    static void place_npc_software( mission * ); // Put NPC-type-dependent software
    static void place_priest_diary( mission * ); // Hides the priest's diary in a local house
    static void place_deposit_box( mission * );  // Place a safe deposit box in a nearby bank
    static void find_safety( mission * );        // Goal is set to non-spawn area
    static void recruit_tracker( mission * );    // Recruit a tracker to help you
    static void start_commune( mission * );      // Focus on starting the ranch commune
    static void ranch_construct_1( mission * );  // Encloses barn
    static void ranch_construct_2( mission * );  // Adds makeshift beds to the barn, 1 NPC
    static void ranch_construct_3( mission * );  // Adds a couple of NPCs and fields
    static void ranch_construct_4( mission * );  // Begins work on wood yard, crop overseer added
    static void ranch_construct_5( mission * );  // Continues work on wood yard, crops, well (pit)
    // Continues work on wood yard, well (covered), fireplaces
    static void ranch_construct_6( mission * );
    // Continues work on wood yard, well (finished), continues walling
    static void ranch_construct_7( mission * );
    // Finishes wood yard, starts outhouse, starts tool shed
    static void ranch_construct_8( mission * );
    static void ranch_construct_9( mission * );  // Finishes outhouse, finishes tool shed, starts clinic
    static void ranch_construct_10( mission * ); // Continues clinic, starts chop-shop
    static void ranch_construct_11( mission * ); // Continues clinic, continues chop-shop
    static void ranch_construct_12( mission * ); // Finish chop-shop, starts junk shop
    static void ranch_construct_13( mission * ); // Continues junk shop
    static void ranch_construct_14( mission * ); // Finish junk shop, starts bar
    static void ranch_construct_15( mission * ); // Continues bar
    static void ranch_construct_16( mission * ); // Finish bar, start greenhouse
    static void ranch_nurse_1( mission * );      // Need aspirin
    static void ranch_nurse_2( mission * );      // Need hotplates
    static void ranch_nurse_3( mission * );      // Need vitamins
    static void ranch_nurse_4( mission * );      // Need charcoal water filters
    static void ranch_nurse_5( mission * );      // Need chemistry set
    static void ranch_nurse_6( mission * );      // Need filter masks
    static void ranch_nurse_7( mission * );      // Need rubber gloves
    static void ranch_nurse_8( mission * );      // Need X-acto
    static void ranch_nurse_9( mission * );      // Need Guide to Advanced Emergency Care
    static void ranch_scavenger_1( mission * );  // Expand Junk Shop
    static void ranch_scavenger_2( mission * );  // Expand Junk Shop
    static void ranch_scavenger_3( mission * );  // Expand Junk Shop
    static void ranch_bartender_1( mission * );  // Expand Bar
    static void ranch_bartender_2( mission * );  // Expand Bar
    static void ranch_bartender_3( mission * );  // Expand Bar
    static void ranch_bartender_4( mission * );  // Expand Bar
    static void place_book( mission * );         // Place a book to retrieve
    static void reveal_refugee_center( mission * ); // Find refugee center
    static void create_lab_console( mission * );  // Reveal lab with an unlocked workstation
    static void create_hidden_lab_console( mission * );  // Reveal hidden lab with workstation
    static void create_ice_lab_console( mission * );  // Reveal lab with an unlocked workstation
    static void reveal_lab_train_depot( mission * );  // Find lab train depot
};

struct mission_end { // These functions are run when a mission ends
    static void standard( mission * ) {}     // Nothing special happens
    static void leave( mission * );          // NPC leaves after the mission is complete
    static void thankful( mission * );       // NPC defaults to being a friendly stranger
    static void deposit_box( mission * );    // random valuable reward
    static void heal_infection( mission * );
    static void evac_construct_5( mission * ); // place can food in evac storage room
};

struct mission_fail {
    static void standard( mission * ) {} // Nothing special happens
    static void kill_npc( mission * );   // Kill the NPC who assigned it!
};

struct mission_util {
    static tripoint target_om_ter( const std::string &omter, int reveal_rad, mission *miss,
                                   bool must_see, int target_z = 0 );
    static tripoint target_om_ter_random( const std::string &omter, int reveal_rad, mission *miss,
                                          bool must_see, int range, tripoint loc = overmap::invalid_tripoint );

};


struct mission_type {
    mission_type_id id = mission_type_id( "MISSION_NULL" ); // Matches it to a mission_type_id above
    bool was_loaded = false;
    std::string name = "Bugged mission type"; // The name the mission is given in menus
    mission_goal goal; // The basic goal type
    int difficulty = 0; // Difficulty; TODO: come up with a scale
    int value = 0; // Value; determines rewards and such
    time_duration deadline_low = 0_turns; // Low and high deadlines
    time_duration deadline_high = 0_turns;
    bool urgent = false; // If true, the NPC will press this mission!

    std::vector<mission_origin> origins; // Points of origin
    itype_id item_id = "null";
    Group_tag group_id = "null";
    itype_id container_id = "null";
    bool remove_container = false;
    itype_id empty_container = "null";
    int item_count = 1;
    npc_class_id recruit_class = npc_class_id( "NC_NONE" );  // The type of NPC you are to recruit
    int target_npc_id = -1;
    mtype_id monster_type = mtype_id::NULL_ID();
    species_id monster_species;
    int monster_kill_goal = -1;
    string_id<oter_type_t> target_id;
    mission_type_id follow_up = mission_type_id( "MISSION_NULL" );

    std::function<bool( const tripoint & )> place = mission_place::always;
    std::function<void( mission * )> start = mission_start::standard;
    std::function<void( mission * )> end = mission_end::standard;
    std::function<void( mission * )> fail = mission_fail::standard;

    std::map<std::string, std::string> dialogue;

    mission_type() = default;
    mission_type( mission_type_id ID, const std::string &NAME, mission_goal GOAL, int DIF, int VAL,
                  bool URGENT,
                  std::function<bool( const tripoint & )> PLACE,
                  std::function<void( mission * )> START,
                  std::function<void( mission * )> END,
                  std::function<void( mission * )> FAIL );

    mission create( int npc_id ) const;

    /**
     * Get the mission_type object of the given id. Returns null if the input is invalid!
     */
    static const mission_type *get( const mission_type_id &id );
    /**
     * Converts the legacy int id to a string_id.
     */
    static mission_type_id from_legacy( int old_id );
    /**
     * Returns a random id of a mission type that can be started at the defined origin
     * around tripoint p, see @ref mission_start.
     * Returns @ref MISSION_NULL if no suitable type could be found.
     */
    static mission_type_id get_random_id( mission_origin origin, const tripoint &p );
    /**
     * Get all mission types at once.
     */
    static const std::vector<mission_type> &get_all();

    static void reset();
    static void load_mission_type( JsonObject &jo, const std::string &src );

    static void check_consistency();

    void parse_start( JsonObject &jo );
    void load( JsonObject &jo, const std::string &src );
};

class mission
{
    public:
        enum class mission_status {
            yet_to_start,
            in_progress,
            success,
            failure
        };
    private:
        friend struct mission_type; // so mission_type::create is simpler
        friend struct mission_start; // so it can initialize some properties
        friend class debug_menu::mission_debug;

        const mission_type *type;
        std::string description;// Basic descriptive text
        mission_status status;
        unsigned long value;    // Cash/Favor value of completing this
        npc_favor reward;       // If there's a special reward for completing it
        int uid;                // Unique ID number, used for referencing elsewhere
        // Marked on the player's map. (INT_MIN, INT_MIN) for none,
        // global overmap terrain coordinates.
        tripoint target;
        itype_id item_id;       // Item that needs to be found (or whatever)
        int item_count;         // The number of above items needed
        string_id<oter_type_t> target_id;      // Destination type to be reached
        npc_class_id recruit_class;// The type of NPC you are to recruit
        int target_npc_id;     // The ID of a specific NPC to interact with
        mtype_id monster_type;    // Monster ID that are to be killed
        species_id monster_species;  // Monster species that are to be killed
        int monster_kill_goal;  // The number of monsters you need to kill
        int kill_count_to_reach; // The kill count you need to reach to complete mission
        time_point deadline;
        int npc_id;             // ID of a related npc
        int good_fac_id, bad_fac_id; // IDs of the protagonist/antagonist factions
        int step;               // How much have we completed?
        mission_type_id follow_up;   // What mission do we get after this succeeds?
        int player_id; // The id of the player that has accepted this mission.
    public:

        std::string name();
        mission_type_id mission_id();
        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        mission();
        /** Getters, they mostly return the member directly, mostly. */
        /*@{*/
        bool has_deadline() const;
        time_point get_deadline() const;
        std::string get_description() const;
        bool has_target() const;
        const tripoint &get_target() const;
        const mission_type &get_type() const;
        bool has_follow_up() const;
        mission_type_id get_follow_up() const;
        long get_value() const;
        int get_id() const;
        const std::string &get_item_id() const;
        int get_npc_id() const;
        /**
         * Whether the mission is assigned to a player character. If not, the mission is free and
         * can be assigned.
         */
        bool is_assigned() const;
        /**
         * To which player the mission is assigned. It returns the id (@ref player::getID) of the
         * player.
         */
        int get_assigned_player_id() const;
        /*@}*/

        /**
         * Simple setters, no checking if the values is performed. */
        /*@{*/
        void set_target( const tripoint &p );
        /*@}*/

        /** Assigns the mission to the player. */
        void assign( player &u );

        /** Called when the mission has failed, calls the mission fail callback. */
        void fail();
        /** Handles mission completion tasks (remove given item, etc.). */
        void wrap_up();
        /** Handles partial mission completion (kill complete, now report back!). */
        void step_complete( int step );
        /** Checks if the player has completed the matching mission and returns true if they have. */
        bool is_complete( int npc_id ) const;
        /** Checks if the player has failed the matching mission and returns true if they have. */
        bool has_failed() const;
        /** Checks if the mission is started, but not failed and not succeeded. */
        bool in_progress() const;
        /** Processes this mission. */
        void process();

        // TODO: Give topics a string_id
        std::string dialogue_for_topic( const std::string &topic ) const;

        /**
         * Create a new mission of the given type and assign it to the given npc.
         * Returns the new mission.
         */
        static mission *reserve_new( const mission_type_id &type, int npc_id );
        static mission *reserve_random( mission_origin origin, const tripoint &p, int npc_id );
        /**
         * Returns the mission with the matching id (@ref uid). Returns NULL if no mission with that
         * id exists.
         */
        static mission *find( int id );
        /**
         * Remove all active missions, used to cleanup on exit and before reloading a new game.
         */
        static void clear_all();
        /**
         * Handles mission deadline processing.
         */
        static void process_all();

        /**
         * various callbacks from events that may affect all missions
         */
        /*@{*/
        static void on_creature_death( Creature &poor_dead_dude );
        /*@}*/

        // Serializes and unserializes all missions
        static void serialize_all( JsonOut &json );
        static void unserialize_all( JsonIn &jsin );
        /** Converts a vector mission ids to a vector of mission pointers. Invalid ids are skipped! */
        static std::vector<mission *> to_ptr_vector( const std::vector<int> &vec );
        static std::vector<int> to_uid_vector( const std::vector<mission *> &vec );

        // For save/load
        static std::vector<mission *> get_all_active();
        static void add_existing( const mission &m );

        static mission_status status_from_string( const std::string &s );
        static const std::string status_to_string( mission_status st );

        /** Used to handle saves from before player_id was a member of mission */
        void set_player_id_legacy_0c( int id );

    private:
        bool legacy_no_player_id = false;
        // Don't use this, it's only for loading legacy saves.
        void load_info( std::istream &info );

        void set_target_to_mission_giver();

        static void get_all_item_group_matches(
            std::vector<item *> &items,
            Group_tag &grp_type,
            std::map<itype_id, int> &matches,
            const itype_id &required_container,
            const itype_id &actual_container,
            bool &specific_container_required );

};

#endif
