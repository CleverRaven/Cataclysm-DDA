#ifndef _GAME_H_
#define _GAME_H_

#include "mtype.h"
#include "monster.h"
#include "map.h"
#include "lightmap.h"
#include "player.h"
#include "overmap.h"
#include "omdata.h"
#include "mapitems.h"
#include "crafting.h"
#include "trap.h"
#include "npc.h"
#include "faction.h"
#include "event.h"
#include "mission.h"
#include "weather.h"
#include "construction.h"
#include "calendar.h"
#include "posix_time.h"
#include "artifact.h"
#include "mutation.h"
#include "gamemode.h"
#include "live_view.h"
#include "worldfactory.h"
#include "creature_tracker.h"
#include <vector>
#include <map>
#include <queue>
#include <list>
#include <stdarg.h>

// Fixed window sizes
#define HP_HEIGHT 14
#define HP_WIDTH 7
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7
#define MONINFO_HEIGHT 12
#define MONINFO_WIDTH 48
#define MESSAGES_HEIGHT 8
#define MESSAGES_WIDTH 48
#define LOCATION_HEIGHT 1
#define LOCATION_WIDTH 48
#define STATUS_HEIGHT 4
#define STATUS_WIDTH 55

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

#define MAX_ITEM_IN_SQUARE 4096 // really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_VOLUME_IN_SQUARE 4000 // 6.25 dead bears is enough for everybody!
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE // no reason to differ
#define MAX_VOLUME_IN_VEHICLE_STORAGE 2000 // todo: variation. semi trailer square could hold more. the real limit would be weight

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
 QUIT_NO = 0,  // Still playing
 QUIT_MENU,    // Quit at the menu
 QUIT_SUICIDE, // Quit with 'Q'
 QUIT_SAVED,   // Saved and quit
 QUIT_DIED,     // Actual death
 QUIT_ERROR
};

// Refactoring into base monster class.

struct monster_and_count
{
 monster critter;
 int count;
 monster_and_count(monster M, int C) : critter (M), count (C) {};
};

struct game_message
{
 calendar turn;
 int count;
 std::string message;
 game_message() { turn = 0; count = 1; message = ""; };
 game_message(calendar T, std::string M) : turn (T), message (M) { count = 1; };
};

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
  quit_status uquit;    // used in main.cpp to determine what type of quit
  void serialize(std::ofstream & fout); // for save
  void unserialize(std::ifstream & fin); // for load
  bool unserialize_legacy(std::ifstream & fin); // for old load
  void unserialize_master(std::ifstream & fin); // for load
  bool unserialize_master_legacy(std::ifstream & fin); // for old load

  void save();
  void delete_world(std::string worldname, bool delete_folder);
  std::vector<std::string> list_active_characters();
  void write_memorial_file();
  void cleanup_at_end();
  bool do_turn();
  void draw();
  void draw_ter(int posx = -999, int posy = -999);
  void draw_veh_dir_indicator(void);
  void advance_nextinv(); // Increment the next inventory letter
  void decrease_nextinv(); // Decrement the next inventory letter
  void vadd_msg(const char* msg, va_list ap );
  void add_msg_string(const std::string &s);
    void add_msg(const char* msg, ...);
  void add_msg_if_player(Creature *t, const char* msg, ...);
  void add_msg_if_npc(Creature* p, const char* msg, ...);
  void add_msg_player_or_npc(Creature* p, const char* player_str, const char* npc_str, ...);
  std::vector<game_message> recent_messages(const int count); //Retrieves the last X messages
  void add_event(event_type type, int on_turn, int faction_id = -1,
                 int x = -1, int y = -1);
  bool event_queued(event_type type);
// Sound at (x, y) of intensity (vol), described to the player is (description)
  bool sound(int x, int y, int vol, std::string description); //returns true if you heard the sound
