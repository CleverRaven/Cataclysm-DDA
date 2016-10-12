#ifndef GAME_H
#define GAME_H

#include "enums.h"
#include "game_constants.h"
#include "calendar.h"
#include "posix_time.h"
#include "int_id.h"
#include "item_location.h"
#include "cursesdef.h"

#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <stdarg.h>

extern const int savegame_version;
extern int save_loading_version;

extern bool test_mode;

// The reference to the one and only game instance.
class game;
extern game *g;

#ifdef TILES
extern void try_sdl_update();
#endif // TILES

extern bool trigdist;
extern bool use_tiles;
extern bool fov_3d;
extern bool tile_iso;

extern const int core_version;

extern const int savegame_version;
extern int savegame_loading_version;

enum class dump_mode {
    TSV,
    HTML
};

enum tut_type {
    TUT_NULL,
    TUT_BASIC, TUT_COMBAT,
    TUT_MAX
};

enum input_ret {
    IR_GOOD,
    IR_BAD,
    IR_TIMEOUT
};

enum quit_status {
    QUIT_NO = 0,    // Still playing
    QUIT_MENU,      // Quit at the menu
    QUIT_SUICIDE,   // Quit with 'Q'
    QUIT_SAVED,     // Saved and quit
    QUIT_DIED,      // Actual death
    QUIT_WATCH,     // Died, and watching aftermath
    QUIT_ERROR
};

enum safe_mode_type {
    SAFE_MODE_OFF = 0, // Moving always allowed
    SAFE_MODE_ON = 1, // Moving allowed, but if a new monsters spawns, go to SAFE_MODE_STOP
    SAFE_MODE_STOP = 2, // New monsters spotted, no movement allowed
};

enum target_mode {
    TARGET_MODE_FIRE,
    TARGET_MODE_THROW,
    TARGET_MODE_TURRET,
    TARGET_MODE_TURRET_MANUAL,
    TARGET_MODE_REACH
};

enum activity_type : int;
enum body_part : int;
enum weather_type : int;
enum action_id : int;

struct special_game;
struct itype;
struct mtype;
using mtype_id = string_id<mtype>;
using itype_id = std::string;
class ammunition_type;
using ammotype = string_id<ammunition_type>;
class mission;
class map;
class Creature;
class Character;
class player;
class npc;
class monster;
class vehicle;
class Creature_tracker;
class calendar;
class scenario;
class DynamicDataLoader;
class salvage_actor;
class input_context;
class map_item_stack;
struct WORLD;
typedef WORLD *WORLDPTR;
class overmap;
struct event;
enum event_type : int;
struct vehicle_part;
struct ter_t;
using ter_id = int_id<ter_t>;
class weather_generator;
struct weather_printable;
class faction;
class live_view;
typedef int nc_color;
struct w_point;
struct explosion_data;
struct visibility_variables;
class scent_map;

// Note: this is copied from inventory.h
// Entire inventory.h would also bring item.h here
typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;
typedef std::vector< const std::list<item>* > const_invslice;
typedef std::vector< std::pair<std::list<item>*, int> > indexed_invslice;

typedef std::function<bool( const item & )> item_filter;
typedef std::function<bool( const item_location & )> item_location_filter;

class game
{
        friend class editmap;
        friend class advanced_inventory;
    public:
        game();
        ~game();

        /** Loads static data that does not depend on mods or similar. */
        void load_static_data();

        /** Loads core dynamic data. May throw. */
        void load_core_data();

        /**
         *  Check if mods can be sucessfully loaded
         *  @param opts check specific mods (or all if unspecified)
         *  @return whether all mods were successfully loaded
         */
        bool check_mod_data( const std::vector<std::string> &opts );

        /** Loads core data and mods from the given world. May throw. */
        void load_world_modfiles(WORLDPTR world);
    protected:
        /** Loads dynamic data from the given directory. May throw. */
        void load_data_from_dir( const std::string &path, const std::string &src );


        // May be a bit hacky, but it's probably better than the header spaghetti
        std::unique_ptr<map> map_ptr;
        std::unique_ptr<player> u_ptr;
        std::unique_ptr<live_view> liveview_ptr;
        live_view& liveview;
        std::unique_ptr<scent_map> scent_ptr;
    public:

