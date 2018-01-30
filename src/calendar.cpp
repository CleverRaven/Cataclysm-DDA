#include "calendar.h"
#include <cmath>
#include <sstream>
#include <limits>
#include <array>

#include "output.h"
#include "options.h"
#include "translations.h"
#include "string_formatter.h"
#include "debug.h"
#include "rng.h"

// Divided by 100 to prevent overflowing when converted to moves
const int calendar::INDEFINITELY_LONG( std::numeric_limits<int>::max() / 100 );

calendar calendar::start;
calendar calendar::turn;
season_type calendar::initial_season;

const time_point calendar::before_time_starts = time_point::from_turn( -1 );
const time_point calendar::time_of_cataclysm = time_point::from_turn( 0 );

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

// How long, does sunrise/sunset last?
static const time_duration twilight_duration = 1_hours;

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
    turn_number = MINUTES(Minute) + HOURS(Hour) + DAYS(Day) + Season * to_days<int>( season_length() ) + Year * to_turns<int>( year_length() );
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

moon_phase calendar::moon() const
{
    //One full phase every 2 rl months = 2/3 season length
    float phase_change_per_day = 1.0 / ((to_days<float>( season_length() ) * 2.0 / 3.0) / float(MOON_PHASE_MAX));

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
    double percent = double(double(day) / to_days<int>( season_length() ));
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
    double percent = double(double(day) / to_days<int>( season_length() ));
    double time = double(start_hour) * (1. - percent) + double(end_hour) * percent;

    newhour = int(time);
    time -= int(time);
    newminute = int(time * 60);

    return calendar (newminute, newhour, day, season, year);
}

bool calendar::is_night() const
{
    const time_duration now = time_past_midnight( *this );
    const time_duration sunrise = time_past_midnight( this->sunrise() );
    const time_duration sunset = time_past_midnight( this->sunset() );

    return now > sunset + twilight_duration || now < sunrise;
}

double calendar::current_daylight_level() const
{
    double percent = double(double(day) / to_days<int>( season_length() ));
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
    const time_duration now = time_past_midnight( *this );
    const time_duration sunrise = time_past_midnight( this->sunrise() );
    const time_duration sunset = time_past_midnight( this->sunset() );

    double daylight_level = current_daylight_level();

    int current_phase = int(moon());
    if ( current_phase > int(MOON_PHASE_MAX)/2 ) {
        current_phase = int(MOON_PHASE_MAX) - current_phase;
    }

    int moonlight = 1 + int(current_phase * MOONLIGHT_PER_QUARTER);

    if( now > sunset + twilight_duration || now < sunrise ) { // Night
        return moonlight;
    } else if( now >= sunrise && now <= sunrise + twilight_duration ) {
        const double percent = ( now - sunrise ) / twilight_duration;
        return double(moonlight) * (1. - percent) + daylight_level * percent;
    } else if( now >= sunset && now <= sunset + twilight_duration ) {
        const double percent = ( now - sunset ) / twilight_duration;
        return daylight_level * (1. - percent) + double(moonlight) * percent;
    } else {
        return daylight_level;
    }
}

std::string to_string_clipped( const time_duration &d )
{
    //@todo change INDEFINITELY_LONG to time_duration
    if( to_turns<int>( d ) >= calendar::INDEFINITELY_LONG ) {
        return _( "forever" );
    }

    if( d < 1_minutes ) {
        //@todo add to_seconds,from_seconds, operator ""_seconds, but currently
        // this could be misleading as we only store turns, which are 6 whole seconds
        const int sec = to_turns<int>( d ) * 6;
        return string_format( ngettext( "%d second", "%d seconds", sec ), sec );
    } else if( d < 1_hours ) {
        const int min = to_minutes<int>( d );
        return string_format( ngettext( "%d minute", "%d minutes", min ), min );
    } else if( d < 1_days ) {
        const int hour = to_hours<int>( d );
        return string_format( ngettext( "%d hour", "%d hours", hour ), hour );
    } else if( d < calendar::season_length() || calendar::eternal_season() ) {
        // eternal seasons means one season is indistinguishable from the next,
        // therefor no way to count them
        const int day = to_days<int>( d );
        return string_format( ngettext( "%d day", "%d days", day ), day );
    } else if( d < calendar::year_length() && !calendar::eternal_season() ) {
        //@todo consider a to_season function, but season length is variable, so
        // this might be misleading
        const int season = to_turns<int>( d ) / to_turns<int>( calendar::season_length() );
        return string_format( ngettext( "%d season", "%d seasons", season ), season );
    } else {
        //@todo consider a to_year function, but year length is variable, so
        // this might be misleading
        const int year = to_turns<int>( d ) / to_turns<int>( calendar::year_length() );
        return string_format( ngettext( "%d year", "%d years", year ), year );
    }
}

