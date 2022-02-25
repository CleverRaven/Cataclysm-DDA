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
#include "display.h"
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
#include "mood_face.h"
#include "move_mode.h"
#include "mtype.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "pimpl.h"
#include "point.h"
#include "string_formatter.h"
#include "tileray.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_type.h"
#include "widget.h"

static const efftype_id effect_got_checked( "got_checked" );
static const efftype_id effect_mending( "mending" );

static const flag_id json_flag_SPLINT( "SPLINT" );
static const flag_id json_flag_THERMOMETER( "THERMOMETER" );

static const string_id<behavior::node_t> behavior__node_t_npc_needs( "npc_needs" );

static const trait_id trait_NOPAIN( "NOPAIN" );

// constructor
window_panel::window_panel(
    const std::function<void( const draw_args & )> &draw_func,
    const std::string &id, const translation &nm, const int ht, const int wd,
    const bool default_toggle_, const std::function<bool()> &render_func,
    const bool force_draw )
    : draw( [draw_func]( const draw_args & d ) -> int { draw_func( d ); return 0; } ),
      render( render_func ), toggle( default_toggle_ ), always_draw( force_draw ),
      height( ht ), width( wd ), id( id ), name( nm )
{
    wgt = widget_id::NULL_ID();
}

window_panel::window_panel(
    const std::string &id, const translation &nm, const int ht, const int wd,
    const bool default_toggle_, const std::function<bool()> &render_func,
    const bool force_draw )
    : draw( []( const draw_args & ) -> int { return 0; } ),
      render( render_func ), toggle( default_toggle_ ), always_draw( force_draw ),
      height( ht ), width( wd ), id( id ), name( nm )
{
    wgt = widget_id::NULL_ID();
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

static int get_wgt_height( const widget_id &wgt )
{
    if( wgt->_widgets.empty() || wgt->_arrange == "columns" ) {
        return wgt->_height > 0 ? wgt->_height : 1;
    }
    int h = 0;
    for( const widget_id &w : wgt->_widgets ) {
        h += get_wgt_height( w );
    }
    return h;
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
    if( wgt.is_valid() && wgt->_arrange != "columns" ) {
        return get_wgt_height( wgt );
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

void window_panel::set_widget( const widget_id &w )
{
    wgt = w;
}

const widget_id &window_panel::get_widget() const
{
    return wgt;
}

void window_panel::set_draw_func( const std::function<int( const draw_args & )> &draw_func )
{
    draw = draw_func;
}

panel_layout::panel_layout( const translation &_name, const std::vector<window_panel> &_panels )
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
    // Map is centered on curs - typically player's global_omt_location
    const point_abs_omt curs = global_omt.xy();
    const tripoint_abs_omt targ = you.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;
    const int start_y = start_input.y;
    const int start_x = start_input.x;
    const point mid( width / 2, height / 2 );
    map &here = get_map();
    const int sight_points = you.overmap_sight_range( g->light_level( you.posz() ) );

    // i scans across width, with 0 in the middle(ish)
    //     -(w/2) ... w-(w/2)-1
    // w:9   -4 ... 4
    // w:10  -5 ... 4
    // w:11  -5 ... 5
    // w:12  -6 ... 5
    // w:13  -6 ... 6
    for( int i = -( width / 2 ); i <= width - ( width / 2 ) - 1; i++ ) {
        // j scans across height, with 0 in the middle(ish)
        // (same algorithm)
        for( int j = -( height / 2 ); j <= height - ( height / 2 ) - 1; j++ ) {
            // omp is the current overmap point, at the current z-level
            const tripoint_abs_omt omp( curs + point( i, j ), here.get_abs_sub().z() );
            // Terrain color and symbol to use for this point
            nc_color ter_color;
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omp );
            const bool vehicle_here = overmap_buffer.has_vehicle( omp );
            if( overmap_buffer.has_note( omp ) ) {
                const std::string &note_text = overmap_buffer.note( omp );
                std::pair<std::string, nc_color> sym_color = display::overmap_note_symbol_color( note_text );
                ter_sym = sym_color.first;
                ter_color = sym_color.second;
            } else if( !seen ) {
                // Always grey # for unseen
                ter_sym = "#";
                ter_color = c_dark_gray;
            } else if( vehicle_here ) {
                // Always cyan c for vehicle
                ter_color = c_cyan;
                ter_sym = "c";
            } else {
                // Otherwise, get symbol and color appropriate for the terrain
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
            // TODO: Build colorized string instead of writing directly to window
            if( i == 0 && j == 0 ) {
                // Highlight player character position in center of minimap
                mvwputch_hi( w_minimap, mid + point( start_x, start_y ), ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, mid + point( i + start_x, j + start_y ), ter_color,
                          ter_sym );
            }

            if( i < -1 || i > 1 || j < -1 || j > 1 ) {
                // Show hordes on minimap, leaving a one-tile space around the player
                int horde_size = overmap_buffer.get_horde_size( omp );
                if( horde_size >= HORDE_VISIBILITY_SIZE &&
                    overmap_buffer.seen( omp ) && you.overmap_los( omp, sight_points ) ) {
                    mvwputch( w_minimap, mid + point( i + start_x, j + start_y ), c_green,
                              horde_size > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
                }
            }
        }
    }

    // When the mission marker is not visible within the current overmap extents,
    // draw an arrow at the edge of the map pointing in the general mission direction.
    // TODO: Replace `drew_mission` with a function like `is_mission_on_map`
    if( !drew_mission ) {
        char glyph = '*';
        if( targ.z() > you.posz() ) {
            glyph = '^';
        } else if( targ.z() < you.posz() ) {
            glyph = 'v';
        }
        const point arrow = display::mission_arrow_offset( you, width, height );
        mvwputch( w_minimap, arrow + point( start_x, start_y ), c_red, glyph );
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

// ===============================
// panels code
// ===============================

static void draw_limb_health( const avatar &u, const catacurses::window &w, bodypart_id bp )
{
    const bool no_feeling = u.has_trait( trait_NOPAIN );
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
            const auto &eff = u.get_effect( effect_mending, bp );
            const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

            if( u.has_effect( effect_got_checked ) ) {
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

    if( u.has_effect( effect_got_checked ) ) {
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

static void draw_limb2( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
        wprintz( w, display::limb_color( u, bp, true, true, true ), str );
        if( i % 2 == 0 ) {
            wmove( w, point( 5, i / 2 ) );
        } else {
            wmove( w, point( 16, i / 2 ) );
        }
        draw_limb_health( u, w, bp );
        i++;
    }

    // print mood
    std::pair<std::string, nc_color> morale_pair = display::morale_face_color( u );

    std::pair<std::string, nc_color> safe_pair = display::safe_mode_text_color( true );
    mvwprintz( w, point( 22, 2 ), safe_pair.second, safe_pair.first );
    mvwprintz( w, point( 27, 2 ), morale_pair.second, morale_pair.first );

    // print stamina
    const auto &stamina = get_hp_bar( u.get_stamina(), u.get_stamina_max() );
    mvwprintz( w, point( 22, 0 ), c_light_gray, _( "STM" ) );
    mvwprintz( w, point( 26, 0 ), stamina.second, stamina.first );

    mvwprintz( w, point( 22, 1 ), c_light_gray, _( "PWR" ) );
    std::pair<std::string, nc_color> power_pair = display::power_text_color( u );
    mvwprintz( w, point( 31 - utf8_width( power_pair.first ), 1 ), power_pair.second,
               power_pair.first );

    wnoutrefresh( w );
}

static void draw_stats( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    nc_color stat_clr = display::str_text_color( u ).second;
    mvwprintz( w, point_zero, c_light_gray, _( "STR" ) );
    int stat = u.get_str();
    mvwprintz( w, point( stat < 10 ? 5 : 4, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = display::dex_text_color( u ).second;
    stat = u.get_dex();
    mvwprintz( w, point( 9, 0 ), c_light_gray, _( "DEX" ) );
    mvwprintz( w, point( stat < 10 ? 14 : 13, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = display::int_text_color( u ).second;
    stat = u.get_int();
    mvwprintz( w, point( 17, 0 ), c_light_gray, _( "INT" ) );
    mvwprintz( w, point( stat < 10 ? 22 : 21, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );
    stat_clr = display::per_text_color( u ).second;
    stat = u.get_per();
    mvwprintz( w, point( 25, 0 ), c_light_gray, _( "PER" ) );
    mvwprintz( w, point( stat < 10 ? 30 : 29, 0 ), stat_clr,
               stat < 100 ? std::to_string( stat ) : "99+" );

    std::pair<std::string, nc_color> weary = display::weariness_text_color( u );
    std::pair<std::string, nc_color> activity = display::activity_text_color( u );

    static const std::string weary_label = translate_marker( "WRY" );
    static const std::string activity_label = translate_marker( "ACT" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point_south, c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 1, 1 ), weary.second, weary.first );
    mvwprintz( w, point( act_start, 1 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 1 ), activity.second, activity.first );

    wnoutrefresh( w );
}

static void draw_stealth( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    mvwprintz( w, point_zero, c_light_gray, _( "Speed" ) );
    mvwprintz( w, point( 7, 0 ), value_color( u.get_speed() ), "%s", u.get_speed() );

    std::pair<std::string, nc_color> move_mode = display::move_mode_letter_color( u );

    mvwprintz( w, point( 15 - utf8_width( move_mode.first ), 0 ), move_mode.second, move_mode.first );
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

static void draw_time( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    // display date
    mvwprintz( w, point_zero, c_light_gray, calendar::name_season( season_of_year( calendar::turn ) ) );
    std::string day = std::to_string( day_of_season<int>( calendar::turn ) + 1 );
    mvwprintz( w, point( 10 - utf8_width( day ), 0 ), c_light_gray, day );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, point( 11, 0 ), c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( get_map().get_abs_sub().z() >= 0 ) {
        wmove( w, point( 11, 0 ) );
        draw_time_graphic( w );
    } else {
        // NOLINTNEXTLINE(cata-text-style): the question mark does not end a sentence
        mvwprintz( w, point( 11, 0 ), c_light_gray, _( "Time: ???" ) );
    }
    //display moon
    mvwprintz( w, point( 22, 0 ), c_white, _( "Moon" ) );
    nc_color clr = c_white;
    print_colored_text( w, point( 27, 0 ), clr, c_white, display::get_moon_graphic() );

    wnoutrefresh( w );
}

static void draw_needs_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    auto hunger_pair = display::hunger_text_color( u );
    mvwprintz( w, point_zero, hunger_pair.second, hunger_pair.first );
    hunger_pair = display::fatigue_text_color( u );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 1 ), hunger_pair.second, hunger_pair.first );
    auto pain_pair = display::pain_text_color( u );
    mvwprintz( w, point( 0, 2 ), pain_pair.second, pain_pair.first );

    hunger_pair = display::thirst_text_color( u );
    mvwprintz( w, point( 17, 0 ), hunger_pair.second, hunger_pair.first );
    std::pair<std::string, nc_color> temp_pair = display::temp_text_color( u );
    mvwprintz( w, point( 17, 1 ), temp_pair.second, temp_pair.first );
    std::pair<std::string, nc_color> arrow_pair = display::temp_delta_arrows( u );
    mvwprintz( w, point( 17 + utf8_width( temp_pair.first ), 1 ), arrow_pair.second, arrow_pair.first );

    mvwprintz( w, point( 17, 2 ), c_light_gray, _( "Focus" ) );
    mvwprintz( w, point( 24, 2 ), focus_color( u.get_focus() ), std::to_string( u.get_focus() ) );

    wnoutrefresh( w );
}

static void draw_limb_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
        wprintz( w, display::limb_color( u, bp, true, true, true ), str + ":" );
        i++;
    }
    wnoutrefresh( w );
}

static void draw_limb_wide( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    int i = 0;
    for( const bodypart_id &bp :
         u.get_all_body_parts( get_body_part_flags::only_main | get_body_part_flags::sorted ) ) {
        int offset = i * 15;
        int ny = offset / 45;
        int nx = offset % 45;
        std::string str = string_format( " %s: ",
                                         left_justify( body_part_hp_bar_ui_text( bp ), 5 ) );
        nc_color part_color = display::limb_color( u, bp, true, true, true );
        print_colored_text( w, point( nx, ny ), part_color, c_white, str );
        draw_limb_health( u, w, bp );
        i++;
    }
    wnoutrefresh( w );
}

static void draw_char_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    std::pair<std::string, nc_color> morale_pair = display::morale_face_color( u );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Stam :" ) );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Focus:" ) );
    mvwprintz( w, point( 19, 0 ), c_light_gray, _( "Mood :" ) );
    mvwprintz( w, point( 19, 1 ), c_light_gray, _( "Speed:" ) );
    mvwprintz( w, point( 19, 2 ), c_light_gray, _( "Move :" ) );

    // print stamina
    auto needs_pair = std::make_pair( get_hp_bar( u.get_stamina(), u.get_stamina_max() ).second,
                                      get_hp_bar( u.get_stamina(), u.get_stamina_max() ).first );
    mvwprintz( w, point( 8, 1 ), needs_pair.first, needs_pair.second );
    const int width = utf8_width( needs_pair.second );
    for( int i = 0; i < 5 - width; i++ ) {
        mvwprintz( w, point( 12 - i, 1 ), c_white, "." );
    }

    mvwprintz( w, point( 8, 2 ), focus_color( u.get_focus() ), "%s", u.get_focus() );
    if( u.calc_focus_change() > 0 ) {
        mvwprintz( w, point( 11, 2 ), c_light_green, "↥" );
    } else if( u.calc_focus_change() < 0 ) {
        mvwprintz( w, point( 11, 2 ), c_light_red, "↧" );
    }

    mvwprintz( w, point( 26, 0 ), morale_pair.second, morale_pair.first );
    mvwprintz( w, point( 26, 1 ), focus_color( u.get_speed() ), "%s", u.get_speed() );
    mvwprintz( w, point( 8, 0 ), c_light_gray, "%s", u.volume );

    std::pair<std::string, nc_color> move_mode_pair = display::move_mode_letter_color( u );
    std::string movecost = std::to_string( u.movecounter ) + string_format( "(%s)",
                           move_mode_pair.first );
    mvwprintz( w, point( 26, 2 ), move_mode_pair.second, "%s", movecost );

    wnoutrefresh( w );
}

static void draw_char_wide( const draw_args &args )
{

    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    std::pair<std::string, nc_color> morale_pair = display::morale_face_color( u );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Sound:" ) );
    mvwprintz( w, point( 16, 0 ), c_light_gray, _( "Mood :" ) );
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Focus:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Stam :" ) );
    mvwprintz( w, point( 16, 1 ), c_light_gray, _( "Speed:" ) );
    mvwprintz( w, point( 31, 1 ), c_light_gray, _( "Move :" ) );

    mvwprintz( w, point( 8, 0 ), c_light_gray, "%s", u.volume );
    mvwprintz( w, point( 23, 0 ), morale_pair.second, morale_pair.first );
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

    std::pair<std::string, nc_color> move_mode_pair = display::move_mode_letter_color( u );
    std::string movecost = std::to_string( u.movecounter ) + string_format( "(%s)",
                           move_mode_pair.first );
    mvwprintz( w, point( 38, 1 ), move_mode_pair.second, "%s", movecost );

    wnoutrefresh( w );
}

static void draw_stat_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Str  :" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Int  :" ) );
    mvwprintz( w, point( 19, 0 ), c_light_gray, _( "Dex  :" ) );
    mvwprintz( w, point( 19, 1 ), c_light_gray, _( "Per  :" ) );

    nc_color stat_clr = display::str_text_color( u ).second;
    mvwprintz( w, point( 8, 0 ), stat_clr, "%s", u.get_str() );
    stat_clr = display::int_text_color( u ).second;
    mvwprintz( w, point( 8, 1 ), stat_clr, "%s", u.get_int() );
    stat_clr = display::dex_text_color( u ).second;
    mvwprintz( w, point( 26, 0 ), stat_clr, "%s", u.get_dex() );
    stat_clr = display::per_text_color( u ).second;
    mvwprintz( w, point( 26, 1 ), stat_clr, "%s", u.get_per() );

    std::pair<std::string, nc_color> power_pair = display::power_text_color( u );
    mvwprintz( w, point( 1, 2 ), c_light_gray, _( "Power:" ) );
    mvwprintz( w, point( 19, 2 ), c_light_gray, _( "Safe :" ) );
    mvwprintz( w, point( 8, 2 ), power_pair.second, "%s", power_pair.first );
    std::pair<std::string, nc_color> safe_pair = display::safe_mode_text_color( false );
    mvwprintz( w, point( 26, 2 ), safe_pair.second, safe_pair.first );

    std::pair<std::string, nc_color> weary = display::weariness_text_color( u );
    std::pair<std::string, nc_color> activity = display::activity_text_color( u );

    static const std::string weary_label = translate_marker( "Weary:" );
    static const std::string activity_label = translate_marker( "Activ:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point( 1, 3 ), c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 2, 3 ), weary.second, weary.first );
    mvwprintz( w, point( act_start, 3 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 3 ), activity.second, activity.first );

    wnoutrefresh( w );
}

static void draw_stat_wide( const draw_args &args )
{

    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    mvwprintz( w, point_east, c_light_gray, _( "Str  :" ) );
    mvwprintz( w, point_south_east, c_light_gray, _( "Int  :" ) );
    mvwprintz( w, point( 16, 0 ), c_light_gray, _( "Dex  :" ) );
    mvwprintz( w, point( 16, 1 ), c_light_gray, _( "Per  :" ) );

    nc_color stat_clr = display::str_text_color( u ).second;
    mvwprintz( w, point( 8, 0 ), stat_clr, "%s", u.get_str() );
    stat_clr = display::int_text_color( u ).second;
    mvwprintz( w, point( 8, 1 ), stat_clr, "%s", u.get_int() );
    stat_clr = display::dex_text_color( u ).second;
    mvwprintz( w, point( 23, 0 ), stat_clr, "%s", u.get_dex() );
    stat_clr = display::per_text_color( u ).second;
    mvwprintz( w, point( 23, 1 ), stat_clr, "%s", u.get_per() );

    std::pair<std::string, nc_color> power_pair = display::power_text_color( u );
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Power:" ) );
    mvwprintz( w, point( 31, 1 ), c_light_gray, _( "Safe :" ) );
    mvwprintz( w, point( 38, 0 ), power_pair.second, "%s", power_pair.first );
    std::pair<std::string, nc_color> safe_pair = display::safe_mode_text_color( false );
    mvwprintz( w, point( 38, 1 ), safe_pair.second, safe_pair.first );

    std::pair<std::string, nc_color> weary = display::weariness_text_color( u );
    std::pair<std::string, nc_color> activity = display::activity_text_color( u );

    static const std::string weary_label = translate_marker( "Weary:" );
    static const std::string activity_label = translate_marker( "Activ:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = ( getmaxx( w ) / 2 ) - 1;

    mvwprintz( w, point( 1, 2 ), c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 2, 2 ), weary.second, weary.first );
    mvwprintz( w, point( act_start, 2 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 2 ), activity.second, activity.first );

    wnoutrefresh( w );
}

static void draw_loc_labels( const draw_args &args, bool minimap )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Place: " ) );
    wprintz( w, c_white, utf8_truncate( display::current_position_text( u.global_omt_location() ),
                                        getmaxx( w ) - 13 ) );
    map &here = get_map();
    // display weather
    if( here.get_abs_sub().z() < 0 ) {
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
    mvwprintz( w, point( 1, 3 ), c_light_gray, _( "Date : %s" ), display::date_string() );

    // display time
    mvwprintz( w, point( 1, 4 ), c_light_gray, _( "Time : %s" ), display::time_string( u ) );

    if( minimap ) {
        const int offset = getmaxx( w ) - 14;
        const tripoint_abs_omt curs = u.global_omt_location();
        overmap_ui::draw_overmap_chunk( w, u, curs, point( offset, 0 ), 13, 5 );
    }
    wnoutrefresh( w );
}

static void draw_loc_narrow( const draw_args &args )
{
    draw_loc_labels( args, false );
}

static void draw_loc_wide( const draw_args &args )
{
    draw_loc_labels( args, false );
}

static void draw_loc_wide_map( const draw_args &args )
{
    draw_loc_labels( args, true );
}

static void draw_moon_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Moon : %s" ), display::get_moon() );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Temp : %s" ), display::get_temp( u ) );
    wnoutrefresh( w );
}

