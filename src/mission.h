#ifndef MISSION_H
#define MISSION_H

#include <vector>
#include <string>
#include <unordered_map>
#include "omdata.h"
#include "itype.h"
#include "npc.h"
#include "json.h"

class mission;
class game;

enum mission_type_id {
    MISSION_NULL,
    MISSION_GET_ANTIBIOTICS,
    MISSION_GET_SOFTWARE,
    MISSION_GET_ZOMBIE_BLOOD_ANAL,
    MISSION_RESCUE_DOG,
    MISSION_KILL_ZOMBIE_MOM,
    MISSION_REACH_SAFETY,
    MISSION_GET_FLAG,                      //patriot 1
    MISSION_GET_BLACK_BOX,                 //patriot 2
    MISSION_GET_BLACK_BOX_TRANSCRIPT,      //patriot 3
    MISSION_EXPLORE_SARCOPHAGUS,           //patriot 4
    MISSION_GET_RELIC,                     //martyr 1
    MISSION_RECOVER_PRIEST_DIARY,          //martyr 2
    MISSION_INVESTIGATE_CULT,              //martyr 3
    MISSION_INVESTIGATE_PRISON_VISIONARY,  //martyr 4
    MISSION_GET_RECORD_WEATHER,            //scientist 1
    MISSION_GET_RECORD_PATIENT,            //humanitarian 1
    MISSION_REACH_FEMA_CAMP,               //humanitarian 2
    MISSION_REACH_FARM_HOUSE,              //humanitarian 3
    MISSION_GET_RECORD_ACCOUNTING,         //vigilante 1
    MISSION_GET_SAFE_BOX,                  //vigilante 2
    MISSION_GET_DEPUTY_BADGE,              //vigilante 3
    MISSION_KILL_JABBERWOCK,               //demon slayer 1
    MISSION_KILL_100_Z,                    //demon slayer 2
    MISSION_KILL_HORDE_MASTER,             //demon slayer 3
    MISSION_RECRUIT_TRACKER,               //demon slayer 4
    MISSION_JOIN_TRACKER,                  //demon slayer 4b
    MISSION_FREE_MERCHANTS_EVAC_1,         //Clear Back Bay
    MISSION_FREE_MERCHANTS_EVAC_2,         //Kill Raiders
    MISSION_FREE_MERCHANTS_EVAC_4,         //Acquire Plutonium Cells
    MISSION_OLD_GUARD_REP_1,               //Bandit Pair
    MISSION_OLD_GUARD_REP_2,               //Raider Informant
    MISSION_OLD_GUARD_REP_3,               //Missing without a trace
    MISSION_OLD_GUARD_REP_4,               //Raider Camp
    MISSION_OLD_GUARD_NEC_1,               //Locate Commo team for Necropolis Commander
    MISSION_OLD_GUARD_NEC_2,               //Cull Nightmares
    MISSION_OLD_GUARD_NEC_COMMO_1,         //Build a radio repeater mod
    MISSION_OLD_GUARD_NEC_COMMO_2,         //Disable external power connection
    MISSION_OLD_GUARD_NEC_COMMO_3,         //Install repeater mod in local radio station
    MISSION_OLD_GUARD_NEC_COMMO_4,         //Cyclical mission to install repeater mods
    MISSION_RANCH_FOREMAN_1,               //Rebuild civilization one 2x4 at a time
    MISSION_RANCH_FOREMAN_2,               //Beds need blankets to make
    MISSION_RANCH_FOREMAN_3,               //You can never have enough nails!
    MISSION_RANCH_FOREMAN_4,               //Need salt to trade for seed
    MISSION_RANCH_FOREMAN_5,               //Need liquid fertilizer
    MISSION_RANCH_FOREMAN_6,               //Need stone for well and fireplaces
    MISSION_RANCH_FOREMAN_7,               //Need pipes to finish well and parts for lumberyard
    MISSION_RANCH_FOREMAN_8,               //Need motors to finish sawmill
    MISSION_RANCH_FOREMAN_9,               //Need bleach to sterilize for clinic
    MISSION_RANCH_FOREMAN_10,              //Need first aid kits for clinic
    MISSION_RANCH_FOREMAN_11,              //Need welders for chop-shop
    MISSION_RANCH_FOREMAN_12,              //Need car batteries to power equipment
    MISSION_RANCH_FOREMAN_13,              //Need pair of two-way radios for scavengers
    MISSION_RANCH_FOREMAN_14,              //Need 5 backpacks for scavengers
    MISSION_RANCH_FOREMAN_15,              //Need Homebrewer's Bible for Bar
    MISSION_RANCH_FOREMAN_16,              //Need Sugar for Bar
    MISSION_RANCH_FOREMAN_17,              //Need glass sheets for 1st green house
    MISSION_RANCH_NURSE_1,                 //Need asprin
    MISSION_RANCH_NURSE_2,                 //Need hotplates
    MISSION_RANCH_NURSE_3,                 //Need vitamins
    MISSION_RANCH_NURSE_4,                 //Need charcoal water filters
    MISSION_RANCH_NURSE_5,                 //Need chemistry set
    MISSION_RANCH_NURSE_6,                 //Need filter masks
    MISSION_RANCH_NURSE_7,                 //Need rubber gloves
    MISSION_RANCH_NURSE_8,                 //Need X-acto
    MISSION_RANCH_NURSE_9,                 //Need Guide to Advanced Emergency Care
    MISSION_RANCH_NURSE_10,                //Need flu shot
    MISSION_RANCH_NURSE_11,                //Need empty syringes
    MISSION_RANCH_SCAVENGER_1,             //Need knife spears
    MISSION_RANCH_SCAVENGER_2,             //Need wearable flashlights
    MISSION_RANCH_SCAVENGER_3,             //Need leather body armor
    MISSION_RANCH_SCAVENGER_4,             //Need Molotov cocktails
    MISSION_RANCH_BARTENDER_1,             //Need Stills
    MISSION_RANCH_BARTENDER_2,             //Need Yeast
    MISSION_RANCH_BARTENDER_3,             //Need Sugar Beet Seeds
    MISSION_RANCH_BARTENDER_4,             //Need Metal Tanks
    MISSION_RANCH_BARTENDER_5,             //Need 55-Gallon Drums
    MISSION_FREE_MERCHANTS_EVAC_3,         //Info from Commune
    NUM_MISSION_IDS
};

