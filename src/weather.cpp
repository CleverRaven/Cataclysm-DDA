#include "weather.h"

#include "coordinate_conversions.h"
#include "options.h"
#include "output.h"
#include "calendar.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "math.h"
#include "string_formatter.h"
#include "translations.h"
#include "weather_gen.h"
#include "sounds.h"
#include "cata_utility.h"
#include "player.h"
#include "game_constants.h"

#include <vector>
#include <sstream>

const efftype_id effect_glare( "glare" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_sleep( "sleep" );

static const trait_id trait_CEPH_VISION( "CEPH_VISION" );
static const trait_id trait_FEATHERS( "FEATHERS" );
static const trait_id trait_GOODHEARING( "GOODHEARING" );
static const trait_id trait_BADHEARING( "BADHEARING" );

/**
 * \defgroup Weather "Weather and its implications."
 * @{
 */

static bool is_player_outside()
{
    return g->m.is_outside( g->u.posx(), g->u.posy() ) && g->get_levz() >= 0;
}

#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

/**
 * Glare.
 * Causes glare effect to player's eyes if they are not wearing applicable eye protection.
 */
void weather_effect::glare()
{
    if( is_player_outside() && g->is_in_sunlight( g->u.pos() ) && !g->u.in_sleep_state() &&
        !g->u.worn_with_flag( "SUN_GLASSES" ) && !g->u.is_blind() &&
        !g->u.has_bionic( bionic_id( "bio_sunglasses" ) ) ) {
        if( !g->u.has_effect( effect_glare ) ) {
            if( g->u.has_trait( trait_CEPH_VISION ) ) {
                g->u.add_env_effect( effect_glare, bp_eyes, 2, 4_turns );
            } else {
                g->u.add_env_effect( effect_glare, bp_eyes, 2, 2_turns );
            }
        } else {
            if( g->u.has_trait( trait_CEPH_VISION ) ) {
                g->u.add_env_effect( effect_glare, bp_eyes, 2, 2_turns );
            } else {
                g->u.add_env_effect( effect_glare, bp_eyes, 2, 1_turns );
            }
        }
    }
}
////// food vs weather

int get_hourly_rotpoints_at_temp( int temp );

time_duration get_rot_since( const time_point &start, const time_point &end,
                             const tripoint &location )
{
    time_duration ret = 0;
    const auto &wgen = g->get_cur_weather_gen();
    for( time_point i = start; i < end; i += 1_hours ) {
        w_point w = wgen.get_weather( location, i, g->get_seed() );
        //Use weather if above ground, use map temp if below

        double temperature = ( location.z >= 0 ? w.temperature : g->get_temperature( location ) ) + ( g->new_game ? 0 : g->m.temperature( g->m.getlocal( location ) ) );
        
        if( !g->new_game && g->m.ter( g->m.getlocal( location ) ) == t_rootcellar ) {
            temperature = AVERAGE_ANNUAL_TEMPERATURE;
        }

        ret += std::min( 1_hours, end - i ) / 1_hours * get_hourly_rotpoints_at_temp( temperature ) * 1_turns;
    }
    return ret;
}

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
    const float tick_sunlight = calendar( to_turn<int>( t ) ).sunlight() + weather_data( wtype ).light_modifier;
    data.sunlight += std::max<float>( 0.0f, to_turns<int>( tick_size ) * tick_sunlight );
}

////// Funnels.
weather_sum sum_conditions( const time_point &start, const time_point &end,
                            const tripoint &location )
{
    time_duration tick_size = 0;
    weather_sum data;

    const auto wgen = g->get_cur_weather_gen();
    for( time_point t = start; t < end; t += tick_size ) {
        const time_duration diff = end - t;
        if( diff < 10_turns ) {
            tick_size = 1_turns;
        } else if( diff > 7_days ) {
            tick_size = 1_hours;
        } else {
            tick_size = 1_minutes;
        }

        const auto wtype = wgen.get_weather_conditions( location, to_turn<int>( t ), g->get_seed() );
        proc_weather_sum( wtype, data, t, tick_size );
    }

    return data;
}

/**
 * Determine what a funnel has filled out of game, using funnelcontainer.bday as a starting point.
 */
void retroactively_fill_from_funnel( item &it, const trap &tr, const time_point &start,
                                     const time_point &end, const tripoint &location )
{
    if( start > end || !tr.is_funnel() ) {
        return;
    }

    it.set_birthday( end ); // bday == last fill check
    auto data = sum_conditions( start, end, location );

    // Technically 0.0 division is OK, but it will be cleaner without it
    if( data.rain_amount > 0 ) {
        const int rain = divide_roll_remainder( 1.0 / tr.funnel_turns_per_charge( data.rain_amount ), 1.0f );
        it.add_rain_to_container( false, rain );
        // add_msg(m_debug, "Retroactively adding %d water from turn %d to %d", rain, startturn, endturn);
    }

    if( data.acid_amount > 0 ) {
        const int acid = divide_roll_remainder( 1.0 / tr.funnel_turns_per_charge( data.acid_amount ), 1.0f );
        it.add_rain_to_container( true, acid );
    }
}

