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
#include <stdarg.h>

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
  void add_footstep(int x, int y, int volume, int distance);
  std::vector<point> footsteps;
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
  void fling_player_or_monster(player *p, monster *zz, int dir, int flvel);

  void nuke(int x, int y);
  std::vector<faction *> factions_at(int x, int y);
  int& scent(int x, int y);
  float natural_light_level();
  unsigned char light_level();
  void reset_light_level();
  int assign_npc_id();
  int assign_faction_id();
  faction* faction_by_id(int it);
  bool sees_u(int x, int y, int &t);
  bool u_see (int x, int y, int &t);
  bool u_see (monster *mon, int &t);
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
  std::string sFilter;
  char inv(std::string title = "Inventory:");
  char inv_type(std::string title = "Inventory:", int inv_item_type = 0);
  std::vector<item> multidrop();
  faction* list_factions(std::string title = "FACTIONS:");
  point find_item(item *it);
  void remove_item(item *it);

  inventory crafting_inventory();  // inv_from_map, inv, & 'weapon'
  void consume_items(std::vector<component> components);
  void consume_tools(std::vector<component> tools);

  std::vector <itype*> itypes;
  std::vector <mtype*> mtypes;
  std::vector <vehicle*> vtypes;
  std::vector <trap*> traps;
  std::vector<recipe*> recipes;	// The list of valid recipes
  std::vector<constructable*> constructions; // The list of constructions

  std::vector <itype_id> mapitems[num_itloc]; // Items at various map types
  std::vector <items_location_and_chance> monitems[num_monsters];
  std::vector <mission_type> mission_types; // The list of mission templates
  mutation_branch mutation_data[PF_MAX2]; // Mutation data
  std::map<char, action_id> keymap;

  calendar turn;
  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h
  char nextinv;	// Determines which letter the next inv item will have
  overmap cur_om;
  map m;
  light_map lm;
  int levx, levy, levz;	// Placement inside the overmap
  player u;
  std::vector<monster> z;
  std::vector<monster_and_count> coming_to_stairs;
  int monstairx, monstairy, monstairz;
  std::vector<npc> active_npc;
  std::vector<mon_id> moncats[num_moncats];
  std::vector<faction> factions;
  std::vector<mission> active_missions; // Missions which may be assigned
// NEW: Dragging a piece of furniture, with a list of items contained
  ter_id dragging;
  std::vector<item> items_dragged;
  int weight_dragged; // Computed once, when you start dragging
  bool debugmon;
  bool no_npc;
// Display data... TODO: Make this more portable?
  int VIEWX;
  int VIEWY;
  int TERRAIN_WINDOW_WIDTH;
  int TERRAIN_WINDOW_HEIGHT;

  WINDOW *w_terrain;
  WINDOW *w_minimap;
  WINDOW *w_HP;
  WINDOW *w_moninfo;
  WINDOW *w_messages;
  WINDOW *w_location;
  WINDOW *w_status;
  overmap *om_hori, *om_vert, *om_diag; // Adjacent overmaps

 private:
// Game-start procedures
  bool opening_screen();// Warn about screen size, then present the main menu
  bool load_master();	// Load the master data file, with factions &c
  void load(std::string name);	// Load a player-specific save file
  void start_game();	// Starts a new game
  void start_special_game(special_game_id gametype); // See gamemode.cpp

// Data Initialization
  void init_itypes();       // Initializes item types
  void init_mapitems();     // Initializes item placement
  void init_mtypes();       // Initializes monster types
  void init_moncats();      // Initializes monster categories
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
  void craft();                    // See crafting.cpp
  void make_craft(recipe *making); // See crafting.cpp
  void complete_craft();           // See crafting.cpp
  void pick_recipes(std::vector<recipe*> &current,
                    std::vector<bool> &available, craft_cat tab);// crafting.cpp
  void disassemble();              // See crafting.cpp
  void disassemble_item(recipe *dis);              // See crafting.cpp
  void complete_disassemble();              // See crafting.cpp
  void construction_menu();                   // See construction.cpp
  bool player_can_build(player &p, inventory inv, constructable* con,
                        const int level = -1, bool cont = false,
			bool exact_level=false);
  void place_construction(constructable *con); // See construction.cpp
  void complete_construction();               // See construction.cpp
  bool pl_choose_vehicle (int &x, int &y);
  bool vehicle_near ();
  void handbrake ();
  void examine();// Examine nearby terrain	'e'
  // open vehicle interaction screen
  void exam_vehicle(vehicle &veh, int examx, int examy, int cx=0, int cy=0);
  void pickup(int posx, int posy, int min);// Pickup items; ',' or via examine()
// Pick where to put liquid; false if it's left where it was
  bool handle_liquid(item &liquid, bool from_ground, bool infinite);
  void compare(int iCompareX = -999, int iCompareY = -999); // Compare two Items	'I'
  void drop();	  // Drop an item		'd'
  void drop_in_direction(); // Drop w/ direction 'D'
  void reassign_item(); // Reassign the letter of an item   '='
  void butcher(); // Butcher a corpse		'B'
  void complete_butcher(int index);	// Finish the butchering process
  void forage();	// Foraging ('a' on underbrush)
  void eat();	  // Eat food or fuel		'E' (or 'a')
  void use_item();// Use item; also tries E,R,W	'a'
  void use_wielded_item();
  void wear();	  // Wear armor			'W' (or 'a')
  void takeoff(); // Remove armor		'T'
  void reload();  // Reload a wielded gun/tool	'r'
  void unload();  // Unload a wielded gun/tool	'U'
  void wield();   // Wield a weapon		'w'
  void read();    // Read a book		'R' (or 'a')
  void chat();    // Talk to a nearby NPC	'C'
  void plthrow(); // Throw an item		't'
  void help();    // Help screen		'?'

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
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  mon_id valid_monster_from(std::vector<mon_id> group);
  int valid_group(mon_id type, int x, int y);// Picks a group from cur_om
  moncat_id mt_to_mc(mon_id type);// Monster type to monster category
  void set_adjacent_overmaps(bool from_scratch = false);

// Routine loop functions, approximately in order of execution
  void cleanup_dead();     // Delete any dead NPCs/monsters
  void monmove();          // Monster movement
  void rustCheck();        // Degrades practice levels
  void update_bodytemp();  // Maintains body temperature
  void process_events();   // Processes and enacts long-term events
  void process_activity(); // Processes and enacts the player's activity
  void update_weather();   // Updates the temperature and weather patten
  void hallucinate(const int x, const int y); // Prints hallucination junk to the screen
  void mon_info();         // Prints a list of nearby monsters (top right)
  input_ret get_input(int timeout_ms);   // Gets player input and calls the proper function
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

// If x & y are OOB, creates a new overmap and returns the proper terrain; also,
// may mark the square as seen by the player
  oter_id ter_at(int x, int y, bool& mark_as_seen);

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
  unsigned char latest_lightlevel;
  calendar latest_lightlevel_turn;

  special_game *gamemode;
};

#endif
