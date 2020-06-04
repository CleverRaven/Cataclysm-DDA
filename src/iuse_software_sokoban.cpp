#include "iuse_software_sokoban.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>

#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "optional.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "translations.h"
#include "ui_manager.h"

sokoban_game::sokoban_game() = default;

void sokoban_game::print_score( const catacurses::window &w_sokoban, int iScore, int iMoves )
{
    mvwprintz( w_sokoban, point( 3, 1 ), c_white, _( "Level: %d/%d" ), iCurrentLevel + 1, iNumLevel );
    wprintw( w_sokoban, "    " );

    mvwprintz( w_sokoban, point( 3, 2 ), c_white, _( "Score: %d" ), iScore );

    mvwprintz( w_sokoban, point( 3, 3 ), c_white, _( "Moves: %d" ), iMoves );
    wprintw( w_sokoban, "    " );

    mvwprintz( w_sokoban, point( 3, 4 ), c_white, _( "Total moves: %d" ), iTotalMoves );
}

void sokoban_game::parse_level( std::istream &fin )
{
    /*
    # Wall
    $ Package
    space Floor
    . Goal
    * Package on Goal
    @ Sokoban
    + Sokoban on Goal
    */

    iCurrentLevel = 0;
    iNumLevel = 0;

    vLevel.clear();
    vUndo.clear();
    vLevelDone.clear();

    std::string sLine;
    while( !fin.eof() ) {
        safe_getline( fin, sLine );

        if( sLine.substr( 0, 3 ) == "; #" ) {
            iNumLevel++;
            continue;
        } else if( sLine[0] == ';' ) {
            continue;
        }

        if( sLine.empty() ) {
            //Find level start
            vLevel.resize( iNumLevel + 1 );
            vLevelDone.resize( iNumLevel + 1 );
            mLevelInfo[iNumLevel]["MaxLevelY"] = 0;
            mLevelInfo[iNumLevel]["MaxLevelX"] = 0;
            mLevelInfo[iNumLevel]["PlayerY"] = 0;
            mLevelInfo[iNumLevel]["PlayerX"] = 0;
            continue;
        }

        if( mLevelInfo[iNumLevel]["MaxLevelX"] < sLine.length() ) {
            mLevelInfo[iNumLevel]["MaxLevelX"] = sLine.length();
        }

        for( size_t i = 0; i < sLine.length(); i++ ) {
            if( sLine[i] == '@' ) {
                if( mLevelInfo[iNumLevel]["PlayerY"] == 0 && mLevelInfo[iNumLevel]["PlayerX"] == 0 ) {
                    mLevelInfo[iNumLevel]["PlayerY"] = mLevelInfo[iNumLevel]["MaxLevelY"];
                    mLevelInfo[iNumLevel]["PlayerX"] = i;
                } else {
                    // TODO: describe why it's invalid
                    throw std::runtime_error( "invalid content of sokoban file" );
                }
            }

            if( sLine[i] == '.' || sLine[i] == '*' || sLine[i] == '+' ) {
                vLevelDone[iNumLevel].push_back( std::make_pair( static_cast<int>
                                                 ( mLevelInfo[iNumLevel]["MaxLevelY"] ), static_cast<int>( i ) ) );
            }

            vLevel[iNumLevel][mLevelInfo[iNumLevel]["MaxLevelY"]][i] = sLine[i];
        }

        mLevelInfo[iNumLevel]["MaxLevelY"]++;
    }
}

int sokoban_game::get_wall_connection( const point &i )
{
    bool bTop = false;
    bool bRight = false;
    bool bBottom = false;
    bool bLeft = false;

    if( mLevel[i.y - 1][i.x] == "#" ) {
        bTop = true;
    }

    if( mLevel[i.y][i.x + 1] == "#" ) {
        bRight = true;
    }

    if( mLevel[i.y + 1][i.x] == "#" ) {
        bBottom = true;
    }

    if( mLevel[i.y][i.x - 1] == "#" ) {
        bLeft = true;
    }

    if( !bRight && !bLeft ) {
        return LINE_XOXO; //

    } else if( !bTop && !bBottom ) {
        return LINE_OXOX;

    } else if( bTop && bRight && !bBottom && !bLeft ) {
        return LINE_XXOO;

    } else if( !bTop && bRight && bBottom && !bLeft ) {
        return LINE_OXXO;

    } else if( !bTop && !bRight && bBottom && bLeft ) {
        return LINE_OOXX;

    } else if( bTop && !bRight && !bBottom && bLeft ) {
        return LINE_XOOX;

    } else if( bTop && bRight && bBottom && !bLeft ) {
        return LINE_XXXO;

    } else if( bTop && bRight && !bBottom && bLeft ) {
        return LINE_XXOX;

    } else if( bTop && !bRight && bBottom && bLeft ) {
        return LINE_XOXX;

    } else if( !bTop && bRight && bBottom && bLeft ) {
        return LINE_OXXX;

    } else if( bTop && bRight && bBottom && bLeft ) {
        return LINE_XXXX;
    }

    return '#';
}

