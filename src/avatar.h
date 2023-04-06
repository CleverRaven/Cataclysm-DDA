#pragma once
#ifndef CATA_SRC_AVATAR_H
#define CATA_SRC_AVATAR_H

#include <cstddef>
#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "enums.h"
#include "game_constants.h"
#include "magic_teleporter_list.h"
#include "mdarray.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class advanced_inv_area;
class advanced_inv_listitem;
class advanced_inventory_pane;
class diary;
class faction;
class item;
class item_location;
class JsonObject;
class JsonOut;
class mission;
class monster;
class nc_color;
class npc;
class talker;
struct bionic;

namespace catacurses
{
class window;
} // namespace catacurses
enum class character_type : int;
class map_memory;
struct memorized_terrain_tile;

namespace debug_menu
{
class mission_debug;
}  // namespace debug_menu
struct mtype;
enum class pool_type;

// Monster visible in different directions (safe mode & compass)
// Suppressions due to a bug in clang-tidy 12
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
struct monster_visible_info {
    // New monsters visible from last update
    std::vector<shared_ptr_fast<monster>> new_seen_mon;

    // Unique monsters (and types of monsters) visible in different directions
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::array<std::vector<npc *>, 9> unique_types;
    std::array<std::vector<std::pair<const mtype *, int>>, 9> unique_mons;

    // If the monster visible in this direction is dangerous
    std::array<bool, 8> dangerous = {};

    // Whether or not there is at last one creature within safemode proximity that
    // is dangerous
    bool has_dangerous_creature_in_proximity = false;

    void remove_npc( npc *n );
};

class avatar : public Character
{
    public:
        avatar();
        avatar( const avatar & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        avatar( avatar && );
        ~avatar() override;
        avatar &operator=( const avatar & ) = delete;
        // NOLINTNEXTLINE(performance-noexcept-move-constructor)
        avatar &operator=( avatar && );

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );
        void serialize( JsonOut &json ) const override;
        void deserialize( const JsonObject &data ) override;
        bool save_map_memory();
        void load_map_memory();

        // newcharacter.cpp
        bool create( character_type type, const std::string &tempname = "" );
        // initialize avatar and avatar mocks
        void initialize( character_type type );
        void add_profession_items();
        void randomize( bool random_scenario, bool play_now = false );
        void randomize_cosmetics();
        bool load_template( const std::string &template_name, pool_type & );
        void save_template( const std::string &name, pool_type );
        void character_to_template( const std::string &name );

        bool is_avatar() const override {
            return true;
        }
        avatar *as_avatar() override {
            return this;
        }
        const avatar *as_avatar() const override {
            return this;
        }

        mfaction_id get_monster_faction() const override;

        std::string get_save_id() const {
            return save_id.empty() ? name : save_id;
        }
        void set_save_id( const std::string &id ) {
            save_id = id;
        }
        /**
         * Makes the avatar "take over" the given NPC, while the current avatar character
         * becomes an NPC.
         */
        void control_npc( npc &, bool debug = false );
        /**
         * Open a menu to choose the NPC to take over.
         */
        void control_npc_menu( bool debug = false );
        using Character::query_yn;
        bool query_yn( const std::string &mes ) const override;

        void toggle_map_memory();
        bool should_show_map_memory() const;
        void prepare_map_memory_region( const tripoint &p1, const tripoint &p2 );
        /** Memorizes a given tile in tiles mode; finalize_tile_memory needs to be called after it */
        void memorize_tile( const tripoint &pos, const std::string &ter, int subtile,
                            int rotation );
        /** Returns last stored map tile in given location in tiles mode */
        const memorized_terrain_tile &get_memorized_tile( const tripoint &p ) const;
        /** Memorizes a given tile in curses mode; finalize_terrain_memory_curses needs to be called after it */
        void memorize_symbol( const tripoint &pos, int symbol );
        /** Returns last stored map tile in given location in curses mode */
        int get_memorized_symbol( const tripoint &p ) const;
        void clear_memorized_tile( const tripoint &pos );

        nc_color basic_symbol_color() const override;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;

        /** Provides the window and detailed morale data */
        void disp_morale();
        /** Opens the medical window */
        void disp_medical();
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Resets all missions before saving character to template */
        void reset_all_missions();

