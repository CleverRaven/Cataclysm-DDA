#include <vector>
#include <sstream>
#include "item_factory.h"
#include "options.h"
#include "game.h"
#include "weather.h"
#include "messages.h"

#define PLAYER_OUTSIDE (g->m.is_outside(g->u.posx, g->u.posy) && g->levz >= 0)
#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

void weather_effect::glare()
{
 if (PLAYER_OUTSIDE && g->is_in_sunlight(g->u.posx, g->u.posy) && !g->u.worn_with_flag("SUN_GLASSES") && !g->u.has_bionic("bio_sunglasses")) {
        if(!g->u.has_effect("glare")) {
            g->u.add_env_effect("glare", bp_eyes, 2, 2);
        } else {
            g->u.add_env_effect("glare", bp_eyes, 2, 1);
        }
    }
}
////// food vs weather

int get_rot_since( const int since, const int endturn ) {
    int ret = 0;
    int tbegin = since;
    int tend = endturn;
    // todo; hourly quick lookback, select weather_log from climate zone
    std::map<int, weather_segment>::iterator wit = g->weather_log.lower_bound( endturn );
    if ( wit == g->weather_log.end() ) { // missing wlog, debugmsg?
       return endturn-since;
    } else if ( wit->first > endturn ) { // incomplete wlog
       return ( (endturn-since) * get_hourly_rotpoints_at_temp(wit->second.temperature) ) / 600;
    }

    for( ;
         wit != g->weather_log.begin(); --wit
       ) {
       if ( wit->first <= endturn ) {
           tbegin = wit->first;
           if ( tbegin < since ) {
               tbegin = since;
               ret += ( (tend-tbegin) * get_hourly_rotpoints_at_temp(wit->second.temperature) );// / 600 ;
               tend = wit->first;
               break;
           }
           ret += ( (tend-tbegin) * get_hourly_rotpoints_at_temp(wit->second.temperature) );// / 600 ;
           tend = wit->first;
       }
    }
    if ( since < tbegin ) { // incomplete wlog
       ret += ( (tbegin-since) * get_hourly_rotpoints_at_temp(65) );
    }
    return ret / 600;
};

////// Funnels.
/*
 * mm/h of rain/acid for weather type (should move to weather_data)
 */
std::pair<int, int> rain_or_acid_level( const int wt )
{
    if ( wt == WEATHER_ACID_RAIN || wt == WEATHER_ACID_DRIZZLE ) {
        return std::make_pair(0, (wt == WEATHER_ACID_RAIN  ? 8 : 4 ));
    } else if (wt == WEATHER_DRIZZLE ) {
        return std::make_pair(4, 0);
        // why isnt this in weather data. now we have multiple rain/turn scales =[
    } else if ( wt ==  WEATHER_RAINY || wt == WEATHER_THUNDER || wt == WEATHER_LIGHTNING ) {
        return std::make_pair(8, 0);
        // todo; bucket of melted snow?
    } else {
        return std::make_pair(0, 0);
    }
}



