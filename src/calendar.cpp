#include "calendar.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>

#include "debug.h"
#include "display.h"
#include "line.h"
#include "options.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"

/** How much light moon provides per lit-up quarter (Full-moon light is four times this value) */
static constexpr float moonlight_per_quarter = 1.5f;

// Divided by 100 to prevent overflowing when converted to moves
const int calendar::INDEFINITELY_LONG( std::numeric_limits<int>::max() / 100 );
const time_duration calendar::INDEFINITELY_LONG_DURATION(
    time_duration::from_turns( std::numeric_limits<int>::max() ) );
static bool is_eternal_season = false;
static bool is_eternal_night = false;
static bool is_eternal_day = false;
static int cur_season_length = 1;
static lat_long location = { 42.36_degrees, -71.06_degrees };

time_point calendar::start_of_cataclysm = calendar::turn_zero;
time_point calendar::start_of_game = calendar::turn_zero;
time_point calendar::turn = calendar::turn_zero;
season_type calendar::initial_season = SPRING;

// The solar altitudes at which light changes in various ways
static constexpr units::angle astronomical_dawn = -18_degrees;
static constexpr units::angle nautical_dawn = -12_degrees;
static constexpr units::angle civil_dawn = -6_degrees;
static constexpr units::angle sunrise_angle = -1_degrees;

float default_daylight_level()
{
    return 100.0f;
}

float max_sun_irradiance()
{
    return 1000.f;
}

time_duration lunar_month()
{
    return 29.530588853 * 1_days;
}

namespace io
{
// *INDENT-OFF*
template<>
std::string enum_to_string<moon_phase>( moon_phase phase_num )
{
    switch( phase_num ) {
        case moon_phase::MOON_NEW: return "MOON_NEW";
        case moon_phase::MOON_WAXING_CRESCENT: return "MOON_WAXING_CRESCENT";
        case moon_phase::MOON_HALF_MOON_WAXING: return "MOON_HALF_MOON_WAXING";
        case moon_phase::MOON_WAXING_GIBBOUS: return "MOON_WAXING_GIBBOUS";
        case moon_phase::MOON_FULL: return "MOON_FULL";
        case moon_phase::MOON_WANING_CRESCENT: return "MOON_WANING_CRESCENT";
        case moon_phase::MOON_HALF_MOON_WANING: return "MOON_HALF_MOON_WANING";
        case moon_phase::MOON_WANING_GIBBOUS: return "MOON_WANING_GIBBOUS";
        case moon_phase::MOON_PHASE_MAX: break;
    }
    cata_fatal( "Invalid moon_phase %d", phase_num );
}
template<>
std::string enum_to_string<time_accuracy>( time_accuracy acc )
{
    switch( acc ) {
        case time_accuracy::PARTIAL: return "PARTIAL";
        case time_accuracy::FULL: return "FULL";
        case time_accuracy::NUM_TIME_ACCURACY:
        case time_accuracy::NONE: break;
        default:
            DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME )
                << "Invalid time_accuracy " << static_cast<int>( acc );
    }
    return "NONE";
}
// *INDENT-ON*
} // namespace io

moon_phase get_moon_phase( const time_point &p )
{
    const time_duration moon_phase_duration = calendar::season_from_default_ratio() * lunar_month();
    // Switch moon phase at noon so it stays the same all night
    const int num_middays = to_days<int>( p - calendar::turn_zero + 1_days / 2 );
    const time_duration nearest_midnight = num_middays * 1_days;
    const double phase_change = nearest_midnight / moon_phase_duration;
    const int current_phase = static_cast<int>( std::round( phase_change * MOON_PHASE_MAX ) ) %
                              static_cast<int>( MOON_PHASE_MAX );
    return static_cast<moon_phase>( current_phase );
}

static constexpr time_duration angle_to_time( const units::angle &a )
{
    return a / 15.0_degrees * 1_hours;
}

season_effective_time::season_effective_time( const time_point &t_ )
    : t( t_ )
{
    if( calendar::eternal_season() ) {
        const time_point start_midnight =
            calendar::start_of_game - time_past_midnight( calendar::start_of_game );
        t = start_midnight + time_past_midnight( t_ );
    }
}

