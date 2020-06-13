#include "iuse_software_minesweeper.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <vector>

#include "catacharset.h"
#include "color.h"
#include "compatibility.h"
#include "cursesdef.h"
#include "input.h"
#include "optional.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

minesweeper_game::minesweeper_game()
{
    min = point( 8, 8 );
    max = point_zero;
    offset = point_zero;
}

void minesweeper_game::new_level()
{
    auto set_num = [&]( const std::string & sType, int &iVal, const int iMin, const int iMax ) {
        const std::string desc = string_format( _( "Min: %d Max: %d" ), iMin, iMax );

        do {
            if( iVal < iMin || iVal > iMax ) {
                iVal = iMin;
            }

            string_input_popup()
            .title( sType )
            .width( 5 )
            .description( desc )
            .edit( iVal );
        } while( iVal < iMin || iVal > iMax );
    };

    uilist difficulty;
    difficulty.allow_cancel = false;
    difficulty.text = _( "Game Difficulty" );
    difficulty.entries.emplace_back( 0, true, 'b', _( "Beginner" ) );
    difficulty.entries.emplace_back( 1, true, 'i', _( "Intermediate" ) );
    difficulty.entries.emplace_back( 2, true, 'e', _( "Expert" ) );
    difficulty.entries.emplace_back( 3, true, 'c', _( "Custom" ) );
    difficulty.query();

    switch( difficulty.ret ) {
        case 0:
            level = point( 8, 8 );
            iBombs = 10;
            break;
        case 1:
            level = point( 16, 16 );
            iBombs = 40;
            break;
        case 2:
            level = point( 30, 16 );
            iBombs = 99;
            break;
        case 3:
        default:
            point new_level = min;

            set_num( _( "Level width:" ), new_level.x, min.x, max.x );
            set_num( _( "Level height:" ), new_level.y, min.y, max.y );

            const int area = new_level.x * new_level.y;
            int new_ibombs = area * 0.1;

            set_num( _( "Number of bombs:" ), new_ibombs, new_ibombs, area * 0.6 );

            level = new_level;
            iBombs = new_ibombs;
            break;
    }

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    offset = ( max - level ) / 2 + point( 1, 1 );

    mLevel.clear();
    mLevelReveal.clear();

    int iRandX;
    int iRandY;
    for( int i = 0; i < iBombs; i++ ) {
        do {
            iRandX = rng( 0, level.x - 1 );
            iRandY = rng( 0, level.y - 1 );
        } while( mLevel[iRandY][iRandX] == bomb );

        mLevel[iRandY][iRandX] = bomb;
    }

    for( int y = 0; y < level.y; y++ ) {
        for( int x = 0; x < level.x; x++ ) {
            if( mLevel[y][x] == bomb ) {
                for( const point &p : closest_points_first( {x, y}, 1 ) ) {
                    if( p.x >= 0 && p.x < level.x && p.y >= 0 && p.y < level.y ) {
                        if( mLevel[p.y][p.x] != bomb ) {
                            mLevel[p.y][p.x]++;
                        }
                    }
                }
            }
        }
    }
}

bool minesweeper_game::check_win()
{
    for( int y = 0; y < level.y; y++ ) {
        for( int x = 0; x < level.x; x++ ) {
            if( ( mLevelReveal[y][x] == flag || mLevelReveal[y][x] == unknown ) &&
                mLevel[y][x] != bomb ) {
                return false;
            }
        }
    }

    return true;
}