std::string mission_dialogue(mission_type_id id, const std::string &state);

enum mission_origin {
    ORIGIN_NULL = 0,
    ORIGIN_GAME_START, // Given when the game starts
    ORIGIN_OPENER_NPC, // NPC comes up to you when the game starts
    ORIGIN_ANY_NPC,    // Any NPC
    ORIGIN_SECONDARY,  // Given at the end of another mission
    NUM_ORIGIN
};

enum mission_goal {
    MGOAL_NULL = 0,
    MGOAL_GO_TO,             // Reach a certain overmap tile
    MGOAL_GO_TO_TYPE,        // Instead of a point, go to an oter_id map tile like "hospital_entrance"
    MGOAL_FIND_ITEM,         // Find an item of a given type
    MGOAL_FIND_ANY_ITEM,     // Find an item tagged with this mission
    MGOAL_FIND_MONSTER,      // Find and retrieve a friendly monster
    MGOAL_FIND_NPC,          // Find a given NPC
    MGOAL_ASSASSINATE,       // Kill a given NPC
    MGOAL_KILL_MONSTER,      // Kill a particular hostile monster
    MGOAL_KILL_MONSTER_TYPE, // Kill a number of a given monster type
    MGOAL_RECRUIT_NPC,       // Recruit a given NPC
    MGOAL_RECRUIT_NPC_CLASS, // Recruit an NPC class
    MGOAL_COMPUTER_TOGGLE,   // Activating the correct terminal will complete the mission
    NUM_MGOAL
};

struct mission_place {
    // Return true if the place (global overmap terrain coordinate) is valid for starting a mission
    bool never( const tripoint& )
    {
        return false;
    }
    bool always( const tripoint& )
    {
        return true;
    }
    bool near_town( const tripoint& );
};

/* mission_start functions are first run when a mission is accepted; this
 * initializes the mission's key values, like the target and description.
 * These functions are also run once a turn for each active mission, to check
 * if the current goal has been reached.  At that point they either start the
 * goal, or run the appropriate mission_end function.
 */