void sokoban_game::draw_level( const catacurses::window &w_sokoban )
{
    const int iOffsetX = ( FULL_SCREEN_WIDTH - 2 - mLevelInfo[iCurrentLevel]["MaxLevelX"] ) / 2;
    const int iOffsetY = ( FULL_SCREEN_HEIGHT - 2 - mLevelInfo[iCurrentLevel]["MaxLevelY"] ) / 2;

    for( auto &elem : mLevel ) {
        for( std::map<int, std::string>::iterator iterX = elem.second.begin();
             iterX != elem.second.end(); ++iterX ) {
            std::string sTile = iterX->second;

            if( sTile == "#" ) {
                mvwputch( w_sokoban, point( iOffsetX + iterX->first, iOffsetY + elem.first ),
                          c_white, get_wall_connection( point( iterX->first, elem.first ) ) );

            } else {
                nc_color cCol = c_white;

                if( sTile == "." || sTile == "*" ||  sTile == "+" ) {
                    cCol = red_background( c_white );
                }

                if( sTile == "." ) {
                    sTile = " ";
                }

                if( sTile == "*" ) {
                    sTile = "$";
                }

                if( sTile == "+" ) {
                    sTile = "@";
                }

                mvwprintz( w_sokoban, point( iOffsetX + iterX->first, iOffsetY + elem.first ), cCol, sTile );
            }
        }
    }
}

bool sokoban_game::check_win()
{
    for( auto &elem : vLevelDone[iCurrentLevel] ) {
        if( mLevel[elem.first][elem.second] != "*" ) {
            return false;
        }
    }
    return true;
}