// creates a list of coordinates to draw footsteps
  void add_footstep(int x, int y, int volume, int distance, monster* source);
  std::vector<std::vector<point> > footsteps;
  std::vector<monster*> footsteps_source;
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
  bool refill_vehicle_part (vehicle &veh, vehicle_part *part, bool test=false);
  bool pl_refill_vehicle (vehicle &veh, int part, bool test=false);
  void resonance_cascade(int x, int y);
  void scrambler_blast(int x, int y);
  void emp_blast(int x, int y);
  int  npc_at(const int x, const int y) const; // Index of the npc at (x, y); -1 for none
  int  npc_by_id(const int id) const; // Index of the npc at (x, y); -1 for none
 // void build_monmap();  // Caches data for mon_at()

  bool add_zombie(monster& critter);
  size_t num_zombies() const;
  monster& zombie(const int idx);
  bool update_zombie_pos(const monster &critter, const int newx, const int newy);
  void remove_zombie(const int idx);
  void clear_zombies();
  bool spawn_hallucination(); //Spawns a hallucination close to the player

  Creature* creature_at(const int x, const int y);
  int  mon_at(const int x, const int y) const; // Index of the monster at (x, y); -1 for none
  int  mon_at(point p) const;
  bool is_empty(const int x, const int y); // True if no PC, no monster, move cost > 0
  bool isBetween(int test, int down, int up);
  bool is_in_sunlight(int x, int y); // Checks outdoors + sunny
  bool is_in_ice_lab(point location);
// Kill that monster; fixes any pointers etc
  void kill_mon(int index, bool player_did_it = false);
  void kill_mon(monster& critter, bool player_did_it = false); // new kill_mon that just takes monster reference
  void explode_mon(int index); // Explode a monster; like kill_mon but messier
  void revive_corpse(int x, int y, int n); // revives a corpse from an item pile
  void revive_corpse(int x, int y, item *it); // revives a corpse by item pointer, caller handles item deletion
// hit_monster_with_flags processes ammo flags (e.g. incendiary, etc)
  void hit_monster_with_flags(monster &critter, const std::set<std::string> &effects);
  void plfire(bool burst, int default_target_x = -1, int default_target_y = -1); // Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
            bool burst);
  void throw_item(player &p, int tarx, int tary, item &thrown,
                  std::vector<point> &trajectory);
  void cancel_activity();
  bool cancel_activity_query(const char* message, ...);
  bool cancel_activity_or_ignore_query(const char* reason, ...);
  void moving_vehicle_dismount(int tox, int toy);
  // Get input from the player to choose an adjacent tile (for examine() etc)
  bool choose_adjacent(std::string message, int &x, int&y);

  int assign_mission_id(); // Just returns the next available one
  void give_mission(mission_id type); // Create the mission and assign it
  void assign_mission(int id); // Just assign an existing mission
