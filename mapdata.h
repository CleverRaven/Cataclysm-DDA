#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include <vector>
#include <string>
#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"

class game;
class monster;

#ifndef SEEX 	// SEEX is how far the player can see in the X direction (at
#define SEEX 12	// least, without scrolling).  All map segments will need to be
#endif		// at least this wide. The map therefore needs to be 3x as wide.

#ifndef SEEY	// Same as SEEX
#define SEEY 12 // Requires 2*SEEY+1= 25 vertical squares
#endif	        // Nuts to 80x24 terms. Mostly exists in graphical clients, and
	        // those fatcats can resize.

// mfb(t_flag) converts a flag to a bit for insertion into a bitfield
#ifndef mfb
#define mfb(n) int(pow(2,(int)n))
#endif

enum t_flag {
 transparent, // Player & monsters can see through/past it
 bashable, // Player & monsters can bash this & make it the next in the list
 container,// Items on this square are hidden until looted by the player
 door,     // Can be opened--used for NPC pathfinding.
 flammable,// May be lit on fire
 explodes, // Explodes when on fire
 diggable, // Digging monsters, seeding monsters, digging w/ shovel, etc.
 swimmable,// You (and monsters) swim here
 sharp,	   // May do minor damage to players/monsters passing it
 rough,    // May hurt the player's feet
 sealed,   // Can't 'e' to retrieve items here
 noitem,   // Items "fall off" this space
 goes_down,// Can '>' to go down a level
 goes_up,  // Can '<' to go up a level
 computer, // Used as a computer
 num_t_flags // MUST be last
};

struct ter_t {
 std::string name;
 char sym;
 nc_color color;
 unsigned char movecost;
 unsigned flags : num_t_flags;
};

enum ter_id {
t_null = 0,
t_hole,	// Real nothingness; makes you fall a z-level
// Ground
t_dirt, t_dirtmound, t_pit,
t_rock_floor, t_rubble,
t_grass,
t_metal_floor,
t_pavement, t_pavement_y, t_sidewalk,
t_floor,
t_grate,
t_slime,
// Walls & doors
t_wall_v, t_wall_h,
t_wall_metal_v, t_wall_metal_h,
t_wall_glass_v, t_wall_glass_h,
t_reinforced_glass_v, t_reinforced_glass_h,
t_bars,
t_door_c, t_door_b, t_door_o, t_door_locked, t_door_frame, t_door_boarded,
t_door_metal_c, t_door_metal_o, t_door_metal_locked,
t_bulletin,
t_portcullis,
t_window, t_window_frame, t_window_boarded,
t_rock,
// Tree
t_tree, t_tree_young, t_underbrush,
t_wax, t_floor_wax,
t_fence_v, t_fence_h,
t_railing_v, t_railing_h,
// Water, lava, etc.
t_water_sh, t_water_dp, t_sewage,
t_lava,
// Embellishments
t_bed, t_toilet,
t_gas_pump, t_gas_pump_smashed,
t_missile, t_missile_exploded,
t_counter,
t_radio_tower, t_radio_controls,
t_computer_broken, t_computer_nether, t_computer_lab, t_computer_silo,
// Containers
t_fridge, t_dresser,
t_rack, t_bookcase,
t_dumpster,
t_vat,
// Staircases etc.
t_stairs_down, t_stairs_up, t_manhole, t_ladder, t_slope_down, t_slope_up,
// Special
t_card_reader, t_card_reader_broken, t_manhole_cover, t_slot_machine,
num_terrain_types
};

