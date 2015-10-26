#include <sstream>
#include "calendar.h"
#include "output.h"
#include "options.h"
#include "translations.h"
#include "game.h"
#include "debug.h"

int calendar::cached_season_length = 14;

calendar calendar::start;
calendar calendar::turn;
season_type calendar::initial_season;
bool calendar::eternal_season = false;

// Internal constants, not part of the calendar interface.
// Times for sunrise, sunset at equinoxes
#define SUNRISE_WINTER   7
#define SUNRISE_SOLSTICE 6
#define SUNRISE_SUMMER   5

#define SUNSET_WINTER   17
#define SUNSET_SOLSTICE 19
#define SUNSET_SUMMER   21

// How long, in seconds, does sunrise/sunset last?
#define TWILIGHT_SECONDS (60 * 60)


calendar::calendar()
{
    turn_number = 0;
    second = 0;
    minute = 0;
    hour = 0;
    day = 0;
    season = SPRING;
    year = 0;
}

calendar::calendar(int Minute, int Hour, int Day, season_type Season, int Year)
{
    turn_number = MINUTES(Minute) + HOURS(Hour) + DAYS(Day) + Season * season_length() + Year * year_turns();
    sync();
}

calendar::calendar(int turn)
{
    turn_number = turn;
    sync();
}

int calendar::get_turn() const
{
    return turn_number;
}

calendar::operator int() const
{
    return turn_number;
}

calendar &calendar::operator =(int rhs)
{
    turn_number = rhs;
    sync();
    return *this;
}

calendar &calendar::operator -=(const calendar &rhs)
{
    turn_number -= rhs.turn_number;
    sync();
    return *this;
}

calendar &calendar::operator -=(int rhs)
{
    turn_number -= rhs;
    sync();
    return *this;
}

calendar &calendar::operator +=(const calendar &rhs)
{
    turn_number += rhs.turn_number;
    sync();
    return *this;
}

calendar &calendar::operator +=(int rhs)
{
    turn_number += rhs;
    sync();
    return *this;
}

bool calendar::operator ==(int rhs) const
{
    return int(*this) == rhs;
}
bool calendar::operator ==(const calendar &rhs) const
{
    return turn_number == rhs.turn_number;
}

/*
calendar& calendar::operator ++()
{
 *this += 1;
 return *this;
}
*/

calendar calendar::operator -(const calendar &rhs) const
{
    return calendar(*this) -= rhs;
}

calendar calendar::operator -(int rhs) const
{
    return calendar(*this) -= rhs;
}

calendar calendar::operator +(const calendar &rhs) const
{
    return calendar(*this) += rhs;
}

calendar calendar::operator +(int rhs) const
{
    return calendar(*this) += rhs;
}

void calendar::increment()
{
    turn_number++;
    sync();
}

int calendar::minutes_past_midnight() const
{
    //debugmsg("minute: %d  hour: %d");
    int ret = minute + hour * 60;
    return ret;
}

int calendar::seconds_past_midnight() const
{
    return second + (minute * 60) + (hour * 60 * 60);
}

moon_phase calendar::moon() const
{
    //One full phase every 2 rl months = 2/3 season length
    static float phase_change_per_day = 1.0 / ((float(season_length()) * 2.0 / 3.0) / float(MOON_PHASE_MAX));

    //Switch moon phase at noon so it stays the same all night
    const int current_day = round( (calendar::turn.get_turn() + DAYS(1) / 2) / DAYS(1) );
    const int current_phase = int(round(float(current_day) * phase_change_per_day)) % int(MOON_PHASE_MAX);

    return moon_phase(current_phase);
}

calendar calendar::sunrise() const
{
    int start_hour = 0, end_hour = 0, newhour = 0, newminute = 0;
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
    double percent = double(double(day) / season_length());
    double time = double(start_hour) * (1. - percent) + double(end_hour) * percent;

    newhour = int(time);
    time -= int(time);
    newminute = int(time * 60);

    return calendar (newminute, newhour, day, season, year);
}

calendar calendar::sunset() const
{
    int start_hour = 0, end_hour = 0, newhour = 0, newminute = 0;
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
    double percent = double(double(day) / season_length());
    double time = double(start_hour) * (1. - percent) + double(end_hour) * percent;

    newhour = int(time);
    time -= int(time);
    newminute = int(time * 60);

    return calendar (newminute, newhour, day, season, year);
}

bool calendar::is_night() const
{
    int seconds         = seconds_past_midnight();
    int sunrise_seconds = sunrise().seconds_past_midnight();
    int sunset_seconds  = sunset().seconds_past_midnight();

    return (seconds > sunset_seconds + TWILIGHT_SECONDS || seconds < sunrise_seconds);
}

