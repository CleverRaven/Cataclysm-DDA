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
ME_BOULDERING,  // Monster is moving over rubble
ME_BOUNCED,
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
 void print_info(game *g, WINDOW* w, int vStart = 6); // Prints information to w.
 char symbol();			// Just our type's symbol; no context
 void draw(WINDOW* w, int plx, int ply, bool inv);
 nc_color color_with_effects();	// Color with fire, beartrapped, etc.
				// Inverts color if inv==true
 bool has_flag(m_flag f);	// Returns true if f is set (see mtype.h)
 bool can_see();		// MF_SEES and no ME_BLIND
 bool can_hear();		// MF_HEARS and no ME_DEAF
 bool can_submerge();           // MF_AQUATIC or MF_SWIMS or MF_NO_BREATH, and not MF_ELECTRONIC
 bool can_drown();              // MF_AQUATIC or MF_SWIMS or MF_NO_BREATHE or MF_FLIES
 bool made_of(std::string m);	// Returns true if it's made of m
 bool made_of(phase_id p); // Returns true if its phase is p
 bool json_load(picojson::value parsed, std::vector<mtype*> *mtypes);
 void json_load(picojson::value parsed, game * g);

 void load_info(std::string data, std::vector<mtype*> *mtypes);
 
 virtual picojson::value json_save(bool save_contents = false);
 std::string save_info();	// String of all data, for save files
 void debug(player &u); 	// Gives debug info

// Movement
 void receive_moves();		// Gives us movement points
 void shift(int sx, int sy); 	// Shifts the monster to the appropriate submap
			     	// Updates current pos AND our plans
 bool wander(); 		// Returns true if we have no plans

 /**
  * Checks whether we can move to/through (x, y).
  *
  * This is used in pathfinding and ONLY checks the terrain. It ignores players
  * and monsters, which might only block this tile temporarily.
  */
 bool can_move_to(game *g, int x, int y);

 bool will_reach(game *g, int x, int y); // Do we have plans to get to (x, y)?
 int  turns_to_reach(game *g, int x, int y); // How long will it take?

 void set_dest(int x, int y, int &t); // Go in a straight line to (x, y)
				      // t determines WHICH Bresenham line

 /**
  * Set (x, y) as wander destination.
  *
  * This will cause the monster to slowly move towards the destination,
  * unless there is an overriding smell or plan.
  *
  * @param f The priority of the destination, as well as how long we should
  *          wander towards there.
  */
 void wander_to(int x, int y, int f); // Try to get to (x, y), we don't know
				      // the route.  Give up after f steps.
 void plan(game *g, const std::vector<int> &friendlies);
 void move(game *g); // Actual movement
 void footsteps(game *g, int x, int y); // noise made by movement
 void friendly_move(game *g);

 point scent_move(game *g);
 point wander_next(game *g);
 void hit_player(game *g, player &p, bool can_grab = true);
 int calc_movecost(game *g, int x1, int y1, int x2, int y2);

 /**
  * Attempt to move to (x,y).
  *
  * If there's something blocking the movement, such as infinite move
  * costs at the target, an existing NPC or monster, this function simply
  * aborts and does nothing.
  *
  * @param force If this is set to true, the movement will happen even if
  *              there's currently something blocking the destination.
  *
  * @return 1 if movement successful, 0 otherwise
  */
 int move_to(game *g, int x, int y, bool force=false);

 /**
  * Attack any enemies at the given location.
  *
  * Attacks only if there is a creature at the given location towards
  * we are hostile.
  *
  * @return 1 if something was attacked, 0 otherwise
  */
 int attack_at(int x, int y);

 /**
  * Try to smash/bash/destroy your way through the terrain at (x, y).
  *
  * @return 1 if we destroyed something, 0 otherwise.
  */
 int bash_at(int x, int y);

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
 // Deals this dam damage; returns true if we dead
 // If real_dam is provided, caps overkill at real_dam.
 bool hurt(int dam, int real_dam = 0);
 int  armor_cut();	// Natural armor, plus any worn armor
 int  armor_bash();	// Natural armor, plus any worn armor
 int  dodge();		// Natural dodge, or 0 if we're occupied
 int  dodge_roll();	// For the purposes of comparing to player::hit_roll()
 int  fall_damage();	// How much a fall hurts us
 void die(game *g);
 void drop_items_on_death(game *g);

// Other
 void add_effect(monster_effect_type effect, int duration);
 bool has_effect(monster_effect_type effect); // True if we have the effect
 void rem_effect(monster_effect_type effect); // Remove a given effect
 void process_effects(game *g);	// Process long-term effects
 bool make_fungus(game *g);	// Makes this monster into a fungus version
				// Returns false if no such monster exists
 void make_friendly();
 void add_item(item it);	// Add an item to inventory

 bool is_hallucination();       // true if the monster isn't actually real

// TEMP VALUES
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
 bool no_extra_death_drops; // if true, don't spawn loot items as part of death
 bool dead;
 bool made_footstep;
 std::string unique_name; // If we're unique
 bool hallucination;

 bool setpos(const int x, const int y, const bool level_change = false);
 bool setpos(const point &p, const bool level_change = false);
 inline int posx() const { return _posx; }
 inline int posy() const { return _posy; }

private:
 std::vector <point> plans;
 int _posx, _posy;
};

#endif
