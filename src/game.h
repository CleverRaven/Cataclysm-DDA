#pragma once
#ifndef GAME_H
#define GAME_H

#include <array>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "calendar.h"
#include "cursesdef.h"
#include "enums.h"
#include "game_constants.h"
#include "int_id.h"
#include "item_location.h"
#include "optional.h"
#include "pimpl.h"
#include "posix_time.h"

extern bool test_mode;

// The reference to the one and only game instance.
class game;
extern std::unique_ptr<game> g;

extern bool trigdist;
extern bool use_tiles;
extern bool fov_3d;
extern bool tile_iso;

extern const int core_version;

extern const int savegame_version;
extern int savegame_loading_version;

class input_context;
input_context get_default_mode_input_context();

enum class dump_mode {
    TSV,
    HTML
};

enum quit_status {
    QUIT_NO = 0,    // Still playing
    QUIT_SUICIDE,   // Quit with 'Q'
    QUIT_SAVED,     // Saved and quit
    QUIT_NOSAVED,   // Quit without saving
    QUIT_DIED,      // Actual death
    QUIT_WATCH,     // Died, and watching aftermath
};

enum safe_mode_type {
    SAFE_MODE_OFF = 0, // Moving always allowed
    SAFE_MODE_ON = 1, // Moving allowed, but if a new monsters spawns, go to SAFE_MODE_STOP
    SAFE_MODE_STOP = 2, // New monsters spotted, no movement allowed
};

enum body_part : int;
enum weather_type : int;
enum action_id : int;
enum target_mode : int;

class item_location;
class item;
struct targeting_data;
struct special_game;
struct itype;
struct mtype;
using mtype_id = string_id<mtype>;
struct species_type;
using species_id = string_id<species_type>;
using itype_id = std::string;
class ammunition_type;
using ammotype = string_id<ammunition_type>;
class mission;
class map;
class Creature;
class zone_type;
using zone_type_id = string_id<zone_type>;
class Character;
class faction_manager;
class player;
class npc;
class monster;
struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;
class vehicle;
class Creature_tracker;
class calendar;
class scenario;
class DynamicDataLoader;
class salvage_actor;
class input_context;
class map_item_stack;
struct WORLD;
class save_t;
typedef WORLD *WORLDPTR;
class overmap;
class event_manager;
enum event_type : int;
struct vehicle_part;
struct ter_t;
using ter_id = int_id<ter_t>;
class weather_generator;
struct weather_printable;
class faction;
class live_view;
class nc_color;
struct w_point;
struct explosion_data;
struct visibility_variables;
class scent_map;
class loading_ui;

typedef std::function<bool( const item & )> item_filter;

enum liquid_dest : int {
    LD_NULL,
    LD_CONSUME,
    LD_ITEM,
    LD_VEH,
    LD_KEG,
    LD_GROUND
};

struct liquid_dest_opt {
    liquid_dest dest_opt = LD_NULL;
    tripoint pos;
    item_location item_loc;
    vehicle *veh = nullptr;
};

enum peek_act : int {
    PA_BLIND_THROW
    // obvious future additional value is PA_BLIND_FIRE
};

struct look_around_result {
    cata::optional<tripoint> position;
    cata::optional<peek_act> peek_action;
};

class game
{
        friend class editmap;
        friend class advanced_inventory;
        friend class main_menu;
        friend class target_handler;
    public:
        game();
        ~game();

        /** Loads static data that does not depend on mods or similar. */
        void load_static_data();

        /** Loads core dynamic data. May throw. */
        void load_core_data( loading_ui &ui );

        /** Returns whether the core data is currently loaded. */
        bool is_core_data_loaded() const;

        /**
         *  Check if mods can be successfully loaded
         *  @param opts check specific mods (or all if unspecified)
         *  @return whether all mods were successfully loaded
         */
        bool check_mod_data( const std::vector<mod_id> &opts, loading_ui &ui );

        /** Loads core data and mods from the active world. May throw. */
        void load_world_modfiles( loading_ui &ui );
        /**
         * Base path for saving player data. Just add a suffix (unique for
         * the thing you want to save) and use the resulting path.
         * Example: `save_ui_data(get_player_base_save_path()+".ui")`
         */
        std::string get_player_base_save_path() const;
        /**
         * Base path for saving world data. This yields a path to a folder.
         */
        std::string get_world_base_save_path() const;
        /**
         *  Load content packs
         *  @param msg string to display whilst loading prompt
         *  @param packs content packs to load in correct dependent order
         *  @param ui structure for load progress display
         *  @return true if all packs were found, false if any were missing
         */
        bool load_packs( const std::string &msg, const std::vector<mod_id> &packs, loading_ui &ui );

    protected:
        /** Loads dynamic data from the given directory. May throw. */
        void load_data_from_dir( const std::string &path, const std::string &src, loading_ui &ui );

        // May be a bit hacky, but it's probably better than the header spaghetti
        pimpl<map> map_ptr;
        pimpl<player> u_ptr;
        pimpl<live_view> liveview_ptr;
        live_view &liveview;
        pimpl<scent_map> scent_ptr;
        pimpl<event_manager> event_manager_ptr;
    public:

        /** Initializes the UI. */
        void init_ui( const bool resized = false );
        void setup();
        /** True if the game has just started or loaded, else false. */
        bool new_game;
        /** Used in main.cpp to determine what type of quit is being performed. */
        quit_status uquit;
        /** Saving and loading functions. */
        void serialize( std::ostream &fout ); // for save
        void unserialize( std::istream &fin ); // for load
        void unserialize_master( std::istream &fin ); // for load

        /** write statistics to stdout and @return true if successful */
        bool dump_stats( const std::string &what, dump_mode mode, const std::vector<std::string> &opts );

        /** Returns false if saving failed. */
        bool save();
        /** Returns a list of currently active character saves. */
        std::vector<std::string> list_active_characters();
        void write_memorial_file( std::string sLastWords );
        bool cleanup_at_end();
        void start_calendar();
        /** MAIN GAME LOOP. Returns true if game is over (death, saved, quit, etc.). */
        bool do_turn();
        void draw();
        void draw_ter( bool draw_sounds = true );
        void draw_ter( const tripoint &center, bool looking = false, bool draw_sounds = true );
        /**
         * Returns the location where the indicator should go relative to the reality bubble,
         * or nothing to indicate no indicator should be drawn.
         * Based on the vehicle the player is driving, if any.
         * @param next If true, bases it on the vehicle the vehicle will turn to next turn,
         * instead of the one it is currently facing.
         */
        cata::optional<tripoint> get_veh_dir_indicator_location( bool next ) const;
        void draw_veh_dir_indicator( bool next );

        /** Make map a reference here, to avoid map.h in game.h */
        map &m;
        player &u;
        scent_map &scent;
        event_manager &events;

        pimpl<Creature_tracker> critter_tracker;
        pimpl<faction_manager> faction_manager_ptr;

        /** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */
        void explosion(
            const tripoint &p, float power, float factor = 0.8f,
            bool fire = false, int casing_mass = 0, float fragment_mass = 0.05
        );

        void explosion(
            const tripoint &p, const explosion_data &ex
        );

        /** Helper for explosion, does the actual blast. */
        void do_blast( const tripoint &p, float power, float factor, bool fire );

        /*
         * Emits shrapnel damaging creatures and sometimes terrain/furniture within range
         * @param src source from which shrapnel radiates outwards in a uniformly random distribution
         * @param power raw kinetic energy which is responsible for damage and reduced by effects of cover
         * @param casing_mass total mass of bomb casing, determines fragment velocity.
         * @param fragment_mass mass of individual fragments, affects range, damage and coverage.
         * @param range maximum distance shrapnel may travel
         * @return vector containing all tiles that took damage.
         */
        std::vector<tripoint> shrapnel( const tripoint &src, int power, int casing_mass,
                                        float fragment_mass, int range = -1 );

        /** Triggers a flashbang explosion at p. */
        void flashbang( const tripoint &p, bool player_immune = false );
        /** Moves the player vertically. If force == true then they are falling. */
        void vertical_move( int z, bool force );
        /** Returns the other end of the stairs (if any). May query, affect u etc.  */
        cata::optional<tripoint> find_or_make_stairs( map &mp, int z_after, bool &rope_ladder );
        /** Actual z-level movement part of vertical_move. Doesn't include stair finding, traps etc. */
        void vertical_shift( int dest_z );
        /** Add goes up/down auto_notes (if turned on) */
        void vertical_notes( int z_before, int z_after );
        /** Checks to see if a player can use a computer (not illiterate, etc.) and uses if able. */
        void use_computer( const tripoint &p );
        /** Triggers a resonance cascade at p. */
        void resonance_cascade( const tripoint &p );
        /** Triggers a scrambler blast at p. */
        void scrambler_blast( const tripoint &p );
        /** Triggers an EMP blast at p. */
        void emp_blast( const tripoint &p );
        /**
         * @return The living creature with the given id. Returns null if no living
         * creature with such an id exists. Never returns a dead creature.
         * Currently only the player character and npcs have ids.
         */
        template<typename T = Creature>
        T * critter_by_id( int id );
        /**
         * Returns the Creature at the given location. Optionally casted to the given
         * type of creature: @ref npc, @ref player, @ref monster - if there is a creature,
         * but it's not of the requested type, returns nullptr.
         * @param allow_hallucination Whether to return monsters that are actually hallucinations.
         */
        template<typename T = Creature>
        T * critter_at( const tripoint &p, bool allow_hallucination = false );
        template<typename T = Creature>
        const T * critter_at( const tripoint &p, bool allow_hallucination = false ) const;
        /**
         * Returns a shared pointer to the given critter (which can be of any of the subclasses of
         * @ref Creature). The function may return an empty pointer if the given critter
         * is not stored anywhere (e.g. it was allocated on the stack, not stored in
         * the @ref critter_tracker nor in @ref active_npc nor is it @ref u).
         */
        template<typename T = Creature>
        std::shared_ptr<T> shared_from( const T &critter );

        /**
         * Summons a brand new monster at the current time. Returns the summoned monster.
         * Returns a `nullptr` if the monster could not be created.
         */
        monster *summon_mon( const mtype_id &id, const tripoint &p );
        /** Calls the creature_tracker add function. Returns true if successful. */
        bool add_zombie( monster &critter );
        bool add_zombie( monster &critter, bool pin_upgrade );
        /**
         * Returns the approximate number of creatures in the reality bubble.
         * Because of performance restrictions it may return a slightly incorrect
         * values (as it includes dead, but not yet cleaned up creatures).
         */
        size_t num_creatures() const;
        /** Redirects to the creature_tracker update_pos() function. */
        bool update_zombie_pos( const monster &critter, const tripoint &pos );
        void remove_zombie( const monster &critter );
        /** Redirects to the creature_tracker clear() function. */
        void clear_zombies();
        /** Spawns a hallucination at a determined position. */
        bool spawn_hallucination( const tripoint &p );
        /** Swaps positions of two creatures */
        bool swap_critters( Creature &first, Creature &second );