float calendar::sunlight() const
{
    int seconds = seconds_past_midnight();
    int sunrise_seconds = sunrise().seconds_past_midnight();
    int sunset_seconds = sunset().seconds_past_midnight();

    int current_phase = int(moon());
    if ( current_phase > int(MOON_PHASE_MAX)/2 ) {
        current_phase = int(MOON_PHASE_MAX) - current_phase;
    }

    int moonlight = 1 + int(current_phase * MOONLIGHT_PER_QUATER);

    if( seconds > sunset_seconds + TWILIGHT_SECONDS || seconds < sunrise_seconds ) { // Night
        return moonlight;
    } else if( seconds >= sunrise_seconds && seconds <= sunrise_seconds + TWILIGHT_SECONDS ) {
        double percent = double(seconds - sunrise_seconds) / TWILIGHT_SECONDS;
        return double(moonlight) * (1. - percent) + double(DAYLIGHT_LEVEL) * percent;
    } else if( seconds >= sunset_seconds && seconds <= sunset_seconds + TWILIGHT_SECONDS ) {
        double percent = double(seconds - sunset_seconds) / TWILIGHT_SECONDS;
        return double(DAYLIGHT_LEVEL) * (1. - percent) + double(moonlight) * percent;
    } else {
        return DAYLIGHT_LEVEL;
    }
}



std::string calendar::print_time(bool just_hour) const
{
    std::stringstream time_string;
    int hour_param;

    if (OPTIONS["24_HOUR"] == "military") {
        hour_param = hour % 24;
        time_string << string_format("%02d%02d.%02d", hour_param, minute, second);
    } else if (OPTIONS["24_HOUR"] == "24h") {
        hour_param = hour % 24;
        if (just_hour) {
            time_string << hour_param;
        } else {
            //~ hour:minute (24hr time display)
            time_string << string_format(_("%02d:%02d:%02d"), hour_param, minute, second);
        }
    } else {
        hour_param = hour % 12;
        if (hour_param == 0) {
            hour_param = 12;
        }
        // Padding is removed as necessary to prevent clipping with SAFE notification in wide sidebar mode
        std::string padding = hour_param < 10 ? " " : "";
        if (just_hour && hour < 12) {
            time_string << string_format(_("%d AM"), hour_param);
        } else if (just_hour) {
            time_string << string_format(_("%d PM"), hour_param);
        } else if (hour < 12) {
            time_string << string_format(_("%d:%02d:%02d%sAM"), hour_param, minute, second, padding.c_str());
        } else {
            time_string << string_format(_("%d:%02d:%02d%sPM"), hour_param, minute, second, padding.c_str());
        }
    }

    return time_string.str();
}

std::string calendar::textify_period() const
{
    int am;
    const char *tx;
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
    /* Design rationale:
     * <kevingranade> here's a question
     * <kevingranade> what day of the week is day 0?
     * <wito> Sunday
     * <GlyphGryph> Why does it matter?
     * <GlyphGryph> For like where people are and stuff?
     * <wito> 7 is also Sunday
     * <kevingranade> NOAA weather forecasts include day of week
     * <GlyphGryph> Also by day0 do you mean the day people start day 0
     * <GlyphGryph> Or actual day 0
     * <kevingranade> good point, turn 0
     * <GlyphGryph> So day 5
     * <wito> Oh, I thought we were talking about week day numbering in general.
     * <wito> Day 5 is a thursday, I think.
     * <wito> Nah, Day 5 feels like a thursday. :P
     * <wito> Which would put the apocalpyse on a saturday?
     * <Starfyre> must be a thursday.  I was never able to get the hang of those.
     * <ZChris13> wito: seems about right to me
     * <wito> kevingranade: add four for thursday. ;)
     * <kevingranade> sounds like consensus to me
     * <kevingranade> Thursday it is */

    enum weekday {
        THURSDAY = 0,
        FRIDAY = 1,
        SATURDAY = 2,
        SUNDAY = 3,
        MONDAY = 4,
        TUESDAY = 5,
        WEDNESDAY = 6
    };

    // calendar::day gets mangled by season transitions, so recalculate days since start.
    int current_day = turn_number / DAYS(1) % 7;

    std::string day_string;

    switch (current_day) {
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
        day_string = _("Wednesday");
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

int calendar::season_length()
{
    return cached_season_length;
}

void calendar::sync()
{
    const int sl = season_length();
    year = turn_number / DAYS(sl * 4);

    if( eternal_season ) {
        season = initial_season;
    } else {
        season = season_type(turn_number / DAYS(sl) % 4);
    }

    day = turn_number / DAYS(1) % sl;
    hour = turn_number / HOURS(1) % 24;
    minute = turn_number / MINUTES(1) % 60;
    second = (turn_number * 6) % 60;
}

bool calendar::once_every(int event_frequency) {
    return (calendar::turn % event_frequency) == 0;
}

void calendar::set_season_length( const int length )
{
    // 14 is the default and it's used whenever the input is invalid so
    // everyone using the cached value can rely on it being larger than 0.
    cached_season_length = length <= 0 ? 14 : length;
}
