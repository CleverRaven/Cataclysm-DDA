#include "options.h"
#include "game.h"
#include "weather.h"
#include "messages.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "math.h"

#include <vector>
#include <sstream>

/**
 * \defgroup Weather "Weather and its implications."
 * @{
 */

#define PLAYER_OUTSIDE (g->m.is_outside(g->u.posx(), g->u.posy()) && g->levz >= 0)
#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

/**
 * Glare.
 * Causes glare effect to player's eyes if they are not wearing applicable eye protection.
 */
void weather_effect::glare()
{
    if (PLAYER_OUTSIDE && g->is_in_sunlight(g->u.posx(), g->u.posy()) &&
        !g->u.worn_with_flag("SUN_GLASSES") && !g->u.has_bionic("bio_sunglasses")) {
        if(!g->u.has_effect("glare")) {
            if (g->u.has_trait("CEPH_VISION")) {
                g->u.add_env_effect("glare", bp_eyes, 2, 4);
            } else {
                g->u.add_env_effect("glare", bp_eyes, 2, 2);
            }
        } else {
            if (g->u.has_trait("CEPH_VISION")) {
                g->u.add_env_effect("glare", bp_eyes, 2, 2);
            } else {
                g->u.add_env_effect("glare", bp_eyes, 2, 1);
            }
        }
    }
}
////// food vs weather

/**
 * Retroactively determine weather-related rotting effects.
 * Applies rot based on the temperatures incurred between a turn range.
 */
int get_rot_since( const int since, const int endturn, const point &location )
{
    // Hack: Ensure food doesn't rot in ice labs, where the
    // temperature is much less than the weather specifies.
    // http://github.com/CleverRaven/Cataclysm-DDA/issues/9162
    // Bug with this hack: Rot is prevented even when it's above
    // freezing on the ground floor.
    oter_id oter = overmap_buffer.ter(g->om_global_location());
    if (is_ot_type("ice_lab", oter)) {
        return 0;
    }
    int ret = 0;
    for (calendar i(since); i.get_turn() < endturn; i += 600) {
        w_point w = g->weatherGen.get_weather(location, i);
        ret += std::min(600, endturn - i.get_turn()) * get_hourly_rotpoints_at_temp(w.temperature) / 600;
    }
    return ret;
}

////// Funnels.
/**
 * mm/h of rain/acid for weather type (should move to weather_data)
 */
std::pair<int, int> rain_or_acid_level( const int wt )
{
    if ( wt == WEATHER_ACID_RAIN || wt == WEATHER_ACID_DRIZZLE ) {
        return std::make_pair(0, (wt == WEATHER_ACID_RAIN  ? 8 : 4 ));
    } else if (wt == WEATHER_DRIZZLE ) {
        return std::make_pair(4, 0);
        // why isn't this in weather data. now we have multiple rain/turn scales =[
    } else if ( wt ==  WEATHER_RAINY || wt == WEATHER_THUNDER || wt == WEATHER_LIGHTNING ) {
        return std::make_pair(8, 0);
        /// @todo bucket of melted snow?
    } else {
        return std::make_pair(0, 0);
    }
}

/**
 * Determine what a funnel has filled out of game, using funnelcontainer.bday as a starting point.
 */
