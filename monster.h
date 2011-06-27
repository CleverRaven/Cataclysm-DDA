#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "player.h"
#include "mtype.h"
#include "enums.h"
#include <vector>

class map;
class player;
class game;
class item;

enum monster_effect_type {
ME_NULL = 0,
ME_BEARTRAP,		// Stuck in beartrap
ME_ONFIRE,		// Lit aflame
ME_STUNNED,		// Stumbling briefly
NUM_MONSTER_EFFECTS
};

struct monster_effect
{
 monster_effect_type type;
 int duration;
 monster_effect(monster_effect_type T, int D) : type (T), duration (D) {}
};

class monster {
 public:
 monster();
 monster(mtype *t);
 monster(mtype *t, int x, int y);
 ~monster();
 void poly(mtype *t);
 void spawn(int x, int y); // All this does is moves the monster to x,y

// Access
 std::string name(); 		// Returns the monster's formal name
 void print_info(game *g, WINDOW* w); // Prints information to w.
 char symbol();			// Just our type's symbol; no context
 void draw(WINDOW* w, int plx, int ply, bool inv);
 nc_color color_with_effects();	// Color with fire, beartrapped, etc.
				// Inverts color if inv==true
 bool has_flag(m_flags f);	// Returns true if f is set (see mtype.h)
 bool has_effect(monster_effect_type t); // True if we have the given effect
 bool made_of(material m);	// Returns true if it's made of m
 void load_info(std::string data, std::vector<mtype*> *mtypes);
 std::string save_info();	// String of all data, for save files
 void debug(player &u); 	// Gives debug info

// Movement
 void receive_moves();		// Gives us movement points
 void shift(int sx, int sy); 	// Shifts the monster to the appropriate submap
			     	// Updates current pos AND our plans
 bool wander(); 		// Returns true if we have no plans
 bool can_move_to(map &m, int x, int y); // Can we move to (x, y)?

 void set_dest(int x, int y, int &t); // Go in a straight line to (x, y)
				      // t determines WHICH Bresenham line
 void wander_to(int x, int y, int f); // Try to get to (x, y), we don't know
				      // the route.  Give up after f steps.
 void plan(game *g);
 void move(game *g); // Actual movement
 void friendly_move(game *g);

 point scent_move(game *g);
 point sound_move(game *g);
 void hit_player(game *g, player &p);
 void move_to(game *g, int x, int y);
 void stumble(game *g, bool moved);

// Combat
 bool is_fleeing(player &u);	// True if we're fleeing
 int  hit(player &p, body_part &bp_hit);	// Returns a damage
 void hit_monster(game *g, int i);
 bool hurt(int dam); 	// Deals this dam damage; returns true if we dead
 int  armor();		// Natural armor, plus any worn armor
 int  dodge();		// Natural dodge, or 0 if we're occupied
 int  dodge_roll();	// For the purposes of comparing to player::hit_roll()
 void die(game *g);

// Other
 void add_effect(monster_effect_type effect, int duration);
 void process_effects(game *g);	// Process long-term effects
 bool make_fungus(game *g);	// Makes this monster into a fungus version
				// Returns false if no such monster exists
 void make_friendly();
 void add_item(item it);	// Add an item to inventory

// TEMP VALUES
 int posx, posy;
 int wandx, wandy; // Wander destination - Just try to move in that direction
 int wandf;	   // Urge to wander - Increased by sound, decrements each move
 std::vector<item> inv; // Inventory
 std::vector<monster_effect> effects; // Active effects, e.g. on fire

// If we were spawned by the map, store our origin for later use
 int spawnmapx, spawnmapy, spawnposx, spawnposy;

// DEFINING VALUES
 int moves, speed;
 int hp;
 int sp_timeout;
 int friendly;
 mtype *type;
 bool dead;

private:
 std::vector <point> plans;
};

#endif
