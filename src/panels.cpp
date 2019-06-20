#include "panels.h"

#include <cstdlib>
#include <cmath>
#include <string>
#include <array>
#include <iosfwd>
#include <iterator>
#include <list>
#include <memory>
#include <utility>

#include "action.h"
#include "avatar.h"
#include "behavior.h"
#include "behavior_oracle.h"
#include "cata_utility.h"
#include "color.h"
#include "cursesdef.h"
#include "effect.h"
#include "game.h"
#include "game_ui.h"
#include "input.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "martialarts.h"
#include "options.h"
#include "messages.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "output.h"
#include "path_info.h"
#include "player.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "bodypart.h"
#include "calendar.h"
#include "catacharset.h"
#include "compatibility.h"
#include "debug.h"
#include "enums.h"
#include "game_constants.h"
#include "int_id.h"
#include "omdata.h"
#include "pldata.h"
#include "string_formatter.h"
#include "tileray.h"
#include "type_id.h"

static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );

const efftype_id effect_got_checked( "got_checked" );

// constructor
window_panel::window_panel( std::function<void( avatar &, const catacurses::window & )>
                            draw_func, const std::string &nm, int ht, int wd, bool def_toggle,
                            std::function<bool()> render_func,  bool force_draw )
{
    draw = draw_func;
    name = nm;
    height = ht;
    width = wd;
    toggle = def_toggle;
    default_toggle = def_toggle;
    always_draw = force_draw;
    render = render_func;
}

// ====================================
// panels prettify and helper functions
// ====================================

static std::string trunc_ellipse( const std::string &input, unsigned int trunc )
{
    if( utf8_width( input ) > static_cast<int>( trunc ) ) {
        return utf8_truncate( input, trunc - 1 ) + "…";
    }
    return input;
}

static void draw_rectangle( const catacurses::window &w, nc_color, point top_left,
                            point bottom_right )
{
    // corners
    mvwaddch( w, top_left.y, top_left.x, LINE_OXXO );
    mvwaddch( w, bottom_right.y, top_left.x, LINE_XXOO );
    mvwaddch( w, top_left.y, bottom_right.x, LINE_OOXX );
    mvwaddch( w, bottom_right.y, bottom_right.x, LINE_XOOX );

    for( int i = 1; i < bottom_right.x; i++ ) {
        mvwaddch( w, top_left.y, i, LINE_OXOX );
        mvwaddch( w, bottom_right.y, i, LINE_OXOX );
    }

    for( int i = 1; i < bottom_right.y; i++ ) {
        mvwaddch( w, i, top_left.x, LINE_XOXO );
        mvwaddch( w, i, bottom_right.x, LINE_XOXO );
    }
}

static std::pair<nc_color, std::string> str_string( const avatar &p )
{
    nc_color clr;

    if( p.get_str() == p.get_str_base() ) {
        clr = c_white;
    } else if( p.get_str() > p.get_str_base() ) {
        clr = c_green;
    } else if( p.get_str() < p.get_str_base() ) {
        clr = c_red;
    }
    return std::make_pair( clr, _( "Str " ) + ( p.get_str() < 100 ? to_string( p.get_str() ) : "++" ) );
}

static std::pair<nc_color, std::string> dex_string( const avatar &p )
{
    nc_color clr;

    if( p.get_dex() == p.get_dex_base() ) {
        clr = c_white;
    } else if( p.get_dex() > p.get_dex_base() ) {
        clr = c_green;
    } else if( p.get_dex() < p.get_dex_base() ) {
        clr = c_red;
    }
    return std::make_pair( clr, _( "Dex " ) + ( p.get_dex() < 100 ? to_string( p.get_dex() ) : "++" ) );
}

static std::pair<nc_color, std::string> int_string( const avatar &p )
{
    nc_color clr;

    if( p.get_int() == p.get_int_base() ) {
        clr = c_white;
    } else if( p.get_int() > p.get_int_base() ) {
        clr = c_green;
    } else if( p.get_int() < p.get_int_base() ) {
        clr = c_red;
    }
    return std::make_pair( clr, _( "Int " ) + ( p.get_int() < 100 ? to_string( p.get_int() ) : "++" ) );
}

static std::pair<nc_color, std::string> per_string( const avatar &p )
{
    nc_color clr;

    if( p.get_per() == p.get_per_base() ) {
        clr = c_white;
    } else if( p.get_per() > p.get_per_base() ) {
        clr = c_green;
    } else if( p.get_per() < p.get_per_base() ) {
        clr = c_red;
    }
    return std::make_pair( clr, _( "Per " ) + ( p.get_per() < 100 ? to_string( p.get_per() ) : "++" ) );
}

static nc_color focus_color( int focus )
{
    if( focus < 25 ) {
        return c_red;
    } else if( focus < 50 ) {
        return c_light_red;
    } else if( focus < 75 ) {
        return c_yellow;
    } else if( focus < 100 ) {
        return c_light_gray;
    } else if( focus < 125 ) {
        return c_white;
    } else {
        return c_green;
    }
}

int window_panel::get_height() const
{
    if( height == -1 ) {
        if( g->pixel_minimap_option ) {
            return  get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) > 0 ?
                    get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) :
                    width / 2;
        } else {
            return 0;
        }
    }
    return height;
}

int window_panel::get_width() const
{
    return width;
}

std::string window_panel::get_name() const
{
    return name;
}

