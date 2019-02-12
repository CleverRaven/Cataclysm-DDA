#include "panels.h"

#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "creature.h"
#include "effect.h"
#include "game.h"
#include "gun_mode.h"
#include "item.h"
#include "map.h"
#include "martialarts.h"
#include "options.h"
#include "messages.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "output.h"
#include "player.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"
#include <cmath>
#include <string>
#include <typeinfo>


static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );

// map of drugs, to list them in the panel when player take some
const std::map<std::string, std::string>drug_map = {
    {"datura", "datura"}, {"depressants", "depress"}, {"drunk", "alcohol"}, {"meth", "meth"},
    {"pkill", "pkillr"},  {"pkill2", "pkillr"}, {"pkill3", "pkillr"},
    {"took_prozac", "prozac"}, {"took_xanax", "xanax"}, {"weed_high", "weed"},
    {"magnesium_supplements", "magn"}, {"cough_suppress", "codeine"}, {"stim", "stims"},
    {"stim_overdose", "stimsOD"}, {"valium", "valium"}, {"antibiotic", "abiotic"}
};

// ===============================
// panels code
// ===============================
void draw_panel_adm( const catacurses::window &w )
{

    input_context ctxt( "PANEL_MGMT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "MOVE_PANEL" );

    int index = 1;
    int counter = 0;
    bool selected = false;
    int source_index = 0;
    int target_index = 0;
    std::string saved_name = "";

    bool redraw = true;
    bool exit = false;

    while( !exit ) {
        if( redraw ) {
            redraw = false;
            werase( w );
            static const std::string title = _( "panel admin" );
            decorate_panel( title, w );

            for( int i = 0; i < ( int )g->win_map.size(); i++ ) {

                mvwprintz( w, i + 1, 4,
                           g->win_map[ i ].toggle  ?
                           source_index == i && selected ? c_yellow : c_white : c_dark_gray,
                           selected && index - 1 == i ? " %s" : "%s",
                           selected && index - 1 == i ? saved_name : g->win_map[ i ].name );

            }
            mvwprintz( w, index, 1, c_yellow, ">>" );
            mvwvline( w, 1, 10, 0, 13 );
            mvwprintz( w, 1, 13, c_white, "right: toggle panels on/off" );
            mvwprintz( w, 2, 13, c_white, "enter: change display order" );
        }
        wrefresh( w );

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            index -= 1;
            if( index < 1 ) {
                index = ( int )g->win_map.size();
            }
            redraw = true;

        } else if( action == "DOWN" ) {
            index += 1;
            if( index > ( int )g->win_map.size() ) {
                index = 1;
            }
            redraw = true;

        } else if( action == "MOVE_PANEL" ) {
            counter += 1;
            // source window from the swap
            if( counter == 1 ) {
                // saving win1 index
                source_index = index - 1;
                selected = true;
                saved_name = g->win_map[ source_index ].name;
            }
            // dest window for the swap
            if( counter == 2 ) {
                // saving win2 index
                target_index = index - 1;

                int distance = target_index - source_index;
                int step_dir = distance > 0 ? 1 : -1;
                for( int i = source_index; i != target_index; i += step_dir ) {
                    std::swap( g->win_map[i], g->win_map[i + step_dir] );
                }

                int y_top = 0;
                for( int i = 0; i < ( int )g->win_map.size(); i++ ) {
                    // stack the panels
                    if( g->win_map[ i ].toggle ) {
                        int maxy = catacurses::getmaxy( g->win_map[i].win );
                        g->win_map[ i ].win = catacurses::newwin( maxy, 32, y_top, TERMX - 32 );
                        y_top += maxy;
                    }
                    if( use_tiles ) {
                        // reinit map
                        if( g->win_map[ i ].name == "map" ) {
                            int begy = catacurses::getbegy( g->win_map[i].win );
                            int maxy = catacurses::getmaxy( g->win_map[i].win );
                            g->w_pixel_minimap = catacurses::newwin( maxy, 32, begy, TERMX - 32 );
                            g->reinitmap = true;
                        }
                    }
                }
                counter = 0;
                selected = false;
            }
            redraw = true;
        } else if( action == "RIGHT" ) {
            // toggling panels
            g->win_map[ index - 1 ].toggle = !g->win_map[ index - 1 ].toggle;

            int y_top = 0;
            for( int i = 0; i < ( int )g->win_map.size(); i++ ) {
                if( g->win_map[ i ].toggle ) {
                    int maxy = catacurses::getmaxy( g->win_map[i].win );
                    g->win_map[ i ].win = catacurses::newwin( maxy, 32, y_top, TERMX - 32 );
                    y_top += maxy;

                    if( use_tiles ) {
                        // reinit map
                        if( g->win_map[ i ].name == "map" ) {
                            int begy = catacurses::getbegy( g->win_map[i].win );
                            int maxy = catacurses::getmaxy( g->win_map[i].win );
                            g->w_pixel_minimap = catacurses::newwin( maxy, 32, begy, TERMX - 32 );
                            g->reinitmap = true;
                        }
                    }
                }
            }
            redraw = true;
        } else if( action == "QUIT" ) {
            exit = true;
            shift_view();
            g->show_panel_adm = false;
        }
    }
}