const ter_t terlist[num_terrain_types] = {  // MUST match enum ter_id above!
{"nothing",	     ' ', c_white,   2,
	mfb(transparent)},
{"empty space",      '#', c_black,   2,
	mfb(transparent)},
{"dirt",	     '.', c_brown,   2,
	mfb(transparent)|mfb(diggable)},
{"mound of dirt",    '#', c_brown,   3,
	mfb(transparent)|mfb(diggable)},
{"shallow pit",	     '0', c_brown,  14,
	mfb(transparent)|mfb(diggable)},
{"rock floor",       '.', c_ltgray,  2,
	mfb(transparent)},
{"pile of rubble",   '#', c_ltgray,  4,
	mfb(transparent)|mfb(rough)|mfb(diggable)},
{"grass",	     '.', c_green,   2,
	mfb(transparent)|mfb(diggable)},
{"metal floor",      '.', c_ltcyan,  2,
	mfb(transparent)},
{"pavement",	     '.', c_dkgray,  2,
	mfb(transparent)},
{"yellow pavement",  '.', c_yellow,  2,
	mfb(transparent)},
{"sidewalk",         '.', c_ltgray,  2,
	mfb(transparent)},
{"floor",	     '.', c_cyan,    2,
	mfb(transparent)},
{"metal grate",      '#', c_dkgray,  2,
	mfb(transparent)},
{"slime",            '~', c_green,   6,
	mfb(transparent)|mfb(container)|mfb(flammable)},
{"wall",             '|', c_ltgray,  0,
	mfb(flammable)},
{"wall",             '-', c_ltgray,  0,
	mfb(flammable)},
{"metal wall",       '|', c_cyan,    0,
	0},
{"metal wall",       '-', c_cyan,    0,
	0},
{"glass wall",       '|', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)},
{"glass wall",       '-', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)},
{"reinforced glass", '|', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)},
{"reinforced glass", '-', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)},
{"metal bars",       '"', c_ltgray,  0,
	mfb(transparent)},
{"closed wood door", '+', c_brown,   0,
	mfb(bashable)|mfb(flammable)|mfb(door)},
{"damaged wood door",'&', c_brown,   0,
	mfb(transparent)|mfb(bashable)|mfb(flammable)},
{"open wood door",  '\'', c_brown,   2,
	mfb(transparent)},
{"closed wood door", '+', c_brown,   0,	// Actually locked
	mfb(bashable)|mfb(flammable)},
{"empty door frame", '.', c_brown,   2,
	mfb(transparent)},
{"boarded up door",  '#', c_brown,   0,
	mfb(bashable)|mfb(flammable)},
{"closed metal door",'+', c_cyan,    0,
	0},
{"open metal door", '\'', c_cyan,    2,
	mfb(transparent)},
{"closed metal door",'+', c_cyan,    0, // Actually locked
	0},
{"bulletin board", '6', c_blue, 0,
	0},
{"makeshift portcullis", '&', c_cyan, 0,
	0},
{"window",	     '"', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)|mfb(flammable)},
{"window frame",     '0', c_ltcyan, 12,
	mfb(container)|mfb(transparent)|mfb(sharp)|mfb(flammable)|mfb(noitem)},
{"boarded up window",'#', c_brown,   0,
	mfb(bashable)|mfb(flammable)},
{"solid rock",       '#', c_white,   0,
	0},
{"tree",	     '7', c_green,   0,
	mfb(flammable)},
{"young tree",       '1', c_green,   0,
	mfb(transparent)|mfb(bashable)|mfb(flammable)},
{"underbrush",       '#', c_green,   7,
	mfb(transparent)|mfb(bashable)|mfb(diggable)|mfb(container)|mfb(rough)|
	mfb(flammable)},
{"wax wall",         '#', c_yellow,  0,
	mfb(container)|mfb(flammable)},
{"wax floor",        '.', c_yellow,  2,
	mfb(transparent)},
{"picket fence",     '|', c_brown,   3,
	mfb(transparent)|mfb(diggable)|mfb(flammable)|mfb(noitem)},
{"picket fence",     '-', c_brown,   3,
	mfb(transparent)|mfb(diggable)|mfb(flammable)|mfb(noitem)},
{"railing",          '|', c_yellow,  3,
	mfb(transparent)|mfb(noitem)},
{"railing",          '-', c_yellow,  3,
	mfb(transparent)|mfb(noitem)},
{"shallow water",    '~', c_ltblue,  5,
	mfb(transparent)|mfb(swimmable)},
{"deep water",       '~', c_blue,    0,
	mfb(transparent)|mfb(swimmable)},
{"sewage",           '~', c_ltgreen, 6,
	mfb(transparent)|mfb(swimmable)},
{"lava",             '~', c_red,     0,
	mfb(transparent)},
{"bed",              '#', c_magenta, 5,
	mfb(transparent)|mfb(container)|mfb(flammable)},
{"toilet",           '&', c_white,   0,
	mfb(transparent)|mfb(bashable)},
{"gasoline pump",    '&', c_red,     0,
	mfb(transparent)|mfb(explodes)},
{"smashed gas pump", '&', c_ltred,   0,
	mfb(transparent)},
{"missile",          '#', c_ltblue,  0,
	mfb(explodes)},
{"blown-out missile",'#', c_ltgray,  0,
	0},
{"counter",	     '#', c_blue,    4,
	mfb(transparent)},
{"radio tower",      '&', c_ltgray,  0,
	0},
{"radio controls",   '6', c_green,   0,
	mfb(transparent)|mfb(bashable)},
{"broken computer",  '6', c_ltgray,  0,
	mfb(transparent)},
{"computer console", '6', c_blue,    0,
	mfb(transparent)|mfb(computer)},
{"computer console", '6', c_blue,    0,
	mfb(transparent)|mfb(computer)},
{"computer console", '6', c_blue,    0,
	mfb(transparent)|mfb(computer)},
{"refrigerator",    '{', c_ltcyan,   0,
	mfb(container)},
{"dresser",          '{', c_brown,   0,
	mfb(transparent)|mfb(container)|mfb(flammable)},
{"display rack",     '{', c_ltgray,  0,
        mfb(transparent)|mfb(container)},
{"book case",        '{', c_brown,   0,
	mfb(container)|mfb(flammable)},
{"dumpster",	     '{', c_green,   0,
	mfb(container)},
{"cloning vat",      '0', c_ltcyan,  0,
	mfb(transparent)|mfb(bashable)|mfb(container)|mfb(sealed)},
{"stairs down",      '>', c_yellow,  2,
	mfb(transparent)|mfb(goes_down)|mfb(container)},
{"stairs up",        '<', c_yellow,  2,
	mfb(transparent)|mfb(goes_up)|mfb(container)},
{"manhole",          '>', c_dkgray,  2,
	mfb(transparent)|mfb(goes_down)|mfb(container)},
{"ladder",           '<', c_dkgray,  2,
	mfb(transparent)|mfb(goes_up)|mfb(container)},
{"downward slope",   '>', c_brown,   2,
	mfb(transparent)|mfb(goes_down)|mfb(container)},
{"upward slope",     '<', c_brown,   2,
	mfb(transparent)|mfb(goes_up)|mfb(container)},
{"card reader",	     '6', c_pink,    0,
	0},
{"broken card reader",'6', c_ltgray, 0,
	0},
{"manhole cover",    '0',  c_dkgray, 2,
	mfb(transparent)},
{"slot machine",     '6', c_green,   0,
	mfb(bashable)}
};

