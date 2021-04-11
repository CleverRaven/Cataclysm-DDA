#include "activity_tracker.h"

#include "game_constants.h"
#include "options.h"
#include "string_formatter.h"

#include <cmath>
#include <limits>

int activity_tracker::weariness() const
{
    if( intake > tracker ) {
        return tracker * 0.5;
    }
    return tracker - intake * 0.5;
}

// Called every 5 minutes, when activity level is logged
void activity_tracker::try_reduce_weariness( int bmr, bool sleeping )
{
    tick_counter++;
    if( average_activity() - NO_EXERCISE <= std::numeric_limits<float>::epsilon() ) {
        low_activity_ticks++;
        // Recover twice as fast at rest
        if( sleeping ) {
            low_activity_ticks++;
        }
    }

    const float recovery_mult = get_option<float>( "WEARY_RECOVERY_MULT" );

    if( low_activity_ticks >= 1 ) {
        int reduction = tracker;
        // 1/120 of whichever's bigger
        if( bmr > reduction ) {
            reduction = std::floor( bmr * recovery_mult * low_activity_ticks / 6.0f );
        } else {
            reduction = std::ceil( reduction * recovery_mult * low_activity_ticks / 6.0f );
        }
        low_activity_ticks = 0;

        tracker -= std::max( reduction, 1 );
    }

    // If happens to be no reduction, character is not (as) hypoglycemic
    if( tick_counter >= 3 ) {
        intake *= std::pow( 1 - recovery_mult, 0.25f );
        tick_counter -= 3;
    }

    // Normalize values, make sure we stay above 0
    intake = std::max( intake, 0 );
    tracker = std::max( tracker, 0 );
    tick_counter = std::max( tick_counter, 0 );
    low_activity_ticks = std::max( low_activity_ticks, 0 );
}

void activity_tracker::weary_clear()
{
    tracker = 0;
    intake = 0;
    low_activity_ticks = 0;
    tick_counter = 0;
}

std::string activity_tracker::debug_weary_info() const
{
    return string_format( "Intake: %d Tracker: %d", intake, tracker );
}

void activity_tracker::calorie_adjust( int nkcal )
{
    if( nkcal > 0 ) {
        intake += nkcal;
    } else {
        // nkcal is negative, we need positive
        tracker -= nkcal;
    }
}

float activity_tracker::activity() const
{
    if( current_turn == calendar::turn ) {
        return current_activity;
    }
    return 1.0f;
}

float activity_tracker::average_activity() const
{
    if( activity_reset && current_turn != calendar::turn ) {
        return previous_activity / num_events;
    }
    return ( accumulated_activity + current_activity ) / num_events;
}

float activity_tracker::instantaneous_activity_level() const
{
    if( current_turn == calendar::turn ) {
        return current_activity;
    }
    return previous_turn_activity;
}

// The idea here is the character is going about their business logging activities,
// and log_activity() handles sorting them out, it records the largest magnitude for a given turn,
// and then rolls the previous turn's value into the accumulator once a new activity is logged.
// After a reset, we have to pretend the previous values weren't logged.
void activity_tracker::log_activity( float new_level )
{
    current_activity = std::max( current_activity, new_level );
    current_turn = calendar::turn;
}

void activity_tracker::new_turn()
{
    if( activity_reset ) {
        activity_reset = false;
        previous_turn_activity = current_activity;
        current_activity = NO_EXERCISE;
        accumulated_activity = 0.0f;
        num_events = 1;
    } else {
        // This is for the last turn that had activity logged.
        accumulated_activity += current_activity;
        // Then handle the interventing turns that had no activity logged.
        int num_turns = to_turns<int>( calendar::turn - current_turn );
        if( num_turns > 1 ) {
            accumulated_activity += ( num_turns - 1 ) * NO_EXERCISE;
            num_events += num_turns - 1;
        }
        previous_turn_activity = current_activity;
        current_activity = NO_EXERCISE;
        num_events++;
    }
}

void activity_tracker::reset_activity_level()
{
    previous_activity = accumulated_activity;
    activity_reset = true;
}

std::string activity_tracker::activity_level_str() const
{
    for( const std::pair<const float, std::string> &member : activity_levels_str_map ) {
        if( current_activity <= member.first ) {
            return member.second;
        }
    }
    return ( --activity_levels_str_map.end() )->second;
}