// reserve_mission() creates a new mission of the given type and pushes it to
// active_missions.  The function returns the UID of the new mission, which can
// then be passed to a MacGuffin or something else that needs to track a mission
  int reserve_mission(mission_id type, int npc_id = -1);
  int reserve_random_mission(mission_origin origin, point p = point(-1, -1),
                             int npc_id = -1);
  npc* find_npc(int id);
  int kill_count(std::string mon);       // Return the number of kills of a given mon_id
  mission* find_mission(int id); // Mission with UID=id; NULL if non-existant
  mission_type* find_mission_type(int id); // Same, but returns its type
  bool mission_complete(int id, int npc_id); // True if we made it
  bool mission_failed(int id); // True if we failed it
  void wrap_up_mission(int id); // Perform required actions
  void fail_mission(int id); // Perform required actions, move to failed list
  void mission_step_complete(int id, int step); // Parial completion
  void process_missions(); // Process missions, see if time's run out

  void teleport(player *p = NULL, bool add_teleglow = true);
  void plswim(int x, int y); // Called by plmove.  Handles swimming
  // when player is thrown (by impact or something)
  void fling_player_or_monster(player *p, monster *zz, const int& dir, float flvel, bool controlled = false);

  /**
   * Nuke the area at (x,y) - global overmap terrain coordinates!
   */
  void nuke(int x, int y);
  bool spread_fungus(int x, int y);
  std::vector<faction *> factions_at(int x, int y);
  int& scent(int x, int y);
  float natural_light_level() const;
  unsigned char light_level();
  void reset_light_level();
  int assign_npc_id();
  int assign_faction_id();
  faction* faction_by_id(int it);
  bool sees_u(int x, int y, int &t);
  bool u_see (int x, int y);
  bool u_see (monster *critter);
  bool u_see (Creature *t); // for backwards compatibility
  bool u_see (Creature &t);
  bool is_hostile_nearby();
  bool is_hostile_very_close();
  void refresh_all();
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void update_overmap_seen(); // Update which overmap tiles we can see
  // Position of the player in overmap terrain coordinates, relative
  // to the current overmap (@ref cur_om).
  point om_location() const;
  // Position of the player in overmap terrain coordinates,
  // in global overmap terrain coordinates.
  tripoint om_global_location() const;

  faction* random_good_faction();
  faction* random_evil_faction();

  void process_artifact(item *it, player *p, bool wielded = false);
  void add_artifact_messages(std::vector<art_effect_passive> effects);

  void peek();
  point look_debug();
  point look_around();// Look at nearby terrain ';'
  int list_items(const int iLastState); //List all items around the player
  int list_monsters(const int iLastState); //List all monsters around the player
  // Shared method to print "look around" info
  void print_all_tile_info(int lx, int ly, WINDOW* w_look, int column, int &line, bool mouse_hover);

  bool list_items_match(item &item, std::string sPattern);
  int list_filter_high_priority(std::vector<map_item_stack> &stack, std::string prorities);
  int list_filter_low_priority(std::vector<map_item_stack> &stack,int start, std::string prorities);
  std::vector<map_item_stack> filter_item_stacks(std::vector<map_item_stack> stack, std::string filter);
  std::vector<map_item_stack> find_nearby_items(int iRadius);
  std::vector<int> find_nearby_monsters(int iRadius);
  std::string ask_item_filter(WINDOW* window, int rows);
  std::string ask_item_priority_high(WINDOW* window, int rows);
  std::string ask_item_priority_low(WINDOW* window, int rows);
  void draw_trail_to_square(int x, int y, bool bDrawX);
  void reset_item_list_state(WINDOW* window, int height);
  std::string sFilter; // this is a member so that it's remembered over time
  std::string list_item_upvote;
  std::string list_item_downvote;
  int inv(const std::string& title);
  int inv_activatable(std::string title);
  int inv_type(std::string title, item_cat inv_item_type = IC_NULL);
  int inv_for_liquid(const item &liquid, const std::string title, bool auto_choose_single);
  int display_slice(indexed_invslice&, const std::string&);
  int inventory_item_menu(int pos, int startx = 0, int width = 50, int position = 0);
  // Same as other multidrop, only the dropped_worn vector
  // is merged into the result.
  std::vector<item> multidrop();
  // Select items to drop, removes those items from the players
  // inventory, takes of the selected armor, unwields weapon (if
  // selected).
  // Selected items that had been worn are taken off and put into dropped_worn.
  // Selected items from main inventory and the weapon are returned directly.
  // removed_storage_space contains the summed up storage of the taken
  // of armor. This includes the storage space of the items in dropped_worn
  // and the items that have been autodropped while taking them off.
  std::vector<item> multidrop(std::vector<item> &dropped_worn, int &removed_storage_space);
  faction* list_factions(std::string title = "FACTIONS:");
  point find_item(item *it);
  void remove_item(item *it);

  inventory crafting_inventory(player *p);  // inv_from_map, inv, & 'weapon'
  std::list<item> consume_items(player *p, std::vector<component> components);
  void consume_tools(player *p, std::vector<component> tools, bool force_available);

  bool has_gametype() const { return gamemode && gamemode->id() != SGAME_NULL; }
  special_game_id gametype() const { return (gamemode) ? gamemode->id() : SGAME_NULL; }

  std::map<std::string, vehicle*> vtypes;
  std::vector <trap*> traps;
  void load_trap(JsonObject &jo);
  void toggle_sidebar_style(void);
  void toggle_fullscreen(void);
  void temp_exit_fullscreen(void);
  void reenter_fullscreen(void);

  std::map<std::string, std::vector <items_location_and_chance> > monitems;
  std::vector <mission_type> mission_types; // The list of mission templates

  calendar turn;
  signed char temperature;              // The air temperature
  int get_temperature();    // Returns outdoor or indoor temperature of current location
  weather_type weather;   // Weather pattern--SEE weather.h
  bool lightning_active;

  std::map<int, weather_segment> weather_log;
  char nextinv; // Determines which letter the next inv item will have
  overmap *cur_om;
  map m;
  int levx, levy, levz; // Placement inside the overmap
  player u;
  std::vector<monster> coming_to_stairs;
  int monstairx, monstairy, monstairz;
  std::vector<npc *> active_npc;
  std::vector<faction> factions;
  std::vector<mission> active_missions; // Missions which may be assigned