static void draw_moon_wide( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Moon : %s" ), display::get_moon() );
    mvwprintz( w, point( 24, 0 ), c_light_gray, _( "Temp : %s" ), display::get_temp( u ) );
    wnoutrefresh( w );
}

static void draw_weapon_labels( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_gray, _( "Wield:" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray, _( "Style:" ) );
    trim_and_print( w, point( 8, 0 ), getmaxx( w ) - 8, c_light_gray, u.weapname() );
    mvwprintz( w, point( 8, 1 ), c_light_gray, "%s", u.martial_arts_data->selected_style_name( u ) );
    wnoutrefresh( w );
}

static void draw_needs_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color( u );
    std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color( u );
    std::pair<std::string, nc_color> rest_pair = display::fatigue_text_color( u );
    std::pair<std::string, nc_color> temp_pair = display::temp_text_color( u );
    std::pair<std::string, nc_color> pain_pair = display::pain_text_color( u );
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
    mvwprintz( w, point( 8, 4 ), temp_pair.second, temp_pair.first );
    wnoutrefresh( w );
}

static void draw_needs_labels( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color( u );
    std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color( u );
    std::pair<std::string, nc_color> rest_pair = display::fatigue_text_color( u );
    std::pair<std::string, nc_color> weight_pair = display::weight_text_color( u );
    std::pair<std::string, nc_color> temp_pair = display::temp_text_color( u );
    std::pair<std::string, nc_color> pain_pair = display::pain_text_color( u );
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
    mvwprintz( w, point( 8, 2 ), temp_pair.second, temp_pair.first );
    mvwprintz( w, point( 23, 2 ), c_light_gray, _( "Weight:" ) );
    mvwprintz( w, point( 30, 2 ), weight_pair.second, weight_pair.first );
    wnoutrefresh( w );
}