        /** Initializes the UI. */
        void init_ui();
        void setup();
        /** Returns true if we actually quit the game. Used in main.cpp. */
        bool game_quit();
        /** Returns true if the game quits through some error. */
        bool game_error();
        /** True if the game has just started or loaded, else false. */
        bool new_game;
        /** Used in main.cpp to determine what type of quit is being performed. */
        quit_status uquit;
        /** Saving and loading functions. */
        void serialize(std::ostream &fout);  // for save
        void unserialize(std::istream &fin);  // for load
        bool unserialize_legacy(std::istream &fin);  // for old load
        void unserialize_master(std::istream &fin);  // for load
        bool unserialize_master_legacy(std::istream &fin);  // for old load

        /** write statisics to stdout and @return true if sucessful */
        bool dump_stats( const std::string& what, dump_mode mode, const std::vector<std::string> &opts );

        /** Returns false if saving failed. */
        bool save();
        /** Deletes the given world. If delete_folder is true delete all the files and directories
         *  of the given world folder. Else just avoid deleting the two config files and the directory
         *  itself. */
        void delete_world(std::string worldname, bool delete_folder);
        /** Returns a list of currently active character saves. */
        std::vector<std::string> list_active_characters();
        void write_memorial_file(std::string sLastWords);
        bool cleanup_at_end();
        void start_calendar();
        /** MAIN GAME LOOP. Returns true if game is over (death, saved, quit, etc.). */
        bool do_turn();
        void draw();
        void draw_ter( bool draw_sounds = true );
        void draw_ter( const tripoint &center, bool looking = false, bool draw_sounds = true );
        /**
         * Returns the location where the indicator should go relative to the reality bubble,
         * or tripoint_min to indicate no indicator should be drawn.
         * Based on the vehicle the player is driving, if any.
         * @param next If true, bases it on the vehicle the vehicle will turn to next turn,
         * instead of the one it is currently facing.
         */
        tripoint get_veh_dir_indicator_location( bool next ) const;
        void draw_veh_dir_indicator( bool next );

        /** Make map a reference here, to avoid map.h in game.h */
        map &m;
        player &u;
        scent_map &scent;

        std::unique_ptr<Creature_tracker> critter_tracker;
        /**
         * Add an entry to @ref events. For further information see event.h
         * @param type Type of event.
         * @param on_turn On which turn event should be happened.
         * @param faction_id Faction of event.
         * @param where The location of the event, optional, defaults to the center of the
         * reality bubble. In global submap coordinates.
         */
        void add_event(event_type type, int on_turn, int faction_id = -1);
        void add_event(event_type type, int on_turn, int faction_id, tripoint where);
        bool event_queued(event_type type) const;
        /** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */
        std::unordered_map<tripoint, std::pair<int, int>> explosion(
            const tripoint &p, float power, float factor = 0.8f,
            bool fire = false, int shrapnel_count = 0, int shrapnel_mass = 10
        );

        std::unordered_map<tripoint, std::pair<int, int>> explosion(
            const tripoint &p, const explosion_data &ex
        );

        /** Helper for explosion, does the actual blast. */
        void do_blast( const tripoint &p, float power, float factor, bool fire );

        /*
         * Emits shrapnel damaging creatures and sometimes terrain/furniture within range
         * @param src source from which shrapnel radiates outwards in a uniformly random distribtion
         * @param power raw kinetic energy which is responsible for damage and reduced by effects of cover
         * @param count abritrary measure of quantity shrapnel emitted affecting number of hits
         * @param mass determines how readily terrain constrains shrapnel and also caps pierce damage
         * @param range maximum distance shrapnel may travel
         * @return map containing all tiles considered with value being sum of damage received (if any)
         */
        std::unordered_map<tripoint,int> shrapnel( const tripoint &src, int power, int count, int mass, int range = -1 );

