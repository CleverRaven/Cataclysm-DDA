#include "activity_scheduling_helper.h"

#include "player_helpers.h"
#include "map_helpers.h"

weariness_events do_activity( tasklist tasks )
{
    // Clear stuff, ensure we're in the right place
    clear_avatar();
    clear_map();


    avatar &guy = get_avatar();
    // How long we've been doing activities for
    time_duration spent = 0_seconds;
    // How weary we are starting
    int weariness_lvl = guy.weariness_level();
    weariness_events activity_log;
    while( tasks.duration() > spent ) {
        // What we're working on now
        const schedule &task = tasks.next_task();

        // Start our task, for however long the schedule says.
        // This may be longer than the interval, which means that we
        // never finish this task
        if( task.actor ) {
            guy.assign_activity( *task.actor, false );
        } else {
            guy.assign_activity( player_activity( task.act, calendar::INDEFINITELY_LONG, -1, INT_MIN,
                                                  "" ), false );
        }

        // How many turn's we've been at it
        time_duration turns = 0_seconds;
        while( turns <= task.interval ) {
            // Start each turn with a fresh set of moves
            guy.moves = 100;
            // Advance a turn
            calendar::turn += 1_turns;
            turns += 1_seconds;
            // Do our activity
            guy.activity.do_turn( guy );
            // Ensure we never actually finish an activity
            if( guy.activity.moves_left < 1000 ) {
                guy.activity.moves_left = 4000;
            }
            // Consume food, become weary, etc
            guy.update_body();
        }
        // Cancel our activity, now that we're done
        guy.cancel_activity();
        // How weary we are after ending this
        int new_weariness = guy.weariness_level();
        spent += task.interval;
        tasks.advance( task.interval );
        // If we're more weary than we were when we started, report it
        if( new_weariness != weariness_lvl ) {
            activity_log.log( weariness_lvl, new_weariness, spent );
            weariness_lvl = new_weariness;
        }
    }
    return activity_log;
}
