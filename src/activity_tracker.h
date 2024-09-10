#pragma once
#ifndef CATA_SRC_ACTIVITY_TRACKER_H
#define CATA_SRC_ACTIVITY_TRACKER_H

#include <string>

#include "calendar.h"

class JsonObject;
class JsonOut;

class activity_tracker
{
    private:
        float current_activity = 0.0;
        float accumulated_activity = 0.0;
        float previous_activity = 0.0;
        float previous_turn_activity = 0.0;
        time_point current_turn = calendar::turn_zero;
        bool activity_reset = true;
        int num_events = 1;

        // Weariness metadata.
        // Semi-consecutive 5 minute ticks of low activity (or 2.5 if we're sleeping). Float to handle mixed cases.
        float low_activity_ticks = 0.0f;
    public:
        int tracker = 0;
        int intake = 0;
        // Logs activity level. If called multiple times in one turn, will preserve the highest.
        void log_activity( float new_level );
        // Informs the tracker that a new turn has started.
        void new_turn( bool sleeping = false );
        // Resets accumulated activity level.
        void reset_activity_level();
        // Outputs player activity level to a printable string.
        std::string activity_level_str() const;
        // Returns activity level recorded for the current turn.
        float activity( bool sleeping = false ) const;
        // Returns average of activity level for the current period.
        float average_activity() const;
        // Returns the previous turn's activity level until an action is taken on the current turn.
        float instantaneous_activity_level() const;

        int weariness() const;
        void try_reduce_weariness( int bmr, float sleepiness_mod, float sleepiness_regen_mod );
        void calorie_adjust( int ncal );
        void weary_clear();
        void set_intake( int ncal );
        std::string debug_weary_info() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &jo );
};

#endif // CATA_SRC_ACTIVITY_TRACKER_H