        /** Triggers a flashbang explosion at p. */
        void flashbang( const tripoint &p, bool player_immune = false );
        /** Moves the player vertically. If force == true then they are falling. */
        void vertical_move(int z, bool force);
        /** Returns the other end of the stairs (if any), otherwise tripoint_min. May query, affect u etc.  */
        tripoint find_or_make_stairs( map &mp, int z_after, bool &rope_ladder );
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
        /** Triggers an emp blast at p. */
        void emp_blast( const tripoint &p );
        /** Returns the NPC index of the npc at p. Returns -1 if no NPC is present. */
        int  npc_at( const tripoint &p ) const;
        /** Returns the NPC index of the npc with a matching ID. Returns -1 if no NPC is present. */
        int  npc_by_id(const int id) const;
        /** Returns the Creature at tripoint p */
        Creature *critter_at( const tripoint &p, bool allow_hallucination = false );
        Creature const* critter_at( const tripoint &p, bool allow_hallucination = false ) const;

        /** Summons a brand new monster at the current time. Returns the summoned monster. */
        bool summon_mon( const mtype_id& id, const tripoint &p );
        /** Calls the creature_tracker add function. Returns true if successful. */
        bool add_zombie(monster &critter);
        bool add_zombie(monster &critter, bool pin_upgrade);
        /** Returns the number of creatures through the creature_tracker size() function. */
        size_t num_zombies() const;
        /** Returns the monster with match index. Redirects to the creature_tracker find() function. */
        monster &zombie(const int idx);
        /** Redirects to the creature_tracker update_pos() function. */
        bool update_zombie_pos( const monster &critter, const tripoint &pos );
        void remove_zombie(const int idx);
        /** Redirects to the creature_tracker clear() function. */
        void clear_zombies();
        /** Spawns a hallucination close to the player. */
        bool spawn_hallucination();
        /** Swaps positions of two creatures */
        bool swap_critters( Creature &first, Creature &second );

        /** Returns the monster index of the monster at the given tripoint. Returns -1 if no monster is present. */
        int mon_at( const tripoint &p, bool allow_hallucination = false ) const;
        /** Returns a pointer to the monster at the given tripoint. */
        monster *monster_at( const tripoint &p, bool allow_hallucination = false );
        /** Returns true if there is no player, NPC, or monster on the tile and move_cost > 0. */
        bool is_empty( const tripoint &p );
        /** Returns true if p is outdoors and it is sunny. */
        bool is_in_sunlight( const tripoint &p );
        /** Returns true if p is indoors, underground, or in a car. */
        bool is_sheltered( const tripoint &p );
        /**
         * Revives a corpse at given location. The monster type and some of its properties are
         * deducted from the corpse. If reviving succeeds, the location is guaranteed to have a
         * new monster there (see @ref mon_at).
         * @param location The place where to put the revived monster.
         * @param corpse The corpse item, it must be a valid corpse (see @ref item::is_corpse).
         * @return Whether the corpse has actually been redivided. Reviving may fail for many
         * reasons, including no space to put the monster, corpse being to much damaged etc.
         * If the monster was revived, the caller should remove the corpse item.
         * If reviving failed, the item is unchanged, as is the environment (no new monsters).
         */
        bool revive_corpse( const tripoint &location, item &corpse );

        /**
         *  Handles interactive parts of gun firing (target selection, etc.).
         *  @return whether an attack was actually performed
         */
        bool plfire();

        /** Target is an interactive function which allows the player to choose a nearby
         *  square.  It display information on any monster/NPC on that square, and also
         *  returns a Bresenham line to that square.  It is called by plfire(),
         *  throw() and vehicle::aim_turrets() */
        std::vector<tripoint> target( tripoint src, tripoint dst, int range,
                                      std::vector<Creature *> t, int target,
                                      item *relevant, target_mode mode );

        /**
         * Targetting UI callback is passed the item being targeted (if any)
         * and should return pointer to effective ammo data (if any)
         */
        using target_callback = std::function<const itype *(item *obj)>;

        /**
         *  Prompts for target and returns trajectory to it
         *  @param relevant active item (if any)
         *  @param ammo effective ammo data (derived from @param relevant if unspecified)
         *  @param on_mode_change callback when user attempts changing firing mode
         *  @param on_ammo_change callback when user attempts changing ammo
         */
        std::vector<tripoint> pl_target_ui( target_mode mode, item *relevant, int range,
                                            const itype *ammo = nullptr,
                                            const target_callback &on_mode_change = target_callback(),
                                            const target_callback &on_ammo_change = target_callback() );