void funnelcontainer_accumulates(int rain_depth_mm_per_hour,bool acid,int funnel_quality,item *fcontainer){

	const int charge_per_turn = rain_depth_mm_per_hour * funnel_quality;
	int accumulation_status = 0; //for passing the progress towards the next unit back and forth
	if (fcontainer->item_vars.find("funnel_accumulation") == fcontainer->item_vars.end() ) {
		//is it our first juice?
		fcontainer->item_vars["funnel_accumulation"] = string_format("%d", charge_per_turn);
		//debugmsg("void funnelcontainer_accumulates - first juice - add %d",charge_per_turn);
		//one turn's worth
		if(acid){ //firt time, we might not have a good value so test both.
			fcontainer->item_vars["accumulated_acid"] = "true";
		} else {
			fcontainer->item_vars["accumulated_acid"] = "false";
		}
	} else { //not first time, add juice, test for acid, is it enough for a charge?
		accumulation_status	= atoi ( fcontainer->item_vars["funnel_accumulation"].c_str());
		//pass accumulation to accumulation_status
		accumulation_status += charge_per_turn;
		//increment by additional charge_per_turn

		if(acid){
			fcontainer->item_vars["accumulated_acid"] = "true";
		} //before we test for a full charge, test for acid

		if(accumulation_status >= 1000000){ //if we have a full charge hanging...
			if(fcontainer->item_vars["accumulated_acid"]=="true"){ //acid test
			acid=true; //getting the boolean out of the string is harder
			} else if (fcontainer->item_vars["accumulated_acid"]=="false"){
			acid=false; //than getting the string out of the boolean
			} else {
			acid=false;
			//debugmsg("accumulated_acid invalid value in funnelcontainer_accumulates");
			} //end acid test

			//debugmsg("void funnelcontainer_accumulates - add %d", accumulation_status); //tell me about it

			fcontainer->add_rain_to_container(acid, 1); //the charge is added
			fcontainer->item_vars["accumulated_acid"]="false";
			//if we don't clean between charges, it will never be clean
			accumulation_status = 0;
			//yes I'm throwing out the partial charge_per_turn, what?
		}

		fcontainer->item_vars["funnel_accumulation"] = string_format("%d", accumulation_status);
		//update our juice count
		//debugmsg("void funnelcontainer_accumulates - add %d for %d",charge_per_turn, accumulation_status);
	}

}//end void funnelcontainer_accumulates

void fill_funnels(/*int rain_depth_mm_per_hour, bool acid,*/ trap_id t)
//fill funnels will call it's own weather data from std::pair rain or acid level or an intermediary
//calling function provides: rain intensity, acidity, funnel type
{
	const int funnel_quality = traplist[t]->funnel_value;
	  //cache this value

    int endturn = (int(calendar::turn));
	  //we're going to attempt to bring every funnel in the active area up to date this turn
      //since we're using weather log, we can't do this current turn though.

	std::string accumulated_acid = "error";
	  //what are we accumulating? Test for "error" before passing out to item vars. This confirms value was set properly.



	int startturn = endturn;
	  //initialize startturn to endturn in case something goes wrong

	bool acid = false;
      //a bool for passing around the acid portion of the std:pair rain_or_acid_level


// Give each funnel on the map a chance to collect the rain.
    std::set<point> funnel_locs = g->m.trap_locations(t);
	//collect a set of trap locations in the active map, call it funnel_locs
    std::set<point>::iterator iamafunnel;
	//set an iterator for counting though the funnel locs, call it iamafunnel



    for (iamafunnel = funnel_locs.begin(); iamafunnel != funnel_locs.end(); ++iamafunnel) {

		item *container = NULL; //setupp pointer to container items
        unsigned int maxcontains = 0; //
        point loc = *iamafunnel;  //pointer for storing location of current trap
        std::vector<item>& items = g->m.i_at(loc.x, loc.y);  //get the set of items under iamafunnel

            //add_msg("%d mm/h %d tps %.4f: fill",int(calendar::turn),rain_depth_mm_per_hour,turns_per_charge);
            // This funnel has collected some rain! Put the rain in the largest
            // container here which is either empty or contains some mixture of
            // impure water and acid.

		for (size_t j = 0; j < items.size(); j++) { //find our container
                item *it = &(items[j]); //increment through items under funnel
            if ( it->is_funnel_container( maxcontains ) ) {
                    container = it;
					//debugmsg("container under funnel");
            } //end if
            //else{debugmsg("invalid container");}
		} //end for loop finding our container

        if (container != NULL) {  //did we find a container?
            //debugmsg("container found in void fill_funnels");
			if ( container->item_vars.find("funnelcontainer_last_checked") == container->item_vars.end() ) {
			//is it our first check? Note the turn we checked it.
				container->item_vars["funnelcontainer_last_checked"] = string_format("%d", endturn);
				//debugmsg("funnelcontainer_last_checked not found, set to %d", endturn);
			}else {

			startturn = atoi ( container->item_vars["funnelcontainer_last_checked"].c_str());

            std::map<int, weather_segment>::iterator weather_iterator = g->weather_log.lower_bound(startturn);
            if(endturn > int(weather_iterator->first))
            {
                //debugmsg("endturn changed from %d to %d",endturn, weather_iterator->first);
                endturn = (weather_iterator->first)+1;  //end this round of funnel updates when the weather changes.

            }
            std::pair<int, int> weathercond = rain_or_acid_level( weather_iterator->second.weather );
            //debugmsg("Weather conditions for %d %d,%d",weather_iterator->first,weathercond.first,weathercond.second);
            if(weather_iterator->first > startturn)
            {
                     weather_iterator --; //select the weather span we want to use
            }
            weathercond = rain_or_acid_level( weather_iterator->second.weather );
            //debugmsg("Weather conditions for %d %d,%d",weather_iterator->first,weathercond.first,weathercond.second);

            if(weathercond.first != 0 || weathercond.second != 0) //if no rain, skip this
            {
                //debugmsg("its raining %d - %d",startturn,endturn);

                for(int turn_iterator = startturn; turn_iterator <= endturn;turn_iterator++)
                {
                	//debugmsg("turn_iterator %d, startturn %d, endturn %d",turn_iterator,startturn,endturn);
                        if ( weathercond.first != 0 )
                    {
                        //do water stuff
                        acid = false;
                        funnelcontainer_accumulates(weathercond.first,acid,funnel_quality,container);
                    } else if ( weathercond.second != 0 ) {
                        //do acid stuff
                        acid = true;
                        funnelcontainer_accumulates(weathercond.second,acid,funnel_quality,container);
                    }
                    else {
                        //do error stuff
                        //debugmsg("invalid acid value reported in void fill_funnels");
                    }
                } // end for turn_iterator
            }// end if norain
			container->item_vars["funnelcontainer_last_checked"] = string_format("%d",endturn);
			//debugmsg("funnelcontainer_last_checked set to %d", endturn);
			//We've processed a full span of weather, update the container so it doesn't do this span again next time.
			} // end if(no funnel_container_last_checked){}else{}
		} //end if container != null
	} //end for iamafunnel
} //end of void fill funnels

