#include "activity_scheduling_helper.h"

#include "player_helpers.h"
#include "map_helpers.h"

void activity_schedule::setup( avatar &guy ) const
{
    // Start our task, for however long the schedule says.
    // This may be longer than the interval, which means that we
    // never finish this task
    if( actor ) {
        guy.assign_activity( *actor, false );
    } else {
        guy.assign_activity( player_activity( act, calendar::INDEFINITELY_LONG, -1, INT_MIN,
                                              "" ), false );
    }
}

void activity_schedule::do_turn( avatar &guy ) const
{
    // Do our activity
    guy.activity.do_turn( guy );
    // Ensure we never actually finish an activity
    if( guy.activity.moves_left < 1000 ) {
        guy.activity.moves_left = 4000;
    }
}

void meal_schedule::setup( avatar &guy ) const
{
    item eaten( food );
    guy.consume( eaten );
}

void meal_schedule::do_turn( avatar & ) const
{
}

bool meal_schedule::instantaneous() const
{
    return true;
}

void clear_guts::setup( avatar &guy ) const
{
    guy.guts.empty();
    guy.stomach.empty();
}

void clear_guts::do_turn( avatar & ) const
{
}

bool clear_guts::instantaneous() const
{
    return true;
}

void sleep_schedule::setup( avatar &guy ) const
{
    guy.fall_asleep();
}

void sleep_schedule::do_turn( avatar &guy ) const
{
    if( !guy.has_effect( efftype_id( "sleep" ) ) ) {
        debugmsg( "Woke up!" );
    }
}

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
        task.setup( guy );

        // How many turn's we've been at it
        time_duration turns = 0_seconds;
        while( turns <= task.interval && !task.instantaneous() ) {
            // Start each turn with a fresh set of moves
            guy.moves = 100;
            task.do_turn( guy );
            // Advance a turn
            calendar::turn += 1_turns;
            turns += 1_seconds;
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
