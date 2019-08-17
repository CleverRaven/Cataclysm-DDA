#include "iuse_software_minesweeper.h"

#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <functional>

#include "catacharset.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "color.h"
#include "compatibility.h"
#include "cursesdef.h"
#include "optional.h"
#include "point.h"

std::vector<tripoint> closest_tripoints_first( int radius, const tripoint &p );

minesweeper_game::minesweeper_game()
{
    iMinY = 8;
    iMinX = 8;

    iMaxY = 0;
    iMaxX = 0;

    iOffsetX = 0;
    iOffsetY = 0;
}

void minesweeper_game::new_level( const catacurses::window &w_minesweeper )
{
    iMaxY = getmaxy( w_minesweeper ) - 2;
    iMaxX = getmaxx( w_minesweeper ) - 2;

    werase( w_minesweeper );

    mLevel.clear();
    mLevelReveal.clear();

    auto set_num = [&]( const std::string & sType, int &iVal, const int iMin, const int iMax ) {
        std::ostringstream ssTemp;
        ssTemp << _( "Min:" ) << iMin << " " << _( "Max:" ) << " " << iMax;

        do {
            if( iVal < iMin || iVal > iMax ) {
                iVal = iMin;
            }

            string_input_popup()
            .title( sType )
            .width( 5 )
            .description( ssTemp.str() )
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
            iLevelY = 8;
            iLevelX = 8;
            iBombs = 10;
            break;
        case 1:
            iLevelY = 16;
            iLevelX = 16;
            iBombs = 40;
            break;
        case 2:
            iLevelY = 16;
            iLevelX = 30;
            iBombs = 99;
            break;
        case 3:
        default:
            iLevelY = iMinY;
            iLevelX = iMinX;

            set_num( _( "Level width:" ), iLevelX, iMinX, iMaxX );
            set_num( _( "Level height:" ), iLevelY, iMinY, iMaxY );

            iBombs = iLevelX * iLevelY * 0.1;

            set_num( _( "Number of bombs:" ), iBombs, iBombs, iLevelX * iLevelY * 0.6 );
            break;
    }

    iOffsetX = ( iMaxX - iLevelX ) / 2 + 1;
    iOffsetY = ( iMaxY - iLevelY ) / 2 + 1;

    int iRandX;
    int iRandY;
    for( int i = 0; i < iBombs; i++ ) {
        do {
            iRandX = rng( 0, iLevelX - 1 );
            iRandY = rng( 0, iLevelY - 1 );
        } while( mLevel[iRandY][iRandX] == static_cast<int>( bomb ) );

        mLevel[iRandY][iRandX] = static_cast<int>( bomb );
    }

    for( int y = 0; y < iLevelY; y++ ) {
        for( int x = 0; x < iLevelX; x++ ) {
            if( mLevel[y][x] == static_cast<int>( bomb ) ) {
                const auto circle = closest_tripoints_first( 1, {x, y, 0} );

                for( const auto &p : circle ) {
                    if( p.x >= 0 && p.x < iLevelX && p.y >= 0 && p.y < iLevelY ) {
                        if( mLevel[p.y][p.x] != static_cast<int>( bomb ) ) {
                            mLevel[p.y][p.x]++;
                        }
                    }
                }
            }
        }
    }

    for( int y = 0; y < iLevelY; y++ ) {
        mvwputch( w_minesweeper, iOffsetY + y, iOffsetX, c_white, std::string( iLevelX, '#' ) );
    }

    mvwputch( w_minesweeper, iOffsetY, iOffsetX, hilite( c_white ), "#" );

    draw_custom_border( w_minesweeper, 1, 1, 1, 1, 1, 1, 1, 1,
                        BORDER_COLOR, iOffsetY - 1, iLevelY + 2, iOffsetX - 1, iLevelX + 2 );
}

bool minesweeper_game::check_win()
{
    for( int y = 0; y < iLevelY; y++ ) {
        for( int x = 0; x < iLevelX; x++ ) {
            if( ( mLevelReveal[y][x] == flag || mLevelReveal[y][x] == unknown ) &&
                mLevel[y][x] != static_cast<int>( bomb ) ) {
                return false;
            }
        }
    }

    return true;
}

int minesweeper_game::start_game()
{
    const int iCenterX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iCenterY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    catacurses::window w_minesweeper_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
            point( iCenterX, iCenterY ) );
    catacurses::window w_minesweeper = catacurses::newwin( FULL_SCREEN_HEIGHT - 2,
                                       FULL_SCREEN_WIDTH - 2, point( iCenterX + 1, iCenterY + 1 ) );

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
        shortcut_print( w_minesweeper_border, 0, iPos, c_white, c_light_green, shortcut );
        iPos += utf8_width( shortcut ) + 1;
    }

    mvwputch( w_minesweeper_border, 0, 2, hilite( c_white ), _( "Minesweeper" ) );

    wrefresh( w_minesweeper_border );

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

    std::function<void ( int, int )> rec_reveal = [&]( const int y, const int x ) {
        if( mLevelReveal[y][x] == unknown || mLevelReveal[y][x] == flag ) {
            mLevelReveal[y][x] = seen;

            if( mLevel[y][x] == 0 ) {
                const auto circle = closest_tripoints_first( 1, {x, y, 0} );

                for( const auto &p : circle ) {
                    if( p.x >= 0 && p.x < iLevelX && p.y >= 0 && p.y < iLevelY ) {
                        if( mLevelReveal[p.y][p.x] != seen ) {
                            rec_reveal( p.y, p.x );
                        }
                    }
                }

                mvwputch( w_minesweeper, iOffsetY + y, iOffsetX + x, c_black, " " );

            } else {
                mvwputch( w_minesweeper, iOffsetY + y, iOffsetX + x,
                          x == iPlayerX && y == iPlayerY ? hilite( aColors[mLevel[y][x]] ) : aColors[mLevel[y][x]],
                          to_string( mLevel[y][x] ) );
            }
        }
    };

    std::string action = "NEW";

    do {
        if( action == "NEW" ) {
            new_level( w_minesweeper );

            iPlayerY = 0;
            iPlayerX = 0;
        }

        wrefresh( w_minesweeper );

        if( check_win() ) {
            popup_top( _( "Congratulations, you won!" ) );

            iScore = 30;

            action = "QUIT";
        } else {
            action = ctxt.handle_input();
        }

        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            if( iPlayerX + vec->x >= 0 && iPlayerX + vec->x < iLevelX && iPlayerY + vec->y >= 0 &&
                iPlayerY + vec->y < iLevelY ) {

                std::string sGlyph;

                for( int i = 0; i < 2; i++ ) {
                    nc_color cColor = c_white;
                    if( mLevelReveal[iPlayerY][iPlayerX] == flag ) {
                        sGlyph = "!";
                        cColor = c_yellow;
                    } else if( mLevelReveal[iPlayerY][iPlayerX] == seen ) {
                        if( mLevel[iPlayerY][iPlayerX] == 0 ) {
                            sGlyph = " ";
                            cColor = c_black;
                        } else {
                            sGlyph = to_string( mLevel[iPlayerY][iPlayerX] );
                            cColor = aColors[mLevel[iPlayerY][iPlayerX]];
                        }
                    } else {
                        sGlyph = '#';
                    }

                    mvwputch( w_minesweeper, iOffsetY + iPlayerY, iOffsetX + iPlayerX,
                              i == 0 ? cColor : hilite( cColor ), sGlyph );

                    if( i == 0 ) {
                        iPlayerX += vec->x;
                        iPlayerY += vec->y;
                    }
                }
            }
        } else if( action == "FLAG" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                mLevelReveal[iPlayerY][iPlayerX] = flag;
                mvwputch( w_minesweeper, iOffsetY + iPlayerY, iOffsetX + iPlayerX, hilite( c_yellow ), "!" );

            } else if( mLevelReveal[iPlayerY][iPlayerX] == flag ) {
                mLevelReveal[iPlayerY][iPlayerX] = unknown;
                mvwputch( w_minesweeper, iOffsetY + iPlayerY, iOffsetX + iPlayerX, hilite( c_white ), "#" );
            }
        } else if( action == "CONFIRM" ) {
            if( mLevelReveal[iPlayerY][iPlayerX] != seen ) {
                if( mLevel[iPlayerY][iPlayerX] == static_cast<int>( bomb ) ) {
                    for( int y = 0; y < iLevelY; y++ ) {
                        for( int x = 0; x < iLevelX; x++ ) {
                            if( mLevel[y][x] == static_cast<int>( bomb ) ) {
                                mvwputch( w_minesweeper, iOffsetY + y, iOffsetX + x, hilite( c_red ),
                                          mLevelReveal[y][x] == flag ? "!" : "*" );
                            }
                        }
                    }

                    wrefresh( w_minesweeper );

                    popup_top( _( "Boom, you're dead! Better luck next time." ) );
                    action = "QUIT";

                } else if( mLevelReveal[iPlayerY][iPlayerX] == unknown ) {
                    rec_reveal( iPlayerY, iPlayerX );
                }
            }
        }

    } while( action != "QUIT" );

    return iScore;
}