    private:
        friend class monster_range;
        friend class Creature_range;

        template<typename T>
        class non_dead_range
        {
            public:
                std::vector<std::weak_ptr<T>> items;

                class iterator
                {
                    private:
                        bool valid();
                    public:
                        std::vector<std::weak_ptr<T>> &items;
                        typename std::vector<std::weak_ptr<T>>::iterator iter;
                        std::shared_ptr<T> current;

                        iterator( std::vector<std::weak_ptr<T>> &i,
                                  const typename std::vector<std::weak_ptr<T>>::iterator t ) : items( i ), iter( t ) {
                            while( iter != items.end() && !valid() ) {
                                ++iter;
                            }
                        }
                        iterator( const iterator & ) = default;
                        iterator &operator=( const iterator & ) = default;

                        bool operator==( const iterator &rhs ) const {
                            return iter == rhs.iter;
                        }
                        bool operator!=( const iterator &rhs ) const {
                            return !operator==( rhs );
                        }
                        iterator &operator++() {
                            do {
                                ++iter;
                            } while( iter != items.end() && !valid() );
                            return *this;
                        }
                        T &operator*() const {
                            return *current;
                        }
                };
                iterator begin() {
                    return iterator( items, items.begin() );
                }
                iterator end() {
                    return iterator( items, items.end() );
                }
        };

        class monster_range : public non_dead_range<monster>
        {
            public:
                monster_range( game &g );
        };

        class npc_range : public non_dead_range<npc>
        {
            public:
                npc_range( game &g );
        };

        class Creature_range : public non_dead_range<Creature>
        {
            private:
                std::shared_ptr<player> u;

            public:
                Creature_range( game &g );
        };

    public:
        /**
         * Returns an anonymous range that contains all creatures. The range allows iteration
         * via a range-based for loop, e.g. `for( Creature &critter : all_creatures() ) { ... }`.
         * One shall not store the returned range nor the iterators.
         * One can freely remove and add creatures to the game during the iteration. Added
         * creatures will not be iterated over.
         */
        Creature_range all_creatures();
        /// Same as @ref all_creatures but iterators only over monsters.
        monster_range all_monsters();
        /// Same as @ref all_creatures but iterators only over npcs.
        npc_range all_npcs();

        /**
         * Returns all creatures matching a predicate. Only living ( not dead ) creatures
         * are checked ( and returned ). Returned pointers are never null.
         */
        std::vector<Creature *> get_creatures_if( const std::function<bool( const Creature & )> &pred );
        std::vector<npc *> get_npcs_if( const std::function<bool( const npc & )> &pred );
        /**
         * Returns a creature matching a predicate. Only living (not dead) creatures
         * are checked. Returns `nullptr` if no creature matches the predicate.
         * There is no guarantee which creature is returned when several creatures match.
         */
        Creature *get_creature_if( const std::function<bool( const Creature & )> &pred );

        /** Returns true if there is no player, NPC, or monster on the tile and move_cost > 0. */
        bool is_empty( const tripoint &p );
        /** Returns true if p is outdoors and it is sunny. */
        bool is_in_sunlight( const tripoint &p );
        /** Returns true if p is indoors, underground, or in a car. */
        bool is_sheltered( const tripoint &p );
        /**
         * Revives a corpse at given location. The monster type and some of its properties are
         * deducted from the corpse. If reviving succeeds, the location is guaranteed to have a
         * new monster there (see @ref critter_at).
         * @param location The place where to put the revived monster.
         * @param corpse The corpse item, it must be a valid corpse (see @ref item::is_corpse).
         * @return Whether the corpse has actually been redivided. Reviving may fail for many
         * reasons, including no space to put the monster, corpse being to much damaged etc.
         * If the monster was revived, the caller should remove the corpse item.
         * If reviving failed, the item is unchanged, as is the environment (no new monsters).
         */
        bool revive_corpse( const tripoint &location, item &corpse );

        /**
         * Returns true if the player is allowed to fire a given item, or false if otherwise.
         * reload_time is stored as a side effect of condition testing.
         * @param args Contains item data and targeting mode for the gun we want to fire.
         * @return True if all conditions are true, otherwise false.
         */
        bool plfire_check( const targeting_data &args );

        /**
         * Handles interactive parts of gun firing (target selection, etc.).
         * @return Whether an attack was actually performed.
         */
        bool plfire();
        /**
         * Handles interactive parts of gun firing (target selection, etc.).
         * This version stores targeting parameters for weapon, used for calls to the nullary form.
         * @param weapon Reference to a weapon we want to start aiming.
         * @param bp_cost The amount by which the player's power reserve is decreased after firing.
         * @return Whether an attack was actually performed.
         */
        bool plfire( item &weapon, int bp_cost = 0 );
        /** Redirects to player::cancel_activity(). */
        void cancel_activity();
        /** Asks if the player wants to cancel their activity, and if so cancels it. */
        bool cancel_activity_query( const std::string &message );
        /** Asks if the player wants to cancel their activity and if so cancels it. Additionally checks
         *  if the player wants to ignore further distractions. */
        bool cancel_activity_or_ignore_query( const distraction_type type, const std::string &reason );
        /** Handles players exiting from moving vehicles. */
        void moving_vehicle_dismount( const tripoint &p );