static std::pair<units::angle, units::angle> sun_ra_declination(
    season_effective_time t, time_duration timezone )
{
    // This derivation is mostly from
    // https://en.wikipedia.org/wiki/Position_of_the_Sun
    // https://en.wikipedia.org/wiki/Celestial_coordinate_system#Notes_on_conversion

    // The computation is inspired by the derivation based on J2000 (Greenwich
    // noon, 2000-01-01), but because we want to be capable of handling a
    // different year length than the real Earth, we don't use the same exact
    // values.
    // Instead we use as our epoch a point that won't change arbitrarily with a
    // different year length - Greenwich midnight on the vernal equinox
    // (note that the vernal equinox happens to be Spring day 1 in the game
    // calendar, which is convenient).
    const double days_since_epoch = to_days<double>( t.t - calendar::turn_zero - timezone );

    // The angle per day the Earth moves around the Sun
    const units::angle angle_per_day = 360_degrees / to_days<int>( calendar::year_length() );

    // It turns out that we want mean longitude to be zero at the vernal
    // equinox, which simplifies the calculations.
    const units::angle mean_long = angle_per_day * days_since_epoch;
    // Roughly 77 degrees offset between mean longitude and mean anomaly at
    // J2000, so use that as our offset too.  The relative drift is slow, so we
    // neglect it.
    const units::angle mean_anomaly = 77_degrees + mean_long;
    // The two arbitrary constants in the calculation of ecliptic longitude
    // relate to the non-circularity of the Earth's orbit.
    const units::angle ecliptic_longitude =
        mean_long + 1.915_degrees * sin( mean_anomaly ) + 0.020_degrees * sin( 2 * mean_anomaly );

    // Obliquity does vary slightly, but for simplicity we'll keep it fixed at
    // its J2000 value.
    static constexpr units::angle obliquity = 23.439279_degrees;

    // ecliptic rectangular coordinates
    const rl_vec2d eclip( cos( ecliptic_longitude ), sin( ecliptic_longitude ) );
    // rotate to equatorial coordinates
    const rl_vec3d rot( eclip.x, eclip.y * cos( obliquity ), eclip.y * sin( obliquity ) );
    const units::angle right_ascension = atan2( rot.xy() );
    const units::angle declination = units::asin( rot.z );
    return { right_ascension, declination };
}

static units::angle sidereal_time_at( season_effective_time t, units::angle longitude,
                                      time_duration timezone )
{
    // Repeat some calculations from sun_ra_declination
    const double days_since_epoch = to_days<double>( t.t - calendar::turn_zero - timezone );
    const units::angle angle_per_day = 360_degrees / to_days<int>( calendar::year_length() );

    // Sidereal Time
    //
    // For the origin of sidereal time consider that at the epoch at Greenwich,
    // it's midnight on the vernal equinox so sidereal time should be 180°.
    // Timezone and longitude are both zero here, so L0 = 180°.
    const units::angle L0 = 180_degrees;
    // Sidereal time advances by 360° per day plus an additional 360° per year
    const units::angle L1 = 360_degrees + angle_per_day;
    return L0 + L1 * days_since_epoch + longitude;
}

std::pair<units::angle, units::angle> sun_azimuth_altitude(
    time_point ti )
{
    const season_effective_time t = season_effective_time( ti );
    units::angle right_ascension;
    units::angle declination;
    time_duration timezone = angle_to_time( location.longitude );
    std::tie( right_ascension, declination ) = sun_ra_declination( t, timezone );
    const units::angle sidereal_time = sidereal_time_at( t, location.longitude, timezone );

    const units::angle hour_angle = sidereal_time - right_ascension;

    // Use a two-step transformation to convert equatorial coordinates to
    // horizontal.
    // https://en.wikipedia.org/wiki/Celestial_coordinate_system#Equatorial_%E2%86%94_horizontal
    const rl_vec3d intermediate(
        cos( hour_angle ) * cos( declination ),
        sin( hour_angle ) * cos( declination ),
        sin( declination ) );

    const rl_vec3d horizontal(
        -intermediate.x * sin( location.latitude ) +
        intermediate.z * cos( location.latitude ),
        intermediate.y,
        intermediate.x * cos( location.latitude ) +
        intermediate.z * sin( location.latitude )
    );

    // Azimuth is from the South, turning positive to the west
    const units::angle azimuth = normalize( -atan2( horizontal.xy() ) + 180_degrees );
    units::angle altitude = units::asin( horizontal.z );

    if( calendar::eternal_day() ) {
        altitude = 90_degrees;
    }
    if( calendar::eternal_night() ) {
        altitude = astronomical_dawn - 10_degrees;
    }

    /*printf(
        "\n"
        "right_ascension = %f, declination = %f\n"
        "sidereal_time = %f, hour_angle = %f\n"
        "aziumth = %f, altitude = %f\n",
        to_degrees( right_ascension ), to_degrees( declination ),
        to_degrees( sidereal_time ), to_degrees( hour_angle ),
        to_degrees( azimuth ), to_degrees( altitude ) );*/

    return std::make_pair( azimuth, altitude );
}