static void draw_minimap( const avatar &u, const catacurses::window &w_minimap )
{
    const tripoint curs = u.global_omt_location();
    const int cursx = curs.x;
    const int cursy = curs.y;
    const tripoint targ = u.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;

    for( int i = -2; i <= 2; i++ ) {
        for( int j = -2; j <= 2; j++ ) {
            const int omx = cursx + i;
            const int omy = cursy + j;
            nc_color ter_color;
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omx, omy, g->get_levz() );
            const bool vehicle_here = overmap_buffer.has_vehicle( omx, omy, g->get_levz() );
            if( overmap_buffer.has_note( omx, omy, g->get_levz() ) ) {

                const std::string &note_text = overmap_buffer.note( omx, omy, g->get_levz() );

                ter_color = c_yellow;
                ter_sym = "N";

                int symbolIndex = note_text.find( ':' );
                int colorIndex = note_text.find( ';' );

                bool symbolFirst = symbolIndex < colorIndex;

                if( colorIndex > -1 && symbolIndex > -1 ) {
                    if( symbolFirst ) {
                        if( colorIndex > 4 ) {
                            colorIndex = -1;
                        }
                        if( symbolIndex > 1 ) {
                            symbolIndex = -1;
                            colorIndex = -1;
                        }
                    } else {
                        if( symbolIndex > 4 ) {
                            symbolIndex = -1;
                        }
                        if( colorIndex > 2 ) {
                            colorIndex = -1;
                        }
                    }
                } else if( colorIndex > 2 ) {
                    colorIndex = -1;
                } else if( symbolIndex > 1 ) {
                    symbolIndex = -1;
                }

                if( symbolIndex > -1 ) {
                    int symbolStart = 0;
                    if( colorIndex > -1 && !symbolFirst ) {
                        symbolStart = colorIndex + 1;
                    }
                    ter_sym = note_text.substr( symbolStart, symbolIndex - symbolStart );
                }

                if( colorIndex > -1 ) {

                    int colorStart = 0;

                    if( symbolIndex > -1 && symbolFirst ) {
                        colorStart = symbolIndex + 1;
                    }

                    std::string sym = note_text.substr( colorStart, colorIndex - colorStart );

                    if( sym.length() == 2 ) {
                        if( sym == "br" ) {
                            ter_color = c_brown;
                        } else if( sym == "lg" ) {
                            ter_color = c_light_gray;
                        } else if( sym == "dg" ) {
                            ter_color = c_dark_gray;
                        }
                    } else {
                        char colorID = sym.c_str()[0];
                        if( colorID == 'r' ) {
                            ter_color = c_light_red;
                        } else if( colorID == 'R' ) {
                            ter_color = c_red;
                        } else if( colorID == 'g' ) {
                            ter_color = c_light_green;
                        } else if( colorID == 'G' ) {
                            ter_color = c_green;
                        } else if( colorID == 'b' ) {
                            ter_color = c_light_blue;
                        } else if( colorID == 'B' ) {
                            ter_color = c_blue;
                        } else if( colorID == 'W' ) {
                            ter_color = c_white;
                        } else if( colorID == 'C' ) {
                            ter_color = c_cyan;
                        } else if( colorID == 'c' ) {
                            ter_color = c_light_cyan;
                        } else if( colorID == 'P' ) {
                            ter_color = c_pink;
                        } else if( colorID == 'm' ) {
                            ter_color = c_magenta;
                        }
                    }
                }
            } else if( !seen ) {
                ter_sym = " ";
                ter_color = c_black;
            } else if( vehicle_here ) {
                ter_color = c_cyan;
                ter_sym = "c";
            } else {
                const oter_id &cur_ter = overmap_buffer.ter( omx, omy, g->get_levz() );
                ter_sym = cur_ter->get_symbol();
                if( overmap_buffer.is_explored( omx, omy, g->get_levz() ) ) {
                    ter_color = c_dark_gray;
                } else {
                    ter_color = cur_ter->get_color();
                }
            }
            if( !drew_mission && targ.x == omx && targ.y == omy ) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
                drew_mission = true;
                if( i != 0 || j != 0 ) {
                    ter_color = red_background( ter_color );
                }
            }
            if( i == 0 && j == 0 ) {
                mvwputch_hi( w_minimap, 3, 3, ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, 3 + j, 3 + i, ter_color, ter_sym );
            }
        }
    }

    // Print arrow to mission if we have one!
    if( !drew_mission ) {
        double slope = ( cursx != targ.x ) ? static_cast<double>( targ.y - cursy ) / static_cast<double>
                       ( targ.x - cursx ) : 4;

        if( cursx == targ.x || fabs( slope ) > 3.5 ) { // Vertical slope
            if( targ.y > cursy ) {
                mvwputch( w_minimap, 6, 3, c_red, '*' );
            } else {
                mvwputch( w_minimap, 0, 3, c_red, '*' );
            }
        } else {
            int arrowx = -1;
            int arrowy = -1;
            if( fabs( slope ) >= 1. ) { // y diff is bigger!
                arrowy = ( targ.y > cursy ? 6 : 0 );
                arrowx = static_cast<int>( 3 + 3 * ( targ.y > cursy ? slope : ( 0 - slope ) ) );
                if( arrowx < 0 ) {
                    arrowx = 0;
                }
                if( arrowx > 6 ) {
                    arrowx = 6;
                }
            } else {
                arrowx = ( targ.x > cursx ? 6 : 0 );
                arrowy = static_cast<int>( 3 + 3 * ( targ.x > cursx ? slope : ( 0 - slope ) ) );
                if( arrowy < 0 ) {
                    arrowy = 0;
                }
                if( arrowy > 6 ) {
                    arrowy = 6;
                }
            }
            char glyph = '*';
            if( targ.z > u.posz() ) {
                glyph = '^';
            } else if( targ.z < u.posz() ) {
                glyph = 'v';
            }

            mvwputch( w_minimap, arrowy, arrowx, c_red, glyph );
        }
    }

    const int sight_points = g->u.overmap_sight_range( g->light_level( g->u.posz() ) );
    for( int i = -3; i <= 3; i++ ) {
        for( int j = -3; j <= 3; j++ ) {
            if( i > -3 && i < 3 && j > -3 && j < 3 ) {
                continue; // only do hordes on the border, skip inner map
            }
            const int omx = cursx + i;
            const int omy = cursy + j;
            if( overmap_buffer.get_horde_size( omx, omy, g->get_levz() ) >= HORDE_VISIBILITY_SIZE ) {
                const tripoint cur_pos{
                    omx, omy, g->get_levz()
                };
                if( overmap_buffer.seen( omx, omy, g->get_levz() )
                    && g->u.overmap_los( cur_pos, sight_points ) ) {
                    mvwputch( w_minimap, j + 3, i + 3, c_green,
                              overmap_buffer.get_horde_size( omx, omy, g->get_levz() ) > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
                }
            }
        }
    }
}

static void decorate_panel( const std::string &name, const catacurses::window &w )
{
    werase( w );
    draw_border( w );

    static const char *title_prefix = " ";
    const std::string &title = name;
    static const char *title_suffix = " ";
    static const std::string full_title = string_format( "%s%s%s",
                                          title_prefix, title, title_suffix );
    const int start_pos = center_text_pos( full_title, 0, getmaxx( w ) - 1 );
    mvwprintz( w, 0, start_pos, c_white, title_prefix );
    wprintz( w, c_light_red, title );
    wprintz( w, c_white, title_suffix );
}

static std::string get_temp( const avatar &u )
{
    std::string temp;
    if( u.has_item_with_flag( "THERMOMETER" ) ||
        u.has_bionic( bionic_id( "bio_meteorologist" ) ) ) {
        temp = print_temperature( g->weather.get_temperature( u.pos() ) );
    }
    if( static_cast<int>( temp.size() ) == 0 ) {
        return "-";
    }
    return temp;
}

static std::string get_moon_graphic()
{
    //moon phase display
    static std::vector<std::string> vMoonPhase = { "(   )", "(  ))", "( | )", "((  )" };

    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    std::string sPhase = vMoonPhase[iPhase % 4];

    if( iPhase > 0 ) {
        sPhase.insert( 5 - ( ( iPhase > 4 ) ? iPhase % 4 : 0 ), "</color>" );
        sPhase.insert( 5 - ( ( iPhase < 4 ) ? iPhase + 1 : 5 ),
                       "<color_" + string_from_color( i_black ) + ">" );
    }
    return sPhase;
}

static std::string get_moon()
{
    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    switch( iPhase ) {
        case 0:
            return _( "New moon" );
        case 1:
            return _( "Waxing crescent" );
        case 2:
            return _( "Half moon" );
        case 3:
            return _( "Waxing gibbous" );
        case 4:
            return _( "Full moon" );
        case 5:
            return _( "Waning gibbous" );
        case 6:
            return _( "Half moon" );
        case 7:
            return _( "Waning crescent" );
        case 8:
            return _( "Dark moon" );
        default:
            return "";
    }
}

static std::string time_approx()
{
    const int iHour = hour_of_day<int>( calendar::turn );
    if( iHour >= 23 || iHour <= 1 ) {
        return _( "Around midnight" );
    } else if( iHour <= 4 ) {
        return _( "Dead of night" );
    } else if( iHour <= 6 ) {
        return _( "Around dawn" );
    } else if( iHour <= 8 ) {
        return _( "Early morning" );
    } else if( iHour <= 10 ) {
        return _( "Morning" );
    } else if( iHour <= 13 ) {
        return _( "Around noon" );
    } else if( iHour <= 16 ) {
        return _( "Afternoon" );
    } else if( iHour <= 18 ) {
        return _( "Early evening" );
    } else if( iHour <= 20 ) {
        return _( "Around dusk" );
    }
    return _( "Night" );
}

static nc_color value_color( int stat )
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

static std::pair<nc_color, int> morale_stat( const avatar &u )
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

static std::pair<int, int> temp_delta( const avatar &u )
{
    int current_bp_extreme = 0;
    int conv_bp_extreme = 0;
    for( int i = 0; i < num_bp; i++ ) {
        if( abs( u.temp_cur[i] - BODYTEMP_NORM ) > abs( u.temp_cur[current_bp_extreme] - BODYTEMP_NORM ) ) {
            current_bp_extreme = i;
        }
        if( abs( u.temp_conv[i] - BODYTEMP_NORM ) > abs( u.temp_conv[conv_bp_extreme] - BODYTEMP_NORM ) ) {
            conv_bp_extreme = i;
        }
    }
    return std::make_pair( current_bp_extreme, conv_bp_extreme );
}

static int define_temp_level( const int lvl )
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

static std::string temp_delta_string( const avatar &u )
{
    std::string temp_message;
    std::pair<int, int> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.temp_cur[temp_pair.first] );
    const int conv_zone = define_temp_level( u.temp_conv[temp_pair.second] );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    if( delta > 2 ) {
        temp_message = _( " (Rising!!)" );
    } else if( delta == 2 ) {
        temp_message = _( " (Rising!)" );
    } else if( delta == 1 ) {
        temp_message = _( " (Rising)" );
    } else if( delta == 0 ) {
        temp_message.clear();
    } else if( delta == -1 ) {
        temp_message = _( " (Falling)" );
    } else if( delta == -2 ) {
        temp_message = _( " (Falling!)" );
    } else {
        temp_message = _( " (Falling!!)" );
    }
    return temp_message;
}

