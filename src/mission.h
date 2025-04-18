#pragma once
#ifndef CATA_SRC_MISSION_H
#define CATA_SRC_MISSION_H

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "coordinates.h"
#include "dialogue_helpers.h"
#include "enums.h"
#include "npc_favor.h"
#include "point.h"
#include "translation.h"
#include "type_id.h"

class Creature;
class JsonArray;
class JsonObject;
class JsonOut;
class avatar;
class item;
class mission;
class npc;
class overmapbuffer;
struct const_dialogue;
struct dialogue;
struct oter_type_t;
template<typename T> struct enum_traits;

namespace debug_menu
{
class mission_debug;
} // namespace debug_menu

enum mission_origin {
    ORIGIN_NULL = 0,
    ORIGIN_GAME_START, // Given when the game starts
    ORIGIN_OPENER_NPC, // NPC comes up to you when the game starts
    ORIGIN_ANY_NPC,    // Any NPC
    ORIGIN_SECONDARY,  // Given at the end of another mission
    ORIGIN_COMPUTER,   // Taken after reading investigation provoking entries in computer terminal
    NUM_ORIGIN
};

template<>
struct enum_traits<mission_origin> {
    static constexpr mission_origin last = mission_origin::NUM_ORIGIN;
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
    MGOAL_KILL_MONSTERS,     // Kill a number of particular hostile monsters
    MGOAL_KILL_MONSTER_TYPE, // Kill a number of a given monster type
    MGOAL_KILL_NEMESIS,      // Kill the nemesis monster from the "hunted" trait
    MGOAL_RECRUIT_NPC,       // Recruit a given NPC
    MGOAL_RECRUIT_NPC_CLASS, // Recruit an NPC class
    MGOAL_COMPUTER_TOGGLE,   // Activating the correct terminal will complete the mission
    MGOAL_KILL_MONSTER_SPEC, // Kill a number of monsters from a given species
    MGOAL_TALK_TO_NPC,       // Talk to a given NPC
    MGOAL_CONDITION,         // Satisfy the dynamically created condition and talk to the mission giver
    NUM_MGOAL
};

template<>
struct enum_traits<mission_goal> {
    static constexpr mission_goal last = mission_goal::NUM_MGOAL;
};

struct mission_place {
    // Return true if the place (global overmap terrain coordinate) is valid for starting a mission
    static bool never( const tripoint_abs_omt & ) {
        return false;
    }
    static bool always( const tripoint_abs_omt & ) {
        return true;
    }
    static bool near_town( const tripoint_abs_omt & );
};

/* mission_start functions are first run when a mission is accepted; this
 * initializes the mission's key values, like the target and description.
 */
struct mission_start {
    static void standard( mission * );           // Standard for its goal type
    static void kill_nemesis( mission * );       // Kill the nemesis spawned with the "hunted" trait
    static void place_npc_software( mission * ); // Put NPC-type-dependent software
    static void place_deposit_box( mission * );  // Place a safe deposit box in a nearby bank
    static void find_safety( mission * );        // Goal is set to non-spawn area
    static void place_book( mission * );         // Place a book to retrieve
    static void reveal_refugee_center( mission * ); // Find refugee center
    static void create_lab_console( mission * );  // Reveal lab with an unlocked workstation
    static void create_hidden_lab_console( mission * );  // Reveal hidden lab with workstation
    static void create_ice_lab_console( mission * );  // Reveal lab with an unlocked workstation
};

// These functions are run when a mission ends
struct mission_end {
    // Nothing special happens
    static void standard( mission * ) {}
    // random valuable reward
};

struct mission_fail {
    // Nothing special happens
    static void standard( mission * ) {}
};

struct mission_target_params {
    str_or_var overmap_terrain;
    ot_match_type overmap_terrain_match_type = ot_match_type::type;
    mission *mission_pointer = nullptr;

    bool origin_u = true;
    std::optional<tripoint_rel_omt> offset;
    std::optional<str_or_var> replaceable_overmap_terrain;
    std::optional<str_or_var> overmap_special;
    std::optional<dbl_or_var> reveal_radius;
    std::optional<var_info> target_var;
    dbl_or_var min_distance;

    bool must_see = false;
    bool cant_see = false;
    bool random = false;
    bool create_if_necessary = true;
    dbl_or_var search_range;
    std::optional<dbl_or_var> z;
    npc *guy = nullptr;
};