        /** Returns the current remotely controlled vehicle. */
        vehicle *remoteveh();
        /** Sets the current remotely controlled vehicle. */
        void setremoteveh( vehicle *veh );

        /** Returns the next available mission id. */
        int assign_mission_id();
        npc *find_npc( int id );
        /** Makes any nearby NPCs on the overmap active. */
        void load_npcs();
        /** Unloads all NPCs */
        void unload_npcs();
        /** Unloads, then loads the NPCs */
        void reload_npcs();
        /** Returns the number of kills of the given mon_id by the player. */
        int kill_count( const mtype_id &id );
        /** Returns the number of kills of the given monster species by the player. */
        int kill_count( const species_id &spec );
        /** Increments the number of kills of the given mtype_id by the player upwards. */
        void increase_kill_count( const mtype_id &id );
        /** Record the fact that the player murdered an NPC. */
        void record_npc_kill( const npc &p );
        /** Return list of killed NPC */
        std::list<std::string> get_npc_kill();

        /** Performs a random short-distance teleport on the given player, granting teleglow if needed. */
        void teleport( player *p = nullptr, bool add_teleglow = true );
        /** Handles swimming by the player. Called by plmove(). */
        void plswim( const tripoint &p );
        /** Picks and spawns a random fish from the remaining fish list when a fish is caught. */
        void catch_a_monster( std::vector<monster *> &catchables, const tripoint &pos, player *p,
                              const time_duration &catch_duration );
        /**
         * Get the fishable monsters within the contiguous fishable terrain starting at fish_pos,
         * out to the specificed distance.
         * @param distance Distance around the fish_pos to examine for contiguous fishable terrain.
         * @param fish_pos The location being fished.
         * @return Fishable monsters within the specified contiguous fishable terrain.
         */
        std::vector<monster *> get_fishable( int distance, const tripoint &fish_pos );
        /** Flings the input creature in the given direction. */
        void fling_creature( Creature *c, const int &dir, float flvel, bool controlled = false );

        /** Nuke the area at p - global overmap terrain coordinates! */
        void nuke( const tripoint &p );
        float natural_light_level( int zlev ) const;
        /** Returns coarse number-of-squares of visibility at the current light level.
         * Used by monster and NPC AI.
         */
        unsigned char light_level( int zlev ) const;
        void reset_light_level();
        int assign_npc_id();
        Creature *is_hostile_nearby();
        Creature *is_hostile_very_close();
        void refresh_all();
        // Handles shifting coordinates transparently when moving between submaps.
        // Helper to make calling with a player pointer less verbose.
        void update_map( player &p );
        point update_map( int &x, int &y );
        void update_overmap_seen(); // Update which overmap tiles we can see

        void process_artifact( item &it, player &p );
        void add_artifact_messages( const std::vector<art_effect_passive> &effects );
        void add_artifact_dreams( );

        void peek();
        void peek( const tripoint &p );
        cata::optional<tripoint> look_debug();

        bool check_zone( const zone_type_id &type, const tripoint &where ) const;
        /** Checks whether or not there is a zone of particular type nearby */
        bool check_near_zone( const zone_type_id &type, const tripoint &where ) const;
        bool is_zones_manager_open() const;
        void zones_manager();

        // Look at nearby terrain ';', or select zone points
        cata::optional<tripoint> look_around();
        look_around_result look_around( catacurses::window w_info, tripoint &center,
                                        const tripoint &start_point, bool has_first_point, bool select_zone, bool peeking );

        // Shared method to print "look around" info
        void pre_print_all_tile_info( const tripoint &lp, const catacurses::window &w_look,
                                      int &line, int last_line, const visibility_variables &cache );

        // Shared method to print "look around" info
        void print_all_tile_info( const tripoint &lp, const catacurses::window &w_look,
                                  const std::string &area_name, int column,
                                  int &line, int last_line, bool draw_terrain_indicators, const visibility_variables &cache );

        /** Long description of (visible) things at tile. */
        void extended_description( const tripoint &p );

        void draw_trail_to_square( const tripoint &t, bool bDrawX );

        // @todo: Move these functions to game_menus::inv and isolate them.
        int inv_for_filter( const std::string &title, item_filter filter,
                            const std::string &none_message = "" );
        int inv_for_all( const std::string &title, const std::string &none_message = "" );
        int inv_for_flag( const std::string &flag, const std::string &title );
        int inv_for_id( const itype_id &id, const std::string &title );

        enum inventory_item_menu_positon {
            RIGHT_TERMINAL_EDGE,
            LEFT_OF_INFO,
            RIGHT_OF_INFO,
            LEFT_TERMINAL_EDGE,
        };
        int inventory_item_menu( int pos, int startx = 0, int width = 50,
                                 inventory_item_menu_positon position = RIGHT_OF_INFO );

        /** Custom-filtered menu for inventory and nearby items and those that within specified radius */
        item_location inv_map_splice( item_filter filter, const std::string &title, int radius = 0,
                                      const std::string &none_message = "" );