static void draw_needs_labels_alt( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color( u );
    std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color( u );
    std::pair<std::string, nc_color> rest_pair = display::fatigue_text_color( u );
    std::pair<std::string, nc_color> temp_pair = display::temp_text_color( u );
    std::pair<std::string, nc_color> pain_pair = display::pain_text_color( u );
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
    mvwprintz( w, point( 8, 4 ), temp_pair.second, temp_pair.first );
    wnoutrefresh( w );
}

static void draw_sound_labels( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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

static void draw_sound_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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

static void draw_env_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
    mvwprintz( w, point( text_left, 2 ), c_white,
               utf8_truncate( display::current_position_text( u.global_omt_location() ), getmaxx( w ) - 8 ) );
    // weather
    if( get_map().get_abs_sub().z() < 0 ) {
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
    weather_manager &weather = get_weather();
    double windpower = get_local_windpower( weather.windspeed, cur_om_ter,
                                            u.pos(), weather.winddirection, g->is_sheltered( u.pos() ) );
    mvwprintz( w, point( text_left, 5 ), get_wind_color( windpower ),
               get_wind_desc( windpower ) + " " + get_wind_arrow( weather.winddirection ) );

    if( u.has_item_with_flag( json_flag_THERMOMETER ) ||
        u.has_flag( STATIC( json_character_flag( "THERMOMETER" ) ) ) ) {
        std::string temp = print_temperature( weather.get_temperature( u.pos() ) );
        mvwprintz( w, point( 31 - utf8_width( temp ), 5 ), c_light_gray, temp );
    }

    wnoutrefresh( w );
}

static void render_wind( const draw_args &args, const std::string &formatstr )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    mvwprintz( w, point_zero, c_light_gray,
               //~ translation should not exceed 5 console cells
               string_format( formatstr, left_justify( _( "Wind" ), 5 ) ) );

    std::pair<std::string, nc_color> wind_pair = display::wind_text_color( u );
    mvwprintz( w, point( 8, 0 ), wind_pair.second, wind_pair.first );
    wnoutrefresh( w );
}

