#include "activity_scheduling_helper.h"

#include <climits>
#include <cstdlib>
#include <list>

#include "avatar.h"
#include "cata_catch.h"
#include "debug.h"
#include "item.h"
#include "make_static.h"
#include "map_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "stomach.h"
#include "string_formatter.h"

void activity_schedule::setup( avatar &guy ) const
{
    // Start our task, for however long the schedule says.
    // This may be longer than the interval, which means that we
    // never finish this task
    guy.assign_activity( player_activity() ); // disallow resuming
    const player_activity p_act = actor
                                  ? player_activity( *actor )
                                  : player_activity( act, calendar::INDEFINITELY_LONG, -1, INT_MIN, "" );
    guy.assign_activity( p_act );
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
    if( !guy.has_effect( STATIC( efftype_id( "sleep" ) ) ) ) {
        debugmsg( "Woke up!" );
    }
}

weariness_events do_activity( tasklist tasks, bool do_clear_avatar )
{
    // Clear stuff, ensure we're in the right place
    if( do_clear_avatar ) {
        clear_avatar();
    }
    clear_map();

    avatar &guy = get_avatar();
    // Ensure we have enough light to see
    item bag( "duffelbag" );
    item light( "atomic_lamp" );
    guy.worn.wear_item( guy, bag, false, false );
    guy.i_add( light );
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
            // Advance a turn
            calendar::turn += 1_turns;
            turns += 1_seconds;
            // Consume food, become weary, etc
            guy.update_body();
            // Start each turn with a fresh set of moves
            guy.set_moves( 100 );
            task.do_turn( guy );
        }
        // Cancel our activity, now that we're done
        guy.cancel_activity();
        // How weary we are after ending this
        int new_weariness = guy.weariness_level();
        spent += task.interval;
        tasks.advance( task.interval );
        // If we're more weary than we were when we started, report it
        if( new_weariness != weariness_lvl ) {
            int new_weary = guy.weariness();
            int new_thresh = guy.weary_threshold();
            int intake = guy.activity_history.intake;
            int tracker = guy.activity_history.tracker;
            activity_log.log( weariness_lvl, new_weariness, spent,
                              new_weary, new_thresh, intake, tracker );
            weariness_lvl = new_weariness;
        }
    }
    return activity_log;
}

const schedule &tasklist::next_task()
{
    // Uh oh! We ran out of tasks!
    if( cursor >= tasks.size() ) {
        debugmsg( "Requested task when none existed!" );
        REQUIRE( !tasks.empty() );
        return *tasks[0].first;
    }
    return *tasks[cursor].first;
}

void tasklist::advance( const time_duration &how_long )
{
    advanced += how_long;
    if( advanced > tasks[cursor].second ) {
        advanced = 0_seconds;
        ++cursor;
    }
    // It's valid for us to finish our task and run out of them, putting
    // the cursor out of bounds. But it's definitely not valid to go further
    // (If we somehow made it here without complaints)
    if( cursor > tasks.size() ) {
        debugmsg( "Attempted to continue advancing once all tasks were finished!" );
        cursor = 0;
    }
}

void tasklist::enschedule( const schedule &added, const time_duration &how_long )
{
    tasks.insert( tasks.end(), { &added, how_long } );
}

void tasklist::clear()
{
    cursor = 0;
    advanced = 0_seconds;
    tasks.clear();
}

time_duration tasklist::duration()
{
    time_duration ret;
    for( const std::pair<const schedule *, time_duration> &cursor : tasks ) {
        ret += cursor.second;
    }
    return ret;
}

void weariness_events::log( const int old_level, const int new_level, const time_duration &when,
                            const int new_weariness, const int new_threshold,
                            const int intake, const int tracker )
{
    weary_transition added;
    added.from = old_level;
    added.to = new_level;
    added.minutes = to_minutes<int>( when );
    added.new_weariness = new_weariness;
    added.new_threshold = new_threshold;
    added.intake = intake;
    added.tracker = tracker;

    transitions.insert( transitions.end(), added );
}

int weariness_events::transition_minutes( const int from, const int to,
        const time_duration &around ) const
{
    int around_mins = to_minutes<int>( around );
    // first = change.minutes, second = difference from around_mins
    std::pair<int, int> ret = {INT_MAX, INT_MAX};
    for( const weary_transition &change : transitions ) {
        int diff = std::abs( change.minutes - around_mins );
        if( change.from == from && change.to == to && ret.second > diff ) {
            ret = { change.minutes, diff };
        }
    }
    return ret.first;
}

bool weariness_events::have_weary_decrease() const
{
    for( const weary_transition &change : transitions ) {
        if( change.from > change.to ) {
            return true;
        }
    }

    return false;
}

std::string weariness_events::summarize() const
{
    std::string buffer;
    for( const weary_transition &change : transitions ) {
        buffer += string_format( "Change: Weary lvl from %d to %d at %d min (W %d Th %d In %d Tr %d)\n",
                                 change.from, change.to, change.minutes,
                                 change.new_weariness, change.new_threshold,
                                 static_cast<int>( change.intake / 1000 ),
                                 static_cast<int>( change.tracker / 1000 ) );
    }
    return buffer;
}

bool weariness_events::empty() const
{
    return transitions.empty();
}