static units::angle sun_altitude( time_point t )
{
    return sun_azimuth_altitude( t ).second;
}

std::optional<rl_vec2d> sunlight_angle( const time_point &t )
{
    units::angle azimuth;
    units::angle altitude;
    std::tie( azimuth, altitude ) = sun_azimuth_altitude( t );
    if( altitude <= sunrise_angle ) {
        // Sun below horizon
        return std::nullopt;
    }
    rl_vec2d horizontal_direction( -sin( azimuth ), cos( azimuth ) );
    rl_vec3d direction( horizontal_direction * cos( altitude ), sin( altitude ) );
    direction /= -direction.z;
    return direction.xy();
}

static time_point solar_noon_near( const time_point &t )
{
    const time_point prior_midnight = t - time_past_midnight( t );
    return prior_midnight + 12_hours;
    // If we were using a timezone rather than local solar time this would be
    // calculated as follows:
    //constexpr time_duration longitude_hours = angle_to_time( location_boston.longitude );
    //return prior_midnight + 12_hours - longitude_hours + timezone;
}

static units::angle offset_to_sun_altitude(
    const units::angle &altitude, const units::angle &longitude,
    const season_effective_time &approx_time, const bool evening )
{
    units::angle ra;
    units::angle declination;
    time_duration timezone = angle_to_time( longitude );
    std::tie( ra, declination ) = sun_ra_declination( approx_time, timezone );
    double cos_hour_angle =
        ( sin( altitude ) - sin( location.latitude ) * sin( declination ) ) /
        cos( location.latitude ) / cos( declination );
    if( std::abs( cos_hour_angle ) > 1 ) {
        // It doesn't actually reach that angle, so we pretend that it does at
        // its maximum possible angle
        cos_hour_angle = cos_hour_angle > 0 ? 1 : -1;
    }
    units::angle hour_angle = units::acos( cos_hour_angle );
    if( !evening ) {
        hour_angle = -hour_angle;
    }
    const units::angle target_sidereal_time = hour_angle + ra;
    const units::angle sidereal_time_at_approx_time =
        sidereal_time_at( approx_time, location.longitude, timezone );
    return normalize( target_sidereal_time - sidereal_time_at_approx_time );
}

static time_point sun_at_altitude( const units::angle &altitude, const units::angle &longitude,
                                   const time_point &t, const bool evening )
{
    const time_point solar_noon = solar_noon_near( t );
    units::angle initial_offset =
        offset_to_sun_altitude( altitude, longitude, season_effective_time( solar_noon ), evening );
    if( !evening ) {
        initial_offset -= 360_degrees;
    }
    const time_duration initial_offset_time = angle_to_time( initial_offset );
    const time_point initial_approximation = solar_noon + initial_offset_time;
    // Now we should have the correct time to within a few minutes; iterate to
    // get a more precise estimate
    units::angle correction_offset =
        offset_to_sun_altitude( altitude, longitude, season_effective_time( initial_approximation ),
                                evening );
    if( correction_offset > 180_degrees ) {
        correction_offset -= 360_degrees;
    }
    const time_duration correction_offset_time = angle_to_time( correction_offset );
    return initial_approximation + correction_offset_time;
}

time_point sunrise( const time_point &p )
{
    return sun_at_altitude( sunrise_angle, location.longitude, p, false );
}

time_point sunset( const time_point &p )
{
    return sun_at_altitude( sunrise_angle, location.longitude, p, true );
}

time_point night_time( const time_point &p )
{
    return sun_at_altitude( civil_dawn, location.longitude, p, true );
}

time_point daylight_time( const time_point &p )
{
    return sun_at_altitude( civil_dawn, location.longitude, p, false );
}

