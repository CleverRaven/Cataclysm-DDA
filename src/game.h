#ifndef GAME_H
#define GAME_H

#include "mtype.h"
#include "monster.h"
#include "map.h"
#include "lightmap.h"
#include "player.h"
#include "scenario.h"
#include "overmap.h"
#include "omdata.h"
#include "crafting.h"
#include "npc.h"
#include "faction.h"
#include "event.h"
#include "mission.h"
#include "weather.h"
#include "construction.h"
#include "calendar.h"
#include "posix_time.h"
#include "mutation.h"
#include "live_view.h"
#include "worldfactory.h"
#include "creature_tracker.h"
#include "game_constants.h"
#include "weather_gen.h"
#include <vector>
#include <map>
#include <queue>
#include <list>
#include <stdarg.h>

extern const int savegame_version;
extern int save_loading_version;

// The reference to the one and only game instance.
extern game *g;

#define PICKUP_RANGE 6
extern bool trigdist;
extern bool use_tiles;

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
    TARGET_MODE_TURRET
};

// Refactoring into base monster class.

struct monster_and_count {
    monster critter;
    int count;
    monster_and_count(monster M, int C) : critter (M), count (C) {};
};

struct special_game;
struct mtype;
struct mission_type;
class map;
class player;
class calendar;
class DynamicDataLoader;

class game
{
        friend class editmap;
        friend class advanced_inventory;
        friend class DynamicDataLoader; // To allow unloading dynamicly loaded stuff
    public:
        game();
        ~game();

        // Static data, does not depend on mods or similar.
        void load_static_data();
        // Load core data and all mods.
        void check_all_mod_data();
    protected:
        // Load core dynamic data
        void load_core_data();
        // Load dynamic data from given directory
        void load_data_from_dir(const std::string &path);
        // Load core data and mods from that world
        void load_world_modfiles(WORLDPTR world);
    public:


        void init_ui();
        void setup();
        bool game_quit(); // True if we actually quit the game - used in main.cpp
        bool game_error();
        bool new_game;    // Is true if game has just started or loaded, false otherwise
        quit_status uquit;    // used in main.cpp to determine what type of quit
        void serialize(std::ofstream &fout);  // for save
        void unserialize(std::ifstream &fin);  // for load
        bool unserialize_legacy(std::ifstream &fin);  // for old load
        void unserialize_master(std::ifstream &fin);  // for load
        bool unserialize_master_legacy(std::ifstream &fin);  // for old load

