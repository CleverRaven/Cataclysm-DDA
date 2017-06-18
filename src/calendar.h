#pragma once
#ifndef CALENDAR_H
#define CALENDAR_H

#include <string>

/**
 * Convert turns to ticks
 *
 * "Moves" is time for an unmodified player to move one square.  This function converts that value
 * to 'ticks', which is the time taken up by one displayed movement point.
 *
 * @param n Time in six-second turns
 * @returns Time in ticks
 */
constexpr int MOVES( int n )
{
    return n * 100;
}

/**
 * Convert seconds to six-second turns
 *
 * @param n Time in seconds
 * @returns Time in six-second turns
 */
constexpr int SECONDS( int n )
{
    return n / 6;
}

/**
 * Convert minutes to six-second turns
 *
 * @param n Time in minutes
 * @returns Time in six-second turns
 *
 */
constexpr int MINUTES( int n )
{
    return n * 10;
}

/**
 * Convert hours to six-second turns
 *
 * @param n Time in hours
 * @returns Time in six-second turns
 */
constexpr int HOURS( int n )
{
    return n * MINUTES( 60 );
}

/**
 * Convert days to six-second turns
 *
 * @param n Time in days
 * @returns Time in six-second turns
 */
constexpr int DAYS( int n )
{
    return n * HOURS( 24 );
}

/** How much light moon provides per lit-up quarter (Full-moon light is four times this value) */
#define MOONLIGHT_PER_QUARTER 2.25

/** How much light is provided in full daylight */
#define DAYLIGHT_LEVEL 100

/** Approximate length of real-world seasons, for reference */
#define REAL_WORLD_SEASON_LENGTH 91

/** Real world seasons */
enum season_type {
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
};

/** Phases of the moon */
enum moon_phase {
    /** New (completely dark) moon */
    MOON_NEW = 0,
    /** One quarter of moon lit, amount lit is increasing every day */
    MOON_WAXING_CRESCENT,
    /** One half of moon lit, amount lit is increasing every day */
    MOON_HALF_MOON_WAXING,
    /** Three quarters of moon lit, amount lit is increasing every day */
    MOON_WAXING_GIBBOUS,
    /** Full moon */
    MOON_FULL,
    /** Three quarters of moon lit, amount lit decreases every day */
    MOON_WANING_GIBBOUS,
    /** Half of moon is lit, amount lit decreases every day */
    MOON_HALF_MOON_WANING,
    /** One quarter of moon lit, amount lit decreases every day */
    MOON_WANING_CRESCENT,
    /** Not a valid moon phase, but can be used for iterating through enum */
    MOON_PHASE_MAX
};

/**
 * Time keeping class
 *
 * Encapsulates the current time of day, date, and year.  Also tracks seasonal variation in day
 * length, the passing of the seasons themselves, and the phases of the moon.
 */
class calendar
{
    private:
        /**
         *  This is the basic "quantum" unit of world time.  It is a six second interval,
         *  so "seconds" value on the clock will always be a multiple of 6.
         */
        int turn_number;

        /** Seconds portion of time */
        int second;

        /** Minutes portion of time */
        int minute;

        /** Hours portion of time (using a 24-hour clock) */
        int hour;

        /** Day of season */
        int day;

        /** Current season */
        season_type season;

        /** Current year, starts counting from 0 in default scenarios */
        int year;

        /**
         * Synchronize all variables to the turn_number
         *
         * Time is actually counted as a running total of turns since start of game.  This function
         * calculates the current seconds/minutes/hour/day/season based on that counter value.
         */
        void sync();

    public:
        /** Initializers */
        /**
         * Default constructor
         *
         * Starts at turn count zero, (midnight of first day, year zero, spring time)
         */
        calendar();
        calendar( const calendar &copy ) = default;

        /** Construct calendar with specific starting time and day. */
        calendar( int Minute, int Hour, int Day, season_type Season, int Year );

        /**
         * Construct calendar with specific elapsed turns count.
         * @param turn Turn count value for constructed calendar
         */
        calendar( int turn );

        /** @returns the current turn_number. */
        int get_turn() const;

        /**
         * Alternative accessor for current turn_number.
         *
         * @deprecated Use get_turn() instead
         *
         * @returns same value as @ref get_turn()
         */
        operator int() const; // Returns get_turn() for backwards compatibility

        // Basic calendar operators. Usually modifies or checks the turn_number of the calendar
        calendar &operator = ( const calendar &rhs ) = default;
        calendar &operator = ( int rhs );
        calendar &operator -=( const calendar &rhs );
        calendar &operator -=( int rhs );
        calendar &operator +=( const calendar &rhs );
        calendar &operator +=( int rhs );
        calendar  operator - ( const calendar &rhs ) const;
        calendar  operator - ( int rhs ) const;
        calendar  operator + ( const calendar &rhs ) const;
        calendar  operator + ( int rhs ) const;
        bool      operator ==( int rhs ) const;
        bool      operator ==( const calendar &rhs ) const;