void fill_water_collectors()
{
    fill_funnels(tr_funnel);
    fill_funnels(tr_makeshift_funnel);
}// end  void fill_water_collectors()

void retroactively_fill_from_funnel( item *it, const trap_id t, const int endturn )
{
    fill_water_collectors();
}

// Add charge(s) of rain to given container, possibly contaminating it
void item::add_rain_to_container(bool acid, int charges)
{
    if( charges <= 0) {
        return;
    }
    const char *typeId = acid ? "water_acid" : "water";
    long max = dynamic_cast<it_container*>(type)->contains;
    long orig = 0;
    long added = charges;
    if (contents.empty()) {
        // This is easy. Just add 1 charge of the rain liquid to the container.
        item ret(typeId, 0);
        if (!acid) {
            // Funnels aren't always clean enough for water. // todo; disinfectant squeegie->funnel
            ret.poison = one_in(10) ? 1 : 0;
        }
        ret.charges = ( charges > max ? max : charges );
        put_in(ret);
    } else {
        // The container already has a liquid.
        item &liq = contents[0];
        orig = liq.charges;
        max -= liq.charges;
        added = ( charges > max ? max : charges );
        if (max > 0 ) {
            liq.charges += added;
        }

        if (liq.typeId() == typeId || liq.typeId() == "water_acid_weak") {
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
            const bool transmute = x_in_y(2*added, liq.charges);

            if (transmute) {
                item transmuted("water_acid_weak", 0);
                transmuted.charges = liq.charges;
                contents[0] = transmuted;
            } else if (liq.typeId() == "water") {
                // The container has water, and the acid rain didn't turn it
                // into weak acid. Poison the water instead, assuming 1
                // charge of acid would act like a charge of water with poison 5.
                long total_poison = liq.poison * (orig) + (5*added);
                liq.poison = total_poison / liq.charges;
                long leftover_poison = total_poison - liq.poison * liq.charges;
                if (leftover_poison > rng(0, liq.charges)) {
                    liq.poison++;
                }
            }
        }
    }
}


