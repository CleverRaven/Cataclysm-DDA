#pragma once
#ifndef CALENDAR_H
#define CALENDAR_H

#include <string>

#include "optional.h"

class time_duration;
class time_point;
class JsonOut;
class JsonIn;

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

/**
 * Convert ticks to seconds.
 *
 * @param ticks number of ticks
 * @returns Time in seconds
 */
constexpr int TICKS_TO_SECONDS( int ticks )
{
    return static_cast<int>( static_cast<float>( ticks ) / 16.67 );
}

/** How much light moon provides per lit-up quarter (Full-moon light is four times this value) */
#define MOONLIGHT_PER_QUARTER 2.25

/** How much light is provided in full daylight */
#define DAYLIGHT_LEVEL 100

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

        /**
         * Accessor for current turn_number.
         *
         * @returns Current turn number.
         */
        operator int() const;

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

        /** Current year, with default game start as year 0 */
        int years() const {
            return year;
        }

        /**
         * Predicate to handle rate-limiting. Returns `true` after every @p event_frequency duration.
         */
        static bool once_every( const time_duration &event_frequency );

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
        /// @returns Whether the eternal season is enabled.
        static bool eternal_season();

        /** @returns Time in a year, (configured in current world settings) */
        static time_duration year_length();

        /** @returns Time of a season (configured in current world settings) */
        static time_duration season_length();

        /// @returns relative length of game season to real life season.
        static float season_ratio();

        /** @returns Number of days elapsed in current year */
        int day_of_year() const;
        /**
         * @returns ratio of actual season length (a world option) to default season length. This
         * should be used to convert JSON values (that assume the default for the season length
         * option) to actual in-game length.
         */
        static float season_from_default_ratio();

        /** Returns the translated name of the season (with first letter being uppercase). */
        static const std::string name_season( season_type s );

        static   calendar start;
        static   calendar turn;
        static season_type initial_season;

        /**
         * A time point that is always before the current turn, even when the game has
         * just started. This implies `before_time_starts < calendar::turn` is always
         * true. It can be used to initialize `time_point` values that denote that last
         * time a cache was update.
         */
        static const time_point before_time_starts;
        /**
         * Represents time point 0.
         */
        // TODO: flesh out the documentation
        static const time_point time_of_cataclysm;
};

template<typename T>
constexpr T to_turns( const time_duration &duration );
template<typename T>
constexpr T to_minutes( const time_duration &duration );
template<typename T>
constexpr T to_hours( const time_duration &duration );
template<typename T>
constexpr T to_days( const time_duration &duration );
template<typename T>
constexpr T to_weeks( const time_duration &duration );
template<typename T>
constexpr T to_moves( const time_duration &duration );

template<typename T>
constexpr T to_turn( const time_point &point );

template<typename T>
constexpr time_duration operator/( const time_duration &lhs, const T rhs );
template<typename T>
inline time_duration &operator/=( time_duration &lhs, const T rhs );
template<typename T>
constexpr time_duration operator*( const time_duration &lhs, const T rhs );
template<typename T>
constexpr time_duration operator*( const T lhs, const time_duration &rhs );
template<typename T>
inline time_duration &operator*=( time_duration &lhs, const T rhs );

/**
 * A duration defined as a number of specific time units.
 * Currently it stores the number (as integer) of turns.
 * Note: currently variable season length is ignored by this class (N turns are
 * always N turns) and there is no way to create an instance from units larger
 * than days (e.g. seasons or years) and no way to convert a duration into those
 * units directly. (You can still use @ref calendar to do this.)
 *
 * Operators for time points and time duration are defined as one may expect them:
 * -duration ==> duration (inverse)
 * point - point ==> duration (duration between to points in time)
 * point + duration ==> point (the revers of above)
 * point - duration ==> point (same as: point + -duration)
 * duration + duration ==> duration
 * duration - duration ==> duration (same as: duration + -duration)
 * duration * scalar ==> duration (simple scaling yields a double with precise value)
 * scalar * duration ==> duration (same as above)
 * duration / duration ==> scalar (revers of above)
 * duration / scalar ==> duration (same as: duration * 1/scalar)
 * duration % duration ==> duration ("remainder" of duration / some integer)
 * Also shortcuts: += and -= and *= and /=
 */