std::string to_string( const time_duration &d )
{
    const int turns = to_turns<int>( d );
    int divider = 0;

    if( turns > MINUTES( 1 ) && turns < calendar::INDEFINITELY_LONG ) {
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
                              to_string_clipped( time_duration::from_turns( turns ) ),
                              to_string_clipped( time_duration::from_turns( remainder ) ) );
    }

    return to_string_clipped( d );
}

std::string to_string_approx( const time_duration &d, const bool verbose )
{
    int turns = to_turns<int>( d );
    const auto make_result = [verbose]( int turns, const char *verbose_str, const char *short_str ) {
        return string_format( verbose ? verbose_str : short_str, to_string_clipped( time_duration::from_turns( turns ) ) );
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
    static const std::array<std::string, 7> weekday_names = {{
        translate_marker( "Sunday" ), translate_marker( "Monday" )
        translate_marker( "Tuesday" ), translate_marker( "Wednesday" )
        translate_marker( "Thursday" ), translate_marker( "Friday" )
        translate_marker( "Saturday" )
    }};

    // calendar::day gets mangled by season transitions, so recalculate days since start.
    static const int start_day = 4; // Thursday is the start day
    const int current_day = ( turn_number / DAYS(1) + start_day ) % 7;
    return _( weekday_names[ current_day ].c_str() );
}

bool calendar::eternal_season()
{
    static const std::string eternal_season_option_name = "ETERNAL_SEASON";
    return get_option<bool>( eternal_season_option_name );
}

time_duration calendar::year_length()
{
    return season_length() * 4;
}

time_duration calendar::season_length()
{
    static const std::string s = "SEASON_LENGTH";
    // Avoid returning 0 as this value is used in division and expected to be non-zero.
    return time_duration::from_days( std::max( get_option<int>( s ), 1 ) );
}

float calendar::season_ratio()
{
    static const int real_world_season_length = 91;
    return to_days<float>( season_length() ) / real_world_season_length;
}

float calendar::season_from_default_ratio()
{
    static const int default_season_length = 14;
    return to_days<float>( season_length() ) / default_season_length;
}

int calendar::turn_of_year() const
{
    const int season_turns = to_turns<int>( season_length() );
    return (season * season_turns) + (turn_number % season_turns);
}

int calendar::day_of_year() const
{
    return day + to_days<int>( season_length() ) * season;
}

void calendar::sync()
{
    const int sl = to_days<int>( season_length() );
    year = turn_number / DAYS(sl * 4);

    if( eternal_season() ) {
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

bool calendar::once_every( const time_duration &event_frequency )
{
    return ( calendar::turn.get_turn() % to_turns<int>( event_frequency ) ) == 0;
}

const std::string calendar::name_season( season_type s )
{
    static const std::array<std::string, 5> season_names_untranslated = {{
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Spring" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Summer" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Autumn" ) ),
        //~First letter is supposed to be uppercase
        std::string( translate_marker( "Winter" ) ),
        std::string( translate_marker( "End times" ) )
    }};
    if( s >= SPRING && s <= WINTER ) {
        return _( season_names_untranslated[ s ].c_str() );
    }

    return _( season_names_untranslated[ 4 ].c_str() );
}

time_duration rng( time_duration lo, time_duration hi )
{
    return time_duration( rng( lo.turns_, hi.turns_ ) );
}

season_type season_of_year( const time_point &p )
{
    if( calendar::eternal_season() ) {
        // If we use calendar::start to determine the initial season, and the user shortens the season length
        // mid-game, the result could be the wrong season!
        return calendar::initial_season;
    }
    const int season = to_turn<int>( p ) / to_turns<int>( calendar::season_length() );
    return static_cast<season_type>( season % 4 );
}