        /** Increases turn_number by 1. (6 seconds) */
        void increment();

        // Sunlight and day/night calculations
        /** Returns the number of minutes past midnight. Used for weather calculations. */
        int minutes_past_midnight() const;
        /** Returns the number of seconds past midnight. Used for sunrise/set calculations. */
        int seconds_past_midnight() const;
        /** Returns the current light level of the moon. */
        moon_phase moon() const;
        /** Returns the current sunrise time based on the time of year. */
        calendar sunrise() const;
        /** Returns the current sunset time based on the time of year. */
        calendar sunset() const;
        /** Returns true if it's currently after sunset + TWILIGHT_SECONDS or before sunrise - TWILIGHT_SECONDS. */
        bool is_night() const;
        /** Returns the current seasonally-adjusted maximum daylight level */
        double current_daylight_level() const;
        /** Returns the current sunlight or moonlight level through the preceding functions. */
        float sunlight() const;

        /** Seconds portion of current time of day */
        int seconds() const {
            return second;
        }
        /** Minutes portion of current time of day */
        int minutes() const {
            return minute;
        }
        /** Hour portion of current time of day */
        int hours() const {
            return hour;
        }
        /** Days since start of current season */
        int days() const {
            return day;
        }
        /** Current season */
        season_type get_season() const {
            return season;
        }
        /** Current year, with default game start as year 0 */
        int years() const {
            return year;
        }

        /**
         * Predicate to handle rate-limiting.
         *
         * @param event_frequency Number of turns between true returns
         * @returns true after every 'event_frequency' turns
         */
        static bool once_every( int event_frequency );

    public:
        /**
         * The expected duration of the cataclysm
         *
         * Large number that can be used to approximate infinite amounts of time.  Represents
         * approximately 60 billion years (in six-second increments).
         *
         * This number should be regarded as a number of turns, and can safely be converted to a
         * number of seconds or moves (movement points) without integer overflow.  If used to
         * represent a larger unit (hours/days/years), then this will result in integer overflow.
         */
        static const int INDEFINITELY_LONG;

        /** Number of six-second turns in a year, (configured in current world settings) */
        static int year_turns() {
            return DAYS( year_length() );
        }

        /** @returns Number of days in a year, (configured in current world settings) */
        static int year_length() {
            return season_length() * 4;
        }

        /** @returns Number of six-second turns in a season (configured in current world settings) */
        static int season_turns() {
            return DAYS( season_length() );
        }

        /** @returns Number of days in a season (configured in current world settings) */
        static int season_length();

        static float season_ratio() { //returns relative length of game season to irl season
            return static_cast<float>( season_length() ) / REAL_WORLD_SEASON_LENGTH;
        }

        /** @returns Number of turns elapsed in current year */
        int turn_of_year() const;

        /** @returns Number of days elapsed in current year */
        int day_of_year() const;

        /**
         * Calculate number of turns until a specified time.
         *
         * @param turn Specified diurnal time and date (in turns)
         * @returns the remaining time (in turns) before the specified diurnal time */
        int diurnal_time_before( int turn ) const;

        /**
         * Print approximate duration in largest practical units
         *
         * Generates a string showing a duration in appropriate units.  Durations of under one
         * minute are shown in seconds.  Durations of one minute to one hour are shown as a whole
         * number of minutes.  Durations of one hour to one day are shown as a whole number of hours.
         * Larger durations are shown as a number of whole days.
         *
         * @param turns Specified duration in six-second turns.
         * @returns String with value and units, suitable for display to player
         */
        static std::string print_clipped_duration( int turns );

        /** Returns normal duration. */
        static std::string print_duration( int turns );
        /**
         * Returns approximate duration.
         * @param turns Duration to print, measured in six-second turns
         * @param verbose If true, 'less than' and 'more than' will be printed instead of '<' and '>' respectively.
         */
        static std::string print_approx_duration( int turns, bool verbose = true );

        /** Returns the current time in a string according to the options set */
        std::string print_time( bool just_hour = false ) const;
        /** Returns the period a calendar has been running in word form; i.e. "1 second", "2 days". */
        std::string textify_period() const;
        /**
         * Returns the name of the current day of the week
         *
         * @note: Day 0 is a Thursday for highly technical reasons which are hard to explain
         */
        std::string day_of_week() const;

        /** Returns the translated name of the season. */
        static const std::string &name_season( season_type s );

        static   calendar start;
        static   calendar turn;
        static season_type initial_season;
};

#endif
