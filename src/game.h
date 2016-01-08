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
#include <list>
#include <stdarg.h>

extern const int savegame_version;
extern int save_loading_version;

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

extern const int savegame_version;
extern int savegame_loading_version;

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

struct special_game;
struct mtype;
using mtype_id = string_id<mtype>;
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

// Note: this is copied from inventory.h
// Entire inventory.h would also bring item.h here
typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;
typedef std::vector< const std::list<item>* > const_invslice;
typedef std::vector< std::pair<std::list<item>*, int> > indexed_invslice;
typedef std::function<bool(const item &)> item_filter;

class game
{
        friend class editmap;
        friend class advanced_inventory;
        friend class DynamicDataLoader; // To allow unloading dynamicly loaded stuff
    public:
        game();
        ~game();

        /** Loads static data that does not depend on mods or similar. */
        void load_static_data();
        /** Loads core data and all mods. */
        void check_all_mod_data();
        /** Loads core dynamic data. */
        void load_core_data();
    protected:
        /** Loads dynamic data from the given directory. */
        void load_data_from_dir(const std::string &path);
        /** Loads core data and mods from the given world. */
        void load_world_modfiles(WORLDPTR world);

        // May be a bit hacky, but it's probably better than the header spaghetti
        std::unique_ptr<map> map_ptr;
        std::unique_ptr<player> u_ptr;
        std::unique_ptr<live_view> liveview_ptr;
        live_view& liveview;
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
        void serialize(std::ofstream &fout);  // for save
        void unserialize(std::ifstream &fin);  // for load
        bool unserialize_legacy(std::ifstream &fin);  // for old load
        void unserialize_master(std::ifstream &fin);  // for load
        bool unserialize_master_legacy(std::ifstream &fin);  // for old load

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
         */
        tripoint get_veh_dir_indicator_location() const;
        void draw_veh_dir_indicator(void);

        /** Make map a reference here, to avoid map.h in game.h */
        map &m;
        player &u;

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
        void explosion( const tripoint &p, float power, float factor = 0.8f,
                        int shrapnel = 0, bool fire = false );
        /** Helper for explosion, does the actual blast. */
        void do_blast( const tripoint &p, float power, float factor, bool fire );
        /** Shoot shrapnel from point p */
        void shrapnel( const tripoint &p, int power, int count, int radius );
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
        /** Attempts to refill the give vehicle's part with the player's current weapon. Returns true if successful. */
        bool refill_vehicle_part (vehicle &veh, vehicle_part *part, bool test = false);
        /** Identical to refill_vehicle_part(veh, &veh.parts[part], test). */
        bool pl_refill_vehicle (vehicle &veh, int part, bool test = false);
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
        bool revive_corpse( const tripoint &location, const item &corpse );
        /** Handles player input parts of gun firing (target selection, etc.). Actual firing is done
         *  in player::fire_gun(). This is interactive and should not be used by NPC's. */
        void plfire( bool burst, const tripoint &default_target = tripoint_min );
        /** Cycle fire mode of held item. If `force_gun` is false, also checks turrets on the tile */
        void cycle_item_mode( bool force_gun );
        /** Target is an interactive function which allows the player to choose a nearby
         *  square.  It display information on any monster/NPC on that square, and also
         *  returns a Bresenham line to that square.  It is called by plfire(),
         *  throw() and vehicle::aim_turrets() */
        std::vector<tripoint> target( tripoint &p, const tripoint &low, const tripoint &high,
                                      std::vector<Creature *> t, int &target,
                                      item *relevant, target_mode mode,
                                      const tripoint &from = tripoint_min );
        /**
         * Interface to target(), collects a list of targets & selects default target
         * finally calls target() and returns its result.
         * Used by vehicle::manual_fire_turret()
         */
        std::vector<tripoint> pl_target_ui( tripoint &p, int range, item *relevant, target_mode mode,
                                            const tripoint &default_target = tripoint_min );
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
        int &scent( const tripoint &p );
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
        void update_map(player *p);
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
        void print_all_tile_info( const tripoint &lp, WINDOW *w_look, int column, int &line, bool mouse_hover );

