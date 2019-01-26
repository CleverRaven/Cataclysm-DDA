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

void player::disp_status()
{
    bool sideStyle = use_narrow_sidebar();

    // Find hottest/coldest bodypart
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
    // Decide if temp_cur is rising or falling
    std::string t_msg1 = "";
    std::string t_msg2 = "";
    nc_color t_color = c_white;

    const int delta = conv_zone - cur_zone;
    if( delta > 2 ) {
        t_msg2 = "(\u2191)";
    } else if( delta ==  2 ) {
        t_msg2 = "(\u2191)";
    } else if( delta ==  1 ) {
        t_msg2 = "(\u2191)";
    } else if( delta ==  0 ) {
        t_msg2 = "";
    } else if( delta < -1 ) {
        t_msg2 = "(\u2193)";
    } else if( delta == -2 ) {
        t_msg2 = "(\u2193)";
    } else {
        t_msg2 = "(\u2193)";
    }

    // print hottest/coldest bodypart, and if it is rising or falling in temperature
    if( temp_cur[current_bp_extreme] >  BODYTEMP_SCORCHING ) {
        t_color = c_red;
        t_msg1 = _( "Scorching!" );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        t_color = c_light_red;
        t_msg1 = _( "Very hot!" );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        t_color = c_yellow;
        t_msg1 = _( "Warm" );
    } else if( temp_cur[current_bp_extreme] > BODYTEMP_COLD ) {
        t_color = c_green;
        t_msg1 = _( "Comfortable" );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        t_color = c_light_blue;
        t_msg1 = _( "Chilly" );
    } else if( temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        t_color = c_cyan;
        t_msg1 =  _( "Very cold!" );
    } else if( temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        t_color =  c_blue;
        t_msg1 = _( "Freezing!" );
    }
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 1 : 4, 0, c_light_gray, _( "Heat :" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 1 : 4, 7, t_color, t_msg1 + " " + t_msg2 );

    // display tiredness
    std::string tiredness = "";
    nc_color tired_color = c_white;
    if( get_fatigue() > EXHAUSTED ) {
        tiredness = _( "Exhausted" );
        tired_color = c_red;
    } else if( get_fatigue() > DEAD_TIRED ) {
        tiredness = _( "Dead tired" );
        tired_color = c_light_red;
    } else if( get_fatigue() > TIRED ) {
        tiredness = _( "Tired" );
        tired_color = c_yellow;
    }
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 2 : 5, 0, c_light_gray, _( "Sleep:" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 2 : 5, 7, tired_color, tiredness );

    // display pain levels
    nc_color col_pain = c_yellow;
    if( get_perceived_pain() >= 60 ) {
        col_pain = c_red;
    } else if( get_perceived_pain() >= 40 ) {
        col_pain = c_light_red;
    }
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 3 : 6, 0, c_light_gray, _( "Pain :" ) );
    if( has_trait( trait_SELFAWARE ) && get_perceived_pain() > 0 ) {
        mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
                   sideStyle ? 3 : 6, 7, col_pain, "%d", get_perceived_pain() );
    } else if( get_perceived_pain() > 0 ) {
        mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
                   sideStyle ? 3 : 6, 7, col_pain, get_pain_description() );
    }

    // display hunger
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
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 4 : 7, 0, c_light_gray, _( "Food :" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 4 : 7, 7, hunger_color, hunger_string );

    // display thirst
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
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 5 : 8, 0, c_light_gray, _( "Drink:" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 5 : 8, 7, hydration_color, hydration_string );

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

    // display used weapon
    if( !style.empty() ) {
        const auto style_color = is_armed() ? c_red : c_blue;
        trim_and_print( sideStyle ? g->w_status2 : g->w_location_wider,
                        sideStyle ? 6 : 9, 0, getmaxx( g->w_status2 ), c_light_gray,
                        _( "Arm  : %s" ), print_gun_mode( *this ) );
        mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
                   sideStyle ? 6 : 9, sideStyle ? 32 : 43, style_color, style );

    } else {
        trim_and_print( sideStyle ? g->w_status2 : g->w_location_wider, sideStyle ? 6 : 9,
                        0, 20, c_light_gray, _( "Arm  : %s" ), print_gun_mode( *this ) );
    }

    // display speed
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 1 : 4, sideStyle ? 32 : 43, c_light_gray, _( "Speed:" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 1 : 4, sideStyle ? 39 : 50,
               stat_color( get_speed_bonus() ), _( "%d" ), get_speed() );

    // display move cost
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
    // get move mode Walk / Run (1 letter)
    const auto str_walk = pgettext( "movement-type", "W" );
    const auto str_run = pgettext( "movement-type", "R" );
    const char *move = move_mode == "walk" ? str_walk : str_run;
    //int offset = get_int_digits( movecounter );
    std::string movecost = std::to_string( movecounter ) + move;
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 2 : 5, sideStyle ? 32 : 43, c_light_gray, _( "Move :" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 2 : 5, sideStyle ? 39 : 50, col_time, movecost );

    // display focus
    nc_color col_xp = c_dark_gray;
    if( focus_pool >= 100 ) {
        col_xp = c_white;
    } else if( focus_pool >  0 ) {
        col_xp = c_light_gray;
    }
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 3 : 6, sideStyle ? 32 : 43, c_light_gray, _( "Focus:" ) );
    mvwprintz( sideStyle ? g->w_status2 : g->w_location_wider,
               sideStyle ? 3 : 6, sideStyle ? 39 : 50, col_xp, "%d", focus_pool );


    // big refresh
    wrefresh( sideStyle ? g->w_status2 : g->w_location_wider );

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
    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? 6 : 29,
               sideStyle ? 11 : 0, c_light_gray, sideStyle ? _( "Mood :" ) : _( "Mood:" ) );

    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? 6 : 30,
               sideStyle ? 18 : 0, col_morale,
               morale_emotion( morale_cur, fc,
                               get_option<std::string>( "MORALE_STYLE" ) == "horizontal" ) );

    // display vehicle controls
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( pos() ) );
    }
    if( veh ) {
        // display direction
        int rotation = ( ( veh->face.dir() + 90 ) % 360 );
        mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 0 : 1,
                   sideStyle ? 0 : 30, c_light_gray, _( "Turn:" ) );
        mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 1 : 2,
                   sideStyle ? 0 : 30, c_light_gray, "%d°", rotation );

        // target speed > current speed
        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
        if( veh->cruise_on ) {
            int t_speed = int( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) );
            int c_speed = int( convert_velocity( veh->velocity, VU_VEHICLE ) );
            int offset = get_int_digits( t_speed );
            mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 3 : 4,
                       sideStyle ? 0 : 30, c_light_gray, "%s:", type );
            mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 4 : 5,
                       sideStyle ? 0 : 30, c_light_green, "%d", t_speed );
            mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 4 : 5,
                       sideStyle ? 1 + offset : 31 + offset, c_light_gray, "%s", ">" );
            mvwprintz( sideStyle ? g->w_status : g->w_location_wider, sideStyle ? 4 : 5,
                       sideStyle ? 3 + offset : 33 + offset, col_vel, "%d", c_speed );
        }

        // display fuel
        veh->print_fuel_indicators( sideStyle ? g->w_status : g->w_location_wider,
                                    sideStyle ? 6 : 7, sideStyle ? 0 : 30 );
    }

    const int wx = sideStyle ? 18 :  0;
    const int wy = 0;
    const int dx = 0;
    const int dy = sideStyle ?  1 :  8;
    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? ( wy + dy * 0 ) : 17,
               sideStyle ? 11 : wx + dx * 0, stat_color( get_str_bonus() ),
               sideStyle ? _( "Str  : %02d" ) : _( "Str %02d" ), str_cur );

    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? ( wy + dy * 1 ) : 18,
               sideStyle ? 11 : wx + dx * 1, stat_color( get_dex_bonus() ),
               sideStyle ? _( "Dex  : %02d" ) : _( "Dex %02d" ), dex_cur );

    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? ( wy + dy * 2 ) : 19,
               sideStyle ? 11 : wx + dx * 2, stat_color( get_int_bonus() ),
               sideStyle ? _( "Int  : %02d" ) : _( "Int %02d" ), int_cur );

    mvwprintz( sideStyle ? g->w_status : g->w_HP, sideStyle ? ( wy + dy * 3 ) : 20,
               sideStyle ? 11 : wx + dx * 3, stat_color( get_per_bonus() ),
               sideStyle ? _( "Per  : %02d" ) : _( "Per %02d" ), per_cur );

    // display power level
    if( this->max_power_level == 0 ) {
        mvwprintz( sideStyle ? g->w_status : g->w_HP,
                   sideStyle ? 4 : 23, sideStyle ? 11 : 0, c_light_gray,
                   sideStyle ? "Pwr  :" : "Power:" );
        mvwprintz( sideStyle ? g->w_status : g->w_HP,
                   sideStyle ? 4 : 24, sideStyle ? 18 : 0, c_light_gray, "--" );
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
        std::string unit = " ";
        if( this->power_level > 999 ) {
            switch( offset ) {
                case 4:
                    display_power /= 1000;
                    unit = "k";
                    break;
                case 5:
                    display_power /= 1000;
                    unit = "k";
                    break;
            }
        } else {
            unit = " ";
        }

        if( sideStyle ) {
            std::string power_value = std::to_string( display_power ) + unit;
            mvwprintz( sideStyle ? g->w_status : g->w_HP, 4, 11, c_white, _( "Pwr  : " ) );
            mvwprintz( sideStyle ? g->w_status : g->w_HP, 4, 18, color, power_value );
        } else {
            mvwprintz( sideStyle ? g->w_status : g->w_HP, 23, 0, c_white, _( "Pwr: " ) );
            mvwprintz( sideStyle ? g->w_status : g->w_HP, 24, 0, color, "%d", this->power_level );
        }
    }
    wrefresh( sideStyle ? g->w_status : g->w_HP );

}

int get_int_digits( const int &digits )
{
    int offset = digits > 0 ? ( int ) log10( ( double ) digits ) + 1 : 1;
    return offset;
}

