#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include <vector>
#include <string>
#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"
#include "computer.h"
#include "vehicle.h"
#include "graffiti.h"
#include <iosfwd>

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
#define mfb(n) long(1 << (n))
#endif

enum t_flag {
 transparent = 0,// Player & monsters can see through/past it
 bashable,     // Player & monsters can bash this & make it the next in the list
 container,    // Items on this square are hidden until looted by the player
 place_item,   // Valid terrain for place_item() to put items on
 door,         // Can be opened--used for NPC pathfinding.
 flammable,    // May be lit on fire
 l_flammable,  // Harder to light on fire, but still possible
 explodes,     // Explodes when on fire
 diggable,     // Digging monsters, seeding monsters, digging w/ shovel, etc.
 tentable,     // I'm lazy, this is just diggable with a few more tacked on.
 liquid,       // Blocks movement but isn't a wall, e.g. lava or water
 swimmable,    // You (and monsters) swim here
 sharp,	       // May do minor damage to players/monsters passing it
 painful,      // May cause a small amount of pain
 rough,        // May hurt the player's feet
 sealed,       // Can't 'e' to retrieve items here
 noitem,       // Items "fall off" this space
 goes_down,    // Can '>' to go down a level
 goes_up,      // Can '<' to go up a level
 console,      // Used as a computer
 alarmed,      // Sets off an alarm if smashed
 supports_roof,// Used as a boundary for roof construction
               // can also knock down adjacent supports_roof and collapses tiles when destroyed
 thin_obstacle,// passable by player and monsters, but not by vehicles
 collapses,    // Tiles that have a roof over them (which can collapse)
 flammable2,   // Burn to ash rather than rubble.
 deconstruct,  // Can be deconstructed
 num_t_flags   // MUST be last
};

struct ter_t {
 std::string name;
 char sym;
 nc_color color;
 unsigned char movecost;
 trap_id trap;
 unsigned long flags;// : num_t_flags;
};

enum ter_id {
t_null = 0,
t_hole,	// Real nothingness; makes you fall a z-level
// Ground
t_dirt, t_sand, t_dirtmound, t_pit_shallow, t_pit,
t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered,
t_rock_floor, t_rubble, t_ash, t_metal, t_wreckage,
t_grass,
t_metal_floor,
t_pavement, t_pavement_y, t_sidewalk,
t_floor,
t_dirtfloor,//Dirt floor(Has roof)
t_hay,
t_grate,
t_slime,
t_bridge,
// Tent Walls & doors
t_canvas_wall, t_canvas_door, t_canvas_door_o, t_groundsheet, t_fema_groundsheet,
// Lighting related
t_skylight, t_emergency_light_flicker, t_emergency_light,
// Walls
t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
t_wall_v, t_wall_h, t_concrete_v, t_concrete_h,
t_wall_metal_v, t_wall_metal_h,
t_wall_glass_v, t_wall_glass_h,
t_wall_glass_v_alarm, t_wall_glass_h_alarm,
t_reinforced_glass_v, t_reinforced_glass_h,
t_bars,
t_door_c, t_door_b, t_door_o, t_door_locked, t_door_locked_alarm, t_door_frame,
t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o, t_door_boarded,
t_door_metal_c, t_door_metal_o, t_door_metal_locked,
t_door_glass_c, t_door_glass_o,
t_bulletin,
t_portcullis,
t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
t_rock, t_fault,
t_paper,
// Tree
t_tree, t_tree_young, t_tree_apple, t_underbrush, t_shrub, t_shrub_blueberry, t_log,
t_root_wall,
t_wax, t_floor_wax,
t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
t_railing_v, t_railing_h,
// Nether
t_marloss, t_fungus, t_tree_fungal,
// Water, lava, etc.
t_water_sh, t_water_dp, t_sewage,
t_lava,
// Embellishments
t_bed, t_toilet, t_makeshift_bed,
// More embellishments than you can shake a stick at.
t_sink, t_oven, t_woodstove, t_bathtub, t_chair, t_armchair, t_sofa, t_cupboard, t_trashcan, t_desk,
t_sandbox, t_slide, t_monkey_bars, t_backboard,
t_bench, t_table, t_pool_table,
t_gas_pump, t_gas_pump_smashed, t_gas_pump_empty,
t_missile, t_missile_exploded,
t_counter,
t_radio_tower, t_radio_controls,
t_console_broken, t_console, t_gates_mech_control, t_barndoor,
t_sewage_pipe, t_sewage_pump,
t_centrifuge,
t_column,
// Containers
t_fridge, t_glass_fridge, t_dresser, t_locker,
t_rack, t_bookcase,
t_dumpster,
t_vat, t_crate_c, t_crate_o,
// Staircases etc.
t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
 t_slope_up, t_rope_up,
t_manhole_cover,
// Special
t_card_science, t_card_military, t_card_reader_broken, t_slot_machine,
 t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
 t_pedestal_temple,
// Temple tiles
t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
 t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
// found at fields
 t_mutpoppy, //mutated poppy flower

num_terrain_types
};