// NEW: Dragging a piece of furniture, with a list of items contained
  ter_id dragging;
  std::vector<item> items_dragged;
  int weight_dragged; // Computed once, when you start dragging
  bool debugmon;

  std::map<int, std::map<int, bool> > mapRain;

  int ter_view_x, ter_view_y;
  WINDOW *w_terrain;
  WINDOW *w_minimap;
  WINDOW *w_HP;
  WINDOW *w_messages;
  WINDOW *w_location;
  WINDOW *w_status;
  WINDOW *w_status2;
  live_view liveview;

  bool handle_liquid(item &liquid, bool from_ground, bool infinite, item *source = NULL, item *cont = NULL);

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
 void knockback(std::vector<point>& traj, int force, int stun, int dam_mult);

 // shockwave applies knockback to all targets within radius of (x,y)
 // parameters force, stun, and dam_mult are passed to knockback()
 // ignore_player determines if player is affected, useful for bionic, etc.
 void shockwave(int x, int y, int radius, int force, int stun, int dam_mult, bool ignore_player);


// Animation related functions
  void draw_explosion(int x, int y, int radius, nc_color col);
  void draw_bullet(Creature &p, int tx, int ty, int i, std::vector<point> trajectory, char bullet, timespec &ts);
  void draw_hit_mon(int x, int y, monster critter, bool dead = false);
  void draw_hit_player(player *p, bool dead = false);
  void draw_line(const int x, const int y, const point center_point, std::vector<point> ret);
  void draw_line(const int x, const int y, std::vector<point> ret);
  void draw_weather(weather_printable wPrint);

// Vehicle related JSON loaders and variables
  void load_vehiclepart(JsonObject &jo);
  void load_vehicle(JsonObject &jo);
  void reset_vehicleparts();
  void reset_vehicles();
  void finalize_vehicles();

  void load_monitem(JsonObject &jo);     // Load monster inventory selection entry
  void reset_monitems();

  std::queue<vehicle_prototype*> vehprototypes;

  nc_color limb_color(player *p, body_part bp, int side, bool bleed = true,
                       bool bite = true, bool infect = true);

  bool opening_screen();// Warn about screen size, then present the main menu

  const int dangerous_proximity;
  bool narrow_sidebar;
  bool fullscreen;
  bool was_fullscreen;

 private:
// Game-start procedures
  void print_menu(WINDOW* w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY, bool bShowDDA = true);
  void print_menu_items(WINDOW* w_in, std::vector<std::string> vItems, int iSel,
                        int iOffsetY, int iOffsetX, int spacing = 1);
  bool load_master(std::string worldname); // Load the master data file, with factions &c
  void load_weather(std::ifstream &fin);
  void load(std::string worldname, std::string name); // Load a player-specific save file
  void start_game(std::string worldname); // Starts a new game in a world
  void start_special_game(special_game_id gametype); // See gamemode.cpp

  //private save functions.
  void save_factions_missions_npcs();
  void serialize_master(std::ofstream &fout);
  void save_artifacts();
  void save_maps();
  void save_weather(std::ofstream &fout);
  void load_legacy_future_weather(std::string data);
  void load_legacy_future_weather(std::istream &fin);
  void save_uistate();
  void load_uistate(std::string worldname);
// Data Initialization
  void init_npctalk();
  void init_fields();
  void init_weather();
  void init_morale();
  void init_itypes();       // Initializes item types
  void init_skills() throw (std::string);
  void init_professions();
  void init_faction_data();
  void init_mongroups() throw (std::string);    // Initualizes monster groups
  void release_traps();     // Release trap types memory
  void init_construction(); // Initializes construction "recipes"
  void init_missions();     // Initializes mission templates
  void init_autosave();     // Initializes autosave parameters
  void init_diseases();     // Initializes disease lookup table.
  void init_savedata_translation_tables();
  void init_lua();          // Initializes lua interpreter.
  void create_factions(); // Creates new factions (for a new game world)
  void load_npcs(); //Make any nearby NPCs from the overmap active.
  void create_starting_npcs(); // Creates NPCs that start near you