        // returns false if saving faild for whatever reason
        bool save();
        void delete_world(std::string worldname, bool delete_folder);
        std::vector<std::string> list_active_characters();
        void write_memorial_file(std::string sLastWords);
        bool cleanup_at_end();
        void start_calendar();
        bool do_turn();
        void draw();
        void draw_ter(int posx = -999, int posy = -999);
        void draw_veh_dir_indicator(void);
        /**
         * Add an entry to @ref events. For further information see event.h
         * @param type Type of event.
         * @param on_turn On which turn event should be happened.
         * @param faction_id Faction of event.
         * @param x,y global submap coordinates.
         */
        void add_event(event_type type, int on_turn, int faction_id = -1,
                       int x = INT_MIN, int y = INT_MIN);
        bool event_queued(event_type type);
        /**
         * Sound at (x, y) of intensity (vol)
         * @param x x-position of sound.
         * @param y y-position of sound.
         * @param vol Volume of sound.
         * @param description Description of the sound for the player,
         * if non-empty string a message is generated.
         * @param ambient If false, the sound interrupts player activities.
         * If true, activities continue.
         * @returns true if the player could hear the sound.
         */
        bool sound(int x, int y, int vol, std::string description, bool ambient = false);
        // same as sound(..., true)
        bool ambient_sound(int x, int y, int vol, std::string description);
        // creates a list of coordinates to draw footsteps
        void add_footstep(int x, int y, int volume, int distance, monster *source);
        std::vector<std::vector<point> > footsteps;
        std::vector<monster *> footsteps_source;
        // Calculate where footstep marker should appear and put those points into the result.
        // It also clears @ref footsteps_source and @ref footsteps
        void calculate_footstep_markers(std::vector<point> &result);
        // visual cue to monsters moving out of the players sight
        void draw_footsteps();
        // Explosion at (x, y) of intensity (power), with (shrapnel) chunks of shrapnel
        void explosion(int x, int y, int power, int shrapnel, bool fire, bool blast = true);
        // Draws an explosion with set radius and color at the given location
        /* Defined later in this file */
        //void draw_explosion(int x, int y, int radius, nc_color col);
        // Flashback at (x, y)
        void flashbang(int x, int y, bool player_immune = false);
        // Move the player vertically, if (force) then they fell
        void vertical_move(int z, bool force);
        void use_computer(int x, int y);
        bool refill_vehicle_part (vehicle &veh, vehicle_part *part, bool test = false);
        bool pl_refill_vehicle (vehicle &veh, int part, bool test = false);
        void resonance_cascade(int x, int y);
        void scrambler_blast(int x, int y);
        void emp_blast(int x, int y);
        int  npc_at(const int x, const int y) const; // Index of the npc at (x, y); -1 for none
        int  npc_by_id(const int id) const; // Index of the npc at (x, y); -1 for none
        // Return any critter at (x,y), be it a monster, an npc, or u (the player).
        Creature *critter_at(int x, int y);
        // void build_monmap();  // Caches data for mon_at()

        bool add_zombie(monster &critter);
        size_t num_zombies() const;
        monster &zombie(const int idx);
        bool update_zombie_pos(const monster &critter, const int newx, const int newy);
        void remove_zombie(const int idx);
        void clear_zombies();
        bool spawn_hallucination(); //Spawns a hallucination close to the player

        int  mon_at(const int x, const int y) const; // Index of the monster at (x, y); -1 for none
        int  mon_at(point p) const;
        bool is_empty(const int x, const int y); // True if no PC, no monster, move cost > 0
        bool isBetween(int test, int down, int up);
        bool is_in_sunlight(int x, int y); // Checks outdoors + sunny
        bool is_sheltered(int x, int y); // Checks if indoors, underground or in a car.
        bool is_in_ice_lab(point location);
        bool revive_corpse(int x, int y, int n); // revives a corpse from an item pile
        bool revive_corpse(int x, int y,
                           item *it); // revives a corpse by item pointer, caller handles item deletion
        // Player wants to fire a gun (target selection) etc. actual firing is done in player::fire_gun
        // This is interactive and therefor not for npcs.
        void plfire(bool burst, int default_target_x = -1, int default_target_y = -1);
        void throw_item(player &p, int tarx, int tary, item &thrown,
                        std::vector<point> &trajectory);
        // Target is an interactive function which allows the player to choose a nearby
        // square.  It display information on any monster/NPC on that square, and also
        // returns a Bresenham line to that square.  It is called by plfire(),
        // throw() and vehicle::aim_turrets()
        std::vector<point> target(int &x, int &y, int lowx, int lowy, int hix,
                                  int hiy, std::vector <Creature *> t, int &target,
                                  item *relevent, target_mode mode, 
                                  point from = point(-1, -1));
        void cancel_activity();
        bool cancel_activity_query(const char *message, ...);
        bool cancel_activity_or_ignore_query(const char *reason, ...);
        void moving_vehicle_dismount(int tox, int toy);

        vehicle *remoteveh(); // Get remotely controlled vehicle
        void setremoteveh(vehicle *veh); // Set remotely controlled vehicle