static std::pair<nc_color, std::string> temp_delta_arrows( const avatar &u )
{
    std::string temp_message;
    nc_color temp_color = c_white;
    std::pair<int, int> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.temp_cur[temp_pair.first] );
    const int conv_zone = define_temp_level( u.temp_conv[temp_pair.second] );

    // delta will be positive if temp_cur is rising
    const int delta = conv_zone - cur_zone;
    // Decide if temp_cur is rising or falling
    if( delta > 2 ) {
        temp_message = " ↑↑↑";
        temp_color = c_red;
    } else if( delta == 2 ) {
        temp_message = " ↑↑";
        temp_color = c_light_red;
    } else if( delta == 1 ) {
        temp_message = " ↑";
        temp_color = c_yellow;
    } else if( delta == 0 ) {
        temp_message = "-";
        temp_color = c_green;
    } else if( delta == -1 ) {
        temp_message = " ↓";
        temp_color = c_light_blue;
    } else if( delta == -2 ) {
        temp_message = " ↓↓";
        temp_color = c_cyan;
    } else {
        temp_message = " ↓↓↓";
        temp_color = c_blue;
    }
    return std::make_pair( temp_color, temp_message );
}

static std::pair<nc_color, std::string> temp_stat( const avatar &u )
{
    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    int current_bp_extreme = temp_delta( u ).first;

    // printCur the hottest/coldest bodypart
    std::string temp_string;
    nc_color temp_color = c_yellow;
    if( u.temp_cur[current_bp_extreme] > BODYTEMP_SCORCHING ) {
        temp_color = c_red;
        temp_string = _( "Scorching!" );
    } else if( u.temp_cur[current_bp_extreme] > BODYTEMP_VERY_HOT ) {
        temp_color = c_light_red;
        temp_string = _( "Very hot!" );
    } else if( u.temp_cur[current_bp_extreme] > BODYTEMP_HOT ) {
        temp_color = c_yellow;
        temp_string = _( "Warm" );
    } else if( u.temp_cur[current_bp_extreme] > BODYTEMP_COLD ) {
        temp_color = c_green;
        temp_string = _( "Comfortable" );
    } else if( u.temp_cur[current_bp_extreme] > BODYTEMP_VERY_COLD ) {
        temp_color = c_light_blue;
        temp_string = _( "Chilly" );
    } else if( u.temp_cur[current_bp_extreme] > BODYTEMP_FREEZING ) {
        temp_color = c_cyan;
        temp_string = _( "Very cold!" );
    } else if( u.temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING ) {
        temp_color = c_blue;
        temp_string = _( "Freezing!" );
    }
    return std::make_pair( temp_color, temp_string );
}

static std::pair<nc_color, std::string> pain_stat( const avatar &u )
{
    nc_color pain_color = c_yellow;
    std::string pain_string;
    // get pain color
    if( u.get_perceived_pain() >= 60 ) {
        pain_color = c_red;
    } else if( u.get_perceived_pain() >= 40 ) {
        pain_color = c_light_red;
    }
    // get pain string
    if( ( u.has_trait( trait_SELFAWARE ) || u.has_effect( effect_got_checked ) ) &&
        u.get_perceived_pain() > 0 ) {
        pain_string = string_format( "%s %d", _( "Pain " ), u.get_perceived_pain() );
    } else if( u.get_perceived_pain() > 0 ) {
        pain_string = u.get_pain_description();
    }
    return std::make_pair( pain_color, pain_string );
}

static std::string get_armor( const avatar &u, body_part bp, unsigned int truncate = 0 )
{
    for( std::list<item>::const_iterator it = u.worn.end(); it != u.worn.begin(); ) {
        --it;
        if( it->covers( bp ) ) {
            return it->tname( 1, true, truncate );
        }
    }
    return "-";
}

static face_type get_face_type( const avatar &u )
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

