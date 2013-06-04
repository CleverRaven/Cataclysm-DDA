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
#include "action.h"
#include <vector>
#include <map>
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

#define PICKUP_RANGE 2

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
 QUIT_DELETE_WORLD  // Quit and delete world
};

struct monster_and_count
{
 monster mon;
 int count;
 monster_and_count(monster M, int C) : mon (M), count (C) {};
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
struct mutation_branch;

class game
{
 public:
  game();
  ~game();
  void init_ui();
  void setup();
  bool game_quit(); // True if we actually quit the game - used in main.cpp
  quit_status uquit;    // used in main.cpp to determine what type of quit
  void save();
  void delete_save();
  void cleanup_at_end();
  bool do_turn();
  void draw();
  void draw_ter(int posx = -999, int posy = -999);
  void advance_nextinv();	// Increment the next inventory letter
  void decrease_nextinv();	// Decrement the next inventory letter
  void vadd_msg(const char* msg, va_list ap );
  void add_msg(const char* msg, ...);
  void add_msg_if_player(player *p, const char* msg, ...);
  void add_event(event_type type, int on_turn, int faction_id = -1,
                 int x = -1, int y = -1);
  bool event_queued(event_type type);
// Sound at (x, y) of intensity (vol), described to the player is (description)
  void sound(int x, int y, int vol, std::string description);
// creates a list of coordinates to draw footsteps
  void add_footstep(int x, int y, int volume, int distance, monster* source);
  std::vector<std::vector<point> > footsteps;
  std::vector<monster*> footsteps_source;
// visual cue to monsters moving out of the players sight
  void draw_footsteps();
// Explosion at (x, y) of intensity (power), with (shrapnel) chunks of shrapnel
  void explosion(int x, int y, int power, int shrapnel, bool fire);
// Flashback at (x, y)
  void flashbang(int x, int y);
// Move the player vertically, if (force) then they fell
  void vertical_move(int z, bool force);
  void use_computer(int x, int y);
  bool pl_refill_vehicle (vehicle &veh, int part, bool test=false);
  void resonance_cascade(int x, int y);
  void scrambler_blast(int x, int y);
  void emp_blast(int x, int y);
  int  npc_at(int x, int y);	// Index of the npc at (x, y); -1 for none
  int  npc_by_id(int id);	// Index of the npc at (x, y); -1 for none
 // void build_monmap();		// Caches data for mon_at()
  int  mon_at(int x, int y);	// Index of the monster at (x, y); -1 for none
  bool is_empty(int x, int y);	// True if no PC, no monster, move cost > 0
  bool isBetween(int test, int down, int up);
  bool is_in_sunlight(int x, int y); // Checks outdoors + sunny
// Kill that monster; fixes any pointers etc
  void kill_mon(int index, bool player_did_it = false);
  void explode_mon(int index);	// Explode a monster; like kill_mon but messier
  void revive_corpse(int x, int y, int n); // revives a corpse from an item pile
  void revive_corpse(int x, int y, item *it); // revives a corpse by item pointer, caller handles item deletion
// hit_monster_with_flags processes ammo flags (e.g. incendiary, etc)
  void hit_monster_with_flags(monster &z, unsigned int flags);
  void plfire(bool burst);	// Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
            bool burst);
  void throw_item(player &p, int tarx, int tary, item &thrown,
                  std::vector<point> &trajectory);
  void cancel_activity();
  void cancel_activity_query(const char* message, ...);
  bool cancel_activity_or_ignore_query(const char* reason, ...); 

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
  mission* find_mission(int id); // Mission with UID=id; NULL if non-existant
  mission_type* find_mission_type(int id); // Same, but returns its type
  bool mission_complete(int id, int npc_id); // True if we made it
  bool mission_failed(int id); // True if we failed it
  void wrap_up_mission(int id); // Perform required actions
  void fail_mission(int id); // Perform required actions, move to failed list
  void mission_step_complete(int id, int step); // Parial completion
  void process_missions(); // Process missions, see if time's run out

  void teleport(player *p = NULL);
  void plswim(int x, int y); // Called by plmove.  Handles swimming
  // when player is thrown (by impact or something)
  void fling_player_or_monster(player *p, monster *zz, const int& dir, float flvel);