        int assign_mission_id(); // Just returns the next available one
        void give_mission(mission_id type); // Create the mission and assign it
        void assign_mission(int id); // Just assign an existing mission
        // reserve_mission() creates a new mission of the given type and pushes it to
        // active_missions.  The function returns the UID of the new mission, which can
        // then be passed to a MacGuffin or something else that needs to track a mission
        int reserve_mission(mission_id type, int npc_id = -1);
        int reserve_random_mission(mission_origin origin, point p = point(-1, -1),
                                   int npc_id = -1);
        npc *find_npc(int id);
        void load_npcs(); //Make any nearby NPCs from the overmap active.
        int kill_count(std::string mon);       // Return the number of kills of a given mon_id
        // Register one kill of a monster of type mtype_id by the player.
        void increase_kill_count(const std::string &mtype_id);
        mission *find_mission(int id); // Mission with UID=id; NULL if non-existant
        mission_type *find_mission_type(int id); // Same, but returns its type
        bool mission_complete(int id, int npc_id); // True if we made it
        bool mission_failed(int id); // True if we failed it
        void wrap_up_mission(int id); // Perform required actions
        void fail_mission(int id); // Perform required actions, move to failed list
        void mission_step_complete(int id, int step); // Parial completion
        void process_missions(); // Process missions, see if time's run out

        void teleport(player *p = NULL, bool add_teleglow = true);
        void plswim(int x, int y); // Called by plmove.  Handles swimming
        void rod_fish(int sSkillLevel, int fishChance); // fish with rod catching function
        void catch_a_monster(std::vector<monster*> &catchables, int posx, int posy, player *p, int catch_duration = 0); //catch monsters
        std::vector<monster*> get_fishable(int distance); //gets the lish of fishable critters
        // when player is thrown (by impact or something)
        void fling_creature(Creature *c, const int &dir, float flvel,
                            bool controlled = false);

        /**
         * Nuke the area at (x,y) - global overmap terrain coordinates!
         */
        void nuke(int x, int y);
        bool spread_fungus(int x, int y);
        std::vector<faction *> factions_at(int x, int y);
        int &scent(int x, int y);
        float ground_natural_light_level() const;
        float natural_light_level() const;
        unsigned char light_level();
        void reset_light_level();
        int assign_npc_id();
        int assign_faction_id();
        faction *faction_by_ident(std::string ident);
        bool sees_u(int x, int y, int &t);
        bool u_see (int x, int y);
        bool u_see (const monster *critter);
        bool u_see (const Creature *t); // for backwards compatibility
        bool u_see (const Creature &t);
        Creature *is_hostile_nearby();
        Creature *is_hostile_very_close();
        void refresh_all();
        void update_map(int &x, int &y);  // Called by plmove when the map updates
        void update_overmap_seen(); // Update which overmap tiles we can see
        // Position of the player in overmap terrain coordinates, relative
        // to the current overmap (@ref cur_om).
        point om_location() const;
        // Position of the player in overmap terrain coordinates,
        // in global overmap terrain coordinates.
        tripoint om_global_location() const;

        void process_artifact(item *it, player *p);
        void add_artifact_messages(std::vector<art_effect_passive> effects);

        void peek( int peekx = 0, int peeky = 0);
        point look_debug();

        bool checkZone(const std::string p_sType, const int p_iX, const int p_iY);
        void zones_manager();
        void zones_manager_shortcuts(WINDOW *w_info);
        void zones_manager_draw_borders(WINDOW *w_border, WINDOW *w_info_border, const int iInfoHeight,
                                        const int width);
        // Look at nearby terrain ';', or select zone points
        point look_around(WINDOW *w_info = NULL, const point pairCoordsFirst = point(-1, -1));

        int list_items(const int iLastState); //List all items around the player
        int list_monsters(const int iLastState); //List all monsters around the player
        // Shared method to print "look around" info
        void print_all_tile_info(int lx, int ly, WINDOW *w_look, int column, int &line, bool mouse_hover);

