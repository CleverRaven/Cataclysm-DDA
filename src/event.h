#ifndef EVENT_H
#define EVENT_H

#include "faction.h"
#include "line.h"
#include <climits>

class game;

enum event_type : int {
    EVENT_NULL,
    EVENT_HELP,
    EVENT_WANTED,
    EVENT_ROBOT_ATTACK,
    EVENT_SPAWN_WYRMS,
    EVENT_AMIGARA,
    EVENT_ROOTS_DIE,
    EVENT_TEMPLE_OPEN,
    EVENT_TEMPLE_FLOOD,
    EVENT_TEMPLE_SPAWN,
    EVENT_DIM,
    EVENT_ARTIFACT_LIGHT,
    NUM_EVENT_TYPES
};

struct event {
    event_type type = EVENT_NULL;
    /** When the event has been created. */
    int turn = 0;
    /** Which faction is responsible for handling this event. */
    int faction_id = -1;
    /** Where the event happens, in global submap coordinates */
    tripoint map_point = tripoint( INT_MIN, INT_MIN, INT_MIN );

    event( event_type e_t, int t, int f_id, tripoint map_point );

    void actualize(); // When the time runs out
    void per_turn();  // Every turn
};

#endif
