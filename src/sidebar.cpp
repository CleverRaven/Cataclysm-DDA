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

void draw_HP( const player &p, const catacurses::window &w_HP, const bool wide )
{
    werase( w_HP );

    // The HP window can be in "tall" mode (7x14) or "wide" mode (14x7).
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
    print_stamina_bar( w_HP, wide ? 7 * dy : 12, wide ? 0 : hpx, p, !wide );
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

void print_stamina_bar( const catacurses::window &w, const int y, const int x, const player &p,
                        const bool narrow )
{
    wmove( w, y, x );
    if( narrow ) {
        wprintz( w, c_light_gray, _( "STA:" ) );
        wmove( w, y + 1, x );
    } else {
        wprintz( w, c_light_gray, _( "STA   " ) );
    }

    std::string sta_bar;
    nc_color sta_color;
    std::tie( sta_bar, sta_color ) = get_hp_bar( p.stamina, p.get_stamina_max() );
    wprintz( w, sta_color, sta_bar );
}

void draw_time( const catacurses::window &time_window, const int y, const int x,
                const bool has_watch,
                const bool can_see_sun )
{
    catacurses::wmove( time_window, y, x );
    if( has_watch ) {
        wprintz( time_window, c_white, to_string_time_of_day( calendar::turn ) );
    } else if( can_see_sun ) {
        std::vector<std::pair<char, nc_color> > vGlyphs;
        vGlyphs.push_back( std::make_pair( '_', c_red ) );
        vGlyphs.push_back( std::make_pair( '_', c_cyan ) );
        vGlyphs.push_back( std::make_pair( '.', c_brown ) );
        vGlyphs.push_back( std::make_pair( ',', c_blue ) );
        vGlyphs.push_back( std::make_pair( '+', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'c', c_light_blue ) );
        vGlyphs.push_back( std::make_pair( '*', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'C', c_white ) );
        vGlyphs.push_back( std::make_pair( '+', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'c', c_light_blue ) );
        vGlyphs.push_back( std::make_pair( '.', c_brown ) );
        vGlyphs.push_back( std::make_pair( ',', c_blue ) );
        vGlyphs.push_back( std::make_pair( '_', c_red ) );
        vGlyphs.push_back( std::make_pair( '_', c_cyan ) );

        const int iHour = hour_of_day<int>( calendar::turn );
        wprintz( time_window, c_white, "[" );
        bool bAddTrail = false;

        for( int i = 0; i < 14; i += 2 ) {
            if( iHour >= 8 + i && iHour <= 13 + ( i / 2 ) ) {
                wputch( time_window, hilite( c_white ), ' ' );

            } else if( iHour >= 6 + i && iHour <= 7 + i ) {
                wputch( time_window, hilite( vGlyphs[i].second ), vGlyphs[i].first );
                bAddTrail = true;

            } else if( iHour >= ( 18 + i ) % 24 && iHour <= ( 19 + i ) % 24 ) {
                wputch( time_window, vGlyphs[i + 1].second, vGlyphs[i + 1].first );

            } else if( bAddTrail && iHour >= 6 + ( i / 2 ) ) {
                wputch( time_window, hilite( c_white ), ' ' );

            } else {
                wputch( time_window, c_white, ' ' );
            }
        }

        wprintz( time_window, c_white, "]" );
    } else {
        wprintz( time_window, c_white, _( "Time: ???" ) );
    }
}

void draw_location( const catacurses::window &w, const int y, const int x,
                    const std::string &location_name )
{
    wrefresh( w );
    mvwprintz( w, y, x, c_light_gray, _( "Location: " ) );
    wprintz( w, c_white, utf8_truncate( location_name, getmaxx( w ) ) );
}

void draw_ui_weather( const catacurses::window &w, const int y, const int x, const bool underground,
                      const nc_color &weather_color, const std::string &weather )
{
    if( underground ) {
        mvwprintz( w, y, x, c_light_gray, _( "Underground" ) );
    } else {
        mvwprintz( w, y, x, c_light_gray, _( "Weather :" ) );
        wprintz( w, weather_color, " %s", weather );
    }
}

void draw_temperature( const catacurses::window &w, const int y, const int x,
                       const std::string &temperature )
{
    mvwprintz( w, y, x, c_light_gray, _( "Temp :" ) );
    wprintz( w, c_white, " %s", temperature );
}

void draw_moon( const catacurses::window &w, const int y, const int x, const int iPhase )
{
    static std::vector<std::string> vMoonPhase = { "(   )", "(  ))", "( | )", "((  )" };

    std::string sPhase = vMoonPhase[iPhase % 4];

    if( iPhase > 0 ) {
        sPhase.insert( 5 - ( ( iPhase > 4 ) ? iPhase % 4 : 0 ), "</color>" );
        sPhase.insert( 5 - ( ( iPhase < 4 ) ? iPhase + 1 : 5 ),
                       "<color_" + string_from_color( i_black ) + ">" );
    }
    mvwprintz( w, y, x, c_light_gray, _( "Moon : " ) );
    trim_and_print( w, y, x + 7, 11, c_white, sPhase.c_str() );
}

void draw_lighting( const catacurses::window &w, const int y, const int x )
{
    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( w, y, x, c_light_gray, "%s ", _( "Lighting:" ) );
    wprintz( w, ll.second, ll.first.c_str() );
}

void draw_sound( const catacurses::window &w, const int y, const int x, const bool is_deaf,
                 const int sound )
{
    if( is_deaf ) {
        mvwprintz( w, y, x, c_red, _( "Deaf!" ) );
    } else {
        mvwprintz( w, y, x, c_light_gray, "%s ", _( "Sound: " ) );
        mvwprintz( w, y, x + 7, c_yellow, std::to_string( sound ) );
        wrefresh( w );
    }
}

void draw_date( const catacurses::window &w, const int y, const int x )
{
    mvwprintz( w, y, x, c_white, _( "%s, day %d" ),
               calendar::name_season( season_of_year( calendar::turn ) ),
               day_of_season<int>( calendar::turn ) + 1 );
}

void draw_safe_mode( const catacurses::window &w, const int y, const int x,
                     const bool safe_mode_off, const int iPercent )
{
    wmove( w, y, x );
    const std::array<std::string, 4> letters = { { "S", "A", "F", "E" } };
    //Safemode coloring
    for( int i = 0; i < 4; i++ ) {
        nc_color c = ( safe_mode_off && iPercent < ( i + 1 ) * 25 ) ? c_red : c_green;
        wprintz( w, c, letters[i].c_str() );
    }
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

    wmove( w, sideStyle ? 3 : 2, 0 );
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
    mvwprintz( w, sideStyle ? 6 : 3, sideStyle ? getmaxx( w ) - 2 : 0, col_morale,
               morale_emotion( morale_cur, fc,
                               get_option<std::string>( "MORALE_STYLE" ) == "horizontal" ) );

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

    draw_ui_power( sideStyle ? w : g->w_HP, sideStyle ? 4 : 21, sideStyle ? 17 : 0, this->power_level,
                   this->max_power_level );
}

void draw_ui_power( const catacurses::window &w, const int y, const int x, const int power,
                    const int max_power )
{
    wmove( w, y, x );
    wprintz( w, c_white, _( "Pwr " ) );

    if( max_power == 0 ) {
        wprintz( w, c_light_gray, " --" );
    } else {
        nc_color color = c_red;
        if( power >= max_power / 2 ) {
            color = c_green;
        } else if( power >= max_power / 3 ) {
            color = c_yellow;
        } else if( power >= max_power / 4 ) {
            color = c_red;
        }

        // calc number of digits in powerlevel int
        int offset = get_int_digits( power );

        // case power_level > 999 display 1k instead
        int display_power = power;
        std::string unit = "";
        if( power > 999 ) {
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

        wmove( w, y, getmaxx( w ) - offset );
        std::string power_value = std::to_string( display_power ) + unit;
        wprintz( w, color, power_value );
    }
    wrefresh( w );
}

std::string player::weapname() const
{
    if( weapon.is_gun() ) {
        std::string str;
        str = weapon.type_name();

        // Is either the base item or at least one auxiliary gunmod loaded (includes empty magazines)
        bool base = weapon.ammo_capacity() > 0 && !weapon.has_flag( "RELOAD_AND_SHOOT" );

        const auto mods = weapon.gunmods();
        bool aux = std::any_of( mods.begin(), mods.end(), [&]( const item * e ) {
            return e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" );
        } );

        if( base || aux ) {
            str += " (";
            if( base ) {
                str += std::to_string( weapon.ammo_remaining() );
                if( weapon.magazine_integral() ) {
                    str += "/" + std::to_string( weapon.ammo_capacity() );
                }
            } else {
                str += "---";
            }
            str += ")";

            for( auto e : mods ) {
                if( e->is_gun() && e->ammo_capacity() > 0 && !e->has_flag( "RELOAD_AND_SHOOT" ) ) {
                    str += " (" + std::to_string( e->ammo_remaining() );
                    if( e->magazine_integral() ) {
                        str += "/" + std::to_string( e->ammo_capacity() );
                    }
                    str += ")";
                }
            }
        }
        return "Weapon  : " + str;

    } else if( weapon.is_container() && weapon.contents.size() == 1 ) {
        return string_format( "Weapon  : %s (%d)", weapon.tname().c_str(),
                              weapon.contents.front().charges );

    } else if( !is_armed() ) {
        return _( "Weapon  : fists" );

    } else {
        return "Weapon  : " + weapon.tname();
    }
}

int get_int_digits( const int &digits )
{
    int offset = digits > 0 ? ( int ) log10( ( double ) digits ) + 1 : 1;
    return offset;
}