static void draw_wind( const draw_args &args )
{
    render_wind( args, "%s: " );
}

static void draw_wind_padding( const draw_args &args )
{
    render_wind( args, " %s: " );
}

static void draw_health_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
        wprintz( w, display::limb_color( u, bp, true, true, true ), str );
        wmove( w, point( 14, i ) );
        draw_limb_health( u, w, bp );
        i++;
    }

    // needs
    auto needs_pair = display::hunger_text_color( u );
    mvwprintz( w, point( 21, 1 ), needs_pair.second, needs_pair.first );
    needs_pair = display::thirst_text_color( u );
    mvwprintz( w, point( 21, 2 ), needs_pair.second, needs_pair.first );
    mvwprintz( w, point( 21, 4 ), c_white, _( "Focus" ) );
    mvwprintz( w, point( 27, 4 ), c_white, std::to_string( u.get_focus() ) );
    needs_pair = display::fatigue_text_color( u );
    mvwprintz( w, point( 21, 3 ), needs_pair.second, needs_pair.first );
    auto pain_pair = display::pain_text_color( u );
    mvwprintz( w, point( 21, 0 ), pain_pair.second, pain_pair.first );

    // print mood
    std::pair<std::string, nc_color> morale_pair = display::morale_face_color( u );
    mvwprintz( w, point( 34, 1 ), morale_pair.second, morale_pair.first );

    vehicle *veh = display::vehicle_driven( u );

    if( !veh ) {
        // stats
        std::pair<std::string, nc_color> pair = display::str_text_color( u );
        mvwprintz( w, point( 38, 0 ), pair.second, pair.first );
        pair = display::dex_text_color( u );
        mvwprintz( w, point( 38, 1 ), pair.second, pair.first );
        pair = display::int_text_color( u );
        mvwprintz( w, point( 38, 2 ), pair.second, pair.first );
        pair = display::per_text_color( u );
        mvwprintz( w, point( 38, 3 ), pair.second, pair.first );
    }

    // print safe mode
    std::pair<std::string, nc_color> safe_pair = display::safe_mode_text_color( true );
    mvwprintz( w, point( 40, 4 ), safe_pair.second, safe_pair.first );

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
        std::pair<std::string, nc_color> move_mode_pair = display::move_mode_letter_color( u );
        std::string move_string = std::to_string( u.movecounter ) + " " + move_mode_pair.first;
        mvwprintz( w, point( 29, 5 ), move_mode_pair.second, move_string );
    }

    // temperature
    std::pair<std::string, nc_color> temp_pair = display::temp_text_color( u );
    mvwprintz( w, point( 21, 6 ), temp_pair.second, temp_pair.first + display::temp_delta_string( u ) );

    // power
    std::pair<std::string, nc_color> power_pair = display::power_text_color( u );
    mvwprintz( w, point( 8, 6 ), c_light_gray, _( "POWER" ) );
    mvwprintz( w, point( 14, 6 ), power_pair.second, power_pair.first );

    // vehicle display
    if( veh ) {
        veh->print_fuel_indicators( w, point( 39, 2 ) );
        mvwprintz( w, point( 35, 4 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        double speed = convert_velocity( veh->velocity, VU_VEHICLE );
        veh->print_speed_gauge( w, point( 21, 5 ), ( -10 < speed && speed < 100 ) ? 1 : 0 );
    }

    wnoutrefresh( w );
}

static void draw_armor_padding( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
    trim_and_print( w, point( heading_length, 0 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "head" ) ) );
    trim_and_print( w, point( heading_length, 1 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "torso" ) ) );
    trim_and_print( w, point( heading_length, 2 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "arm_r" ) ) );
    trim_and_print( w, point( heading_length, 3 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "leg_r" ) ) );
    trim_and_print( w, point( heading_length, 4 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "foot_r" ) ) );
    wnoutrefresh( w );
}