static std::string morale_emotion( const int morale_cur, const face_type face,
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

static std::pair<nc_color, std::string> power_stat( const avatar &u )
{
    nc_color c_pwr = c_red;
    std::string s_pwr;
    if( u.max_power_level == 0 ) {
        s_pwr = "--";
        c_pwr = c_light_gray;
    } else {
        if( u.power_level >= u.max_power_level / 2 ) {
            c_pwr = c_light_blue;
        } else if( u.power_level >= u.max_power_level / 3 ) {
            c_pwr = c_yellow;
        } else if( u.power_level >= u.max_power_level / 4 ) {
            c_pwr = c_red;
        }
        s_pwr = to_string( u.power_level );
    }
    return std::make_pair( c_pwr, s_pwr );
}

static std::pair<nc_color, std::string> mana_stat( const player &u )
{
    nc_color c_mana = c_red;
    std::string s_mana;
    if( u.magic.max_mana( u ) <= 0 ) {
        s_mana = "--";
        c_mana = c_light_gray;
    } else {
        if( u.magic.available_mana() >= u.magic.max_mana( u ) / 2 ) {
            c_mana = c_light_blue;
        } else if( u.magic.available_mana() >= u.magic.max_mana( u ) / 3 ) {
            c_mana = c_yellow;
        }
        s_mana = to_string( u.magic.available_mana() );
    }
    return std::make_pair( c_mana, s_mana );
}

static nc_color safe_color()
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

static int get_int_digits( const int &digits )
{
    int temp = abs( digits );
    if( digits > 0 ) {
        return static_cast<int>( log10( static_cast<double>( temp ) ) ) + 1;
    } else if( digits < 0 ) {
        return static_cast<int>( log10( static_cast<double>( temp ) ) ) + 2;
    }
    return 1;
}

// ===============================
// panels code
// ===============================

static void draw_limb_health( avatar &u, const catacurses::window &w, int limb_index )
{
    const bool no_feeling = u.has_trait( trait_NOPAIN );
    const bool is_self_aware = u.has_trait( trait_SELFAWARE ) && !no_feeling;
    static auto print_symbol_num = []( const catacurses::window & w, int num, const std::string & sym,
    const nc_color & color ) {
        while( num-- > 0 ) {
            wprintz( w, color, sym );
        }
    };
    if( u.hp_cur[limb_index] == 0 && ( limb_index >= hp_arm_l && limb_index <= hp_leg_r ) ) {
        //Limb is broken
        std::string limb = "~~%~~";
        nc_color color = c_light_red;

        const auto bp = u.hp_to_bp( static_cast<hp_part>( limb_index ) );
        if( u.worn_with_flag( "SPLINT", bp ) ) {
            static const efftype_id effect_mending( "mending" );
            const auto &eff = u.get_effect( effect_mending, bp );
            const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

            if( is_self_aware || u.has_effect( effect_got_checked ) ) {
                limb = string_format( "=%2d%%=", mend_perc );
                color = c_blue;
            } else if( !no_feeling ) {
                const int num = mend_perc / 20;
                print_symbol_num( w, num, "#", c_blue );
                print_symbol_num( w, 5 - num, "=", c_blue );
                return;
            } else {
                color = c_blue;
            }
        }

        wprintz( w, color, limb );
        return;
    }

    std::pair<std::string, nc_color> hp = get_hp_bar( u.hp_cur[limb_index], u.hp_max[limb_index] );

    if( is_self_aware || u.has_effect( effect_got_checked ) ) {
        wprintz( w, hp.second, "%3d  ", u.hp_cur[limb_index] );
    } else if( no_feeling ) {
        if( u.hp_cur[limb_index] < u.hp_max[limb_index] / 2 ) {
            hp = std::make_pair( string_format( " %s", _( "Bad" ) ), c_red );
        } else {
            hp = std::make_pair( string_format( " %s", _( "Good" ) ), c_green );
        }
        wprintz( w, hp.second, hp.first );
    } else {
        wprintz( w, hp.second, hp.first );

        //Add the trailing symbols for a not-quite-full health bar
        print_symbol_num( w, 5 - static_cast<int>( hp.first.size() ), ".", c_white );
    }
}

static void draw_limb2( avatar &u, const catacurses::window &w )
{
    static std::array<body_part, 6> part = { {
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };

    werase( w );
    // print limb health
    for( int i = 0; i < num_hp_parts; i++ ) {
        const std::string str = body_part_hp_bar_ui_text( part[i] );
        if( i % 2 == 0 ) {
            wmove( w, i / 2, 0 );
        } else {
            wmove( w, i / 2, 11 );
        }
        wprintz( w, u.limb_color( part[i], true, true, true ), str );
        if( i % 2 == 0 ) {
            wmove( w, i / 2, 5 );
        } else {
            wmove( w, i / 2, 16 );
        }
        draw_limb_health( u, w, i );
    }

    // print mood
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );

    // print safe mode
    std::string safe_str;
    if( g->safe_mode || get_option<bool>( "AUTOSAFEMODE" ) ) {
        safe_str = _( "SAFE" );
    }
    mvwprintz( w, 2, 22, safe_color(), safe_str );
    mvwprintz( w, 2, 27, morale_pair.first, smiley );

    // print stamina
    const auto &stamina = get_hp_bar( u.stamina, u.get_stamina_max() );
    mvwprintz( w, 0, 22, c_light_gray, _( "STM" ) );
    mvwprintz( w, 0, 26, stamina.second, stamina.first );

    mvwprintz( w, 1, 22, c_light_gray, _( "PWR" ) );
    const auto pwr = power_stat( u );
    mvwprintz( w, 1, 31 - pwr.second.length(), pwr.first, pwr.second );

    wrefresh( w );
}

static void draw_stats( avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color stat_clr = str_string( u ).first;
    mvwprintz( w, 0, 0, c_light_gray, _( "STR" ) );
    mvwprintz( w, 0, u.str_cur < 10 ? 5 : 4, stat_clr,
               u.str_cur < 100 ? to_string( u.str_cur ) : "99+" );
    stat_clr = dex_string( u ).first;
    mvwprintz( w, 0, 9, c_light_gray, _( "DEX" ) );
    mvwprintz( w, 0, u.dex_cur < 10 ? 14 : 13, stat_clr,
               u.dex_cur < 100 ? to_string( u.dex_cur ) : "99+" );
    stat_clr = int_string( u ).first;
    mvwprintz( w, 0, 17, c_light_gray, _( "INT" ) );
    mvwprintz( w, 0, u.int_cur < 10 ? 22 : 21, stat_clr,
               u.int_cur < 100 ? to_string( u.int_cur ) : "99+" );
    stat_clr = per_string( u ).first;
    mvwprintz( w, 0, 25, c_light_gray, _( "PER" ) );
    mvwprintz( w, 0, u.per_cur < 10 ? 30 : 29, stat_clr,
               u.per_cur < 100 ? to_string( u.per_cur ) : "99+" );
    wrefresh( w );
}

static void draw_stealth( avatar &u, const catacurses::window &w )
{
    werase( w );
    mvwprintz( w, 0, 0, c_light_gray, _( "Speed" ) );
    mvwprintz( w, 0, 7, value_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w, 0, 15 - to_string( u.movecounter ).length(), c_light_gray,
               to_string( u.movecounter ) + ( u.get_movement_mode() == "walk" ? "W" :
                       ( u.get_movement_mode() == "crouch" ? "C" :
                         "R" ) ) );

    if( u.is_deaf() ) {
        mvwprintz( w, 0, 22, c_red, _( "DEAF" ) );
    } else {
        mvwprintz( w, 0, 20, c_light_gray, _( "Sound:" ) );
        const std::string snd = to_string( u.volume );
        mvwprintz( w, 0, 30 - snd.length(), u.volume != 0 ? c_yellow : c_light_gray, snd );
    }

    wrefresh( w );
}

static void draw_time_graphic( const catacurses::window &w )
{
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
    wprintz( w, c_white, "[" );
    bool bAddTrail = false;

    for( int i = 0; i < 14; i += 2 ) {
        if( iHour >= 8 + i && iHour <= 13 + ( i / 2 ) ) {
            wputch( w, hilite( c_white ), ' ' );

        } else if( iHour >= 6 + i && iHour <= 7 + i ) {
            wputch( w, hilite( vGlyphs[i].second ), vGlyphs[i].first );
            bAddTrail = true;

        } else if( iHour >= ( 18 + i ) % 24 && iHour <= ( 19 + i ) % 24 ) {
            wputch( w, vGlyphs[i + 1].second, vGlyphs[i + 1].first );

        } else if( bAddTrail && iHour >= 6 + ( i / 2 ) ) {
            wputch( w, hilite( c_white ), ' ' );

        } else {
            wputch( w, c_white, ' ' );
        }
    }

    wprintz( w, c_white, "]" );
}

static void draw_time( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // display date
    mvwprintz( w, 0, 0, c_light_gray, calendar::name_season( season_of_year( calendar::turn ) ) );
    std::string day = to_string( day_of_season<int>( calendar::turn ) + 1 );
    mvwprintz( w, 0, 10 - day.length(), c_light_gray, day );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, 0, 11, c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( g->get_levz() >= 0 ) {
        wmove( w, 0, 11 );
        draw_time_graphic( w );
    } else {
        mvwprintz( w, 0, 11, c_light_gray, _( "Time: ???" ) );
    }
    //display moon
    mvwprintz( w, 0, 22, c_white, _( "Moon" ) );
    nc_color clr = c_white;
    print_colored_text( w, 0, 27, clr, c_white, get_moon_graphic() );

    wrefresh( w );
}

static void draw_needs( const avatar &u, const catacurses::window &w )
{
    werase( w );

    auto hunger_pair = u.get_hunger_description();
    mvwprintz( w, 0, 0, hunger_pair.second, hunger_pair.first );
    hunger_pair = u.get_fatigue_description();
    mvwprintz( w, 1, 0, hunger_pair.second, hunger_pair.first );
    auto pair = pain_stat( u );
    mvwprintz( w, 2, 0, pair.first, pair.second );

    hunger_pair = u.get_thirst_description();
    mvwprintz( w, 0, 17, hunger_pair.second, hunger_pair.first );
    pair = temp_stat( u );
    mvwprintz( w, 1, 17, pair.first, pair.second );
    const auto arrow = temp_delta_arrows( u );
    mvwprintz( w, 1, 17 + utf8_width( pair.second ), arrow.first, arrow.second );

    mvwprintz( w, 2, 17, c_light_gray, _( "Focus" ) );
    mvwprintz( w, 2, 24, focus_color( u.focus_pool ), to_string( u.focus_pool ) );

    wrefresh( w );
}

static void draw_limb( avatar &u, const catacurses::window &w )
{
    werase( w );
    int ny2 = 0;
    for( int i = 0; i < num_hp_parts; i++ ) {
        int ny;
        int nx;
        if( i < 3 ) {
            ny = i;
            nx = 8;
        } else {
            ny = ny2++;
            nx = 26;
        }
        wmove( w, ny, nx );
        draw_limb_health( u, w, i );
    }

    // display limbs status
    static std::array<body_part, 6> part = { {
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };
    ny2 = 0;
    for( size_t i = 0; i < part.size(); i++ ) {
        int ny;
        int nx;
        if( i < 3 ) {
            ny = i;
            nx = 1;
        } else {
            ny = ny2++;
            nx = 19;
        }

        std::string str = body_part_hp_bar_ui_text( part[i] );
        wmove( w, ny, nx );
        while( str.length() < 5 ) {
            str = str + " ";
        }
        wprintz( w, u.limb_color( part[i], true, true, true ), str + ":" );
    }
    wrefresh( w );
}

static void draw_char( avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    mvwprintz( w, 0, 1, c_light_gray, _( "Sound:" ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Stam :" ) );
    mvwprintz( w, 2, 1, c_light_gray, _( "Focus:" ) );
    mvwprintz( w, 0, 19, c_light_gray, _( "Mood :" ) );
    mvwprintz( w, 1, 19, c_light_gray, _( "Speed:" ) );
    mvwprintz( w, 2, 19, c_light_gray, _( "Move :" ) );

    const auto str_walk = pgettext( "movement-type", "W" );
    const auto str_run = pgettext( "movement-type", "R" );
    const auto str_crouch = pgettext( "movement-type", "C" );
    const char *move = u.get_movement_mode() == "walk" ? str_walk : ( u.get_movement_mode() == "crouch"
                       ? str_crouch :
                       str_run );
    std::string movecost = std::to_string( u.movecounter ) + "(" + move + ")";
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );
    mvwprintz( w, 0, 8, c_light_gray, "%s", u.volume );

    // print stamina
    auto needs_pair = std::make_pair( get_hp_bar( u.stamina, u.get_stamina_max() ).second,
                                      get_hp_bar( u.stamina, u.get_stamina_max() ).first );
    mvwprintz( w, 1, 8, needs_pair.first, needs_pair.second );
    for( size_t i = 0; i < 5 - needs_pair.second.length(); i++ ) {
        mvwprintz( w, 1, 12 - i, c_white, "." );
    }

    mvwprintz( w, 2, 8, focus_color( u.focus_pool ), "%s", u.focus_pool );
    mvwprintz( w, 0, 26, morale_pair.first, "%s", smiley );
    mvwprintz( w, 1, 26, focus_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w, 2, 26, c_light_gray, "%s", movecost );
    wrefresh( w );
}

static void draw_stat( avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, 0, 1, c_light_gray, _( "Str  :" ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Int  :" ) );
    mvwprintz( w, 0, 19, c_light_gray, _( "Dex  :" ) );
    mvwprintz( w, 1, 19, c_light_gray, _( "Per  :" ) );

    nc_color stat_clr = str_string( u ).first;
    mvwprintz( w, 0, 8, stat_clr, "%s", u.str_cur );
    stat_clr = int_string( u ).first;
    mvwprintz( w, 1, 8, stat_clr, "%s", u.int_cur );
    stat_clr = dex_string( u ).first;
    mvwprintz( w, 0, 26, stat_clr, "%s", u.dex_cur );
    stat_clr = per_string( u ).first;
    mvwprintz( w, 1, 26, stat_clr, "%s", u.per_cur );

    std::pair<nc_color, std::string> pwr_pair = power_stat( u );
    mvwprintz( w, 2, 1, c_light_gray, _( "Power:" ) );
    mvwprintz( w, 2, 19, c_light_gray, _( "Safe :" ) );
    mvwprintz( w, 2, 8, pwr_pair.first, "%s", pwr_pair.second );
    mvwprintz( w, 2, 26, safe_color(), g->safe_mode ? _( "On" ) : _( "Off" ) );
    wrefresh( w );
}

static void draw_env1( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // display location
    const oter_id &cur_ter = overmap_buffer.ter( u.global_omt_location() );
    mvwprintz( w, 0, 1, c_light_gray, _( "Place: " ) );
    wprintz( w, c_white, utf8_truncate( cur_ter->get_name(), getmaxx( w ) - 13 ) );
    // display weather
    if( g->get_levz() < 0 ) {
        mvwprintz( w, 1, 1, c_light_gray, _( "Sky  : Underground" ) );
    } else {
        mvwprintz( w, 1, 1, c_light_gray, _( "Sky  :" ) );
        const weather_datum wdata = weather_data( g->weather.weather );
        wprintz( w, wdata.color, " %s", wdata.name );
    }
    // display lighting
    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( w, 2, 1, c_light_gray, "%s ", _( "Light:" ) );
    wprintz( w, ll.second, ll.first );

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

static void draw_env2( const avatar &u, const catacurses::window &w )
{
    werase( w );
    mvwprintz( w, 0, 1, c_light_gray, _( "Moon : %s" ), get_moon() );
    mvwprintz( w, 1, 1, c_light_gray, _( "Temp : %s" ), get_temp( u ) );
    wrefresh( w );
}

static void draw_weapon_labels( const avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color color = c_light_gray;
    mvwprintz( w, 0, 1, c_light_gray, _( "Wield:" ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Style:" ) );
    print_colored_text( w, 0, 8, color, c_light_gray, u.weapname( getmaxx( w ) - 8 ) );
    mvwprintz( w, 1, 8, c_light_gray, u.get_combat_style().name );
    wrefresh( w );
}

static void draw_mod1( const avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<std::string, nc_color> hunger_pair = u.get_hunger_description();
    std::pair<std::string, nc_color> thirst_pair = u.get_thirst_description();
    std::pair<std::string, nc_color> rest_pair = u.get_fatigue_description();
    std::pair<nc_color, std::string> temp_pair = temp_stat( u );
    std::pair<nc_color, std::string> pain_pair = pain_stat( u );
    mvwprintz( w, 0, 1, c_light_gray, _( "Food :" ) );
    mvwprintz( w, 1, 1, c_light_gray, _( "Drink:" ) );
    mvwprintz( w, 2, 1, c_light_gray, _( "Rest :" ) );
    mvwprintz( w, 3, 1, c_light_gray, _( "Pain :" ) );
    mvwprintz( w, 4, 1, c_light_gray, _( "Heat :" ) );
    mvwprintz( w, 0, 8, hunger_pair.second, hunger_pair.first );
    mvwprintz( w, 1, 8, thirst_pair.second, thirst_pair.first );
    mvwprintz( w, 2, 8, rest_pair.second, rest_pair.first );
    mvwprintz( w, 3, 8, pain_pair.first, pain_pair.second );
    mvwprintz( w, 4, 8, temp_pair.first, temp_pair.second );
    wrefresh( w );
}

static void draw_env_compact( avatar &u, const catacurses::window &w )
{
    werase( w );

    draw_minimap( u, w );
    // wielded item
    nc_color color = c_light_gray;
    print_colored_text( w, 0, 8, color, c_light_gray, u.weapname( getmaxx( w ) - 8 ) );
    // style
    mvwprintz( w, 1, 8, c_light_gray, u.get_combat_style().name );
    // location
    mvwprintz( w, 2, 8, c_white, utf8_truncate( overmap_buffer.ter(
                   u.global_omt_location() )->get_name(), getmaxx( w ) - 8 ) );
    // weather
    if( g->get_levz() < 0 ) {
        mvwprintz( w, 3, 8, c_light_gray, _( "Underground" ) );
    } else {
        const weather_datum wdata = weather_data( g->weather.weather );
        mvwprintz( w, 3, 8, wdata.color, wdata.name );
    }
    // display lighting
    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( w, 4, 8, ll.second, ll.first );
    // wind
    const oter_id &cur_om_ter = overmap_buffer.ter( u.global_omt_location() );
    double windpower = get_local_windpower( g->weather.windspeed, cur_om_ter,
                                            u.pos(), g->weather.winddirection, g->is_sheltered( u.pos() ) );
    mvwprintz( w, 5, 8, get_wind_color( windpower ),
               get_wind_desc( windpower ) + " " + get_wind_arrow( g->weather.winddirection ) );

    if( u.has_item_with_flag( "THERMOMETER" ) || u.has_bionic( bionic_id( "bio_meteorologist" ) ) ) {
        std::string temp = print_temperature( g->weather.get_temperature( u.pos() ) );
        mvwprintz( w, 5, 31 - utf8_width( temp ), c_light_gray, temp );
    }

    wrefresh( w );
}

static void draw_health_classic( avatar &u, const catacurses::window &w )
{
    static std::array<body_part, 6> part = { {
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };

    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( u.pos() ) );
    }

    werase( w );

    draw_minimap( u, w );
    draw_rectangle( w, c_light_gray, point( 0, 0 ), point( 6, 6 ) );

    // print limb health
    for( int i = 0; i < num_hp_parts; i++ ) {
        const std::string str = body_part_hp_bar_ui_text( part[i] );
        wmove( w, i, 8 );
        wprintz( w, u.limb_color( part[i], true, true, true ), str );
        wmove( w, i, 14 );
        draw_limb_health( u, w, i );
    }

    // needs
    auto needs_pair = u.get_hunger_description();
    mvwprintz( w, 1, 21, needs_pair.second, needs_pair.first );
    needs_pair = u.get_thirst_description();
    mvwprintz( w, 2, 21, needs_pair.second, needs_pair.first );
    mvwprintz( w, 4, 21, c_white, _( "Focus" ) );
    mvwprintz( w, 4, 27, c_white, to_string( u.focus_pool ) );
    needs_pair = u.get_fatigue_description();
    mvwprintz( w, 3, 21, needs_pair.second, needs_pair.first );
    auto pain_pair = pain_stat( u );
    mvwprintz( w, 0, 21, pain_pair.first, pain_pair.second );

    // print mood
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );
    mvwprintz( w, 1, 34, morale_pair.first, smiley );

    if( !veh ) {
        // stats
        auto pair = str_string( u );
        mvwprintz( w, 0, 38, pair.first, pair.second );
        pair = dex_string( u );
        mvwprintz( w, 1, 38, pair.first, pair.second );
        pair = int_string( u );
        mvwprintz( w, 2, 38, pair.first, pair.second );
        pair = per_string( u );
        mvwprintz( w, 3, 38, pair.first, pair.second );
    }

    // print safe mode// print safe mode
    std::string safe_str;
    if( g->safe_mode || get_option<bool>( "AUTOSAFEMODE" ) ) {
        safe_str = "SAFE";
    }
    mvwprintz( w, 4, 40, safe_color(), safe_str );

    // print stamina
    auto pair = std::make_pair( get_hp_bar( u.stamina, u.get_stamina_max() ).second,
                                get_hp_bar( u.stamina, u.get_stamina_max() ).first );
    mvwprintz( w, 5, 35, c_light_gray, _( "Stm" ) );
    mvwprintz( w, 5, 39, pair.first, pair.second );
    for( size_t i = 0; i < 5 - pair.second.length(); i++ ) {
        mvwprintz( w, 5, 43 - i, c_white, "." );
    }

    // speed
    if( !u.in_vehicle ) {
        mvwprintz( w, 5, 21, u.get_speed() < 100 ? c_red : c_white,
                   _( "Spd " ) + to_string( u.get_speed() ) );
        mvwprintz( w, 5, 26 + to_string( u.get_speed() ).length(), c_white,
                   to_string( u.movecounter ) + " " + ( u.get_movement_mode() == "walk" ? "W" :
                           ( u.get_movement_mode() == "crouch" ? "C" :
                             "R" ) ) );
    }

    // temperature
    pair = temp_stat( u );
    mvwprintz( w, 6, 21, pair.first, pair.second + temp_delta_string( u ) );

    // power
    pair = power_stat( u );
    mvwprintz( w, 6, 8, c_light_gray, _( "POWER" ) );
    mvwprintz( w, 6, 14, pair.first, pair.second );

    // vehicle display
    if( veh ) {
        veh->print_fuel_indicators( w, 2, 39 );
        mvwprintz( w, 4, 35, c_light_gray, to_string( ( veh->face.dir() + 90 ) % 360 ) + "°" );
        // target speed > current speed
        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );
        if( veh->cruise_on ) {
            int t_speed = static_cast<int>( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) );
            int c_speed = static_cast<int>( convert_velocity( veh->velocity, VU_VEHICLE ) );
            int offset = get_int_digits( t_speed );
            const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
            mvwprintz( w, 5, 21, c_light_gray, type );
            mvwprintz( w, 5, 26, col_vel, "%d", c_speed );
            mvwprintz( w, 5, 26 + offset, c_light_gray, ">" );
            mvwprintz( w, 5, 28 + offset, c_light_green, "%d", t_speed );
        }
    }

    wrefresh( w );
}

static void draw_mod2( const avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color color = c_light_gray;
    mvwprintz( w, 0, 1, color, _( "Head :" ) );
    mvwprintz( w, 1, 1, color, _( "Torso:" ) );
    mvwprintz( w, 2, 1, color, _( "Arms :" ) );
    mvwprintz( w, 3, 1, color, _( "Legs :" ) );
    mvwprintz( w, 4, 1, color, _( "Feet :" ) );

    unsigned int max_length = getmaxx( w ) - 8;
    print_colored_text( w, 0, 8, color, color, get_armor( u, bp_head, max_length ) );
    print_colored_text( w, 1, 8, color, color, get_armor( u, bp_torso, max_length ) );
    print_colored_text( w, 2, 8, color, color, get_armor( u, bp_arm_r, max_length ) );
    print_colored_text( w, 3, 8, color, color, get_armor( u, bp_leg_r, max_length ) );
    print_colored_text( w, 4, 8, color, color, get_armor( u, bp_foot_r, max_length ) );
    wrefresh( w );
}

static void draw_armor( const avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color color = c_light_gray;
    mvwprintz( w, 0, 0, color, _( "Head :" ) );
    mvwprintz( w, 1, 0, color, _( "Torso:" ) );
    mvwprintz( w, 2, 0, color, _( "Arms :" ) );
    mvwprintz( w, 3, 0, color, _( "Legs :" ) );
    mvwprintz( w, 4, 0, color, _( "Feet :" ) );

    unsigned int max_length = getmaxx( w ) - 7;
    print_colored_text( w, 0, 7, color, color, get_armor( u, bp_head, max_length ) );
    print_colored_text( w, 1, 7, color, color, get_armor( u, bp_torso, max_length ) );
    print_colored_text( w, 2, 7, color, color, get_armor( u, bp_arm_r, max_length ) );
    print_colored_text( w, 3, 7, color, color, get_armor( u, bp_leg_r, max_length ) );
    print_colored_text( w, 4, 7, color, color, get_armor( u, bp_foot_r, max_length ) );
    wrefresh( w );
}

static void draw_messages( avatar &, const catacurses::window &w )
{
    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 1, 0 /*topline*/, maxlength - 1, line );
    wrefresh( w );
}

static void draw_messages_classic( avatar &, const catacurses::window &w )
{
    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 0, 0 /*topline*/, maxlength, line );
    wrefresh( w );
}

#if defined(TILES)
static void draw_mminimap( avatar &, const catacurses::window &w )
{
    werase( w );
    g->draw_pixel_minimap( w );
    wrefresh( w );
}
#endif

static void draw_compass( avatar &, const catacurses::window &w )
{
    werase( w );
    g->mon_info( w );
    wrefresh( w );
}

static void draw_compass_padding( avatar &, const catacurses::window &w )
{
    werase( w );
    g->mon_info( w, 1 );
    wrefresh( w );
}

static void draw_veh_compact( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // vehicle display
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( u.pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, 0, 0 );
        mvwprintz( w, 0, 6, c_light_gray, to_string( ( veh->face.dir() + 90 ) % 360 ) + "°" );
        // target speed > current speed
        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );

        if( veh->cruise_on ) {
            int t_speed = static_cast<int>( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) );
            int c_speed = static_cast<int>( convert_velocity( veh->velocity, VU_VEHICLE ) );
            int offset = get_int_digits( t_speed );
            const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
            mvwprintz( w, 0, 12, c_light_gray, "%s :", type );
            mvwprintz( w, 0, 19, c_light_green, "%d", t_speed );
            mvwprintz( w, 0, 20 + offset, c_light_gray, "%s", ">" );
            mvwprintz( w, 0, 22 + offset, col_vel, "%d", c_speed );
        }
    }

    wrefresh( w );
}