        bool has_gametype() const;
        special_game_id gametype() const;

        void toggle_sidebar_style();
        void toggle_fullscreen();
        void toggle_pixel_minimap();
        void reload_tileset();
        void temp_exit_fullscreen();
        void reenter_fullscreen();
        void zoom_in();
        void zoom_out();
        void reset_zoom();
        int get_moves_since_last_save() const;
        int get_user_action_counter() const;

        signed char temperature;              // The air temperature
        // Returns outdoor or indoor temperature of given location (in absolute (@ref map::getabs))
        int get_temperature( const tripoint &location );
        weather_type weather;   // Weather pattern--SEE weather.h
        bool lightning_active;
        int winddirection;
        int windspeed;
        pimpl<w_point> weather_precise; // Cached weather data

        /**
         * The top left corner of the reality bubble (in submaps coordinates). This is the same
         * as @ref map::abs_sub of the @ref m map.
         */
        int get_levx() const;
        int get_levy() const;
        int get_levz() const;
        /**
         * Load the main map at given location, see @ref map::load, in global, absolute submap
         * coordinates.
         */
        void load_map( const tripoint &pos_sm );
        /**
         * The overmap which contains the center submap of the reality bubble.
         */
        overmap &get_cur_om() const;
        const weather_generator &get_cur_weather_gen() const;
        const scenario *scen;
        std::vector<monster> coming_to_stairs;
        int monstairz;

        /** Get all living player allies */
        std::vector<npc *> allies();

    private:
        std::shared_ptr<player> u_shared_ptr;
        std::vector<std::shared_ptr<npc>> active_npc;
    public:
        int ter_view_x;
        int ter_view_y;
        int ter_view_z;

    private:
        catacurses::window w_terrain_ptr;
        catacurses::window w_minimap_ptr;
        catacurses::window w_pixel_minimap_ptr;
        catacurses::window w_HP_ptr;
        catacurses::window w_messages_short_ptr;
        catacurses::window w_messages_long_ptr;
        catacurses::window w_location_wider_ptr;
        catacurses::window w_location_ptr;
        catacurses::window w_status_ptr;
        catacurses::window w_status2_ptr;

    public:
        catacurses::window w_terrain;
        catacurses::window w_overmap;
        catacurses::window w_omlegend;
        catacurses::window w_minimap;
        catacurses::window w_pixel_minimap;
        catacurses::window w_HP;
        //only a pointer, can refer to w_messages_short or w_messages_long
        catacurses::window w_messages;
        catacurses::window w_messages_short;
        catacurses::window w_messages_long;
        catacurses::window w_location_wider;
        catacurses::window w_location;
        catacurses::window w_status;
        catacurses::window w_status2;
        catacurses::window w_blackspace;

        // View offset based on the driving speed (if any)
        // that has been added to u.view_offset,
        // Don't write to this directly, always use set_driving_view_offset
        point driving_view_offset;
        // Setter for driving_view_offset
        void set_driving_view_offset( const point &p );
        // Calculates the driving_view_offset for the given vehicle
        // and sets it (view set_driving_view_offset), if
        // the options for this feature is deactivated or if veh is NULL,
        // the function set the driving offset to (0,0)
        void calc_driving_offset( vehicle *veh = nullptr );

        /**
         * @name Liquid handling
         */
        /**@{*/
        /**
         * Consume / handle all of the liquid. The function can be used when the liquid needs
         * to be handled and can not be put back to where it came from (e.g. when it's a newly
         * created item from crafting).
         * The player is forced to handle all of it, which may required them to pour it onto
         * the ground (if they don't have enough container space available) and essentially
         * loose the item.
         * @return Whether any of the liquid has been consumed. `false` indicates the player has
         * declined all options to handle the liquid (essentially canceled the action) and no
         * charges of the liquid have been transferred.
         * `true` indicates some charges have been transferred (but not necessarily all of them).
         */
        void handle_all_liquid( item liquid, int radius );

        /**
         * Consume / handle as much of the liquid as possible in varying ways. This function can
         * be used when the action can be canceled, which implies the liquid can be put back
         * to wherever it came from and is *not* lost if the player cancels the action.
         * It returns when all liquid has been handled or if the player has explicitly canceled
         * the action (use the charges count to distinguish).
         * @return Whether any of the liquid has been consumed. `false` indicates the player has
         * declined all options to handle the liquid and no charges of the liquid have been transferred.
         * `true` indicates some charges have been transferred (but not necessarily all of them).
         */
        bool consume_liquid( item &liquid, int radius = 0 );

        /**
         * Handle finite liquid from ground. The function also handles consuming move points.
         * This may start a player activity.
         * @param on_ground Iterator to the item on the ground. Must be valid and point to an
         * item in the stack at `m.i_at(pos)`
         * @param pos The position of the item on the map.
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */
        bool handle_liquid_from_ground( std::list<item>::iterator on_ground, const tripoint &pos,
                                        int radius = 0 );

        /**
         * Handle liquid from inside a container item. The function also handles consuming move points.
         * @param in_container Iterator to the liquid. Must be valid and point to an
         * item in the @ref item::contents of the container.
         * @param container Container of the liquid
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */
        bool handle_liquid_from_container( std::list<item>::iterator in_container, item &container,
                                           int radius = 0 );
        /**
         * Shortcut to the above: handles the first item in the container.
         */
        bool handle_liquid_from_container( item &container, int radius = 0 );

