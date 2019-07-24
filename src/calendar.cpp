#include "calendar.h"

#include <array>
#include <cmath>
#include <limits>
#include <algorithm>

#include "options.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

// Divided by 100 to prevent overflowing when converted to moves
const int calendar::INDEFINITELY_LONG( std::numeric_limits<int>::max() / 100 );
const time_duration calendar::INDEFINITELY_LONG_DURATION(
    time_duration::from_turns( std::numeric_limits<int>::max() ) );
bool calendar::is_eternal_season = false;
int calendar::cur_season_length = 1;

calendar calendar::start_of_cataclysm;
calendar calendar::turn;
season_type calendar::initial_season;

const time_point calendar::before_time_starts = time_point::from_turn( -1 );
const time_point calendar::turn_zero = time_point::from_turn( 0 );

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

/** Hour of sunset at summer solstice */
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

calendar::calendar( int Minute, int Hour, int Day, season_type Season, int Year )
{
    turn_number = MINUTES( Minute ) +
                  HOURS( Hour ) +
                  DAYS( Day ) +
                  Season * to_turns<int>( season_length() ) +
                  Year * to_turns<int>( year_length() );
    sync();
}

calendar::calendar( int turn )
{
    turn_number = turn;
    sync();
}

calendar::operator int() const
{
    return turn_number;
}

calendar &calendar::operator =( int rhs )
{
    turn_number = rhs;
    sync();
    return *this;
}

calendar &calendar::operator -=( const calendar &rhs )
{
    turn_number -= rhs.turn_number;
    sync();
    return *this;
}

calendar &calendar::operator -=( int rhs )
{
    turn_number -= rhs;
    sync();
    return *this;
}

calendar &calendar::operator +=( const calendar &rhs )
{
    turn_number += rhs.turn_number;
    sync();
    return *this;
}

calendar &calendar::operator +=( int rhs )
{
    turn_number += rhs;
    sync();
    return *this;
}

bool calendar::operator ==( int rhs ) const
{
    return static_cast<int>( *this ) == rhs;
}
bool calendar::operator ==( const calendar &rhs ) const
{
    return turn_number == rhs.turn_number;
}

calendar calendar::operator -( const calendar &rhs ) const
{
    return calendar( *this ) -= rhs;
}

calendar calendar::operator -( int rhs ) const
{
    return calendar( *this ) -= rhs;
}

calendar calendar::operator +( const calendar &rhs ) const
{
    return calendar( *this ) += rhs;
}

calendar calendar::operator +( int rhs ) const
{
    return calendar( *this ) += rhs;
}

void calendar::increment()
{
    turn_number++;
    sync();
}

moon_phase get_moon_phase( const time_point &p )
{
    //One full phase every 2 rl months = 2/3 season length
    const time_duration moon_phase_duration = calendar::season_length() * 2.0 / 3.0;
    //Switch moon phase at noon so it stays the same all night
    const time_duration current_day = ( p - calendar::turn_zero ) + 1_days / 2;
    const double phase_change = current_day / moon_phase_duration;
    const int current_phase = static_cast<int>( round( phase_change * MOON_PHASE_MAX ) ) %
                              static_cast<int>( MOON_PHASE_MAX );
    return static_cast<moon_phase>( current_phase );
}

time_point sunrise( const time_point &p )
{
    static_assert( static_cast<int>( SPRING ) == 0,
                   "Expected spring to be the first season. If not, code below will use wrong index into array" );

    static const std::array<int, 4> start_hours = { { SUNRISE_EQUINOX, SUNRISE_SUMMER, SUNRISE_EQUINOX, SUNRISE_WINTER, } };
    const size_t season = static_cast<size_t>( season_of_year( p ) );
    assert( season < start_hours.size() );

    const double start_hour = start_hours[season];
    const double end_hour = start_hours[( season + 1 ) % 4];

    const double into_month = static_cast<double>( day_of_season<int>( p ) ) / to_days<int>
                              ( calendar::season_length() );
    const double time = start_hour * ( 1.0 - into_month ) + end_hour * into_month;

    const time_point midnight = p - time_past_midnight( p );
    return midnight + time_duration::from_minutes( static_cast<int>( time * 60 ) );
}

