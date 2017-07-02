#include <cmath>
#include <sstream>
#include <limits>
#include <array>

#include "calendar.h"
#include "output.h"
#include "options.h"
#include "translations.h"
#include "game.h"
#include "debug.h"

// Divided by 100 to prevent overflowing when converted to moves
const int calendar::INDEFINITELY_LONG( std::numeric_limits<int>::max() / 100 );

calendar calendar::start;
calendar calendar::turn;
season_type calendar::initial_season;

// Internal constants, not part of the calendar interface.
// Times for sunrise, sunset at equinoxes

/** Hour of sunrise at winter solstice */
#define SUNRISE_WINTER   7

/** Hour of sunrise at fall and spring equinox */
#define SUNRISE_EQUINOX 6

/** Hour of sunrise at summer solstice */
#define SUNRISE_SUMMER   5

/** Hour of sunset at winter solstice */
#define SUNSET_WINTER   17

/** Hour of sunset at fall and spring equinox */
#define SUNSET_EQUINOX 19

/** Hour of setset at summer solstice */
#define SUNSET_SUMMER   21

// How long, in seconds, does sunrise/sunset last?
#define TWILIGHT_SECONDS (60 * 60)

constexpr int FULL_SECONDS_IN( int n )
{
    return n * 6;
}

constexpr int FULL_MINUTES_IN( int n )
{
    return n / MINUTES( 1 );
}

constexpr int FULL_HOURS_IN( int n )
{
    return n / HOURS( 1 );
}

constexpr int FULL_DAYS_IN( int n )
{
    return n / DAYS( 1 );
}

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
        start_hour = SUNRISE_EQUINOX;
        end_hour   = SUNRISE_SUMMER;
        break;
    case SUMMER:
        start_hour = SUNRISE_SUMMER;
        end_hour   = SUNRISE_EQUINOX;
        break;
    case AUTUMN:
        start_hour = SUNRISE_EQUINOX;
        end_hour   = SUNRISE_WINTER;
        break;
    case WINTER:
        start_hour = SUNRISE_WINTER;
        end_hour   = SUNRISE_EQUINOX;
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
        start_hour = SUNSET_EQUINOX;
        end_hour   = SUNSET_SUMMER;
        break;
    case SUMMER:
        start_hour = SUNSET_SUMMER;
        end_hour   = SUNSET_EQUINOX;
        break;
    case AUTUMN:
        start_hour = SUNSET_EQUINOX;
        end_hour   = SUNSET_WINTER;
        break;
    case WINTER:
        start_hour = SUNSET_WINTER;
        end_hour   = SUNSET_EQUINOX;
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

double calendar::current_daylight_level() const
{
    double percent = double(double(day) / season_length());
    double modifier = 1.0;
    // For ~Boston: solstices are +/- 25% sunlight intensity from equinoxes
    static double deviation = 0.25;
    
    switch (season) {
    case SPRING:
        modifier = 1. + (percent * deviation);
        break;
    case SUMMER:
        modifier = (1. + deviation) - (percent * deviation);
        break;
    case AUTUMN:
        modifier = 1. - (percent * deviation);
        break;
    case WINTER:
        modifier = (1. - deviation) + (percent * deviation);
        break;
    }
    
    return double(modifier * DAYLIGHT_LEVEL);
}

float calendar::sunlight() const
{
    int seconds = seconds_past_midnight();
    int sunrise_seconds = sunrise().seconds_past_midnight();
    int sunset_seconds = sunset().seconds_past_midnight();
    double daylight_level = current_daylight_level();

    int current_phase = int(moon());
    if ( current_phase > int(MOON_PHASE_MAX)/2 ) {
        current_phase = int(MOON_PHASE_MAX) - current_phase;
    }

    int moonlight = 1 + int(current_phase * MOONLIGHT_PER_QUARTER);

    if( seconds > sunset_seconds + TWILIGHT_SECONDS || seconds < sunrise_seconds ) { // Night
        return moonlight;
    } else if( seconds >= sunrise_seconds && seconds <= sunrise_seconds + TWILIGHT_SECONDS ) {
        double percent = double(seconds - sunrise_seconds) / TWILIGHT_SECONDS;
        return double(moonlight) * (1. - percent) + daylight_level * percent;
    } else if( seconds >= sunset_seconds && seconds <= sunset_seconds + TWILIGHT_SECONDS ) {
        double percent = double(seconds - sunset_seconds) / TWILIGHT_SECONDS;
        return daylight_level * (1. - percent) + double(moonlight) * percent;
    } else {
        return daylight_level;
    }
}

