#ifndef _CALENDAR_H_
#define _CALENDAR_H_

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
  int second;
  int minute;
  int hour;
  int day;
  season_type season;
  int year;
// End data

public:
  calendar();
  calendar(const calendar &copy);
  calendar(int Minute, int Hour, int Day, season_type Season, int Year);
  calendar(int turn);
  int get_turn() const;
  operator int() const; // Returns get_turn() for backwards compatibility
  calendar& operator = (calendar &rhs);
  calendar& operator = (int rhs);
  calendar& operator -=(calendar &rhs);
  calendar& operator -=(int rhs);
  calendar& operator +=(calendar &rhs);
  calendar& operator +=(int rhs);
  calendar  operator - (calendar &rhs);
  calendar  operator - (int rhs);
  calendar  operator + (calendar &rhs);
  calendar  operator + (int rhs);
  bool      operator ==(int rhs) const;
  bool      operator ==(calendar &rhs) const;

  void increment();   // Add one turn / 6 seconds

  void standardize(); // Ensure minutes <= 59, hour <= 23, etc.

  int getHour(); // return hour

// Sunlight and day/night calcuations
  int minutes_past_midnight() const; // Useful for sunrise/set calculations
  moon_phase moon() const;  // Find phase of moon
  calendar sunrise() const; // Current time of sunrise
  calendar sunset() const;  // Current time of sunset
  bool is_night() const;    // After sunset + TWILIGHT_MINUTES, before sunrise
  int sunlight() const;     // Current amount of sun/moonlight; uses preceding funcs

  // Basic accessors
  int seconds() const {return second;}
  int minutes() const {return minute;}
  int hours() const {return hour;}
  int days() const {return day;}
  season_type get_season() const {return season;}
  int years() const {return year;}

  void set_season(season_type new_season) {season = new_season;}


// Print-friendly stuff
  std::string print_time(bool just_hour = false) const;
  std::string textify_period(); // "1 second" "2 hours" "two days"
  std::string day_of_week() const;
};
#endif // _CALENDAR_H_
