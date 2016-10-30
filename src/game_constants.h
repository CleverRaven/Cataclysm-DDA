#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

#define BLINK_SPEED 300
#define EXPLOSION_MULTIPLIER 7

// Really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_ITEM_IN_SQUARE 4096
// no reason to differ
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE
// only can wear a maximum of two of any type of clothing
#define MAX_WORN_PER_TYPE 2

#define MAPSIZE 11

// SEEX/SEEY define the size of a nonant, or grid.
// All map segments will need to be at least this wide.
#define SEEX 12
#define SEEY SEEX

#define MAX_VIEW_DISTANCE ( SEEX * int( MAPSIZE / 2 ) )

// Size of the overmap. This is the number of overmap terrain tiles per dimension in one overmap,
// it's just like SEEX/SEEY for submaps.
#define OMAPX 180
#define OMAPY 180

// Items on the map with at most this distance to the player are considered available for crafting,
// see inventory::form_from_map
#define PICKUP_RANGE 6

/** Number of z-levels below 0 (not including 0). */
#define OVERMAP_DEPTH 10
/** Number of z-levels above 0 (not including 0). */
#define OVERMAP_HEIGHT 10
/** Total number of z-levels */
#define OVERMAP_LAYERS (1 + OVERMAP_DEPTH + OVERMAP_HEIGHT)

/** Base move cost (before modifiers) per unit volume when handling items */
#define VOLUME_MOVE_COST 20
/** Maximum move cost when handling an item */
#define MAX_HANDLING_COST 800
/** Factor for item handling costs when item found in inventory */
#define INVENTORY_HANDLING_FACTOR 3
/** Factor for item handling costs when item found on map tile */
#define MAP_HANDLING_FACTOR 5
/** Factor for item handling costs when item found in vehicle */
#define VEHICLE_HANDLING_FACTOR 4

/** Amount by which to charge an item for each unit of plutonium cell */
#define PLUTONIUM_CHARGES 500

///\EFFECT_STR allows lifting of heavier objects */
#define STR_LIFT_FACTOR 50000 // 50kg/STR @todo revert to 10kg/STR

/** Weight per level of LIFT/JACK tool quality */
#define TOOL_LIFT_FACTOR 500000 // 500kg/level

/** Cap JACK requirements to support arbritrarily large vehicles */
#define JACK_LIMIT 6000000 // 6000kg (6 metric tonnes)

/** Maximum density of a map field */
#define MAX_FIELD_DENSITY 3

/** Slowest speed at which a gun can be aimed */
#define MAX_AIM_COST 10

/** Maximum (effective) level for a skill */
#define MAX_SKILL 10

/** Maximum (effective) level for a stat */
#define MAX_STAT 20

/** Maximum range for aimed weapons */
#define MAX_RANGE 30

/** Accuracy levels which a shots tangent must be below */
constexpr double accuracy_headshot = 0.1;
constexpr double accuracy_critical = 0.2;
constexpr double accuracy_goodhit  = 0.5;
constexpr double accuracy_standard = 0.8;
constexpr double accuracy_grazing  = 1.0;

/** Minimum item damage output of relevant type to allow using with relevant weapon skill */
#define MELEE_STAT 5

/** Effective lower bound to combat skill levels when CQB bionic is active */
#define BIO_CQB_LEVEL 5

#endif
