#include "iuse_software_lightson.h"

#include <algorithm>
#include <string>
#include <vector>
#include <sstream>

#include "cursesdef.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
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
    level_size = std::make_pair( lvl_height, lvl_width );
    level.resize( lvl_height * lvl_width );

    const int steps_rng = half_perimeter / 2.0 + rng_float( 0, 2 );
    generate_change_coords( steps_rng );

    reset_level();
}

void lightson_game::reset_level()
{
    std::fill( level.begin(), level.end(), true );

    // change level
    std::for_each( change_coords.begin(), change_coords.end(), [this]( std::pair<int, int> &p ) {
        this->position = std::make_pair( p.first, p.second );
        toggle_lights();
    } );

    position = std::make_pair( 0, 0 );

    werase( w );
    draw_border( w );
}

void lightson_game::draw_level()
{
    for( int i = 0; i < level_size.first; i++ ) {
        for( int j = 0; j < level_size.second; j++ ) {
            bool selected = position.first == i && position.second == j;
            bool on = level[i * level_size.second + j];
            const nc_color fg = on ? c_white : c_dark_gray;
            const char symbol = on ? '#' : '-';
            mvwputch( w, i + 1, j + 1, selected ? hilite( c_white ) : fg, symbol );
        }
    }
    wrefresh( w );
}

void lightson_game::generate_change_coords( int changes )
{
    change_coords.resize( changes );
    const int size = level_size.first * level_size.second;

    std::pair< int, int > candidate;
    for( int k = 0; k < changes; k++ ) {
        do {
            const int candidate_index = rng( 0, size - 1 );

            const int col = candidate_index % level_size.second;
            const int row = ( candidate_index - col ) / level_size.second;
            candidate = std::make_pair( row, col );
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
    const int row = position.first;
    const int col = position.second;
    const int height = level_size.first;
    const int width = level_size.second;

    if( row > 0 ) {
        level[( row - 1 ) * width + col] = !level[( row - 1 ) * width + col]; // N
    }
    level[row * width + col] = !level[row * width + col]; // x
    if( row < height - 1 ) {
        level[( row + 1 ) * width + col] = !level[( row + 1 ) * width + col]; // S
    }

    if( col > 0 ) {
        level[row * width + ( col - 1 )] = !level[row * width + ( col - 1 )]; // W
    }
    if( col < width - 1 ) {
        level[row * width + ( col + 1 )] = !level[row * width + ( col + 1 )]; // E
    }
}

int lightson_game::start_game()
{
    const int w_height = 15;
    const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iOffsetY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
    w_border = catacurses::newwin( w_height, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX );
    w = catacurses::newwin( w_height - 6, FULL_SCREEN_WIDTH - 2, iOffsetY + 1, iOffsetX + 1 );
    draw_border( w_border );

    input_context ctxt( "LIGHTSON" );
    ctxt.register_directions();
    ctxt.register_action( "TOGGLE_SPACE" );
    ctxt.register_action( "TOGGLE_5" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );

    std::vector<std::string> shortcuts;
    shortcuts.push_back( _( "<spacebar or 5> toggle lights" ) ); // '5': toggle
    shortcuts.push_back( _( "<r>eset" ) );                       // 'r': reset
    shortcuts.push_back( _( "<q>uit" ) );                        // 'q': quit

    int iWidth = 0;
    for( auto &shortcut : shortcuts ) {
        if( iWidth > 0 ) {
            iWidth += 1;
        }
        iWidth += utf8_width( shortcut );
    }

    int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
    for( auto &shortcut : shortcuts ) {
        shortcut_print( w_border, 0, iPos, c_white, c_light_green, shortcut );
        iPos += utf8_width( shortcut ) + 1;
    }

    mvwputch( w_border, 0, 2, hilite( c_white ), _( "Lights on!" ) );
    std::ostringstream str;
    str << _( "<color_white>Game goal:</color> Switch all the lights on." ) << '\n' <<
        _( "<color_white>Legend: #</color> on, <color_dark_gray>-</color> off." ) << '\n' <<
        _( "Toggle lights switches selected light and 4 its neighbors." );
    fold_and_print( w_border, w_height - 5, 2, FULL_SCREEN_WIDTH - 4, c_light_gray, str.str() );

    wrefresh( w_border );

    win = true;
    int hasWon = 0;

    do {
        if( win ) {
            new_level();
            draw_level();
        }
        std::string action = ctxt.handle_input();
        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            position.first = std::min( std::max( position.first + vec->y, 0 ), level_size.first - 1 );
            position.second = std::min( std::max( position.second + vec->x, 0 ), level_size.second - 1 );
        } else if( action == "TOGGLE_SPACE" || action == "TOGGLE_5" ) {
            toggle_lights();
            win = check_win();
            if( win ) {
                draw_level();
                popup_top( _( "Congratulations, you won!" ) );
                hasWon++;
            }
        } else if( action == "RESET" ) {
            reset_level();
        } else if( action == "QUIT" ) {
            break;
        }
        draw_level();
    } while( true );

    return hasWon;
}