class time_duration
{
    private:
        friend class time_point;
        int turns_;

        explicit constexpr time_duration( const int t ) : turns_( t ) { }

    public:
        /// Allows writing `time_duration d = 0;`
        time_duration( const std::nullptr_t ) : turns_( 0 ) { }

        static time_duration read_from_json_string( JsonIn &jsin );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        /**
         * Named constructors to get a duration representing a multiple of the named time
         * units. Note that a duration is stored as integer number of turns, so
         * `from_minutes( 0.0001 )` will be stored as "0 turns".
         * The template type is used for the conversion from given time unit to turns, so
         * `from_hours( 0.5 )` will yield "300 turns".
         * Conversion of units greater than days (seasons) is not supported because they
         * depend on option settings ("season length").
         */
        /**@{*/
        template<typename T>
        static constexpr time_duration from_turns( const T t ) {
            return time_duration( t );
        }
        template<typename T>
        static constexpr time_duration from_minutes( const T m ) {
            return from_turns( m * 10 );
        }
        template<typename T>
        static constexpr time_duration from_hours( const T h ) {
            return from_minutes( h * 60 );
        }
        template<typename T>
        static constexpr time_duration from_days( const T d ) {
            return from_hours( d * 24 );
        }
        /**@}*/

        /**
         * Converts the duration to an amount of the given units. The conversions is
         * done with values of the given template type. That means using an integer
         * type (e.g. `int`) will return a truncated value (amount of *full* minutes
         * that make up the duration, discarding the remainder).
         * Calling `to_minutes<double>` will return a precise number.
         * Example:
         * `to_hours<int>( from_minutes( 90 ) ) == 1`
         * `to_hours<double>( from_minutes( 90 ) ) == 1.5`
         */
        /**@{*/
        template<typename T>
        friend constexpr T to_turns( const time_duration &duration ) {
            return duration.turns_;
        }
        template<typename T>
        friend constexpr T to_minutes( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 10 );
        }
        template<typename T>
        friend constexpr T to_hours( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 10 * 60 );
        }
        template<typename T>
        friend constexpr T to_days( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 10 * 60 * 24 );
        }
        template<typename T>
        friend constexpr T to_weeks( const time_duration &duration ) {
            return static_cast<T>( duration.turns_ ) / static_cast<T>( 10 * 60 * 24 * 7 );
        }
        template<typename T>
        friend constexpr T to_moves( const time_duration &duration ) {
            return to_turns<int>( duration ) * 100;
        }
        /**@{*/

        constexpr bool operator<( const time_duration &rhs ) const {
            return turns_ < rhs.turns_;
        }
        constexpr bool operator<=( const time_duration &rhs ) const {
            return turns_ <= rhs.turns_;
        }
        constexpr bool operator>( const time_duration &rhs ) const {
            return turns_ > rhs.turns_;
        }
        constexpr bool operator>=( const time_duration &rhs ) const {
            return turns_ >= rhs.turns_;
        }
        constexpr bool operator==( const time_duration &rhs ) const {
            return turns_ == rhs.turns_;
        }
        constexpr bool operator!=( const time_duration &rhs ) const {
            return turns_ != rhs.turns_;
        }

        friend constexpr time_duration operator-( const time_duration &duration ) {
            return time_duration( -duration.turns_ );
        }
        friend constexpr time_duration operator+( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ + rhs.turns_ );
        }
        friend time_duration &operator+=( time_duration &lhs, const time_duration &rhs ) {
            return lhs = time_duration( lhs.turns_ + rhs.turns_ );
        }
        friend constexpr time_duration operator-( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ - rhs.turns_ );
        }
        friend time_duration &operator-=( time_duration &lhs, const time_duration &rhs ) {
            return lhs = time_duration( lhs.turns_ - rhs.turns_ );
        }
        // Using double here because it has the highest precision. Callers can cast it to whatever they want.
        friend double operator/( const time_duration &lhs, const time_duration &rhs ) {
            return static_cast<double>( lhs.turns_ ) / static_cast<double>( rhs.turns_ );
        }
        template<typename T>
        friend constexpr time_duration operator/( const time_duration &lhs, const T rhs ) {
            return time_duration( lhs.turns_ / rhs );
        }
        template<typename T>
        friend time_duration &operator/=( time_duration &lhs, const T rhs ) {
            return lhs = time_duration( lhs.turns_ / rhs );
        }
        template<typename T>
        friend constexpr time_duration operator*( const time_duration &lhs, const T rhs ) {
            return time_duration( lhs.turns_ * rhs );
        }
        template<typename T>
        friend constexpr time_duration operator*( const T lhs, const time_duration &rhs ) {
            return time_duration( lhs * rhs.turns_ );
        }
        template<typename T>
        friend time_duration &operator*=( time_duration &lhs, const T rhs ) {
            return lhs = time_duration( lhs.turns_ * rhs );
        }
        friend time_duration operator%( const time_duration &lhs, const time_duration &rhs ) {
            return time_duration( lhs.turns_ % rhs.turns_ );
        }

        /// Returns a random duration in the range [low, hi].
        friend time_duration rng( time_duration lo, time_duration hi );
};