static void draw_armor( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
    trim_and_print( w, point( heading_length, 0 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "head" ) ) );
    trim_and_print( w, point( heading_length, 1 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "torso" ) ) );
    trim_and_print( w, point( heading_length, 2 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "arm_r" ) ) );
    trim_and_print( w, point( heading_length, 3 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "leg_r" ) ) );
    trim_and_print( w, point( heading_length, 4 ), max_length, color,
                    display::colorized_bodypart_outer_armor( u,
                            bodypart_id( "foot_r" ) ) );
    wnoutrefresh( w );
}

static void draw_messages( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 1, 0 /*topline*/, maxlength - 1, line );
    wnoutrefresh( w );
}

static void draw_messages_classic( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    int line = getmaxy( w ) - 2;
    int maxlength = getmaxx( w );
    Messages::display_messages( w, 0, 0 /*topline*/, maxlength, line );
    wnoutrefresh( w );
}

#if defined(TILES)
static void draw_mminimap( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    g->draw_pixel_minimap( w );
    wnoutrefresh( w );
}
#endif

static void draw_compass( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    display::print_mon_info( u, w );
    wnoutrefresh( w );
}

static void draw_compass_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    display::print_mon_info( u, w, 0, true );
    wnoutrefresh( w );
}

static void draw_compass_padding( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    display::print_mon_info( u, w, 1 );
    wnoutrefresh( w );
}