        std::vector<map_item_stack> find_nearby_items(int iRadius);
        void draw_item_filter_rules(WINDOW *window, int rows);
        std::string ask_item_priority_high(WINDOW *window, int rows);
        std::string ask_item_priority_low(WINDOW *window, int rows);
        void draw_trail_to_square( const tripoint &t, bool bDrawX );
        void reset_item_list_state(WINDOW *window, int height, bool bRadiusSort);
        std::string sFilter; // this is a member so that it's remembered over time
        std::string list_item_upvote;
        std::string list_item_downvote;
        int inv(const std::string &title, int position = INT_MIN);
        int inv_activatable(std::string const &title);
        int inv_for_liquid(const item &liquid, const std::string &title, bool auto_choose_single);
        int inv_for_salvage(const std::string &title, const salvage_actor &actor );
        item *inv_map_for_liquid(const item &liquid, const std::string &title, int radius = 0);
        int inv_for_flag(const std::string &flag, const std::string &title, bool auto_choose_single);
        int inv_for_filter(const std::string &title, item_filter filter);
        int inv_for_unequipped(std::string const &title, item_filter filter);
        int display_slice(indexed_invslice const&, const std::string &, bool show_worn = true, int position = INT_MIN);
        enum inventory_item_menu_positon {
            RIGHT_TERMINAL_EDGE,
            LEFT_OF_INFO,
            RIGHT_OF_INFO,
            LEFT_TERMINAL_EDGE,
        };
        int inventory_item_menu(int pos, int startx = 0, int width = 50, inventory_item_menu_positon position = RIGHT_OF_INFO);

        // Combines filtered player inventory with filtered ground and vehicle items within radius to create a pseudo-inventory.
        item_location inv_map_splice( item_filter inv_filter,
                                      item_filter ground_filter,
                                      item_filter vehicle_filter,
                                      const std::string &title,
                                      int radius = 0 );
        item_location inv_map_splice( item_filter filter, const std::string &title, int radius = 0 );

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

        std::unique_ptr<weather_generator> weather_gen;
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
        scenario *scen;
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

        bool handle_liquid(item &liquid, bool from_ground, bool infinite, item *source = NULL,
                           item *cont = NULL, int radius = 0);

        //Move_liquid returns the amount of liquid left if we didn't move all the liquid,
        //otherwise returns sentinel -1, signifies transaction fail.
        int move_liquid(item &liquid);

        void open_gate( const tripoint &p, const ter_id handle_type );

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
        bool check_save_mode_allowed();

        const int dangerous_proximity;
        bool narrow_sidebar;
        bool right_sidebar;
        bool fullscreen;
        bool was_fullscreen;
        void exam_vehicle(vehicle &veh, const tripoint &p, int cx = 0,
                          int cy = 0); // open vehicle interaction screen

        // put items from the item-vector on the map/a vehicle
        // at (dirx, diry), items are dropped into a vehicle part
        // with the cargo flag (if there is one), otherwise they are
        // dropped onto the ground.
        void drop(std::vector<item> &dropped, std::vector<item> &dropped_worn,
                  int freed_volume_capacity, tripoint dir,
                  bool to_vehicle = true); // emulate old behaviour normally
        void drop(std::vector<item> &dropped, std::vector<item> &dropped_worn,
                  int freed_volume_capacity, int dirx, int diry,
                  bool to_vehicle = true); // emulate old behaviour normally
        bool make_drop_activity(enum activity_type act, const tripoint &target, bool to_vehicle = true);

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
        bool forced_gate_closing( const tripoint &p, const ter_id door_type, int bash_dmg );


        //pixel minimap management
        int pixel_minimap_option;
    private:
        // Game-start procedures
        void print_menu(WINDOW *w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY,
                        bool bShowDDA = true);
        void print_menu_items(WINDOW *w_in, std::vector<std::string> vItems, int iSel,
                              int iOffsetY, int iOffsetX, int spacing = 1);
        bool load_master(std::string worldname); // Load the master data file, with factions &c
        void load_weather(std::ifstream &fin);
        void load(std::string worldname, std::string name); // Load a player-specific save file
        void start_game(std::string worldname); // Starts a new game in a world
        void start_special_game(special_game_id gametype); // See gamemode.cpp