// Player actions
  void wishitem( player * p=NULL, int x=-1, int y=-1 );
  void wishmonster( int x=-1, int y=-1 );
  void wishmutate( player * p );
  void wishskill( player * p );
  void mutation_wish(); // Mutate

  void pldrive(int x, int y); // drive vehicle
  // Standard movement; handles attacks, traps, &c. Returns false if auto move
  // should be canceled
  bool plmove(int dx, int dy);
  void wait(); // Long wait (player action)  '^'
  void open(); // Open a door  'o'
  void close(int closex = -1, int closey = -1); // Close a door  'c'
  void smash(); // Smash terrain
  void craft();                        // See crafting.cpp
  void recraft();                      // See crafting.cpp
  void long_craft();                   // See crafting.cpp
  bool crafting_allowed();             // See crafting.cpp
  bool crafting_can_see();             // See crafting.cpp
  recipe* select_crafting_recipe();    // See crafting.cpp
  bool making_would_work(recipe *r);   // See crafting.cpp
  bool can_make(recipe *r);            // See crafting.cpp
  bool can_make_with_inventory(recipe *r, const inventory& crafting_inv);            // See crafting.cpp
    bool check_enough_materials(recipe *r, const inventory& crafting_inv);
  void make_craft(recipe *making);     // See crafting.cpp
  void make_all_craft(recipe *making); // See crafting.cpp
  void complete_craft();               // See crafting.cpp
  void pick_recipes(const inventory& crafting_inv, std::vector<recipe*> &current,
                    std::vector<bool> &available, craft_cat tab, craft_subcat subtab, std::string filter);// crafting.cpp
  void disassemble(int pos = INT_MAX);       // See crafting.cpp
  void complete_disassemble();         // See crafting.cpp
  recipe* recipe_by_index(int index);  // See crafting.cpp

  // Forcefully close a door at (x, y).
  // The function checks for creatures/items/vehicles at that point and
  // might kill/harm/destroy them.
  // If there still remains something that prevents the door from closing
  // (e.g. a very big creatures, a vehicle) the door will not be closed and
  // the function returns false.
  // If the door gets closed the terrain at (x, y) is set to door_type and
  // true is returned.
  // bash_dmg controlls how much damage the door does to the
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
  // open vehicle interaction screen
  void exam_vehicle(vehicle &veh, int examx, int examy, int cx=0, int cy=0);
  void pickup(int posx, int posy, int min);// Pickup items; ',' or via examine()
  // Establish a grab on something.
  void grab();
// Pick where to put liquid; false if it's left where it was

  void compare(int iCompareX = -999, int iCompareY = -999); // Compare two Items 'I'
  void drop(int pos = INT_MIN); // Drop an item  'd'
  void drop_in_direction(); // Drop w/ direction  'D'
  // put items from the item-vector on the map/a vehicle
  // at (dirx, diry), items are dropped into a vehicle part
  // with the cargo flag (if ther eis one), otherwise they are
  // droppend onto the ground.
  void drop(std::vector<item> &dropped, std::vector<item> &dropped_worn, int freed_volume_capacity, int dirx, int diry);
  // calculate the time (in player::moves) it takes to drop the
  // items in dropped and dropped_worn.
  int calculate_drop_cost(std::vector<item> &dropped, const std::vector<item> &dropped_worn, int freed_volume_capacity) const;
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
  void unload(item& it); // Unload a gun/tool  'U'
  void unload(int pos = INT_MIN);
  void wield(int pos = INT_MIN); // Wield a weapon  'w'
  void read(); // Read a book  'R' (or 'a')
  void chat(); // Talk to a nearby NPC  'C'
  void plthrow(int pos = INT_MIN); // Throw an item  't'

  // Internal methods to show "look around" info
  void print_fields_info(int lx, int ly, WINDOW* w_look, int column, int &line);
  void print_terrain_info(int lx, int ly, WINDOW* w_look, int column, int &line);
  void print_trap_info(int lx, int ly, WINDOW* w_look, const int column, int &line);
  void print_object_info(int lx, int ly, WINDOW* w_look, const int column, int &line, bool mouse_hover);
  void handle_multi_item_info(int lx, int ly, WINDOW* w_look, const int column, int &line, bool mouse_hover);
  void get_lookaround_dimensions(int &lookWidth, int &begin_y, int &begin_x) const;

  input_context get_player_input(std::string &action);