  void nuke(int x, int y);
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
  bool u_see (monster *mon);
  bool pl_sees(player *p, monster *mon, int &t);
  void refresh_all();
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void update_overmap_seen(); // Update which overmap tiles we can see
  point om_location(); // levx and levy converted to overmap coordinates

  faction* random_good_faction();
  faction* random_evil_faction();

  itype* new_artifact();
  itype* new_natural_artifact(artifact_natural_property prop = ARTPROP_NULL);
  void process_artifact(item *it, player *p, bool wielded = false);
  void add_artifact_messages(std::vector<art_effect_passive> effects);

  void peek();
  point look_around();// Look at nearby terrain	';'
  void list_items(); //List all items around the player
  bool list_items_match(std::string sText, std::string sPattern);
  int list_filter_high_priority(std::vector<map_item_stack> &stack, std::string prorities);
  int list_filter_low_priority(std::vector<map_item_stack> &stack,int start, std::string prorities);
  std::vector<map_item_stack> filter_item_stacks(std::vector<map_item_stack> stack, std::string filter);
  std::vector<map_item_stack> find_nearby_items(int search_x, int search_y);
  std::string ask_item_filter(WINDOW* window, int rows);
  void draw_trail_to_square(std::vector<point>& vPoint, int x, int y);
  void reset_item_list_state(WINDOW* window, int height);
  std::string sFilter; // this is a member so that it's remembered over time
  std::string list_item_upvote;
  std::string list_item_downvote;
  char inv(std::string title = "Inventory:");
  char inv_type(std::string title = "Inventory:", item_cat inv_item_type = IC_NULL);
  std::vector<item> multidrop();
  faction* list_factions(std::string title = "FACTIONS:");
  point find_item(item *it);
  void remove_item(item *it);

  inventory crafting_inventory(player *p);  // inv_from_map, inv, & 'weapon'
  std::list<item> consume_items(player *p, std::vector<component> components);
  void consume_tools(player *p, std::vector<component> tools, bool force_available);

  bool has_gametype() const { return gamemode && gamemode->id() != SGAME_NULL; }
  special_game_id gametype() const { return (gamemode) ? gamemode->id() : SGAME_NULL; }

  std::map<std::string, itype*> itypes;
  std::vector <mtype*> mtypes;
  std::vector <vehicle*> vtypes;
  std::vector <trap*> traps;
  recipe_map recipes;	// The list of valid recipes
  std::vector<constructable*> constructions; // The list of constructions

  std::vector <itype_id> mapitems[num_itloc]; // Items at various map types
  std::vector <items_location_and_chance> monitems[num_monsters];
  std::vector <mission_type> mission_types; // The list of mission templates
  mutation_branch mutation_data[PF_MAX2]; // Mutation data
  std::map<char, action_id> keymap;

  calendar turn;
  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h

  std::list<weather_segment> future_weather;

  char nextinv;	// Determines which letter the next inv item will have
  overmap *cur_om;
  map m;
  int levx, levy, levz;	// Placement inside the overmap
  player u;
  std::vector<monster> z;
  std::vector<monster_and_count> coming_to_stairs;
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

  WINDOW *w_terrain;
  WINDOW *w_minimap;
  WINDOW *w_HP;
  WINDOW *w_moninfo;
  WINDOW *w_messages;
  WINDOW *w_location;
  WINDOW *w_status;
  WINDOW *w_void; //space unter status if viewport Y > 12
  overmap *om_hori, *om_vert, *om_diag; // Adjacent overmaps

 bool handle_liquid(item &liquid, bool from_ground, bool infinite);

 void open_gate( game *g, const int examx, const int examy, const enum ter_id handle_type );

 recipe* recipe_by_name(std::string name); // See crafting.cpp

 bionic_id random_good_bionic() const; // returns a non-faulty, valid bionic

 void load_artifacts(); // Load artifact data
                        // Needs to be called by main() before MAPBUFFER.load

 private:
// Game-start procedures
  bool opening_screen();// Warn about screen size, then present the main menu
  void print_menu(WINDOW* w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY, bool bShowDDA = true);
  void print_menu_items(WINDOW* w_in, std::vector<std::string> vItems, int iSel, int iOffsetY, int iOffsetX);
  bool load_master();	// Load the master data file, with factions &c
  void load_weather(std::ifstream &fin);
  void load(std::string name);	// Load a player-specific save file
  void start_game();	// Starts a new game
  void start_special_game(special_game_id gametype); // See gamemode.cpp

