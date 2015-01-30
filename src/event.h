#ifndef EVENT_H
#define EVENT_H

#include "faction.h"
#include "line.h"

class game;

enum event_type {
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
    event_type type;
    int turn;
    int faction_id;
    /** Where the event happens, in global submap coordinates */
    tripoint map_point;

    event();
    event(event_type e_t, int t, int f_id, tripoint map_point);

    void actualize(); // When the time runs out
    void per_turn();  // Every turn
};

#endif