/// @see x_in_y(int,int)
bool x_in_y( const time_duration &a, const time_duration &b );

/**
 * Convert the given number into an duration by calling the matching
 * `time_duration::from_*` function.
 */
/**@{*/
constexpr time_duration operator"" _turns( const unsigned long long int v )
{
    return time_duration::from_turns( v );
}
constexpr time_duration operator"" _minutes( const unsigned long long int v )
{
    return time_duration::from_minutes( v );
}
constexpr time_duration operator"" _hours( const unsigned long long int v )
{
    return time_duration::from_hours( v );
}
constexpr time_duration operator"" _days( const unsigned long long int v )
{
    return time_duration::from_days( v );
}
/**@}*/

/**
 * Returns a string showing a duration. The string contains at most two numbers
 * along with their units. E.g. 3661 seconds will return "1 hour and 1 minute"
 * (the 1 additional second is clipped). An input of 3601 will return "1 hour"
 * (the second is clipped again and the number of additional minutes would be
 * 0 so it's skipped).
 */
std::string to_string( const time_duration &d );

enum class clipped_align {
    none,
    right,
};

enum class clipped_unit {
    forever,
    second,
    minute,
    hour,
    day,
    week,
    season,
    year,
};

/**
 * Returns a value representing the passed in duration truncated to an appropriate unit
 * along with the unit in question.
 * "10 days" or "1 minute".
 * The chosen unit will be the smallest unit, that is at least as much as the
 * given duration. E.g. an input of 60 minutes will return "1 hour", an input of
 * 59 minutes will return "59 minutes".
 */
std::pair<int, clipped_unit> clipped_time( const time_duration &d );

/**
 * Returns a string showing a duration as whole number of appropriate units, e.g.
 * "10 days" or "1 minute".
 * The chosen unit will be the smallest unit, that is at least as much as the
 * given duration. E.g. an input of 60 minutes will return "1 hour", an input of
 * 59 minutes will return "59 minutes".
 */
std::string to_string_clipped( const time_duration &d, clipped_align align = clipped_align::none );
/**
 * Returns approximate duration.
 * @param verbose If true, 'less than' and 'more than' will be printed instead of '<' and '>' respectively.
 */
std::string to_string_approx( const time_duration &dur, bool verbose = true );

/**
 * A point in the game time. Use `calendar::turn` to get the current point.
 * Modify it by adding/subtracting @ref time_duration.
 * This can be compared with the usual comparison operators.
 * It can be (de)serialized via JSON.
 *
 * Note that is does not handle variable sized season length. Changing the
 * season length has no effect on it.
 */