const ter_t terlist[num_terrain_types] = {  // MUST match enum ter_id above!
{"nothing",	     ' ', c_white,   2, tr_null,
	mfb(transparent)|mfb(diggable)},
{"empty space",      '#', c_black,   2, tr_ledge,
	mfb(transparent)},
{"dirt",	     '.', c_brown,   2, tr_null,
	mfb(transparent)|mfb(diggable)|mfb(tentable)},
{"sand",	     '.', c_yellow,   2, tr_null,
	mfb(transparent)|mfb(diggable)|mfb(tentable)},
{"mound of dirt",    '#', c_brown,   3, tr_null,
	mfb(transparent)|mfb(diggable)},
{"shallow pit",	     '0', c_yellow,  8, tr_null,
	mfb(transparent)|mfb(diggable)},
{"pit",              '0', c_brown,  10, tr_pit,
	mfb(transparent)|mfb(diggable)},
{"corpse filled pit",'#', c_green,  5,  tr_null,
        mfb(transparent)|mfb(diggable)},
{"covered pit",       '#', c_ltred,   2, tr_null,
	mfb(transparent)},
{"spiked pit",       '0', c_ltred,  10, tr_spike_pit,
	mfb(transparent)|mfb(diggable)},
{"covered spiked pit",'#',c_ltred,   2, tr_null,
	mfb(transparent)},
{"rock floor",       '.', c_ltgray,  2, tr_null,
	mfb(transparent)},
{"pile of rubble",   '^', c_ltgray,  4, tr_null,
	mfb(transparent)|mfb(rough)|mfb(diggable)},
{"pile of ash",   '#', c_ltgray,  2, tr_null,
	mfb(transparent)|mfb(diggable)},
{"twisted metal",    '#', c_cyan,    5, tr_null,
        mfb(transparent)|mfb(rough)|mfb(sharp)|mfb(place_item)},
{"metal wreckage",   '#', c_cyan,    5, tr_null,
        mfb(transparent)|mfb(rough)|mfb(sharp)|mfb(place_item)},
{"grass",	     '.', c_green,   2, tr_null,
	mfb(transparent)|mfb(diggable)|mfb(tentable)},
{"metal floor",      '.', c_ltcyan,  2, tr_null,
	mfb(transparent)},
{"pavement",	     '.', c_dkgray,  2, tr_null,
	mfb(transparent)},
{"yellow pavement",  '.', c_yellow,  2, tr_null,
	mfb(transparent)},
{"sidewalk",         '.', c_ltgray,  2, tr_null,
	mfb(transparent)},
{"floor",	     '.', c_cyan,    2, tr_null,
	mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)},
{"dirt floor",	     '.', c_brown,    2, tr_null,  //Dirt Floor, must have roofs!
	mfb(transparent)|mfb(diggable)|mfb(supports_roof)|mfb(collapses)},
{"hay",              '#', i_brown, 5, tr_null,
	mfb(transparent)|mfb(container)|mfb(flammable2)|mfb(collapses)},
{"metal grate",      '#', c_dkgray,  2, tr_null,
	mfb(transparent)},
{"slime",            '~', c_green,   6, tr_null,
        mfb(transparent)|mfb(container)|mfb(flammable2)|mfb(place_item)},
{"walkway",          '#', c_yellow,  2, tr_null,
	mfb(transparent)},
{"canvas wall",      '#', c_blue,   0, tr_null,
        mfb(l_flammable)|mfb(bashable)|mfb(noitem)|mfb(tentable)},
{"canvas flap",      '+', c_blue,   0, tr_null,
        mfb(l_flammable)|mfb(bashable)|mfb(noitem)|mfb(tentable)},
{"open canvas flap", '.', c_blue,   2, tr_null,
        mfb(transparent)},
{"groundsheet",      ';', c_green,   2, tr_null,
        mfb(transparent)|mfb(tentable)},
{"groundsheet",      ';', c_green,   2, tr_null,
        mfb(transparent)},
{"floor",	     '.', c_white,    2, tr_null,
	mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)}, // Skylight
{"floor",	     '.', c_white,    2, tr_null,
	mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)}, // Emergency Light
{"floor",	     '.', c_white,    2, tr_null,
	mfb(transparent)|mfb(l_flammable)|mfb(supports_roof)|mfb(collapses)}, // Regular Light
{"half-built wall", '#', c_brown,   4, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)},
{"log wall",        '#', c_brown,   0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"chipped log wall",'#', c_brown,   0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"broken log wall", '&', c_brown,   0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
	mfb(supports_roof)},
{"palisade wall",        '#', c_brown,   0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)|mfb(transparent)},
{"palisade gate",        '+', c_ltred,    0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)|mfb(door)|mfb(transparent)},
{"open palisade gate",   '#', c_ltred,   2, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)|mfb(transparent)},
{"half-built wall",  '#', c_ltred,   4, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)},
{"wooden wall",      '#', c_ltred,   0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"chipped wood wall",'#', c_ltred,   0, tr_null,
        mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"broken wood wall", '&', c_ltred,   0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
    mfb(supports_roof)},
{"wall",             '|', c_ltgray,  0, tr_null,
        mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"wall",             '-', c_ltgray,  0, tr_null,
        mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"concrete wall",    '|', c_dkgray,  0, tr_null,
        mfb(noitem)|mfb(supports_roof)},
{"concrete wall",    '-', c_dkgray,  0, tr_null,
        mfb(noitem)|mfb(supports_roof)},
{"metal wall",       '|', c_cyan,    0, tr_null,
        mfb(noitem)|mfb(noitem)|mfb(supports_roof)},
{"metal wall",       '-', c_cyan,    0, tr_null,
        mfb(noitem)|mfb(noitem)|mfb(supports_roof)},
{"glass wall",       '|', c_ltcyan,  0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof)},
{"glass wall",       '-', c_ltcyan,  0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof)},
{"glass wall",       '|', c_ltcyan,  0, tr_null, // Alarmed
	mfb(transparent)|mfb(bashable)|mfb(alarmed)|mfb(noitem)|
 mfb(supports_roof)},
{"glass wall",       '-', c_ltcyan,  0, tr_null, // Alarmed
	mfb(transparent)|mfb(bashable)|mfb(alarmed)|mfb(noitem)|
 mfb(supports_roof)},
{"reinforced glass", '|', c_ltcyan,  0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof)},
{"reinforced glass", '-', c_ltcyan,  0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(supports_roof)},
{"metal bars",       '"', c_ltgray,  0, tr_null,
	mfb(transparent)|mfb(noitem)},
{"closed wood door", '+', c_brown,   0, tr_null,
        mfb(bashable)|mfb(flammable2)|mfb(door)|mfb(noitem)|mfb(supports_roof)},
{"damaged wood door",'&', c_brown,   0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)|
        mfb(supports_roof)},
{"open wood door",  '\'', c_brown,   2, tr_null,
        mfb(flammable2)|mfb(transparent)|mfb(supports_roof)},
{"closed wood door", '+', c_brown,   0, tr_null,	// Actually locked
        mfb(bashable)|mfb(flammable2)|mfb(noitem)|mfb(supports_roof)},
{"closed wood door", '+', c_brown,   0, tr_null, // Locked and alarmed
	mfb(bashable)|mfb(flammable2)|mfb(alarmed)|mfb(noitem)|mfb(supports_roof)},
{"empty door frame", '.', c_brown,   2, tr_null,
        mfb(transparent)|mfb(supports_roof)},
{"locked wire gate", '+', c_cyan,   0, tr_null,
        mfb(transparent)|mfb(supports_roof)},
{"closed wooden gate", '+', c_brown,   3, tr_null,
        mfb(transparent)|mfb(supports_roof) |mfb(bashable)|mfb(flammable2)},
{"open wooden gate",   '.', c_brown,   2, tr_null,
        mfb(transparent)|mfb(supports_roof) |mfb(bashable)|mfb(flammable2)},
{"closed wire gate", '+', c_cyan,   0, tr_null,
        mfb(transparent)|mfb(supports_roof)},
{"open wire gate",   '.', c_cyan,   2, tr_null,
        mfb(transparent)|mfb(supports_roof)},
{"boarded up door",  '#', c_brown,   0, tr_null,
	mfb(bashable)|mfb(flammable2)|mfb(noitem)|mfb(supports_roof)},
{"closed metal door",'+', c_cyan,    0, tr_null,
	mfb(noitem)|mfb(supports_roof)},
{"open metal door", '\'', c_cyan,    2, tr_null,
	mfb(transparent)|mfb(supports_roof)},
{"closed metal door",'+', c_cyan,    0, tr_null, // Actually locked
	mfb(noitem)|mfb(supports_roof)},
{"closed glass door",'+', c_ltcyan,  0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(door)|mfb(noitem)|mfb(supports_roof)},
{"open glass door", '\'', c_ltcyan,  2, tr_null,
	mfb(transparent)|mfb(supports_roof)},
{"bulletin board",   '6', c_blue,    0, tr_null,
	mfb(flammable)|mfb(noitem)},
{"makeshift portcullis", '&', c_cyan, 0, tr_null,
	mfb(noitem)},
{"steel compactor",      '&', c_green, 0, tr_null,
        mfb(transparent)},
{"window",	     '"', c_ltcyan,  0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(noitem)|
        mfb(supports_roof)|mfb(deconstruct)}, // Plain Ol' window
{"taped window",  '"', c_dkgray,    0, tr_null,
	mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)}, // Regular window
{"window",	     '"', c_ltcyan,  0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(noitem)|
        mfb(supports_roof)|mfb(deconstruct)}, //has curtains
{"taped window",  '"', c_dkgray,    0, tr_null,
	mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)}, // Curtain window
{"open window",      '\'', c_ltcyan, 4, tr_null,
	mfb(transparent)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)},
{"closed curtains",  '"', c_dkgray,    0, tr_null,
	mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)},
{"window",	     '"', c_ltcyan,  0, tr_null, // Actually alarmed
	mfb(transparent)|mfb(bashable)|mfb(flammable)|mfb(alarmed)|mfb(noitem)|
        mfb(supports_roof)},
{"taped window",  '"', c_dkgray,    0, tr_null,
	mfb(bashable)|mfb(flammable)|mfb(noitem)| mfb(supports_roof)|mfb(alarmed)}, //Alarmed, duh.
{"empty window",     '0', c_yellow,  8, tr_null,
	mfb(transparent)|mfb(flammable)|mfb(supports_roof)},
{"window frame",     '0', c_ltcyan,  8, tr_null,
	mfb(transparent)|mfb(sharp)|mfb(flammable)|mfb(noitem)|
        mfb(supports_roof)},
{"boarded up window",'#', c_brown,   0, tr_null,
	mfb(bashable)|mfb(flammable)|mfb(noitem)|mfb(supports_roof)},
{"solid rock",       '#', c_white,   0, tr_null,
	mfb(noitem)|mfb(supports_roof)},
{"odd fault",        '#', c_magenta, 0, tr_null,
	mfb(noitem)|mfb(supports_roof)},
{"paper wall",       '#', c_white,   0, tr_null,
	mfb(bashable)|mfb(flammable2)|mfb(noitem)},
{"tree",	     '7', c_green,   0, tr_null,
	mfb(flammable2)|mfb(noitem)|mfb(supports_roof)},
{"young tree",       '1', c_green,   4, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(noitem)},
{"apple tree", '7', c_ltgreen,   0, tr_null,
	mfb(flammable2)|mfb(noitem)|mfb(supports_roof)},
{"underbrush",       '#', c_ltgreen, 6, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(diggable)|mfb(container)|
        mfb(flammable2)|mfb(thin_obstacle)|mfb(place_item)},
{"shrub",            '#', c_green,   8, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable2)|
        mfb(thin_obstacle)|mfb(place_item)},
{"blueberry bush",   '#', c_ltgreen,   8, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable2)|mfb(thin_obstacle)},
{"tree trunk",              '1', c_brown,   4, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(diggable)},
{"root wall",        '#', c_brown,   0, tr_null,
	mfb(noitem)|mfb(supports_roof)},
{"wax wall",         '#', c_yellow,  0, tr_null,
        mfb(flammable2)|mfb(noitem)|mfb(supports_roof)|mfb(place_item)},
{"wax floor",        '.', c_yellow,  2, tr_null,
	mfb(transparent)|mfb(l_flammable)},
{"picket fence",     '|', c_brown,   3, tr_null,
	mfb(transparent)|mfb(diggable)|mfb(flammable2)|mfb(noitem)|mfb(thin_obstacle)},
{"picket fence",     '-', c_brown,   3, tr_null,
	mfb(transparent)|mfb(diggable)|mfb(flammable2)|mfb(noitem)|mfb(thin_obstacle)},
{"chain link fence", '|', c_cyan,    0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(thin_obstacle)},
{"chain link fence", '-', c_cyan,    0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(noitem)|mfb(thin_obstacle)},
{"metal post",       '#', c_cyan,    2, tr_null,
        mfb(transparent)|mfb(thin_obstacle)},
{"fence post",       '#', c_brown,   2, tr_null,
        mfb(transparent)|mfb(thin_obstacle)},
{"wire fence",       '$', c_blue,    4, tr_null,
        mfb(transparent)|mfb(thin_obstacle)},
{"barbed wire fence",'$', c_blue,    4, tr_null,
        mfb(transparent)|mfb(sharp)|mfb(thin_obstacle)},
{"rope fence",       '$', c_brown,   3, tr_null,
        mfb(transparent)|mfb(thin_obstacle)},
{"railing",          '|', c_yellow,  3, tr_null,
	mfb(transparent)|mfb(noitem)|mfb(thin_obstacle)},
{"railing",          '-', c_yellow,  3, tr_null,
	mfb(transparent)|mfb(noitem)|mfb(thin_obstacle)},
{"marloss bush",     '1', c_dkgray,  0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(flammable2)},
{"fungal bed",       '#', c_dkgray,  3, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(diggable)},
{"fungal tree",      '7', c_dkgray,  0, tr_null,
	mfb(flammable2)|mfb(noitem)},
{"shallow water",    '~', c_ltblue,  5, tr_null,
	mfb(transparent)|mfb(liquid)|mfb(swimmable)},
{"deep water",       '~', c_blue,    0, tr_null,
	mfb(transparent)|mfb(liquid)|mfb(swimmable)},
{"sewage",           '~', c_ltgreen, 6, tr_null,
	mfb(transparent)|mfb(swimmable)},
{"lava",             '~', c_red,     4, tr_lava,
	mfb(transparent)|mfb(liquid)},
{"bed",              '#', c_magenta, 5, tr_null,
        mfb(transparent)|mfb(container)|mfb(flammable2)|mfb(collapses)|
        mfb(deconstruct)|mfb(place_item)},
{"toilet",           '&', c_white,   4, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(l_flammable)|mfb(collapses)},
{"makeshift bed",    '#', c_magenta, 5, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)},
{"sink",             '&', c_white,   4, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(l_flammable)|mfb(collapses)|mfb(container)|mfb(place_item)},
{"oven",             '#', c_dkgray,   4, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(l_flammable)|mfb(collapses)|mfb(container)|mfb(place_item)},