struct mission_start {
    void standard           ( mission *); // Standard for its goal type
    void join               ( mission *); // NPC giving mission joins your party
    void infect_npc         ( mission *); // "infection", remove antibiotics
    void place_dog          ( mission *); // Put a dog in a house!
    void place_zombie_mom   ( mission *); // Put a zombie mom in a house!
    void place_zombie_bay   ( mission *); // Put a boss zombie in the refugee/evac center back bay
    void place_caravan_ambush ( mission *); // For Free Merchants mission
    void place_bandit_cabin ( mission *); // For Old Guard mission
    void place_informant    ( mission *); // For Old Guard mission
    void place_grabber      ( mission *); // For Old Guard mission
    void place_bandit_camp  ( mission *); // For Old Guard mission
    void place_jabberwock   ( mission *); // Put a jabberwok in the woods nearby
    void kill_100_z         ( mission *); // Kill 100 more regular zombies
    void kill_20_nightmares ( mission *); // Kill 20 more regular nightmares
    void kill_horde_master  ( mission *); // Kill the master zombie at the center of the horde
    void place_npc_software ( mission *); // Put NPC-type-dependent software
    void place_priest_diary ( mission *); // Hides the priest's diary in a local house
    void place_deposit_box  ( mission *); // Place a safe deposit box in a nearby bank
    void reveal_lab_black_box ( mission *); // Reveal the nearest lab and give black box
    void open_sarcophagus   ( mission *); // Reveal the sarcophagus and give access code
    void reveal_hospital    ( mission *); // Reveal the nearest hospital
    void find_safety        ( mission *); // Goal is set to non-spawn area
    void point_prison       ( mission *); // Point to prison entrance
    void point_cabin_strange ( mission *); // Point to strange cabin location
    void recruit_tracker    ( mission *); // Recruit a tracker to help you
    void radio_repeater     ( mission *); // Gives you the plans for the radio repeater mod
    void start_commune      ( mission *); // Focus on starting the ranch commune
    void ranch_construct_1  ( mission *); // Encloses barn
    void ranch_construct_2  ( mission *); // Adds makeshift beds to the barn, 1 NPC
    void ranch_construct_3  ( mission *); // Adds a couple of NPCs and fields
    void ranch_construct_4  ( mission *); // Begins work on wood yard, crop overseer added
    void ranch_construct_5  ( mission *); // Continues work on wood yard, crops, well (pit)
    void ranch_construct_6  ( mission *); // Continues work on wood yard, well (covered), fireplaces
    void ranch_construct_7  ( mission *); // Continues work on wood yard, well (finished), continues walling
    void ranch_construct_8  ( mission *); // Finishes wood yard, starts outhouse, starts toolshed
    void ranch_construct_9  ( mission *); // Finishes outhouse, finishes toolshed, starts clinic
    void ranch_construct_10 ( mission *); // Continues clinic, starts chop-shop
    void ranch_construct_11 ( mission *); // Continues clinic, continues chop-shop
    void ranch_construct_12 ( mission *); // Finish chop-shop, starts junk shop
    void ranch_construct_13 ( mission *); // Continues junk shop
    void ranch_construct_14 ( mission *); // Finish junk shop, starts bar
    void ranch_construct_15 ( mission *); // Continues bar
    void ranch_construct_16 ( mission *); // Finish bar, start green shouse
    void ranch_nurse_1      ( mission *); // Need asprin
    void ranch_nurse_2      ( mission *); // Need hotplates
    void ranch_nurse_3      ( mission *); // Need vitamins
    void ranch_nurse_4      ( mission *); // Need charcoal water filters
    void ranch_nurse_5      ( mission *); // Need chemistry set
    void ranch_nurse_6      ( mission *); // Need filter masks
    void ranch_nurse_7      ( mission *); // Need rubber gloves
    void ranch_nurse_8      ( mission *); // Need X-acto
    void ranch_nurse_9      ( mission *); // Need Guide to Advanced Emergency Care
    void ranch_scavenger_1  ( mission *); // Expand Junk Shop
    void ranch_scavenger_2  ( mission *); // Expand Junk Shop
    void ranch_scavenger_3  ( mission *); // Expand Junk Shop
    void ranch_bartender_1  ( mission *); // Expand Bar
    void ranch_bartender_2  ( mission *); // Expand Bar
    void ranch_bartender_3  ( mission *); // Expand Bar
    void ranch_bartender_4  ( mission *); // Expand Bar
    void place_book         ( mission *); // Place a book to retrieve
};

struct mission_end { // These functions are run when a mission ends
    void standard       ( mission *) {}; // Nothing special happens
    void leave          ( mission *); // NPC leaves after the mission is complete
    void thankful       ( mission *); // NPC defaults to being a friendly stranger
    void deposit_box    ( mission *); // random valuable reward
    void heal_infection ( mission *);
};

struct mission_fail {
    void standard   ( mission *) {}; // Nothing special happens
    void kill_npc   ( mission *); // Kill the NPC who assigned it!
};

struct mission_type {
    mission_type_id id; // Matches it to a mission_type_id above
    std::string name; // The name the mission is given in menus
    mission_goal goal; // The basic goal type
    int difficulty; // Difficulty; TODO: come up with a scale
    int value; // Value; determines rewards and such
    npc_favor special_reward; // If we have a special gift, not cash value
    int deadline_low, deadline_high; // Low and high deadlines (turn numbers)
    bool urgent; // If true, the NPC will press this mission!

    std::vector<mission_origin> origins; // Points of origin
    itype_id item_id;
    int item_count;
    npc_class recruit_class;  // The type of NPC you are to recruit
    int target_npc_id;
    std::string monster_type;
    int monster_kill_goal;
    oter_id target_id;
    mission_type_id follow_up;

    bool (mission_place::*place)( const tripoint& );
    void (mission_start::*start)(mission *);
    void (mission_end  ::*end  )(mission *);
    void (mission_fail ::*fail )(mission *);

