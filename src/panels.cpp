#include "panels.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iosfwd>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "action.h"
#include "avatar.h"
#include "behavior.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_martial_arts.h"
#include "character_oracle.h"
#include "color.h"
#include "creature.h"
#include "cursesdef.h"
#include "debug.h"
#include "effect.h"
#include "enum_traits.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "input.h"
#include "item.h"
#include "json.h"
#include "make_static.h"
#include "magic.h"
#include "map.h"
#include "messages.h"
#include "move_mode.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "pimpl.h"
#include "player.h"
#include "point.h"
#include "string_formatter.h"
#include "tileray.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_type.h"

static const trait_id trait_NOPAIN( "NOPAIN" );
static const trait_id trait_SELFAWARE( "SELFAWARE" );
static const trait_id trait_THRESH_FELINE( "THRESH_FELINE" );
static const trait_id trait_THRESH_BIRD( "THRESH_BIRD" );
static const trait_id trait_THRESH_URSINE( "THRESH_URSINE" );

static const efftype_id effect_got_checked( "got_checked" );

static const flag_id json_flag_THERMOMETER( "THERMOMETER" );
static const flag_id json_flag_SPLINT( "SPLINT" );

// constructor
window_panel::window_panel(
    const std::function<void( avatar &, const catacurses::window & )> &draw_func,
    const std::string &id, const translation &nm, const int ht, const int wd,
    const bool default_toggle_, const std::function<bool()> &render_func,
    const bool force_draw )
    : draw( draw_func ), render( render_func ), toggle( default_toggle_ ),
      always_draw( force_draw ), height( ht ), width( wd ), id( id ), name( nm )
{
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
    mvwaddch( w, top_left, LINE_OXXO );
    mvwaddch( w, point( top_left.x, bottom_right.y ), LINE_XXOO );
    mvwaddch( w, point( bottom_right.x, top_left.y ), LINE_OOXX );
    mvwaddch( w, bottom_right, LINE_XOOX );

    for( int i = 1; i < bottom_right.x; i++ ) {
        mvwaddch( w, point( i, top_left.y ), LINE_OXOX );
        mvwaddch( w, point( i, bottom_right.y ), LINE_OXOX );
    }

    for( int i = 1; i < bottom_right.y; i++ ) {
        mvwaddch( w, point( top_left.x, i ), LINE_XOXO );
        mvwaddch( w, point( bottom_right.x, i ), LINE_XOXO );
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
    return std::make_pair( clr, _( "Str " ) + ( p.get_str() < 100 ? std::to_string(
                               p.get_str() ) : "++" ) );
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
    return std::make_pair( clr, _( "Dex " ) + ( p.get_dex() < 100 ? std::to_string(
                               p.get_dex() ) : "++" ) );
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
    return std::make_pair( clr, _( "Int " ) + ( p.get_int() < 100 ? std::to_string(
                               p.get_int() ) : "++" ) );
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
    return std::make_pair( clr, _( "Per " ) + ( p.get_per() < 100 ? std::to_string(
                               p.get_per() ) : "++" ) );
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
        if( pixel_minimap_option ) {
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

const std::string &window_panel::get_id() const
{
    return id;
}

std::string window_panel::get_name() const
{
    return name.translated();
}

panel_layout::panel_layout( const translation &_name,
                            const std::vector<window_panel> &_panels )
    : _name( _name ), _panels( _panels )
{
}

std::string panel_layout::name() const
{
    return _name.translated();
}

const std::vector<window_panel> &panel_layout::panels() const
{
    return _panels;
}

std::vector<window_panel> &panel_layout::panels()
{
    return _panels;
}

void overmap_ui::draw_overmap_chunk( const catacurses::window &w_minimap, const avatar &you,
                                     const tripoint_abs_omt &global_omt, const point &start_input,
                                     const int width, const int height )
{
    const point_abs_omt curs = global_omt.xy();
    const tripoint_abs_omt targ = you.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;
    const int start_y = start_input.y;
    const int start_x = start_input.x;
    const point mid( width / 2, height / 2 );
    map &here = get_map();

    for( int i = -( width / 2 ); i <= width - ( width / 2 ) - 1; i++ ) {
        for( int j = -( height / 2 ); j <= height - ( height / 2 ) - 1; j++ ) {
            const tripoint_abs_omt omp( curs + point( i, j ), here.get_abs_sub().z );
            nc_color ter_color;
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omp );
            const bool vehicle_here = overmap_buffer.has_vehicle( omp );
            if( overmap_buffer.has_note( omp ) ) {

                const std::string &note_text = overmap_buffer.note( omp );

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
                ter_sym = "#";
                ter_color = c_dark_gray;
            } else if( vehicle_here ) {
                ter_color = c_cyan;
                ter_sym = "c";
            } else {
                const oter_id &cur_ter = overmap_buffer.ter( omp );
                ter_sym = cur_ter->get_symbol();
                if( overmap_buffer.is_explored( omp ) ) {
                    ter_color = c_dark_gray;
                } else {
                    ter_color = cur_ter->get_color();
                }
            }
            if( !drew_mission && targ.xy() == omp.xy() ) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
                drew_mission = true;
                if( i != 0 || j != 0 ) {
                    ter_color = red_background( ter_color );
                }
            }
            if( i == 0 && j == 0 ) {
                // Highlight player character position in center of minimap
                mvwputch_hi( w_minimap, mid + point( start_x, start_y ), ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, mid + point( i + start_x, j + start_y ), ter_color,
                          ter_sym );
            }
        }
    }

    // Print arrow to mission if we have one!
    if( !drew_mission ) {
        // Use an extreme slope rather than dividing by zero
        double slope = curs.x() == targ.x() ? 1000 :
                       static_cast<double>( targ.y() - curs.y() ) / ( targ.x() - curs.x() );

        if( std::fabs( slope ) > 12 ) {
            // For any near-vertical slope, center the marker
            if( targ.y() > curs.y() ) {
                mvwputch( w_minimap, point( mid.x + start_x, height - 1 + start_y ), c_red, '*' );
            } else {
                mvwputch( w_minimap, point( mid.x + start_x, 1 + start_y ), c_red, '*' );
            }
        } else {
            point arrow( point_north_west );
            if( std::fabs( slope ) >= 1. ) {
                // If target to the north or south, arrow on top or bottom edge of minimap
                if( targ.y() > curs.y() ) {
                    arrow.x = static_cast<int>( ( 1. + ( 1. / slope ) ) * mid.x );
                    arrow.y = height - 1;
                } else {
                    arrow.x = static_cast<int>( ( 1. - ( 1. / slope ) ) * mid.x );
                    arrow.y = 0;
                }
                // Clip to left/right edges
                arrow.x = std::max( arrow.x, 0 );
                arrow.x = std::min( arrow.x, width - 1 );
            } else {
                // If target to the east or west, arrow on left or right edge of minimap
                if( targ.x() > curs.x() ) {
                    arrow.x = width - 1;
                    arrow.y = static_cast<int>( ( 1. + slope ) * mid.y );
                } else {
                    arrow.x = 0;
                    arrow.y = static_cast<int>( ( 1. - slope ) * mid.y );
                }
                // Clip to top/bottom edges
                arrow.y = std::max( arrow.y, 0 );
                arrow.y = std::min( arrow.y, height - 1 );
            }
            char glyph = '*';
            if( targ.z() > you.posz() ) {
                glyph = '^';
            } else if( targ.z() < you.posz() ) {
                glyph = 'v';
            }

            mvwputch( w_minimap, arrow + point( start_x, start_y ), c_red, glyph );
        }
    }
    avatar &player_character = get_avatar();
    const int sight_points = player_character.overmap_sight_range( g->light_level(
                                 player_character.posz() ) );
    for( int i = -3; i <= 3; i++ ) {
        for( int j = -3; j <= 3; j++ ) {
            if( i > -3 && i < 3 && j > -3 && j < 3 ) {
                continue; // only do hordes on the border, skip inner map
            }
            const tripoint_abs_omt omp( curs + point( i, j ), here.get_abs_sub().z );
            int horde_size = overmap_buffer.get_horde_size( omp );
            if( horde_size >= HORDE_VISIBILITY_SIZE ) {
                if( overmap_buffer.seen( omp )
                    && player_character.overmap_los( omp, sight_points ) ) {
                    mvwputch( w_minimap, mid + point( i, j ), c_green,
                              horde_size > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
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
    mvwprintz( w, point( start_pos, 0 ), c_white, title_prefix );
    wprintz( w, c_light_red, title );
    wprintz( w, c_white, title_suffix );
}

static std::string get_temp( const avatar &u )
{
    std::string temp;
    if( u.has_item_with_flag( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        temp = print_temperature( get_weather().get_temperature( u.pos() ) );
    }
    if( temp.empty() ) {
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

static std::pair<bodypart_id, bodypart_id> temp_delta( const avatar &u )
{
    bodypart_id current_bp_extreme( "torso" );
    bodypart_id conv_bp_extreme( "torso" );
    for( const bodypart_id &bp : u.get_all_body_parts() ) {
        if( std::abs( u.get_part_temp_cur( bp ) - BODYTEMP_NORM ) >
            std::abs( u.get_part_temp_cur( current_bp_extreme ) - BODYTEMP_NORM ) ) {
            current_bp_extreme = bp;
        }
        if( std::abs( u.get_part_temp_conv( bp ) - BODYTEMP_NORM ) >
            std::abs( u.get_part_temp_conv( conv_bp_extreme ) - BODYTEMP_NORM ) ) {
            conv_bp_extreme = bp;
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
    std::pair<bodypart_id, bodypart_id> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.get_part_temp_cur( temp_pair.first ) );
    const int conv_zone = define_temp_level( u.get_part_temp_conv( temp_pair.second ) );

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
    std::pair<bodypart_id, bodypart_id> temp_pair = temp_delta( u );
    // Assign zones for comparisons
    const int cur_zone = define_temp_level( u.get_part_temp_cur( temp_pair.first ) );
    const int conv_zone = define_temp_level( u.get_part_temp_conv( temp_pair.second ) );

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
    const bodypart_id &current_bp_extreme = temp_delta( u ).first;

    // printCur the hottest/coldest bodypart
    std::string temp_string;
    nc_color temp_color = c_yellow;
    if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_SCORCHING ) {
        temp_color = c_red;
        temp_string = _( "Scorching!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_VERY_HOT ) {
        temp_color = c_light_red;
        temp_string = _( "Very hot!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_HOT ) {
        temp_color = c_yellow;
        temp_string = _( "Warm" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_COLD ) {
        temp_color = c_green;
        temp_string = _( "Comfortable" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_VERY_COLD ) {
        temp_color = c_light_blue;
        temp_string = _( "Chilly" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) > BODYTEMP_FREEZING ) {
        temp_color = c_cyan;
        temp_string = _( "Very cold!" );
    } else if( u.get_part_temp_cur( current_bp_extreme ) <= BODYTEMP_FREEZING ) {
        temp_color = c_blue;
        temp_string = _( "Freezing!" );
    }
    return std::make_pair( temp_color, temp_string );
}

static std::string get_armor( const avatar &u, bodypart_id bp, unsigned int truncate = 0 )
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
                return "@^@";
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
    if( !u.has_max_power() ) {
        s_pwr = "--";
        c_pwr = c_light_gray;
    } else {
        if( u.get_power_level() >= u.get_max_power_level() / 2 ) {
            c_pwr = c_light_blue;
        } else if( u.get_power_level() >= u.get_max_power_level() / 3 ) {
            c_pwr = c_yellow;
        } else if( u.get_power_level() >= u.get_max_power_level() / 4 ) {
            c_pwr = c_red;
        }

        if( u.get_power_level() < 1_J ) {
            s_pwr = std::to_string( units::to_millijoule( u.get_power_level() ) ) +
                    pgettext( "energy unit: millijoule", "mJ" );
        } else if( u.get_power_level() < 1_kJ ) {
            s_pwr = std::to_string( units::to_joule( u.get_power_level() ) ) +
                    pgettext( "energy unit: joule", "J" );
        } else {
            s_pwr = std::to_string( units::to_kilojoule( u.get_power_level() ) ) +
                    pgettext( "energy unit: kilojoule", "kJ" );
        }
    }
    return std::make_pair( c_pwr, s_pwr );
}

static std::pair<nc_color, std::string> mana_stat( const player &u )
{
    nc_color c_mana = c_red;
    std::string s_mana;
    if( u.magic->max_mana( u ) <= 0 ) {
        s_mana = "--";
        c_mana = c_light_gray;
    } else {
        if( u.magic->available_mana() >= u.magic->max_mana( u ) / 2 ) {
            c_mana = c_light_blue;
        } else if( u.magic->available_mana() >= u.magic->max_mana( u ) / 3 ) {
            c_mana = c_yellow;
        }
        s_mana = std::to_string( u.magic->available_mana() );
    }
    return std::make_pair( c_mana, s_mana );
}

static nc_color safe_color()
{
    nc_color s_color = g->safe_mode ? c_green : c_red;
    if( g->safe_mode == SAFE_MODE_OFF && get_option<bool>( "AUTOSAFEMODE" ) ) {
        time_duration s_return = time_duration::from_turns( get_option<int>( "AUTOSAFEMODETURNS" ) );
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

// ===============================
// panels code
// ===============================

static void draw_limb_health( avatar &u, const catacurses::window &w, bodypart_id bp )
{
    const bool no_feeling = u.has_trait( trait_NOPAIN );
    const bool is_self_aware = u.has_trait( trait_SELFAWARE ) && !no_feeling;
    static auto print_symbol_num = []( const catacurses::window & w, int num, const std::string & sym,
    const nc_color & color ) {
        while( num-- > 0 ) {
            wprintz( w, color, sym );
        }
    };

    if( u.is_limb_broken( bp ) && bp->is_limb ) {
        //Limb is broken
        std::string limb = "~~%~~";
        nc_color color = c_light_red;

        if( u.worn_with_flag( json_flag_SPLINT,  bp ) ) {
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

    const int hp_cur = u.get_part_hp_cur( bp );
    const int hp_max = u.get_part_hp_max( bp );
    std::pair<std::string, nc_color> hp = get_hp_bar( hp_cur, hp_max );

    if( is_self_aware || u.has_effect( effect_got_checked ) ) {
        wprintz( w, hp.second, "%3d  ", hp_cur );
    } else if( no_feeling ) {
        if( hp_cur < hp_max / 2 ) {
            hp = std::make_pair( string_format( " %s", _( "Bad" ) ), c_red );
        } else {
            hp = std::make_pair( string_format( " %s", _( "Good" ) ), c_green );
        }
        wprintz( w, hp.second, hp.first );
    } else {
        wprintz( w, hp.second, hp.first );

        //Add the trailing symbols for a not-quite-full health bar
        print_symbol_num( w, 5 - utf8_width( hp.first ), ".", c_white );
    }
}

static void draw_limb2( avatar &u, const catacurses::window &w )
{
    werase( w );
    // print bodypart health
    int i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        const std::string str = body_part_hp_bar_ui_text( bp );
        if( i % 2 == 0 ) {
            wmove( w, point( 0, i / 2 ) );
        } else {
            wmove( w, point( 11, i / 2 ) );
        }
        wprintz( w, u.limb_color( bp, true, true, true ), str );
        if( i % 2 == 0 ) {
            wmove( w, point( 5, i / 2 ) );
        } else {
            wmove( w, point( 16, i / 2 ) );
        }
        draw_limb_health( u, w, bp );
        i++;
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
    mvwprintz( w, point( 22, 2 ), safe_color(), safe_str );
    mvwprintz( w, point( 27, 2 ), morale_pair.first, smiley );

    // print stamina
    const auto &stamina = get_hp_bar( u.get_stamina(), u.get_stamina_max() );
    mvwprintz( w, point( 22, 0 ), c_light_gray, _( "STM" ) );
    mvwprintz( w, point( 26, 0 ), stamina.second, stamina.first );

    mvwprintz( w, point( 22, 1 ), c_light_gray, _( "PWR" ) );
    const auto pwr = power_stat( u );
    mvwprintz( w, point( 31 - utf8_width( pwr.second ), 1 ), pwr.first, pwr.second );

    wnoutrefresh( w );
}

static std::pair<translation, nc_color> weariness_description( size_t weariness )
{
    static const std::array<std::pair<translation, nc_color>, 6> weary_descriptions { {
            {to_translation( "weariness description", "Fresh" ), c_green},
            {to_translation( "weariness description", "Light" ), c_yellow },
            {to_translation( "weariness description", "Moderate" ), c_light_red},
            {to_translation( "weariness description", "Weary" ), c_light_red },
            {to_translation( "weariness description", "Very" ), c_red },
            {to_translation( "weariness description", "Extreme" ), c_red }
        } };
    // We can have more than level 5, but we don't really care about it
    if( weariness >= weary_descriptions.size() ) {
        weariness = 5;
    }
    return weary_descriptions[weariness];
}

std::string activity_level::activity_level_str( float level )
{
    static const std::array<translation, 6> activity_descriptions { {
            to_translation( "activity description", "None" ),
            to_translation( "activity description", "Light" ),
            to_translation( "activity description", "Moderate" ),
            to_translation( "activity description", "Brisk" ),
            to_translation( "activity description", "Active" ),
            to_translation( "activity description", "Extreme" ),
        } };
    // Activity levels are 1, 2, 4, 6, 8, 10
    // So we can easily cut them in half and round down for an index
    int idx = std::floor( level / 2 );

    return activity_descriptions[idx].translated();
}

static void draw_stats( avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color stat_clr = str_string( u ).first;
    mvwprintz( w, point_zero, c_light_gray, _( "STR" ) );
    int stat = u.get_str();
    mvwprintz( w, point( stat < 10 ? 5 : 4, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = dex_string( u ).first;
    stat = u.get_dex();
    mvwprintz( w, point( 9, 0 ), c_light_gray, _( "DEX" ) );
    mvwprintz( w, point( stat < 10 ? 14 : 13, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = int_string( u ).first;
    stat = u.get_int();
    mvwprintz( w, point( 17, 0 ), c_light_gray, _( "INT" ) );
    mvwprintz( w, point( stat < 10 ? 22 : 21, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = per_string( u ).first;
    stat = u.get_per();
    mvwprintz( w, point( 25, 0 ), c_light_gray, _( "PER" ) );
    mvwprintz( w, point( stat < 10 ? 30 : 29, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    if( u.exertion_adjusted_move_multiplier( activity ) < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    static const std::string weary_label = translate_marker( "WRY" );
    static const std::string activity_label = translate_marker( "ACT" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point_south, c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 1, 1 ), weary.second, weary.first.translated() );
    mvwprintz( w, point( act_start, 1 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 1 ), act_color,
               activity_level::activity_level_str( activity ) );

    wnoutrefresh( w );
}

static nc_color move_mode_color( avatar &u )
{
    return u.current_movement_mode()->panel_color();
}

static char move_mode_string( avatar &u )
{
    return u.current_movement_mode()->panel_letter();
}

static void draw_stealth( avatar &u, const catacurses::window &w )
{
    werase( w );
    mvwprintz( w, point_zero, c_light_gray, _( "Speed" ) );
    mvwprintz( w, point( 7, 0 ), value_color( u.get_speed() ), "%s", u.get_speed() );
    nc_color move_color = move_mode_color( u );
    std::string move_string = std::to_string( u.movecounter ) + move_mode_string( u );
    mvwprintz( w, point( 15 - utf8_width( move_string ), 0 ), move_color, move_string );
    if( u.is_deaf() ) {
        mvwprintz( w, point( 22, 0 ), c_red, _( "DEAF" ) );
    } else {
        mvwprintz( w, point( 20, 0 ), c_light_gray, _( "Sound:" ) );
        const std::string snd = std::to_string( u.volume );
        mvwprintz( w, point( 30 - utf8_width( snd ), 0 ), u.volume != 0 ? c_yellow : c_light_gray, snd );
    }

    wnoutrefresh( w );
}

static void draw_time_graphic( const catacurses::window &w )
{
    std::vector<std::pair<char, nc_color> > vGlyphs;
    vGlyphs.emplace_back( '_', c_red );
    vGlyphs.emplace_back( '_', c_cyan );
    vGlyphs.emplace_back( '.', c_brown );
    vGlyphs.emplace_back( ',', c_blue );
    vGlyphs.emplace_back( '+', c_yellow );
    vGlyphs.emplace_back( 'c', c_light_blue );
    vGlyphs.emplace_back( '*', c_yellow );
    vGlyphs.emplace_back( 'C', c_white );
    vGlyphs.emplace_back( '+', c_yellow );
    vGlyphs.emplace_back( 'c', c_light_blue );
    vGlyphs.emplace_back( '.', c_brown );
    vGlyphs.emplace_back( ',', c_blue );
    vGlyphs.emplace_back( '_', c_red );
    vGlyphs.emplace_back( '_', c_cyan );

    const int iHour = hour_of_day<int>( calendar::turn );
    wprintz( w, c_white, "[" );
    bool bAddTrail = false;

    for( int i = 0; i < 14; i += 2 ) {
        if( iHour >= 8 + i && iHour <= 13 + ( i / 2 ) ) { // NOLINT(bugprone-branch-clone)
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
    mvwprintz( w, point_zero, c_light_gray, calendar::name_season( season_of_year( calendar::turn ) ) );
    std::string day = std::to_string( day_of_season<int>( calendar::turn ) + 1 );
    mvwprintz( w, point( 10 - utf8_width( day ), 0 ), c_light_gray, day );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, point( 11, 0 ), c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( get_map().get_abs_sub().z >= 0 ) {
        wmove( w, point( 11, 0 ) );
        draw_time_graphic( w );
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        mvwprintz( w, point( 11, 0 ), c_light_gray, _( "Time: ???" ) );
    }
    //display moon
    mvwprintz( w, point( 22, 0 ), c_white, _( "Moon" ) );
    nc_color clr = c_white;
    print_colored_text( w, point( 27, 0 ), clr, c_white, get_moon_graphic() );

    wnoutrefresh( w );
}

static void draw_needs_compact( const avatar &u, const catacurses::window &w )
{
    werase( w );

    auto hunger_pair = u.get_hunger_description();
    mvwprintz( w, point_zero, hunger_pair.second, hunger_pair.first );
    hunger_pair = u.get_fatigue_description();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 1 ), hunger_pair.second, hunger_pair.first );
    auto pain_pair = u.get_pain_description();
    mvwprintz( w, point( 0, 2 ), pain_pair.second, pain_pair.first );

    hunger_pair = u.get_thirst_description();
    mvwprintz( w, point( 17, 0 ), hunger_pair.second, hunger_pair.first );
    auto pair = temp_stat( u );
    mvwprintz( w, point( 17, 1 ), pair.first, pair.second );
    const auto arrow = temp_delta_arrows( u );
    mvwprintz( w, point( 17 + utf8_width( pair.second ), 1 ), arrow.first, arrow.second );

    mvwprintz( w, point( 17, 2 ), c_light_gray, _( "Focus" ) );
    mvwprintz( w, point( 24, 2 ), focus_color( u.get_focus() ), std::to_string( u.get_focus() ) );

    wnoutrefresh( w );
}

static void draw_limb_narrow( avatar &u, const catacurses::window &w )
{
    werase( w );
    int ny2 = 0;
    int i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        int ny;
        int nx;
        if( i < 3 ) {
            ny = i;
            nx = 8;
        } else {
            ny = ny2++;
            nx = 26;
        }
        wmove( w, point( nx, ny ) );
        draw_limb_health( u, w, bp );
        i++;
    }

    // display limbs status
    ny2 = 0;
    i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        int ny;
        int nx;
        if( i < 3 ) {
            ny = i;
            nx = 1;
        } else {
            ny = ny2++;
            nx = 19;
        }

        std::string str = body_part_hp_bar_ui_text( bp );
        wmove( w, point( nx, ny ) );
        str = left_justify( str, 5 );
        wprintz( w, u.limb_color( bp, true, true, true ), str + ":" );
        i++;
    }
    wnoutrefresh( w );
}

static void draw_limb_wide( avatar &u, const catacurses::window &w )
{
    werase( w );
    int i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        int offset = i * 15;
        int ny = offset / 45;
        int nx = offset % 45;
        std::string str = string_format( " %s: ",
                                         left_justify( body_part_hp_bar_ui_text( bp ), 5 ) );
        nc_color part_color = u.limb_color( bp, true, true, true );
        print_colored_text( w, point( nx, ny ), part_color, c_white, str );
        draw_limb_health( u, w, bp );
        i++;
    }
    wnoutrefresh( w );
}

static void draw_char_narrow( avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Stam :" ) );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Focus:" ) );
    mvwprintz( w, point( 19, 0 ), c_light_gray, _( "Mood :" ) );
    mvwprintz( w, point( 19, 1 ), c_light_gray, _( "Speed:" ) );
    mvwprintz( w, point( 19, 2 ), c_light_gray, _( "Move :" ) );

    nc_color move_color =  move_mode_color( u );
    char move_char = move_mode_string( u );
    std::string movecost = std::to_string( u.movecounter ) + "(" + move_char + ")";
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );
    mvwprintz( w, point( 8, 0 ), c_light_gray, "%s", u.volume );

    // print stamina
    auto needs_pair = std::make_pair( get_hp_bar( u.get_stamina(), u.get_stamina_max() ).second,
                                      get_hp_bar( u.get_stamina(), u.get_stamina_max() ).first );
    mvwprintz( w, point( 8, 1 ), needs_pair.first, needs_pair.second );
    const int width = utf8_width( needs_pair.second );
    for( int i = 0; i < 5 - width; i++ ) {
        mvwprintz( w, point( 12 - i, 1 ), c_white, "." );
    }

    mvwprintz( w, point( 8, 2 ), focus_color( u.get_focus() ), "%s", u.get_focus() );
    if( u.get_focus() < u.calc_focus_equilibrium() ) {
        mvwprintz( w, point( 11, 2 ), c_light_green, "↥" );
    } else if( u.get_focus() > u.calc_focus_equilibrium() ) {
        mvwprintz( w, point( 11, 2 ), c_light_red, "↧" );
    }
    mvwprintz( w, point( 26, 0 ), morale_pair.first, "%s", smiley );
    mvwprintz( w, point( 26, 1 ), focus_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w, point( 26, 2 ), move_color, "%s", movecost );
    wnoutrefresh( w );
}

static void draw_char_wide( avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    mvwprintz( w, point( 16, 0 ), c_light_gray, _( "Mood :" ) );
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Focus:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Stam :" ) );
    mvwprintz( w, point( 16, 1 ), c_light_gray, _( "Speed:" ) );
    mvwprintz( w, point( 31, 1 ), c_light_gray, _( "Move :" ) );

    nc_color move_color =  move_mode_color( u );
    char move_char = move_mode_string( u );
    std::string movecost = std::to_string( u.movecounter ) + "(" + move_char + ")";
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );

    mvwprintz( w, point( 8, 0 ), c_light_gray, "%s", u.volume );
    mvwprintz( w, point( 23, 0 ), morale_pair.first, "%s", smiley );
    mvwprintz( w, point( 38, 0 ), focus_color( u.get_focus() ), "%s", u.get_focus() );

    // print stamina
    auto needs_pair = std::make_pair( get_hp_bar( u.get_stamina(), u.get_stamina_max() ).second,
                                      get_hp_bar( u.get_stamina(), u.get_stamina_max() ).first );
    mvwprintz( w, point( 8, 1 ), needs_pair.first, needs_pair.second );
    const int width = utf8_width( needs_pair.second );
    for( int i = 0; i < 5 - width; i++ ) {
        mvwprintz( w, point( 12 - i, 1 ), c_white, "." );
    }

    mvwprintz( w, point( 23, 1 ), focus_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w, point( 38, 1 ), move_color, "%s", movecost );
    wnoutrefresh( w );
}

static void draw_stat_narrow( avatar &u, const catacurses::window &w )
{
    werase( w );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Str  :" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Int  :" ) );
    mvwprintz( w, point( 19, 0 ), c_light_gray, _( "Dex  :" ) );
    mvwprintz( w, point( 19, 1 ), c_light_gray, _( "Per  :" ) );

    nc_color stat_clr = str_string( u ).first;
    mvwprintz( w, point( 8, 0 ), stat_clr, "%s", u.get_str() );
    stat_clr = int_string( u ).first;
    mvwprintz( w, point( 8, 1 ), stat_clr, "%s", u.get_int() );
    stat_clr = dex_string( u ).first;
    mvwprintz( w, point( 26, 0 ), stat_clr, "%s", u.get_dex() );
    stat_clr = per_string( u ).first;
    mvwprintz( w, point( 26, 1 ), stat_clr, "%s", u.get_per() );

    std::pair<nc_color, std::string> pwr_pair = power_stat( u );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Power:" ) );
    mvwprintz( w, point( 19, 2 ), c_light_gray, _( "Safe :" ) );
    mvwprintz( w, point( 8, 2 ), pwr_pair.first, "%s", pwr_pair.second );
    mvwprintz( w, point( 26, 2 ), safe_color(), g->safe_mode ? _( "On" ) : _( "Off" ) );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    if( u.exertion_adjusted_move_multiplier( activity ) < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    static const std::string weary_label = translate_marker( "Weary:" );
    static const std::string activity_label = translate_marker( "Activ:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point( 1, 3 ), c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 2, 3 ), weary.second, weary.first.translated() );
    mvwprintz( w, point( act_start, 3 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 3 ), act_color,
               activity_level::activity_level_str( activity ) );

    wnoutrefresh( w );
}

static void draw_stat_wide( avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, point_east, c_light_gray, _( "Str  :" ) );
    mvwprintz( w, point_south_east, c_light_gray, _( "Int  :" ) );
    mvwprintz( w, point( 16, 0 ), c_light_gray, _( "Dex  :" ) );
    mvwprintz( w, point( 16, 1 ), c_light_gray, _( "Per  :" ) );

    nc_color stat_clr = str_string( u ).first;
    mvwprintz( w, point( 8, 0 ), stat_clr, "%s", u.get_str() );
    stat_clr = int_string( u ).first;
    mvwprintz( w, point( 8, 1 ), stat_clr, "%s", u.get_int() );
    stat_clr = dex_string( u ).first;
    mvwprintz( w, point( 23, 0 ), stat_clr, "%s", u.get_dex() );
    stat_clr = per_string( u ).first;
    mvwprintz( w, point( 23, 1 ), stat_clr, "%s", u.get_per() );

    std::pair<nc_color, std::string> pwr_pair = power_stat( u );
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Power:" ) );
    mvwprintz( w, point( 31, 1 ), c_light_gray, _( "Safe :" ) );
    mvwprintz( w, point( 38, 0 ), pwr_pair.first, "%s", pwr_pair.second );
    mvwprintz( w, point( 38, 1 ), safe_color(), g->safe_mode ? _( "On" ) : _( "Off" ) );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    if( u.exertion_adjusted_move_multiplier( activity ) < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    static const std::string weary_label = translate_marker( "Weary:" );
    static const std::string activity_label = translate_marker( "Activ:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point( 1, 2 ), c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 2, 2 ), weary.second, weary.first.translated() );
    mvwprintz( w, point( act_start, 2 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 2 ), act_color,
               activity_level::activity_level_str( activity ) );

    wnoutrefresh( w );
}

static void draw_loc_labels( const avatar &u, const catacurses::window &w, bool minimap )
{
    werase( w );
    // display location
    const oter_id &cur_ter = overmap_buffer.ter( u.global_omt_location() );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Place: " ) );
    wprintz( w, c_white, utf8_truncate( cur_ter->get_name(), getmaxx( w ) - 13 ) );
    map &here = get_map();
    // display weather
    if( here.get_abs_sub().z < 0 ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Sky  : Underground" ) );
    } else {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Sky  :" ) );
        wprintz( w, get_weather().weather_id->color, " %s", get_weather().weather_id->name.translated() );
    }
    // display lighting
    const std::pair<std::string, nc_color> ll = get_light_level(
                get_avatar().fine_detail_vision_mod() );
    mvwprintz( w, point( 1, 2 ), c_light_gray, "%s ", _( "Light:" ) );
    wprintz( w, ll.second, ll.first );

    // display date
    mvwprintz( w, point( 1, 3 ), c_light_gray, _( "Date : %s, day %d" ),
               calendar::name_season( season_of_year( calendar::turn ) ),
               day_of_season<int>( calendar::turn ) + 1 );

    // display time
    if( u.has_watch() ) {
        mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Time : %s" ),
                   to_string_time_of_day( calendar::turn ) );
    } else if( here.get_abs_sub().z >= 0 ) {
        mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Time : %s" ), time_approx() );
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Time : ???" ) );
    }
    if( minimap ) {
        const int offset = getmaxx( w ) - 14;
        const tripoint_abs_omt curs = u.global_omt_location();
        overmap_ui::draw_overmap_chunk( w, u, curs, point( offset, 0 ), 13, 5 );
    }
    wnoutrefresh( w );
}

static void draw_loc_narrow( const avatar &u, const catacurses::window &w )
{
    draw_loc_labels( u, w, false );
}

static void draw_loc_wide( const avatar &u, const catacurses::window &w )
{
    draw_loc_labels( u, w, false );
}

static void draw_loc_wide_map( const avatar &u, const catacurses::window &w )
{
    draw_loc_labels( u, w, true );
}

static void draw_moon_narrow( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Moon : %s" ), get_moon() );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Temp : %s" ), get_temp( u ) );
    wnoutrefresh( w );
}

static void draw_moon_wide( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Moon : %s" ), get_moon() );
    mvwprintz( w, point( 23, 0 ), c_light_gray, _( "Temp : %s" ), get_temp( u ) );
    wnoutrefresh( w );
}

static void draw_weapon_labels( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Wield:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Style:" ) );
    trim_and_print( w, point( 8, 0 ), getmaxx( w ) - 8, c_light_gray, u.weapname() );
    mvwprintz( w, point( 8, 1 ), c_light_gray, "%s", u.martial_arts_data->selected_style_name( u ) );
    wnoutrefresh( w );
}

static void draw_needs_narrow( const avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<std::string, nc_color> hunger_pair = u.get_hunger_description();
    std::pair<std::string, nc_color> thirst_pair = u.get_thirst_description();
    std::pair<std::string, nc_color> rest_pair = u.get_fatigue_description();
    std::pair<nc_color, std::string> temp_pair = temp_stat( u );
    std::pair<std::string, nc_color> pain_pair = u.get_pain_description();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Hunger:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Thirst:" ) );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Rest :" ) );
    mvwprintz( w, point( 1, 3 ), c_light_gray, _( "Pain :" ) );
    mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Heat :" ) );
    mvwprintz( w, point( 8, 0 ), hunger_pair.second, hunger_pair.first );
    mvwprintz( w, point( 8, 1 ), thirst_pair.second, thirst_pair.first );
    mvwprintz( w, point( 8, 2 ), rest_pair.second, rest_pair.first );
    mvwprintz( w, point( 8, 3 ), pain_pair.second, pain_pair.first );
    mvwprintz( w, point( 8, 4 ), temp_pair.first, temp_pair.second );
    wnoutrefresh( w );
}

static void draw_needs_labels( const avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<std::string, nc_color> hunger_pair = u.get_hunger_description();
    std::pair<std::string, nc_color> thirst_pair = u.get_thirst_description();
    std::pair<std::string, nc_color> rest_pair = u.get_fatigue_description();
    std::pair<std::string, nc_color> weight_pair = u.get_weight_description();
    std::pair<nc_color, std::string> temp_pair = temp_stat( u );
    std::pair<std::string, nc_color> pain_pair = u.get_pain_description();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Pain :" ) );
    mvwprintz( w, point( 8, 0 ), pain_pair.second, pain_pair.first );
    mvwprintz( w, point( 23, 0 ), c_light_gray, _( "Thirst:" ) );
    mvwprintz( w, point( 30, 0 ), thirst_pair.second, thirst_pair.first );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Rest :" ) );
    mvwprintz( w, point( 8, 1 ), rest_pair.second, rest_pair.first );
    mvwprintz( w, point( 23, 1 ), c_light_gray, _( "Hunger:" ) );
    mvwprintz( w, point( 30, 1 ), hunger_pair.second, hunger_pair.first );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Heat :" ) );
    mvwprintz( w, point( 8, 2 ), temp_pair.first, temp_pair.second );
    mvwprintz( w, point( 23, 2 ), c_light_gray, _( "Weight:" ) );
    mvwprintz( w, point( 30, 2 ), weight_pair.second, weight_pair.first );
    wnoutrefresh( w );
}

static void draw_needs_labels_alt( const avatar &u, const catacurses::window &w )
{
    werase( w );
    std::pair<std::string, nc_color> hunger_pair = u.get_hunger_description();
    std::pair<std::string, nc_color> thirst_pair = u.get_thirst_description();
    std::pair<std::string, nc_color> rest_pair = u.get_fatigue_description();
    std::pair<nc_color, std::string> temp_pair = temp_stat( u );
    std::pair<std::string, nc_color> pain_pair = u.get_pain_description();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Pain :" ) );
    mvwprintz( w, point( 8, 0 ), pain_pair.second, pain_pair.first );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Drink:" ) );
    mvwprintz( w, point( 8, 1 ), thirst_pair.second, thirst_pair.first );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Food :" ) );
    mvwprintz( w, point( 8, 2 ), hunger_pair.second, hunger_pair.first );
    mvwprintz( w, point( 1, 3 ), c_light_gray, _( "Rest :" ) );
    mvwprintz( w, point( 8, 3 ), rest_pair.second, rest_pair.first );
    mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Heat :" ) );
    mvwprintz( w, point( 8, 4 ), temp_pair.first, temp_pair.second );
    wnoutrefresh( w );
}

static void draw_sound_labels( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    if( !u.is_deaf() ) {
        mvwprintz( w, point( 8, 0 ), c_yellow, std::to_string( u.volume ) );
    } else {
        mvwprintz( w, point( 8, 0 ), c_red, _( "Deaf!" ) );
    }
    wnoutrefresh( w );
}

static void draw_sound_narrow( const avatar &u, const catacurses::window &w )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    if( !u.is_deaf() ) {
        mvwprintz( w, point( 8, 0 ), c_yellow, std::to_string( u.volume ) );
    } else {
        mvwprintz( w, point( 8, 0 ), c_red, _( "Deaf!" ) );
    }
    wnoutrefresh( w );
}

static void draw_env_compact( avatar &u, const catacurses::window &w )
{
    werase( w );

    // Minimap to the left of text labels
    const int text_left = 12;
    const tripoint_abs_omt curs = u.global_omt_location();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    overmap_ui::draw_overmap_chunk( w, u, curs, point( 1, 1 ), text_left - 3, 5 );

    // wielded item
    trim_and_print( w, point( text_left, 0 ), getmaxx( w ) - text_left, c_light_gray, u.weapname() );
    // style
    mvwprintz( w, point( text_left, 1 ), c_light_gray, "%s",
               u.martial_arts_data->selected_style_name( u ) );
    // location
    mvwprintz( w, point( text_left, 2 ), c_white, utf8_truncate( overmap_buffer.ter(
                   u.global_omt_location() )->get_name(), getmaxx( w ) - 8 ) );
    // weather
    if( get_map().get_abs_sub().z < 0 ) {
        mvwprintz( w, point( text_left, 3 ), c_light_gray, _( "Underground" ) );
    } else {
        mvwprintz( w, point( text_left, 3 ), get_weather().weather_id->color,
                   get_weather().weather_id->name.translated() );
    }
    // display lighting
    const std::pair<std::string, nc_color> ll = get_light_level(
                get_avatar().fine_detail_vision_mod() );
    mvwprintz( w, point( text_left, 4 ), ll.second, ll.first );
    // wind
    const oter_id &cur_om_ter = overmap_buffer.ter( u.global_omt_location() );
    double windpower = get_local_windpower( g->weather.windspeed, cur_om_ter,
                                            u.pos(), g->weather.winddirection, g->is_sheltered( u.pos() ) );
    mvwprintz( w, point( text_left, 5 ), get_wind_color( windpower ),
               get_wind_desc( windpower ) + " " + get_wind_arrow( g->weather.winddirection ) );

    if( u.has_item_with_flag( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        std::string temp = print_temperature( g->weather.get_temperature( u.pos() ) );
        mvwprintz( w, point( 31 - utf8_width( temp ), 5 ), c_light_gray, temp );
    }

    wnoutrefresh( w );
}

static void render_wind( avatar &u, const catacurses::window &w, const std::string &formatstr )
{
    werase( w );
    mvwprintz( w, point_zero, c_light_gray,
               //~ translation should not exceed 5 console cells
               string_format( formatstr, left_justify( _( "Wind" ), 5 ) ) );
    const oter_id &cur_om_ter = overmap_buffer.ter( u.global_omt_location() );
    double windpower = get_local_windpower( g->weather.windspeed, cur_om_ter,
                                            u.pos(), g->weather.winddirection, g->is_sheltered( u.pos() ) );
    mvwprintz( w, point( 8, 0 ), get_wind_color( windpower ),
               get_wind_desc( windpower ) + " " + get_wind_arrow( g->weather.winddirection ) );
    wnoutrefresh( w );
}

static void draw_wind( avatar &u, const catacurses::window &w )
{
    render_wind( u, w, "%s: " );
}

static void draw_wind_padding( avatar &u, const catacurses::window &w )
{
    render_wind( u, w, " %s: " );
}

static void draw_health_classic( avatar &u, const catacurses::window &w )
{
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( get_map().veh_at( u.pos() ) );
    }

    werase( w );

    // 7x7 minimap
    const tripoint_abs_omt curs = u.global_omt_location();
    overmap_ui::draw_overmap_chunk( w, u, curs, point_zero, 7, 7 );

    // print limb health
    int i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        const std::string str = body_part_hp_bar_ui_text( bp );
        wmove( w, point( 8, i ) );
        wprintz( w, u.limb_color( bp, true, true, true ), str );
        wmove( w, point( 14, i ) );
        draw_limb_health( u, w, bp );
        i++;
    }

    // needs
    auto needs_pair = u.get_hunger_description();
    mvwprintz( w, point( 21, 1 ), needs_pair.second, needs_pair.first );
    needs_pair = u.get_thirst_description();
    mvwprintz( w, point( 21, 2 ), needs_pair.second, needs_pair.first );
    mvwprintz( w, point( 21, 4 ), c_white, _( "Focus" ) );
    mvwprintz( w, point( 27, 4 ), c_white, std::to_string( u.get_focus() ) );
    needs_pair = u.get_fatigue_description();
    mvwprintz( w, point( 21, 3 ), needs_pair.second, needs_pair.first );
    auto pain_pair = u.get_pain_description();
    mvwprintz( w, point( 21, 0 ), pain_pair.second, pain_pair.first );

    // print mood
    std::pair<nc_color, int> morale_pair = morale_stat( u );
    bool m_style = get_option<std::string>( "MORALE_STYLE" ) == "horizontal";
    std::string smiley = morale_emotion( morale_pair.second, get_face_type( u ), m_style );
    mvwprintz( w, point( 34, 1 ), morale_pair.first, smiley );

    if( !veh ) {
        // stats
        auto pair = str_string( u );
        mvwprintz( w, point( 38, 0 ), pair.first, pair.second );
        pair = dex_string( u );
        mvwprintz( w, point( 38, 1 ), pair.first, pair.second );
        pair = int_string( u );
        mvwprintz( w, point( 38, 2 ), pair.first, pair.second );
        pair = per_string( u );
        mvwprintz( w, point( 38, 3 ), pair.first, pair.second );
    }

    // print safe mode// print safe mode
    std::string safe_str;
    if( g->safe_mode || get_option<bool>( "AUTOSAFEMODE" ) ) {
        safe_str = "SAFE";
    }
    mvwprintz( w, point( 40, 4 ), safe_color(), safe_str );

    // print stamina
    auto pair = std::make_pair( get_hp_bar( u.get_stamina(), u.get_stamina_max() ).second,
                                get_hp_bar( u.get_stamina(), u.get_stamina_max() ).first );
    mvwprintz( w, point( 35, 5 ), c_light_gray, _( "Stm" ) );
    mvwprintz( w, point( 39, 5 ), pair.first, pair.second );
    const int width = utf8_width( pair.second );
    for( int i = 0; i < 5 - width; i++ ) {
        mvwprintz( w, point( 43 - i, 5 ), c_white, "." );
    }

    // speed
    if( !veh ) {
        mvwprintz( w, point( 21, 5 ), u.get_speed() < 100 ? c_red : c_white,
                   _( "Spd " ) + std::to_string( u.get_speed() ) );
        nc_color move_color = move_mode_color( u );
        std::string move_string = std::to_string( u.movecounter ) + " " + move_mode_string( u );
        mvwprintz( w, point( 29, 5 ), move_color, move_string );
    }

    // temperature
    pair = temp_stat( u );
    mvwprintz( w, point( 21, 6 ), pair.first, pair.second + temp_delta_string( u ) );

    // power
    pair = power_stat( u );
    mvwprintz( w, point( 8, 6 ), c_light_gray, _( "POWER" ) );
    mvwprintz( w, point( 14, 6 ), pair.first, pair.second );

    // vehicle display
    if( veh ) {
        veh->print_fuel_indicators( w, point( 39, 2 ) );
        mvwprintz( w, point( 35, 4 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        double speed = convert_velocity( veh->velocity, VU_VEHICLE );
        veh->print_speed_gauge( w, point( 21, 5 ), ( -10 < speed && speed < 100 ) ? 1 : 0 );
    }

    wnoutrefresh( w );
}

static void draw_armor_padding( const avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color color = c_light_gray;
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), color, _( "Head :" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), color, _( "Torso:" ) );
    mvwprintz( w, point( 1, 2 ), color, _( "Arms :" ) );
    mvwprintz( w, point( 1, 3 ), color, _( "Legs :" ) );
    mvwprintz( w, point( 1, 4 ), color, _( "Feet :" ) );
    const int heading_length = std::max( {utf8_width( _( "Head :" ) ), utf8_width( _( "Torso:" ) ), utf8_width( _( "Arms :" ) ), utf8_width( _( "Legs :" ) ), utf8_width( _( "Feet :" ) )} )
                               + 2;
    const int max_length = getmaxx( w ) - heading_length;
    trim_and_print( w, point( heading_length, 0 ), max_length, color, get_armor( u,
                    bodypart_id( "head" ) ) );
    trim_and_print( w, point( heading_length, 1 ), max_length, color, get_armor( u,
                    bodypart_id( "torso" ) ) );
    trim_and_print( w, point( heading_length, 2 ), max_length, color, get_armor( u,
                    bodypart_id( "arm_r" ) ) );
    trim_and_print( w, point( heading_length, 3 ), max_length, color, get_armor( u,
                    bodypart_id( "leg_r" ) ) );
    trim_and_print( w, point( heading_length, 4 ), max_length, color, get_armor( u,
                    bodypart_id( "foot_r" ) ) );
    wnoutrefresh( w );
}

static void draw_armor( const avatar &u, const catacurses::window &w )
{
    werase( w );
    nc_color color = c_light_gray;
    mvwprintz( w, point_zero, color, _( "Head :" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 1 ), color, _( "Torso:" ) );
    mvwprintz( w, point( 0, 2 ), color, _( "Arms :" ) );
    mvwprintz( w, point( 0, 3 ), color, _( "Legs :" ) );
    mvwprintz( w, point( 0, 4 ), color, _( "Feet :" ) );
    const int heading_length = std::max( {utf8_width( _( "Head :" ) ), utf8_width( _( "Torso:" ) ), utf8_width( _( "Arms :" ) ), utf8_width( _( "Legs :" ) ), utf8_width( _( "Feet :" ) )} )
                               + 1;
    const int max_length = getmaxx( w ) - heading_length;
    trim_and_print( w, point( heading_length, 0 ), max_length, color, get_armor( u,
                    bodypart_id( "head" ) ) );
    trim_and_print( w, point( heading_length, 1 ), max_length, color, get_armor( u,
                    bodypart_id( "torso" ) ) );
    trim_and_print( w, point( heading_length, 2 ), max_length, color, get_armor( u,
                    bodypart_id( "arm_r" ) ) );
    trim_and_print( w, point( heading_length, 3 ), max_length, color, get_armor( u,
                    bodypart_id( "leg_r" ) ) );
    trim_and_print( w, point( heading_length, 4 ), max_length, color, get_armor( u,
                    bodypart_id( "foot_r" ) ) );
    wnoutrefresh( w );
}

static void draw_messages( avatar &, const catacurses::window &w )
{
    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 1, 0 /*topline*/, maxlength - 1, line );
    wnoutrefresh( w );
}

static void draw_messages_classic( avatar &, const catacurses::window &w )
{
    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 0, 0 /*topline*/, maxlength, line );
    wnoutrefresh( w );
}

#if defined(TILES)
static void draw_mminimap( avatar &, const catacurses::window &w )
{
    werase( w );
    g->draw_pixel_minimap( w );
    wnoutrefresh( w );
}
#endif

static void draw_compass( avatar &, const catacurses::window &w )
{
    werase( w );
    g->mon_info( w );
    wnoutrefresh( w );
}

static void draw_compass_padding( avatar &, const catacurses::window &w )
{
    werase( w );
    g->mon_info( w, 1 );
    wnoutrefresh( w );
}

static void draw_overmap_narrow( avatar &u, const catacurses::window &w )
{
    werase( w );
    const tripoint_abs_omt curs = u.global_omt_location();
    draw_rectangle( w, c_light_gray, point_zero, point( 31, 13 ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    overmap_ui::draw_overmap_chunk( w, u, curs, point( 1, 1 ), 30, 12 );
    wnoutrefresh( w );
}

static void draw_overmap_wide( avatar &u, const catacurses::window &w )
{
    werase( w );
    const tripoint_abs_omt curs = u.global_omt_location();
    draw_rectangle( w, c_light_gray, point_zero, point( 43, 19 ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    overmap_ui::draw_overmap_chunk( w, u, curs, point( 1, 1 ), 42, 18 );
    wnoutrefresh( w );
}

static void draw_veh_compact( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // vehicle display
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( get_map().veh_at( u.pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, point_zero );
        mvwprintz( w, point( 6, 0 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        veh->print_speed_gauge( w, point( 12, 0 ), 1 );
    }

    wnoutrefresh( w );
}

static void draw_veh_padding( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // vehicle display
    vehicle *veh = g->remoteveh();
    if( veh == nullptr && u.in_vehicle ) {
        veh = veh_pointer_or_null( get_map().veh_at( u.pos() ) );
    }
    if( veh ) {
        veh->print_fuel_indicators( w, point_east );
        mvwprintz( w, point( 7, 0 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        veh->print_speed_gauge( w, point( 13, 0 ), 1 );
    }

    wnoutrefresh( w );
}

static void draw_ai_goal( const avatar &u, const catacurses::window &w )
{
    werase( w );
    behavior::tree needs;
    needs.add( &string_id<behavior::node_t>( "npc_needs" ).obj() );
    behavior::character_oracle_t player_oracle( &u );
    std::string current_need = needs.tick( &player_oracle );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Goal: %s" ), current_need );
    wnoutrefresh( w );
}

static void draw_location_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, point_zero, c_light_gray, _( "Location:" ) );
    mvwprintz( w, point( 10, 0 ), c_white, utf8_truncate( overmap_buffer.ter(
                   u.global_omt_location() )->get_name(), getmaxx( w ) - 13 ) );

    wnoutrefresh( w );
}

static void draw_weather_classic( avatar &, const catacurses::window &w )
{
    werase( w );

    if( get_map().get_abs_sub().z < 0 ) {
        mvwprintz( w, point_zero, c_light_gray, _( "Underground" ) );
    } else {
        mvwprintz( w, point_zero, c_light_gray, _( "Weather :" ) );
        mvwprintz( w, point( 10, 0 ), get_weather().weather_id->color,
                   get_weather().weather_id->name.translated() );
    }
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Moon :" ) );
    nc_color clr = c_white;
    print_colored_text( w, point( 38, 0 ), clr, c_white, get_moon_graphic() );

    wnoutrefresh( w );
}

static void draw_lighting_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    const std::pair<std::string, nc_color> ll = get_light_level(
                get_avatar().fine_detail_vision_mod() );
    mvwprintz( w, point_zero, c_light_gray, _( "Lighting:" ) );
    mvwprintz( w, point( 10, 0 ), ll.second, ll.first );

    if( !u.is_deaf() ) {
        mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Sound:" ) );
        mvwprintz( w, point( 38, 0 ), c_yellow, std::to_string( u.volume ) );
    } else {
        mvwprintz( w, point( 31, 0 ), c_red, _( "Deaf!" ) );
    }

    wnoutrefresh( w );
}

static void draw_weapon_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    mvwprintz( w, point_zero, c_light_gray, _( "Weapon  :" ) );
    trim_and_print( w, point( 10, 0 ), getmaxx( w ) - 24, c_light_gray, u.weapname() );

    // Print in sidebar currently used martial style.
    const std::string style = u.martial_arts_data->selected_style_name( u );

    if( !style.empty() ) {
        const nc_color style_color = u.is_armed() ? c_red : c_blue;
        mvwprintz( w, point( 31, 0 ), style_color, style );
    }

    wnoutrefresh( w );
}

static void draw_time_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    // display date
    mvwprintz( w, point_zero, c_white,
               calendar::name_season( season_of_year( calendar::turn ) ) + "," );
    std::string day = std::to_string( day_of_season<int>( calendar::turn ) + 1 );
    mvwprintz( w, point( 8, 0 ), c_white, _( "Day " ) + day );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, point( 15, 0 ), c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( get_map().get_abs_sub().z >= 0 ) {
        wmove( w, point( 15, 0 ) );
        draw_time_graphic( w );
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        mvwprintz( w, point( 15, 0 ), c_light_gray, _( "Time: ???" ) );
    }

    if( u.has_item_with_flag( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        std::string temp = print_temperature( get_weather().get_temperature( u.pos() ) );
        mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Temp : " ) + temp );
    }

    wnoutrefresh( w );
}

static void draw_hint( const avatar &, const catacurses::window &w )
{
    werase( w );
    std::string press = press_x( ACTION_TOGGLE_PANEL_ADM );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_green, press );
    mvwprintz( w, point( 2 + utf8_width( press ), 0 ), c_white, _( "to open sidebar options" ) );

    wnoutrefresh( w );
}

static void draw_weariness( const avatar &u, const catacurses::window &w )
{
    werase( w );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    const float exertion_mult = u.exertion_adjusted_move_multiplier( activity ) ;
    if( exertion_mult < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    std::pair<int, int> bar = u.weariness_transition_progress();
    std::pair<std::string, nc_color> weary_bar = get_hp_bar( bar.first, bar.second );

    static const std::string progress_label = translate_marker( "Transition:" );
    static const std::string malus_label = translate_marker( "Malus: " );

    const int wplabel_len = utf8_width( _( progress_label ) );
    const int malus_start = getmaxx( w ) - utf8_width( _( malus_label ) ) - 5;
    const int malus = ( 1 / exertion_mult ) * 100 - 100;

    const std::string malus_str = string_format( "%s+%3d%%", _( malus_label ), malus );

    mvwprintz( w, point_zero, c_light_gray, _( progress_label ) );
    mvwprintz( w, point( wplabel_len + 1, 0 ), weary_bar.second, weary_bar.first );
    mvwprintz( w, point( malus_start, 0 ), act_color, malus_str );

    wnoutrefresh( w );
}

static void draw_weariness_narrow( const avatar &u, const catacurses::window &w )
{
    werase( w );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    const float exertion_mult = u.exertion_adjusted_move_multiplier( activity ) ;
    if( exertion_mult < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    std::pair<int, int> bar = u.weariness_transition_progress();
    std::pair<std::string, nc_color> weary_bar = get_hp_bar( bar.first, bar.second );

    static const std::string progress_label = translate_marker( "Transition:" );
    static const std::string malus_label = translate_marker( "Malus: " );

    const int wplabel_len = utf8_width( _( progress_label ) );
    const int malus_start = getmaxx( w ) - utf8_width( _( malus_label ) ) - 5;
    const int malus = ( 1 / exertion_mult ) * 100 - 100;

    const std::string malus_str = string_format( "%s+%3d%%", _( malus_label ), malus );

    mvwprintz( w, point_east, c_light_gray, _( progress_label ) );
    mvwprintz( w, point( wplabel_len + 1, 0 ), weary_bar.second, weary_bar.first );
    mvwprintz( w, point( malus_start, 0 ), act_color, malus_str );

    wnoutrefresh( w );
}

static void draw_weariness_wide( const avatar &u, const catacurses::window &w )
{
    werase( w );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    const float exertion_mult = u.exertion_adjusted_move_multiplier( activity ) ;
    if( exertion_mult < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    std::pair<int, int> bar = u.weariness_transition_progress();
    std::pair<std::string, nc_color> weary_bar = get_hp_bar( bar.first, bar.second );

    static const std::string progress_label = translate_marker( "Weary Transition:" );
    static const std::string malus_label = translate_marker( "Weary Malus: " );

    const int wplabel_len = utf8_width( _( progress_label ) );
    const int malus_start = getmaxx( w ) - utf8_width( _( malus_label ) ) - 5;
    const int malus = ( 1 / exertion_mult ) * 100 - 100;

    const std::string malus_str = string_format( "%s+%3d%%", _( malus_label ), malus );

    mvwprintz( w, point_east, c_light_gray, _( progress_label ) );
    mvwprintz( w, point( wplabel_len + 1, 0 ), weary_bar.second, weary_bar.first );
    mvwprintz( w, point( malus_start, 0 ), act_color, malus_str );

    wnoutrefresh( w );
}

static void draw_weariness_classic( const avatar &u, const catacurses::window &w )
{
    werase( w );

    int weariness = u.weariness_level();
    std::pair<translation, nc_color> weary = weariness_description( weariness );
    const float activity = u.instantaneous_activity_level();

    nc_color act_color;
    const float exertion_mult = u.exertion_adjusted_move_multiplier( activity ) ;
    if( exertion_mult < 1.0 ) {
        act_color = c_red;
    } else {
        act_color = c_light_gray;
    }

    static const std::string weary_label = translate_marker( "Weariness:" );
    static const std::string activity_label = translate_marker( "Activity:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = std::floor( getmaxx( w ) / 2 );

    mvwprintz( w, point_zero, c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 1, 0 ), weary.second, weary.first.translated() );
    mvwprintz( w, point( act_start, 0 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 0 ), act_color,
               activity_level::activity_level_str( activity ) );

    std::pair<int, int> bar = u.weariness_transition_progress();
    std::pair<std::string, nc_color> weary_bar = get_hp_bar( bar.first, bar.second );

    static const std::string progress_label = translate_marker( "Weary Transition:" );
    static const std::string malus_label = translate_marker( "Weary Malus: " );

    const int wplabel_len = utf8_width( _( progress_label ) );
    const int malus_start = getmaxx( w ) - utf8_width( _( malus_label ) ) - 5;
    const int malus = ( 1 / exertion_mult ) * 100 - 100;

    const std::string malus_str = string_format( "%s+%3d%%", _( malus_label ), malus );

    mvwprintz( w, point_south, c_light_gray, _( progress_label ) );
    mvwprintz( w, point( wplabel_len + 1, 1 ), weary_bar.second, weary_bar.first );
    mvwprintz( w, point( malus_start, 1 ), act_color, malus_str );

    wnoutrefresh( w );
}

static void print_mana( const player &u, const catacurses::window &w, const std::string &fmt_string,
                        const int j1, const int j2, const int j3, const int j4 )
{
    werase( w );

    auto mana_pair = mana_stat( u );
    const std::string mana_string = string_format( fmt_string,
                                    //~ translation should not exceed 4 console cells
                                    utf8_justify( _( "Mana" ), j1 ),
                                    colorize( utf8_justify( mana_pair.second, j2 ), mana_pair.first ),
                                    //~ translation should not exceed 9 console cells
                                    utf8_justify( _( "Max Mana" ), j3 ),
                                    colorize( utf8_justify( std::to_string( u.magic->max_mana( u ) ), j4 ), c_light_blue ) );
    nc_color gray = c_light_gray;
    print_colored_text( w, point_zero, gray, gray, mana_string );

    wnoutrefresh( w );
}

static void draw_mana_classic( const player &u, const catacurses::window &w )
{
    print_mana( u, w, "%s: %s %s: %s", -8, -5, 20, -5 );
}

static void draw_mana_compact( const player &u, const catacurses::window &w )
{
    print_mana( u, w, "%s %s %s %s", 4, -5, 12, -5 );
}

static void draw_mana_narrow( const player &u, const catacurses::window &w )
{
    print_mana( u, w, " %s: %s %s : %s", -5, -5, 9, -5 );
}

static void draw_mana_wide( const player &u, const catacurses::window &w )
{
    print_mana( u, w, " %s: %s %s : %s", -5, -5, 13, -5 );
}

// ============
// INITIALIZERS
// ============

static bool spell_panel()
{
    return get_avatar().magic->knows_spell();
}

bool default_render()
{
    return true;
}

static std::vector<window_panel> initialize_default_classic_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_health_classic, "Health", to_translation( "Health" ),
                                    7, 44, true ) );
    ret.emplace_back( window_panel( draw_weariness_classic, "Weariness", to_translation( "Weariness" ),
                                    2, 44, true ) );
    ret.emplace_back( window_panel( draw_location_classic, "Location", to_translation( "Location" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_mana_classic, "Mana", to_translation( "Mana" ),
                                    1, 44, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_weather_classic, "Weather", to_translation( "Weather" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_lighting_classic, "Lighting", to_translation( "Lighting" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_weapon_classic, "Weapon", to_translation( "Weapon" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_time_classic, "Time", to_translation( "Time" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_wind, "Wind", to_translation( "Wind" ),
                                    1, 44, false ) );
    ret.emplace_back( window_panel( draw_armor, "Armor", to_translation( "Armor" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_compass_padding, "Compass", to_translation( "Compass" ),
                                    8, 44, true ) );
    ret.emplace_back( window_panel( draw_overmap_wide, "Overmap", to_translation( "Overmap" ),
                                    20, 44, false ) );
    ret.emplace_back( window_panel( draw_messages_classic, "Log", to_translation( "Log" ),
                                    -2, 44, true ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, 44, true, default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", to_translation( "AI Needs" ),
                                    1, 44, false ) );
    return ret;
}

static std::vector<window_panel> initialize_default_compact_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_limb2, "Limbs", to_translation( "Limbs" ),
                                    3, 32, true ) );
    ret.emplace_back( window_panel( draw_stealth, "Sound", to_translation( "Sound" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_stats, "Stats", to_translation( "Stats" ),
                                    2, 32, true ) );
    ret.emplace_back( window_panel( draw_weariness, "Weariness", to_translation( "Weariness" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_mana_compact, "Mana", to_translation( "Mana" ),
                                    1, 32, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_time, "Time", to_translation( "Time" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_needs_compact, "Needs", to_translation( "Needs" ),
                                    3, 32, true ) );
    ret.emplace_back( window_panel( draw_env_compact, "Env", to_translation( "Env" ),
                                    6, 32, true ) );
    ret.emplace_back( window_panel( draw_veh_compact, "Vehicle", to_translation( "Vehicle" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_armor, "Armor", to_translation( "Armor" ),
                                    5, 32, false ) );
    ret.emplace_back( window_panel( draw_messages_classic, "Log", to_translation( "Log" ),
                                    -2, 32, true ) );
    ret.emplace_back( window_panel( draw_compass, "Compass", to_translation( "Compass" ),
                                    8, 32, true ) );
    ret.emplace_back( window_panel( draw_overmap_narrow, "Overmap", to_translation( "Overmap" ),
                                    14, 32, false ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, 32, true, default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", to_translation( "AI Needs" ),
                                    1, 32, false ) );

    return ret;
}

static std::vector<window_panel> initialize_default_label_narrow_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_hint, "Hint", to_translation( "Hint" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_limb_narrow, "Limbs", to_translation( "Limbs" ),
                                    3, 32, true ) );
    ret.emplace_back( window_panel( draw_char_narrow, "Movement", to_translation( "Movement" ),
                                    3, 32, true ) );
    ret.emplace_back( window_panel( draw_mana_narrow, "Mana", to_translation( "Mana" ),
                                    1, 32, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_stat_narrow, "Stats", to_translation( "Stats" ),
                                    4, 32, true ) );
    ret.emplace_back( window_panel( draw_weariness_narrow, "Weariness", to_translation( "Weariness" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_veh_padding, "Vehicle", to_translation( "Vehicle" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_loc_narrow, "Location", to_translation( "Location" ),
                                    5, 32, true ) );
    ret.emplace_back( window_panel( draw_wind_padding, "Wind", to_translation( "Wind" ),
                                    1, 32, false ) );
    ret.emplace_back( window_panel( draw_weapon_labels, "Weapon", to_translation( "Weapon" ),
                                    2, 32, true ) );
    ret.emplace_back( window_panel( draw_needs_narrow, "Needs", to_translation( "Needs" ),
                                    5, 32, true ) );
    ret.emplace_back( window_panel( draw_sound_narrow, "Sound", to_translation( "Sound" ),
                                    1, 32, true ) );
    ret.emplace_back( window_panel( draw_messages, "Log", to_translation( "Log" ),
                                    -2, 32, true ) );
    ret.emplace_back( window_panel( draw_moon_narrow, "Moon", to_translation( "Moon" ),
                                    2, 32, false ) );
    ret.emplace_back( window_panel( draw_armor_padding, "Armor", to_translation( "Armor" ),
                                    5, 32, false ) );
    ret.emplace_back( window_panel( draw_compass_padding, "Compass", to_translation( "Compass" ),
                                    8, 32, true ) );
    ret.emplace_back( window_panel( draw_overmap_narrow, "Overmap", to_translation( "Overmap" ),
                                    14, 32, false ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, 32, true, default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", to_translation( "AI Needs" ),
                                    1, 32, false ) );

    return ret;
}

static std::vector<window_panel> initialize_default_label_panels()
{
    std::vector<window_panel> ret;

    ret.emplace_back( window_panel( draw_hint, "Hint", to_translation( "Hint" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_limb_wide, "Limbs", to_translation( "Limbs" ),
                                    2, 44, true ) );
    ret.emplace_back( window_panel( draw_char_wide, "Movement", to_translation( "Movement" ),
                                    2, 44, true ) );
    ret.emplace_back( window_panel( draw_mana_wide, "Mana", to_translation( "Mana" ),
                                    1, 44, true, spell_panel ) );
    ret.emplace_back( window_panel( draw_stat_wide, "Stats", to_translation( "Stats" ),
                                    3, 44, true ) );
    ret.emplace_back( window_panel( draw_weariness_wide, "Weariness", to_translation( "Weariness" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_veh_padding, "Vehicle", to_translation( "Vehicle" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_loc_wide_map, "Location", to_translation( "Location" ),
                                    5, 44, true ) );
    ret.emplace_back( window_panel( draw_wind_padding, "Wind", to_translation( "Wind" ),
                                    1, 44, false ) );
    ret.emplace_back( window_panel( draw_loc_wide, "Location Alt", to_translation( "Location Alt" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_weapon_labels, "Weapon", to_translation( "Weapon" ),
                                    2, 44, true ) );
    ret.emplace_back( window_panel( draw_needs_labels, "Needs", to_translation( "Needs" ),
                                    3, 44, true ) );
    ret.emplace_back( window_panel( draw_needs_labels_alt, "Needs_Alt", to_translation( "Needs Alt" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_sound_labels, "Sound", to_translation( "Sound" ),
                                    1, 44, true ) );
    ret.emplace_back( window_panel( draw_messages, "Log", to_translation( "Log" ),
                                    -2, 44, true ) );
    ret.emplace_back( window_panel( draw_moon_wide, "Moon", to_translation( "Moon" ),
                                    1, 44, false ) );
    ret.emplace_back( window_panel( draw_armor_padding, "Armor", to_translation( "Armor" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_compass_padding, "Compass", to_translation( "Compass" ),
                                    8, 44, true ) );
    ret.emplace_back( window_panel( draw_overmap_wide, "Overmap", to_translation( "Overmap" ),
                                    20, 44, false ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, 44, true, default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", to_translation( "AI Needs" ),
                                    1, 44, false ) );

    return ret;
}

static std::map<std::string, panel_layout> initialize_default_panel_layouts()
{
    std::map<std::string, panel_layout> ret;

    ret.emplace( "classic", panel_layout( to_translation( "classic" ),
                                          initialize_default_classic_panels() ) );
    ret.emplace( "compact", panel_layout( to_translation( "compact" ),
                                          initialize_default_compact_panels() ) );
    ret.emplace( "labels-narrow", panel_layout( to_translation( "labels narrow" ),
                 initialize_default_label_narrow_panels() ) );
    ret.emplace( "labels", panel_layout( to_translation( "labels" ),
                                         initialize_default_label_panels() ) );

    return ret;
}

panel_manager::panel_manager()
{
    current_layout_id = "labels";
    layouts = initialize_default_panel_layouts();
}

panel_layout &panel_manager::get_current_layout()
{
    auto kv = layouts.find( current_layout_id );
    if( kv != layouts.end() ) {
        return kv->second;
    }
    debugmsg( "Invalid current panel layout, defaulting to classic" );
    current_layout_id = "classic";
    return get_current_layout();
}

std::string panel_manager::get_current_layout_id() const
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
    update_offsets( get_current_layout().panels().begin()->get_width() );
}

void panel_manager::update_offsets( int x )
{
    width_right = x;
    width_left = 0;
}

bool panel_manager::save()
{
    return write_to_file( PATH_INFO::panel_options(), [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );
    }, _( "panel options" ) );
}

bool panel_manager::load()
{
    return read_from_file_optional_json( PATH_INFO::panel_options(), [&]( JsonIn & jsin ) {
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

        for( const auto &panel : kv.second.panels() ) {
            json.start_object();

            json.member( "name", panel.get_id() );
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
    for( JsonObject joLayout : joLayouts.get_array( "layouts" ) ) {
        std::string layout_id = joLayout.get_string( "layout_id" );
        auto &layout = layouts.find( layout_id )->second.panels();
        auto it = layout.begin();

        for( JsonObject joPanel : joLayout.get_array( "panels" ) ) {
            std::string id = joPanel.get_string( "name" );
            bool toggle = joPanel.get_bool( "toggle" );

            for( auto it2 = layout.begin() + std::distance( layout.begin(), it ); it2 != layout.end(); ++it2 ) {
                if( it2->get_id() == id ) {
                    if( it->get_id() != id ) {
                        window_panel panel = *it2;
                        layout.erase( it2 );
                        it = layout.insert( it, panel );
                    }
                    it->toggle = toggle;
                    ++it;
                    break;
                }
            }
        }
    }
    jsin.end_array();
}

void panel_manager::show_adm()
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

    const std::vector<int> column_widths = { 17, 37, 17 };

    size_t current_col = 0;
    size_t current_row = 0;
    bool swapping = false;
    size_t source_row = 0;
    size_t source_index = 0;

    bool recalc = true;
    bool exit = false;
    // map of row the panel is on vs index
    // panels not renderable due to game configuration will not be in this map
    std::map<size_t, size_t> row_indices;

    g->show_panel_adm = true;
    g->invalidate_main_ui_adaptor();

    catacurses::window w;

    const int popup_height = 24;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( popup_height, 75,
                                point( ( TERMX / 2 ) - 38, ( TERMY / 2 ) - 10 ) );

        ui.position_from_window( w );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        auto &panels = get_current_layout().panels();

        werase( w );
        decorate_panel( _( "SIDEBAR OPTIONS" ), w );

        for( std::pair<size_t, size_t> row_indx : row_indices ) {
            const std::string name = panels[row_indx.second].get_name();
            if( swapping && source_index == row_indx.second ) {
                mvwprintz( w, point( 5, current_row + 1 ), c_yellow, name );
            } else {
                int offset = 0;
                if( !swapping ) {
                    offset = 0;
                } else if( current_row > source_row && row_indx.first > source_row &&
                           row_indx.first <= current_row ) {
                    offset = -1;
                } else if( current_row < source_row && row_indx.first < source_row &&
                           row_indx.first >= current_row ) {
                    offset = 1;
                }
                const nc_color toggle_color = panels[row_indx.second].toggle ? c_white : c_dark_gray;
                mvwprintz( w, point( 4, row_indx.first + 1 + offset ), toggle_color, name );
            }
        }
        size_t i = 1;
        for( const auto &layout : layouts ) {
            mvwprintz( w, point( column_widths[0] + column_widths[1] + 4, i ),
                       current_layout_id == layout.first ? c_light_blue : c_white, layout.second.name() );
            i++;
        }
        int col_offset = 0;
        for( i = 0; i < current_col; i++ ) {
            col_offset += column_widths[i];
        }
        mvwprintz( w, point( 1 + ( col_offset ), current_row + 1 ), c_yellow, ">>" );
        // Draw vertical separators
        mvwvline( w, point( column_widths[0], 1 ), 0, popup_height - 2 );
        mvwvline( w, point( column_widths[0] + column_widths[1], 1 ), 0, popup_height - 2 );

        col_offset = column_widths[0] + 2;
        int col_width = column_widths[1] - 4;
        mvwprintz( w, point( col_offset, 1 ), c_light_green, trunc_ellipse( ctxt.get_desc( "TOGGLE_PANEL" ),
                   col_width ) + ":" );
        mvwprintz( w, point( col_offset, 2 ), c_white, _( "Toggle panels on/off" ) );
        mvwprintz( w, point( col_offset, 3 ), c_light_green, trunc_ellipse( ctxt.get_desc( "MOVE_PANEL" ),
                   col_width ) + ":" );
        mvwprintz( w, point( col_offset, 4 ), c_white, _( "Change display order" ) );
        mvwprintz( w, point( col_offset, 5 ), c_light_green, trunc_ellipse( ctxt.get_desc( "QUIT" ),
                   col_width ) + ":" );
        mvwprintz( w, point( col_offset, 6 ), c_white, _( "Exit" ) );

        wnoutrefresh( w );
    } );

    while( !exit ) {
        auto &panels = get_current_layout().panels();

        if( recalc ) {
            recalc = false;

            row_indices.clear();
            for( size_t i = 0, row = 0; i < panels.size(); i++ ) {
                if( panels[i].render() ) {
                    row_indices.emplace( row, i );
                    row++;
                }
            }
        }

        const size_t num_rows = current_col == 0 ? row_indices.size() : layouts.size();
        current_row = clamp<size_t>( current_row, 0, num_rows - 1 );

        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            if( current_row > 0 ) {
                current_row -= 1;
            } else {
                current_row = num_rows - 1;
            }
        } else if( action == "DOWN" ) {
            if( current_row + 1 < num_rows ) {
                current_row += 1;
            } else {
                current_row = 0;
            }
        } else if( action == "MOVE_PANEL" && current_col == 0 ) {
            swapping = !swapping;
            if( swapping ) {
                // source window from the swap
                // saving win1 index
                source_row = current_row;
                source_index = row_indices[current_row];
            } else {
                // dest window for the swap
                // saving win2 index
                const size_t target_index = row_indices[current_row];

                int distance = target_index - source_index;
                size_t step_dir = distance > 0 ? 1 : -1;
                for( size_t i = source_index; i != target_index; i += step_dir ) {
                    std::swap( panels[i], panels[i + step_dir] );
                }
                g->invalidate_main_ui_adaptor();
                recalc = true;
            }
        } else if( !swapping && action == "MOVE_PANEL" && current_col == 2 ) {
            auto iter = std::next( layouts.begin(), current_row );
            current_layout_id = iter->first;
            int width = panel_manager::get_manager().get_current_layout().panels().begin()->get_width();
            update_offsets( width );
            int h; // to_map_font_dimension needs a second input
            to_map_font_dimension( width, h );
            // tell the game that the main screen might have a different size now.
            g->mark_main_ui_adaptor_resize();
            recalc = true;
        } else if( !swapping && ( action == "RIGHT" || action == "LEFT" ) ) {
            // there are only two columns
            if( current_col == 0 ) {
                current_col = 2;
            } else {
                current_col = 0;
            }
        } else if( !swapping && action == "TOGGLE_PANEL" && current_col == 0 ) {
            panels[row_indices[current_row]].toggle = !panels[row_indices[current_row]].toggle;
            g->invalidate_main_ui_adaptor();
        } else if( action == "QUIT" ) {
            exit = true;
            save();
        }
    }

    g->show_panel_adm = false;
    g->invalidate_main_ui_adaptor();
}