void shift_view()
{
    // shift view depending if there is more panels displayed than
    // half the screen (TERMY/2)
    std::vector<int> panel_max_y;
    for( int i = 0; i < ( int )g->win_map.size(); i++ ) {
        if( g->win_map[ i ].toggle ) {
            int begy = catacurses::getbegy( g->win_map[i].win );
            int maxy = catacurses::getmaxy( g->win_map[i].win );
            int panel_y = begy + maxy;
            panel_max_y.push_back( panel_y );
        }
    }
    sort( panel_max_y.begin(), panel_max_y.end() );
    if( panel_max_y[panel_max_y.size() - 1] >= TERMY / 2 ) {
        switch( g->tileset_zoom ) {
            case 4:
                g->u.view_offset.x = 24;
                break;

            case 8:
                g->u.view_offset.x = 13;
                break;

            case 16:
                g->u.view_offset.x = 6;
                break;

            case 32:
                g->u.view_offset.x = 3;
                break;

            case 64:
                g->u.view_offset.x = 1;
                break;
        }
    } else if( panel_max_y[panel_max_y.size() - 1] < TERMY / 2 ) {
        g->u.view_offset.x = 0;
    }
}

void draw_limb( player &u, const catacurses::window &w )
{
    werase( w );

    int ny  = 0;
    int ny2 = 0;
    int nx  = 0;
    const bool is_self_aware = u.has_trait( trait_SELFAWARE );

    // loop of eternal shame #1
    // (could't find any better way yet to just import the limb status code,
    // so for now, i've done it like that)
    for( int i = 0; i < num_hp_parts; i++ ) {
        if( i < 3 ) {
            ny = i;
            nx = 8;
        } else {
            ny = ny2++;
            nx = 26;
        }
        wmove( w, ny, nx );
        static auto print_symbol_num = []( const catacurses::window & w, int num, const std::string & sym,
        const nc_color & color ) {
            while( num-- > 0 ) {
                wprintz( w, color, sym.c_str() );
            }
        };

        if( u.hp_cur[i] == 0 && ( i >= hp_arm_l && i <= hp_leg_r ) ) {
            //Limb is broken
            std::string limb = "~~%~~";
            nc_color color = c_light_red;

            const auto bp = u.hp_to_bp( static_cast<hp_part>( i ) );
            if( u.worn_with_flag( "SPLINT", bp ) ) {
                static const efftype_id effect_mending( "mending" );
                const auto &eff = u.get_effect( effect_mending, bp );
                const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                if( is_self_aware ) {
                    limb = string_format( "=%2d%%=", mend_perc );
                    color = c_blue;
                } else {
                    const int num = mend_perc / 20;
                    print_symbol_num( w, num, "#", c_blue );
                    print_symbol_num( w, 5 - num, "=", c_blue );
                    continue;
                }
            }

            wprintz( w, color, limb );
            continue;
        }

        const auto &hp = get_hp_bar( u.hp_cur[i], u.hp_max[i] );

        if( is_self_aware ) {
            wprintz( w, hp.second, "%3d  ", u.hp_cur[i] );
        } else {
            wprintz( w, hp.second, hp.first );

            //Add the trailing symbols for a not-quite-full health bar
            print_symbol_num( w, 5 - static_cast<int>( hp.first.size() ), ".", c_white );
        }
    }

    // display limbs status
    static std::array<body_part, 6> part = {{
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };

    ny  = 0;
    ny2 = 0;
    nx  = 0;
    // loop of lame #2
    for( size_t i = 0; i < part.size(); i++ ) {
        if( i < 3 ) {
            ny = i;
            nx = 1;
        } else {
            ny = ny2++;
            nx = 19;
        }

        const std::string str = body_part_hp_bar_ui_text( part[i] );
        wmove( w, ny, nx );
        if( i == 0 ) {
            wprintz( w, u.limb_color( part[i], true, true, true ), str + " :" );
        } else {
            wprintz( w, u.limb_color( part[i], true, true, true ), str + ":" );
        }
    }
    wrefresh( w );
}

void draw_char( player &u, const catacurses::window &w )
{
    werase( w );
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    mvwprintz( w,  0, 1,  c_light_gray, _( "Sound:" ) );
    mvwprintz( w,  1, 1,  c_light_gray, _( "Stam :" ) );
    mvwprintz( w,  2, 1,  c_light_gray, _( "Focus:" ) );
    mvwprintz( w,  0, 19, c_light_gray, _( "Mood :" ) );
    mvwprintz( w,  1, 19, c_light_gray, _( "Speed:" ) );
    mvwprintz( w,  2, 19, c_light_gray, _( "move :" ) );

    const auto str_walk = pgettext( "movement-type", "W" );
    const auto str_run = pgettext( "movement-type", "R" );
    const char *move = u.move_mode == "walk" ? str_walk : str_run;
    std::string movecost = std::to_string( u.movecounter ) + "(" + move + ")";
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );
    mvwprintz( w,  0, 8, c_light_gray, "%s", u.volume );
    mvwprintz( w,  1, 8, value_color( u.stamina / 10 ), "%s", u.stamina / 10 );
    mvwprintz( w,  2, 8, value_color( u.focus_pool ), "%s", u.focus_pool );
    mvwprintz( w,  0, 26, morale_pair.first, "%s", smiley );
    mvwprintz( w,  1, 26, value_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w,  2, 26, c_light_gray, "%s", movecost );
    wrefresh( w );
}

