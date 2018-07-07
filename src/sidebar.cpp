#include "sidebar.h"

#include "player.h"
#include "string_formatter.h"
#include "effect.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "gun_mode.h"
#include "weather.h"
#include "item.h"
#include "translations.h"
#include "vpart_position.h"
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

static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );

enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

std::string morale_emotion( const int morale_cur, const face_type face,
                            const bool horizontal_style )
{
    if( horizontal_style ) {
        if( face == face_bear || face == face_cat ) {
            if( morale_cur >= 200 ) {
                return "@W@";
            } else if( morale_cur >= 100 ) {
                return "OWO";
            } else if( morale_cur >= 50 ) {
                return "owo";
            } else if( morale_cur >= 10 ) {
                return "^w^";
            } else if( morale_cur >= -10 ) {
                return "-w-";
            } else if( morale_cur >= -50 ) {
                return "-m-";
            } else if( morale_cur >= -100 ) {
                return "TmT";
            } else if( morale_cur >= -200 ) {
                return "XmX";
            } else {
                return "@m@";
            }
        } else if( face == face_bird ) {
            if( morale_cur >= 200 ) {
                return "@v@";
            } else if( morale_cur >= 100 ) {
                return "OvO";
            } else if( morale_cur >= 50 ) {
                return "ovo";
            } else if( morale_cur >= 10 ) {
                return "^v^";
            } else if( morale_cur >= -10 ) {
                return "-v-";
            } else if( morale_cur >= -50 ) {
                return ".v.";
            } else if( morale_cur >= -100 ) {
                return "TvT";
            } else if( morale_cur >= -200 ) {
                return "XvX";
            } else {
                return "@v@";
            }
        } else if( morale_cur >= 200 ) {
            return "@U@";
        } else if( morale_cur >= 100 ) {
            return "OuO";
        } else if( morale_cur >= 50 ) {
            return "^u^";
        } else if( morale_cur >= 10 ) {
            return "n_n";
        } else if( morale_cur >= -10 ) {
            return "-_-";
        } else if( morale_cur >= -50 ) {
            return "-n-";
        } else if( morale_cur >= -100 ) {
            return "TnT";
        } else if( morale_cur >= -200 ) {
            return "XnX";
        } else {
            return "@n@";
        }
    } else if( morale_cur >= 100 ) {
        return "8D";
    } else if( morale_cur >= 50 ) {
        return ":D";
    } else if( face == face_cat && morale_cur >= 10 ) {
        return ":3";
    } else if( face != face_cat && morale_cur >= 10 ) {
        return ":)";
    } else if( morale_cur >= -10 ) {
        return ":|";
    } else if( morale_cur >= -50 ) {
        return "):";
    } else if( morale_cur >= -100 ) {
        return "D:";
    } else {
        return "D8";
    }
}

