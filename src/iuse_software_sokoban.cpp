#include "iuse_software_sokoban.h"

#include "output.h"
#include "input.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "string_formatter.h"
#include "debug.h"
#include "path_info.h"
#include "translations.h"
#include "cata_utility.h"

#include <iostream>
#include <iterator>
#include <sstream>

sokoban_game::sokoban_game()
{
}

void sokoban_game::print_score( const catacurses::window &w_sokoban, int iScore, int iMoves )
{
    std::stringstream ssTemp;
    ssTemp << string_format( _( "Level: %d/%d" ), iCurrentLevel + 1, iNumLevel ) << "    ";
    mvwprintz( w_sokoban, 1, 3, c_white, ssTemp.str().c_str() );

    ssTemp.str( "" );
    ssTemp << string_format( _( "Score: %d" ), iScore );
    mvwprintz( w_sokoban, 2, 3, c_white, ssTemp.str().c_str() );

    ssTemp.str( "" );
    ssTemp << string_format( _( "Moves: %d" ), iMoves ) << "    ";
    mvwprintz( w_sokoban, 3, 3, c_white, ssTemp.str().c_str() );

    ssTemp.str( "" );
    ssTemp << string_format( _( "Total moves: %d" ), iTotalMoves );
    mvwprintz( w_sokoban, 4, 3, c_white, ssTemp.str().c_str() );

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

int sokoban_game::get_wall_connection( const int iY, const int iX )
{
    bool bTop = false;
    bool bRight = false;
    bool bBottom = false;
    bool bLeft = false;

    if( mLevel[iY - 1][iX] == "#" ) {
        bTop = true;
    }

    if( mLevel[iY][iX + 1] == "#" ) {
        bRight = true;
    }

    if( mLevel[iY + 1][iX] == "#" ) {
        bBottom = true;
    }

    if( mLevel[iY][iX - 1] == "#" ) {
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

void sokoban_game::clear_level( const catacurses::window &w_sokoban )
{
    const int iOffsetX = ( FULL_SCREEN_WIDTH - 2 - mLevelInfo[iCurrentLevel]["MaxLevelX"] ) / 2;
    const int iOffsetY = ( FULL_SCREEN_HEIGHT - 2 - mLevelInfo[iCurrentLevel]["MaxLevelY"] ) / 2;

    for( size_t iY = 0; iY < mLevelInfo[iCurrentLevel]["MaxLevelY"]; iY++ ) {
        for( size_t iX = 0; iX < mLevelInfo[iCurrentLevel]["MaxLevelX"]; iX++ ) {
            mvwputch( w_sokoban, iOffsetY + iY, iOffsetX + iX, c_black, ' ' );
        }
    }
}

void sokoban_game::draw_level( const catacurses::window &w_sokoban )
{
    const int iOffsetX = ( FULL_SCREEN_WIDTH - 2 - mLevelInfo[iCurrentLevel]["MaxLevelX"] ) / 2;
    const int iOffsetY = ( FULL_SCREEN_HEIGHT - 2 - mLevelInfo[iCurrentLevel]["MaxLevelY"] ) / 2;

    for( auto &elem : mLevel ) {
        for( std::map<int, std::string>::iterator iterX = ( elem.second ).begin();
             iterX != ( elem.second ).end(); ++iterX ) {
            std::string sTile = iterX->second;

            if( sTile == "#" ) {
                mvwputch( w_sokoban, iOffsetY + ( elem.first ), iOffsetX + ( iterX->first ),
                          c_white, get_wall_connection( elem.first, iterX->first ) );

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

                mvwprintz( w_sokoban, iOffsetY + ( elem.first ), iOffsetX + ( iterX->first ), cCol,
                           sTile.c_str() );
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

    const int iOffsetX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    using namespace std::placeholders;
    read_from_file( FILENAMES["sokoban"], std::bind( &sokoban_game::parse_level, this, _1 ) );

    catacurses::window w_sokoban = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY,
                                   iOffsetX );
    draw_border( w_sokoban, BORDER_COLOR, _( "Sokoban" ), hilite( c_white ) );
    input_context ctxt( "SOKOBAN" );
    ctxt.register_cardinal();
    ctxt.register_action( "NEXT" );
    ctxt.register_action( "PREV" );
    ctxt.register_action( "RESET" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "UNDO" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

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
        shortcut_print( w_sokoban, i + 1, FULL_SCREEN_WIDTH - indent,
                        c_white, c_light_green, shortcuts[i] );
    }

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

        print_score( w_sokoban, iScore, iMoves );

        std::string action;
        if( check_win() ) {
            //we won yay
            if( !mAlreadyWon[iCurrentLevel] ) {
                iScore += 500;
                mAlreadyWon[iCurrentLevel] = true;
            }
            action = "NEXT";

        } else {
            draw_level( w_sokoban );
            wrefresh( w_sokoban );

            //Check input
            action = ctxt.handle_input();
        }

        bMoved = false;
        if( ctxt.get_direction( iDirX, iDirY, action ) ) {
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
                mLevel[iPlayerY][iPlayerX] = ( mLevel[iPlayerY][iPlayerX] == "+" ) ? "." : " ";
                iPlayerYNew = vUndo[vUndo.size() - 1].iOldY;
                iPlayerXNew = vUndo[vUndo.size() - 1].iOldX;
                mLevel[iPlayerYNew][iPlayerXNew] = vUndo[vUndo.size() - 1].sTileOld;

                vUndo.pop_back();

                bUndoSkip = true;
            }

            if( bUndoSkip && !vUndo.empty() ) {
                iDirY = vUndo[vUndo.size() - 1].iOldY;
                iDirX = vUndo[vUndo.size() - 1].iOldX;

                if( vUndo[vUndo.size() - 1].sTileOld == "$" ||
                    vUndo[vUndo.size() - 1].sTileOld == "*" ) {
                    mLevel[iPlayerY][iPlayerX] = ( mLevel[iPlayerY][iPlayerX] == "." ) ? "*" : "$";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = ( mLevel[iPlayerY + iDirY][iPlayerX + iDirX] == "*" ) ?
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
            clear_level( w_sokoban );
            iCurrentLevel++;
            if( iCurrentLevel >= iNumLevel ) {
                iCurrentLevel = 0;
            }
            bNewLevel = true;
        } else if( action == "PREV" ) {
            //previous level
            clear_level( w_sokoban );
            iCurrentLevel--;
            if( iCurrentLevel < 0 ) {
                iCurrentLevel =  iNumLevel - 1;
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
                    std::string sMovePackTo = mLevel[iPlayerY + ( iDirY * 2 )][iPlayerX + ( iDirX * 2 )];
                    if( sMovePackTo == "." || sMovePackTo == " " ) {
                        //move both
                        bMovePlayer = true;
                        mLevel[iPlayerY + ( iDirY * 2 )][iPlayerX + ( iDirX * 2 )] = ( sMovePackTo == "." ) ? "*" : "$";

                        vUndo.push_back( cUndo( iDirY, iDirX, sMoveTo ) );

                        iMoves--;
                    }
                } else {
                    bMovePlayer = true;
                }

                if( bMovePlayer ) {
                    //move player
                    vUndo.push_back( cUndo( iPlayerY, iPlayerX, mLevel[iPlayerY][iPlayerX] ) );

                    mLevel[iPlayerY][iPlayerX] = ( mLevel[iPlayerY][iPlayerX] == "+" ) ? "." : " ";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = ( sMoveTo == "." || sMoveTo == "*" ) ? "+" : "@";

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