        /** Redirects to player::cancel_activity(). */
        void cancel_activity();
        /** Asks if the player wants to cancel their activity, and if so cancels it. */
        bool cancel_activity_query(const char *message, ...);
        /** Asks if the player wants to cancel their activity and if so cancels it. Additionally checks
         *  if the player wants to ignore further distractions. */
        bool cancel_activity_or_ignore_query(const char *reason, ...);
        /** Handles players exiting from moving vehicles. */
        void moving_vehicle_dismount( const tripoint &p );

        /** Returns the current remotely controlled vehicle. */
        vehicle *remoteveh();
        /** Sets the current remotely controlled vehicle. */
        void setremoteveh(vehicle *veh);

        /** Returns the next available mission id. */
        int assign_mission_id();
        npc *find_npc(int id);
        /** Makes any nearby NPCs on the overmap active. */
        void load_npcs();
        /** Unloads all NPCs */
        void unload_npcs();
        /** Unloads, then loads the NPCs */
        void reload_npcs();
        /** Pulls the NPCs that were dumped into the world map on save back into mission_npcs */
        void load_mission_npcs();
        /** Returns the number of kills of the given mon_id by the player. */
        int kill_count( const mtype_id& id );
        /** Increments the number of kills of the given mtype_id by the player upwards. */
        void increase_kill_count( const mtype_id& id );

        /** Performs a random short-distance teleport on the given player, granting teleglow if needed. */
        void teleport(player *p = NULL, bool add_teleglow = true);
        /** Handles swimming by the player. Called by plmove(). */
        void plswim( const tripoint &p );
        /** Picks and spawns a random fish from the remaining fish list when a fish is caught. */
        void catch_a_monster(std::vector<monster*> &catchables, const tripoint &pos, player *p, int catch_duration = 0);
        /** Returns the list of currently fishable monsters within distance of the player. */
        std::vector<monster*> get_fishable(int distance);
        /** Flings the input creature in the given direction. */
        void fling_creature(Creature *c, const int &dir, float flvel, bool controlled = false);

        /** Nuke the area at p - global overmap terrain coordinates! */
        void nuke( const tripoint &p );
        bool spread_fungus( const tripoint &p );
        std::vector<faction *> factions_at( const tripoint &p );
        float natural_light_level( int zlev ) const;
        /** Returns coarse number-of-squares of visibility at the current light level.
         * Used by monster and NPC AI.
         */
        unsigned char light_level( int zlev ) const;
        void reset_light_level();
        int assign_npc_id();
        int assign_faction_id();
        faction *faction_by_ident(std::string ident);
        Creature *is_hostile_nearby();
        Creature *is_hostile_very_close();
        void refresh_all();
        // Handles shifting coordinates transparently when moving between submaps.
        // Helper to make calling with a player pointer less verbose.
        void update_map( player &p );
        void update_map(int &x, int &y);
        void update_overmap_seen(); // Update which overmap tiles we can see

        void process_artifact(item *it, player *p);
        void add_artifact_messages(std::vector<art_effect_passive> effects);

        void peek();
        void peek( const tripoint &p );
        tripoint look_debug();

        bool check_zone( const std::string &type, const tripoint &where ) const;
        void zones_manager();
        void zones_manager_shortcuts(WINDOW *w_info);
        void zones_manager_draw_borders(WINDOW *w_border, WINDOW *w_info_border, const int iInfoHeight,
                                        const int width);
        // Look at nearby terrain ';', or select zone points
        tripoint look_around();
        tripoint look_around( WINDOW *w_info, const tripoint &start_point,
                              bool has_first_point, bool select_zone );

        void list_items_monsters();
        int list_items(const int iLastState); //List all items around the player
        int list_monsters(const int iLastState); //List all monsters around the player
        // Shared method to print "look around" info
        void print_all_tile_info( const tripoint &lp, WINDOW *w_look, int column, int &line,
                                  int last_line, bool draw_terrain_indicators,
                                  const visibility_variables &cache );