static void draw_compass_padding_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    display::print_mon_info( u, w, 1, true );
    wnoutrefresh( w );
}

static void draw_overmap( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    const tripoint_abs_omt curs = u.global_omt_location();
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    overmap_ui::draw_overmap_chunk( w, u, curs, point_zero, getmaxx( w ) - 1, getmaxy( w ) - 1 );
    wnoutrefresh( w );
}

static void draw_veh_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    vehicle *veh = display::vehicle_driven( u );
    if( veh ) {
        veh->print_fuel_indicators( w, point_zero );
        mvwprintz( w, point( 6, 0 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        veh->print_speed_gauge( w, point( 12, 0 ), 1 );
    }

    wnoutrefresh( w );
}

static void draw_veh_padding( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    vehicle *veh = display::vehicle_driven( u );
    if( veh ) {
        veh->print_fuel_indicators( w, point_east );
        mvwprintz( w, point( 7, 0 ), c_light_gray, veh->face.to_string_azimuth_from_north() );
        veh->print_speed_gauge( w, point( 13, 0 ), 1 );
    }

    wnoutrefresh( w );
}

static void draw_ai_goal( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    behavior::tree needs;
    needs.add( &behavior__node_t_npc_needs.obj() );
    behavior::character_oracle_t player_oracle( &u );
    std::string current_need = needs.tick( &player_oracle );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 0 ), c_light_gray, _( "Goal : %s" ), current_need );
    wnoutrefresh( w );
}

static void draw_location_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    mvwprintz( w, point_zero, c_light_gray, _( "Location:" ) );
    mvwprintz( w, point( 10, 0 ), c_white,
               utf8_truncate( display::current_position_text( u.global_omt_location() ), getmaxx( w ) - 13 ) );

    wnoutrefresh( w );
}

static void draw_weather_classic( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );

    if( get_map().get_abs_sub().z() < 0 ) {
        mvwprintz( w, point_zero, c_light_gray, _( "Underground" ) );
    } else {
        mvwprintz( w, point_zero, c_light_gray, _( "Weather :" ) );
        mvwprintz( w, point( 10, 0 ), get_weather().weather_id->color,
                   get_weather().weather_id->name.translated() );
    }
    mvwprintz( w, point( 31, 0 ), c_light_gray, _( "Moon :" ) );
    nc_color clr = c_white;
    print_colored_text( w, point( 38, 0 ), clr, c_white, display::get_moon_graphic() );

    wnoutrefresh( w );
}

static void draw_lighting_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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

static void draw_weapon_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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

static void draw_time_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    // display date
    mvwprintz( w, point_zero, c_white, display::date_string() );
    // display time
    if( u.has_watch() ) {
        mvwprintz( w, point( 15, 0 ), c_light_gray, to_string_time_of_day( calendar::turn ) );
    } else if( get_map().get_abs_sub().z() >= 0 ) {
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

static void draw_hint( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    std::string press = press_x( ACTION_TOGGLE_PANEL_ADM );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_green, press );
    mvwprintz( w, point( 2 + utf8_width( press ), 0 ), c_white, _( "to open sidebar options" ) );

    wnoutrefresh( w );
}

// Draw weariness info for the avatar in the given window, with (x, y) offsets for its position,
// and the option to use "wide labels" for the wide/classic layouts.
static void draw_weariness_partial( const avatar &u, const catacurses::window &w,
                                    const point start_pos,
                                    const bool wide_labels )
{
    std::pair<std::string, nc_color> weary = display::weariness_text_color( u );
    std::pair<std::string, nc_color> activity = display::activity_text_color( u );

    std::pair<int, int> bar = u.weariness_transition_progress();
    std::pair<std::string, nc_color> weary_bar = get_hp_bar( bar.first, bar.second );

    const std::string progress_label = wide_labels ? translate_marker( "Weary Transition:" ) :
                                       translate_marker( "Transition:" );
    const std::string malus_label = wide_labels ? translate_marker( "Weary Malus: " ) :
                                    translate_marker( "Malus: " );

    const int wplabel_len = utf8_width( _( progress_label ) );
    const int malus_start = getmaxx( w ) - utf8_width( _( malus_label ) ) - 5;

    const std::string malus_str = string_format( "%s%s", _( malus_label ),
                                  display::activity_malus_str( u ) );

    mvwprintz( w, start_pos, c_light_gray, _( progress_label ) );
    mvwprintz( w, point( wplabel_len + 1, start_pos.y ), weary_bar.second, weary_bar.first );
    mvwprintz( w, point( malus_start, start_pos.y ), activity.second, malus_str );
    wnoutrefresh( w );
}

static void draw_weariness( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    draw_weariness_partial( u, w, point_zero, false );
    wnoutrefresh( w );
}

static void draw_weariness_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    draw_weariness_partial( u, w, point_east, false );
    wnoutrefresh( w );
}

static void draw_weariness_wide( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );
    draw_weariness_partial( u, w, point_east, true );
    wnoutrefresh( w );
}

