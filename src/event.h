#pragma once
#ifndef EVENT_H
#define EVENT_H

#include <climits>
#include <list>

#include "calendar.h"
#include "enums.h"

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
    /** On which turn event should be happening. */
    time_point when = calendar::time_of_cataclysm;
    /** Which faction is responsible for handling this event. */
    int faction_id = -1;
    /** Where the event happens, in global submap coordinates */
    tripoint map_point = tripoint_min;

    event( event_type e_t, const time_point &w, int f_id, tripoint p );

    void actualize(); // When the time runs out
    void per_turn();  // Every turn
};

class event_manager
{
    private:
        std::list<event> events;

    public:
        /**
         * Add an entry to the event queue. Parameters are basically passed
         * through to @ref event::event.
         */
        void add( event_type type, const time_point &when, int faction_id = -1 );
        /**
         * Add an entry to the event queue. Parameters are basically passed
         * through to @ref event::event.
         */
        void add( event_type type, const time_point &when, int faction_id, const tripoint &where );
        /// @returns Whether at least one element of the given type is queued.
        bool queued( event_type type ) const;
        /// @returns One of the queued events of the given type, or `nullptr`
        /// if no event of that type is queued.
        event *get( event_type type );
        /// Process all queued events, potentially altering the game state and
        /// modifying the event queue.
        void process();
};

#endif