int sokoban_game::start_game()
{
    int iScore = 0;
    int iMoves = 0;
    iTotalMoves = 0;

    int iDirY = 0;
    int iDirX = 0;

    using namespace std::placeholders;
    read_from_file( PATH_INFO::sokoban(), std::bind( &sokoban_game::parse_level, this, _1 ) );

    catacurses::window w_sokoban;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ) {
        const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        const int iOffsetY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
        w_sokoban = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                        point( iOffsetX, iOffsetY ) );
        ui.position_from_window( w_sokoban );
    } );
    ui.mark_resize();

    input_context ctxt( "SOKOBAN" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEXT" );
    ctxt.register_action( "PREV" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UNDO" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_sokoban );
        draw_border( w_sokoban, BORDER_COLOR, _( "Sokoban" ), hilite( c_white ) );

        std::vector<std::string> shortcuts;
        shortcuts.push_back( _( "<+> next" ) ); // '+': next
        shortcuts.push_back( _( "<-> prev" ) ); // '-': prev
        shortcuts.push_back( _( "<r>eset" ) ); // 'r': reset
        shortcuts.push_back( _( "<q>uit" ) );  // 'q': quit
        shortcuts.push_back( _( "<u>ndo move" ) ); // 'u': undo move

        int indent = 10;
        for( auto &shortcut : shortcuts ) {
            indent = std::max( indent, utf8_width( shortcut ) + 1 );
        }
        indent = std::min( indent, 30 );

        for( size_t i = 0; i < shortcuts.size(); i++ ) {
            shortcut_print( w_sokoban, point( FULL_SCREEN_WIDTH - indent, i + 1 ),
                            c_white, c_light_green, shortcuts[i] );
        }

        print_score( w_sokoban, iScore, iMoves );

        draw_level( w_sokoban );
        wrefresh( w_sokoban );
    } );

    int iPlayerY = 0;
    int iPlayerX = 0;

    bool bNewLevel = true;
    bool bMoved = false;
    do {
        if( bNewLevel ) {
            bNewLevel = false;

            iMoves = 0;
            vUndo.clear();

            iPlayerY = mLevelInfo[iCurrentLevel]["PlayerY"];
            iPlayerX = mLevelInfo[iCurrentLevel]["PlayerX"];
            mLevel = vLevel[iCurrentLevel];
        }

        std::string action;
        if( check_win() ) {
            //we won yay
            if( !mAlreadyWon[iCurrentLevel] ) {
                iScore += 500;
                mAlreadyWon[iCurrentLevel] = true;
            }
            action = "NEXT";

        } else {
            ui_manager::redraw();
            //Check input
            action = ctxt.handle_input();
        }

        bMoved = false;
        if( const cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            iDirX = vec->x;
            iDirY = vec->y;
            bMoved = true;
        } else if( action == "QUIT" ) {
            return iScore;
        } else if( action == "UNDO" ) {
            int iPlayerYNew = 0;
            int iPlayerXNew = 0;
            bool bUndoSkip = false;
            //undo move
            if( !vUndo.empty() ) {
                //reset last player pos
                mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "+" ? "." : " ";
                iPlayerYNew = vUndo[vUndo.size() - 1].old.y;
                iPlayerXNew = vUndo[vUndo.size() - 1].old.x;
                mLevel[iPlayerYNew][iPlayerXNew] = vUndo[vUndo.size() - 1].sTileOld;

                vUndo.pop_back();

                bUndoSkip = true;
            }

            if( bUndoSkip && !vUndo.empty() ) {
                iDirY = vUndo[vUndo.size() - 1].old.y;
                iDirX = vUndo[vUndo.size() - 1].old.x;

                if( vUndo[vUndo.size() - 1].sTileOld == "$" ||
                    vUndo[vUndo.size() - 1].sTileOld == "*" ) {
                    mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "." ? "*" : "$";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = mLevel[iPlayerY + iDirY][iPlayerX + iDirX] == "*" ?
                            "." : " ";

                    vUndo.pop_back();
                }
            }

            if( bUndoSkip ) {
                iPlayerY = iPlayerYNew;
                iPlayerX = iPlayerXNew;
            }
        } else if( action == "RESET" ) {
            //reset level
            bNewLevel = true;
        } else if( action == "NEXT" ) {
            //next level
            iCurrentLevel++;
            if( iCurrentLevel >= iNumLevel ) {
                iCurrentLevel = 0;
            }
            bNewLevel = true;
        } else if( action == "PREV" ) {
            //previous level
            iCurrentLevel--;
            if( iCurrentLevel < 0 ) {
                iCurrentLevel = iNumLevel - 1;
            }
            bNewLevel = true;
        }

        if( bMoved ) {
            //check if we can move the player
            std::string sMoveTo = mLevel[iPlayerY + iDirY][iPlayerX + iDirX];
            bool bMovePlayer = false;

            if( sMoveTo != "#" ) {
                if( sMoveTo == "$" || sMoveTo == "*" ) {
                    //Check if we can move the package
                    std::string sMovePackTo = mLevel[iPlayerY + iDirY * 2][iPlayerX + iDirX * 2];
                    if( sMovePackTo == "." || sMovePackTo == " " ) {
                        //move both
                        bMovePlayer = true;
                        mLevel[iPlayerY + iDirY * 2][iPlayerX + iDirX * 2] = sMovePackTo == "." ? "*" : "$";

                        vUndo.push_back( cUndo( iDirY, iDirX, sMoveTo ) );

                        iMoves--;
                    }
                } else {
                    bMovePlayer = true;
                }

                if( bMovePlayer ) {
                    //move player
                    vUndo.push_back( cUndo( iPlayerY, iPlayerX, mLevel[iPlayerY][iPlayerX] ) );

                    mLevel[iPlayerY][iPlayerX] = mLevel[iPlayerY][iPlayerX] == "+" ? "." : " ";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = sMoveTo == "." || sMoveTo == "*" ? "+" : "@";

                    iPlayerY += iDirY;
                    iPlayerX += iDirX;

                    iMoves++;
                    iTotalMoves++;
                }
            }
        }

    } while( true );

    return iScore;
}