{"wood stove",             '#', i_red,   4, tr_null,
        mfb(transparent)|mfb(container)|mfb(place_item)},


{"bathtub",          '~', c_white,   4, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(l_flammable)|mfb(collapses)|mfb(container)|mfb(place_item)},

{"chair",            '#', c_brown,   2, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)},

{"arm chair",            'H', c_green,   3, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)},

{"sofa",            'H', i_red,   3, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)},
{"cupboard",         '#', c_blue,    3, tr_null,
        mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)|
        mfb(container)|mfb(place_item)},
{"trash can",        '&', c_ltcyan,  3, tr_null,
        mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(container)|mfb(place_item)},
{"desk",             '#', c_ltred,   3, tr_null,
        mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)|
        mfb(container)|mfb(place_item)},
{"sandbox", '#', c_yellow, 3, tr_null,
	mfb(transparent)|mfb(deconstruct)},
{"slide",            '#', c_ltcyan,  4, tr_null,
	mfb(transparent)|mfb(deconstruct)},
{"monkey bars",      '#', c_cyan,    4, tr_null,
	mfb(transparent)|mfb(deconstruct)},
{"backboard",        '7', c_red,     0, tr_null,
	mfb(transparent)|mfb(deconstruct)},
{"bench",            '#', c_brown,   3, tr_null,
	mfb(transparent)|mfb(flammable2)|mfb(collapses)|mfb(deconstruct)},
{"table",            '#', c_red,     4, tr_null,
	mfb(transparent)|mfb(flammable)|mfb(collapses)|mfb(deconstruct)},
{"pool table",       '#', c_green,   4, tr_null,
	mfb(transparent)|mfb(flammable)|mfb(collapses)|mfb(deconstruct)},
{"gasoline pump",    '&', c_red,     0, tr_null,
	mfb(transparent)|mfb(explodes)|mfb(noitem)},
{"smashed gas pump", '&', c_ltred,   0, tr_null,
	mfb(transparent)|mfb(noitem)},
{"out-of-order gasoline pump",    '&', c_red,     0, tr_null,
	mfb(transparent)|mfb(noitem)},
{"missile",          '#', c_ltblue,  0, tr_null,
	mfb(explodes)|mfb(noitem)},
{"blown-out missile",'#', c_ltgray,  0, tr_null,
	mfb(noitem)},
{"counter",	     '#', c_blue,    4, tr_null,
	mfb(transparent)|mfb(flammable)|mfb(collapses)|mfb(deconstruct)},
{"radio tower",      '&', c_ltgray,  0, tr_null,
	mfb(noitem)},
{"radio controls",   '6', c_green,   0, tr_null,
	mfb(transparent)|mfb(bashable)|mfb(noitem)},
{"broken console",   '6', c_ltgray,  0, tr_null,
	mfb(transparent)|mfb(noitem)|mfb(collapses)},
{"computer console", '6', c_blue,    0, tr_null,
	mfb(transparent)|mfb(console)|mfb(noitem)|mfb(collapses)},
{"mechanical winch", '6', c_cyan_red, 0, tr_null,
        mfb(transparent)|mfb(noitem)|mfb(collapses)},
{"rope and pulley", '|', i_brown, 0, tr_null,
        mfb(transparent)|mfb(noitem)|mfb(collapses)},
{"sewage pipe",      '1', c_ltgray,  0, tr_null,
	mfb(transparent)},
{"sewage pump",      '&', c_ltgray,  0, tr_null,
	mfb(noitem)},
{"centrifuge",       '{', c_magenta, 0, tr_null,
	mfb(transparent)},
{"column",           '1', c_ltgray,  0, tr_null,
	mfb(flammable)},
{"refrigerator",     '{', c_ltcyan,  0, tr_null,
	mfb(container)|mfb(collapses)|mfb(bashable)|mfb(place_item)},
{"glass door fridge",'{', c_ltcyan,  0, tr_null,
        mfb(collapses)|mfb(bashable)|mfb(place_item)},
{"dresser",          '{', c_brown,   0, tr_null,
        mfb(transparent)|mfb(container)|mfb(flammable)|mfb(collapses)|
        mfb(bashable)|mfb(deconstruct)|mfb(place_item)},
{"locker",           '{', c_ltgray,  0, tr_null,
        mfb(container)|mfb(collapses)|mfb(bashable)|mfb(place_item)},
{"display rack",     '{', c_ltgray,  0, tr_null,
        mfb(transparent)|mfb(l_flammable)|mfb(collapses)|mfb(bashable)|
        mfb(deconstruct)|mfb(place_item)},
{"book case",        '{', c_brown,   0, tr_null,
        mfb(flammable)|mfb(collapses)|mfb(bashable)|mfb(deconstruct)|mfb(place_item)},
{"dumpster",	     '{', c_green,   0, tr_null,
        mfb(container)|mfb(bashable)|mfb(place_item)},
{"cloning vat",      '0', c_ltcyan,  0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(sealed)|mfb(place_item)},
{"crate",            'X', i_brown,   0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(container)|mfb(sealed)|
        mfb(flammable)|mfb(deconstruct)|mfb(place_item)},
{"open crate",       'O', i_brown,   0, tr_null,
        mfb(transparent)|mfb(bashable)|mfb(container)|mfb(flammable)|mfb(place_item)},
{"stairs down",      '>', c_yellow,  2, tr_null,
        mfb(transparent)|mfb(goes_down)|mfb(place_item)},
{"stairs up",        '<', c_yellow,  2, tr_null,
        mfb(transparent)|mfb(goes_up)|mfb(place_item)},
{"manhole",          '>', c_dkgray,  2, tr_null,
        mfb(transparent)|mfb(goes_down)|mfb(place_item)},
{"ladder",           '<', c_dkgray,  2, tr_null,
        mfb(transparent)|mfb(goes_up)|mfb(place_item)},
{"ladder",           '>', c_dkgray,  2, tr_null,
        mfb(transparent)|mfb(goes_down)|mfb(place_item)},
{"downward slope",   '>', c_brown,   2, tr_null,
        mfb(transparent)|mfb(goes_down)|mfb(place_item)},
{"upward slope",     '<', c_brown,   2, tr_null,
        mfb(transparent)|mfb(goes_up)|mfb(place_item)},
{"rope leading up",  '<', c_white,   2, tr_null,
	mfb(transparent)|mfb(goes_up)},
{"manhole cover",    '0', c_dkgray,  2, tr_null,
	mfb(transparent)},
{"card reader",	     '6', c_pink,    0, tr_null,	// Science
	mfb(noitem)},
{"card reader",	     '6', c_pink,    0, tr_null,	// Military
	mfb(noitem)},
{"broken card reader",'6',c_ltgray,  0, tr_null,
	mfb(noitem)},
{"slot machine",     '6', c_green,   0, tr_null,
	mfb(bashable)|mfb(noitem)},
{"elevator controls",'6', c_ltblue,  0, tr_null,
	mfb(noitem)},
{"powerless controls",'6',c_ltgray,  0, tr_null,
	mfb(noitem)},
{"elevator",         '.', c_magenta, 2, tr_null,
	0},
{"dark pedestal",    '&', c_dkgray,  0, tr_null,
	mfb(transparent)},
{"light pedestal",   '&', c_white,   0, tr_null,
	mfb(transparent)},
{"red stone",        '#', c_red,     0, tr_null,
	0},
{"green stone",      '#', c_green,   0, tr_null,
	0},
{"blue stone",       '#', c_blue,    0, tr_null,
	0},
{"red floor",        '.', c_red,     2, tr_null,
	mfb(transparent)},
{"green floor",      '.', c_green,   2, tr_null,
	mfb(transparent)},
{"blue floor",       '.', c_blue,    2, tr_null,
	mfb(transparent)},
{"yellow switch",    '6', c_yellow,  0, tr_null,
	mfb(transparent)},
{"cyan switch",      '6', c_cyan,    0, tr_null,
	mfb(transparent)},
{"purple switch",    '6', c_magenta, 0, tr_null,
	mfb(transparent)},
{"checkered switch", '6', c_white,   0, tr_null,
	mfb(transparent)},
{"mutated poppy flower", 'f', c_red, 3, tr_null,
        mfb(transparent)}
};