        std::vector<mission *> get_active_missions() const;
        std::vector<mission *> get_completed_missions() const;
        std::vector<mission *> get_failed_missions() const;
        /**
         * Returns the mission that is currently active. Returns null if mission is active.
         */
        mission *get_active_mission() const;
        /**
         * Returns the target of the active mission or @ref overmap::invalid_tripoint if there is
         * no active mission.
         */
        tripoint_abs_omt get_active_mission_target() const;
        /**
         * Set which mission is active. The mission must be listed in @ref active_missions.
         */
        void set_active_mission( mission &cur_mission );
        /**
         * Called when a mission has been assigned to the player.
         */
        void on_mission_assignment( mission &new_mission );
        /**
         * Called when a mission has been completed or failed. Either way it's finished.
         * Check @ref mission::has_failed to see which case it is.
         */
        void on_mission_finished( mission &cur_mission );

        void remove_active_mission( mission &cur_mission );

        //return avatar diary
        diary *get_avatar_diary();

        // Dialogue and bartering--see npctalk.cpp
        void talk_to( std::unique_ptr<talker> talk_with, bool radio_contact = false,
                      bool is_computer = false, bool is_not_conversation = false );

        /**
         * Try to disarm the NPC. May result in fail attempt, you receiving the weapon and instantly wielding it,
         * or the weapon falling down on the floor nearby. NPC is always getting angry with you.
         * @param target Target NPC to disarm
         */
        void disarm( npc &target );
        /**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Container containing the item to be wielded
         * @param internal_item reference to contained item to wield.
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */
        bool wield_contents( item &container, item *internal_item = nullptr, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY );
        /** Handles sleep attempts by the player, starts ACT_TRY_SLEEP activity */
        void try_to_sleep( const time_duration &dur );
        void set_location( const tripoint_abs_ms &loc );
        /** Handles reading effects and returns true if activity started */
        bool read( item_location &book, item_location ereader = {} );
        /** Note that we've read a book at least once. **/
        bool has_identified( const itype_id &item_id ) const override;
        void identify( const item &item ) override;
        void clear_identified();

        void add_snippet( snippet_id snippet );
        bool has_seen_snippet( const snippet_id &snippet ) const;
        const std::set<snippet_id> &get_snippets();


        /**
         * Opens the targeting menu to pull a nearby creature towards the character.
         * @param name Name of the implement used to pull the creature. */
        void longpull( const std::string &name );

        void wake_up() override;
        // Grab furniture / vehicle
        void grab( object_type grab_type, const tripoint &grab_point = tripoint_zero );
        object_type get_grab_type() const;
        /** Handles player vomiting effects */
        void vomit();
        void add_pain_msg( int val, const bodypart_id &bp ) const;
        /**
         * Try to steal an item from the NPC's inventory. May result in fail attempt, when NPC not notices you,
         * notices your steal attempt and getting angry with you, and you successfully stealing the item.
         * @param target Target NPC to steal from
         */
        void steal( npc &target );
        /** Reassign letter. */
        void reassign_item( item &it, int invlet );

        teleporter_list translocators;

        void upgrade_stat_prompt( const character_stat &stat_name );
        // how many points are available to upgrade via STK
        int free_upgrade_points() const;
        void power_bionics() override;
        void power_mutations() override;
        /** Returns the bionic with the given invlet, or NULL if no bionic has that invlet */
        bionic *bionic_by_invlet( int ch );

        faction *get_faction() const override;
        // Set in npc::talk_to_you for use in further NPC interactions
        bool dialogue_by_radio = false;
        // Preferred aim mode - ranged.cpp aim mode defaults to this if possible
        std::string preferred_aiming_mode;

        // checks if the point is blocked based on characters current aiming state
        bool cant_see( const tripoint &p );

        // rebuilds the full aim cache for the character if it is dirty
        void rebuild_aim_cache();

        void set_movement_mode( const move_mode_id &mode ) override;

        // Cycles to the next move mode.
        void cycle_move_mode();
        // Cycles to the previous move mode.
        void cycle_move_mode_reverse();
        // Resets to walking.
        void reset_move_mode();
        // Toggles running on/off.
        void toggle_run_mode();
        // Toggles crouching on/off.
        void toggle_crouch_mode();
        // Toggles lying down on/off.
        void toggle_prone_mode();
        // Activate crouch mode if not in crouch mode.
        void activate_crouch_mode();

