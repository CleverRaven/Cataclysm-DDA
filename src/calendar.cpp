#include <sstream>
#include "calendar.h"
#include "output.h"
#include "options.h"
#include "translations.h"

calendar::calendar()
{
 second = 0;
 minute = 0;
 hour = 0;
 day = 0;
 season = SPRING;
 year = 0;
}

calendar::calendar(const calendar &copy)
{
 second = copy.second;
 minute = copy.minute;
 hour   = copy.hour;
 day    = copy.day;
 season = copy.season;
 year   = copy.year;
}

calendar::calendar(int Minute, int Hour, int Day, season_type Season, int Year)
{
 second = 0;
 minute = Minute;
 hour = Hour;
 day = Day;
 season = Season;
 year = Year;
}

calendar::calendar(int turn)
{
 int minute_param = int(turn / 10);
 int hour_param = minute_param / 60;
 int day_param = hour_param / 24;
 int season_param = int(day_param / OPTIONS["SEASON_LENGTH"]);
 second = 6 * (turn % 10);
 minute = minute_param % 60;
 hour = hour_param % 24;
 day = 1 + day_param % (int)OPTIONS["SEASON_LENGTH"];
 season = season_type(season_param % 4);
 year = season_param / 4;
}

int calendar::get_turn() const
{
 int ret = second / 6;
 ret += minute * 10;
 ret += hour * 600;
 ret += day * 14400;
 ret += int(season) * 14400 * (int)OPTIONS["SEASON_LENGTH"];
 ret += year * 14400 * 4 * (int)OPTIONS["SEASON_LENGTH"];
 return ret;
}

calendar::operator int() const
{
 int ret = second / 6;
 ret += minute * 10;
 ret += hour * 600;
 ret += day * 14400;
 ret += int(season) * 14400 * (int)OPTIONS["SEASON_LENGTH"];
 ret += year * 14400 * 4 * (int)OPTIONS["SEASON_LENGTH"];
 return ret;
}

calendar& calendar::operator =(calendar &rhs)
{
 if (this == &rhs)
  return *this;

 second = rhs.second;
 minute = rhs.minute;
 hour = rhs.hour;
 day = rhs.day;
 season = rhs.season;
 year = rhs.year;

 return *this;
}

calendar& calendar::operator =(int rhs)
{
 int minute_param = int(rhs / 10);
 int hour_param = minute_param / 60;
 int day_param = hour_param / 24;
 int season_param = int(day_param / OPTIONS["SEASON_LENGTH"]);
 second = 6 * (rhs % 10);
 minute = minute_param % 60;
 hour = hour_param % 24;
 day = day_param % (int)OPTIONS["SEASON_LENGTH"];
 season = season_type(season_param % 4);
 year = season_param / 4;
 return *this;
}

calendar& calendar::operator -=(calendar &rhs)
{
 calendar tmp(rhs);
 tmp.standardize();
 second -= tmp.second;
 minute -= tmp.minute;
 hour   -= tmp.hour;
 day    -= tmp.day;
 int tmpseason = int(season) - int(tmp.season);
 while (tmpseason < 0) {
  year--;
  tmpseason += 4;
 }
 season = season_type(tmpseason);
 year -= tmp.year;
 standardize();
 return *this;
}

calendar& calendar::operator -=(int rhs)
{
 calendar tmp(rhs);
 *this -= tmp;
 return *this;
}

calendar& calendar::operator +=(calendar &rhs)
{
 second += rhs.second;
 minute += rhs.minute;
 hour   += rhs.hour;
 day    += rhs.day;
 int tmpseason = int(season) + int(rhs.season);
 while (tmpseason >= 4) {
  year++;
  tmpseason -= 4;
 }
 season = season_type(tmpseason);
 year += rhs.year;
 standardize();
 return *this;
}

calendar& calendar::operator +=(int rhs)
{
 second += rhs * 6;
 standardize();
 return *this;
}

bool calendar::operator ==(int rhs) const
{
 return int(*this) == rhs;
}
bool calendar::operator ==(calendar &rhs) const
{
 return (second == rhs.second &&
         minute == rhs.minute &&
         hour   == rhs.hour &&
         day    == rhs.day &&
         season == rhs.season &&
         year   == rhs.year);
}

/*
calendar& calendar::operator ++()
{
 *this += 1;
 return *this;
}
*/

calendar calendar::operator -(calendar &rhs)
{
 return calendar(*this) -= rhs;
}