namespace mission_util
{
tripoint_abs_omt random_house_in_closest_city();
tripoint_abs_omt target_closest_lab_entrance( const tripoint_abs_omt &origin, int reveal_rad,
        mission *miss );
bool reveal_road( const tripoint_abs_omt &source, const tripoint_abs_omt &dest,
                  overmapbuffer &omb );
tripoint_abs_omt reveal_om_ter( const std::string &omter, int reveal_rad, bool must_see,
                                int target_z = 0 );
tripoint_abs_omt target_om_ter( const std::string &omter, int reveal_rad, mission *miss,
                                bool must_see, int target_z = 0 );
tripoint_abs_omt target_om_ter_random(
    const std::string &omter, int reveal_rad, mission *miss, bool must_see, int range,
    tripoint_abs_omt loc = tripoint_abs_omt::invalid );
void set_reveal( const std::string &terrain,
                 std::vector<std::function<void( mission *miss )>> &funcs );
void set_reveal_any( const JsonArray &ja,
                     std::vector<std::function<void( mission *miss )>> &funcs );
mission_target_params parse_mission_om_target( const JsonObject &jo );
std::optional<tripoint_abs_omt> assign_mission_target( const mission_target_params
        &params, dialogue &d );
tripoint_abs_omt get_om_terrain_pos( const mission_target_params &params, dialogue &d );
void set_assign_om_target( const JsonObject &jo,
                           std::vector<std::function<void( mission *miss )>> &funcs );
bool set_update_mapgen( const JsonObject &jo,
                        std::vector<std::function<void( mission *miss )>> &funcs );
bool load_funcs( const JsonObject &jo, std::vector<std::function<void( mission *miss )>> &funcs );
} // namespace mission_util

struct mission_type {
    public:
        // Matches it to a mission_type_id above
        mission_type_id id = mission_type_id( "MISSION_NULL" );
        std::vector<std::pair<mission_type_id, mod_id>> src;
    private:
        // The untranslated name of the mission
        translation name = to_translation( "Bugged mission type" );
    public:
        translation description;
        // The basic goal type
        mission_goal goal = mission_goal::MGOAL_NULL;
        // Difficulty; TODO: come up with a scale
        int difficulty = 0;
        // Value; determines rewards and such
        int value = 0;
        // When this mission will auto-fail, if ever. Can be pair of values or just one
        // If loaded as a pair, automatically calls rng(min, max) when evaluated, standard stuff
        duration_or_var deadline;
        // If true, the NPC will press this mission!
        bool urgent = false;
        // If the mission has generic rewards, so that the completion dialogue knows whether to offer them.
        bool has_generic_rewards = true;

        bool was_loaded = false;

        // A limited subset of the talk_effects on the mission
        talk_effect_fun_t::likely_rewards_t likely_rewards;

        // Points of origin
        std::vector<mission_origin> origins;
        itype_id item_id = itype_id::NULL_ID();
        item_group_id group_id = item_group_id::NULL_ID();
        itype_id container_id = itype_id::NULL_ID();
        bool remove_container = false;
        bool invisible_on_complete = false;
        itype_id empty_container = itype_id::NULL_ID();
        int item_count = 1;
        npc_class_id recruit_class = npc_class_id::NULL_ID();  // The type of NPC you are to recruit
        character_id target_npc_id;
        mtype_id monster_type = mtype_id::NULL_ID();
        species_id monster_species;
        int monster_kill_goal = -1;
        string_id<oter_type_t> target_id;
        mission_type_id follow_up = mission_type_id( "MISSION_NULL" );

        std::function<bool( const tripoint_abs_omt & )> place = mission_place::always;
        std::function<void( mission * )> start = mission_start::standard;
        std::function<void( mission * )> end = mission_end::standard;
        std::function<void( mission * )> fail = mission_fail::standard;

        std::map<std::string, translation> dialogue;

        // A dynamic goal condition invoked by MGOAL_CONDITION.
        std::function<bool( const_dialogue const & )> goal_condition;

        mission_type() = default;

        mission create( const character_id &npc_id ) const;

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
        static mission_type_id get_random_id( mission_origin origin, const tripoint_abs_omt &p );
        /**
         * Get all mission types at once.
         */
        static const std::vector<mission_type> &get_all();

        bool test_goal_condition( struct dialogue &d ) const;

        static void reset();
        static void load_mission_type( const JsonObject &jo, const std::string &src );
        static void finalize();
        static void check_consistency();

        bool parse_funcs( const JsonObject &jo, std::string_view src,
                          std::function<void( mission * )> &phase_func );
        bool load( const JsonObject &jo, const std::string &src );

        /**
         * Returns the translated name
         */
        std::string tname() const;
};

class mission
{
    public:
        enum class mission_status : int {
            yet_to_start,
            in_progress,
            success,
            failure,
            num_mission_status
        };
    private:
        // So mission_type::create is simpler
        friend struct mission_type;
        // So it can initialize some properties
        friend struct mission_start;
        friend class debug_menu::mission_debug;