struct field_t {
 std::string name[3];
 char sym;
 nc_color color[3];
 bool transparent[3];
 bool dangerous[3];
 int halflife;	// In turns
};

enum field_id {
 fd_null = 0,
 fd_blood,
 fd_bile,
 fd_slime,
 fd_acid,
 fd_fire,
 fd_smoke,
 fd_tear_gas,
 fd_nuke_gas,
 fd_electricity,
 num_fields
};

const field_t fieldlist[] = {
{{"",	"",	"",},					'%',
 {c_white, c_white, c_white},	{true, true, true}, {false, false, false},  0},
{{"blood splatter", "blood stain", "puddle of blood"},	'%',
 {c_red, c_red, c_red},		{true, true, true}, {false, false, false},2500},
{{"bile splatter", "bile stain", "puddle of bile"},	'%',
 {c_pink, c_pink, c_pink},	{true, true, true}, {false, false, false},2500},
{{"slime trail", "slime stain", "puddle of slime"},	'%',
 {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false},2500},
{{"acid splatter", "acid streak", "pool of acid"},	'5',
 {c_ltgreen, c_green, c_green},	{true, true, true}, {true, true, true},	   10},
{{"small fire",	"fire",	"raging fire"},			'4',
 {c_yellow, c_ltred, c_red},	{true, true, true}, {true, true, true},	 2000},
{{"thin smoke",	"smoke", "thick smoke"},		'8',
 {c_white, c_ltgray, c_dkgray},	{true, false, false},{false, true, true}, 400},
{{"hazy cloud","tear gas","thick tear gas"},		'8',
 {c_white, c_yellow, c_brown},	{true, false, false},{true, true, true},  600},
{{"hazy cloud","radioactive gas", "thick radioactive gas"}, '8',
 {c_white, c_ltgreen, c_green},	{true, true, false}, {true, true, true}, 1000},
{{"sparks", "electric crackle", "electric cloud"},	'9',
 {c_white, c_cyan, c_blue},	{true, true, true}, {true, true, true},	    2}
};

struct field {
 field_id type;
 signed char density;
 int age;
 field() { type = fd_null; density = 1; age = 0; };
 field(field_id t, unsigned char d, unsigned int a) {
  type = t;
  density = d;
  age = a;
 }
 bool is_null() {
  if (type == fd_null || type == fd_blood || type == fd_bile ||
      type == fd_slime)
   return true;
  return false;
 }
 bool is_dangerous() {
  return fieldlist[type].dangerous[density - 1];
 }
 std::string name() {
  return fieldlist[type].name[density - 1];
 }
};

struct spawn_point {
 int posx, posy;
 int count;
 mon_id type;
 spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1) :
             posx (X), posy (Y), count (C), type (T) {}
};

struct submap {
 ter_id			ter[SEEX][SEEY]; // Terrain on each square
 std::vector<item>	itm[SEEX][SEEY]; // Items on each square
 trap_id		trp[SEEX][SEEY]; // Trap on each square
 field			fld[SEEX][SEEY]; // Field on each square
 int			rad[SEEX][SEEY]; // Irradiation of each square
 std::vector<spawn_point> spawns;
};

#endif