static void draw_veh_padding( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // vehicle display
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( g->m.veh_at( u.pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, 0, 1 );
        mvwprintz( w, 0, 7, c_light_gray, to_string( ( veh->face.dir() + 90 ) % 360 ) + "°" );
        // target speed > current speed
        const float strain = veh->strain();
        nc_color col_vel = strain <= 0 ? c_light_blue :
                           ( strain <= 0.2 ? c_yellow :
                             ( strain <= 0.4 ? c_light_red : c_red ) );
        if( veh->cruise_on ) {
            int t_speed = static_cast<int>( convert_velocity( veh->cruise_velocity, VU_VEHICLE ) );
            int c_speed = static_cast<int>( convert_velocity( veh->velocity, VU_VEHICLE ) );
            int offset = get_int_digits( t_speed );
            const std::string type = get_option<std::string>( "USE_METRIC_SPEEDS" );
            mvwprintz( w, 0, 13, c_light_gray, "%s :", type );
            mvwprintz( w, 0, 20, c_light_green, "%d", t_speed );
            mvwprintz( w, 0, 21 + offset, c_light_gray, "%s", ">" );
            mvwprintz( w, 0, 23 + offset, col_vel, "%d", c_speed );
        }
    }

    wrefresh( w );
}

static void draw_ai_goal( const avatar &u, const catacurses::window &w )
{
    werase( w );
    behavior::tree needs;
    needs.add( &string_id<behavior::node_t>( "npc_needs" ).obj() );
    behavior::character_oracle_t player_oracle( &u );
    std::string current_need = needs.tick( &player_oracle );
    mvwprintz( w, 0, 1, c_light_gray, _( "Goal: %s" ), current_need );
    wrefresh( w );
}

