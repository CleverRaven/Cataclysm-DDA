#include <string>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#include "keypress.h"
#include "output.h"
#include "catacharset.h"
#include "options.h"
#include "debug.h"
#include "iuse_software_sokoban.h"
#include "globals.h"

sokoban_game::sokoban_game()
{
}

void sokoban_game::print_score(WINDOW *w_sokoban, int iScore, int iMoves)
{
    std::stringstream ssTemp;
    ssTemp << "Level: " << iCurrentLevel+1 << "/" << iNumLevel << "    ";
    mvwprintz(w_sokoban, 1, 3, c_white, ssTemp.str().c_str());

    ssTemp.str("");
    ssTemp << "Score: " << iScore;
    mvwprintz(w_sokoban, 2, 3, c_white, ssTemp.str().c_str());

    ssTemp.str("");
    ssTemp << "Moves: " << iMoves << "    ";
    mvwprintz(w_sokoban, 3, 3, c_white, ssTemp.str().c_str());

    ssTemp.str("");
    ssTemp << "Total moves: " << iTotalMoves;
    mvwprintz(w_sokoban, 4, 3, c_white, ssTemp.str().c_str());

}

bool sokoban_game::parse_level()
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

    std::ifstream fin;
    fin.open(std::string(FILENAMES["sokoban"]).c_str());
    if(!fin.is_open()) {
        fin.close();
        debugmsg("Could not read ./data/raw/sokoban.txt");
        return false;
    }

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if (sLine.substr(0, 3) == "; #") {
            iNumLevel++;
            continue;
        } else if (sLine[0] == ';') {
            continue;
        }

        if (sLine == "") {
            //Find level start
            vLevel.resize(iNumLevel+1);
            vLevelDone.resize(iNumLevel+1);
            mLevelInfo[iNumLevel]["MaxLevelY"] = 0;
            mLevelInfo[iNumLevel]["MaxLevelX"] = 0;
            mLevelInfo[iNumLevel]["PlayerY"] = 0;
            mLevelInfo[iNumLevel]["PlayerX"] = 0;
            continue;
        }

        if (mLevelInfo[iNumLevel]["MaxLevelX"] < sLine.length()) {
            mLevelInfo[iNumLevel]["MaxLevelX"] = sLine.length();
        }

        for (int i=0; i < sLine.length(); i++) {
            if ( sLine[i] == '@' ) {
                if (mLevelInfo[iNumLevel]["PlayerY"] == 0 && mLevelInfo[iNumLevel]["PlayerX"] == 0) {
                    mLevelInfo[iNumLevel]["PlayerY"] = mLevelInfo[iNumLevel]["MaxLevelY"];
                    mLevelInfo[iNumLevel]["PlayerX"] = i;
                } else {
                    //2 @ found error!
                    fin.close();
                    return false;
                }
            }

            if (sLine[i] == '.' || sLine[i] == '*' || sLine[i] == '+') {
                vLevelDone[iNumLevel].push_back(std::make_pair(mLevelInfo[iNumLevel]["MaxLevelY"], i));
            }

            vLevel[iNumLevel][mLevelInfo[iNumLevel]["MaxLevelY"]][i] = sLine[i];
        }

        mLevelInfo[iNumLevel]["MaxLevelY"]++;
    }

    fin.close();

    return true;
}

int sokoban_game::get_wall_connection(const int iY, const int iX)
{
    bool bTop = false;
    bool bRight = false;
    bool bBottom = false;
    bool bLeft = false;

    if (mLevel[iY-1][iX] == "#") {
        bTop = true;
    }

    if (mLevel[iY][iX+1] == "#") {
        bRight = true;
    }

    if (mLevel[iY+1][iX] == "#") {
        bBottom = true;
    }

    if (mLevel[iY][iX-1] == "#") {
        bLeft = true;
    }

    if (!bRight && !bLeft) {
        //#define LINE_XOXO 4194424
        return 4194424;

    } else if (!bTop && !bBottom) {
        //#define LINE_OXOX 4194417
        return 4194417;

    } else if (bTop && bRight && !bBottom && !bLeft) {
        //#define LINE_XXOO 4194413
        return 4194413;

    } else if (!bTop && bRight && bBottom && !bLeft) {
        //#define LINE_OXXO 4194412
        return 4194412;

    } else if (!bTop && !bRight && bBottom && bLeft) {
        //#define LINE_OOXX 4194411
        return 4194411;

    } else if (bTop && !bRight && !bBottom && bLeft) {
        //#define LINE_XOOX 4194410
        return 4194410;

    } else if (bTop && bRight && bBottom && !bLeft) {
        //#define LINE_XXXO 4194420
        return 4194420;

    } else if (bTop && bRight && !bBottom && bLeft) {
        //#define LINE_XXOX 4194422
        return 4194422;

    } else if (bTop && !bRight && bBottom && bLeft) {
        //#define LINE_XOXX 4194421
        return 4194421;

    } else if (!bTop && bRight && bBottom && bLeft) {
        //#define LINE_OXXX 4194423
        return 4194423;

    } else if (bTop && bRight && bBottom && bLeft) {
        //#define LINE_XXXX 4194414
        return 4194414;
    }

    return '#';
}