void draw_stat( player &u, const catacurses::window &w )
{
    werase( w );
    // display vehicle controls
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( u.pos() ) );
    }
    if( veh ) {
        // display direction
        int rotation = ( ( veh->face.dir() + 90 ) % 360 );
        mvwprintz( w, 0, 1, c_light_gray, _( "Turn :" ) );
        mvwprintz( w, 0, 8, c_light_gray, "%d°", rotation );

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
            mvwprintz( w, 1, 1, c_light_gray, "%s :", type );
            mvwprintz( w, 1, 8, c_light_green, "%d", t_speed );
            mvwprintz( w, 1, 9 + offset, c_light_gray, "%s", ">" );
            mvwprintz( w, 1, 11 + offset, col_vel, "%d", c_speed );
        }

        // display fuel
        mvwprintz( w, 0, 19, c_light_gray, "Fuel :" );
        veh->print_fuel_indicators( w, 0, 26 );
    } else {
        mvwprintz( w, 0, 1,  c_light_gray, _( "Str  :" ) );
        mvwprintz( w, 1, 1,  c_light_gray, _( "Int  :" ) );
        mvwprintz( w, 0, 19, c_light_gray, _( "Dex  :" ) );
        mvwprintz( w, 1, 19, c_light_gray, _( "Per  :" ) );

        mvwprintz( w, 0, 8,  stat_color( u.str_cur ), "%s", u.str_cur );
        mvwprintz( w, 1, 8,  stat_color( u.int_cur ), "%s", u.int_cur );
        mvwprintz( w, 0, 26, stat_color( u.dex_cur ), "%s", u.dex_cur );
        mvwprintz( w, 1, 26, stat_color( u.per_cur ), "%s", u.per_cur );
    }
    std::pair<nc_color, std::string> pwr_pair = power_stat( u );
    mvwprintz( w, 2, 1,  c_light_gray, _( "Power:" ) );
    mvwprintz( w, 2, 19, c_light_gray, "Safe :" );
    mvwprintz( w, 2, 8, pwr_pair.first, "%s", pwr_pair.second );
    mvwprintz( w, 2, 26, safe_color(), g->safe_mode ? "On" : "Off" );
    wrefresh( w );
}