        std::vector<map_item_stack> find_nearby_items(int iRadius);
        void draw_item_filter_rules(WINDOW *window, int rows);
        std::string ask_item_priority_high(WINDOW *window, int rows);
        std::string ask_item_priority_low(WINDOW *window, int rows);
        void draw_trail_to_square( const tripoint &t, bool bDrawX );
        void reset_item_list_state(WINDOW *window, int height, bool bRadiusSort);
        std::string sFilter; // this is a member so that it's remembered over time
        std::string list_item_upvote;
        std::string list_item_downvote;

        item *inv_map_for_liquid(const item &liquid, const std::string &title, int radius = 0);

        void interactive_inv();

        int inv_for_filter( const std::string &title, item_filter filter, const std::string &none_message = "" );
        int inv_for_filter( const std::string &title, item_location_filter filter, const std::string &none_message = "" );

        int inv_for_all( const std::string &title, const std::string &none_message = "" );
        int inv_for_activatables( const player &p, const std::string &title );
        int inv_for_flag( const std::string &flag, const std::string &title );
        int inv_for_id( const itype_id &id, const std::string &title );
        int inv_for_tools_powered_by( const ammotype &battery_id, const std::string &title );
        int inv_for_equipped( const std::string &title );
        int inv_for_unequipped( const std::string &title );

        enum inventory_item_menu_positon {
            RIGHT_TERMINAL_EDGE,
            LEFT_OF_INFO,
            RIGHT_OF_INFO,
            LEFT_TERMINAL_EDGE,
        };
        int inventory_item_menu(int pos, int startx = 0, int width = 50, inventory_item_menu_positon position = RIGHT_OF_INFO);

        item_location inv_map_splice( item_filter filter, const std::string &title, int radius = 0,
                                      const std::string &none_message = "" );
        item_location inv_map_splice( item_location_filter filter, const std::string &title, int radius = 0,
                                      const std::string &none_message = "" );

        // Select items to drop.  Returns a list of pairs of position, quantity.
        std::list<std::pair<int, int>> multidrop();
        faction *list_factions(std::string title = "FACTIONS:");

        bool has_gametype() const;
        special_game_id gametype() const;

        void toggle_sidebar_style(void);
        void toggle_fullscreen(void);
        void toggle_pixel_minimap(void);
        void temp_exit_fullscreen(void);
        void reenter_fullscreen(void);
        void zoom_in();
        void zoom_out();
        void reset_zoom();
        int get_user_action_counter() const;

        signed char temperature;              // The air temperature
        int get_temperature();    // Returns outdoor or indoor temperature of current location
        weather_type weather;   // Weather pattern--SEE weather.h
        bool lightning_active;
        std::unique_ptr<w_point> weather_precise; // Cached weather data

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
        void load_map( tripoint pos_sm );
        /**
         * The overmap which contains the center submap of the reality bubble.
         */
        overmap &get_cur_om() const;
        const weather_generator &get_cur_weather_gen() const;
        const scenario *scen;
        std::vector<monster> coming_to_stairs;
        int monstairz;
        std::vector<npc *> active_npc;
        std::vector<npc *> mission_npc;
        std::vector<faction> factions;
        int weight_dragged; // Computed once, when you start dragging

        int ter_view_x, ter_view_y, ter_view_z;
        WINDOW *w_terrain;
        WINDOW *w_overmap;
        WINDOW *w_omlegend;
        WINDOW *w_minimap;
        WINDOW *w_pixel_minimap;
        WINDOW *w_HP;
        //only a pointer, can refer to w_messages_short or w_messages_long
        WINDOW *w_messages;
        WINDOW *w_messages_short;
        WINDOW *w_messages_long;
        WINDOW *w_location;
        WINDOW *w_status;
        WINDOW *w_status2;
        WINDOW *w_blackspace;

        // View offset based on the driving speed (if any)
        // that has been added to u.view_offset,
        // Don't write to this directly, always use set_driving_view_offset
        point driving_view_offset;
        // Setter for driving_view_offset
        void set_driving_view_offset(const point &p);
        // Calculates the driving_view_offset for the given vehicle
        // and sets it (view set_driving_view_offset), if
        // the options for this feautre is dactivated or if veh is NULL,
        // the function set the driving offset to (0,0)
        void calc_driving_offset(vehicle *veh = NULL);

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
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */
        bool handle_liquid_from_ground( std::list<item>::iterator on_ground, const tripoint &pos, int radius = 0 );