        bool list_items_match(item &item, std::string sPattern);
        int list_filter_high_priority(std::vector<map_item_stack> &stack, std::string prorities);
        int list_filter_low_priority(std::vector<map_item_stack> &stack, int start, std::string prorities);
        std::vector<map_item_stack> filter_item_stacks(std::vector<map_item_stack> stack,
                std::string filter);
        std::vector<map_item_stack> find_nearby_items(int iRadius);
        std::string ask_item_filter(WINDOW *window, int rows);
        std::string ask_item_priority_high(WINDOW *window, int rows);
        std::string ask_item_priority_low(WINDOW *window, int rows);
        void draw_trail_to_square(int x, int y, bool bDrawX);
        void reset_item_list_state(WINDOW *window, int height);
        std::string sFilter; // this is a member so that it's remembered over time
        std::string list_item_upvote;
        std::string list_item_downvote;
        int inv(const std::string &title, const int &position = INT_MIN);
        int inv_activatable(std::string title);
        int inv_type(std::string title, item_cat inv_item_type = IC_NULL);
        int inv_for_liquid(const item &liquid, const std::string title, bool auto_choose_single);
        int inv_for_salvage(const std::string title);
        item *inv_map_for_liquid(const item &liquid, const std::string title);
        int inv_for_flag(const std::string flag, const std::string title, bool auto_choose_single);
        int inv_for_filter(const std::string title, const item_filter filter );
        int display_slice(indexed_invslice &, const std::string &, const int &position = INT_MIN);
        int inventory_item_menu(int pos, int startx = 0, int width = 50, int position = 0);
        // Select items to drop.  Returns a list of pairs of position, quantity.
        std::list<std::pair<int, int>> multidrop();
        faction *list_factions(std::string title = "FACTIONS:");

        bool has_gametype() const;
        special_game_id gametype() const;

        std::map<std::string, vehicle *> vtypes;
        void toggle_sidebar_style(void);
        void toggle_fullscreen(void);
        void temp_exit_fullscreen(void);
        void reenter_fullscreen(void);
        void zoom_in();
        void zoom_out();

        std::vector <mission_type> mission_types; // The list of mission templates

        weather_generator weatherGen; //A weather engine.
        bool has_generator = false;
        unsigned int weatherSeed = 0;
        signed char temperature;              // The air temperature
        int get_temperature();    // Returns outdoor or indoor temperature of current location
        weather_type weather;   // Weather pattern--SEE weather.h
        bool lightning_active;

        std::map<int, weather_segment> weather_log;
        overmap *cur_om;
        map m;

        int levx, levy, levz; // Placement inside the overmap
        /** Absolute values of lev[xyz] (includes the offset of cur_om) */
        int get_abs_levx() const;
        int get_abs_levy() const;
        int get_abs_levz() const;
        player u;
        scenario *scen;
        std::vector<monster> coming_to_stairs;
        int monstairx, monstairy, monstairz;
        std::vector<npc *> active_npc;
        std::vector<faction> factions;
        std::vector<mission> active_missions; // Missions which may be assigned
        // NEW: Dragging a piece of furniture, with a list of items contained
        ter_id dragging;
        std::vector<item> items_dragged;
        int weight_dragged; // Computed once, when you start dragging

        int ter_view_x, ter_view_y;
        WINDOW *w_terrain;
        WINDOW *w_overmap;
        WINDOW *w_omlegend;
        WINDOW *w_minimap;
        WINDOW *w_HP;
        WINDOW *w_messages;
        WINDOW *w_location;
        WINDOW *w_status;
        WINDOW *w_status2;
        WINDOW *w_blackspace;
        live_view liveview;