void draw_env1( const player &u, const catacurses::window &w )
{
    werase( w );
    // display location
    const oter_id &cur_ter = overmap_buffer.ter( u.global_omt_location() );
    mvwprintz( w, 0, 1, c_light_gray, "Place: " );
    wprintz( w, c_white, utf8_truncate( cur_ter->get_name(), getmaxx( w ) - 13 ) );
    // display weather
    if( g->get_levz() < 0 ) {
        mvwprintz( w, 1, 1, c_light_gray, _( "Sky  : Underground" ) );
    } else {
        mvwprintz( w, 1, 1, c_light_gray, _( "Sky  :" ) );
        wprintz( w, weather_data( g->weather ).color, " %s",
                 weather_data( g->weather ).name.c_str() );
    }
    // display lighting
    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( w, 2, 1, c_light_gray, "%s ", _( "Light:" ) );
    wprintz( w, ll.second, ll.first.c_str() );

    // display date
    mvwprintz( w, 3, 1, c_light_gray, _( "Date : %s, day %d" ),
               calendar::name_season( season_of_year( calendar::turn ) ),
               day_of_season<int>( calendar::turn ) + 1 );

    // display time
    if( u.has_watch() ) {
        mvwprintz( w, 4, 1, c_light_gray, _( "Time : %s" ),
                   to_string_time_of_day( calendar::turn ) );
    } else if( g->get_levz() >= 0 ) {
        mvwprintz( w, 4, 1, c_light_gray, _( "Time : %s" ), time_approx() );
    } else {
        mvwprintz( w, 4, 1, c_light_gray, _( "Time : ???" ) );
    }
    wrefresh( w );
}
void draw_env2( const player &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, 0, 1, c_light_gray, _( "Moon : %s" ), get_moon( ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Temp : %s" ), get_temp( u ) );
    mvwprintz( w, 2, 1, c_light_gray, _( "Wind : %s" ), wind_string() );
    mvwprintz( w, 3, 1, c_light_gray, _( "Rad  : -" ) );
    wrefresh( w );
}

void draw_mod1( const player &u, const catacurses::window &w )
{
    werase( w );
    auto hunger_pair = hunger_stat( u );
    auto thirst_pair = thirst_stat( u );
    auto rest_pair = rest_stat( u );
    auto temp_pair = temp_stat( u );
    auto pain_pair = pain_stat( u );
    mvwprintz( w, 0,  1,  c_light_gray, _( "Arm  :" ) );
    mvwprintz( w, 1,  1,  c_light_gray, _( "Food :" ) );
    mvwprintz( w, 2,  1,  c_light_gray, _( "Drink:" ) );
    mvwprintz( w, 3,  1,  c_light_gray, _( "Rest :" ) );
    mvwprintz( w, 4,  1,  c_light_gray, _( "Pain :" ) );
    mvwprintz( w, 5,  1,  c_light_gray, _( "Heat :" ) );
    mvwprintz( w, 0,  8, c_light_gray, u.weapname().c_str() );
    mvwprintz( w, 1,  8, hunger_pair.first, hunger_pair.second );
    mvwprintz( w, 2,  8, thirst_pair.first, thirst_pair.second );
    mvwprintz( w, 3,  8, rest_pair.first, rest_pair.second );
    mvwprintz( w, 4,  8, pain_pair.first, pain_pair.second );
    mvwprintz( w, 5,  8, temp_pair.first, temp_pair.second );
    wrefresh( w );
}

void draw_mod2( const player &u, const catacurses::window &w )
{
    werase( w );
    mvwprintz( w, 0, 1, c_light_gray, _( "Drug : %s" ), get_effects( u ) ); //too much cafeine O_O" ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Head :" ) );
    mvwprintz( w, 2, 1, c_light_gray, _( "Torso:" ) );
    mvwprintz( w, 3, 1, c_light_gray, _( "Arms :" ) );
    mvwprintz( w, 4, 1, c_light_gray, _( "Legs :" ) );
    mvwprintz( w, 5, 1, c_light_gray, _( "Feet :" ) );

    mvwprintz( w, 1, 8, c_light_gray, get_armor( u, bp_head, w ) );
    mvwprintz( w, 2, 8, c_light_gray, get_armor( u, bp_torso, w ) );
    mvwprintz( w, 3, 8, c_light_gray, get_armor( u, bp_arm_r, w ) );
    mvwprintz( w, 4, 8, c_light_gray, get_armor( u, bp_leg_r, w ) );
    mvwprintz( w, 5, 8, c_light_gray, get_armor( u, bp_foot_r, w ) );
    wrefresh( w );
}

void draw_messages( const catacurses::window &w )
{
    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 1, 0 /*topline*/, maxlength - 1, line );
    wrefresh( w );
}

