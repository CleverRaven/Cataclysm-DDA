#include "calendar.h"

calendar::calendar() {
  t.tm_year = 12; 
  t.tm_mon = 12;
  t.tm_mday = 21;
  t.tm_hour = 8;
  t.tm_min = 0;
  t.tm_sec = 0;
  t.tm_isdst = 0;

  timedate = mktime(&t);
  starting_seconds = timedate;
  sunrise = time(0);
  sr = *localtime(&sunrise);
  sunset = time(0);
  ss = *localtime(&sunset);

  get_sunrise();
  get_sunset();
}

calendar::~calendar() {

}

unsigned int calendar::reinitialize(unsigned int turn) {
  timedate = (turn * 6) + starting_seconds;
}

unsigned int calendar::turn() {
  return((timedate - starting_seconds) / 6);
}

/* returns day of season. assumes 14 days in a season */
int calendar::day() const {
  int seconds;
  seconds = timedate - starting_seconds;
  seconds += 8 * 3600; /* prevent wonkiness of the day changing in the middle of the night */
  return(seconds / 86400 % 14 + 1);
}

/* returns the current season */
season_type calendar::season() const {
  int seconds;
  seconds = timedate - starting_seconds;
  seconds += 8 * 3600;
  return((season_type) (seconds / 86400 / 14 % 4));
}

/* determines the year based on 14 days in a season and 4 season / year */
int calendar::year() const {
  int seconds;
  seconds = timedate - starting_seconds;
  seconds += 8 * 3600;
  return(seconds / 86400 / 14 / 4 + 1);
}

/* returns the phase of the moon. argument is a day from 1-14 */
moon_phase calendar::moon(int m) const { 
  if (m > 12 || m < 3)
    return(MOON_FULL);
  else if (m > 10 || m < 5)
    return(MOON_BRIGHT);
  else if (m > 8 || m < 7)
    return(MOON_DIM);
  else
    return(MOON_NEW);
}

/* full moon lasts from day 13 until day 2 of the next season 
   this lets us start the game w/ a full moon. From there, we progress 
   downwards to a new moon in the middle of the month */
int calendar::moonlight(int m) const {
  switch (moon(m)) {
    case MOON_FULL:
      return(9);
      break;
    case MOON_BRIGHT:
      return(6);
      break;
    case MOON_DIM:
      return(3);
      break;
    default:
      return(1);
  }
}

int calendar::hour() const {
  struct tm *t;
  t = localtime(&timedate);
  return(t->tm_hour);
}

int calendar::minute() const {
  struct tm *t;
  t = localtime(&timedate);
  return(t->tm_min);
}

int calendar::second() const {
  struct tm *t;
  t = localtime(&timedate);
  return(t->tm_sec);
}

struct tm calendar::get_sunrise() {
  struct tm *t;
  t = localtime(&timedate);
/* we only care about hour and minute for sunset / sunrise,
   but time structures are convenient	*/
  if (sr.tm_mday != t->tm_mday) {
    sr.tm_year = t->tm_year; 
    sr.tm_mon = t->tm_mon;
    sr.tm_mday = t->tm_mday;
    /* nominal sunrise 6:30 can vary by 84 minutes */
    sr.tm_hour = 6;
    sr.tm_min = 30;
    sr.tm_sec = 0;
    sr.tm_isdst = 0;
    sunrise = mktime(&sr);
    sunrise -= offset() * 60;
    sr = *localtime(&sunrise);
  }
  return(sr);
}

struct tm calendar::get_sunset() {
  struct tm *t;
  t = localtime(&timedate);
  if (ss.tm_mday != t->tm_mday) {
    ss.tm_year = t->tm_year; 
    ss.tm_mon = t->tm_mon;
    ss.tm_mday = t->tm_mday;
    ss.tm_hour = 18;
    ss.tm_min = 30;
    ss.tm_sec = 0;
    ss.tm_isdst = 0;
    sunset = mktime(&ss);
    sunset += offset() * 60;
    ss = *localtime(&sunset);
  }
  return(ss);
}

int calendar::offset() const {
/* sunrise and sunset varies by 3 minutes per day. overall, the daylight period
   will vary 6 minutes from one day to the next. Over the course of the year, this
   will give roughly 5 1/2 hours more sunlight in the middle of the summer than the
   middle of winter */
  int d;
  int m;
  d = day();

  switch (season()) {
    case SPRING:
      m = 21; /* minutes we gained since winter solstice */
      m += 3 * d;
      break;
    case SUMMER:
      m = 63; /* late winter + spring gain */
      if (d < 8) {
        m += 3 * d;
      } else 
        m -= (3 * d);
      break;
    case AUTUMN:
      m = -21; /* time loss since summer solstice */
      m -= 3 * d;
      break;
    case WINTER:
      m = -63; /* summer / fall loss */
      if (d > 7)
        m += 3 * d;
      else 
        m -= 3 * d;
      break;
     default: // shouldn't be possible
       m = 0;
  }
  return(m);
}