static void draw_weariness_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    werase( w );

    std::pair<std::string, nc_color> weary = display::weariness_text_color( u );
    std::pair<std::string, nc_color> activity = display::activity_text_color( u );

    static const std::string weary_label = translate_marker( "Weariness:" );
    static const std::string activity_label = translate_marker( "Activity:" );

    const int wlabel_len = utf8_width( _( weary_label ) );
    const int alabel_len = utf8_width( _( activity_label ) );
    const int act_start = std::floor( getmaxx( w ) / 2 );

    mvwprintz( w, point_zero, c_light_gray, _( weary_label ) );
    mvwprintz( w, point( wlabel_len + 1, 0 ), weary.second, weary.first );
    mvwprintz( w, point( act_start, 0 ), c_light_gray, _( activity_label ) );
    mvwprintz( w, point( act_start + alabel_len + 1, 0 ), activity.second, activity.first );

    draw_weariness_partial( u, w, point_south, true );

    wnoutrefresh( w );
}

static void print_mana( const Character &you, const catacurses::window &w,
                        const std::string &fmt_string,
                        const int j1, const int j2, const int j3, const int j4 )
{
    werase( w );

    std::pair<std::string, nc_color> mana_pair = display::mana_text_color( you );
    const std::string mana_string = string_format( fmt_string,
                                    //~ translation should not exceed 4 console cells
                                    utf8_justify( _( "Mana" ), j1 ),
                                    colorize( utf8_justify( mana_pair.first, j2 ), mana_pair.second ),
                                    //~ translation should not exceed 9 console cells
                                    utf8_justify( _( "Max Mana" ), j3 ),
                                    colorize( utf8_justify( std::to_string( you.magic->max_mana( you ) ), j4 ), c_light_blue ) );
    nc_color gray = c_light_gray;
    print_colored_text( w, point_zero, gray, gray, mana_string );

    wnoutrefresh( w );
}

static void draw_mana_classic( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    print_mana( u, w, "%s: %s %s: %s", -8, -5, 20, -5 );
}

static void draw_mana_compact( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    print_mana( u, w, "%s %s %s %s", 4, -5, 12, -5 );
}

static void draw_mana_narrow( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

    print_mana( u, w, " %s: %s %s : %s", -5, -5, 9, -5 );
}