void draw_mminimap( const catacurses::window &w )
{
    std::string type = get_option<std::string>( "MINIMAP_MODE" );
    if( type == "pixel" ) {
        werase( w );
        wrefresh( w );
        g->draw_pixel_minimap();
    } else {
        g->draw_minimap( w );
    }
}

void draw_compass( const catacurses::window &w )
{
    werase( w );
    g->mon_info( w );
    wrefresh( w );
}


// ====================================
// panels prettify and helper functions
// ====================================

void decorate_panel( const std::string name, const catacurses::window &w )
{
    werase( w );
    draw_border( w );

    static const char *title_prefix = "< ";
    const std::string title = name;
    static const char *title_suffix = " >";
    static const std::string full_title = string_format( "%s%s%s",
                                          title_prefix, title, title_suffix );
    const int start_pos = center_text_pos( full_title.c_str(), 0, getmaxx( w ) - 1 );
    mvwprintz( w, 0, start_pos, c_white, title_prefix );
    wprintz( w, c_blue, title );
    wprintz( w, c_white, title_suffix );
}

std::string get_effects( const player &u )
{
    std::vector<std::string> hits;
    std::string effect = "";
    //grab all the drug effects
    for( auto &elem : *u.effects ) {
        for( auto &_effect_it : elem.second ) {
            auto &it = _effect_it.second;
            for( auto const &element : drug_map ) {
                std::string temp1 = element.first;
                std::string temp2 = ( std::string )it.get_id();
                if( temp1 == temp2 ) {
                    hits.push_back( element.second );
                }
            }
        }
    }
    // build our string
    for( size_t i = 0; i < hits.size(); i++ ) {
        std::string s = hits[i];
        std::string chars = "[]1234567890";

        for( char c : chars ) {
            s.erase( std::remove( s.begin(), s.end(), c ), s.end() );
        }

        if( effect.size() < 22 ) {
            effect += s;
            if( !( i == hits.size() ) ) {
                effect += ",";
            }
        }
    }
    //check for max string length
    if( effect.size() > 24 ) {
        effect.resize( 23 );
        effect += ".";
        return effect;
    }
    //if last char is "," delete it
    char back = effect[effect.size() - 1];
    char last = ',';
    if( back == last ) {
        effect.pop_back();
    }
    return effect;
}

std::string get_temp( const player &u )
{
    std::string temp = "";
    if( u.has_item_with_flag( "THERMOMETER" ) ||
        u.has_bionic( bionic_id( "bio_meteorologist" ) ) ) {
        temp = print_temperature( g->get_temperature( u.pos() ) ).c_str();
    }
    if( ( int )temp.size() == 0 ) {
        return "-";
    }
    return temp;
}

