#include "sidebar.h"

#include <cmath>
#include <cstdlib>
#include <string>

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "effect.h"
#include "game.h"
#include "gun_mode.h"
#include "item.h"
#include "map.h"
#include "martialarts.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "string_formatter.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"

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

    const bool is_self_aware = p.has_trait( trait_SELFAWARE );

    for( int i = 0; i < num_hp_parts; i++ ) {
        wmove( w_HP, i * dy + hpy, hpx );

        static auto print_symbol_num = []( const catacurses::window & w, int num, const std::string & sym,
        const nc_color & color ) {
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

        const auto &hp = get_hp_bar( p.hp_cur[i], p.hp_max[i] );

        if( is_self_aware ) {
            wprintz( w_HP, hp.second, "%3d  ", p.hp_cur[i] );
        } else {
            wprintz( w_HP, hp.second, hp.first );

            //Add the trailing symbols for a not-quite-full health bar
            print_symbol_num( w_HP, 5 - static_cast<int>( hp.first.size() ), ".", c_white );
        }
    }

    // display limbs status
    static std::array<body_part, 7> part = {{
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r, num_bp
        }
    };
    for( size_t i = 0; i < part.size(); i++ ) {
        const std::string str = body_part_hp_bar_ui_text( part[i] );
        wmove( w_HP, i * dy, 0 );
        if( wide ) {
            wprintz( w_HP, p.limb_color( part[i], true, true, true ), " " );
        }
        wprintz( w_HP, p.limb_color( part[i], true, true, true ), str );
        if( !wide && !( i == part.size() - 1 ) ) {
            wprintz( w_HP, p.limb_color( part[i], true, true, true ), ":" );
        }
    }
    // display stamina
    if( wide ) {
        wmove( w_HP, 7 * dy, 0 );
        wprintz( w_HP, c_light_gray, _( "STA   " ) );
        print_stamina_bar( p, w_HP );
    }
    if( !wide ) {
        wmove( w_HP, 12, hpx );
        wprintz( w_HP, c_light_gray, _( "STA:" ) );
        wmove( w_HP, 13, hpx );
        print_stamina_bar( p, w_HP );
    }
    wrefresh( w_HP );
}