time_point sunset( const time_point &p )
{
    static_assert( static_cast<int>( SPRING ) == 0,
                   "Expected spring to be the first season. If not, code below will use wrong index into array" );

    static const std::array<int, 4> start_hours = { { SUNSET_EQUINOX, SUNSET_SUMMER, SUNSET_EQUINOX, SUNSET_WINTER } };
    const size_t season = static_cast<size_t>( season_of_year( p ) );
    assert( season < start_hours.size() );

    const double start_hour = start_hours[season];
    const double end_hour = start_hours[( season + 1 ) % 4];

    const double into_month = static_cast<double>( day_of_season<int>( p ) ) / to_days<int>
                              ( calendar::season_length() );
    const double time = start_hour * ( 1.0 - into_month ) + end_hour * into_month;

    const time_point midnight = p - time_past_midnight( p );
    return midnight + time_duration::from_minutes( static_cast<int>( time * 60 ) );
}

bool is_night( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    const time_duration sunrise = time_past_midnight( ::sunrise( p ) );
    const time_duration sunset = time_past_midnight( ::sunset( p ) );

    return now > sunset + twilight_duration || now < sunrise;
}

bool is_sunset_now( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    const time_duration sunset = time_past_midnight( ::sunset( p ) );

    return now > sunset && now < sunset + twilight_duration;
}

bool is_sunrise_now( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    const time_duration sunrise = time_past_midnight( ::sunrise( p ) );

    return now > sunrise && now < sunrise + twilight_duration;
}

double calendar::current_daylight_level() const
{
    const double percent = static_cast<double>( static_cast<double>( day ) / to_days<int>
                           ( season_length() ) );
    double modifier = 1.0;
    // For ~Boston: solstices are +/- 25% sunlight intensity from equinoxes
    static double deviation = 0.25;

    switch( season ) {
        case SPRING:
            modifier = 1. + ( percent * deviation );
            break;
        case SUMMER:
            modifier = ( 1. + deviation ) - ( percent * deviation );
            break;
        case AUTUMN:
            modifier = 1. - ( percent * deviation );
            break;
        case WINTER:
            modifier = ( 1. - deviation ) + ( percent * deviation );
            break;
    }

    return double( modifier * DAYLIGHT_LEVEL );
}

float calendar::sunlight() const
{
    const time_duration now = time_past_midnight( *this );
    const time_duration sunrise = time_past_midnight( ::sunrise( *this ) );
    const time_duration sunset = time_past_midnight( ::sunset( *this ) );

    const double daylight_level = current_daylight_level();

    int current_phase = static_cast<int>( get_moon_phase( *this ) );
    if( current_phase > static_cast<int>( MOON_PHASE_MAX ) / 2 ) {
        current_phase = static_cast<int>( MOON_PHASE_MAX ) - current_phase;
    }

    const int moonlight = 1 + static_cast<int>( current_phase * MOONLIGHT_PER_QUARTER );

    if( now > sunset + twilight_duration || now < sunrise ) { // Night
        return moonlight;
    } else if( now >= sunrise && now <= sunrise + twilight_duration ) {
        const double percent = ( now - sunrise ) / twilight_duration;
        return static_cast<double>( moonlight ) * ( 1. - percent ) + daylight_level * percent;
    } else if( now >= sunset && now <= sunset + twilight_duration ) {
        const double percent = ( now - sunset ) / twilight_duration;
        return daylight_level * ( 1. - percent ) + static_cast<double>( moonlight ) * percent;
    } else {
        return daylight_level;
    }
}

static std::string to_string_clipped( const int num, const clipped_unit type,
                                      const clipped_align align )
{
    switch( align ) {
        default:
        case clipped_align::none:
            switch( type ) {
                default:
                case clipped_unit::forever:
                    return _( "forever" );
                case clipped_unit::second:
                    return string_format( ngettext( "%d second", "%d seconds", num ), num );
                case clipped_unit::minute:
                    return string_format( ngettext( "%d minute", "%d minutes", num ), num );
                case clipped_unit::hour:
                    return string_format( ngettext( "%d hour", "%d hours", num ), num );
                case clipped_unit::day:
                    return string_format( ngettext( "%d day", "%d days", num ), num );
                case clipped_unit::week:
                    return string_format( ngettext( "%d week", "%d weeks", num ), num );
                case clipped_unit::season:
                    return string_format( ngettext( "%d season", "%d seasons", num ), num );
                case clipped_unit::year:
                    return string_format( ngettext( "%d year", "%d years", num ), num );
            }
        case clipped_align::right:
            switch( type ) {
                default:
                case clipped_unit::forever:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return _( "    forever" );
                case clipped_unit::second:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d  second", "%3d seconds", num ), num );
                case clipped_unit::minute:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d  minute", "%3d minutes", num ), num );
                case clipped_unit::hour:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d    hour", "%3d   hours", num ), num );
                case clipped_unit::day:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d     day", "%3d    days", num ), num );
                case clipped_unit::week:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d    week", "%3d   weeks", num ), num );
                case clipped_unit::season:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d  season", "%3d seasons", num ), num );
                case clipped_unit::year:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( ngettext( "%3d    year", "%3d   years", num ), num );
            }
    }
}