void sokoban_game::clear_level(WINDOW *w_sokoban)
{
    const int iOffsetX = (FULL_SCREEN_WIDTH-2-mLevelInfo[iCurrentLevel]["MaxLevelX"])/2;
    const int iOffsetY = (FULL_SCREEN_HEIGHT-2-mLevelInfo[iCurrentLevel]["MaxLevelY"])/2;

    for (int iY=0; iY < mLevelInfo[iCurrentLevel]["MaxLevelY"]; iY++) {
        for (int iX=0; iX < mLevelInfo[iCurrentLevel]["MaxLevelX"]; iX++) {
            mvwputch(w_sokoban, iOffsetY+iY, iOffsetX+iX, c_black, ' ');
        }
    }
}

void sokoban_game::draw_level(WINDOW *w_sokoban)
{
    const int iOffsetX = (FULL_SCREEN_WIDTH-2-mLevelInfo[iCurrentLevel]["MaxLevelX"])/2;
    const int iOffsetY = (FULL_SCREEN_HEIGHT-2-mLevelInfo[iCurrentLevel]["MaxLevelY"])/2;

    for (std::map<int, std::map<int, std::string> >::iterator iterY = mLevel.begin(); iterY != mLevel.end(); ++iterY) {
        for (std::map<int, std::string>::iterator iterX = (iterY->second).begin(); iterX != (iterY->second).end(); ++iterX) {
            std::string sTile = iterX->second;

            if (sTile == "#") {
                mvwputch(w_sokoban, iOffsetY+(iterY->first), iOffsetX+(iterX->first), c_white, get_wall_connection(iterY->first, iterX->first));

            } else {
                nc_color cCol = c_white;

                if (sTile == "." || sTile == "*" ||  sTile == "+") {
                    cCol = red_background(c_white);
                }

                if (sTile == ".") {
                    sTile = " ";
                }

                if (sTile == "*") {
                    sTile = "$";
                }

                if (sTile == "+") {
                    sTile = "@";
                }

                mvwprintz(w_sokoban, iOffsetY+(iterY->first), iOffsetX+(iterX->first), cCol, sTile.c_str());
            }
        }
    }
}