        /**
         * Handle liquid from inside a container item. The function also handles consuming move points.
         * @param in_container Iterator to the liquid. Must be valid and point to an
         * item in the @ref item::contents of the container.
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */
        bool handle_liquid_from_container( std::list<item>::iterator in_container, item &container, int radius = 0 );
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
         * @param source The container that currently contains the liquid.
         * @param source_pos The source of the liquid when it's from the map.
         * @param source_veh The vehicle that currently contains the liquid in its tank.
         * @return Whether the user has handled the liquid (at least part of it). `false` indicates
         * the user has rejected all possible actions. But note that `true` does *not* indicate any
         * liquid was actually consumed, the user may have chosen an option that turned out to be
         * invalid (chose to fill into a full/unsuitable container).
         * Basically `false` indicates the user does not *want* to handle the liquid, `true`
         * indicates they want to handle it.
         */
        bool handle_liquid( item &liquid, item *source = NULL, int radius = 0,
                            const tripoint *source_pos = nullptr,
                            const vehicle *source_veh = nullptr );
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
        void shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult, bool ignore_player );

        // Animation related functions
        void draw_explosion( const tripoint &p, int radius, nc_color col );
        void draw_custom_explosion( const tripoint &p, const std::map<tripoint, nc_color> &area );
        void draw_bullet( Creature const &p, const tripoint &pos, int i,
                          std::vector<tripoint> const &trajectory, char bullet );
        void draw_hit_mon( const tripoint &p, const monster &critter, bool dead = false);
        void draw_hit_player(player const &p, int dam);
        void draw_line( const tripoint &p, const tripoint &center_point, std::vector<tripoint> const &ret );
        void draw_line( const tripoint &p, std::vector<tripoint> const &ret);
        void draw_weather(weather_printable const &wPrint);
        void draw_sct();
        void draw_zones(const tripoint &start, const tripoint &end, const tripoint &offset);
        // Draw critter (if visible!) on its current position into w_terrain.
        // @param center the center of view, same as when calling map::draw
        void draw_critter( const Creature &critter, const tripoint &center );

        bool opening_screen();// Warn about screen size, then present the main menu
        void mmenu_refresh_title();
        void mmenu_refresh_motd();
        void mmenu_refresh_credits();

        /**
         * Check whether movement is allowed according to safe mode settings.
         * @return true if the movement is allowed, otherwise false.
         */
        bool check_safe_mode_allowed( bool repeat_safe_mode_warnings = true );
        void set_safe_mode( safe_mode_type mode );

        const int dangerous_proximity;
        bool narrow_sidebar;
        bool right_sidebar;
        bool fullscreen;
        bool was_fullscreen;

        /** open vehicle interaction screen */
        void exam_vehicle(vehicle &veh, int cx = 0, int cy = 0);

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
        bool forced_door_closing( const tripoint &p, const ter_id door_type, int bash_dmg );


        //pixel minimap management
        int pixel_minimap_option;
    private:
        // Game-start procedures
        void print_menu(WINDOW *w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY,
                        bool bShowDDA = true);
        bool load_master(std::string worldname); // Load the master data file, with factions &c
        void load_weather(std::istream &fin);
        void load(std::string worldname, std::string name); // Load a player-specific save file
        bool start_game(std::string worldname); // Starts a new game in a world
        void start_special_game(special_game_id gametype); // See gamemode.cpp

        //private save functions.
        // returns false if saving failed for whatever reason
        bool save_factions_missions_npcs();
        void serialize_master(std::ostream &fout);
        // returns false if saving failed for whatever reason
        bool save_artifacts();
        // returns false if saving failed for whatever reason
        bool save_maps();
        void save_weather(std::ostream &fout);
        // returns false if saving failed for whatever reason
        bool save_uistate();
        void load_uistate(std::string worldname);
        // Data Initialization
        void init_fields();
        void init_faction_data();
        void init_autosave();     // Initializes autosave parameters
        void init_savedata_translation_tables();
        void init_lua();          // Initializes lua interpreter.
        void create_factions(); // Creates new factions (for a new game world)
        void create_starting_npcs(); // Creates NPCs that start near you

        // Player actions
        void wishitem( player *p = nullptr, int x = -1, int y = -1, int z = -1 );
        void wishmonster( const tripoint &p = tripoint_min );
        void wishmutate( player *p );
        void wishskill( player *p );
        void mutation_wish(); // Mutate

        /** Check for dangerous stuff at dest_loc, return false if the player decides
        not to step there */
        bool prompt_dangerous_tile( const tripoint &dest_loc ) const;
        /** Returns true if the menu handled stuff and player shouldn't do anything else */
        bool npc_menu( npc &who );
        void pldrive(int x, int y); // drive vehicle
        // Standard movement; handles attacks, traps, &c. Returns false if auto move
        // should be canceled
        bool plmove(int dx, int dy, int dz = 0);
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
        // Places the player at the end of a move; hurts feet, lists items etc.
        void place_player( const tripoint &dest );
        void on_move_effects();
        void wait(); // Long wait (player action)  '^'
        void open(); // Open a door  'o'
        void close();
        void close( const tripoint &p ); // Close a door  'c'
        void smash(); // Smash terrain

        void handbrake ();
        void control_vehicle(); // Use vehicle controls  '^'
        void examine( const tripoint &p );// Examine nearby terrain  'e'
        void examine();

        void grab(); // Establish a grab on something.
        void compare( const tripoint &offset = tripoint_min ); // Compare items 'I'
        void drop(int pos = INT_MIN, const tripoint &where = tripoint_min ); // Drop an item  'd'
        void drop_in_direction(); // Drop w/ direction  'D'

        void reassign_item(int pos = INT_MIN); // Reassign the letter of an item  '='
        void butcher(); // Butcher a corpse  'B'
        void eat(int pos = INT_MIN); // Eat food or fuel  'E' (or 'a')
        void use_item(int pos = INT_MIN); // Use item; also tries E,R,W  'a'
        void use_wielded_item();
        void wear(int pos = INT_MIN); // Wear armor  'W' (or 'a')
        void takeoff(int pos = INT_MIN); // Remove armor  'T'
        void change_side(int pos = INT_MIN); // Change the side on which an item is worn 'c'
        void reload(); // Reload a wielded gun/tool  'r'
        void reload( int pos, bool prompt = false );
        void mend( int pos = INT_MIN );
        void autoattack();