time_point noon( const time_point &p )
{
    const time_duration time_of_day = ( p - calendar::turn_zero ) % 1_days;
    const time_duration till_noon = time_of_day - 12_hours;
    return ( till_noon > 0_seconds ) ? p - till_noon : p + till_noon;
}

bool is_night( const time_point &p )
{
    return sun_altitude( p ) <= civil_dawn;
}

bool is_day( const time_point &p )
{
    return sun_altitude( p ) >= sunrise_angle;
}

static bool is_twilight( const time_point &p )
{
    units::angle altitude = sun_altitude( p );
    return altitude >= civil_dawn && altitude <= sunrise_angle;
}

bool is_dusk( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    return now > 12_hours && is_twilight( p );
}

bool is_dawn( const time_point &p )
{
    const time_duration now = time_past_midnight( p );
    return now < 12_hours && is_twilight( p );
}

static float moon_light_at( const time_point &p )
{
    int current_phase = static_cast<int>( get_moon_phase( p ) );
    if( current_phase > static_cast<int>( MOON_PHASE_MAX ) / 2 ) {
        current_phase = static_cast<int>( MOON_PHASE_MAX ) - current_phase;
    }

    return 1. + moonlight_per_quarter * current_phase;
}

float sun_light_at( const time_point &p )
{
    const units::angle solar_alt = sun_altitude( p );

    if( solar_alt < astronomical_dawn ) {
        return 0;
    } else if( solar_alt <= nautical_dawn ) {
        // Sunlight rises exponentially from 0 to 3.7f as sun rises from -18° to -12°
        return 3.7f * ( std::exp2( to_degrees( solar_alt - astronomical_dawn ) / 6.f ) - 1 );
    } else if( solar_alt <= civil_dawn ) {
        // Sunlight rises exponentially from 3.7f to 5.0f as sun rises from -12° to -6°
        return ( 5.0f - 3.7f ) * ( std::exp2( to_degrees( solar_alt - nautical_dawn ) / 6.f ) - 1 ) + 3.7f;
    } else if( solar_alt <= sunrise_angle ) {
        // Sunlight rises exponentially from 5.0f to 60 as sun rises from -6° to -1°
        return ( 60 - 5.0f ) * ( std::exp2( to_degrees( solar_alt - civil_dawn ) / 5.f ) - 1 ) + 5.0f;
    } else if( solar_alt <= 60_degrees ) {
        // Linear increase from -1° to 60° degrees light increases from 60 to 125 brightness.
        return ( 65.f / 61 ) * to_degrees( solar_alt ) + 65.f / 61  + 60;
    } else {
        return 125.f;
    }
}

float sun_irradiance( const time_point &p )
{
    const units::angle solar_alt = sun_altitude( p );

    if( solar_alt < 0_degrees ) {
        return 0;
    }
    return max_sun_irradiance() * sin( solar_alt );
}

float sun_moon_light_at( const time_point &p )
{
    return sun_light_at( p ) + moon_light_at( p );
}