bool sokoban_game::check_win()
{
    for (int i=0; i < vLevelDone[iCurrentLevel].size(); i++) {
        if (mLevel[vLevelDone[iCurrentLevel][i].first][vLevelDone[iCurrentLevel][i].second] != "*") {
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

    int iDirY, iDirX;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    WINDOW* w_sokoban = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    draw_border(w_sokoban);

    parse_level();

    mvwprintz(w_sokoban, 0, (FULL_SCREEN_WIDTH/2)-5, hilite(c_white), "Sokoban");

    mvwprintz(w_sokoban, 1, FULL_SCREEN_WIDTH-10, c_ltgreen, "+");
    wprintz(w_sokoban, c_white, " next");

    mvwprintz(w_sokoban, 2, FULL_SCREEN_WIDTH-10, c_ltgreen, "-");
    wprintz(w_sokoban, c_white, " prev");

    mvwprintz(w_sokoban, 3, FULL_SCREEN_WIDTH-10, c_ltgreen,  "r");
    wprintz(w_sokoban, c_white, "eset");

    mvwprintz(w_sokoban, 4, FULL_SCREEN_WIDTH-10, c_ltgreen,  "q");
    wprintz(w_sokoban, c_white, "uit");

    mvwprintz(w_sokoban, 5, FULL_SCREEN_WIDTH-10, c_ltgreen,  "u");
    wprintz(w_sokoban, c_white, "ndo move");

    int input = '.';

    int iPlayerY = 0;
    int iPlayerX = 0;

    bool bNewLevel = true;
    bool bMoved = false;
    do {
        if (bNewLevel) {
            bNewLevel = false;

            iMoves = 0;
            vUndo.clear();

            iPlayerY = mLevelInfo[iCurrentLevel]["PlayerY"];
            iPlayerX = mLevelInfo[iCurrentLevel]["PlayerX"];
            mLevel = vLevel[iCurrentLevel];
        }

        print_score(w_sokoban, iScore, iMoves);

        if (check_win()) {
            //we won yay
            if (!mAlreadyWon[iCurrentLevel]) {
                iScore += 500;
                mAlreadyWon[iCurrentLevel] = true;
            }
            input = '+';

        } else {
            draw_level(w_sokoban);
            wrefresh(w_sokoban);

            //Check input
            input = getch();
        }

        bMoved = false;
        switch (input) {
            case KEY_UP: /* up */
                iDirY = -1;
                iDirX = 0;
                bMoved = true;
                break;
            case KEY_DOWN: /* down */
                iDirY = 1;
                iDirX = 0;
                bMoved = true;
                break;
            case KEY_LEFT: /* left */
                iDirY = 0;
                iDirX = -1;
                bMoved = true;
                break;
            case KEY_RIGHT: /* right */
                iDirY = 0;
                iDirX = 1;
                bMoved = true;
                break;
            case 'q':
                return iScore;
                break;
            case 'u':
                {
                    int iPlayerYNew = 0;
                    int iPlayerXNew = 0;
                    bool bUndoSkip = false;
                    //undo move
                    if (vUndo.size() > 0) {
                        //reset last player pos
                        mLevel[iPlayerY][iPlayerX] = (mLevel[iPlayerY][iPlayerX] == "+") ? "." : " ";
                        iPlayerYNew = vUndo[vUndo.size()-1].iOldY;
                        iPlayerXNew = vUndo[vUndo.size()-1].iOldX;
                        mLevel[iPlayerYNew][iPlayerXNew] = vUndo[vUndo.size()-1].sTileOld;

                        vUndo.pop_back();

                        bUndoSkip = true;
                    }

                    if (bUndoSkip && vUndo.size() > 0) {
                        iDirY = vUndo[vUndo.size()-1].iOldY;
                        iDirX = vUndo[vUndo.size()-1].iOldX;

                        if (vUndo[vUndo.size()-1].sTileOld == "$" || vUndo[vUndo.size()-1].sTileOld == "*") {
                            mLevel[iPlayerY][iPlayerX] = (mLevel[iPlayerY][iPlayerX] == ".") ? "*" : "$";
                            mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = (mLevel[iPlayerY + iDirY][iPlayerX + iDirX] == "*") ? "." : " ";

                            vUndo.pop_back();
                        }
                    }

                    if (bUndoSkip) {
                        iPlayerY = iPlayerYNew;
                        iPlayerX = iPlayerXNew;
                    }
                }
                break;
            case 'r':
                //reset level
                bNewLevel = true;
                break;
            case '+':
                //next level
                clear_level(w_sokoban);
                iCurrentLevel++;
                if (iCurrentLevel >= iNumLevel) {
                    iCurrentLevel = 0;
                }
                bNewLevel = true;
                break;
            case '-':
                //prev level
                clear_level(w_sokoban);
                iCurrentLevel--;
                if (iCurrentLevel < 0) {
                    iCurrentLevel =  iNumLevel - 1;
                }
                bNewLevel = true;
                break;
            default:
                break;
        }

        if (bMoved) {
            //check if we can move the player
            std::string sMoveTo = mLevel[iPlayerY + iDirY][iPlayerX + iDirX];
            bool bMovePlayer = false;

            if (sMoveTo != "#") {
                if (sMoveTo == "$" || sMoveTo == "*") {
                    //Check if we can move the package
                    std::string sMovePackTo = mLevel[iPlayerY + (iDirY * 2)][iPlayerX + (iDirX * 2)];
                    if (sMovePackTo == "." || sMovePackTo == " ") {
                        //move both
                        bMovePlayer = true;
                        mLevel[iPlayerY + (iDirY * 2)][iPlayerX + (iDirX * 2)] = (sMovePackTo == ".") ? "*" : "$";

                        vUndo.push_back(cUndo(iDirY, iDirX, sMoveTo));

                        iMoves--;
                    }
                } else {
                    bMovePlayer = true;
                }

                if (bMovePlayer) {
                    //move player
                    vUndo.push_back(cUndo(iPlayerY, iPlayerX, mLevel[iPlayerY][iPlayerX]));

                    mLevel[iPlayerY][iPlayerX] = (mLevel[iPlayerY][iPlayerX] == "+") ? "." : " ";
                    mLevel[iPlayerY + iDirY][iPlayerX + iDirX] = (sMoveTo == "." || sMoveTo == "*") ? "+" : "@";

                    iPlayerY += iDirY;
                    iPlayerX += iDirX;

                    iMoves++;
                    iTotalMoves++;
                }
            }
        }

    } while (true);

    return iScore;
}