void draw_HP( const player &p, const catacurses::window &w_HP )
{
    werase( w_HP );

    // The HP window can be in "tall" mode (7x14) or "wide" mode (14x7).
    const bool wide = ( getmaxy( w_HP ) == 7 );
    const int hpx = wide ? 7 : 0;
    const int hpy = wide ? 0 : 1;
    const int dy = wide ? 1 : 2;

    bool const is_self_aware = p.has_trait( trait_SELFAWARE );

    for( int i = 0; i < num_hp_parts; i++ ) {
        wmove( w_HP, i * dy + hpy, hpx );

        static auto print_symbol_num = []( const catacurses::window & w, int num, const std::string & sym,
        const nc_color color ) {
            while( num-- > 0 ) {
                wprintz( w, color, sym.c_str() );
            }
        };

        if( p.hp_cur[i] == 0 && ( i >= hp_arm_l && i <= hp_leg_r ) ) {
            //Limb is broken
            std::string limb = "~~%~~";
            nc_color color = c_light_red;

            const auto bp = p.hp_to_bp( static_cast<hp_part>( i ) );
            if( p.worn_with_flag( "SPLINT", bp ) ) {
                static const efftype_id effect_mending( "mending" );
                const auto &eff = p.get_effect( effect_mending, bp );
                const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                if( is_self_aware ) {
                    limb = string_format( "=%2d%%=", mend_perc );
                    color = c_blue;
                } else {
                    const int num = mend_perc / 20;
                    print_symbol_num( w_HP, num, "#", c_blue );
                    print_symbol_num( w_HP, 5 - num, "=", c_blue );
                    continue;
                }
            }

            wprintz( w_HP, color, limb );
            continue;
        }

        auto const &hp = get_hp_bar( p.hp_cur[i], p.hp_max[i] );

        if( is_self_aware ) {
            wprintz( w_HP, hp.second, "%3d  ", p.hp_cur[i] );
        } else {
            wprintz( w_HP, hp.second, hp.first );

            //Add the trailing symbols for a not-quite-full health bar
            print_symbol_num( w_HP, 5 - ( int )hp.first.size(), ".", c_white );
        }
    }

    static std::array<body_part, 7> part = {{
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r, num_bp
        }
    };
    for( size_t i = 0; i < part.size(); i++ ) {
        const std::string str = ( i == part.size() - 1 ) ?
                                _( "POWER" ) : body_part_hp_bar_ui_text( part[i] );
        wmove( w_HP, i * dy, 0 );
        if( wide ) {
            wprintz( w_HP, p.limb_color( part[i], true, true, true ), " " );
        }
        wprintz( w_HP, p.limb_color( part[i], true, true, true ), str );
        if( !wide ) {
            wprintz( w_HP, p.limb_color( part[i], true, true, true ), ":" );
        }
    }

    const int powx = hpx;
    const int powy = wide ? 6 : 13;
    if( p.max_power_level == 0 ) {
        wmove( w_HP, powy, powx );
        if( wide )
            for( int i = 0; i < 2; i++ ) {
                wputch( w_HP, c_light_gray, LINE_OXOX );
            } else {
            wprintz( w_HP, c_light_gray, " --   " );
        }
    } else {
        nc_color color = c_red;
        if( p.power_level == p.max_power_level ) {
            color = c_blue;
        } else if( p.power_level >= p.max_power_level * .5 ) {
            color = c_light_blue;
        } else if( p.power_level > 0 ) {
            color = c_yellow;
        }
        mvwprintz( w_HP, powy, powx, color, "%-3d", p.power_level );
    }
    if( !wide ) {
        mvwprintz( w_HP, 14, hpx, c_white, "%s", _( "Stm" ) );
        wmove( w_HP, 15, hpx );
        print_stamina_bar( p, w_HP );
    }
    wrefresh( w_HP );
}