/**
 * Add charge(s) of rain to given container, possibly contaminating it.
 */
void item::add_rain_to_container(bool acid, int charges)
{
    if( charges <= 0) {
        return;
    }
    item ret( acid ? "water_acid" : "water", calendar::turn );
    const long capa = get_remaining_capacity_for_liquid( ret, true );
    if (contents.empty()) {
        // This is easy. Just add 1 charge of the rain liquid to the container.
        if (!acid) {
            // Funnels aren't always clean enough for water. // @todo: disinfectant squeegie->funnel
            ret.poison = one_in(10) ? 1 : 0;
        }
        ret.charges = std::min<long>( charges, capa );
        put_in(ret);
    } else {
        // The container already has a liquid.
        item &liq = contents.front();
        long orig = liq.charges;
        long added = std::min<long>( charges, capa );
        if (capa > 0 ) {
            liq.charges += added;
        }

        if (liq.typeId() == ret.typeId() || liq.typeId() == "water_acid_weak") {
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
            const bool transmute = x_in_y(2 * added, liq.charges);

            if (transmute) {
                contents.front() = item( "water_acid_weak", calendar::turn, liq.charges );
            } else if (liq.typeId() == "water") {
                // The container has water, and the acid rain didn't turn it
                // into weak acid. Poison the water instead, assuming 1
                // charge of acid would act like a charge of water with poison 5.
                long total_poison = liq.poison * (orig) + (5 * added);
                liq.poison = total_poison / liq.charges;
                long leftover_poison = total_poison - liq.poison * liq.charges;
                if (leftover_poison > rng(0, liq.charges)) {
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
    static const item water("water", 0);
    static const double charge_ml = ( double ) to_gram( water.weight() ) / water.charges; // 250ml

    const double vol_mm3_per_hour = surface_area_mm2 * rain_depth_mm_per_hour;
    const double vol_mm3_per_turn = vol_mm3_per_hour / HOURS(1);

    const double ml_to_mm3 = 1000;
    const double charges_per_turn = vol_mm3_per_turn / (charge_ml * ml_to_mm3);

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

    const double surface_area_mm2 = M_PI * (funnel_radius_mm * funnel_radius_mm);
    const double charges_per_turn = funnel_charges_per_turn( surface_area_mm2, rain_depth_mm_per_hour );

    if( charges_per_turn > 0.0 ) {
        return 1.0 / charges_per_turn;
    }

    return 0.0;
}

/**
 * Main routine for filling funnels from weather effects.
 */
void fill_funnels(int rain_depth_mm_per_hour, bool acid, const trap &tr)
{
    const double turns_per_charge = tr.funnel_turns_per_charge(rain_depth_mm_per_hour);
    // Give each funnel on the map a chance to collect the rain.
    const auto &funnel_locs = g->m.trap_locations( tr.loadid );
    for( auto loc : funnel_locs ) {
        units::volume maxcontains = 0;
        if (one_in(turns_per_charge)) { // @todo: fixme
            //add_msg("%d mm/h %d tps %.4f: fill",int(calendar::turn),rain_depth_mm_per_hour,turns_per_charge);
            // This funnel has collected some rain! Put the rain in the largest
            // container here which is either empty or contains some mixture of
            // impure water and acid.
            auto items = g->m.i_at(loc);
            auto container = items.end();
            for( auto candidate_container = items.begin(); candidate_container != items.end();
                 ++candidate_container ) {
                if ( candidate_container->is_funnel_container( maxcontains ) ) {
                    container = candidate_container;
                }
            }

            if( container != items.end() ) {
                container->add_rain_to_container(acid, 1);
                container->set_age( 0 );
            }
        }
    }
}

/**
 * Fill funnels and makeshift funnels from weather effects.
 * @see fill_funnels
 */
void fill_water_collectors(int mmPerHour, bool acid)
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
void wet_player( int amount )
{
    if( !is_player_outside() ||
        g->u.has_trait( trait_FEATHERS ) ||
        g->u.weapon.has_flag("RAIN_PROTECT") ||
        ( !one_in(50) && g->u.worn_with_flag("RAINPROOF") ) ) {
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
void generic_wet(bool acid)
{
    fill_water_collectors(4, acid);
    g->m.decay_fields_and_scent( 15_turns );
    wet_player( 30 );
}

/**
 * Main routine for very wet effects caused by weather.
 * Similar to generic_wet() but with more aggressive numbers.
 */
void generic_very_wet(bool acid)
{
    fill_water_collectors( 8, acid );
    g->m.decay_fields_and_scent( 45_turns );
    wet_player( 60 );
}

void weather_effect::none()      {};
void weather_effect::flurry()    {};
void weather_effect::snow()      {};
void weather_effect::snowstorm() {};

/**
 * Wet.
 * @see generic_wet
 */
void weather_effect::wet()
{
    generic_wet(false);
}

/**
 * Very wet.
 * @see generic_very_wet
 */
void weather_effect::very_wet()
{
    generic_very_wet(false);
}

/**
 * Thunder.
 * Flavor messages. Very wet.
 */
void weather_effect::thunder()
{
    very_wet();
    if (!g->u.has_effect( effect_sleep ) && !g->u.is_deaf() && one_in(THUNDER_CHANCE)) {
        if (g->get_levz() >= 0) {
            add_msg(_("You hear a distant rumble of thunder."));
            sfx::play_variant_sound("environment", "thunder_far", 80, rng(0, 359));
        } else if (g->u.has_trait( trait_GOODHEARING ) && one_in(1 - 2 * g->get_levz())) {
            add_msg(_("You hear a rumble of thunder from above."));
            sfx::play_variant_sound("environment", "thunder_far", 100, rng(0, 359));
        } else if (!g->u.has_trait( trait_BADHEARING ) && one_in(1 - 3 * g->get_levz())) {
            add_msg(_("You hear a rumble of thunder from above."));
            sfx::play_variant_sound("environment", "thunder_far", 60, rng(0, 359));
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
    if(one_in(LIGHTNING_CHANCE)) {
        if(g->get_levz() >= 0) {
            add_msg(_("A flash of lightning illuminates your surroundings!"));
            sfx::play_variant_sound("environment", "thunder_near", 100, rng(0, 359));
            g->lightning_active = true;
        }
    } else {
        g->lightning_active = false;
    }
}

/**
 * Acid drizzle.
 * Causes minor pain only.
 */
void weather_effect::light_acid()
{
    generic_wet(true);
    if( calendar::once_every( 1_minutes ) && is_player_outside() ) {
        if (g->u.weapon.has_flag("RAIN_PROTECT") && !one_in(3)) {
            add_msg(_("Your %s protects you from the acidic drizzle."), g->u.weapon.tname().c_str());
        } else {
            if (g->u.worn_with_flag("RAINPROOF") && !one_in(4)) {
                add_msg(_("Your clothing protects you from the acidic drizzle."));
            } else {
                bool has_helmet = false;
                if (g->u.is_wearing_power_armor(&has_helmet) && (has_helmet || !one_in(4))) {
                    add_msg(_("Your power armor protects you from the acidic drizzle."));
                } else {
                    add_msg(m_warning, _("The acid rain stings, but is mostly harmless for now..."));
                    if (one_in(10) && (g->u.get_pain() < 10)) {
                        g->u.mod_pain(1);
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
        if (g->u.weapon.has_flag("RAIN_PROTECT") && one_in(4)) {
            add_msg(_("Your umbrella protects you from the acid rain."));
        } else {
            if (g->u.worn_with_flag("RAINPROOF") && one_in(2)) {
                add_msg(_("Your clothing protects you from the acid rain."));
            } else {
                bool has_helmet = false;
                if (g->u.is_wearing_power_armor(&has_helmet) && (has_helmet || !one_in(2))) {
                    add_msg(_("Your power armor protects you from the acid rain."));
                } else {
                    add_msg(m_bad, _("The acid rain burns!"));
                    if (one_in(2) && (g->u.get_pain() < 100)) {
                        g->u.mod_pain( rng(1, 5) );
                    }
                }
            }
        }
    }
    generic_very_wet(true);
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
    return _( weekday_names[ static_cast<int>( d ) ].c_str() );
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

// 0% – No mention of precipitation
// 10% – No mention of precipitation, or isolated/slight chance
// 20% – Isolated/slight chance
// 30% – (Widely) scattered/chance
// 40% or 50% – Scattered/chance
// 60% or 70% – Numerous/likely
// 80%, 90% or 100% – No additional modifiers (i.e. "showers and thunderstorms")
/**
 * Generate textual weather forecast for the specified radio tower.
 */
std::string weather_forecast( point const &abs_sm_pos )
{
    std::ostringstream weather_report;
    // Local conditions
    const auto cref = overmap_buffer.closest_city( tripoint( abs_sm_pos, 0 ) );
    const std::string city_name = cref ? cref.city->name : std::string( _( "middle of nowhere" ) );
    // Current time
    weather_report << string_format(
                       _("The current time is %s Eastern Standard Time.  At %s in %s, it was %s. The temperature was %s. "),
                       to_string_time_of_day( calendar::turn ), print_time_just_hour( calendar::turn ),
                       city_name.c_str(),
                       weather_data(g->weather).name.c_str(), print_temperature(g->temperature).c_str()
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
    // TODO wind direction and speed
    int last_hour = calendar::turn - ( calendar::turn % HOURS(1) );
    for(int d = 0; d < 6; d++) {
        weather_type forecast = WEATHER_NULL;
        const auto wgen = g->get_cur_weather_gen();
        for(calendar i(last_hour + 7200 * d); i < last_hour + 7200 * (d + 1); i += 600) {
            w_point w = wgen.get_weather( abs_ms_pos, i, g->get_seed() );
            forecast = std::max( forecast, wgen.get_weather_conditions( w ) );
            high = std::max(high, w.temperature);
            low = std::min(low, w.temperature);
        }
        std::string day;
        bool started_at_night;
        calendar c(last_hour + 7200 * d);
        if(d == 0 && c.is_night()) {
            day = _("Tonight");
            started_at_night = true;
        } else {
            day = _("Today");
            started_at_night = false;
        }
        if(d > 0 && ((started_at_night && !(d % 2)) || (!started_at_night && d % 2))) {
            day = string_format( pgettext( "Mon Night", "%s Night" ), to_string( day_of_week( c ) ) );
        } else {
            day = to_string( day_of_week( c ) );
        }
        weather_report << string_format(
                           _("%s... %s. Highs of %s. Lows of %s. "),
                           day.c_str(), weather_data(forecast).name.c_str(),
                           print_temperature(high).c_str(), print_temperature(low).c_str()
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

    if(get_option<std::string>( "USE_CELSIUS" ) == "celsius") {
        ret << temp_to_celsius( fahrenheit );
        return string_format( pgettext( "temperature in Celsius", "%sC" ), ret.str().c_str() );
    } else {
        ret << fahrenheit;
        return string_format( pgettext( "temperature in Fahrenheit", "%sF" ), ret.str().c_str() );
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
    return string_format( pgettext( "humidity in percent", "%s%%" ), ret.str().c_str() );
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
    return string_format( pgettext( "air pressure in kPa", "%s kPa" ), ret.str().c_str() );
}


int get_local_windchill( double temperature, double humidity, double windpower )
{
    double tmptemp = temperature;
    double tmpwind = windpower;
    double windchill = 0;

    if (tmptemp < 50) {
        /// Model 1, cold wind chill (only valid for temps below 50F)
        /// Is also used as a standard in North America.

        // Temperature is removed at the end, because get_local_windchill is meant to calculate the difference.
        // Source : http://en.wikipedia.org/wiki/Wind_chill#North_American_and_United_Kingdom_wind_chill_index
        windchill = 35.74 + 0.6215 * tmptemp - 35.75 * (pow(tmpwind,
                    0.16)) + 0.4275 * tmptemp * (pow(tmpwind, 0.16)) - tmptemp;
        if (tmpwind < 4) {
            windchill = 0;    // This model fails when there is 0 wind.
        }
    } else {
        /// Model 2, warm wind chill

        // Source : http://en.wikipedia.org/wiki/Wind_chill#Australian_Apparent_Temperature
        tmpwind = tmpwind * 0.44704; // Convert to meters per second.
        tmptemp = temp_to_celsius( tmptemp );

        windchill = (0.33 * ((humidity / 100.00) * 6.105 * exp((17.27 * tmptemp) /
                             (237.70 + tmptemp))) - 0.70 * tmpwind - 4.00);
        // Convert to Fahrenheit, but omit the '+ 32' because we are only dealing with a piece of the felt air temperature equation.
        windchill = windchill * 9 / 5;
    }

    return windchill;
}

int get_local_humidity( double humidity, weather_type weather, bool sheltered )
{
    int tmphumidity = humidity;
    if( sheltered ) {
        tmphumidity = humidity * (100 - humidity) / 100 + humidity; // norm for a house?
    } else if (weather == WEATHER_RAINY || weather == WEATHER_DRIZZLE || weather == WEATHER_THUNDER ||
               weather == WEATHER_LIGHTNING) {
        tmphumidity = 100;
    }

    return tmphumidity;
}

int get_local_windpower(double windpower, const oter_id &omter, bool sheltered)
{
    /**
    *  A player is sheltered if he is underground, in a car, or indoors.
    **/

    double tmpwind = windpower;

    // Over map terrain may modify the effect of wind.
    if (sheltered) {
        tmpwind  = 0.0;
    } else if ( omter.id() == "forest_water") {
        tmpwind *= 0.7;
    } else if ( omter.id() == "forest" ) {
        tmpwind *= 0.5;
    } else if ( omter.id() == "forest_thick" || omter.id() == "hive") {
        tmpwind *= 0.4;
    }

    return tmpwind;
}

bool warm_enough_to_plant() {
    return g->get_temperature( g-> u.pos() ) >= 50; // semi-appropriate temperature for most plants
}

///@}