std::pair<int, clipped_unit> clipped_time( const time_duration &d )
{
    if( d >= calendar::INDEFINITELY_LONG_DURATION ) {
        return { 0, clipped_unit::forever };
    }

    if( d < 1_minutes ) {
        const int sec = to_seconds<int>( d );
        return { sec, clipped_unit::second };
    } else if( d < 1_hours ) {
        const int min = to_minutes<int>( d );
        return { min, clipped_unit::minute };
    } else if( d < 1_days ) {
        const int hour = to_hours<int>( d );
        return { hour, clipped_unit::hour };
    } else if( d < 7_days ) {
        const int day = to_days<int>( d );
        return { day, clipped_unit::day };
    } else if( d < calendar::season_length() || calendar::eternal_season() ) {
        // eternal seasons means one season is indistinguishable from the next,
        // therefore no way to count them
        const int week = to_weeks<int>( d );
        return { week, clipped_unit::week };
    } else if( d < calendar::year_length() && !calendar::eternal_season() ) {
        // TODO: consider a to_season function, but season length is variable, so
        // this might be misleading
        const int season = to_turns<int>( d ) / to_turns<int>( calendar::season_length() );
        return { season, clipped_unit::season };
    } else {
        // TODO: consider a to_year function, but year length is variable, so
        // this might be misleading
        const int year = to_turns<int>( d ) / to_turns<int>( calendar::year_length() );
        return { year, clipped_unit::year };
    }
}

std::string to_string_clipped( const time_duration &d,
                               const clipped_align align )
{
    const std::pair<int, clipped_unit> time = clipped_time( d );
    return to_string_clipped( time.first, time.second, align );
}

std::string to_string( const time_duration &d )
{
    if( d >= calendar::INDEFINITELY_LONG_DURATION ) {
        return _( "forever" );
    }

    if( d <= 1_minutes ) {
        return to_string_clipped( d );
    }

    time_duration divider = 0_turns;
    if( d < 1_hours ) {
        divider = 1_minutes;
    } else if( d < 1_days ) {
        divider = 1_hours;
    } else {
        divider = 24_hours;
    }

    if( d % divider != 0_turns ) {
        //~ %1$s - greater units of time (e.g. 3 hours), %2$s - lesser units of time (e.g. 11 minutes).
        return string_format( _( "%1$s and %2$s" ),
                              to_string_clipped( d ),
                              to_string_clipped( d % divider ) );
    }
    return to_string_clipped( d );
}

std::string to_string_approx( const time_duration &dur, const bool verbose )
{
    time_duration d = dur;
    const auto make_result = [verbose]( const time_duration & d, const char *verbose_str,
    const char *short_str ) {
        return string_format( verbose ? verbose_str : short_str, to_string_clipped( d ) );
    };

    time_duration divider = 0_turns;
    time_duration vicinity = 0_turns;

    if( d > 1_days ) {
        divider = 1_days;
        vicinity = 2_hours;
    } else if( d > 1_hours ) {
        divider = 1_hours;
        vicinity = 5_minutes;
    } // Minutes and seconds can be estimated precisely.

    if( divider != 0_turns ) {
        const time_duration remainder = d % divider;

        if( remainder >= divider - vicinity ) {
            d += divider;
        } else if( remainder > vicinity ) {
            if( remainder < divider / 2 ) {
                //~ %s - time (e.g. 2 hours).
                return make_result( d, _( "more than %s" ), ">%s" );
            } else {
                //~ %s - time (e.g. 2 hours).
                return make_result( d + divider, _( "less than %s" ), "<%s" );
            }
        }
    }
    //~ %s - time (e.g. 2 hours).
    return make_result( d, _( "about %s" ), "%s" );
}

std::string to_string_time_of_day( const time_point &p )
{
    const int hour = hour_of_day<int>( p );
    const int minute = minute_of_hour<int>( p );
    const int second = ( to_seconds<int>( time_past_midnight( p ) ) ) % 60;
    const std::string format_type = get_option<std::string>( "24_HOUR" );

    if( format_type == "military" ) {
        return string_format( "%02d%02d.%02d", hour, minute, second );
    } else if( format_type == "24h" ) {
        //~ hour:minute (24hr time display)
        return string_format( _( "%02d:%02d:%02d" ), hour, minute, second );
    } else {
        int hour_param = hour % 12;
        if( hour_param == 0 ) {
            hour_param = 12;
        }
        // Padding is removed as necessary to prevent clipping with SAFE notification in wide sidebar mode
        const std::string padding = hour_param < 10 ? " " : "";
        if( hour < 12 ) {
            return string_format( _( "%d:%02d:%02d%sAM" ), hour_param, minute, second, padding );
        } else {
            return string_format( _( "%d:%02d:%02d%sPM" ), hour_param, minute, second, padding );
        }
    }
}