calendar calendar::operator -(int rhs)
{
 return calendar(*this) -= rhs;
}

calendar calendar::operator +(calendar &rhs)
{
 return calendar(*this) += rhs;
}

calendar calendar::operator +(int rhs)
{
 return calendar(*this) += rhs;
}

void calendar::increment()
{
 second += 6;
 if (second >= 60)
  standardize();
}

int calendar::getHour()
{
    return hour;
}

void calendar::standardize()
{
 if (second >= 60) {
  minute += second / 60;
  second %= 60;
 }
 if (minute >= 60) {
  hour += minute / 60;
  minute %= 60;
 }
 if (hour >= 24) {
  day += hour / 24;
  hour %= 24;
 }
 int tmpseason = int(season);
 if (day >= OPTIONS["SEASON_LENGTH"]) {
  tmpseason += int(day / OPTIONS["SEASON_LENGTH"]);
  day %= (int)OPTIONS["SEASON_LENGTH"];
 }
 if (tmpseason >= 4) {
  year += tmpseason / 4;
  tmpseason %= 4;
 }
 season = season_type(tmpseason);
}

int calendar::minutes_past_midnight() const
{
 //debugmsg("minute: %d  hour: %d");
 int ret = minute + hour * 60;
 return ret;
}

moon_phase calendar::moon() const
{
 int phase = int(day / (OPTIONS["SEASON_LENGTH"] / 4));
 //phase %= 4;   Redundant?
 if (phase == 3)
  return MOON_HALF;
 else
  return moon_phase(phase);
}

calendar calendar::sunrise() const
{
 calendar ret;
 int start_hour = 0, end_hour = 0;
 switch (season) {
  case SPRING:
   start_hour = SUNRISE_SOLSTICE;
   end_hour   = SUNRISE_SUMMER;
   break;
  case SUMMER:
   start_hour = SUNRISE_SUMMER;
   end_hour   = SUNRISE_SOLSTICE;
   break;
  case AUTUMN:
   start_hour = SUNRISE_SOLSTICE;
   end_hour   = SUNRISE_WINTER;
   break;
  case WINTER:
   start_hour = SUNRISE_WINTER;
   end_hour   = SUNRISE_SOLSTICE;
   break;
 }
 double percent = double(double(day) / OPTIONS["SEASON_LENGTH"]);
 double time = double(start_hour) * (1.- percent) + double(end_hour) * percent;

 ret.hour = int(time);
 time -= int(time);
 ret.minute = int(time * 60);

 return ret;
}

calendar calendar::sunset() const
{
 calendar ret;
 int start_hour = 0, end_hour = 0;
 switch (season) {
  case SPRING:
   start_hour = SUNSET_SOLSTICE;
   end_hour   = SUNSET_SUMMER;
   break;
  case SUMMER:
   start_hour = SUNSET_SUMMER;
   end_hour   = SUNSET_SOLSTICE;
   break;
  case AUTUMN:
   start_hour = SUNSET_SOLSTICE;
   end_hour   = SUNSET_WINTER;
   break;
  case WINTER:
   start_hour = SUNSET_WINTER;
   end_hour   = SUNSET_SOLSTICE;
   break;
 }
 double percent = double(double(day) / OPTIONS["SEASON_LENGTH"]);
 double time = double(start_hour) * (1.- percent) + double(end_hour) * percent;

 ret.hour = int(time);
 time -= int(time);
 ret.minute = int(time * 60);

 return ret;
}

bool calendar::is_night() const
{
 calendar sunrise_time = sunrise(), sunset_time = sunset();

 int mins         = minutes_past_midnight(),
     sunrise_mins = sunrise_time.minutes_past_midnight(),
     sunset_mins  = sunset_time.minutes_past_midnight();

 return (mins > sunset_mins + TWILIGHT_MINUTES || mins < sunrise_mins);
}

int calendar::sunlight() const
{
 calendar sunrise_time = sunrise(), sunset_time = sunset();

 int mins = 0, sunrise_mins = 0, sunset_mins = 0;
 mins = minutes_past_midnight();
 sunrise_mins = sunrise_time.minutes_past_midnight();
 sunset_mins = sunset_time.minutes_past_midnight();

 int moonlight = 1 + int(moon()) * MOONLIGHT_LEVEL;

 if (mins > sunset_mins + TWILIGHT_MINUTES || mins < sunrise_mins) // Night
  return moonlight;

 else if (mins >= sunrise_mins && mins <= sunrise_mins + TWILIGHT_MINUTES) {

  double percent = double(mins - sunrise_mins) / TWILIGHT_MINUTES;
  return int( double(moonlight)      * (1. - percent) +
              double(DAYLIGHT_LEVEL) * percent         );

 } else if (mins >= sunset_mins && mins <= sunset_mins + TWILIGHT_MINUTES) {

  double percent = double(mins - sunset_mins) / TWILIGHT_MINUTES;
  return int( double(DAYLIGHT_LEVEL) * (1. - percent) +
              double(moonlight)      * percent         );

 } else
  return DAYLIGHT_LEVEL;
}