static void draw_location_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, 0, 0, c_light_gray, _( "Location:" ) );
    mvwprintz( w, 0, 10, c_white, utf8_truncate( overmap_buffer.ter(
                   u.global_omt_location() )->get_name(), getmaxx( w ) - 13 ) );

    wrefresh( w );
}

static void draw_weather_classic( avatar &, const catacurses::window &w )
{
    werase( w );

    if( g->get_levz() < 0 ) {
        mvwprintz( w, 0, 0, c_light_gray, _( "Underground" ) );
    } else {
        const weather_datum wdata = weather_data( g->weather.weather );
        mvwprintz( w, 0, 0, c_light_gray, _( "Weather :" ) );
        mvwprintz( w, 0, 10, wdata.color, wdata.name );
    }
    mvwprintz( w, 0, 31, c_light_gray, _( "Moon :" ) );
    nc_color clr = c_white;
    print_colored_text( w, 0, 38, clr, c_white, get_moon_graphic() );

    wrefresh( w );
}

static void draw_lighting_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( w, 0, 0, c_light_gray, _( "Lighting:" ) );
    mvwprintz( w, 0, 10, ll.second, ll.first );

    if( !u.is_deaf() ) {
        mvwprintz( w, 0, 31, c_light_gray, _( "Sound:" ) );
        mvwprintz( w, 0, 38, c_yellow, to_string( u.volume ) );
    } else {
        mvwprintz( w, 0, 31, c_red, _( "Deaf!" ) );
    }

    wrefresh( w );
}