std::string get_moon()
{
    std::string moon = "";
    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    if( iPhase == 0 ) {
        moon = "New moon";
    }
    if( iPhase == 1 ) {
        moon  = "Waxing crescent";
    }
    if( iPhase == 2 ) {
        moon  = "Half moon";
    }
    if( iPhase == 3 ) {
        moon  = "Waxing gibbous";
    }
    if( iPhase == 4 ) {
        moon  = "Full moon";
    }
    if( iPhase == 5 ) {
        moon  = "Waning gibbous";
    }
    if( iPhase == 6 ) {
        moon  = "Half moon";
    }
    if( iPhase == 7 ) {
        moon  = "Waning crescent";
    }
    if( iPhase == 8 ) {
        moon  = "Dark moon";
    }
    return moon;
}

std::string wind_string()
{
    const oter_id &cur_om_ter = overmap_buffer.ter( g->u.global_omt_location() );
    w_point weather_point = *g->weather_precise;
    bool sheltered = g->is_sheltered( g->u.pos() );
    double windpower = get_local_windpower( weather_point.windpower, cur_om_ter,
                                            g->u.pos(), weather_point.winddirection, sheltered );
    std::string dir = "";
    std::string wind_desc = get_wind_desc( windpower );
    std::string wind_dir = get_wind_arrow( weather_point.winddirection );
    if( wind_dir == "\u21D3" ) {
        dir = "(N)";
    } else if( wind_dir == "\u21D9" ) {
        dir = "(NE)";
    } else if( wind_dir == "\u21D0" ) {
        dir = "(E)";
    } else if( wind_dir == "\u21D6" ) {
        dir = "(SE)";
    } else if( wind_dir == "\u21D1" ) {
        dir = "(S)";
    } else if( wind_dir == "\u21D7" ) {
        dir = "(SW)";
    } else if( wind_dir == "\u21D2" ) {
        dir = "(W)";
    } else if( wind_dir == "\u21D8" ) {
        dir = "(NW)";
    }
    wind_desc += " ";
    wind_desc += dir;
    if( sheltered ) {
        wind_desc = "Sheltered";
    }

    return wind_desc;
}

std::string time_approx()
{
    const int iHour = hour_of_day<int>( calendar::turn );
    std::string time_approx = "";
    if( iHour >= 22 ) {
        time_approx = "Around midnight";
    } else if( iHour >= 20 ) {
        time_approx = "The night is young";
    } else if( iHour >= 16 ) {
        time_approx = "It's getting darker";
    } else if( iHour >= 13 ) {
        time_approx = "In the afternoon";
    } else if( iHour >= 11 ) {
        time_approx = "Around noon";
    } else if( iHour >= 8 ) {
        time_approx = "Early Morning";
    } else if( iHour >= 5 ) {
        time_approx = "Around Dawn";
    } else if( iHour >= 0 ) {
        time_approx = "Dead of Night";
    }
    return time_approx;
}

nc_color value_color( int stat )
{
    nc_color valuecolor = c_light_gray;

    if( stat >= 75 ) {
        valuecolor = c_green;
    } else if( stat >= 50 ) {
        valuecolor = c_yellow;
    } else if( stat >= 25 ) {
        valuecolor = c_red;
    } else {
        valuecolor = c_magenta;
    }
    return valuecolor;
}

nc_color stat_color( int stat )
{
    nc_color statcolor = c_light_gray;

    if( stat >= 7 ) {
        statcolor = c_green;
    } else if( stat >= 5 ) {
        statcolor = c_yellow;
    } else if( stat >= 2 ) {
        statcolor = c_red;
    } else {
        statcolor = c_magenta;
    }

    return statcolor;
}

std::pair<nc_color, int> morale_stat( const player &u )
{
    const int morale_int = u.get_morale_level();
    nc_color morale_color = c_white;
    if( morale_int >= 10 ) {
        morale_color = c_green;
    } else if( morale_int <= -10 ) {
        morale_color = c_red;
    }
    return std::make_pair( morale_color, morale_int );
}

