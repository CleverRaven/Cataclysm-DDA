#ifndef CALENDAR_H
#define CALENDAR_H

#include <string>

// Convert minutes, hours, days to turns
#define MINUTES(x) ((x) * 10)
#define HOURS(x)   ((x) * 600)
#define DAYS(x)    ((x) * 14400)

// Times for sunrise, sunset at equinoxes
#define SUNRISE_WINTER   7
#define SUNRISE_SOLSTICE 6
#define SUNRISE_SUMMER   5

#define SUNSET_WINTER   17
#define SUNSET_SOLSTICE 19
#define SUNSET_SUMMER   21

// How long, in minutes, does sunrise/sunset last?
#define TWILIGHT_MINUTES 60

// How much light moon provides--double for full moon
#define MOONLIGHT_LEVEL 4
// How much light is provided in full daylight
#define DAYLIGHT_LEVEL 60

enum season_type {
    SPRING = 0,
    SUMMER = 1,
    AUTUMN = 2,
    WINTER = 3
#define FALL AUTUMN
};

enum moon_phase {
    MOON_NEW = 0,
    MOON_HALF,
    MOON_FULL
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
        calendar(const calendar &copy) = default;
        calendar(int Minute, int Hour, int Day, season_type Season, int Year);
        calendar(int turn);
        /** Returns the current turn_number. */
        int get_turn() const;
        operator int() const; // Returns get_turn() for backwards compatibility
        /** Basic calendar operators. Usually modifies or checks the turn_number of the calendar */
        calendar &operator = (const calendar &rhs) = default;
        calendar &operator = (int rhs);
        calendar &operator -=(const calendar &rhs);
        calendar &operator -=(int rhs);
        calendar &operator +=(const calendar &rhs);
        calendar &operator +=(int rhs);
        calendar  operator - (const calendar &rhs) const;
        calendar  operator - (int rhs) const;
        calendar  operator + (const calendar &rhs) const;
        calendar  operator + (int rhs) const;
        bool      operator ==(int rhs) const;
        bool      operator ==(const calendar &rhs) const;

        /** Increases turn_number by 1. (6 seconds) */
        void increment();

        // Sunlight and day/night calculations
        /** Returns the number of minutes past midnight. Used for sunrise/set calculations. */
        int minutes_past_midnight() const;
        /** Returns the current phase of the moon. */
        moon_phase moon() const;
        /** Returns the current sunrise time based on the time of year. */
        calendar sunrise() const;
        /** Returns the current sunset time based on the time of year. */
        calendar sunset() const;
        /** Returns true if it's currently after sunset + TWILIGHT_MINUTES or before sunrise. */
        bool is_night() const;
        /** Returns the current sunlight or moonlight level through the preceding functions. */
        int sunlight() const;

        /** Basic accessors */
        int seconds() const
        {
            return second;
        }
        int minutes() const
        {
            return minute;
        }
        int hours() const
        {
            return hour;
        }
        int days() const
        {
            return day;
        }
        season_type get_season() const
        {
            return season;
        }
        int years() const
        {
            return year;
        }


        // Season and year length stuff
        static int year_turns()
        {
            return DAYS(year_length());
        }
        static int year_length() // In days
        {
            return season_length() * 4;
        }
        static int season_length(); // In days

        int turn_of_year() const
        {
            return turn_number % year_turns();
        }
        int day_of_year() const
        {
            return day + season_length() * season;
        }

        /** Returns the current time in a string according to the options set */
        std::string print_time(bool just_hour = false) const;
        /** Returns the period a calendar has been running in word form; i.e. "1 second", "2 days". */
        std::string textify_period();
        /** Returns the name of the current day of the week */
        std::string day_of_week() const;

        static   calendar start;
        static   calendar turn;
};
#endif