float sun_moon_light_at_noon_near( const time_point &p )
{
    const time_point solar_noon = solar_noon_near( p );
    return sun_moon_light_at( solar_noon );
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
                    return string_format( n_gettext( "%d second", "%d seconds", num ), num );
                case clipped_unit::minute:
                    return string_format( n_gettext( "%d minute", "%d minutes", num ), num );
                case clipped_unit::hour:
                    return string_format( n_gettext( "%d hour", "%d hours", num ), num );
                case clipped_unit::day:
                    return string_format( n_gettext( "%d day", "%d days", num ), num );
                case clipped_unit::week:
                    return string_format( n_gettext( "%d week", "%d weeks", num ), num );
                case clipped_unit::season:
                    return string_format( n_gettext( "%d season", "%d seasons", num ), num );
                case clipped_unit::year:
                    return string_format( n_gettext( "%d year", "%d years", num ), num );
            }
        case clipped_align::right:
            switch( type ) {
                default:
                case clipped_unit::forever:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return _( "    forever" );
                case clipped_unit::second:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d  second", "%3d seconds", num ), num );
                case clipped_unit::minute:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d  minute", "%3d minutes", num ), num );
                case clipped_unit::hour:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d    hour", "%3d   hours", num ), num );
                case clipped_unit::day:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d     day", "%3d    days", num ), num );
                case clipped_unit::week:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d    week", "%3d   weeks", num ), num );
                case clipped_unit::season:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d  season", "%3d seasons", num ), num );
                case clipped_unit::year:
                    //~ Right-aligned time string. should right-align with other strings with this same comment
                    return string_format( n_gettext( "%3d    year", "%3d   years", num ), num );
            }
        case clipped_align::compact:
            switch( type ) {
                default:
                case clipped_unit::forever:
                    return _( "forever" );
                case clipped_unit::second:
                    return string_format( n_gettext( "%d sec", "%d secs", num ), num );
                case clipped_unit::minute:
                    return string_format( n_gettext( "%d min", "%d mins", num ), num );
                case clipped_unit::hour:
                    return string_format( n_gettext( "%d hr", "%d hrs", num ), num );
                case clipped_unit::day:
                    return string_format( n_gettext( "%d day", "%d days", num ), num );
                case clipped_unit::week:
                    return string_format( n_gettext( "%d wk", "%d wks", num ), num );
                case clipped_unit::season:
                    return string_format( n_gettext( "%d seas", "%d seas", num ), num );
                case clipped_unit::year:
                    return string_format( n_gettext( "%d yr", "%d yrs", num ), num );
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

std::string to_string( const time_duration &d, const bool compact )
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
    } else if( d < 1_weeks ) {
        divider = 1_days;
    } else if( d < calendar::season_length() || calendar::eternal_season() ) {
        divider = 1_weeks;
    } else if( d < calendar::year_length() ) {
        divider = calendar::season_length();
    } else {
        divider = calendar::year_length();
    }

    if( d % divider != 0_turns ) {
        if( compact ) {
            //~ %1$s - greater units of time (e.g. 3 hours), %2$s - lesser units of time (e.g. 11 minutes).
            return string_format( pgettext( "time duration", "%1$s %2$s" ),
                                  to_string_clipped( d, clipped_align::compact ),
                                  to_string_clipped( d % divider, clipped_align::compact ) );
        } else {
            //~ %1$s - greater units of time (e.g. 3 hours), %2$s - lesser units of time (e.g. 11 minutes).
            return string_format( _( "%1$s and %2$s" ),
                                  to_string_clipped( d ),
                                  to_string_clipped( d % divider ) );
        }
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

    // Minutes and seconds can be estimated precisely.
    if( d > 1_days ) {
        divider = 1_days;
        vicinity = 2_hours;
    } else if( d > 1_hours ) {
        divider = 1_hours;
        vicinity = 5_minutes;
    }

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

std::string to_string_writable( const time_duration &dur )
{
    if( dur % 1_days == 0_seconds ) {
        return string_format( "%d d", static_cast<int>( dur / 1_days ) );
    } else if( dur % 1_hours == 0_seconds ) {
        return string_format( "%d h", static_cast<int>( dur / 1_hours ) );
    } else if( dur % 1_minutes == 0_seconds ) {
        return string_format( "%d m", static_cast<int>( dur / 1_minutes ) );
    } else {
        return string_format( "%d s", static_cast<int>( dur / 1_seconds ) );
    }
}

std::string to_string_time_of_day( const time_point &p )
{
    const int hour = hour_of_day<int>( p );
    const int minute = minute_of_hour<int>( p );
    const int second = to_seconds<int>( time_past_midnight( p ) ) % 60;
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
     * <wito> Which would put the apocalypse on a saturday?
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

bool calendar::eternal_night()
{
    return is_eternal_night;
}

bool calendar::eternal_day()
{
    return is_eternal_day;
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
void calendar::set_eternal_night( bool is_eternal )
{
    is_eternal_night = is_eternal;
}
void calendar::set_eternal_day( bool is_eternal )
{
    is_eternal_day = is_eternal;
}
void calendar::set_season_length( const int dur )
{
    cur_season_length = dur;
}
void calendar::set_location( float latitude, float longitude )
{
    location = { units::from_degrees( latitude ), units::from_degrees( longitude ) };
}

static constexpr int real_world_season_length = 91;
static constexpr int default_season_length = real_world_season_length;

float calendar::season_ratio()
{
    return to_days<float>( season_length() ) / real_world_season_length;
}

float calendar::season_from_default_ratio()
{
    return to_days<float>( season_length() ) / default_season_length;
}

bool calendar::once_every( const time_duration &event_frequency )
{
    return ( calendar::turn - calendar::turn_zero ) % event_frequency == 0_turns;
}

std::string calendar::name_season( season_type s )
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
        const int day = day_of_season<int>( p ) + 1;
        //~ 1 is the year, 2 is the season name, 3 is the day (of the season), 4 is the time of the day in its usual format
        return string_format( _( "Year %1$d, %2$s, day %3$d %4$s" ), year,
                              calendar::name_season( season_of_year( p ) ), day, time );
    }
}

std::string get_diary_time_since_str( const time_duration &turn_diff, time_accuracy acc )
{
    const int days = to_days<int>( turn_diff );
    const int hours = to_hours<int>( turn_diff ) % 24;
    const int minutes = to_minutes<int>( turn_diff ) % 60;
    std::string days_text;
    std::string hours_text;
    std::string minutes_text;
    switch( acc ) {
        case time_accuracy::FULL:
            if( days > 0 ) {
                days_text = string_format( n_gettext( "%d day, ", "%d days, ", days ), days );
            }
            if( hours > 0 ) {
                hours_text = string_format( n_gettext( "%d hour, ", "%d hours, ", hours ), hours );
            }
            minutes_text = string_format( n_gettext( "%d minute", "%d minutes", minutes ), minutes );
            break;
        case time_accuracy::PARTIAL:
            if( days > 0 ) {
                days_text = string_format( n_gettext( "%d day", "%d days", days ), days );
            } else if( hours > 0 ) {
                //~ Estimate of how much time has passed since the last entry
                days_text = _( "Less than a day" );
            } else {
                //~ Estimate of how much time has passed since the last entry
                days_text = _( "Not long" );
            }
            break;
        default:
            DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME )
                    << "Unknown time_accuracy " << io::enum_to_string<time_accuracy>( acc );
        /* fallthrough */
        case time_accuracy::NUM_TIME_ACCURACY:
        case time_accuracy::NONE:
            //~ Estimate of how much time has passed since the last entry
            days_text = days > 0 ? _( "A long while" ) : hours > 0 ? _( "A while" ) : _( "A short while" );
            break;

    }
    //~ %1$s is xx days, %2$s is xx hours, %3$s is xx minutes
    return string_format( _( "%1$s%2$s%3$s since last entry" ), days_text, hours_text, minutes_text );
}

