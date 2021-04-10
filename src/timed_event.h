#pragma once
#ifndef CATA_SRC_TIMED_EVENT_H
#define CATA_SRC_TIMED_EVENT_H

#include <list>

#include "calendar.h"
#include "point.h"

enum class timed_event_type : int {
    NONE,
    HELP,
    WANTED,
    ROBOT_ATTACK,
    SPAWN_WYRMS,
    AMIGARA,
    ROOTS_DIE,
    TEMPLE_OPEN,
    TEMPLE_FLOOD,
    TEMPLE_SPAWN,
    DIM,
    ARTIFACT_LIGHT,
    DSA_ALRP_SUMMON,
    NUM_TIMED_EVENT_TYPES
};

struct timed_event {
    timed_event_type type = timed_event_type::NONE;
    /** On which turn event should be happening. */
    time_point when = calendar::turn_zero;
    /** Which faction is responsible for handling this event. */
    int faction_id = -1;
    /** Where the event happens, in global submap coordinates */
    tripoint map_point = tripoint_min;

    timed_event( timed_event_type e_t, const time_point &w, int f_id, tripoint p );

    // When the time runs out
    void actualize();
    // Every turn
    void per_turn();
};

class timed_event_manager
{
    private:
        std::list<timed_event> events;

    public:
        /**
         * Add an entry to the event queue. Parameters are basically passed
         * through to @ref timed_event::timed_event.
         */
        void add( timed_event_type type, const time_point &when, int faction_id = -1 );
        /**
         * Add an entry to the event queue. Parameters are basically passed
         * through to @ref timed_event::timed_event.
         */
        void add( timed_event_type type, const time_point &when, int faction_id, const tripoint &where );
        /// @returns Whether at least one element of the given type is queued.
        bool queued( timed_event_type type ) const;
        /// @returns One of the queued events of the given type, or `nullptr`
        /// if no event of that type is queued.
        timed_event *get( timed_event_type type );
        /// Process all queued events, potentially altering the game state and
        /// modifying the event queue.
        void process();
};

timed_event_manager &get_timed_events();

#endif // CATA_SRC_TIMED_EVENT_H