std::string calendar::print_clipped_duration( int turns )
{
    if( turns >= INDEFINITELY_LONG ) {
        return _( "forever" );
    }

    if( turns < MINUTES( 1 ) ) {
        const int sec = FULL_SECONDS_IN( turns );
        return string_format( ngettext( "%d second", "%d seconds", sec ), sec );
    } else if( turns < HOURS( 1 ) ) {
        const int min = FULL_MINUTES_IN( turns );
        return string_format( ngettext( "%d minute", "%d minutes", min ), min );
    } else if( turns < DAYS( 1 ) ) {
        const int hour = FULL_HOURS_IN( turns );
        return string_format( ngettext( "%d hour", "%d hours", hour ), hour );
    }
    const int day = FULL_DAYS_IN( turns );
    return string_format( ngettext( "%d day", "%d days", day ), day );
}

std::string calendar::print_duration( int turns )
{
    int divider = 0;

    if( turns > MINUTES( 1 ) && turns < INDEFINITELY_LONG ) {
        if( turns < HOURS( 1 ) ) {
            divider = MINUTES( 1 );
        } else if( turns < DAYS( 1 ) ) {
            divider = HOURS( 1 );
        } else {
            divider = DAYS( 1 );
        }
    }

    const int remainder = divider ? turns % divider : 0;
    if( remainder != 0 ) {
        //~ %1$s - greater units of time (e.g. 3 hours), %2$s - lesser units of time (e.g. 11 minutes).
        return string_format( _( "%1$s and %2$s" ),
                              print_clipped_duration( turns ).c_str(),
                              print_clipped_duration( remainder ).c_str() );
    }

    return print_clipped_duration( turns );
}

std::string calendar::print_approx_duration( int turns, bool verbose )
{
    const auto make_result = [verbose]( int turns, const char *verbose_str, const char *short_str ) {
        return string_format( verbose ? verbose_str : short_str, print_clipped_duration( turns ).c_str() );
    };

    int divider = 0;
    int vicinity = 0;

    if( turns > DAYS( 1 ) ) {
        divider = DAYS( 1 );
        vicinity = HOURS( 2 );
    } else if( turns > HOURS( 1 ) ) {
        divider = HOURS( 1 );
        vicinity = MINUTES( 5 );
    } // Minutes and seconds can be estimated precisely.

    if( divider != 0 ) {
        const int remainder = turns % divider;

        if( remainder >= divider - vicinity ) {
            turns += divider;
        } else if( remainder > vicinity ) {
            if( remainder < divider / 2 ) {
                //~ %s - time (e.g. 2 hours).
                return make_result( turns, _( "more than %s" ), ">%s" );
            } else {
                //~ %s - time (e.g. 2 hours).
                return make_result( turns + divider, _( "less than %s" ), "<%s" );
            }
        }
    }
    //~ %s - time (e.g. 2 hours).
    return make_result( turns, _( "about %s" ), "%s" );
}

std::string calendar::print_time(bool just_hour) const
{
    std::ostringstream time_string;
    int hour_param;

    if (get_option<std::string>( "24_HOUR" ) == "military") {
        hour_param = hour % 24;
        time_string << string_format("%02d%02d.%02d", hour_param, minute, second);
    } else if (get_option<std::string>( "24_HOUR" ) == "24h") {
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
    } else if ( season > 0 && !get_option<bool>( "ETERNAL_SEASON" ) ) {
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
    static const std::string s = "SEASON_LENGTH";
    // Avoid returning 0 as this value is used in division and expected to be non-zero.
    return std::max( get_option<int>( s ), 1 );
}

int calendar::turn_of_year() const
{
    return (season * season_turns()) + (turn_number % season_turns());
}

int calendar::day_of_year() const
{
    return day + season_length() * season;
}

int calendar::diurnal_time_before( int turn ) const
{
    const int remainder = turn % DAYS( 1 ) - get_turn() % DAYS( 1 );
    return ( remainder > 0 ) ? remainder : DAYS( 1 ) + remainder;
}

void calendar::sync()
{
    const int sl = season_length();
    year = turn_number / DAYS(sl * 4);

    static const std::string eternal = "ETERNAL_SEASON";
    if( get_option<bool>( eternal ) ) {
        // If we use calendar::start to determine the initial season, and the user shortens the season length
        // mid-game, the result could be the wrong season!
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

const std::string &calendar::name_season( season_type s )
{
    static const std::array<std::string, 5> season_names = {{
        std::string( _( "Spring" ) ),
        std::string( _( "Summer" ) ),
        std::string( _( "Autumn" ) ),
        std::string( _( "Winter" ) ),
        std::string( _( "End times" ) )
    }};
    if( s >= SPRING && s <= WINTER ) {
        return season_names[ s ];
    }

    return season_names[ 4 ];
}

