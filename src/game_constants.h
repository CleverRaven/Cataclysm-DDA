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

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define EXPLOSION_MULTIPLIER 7

// Really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_ITEM_IN_SQUARE 4096
// 6.25 dead bears is enough for everybody!
#define MAX_VOLUME_IN_SQUARE 4000
// no reason to differ
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE
// only can wear a maximum of two of any type of clothing
#define MAX_WORN_PER_TYPE 2

#define MAPSIZE 11

// SEEX/SEEY define the size of a nonant, or grid.
// All map segments will need to be at least this wide.
#define SEEX 12
#define SEEY SEEX

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

#endif
