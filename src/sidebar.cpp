#include "sidebar.h"

#include "player.h"
#include "string_formatter.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "weather.h"
#include "item.h"
#include "translations.h"
#include "color.h"
#include "cursesdef.h"
#include "martialarts.h"
#include "output.h"
#include "input.h"
#include "vehicle.h"
#include "cata_utility.h"

#include <iterator>

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#include <ctime>
#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <limits>

static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );

static std::string print_gun_mode( const player &p )
{
    auto m = p.weapon.gun_current_mode();
    if( m ) {
        if( m.melee() || !m->is_gunmod() ) {
            return string_format( m.mode.empty() ? "%s" : "%s (%s)",
                                  p.weapname().c_str(), _( m.mode.c_str() ) );
        } else {
            return string_format( "%s (%i/%i)", m->tname().c_str(),
                                  m->ammo_remaining(), m->ammo_capacity() );
        }
    } else {
        return p.weapname();
    }
}

void print_stamina_bar( const player &p, const catacurses::window &w )
{
    std::string sta_bar;
    nc_color sta_color;
    std::tie( sta_bar, sta_color ) = get_hp_bar( p.stamina, p.get_stamina_max() );
    wprintz( w, sta_color, sta_bar.c_str() );
}

void player::disp_status( const catacurses::window &w, const catacurses::window &w2 )
{
    bool sideStyle = use_narrow_sidebar();
    const catacurses::window &weapwin = sideStyle ? w2 : w;

    {
        const int y = sideStyle ? 1 : 0;
        const int wn = getmaxx( weapwin );
        trim_and_print( weapwin, y, 0, wn, c_light_gray, print_gun_mode( *this ) );
    }

    // Print currently used style or weapon mode.
    std::string style;
    const auto style_color = is_armed() ? c_red : c_blue;
    const auto &cur_style = style_selected.obj();
    if( cur_style.force_unarmed || cur_style.weapon_valid( weapon ) ) {
        style = cur_style.name;
    } else if( is_armed() ) {
        style = _( "Normal" );
    } else {
        style = _( "No Style" );
    }

    if( !style.empty() ) {
        int x = sideStyle ? ( getmaxx( weapwin ) - 13 ) : 0;
        mvwprintz( weapwin, 1, x, style_color, style.c_str() );
    }

    wmove( w, sideStyle ? 1 : 2, 0 );
    if( get_hunger() > 2800 ) {
        wprintz( w, c_red,    _( "Starving!" ) );
    } else if( get_hunger() > 1400 ) {
        wprintz( w, c_light_red,  _( "Near starving" ) );
    } else if( get_hunger() > 300 ) {
        wprintz( w, c_light_red,  _( "Famished" ) );
    } else if( get_hunger() > 100 ) {
        wprintz( w, c_yellow, _( "Very hungry" ) );
    } else if( get_hunger() > 40 ) {
        wprintz( w, c_yellow, _( "Hungry" ) );
    } else if( get_hunger() < -60 ) {
        wprintz( w, c_green,  _( "Engorged" ) );
    } else if( get_hunger() < -20 ) {
        wprintz( w, c_green,  _( "Sated" ) );
    } else if( get_hunger() < 0 ) {
        wprintz( w, c_green,  _( "Full" ) );
    }

    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    int current_bp_extreme = 0, conv_bp_extreme = 0;
    for( int i = 0; i < num_bp ; i++ ) {
        if( abs( temp_cur[i] - BODYTEMP_NORM ) > abs( temp_cur[current_bp_extreme] - BODYTEMP_NORM ) ) {
            current_bp_extreme = i;
        }
        if( abs( temp_conv[i] - BODYTEMP_NORM ) > abs( temp_conv[conv_bp_extreme] - BODYTEMP_NORM ) ) {
            conv_bp_extreme = i;
        }
    }

    // Assign zones for comparisons
    int cur_zone = 0, conv_zone = 0;
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        cur_zone = 7;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        cur_zone = 6;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        cur_zone = 5;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_COLD ) {
        cur_zone = 4;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        cur_zone = 3;
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        cur_zone = 2;
    } else if( temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        cur_zone = 1;
    }

    if( temp_conv[conv_bp_extreme] >  BODYTEMP_SCORCHING ) {
        conv_zone = 7;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        conv_zone = 6;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_HOT ) {
        conv_zone = 5;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_COLD ) {
        conv_zone = 4;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        conv_zone = 3;
    } else if( temp_conv[conv_bp_extreme] >  BODYTEMP_FREEZING ) {
        conv_zone = 2;
    } else if( temp_conv[conv_bp_extreme] <= BODYTEMP_FREEZING ) {
        conv_zone = 1;
    }

    // delta will be positive if temp_cur is rising
    int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    const char *temp_message = "Error";
    if( delta >   2 ) {
        temp_message = _( " (Rising!!)" );
    } else if( delta ==  2 ) {
        temp_message = _( " (Rising!)" );
    } else if( delta ==  1 ) {
        temp_message = _( " (Rising)" );
    } else if( delta ==  0 ) {
        temp_message = "";
    } else if( delta == -1 ) {
        temp_message = _( " (Falling)" );
    } else if( delta == -2 ) {
        temp_message = _( " (Falling!)" );
    } else {
        temp_message = _( " (Falling!!)" );
    }

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    wmove( w, sideStyle ? 6 : 1, sideStyle ? 0 : 9 );
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        wprintz( w, c_red,   _( "Scorching!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        wprintz( w, c_light_red, _( "Very hot!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        wprintz( w, c_yellow, _( "Warm%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >
               BODYTEMP_COLD ) { // If you're warmer than cold, you are comfortable
        wprintz( w, c_green, _( "Comfortable%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        wprintz( w, c_light_blue, _( "Chilly%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        wprintz( w, c_cyan,  _( "Very cold!%s" ), temp_message );
    } else if( temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        wprintz( w, c_blue,  _( "Freezing!%s" ), temp_message );
    }

    int x = 32;
    int y = sideStyle ?  0 :  1;
    if( is_deaf() ) {
        mvwprintz( sideStyle ? w2 : w, y, x, c_red, _( "Deaf!" ) );
    } else {
        mvwprintz( sideStyle ? w2 : w, y, x, c_yellow, _( "Sound %d" ), volume );
    }
    volume = 0;

    wmove( w, 2, sideStyle ? 0 : 15 );
    if( get_thirst() > 520 ) {
        wprintz( w, c_light_red,  _( "Parched" ) );
    } else if( get_thirst() > 240 ) {
        wprintz( w, c_light_red,  _( "Dehydrated" ) );
    } else if( get_thirst() > 80 ) {
        wprintz( w, c_yellow, _( "Very thirsty" ) );
    } else if( get_thirst() > 40 ) {
        wprintz( w, c_yellow, _( "Thirsty" ) );
    } else if( get_thirst() < -60 ) {
        wprintz( w, c_green,  _( "Turgid" ) );
    } else if( get_thirst() < -20 ) {
        wprintz( w, c_green,  _( "Hydrated" ) );
    } else if( get_thirst() < 0 ) {
        wprintz( w, c_green,  _( "Slaked" ) );
    }

    wmove( w, sideStyle ? 3 : 2, sideStyle ? 0 : 30 );
    if( get_fatigue() > EXHAUSTED ) {
        wprintz( w, c_red,    _( "Exhausted" ) );
    } else if( get_fatigue() > DEAD_TIRED ) {
        wprintz( w, c_light_red,  _( "Dead tired" ) );
    } else if( get_fatigue() > TIRED ) {
        wprintz( w, c_yellow, _( "Tired" ) );
    }

    wmove( w, sideStyle ? 4 : 2, sideStyle ? 0 : 41 );
    wprintz( w, c_white, _( "Focus" ) );
    nc_color col_xp = c_dark_gray;
    if( focus_pool >= 100 ) {
        col_xp = c_white;
    } else if( focus_pool >  0 ) {
        col_xp = c_light_gray;
    }
    wprintz( w, col_xp, " %d", focus_pool );

    nc_color col_pain = c_yellow;
    if( get_perceived_pain() >= 60 ) {
        col_pain = c_red;
    } else if( get_perceived_pain() >= 40 ) {
        col_pain = c_light_red;
    }
    if( get_perceived_pain() > 0 ) {
        mvwprintz( w, sideStyle ? 0 : 3, 0, col_pain, _( "Pain %d" ), get_perceived_pain() );
    }

    int morale_cur = get_morale_level();
    nc_color col_morale = c_white;
    if( morale_cur >= 10 ) {
        col_morale = c_green;
    } else if( morale_cur <= -10 ) {
        col_morale = c_red;
    }
    const char *morale_str;
    if( get_option<std::string>( "MORALE_STYLE" ) == "horizontal" ) {
        if( has_trait( trait_THRESH_FELINE ) || has_trait( trait_THRESH_URSINE ) ) {
            if( morale_cur >= 200 ) {
                morale_str = "@W@";
            } else if( morale_cur >= 100 ) {
                morale_str = "OWO";
            } else if( morale_cur >= 50 ) {
                morale_str = "owo";
            } else if( morale_cur >= 10 ) {
                morale_str = "^w^";
            } else if( morale_cur >= -10 ) {
                morale_str = "-w-";
            } else if( morale_cur >= -50 ) {
                morale_str = "-m-";
            } else if( morale_cur >= -100 ) {
                morale_str = "TmT";
            } else if( morale_cur >= -200 ) {
                morale_str = "XmX";
            } else {
                morale_str = "@m@";
            }
        } else if( has_trait( trait_THRESH_BIRD ) ) {
            if( morale_cur >= 200 ) {
                morale_str = "@v@";
            } else if( morale_cur >= 100 ) {
                morale_str = "OvO";
            } else if( morale_cur >= 50 ) {
                morale_str = "ovo";
            } else if( morale_cur >= 10 ) {
                morale_str = "^v^";
            } else if( morale_cur >= -10 ) {
                morale_str = "-v-";
            } else if( morale_cur >= -50 ) {
                morale_str = ".v.";
            } else if( morale_cur >= -100 ) {
                morale_str = "TvT";
            } else if( morale_cur >= -200 ) {
                morale_str = "XvX";
            } else {
                morale_str = "@v@";
            }
        } else if( morale_cur >= 200 ) {
            morale_str = "@U@";
        } else if( morale_cur >= 100 ) {
            morale_str = "OuO";
        } else if( morale_cur >= 50 ) {
            morale_str = "^u^";
        } else if( morale_cur >= 10 ) {
            morale_str = "n_n";
        } else if( morale_cur >= -10 ) {
            morale_str = "-_-";
        } else if( morale_cur >= -50 ) {
            morale_str = "-n-";
        } else if( morale_cur >= -100 ) {
            morale_str = "TnT";
        } else if( morale_cur >= -200 ) {
            morale_str = "XnX";
        } else {
            morale_str = "@n@";
        }
    } else if( morale_cur >= 100 ) {
        morale_str = "8D";
    } else if( morale_cur >= 50 ) {
        morale_str = ":D";
    } else if( has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":3";
    } else if( !has_trait( trait_THRESH_FELINE ) && morale_cur >= 10 ) {
        morale_str = ":)";
    } else if( morale_cur >= -10 ) {
        morale_str = ":|";
    } else if( morale_cur >= -50 ) {
        morale_str = "):";
    } else if( morale_cur >= -100 ) {
        morale_str = "D:";
    } else {
        morale_str = "D8";
    }
    mvwprintz( w, sideStyle ? 0 : 3, sideStyle ? 11 : 9, col_morale, morale_str );

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = g->m.veh_at( pos() );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, sideStyle ? 2 : 3, sideStyle ? getmaxx( w ) - 5 : 49 );
        nc_color col_indf1 = c_light_gray;

        float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        // Draw the speedometer.
        int speedox = sideStyle ? 0 : 28;
        int speedoy = sideStyle ? 5 :  3;

        bool metric = get_option<std::string>( "USE_METRIC_SPEEDS" ) == "km/h";
        // Logic below is not applicable to translated units and should be changed
        int velx    = metric ? 4 : 3; // strlen(units) + 1
        int cruisex = metric ? 9 : 8; // strlen(units) + 6

        if( !sideStyle ) {
            if( !veh->cruise_on ) {
                speedox += 2;
            }
            if( !metric ) {
                speedox++;
            }
        }

        const char *speedo = veh->cruise_on ? "%s....>...." : "%s....";
        mvwprintz( w, speedoy, speedox,        col_indf1, speedo, velocity_units( VU_VEHICLE ) );
        mvwprintz( w, speedoy, speedox + velx, col_vel,   "%4d",
                   int( convert_velocity( veh->velocity, VU_VEHICLE ) ) );
        if( veh->cruise_on ) {
            mvwprintz( w, speedoy, speedox + cruisex, c_light_green, "%4d",
                       int( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) ) );
        }

        const int vel_offset = 11 + ( veh->velocity != 0 ? 2 : 0 );
        wmove( w, sideStyle ? 4 : 3, getmaxx( w ) - vel_offset );
        if( veh->velocity != 0 ) {
            nc_color col_indc = veh->skidding ? c_red : c_green;
            int dfm = veh->face.dir() - veh->move.dir();

            if( dfm == 0 ) {
                wprintz( w, col_indc, "^" );
            } else if( dfm < 0 ) {
                wprintz( w, col_indc, "<" );
            } else {
                wprintz( w, col_indc, ">" );
            }

            wprintz( w, c_white, " " );
        }

        //Vehicle direction indicator in 0-359° where 0 is north (veh->face.dir() 0° is west)
        wprintz( w, c_white, string_format( "%3d°", ( veh->face.dir() + 90 ) % 360 ).c_str() );

        if( sideStyle ) {
            // Make sure this is left-aligned.
            mvwprintz( w, speedoy, getmaxx( w ) - 9, c_white, "%s", _( "Stm " ) );
            print_stamina_bar( *this, w );
        }
    } else {  // Not in vehicle
        nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
                 col_per = c_white, col_spd = c_white, col_time = c_white;
        int str_bonus = get_str_bonus();
        int dex_bonus = get_dex_bonus();
        int int_bonus = get_int_bonus();
        int per_bonus = get_per_bonus();
        int spd_bonus = get_speed_bonus();
        if( str_bonus < 0 ) {
            col_str = c_red;
        }
        if( str_bonus > 0 ) {
            col_str = c_green;
        }
        if( dex_bonus  < 0 ) {
            col_dex = c_red;
        }
        if( dex_bonus  > 0 ) {
            col_dex = c_green;
        }
        if( int_bonus  < 0 ) {
            col_int = c_red;
        }
        if( int_bonus  > 0 ) {
            col_int = c_green;
        }
        if( per_bonus  < 0 ) {
            col_per = c_red;
        }
        if( per_bonus  > 0 ) {
            col_per = c_green;
        }
        if( spd_bonus < 0 ) {
            col_spd = c_red;
        }
        if( spd_bonus > 0 ) {
            col_spd = c_green;
        }

        int wx  = sideStyle ? 18 : 12;
        int wy  = sideStyle ?  0 :  3;
        int dx = sideStyle ?  0 :  7;
        int dy = sideStyle ?  1 :  0;
        mvwprintz( w, wy + dy * 0, wx + dx * 0, col_str, _( "Str %d" ), str_cur );
        mvwprintz( w, wy + dy * 1, wx + dx * 1, col_dex, _( "Dex %d" ), dex_cur );
        mvwprintz( w, wy + dy * 2, wx + dx * 2, col_int, _( "Int %d" ), int_cur );
        mvwprintz( w, wy + dy * 3, wx + dx * 3, col_per, _( "Per %d" ), per_cur );

        int spdx = sideStyle ?  0 : wx + dx * 4 + 1;
        int spdy = sideStyle ?  5 : wy + dy * 4;
        mvwprintz( w, spdy, spdx, col_spd, _( "Spd %d" ), get_speed() );
        if( this->weight_carried() > this->weight_capacity() ) {
            col_time = h_black;
        }
        if( this->volume_carried() > this->volume_capacity() ) {
            if( this->weight_carried() > this->weight_capacity() ) {
                col_time = c_dark_gray_magenta;
            } else {
                col_time = c_dark_gray_red;
            }
        }
        wprintz( w, col_time, " %d", movecounter );

        //~ Movement type: "walking". Max string length: one letter.
        const auto str_walk = pgettext( "movement-type", "W" );
        //~ Movement type: "running". Max string length: one letter.
        const auto str_run = pgettext( "movement-type", "R" );
        wprintz( w, c_white, " %s", move_mode == "walk" ? str_walk : str_run );
        if( sideStyle ) {
            mvwprintz( w, spdy, wx + dx * 4 - 3, c_white, _( "Stm " ) );
            print_stamina_bar( *this, w );
        }
    }
}