class time_point
{
    private:
        friend class time_duration;
        int turn_;

    public:
        // TODO: make private
        // TODO: make explicit
        constexpr time_point( const int t ) : turn_( t ) { }

    public:
        // TODO: replace usage of `calendar` with `time_point`, remove this constructor
        time_point( const calendar &c ) : turn_( c ) { }
        /// Allows writing `time_point p = 0;`
        constexpr time_point( const std::nullptr_t ) : turn_( 0 ) { }
        // TODO: remove this, nobody should need it, one should use a constant `time_point`
        // (representing turn 0) and a `time_duration` instead.
        static constexpr time_point from_turn( const int t ) {
            return time_point( t );
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        // TODO: try to get rid of this
        template<typename T>
        friend constexpr T to_turn( const time_point &point ) {
            return point.turn_;
        }

        // TODO: implement minutes_of_hour and so on and use it.
};

constexpr inline bool operator<( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) < to_turn<int>( rhs );
}
constexpr inline bool operator<=( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) <= to_turn<int>( rhs );
}
constexpr inline bool operator>( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) > to_turn<int>( rhs );
}
constexpr inline bool operator>=( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) >= to_turn<int>( rhs );
}
constexpr inline bool operator==( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) == to_turn<int>( rhs );
}
constexpr inline bool operator!=( const time_point &lhs, const time_point &rhs )
{
    return to_turn<int>( lhs ) != to_turn<int>( rhs );
}

constexpr inline time_duration operator-( const time_point &lhs, const time_point &rhs )
{
    return time_duration::from_turns( to_turn<int>( lhs ) - to_turn<int>( rhs ) );
}
constexpr inline time_point operator+( const time_point &lhs, const time_duration &rhs )
{
    return time_point::from_turn( to_turn<int>( lhs ) + to_turns<int>( rhs ) );
}
time_point inline &operator+=( time_point &lhs, const time_duration &rhs )
{
    return lhs = time_point::from_turn( to_turn<int>( lhs ) + to_turns<int>( rhs ) );
}
constexpr inline time_point operator-( const time_point &lhs, const time_duration &rhs )
{
    return time_point::from_turn( to_turn<int>( lhs ) - to_turns<int>( rhs ) );
}
time_point inline &operator-=( time_point &lhs, const time_duration &rhs )
{
    return lhs = time_point::from_turn( to_turn<int>( lhs ) - to_turns<int>( rhs ) );
}

inline time_duration time_past_midnight( const time_point &p )
{
    return ( p - calendar::time_of_cataclysm ) % 1_days;
}

inline time_duration time_past_new_year( const time_point &p )
{
    return ( p - calendar::time_of_cataclysm ) % calendar::year_length();
}

template<typename T>
inline T minute_of_hour( const time_point &p )
{
    return to_minutes<T>( ( p - calendar::time_of_cataclysm ) % 1_hours );
}

template<typename T>
inline T hour_of_day( const time_point &p )
{
    return to_hours<T>( ( p - calendar::time_of_cataclysm ) % 1_days );
}

/// This uses the current season length.
template<typename T>
inline T day_of_season( const time_point &p )
{
    return to_days<T>( ( p - calendar::time_of_cataclysm ) % calendar::season_length() );
}

/// @returns The season of the of the given time point. Returns the same season for
/// any input if the calendar::eternal_season yields true.
season_type season_of_year( const time_point &p );
/// @returns The time point formatted to be shown to the player. Contains year, season, day and time of day.
std::string to_string( const time_point &p );
/// @returns The time point formatted to be shown to the player. Contains only the time of day, not the year, day or season.
std::string to_string_time_of_day( const time_point &p );
/** Returns the current light level of the moon. */
moon_phase get_moon_phase( const time_point &p );

enum class weekdays : int {
    SUNDAY = 0,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
};

weekdays day_of_week( const time_point &p );

#endif