static void draw_mana_wide( const draw_args &args )
{
    const avatar &u = args._ava;
    const catacurses::window &w = args._win;

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
    ret.emplace_back( window_panel( draw_compass_padding_compact, "Alt Compass",
                                    to_translation( "Alt Compass" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_overmap, "Overmap", to_translation( "Overmap" ),
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
    ret.emplace_back( window_panel( draw_compass_compact, "Alt Compass",
                                    to_translation( "Alt Compass" ),
                                    5, 32, true ) );
    ret.emplace_back( window_panel( draw_overmap, "Overmap", to_translation( "Overmap" ),
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
    ret.emplace_back( window_panel( draw_compass_padding_compact, "Alt Compass",
                                    to_translation( "Alt Compass" ),
                                    5, 32, false ) );
    ret.emplace_back( window_panel( draw_overmap, "Overmap", to_translation( "Overmap" ),
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
    ret.emplace_back( window_panel( draw_compass_padding_compact, "Alt Compass",
                                    to_translation( "Alt Compass" ),
                                    5, 44, false ) );
    ret.emplace_back( window_panel( draw_overmap, "Overmap", to_translation( "Overmap" ),
                                    20, 44, false ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, 44, true, default_render, true ) );
#endif // TILES
    ret.emplace_back( window_panel( draw_ai_goal, "AI Needs", to_translation( "AI Needs" ),
                                    1, 44, false ) );

    return ret;
}

// Message on how to use the custom sidebar panel and edit its JSON
static void draw_custom_hint( const draw_args &args )
{
    const catacurses::window &w = args._win;

    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_white, _( "Custom sidebar" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_light_gray,
               _( "Edit sidebar.json to adjust." ) );
    mvwprintz( w, point( 1, 2 ), c_light_gray,
               _( "See WIDGETS.md for help." ) );

    wnoutrefresh( w );
}

// Initialize custom panels from a given "sidebar" style widget
static std::vector<window_panel> initialize_default_custom_panels( const widget &wgt )
{
    std::vector<window_panel> ret;

    // Use defined width, or at least 16
    const int width = std::max( wgt._width, 16 );

    // Show hint on configuration
    ret.emplace_back( window_panel( draw_custom_hint, "Hint", to_translation( "Hint" ),
                                    3, width, true ) );

    // Add window panel for each child widget
    for( const widget_id &row_wid : wgt._widgets ) {
        widget row_widget = row_wid.obj();
        ret.emplace_back( row_widget.get_window_panel( width ) );
    }

    // Add compass, message log, and map to fill remaining space
    // TODO: Make these into proper widgets
    ret.emplace_back( window_panel( draw_messages, "Log", to_translation( "Log" ),
                                    -2, width, true ) );
#if defined(TILES)
    ret.emplace_back( window_panel( draw_mminimap, "Map", to_translation( "Map" ),
                                    -1, width, true, default_render, true ) );
#endif // TILES

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

    // Add panel layout for each "sidebar" widget
    for( const widget &wgt : widget::get_all() ) {
        if( wgt._style == "sidebar" ) {
            ret.emplace( wgt.getId().str(),
                         panel_layout( wgt._label, initialize_default_custom_panels( wgt ) ) );
        }
    }

    return ret;
}

panel_manager::panel_manager()
{
    current_layout_id = "labels";
    // Set empty layouts; these will be populated by load()
    layouts = std::map<std::string, panel_layout>();
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
    layouts = initialize_default_panel_layouts();
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
    if( layouts.find( current_layout_id ) == layouts.end() ) {
        // Layout id updated between loads.
        // Shouldn't happen unless custom sidebar id's were modified or removed.
        joLayouts.allow_omitted_members();
        current_layout_id = "labels";
        return;
    }

    for( JsonObject joLayout : joLayouts.get_array( "layouts" ) ) {
        std::string layout_id = joLayout.get_string( "layout_id" );
        const auto &cur_layout = layouts.find( layout_id );
        if( cur_layout == layouts.end() ) {
            joLayout.allow_omitted_members();
            continue;
        }

        auto &layout = cur_layout->second.panels();
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

static void draw_border_win( catacurses::window &w, const std::vector<int> &column_widths,
                             int popup_height )
{
    werase( w );
    decorate_panel( _( "SIDEBAR OPTIONS" ), w );
    // Draw vertical separators
    mvwvline( w, point( column_widths[0] + 1, 1 ), 0, popup_height - 2 );
    mvwvline( w, point( column_widths[0] + column_widths[1] + 2, 1 ), 0, popup_height - 2 );
    wnoutrefresh( w );
}

static void draw_left_win( catacurses::window &w, const std::map<size_t, size_t> &row_indices,
                           const std::vector<window_panel> &panels, size_t source_index, size_t current_row,
                           size_t source_row, bool cur_col, bool swapping, int width, int height, int start )
{
    werase( w );
    for( std::pair<size_t, size_t> row_indx : row_indices ) {
        if( row_indx.first < static_cast<size_t>( start ) ) {
            continue;
        }
        const size_t row = row_indx.first - start;

        const std::string name = panels[row_indx.second].get_name();
        if( swapping && source_index == row_indx.second ) {
            mvwprintz( w, point( 4, current_row - start ), c_yellow, name );
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
            mvwprintz( w, point( 3, row + offset ), toggle_color, name );
        }
    }
    if( cur_col ) {
        mvwprintz( w, point( 0, current_row - start ), c_yellow, ">>" );
    }

    scrollbar()
    .offset_x( width - 1 )
    .offset_y( 0 )
    .content_size( row_indices.size() )
    .viewport_pos( start )
    .viewport_size( height )
    .apply( w );

    wnoutrefresh( w );
}

static void draw_right_win( catacurses::window &w,
                            const std::map<std::string, panel_layout> &layouts,
                            const std::string &current_layout_id,
                            size_t current_row, bool cur_col )
{
    werase( w );
    size_t i = 0;
    for( const auto &layout : layouts ) {
        mvwprintz( w, point( 3, i ), current_layout_id == layout.first ? c_light_blue : c_white,
                   layout.second.name() );
        i++;
    }
    if( cur_col ) {
        mvwprintz( w, point( 0, current_row ), c_yellow, ">>" );
    }
    wnoutrefresh( w );
}

static void draw_center_win( catacurses::window &w, int col_width, const input_context &ctxt )
{
    werase( w );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 0 ), c_light_green, trunc_ellipse( ctxt.get_desc( "TOGGLE_PANEL" ),
               col_width - 1 ) + ":" );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 1, 1 ), c_white, _( "Toggle panels on/off" ) );
    mvwprintz( w, point( 1, 2 ), c_light_green, trunc_ellipse( ctxt.get_desc( "MOVE_PANEL" ),
               col_width - 1 ) + ":" );
    mvwprintz( w, point( 1, 3 ), c_white, _( "Change display order" ) );
    mvwprintz( w, point( 1, 4 ), c_light_green, trunc_ellipse( ctxt.get_desc( "QUIT" ),
               col_width - 1 ) + ":" );
    mvwprintz( w, point( 1, 5 ), c_white, _( "Exit" ) );

    wnoutrefresh( w );
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

    const std::vector<int> column_widths = { 25, 37, 17 };

    size_t current_col = 0;
    size_t current_row = 0;
    bool swapping = false;
    size_t source_row = 0;
    size_t source_index = 0;
    size_t start = 0;

    bool recalc = true;
    bool exit = false;
    // map of row the panel is on vs index
    // panels not renderable due to game configuration will not be in this map
    std::map<size_t, size_t> row_indices;

    g->show_panel_adm = true;
    g->invalidate_main_ui_adaptor();

    catacurses::window w_border;
    catacurses::window w_left;
    catacurses::window w_center;
    catacurses::window w_right;

    const int popup_height = 24;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const point uipos( ( TERMX / 2 ) - 38, ( TERMY / 2 ) - 10 );
        w_border = catacurses::newwin( popup_height, 83, uipos );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        w_left = catacurses::newwin( popup_height - 2, column_widths[0], uipos + point( 1, 1 ) );
        w_center = catacurses::newwin( popup_height - 2, column_widths[1],
                                       uipos + point( 2 + column_widths[0], 1 ) );
        w_right = catacurses::newwin( popup_height - 2, column_widths[2],
                                      uipos + point( 3 + column_widths[0] + column_widths[1], 1 ) );

        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border_win( w_border, column_widths, popup_height );
        draw_right_win( w_right, layouts, current_layout_id, current_row, current_col == 2 );
        auto &panels = get_current_layout().panels();
        draw_left_win( w_left, row_indices, panels, source_index, current_row, source_row, current_col == 0,
                       swapping, column_widths[0], popup_height - 2, start );
        draw_center_win( w_center, column_widths[1], ctxt );
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
        if( current_row < start ) {
            start = current_row > popup_height - 3 ? current_row - ( popup_height - 3 ) : 0;
        }

        ui_manager::redraw();

        const std::string action = ctxt.handle_input();
        if( action == "UP" ) {
            if( current_row > 0 ) {
                current_row -= 1;
                if( current_row < start ) {
                    start = current_row;
                }
            } else {
                current_row = num_rows - 1;
                if( current_row > popup_height - 3 ) {
                    start = current_row - ( popup_height - 3 );
                }
            }
        } else if( action == "DOWN" ) {
            if( current_row + 1 < num_rows ) {
                current_row += 1;
                if( current_row > start + popup_height - 3 ) {
                    start = current_row - ( popup_height - 3 );
                }
            } else {
                current_row = 0;
                start = 0;
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