static void draw_weapon_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, 0, 0, c_light_gray, _( "Weapon  :" ) );
    nc_color color = c_light_gray;
    print_colored_text( w, 0, 10, color, color, u.weapname( getmaxx( w ) - 24 ) );

    // Print in sidebar currently used martial style.
    std::string style;
    const auto &cur_style = u.style_selected.obj();
    if( !u.weapon.is_gun() ) {
        if( cur_style.force_unarmed || cur_style.weapon_valid( u.weapon ) ) {
            style = _( cur_style.name );
        } else if( u.is_armed() ) {
            style = _( "Normal" );
        } else {
            style = _( "No Style" );
        }
    }

    if( !style.empty() ) {
        const auto style_color = u.is_armed() ? c_red : c_blue;
        mvwprintz( w, 0, 31, style_color, style );
    }

    wrefresh( w );
}

static void draw_time_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // display date
    mvwprintz( w, 0, 0, c_white, calendar::name_season( season_of_year( calendar::turn ) ) + "," );
    std::string day = to_string( day_of_season<int>( calendar::turn ) + 1 );
    mvwprintz( w, 0, 8, c_white, _( "Day " ) + day );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, 0, 15, c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( g->get_levz() >= 0 ) {
        wmove( w, 0, 15 );
        draw_time_graphic( w );
    } else {
        mvwprintz( w, 0, 15, c_light_gray, _( "Time: ???" ) );
    }

    if( u.has_item_with_flag( "THERMOMETER" ) || u.has_bionic( bionic_id( "bio_meteorologist" ) ) ) {
        std::string temp = print_temperature( g->weather.get_temperature( u.pos() ) );
        mvwprintz( w, 0, 31, c_light_gray, _( "Temp : " ) + temp );
    }

    wrefresh( w );
}

static void draw_hint( const avatar &, const catacurses::window &w )
{
    werase( w );
    std::string press = press_x( ACTION_TOGGLE_PANEL_ADM );
    mvwprintz( w, 0, 1, c_light_green, press );
    mvwprintz( w, 0, 2 + utf8_width( press ), c_white, _( "to open sidebar options" ) );

    wrefresh( w );
}

static void draw_mana( const player &u, const catacurses::window &w )
{
    werase( w );

    auto mana_pair = mana_stat( u );
    const std::string mana_string = string_format( "%6s %5s %10s %5s", _( "Mana" ),
                                    colorize( mana_pair.second, mana_pair.first ), _( "Max Mana" ),
                                    colorize( to_string( u.magic.max_mana( u ) ), c_light_blue ) );
    nc_color gray = c_light_gray;
    print_colored_text( w, 0, getmaxx( w ) - mana_string.size(), gray, gray, mana_string );

    wrefresh( w );
}

// ============
// INITIALIZERS
// ============

static bool spell_panel()
{
    return !spell_type::get_all().empty();
}

bool default_render()
{
    return true;
}