void decay_fire_and_scent(int fire_amount)
{
    for (int x = g->u.posx - SEEX * 2; x <= g->u.posx + SEEX * 2; x++) {
        for (int y = g->u.posy - SEEY * 2; y <= g->u.posy + SEEY * 2; y++) {
            if (g->m.is_outside(x, y)) {
                g->m.adjust_field_age(point(x, y), fd_fire, fire_amount);
                if (g->scent(x, y) > 0)
                g->scent(x, y)--;
            }
        }
    }
}

void generic_wet(bool acid)
{
    if ((!g->u.worn_with_flag("RAINPROOF") || one_in(100)) &&
         (!g->u.weapon.has_flag("RAIN_PROTECT") || one_in(20)) && !g->u.has_trait("FEATHERS") &&
         (g->u.warmth(bp_torso) * 4/5 + g->u.warmth(bp_head) / 5) < 30 && PLAYER_OUTSIDE &&
         one_in(2)) {
            if (g->u.weapon.has_flag("RAIN_PROTECT")) {
            // Umbrellas tend to protect one's head and torso pretty well
                g->u.drench(30 - (g->u.warmth(bp_legs) * 4/5 + g->u.warmth(bp_feet) / 5),
                     mfb(bp_legs));
            }
            else {
                g->u.drench(30 - (g->u.warmth(bp_torso) * 4/5 + g->u.warmth(bp_head) / 5),
                     mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head));
            }
    }

    fill_water_collectors(); // fixme; consolidate drench, this, and decay_fire_and_scent.
    decay_fire_and_scent(15);
}

void generic_very_wet(bool acid)
{
    if ((!g->u.worn_with_flag("RAINPROOF") || one_in(50)) &&
         (!g->u.weapon.has_flag("RAIN_PROTECT") || one_in(10)) && !g->u.has_trait("FEATHERS") &&
         (g->u.warmth(bp_torso) * 4/5 + g->u.warmth(bp_head) / 5) < 60 && PLAYER_OUTSIDE) {
            if (g->u.weapon.has_flag("RAIN_PROTECT")) {
            // Umbrellas tend to protect one's head and torso pretty well
                g->u.drench(60 - (g->u.warmth(bp_legs) * 4/5 + g->u.warmth(bp_feet) / 5),
                     mfb(bp_legs));
            }
            else {
                g->u.drench(60 - (g->u.warmth(bp_torso) * 4/5 + g->u.warmth(bp_head) / 5),
                     mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head));
            }
    }

    fill_water_collectors();
    decay_fire_and_scent(45);
}

void weather_effect::wet()
{
    generic_wet(false);
}

void weather_effect::very_wet()
{
    generic_very_wet(false);
}

void weather_effect::thunder()
{
    very_wet();
    if (one_in(THUNDER_CHANCE)) {
        if (g->levz >= 0) {
            add_msg(_("You hear a distant rumble of thunder."));
        } else if (g->u.has_trait("GOODHEARING") && one_in(1 - 2 * g->levz)) {
            add_msg(_("You hear a rumble of thunder from above."));
        } else if (!g->u.has_trait("BADHEARING") && one_in(1 - 3 * g->levz)) {
            add_msg(_("You hear a rumble of thunder from above."));
        }
    }
}

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