weekdays day_of_week( const time_point &p )
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
    const int day_since_cataclysm = to_days<int>( p - calendar::turn_zero );
    static const weekdays start_day = weekdays::THURSDAY;
    const int result = day_since_cataclysm + static_cast<int>( start_day );
    return static_cast<weekdays>( result % 7 );
}

bool calendar::eternal_season()
{
    return is_eternal_season;
}

time_duration calendar::year_length()
{
    return season_length() * 4;
}

time_duration calendar::season_length()
{
    return time_duration::from_days( std::max( cur_season_length, 1 ) );
}
void calendar::set_eternal_season( bool is_eternal )
{
    is_eternal_season = is_eternal;
}
void calendar::set_season_length( const int dur )
{
    cur_season_length = dur;
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

int calendar::day_of_year() const
{
    return day + to_days<int>( season_length() ) * season;
}

void calendar::sync()
{
    const int sl = to_days<int>( season_length() );
    year = turn_number / DAYS( sl * 4 );

    if( eternal_season() ) {
        // If we use calendar::start to determine the initial season, and the user shortens the season length
        // mid-game, the result could be the wrong season!
        season = initial_season;
    } else {
        season = static_cast<season_type>( turn_number / DAYS( sl ) % 4 );
    }

    day = turn_number / DAYS( 1 ) % sl;
    hour = turn_number / HOURS( 1 ) % 24;
    minute = turn_number / MINUTES( 1 ) % 60;
    second = turn_number % 60;
}

bool calendar::once_every( const time_duration &event_frequency )
{
    return ( calendar::turn % to_turns<int>( event_frequency ) ) == 0;
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
        }
    };
    if( s >= SPRING && s <= WINTER ) {
        return _( season_names_untranslated[ s ] );
    }

    return _( season_names_untranslated[ 4 ] );
}

time_duration rng( time_duration lo, time_duration hi )
{
    return time_duration( rng( lo.turns_, hi.turns_ ) );
}

bool x_in_y( const time_duration &a, const time_duration &b )
{
    return ::x_in_y( to_turns<int>( a ), to_turns<int>( b ) );
}

const std::vector<std::pair<std::string, time_duration>> time_duration::units = { {
        { "turns", 1_turns },
        { "turn", 1_turns },
        { "t", 1_turns },
        { "seconds", 1_seconds },
        { "second", 1_seconds },
        { "s", 1_seconds },
        { "minutes", 1_minutes },
        { "minute", 1_minutes },
        { "m", 1_minutes },
        { "hours", 1_hours },
        { "hour", 1_hours },
        { "h", 1_hours },
        { "days", 1_days },
        { "day", 1_days },
        { "d", 1_days },
        // TODO: maybe add seasons?
        // TODO: maybe add years? Those two things depend on season length!
    }
};

season_type season_of_year( const time_point &p )
{
    static time_point prev_turn = calendar::before_time_starts;
    static season_type prev_season = calendar::initial_season;

    if( p != prev_turn ) {
        prev_turn = p;
        if( calendar::eternal_season() ) {
            // If we use calendar::start to determine the initial season, and the user shortens the season length
            // mid-game, the result could be the wrong season!
            return prev_season = calendar::initial_season;
        }
        return prev_season = static_cast<season_type>(
                                 to_turn<int>( p ) / to_turns<int>( calendar::season_length() ) % 4
                             );
    }

    return prev_season;
}

std::string to_string( const time_point &p )
{
    const int year = to_turns<int>( p - calendar::turn_zero ) / to_turns<int>
                     ( calendar::year_length() ) + 1;
    const std::string time = to_string_time_of_day( p );
    if( calendar::eternal_season() ) {
        const int day = to_days<int>( time_past_new_year( p ) );
        //~ 1 is the year, 2 is the day (of the *year*), 3 is the time of the day in its usual format
        return string_format( _( "Year %1$d, day %2$d %3$s" ), year, day, time );
    } else {
        const int day = day_of_season<int>( p );
        //~ 1 is the year, 2 is the season name, 3 is the day (of the season), 4 is the time of the day in its usual format
        return string_format( _( "Year %1$d, %2$s, day %3$d %4$s" ), year,
                              calendar::name_season( season_of_year( p ) ), day, time );
    }
}

time_point::time_point()
{
    turn_ = 0;
}