static std::vector<window_panel> initialize_default_classic_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_health_classic, translate_marker( "Health" ), 7, 44, true ) );
    ret.emplace_back( window_panel( draw_location_classic, translate_marker( "Location" ), 1, 44,
                                    true ) );
    ret.emplace_back( window_panel( draw_mana, translate_marker( "Mana" ), 1, 44, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_weather_classic, translate_marker( "Weather" ), 1, 44,
                                    true ) );
    ret.emplace_back( window_panel( draw_lighting_classic, translate_marker( "Lighting" ), 1, 44,
                                    true ) );
    ret.emplace_back( window_panel( draw_weapon_classic, translate_marker( "Weapon" ), 1, 44, true ) );
    ret.emplace_back( window_panel( draw_time_classic, translate_marker( "Time" ), 1, 44, true ) );
    ret.emplace_back( window_panel( draw_armor, translate_marker( "Armor" ), 5, 44, false ) );
    ret.emplace_back( window_panel( draw_compass_padding, translate_marker( "Compass" ), 8, 44,
                                    true ) );
    ret.emplace_back( window_panel( draw_messages_classic, translate_marker( "Log" ), -2, 44, true ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, translate_marker( "Map" ), -1, 44, true,
                                    default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", 1, 44, false ) );
    return ret;
}

static std::vector<window_panel> initialize_default_compact_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_limb2, translate_marker( "Limbs" ), 3, 32, true ) );
    ret.emplace_back( window_panel( draw_stealth, translate_marker( "Sound" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_stats, translate_marker( "Stats" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_mana, translate_marker( "Mana" ), 1, 32, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_time, translate_marker( "Time" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_needs, translate_marker( "Needs" ), 3, 32, true ) );
    ret.emplace_back( window_panel( draw_env_compact, translate_marker( "Env" ), 6, 32, true ) );
    ret.emplace_back( window_panel( draw_veh_compact, translate_marker( "Vehicle" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_armor, translate_marker( "Armor" ), 5, 32, false ) );
    ret.emplace_back( window_panel( draw_messages_classic, translate_marker( "Log" ), -2, 32, true ) );
    ret.emplace_back( window_panel( draw_compass, translate_marker( "Compass" ), 8, 32, true ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, translate_marker( "Map" ), -1, 32, true,
                                    default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", 1, 32, false ) );

    return ret;
}

static std::vector<window_panel> initialize_default_label_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_hint, translate_marker( "Hint" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_limb, translate_marker( "Limbs" ), 3, 32, true ) );
    ret.emplace_back( window_panel( draw_char, translate_marker( "Movement" ), 3, 32, true ) );
    ret.emplace_back( window_panel( draw_mana, translate_marker( "Mana" ), 1, 32, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_stat, translate_marker( "Stats" ), 3, 32, true ) );
    ret.emplace_back( window_panel( draw_veh_padding, translate_marker( "Vehicle" ), 1, 32, true ) );
    ret.emplace_back( window_panel( draw_env1, translate_marker( "Location" ), 5, 32, true ) );
    ret.emplace_back( window_panel( draw_weapon_labels, translate_marker( "Weapon" ), 2, 32, true ) );
    ret.emplace_back( window_panel( draw_mod1, translate_marker( "Needs" ), 5, 32, true ) );
    ret.emplace_back( window_panel( draw_messages, translate_marker( "Log" ), -2, 32, true ) );
    ret.emplace_back( window_panel( draw_env2, translate_marker( "Moon" ), 2, 32, false ) );
    ret.emplace_back( window_panel( draw_mod2, translate_marker( "Armor" ), 5, 32, false ) );
    ret.emplace_back( window_panel( draw_compass_padding, translate_marker( "Compass" ), 8, 32,
                                    true ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, translate_marker( "Map" ), -1, 32, true,
                                    default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", 1, 32, false ) );

    return ret;
}

static std::map<std::string, std::vector<window_panel>> initialize_default_panel_layouts()
{
    std::map<std::string, std::vector<window_panel>> ret;

    ret.emplace( std::make_pair( translate_marker( "classic" ), initialize_default_classic_panels() ) );
    ret.emplace( std::make_pair( translate_marker( "compact" ), initialize_default_compact_panels() ) );
    ret.emplace( std::make_pair( translate_marker( "labels" ), initialize_default_label_panels() ) );

    return ret;
}

panel_manager::panel_manager()
{
    current_layout_id = "labels";
    layouts = initialize_default_panel_layouts();
}

std::vector<window_panel> &panel_manager::get_current_layout()
{
    auto kv = layouts.find( current_layout_id );
    if( kv != layouts.end() ) {
        return kv->second;
    }
    debugmsg( "Invalid current panel layout, defaulting to classic" );
    current_layout_id = "classic";
    return get_current_layout();
}

const std::string panel_manager::get_current_layout_id() const
{
    return current_layout_id;
}

int panel_manager::get_width_right()
{
    if( get_option<std::string>( "SIDEBAR_POSITION" ) == "left" ) {
        return width_left;
    }
    return width_right;
}

int panel_manager::get_width_left()
{
    if( get_option<std::string>( "SIDEBAR_POSITION" ) == "left" ) {
        return width_right;
    }
    return width_left;
}

void panel_manager::init()
{
    load();
    update_offsets( get_current_layout().begin()->get_width() );
}

void panel_manager::update_offsets( int x )
{
    width_right = x;
    width_left = 0;
}

bool panel_manager::save()
{
    return write_to_file( FILENAMES["panel_options"], [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );
    }, _( "panel options" ) );
}

bool panel_manager::load()
{
    return read_from_file_optional_json( FILENAMES["panel_options"], [&]( JsonIn & jsin ) {
        deserialize( jsin );
    } );
}

void panel_manager::serialize( JsonOut &json )
{
    json.start_array();
    json.start_object();

    json.member( "current_layout_id", current_layout_id );
    json.member( "layouts" );

    json.start_array();

    for( const auto &kv : layouts ) {
        json.start_object();

        json.member( "layout_id", kv.first );
        json.member( "panels" );

        json.start_array();

        for( const auto &panel : kv.second ) {
            json.start_object();

            json.member( "name", panel.get_name() );
            json.member( "toggle", panel.toggle );

            json.end_object();
        }

        json.end_array();
        json.end_object();
    }

    json.end_array();

    json.end_object();
    json.end_array();
}

void panel_manager::deserialize( JsonIn &jsin )
{
    jsin.start_array();
    JsonObject joLayouts( jsin.get_object() );

    current_layout_id = joLayouts.get_string( "current_layout_id" );
    JsonArray jaLayouts = joLayouts.get_array( "layouts" );

    while( jaLayouts.has_more() ) {
        JsonObject joLayout = jaLayouts.next_object();

        std::string layout_id = joLayout.get_string( "layout_id" );
        auto &layout = layouts.find( layout_id )->second;
        auto it = layout.begin();

        JsonArray jaPanels = joLayout.get_array( "panels" );

        while( jaPanels.has_more() ) {
            JsonObject joPanel = jaPanels.next_object();

            std::string name = joPanel.get_string( "name" );
            for( auto it2 = layout.begin() + std::distance( layout.begin(), it ); it2 != layout.end(); ++it2 ) {
                if( it2->get_name() == name ) {
                    if( it->get_name() != name ) {
                        window_panel panel = *it2;
                        layout.erase( it2 );
                        it = layout.insert( it, panel );
                    }
                    it->toggle = joPanel.get_bool( "toggle" );
                    ++it;
                    break;
                }
            }
        }
    }
    jsin.end_array();
}

void panel_manager::draw_adm( const catacurses::window &w, size_t column, size_t index )
{

    input_context ctxt( "PANEL_MGMT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "MOVE_PANEL" );
    ctxt.register_action( "TOGGLE_PANEL" );

    const int column_width = 43; // how far apart the columns are
    size_t max_index = 0;
    int counter = 0;
    bool selected = false;
    size_t source_index = 0;
    size_t target_index = 0;

    bool redraw = true;
    bool exit = false;
    // map of row the panel is on vs index
    // panels not renderable due to game configuration will not be in this map
    std::map<size_t, size_t> row_indices;
    while( !exit ) {
        auto &panels = layouts[current_layout_id];

        if( redraw ) {
            redraw = false;
            werase( w );
            static const std::string title = _( "SIDEBAR OPTIONS" );
            decorate_panel( title, w );
            // clear the panel list
            for( int i = 1; i <= 13; i++ ) {
                for( int j = 1; j <= 12; j++ ) {
                    mvwputch( w, i, j, c_black, ' ' );
                }
            }
            // the row that the panel name is printed on
            int row = 1;
            row_indices.clear();
            for( size_t i = 0; i < panels.size(); i++ ) {
                if( panels[i].render() ) {
                    row_indices.emplace( row - 1, i );
                    row++;
                } else if( !panels[i].render && column == 0 ) {
                    if( selected && index == i ) {
                        row++;
                    }
                }
            }

            max_index = column == 0 ? row_indices.size() : layouts.size();
            int vertical_offset = 0;
            int selected_offset = 0;
            size_t modified_index = row_indices[index - 1];

            for( std::pair<size_t, size_t> row_indx : row_indices ) {
                nc_color toggle_color = panels[row_indx.second].toggle ? c_white : c_dark_gray;
                std::string name = _( panels[row_indx.second].get_name() );
                if( !selected ) {
                    mvwprintz( w, row_indx.first + 1, 4, toggle_color, name );
                } else {
                    if( modified_index < row_indx.first ) {
                        vertical_offset = 2;
                    } else if( modified_index == row_indx.first && row_indx.first < source_index ) {
                        vertical_offset = 2;
                    } else {
                        vertical_offset = 1;
                    }
                    mvwprintz( w, row_indx.first + vertical_offset, 4, toggle_color, name );
                    if( source_index == row_indx.first ) {
                        if( modified_index < source_index ) {
                            selected_offset = 0;
                        } else {
                            selected_offset = 1;
                        }
                        mvwprintz( w, index + selected_offset, 5, c_yellow, name );
                    }
                }
            }
            int i = 1;
            for( const auto &layout : layouts ) {
                mvwprintz( w, i, 47, current_layout_id == layout.first ? c_light_blue : c_white,
                           _( layout.first ) );
                i++;
            }
            mvwprintz( w, index + selected_offset, 1 + ( column_width * column ), c_yellow, ">>" );
            mvwvline( w, 1, 13, 0, 13 );
            mvwvline( w, 1, 43, 0, 13 );
            mvwprintz( w, 1, 15, c_light_green, trunc_ellipse( ctxt.press_x( "TOGGLE_PANEL" ), 27 ) + ":" );
            mvwprintz( w, 2, 15, c_white, _( "Toggle panels on/off" ) );
            mvwprintz( w, 3, 15, c_light_green, trunc_ellipse( ctxt.press_x( "MOVE_PANEL" ), 27 ) + ":" );
            mvwprintz( w, 4, 15, c_white, _( "Change display order" ) );
            mvwprintz( w, 5, 15, c_light_green, trunc_ellipse( ctxt.press_x( "QUIT" ), 27 ) + ":" );
            mvwprintz( w, 6, 15, c_white, _( "Exit" ) );
        }
        wrefresh( w );

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            if( index > 1 ) {
                index -= 1;
            } else if( index == 1 ) {
                index = max_index;
            }
            redraw = true;
        } else if( action == "DOWN" ) {
            if( index < max_index ) {
                index += 1;
            } else if( index == max_index ) {
                index = 1;
            }
            redraw = true;
        } else if( action == "MOVE_PANEL" && column == 0 ) {
            counter += 1;
            // source window from the swap
            if( counter == 1 ) {
                // saving win1 index
                source_index = row_indices[index - 1];
                selected = true;
            }
            // dest window for the swap
            if( counter == 2 ) {
                // saving win2 index
                target_index = row_indices[index - 1];

                int distance = target_index - source_index;
                size_t step_dir = distance > 0 ? 1 : -1;
                for( size_t i = source_index; i != target_index; i += step_dir ) {
                    std::swap( panels[i], panels[i + step_dir] );
                }
                werase( w );
                wrefresh( g->w_terrain );
                g->draw_panels( column, index, true );
                return;
            }
            redraw = true;
        } else if( action == "MOVE_PANEL" && column == 1 ) {
            auto iter = layouts.begin();
            for( size_t i = 1; i < index; i++ ) {
                iter++;
            }
            current_layout_id = iter->first;
            int width = panel_manager::get_manager().get_current_layout().begin()->get_width();
            update_offsets( width );
            int h; // to_map_font_dimension needs a second input
            to_map_font_dimension( width, h );
            werase( w );
            wrefresh( g->w_terrain );
            g->draw_panels( column, index, true );
            // tell the game that the main screen might have a different size now.
            g->init_ui( true );
            return;
        } else if( action == "RIGHT" || action == "LEFT" ) {
            // there are only two columns
            if( column == 0 ) {
                column = 1;
                if( index > layouts.size() ) {
                    index = layouts.size();
                }
            } else {
                column = 0;
                if( index > get_current_layout().size() ) {
                    index = get_current_layout().size();
                }
            }
            redraw = true;
        }
        if( action == "TOGGLE_PANEL" && column == 0 ) {
            panels[row_indices[index - 1]].toggle = !panels[row_indices[index - 1]].toggle;
            wrefresh( g->w_terrain );
            g->draw_panels( column, index, true );
            return;
        } else if( action == "QUIT" ) {
            exit = true;
            g->show_panel_adm = false;
            save();
        }
    }
}
