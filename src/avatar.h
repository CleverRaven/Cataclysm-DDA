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
#include "json.h"
#include "magic_teleporter_list.h"
#include "map_memory.h"
#include "memory_fast.h"
#include "player.h"
#include "point.h"
#include "type_id.h"

class advanced_inv_area;
class advanced_inv_listitem;
class advanced_inventory_pane;
class faction;
class item;
class item_location;
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

namespace debug_menu
{
class mission_debug;
}  // namespace debug_menu
struct mtype;
struct points_left;

// Monster visible in different directions (safe mode & compass)
struct monster_visible_info {
    // New monsters visible from last update
    std::vector<shared_ptr_fast<monster>> new_seen_mon;

    // Unique monsters (and types of monsters) visible in different directions
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::vector<npc *> unique_types[9];
    std::vector<const mtype *> unique_mons[9];

    // If the monster visible in this direction is dangerous
    bool dangerous[8] = {};
};

class avatar : public player
{
    public:
        avatar();

        void store( JsonOut &json ) const;
        void load( const JsonObject &data );
        void serialize( JsonOut &json ) const override;
        void deserialize( JsonIn &jsin ) override;
        void serialize_map_memory( JsonOut &jsout ) const;
        void deserialize_map_memory( JsonIn &jsin );

        // newcharacter.cpp
        bool create( character_type type, const std::string &tempname = "" );
        void add_profession_items();
        void randomize( bool random_scenario, points_left &points, bool play_now = false );
        bool load_template( const std::string &template_name, points_left &points );
        void save_template( const std::string &name, const points_left &points );

        bool is_avatar() const override {
            return true;
        }
        avatar *as_avatar() override {
            return this;
        }
        const avatar *as_avatar() const override {
            return this;
        }

        void toggle_map_memory();
        bool should_show_map_memory();
        /** Memorizes a given tile in tiles mode; finalize_tile_memory needs to be called after it */
        void memorize_tile( const tripoint &pos, const std::string &ter, int subtile,
                            int rotation );
        /** Returns last stored map tile in given location in tiles mode */
        memorized_terrain_tile get_memorized_tile( const tripoint &p ) const;
        /** Memorizes a given tile in curses mode; finalize_terrain_memory_curses needs to be called after it */
        void memorize_symbol( const tripoint &pos, int symbol );
        /** Returns last stored map tile in given location in curses mode */
        int get_memorized_symbol( const tripoint &p ) const;
        /** Returns the amount of tiles survivor can remember. */
        size_t max_memorized_tiles() const;
        void clear_memorized_tile( const tripoint &pos );

        nc_color basic_symbol_color() const override;
        int print_info( const catacurses::window &w, int vStart, int vLines, int column ) const override;

        /** Provides the window and detailed morale data */
        void disp_morale();
        /** Uses morale and other factors to return the player's focus target goto value */
        int calc_focus_equilibrium( bool ignore_pain = false ) const;
        /** Calculates actual focus gain/loss value from focus equilibrium*/
        int calc_focus_change() const;
        /** Uses calc_focus_change to update the player's current focus */
        void update_mental_focus();
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

        // Dialogue and bartering--see npctalk.cpp
        void talk_to( std::unique_ptr<talker> talk_with, bool text_only = false,
                      bool radio_contact = false );

        /**
         * Try to disarm the NPC. May result in fail attempt, you receiving the weapon and instantly wielding it,
         * or the weapon falling down on the floor nearby. NPC is always getting angry with you.
         * @param target Target NPC to disarm
         */
        void disarm( npc &target );

        /**
         * Helper function for player::read.
         *
         * @param book Book to read
         * @param reasons Starting with g->u, for each player/NPC who cannot read, a message will be pushed back here.
         * @returns nullptr, if neither the player nor his followers can read to the player, otherwise the player/NPC
         * who can read and can read the fastest
         */
        const player *get_book_reader( const item &book, std::vector<std::string> &reasons ) const;
        /**
         * Helper function for get_book_reader
         * @warning This function assumes that the everyone is able to read
         *
         * @param book The book being read
         * @param reader the player/NPC who's reading to the caller
         * @param learner if not nullptr, assume that the caller and reader read at a pace that isn't too fast for him
         */
        int time_to_read( const item &book, const player &reader, const player *learner = nullptr ) const;
        /** Handles reading effects and returns true if activity started */
        bool read( item &it, bool continuous = false );
        /** Completes book reading action. **/
        void do_read( item &book );
        /** Note that we've read a book at least once. **/
        bool has_identified( const itype_id &item_id ) const override;
        void identify( const item &item ) override;
        void clear_identified();