void weather_effect::light_acid()
{
    generic_wet(true);
    if (int(calendar::turn) % 10 == 0 && PLAYER_OUTSIDE) {
        if (g->u.weapon.has_flag("RAIN_PROTECT") && !one_in(3)) {
            add_msg(_("Your %s protects you from the acidic drizzle."), g->u.weapon.name.c_str());
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

    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->m.is_outside(g->zombie(i).posx(), g->zombie(i).posy())) {
            if (!g->zombie(i).has_flag(MF_ACIDPROOF)) {
                g->zombie(i).hurt(1);
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

std::string weather_forecast(radio_tower tower)
{
    std::stringstream weather_report;
    // Local conditions
    city *closest_city = &g->cur_om->cities[g->cur_om->closest_city(point(tower.x, tower.y))];
    // Current time
    weather_report << string_format(
        _("The current time is %s Eastern Standard Time.  At %s in %s, it was %s. The temperature was %s"),
        calendar::turn.print_time().c_str(), calendar::turn.print_time(true).c_str(), closest_city->name.c_str(),
        weather_data[g->weather].name.c_str(), print_temperature(g->temperature).c_str()
    );

    //weather_report << ", the dewpoint ???, and the relative humidity ???.  ";
    //weather_report << "The wind was <direction> at ? mi/km an hour.  ";
    //weather_report << "The pressure was ??? in/mm and steady/rising/falling.";

    // Regional conditions (simulated by chosing a random range containing the current conditions).
    // Adjusted for weather volatility based on how many weather changes are coming up.
    //weather_report << "Across <region>, skies ranged from <cloudiest> to <clearest>.  ";
    // TODO: Add fake reports for nearby cities

    // TODO: weather forecast
    // forecasting periods are divided into 12-hour periods, day (6-18) and night (19-5)
    // Accumulate percentages for each period of various weather statistics, and report that
    // (with fuzz) as the weather chances.
    int weather_proportions[NUM_WEATHER_TYPES] = {0};
    signed char high = 0;
    signed char low = 0;
    calendar start_time = calendar::turn;
    int period_start = calendar::turn.hours();
    // TODO wind direction and speed
    for(std::map<int, weather_segment>::iterator it = g->weather_log.lower_bound( int(calendar::turn) ); it != g->weather_log.end(); ++it ) {
        weather_segment * period = &(it->second);
        int period_deadline = period->deadline.hours();
        signed char period_temperature = period->temperature;
        weather_type period_weather = period->weather;
        bool start_day = period_start >= 6 && period_start <= 18;
        bool end_day = period_deadline >= 6 && period_deadline <= 18;

        high = std::max(high, period_temperature);
        low = std::min(low, period_temperature);

        if(start_day != end_day) // Assume a period doesn't last over 12 hrs?
        {
            weather_proportions[period_weather] += end_day ? 6 : 18 - period_start;
            int weather_duration = 0;
            int predominant_weather = 0;
            std::string day;
            if( calendar::turn.days() == period->deadline.days() )
            {
                if( start_day )
                {
                    day = _("Today");
                }
                else
                {
                    day = _("Tonight");
                }
            }
            else
            {
                std::string dayofweak = start_time.day_of_week();
                if( !start_day )
                {
                    day = rmp_format(_("<Mon Night>%s Night"), dayofweak.c_str());
                }
                else
                {
                    day = dayofweak;
                }
            }
            for( int i = WEATHER_CLEAR; i < NUM_WEATHER_TYPES; i++)
            {
                if( weather_proportions[i] > weather_duration)
                {
                    weather_duration = weather_proportions[i];
                    predominant_weather = i;
                }
            }
            // Print forecast
            weather_report << string_format(
                _("%s...%s. Highs of %s. Lows of %s. "),
                day.c_str(), weather_data[predominant_weather].name.c_str(),
                print_temperature(high).c_str(), print_temperature(low).c_str()
            );
            low = period_temperature;
            high = period_temperature;
            weather_proportions[period_weather] += end_day ? 6 : 18 - period_start;
        } else {
            weather_proportions[period_weather] += period_deadline - period_start;
        }
        start_time = period->deadline;
        period_start = period_deadline;
    }
    return weather_report.str();
}

std::string print_temperature(float fahrenheit, int decimals)
{
    std::stringstream ret;
    ret.precision(decimals);
    ret << std::fixed;

    if(OPTIONS["USE_CELSIUS"] == "celsius")
    {
        ret << ((fahrenheit-32) * 5 / 9);
        return rmp_format(_("<Celsius>%sC"), ret.str().c_str());
    }
    else
    {
        ret << fahrenheit;
        return rmp_format(_("<Fahrenheit>%sF"), ret.str().c_str());
    }

}