std::string get_diary_time_str( const time_point &turn, time_accuracy acc )
{
    const int year = to_turns<int>( turn - calendar::turn_zero ) /
                     to_turns<int>( calendar::year_length() ) + 1;
    const int day = day_of_season<int>( turn ) + 1;
    switch( acc ) {
        case time_accuracy::FULL:
            return to_string( turn );
        case time_accuracy::PARTIAL:
            // partial accuracy, able to see the sky
            //~ Time of year:
            //~ $1 = year since Cataclysm
            //~ $2 = season
            //~ $3 = day of season
            //~ $4 = approximate time of day
            return string_format( _( "Year %1$d, %2$s, day %3$d, %4$s" ), year,
                                  calendar::name_season( season_of_year( turn ) ),
                                  day, display::time_approx( turn ) );
        default:
            DebugLog( DebugLevel::D_WARNING, DebugClass::D_GAME )
                    << "Unknown time_accuracy " << io::enum_to_string<time_accuracy>( acc );
        /* fallthrough */
        case time_accuracy::NUM_TIME_ACCURACY:
        case time_accuracy::NONE: {
            // normalized to 100 day seasons
            const int day_norm = ( day * 100 ) / to_days<int>( calendar::season_length() );
            std::string seas_point;
            if( day_norm < 33 ) {
                //~ Estimated point in the current season
                seas_point = pgettext( "time of season", "Early" );
            } else if( day_norm < 66 ) {
                //~ Estimated point in the current season
                seas_point = pgettext( "time of season", "Mid" );
            } else {
                //~ Estimated point in the current season
                seas_point = pgettext( "time of season", "Late" );
            }
            //~ Estimated day-of-season string: $1 = Early/Mid/Late, $2 = Spring/Summer/Fall/Winter
            std::string season = string_format( _( "%1$s %2$s" ), seas_point,
                                                calendar::name_season( season_of_year( turn ) ) );
            //~ Time of year: $1 = year since Cataclysm, $2 = season
            return string_format( _( "Year %1$d, %2$s" ), year, season );
        }
    }
    return std::string();
}

time_point::time_point()
{
    turn_ = 0;
}