        /**
         * This may start a player activity if either \p source_pos or \p source_veh is not
         * null.
         * The function consumes moves of the player as needed.
         * Supply one of the source parameters to prevent the player from pouring the liquid back
         * into that "container". If no source parameter is given, the liquid must not be in a
         * container at all (e.g. freshly crafted, or already removed from the container).
         * @param liquid The actual liquid
         * @param source The container that currently contains the liquid.
         * @param radius Radius to look for liquid around pos
         * @param source_pos The source of the liquid when it's from the map.
         * @param source_veh The vehicle that currently contains the liquid in its tank.
         * @return Whether the user has handled the liquid (at least part of it). `false` indicates
         * the user has rejected all possible actions. But note that `true` does *not* indicate any
         * liquid was actually consumed, the user may have chosen an option that turned out to be
         * invalid (chose to fill into a full/unsuitable container).
         * Basically `false` indicates the user does not *want* to handle the liquid, `true`
         * indicates they want to handle it.
         */
        bool handle_liquid( item &liquid, item *source = nullptr, int radius = 0,
                            const tripoint *source_pos = nullptr,
                            const vehicle *source_veh = nullptr, const int part_num = -1,
                            const monster *source_mon = nullptr );
        /**
             * These are helper functions for transfer liquid, for times when you just want to
             * get the target of the transfer, or know the target and just want to transfer the
             * liquid. They take the same arguments as handle_liquid, plus
             * @param target structure containing information about the target
             */
        bool get_liquid_target( item &liquid, item *const source, const int radius,
                                const tripoint *source_pos, const vehicle *const source_veh,
                                const monster *const source_mon, liquid_dest_opt &target );
        bool perform_liquid_transfer( item &liquid,
                                      const tripoint *source_pos,
                                      const vehicle *const source_veh, const int part_num,
                                      const monster *const source_mon, liquid_dest_opt &target );
        /**@}*/

        void open_gate( const tripoint &p );

        // Knockback functions: knock target at t along a line, either calculated
        // from source position s using force parameter or passed as an argument;
        // force determines how far target is knocked, if trajectory is calculated
        // force also determines damage along with dam_mult;
        // stun determines base number of turns target is stunned regardless of impact
        // stun == 0 means no stun, stun == -1 indicates only impact stun (wall or npc/monster)
        void knockback( const tripoint &s, const tripoint &t, int force, int stun, int dam_mult );
        void knockback( std::vector<tripoint> &traj, int force, int stun, int dam_mult );

        // shockwave applies knockback to all targets within radius of p
        // parameters force, stun, and dam_mult are passed to knockback()
        // ignore_player determines if player is affected, useful for bionic, etc.
        void shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult,
                        bool ignore_player );

        // Animation related functions
        void draw_explosion( const tripoint &p, int radius, const nc_color &col );
        void draw_custom_explosion( const tripoint &p, const std::map<tripoint, nc_color> &area );
        void draw_bullet( const tripoint &pos, int i, const std::vector<tripoint> &trajectory,
                          char bullet );
        void draw_hit_mon( const tripoint &p, const monster &critter, bool dead = false );
        void draw_hit_player( const player &p, int dam );
        void draw_line( const tripoint &p, const tripoint &center_point, const std::vector<tripoint> &ret );
        void draw_line( const tripoint &p, const std::vector<tripoint> &ret );
        void draw_weather( const weather_printable &wPrint );
        void draw_sct();
        void draw_zones( const tripoint &start, const tripoint &end, const tripoint &offset );
        // Draw critter (if visible!) on its current position into w_terrain.
        // @param center the center of view, same as when calling map::draw
        void draw_critter( const Creature &critter, const tripoint &center );
        void draw_cursor( const tripoint &p );

        bool is_in_viewport( const tripoint &p, int margin = 0 ) const;
        /**
         * Check whether movement is allowed according to safe mode settings.
         * @return true if the movement is allowed, otherwise false.
         */
        bool check_safe_mode_allowed( bool repeat_safe_mode_warnings = true );
        void set_safe_mode( safe_mode_type mode );

        bool narrow_sidebar;
        bool right_sidebar;
        bool fullscreen;
        bool was_fullscreen;

        /** open vehicle interaction screen */
        void exam_vehicle( vehicle &veh, int cx = 0, int cy = 0 );

        // Forcefully close a door at p.
        // The function checks for creatures/items/vehicles at that point and
        // might kill/harm/destroy them.
        // If there still remains something that prevents the door from closing
        // (e.g. a very big creatures, a vehicle) the door will not be closed and
        // the function returns false.
        // If the door gets closed the terrain at p is set to door_type and
        // true is returned.
        // bash_dmg controls how much damage the door does to the
        // creatures/items/vehicle.
        // If bash_dmg is 0 or smaller, creatures and vehicles are not damaged
        // at all and they will prevent the door from closing.
        // If bash_dmg is smaller than 0, _every_ item on the door tile will
        // prevent the door from closing. If bash_dmg is 0, only very small items
        // will do so, if bash_dmg is greater than 0, items won't stop the door
        // from closing at all.
        // If the door gets closed the items on the door tile get moved away or destroyed.
        bool forced_door_closing( const tripoint &p, const ter_id &door_type, int bash_dmg );

        //pixel minimap management
        int pixel_minimap_option;

        /** Attempt to load first valid save (if any) in world */
        bool load( const std::string &world );

    private:
        // Game-start procedures
        void load( const save_t &name ); // Load a player-specific save file
        void load_master(); // Load the master data file, with factions &c
        void load_weather( std::istream &fin );
