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

#define MAX_ITEM_IN_SQUARE 4096 // really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_VOLUME_IN_SQUARE 4000 // 6.25 dead bears is enough for everybody!
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE // no reason to differ

#define MAPSIZE 11

//More importantly: SEEX defines the size of a nonant, or grid. Same with SEEY.
                // SEEX is how far the player can see in the X direction (at
#define SEEX 12 // least, without scrolling).  All map segments will need to be
                // at least this wide. The map therefore needs to be 3x as wide.

                // Same as SEEX
#define SEEY 12 // Requires 2*SEEY+1= 25 vertical squares
                // Nuts to 80x24 terms. Mostly exists in graphical clients, and
                // those fatcats can resize.

/** Number of z-levels below 0 (not including 0). */
#define OVERMAP_DEPTH 10
/** Number of z-levels above 0 (not including 0). */
#define OVERMAP_HEIGHT 10
/** Total number of z-levels */
#define OVERMAP_LAYERS (1 + OVERMAP_DEPTH + OVERMAP_HEIGHT)

#endif