int minesweeper_game::start_game()
{
    catacurses::window w_minesweeper_border;
    catacurses::window w_minesweeper;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int iCenterX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        const int iCenterY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

        w_minesweeper_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                               point( iCenterX, iCenterY ) );
        w_minesweeper = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                            point( iCenterX + 1, iCenterY + 1 ) );
        max = point( FULL_SCREEN_WIDTH - 4, FULL_SCREEN_HEIGHT - 4 );
        ui.position_from_window( w_minesweeper_border );
    } );
    ui.mark_resize();

    input_context ctxt( "MINESWEEPER" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEW" );
    ctxt.register_action( "FLAG" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    static const std::array<nc_color, 9> aColors = {{
            c_white,
            c_light_gray,
            c_cyan,
            c_blue,
            c_light_blue,
            c_green,
            c_magenta,
            c_red,
            c_yellow
        }
    };

    int iScore = 5;

    int iPlayerY = 0;
    int iPlayerX = 0;

    bool started = false;
    bool boom = false;
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_minesweeper_border );
        draw_border( w_minesweeper_border );

        std::vector<std::string> shortcuts;
        shortcuts.push_back( _( "<n>ew level" ) );
        shortcuts.push_back( _( "<f>lag" ) );
        shortcuts.push_back( _( "<q>uit" ) );

        int iWidth = 0;
        for( auto &shortcut : shortcuts ) {
            if( iWidth > 0 ) {
                iWidth += 1;
            }
            iWidth += utf8_width( shortcut );
        }

        int iPos = FULL_SCREEN_WIDTH - iWidth - 1;
        for( auto &shortcut : shortcuts ) {
            shortcut_print( w_minesweeper_border, point( iPos, 0 ), c_white, c_light_green, shortcut );
            iPos += utf8_width( shortcut ) + 1;
        }

        mvwputch( w_minesweeper_border, point( 2, 0 ), hilite( c_white ), _( "Minesweeper" ) );

        wnoutrefresh( w_minesweeper_border );

        if( !started ) {
            return;
        }

        werase( w_minesweeper );
        draw_custom_border( w_minesweeper, 1, 1, 1, 1, 1, 1, 1, 1, BORDER_COLOR,
                            // NOLINTNEXTLINE(cata-use-named-point-constants)
                            offset + point( -1, -1 ), level.y + 2, level.x + 2 );
        for( int y = 0; y < level.y; ++y ) {
            for( int x = 0; x < level.x; ++x ) {
                char ch = '?';
                nc_color col = c_red;
                const int num = mLevel[y][x];
                switch( mLevelReveal[y][x] ) {
                    case unknown:
                        if( boom && num == bomb ) {
                            ch = '*';
                            col = h_red;
                        } else {
                            ch = '#';
                            col = c_white;
                        }
                        break;
                    case flag:
                        ch = '!';
                        if( boom && num == bomb ) {
                            col = h_red;
                        } else {
                            col = c_yellow;
                        }
                        break;
                    case seen: {
                        if( num == bomb ) {
                            ch = '*';
                            col = boom ? h_red : c_red;
                        } else if( num == 0 ) {
                            ch = ' ';
                            col = aColors[num];
                        } else if( num >= 1 && num <= 8 ) {
                            ch = '0' + num;
                            col = aColors[num];
                        }
                        break;
                    }
                }
                if( !boom && iPlayerX == x && iPlayerY == y ) {
                    col = hilite( col );
                }
                mvwputch( w_minesweeper, offset + point( x, y ), col, ch );
            }
        }
        wnoutrefresh( w_minesweeper );
    } );

    std::function<void ( const point & )> rec_reveal = [&]( const point & p ) {
        if( mLevelReveal[p.y][p.x] == unknown || mLevelReveal[p.y][p.x] == flag ) {
            mLevelReveal[p.y][p.x] = seen;

            if( mLevel[p.y][p.x] == 0 ) {
                for( const point &near_p : closest_points_first( p, 1 ) ) {
                    if( near_p.x >= 0 && near_p.x < level.x && near_p.y >= 0 && near_p.y < level.y ) {
                        if( mLevelReveal[near_p.y][near_p.x] != seen ) {
                            rec_reveal( near_p );
                        }
                    }
                }
            }
        }
    };

    const auto &reveal_all = [&]() {
        for( int y = 0; y < level.y; ++y ) {
            for( int x = 0; x < level.x; ++x ) {
                if( mLevelReveal[y][x] == unknown || ( mLevelReveal[y][x] == flag && mLevel[y][x] != bomb ) ) {
                    mLevelReveal[y][x] = seen;
                }
            }
        }
    };

    std::string action = "NEW";

    do {
        if( action == "NEW" ) {
            new_level();

            iPlayerY = 0;
            iPlayerX = 0;

            started = true;
            boom = false;
        }

        if( check_win() ) {
            reveal_all();
            ui.invalidate_ui();
            popup_top( _( "Congratulations, you won!" ) );

            iScore = 30;

            action = "QUIT";
        } else {
            ui_manager::redraw();
            action = ctxt.handle_input();
        }

        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            const int new_x = iPlayerX + vec->x;
            const int new_y = iPlayerY + vec->y;
            if( new_x >= 0 && new_x < level.x && new_y >= 0 && new_y < level.y ) {
                iPlayerX = new_x;
                iPlayerY = new_y;
            }
        } else if( action == "FLAG" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                mLevelReveal[iPlayerY][iPlayerX] = flag;
            } else if( mLevelReveal[iPlayerY][iPlayerX] == flag ) {
                mLevelReveal[iPlayerY][iPlayerX] = unknown;
            }
        } else if( action == "CONFIRM" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] != seen ) {
                if( mLevel[iPlayerY][iPlayerX] == bomb ) {
                    boom = true;
                    reveal_all();
                    ui.invalidate_ui();
                    popup_top( _( "Boom, you're dead!  Better luck next time." ) );
                    action = "QUIT";
                } else if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                    rec_reveal( point( iPlayerY, iPlayerX ) );
                }
            }
        }
    } while( action != "QUIT" );

    return iScore;
}