std::pair<nc_color, std::string> hunger_stat( const player &u )
{
    std::string hunger_string = "";
    nc_color hunger_color = c_yellow;
    if( u.get_hunger() >= 300 && u.get_starvation() > 2500 ) {
        hunger_color = c_red;
        hunger_string = _( "Starving!" );
    } else if( u.get_hunger() >= 300 && u.get_starvation() > 1100 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Near starving" );
    } else if( u.get_hunger() > 250 ) {
        hunger_color = c_light_red;
        hunger_string = _( "Famished" );
    } else if( u.get_hunger() > 100 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Very hungry" );
    } else if( u.get_hunger() > 40 ) {
        hunger_color = c_yellow;
        hunger_string = _( "Hungry" );
    } else if( u.get_hunger() < -60 ) {
        hunger_color = c_green;
        hunger_string = _( "Engorged" );
    } else if( u.get_hunger() < -20 ) {
        hunger_color = c_green;
        hunger_string = _( "Sated" );
    } else if( u.get_hunger() < 0 ) {
        hunger_color = c_green;
        hunger_string = _( "Full" );
    }
    return std::make_pair( hunger_color, hunger_string );
}

std::pair<nc_color, std::string> thirst_stat( const player &u )
{
    std::string hydration_string = "";
    nc_color hydration_color = c_yellow;
    if( u.get_thirst() > 520 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Parched" );
    } else if( u.get_thirst() > 240 ) {
        hydration_color = c_light_red;
        hydration_string = _( "Dehydrated" );
    } else if( u.get_thirst() > 80 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Very Thirsty" );
    } else if( u.get_thirst() > 40 ) {
        hydration_color = c_yellow;
        hydration_string = _( "Thirsty" );
    } else if( u.get_thirst() < -60 ) {
        hydration_color = c_green;
        hydration_string = _( "Turgid" );
    } else if( u.get_thirst() < -20 ) {
        hydration_color = c_green;
        hydration_string = _( "Hydrated" );
    } else if( u.get_thirst() < 0 ) {
        hydration_color = c_green;
        hydration_string = _( "Slaked" );
    }
    return std::make_pair( hydration_color, hydration_string );
}

std::pair<nc_color, std::string> rest_stat( const player &u )
{
    std::string rest_string = "";
    nc_color rest_color = c_yellow;
    if( u.get_fatigue() > EXHAUSTED ) {
        rest_color = c_red;
        rest_string = _( "Exhausted" );
    } else if( u.get_fatigue() > DEAD_TIRED ) {
        rest_color = c_light_red;
        rest_string = _( "Dead tired" );
    } else if( u.get_fatigue() > TIRED ) {
        rest_color = c_yellow;
        rest_string = _( "Tired" );
    }
    return std::make_pair( rest_color, rest_string );
}

std::pair<nc_color, std::string> temp_stat( const player &u )
{
    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    int current_bp_extreme = 0;
    int conv_bp_extreme = 0;
    for( int i = 0; i < num_bp ; i++ ) {
        if( abs( u.temp_cur[i] - BODYTEMP_NORM ) > abs( u.temp_cur[current_bp_extreme] - BODYTEMP_NORM ) ) {
            current_bp_extreme = i;
        }
        if( abs( u.temp_conv[i] - BODYTEMP_NORM ) > abs( u.temp_conv[conv_bp_extreme] - BODYTEMP_NORM ) ) {
            conv_bp_extreme = i;
        }
    }

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    std::string temp_string = "";
    nc_color temp_color = c_yellow;
    if( u.temp_cur[current_bp_extreme] >         BODYTEMP_SCORCHING ) {
        temp_color  = c_red;
        temp_string = _( "Scorching!" );
    } else if( u.temp_cur[current_bp_extreme] >  BODYTEMP_VERY_HOT ) {
        temp_color  = c_light_red;
        temp_string = _( "Very hot!" );
    } else if( u.temp_cur[current_bp_extreme] >  BODYTEMP_HOT ) {
        temp_color  = c_yellow;
        temp_string =  _( "Warm" );
    } else if( u.temp_cur[current_bp_extreme] >  BODYTEMP_COLD ) {
        temp_color  = c_green;
        temp_string = _( "Comfortable" );
    } else if( u.temp_cur[current_bp_extreme] >  BODYTEMP_VERY_COLD ) {
        temp_color  = c_light_blue;
        temp_string = _( "Chilly" );
    } else if( u.temp_cur[current_bp_extreme] >  BODYTEMP_FREEZING ) {
        temp_color  = c_cyan;
        temp_string = _( "Very cold!" );
    } else if( u.temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        temp_color  = c_blue;
        temp_string = _( "Freezing!" );
    }

    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.temp_cur[current_bp_extreme] );
    const int conv_zone = define_temp_level( u.temp_conv[conv_bp_extreme] );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    std::string temp_message;
    if( delta > 2 ) {
        temp_message = _( " (\u2191)" );
    } else if( delta ==  2 ) {
        temp_message = _( " (\u2191)" );
    } else if( delta ==  1 ) {
        temp_message = _( " (\u2191)" );
    } else if( delta ==  0 ) {
        temp_message = "";
    } else if( delta == -1 ) {
        temp_message = _( " (\u2193)" );
    } else if( delta == -2 ) {
        temp_message = _( " (\u2193)" );
    } else {
        temp_message = _( " (\u2193)" );
    }

    temp_string += temp_message;
    return std::make_pair( temp_color, temp_string );
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

