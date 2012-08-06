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
ME_POISONED,		// Slowed, takes damage
ME_ONFIRE,		// Lit aflame
ME_STUNNED,		// Stumbling briefly
ME_DOWNED,		// Knocked down
ME_BLIND,		// Can't use sight
ME_DEAF,		// Can't use hearing
ME_TARGETED,		// Targeting locked on--for robots that shoot guns
ME_DOCILE,		// Don't attack other monsters--for tame monster
ME_HIT_BY_PLAYER,	// We shot or hit them
ME_RUN,			// For hit-and-run monsters; we're running for a bit;
NUM_MONSTER_EFFECTS
};

enum monster_attitude {
MATT_NULL = 0,
MATT_FRIEND,
MATT_FLEE,
MATT_IGNORE,
MATT_FOLLOW,
MATT_ATTACK,
NUM_MONSTER_ATTITUDES
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
 std::string name_with_armor(); // Name, with whatever our armor is called
 void print_info(game *g, WINDOW* w); // Prints information to w.
 char symbol();			// Just our type's symbol; no context
 void draw(WINDOW* w, int plx, int ply, bool inv);
 nc_color color_with_effects();	// Color with fire, beartrapped, etc.
				// Inverts color if inv==true
 bool has_flag(m_flag f);	// Returns true if f is set (see mtype.h)
 bool can_see();		// MF_SEES and no ME_BLIND
 bool can_hear();		// MF_HEARS and no ME_DEAF
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
 bool will_reach(game *g, int x, int y); // Do we have plans to get to (x, y)?
 int  turns_to_reach(game *g, int x, int y); // How long will it take?

 void set_dest(int x, int y, int &t); // Go in a straight line to (x, y)
				      // t determines WHICH Bresenham line
 void wander_to(int x, int y, int f); // Try to get to (x, y), we don't know
				      // the route.  Give up after f steps.
 void plan(game *g);
 void move(game *g); // Actual movement
 void footsteps(game *g, int x, int y); // noise made by movement
 void friendly_move(game *g);

 point scent_move(game *g);
 point sound_move(game *g);
 void hit_player(game *g, player &p, bool can_grab = true);
 void move_to(game *g, int x, int y);
 void stumble(game *g, bool moved);
 void knock_back_from(game *g, int posx, int posy);

// Combat
 bool is_fleeing(player &u);	// True if we're fleeing
 monster_attitude attitude(player *u = NULL);	// See the enum above
 int morale_level(player &u);	// Looks at our HP etc.
 void process_triggers(game *g);// Process things that anger/scare us
 void process_trigger(monster_trigger trig, int amount);// Single trigger
 int trigger_sum(game *g, std::vector<monster_trigger> *triggers);
 int  hit(game *g, player &p, body_part &bp_hit); // Returns a damage
 void hit_monster(game *g, int i);
 bool hurt(int dam); 	// Deals this dam damage; returns true if we dead
 int  armor_cut();	// Natural armor, plus any worn armor
 int  armor_bash();	// Natural armor, plus any worn armor
 int  dodge();		// Natural dodge, or 0 if we're occupied
 int  dodge_roll();	// For the purposes of comparing to player::hit_roll()
 int  fall_damage();	// How much a fall hurts us
 void die(game *g);

// Other
 void add_effect(monster_effect_type effect, int duration);
 bool has_effect(monster_effect_type effect); // True if we have the effect
 void rem_effect(monster_effect_type effect); // Remove a given effect
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
 int anger, morale;
 int faction_id; // If we belong to a faction
 int mission_id; // If we're related to a mission
 mtype *type;
 bool dead;
 bool made_footstep;
 std::string unique_name; // If we're unique

private:
 std::vector <point> plans;
};

#endif