        //private save functions.
        // returns false if saving failed for whatever reason
        bool save_factions_missions_npcs();
        void serialize_master(std::ofstream &fout);
        // returns false if saving failed for whatever reason
        bool save_artifacts();
        // returns false if saving failed for whatever reason
        bool save_maps();
        void save_weather(std::ofstream &fout);
        // returns false if saving failed for whatever reason
        bool save_uistate();
        void load_uistate(std::string worldname);
        // Data Initialization
        void init_npctalk();
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

        // Establish a grab on something.
        void grab();
        // Pick where to put liquid; false if it's left where it was

        void compare(); // Compare two Items 'I'
        void compare( const tripoint &offset ); // Offset is added to player's position
        void drop(int pos = INT_MIN); // Drop an item  'd'
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
        void reload(int pos);
        void unload(item &it); // Unload a gun/tool  'U'
        void unload(int pos = INT_MIN);
        void wield(int pos = INT_MIN); // Wield a weapon  'w'
        void read(); // Read a book  'R' (or 'a')
        void chat(); // Talk to a nearby NPC  'C'
        void plthrow(int pos = INT_MIN); // Throw an item  't'

        // Internal methods to show "look around" info
        void print_fields_info( const tripoint &lp, WINDOW *w_look, int column, int &line );
        void print_terrain_info( const tripoint &lp, WINDOW *w_look, int column, int &line );
        void print_trap_info( const tripoint &lp, WINDOW *w_look, const int column, int &line );
        void print_object_info( const tripoint &lp, WINDOW *w_look, const int column, int &line,
                               bool mouse_hover );
        void handle_multi_item_info( const tripoint &lp, WINDOW *w_look, const int column, int &line,
                                    bool mouse_hover );
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
        void hallucinate( const tripoint &center ); // Prints hallucination junk to the screen
        int  mon_info(WINDOW *); // Prints a list of nearby monsters
        void handle_key_blocking_activity(); // Abort reading etc.
        bool handle_action();

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

        void update_scent();     // Updates the scent map
        bool is_game_over();     // Returns true if the player quit or died
        void death_screen();     // Display our stats, "GAME OVER BOO HOO"
        void gameover();         // Ends the game
        void msg_buffer();       // Opens a window with old messages in it
        void draw_minimap();     // Draw the 5x5 minimap
        void draw_HP();          // Draws the player's HP and Power level
        /** Draws the sidebar (if it's visible), including all windows there */
        void draw_sidebar();
        void draw_pixel_minimap();  // Draws the pixel minimap based on the player's current location

        //  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
        void autosave();         // automatic quicksaves - Performs some checks before calling quicksave()
        void quicksave();        // Saves the game without quitting

        // Input related
        bool handle_mouseview(input_context &ctxt,
                              std::string &action); // Handles box showing items under mouse
        void hide_mouseview(); // Hides the mouse hover box and redraws what was under it

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
        int grscent[SEEX *MAPSIZE][SEEY *MAPSIZE];   // The scent map
        int nulscent;    // Returned for OOB scent checks
        std::list<event> events;         // Game events to be processed
        std::map<mtype_id, int> kills;         // Player's kill count
        int moves_since_last_save;
        time_t last_save_timestamp;
        mutable std::array<float, OVERMAP_LAYERS> latest_lightlevels;
        // remoteveh() cache
        int remoteveh_cache_turn;
        vehicle *remoteveh_cache;

        special_game *gamemode;

        int moveCount; //Times the player has moved (not pause, sleep, etc)
        const int lookHeight; // Look Around window height

        /** How far the tileset should be zoomed out, 16 is default. 32 is zoomed in by x2, 8 is zoomed out by x0.5 */
        int tileset_zoom;

        // Preview for auto move route
        std::vector<tripoint> destination_preview;

        Creature *is_hostile_within(int distance);

        void move_save_to_graveyard();
        bool save_player_data();
};

#endif
