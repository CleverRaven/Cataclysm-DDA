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
#include <vector>

enum tut_type {
 TUT_NULL,
 TUT_BASIC, TUT_COMBAT,
 TUT_MAX
};

struct mtype;
class map;
class player;

class game
{
 public:
  game();
  ~game();
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
  void kill_mon(int index);	// Kill that monster; fixes any pointers etc
  void plfire(bool burst);	// Player fires a gun (setup of target)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
            bool burst);
  void throw_item(player &p, int tarx, int tary, item &thrown,
                  std::vector<point> &trajectory);
  void cancel_activity();
  void cancel_activity_query(std::string message);
  void teleport();
  int& scent(int x, int y);
  unsigned char light_level();
  bool sees_u(int x, int y, int &t);
  bool u_see (int x, int y, int &t);
  bool u_see (monster *mon, int &t);
  bool pl_sees(player *p, monster *mon, int &t);
  void refresh_all();

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
  bool opening_screen();
  void load(std::string name);
  void save();
  void start_game();
  void start_tutorial(tut_type type);

// Data Initialization
  void init_itypes();
  void init_mapitems();
  void init_mtypes();
  void init_moncats();
  void init_monitems();
  void init_traps();
  void init_recipes();
  void init_factions();

// Player actions
  void wish();
  void plmove(int x, int y);
  void plswim(int x, int y);
  void update_map(int &x, int &y);  // Called by plmove, sometimes
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  mon_id valid_monster_from(std::vector<mon_id> group);
  int valid_group(mon_id type, int x, int y);
  moncat_id mt_to_mc(mon_id type);
  void wait();
  void open();
  void close();
  void smash();
  void craft();				// See crafting.cpp
  void make_craft(recipe *making);	// See crafting.cpp
  void complete_craft();		// See crafting.cpp
  void pick_recipes(std::vector<recipe*> &current,
                    std::vector<bool> &available, craft_cat tab);// crafting.cpp
  void examine();
  void look_around();
  void pickup(int posx, int posy, int min);
  void drop();
  void butcher();
  void eat();
  void use_item();
  void wear();
  void takeoff();
  void reload();
  void unload();
  void wield();
  void read();
  void chat();			// Chat to an NPC
  void plthrow();
  std::vector<point> target(int &x, int &y, int lowx, int lowy, int hix,
                            int hiy, std::vector <monster> t, int &target,
                            item *relevent);
  void help();

// Routine loop functions, approximately in order of execution
  void monmove();
  void om_npcs_move();
  void check_warmth();
  void update_skills();
  void process_events();
  void process_activity();
  void hallucinate();
  void mon_info();
  void get_input();
  void update_scent();
  bool is_game_over();
  void gameover();
  void write_msg();
  void draw_minimap();
  void draw_HP();

// On-request draw functions
  void draw_overmap();
  void disp_kills();
  void disp_NPCs();

// If x & y are OOB, gens a new overmap returns the proper terrain; also, may
//  mark the square as seen by the player
  oter_id ter_at(int x, int y, bool& mark_as_seen);

// Debug functions
  void debug();
  void display_scent();
  void mondebug();
  void groupdebug();
// Data
  signed char last_target;// The last monster targeted
  char run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
		 //  monsters spawns, go to 2 - No movement allowed
  int mostseen;	// # of mons seen last turn; if this increases, run_mode++

  bool uquit;

  int nextspawn;
  signed char temp;
  std::vector <std::string> messages;
  char curmes;	// The last-seen message.  Older than 256 is deleted.
  int grscent[SEEX * 3][SEEY * 3];	// The scent map
  int nulscent;				// Returned for OOB scent checks
  std::vector<recipe> recipes;
  std::vector<event> events;
  int kills[num_monsters];

  bool tutorials_seen[NUM_LESSONS]; // Which tutorial lessons have we learned
  bool in_tutorial;
};

#endif
