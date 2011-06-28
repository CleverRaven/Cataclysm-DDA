#ifndef _GAME_H_
#define _GAME_H_

#include "mtype.h"
#include "monster.h"
#include "map.h"
#include "player.h"
#include "overmap.h"
#include "omdata.h"
#include "mapitems.h"
#include "crafting.h"
#include "trap.h"
#include "npc.h"
#include "tutorial.h"
#include "faction.h"
#include "event.h"
#include "mission.h"
#include "weather.h"
#include <vector>

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

enum tut_type {
 TUT_NULL,
 TUT_BASIC, TUT_COMBAT,
 TUT_MAX
};

struct mtype;
struct mission_type;
class map;
class player;

class game
{
 public:
  game();
  ~game();
  void save();
  bool do_turn();
  void tutorial_message(tut_lesson lesson);
  void draw();
  void draw_ter();
  void advance_nextinv();	// Increment the next inventory letter
  void add_msg(const char* msg, ...);
  void add_event(event_type type, int on_turn, faction* rel);
// Sound at (x, y) of intensity (vol), described to the player is (description)
  void sound(int x, int y, int vol, std::string description);
// Explosion at (x, y) of intensity (power), with (shrapnel) chunks of shrapnel
  void explosion(int x, int y, int power, int shrapnel, bool fire);
// Move the player vertically, if (force) then they fell
  void vertical_move(int z, bool force);
  void use_computer(int x, int y);
  void resonance_cascade(int x, int y);
  void emp_blast(int x, int y);
  int  npc_at(int x, int y);	// Index of the npc at (x, y); -1 for none
  int  mon_at(int x, int y);	// Index of the monster at (x, y); -1 for none
  bool is_empty(int x, int y);	// True if no PC, no monster, move cost > 0
  bool is_in_sunlight(int x, int y); // Checks outdoors + sunny
  void kill_mon(int index);	// Kill that monster; fixes any pointers etc
  void explode_mon(int index);	// Explode a monster; like kill_mon but messier
  void plfire(bool burst);	// Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
            bool burst);
  void throw_item(player &p, int tarx, int tary, item &thrown,
                  std::vector<point> &trajectory);
  void cancel_activity();
  void cancel_activity_query(std::string message);
  void give_mission(mission_id type);
// reserve_mission() creates a new mission of the given type and pushes it to
// active_missions.  The function returns the UID of the new mission, which can
// then be passed to a MacGuffin or something else that needs to track a mission
  int reserve_mission(mission_id type);
  mission* find_mission(int id); // Mission with UID=id; NULL if non-existant
  void teleport();
  void nuke(int x, int y);
  std::vector<faction *> factions_at(int x, int y);
  int& scent(int x, int y);
  unsigned char light_level();
  int assign_npc_id();
  bool sees_u(int x, int y, int &t);
  bool u_see (int x, int y, int &t);
  bool u_see (monster *mon, int &t);
  bool pl_sees(player *p, monster *mon, int &t);
  void refresh_all();

  faction* random_good_faction();
  faction* random_evil_faction();

  char inv(std::string title = "INVENTORY:");
  faction* list_factions(std::string title = "FACTIONS:");
  point find_item(item *it);
  void remove_item(item *it);

  std::vector <itype*> itypes;
  std::vector <mtype*> mtypes;
  std::vector <trap*> traps;
  std::vector <itype_id> mapitems[num_itloc]; // Items at various map types
  std::vector <items_location_and_chance> monitems[num_monsters];
  unsigned int turn;
  char nextinv;	// Determines which letter the next inv item will have
  overmap cur_om;
  map m;
  int levx, levy, levz;	// Placement inside the overmap
  player u;
  std::vector<monster> z;
  std::vector<monster> monbuff;
  int monbuffx, monbuffy, monbuffz, monbuff_turn;
  std::vector<npc> active_npc;
  std::vector<mon_id> moncats[num_moncats];
  std::vector<faction> factions;
  bool debugmon;
// Display data... TODO: Make this more portable?
  WINDOW *w_terrain;
  WINDOW *w_minimap;
  WINDOW *w_HP;
  WINDOW *w_moninfo;
  WINDOW *w_messages;
  WINDOW *w_status;
 private:
// Game-start procedures
  bool opening_screen();// Warn about screen size, then present the main menu
  bool load_master();	// Load the master data file, with factions &c
  void load(std::string name);	// Load a player-specific save file
  void start_game();	// Starts a new game
  void start_tutorial(tut_type type);	// Starts a new tutorial

// Data Initialization
  void init_itypes();	// Initializes item types
  void init_mapitems();	// Initializes item placement
  void init_mtypes();	// Initializes monster types
  void init_moncats();	// Initializes monster categories
  void init_monitems();	// Initializes monster inventory selection
  void init_traps();	// Initializes trap types
  void init_recipes();	// Initializes crafting recipes
  void init_missions();	// Initializes mission templates

  void create_factions();	// Creates new factions (for a new game world)

// Player actions
  void wish();	// Cheat by wishing for an item 'Z'
  void plmove(int x, int y);	// Standard movement; handles attacks, traps, &c
  void plswim(int x, int y);	// Called by plmove.  Handles swimming

  void wait();	// Long wait (player action)	'^'
  void open();	// Open a door			'o'
  void close();	// Close a door			'c'
  void smash();	// Smash terrain		
  void craft();				// See crafting.cpp
  void make_craft(recipe *making);	// See crafting.cpp
  void complete_craft();		// See crafting.cpp
  void pick_recipes(std::vector<recipe*> &current,
                    std::vector<bool> &available, craft_cat tab);// crafting.cpp
  void examine();// Examine nearby terrain	'e'
  void look_around();// Look at nearby terrain	';'
  void pickup(int posx, int posy, int min);// Pickup items; ',' or via examine()
// Pick where to put liquid; false if it's left where it was
  bool handle_liquid(item &liquid, bool from_ground, bool infinite);
  void drop();	  // Drop an item		'd'	TODO: Multidrop
  void butcher(); // Butcher a corpse		'B'
  void complete_butcher(int index);	// Finish the butchering process
  void eat();	  // Eat food or fuel		'E' (or 'a')
  void use_item();// Use item; also tries E,R,W	'a'
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
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  mon_id valid_monster_from(std::vector<mon_id> group);
  int valid_group(mon_id type, int x, int y);// Picks a group from cur_om
  moncat_id mt_to_mc(mon_id type);// Monster type to monster category

// Routine loop functions, approximately in order of execution
  void monmove();          // Monster movement
  void om_npcs_move();     // Movement of NPCs on the overmap (non-local)
  void check_warmth();     // Checks the player's warmth (applying clothing)
  void update_skills();    // Degrades practice levels, checks & upgrades skills
  void process_events();   // Processes and enacts long-term events
  void process_activity(); // Processes and enacts the player's activity
  void update_weather();   // Updates the temperature and weather patten
  void hallucinate();      // Prints hallucination junk to the screen
  void mon_info();         // Prints a list of nearby monsters (top right)
  void get_input();        // Gets player input and calls the proper function
  void update_scent();     // Updates the scent map
  bool is_game_over();     // Returns true if the player quit or died
  void death_screen();     // Display our stats, "GAME OVER BOO HOO"
  void gameover();         // Ends the game
  void write_msg();        // Prints the messages in the messages list
  void draw_minimap();     // Draw the 5x5 minimap
  void draw_HP();          // Draws the player's HP and Power level

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

  bool uquit;    // Set to true if the player quits ('Q')

  int nextspawn;          // The turn on which monsters will spawn next.
  int next_npc_id, next_mission_id;	// Keep track of UIDs
  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h
  season_type season;
  std::vector <std::string> messages;   // Messages to be printed
  unsigned char curmes;	  // The last-seen message.  Older than 256 is deleted.
  int grscent[SEEX * 3][SEEY * 3];	// The scent map
  int nulscent;				// Returned for OOB scent checks
  std::vector<event> events;	        // Game events to be processed
  int kills[num_monsters];	        // Player's kill count
  std::string last_action;		// The keypresses of last turn

  std::vector<recipe> recipes;	// The list of valid recipes
  std::vector<mission_type> mission_types; // The list of mission templates
  std::vector<mission> active_missions; // Missions which may be assigned

  bool tutorials_seen[NUM_LESSONS]; // Which tutorial lessons have we learned
  bool in_tutorial;                 // True if we're in a tutorial right now
};

#endif