static std::string print_gun_mode( const player &p )
{
    auto m = p.weapon.gun_current_mode();
    if( m ) {
        if( m.melee() || !m->is_gunmod() ) {
            return string_format( m.name().empty() ? "%s" : "%s (%s)",
                                  p.weapname().c_str(), m.name() );
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
    wprintz( w, sta_color, sta_bar );
}

int define_temp_level( const int lvl )
{
    if( lvl > BODYTEMP_SCORCHING ) {
        return 7;
    } else if( lvl > BODYTEMP_VERY_HOT ) {
        return 6;
    } else if( lvl > BODYTEMP_HOT ) {
        return 5;
    } else if( lvl > BODYTEMP_COLD ) {
        return 4;
    } else if( lvl > BODYTEMP_VERY_COLD ) {
        return 3;
    } else if( lvl > BODYTEMP_FREEZING ) {
        return 2;
    }
    return 1;
}

nc_color stat_color( const int bonus )
{
    if( bonus > 0 ) {
        return c_green;
    } else if( bonus < 0 ) {
        return c_red;
    }
    return c_white;
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
    const auto &cur_style = style_selected.obj();
    if( cur_style.force_unarmed || cur_style.weapon_valid( weapon ) ) {
        style = _( cur_style.name.c_str() );
    } else if( is_armed() ) {
        style = _( "Normal" );
    } else {
        style = _( "No Style" );
    }

    if( !style.empty() ) {
        const auto style_color = is_armed() ? c_red : c_blue;
        const int x = sideStyle ? ( getmaxx( weapwin ) - 13 ) : 0;
        mvwprintz( weapwin, 1, x, style_color, style );
    }

    wmove( w, sideStyle ? 1 : 2, 0 );
    if( get_hunger() >= 300 && get_starvation() > 2500 ) {
        wprintz( w, c_red,    _( "Starving!" ) );
    } else if( get_hunger() >= 300 && get_starvation() > 1100 ) {
        wprintz( w, c_light_red,  _( "Near starving" ) );
    } else if( get_hunger() > 250 ) {
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
    int current_bp_extreme = 0;
    int conv_bp_extreme = 0;
    for( int i = 0; i < num_bp ; i++ ) {
        if( abs( temp_cur[i] - BODYTEMP_NORM ) > abs( temp_cur[current_bp_extreme] - BODYTEMP_NORM ) ) {
            current_bp_extreme = i;
        }
        if( abs( temp_conv[i] - BODYTEMP_NORM ) > abs( temp_conv[conv_bp_extreme] - BODYTEMP_NORM ) ) {
            conv_bp_extreme = i;
        }
    }

    // Assign zones for comparisons
    const int cur_zone = define_temp_level( temp_cur[current_bp_extreme] );
    const int conv_zone = define_temp_level( temp_conv[conv_bp_extreme] );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    const char *temp_message;
    if( delta > 2 ) {
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

    const int x = 32;
    const int y = sideStyle ?  0 :  1;
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

    const int morale_cur = get_morale_level();
    nc_color col_morale = c_white;
    if( morale_cur >= 10 ) {
        col_morale = c_green;
    } else if( morale_cur <= -10 ) {
        col_morale = c_red;
    }

    face_type fc = face_human;
    if( has_trait( trait_THRESH_FELINE ) ) {
        fc = face_cat;
    } else if( has_trait( trait_THRESH_URSINE ) ) {
        fc = face_bear;
    } else if( has_trait( trait_THRESH_BIRD ) ) {
        fc = face_bird;
    }

    mvwprintz( w, sideStyle ? 0 : 3, sideStyle ? 11 : 9, col_morale,
               morale_emotion( morale_cur, fc, get_option<std::string>( "MORALE_STYLE" ) == "horizontal" ) );

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, sideStyle ? 2 : 3, sideStyle ? getmaxx( w ) - 5 : 49 );
        nc_color col_indf1 = c_light_gray;

        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        // Draw the speedometer.
        int speedox = sideStyle ? 0 : 28;
        int speedoy = sideStyle ? 5 :  3;

        const bool metric = get_option<std::string>( "USE_METRIC_SPEEDS" ) == "km/h";
        // @todo: Logic below is not applicable to translated units and should be changed
        const int velx    = metric ? 4 : 3; // strlen(units) + 1
        const int cruisex = metric ? 9 : 8; // strlen(units) + 6

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
        wprintz( w, c_white, "%3d°", ( veh->face.dir() + 90 ) % 360 );

        if( sideStyle ) {
            // Make sure this is left-aligned.
            mvwprintz( w, speedoy, getmaxx( w ) - 9, c_white, "%s", _( "Stm " ) );
            print_stamina_bar( *this, w );
        }
    } else {  // Not in vehicle
        const int wx = sideStyle ? 18 : 12;
        const int wy = sideStyle ?  0 :  3;
        const int dx = sideStyle ?  0 :  7;
        const int dy = sideStyle ?  1 :  0;
        mvwprintz( w, wy + dy * 0, wx + dx * 0, stat_color( get_str_bonus() ), _( "Str %d" ), str_cur );
        mvwprintz( w, wy + dy * 1, wx + dx * 1, stat_color( get_dex_bonus() ), _( "Dex %d" ), dex_cur );
        mvwprintz( w, wy + dy * 2, wx + dx * 2, stat_color( get_int_bonus() ), _( "Int %d" ), int_cur );
        mvwprintz( w, wy + dy * 3, wx + dx * 3, stat_color( get_per_bonus() ), _( "Per %d" ), per_cur );

        const int spdx = sideStyle ?  0 : wx + dx * 4 + 1;
        const int spdy = sideStyle ?  5 : wy + dy * 4;
        mvwprintz( w, spdy, spdx, stat_color( get_speed_bonus() ), _( "Spd %d" ), get_speed() );

        nc_color col_time = c_white;
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