        void wake_up();
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

        teleporter_list translocators;

        int get_str_base() const override;
        int get_dex_base() const override;
        int get_int_base() const override;
        int get_per_base() const override;

        void upgrade_stat_prompt( const character_stat &stat_name );
        // how many points are available to upgrade via STK
        int free_upgrade_points() const;
        // how much "kill xp" you have
        int kill_xp() const;
        void power_bionics() override;
        void power_mutations() override;
        /** Returns the bionic with the given invlet, or NULL if no bionic has that invlet */
        bionic *bionic_by_invlet( int ch );

        faction *get_faction() const override;
        // Set in npc::talk_to_you for use in further NPC interactions
        bool dialogue_by_radio = false;
        // Preferred aim mode - ranged.cpp aim mode defaults to this if possible
        std::string preferred_aiming_mode;

        void set_movement_mode( const move_mode_id &mode ) override;

        // Cycles to the next move mode.
        void cycle_move_mode();
        // Resets to walking.
        void reset_move_mode();
        // Toggles running on/off.
        void toggle_run_mode();
        // Toggles crouching on/off.
        void toggle_crouch_mode();
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

        struct daily_calories {
            int spent = 0;
            int gained = 0;
            int total() const {
                return gained - spent;
            }
            std::map<float, int> activity_levels;

            void serialize( JsonOut &json ) const {
                json.start_object();

                json.member( "spent", spent );
                json.member( "gained", gained );
                save_activity( json );

                json.end_object();
            }
            void deserialize( JsonIn &jsin ) {
                JsonObject data = jsin.get_object();

                data.read( "spent", spent );
                data.read( "gained", gained );
                if( data.has_member( "activity" ) ) {
                    read_activity( data );
                }
            }

            daily_calories() {
                activity_levels.emplace( NO_EXERCISE, 0 );
                activity_levels.emplace( LIGHT_EXERCISE, 0 );
                activity_levels.emplace( MODERATE_EXERCISE, 0 );
                activity_levels.emplace( BRISK_EXERCISE, 0 );
                activity_levels.emplace( ACTIVE_EXERCISE, 0 );
                activity_levels.emplace( EXTRA_EXERCISE, 0 );
            }

            void save_activity( JsonOut &json ) const;
            void read_activity( JsonObject &data );

        };
        // called once a day; adds a new daily_calories to the
        // front of the list and pops off the back if there are more than 30
        void advance_daily_calories();
        void add_spent_calories( int cal ) override;
        void add_gained_calories( int cal ) override;
        void log_activity_level( float level ) override;
        std::string total_daily_calories_string() const;

    private:
        map_memory player_map_memory;
        bool show_map_memory;
        /** Used in max_memorized_tiles to cache memory capacity. **/
        mutable time_point current_map_memory_turn = calendar::before_time_starts;
        mutable size_t current_map_memory_capacity = 0;

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
         * The amount of calories spent and gained per day for the last 30 days.
         * the back is popped off and a new one added to the front at midnight each day
         */
        std::list<daily_calories> calorie_diary;

        // Items the player has identified.
        std::unordered_set<itype_id> items_identified;

        object_type grab_type;

        // these are the stat upgrades from stats through kills

        int str_upgrade = 0;
        int dex_upgrade = 0;
        int int_upgrade = 0;
        int per_upgrade = 0;

        monster_visible_info mon_visible;
};

avatar &get_avatar();
std::unique_ptr<talker> get_talker_for( avatar &me );
std::unique_ptr<talker> get_talker_for( avatar *me );

struct points_left {
    int stat_points;
    int trait_points;
    int skill_points;

    enum point_limit : int {
        FREEFORM = 0,
        ONE_POOL,
        MULTI_POOL,
        TRANSFER,
    } limit;

    points_left();
    void init_from_options();
    // Highest amount of points to spend on stats without points going invalid
    int stat_points_left() const;
    int trait_points_left() const;
    int skill_points_left() const;
    bool is_freeform();
    bool is_valid();
    bool has_spare();
    std::string to_string();
};

#endif // CATA_SRC_AVATAR_H