std::string calendar::print_time(bool just_hour) const
{
    std::stringstream time_string;
    int hour_param;

    if (OPTIONS["24_HOUR"] == "military") {
        hour_param = hour % 24;
        time_string << string_format("%02d%02d", hour_param, minute);
    } else if (OPTIONS["24_HOUR"] == "24h") {
        hour_param = hour % 24;
        if (just_hour) {
            time_string << hour_param;
        } else {
            //~ hour:minute (24hr time display)
            time_string << string_format(_("%02d:%02d"), hour_param, minute);
        }
    } else {
        hour_param = hour % 12;
        if (hour_param == 0) {
            hour_param = 12;
        }
        if (just_hour && hour < 12) {
            time_string << string_format(_("%d AM"), hour_param);
        } else if (just_hour) {
            time_string << string_format(_("%d PM"), hour_param);
        } else if (hour < 12) {
            time_string << string_format(_("%d:%02d AM"), hour_param, minute);
        } else {
            time_string << string_format(_("%d:%02d PM"), hour_param, minute);
        }
    }

    return time_string.str();
}

std::string calendar::textify_period()
{
 standardize();
 int am;
 const char* tx;
// Describe the biggest time period, as "<am> <tx>s", am = amount, tx = name
 if (year > 0) {
  am = year;
  tx = ngettext("%d year", "%d years", am);
 } else if (season > 0) {
  am = season;
  tx = ngettext("%d season", "%d seasons", am);
 } else if (day > 0) {
  am = day;
  tx = ngettext("%d day", "%d days", am);
 } else if (hour > 0) {
  am = hour;
  tx = ngettext("%d hour", "%d hours", am);
 } else if (minute >= 5) {
  am = minute;
  tx = ngettext("%d minute", "%d minutes", am);
 } else {
  am = second / 6 + minute * 10;
  tx = ngettext("%d turn", "%d turns", am);
 }

 return string_format(tx, am);
}

std::string calendar::day_of_week() const
{
    // Design rationale
    // <kevingranade> here's a question
    // <kevingranade> what day of the week is day 0?
    // <wito> Sunday
    // <GlyphGryph> Why does it matter?
    // <GlyphGryph> For like where people are and stuff?
    // <wito> 7 is also Sunday
    // <kevingranade> NOAA weather forecasts include day of week
    // <GlyphGryph> Also by day0 do you mean the day people start day 0
    // <GlyphGryph> Or actual day 0
    // <kevingranade> good point, turn 0
    // <GlyphGryph> So day 5
    // <wito> Oh, I thought we were talking about week day numbering in general.
    // <wito> Day 5 is a thursday, I think.
    // <wito> Nah, Day 5 feels like a thursday. :P
    // <wito> Which would put the apocalpyse on a saturday?
    // <Starfyre> must be a thursday.  I was never able to get the hang of those.
    // <ZChris13> wito: seems about right to me
    // <wito> kevingranade: add four for thursday. ;)
    // <kevingranade> sounds like consensus to me
    // <kevingranade> Thursday it is

    enum weekday
    {
        THURSDAY = 0,
        FRIDAY = 1,
        SATURDAY = 2,
        SUNDAY = 3,
        MONDAY = 4,
        TUESDAY = 5,
        WEDNESDAY = 6
    };

    // calendar::day gets mangled by season transitions, so reclaculate days since start.
    int current_day = day % 7;

    std::string day_string;

    switch (current_day)
    {
    case SUNDAY:
        day_string = _("Sunday");
        break;
    case MONDAY:
        day_string = _("Monday");
        break;
    case TUESDAY:
        day_string = _("Tuesday");
        break;
    case WEDNESDAY:
        day_string = _("Wendsday");
        break;
    case THURSDAY:
        day_string = _("Thursday");
        break;
    case FRIDAY:
        day_string = _("Friday");
        break;
    case SATURDAY:
        day_string = _("Saturday");
        break;
    }

    return day_string;
}
