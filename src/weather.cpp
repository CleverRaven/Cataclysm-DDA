#include "weather.h"

#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <list>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "game_constants.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "options.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "weather_gen.h"
#include "bodypart.h"
#include "enums.h"
#include "item.h"
#include "math_defines.h"
#include "rng.h"
#include "string_id.h"
#include "units.h"
#include "colony.h"
#include "player_activity.h"
#include "regional_settings.h"

const efftype_id effect_glare( "glare" );
const efftype_id effect_snow_glare( "snow_glare" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_sleep( "sleep" );

static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_FEATHERS( "FEATHERS" );

/**
 * \defgroup Weather "Weather and its implications."
 * @{
 */

static bool is_player_outside()
{
    return g->m.is_outside( point( g->u.posx(), g->u.posy() ) ) && g->get_levz() >= 0;
}

#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

/**
 * Glare.
 * Causes glare effect to player's eyes if they are not wearing applicable eye protection.
 * @param intensity Level of sun brighthess
 */
void weather_effect::glare( sun_intensity intensity )
{
    //General prepequisites for glare
    if( !is_player_outside() || !g->is_in_sunlight( g->u.pos() ) || g->u.in_sleep_state() ||
        g->u.worn_with_flag( "SUN_GLASSES" ) ||
        g->u.has_bionic( bionic_id( "bio_sunglasses" ) ) ||
        g->u.is_blind() ) {
        return;
    }

    time_duration dur = 0_turns;
    const efftype_id *effect = nullptr;
    season_type season = season_of_year( calendar::turn );
    if( season == WINTER ) {
        //Winter snow glare: for both clear & sunny weather
        effect = &effect_snow_glare;
        dur = g->u.has_effect( *effect ) ? 1_turns : 2_turns;
    } else if( intensity > 1 ) {
        //Sun glare: only for bright sunny weather
        effect = &effect_glare;
        dur = g->u.has_effect( *effect ) ? 1_turns : 2_turns;
    }
    //apply final glare effect
    if( dur > 0_turns && effect != nullptr ) {
        //enhance/reduce by some traits
        if( g->u.has_trait( trait_CEPH_VISION ) ) {
            dur = dur * 2;
        }
        g->u.add_env_effect( *effect, bp_eyes, 2, dur );
    }
}

////// food vs weather

inline void proc_weather_sum( const weather_type wtype, weather_sum &data,
                              const time_point &t, const time_duration &tick_size )
{
    switch( wtype ) {
        case WEATHER_DRIZZLE:
            data.rain_amount += 4 * to_turns<int>( tick_size );
            break;
        case WEATHER_RAINY:
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            data.rain_amount += 8 * to_turns<int>( tick_size );
            break;
        case WEATHER_ACID_DRIZZLE:
            data.acid_amount += 4 * to_turns<int>( tick_size );
            break;
        case WEATHER_ACID_RAIN:
            data.acid_amount += 8 * to_turns<int>( tick_size );
            break;
        default:
            break;
    }

    // TODO: Change this sunlight "sampling" here into a proper interpolation
    const float tick_sunlight = sunlight( t, false ) + weather::light_modifier( wtype );
    data.sunlight += std::max<float>( 0.0f, to_turns<int>( tick_size ) * tick_sunlight );
}

////// Funnels.
weather_sum sum_conditions( const time_point &start, const time_point &end,
                            const tripoint &location )
{
    time_duration tick_size = 0_turns;
    weather_sum data;

    const auto wgen = g->weather.get_cur_weather_gen();
    for( time_point t = start; t < end; t += tick_size ) {
        const time_duration diff = end - t;
        if( diff < 10_turns ) {
            tick_size = 1_turns;
        } else if( diff > 7_days ) {
            tick_size = 1_hours;
        } else {
            tick_size = 1_minutes;
        }

        weather_type wtype;
        if( g->weather.weather_override == WEATHER_NULL ) {
            wtype = wgen.get_weather_conditions( location, t, g->get_seed() );
        } else {
            wtype = g->weather.weather_override;
        }
        proc_weather_sum( wtype, data, t, tick_size );
        w_point w = wgen.get_weather( location, t, g->get_seed() );
        data.wind_amount += get_local_windpower( g->weather.windspeed, overmap_buffer.ter( location ),
                            location,
                            g->weather.winddirection, false ) * to_turns<int>( tick_size );
    }
    return data;
}

/**
 * Determine what a funnel has filled out of game, using funnelcontainer.bday as a starting point.
 */
void retroactively_fill_from_funnel( item &it, const trap &tr, const time_point &start,
                                     const time_point &end, const tripoint &pos )
{
    if( start > end || !tr.is_funnel() ) {
        return;
    }

    // bday == last fill check
    it.set_birthday( end );
    auto data = sum_conditions( start, end, pos );

    // Technically 0.0 division is OK, but it will be cleaner without it
    if( data.rain_amount > 0 ) {
        const int rain = roll_remainder( 1.0 / tr.funnel_turns_per_charge( data.rain_amount ) );
        it.add_rain_to_container( false, rain );
        // add_msg(m_debug, "Retroactively adding %d water from turn %d to %d", rain, startturn, endturn);
    }

    if( data.acid_amount > 0 ) {
        const int acid = roll_remainder( 1.0 / tr.funnel_turns_per_charge( data.acid_amount ) );
        it.add_rain_to_container( true, acid );
    }
}

/**
 * Add charge(s) of rain to given container, possibly contaminating it.
 */
void item::add_rain_to_container( bool acid, int charges )
{
    if( charges <= 0 ) {
        return;
    }
    item ret( acid ? "water_acid" : "water", calendar::turn );
    const int capa = get_remaining_capacity_for_liquid( ret, true );
    if( contents.empty() ) {
        // This is easy. Just add 1 charge of the rain liquid to the container.
        if( !acid ) {
            // Funnels aren't always clean enough for water. // TODO: disinfectant squeegie->funnel
            ret.poison = one_in( 10 ) ? 1 : 0;
        }
        ret.charges = std::min( charges, capa );
        put_in( ret );
    } else {
        // The container already has a liquid.
        item &liq = contents.front();
        int orig = liq.charges;
        int added = std::min( charges, capa );
        if( capa > 0 ) {
            liq.charges += added;
        }

        if( liq.typeId() == ret.typeId() || liq.typeId() == "water_acid_weak" ) {
            // The container already contains this liquid or weakly acidic water.
            // Don't do anything special -- we already added liquid.
        } else {
            // The rain is different from what's in the container.
            // Turn the container's liquid into weak acid with a probability
            // based on its current volume.

            // If it's raining acid and this container started with 7
            // charges of water, the liquid will now be 1/8th acid or,
            // equivalently, 1/4th weak acid (the rest being water). A
            // stochastic approach gives the liquid a 1 in 4 (or 2 in
            // liquid.charges) chance of becoming weak acid.
            const bool transmute = x_in_y( 2 * added, liq.charges );

            if( transmute ) {
                contents.front() = item( "water_acid_weak", calendar::turn, liq.charges );
            } else if( liq.typeId() == "water" ) {
                // The container has water, and the acid rain didn't turn it
                // into weak acid. Poison the water instead, assuming 1
                // charge of acid would act like a charge of water with poison 5.
                int total_poison = liq.poison * orig + 5 * added;
                liq.poison = total_poison / liq.charges;
                int leftover_poison = total_poison - liq.poison * liq.charges;
                if( leftover_poison > rng( 0, liq.charges ) ) {
                    liq.poison++;
                }
            }
        }
    }
}

double funnel_charges_per_turn( const double surface_area_mm2, const double rain_depth_mm_per_hour )
{
    // 1mm rain on 1m^2 == 1 liter water == 1000ml
    // 1 liter == 4 volume
    // 1 volume == 250ml: containers
    // 1 volume == 200ml: water
    // How many charges of water can we collect in a turn (usually <1.0)?
    if( rain_depth_mm_per_hour == 0.0 ) {
        return 0.0;
    }

    // Calculate once, because that part is expensive
    static const item water( "water", 0 );
    // 250ml
    static const double charge_ml = static_cast<double>( to_gram( water.weight() ) ) /
                                    water.charges;

    const double vol_mm3_per_hour = surface_area_mm2 * rain_depth_mm_per_hour;
    const double vol_mm3_per_turn = vol_mm3_per_hour / to_turns<int>( 1_hours );

    const double ml_to_mm3 = 1000;
    const double charges_per_turn = vol_mm3_per_turn / ( charge_ml * ml_to_mm3 );

    return charges_per_turn;
}

double trap::funnel_turns_per_charge( double rain_depth_mm_per_hour ) const
{
    // 1mm rain on 1m^2 == 1 liter water == 1000ml
    // 1 liter == 4 volume
    // 1 volume == 250ml: containers
    // 1 volume == 200ml: water
    // How many turns should it take for us to collect 1 charge of rainwater?
    // "..."
    if( rain_depth_mm_per_hour == 0.0 ) {
        return 0.0;
    }

    const double surface_area_mm2 = M_PI * ( funnel_radius_mm * funnel_radius_mm );
    const double charges_per_turn = funnel_charges_per_turn( surface_area_mm2, rain_depth_mm_per_hour );

    if( charges_per_turn > 0.0 ) {
        return 1.0 / charges_per_turn;
    }

    return 0.0;
}

/**
 * Main routine for filling funnels from weather effects.
 */
static void fill_funnels( int rain_depth_mm_per_hour, bool acid, const trap &tr )
{
    const double turns_per_charge = tr.funnel_turns_per_charge( rain_depth_mm_per_hour );
    // Give each funnel on the map a chance to collect the rain.
    const auto &funnel_locs = g->m.trap_locations( tr.loadid );
    for( auto loc : funnel_locs ) {
        units::volume maxcontains = 0_ml;
        if( one_in( turns_per_charge ) ) { // FIXME:
            //add_msg("%d mm/h %d tps %.4f: fill",int(calendar::turn),rain_depth_mm_per_hour,turns_per_charge);
            // This funnel has collected some rain! Put the rain in the largest
            // container here which is either empty or contains some mixture of
            // impure water and acid.
            auto items = g->m.i_at( loc );
            auto container = items.end();
            for( auto candidate_container = items.begin(); candidate_container != items.end();
                 ++candidate_container ) {
                if( candidate_container->is_funnel_container( maxcontains ) ) {
                    container = candidate_container;
                }
            }

            if( container != items.end() ) {
                container->add_rain_to_container( acid, 1 );
                container->set_age( 0_turns );
            }
        }
    }
}

/**
 * Fill funnels and makeshift funnels from weather effects.
 * @see fill_funnels
 */
static void fill_water_collectors( int mmPerHour, bool acid )
{
    for( auto &e : trap::get_funnels() ) {
        fill_funnels( mmPerHour, acid, *e );
    }
}

/**
 * Main routine for wet effects caused by weather.
 * Drenching the player is applied after checks against worn and held items.
 *
 * The warmth of armor is considered when determining how much drench happens per tick.
 *
 * Note that this is not the only place where drenching can happen.
 * For example, moving or swimming into water tiles will also cause drenching.
 * @see fill_water_collectors
 * @see map::decay_fields_and_scent
 * @see player::drench
 */
static void wet_player( int amount )
{
    if( !is_player_outside() ||
        g->u.has_trait( trait_FEATHERS ) ||
        g->u.weapon.has_flag( "RAIN_PROTECT" ) ||
        ( !one_in( 50 ) && g->u.worn_with_flag( "RAINPROOF" ) ) ) {
        return;
    }

    if( rng( 0, 100 - amount + g->u.warmth( bp_torso ) * 4 / 5 + g->u.warmth( bp_head ) / 5 ) > 10 ) {
        // Thick clothing slows down (but doesn't cap) soaking
        return;
    }

    const auto &wet = g->u.body_wetness;
    const auto &capacity = g->u.drench_capacity;
    body_part_set drenched_parts{ { bp_torso, bp_arm_l, bp_arm_r, bp_head } };
    if( wet[bp_torso] * 100 >= capacity[bp_torso] * 50 ) {
        // Once upper body is 50%+ drenched, start soaking the legs too
        drenched_parts |= { { bp_leg_l, bp_leg_r } };
    }

    g->u.drench( amount, drenched_parts, false );
}

/**
 * Main routine for wet effects caused by weather.
 */
static void generic_wet( bool acid )
{
    fill_water_collectors( 4, acid );
    g->m.decay_fields_and_scent( 15_turns );
    wet_player( 30 );
}

/**
 * Main routine for very wet effects caused by weather.
 * Similar to generic_wet() but with more aggressive numbers.
 */
static void generic_very_wet( bool acid )
{
    fill_water_collectors( 8, acid );
    g->m.decay_fields_and_scent( 45_turns );
    wet_player( 60 );
}

void weather_effect::none()
{
    glare( sun_intensity::normal );
}
void weather_effect::flurry()    {}

void weather_effect::sunny()
{
    glare( sun_intensity::high );
}

/**
 * Wet.
 * @see generic_wet
 */
void weather_effect::wet()
{
    generic_wet( false );
}

/**
 * Very wet.
 * @see generic_very_wet
 */
void weather_effect::very_wet()
{
    generic_very_wet( false );
}

void weather_effect::snow()
{
    wet_player( 10 );
}

void weather_effect::snowstorm()
{
    wet_player( 40 );
}
/**
 * Thunder.
 * Flavor messages. Very wet.
 */
void weather_effect::thunder()
{
    very_wet();
    if( !g->u.has_effect( effect_sleep ) && !g->u.is_deaf() && one_in( THUNDER_CHANCE ) ) {
        if( g->get_levz() >= 0 ) {
            add_msg( _( "You hear a distant rumble of thunder." ) );
            sfx::play_variant_sound( "environment", "thunder_far", 80, rng( 0, 359 ) );
        } else if( one_in( std::max( roll_remainder( 2.0f * g->get_levz() /
                                     g->u.mutation_value( "hearing_modifier" ) ), 1 ) ) ) {
            add_msg( _( "You hear a rumble of thunder from above." ) );
            sfx::play_variant_sound( "environment", "thunder_far",
                                     ( 80 * g->u.mutation_value( "hearing_modifier" ) ), rng( 0, 359 ) );
        }
    }
}

/**
 * Lightning.
 * Chance of lightning illumination for the current turn when aboveground. Thunder.
 *
 * This used to manifest actual lightning on the map, causing fires and such, but since such effects
 * only manifest properly near the player due to the "reality bubble", this was causing undesired metagame tactics
 * such as players leaving their shelter for a more "expendable" area during lightning storms.
 */
void weather_effect::lightning()
{
    thunder();
    if( one_in( LIGHTNING_CHANCE ) ) {
        if( g->get_levz() >= 0 ) {
            add_msg( _( "A flash of lightning illuminates your surroundings!" ) );
            sfx::play_variant_sound( "environment", "thunder_near", 100, rng( 0, 359 ) );
            g->weather.lightning_active = true;
        }
    } else {
        g->weather.lightning_active = false;
    }
}

/**
 * Acid drizzle.
 * Causes minor pain only.
 */
void weather_effect::light_acid()
{
    generic_wet( true );
    if( calendar::once_every( 1_minutes ) && is_player_outside() ) {
        if( g->u.weapon.has_flag( "RAIN_PROTECT" ) && !one_in( 3 ) ) {
            add_msg( _( "Your %s protects you from the acidic drizzle." ), g->u.weapon.tname() );
        } else {
            if( g->u.worn_with_flag( "RAINPROOF" ) && !one_in( 4 ) ) {
                add_msg( _( "Your clothing protects you from the acidic drizzle." ) );
            } else {
                bool has_helmet = false;
                if( g->u.is_wearing_power_armor( &has_helmet ) && ( has_helmet || !one_in( 4 ) ) ) {
                    add_msg( _( "Your power armor protects you from the acidic drizzle." ) );
                } else {
                    add_msg( m_warning, _( "The acid rain stings, but is mostly harmless for now..." ) );
                    if( one_in( 10 ) && ( g->u.get_pain() < 10 ) ) {
                        g->u.mod_pain( 1 );
                    }
                }
            }
        }
    }
}

/**
 * Acid rain.
 * Causes major pain. Damages non acid-proof mobs. Very wet (acid).
 */
void weather_effect::acid()
{
    if( calendar::once_every( 2_turns ) && is_player_outside() ) {
        if( g->u.weapon.has_flag( "RAIN_PROTECT" ) && one_in( 4 ) ) {
            add_msg( _( "Your umbrella protects you from the acid rain." ) );
        } else {
            if( g->u.worn_with_flag( "RAINPROOF" ) && one_in( 2 ) ) {
                add_msg( _( "Your clothing protects you from the acid rain." ) );
            } else {
                bool has_helmet = false;
                if( g->u.is_wearing_power_armor( &has_helmet ) && ( has_helmet || !one_in( 2 ) ) ) {
                    add_msg( _( "Your power armor protects you from the acid rain." ) );
                } else {
                    add_msg( m_bad, _( "The acid rain burns!" ) );
                    if( one_in( 2 ) && ( g->u.get_pain() < 100 ) ) {
                        g->u.mod_pain( rng( 1, 5 ) );
                    }
                }
            }
        }
    }
    generic_very_wet( true );
}

static std::string to_string( const weekdays &d )
{
    static const std::array<std::string, 7> weekday_names = {{
            translate_marker( "Sunday" ), translate_marker( "Monday" )
            translate_marker( "Tuesday" ), translate_marker( "Wednesday" )
            translate_marker( "Thursday" ), translate_marker( "Friday" )
            translate_marker( "Saturday" )
        }
    };
    static_assert( static_cast<int>( weekdays::SUNDAY ) == 0,
                   "weekday_names array is out of sync with weekdays enumeration values" );
    return _( weekday_names[ static_cast<int>( d ) ] );
}

static std::string print_time_just_hour( const time_point &p )
{
    const int hour = to_hours<int>( time_past_midnight( p ) );
    int hour_param = hour % 12;
    if( hour_param == 0 ) {
        hour_param = 12;
    }
    return string_format( hour < 12 ? _( "%d AM" ) : _( "%d PM" ), hour_param );
}

// Script from Wikipedia:
// Current time
// The current time is hour/minute Eastern Standard Time
// Local conditions
// At 8 AM in Falls City, it was sunny. The temperature was 60 degrees, the dewpoint 59,
// and the relative humidity 97%. The wind was west at 6 miles (9.7 km) an hour.
// The pressure was 30.00 inches (762 mm) and steady.
// Regional conditions
// Across eastern Nebraska, southwest Iowa, and northwest Missouri, skies ranged from
// sunny to mostly sunny. It was 60 at Beatrice, 59 at Lincoln, 59 at Nebraska City, 57 at Omaha,
// 59 at Red Oak, and 62 at St. Joseph."
// Forecast
// TODAY...MOSTLY SUNNY. HIGHS IN THE LOWER 60S. NORTHEAST WINDS 5 TO 10 MPH WITH GUSTS UP TO 25 MPH.
// TONIGHT...MOSTLY CLEAR. LOWS IN THE UPPER 30S. NORTHEAST WINDS 5 TO 10 MPH.
// MONDAY...MOSTLY SUNNY. HIGHS IN THE LOWER 60S. NORTHEAST WINDS 10 TO 15 MPH.
// MONDAY NIGHT...PARTLY CLOUDY. LOWS AROUND 40. NORTHEAST WINDS 5 TO 10 MPH.

// 0% - No mention of precipitation
// 10% - No mention of precipitation, or isolated/slight chance
// 20% - Isolated/slight chance
// 30% - (Widely) scattered/chance
// 40% or 50% - Scattered/chance
// 60% or 70% - Numerous/likely
// 80%, 90% or 100% - No additional modifiers (i.e. "showers and thunderstorms")
/**
 * Generate textual weather forecast for the specified radio tower.
 */
std::string weather_forecast( const point &abs_sm_pos )
{
    std::ostringstream weather_report;
    // Local conditions
    const auto cref = overmap_buffer.closest_city( tripoint( abs_sm_pos, 0 ) );
    const std::string city_name = cref ? cref.city->name : std::string( _( "middle of nowhere" ) );
    // Current time
    weather_report << string_format(
                       _( "The current time is %s Eastern Standard Time.  At %s in %s, it was %s. The temperature was %s. " ),
                       to_string_time_of_day( calendar::turn ), print_time_just_hour( calendar::turn ),
                       city_name,
                       weather::name( g->weather.weather ), print_temperature( g->weather.temperature )
                   );

    //weather_report << ", the dewpoint ???, and the relative humidity ???.  ";
    //weather_report << "The wind was <direction> at ? mi/km an hour.  ";
    //weather_report << "The pressure was ??? in/mm and steady/rising/falling.";

    // Regional conditions (simulated by choosing a random range containing the current conditions).
    // Adjusted for weather volatility based on how many weather changes are coming up.
    //weather_report << "Across <region>, skies ranged from <cloudiest> to <clearest>.  ";
    // TODO: Add fake reports for nearby cities

    // TODO: weather forecast
    // forecasting periods are divided into 12-hour periods, day (6-18) and night (19-5)
    // Accumulate percentages for each period of various weather statistics, and report that
    // (with fuzz) as the weather chances.
    // int weather_proportions[NUM_WEATHER_TYPES] = {0};
    double high = -100.0;
    double low = 100.0;
    const tripoint abs_ms_pos = tripoint( sm_to_ms_copy( abs_sm_pos ), 0 );
    // TODO: wind direction and speed
    const time_point last_hour = calendar::turn - ( calendar::turn - calendar::turn_zero ) %
                                 1_hours;
    for( int d = 0; d < 6; d++ ) {
        weather_type forecast = WEATHER_NULL;
        const auto wgen = g->weather.get_cur_weather_gen();
        for( time_point i = last_hour + d * 12_hours; i < last_hour + ( d + 1 ) * 12_hours; i += 1_hours ) {
            w_point w = wgen.get_weather( abs_ms_pos, i, g->get_seed() );
            forecast = std::max( forecast, wgen.get_weather_conditions( w ) );
            high = std::max( high, w.temperature );
            low = std::min( low, w.temperature );
        }
        std::string day;
        bool started_at_night;
        const time_point c = last_hour + 12_hours * d;
        if( d == 0 && is_night( c ) ) {
            day = _( "Tonight" );
            started_at_night = true;
        } else {
            day = _( "Today" );
            started_at_night = false;
        }
        if( d > 0 && ( ( started_at_night && !( d % 2 ) ) || ( !started_at_night && d % 2 ) ) ) {
            day = string_format( pgettext( "Mon Night", "%s Night" ), to_string( day_of_week( c ) ) );
        } else {
            day = to_string( day_of_week( c ) );
        }
        weather_report << string_format(
                           _( "%s... %s. Highs of %s. Lows of %s. " ),
                           day, weather::name( forecast ),
                           print_temperature( high ), print_temperature( low )
                       );
    }
    return weather_report.str();
}

/**
 * Print temperature (and convert to Celsius if Celsius display is enabled.)
 */
std::string print_temperature( double fahrenheit, int decimals )
{
    std::ostringstream ret;
    ret.precision( decimals );
    ret << std::fixed;

    if( get_option<std::string>( "USE_CELSIUS" ) == "celsius" ) {
        ret << temp_to_celsius( fahrenheit );
        return string_format( pgettext( "temperature in Celsius", "%sC" ), ret.str() );
    } else if( get_option<std::string>( "USE_CELSIUS" ) == "kelvin" ) {
        ret << temp_to_kelvin( fahrenheit );
        return string_format( pgettext( "temperature in Kelvin", "%sK" ), ret.str() );
    } else {
        ret << fahrenheit;
        return string_format( pgettext( "temperature in Fahrenheit", "%sF" ), ret.str() );
    }
}

/**
 * Print relative humidity (no conversions.)
 */
std::string print_humidity( double humidity, int decimals )
{
    std::ostringstream ret;
    ret.precision( decimals );
    ret << std::fixed;

    ret << humidity;
    return string_format( pgettext( "humidity in percent", "%s%%" ), ret.str() );
}

/**
 * Print pressure (no conversions.)
 */
std::string print_pressure( double pressure, int decimals )
{
    std::ostringstream ret;
    ret.precision( decimals );
    ret << std::fixed;

    ret << pressure / 10;
    return string_format( pgettext( "air pressure in kPa", "%s kPa" ), ret.str() );
}

int get_local_windchill( double temperature, double humidity, double windpower )
{
    double tmptemp = temperature;
    double tmpwind = windpower;
    double windchill = 0;

    if( tmptemp < 50 ) {
        /// Model 1, cold wind chill (only valid for temps below 50F)
        /// Is also used as a standard in North America.

        // Temperature is removed at the end, because get_local_windchill is meant to calculate the difference.
        // Source : http://en.wikipedia.org/wiki/Wind_chill#North_American_and_United_Kingdom_wind_chill_index
        windchill = 35.74 + 0.6215 * tmptemp - 35.75 * pow( tmpwind,
                    0.16 ) + 0.4275 * tmptemp * pow( tmpwind, 0.16 ) - tmptemp;
        if( tmpwind < 4 ) {
            windchill = 0;    // This model fails when there is 0 wind.
        }
    } else {
        /// Model 2, warm wind chill

        // Source : http://en.wikipedia.org/wiki/Wind_chill#Australian_Apparent_Temperature
        tmpwind = tmpwind * 0.44704; // Convert to meters per second.
        tmptemp = temp_to_celsius( tmptemp );

        windchill = 0.33 * ( humidity / 100.00 * 6.105 * exp( 17.27 * tmptemp /
                             ( 237.70 + tmptemp ) ) ) - 0.70 * tmpwind - 4.00;
        // Convert to Fahrenheit, but omit the '+ 32' because we are only dealing with a piece of the felt air temperature equation.
        windchill = windchill * 9 / 5;
    }

    return windchill;
}

nc_color get_wind_color( double windpower )
{
    nc_color windcolor;
    if( windpower < 1 ) {
        windcolor = c_dark_gray;
    } else if( windpower < 3 ) {
        windcolor = c_dark_gray;
    } else if( windpower < 7 ) {
        windcolor = c_light_gray;
    } else if( windpower < 12 ) {
        windcolor = c_light_gray;
    } else if( windpower < 18 ) {
        windcolor = c_blue;
    } else if( windpower < 24 ) {
        windcolor = c_blue;
    } else if( windpower < 31 ) {
        windcolor = c_light_blue;
    } else if( windpower < 38 ) {
        windcolor = c_light_blue;
    } else if( windpower < 46 ) {
        windcolor = c_cyan;
    } else if( windpower < 54 ) {
        windcolor = c_cyan;
    } else if( windpower < 63 ) {
        windcolor = c_light_cyan;
    } else if( windpower < 72 ) {
        windcolor = c_light_cyan;
    } else if( windpower > 72 ) {
        windcolor = c_white;
    }
    return windcolor;
}

std::string get_shortdirstring( int angle )
{
    std::string dirstring;
    int dirangle = angle;
    if( dirangle <= 23 || dirangle > 338 ) {
        dirstring = _( "N" );
    } else if( dirangle <= 68 && dirangle > 23 ) {
        dirstring = _( "NE" );
    } else if( dirangle <= 113 && dirangle > 68 ) {
        dirstring = _( "E" );
    } else if( dirangle <= 158 && dirangle > 113 ) {
        dirstring = _( "SE" );
    } else if( dirangle <= 203 && dirangle > 158 ) {
        dirstring = _( "S" );
    } else if( dirangle <= 248 && dirangle > 203 ) {
        dirstring = _( "SW" );
    } else if( dirangle <= 293 && dirangle > 248 ) {
        dirstring = _( "W" );
    } else if( dirangle <= 338 && dirangle > 293 ) {
        dirstring = _( "NW" );
    }
    return dirstring;
}

std::string get_dirstring( int angle )
{
    // Convert angle to cardinal directions
    std::string dirstring;
    int dirangle = angle;
    if( dirangle <= 23 || dirangle > 338 ) {
        dirstring = _( "North" );
    } else if( dirangle <= 68 && dirangle > 23 ) {
        dirstring = _( "North-East" );
    } else if( dirangle <= 113 && dirangle > 68 ) {
        dirstring = _( "East" );
    } else if( dirangle <= 158 && dirangle > 113 ) {
        dirstring = _( "South-East" );
    } else if( dirangle <= 203 && dirangle > 158 ) {
        dirstring = _( "South" );
    } else if( dirangle <= 248 && dirangle > 203 ) {
        dirstring = _( "South-West" );
    } else if( dirangle <= 293 && dirangle > 248 ) {
        dirstring = _( "West" );
    } else if( dirangle <= 338 && dirangle > 293 ) {
        dirstring = _( "North-West" );
    }
    return dirstring;
}

std::string get_wind_arrow( int dirangle )
{
    std::string wind_arrow;
    if( ( dirangle <= 23 && dirangle >= 0 ) || ( dirangle > 338 && dirangle < 360 ) ) {
        wind_arrow = "\u21D3";
    } else if( dirangle <= 68 && dirangle > 23 ) {
        wind_arrow = "\u21D9";
    } else if( dirangle <= 113 && dirangle > 68 ) {
        wind_arrow = "\u21D0";
    } else if( dirangle <= 158 && dirangle > 113 ) {
        wind_arrow = "\u21D6";
    } else if( dirangle <= 203 && dirangle > 158 ) {
        wind_arrow = "\u21D1";
    } else if( dirangle <= 248 && dirangle > 203 ) {
        wind_arrow = "\u21D7";
    } else if( dirangle <= 293 && dirangle > 248 ) {
        wind_arrow = "\u21D2";
    } else if( dirangle <= 338 && dirangle > 293 ) {
        wind_arrow = "\u21D8";
    } else {
        wind_arrow.clear();
    }
    return wind_arrow;
}

int get_local_humidity( double humidity, weather_type weather, bool sheltered )
{
    int tmphumidity = humidity;
    if( sheltered ) {
        // Norm for a house?
        tmphumidity = humidity * ( 100 - humidity ) / 100 + humidity;
    } else if( weather == WEATHER_RAINY || weather == WEATHER_DRIZZLE || weather == WEATHER_THUNDER ||
               weather == WEATHER_LIGHTNING ) {
        tmphumidity = 100;
    }

    return tmphumidity;
}

double get_local_windpower( double windpower, const oter_id &omter, const tripoint &location,
                            const int &winddirection, bool sheltered )
{
    /**
    *  A player is sheltered if he is underground, in a car, or indoors.
    **/
    if( sheltered ) {
        return 0.0;
    }
    rl_vec2d windvec = convert_wind_to_coord( winddirection );
    double tmpwind = windpower;
    tripoint triblocker( location + point( windvec.x, windvec.y ) );
    // Over map terrain may modify the effect of wind.
    if( omter.id() == "forest_water" ) {
        tmpwind *= 0.7;
    } else if( omter.id() == "forest" ) {
        tmpwind *= 0.5;
    } else if( omter.id() == "forest_thick" || omter.id() == "hive" ) {
        tmpwind *= 0.4;
    }
    // An adjacent wall will block wind
    if( is_wind_blocker( triblocker ) ) {
        tmpwind *= 0.1;
    }
    return tmpwind;
}

bool is_wind_blocker( const tripoint &location )
{
    return g->m.has_flag( "BLOCK_WIND", location );
}

// Description of Wind Speed - https://en.wikipedia.org/wiki/Beaufort_scale
std::string get_wind_desc( double windpower )
{
    std::string winddesc;
    if( windpower < 1 ) {
        winddesc = _( "Calm" );
    } else if( windpower <= 3 ) {
        winddesc = _( "Light Air" );
    } else if( windpower <= 7 ) {
        winddesc = _( "Light Breeze" );
    } else if( windpower <= 12 ) {
        winddesc = _( "Gentle Breeze" );
    } else if( windpower <= 18 ) {
        winddesc = _( "Moderate Breeze" );
    } else if( windpower <= 24 ) {
        winddesc = _( "Fresh Breeze" );
    } else if( windpower <= 31 ) {
        winddesc = _( "Strong Breeze" );
    } else if( windpower <= 38 ) {
        winddesc = _( "Moderate Gale" );
    } else if( windpower <= 46 ) {
        winddesc = _( "Gale" );
    } else if( windpower <= 54 ) {
        winddesc = _( "Strong Gale" );
    } else if( windpower <= 63 ) {
        winddesc = _( "Whole Gale" );
    } else if( windpower <= 72 ) {
        winddesc = _( "Violent Storm" );
    } else if( windpower > 72 ) {
        // Anything above Whole Gale is very unlikely to happen and has no additional effects.
        winddesc = _( "Hurricane" );
    }
    return winddesc;
}

rl_vec2d convert_wind_to_coord( const int angle )
{
    static const std::array<std::pair<int, rl_vec2d>, 9> outputs = {{
            { 330, rl_vec2d( 0, -1 ) },
            { 301, rl_vec2d( -1, -1 ) },
            { 240, rl_vec2d( -1, 0 ) },
            { 211, rl_vec2d( -1, 1 ) },
            { 150, rl_vec2d( 0, 1 ) },
            { 121, rl_vec2d( 1, 1 ) },
            { 60, rl_vec2d( 1, 0 ) },
            { 31, rl_vec2d( 1, -1 ) },
            { 0, rl_vec2d( 0, -1 ) }
        }
    };
    for( const std::pair<int, rl_vec2d> &val : outputs ) {
        if( angle >= val.first ) {
            return val.second;
        }
    }
    return rl_vec2d( 0, 0 );
}

bool warm_enough_to_plant( const tripoint &pos )
{
    // semi-appropriate temperature for most plants
    return g->weather.get_temperature( pos ) >= 50;
}

weather_manager::weather_manager()
{

    lightning_active = false;
    weather_override = WEATHER_NULL;
    nextweather = calendar::before_time_starts;
    temperature = 0;
    weather = WEATHER_CLEAR;
}

const weather_generator &weather_manager::get_cur_weather_gen() const
{
    const overmap &om = g->get_cur_om();
    const regional_settings &settings = om.get_settings();
    return settings.weather;
}

void weather_manager::update_weather()
{
    w_point &w = *weather_precise;
    winddirection = wind_direction_override ? *wind_direction_override : w.winddirection;
    windspeed = windspeed_override ? *windspeed_override : w.windpower;
    if( weather == WEATHER_NULL || calendar::turn >= nextweather ) {
        const weather_generator &weather_gen = get_cur_weather_gen();
        w = weather_gen.get_weather( g->u.global_square_location(), calendar::turn, g->get_seed() );
        weather_type old_weather = weather;
        weather = weather_override == WEATHER_NULL ?
                  weather_gen.get_weather_conditions( w )
                  : weather_override;
        if( weather == WEATHER_SUNNY && is_night( calendar::turn ) ) {
            weather = WEATHER_CLEAR;
        }
        sfx::do_ambient();
        temperature = w.temperature;
        lightning_active = false;
        // Check weather every few turns, instead of every turn.
        // TODO: predict when the weather changes and use that time.
        nextweather = calendar::turn + 5_minutes;
        const weather_datum wdata = weather_data( weather );
        if( weather != old_weather && wdata.dangerous &&
            g->get_levz() >= 0 && g->m.is_outside( g->u.pos() )
            && !g->u.has_activity( activity_id( "ACT_WAIT_WEATHER" ) ) ) {
            g->cancel_activity_or_ignore_query( distraction_type::weather_change,
                                                string_format( _( "The weather changed to %s!" ), wdata.name ) );
        }

        if( weather != old_weather && g->u.has_activity( activity_id( "ACT_WAIT_WEATHER" ) ) ) {
            g->u.assign_activity( activity_id( "ACT_WAIT_WEATHER" ), 0, 0 );
        }

        if( wdata.sight_penalty !=
            weather::sight_penalty( old_weather ) ) {
            for( int i = -OVERMAP_DEPTH; i <= OVERMAP_HEIGHT; i++ ) {
                g->m.set_transparency_cache_dirty( i );
            }
        }
    }
}

void weather_manager::set_nextweather( time_point t )
{
    nextweather = t;
    update_weather();
}

int weather_manager::get_temperature( const tripoint &location )
{
    const auto &cached = temperature_cache.find( location );
    if( cached != temperature_cache.end() ) {
        return cached->second;
    }

    int temp_mod = 0; // local modifier

    if( !g->new_game ) {
        temp_mod += get_heat_radiation( location, false );
        temp_mod += get_convection_temperature( location );
    }
    //underground temperature = average New England temperature = 43F/6C rounded to int
    const int temp = ( location.z < 0 ? AVERAGE_ANNUAL_TEMPERATURE : temperature ) +
                     ( g->new_game ? 0 : g->m.get_temperature( location ) + temp_mod );

    temperature_cache.emplace( std::make_pair( location, temp ) );
    return temp;
}

void weather_manager::clear_temp_cache()
{
    temperature_cache.clear();
}

///@}