static std::string print_gun_mode( const player &p )
{
    auto m = p.weapon.gun_current_mode();
    if( m ) {
        if( m.melee() || !m->is_gunmod() ) {
            if( p.ammo_location && p.weapon.can_reload_with( p.ammo_location->typeId() ) ) {
                return string_format( "%s (%d)", p.weapname().c_str(),
                                      p.ammo_location->charges );
            }
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
    std::tie( sta_bar, sta_color ) = get_stamina_bar( p.stamina, p.get_stamina_max() );
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
        // sidebar display currently used weapon.
        const int y = 0;
        const int wn = getmaxx( weapwin );
        trim_and_print( weapwin, y, 0, wn, c_light_gray, print_gun_mode( *this ) );
    }

    // Print in sidebar currently used martial style.
    std::string style;
    const auto &cur_style = style_selected.obj();
    if( !weapon.is_gun() ) {
        if( cur_style.force_unarmed || cur_style.weapon_valid( weapon ) ) {
            style = _( cur_style.name.c_str() );
        } else if( is_armed() ) {
            style = _( "Normal" );
        } else {
            style = _( "No Style" );
        }
    }

    if( !style.empty() ) {
        const auto style_color = is_armed() ? c_red : c_blue;
        const int x = sideStyle ? ( getmaxx( weapwin ) - 13 ) : ( getmaxx( weapwin ) - 12 );
        mvwprintz( weapwin, 0, x, style_color, style );
    }

    std::string hunger_string = "";
    nc_color hunger_color = c_yellow;
    if( get_hunger() >= 300 && get_starvation() > 2500 ) {
        hunger_color = c_red;
        hunger_string = _( "Starving!" );
    } else if( get_hunger() >= 300 && get_starvation() > 1100 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Near starving" );
    } else if( get_hunger() > 250 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Famished" );
    } else if( get_hunger() > 100 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Very hungry" );
    } else if( get_hunger() > 40 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Hungry" );
    } else if( get_hunger() < -60 ) {
        hunger_color = c_green;
        hunger_string = _( "Engorged" );
    } else if( get_hunger() < -20 ) {
        hunger_color = c_green;
        hunger_string = _( "Sated" );
    } else if( get_hunger() < 0 ) {
        hunger_color = c_green;
        hunger_string = _( "Full" );
    }
    mvwprintz( sideStyle ? w : g->w_location_wider,
               sideStyle ? 1 : 2, sideStyle ? 0 : 22, hunger_color, hunger_string );

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
    wmove( w, sideStyle ? 6 : 1, sideStyle ? 0 : 22 );
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

    std::string hydration_string = "";
    nc_color hydration_color = c_yellow;
    if( get_thirst() > 520 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Parched" );
    } else if( get_thirst() > 240 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Dehydrated" );
    } else if( get_thirst() > 80 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Very thirsty" );
    } else if( get_thirst() > 40 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Thirsty" );
    } else if( get_thirst() < -60 ) {
        hydration_color = c_green;
        hydration_string = _( "Turgid" );
    } else if( get_thirst() < -20 ) {
        hydration_color = c_green;
        hydration_string = _( "Hydrated" );
    } else if( get_thirst() < 0 ) {
        hydration_color = c_green;
        hydration_string = _( "Slaked" );
    }
    mvwprintz( sideStyle ? w : g->w_location_wider,
               sideStyle ? 2 : 1, sideStyle ? 0 : 22, hydration_color, hydration_string );
    wrefresh( sideStyle ? w : g->w_location_wider );

    wmove( w, sideStyle ? 3 : 2, sideStyle ? 0 : 22 );
    if( get_fatigue() > EXHAUSTED ) {
        wprintz( w, c_red,    _( "Exhausted" ) );
    } else if( get_fatigue() > DEAD_TIRED ) {
        wprintz( w, c_light_red,  _( "Dead tired" ) );
    } else if( get_fatigue() > TIRED ) {
        wprintz( w, c_yellow, _( "Tired" ) );
    }

    wmove( w, sideStyle ? 4 : 2, sideStyle ? 0 : 43 );
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

    if( has_trait( trait_SELFAWARE ) && get_perceived_pain() > 0 ) {
        mvwprintz( w, sideStyle ? 0 : 3, 0, col_pain, _( "Pain %d" ), get_perceived_pain() );
    } else if( get_perceived_pain() > 0 ) {
        mvwprintz( w, sideStyle ? 0 : 2, 0, col_pain, get_pain_description() );
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

    // display mood smiley
    const bool mood_style_hor = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    int mood_x = 0;
    if( sideStyle ) {
        if( mood_style_hor ) {
            //Currently, mood is next to current temp but we have space
            mood_x = getmaxx( w ) - 3;
        } else {
            mood_x = getmaxx( w ) - 2;
        }
    }
    mvwprintz( w, sideStyle ? 6 : 3, mood_x, col_morale,
               morale_emotion( morale_cur, fc, mood_style_hor ) );

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, sideStyle ? 5 : 3, sideStyle ? getmaxx( w ) - 5 : 49 );
        nc_color col_indf1 = c_light_gray;

        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        // Draw the speedometer.
        int speedox = sideStyle ? 0 : 28;
        int speedoy = sideStyle ? 5 :  3;

        const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
        const bool metric = type == "km/h";
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
        mvwprintz( w, speedoy, speedox + velx, col_vel,   type == "t/t" ? "%4.1f" : "%4.0f",
                   convert_velocity( veh->velocity, VU_VEHICLE ) );
        if( veh->cruise_on ) {
            mvwprintz( w, speedoy, speedox + cruisex, c_light_green, "%4d",
                       int( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) ) );
        }

        const int vel_offset = 11 + ( veh->is_moving() ? 2 : 0 );
        wmove( w, sideStyle ? 4 : 3, getmaxx( w ) - vel_offset );
        if( veh->is_moving() ) {
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
        wmove( w, sideStyle ? 4 : 3, getmaxx( w ) - ( sideStyle ? 14 : 12 ) );
        //Vehicle direction indicator in 0-359° where 0 is north (veh->face.dir() 0° is west)
        wprintz( w, c_white, "%3d°", ( veh->face.dir() + 90 ) % 360 );

    } else {  // Not in vehicle
        const int wx = sideStyle ? 18 :  0;
        const int wy = 0;
        const int dx = 0;
        const int dy = sideStyle ?  1 :  8;
        mvwprintz( sideStyle ? w : g->w_HP, sideStyle ? ( wy + dy * 0 ) : 17,
                   sideStyle ? ( wx + dx * 0 ) - 1 : wx + dx * 0, stat_color( get_str_bonus() ),
                   _( "Str  %02d" ), str_cur );

        mvwprintz( sideStyle ? w : g->w_HP, sideStyle ? ( wy + dy * 1 ) : 18,
                   sideStyle ? ( wx + dx * 1 ) - 1 : wx + dx * 1, stat_color( get_dex_bonus() ),
                   _( "Dex  %02d" ), dex_cur );

        mvwprintz( sideStyle ? w : g->w_HP, sideStyle ? ( wy + dy * 2 ) : 19,
                   sideStyle ? ( wx + dx * 2 ) - 1 : wx + dx * 2, stat_color( get_int_bonus() ),
                   _( "Int  %02d" ), int_cur );

        mvwprintz( sideStyle ? w : g->w_HP, sideStyle ? ( wy + dy * 3 ) : 20,
                   sideStyle ? ( wx + dx * 3 ) - 1 : wx + dx * 3, stat_color( get_per_bonus() ),
                   _( "Per  %02d" ), per_cur );

        const int spdx = sideStyle ?  0 : getmaxx( w ) - 12;
        const int spdy = sideStyle ?  5 : wy + dy * 4;
        mvwprintz( w, sideStyle ? spdy : 3, sideStyle ? spdx : 43,
                   stat_color( get_speed_bonus() ), _( "Spd %d" ), get_speed() );

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
    }
    // display power level
    wmove( sideStyle ? w : g->w_HP,
           sideStyle ? 4 : 21,
           sideStyle ? 17 : 0 );

    wprintz( sideStyle ? w : g->w_HP, c_white, _( "Pwr " ) );

    if( this->max_power_level == 0 ) {
        wprintz( sideStyle ? w : g->w_HP, c_light_gray, " --" );
    } else {
        nc_color color = c_red;
        if( this->power_level >= this->max_power_level / 2 ) {
            color = c_green;
        } else if( this->power_level >= this->max_power_level / 3 ) {
            color = c_yellow;
        } else if( this->power_level >= this->max_power_level / 4 ) {
            color = c_red;
        }

        // calc number of digits in powerlevel int
        int offset = get_int_digits( this->power_level );

        // case power_level > 999 display 1k instead
        int display_power = this->power_level;
        std::string unit = "";
        if( this->power_level > 999 ) {
            switch( offset ) {
                case 4:
                    display_power /= 1000;
                    unit = "k";
                    offset = 2;
                    break;
                case 5:
                    display_power /= 1000;
                    unit = "k";
                    offset = 0;
                    break;
            }
        } else {
            unit = "";
        }

        wmove( sideStyle ? w : g->w_HP,
               sideStyle ? 4 : 21,
               sideStyle ? getmaxx( sideStyle ? w : g->w_HP ) - offset : 7 - offset );
        std::string power_value = std::to_string( display_power ) + unit;
        wprintz( sideStyle ? w : g->w_HP, color, power_value );
    }
    wrefresh( sideStyle ? w : g->w_HP );

}

int get_int_digits( const int &digits )
{
    int offset = digits > 0 ? ( int ) log10( ( double ) digits ) + 1 : 1;
    return offset;
}

