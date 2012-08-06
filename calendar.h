#include <string>
#include "weather.h"

// How many minutes exist when the game starts - 8:00 AM
#define STARTING_MINUTES 480
// The number of days in a season - this also defines moon cycles
#define DAYS_IN_SEASON 14

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

enum moon_phase {
MOON_NEW = 0,
MOON_HALF,
MOON_FULL
};

class calendar
{
 public:
// The basic data; note that "second" should always be a multiple of 6
  int second;
  int minute;
  int hour;
  int day;
  season_type season;
  int year;
// End data

  calendar();
  calendar(const calendar &copy);
  calendar(int Minute, int Hour, int Day, season_type Season, int Year);
  calendar(int turn);
  int get_turn();
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

  void increment();   // Add one turn / 6 seconds

  void standardize(); // Ensure minutes <= 59, hour <= 23, etc.

// Sunlight and day/night calcuations
  int minutes_past_midnight(); // Useful for sunrise/set calculations
  moon_phase moon();  // Find phase of moon
  calendar sunrise(); // Current time of sunrise
  calendar sunset();  // Current time of sunset
  bool is_night();    // After sunset + TWILIGHT_MINUTES, before sunrise
  int sunlight();     // Current amount of sun/moonlight; uses preceding funcs

// Print-friendly stuff
  std::string print_time(bool twentyfour = false);
  std::string textify_period(); // "1 second" "2 hours" "two days"
};