  //private save functions.
		void save_factions_missions_npcs();
  void save_artifacts();
	 void save_maps();
  std::string save_weather() const;

// Data Initialization
  void init_itypes();       // Initializes item types
  void init_skills();
  void init_bionics();      // Initializes bionics... for now.
  void init_mapitems();     // Initializes item placement
  void init_mtypes();       // Initializes monster types
  void init_mongroups();    // Initualizes monster groups
  void init_monitems();     // Initializes monster inventory selection
  void init_traps();        // Initializes trap types
  void init_recipes();      // Initializes crafting recipes
  void init_construction(); // Initializes construction "recipes"
  void init_missions();     // Initializes mission templates
  void init_mutations();    // Initializes mutation "tech tree"
  void init_vehicles();     // Initializes vehicle types
  void init_autosave();     // Initializes autosave parameters

  void load_keyboard_settings(); // Load keybindings from disk

  void save_keymap();		// Save keybindings to disk
  std::vector<char> keys_bound_to(action_id act); // All keys bound to act
  void clear_bindings(action_id act); // Deletes all keys bound to act

  void create_factions();   // Creates new factions (for a new game world)
  void load_npcs(); //Make any nearby NPCs from the overmap active.
  void create_starting_npcs(); // Creates NPCs that start near you

// Player actions
  void wish();	// Cheat by wishing for an item 'Z'
  void monster_wish(); // Create a monster
  void mutation_wish(); // Mutate

  void pldrive(int x, int y); // drive vehicle
  void plmove(int x, int y); // Standard movement; handles attacks, traps, &c
  void wait();	// Long wait (player action)	'^'
  void open();	// Open a door			'o'
  void close();	// Close a door			'c'
  void smash();	// Smash terrain
  void craft();                        // See crafting.cpp
  void recraft();                      // See crafting.cpp
  void long_craft();                   // See crafting.cpp
  bool crafting_allowed();             // See crafting.cpp
  recipe* select_crafting_recipe();    // See crafting.cpp
  bool making_would_work(recipe *r);   // See crafting.cpp
  bool can_make(recipe *r);            // See crafting.cpp
    bool check_enough_materials(recipe *r, inventory crafting_inv);
  void make_craft(recipe *making);     // See crafting.cpp
  void make_all_craft(recipe *making); // See crafting.cpp
  void complete_craft();               // See crafting.cpp
  void pick_recipes(std::vector<recipe*> &current,
                    std::vector<bool> &available, craft_cat tab,std::string filter);// crafting.cpp
  void add_known_recipes(std::vector<recipe*> &current, recipe_list source,
                             std::string filter = ""); //crafting.cpp
  craft_cat next_craft_cat(craft_cat cat); // crafting.cpp
  craft_cat prev_craft_cat(craft_cat cat); // crafting.cpp
  void disassemble(char ch = 0);       // See crafting.cpp
  void complete_disassemble();         // See crafting.cpp
  recipe* recipe_by_index(int index);  // See crafting.cpp
  void construction_menu();            // See construction.cpp
  bool player_can_build(player &p, inventory inv, constructable* con,
                        const int level = -1, bool cont = false,
			bool exact_level=false);
  void place_construction(constructable *con); // See construction.cpp
  void complete_construction();               // See construction.cpp
  bool pl_choose_vehicle (int &x, int &y);
  bool vehicle_near ();
  void handbrake ();
  void examine();// Examine nearby terrain	'e'
  void advanced_inv();
  // open vehicle interaction screen
  void exam_vehicle(vehicle &veh, int examx, int examy, int cx=0, int cy=0);
  void pickup(int posx, int posy, int min);// Pickup items; ',' or via examine()
// Pick where to put liquid; false if it's left where it was