        // View offset based on the driving speed (if any)
        // that has been added to u.view_offset_*,
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
                           item *cont = NULL);

        //Move_liquid returns the amount of liquid left if we didn't move all the liquid,
        //otherwise returns sentinel -1, signifies transaction fail.
        int move_liquid(item &liquid);

        void open_gate( const int examx, const int examy, const ter_id handle_type );

        bionic_id random_good_bionic() const; // returns a non-faulty, valid bionic

        // Helper because explosion was getting too big.
        void do_blast( const int x, const int y, const int power, const int radius, const bool fire );

        // Knockback functions: knock target at (tx,ty) along a line, either calculated
        // from source position (sx,sy) using force parameter or passed as an argument;
        // force determines how far target is knocked, if trajectory is calculated
        // force also determines damage along with dam_mult;
        // stun determines base number of turns target is stunned regardless of impact
        // stun == 0 means no stun, stun == -1 indicates only impact stun (wall or npc/monster)
        void knockback(int sx, int sy, int tx, int ty, int force, int stun, int dam_mult);
        void knockback(std::vector<point> &traj, int force, int stun, int dam_mult);

        // shockwave applies knockback to all targets within radius of (x,y)
        // parameters force, stun, and dam_mult are passed to knockback()
        // ignore_player determines if player is affected, useful for bionic, etc.
        void shockwave(int x, int y, int radius, int force, int stun, int dam_mult, bool ignore_player);

        // Animation related functions
        void draw_explosion(int x, int y, int radius, nc_color col);
        void draw_bullet(Creature &p, int tx, int ty, int i, std::vector<point> trajectory, char bullet,
                         timespec &ts);
        void draw_hit_mon(int x, int y, monster critter, bool dead = false);
        void draw_hit_player(player *p, const int iDam, bool dead = false);
        void draw_line(const int x, const int y, const point center_point, std::vector<point> ret);
        void draw_line(const int x, const int y, std::vector<point> ret);
        void draw_weather(weather_printable wPrint);
        void draw_sct();
        void draw_zones(const point &p_pointStart, const point &p_pointEnd, const point &p_pointOffset);
        // Draw critter (if visible!) on its current position into w_terrain.
        // @param center the center of view, same as when calling map::draw
        void draw_critter(const Creature &critter, const point &center);

        // Vehicle related JSON loaders and variables
        void load_vehiclepart(JsonObject &jo);
        void check_vehicleparts();
        void load_vehicle(JsonObject &jo);
        void reset_vehicleparts();
        void reset_vehicles();
        void finalize_vehicles();

        std::queue<vehicle_prototype *> vehprototypes;

        nc_color limb_color(player *p, body_part bp, bool bleed = true,
                            bool bite = true, bool infect = true);

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
        bool fullscreen;
        bool was_fullscreen;
        void exam_vehicle(vehicle &veh, int examx, int examy, int cx = 0,
                          int cy = 0); // open vehicle interaction screen

        // put items from the item-vector on the map/a vehicle
        // at (dirx, diry), items are dropped into a vehicle part
        // with the cargo flag (if there is one), otherwise they are
        // dropped onto the ground.
        void drop(std::vector<item> &dropped, std::vector<item> &dropped_worn,
                  int freed_volume_capacity, int dirx, int diry);
        bool make_drop_activity( enum activity_type act, point target );
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
        void load_legacy_future_weather(std::string data);
        void load_legacy_future_weather(std::istream &fin);
        // returns false if saving failed for whatever reason
        bool save_uistate();
        void load_uistate(std::string worldname);
        // Data Initialization
        void init_npctalk();
        void init_fields();
        void init_weather();
        void init_weather_anim();
        void init_morale();
        void init_skills() throw (std::string);
        void init_professions();
        void init_faction_data();
        void init_mongroups() throw (std::string);    // Initializes monster groups
        void init_construction(); // Initializes construction "recipes"
        void init_missions();     // Initializes mission templates
        void init_autosave();     // Initializes autosave parameters
        void init_diseases();     // Initializes disease lookup table.
        void init_savedata_translation_tables();
        void init_lua();          // Initializes lua interpreter.
        void create_factions(); // Creates new factions (for a new game world)
        void create_starting_npcs(); // Creates NPCs that start near you

        // Player actions
        void wishitem( player *p = NULL, int x = -1, int y = -1 );
        void wishmonster( int x = -1, int y = -1 );
        void wishmutate( player *p );
        void wishskill( player *p );
        void mutation_wish(); // Mutate

        void pldrive(int x, int y); // drive vehicle
        // Standard movement; handles attacks, traps, &c. Returns false if auto move
        // should be canceled
        bool plmove(int dx, int dy);
        void on_move_effects();
        void wait(); // Long wait (player action)  '^'
        void open(); // Open a door  'o'
        void close(int closex = -1, int closey = -1); // Close a door  'c'
        void smash(); // Smash terrain

        // Forcefully close a door at (x, y).
        // The function checks for creatures/items/vehicles at that point and
        // might kill/harm/destroy them.
        // If there still remains something that prevents the door from closing
        // (e.g. a very big creatures, a vehicle) the door will not be closed and
        // the function returns false.
        // If the door gets closed the terrain at (x, y) is set to door_type and
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
        bool forced_gate_closing(int x, int y, ter_id door_type, int bash_dmg);

        bool vehicle_near ();
        void handbrake ();
        void control_vehicle(); // Use vehicle controls  '^'
        void examine(int examx = -1, int examy = -1);// Examine nearby terrain  'e'
        void advanced_inv();

        // Establish a grab on something.
        void grab();
        // Pick where to put liquid; false if it's left where it was

        void compare(int iCompareX = -999, int iCompareY = -999); // Compare two Items 'I'
        void drop(int pos = INT_MIN); // Drop an item  'd'
        void drop_in_direction(); // Drop w/ direction  'D'

        // calculate the time (in player::moves) it takes to drop the
        // items in dropped and dropped_worn.
        int calculate_drop_cost(std::vector<item> &dropped, const std::vector<item> &dropped_worn,
                                int freed_volume_capacity) const;
        void reassign_item(int pos = INT_MIN); // Reassign the letter of an item  '='
        void butcher(); // Butcher a corpse  'B'
        void complete_butcher(int index); // Finish the butchering process
        void forage(); // Foraging ('a' on underbrush)
        void eat(int pos = INT_MIN); // Eat food or fuel  'E' (or 'a')
        void use_item(int pos = INT_MIN); // Use item; also tries E,R,W  'a'
        void use_wielded_item();
        void wear(int pos = INT_MIN); // Wear armor  'W' (or 'a')
        void takeoff(int pos = INT_MIN); // Remove armor  'T'
        void reload(); // Reload a wielded gun/tool  'r'
        void reload(int pos);
        void unload(item &it); // Unload a gun/tool  'U'
        void unload(int pos = INT_MIN);
        void wield(int pos = INT_MIN); // Wield a weapon  'w'
        void read(); // Read a book  'R' (or 'a')
        void chat(); // Talk to a nearby NPC  'C'
        void plthrow(int pos = INT_MIN); // Throw an item  't'

        // Internal methods to show "look around" info
        void print_fields_info(int lx, int ly, WINDOW *w_look, int column, int &line);
        void print_terrain_info(int lx, int ly, WINDOW *w_look, int column, int &line);
        void print_trap_info(int lx, int ly, WINDOW *w_look, const int column, int &line);
        void print_object_info(int lx, int ly, WINDOW *w_look, const int column, int &line,
                               bool mouse_hover);
        void handle_multi_item_info(int lx, int ly, WINDOW *w_look, const int column, int &line,
                                    bool mouse_hover);
        void get_lookaround_dimensions(int &lookWidth, int &begin_y, int &begin_x) const;

        input_context get_player_input(std::string &action);

        // interface to target(), collects a list of targets & selects default target
        // finally calls target() and returns its result.
        std::vector<point> pl_target_ui(int &x, int &y, int range, item *relevent, target_mode mode,
                                        int default_target_x = -1, int default_target_y = -1);

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
        void hallucinate(const int x, const int y); // Prints hallucination junk to the screen
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
        bool disable_robot( point p );

        void update_scent();     // Updates the scent map
        bool is_game_over();     // Returns true if the player quit or died
        void death_screen();     // Display our stats, "GAME OVER BOO HOO"
        void gameover();         // Ends the game
        void msg_buffer();       // Opens a window with old messages in it
        void draw_minimap();     // Draw the 5x5 minimap
        void draw_HP();          // Draws the player's HP and Power level
        /** Draws the sidebar (if it's visible), including all windows there */
        void draw_sidebar();

        //  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
        void autosave();         // automatic quicksaves - Performs some checks before calling quicksave()
        void quicksave();        // Saves the game without quitting

        // Input related
        bool handle_mouseview(input_context &ctxt,
                              std::string &action); // Handles box showing items under mouse
        void hide_mouseview(); // Hides the mouse hover box and redraws what was under it

        // On-request draw functions
        void draw_overmap();     // Draws the overmap, allows note-taking etc.
        void disp_kills();       // Display the player's kill counts
        void disp_NPCs();        // Currently UNUSED.  Lists global NPCs.
        void list_missions();    // Listed current, completed and failed missions.

        // Debug functions
        void debug();           // All-encompassing debug screen.  TODO: This.
        void display_scent();   // Displays the scent map
        void groupdebug();      // Get into on monster groups

        // ########################## DATA ################################

        Creature_tracker critter_tracker;

        int last_target; // The last monster targeted
        bool last_target_was_npc;
        safe_mode_type safe_mode;
        std::vector<int> new_seen_mon;
        int mostseen;  // # of mons seen last turn; if this increases, set safe_mode to SAFE_MODE_STOP
        bool autosafemode; // is autosafemode enabled?
        bool safemodeveh; // safemode while driving?
        int turnssincelastmon; // needed for auto run mode
        //  quit_status uquit;    // Set to true if the player quits ('Q')

        calendar nextspawn; // The turn on which monsters will spawn next.
        calendar nextweather; // The turn on which weather will shift next.
        int next_npc_id, next_faction_id, next_mission_id; // Keep track of UIDs
        int grscent[SEEX *MAPSIZE][SEEY *MAPSIZE];   // The scent map
        //int monmap[SEEX * MAPSIZE][SEEY * MAPSIZE]; // Temp monster map, for mon_at()
        int nulscent;    // Returned for OOB scent checks
        std::list<event> events;         // Game events to be processed
        std::map<std::string, int> kills;         // Player's kill count
        int moves_since_last_save;
        time_t last_save_timestamp;
        unsigned char latest_lightlevel;
        calendar latest_lightlevel_turn;
        // remoteveh() cache
        int remoteveh_cache_turn;
        vehicle *remoteveh_cache;

        special_game *gamemode;

        int moveCount; //Times the player has moved (not pause, sleep, etc)
        const int lookHeight; // Look Around window height

        /** How far the tileset should be zoomed out, 16 is default. 32 is zoomed in by x2, 8 is zoomed out by x0.5 */
        int tileset_zoom;

        // Preview for auto move route
        std::vector<point> destination_preview;

        Creature *is_hostile_within(int distance);
        void activity_on_turn();
        void activity_on_turn_game();
        void activity_on_turn_drop();
        void activity_on_turn_stash();
        void activity_on_turn_pickup();
        void activity_on_turn_move_items();
        void activity_on_turn_vibe();
        void activity_on_turn_fill_liquid();
        void activity_on_turn_refill_vehicle();
        void activity_on_turn_pulp();
        void activity_on_turn_start_fire_lens();
        void activity_on_finish();
        void activity_on_finish_reload();
        void activity_on_finish_train();
        void activity_on_finish_firstaid();
        void activity_on_finish_fish();
        void activity_on_finish_vehicle();
        void activity_on_finish_make_zlave();
        void activity_on_finish_start_fire();
        void activity_on_finish_hotwire();
        void longsalvage(); // Salvage everything activity
        void move_save_to_graveyard();
        bool save_player_data();
};

#endif
