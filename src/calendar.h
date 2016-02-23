#ifndef CALENDAR_H
#define CALENDAR_H

#include <string>

// Convert minutes, hours, days to turns
#define MINUTES(x) ((x) * 10)
#define HOURS(x)   ((x) * 600)
#define DAYS(x)    ((x) * 14400)

// How much light the moon provides per quater
#define MOONLIGHT_PER_QUATER 2.25

// How much light is provided in full daylight
#define DAYLIGHT_LEVEL 100

// How long real-life seasons last, in days, for reference
#define REAL_WORLD_SEASON_LENGTH 91

enum season_type {
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
};

enum moon_phase {
    MOON_NEW = 0,
    MOON_WAXING_CRESCENT,
    MOON_HALF_MOON_WAXING,
    MOON_WAXING_GIBBOUS,
    MOON_FULL,
    MOON_WANING_GIBBOUS,
    MOON_HALF_MOON_WANING,
    MOON_WANING_CRESCENT,
    MOON_PHASE_MAX
};

class calendar
{
    private:
        // The basic data; note that "second" should always be a multiple of 6
        int turn_number;
        int second;
        int minute;
        int hour;
        int day;
        season_type season;
        int year;
        // End data

        void sync(); // Synchronize all variables to the turn_number

    public:
        /** Initializers */
        calendar();
        calendar( const calendar &copy ) = default;
        calendar( int Minute, int Hour, int Day, season_type Season, int Year );
        calendar( int turn );
        /** Returns the current turn_number. */
        int get_turn() const;
        operator int() const; // Returns get_turn() for backwards compatibility
        /** Basic calendar operators. Usually modifies or checks the turn_number of the calendar */
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
        /** Returns true if it's currently after sunset + TWILIGHT_SECONDS or before sunrise. */
        bool is_night() const;
        /** Returns the current sunlight or moonlight level through the preceding functions. */
        float sunlight() const;

        /** Basic accessors */
        int seconds() const {
            return second;
        }
        int minutes() const {
            return minute;
        }
        int hours() const {
            return hour;
        }
        int days() const {
            return day;
        }
        season_type get_season() const {
            return season;
        }
        int years() const {
            return year;
        }

        /**
         * Predicate to handle rate-limiting, returns true once every @event_frequency turns.
         */
        static bool once_every( int event_frequency );

        // Season and year length stuff
    private:
        // cached value from world options
        static int cached_season_length;
    public:
        // to be called from option handling when the options of the active world change.
        static void set_season_length( int new_length );
        static int year_turns() {
            return DAYS( year_length() );
        }
        static int year_length() { // In days
            return season_length() * 4;
        }
        static int season_length(); // In days

        static float season_ratio() { //returns relative length of game season to irl season
            return static_cast<float>( season_length() ) / REAL_WORLD_SEASON_LENGTH;
        }

        int turn_of_year() const {
            return turn_number % year_turns();
        }
        int day_of_year() const {
            return day + season_length() * season;
        }

        /** Returns the current time in a string according to the options set */
        std::string print_time( bool just_hour = false ) const;
        /** Returns the period a calendar has been running in word form; i.e. "1 second", "2 days". */
        std::string textify_period() const;
        /** Returns the name of the current day of the week */
        std::string day_of_week() const;

        static   calendar start;
        static   calendar turn;
        static season_type initial_season;
        static bool eternal_season;
};
#endif