enum map_extra {
 mx_null = 0,
 mx_helicopter,
 mx_military,
 mx_science,
 mx_stash,
 mx_drugdeal,
 mx_supplydrop,
 mx_portal,
 mx_minefield,
 mx_wolfpack,
 mx_cougar,
 mx_puddle,
 mx_crater,
 mx_fumarole,
 mx_portal_in,
 mx_anomaly,
 num_map_extras
};

// Chances are relative to eachother; e.g. a 200 chance is twice as likely
// as a 100 chance to appear.
const int map_extra_chance[num_map_extras + 1] = {
  0,	// Null - 0 chance
 40,	// Helicopter
 50,	// Military
120,	// Science
200,	// Stash
 20,	// Drug deal
 10, // Supply drop
  5,	// Portal
 70,	// Minefield
 30,	// Wolf pack
 40, // Cougar
250,	// Puddle
 10,	// Crater
  8,	// Fumarole
  7,	// One-way portal into this world
 10,	// Anomaly
  0	 // Just a cap value; leave this as the last one
};

struct map_extras {
 unsigned int chance;
 int chances[num_map_extras + 1];
 map_extras(unsigned int embellished, int helicopter = 0, int mili = 0,
            int sci = 0, int stash = 0, int drug = 0, int supply = 0,
            int portal = 0, int minefield = 0, int wolves = 0, int cougar = 0, int puddle = 0,
            int crater = 0, int lava = 0, int marloss = 0, int anomaly = 0)
            : chance(embellished)
 {
  chances[ 0] = 0;
  chances[ 1] = helicopter;
  chances[ 2] = mili;
  chances[ 3] = sci;
  chances[ 4] = stash;
  chances[ 5] = drug;
  chances[ 6] = supply;
  chances[ 7] = portal;
  chances[ 8] = minefield;
  chances[ 9] = wolves;
  chances[10] = cougar;
  chances[11] = puddle;
  chances[12] = crater;
  chances[13] = lava;
  chances[14] = marloss;
  chances[15] = anomaly;
  chances[16] = 0;
 }
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
 fd_web,
 fd_slime,
 fd_acid,
 fd_sap,
 fd_fire,
 fd_smoke,
 fd_toxic_gas,
 fd_tear_gas,
 fd_nuke_gas,
 fd_gas_vent,
 fd_fire_vent,
 fd_flame_burst,
 fd_electricity,
 fd_fatigue,
 fd_push_items,
 fd_shock_vent,
 fd_acid_vent,
 num_fields
};