std::pair<nc_color, std::string> pain_stat( const player &u )
{
    nc_color pain_color = c_yellow;
    std::string pain_string = "";
    // get pain color
    if( u.get_perceived_pain() >= 60 ) {
        pain_color = c_red;
    } else if( u.get_perceived_pain() >= 40 ) {
        pain_color = c_light_red;
    }
    // get pain string
    if( u.has_trait( trait_SELFAWARE ) && u.get_perceived_pain() > 0 ) {
        pain_string = u.get_perceived_pain();
    } else if( u.get_perceived_pain() > 0 ) {
        pain_string = u.get_pain_description();
    }
    return std::make_pair( pain_color, pain_string );
}

std::string get_armor( const player &u, body_part bp, const catacurses::window &w )
{
    for( std::list<item>::const_iterator it = u.worn.end(); it != u.worn.begin(); ) {
        --it;
        if( it->covers( bp ) ) {
            std::string temp = it->tname( 1, false );
            if( ( int )temp.length() > getmaxx( w ) - 9 ) {
                // string minus window size.x and label
                temp = temp.substr( 0, getmaxx( w ) - 11 );
                temp += "..";
                return temp;
            }
            return temp;
        }
    }
    return "-";
}

face_type get_face_type( const player &u )
{
    face_type fc = face_human;
    if( u.has_trait( trait_THRESH_FELINE ) ) {
        fc = face_cat;
    } else if( u.has_trait( trait_THRESH_URSINE ) ) {
        fc = face_bear;
    } else if( u.has_trait( trait_THRESH_BIRD ) ) {
        fc = face_bird;
    }
    return fc;
}

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

std::pair<nc_color, std::string> power_stat( const player &u )
{
    nc_color c_pwr = c_red;
    std::string s_pwr = "";
    if( u.max_power_level == 0 ) {
        s_pwr = "--";
    } else {
        if( u.power_level >= u.max_power_level / 2 ) {
            c_pwr = c_green;
        } else if( u.power_level >= u.max_power_level / 3 ) {
            c_pwr = c_yellow;
        } else if( u.power_level >= u.max_power_level / 4 ) {
            c_pwr = c_red;
        }
        s_pwr = to_string( u.power_level );
    }
    return std::make_pair( c_pwr, s_pwr );
}

nc_color safe_color()
{
    nc_color s_color = g->safe_mode ? c_green : c_red;
    if( g->safe_mode == SAFE_MODE_OFF && get_option<bool>( "AUTOSAFEMODE" ) ) {
        int s_return = get_option<int>( "AUTOSAFEMODETURNS" );
        int iPercent = g->turnssincelastmon * 100 / s_return;
        if( iPercent >= 100 ) {
            s_color = c_green;
        } else if( iPercent >= 75 ) {
            s_color = c_yellow;
        } else if( iPercent >= 50 ) {
            s_color = c_light_red;
        } else if( iPercent >= 25 ) {
            s_color = c_red;
        }
    }
    return s_color;
}

int get_int_digits( const int &digits )
{
    int temp = abs( digits );
    int offset = temp > 0 ? ( int ) log10( ( double ) temp ) + 1 : 1;
    return offset;
}