        const mission_type *type;
        mission_status status;
        // Cash/Favor value of completing this
        unsigned int value;
        // If there's a special reward for completing it
        npc_favor reward;
        // Unique ID number, used for referencing elsewhere
        int uid;
        // Marked on the player's map. (INT_MIN, INT_MIN) for none,
        // global overmap terrain coordinates.
        tripoint_abs_omt target;
        // Item that needs to be found (or whatever)
        itype_id item_id;
        // The number of above items needed
        int item_count = 0;
        // Destination type to be reached
        string_id<oter_type_t> target_id;
        // The type of NPC you are to recruit
        npc_class_id recruit_class;
        // The ID of a specific NPC to interact with
        character_id target_npc_id;
        // Monster ID that are to be killed
        mtype_id monster_type;
        // Monster species that are to be killed
        species_id monster_species;
        // The number of monsters you need to kill
        int monster_kill_goal = 0;
        // The kill count you need to reach to complete mission
        int kill_count_to_reach = 0;
        // When this mission will auto-fail if not already completed/failed
        // Also overloaded to track when the mission was completed/loaded
        time_point deadline;
        // ID of a related npc
        character_id npc_id;
        // IDs of the protagonist/antagonist factions
        int good_fac_id = 0;
        int bad_fac_id = 0;
        // How much have we completed?
        int step = 0;
        // What mission do we get after this succeeds?
        mission_type_id follow_up;
        // The id of the player that has accepted this mission.
        character_id player_id;
    public:

        std::string name() const;
        mission_type_id mission_id() const;
        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );

        mission();
        /** Getters, they mostly return the member directly, mostly. */
        /*@{*/
        bool has_deadline() const;
        time_point get_deadline() const;
        std::string get_description() const;
        bool has_target() const;
        const tripoint_abs_omt &get_target() const;
        const mission_type &get_type() const;
        bool has_follow_up() const;
        mission_type_id get_follow_up() const;
        int get_value() const;
        int get_id() const;
        const itype_id &get_item_id() const;
        character_id get_npc_id() const;
        const talk_effect_fun_t::likely_rewards_t &get_likely_rewards() const;
        bool has_generic_rewards() const;
        void register_kill_needed() {
            monster_kill_goal++;
        };
        /**
         * Whether the mission is assigned to a player character. If not, the mission is free and
         * can be assigned.
         */
        bool is_assigned() const;
        /**
         * To which player the mission is assigned. It returns the id (@ref player::getID) of the
         * player.
         */
        character_id get_assigned_player_id() const;
        /*@}*/

        /**
         * Simple setters, no checking if the values is performed. */
        /*@{*/
        void set_deadline( time_point new_deadline );
        void set_target( const tripoint_abs_omt &p );
        void set_target_npc_id( const character_id &npc_id );
        void set_assigned_player_id( const character_id &char_id );
        void update_world_missions_character( const character_id &old_char_id,
                                              const character_id &new_char_id );
        /*@}*/

        /** Assigns the mission to the player. */
        void assign( avatar &u );

        /** Called when the mission has failed, calls the mission fail callback. */
        void fail();
        /** Handles mission completion tasks (remove given item, etc.). */
        void wrap_up();
        /** Handles partial mission completion (kill complete, now report back!). */
        void step_complete( int step );
        /** Checks if the player has completed the matching mission and returns true if they have. */
        bool is_complete( const character_id &npc_id ) const;
        /** Checks if the player has failed the matching mission and returns true if they have. */
        bool has_failed() const;
        /** Checks if the mission is started, but not failed and not succeeded. */
        bool in_progress() const;
        /** Processes this mission. */
        void process();
        /** Called when the player talks with an NPC. May resolve mission goals, e.g. MGOAL_TALK_TO_NPC. */
        void on_talk_with_npc( const character_id &npc_id );

        // TODO: Give topics a string_id
        std::string dialogue_for_topic( const std::string &topic ) const;

        /**
         * Create a new mission of the given type and assign it to the given npc.
         * Returns the new mission.
         */
        static mission *reserve_new( const mission_type_id &type, const character_id &npc_id );
        static mission *reserve_random( mission_origin origin, const tripoint_abs_omt &p,
                                        const character_id &npc_id );
        /**
         * Returns the mission with the matching id (@ref uid). Returns NULL if no mission with that
         * id exists.
         */
        static mission *find( int id, bool ok_missing = false );
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
        // returns: whether any mission is transferred to fuser
        static bool on_creature_fusion( Creature &fuser, Creature &fused );
        /*@}*/

        // Serializes and unserializes all missions
        static void serialize_all( JsonOut &json );
        static void unserialize_all( const JsonArray &ja );
        /** Converts a vector mission ids to a vector of mission pointers. Invalid ids are skipped! */
        static std::vector<mission *> to_ptr_vector( const std::vector<int> &vec, bool ok_missing = false );
        static std::vector<int> to_uid_vector( const std::vector<mission *> &vec );

        // For save/load
        static std::vector<mission *> get_all_active();
        void remove_active_world_mission( mission &cur_mission );
        static void add_existing( const mission &m );

        static mission_status status_from_string( const std::string &s );
        static std::string status_to_string( mission_status st );

    private:

        void set_target_to_mission_giver();

        static void get_all_item_group_matches(
            std::vector<item *> &items,
            item_group_id &grp_type,
            std::map<itype_id, int> &matches,
            const itype_id &required_container,
            const itype_id &actual_container,
            bool &specific_container_required );

};

template<>
struct enum_traits<mission::mission_status> {
    static constexpr mission::mission_status last = mission::mission_status::num_mission_status;
};

#endif // CATA_SRC_MISSION_H
