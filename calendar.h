/* not sure how to port this to windows */
#include "weather.h"
#include <time.h>
#include <sys/time.h>

enum moon_phase {
MOON_NEW,
MOON_DIM,
MOON_BRIGHT,
MOON_FULL
};

class calendar
{
  public:
    calendar();
    ~calendar();
    time_t timedate, sunset, sunrise; // time, sunrise/set in seconds
    struct tm t, sr, ss; // fancy struct to deal w/ hours etc
    unsigned int reinitialize(unsigned int turn); // restore from save file
    unsigned int turn(); // return the current turn number
    int day() const; // returns 1-14 for day of season
    season_type season() const; // season of the year
    int year() const; // current year
    moon_phase moon(int m) const; // phase of moon
    int moonlight(int m) const; // how much moonlight
    int hour() const; // hours
    int minute() const; // minutes
    int second() const; // seconds
    int offset() const; // offset in minutes for seasonal sunset/rise
    struct tm get_sunset(); // time of sunset / sunrise
    struct tm get_sunrise();
  private:
    unsigned int starting_seconds; // determines game time versus calendar time
};