    mission_type(mission_type_id ID, std::string NAME, mission_goal GOAL, int DIF, int VAL,
                 bool URGENT,
                 bool (mission_place::*PLACE)( const tripoint& ),
                 void (mission_start::*START)(mission *),
                 void (mission_end  ::*END  )(mission *),
                 void (mission_fail ::*FAIL )(mission *)) :
        id (ID), name (NAME), goal (GOAL), difficulty (DIF), value (VAL),
        urgent(URGENT), place (PLACE), start (START), end (END), fail (FAIL)
    {
        deadline_low = 0;
        deadline_high = 0;
        item_id = "null";
        item_count = 1;
        target_id = 0;///(0);// = "";
        recruit_class = NC_NONE;
        target_npc_id = -1;
        monster_type = "mon_null";
        monster_kill_goal = -1;
        follow_up = MISSION_NULL;
    };

    mission create( int npc_id ) const;

    /**
     * Get the mission_type object of the given id. Returns null if the input is invalid!
     */
    static const mission_type *get( mission_type_id id );
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
    static void initialize();
private:
    /**
     * All the known mission templates.
     */
    static std::vector<mission_type> types;
};

class mission : public JsonSerializer, public JsonDeserializer
{
private:
    friend struct mission_type; // so mission_type::create is simpler
    friend struct mission_start; // so it can initialize some properties
        const mission_type *type;
        std::string description;// Basic descriptive text
        /**
         * True if the mission is failed. Failed missions are completed per definition
         * and should not be reused. Failed mission should not be changed further.
         */
        bool failed;
        unsigned long value;    // Cash/Favor value of completing this
        npc_favor reward;       // If there's a special reward for completing it
        int uid;                // Unique ID number, used for referencing elsewhere
        // Marked on the player's map. (INT_MIN, INT_MIN) for none,
        // global overmap terrain coordinates.
        tripoint target;
        itype_id item_id;       // Item that needs to be found (or whatever)
        int item_count;         // The number of above items needed
        oter_id target_id;      // Destination type to be reached
        npc_class recruit_class;// The type of NPC you are to recruit
        int target_npc_id;     // The ID of a specific NPC to interact with
        std::string monster_type;    // Monster ID that are to be killed
        int monster_kill_goal;  // the kill count you wish to reach
        int deadline;           // Turn number
        int npc_id;             // ID of a related npc
        int good_fac_id, bad_fac_id; // IDs of the protagonist/antagonist factions
        int step;               // How much have we completed?
        mission_type_id follow_up;   // What mission do we get after this succeeds?
        int player_id; // The id of the player that has accepted this mission.
        bool was_started; // whether @ref mission_type::start had been called
public:

        std::string name();
        using JsonSerializer::serialize;
        void serialize(JsonOut &jsout) const override;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) override;

        mission()
        {
            type = NULL;
            description = "";
            failed = false;
            value = 0;
            uid = -1;
            target = tripoint(INT_MIN, INT_MIN, INT_MIN);
            item_id = "null";
            item_count = 1;
            target_id = 0;
            recruit_class = NC_NONE;
            target_npc_id = -1;
            monster_type = "mon_null";
            monster_kill_goal = -1;
            deadline = 0;
            npc_id = -1;
            good_fac_id = -1;
            bad_fac_id = -1;
            step = 0;
            player_id = -1;
            was_started = false;
        }

    /** Getters, they mostly return the member directly, mostly. */
    /*@{*/
    bool has_deadline() const;
    calendar get_deadline() const;
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
    void set_target( const tripoint &target );
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

    /**
     * Create a new mission of the given type and assign it to the given npc.
     * Returns the new mission.
     */
    static mission* reserve_new( mission_type_id type, int npc_id );
    static mission* reserve_random( mission_origin origin, const tripoint &p, int npc_id );
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

    // Don't use this, it's only for loading legacy saves.
    static void unserialize_legacy( std::istream &fin );
    // Serializes and unserializes all missions in @ref active_missions
    static void serialize_all( JsonOut &json );
    static void unserialize_all( JsonIn &jsin );
    /** Converts a vector mission ids to a vector of mission pointers. Invalid ids are skipped! */
    static std::vector<mission*> to_ptr_vector( const std::vector<int> &vec );
    static std::vector<int> to_uid_vector( const std::vector<mission*> &vec );

private:
    /**
     * Missions which have been created, they might have been assigned or can be assigned or
     * are already done. They are stored with the main save.
     * Key is the mission id (@ref uid).
     */
    static std::unordered_map<int, mission> active_missions;

    // Don't use this, it's only for loading legacy saves.
    void load_info(std::istream &info);

    void set_target_to_mission_giver();
};

#endif