        bool wield( item_location target );
        bool wield( item &target ) override;
        bool wield( item &target, int obtain_cost );

        /** gets the inventory from the avatar that is interactible via advanced inventory management */
        std::vector<advanced_inv_listitem> get_AIM_inventory( const advanced_inventory_pane &pane,
                advanced_inv_area &square );

        using Character::invoke_item;
        bool invoke_item( item *, const tripoint &pt, int pre_obtain_moves ) override;
        bool invoke_item( item * ) override;
        bool invoke_item( item *, const std::string &, const tripoint &pt,
                          int pre_obtain_moves = -1 ) override;
        bool invoke_item( item *, const std::string & ) override;

        monster_visible_info &get_mon_visible() {
            return mon_visible;
        }

        const monster_visible_info &get_mon_visible() const {
            return mon_visible;
        }

        struct daily_calories {
            int spent = 0;
            int gained = 0;
            int ingested = 0;
            int total() const noexcept {
                return gained - spent;
            }
            std::map<float, int> activity_levels; // NOLINT(cata-serialize)

            daily_calories();

            void serialize( JsonOut &json ) const;
            void deserialize( const JsonObject &data );

            void save_activity( JsonOut &json ) const;
            void read_activity( const JsonObject &data );
        };

        // called once a day; adds a new daily_calories to the
        // front of the list and pops off the back if there are more than 30
        void advance_daily_calories();
        int get_daily_spent_kcal( bool yesterday ) const;
        int get_daily_ingested_kcal( bool yesterday ) const;
        void add_ingested_kcal( int kcal );
        void update_cardio_acc() override;
        void add_spent_calories( int cal ) override;
        void add_gained_calories( int cal ) override;
        void log_activity_level( float level ) override;
        std::string total_daily_calories_string() const;
        //set 0-3 random hobbies, with 1 and 2 being twice as likely as 0 and 3
        void randomize_hobbies();
        void add_random_hobby( std::vector<profession_id> &choices );

        int movecounter = 0;

        // bionic power in the last turn
        units::energy power_prev_turn = 0_kJ;
        // balance/net power generation/loss during the last turn
        units::energy power_balance = 0_kJ;

        // amount of turns since last check for pocket noise
        time_point last_pocket_noise = time_point( 0 );

        vproto_id starting_vehicle = vproto_id::NULL_ID();
        std::vector<mtype_id> starting_pets;
        std::set<character_id> follower_ids;

        bool aim_cache_dirty = true;

        const mood_face_id &character_mood_face( bool clear_cache = false ) const;

    private:

        // The name used to generate save filenames for this avatar. Not serialized in json.
        std::string save_id;

        std::unique_ptr<map_memory> player_map_memory;
        bool show_map_memory;

        friend class debug_menu::mission_debug;
        /**
         * Missions that the player has accepted and that are not finished (one
         * way or the other).
         */
        std::vector<mission *> active_missions;
        /**
         * Missions that the player has successfully completed.
         */
        std::vector<mission *> completed_missions;
        /**
         * Missions that have failed while being assigned to the player.
         */
        std::vector<mission *> failed_missions;
        /**
         * The currently active mission, or null if no mission is currently in progress.
         */
        mission *active_mission;
        /**
        * diary to track player progression and to write the players stroy
        */
        std::unique_ptr <diary> a_diary;
        /**
         * The amount of calories spent and gained per day for the last 30 days.
         * the back is popped off and a new one added to the front at midnight each day
         */
        std::list<daily_calories> calorie_diary;

        // Items the player has identified.
        std::unordered_set<itype_id> items_identified;

        // Snippets the player has seen
        std::set<snippet_id> snippets_read;

        object_type grab_type;

        monster_visible_info mon_visible;

        /**
         * The NPC that would control the avatar's character in the avatar's absence.
         * The Character data in this object is not relevant/used.
         */
        std::unique_ptr<npc> shadow_npc;

        // true when the space is still visible when aiming
        cata::mdarray<bool, point_bub_ms> aim_cache;
};

avatar &get_avatar();
std::unique_ptr<talker> get_talker_for( avatar &me );
std::unique_ptr<talker> get_talker_for( avatar *me );

#endif // CATA_SRC_AVATAR_H