public:
        bool unload( item &it ); // Unload a gun/tool  'U'
        void unload(int pos = INT_MIN);

        unsigned int get_seed() const;

        /** If invoked, NPCs will be reloaded before next turn. */
        void set_npcs_dirty();
private:
        void wield(int pos = INT_MIN); // Wield a weapon  'w'
        void read(); // Read a book  'R' (or 'a')
        void chat(); // Talk to a nearby NPC  'C'
        void plthrow(int pos = INT_MIN); // Throw an item  't'

        // Internal methods to show "look around" info
        void print_fields_info( const tripoint &lp, WINDOW *w_look, int column, int &line );
        void print_terrain_info( const tripoint &lp, WINDOW *w_look, int column, int &line );
        void print_trap_info( const tripoint &lp, WINDOW *w_look, const int column, int &line );
        void print_creature_info( const Creature *creature, WINDOW *w_look, int column,
                                  int &line );
        void print_vehicle_info( const vehicle *veh, int veh_part, WINDOW *w_look,
                                 int column, int &line, int last_line );
        void print_visibility_info( WINDOW *w_look, int column, int &line,
                                    visibility_type visibility );
        void print_visibility_indicator( visibility_type visibility );
        void print_items_info( const tripoint &lp, WINDOW *w_look, int column, int &line,
                               int last_line );
        void print_graffiti_info( const tripoint &lp, WINDOW *w_look, int column, int &line,
                                  int last_line );
        void get_lookaround_dimensions(int &lookWidth, int &begin_y, int &begin_x) const;

        input_context get_player_input(std::string &action);

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
        void shift_monsters(const int shiftx, const int shifty, const int shiftz);
        /**
         * Despawn a specific monster, it's stored on the overmap. Also removes
         * it from the creature tracker. Keep in mind that mondex points to a
         * different monster after calling this (or to no monster at all).
         */
        void despawn_monster(int mondex);

        void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
        void rebuild_mon_at_cache();

        // Routine loop functions, approximately in order of execution
        void cleanup_dead();     // Delete any dead NPCs/monsters
        void monmove();          // Monster movement
        void rustCheck();        // Degrades practice levels
        void process_events();   // Processes and enacts long-term events
        void process_activity(); // Processes and enacts the player's activity
        void update_weather();   // Updates the temperature and weather patten
        int  mon_info(WINDOW *); // Prints a list of nearby monsters
        void handle_key_blocking_activity(); // Abort reading etc.
        bool handle_action();
        bool try_get_right_click_action( action_id &act, const tripoint &mouse_target );
        bool try_get_left_click_action( action_id &act, const tripoint &mouse_target );

        void item_action_menu(); // Displays item action menu


        void rcdrive(int dx, int dy); //driving radio car
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
        void msg_buffer();       // Opens a window with old messages in it
        void draw_minimap();     // Draw the 5x5 minimap
        void draw_HP();          // Draws the player's HP and Power level
        /** Draws the sidebar (if it's visible), including all windows there */
        void draw_sidebar();
        void draw_sidebar_messages();
        void draw_pixel_minimap();  // Draws the pixel minimap based on the player's current location

        //  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
        void autosave();         // automatic quicksaves - Performs some checks before calling quicksave()
        void quicksave();        // Saves the game without quitting
        void quickload();        // Loads the previously saved game if it exists

        // Input related
        // Handles box showing items under mouse
        bool handle_mouseview( input_context &ctxt, std::string &action );

        // On-request draw functions
        void draw_overmap();        // Draws the overmap, allows note-taking etc.
        void disp_kills();          // Display the player's kill counts
        void disp_faction_ends();   // Display the faction endings
        void disp_NPC_epilogues();  // Display NPC endings
        void disp_NPCs();           // Currently UNUSED.  Lists global NPCs.
        void list_missions();       // Listed current, completed and failed missions.

        // Debug functions
        void debug();           // All-encompassing debug screen.  TODO: This.
        void display_scent();   // Displays the scent map
        void groupdebug();      // Get into on monster groups

        // ########################## DATA ################################

        int last_target; // The last monster targeted
        bool last_target_was_npc;
        safe_mode_type safe_mode;
        bool safe_mode_warning_logged;
        std::vector<int> new_seen_mon;
        int mostseen;  // # of mons seen last turn; if this increases, set safe_mode to SAFE_MODE_STOP
        bool autosafemode; // is autosafemode enabled?
        bool safemodeveh; // safemode while driving?
        int turnssincelastmon; // needed for auto run mode
        //  quit_status uquit;    // Set to true if the player quits ('Q')
        bool bVMonsterLookFire;
        calendar nextspawn; // The turn on which monsters will spawn next.
        calendar nextweather; // The turn on which weather will shift next.
        int next_npc_id, next_faction_id, next_mission_id; // Keep track of UIDs
        std::list<event> events;         // Game events to be processed
        std::map<mtype_id, int> kills;         // Player's kill count
        int moves_since_last_save;
        time_t last_save_timestamp;
        mutable std::array<float, OVERMAP_LAYERS> latest_lightlevels;
        // remoteveh() cache
        int remoteveh_cache_turn;
        vehicle *remoteveh_cache;
        /** Has a NPC been spawned since last load? */
        bool npcs_dirty;

        std::unique_ptr<special_game> gamemode;

        int moveCount; //Times the player has moved (not pause, sleep, etc)
        int user_action_counter; // Times the user has input an action
        const int lookHeight; // Look Around window height

        /** How far the tileset should be zoomed out, 16 is default. 32 is zoomed in by x2, 8 is zoomed out by x0.5 */
        int tileset_zoom;

        /** Seed for all the random numbers that should have consistent randomness (weather). */
        unsigned int seed;

        weather_type weather_override;

        // Preview for auto move route
        std::vector<tripoint> destination_preview;

        Creature *is_hostile_within(int distance);

        void move_save_to_graveyard();
        bool save_player_data();
};

#endif
