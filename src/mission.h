#ifndef MISSION_H
#define MISSION_H

#include <vector>
#include <string>
#include "omdata.h"
#include "itype.h"
#include "npc.h"
#include "json.h"

class mission;
class game;
enum talk_topic;

enum mission_id {
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
    MISSION_FREE_MERCHANTS_EVAC_3,         //Acquire Plutonium Cells
    MISSION_OLD_GUARD_REP_1,               //Bandit Pair
    MISSION_OLD_GUARD_REP_2,               //Raider Informant
    MISSION_OLD_GUARD_REP_3,               //Missing without a trace
    MISSION_OLD_GUARD_REP_4,               //Raider Camp
    NUM_MISSION_IDS
};

std::string mission_dialogue(mission_id id, talk_topic state);

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

    NUM_MGOAL
};

struct mission_place { // Return true if [posx,posy] is valid in overmap
    bool never     (int, int)
    {
        return false;
    }
    bool always    (int, int)
    {
        return true;
    }
    bool near_town (int posx, int posy);
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
    void infect_npc         ( mission *); // DI_INFECTION, remove antibiotics
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
    int id; // Matches it to a mission_id above
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
    mission_id follow_up;

    bool (mission_place::*place)(int x, int y);
    void (mission_start::*start)(mission *);
    void (mission_end  ::*end  )(mission *);
    void (mission_fail ::*fail )(mission *);

    mission_type(int ID, std::string NAME, mission_goal GOAL, int DIF, int VAL,
                 bool URGENT,
                 bool (mission_place::*PLACE)(int x, int y),
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

    mission create(int npc_id = -1); // Create a mission
};

class mission : public JsonSerializer, public JsonDeserializer
{
    public:
        mission_type *type;
        std::string description;// Basic descriptive text
        bool failed;            // True if we've failed it!
        unsigned long value;    // Cash/Favor value of completing this
        npc_favor reward;       // If there's a special reward for completing it
        int uid;                // Unique ID number, used for referencing elsewhere
        // Marked on the player's map. (INT_MIN, INT_MIN) for none,
        // global overmap terrain coordinates.
        point target;
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
        mission_id follow_up;   // What mission do we get after this succeeds?

        std::string name();
        void load_info(std::ifstream &info);
        using JsonSerializer::serialize;
        void serialize(JsonOut &jsout) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin);

        mission()
        {
            type = NULL;
            description = "";
            failed = false;
            value = 0;
            uid = -1;
            target = point(INT_MIN, INT_MIN);
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
        }
};

#endif