const field_t fieldlist[] = {
{{"",	"",	""},					'%',
 {c_white, c_white, c_white},	{true, true, true}, {false, false, false},   0},

{{"blood splatter", "blood stain", "puddle of blood"},	'%',
 {c_red, c_red, c_red},		{true, true, true}, {false, false, false},2500},

{{"bile splatter", "bile stain", "puddle of bile"},	'%',
 {c_pink, c_pink, c_pink},	{true, true, true}, {false, false, false},2500},

{{"cobwebs","webs", "thick webs"},			'}',
 {c_white, c_white, c_white},	{true, true, false},{false, false, false},   0},

{{"slime trail", "slime stain", "puddle of slime"},	'%',
 {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false},2500},

{{"acid splatter", "acid streak", "pool of acid"},	'5',
 {c_ltgreen, c_green, c_green},	{true, true, true}, {true, true, true},	    10},

{{"sap splatter", "glob of sap", "pool of sap"},	'5',
 {c_yellow, c_brown, c_brown},	{true, true, true}, {true, true, true},     20},

{{"small fire",	"fire",	"raging fire"},			'4',
 {c_yellow, c_ltred, c_red},	{true, true, true}, {true, true, true},	   800},

{{"thin smoke",	"smoke", "thick smoke"},		'8',
 {c_white, c_ltgray, c_dkgray},	{true, false, false},{false, true, true},  300},

{{"hazy cloud","toxic gas","thick toxic gas"},		'8',
 {c_white, c_ltgreen, c_green}, {true, false, false},{false, true, true},  900},

{{"hazy cloud","tear gas","thick tear gas"},		'8',
 {c_white, c_yellow, c_brown},	{true, false, false},{true, true, true},   600},

{{"hazy cloud","radioactive gas", "thick radioactive gas"}, '8',
 {c_white, c_ltgreen, c_green},	{true, true, false}, {true, true, true},  1000},

{{"gas vent", "gas vent", "gas vent"}, '%',
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // fire vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"fire", "fire", "fire"}, '5',
 {c_red, c_red, c_red}, {true, true, true}, {true, true, true}, 0},

{{"sparks", "electric crackle", "electric cloud"},	'9',
 {c_white, c_cyan, c_blue},	{true, true, true}, {true, true, true},	     2},

{{"odd ripple", "swirling air", "tear in reality"},	'*',
 {c_ltgray, c_dkgray, c_magenta},{true, true, false},{false, false, false},  0},

{{"", "", ""}, '&', // push items
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // shock vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // acid vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0}
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

 bool is_null()
 {
  return (type == fd_null || type == fd_blood || type == fd_bile ||
          type == fd_slime);
 }

 bool is_dangerous()
 {
  return fieldlist[type].dangerous[density - 1];
 }

 std::string name()
 {
  return fieldlist[type].name[density - 1];
 }

};

struct spawn_point {
 int posx, posy;
 int count;
 mon_id type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1,
             int FAC = -1, int MIS = -1, bool F = false,
             std::string N = "NONE") :
             posx (X), posy (Y), count (C), type (T), faction_id (FAC),
             mission_id (MIS), friendly (F), name (N) {}
};

struct submap {
 ter_id			ter[SEEX][SEEY]; // Terrain on each square
 std::vector<item>	itm[SEEX][SEEY]; // Items on each square
 trap_id		trp[SEEX][SEEY]; // Trap on each square
 field			fld[SEEX][SEEY]; // Field on each square
 int			rad[SEEX][SEEY]; // Irradiation of each square
 graffiti graf[SEEX][SEEY]; // Graffiti on each square
 int active_item_count;
 int field_count;
 int turn_last_touched;
 std::vector<spawn_point> spawns;
 std::vector<vehicle*> vehicles;
 computer comp;
};

std::ostream & operator<<(std::ostream &, const submap *);
std::ostream & operator<<(std::ostream &, const submap &);

#endif
