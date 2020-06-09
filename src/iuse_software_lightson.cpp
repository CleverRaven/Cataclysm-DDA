#include "iuse_software_lightson.h"

#include <algorithm>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include "ui_manager.h"
#include "catacharset.h"
#include "color.h"
#include "optional.h"
#include "point.h"

void lightson_game::new_level()
{
    win = false;

    // level generation
    const int half_perimeter = rng( 8, 11 );
    const int lvl_width = rng( 4, 6 );
    const int lvl_height = half_perimeter - lvl_width;
    level_size = point( lvl_width, lvl_height );
    level.resize( lvl_height * lvl_width );

    const int steps_rng = half_perimeter / 2.0 + rng_float( 0.0, 2.0 );
    generate_change_coords( steps_rng );

    reset_level();
}

void lightson_game::reset_level()
{
    std::fill( level.begin(), level.end(), true );

    // change level
    std::for_each( change_coords.begin(), change_coords.end(), [this]( point & p ) {
        toggle_lights_at( p );
    } );

    position = point_zero;
}

bool lightson_game::get_value_at( const point &pt )
{
    return level[pt.y * level_size.x + pt.x];
}

void lightson_game::set_value_at( const point &pt, bool value )
{
    level[pt.y * level_size.x + pt.x] = value;
}

void lightson_game::toggle_value_at( const point &pt )
{
    set_value_at( pt, !get_value_at( pt ) );
}

void lightson_game::draw_level()
{
    werase( w );
    draw_border( w );
    for( int i = 0; i < level_size.y; i++ ) {
        for( int j = 0; j < level_size.x; j++ ) {
            point current = point( j, i );
            bool selected = position == current;
            bool on = get_value_at( current );
            const nc_color fg = on ? c_white : c_dark_gray;
            const char symbol = on ? '#' : '-';
            mvwputch( w, current + point_south_east, selected ? hilite( c_white ) : fg, symbol );
        }
    }
    wnoutrefresh( w );
}

void lightson_game::generate_change_coords( int changes )
{
    change_coords.resize( changes );
    const int size = level_size.y * level_size.x;

    point candidate;
    for( int k = 0; k < changes; k++ ) {
        do {
            const int candidate_index = rng( 0, size - 1 );

            candidate.x = candidate_index % level_size.x;
            candidate.y = ( candidate_index - candidate.x ) / level_size.x;
            // not accept repeatable coordinates
        } while( !( k == 0 ||
                    std::find( change_coords.begin(), change_coords.end(), candidate ) == change_coords.end() ) );
        change_coords[k] = candidate;
    }
}

bool lightson_game::check_win()
{
    return std::all_of( level.begin(), level.end(), []( bool i ) {
        return i;
    } );
}

void lightson_game::toggle_lights()
{
    toggle_lights_at( position );
}

void lightson_game::toggle_lights_at( const point &pt )
{
    toggle_value_at( pt );

    if( pt.y > 0 ) {
        toggle_value_at( pt + point_north );
    }
    if( pt.y < level_size.y - 1 ) {
        toggle_value_at( pt + point_south );
    }

    if( pt.x > 0 ) {
        toggle_value_at( pt + point_west );
    }
    if( pt.x < level_size.x - 1 ) {
        toggle_value_at( pt + point_east );
    }
}

int lightson_game::start_game()
{
    const int w_height = 15;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        const int iOffsetY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
        w_border = catacurses::newwin( w_height, FULL_SCREEN_WIDTH, point( iOffsetX, iOffsetY ) );
        w = catacurses::newwin( w_height - 6, FULL_SCREEN_WIDTH - 2, point( iOffsetX + 1, iOffsetY + 1 ) );
        ui.position_from_window( w_border );
    } );
    ui.mark_resize();

    input_context ctxt( "LIGHTSON" );
    ctxt.register_directions();
    ctxt.register_action( "TOGGLE_SPACE" );
    ctxt.register_action( "TOGGLE_5" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        std::vector<std::string> shortcuts;
        shortcuts.push_back( _( "<spacebar or 5> toggle lights" ) );
        shortcuts.push_back( _( "<r>eset" ) );
        shortcuts.push_back( _( "<q>uit" ) );

        int iWidth = 0;
        for( auto &shortcut : shortcuts ) {
            if( iWidth > 0 ) {
                iWidth += 1;
            }
            iWidth += utf8_width( shortcut );
        }

        werase( w_border );
        draw_border( w_border );
        int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
        for( auto &shortcut : shortcuts ) {
            shortcut_print( w_border, point( iPos, 0 ), c_white, c_light_green, shortcut );
            iPos += utf8_width( shortcut ) + 1;
        }

        mvwputch( w_border, point( 2, 0 ), hilite( c_white ), _( "Lights on!" ) );
        fold_and_print( w_border, point( 2, w_height - 5 ), FULL_SCREEN_WIDTH - 4, c_light_gray,
                        "%s\n%s\n%s", _( "<color_white>Game goal:</color> Switch all the lights on." ),
                        _( "<color_white>Legend: #</color> on, <color_dark_gray>-</color> off." ),
                        _( "Toggle lights switches selected light and 4 its neighbors." ) );

        wnoutrefresh( w_border );

        draw_level();
    } );

    win = true;
    int hasWon = 0;

    do {
        if( win ) {
            new_level();
        }
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            position.y = clamp( position.y + vec->y, 0, level_size.y - 1 );
            position.x = clamp( position.x + vec->x, 0, level_size.x - 1 );
        } else if( action == "TOGGLE_SPACE" || action == "TOGGLE_5" ) {
            toggle_lights();
            win = check_win();
            if( win ) {
                ui.invalidate_ui();
                popup_top( _( "Congratulations, you won!" ) );
                hasWon++;
            }
        } else if( action == "RESET" ) {
            reset_level();
        } else if( action == "QUIT" ) {
            break;
        }
    } while( true );

    return hasWon;
}
