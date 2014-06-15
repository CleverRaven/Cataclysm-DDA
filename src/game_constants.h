#ifndef _GAME_CONSTANTS_H_
#define _GAME_CONSTANTS_H_

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
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

#define MAX_ITEM_IN_SQUARE 4096 // really just a sanity check for functions not tested beyond this. in theory 4096 works (`InvletInvlet)
#define MAX_VOLUME_IN_SQUARE 4000 // 6.25 dead bears is enough for everybody!
#define MAX_ITEM_IN_VEHICLE_STORAGE MAX_ITEM_IN_SQUARE // no reason to differ

#endif // _GAME_CONSTANTS_H_