#ifdef __ANDROID__
        void load_shortcuts( std::istream &fin );
#endif
        bool start_game(); // Starts a new game in the active world

        //private save functions.
        // returns false if saving failed for whatever reason
        bool save_factions_missions_npcs();
        void serialize_master( std::ostream &fout );
        // returns false if saving failed for whatever reason
        bool save_artifacts();
        // returns false if saving failed for whatever reason
        bool save_maps();
        void save_weather( std::ostream &fout );
#ifdef __ANDROID__
        void save_shortcuts( std::ostream &fout );
#endif
        // Data Initialization
        void init_autosave();     // Initializes autosave parameters
        void init_lua();          // Initializes lua interpreter.
        void create_starting_npcs(); // Creates NPCs that start near you

        // V Menu Functions and helpers:
        void list_items_monsters(); // Called when you invoke the `V`-menu

        enum class vmenu_ret : int {
            CHANGE_TAB,
            QUIT,
            FIRE, // Who knew, apparently you can do that in list_monsters
        };

        game::vmenu_ret list_items( const std::vector<map_item_stack> &item_list );
        std::vector<map_item_stack> find_nearby_items( int iRadius );
        void reset_item_list_state( const catacurses::window &window, int height, bool bRadiusSort );
        std::string sFilter; // this is a member so that it's remembered over time
        std::string list_item_upvote;
        std::string list_item_downvote;

        game::vmenu_ret list_monsters( const std::vector<Creature *> &monster_list );

        /** Check for dangerous stuff at dest_loc, return false if the player decides
        not to step there */
        bool prompt_dangerous_tile( const tripoint &dest_loc ) const;
        /** Returns true if the menu handled stuff and player shouldn't do anything else */
        bool npc_menu( npc &who );
        // Standard movement; handles attacks, traps, &c. Returns false if auto move
        // should be canceled
        bool plmove( int dx, int dy, int dz = 0 );
        // Handle pushing during move, returns true if it handled the move
        bool grabbed_move( const tripoint &dp );
        bool grabbed_veh_move( const tripoint &dp );
        bool grabbed_furn_move( const tripoint &dp );
        // Handle moving from a ramp
        bool ramp_move( const tripoint &dest );
        // Handle phasing through walls, returns true if it handled the move
        bool phasing_move( const tripoint &dest );
        // Regular movement. Returns false if it failed for any reason
        bool walk_move( const tripoint &dest );
        void on_move_effects();

        void control_vehicle(); // Use vehicle controls  '^'
        void examine( const tripoint &p );// Examine nearby terrain  'e'
        void examine();

        void drop(); // Drop an item  'd'
        void drop_in_direction(); // Drop w/ direction  'D'

        void butcher(); // Butcher a corpse  'B'
        void eat( int pos = INT_MIN ); // Eat food or fuel  'E' (or 'a')
        void use_item( int pos = INT_MIN ); // Use item; also tries E,R,W  'a'

        void change_side( int pos = INT_MIN ); // Change the side on which an item is worn 'c'
        void reload(); // Reload a wielded gun/tool  'r'
        void reload( int pos, bool prompt = false );
        void reload( item_location &loc, bool prompt = false );
        void mend( int pos = INT_MIN );
        void autoattack();
    public:
        // Places the player at the specified point; hurts feet, lists items etc.
        void place_player( const tripoint &dest );
        void place_player_overmap( const tripoint &om_dest );

        bool unload( item &it ); // Unload a gun/tool  'U'
        void unload( int pos = INT_MIN );

        unsigned int get_seed() const;

        /** If invoked, NPCs will be reloaded before next turn. */
        void set_npcs_dirty();
        /** If invoked, dead will be cleaned this turn. */
        void set_critter_died();
    private:
        void wield();
        void wield( int pos ); // Wield a weapon  'w'
        void wield( item_location &loc );

        void chat(); // Talk to a nearby NPC  'C'
        void plthrow( int pos = INT_MIN,
                      const cata::optional<tripoint> &blind_throw_from_pos = cata::nullopt ); // Throw an item  't'

        // Internal methods to show "look around" info
        void print_fields_info( const tripoint &lp, const catacurses::window &w_look, int column,
                                int &line );
        void print_terrain_info( const tripoint &lp, const catacurses::window &w_look,
                                 const std::string &area_name, int column,
                                 int &line );
        void print_trap_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                              int &line );
        void print_creature_info( const Creature *creature, const catacurses::window &w_look, int column,
                                  int &line );
        void print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                                 int column, int &line, int last_line );
        void print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                    visibility_type visibility );
        void print_visibility_indicator( visibility_type visibility );
        void print_items_info( const tripoint &lp, const catacurses::window &w_look, int column, int &line,
                               int last_line );
        void print_graffiti_info( const tripoint &lp, const catacurses::window &w_look, int column,
                                  int &line, int last_line );
        void get_lookaround_dimensions( int &lookWidth, int &begin_y, int &begin_x ) const;

        input_context get_player_input( std::string &action );

        // Map updating and monster spawning
        void replace_stair_monsters();
        void update_stair_monsters();
        /**
         * Shift all active monsters, the shift vector (x,y,z) is the number of
         * shifted submaps. Monsters that are outside of the reality bubble after
         * shifting are despawned.
         * Note on z-levels: this works with vertical shifts, but currently all
         * monsters are despawned upon a vertical shift.
         */
        void shift_monsters( const int shiftx, const int shifty, const int shiftz );
        /**
         * Despawn a specific monster, it's stored on the overmap. Also removes
         * it from the creature tracker. Keep in mind that any monster index may
         * point to a different monster after calling this (or to no monster at all).
         */
        void despawn_monster( monster &critter );

        void perhaps_add_random_npc();
        void rebuild_mon_at_cache();

        // Routine loop functions, approximately in order of execution
        void cleanup_dead();     // Delete any dead NPCs/monsters
        void monmove();          // Monster movement
        void process_activity(); // Processes and enacts the player's activity
        void update_weather();   // Updates the temperature and weather patten
        int  mon_info( const catacurses::window & ); // Prints a list of nearby monsters
        void handle_key_blocking_activity(); // Abort reading etc.
        bool handle_action();
        bool try_get_right_click_action( action_id &act, const tripoint &mouse_target );
        bool try_get_left_click_action( action_id &act, const tripoint &mouse_target );

        void item_action_menu(); // Displays item action menu

        /**
         * If there is a robot (that can be disabled), query the player
         * and try to disable it.
         * @return true if the robot has been disabled or a similar action has
         * been done. false if the player did not choose any action and the function
         * has effectively done nothing.
         */
        bool disable_robot( const tripoint &p );

        bool is_game_over();     // Returns true if the player quit or died
        void death_screen();     // Display our stats, "GAME OVER BOO HOO"
        void draw_minimap();     // Draw the 5x5 minimap
        /** Draws the sidebar (if it's visible), including all windows there */
        void draw_sidebar();
    public:
        void draw_sidebar_messages();
    private:
        void draw_pixel_minimap();  // Draws the pixel minimap based on the player's current location

        //  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
        void autosave();         // automatic quicksaves - Performs some checks before calling quicksave()
    public:
        void quicksave();        // Saves the game without quitting
    private:
        void quickload();        // Loads the previously saved game if it exists

        // Input related
        // Handles box showing items under mouse
        bool handle_mouseview( input_context &ctxt, std::string &action );

        // On-request draw functions
        void disp_kills();          // Display the player's kill counts
        void disp_faction_ends();   // Display the faction endings
        void disp_NPC_epilogues();  // Display NPC endings
        void disp_NPCs();           // Currently UNUSED.  Lists global NPCs.
        void list_missions();       // Listed current, completed and failed missions (mission_ui.cpp)

        // Debug functions
        void debug();           // All-encompassing debug screen.  TODO: This.
        void display_scent();   // Displays the scent map

        // ########################## DATA ################################

        safe_mode_type safe_mode;
        bool safe_mode_warning_logged;
        std::vector<std::shared_ptr<monster>> new_seen_mon;
        int mostseen;  // # of mons seen last turn; if this increases, set safe_mode to SAFE_MODE_STOP
        int turnssincelastmon; // needed for auto run mode
        //  quit_status uquit;    // Set to true if the player quits ('Q')
        bool bVMonsterLookFire;
        time_point nextweather; // The time on which weather will shift next.
        int next_npc_id, next_mission_id; // Keep track of UIDs
        std::map<mtype_id, int> kills;         // Player's kill count
        std::list<std::string> npc_kills;      // names of NPCs the player killed
        int moves_since_last_save;
        time_t last_save_timestamp;
        mutable std::array<float, OVERMAP_LAYERS> latest_lightlevels;
        // remoteveh() cache
        time_point remoteveh_cache_time;
        vehicle *remoteveh_cache;
        /** temperature cache, cleared every turn, sparse map of map tripoints to temperatures */
        std::unordered_map< tripoint, int > temperature_cache;
        /** Has a NPC been spawned since last load? */
        bool npcs_dirty;
        /** Has anything died in this turn and needs to be cleaned up? */
        bool critter_died;
        /** Was the player sleeping during this turn. */
        bool player_was_sleeping;
        /** Is Zone manager open or not - changes graphics of some zone tiles */
        bool zones_manager_open = false;

        std::unique_ptr<special_game> gamemode;

        int user_action_counter; // Times the user has input an action

        /** How far the tileset should be zoomed out, 16 is default. 32 is zoomed in by x2, 8 is zoomed out by x0.5 */
        int tileset_zoom;

        /** Seed for all the random numbers that should have consistent randomness (weather). */
        unsigned int seed;

        // Preview for auto move route
        std::vector<tripoint> destination_preview;

        Creature *is_hostile_within( int distance );

        void move_save_to_graveyard();
        bool save_player_data();
    public:
        cata::optional<int> wind_direction_override;
        cata::optional<int> windspeed_override;
        weather_type weather_override;
};

// Returns temperature modifier from direct heat radiation of nearby sources
// @param location Location affected by heat sources
// @param direct forces return of heat intensity (and not temperature modifier) of
// adjacent hottest heat source
int get_heat_radiation( const tripoint &location, bool direct );
// Returns temperature modifier from hot air fields of given location
int get_convection_temperature( const tripoint &location );

#endif