  void compare(int iCompareX = -999, int iCompareY = -999); // Compare two Items	'I'
  void drop(char chInput = '.');	  // Drop an item		'd'
  void drop_in_direction(); // Drop w/ direction 'D'
  void reassign_item(char ch = '.'); // Reassign the letter of an item   '='
  void butcher(); // Butcher a corpse		'B'
  void complete_butcher(int index);	// Finish the butchering process
  void forage();	// Foraging ('a' on underbrush)
  void eat(char chInput = '.');	  // Eat food or fuel		'E' (or 'a')
  void use_item(char chInput = '.');// Use item; also tries E,R,W	'a'
  void use_wielded_item();
  void wear(char chInput = '.');	  // Wear armor			'W' (or 'a')
  void takeoff(char chInput = '.'); // Remove armor		'T'
  void reload();  // Reload a wielded gun/tool	'r'
  void reload(char chInput);
  void unload(item& it);  // Unload a gun/tool	'U'
  void unload(char chInput);
  void wield(char chInput = '.');   // Wield a weapon		'w'
  void read();    // Read a book		'R' (or 'a')
  void chat();    // Talk to a nearby NPC	'C'
  void plthrow(char chInput = '.'); // Throw an item		't'
  void help();    // Help screen		'?'
  void show_options();    // Options screen		'?'

// Target is an interactive function which allows the player to choose a nearby
// square.  It display information on any monster/NPC on that square, and also
// returns a Bresenham line to that square.  It is called by plfire() and
// throw().
  std::vector<point> target(int &x, int &y, int lowx, int lowy, int hix,
                            int hiy, std::vector <monster> t, int &target,
                            item *relevent);

// Map updating and monster spawning
  void replace_stair_monsters();
  void update_stair_monsters();
  void despawn_monsters(const bool stairs = false, const int shiftx = 0, const int shifty = 0);
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  int valid_group(mon_id type, int x, int y, int z);// Picks a group from cur_om
  void set_adjacent_overmaps(bool from_scratch = false);

// Routine loop functions, approximately in order of execution
  void cleanup_dead();     // Delete any dead NPCs/monsters
  void monmove();          // Monster movement
  void rustCheck();        // Degrades practice levels
  void process_events();   // Processes and enacts long-term events
  void process_activity(); // Processes and enacts the player's activity
  void update_weather();   // Updates the temperature and weather patten
  void hallucinate(const int x, const int y); // Prints hallucination junk to the screen
  void mon_info();         // Prints a list of nearby monsters (top right)
  void handle_key_blocking_activity(); // Abort reading etc.
  bool handle_action();
  void update_scent();     // Updates the scent map
  bool is_game_over();     // Returns true if the player quit or died
  void death_screen();     // Display our stats, "GAME OVER BOO HOO"
  void gameover();         // Ends the game
  void write_msg();        // Prints the messages in the messages list
  void msg_buffer();       // Opens a window with old messages in it
  void draw_minimap();     // Draw the 5x5 minimap
  void draw_HP();          // Draws the player's HP and Power level
  int autosave_timeout();  // If autosave enabled, how long we should wait for user inaction before saving.
  void autosave();         // Saves map

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

  signed char last_target;// The last monster targeted
  char run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
		 //  monsters spawns, go to 2 - No movement allowed
  int mostseen;	 // # of mons seen last turn; if this increases, run_mode++
  bool autosafemode; // is autosafemode enabled?
  int turnssincelastmon; // needed for auto run mode
//  quit_status uquit;    // Set to true if the player quits ('Q')

  calendar nextspawn; // The turn on which monsters will spawn next.
  calendar nextweather; // The turn on which weather will shift next.
  int next_npc_id, next_faction_id, next_mission_id; // Keep track of UIDs
  std::vector <game_message> messages;   // Messages to be printed
  int curmes;	  // The last-seen message.
  int grscent[SEEX * MAPSIZE][SEEY * MAPSIZE];	// The scent map
  //int monmap[SEEX * MAPSIZE][SEEY * MAPSIZE]; // Temp monster map, for mon_at()
  int nulscent;				// Returned for OOB scent checks
  std::vector<event> events;	        // Game events to be processed
  int kills[num_monsters];	        // Player's kill count
  std::string last_action;		// The keypresses of last turn
  int moves_since_last_save;
  int item_exchanges_since_save;
  time_t last_save_timestamp;
  unsigned char latest_lightlevel;
  calendar latest_lightlevel_turn;

  special_game *gamemode;

  int moveCount; //Times the player has moved (not pause, sleep, etc)
};

#endif