void retroactively_fill_from_funnel( item *it, const trap_id t, const calendar &endturn,
                                     const point &location )
{
    const calendar startturn = calendar( it->bday > 0 ? it->bday - 1 : 0 );
    if ( startturn > endturn || traplist[t]->funnel_radius_mm < 1 ) {
        return;
    }
    it->bday = int(endturn.get_turn()); // bday == last fill check
    int rain_amount = 0;
    int acid_amount = 0;
    int rain_turns = 0;
    int acid_turns = 0;
    for( calendar turn(startturn); turn >= endturn; turn += 10) {
        switch(g->weatherGen.get_weather_conditions(location, turn)) {
        case WEATHER_DRIZZLE:
            rain_amount += 4;
            rain_turns++;
            break;
        case WEATHER_RAINY:
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            rain_amount += 8;
            rain_turns++;
            break;
        case WEATHER_ACID_DRIZZLE:
            acid_amount += 4;
            acid_turns++;
            break;
        case WEATHER_ACID_RAIN:
            acid_amount += 8;
            acid_turns++;
            break;
        default:
            break;
        }
    }
    int rain = rain_turns / traplist[t]->funnel_turns_per_charge( rain_amount );
    int acid = acid_turns / traplist[t]->funnel_turns_per_charge( acid_amount );
    it->add_rain_to_container( false, rain );
    it->add_rain_to_container( true, acid );
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
    const long capa = get_remaining_capacity_for_liquid( ret );
    if (contents.empty()) {
        // This is easy. Just add 1 charge of the rain liquid to the container.
        if (!acid) {
            // Funnels aren't always clean enough for water. // todo; disinfectant squeegie->funnel
            ret.poison = one_in(10) ? 1 : 0;
        }
        ret.charges = std::min<long>( charges, capa );
        put_in(ret);
    } else {
        // The container already has a liquid.
        item &liq = contents[0];
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
                item transmuted("water_acid_weak", 0);
                transmuted.charges = liq.charges;
                contents[0] = transmuted;
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

double trap::funnel_turns_per_charge( double rain_depth_mm_per_hour ) const
{
    // 1mm rain on 1m^2 == 1 liter water == 1000ml
    // 1 liter == 4 volume
    // 1 volume == 250ml: containers
    // 1 volume == 200ml: water
    // How many turns should it take for us to collect 1 charge of rainwater?
    // "..."
    if ( rain_depth_mm_per_hour == 0 ) {
        return 0;
    }
    const item water("water", 0);
    const double charge_ml = (double) (water.weight()) / water.charges; // 250ml
    const double PI = 3.14159265358979f;

    const double surface_area_mm2 = PI * (funnel_radius_mm * funnel_radius_mm);

    const double vol_mm3_per_hour = surface_area_mm2 * rain_depth_mm_per_hour;
    const double vol_mm3_per_turn = vol_mm3_per_hour / 600;

    const double ml_to_mm3 = 1000;
    const double turns_per_charge = (charge_ml * ml_to_mm3) / vol_mm3_per_turn;
    return turns_per_charge;// / rain_depth_mm_per_hour;
}

/**
 * Main routine for filling funnels from weather effects.
 */
void fill_funnels(int rain_depth_mm_per_hour, bool acid, trap_id t)
{
    const double turns_per_charge = traplist[t]->funnel_turns_per_charge(rain_depth_mm_per_hour);
    // Give each funnel on the map a chance to collect the rain.
    const std::set<point> &funnel_locs = g->m.trap_locations(t);
    std::set<point>::const_iterator i;
    for (i = funnel_locs.begin(); i != funnel_locs.end(); ++i) {
        int maxcontains = 0;
        point loc = *i;
        auto items = g->m.i_at(loc.x, loc.y);
        if (one_in(turns_per_charge)) { // todo; fixme. todo; fixme
            //add_msg("%d mm/h %d tps %.4f: fill",int(calendar::turn),rain_depth_mm_per_hour,turns_per_charge);
            // This funnel has collected some rain! Put the rain in the largest
            // container here which is either empty or contains some mixture of
            // impure water and acid.
            auto container = items.end();
            for( auto candidate_container = items.begin(); candidate_container != items.end();
                 ++candidate_container ) {
                if ( candidate_container->is_funnel_container( maxcontains ) ) {
                    container = candidate_container;
                }
            }

            if( container != items.end() ) {
                container->add_rain_to_container(acid, 1);
                container->bday = int(calendar::turn);
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
    fill_funnels(mmPerHour, acid, tr_funnel);
    fill_funnels(mmPerHour, acid, tr_makeshift_funnel);
    fill_funnels(mmPerHour, acid, tr_leather_funnel);
}

/**
 * Weather-based degradation of fires and scentmap.
 */
void decay_fire_and_scent(int fire_amount)
{
    for (int x = g->u.posx() - SEEX * 2; x <= g->u.posx() + SEEX * 2; x++) {
        for (int y = g->u.posy() - SEEY * 2; y <= g->u.posy() + SEEY * 2; y++) {
            if (g->m.is_outside(x, y)) {
                g->m.adjust_field_age(point(x, y), fd_fire, fire_amount);
                if (g->scent(x, y) > 0) {
                    g->scent(x, y)--;
                }
            }
        }
    }
}

/**
 * Main routine for wet effects caused by weather.
 * Drenching the player is applied after checks against worn and held items.
 *
 * The warmth of armor is considered when determining how much drench happens.
 *
 * Note that this is not the only place where drenching can happen. For example, moving or swimming into water tiles will also cause drenching.
 * @see fill_water_collectors
 * @see decay_fire_and_scent
 * @see player::drench
 */
void generic_wet(bool acid)
{
    if ((!g->u.worn_with_flag("RAINPROOF") || one_in(100)) &&
        (!g->u.weapon.has_flag("RAIN_PROTECT") || one_in(20)) && !g->u.has_trait("FEATHERS") &&
        (g->u.warmth(bp_torso) * 4 / 5 + g->u.warmth(bp_head) / 5) < 30 && PLAYER_OUTSIDE &&
        one_in(2)) {
        if (g->u.weapon.has_flag("RAIN_PROTECT")) {
            // Umbrellas tend to protect one's head and torso pretty well
            g->u.drench(30 - (g->u.warmth(bp_leg_l) + (g->u.warmth(bp_leg_r)) * 2 / 5 +
                              (g->u.warmth(bp_foot_l) + g->u.warmth(bp_foot_r)) / 10),
                        mfb(bp_leg_l) | mfb(bp_leg_r));
        } else {
            g->u.drench(30 - (g->u.warmth(bp_torso) * 4 / 5 + g->u.warmth(bp_head) / 5),
                        mfb(bp_torso) | mfb(bp_arm_l) | mfb(bp_arm_r) | mfb(bp_head));
        }
    }

    fill_water_collectors(4, acid); // fixme; consolidate drench, this, and decay_fire_and_scent.
    decay_fire_and_scent(15);
}

/**
 * Main routine for very wet effects caused by weather.
 * Similar to generic_wet() but with more aggressive numbers.
 * @see fill_water_collectors
 * @see decay_fire_and_scent
 * @see player::drench
 */
void generic_very_wet(bool acid)
{
    if ((!g->u.worn_with_flag("RAINPROOF") || one_in(50)) &&
        (!g->u.weapon.has_flag("RAIN_PROTECT") || one_in(10)) && !g->u.has_trait("FEATHERS") &&
        (g->u.warmth(bp_torso) * 4 / 5 + g->u.warmth(bp_head) / 5) < 60 && PLAYER_OUTSIDE) {
        if (g->u.weapon.has_flag("RAIN_PROTECT")) {
            // Umbrellas tend to protect one's head and torso pretty well
            g->u.drench(60 - ((g->u.warmth(bp_leg_l) + g->u.warmth(bp_leg_r)) * 2 / 5 +
                              (g->u.warmth(bp_foot_l) + g->u.warmth(bp_foot_r)) / 10),
                        mfb(bp_leg_l) | mfb(bp_leg_r));
        } else {
            g->u.drench(60 - (g->u.warmth(bp_torso) * 4 / 5 + g->u.warmth(bp_head) / 5),
                        mfb(bp_torso) | mfb(bp_arm_l) | mfb(bp_arm_r) | mfb(bp_head));
        }
    }

    fill_water_collectors(8, acid);
    decay_fire_and_scent(45);
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
    if (!g->u.is_deaf() && one_in(THUNDER_CHANCE)) {
        if (g->levz >= 0) {
            add_msg(_("You hear a distant rumble of thunder."));
        } else if (g->u.has_trait("GOODHEARING") && one_in(1 - 2 * g->levz)) {
            add_msg(_("You hear a rumble of thunder from above."));
        } else if (!g->u.has_trait("BADHEARING") && one_in(1 - 3 * g->levz)) {
            add_msg(_("You hear a rumble of thunder from above."));
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
        if(g->levz >= 0) {
            add_msg(_("A flash of lightning illuminates your surroundings!"));
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
    if (int(calendar::turn) % 10 == 0 && PLAYER_OUTSIDE) {
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
                    if (one_in(10) && (g->u.pain < 10)) {
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
    if (int(calendar::turn) % 2 == 0 && PLAYER_OUTSIDE) {
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
                    if (one_in(2) && (g->u.pain < 100)) {
                        g->u.mod_pain( rng(1, 5) );
                    }
                }
            }
        }
    }
    generic_very_wet(true);
}

// Script from wikipedia:
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
std::string weather_forecast(radio_tower const &tower)
{
    std::ostringstream weather_report;
    // Local conditions
    city *closest_city = &g->cur_om->cities[g->cur_om->closest_city(point(tower.x, tower.y))];
    // Current time
    weather_report << string_format(
                       _("The current time is %s Eastern Standard Time.  At %s in %s, it was %s. The temperature was %s. "),
                       calendar::turn.print_time().c_str(), calendar::turn.print_time(true).c_str(),
                       closest_city->name.c_str(),
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
    // TODO wind direction and speed
    int last_hour = calendar::turn - (calendar::turn % HOURS(1));
    for(int d = 0; d < 6; d++) {
        weather_type forecast = WEATHER_NULL;
        for(calendar i(last_hour + 7200 * d); i < last_hour + 7200 * (d + 1); i += 600) {
            w_point w = g->weatherGen.get_weather(point(tower.x, tower.y), i);
            forecast = std::max(forecast, g->weatherGen.get_weather_conditions(w));
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
            day = rmp_format(_("<Mon Night>%s Night"), c.day_of_week().c_str());
        } else {
            day = c.day_of_week();
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
 * Print temperature (and convert to celsius if celsius display is enabled.)
 */
std::string print_temperature(float fahrenheit, int decimals)
{
    std::ostringstream ret;
    ret.precision(decimals);
    ret << std::fixed;

    if(OPTIONS["USE_CELSIUS"] == "celsius") {
        ret << ((fahrenheit - 32) * 5 / 9);
        return rmp_format(_("<Celsius>%sC"), ret.str().c_str());
    } else {
        ret << fahrenheit;
        return rmp_format(_("<Fahrenheit>%sF"), ret.str().c_str());
    }

}

/**
 * Print wind speed (and convert to m/s if km/h is enabled.)
 */
std::string print_windspeed(float windspeed, int decimals)
{
    std::ostringstream ret;
    ret.precision(decimals);
    ret << std::fixed;

    if (OPTIONS["USE_METRIC_SPEEDS"] == "mph") {
        ret << windspeed;
        return rmp_format(_("%s mph"), ret.str().c_str());
    } else {
        ret << windspeed * 0.44704;
        return rmp_format(_("%s m/s"), ret.str().c_str());
    }
}

/**
 * Print relative humidity (no conversions.)
 */
std::string print_humidity(float humidity, int decimals)
{
    std::ostringstream ret;
    ret.precision(decimals);
    ret << std::fixed;

    ret << humidity;
    return rmp_format(_("%s %%"), ret.str().c_str());
}

/**
 * Print pressure (no conversions.)
 */
std::string print_pressure(float pressure, int decimals)
{
    std::ostringstream ret;
    ret.precision(decimals);
    ret << std::fixed;

    ret << pressure/10;
    return rmp_format(_("%s kPa"), ret.str().c_str());
}


int get_local_windchill(double temperature, double humidity, double windpower)
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
        tmptemp = (tmptemp - 32) * 5 / 9; // Convert to celsius.

        windchill = (0.33 * ((humidity / 100.00) * 6.105 * exp((17.27 * tmptemp) /
                             (237.70 + tmptemp))) - 0.70 * tmpwind - 4.00);
        // Convert to Fahrenheit, but omit the '+ 32' because we are only dealing with a piece of the felt air temperature equation.
        windchill = windchill * 9 / 5;
    }

    return windchill;
}

int get_local_humidity(double humidity, weather_type weather, bool sheltered)
{
    int tmphumidity = humidity;
    if (sheltered) {
        tmphumidity = humidity * (100 - humidity) / 100 + humidity; // norm for a house?
    } else if (weather == WEATHER_RAINY || weather == WEATHER_DRIZZLE || weather == WEATHER_THUNDER ||
               weather == WEATHER_LIGHTNING) {
        tmphumidity = 100;
    }

    return tmphumidity;
}

int get_local_windpower(double windpower, std::string const &omtername, bool sheltered)
{
    /**
    *  A player is sheltered if he is underground, in a car, or indoors.
    **/

    double tmpwind = windpower;

    // Over map terrain may modify the effect of wind.
    if (sheltered) {
        tmpwind  = 0.0;
    } else if ( omtername == "forest_water") {
        tmpwind *= 0.7;
    } else if ( omtername == "forest" ) {
        tmpwind *= 0.5;
    } else if ( omtername == "forest_thick" || omtername == "hive") {
        tmpwind *= 0.4;
    }

    return tmpwind;
}

///@}