// Target is an interactive function which allows the player to choose a nearby
// square.  It display information on any monster/NPC on that square, and also
// returns a Bresenham line to that square.  It is called by plfire() and
// throw().
  std::vector<point> target(int &x, int &y, int lowx, int lowy, int hix,
                            int hiy, std::vector <Creature*> t, int &target,
                            item *relevent);
  // interface to target(), collects a list of targets & selects default target
  // finally calls target() and returns its result.
  std::vector<point> pl_target_ui(int &x, int &y, int range, item *relevent, int default_target_x = -1, int default_target_y = -1);

// Map updating and monster spawning
  void replace_stair_monsters();
  void update_stair_monsters();
  void despawn_monsters(const int shiftx = 0, const int shifty = 0);
  void force_save_monster(monster &critter);
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  int valid_group(std::string type, int x, int y, int z);// Picks a group from cur_om
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
  void update_scent();     // Updates the scent map
  bool is_game_over();     // Returns true if the player quit or died
  void place_corpse();     // Place player corpse
  void death_screen();     // Display our stats, "GAME OVER BOO HOO"
  void gameover();         // Ends the game
  void write_msg();        // Prints the messages in the messages list
  void msg_buffer();       // Opens a window with old messages in it
  void draw_minimap();     // Draw the 5x5 minimap
  void draw_HP();          // Draws the player's HP and Power level

//  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
  void autosave();         // automatic quicksaves - Performs some checks before calling quicksave()
  void quicksave();        // Saves the game without quitting

// Input related
  bool handle_mouseview(input_context &ctxt, std::string &action); // Handles box showing items under mouse
  void hide_mouseview(); // Hides the mouse hover box and redraws what was under it

// On-request draw functions
  void draw_overmap();     // Draws the overmap, allows note-taking etc.
  void disp_kills();       // Display the player's kill counts
  void disp_NPCs();        // Currently UNUSED.  Lists global NPCs.
  void list_missions();    // Listed current, completed and failed missions.

// Debug functions
  void debug();           // All-encompassing debug screen.  TODO: This.
  void display_scent();   // Displays the scent map
  void mondebug();        // Debug monster behavior directly
  void groupdebug();      // Get into on monster groups

// ########################## DATA ################################

  Creature_tracker critter_tracker;

  int last_target; // The last monster targeted
  bool last_target_was_npc;
  int run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
                //  monsters spawns, go to 2 - No movement allowed
  std::vector<int> new_seen_mon;
  int mostseen;  // # of mons seen last turn; if this increases, run_mode++
  bool autosafemode; // is autosafemode enabled?
  bool safemodeveh; // safemode while driving?
  int turnssincelastmon; // needed for auto run mode
//  quit_status uquit;    // Set to true if the player quits ('Q')

  calendar nextspawn; // The turn on which monsters will spawn next.
  calendar nextweather; // The turn on which weather will shift next.
  int next_npc_id, next_faction_id, next_mission_id; // Keep track of UIDs
  std::vector <game_message> messages;   // Messages to be printed
  int curmes;   // The last-seen message.
  int grscent[SEEX * MAPSIZE][SEEY * MAPSIZE]; // The scent map
  //int monmap[SEEX * MAPSIZE][SEEY * MAPSIZE]; // Temp monster map, for mon_at()
  int nulscent;    // Returned for OOB scent checks
  std::vector<event> events;         // Game events to be processed
  std::map<std::string, int> kills;         // Player's kill count
  int moves_since_last_save;
  int item_exchanges_since_save;
  time_t last_save_timestamp;
  unsigned char latest_lightlevel;
  calendar latest_lightlevel_turn;

  special_game *gamemode;

  int moveCount; //Times the player has moved (not pause, sleep, etc)
  const int lookHeight; // Look Around window height

  // Preview for auto move route
  std::vector<point> destination_preview;

  bool is_hostile_within(int distance);
    void activity_on_turn();
    void activity_on_turn_game();
    void activity_on_turn_refill_vehicle();
    void activity_on_turn_pulp();
    void activity_on_finish();
    void activity_on_finish_reload();
    void activity_on_finish_read();
    void activity_on_finish_train();
    void activity_on_finish_firstaid();
    void activity_on_finish_fish();
    void activity_on_finish_vehicle();
};

#endif
