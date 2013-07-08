#include "game.h"
#include "auto_pickup.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <algorithm>

std::vector<cPickupRules> vAutoPickupRules[3];

void game::show_auto_pickup()
{
    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT-2-iHeaderHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[50] = true;
    mapLines[54] = true;
    //mapLines[61] = true;
    //mapLines[68] = true;

    const int iTotalCols = mapLines.size()-1;

    WINDOW* w_auto_pickup_options = newwin(FULL_SCREEN_HEIGHT/2, FULL_SCREEN_WIDTH/2, iOffsetY + (FULL_SCREEN_HEIGHT/2)/2, iOffsetX + (FULL_SCREEN_WIDTH/2)/2);

    WINDOW* w_auto_pickup_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW* w_auto_pickup_header = newwin(iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW* w_auto_pickup = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY, 1 + iOffsetX);

    wborder(w_auto_pickup_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwputch(w_auto_pickup_border, 3,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_auto_pickup_border, 3, 79, c_ltgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_auto_pickup_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, c_ltgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_auto_pickup_border, 0, 29, c_ltred, " AUTO PICKUP MANAGER ");
    wrefresh(w_auto_pickup_border);

    mvwprintz(w_auto_pickup_header, 0, 0, c_ltgreen, " A");
    wprintz(w_auto_pickup_header, c_white, "dd  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "R");
    wprintz(w_auto_pickup_header, c_white, "emove  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "C");
    wprintz(w_auto_pickup_header, c_white, "opy  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "M");
    wprintz(w_auto_pickup_header, c_white, "ove  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "E");
    wprintz(w_auto_pickup_header, c_white, "nable  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "D");
    wprintz(w_auto_pickup_header, c_white, "isable  ");

    mvwprintz(w_auto_pickup_header, 1, 0, c_ltgreen, " +");
    wprintz(w_auto_pickup_header, c_white, "/");
    wprintz(w_auto_pickup_header, c_ltgreen, "-");
    wprintz(w_auto_pickup_header, c_white, " Move up/down  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "Enter");
    wprintz(w_auto_pickup_header, c_white, "-Edit  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "Tab");
    wprintz(w_auto_pickup_header, c_white, "-Switch Page  ");


    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXXX);
            mvwputch(w_auto_pickup_header, 3, i, c_ltgray, LINE_XOXO);
        } else {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXOX); // Draw line under header
        }
    }

    mvwprintz(w_auto_pickup_header, 3, 0, c_white, "#");
    mvwprintz(w_auto_pickup_header, 3, 7, c_white, "Rules");
    mvwprintz(w_auto_pickup_header, 3, 51, c_white, "I/E");
    //mvwprintz(w_auto_pickup_header, 3, 55, c_white, "Volume");
    //mvwprintz(w_auto_pickup_header, 3, 62, c_white, "Weight");

    wrefresh(w_auto_pickup_header);

    int iCurrentPage = 1;
    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    bool bStuffChanged = false;
    char ch = ' ';

    std::stringstream sTemp;

    do {
        mvwprintz(w_auto_pickup_header, 2, 12 + 0, c_white, "[");
        wprintz(w_auto_pickup_header, (iCurrentPage == 1) ? hilite(c_white) : c_white, "Global");
        wprintz(w_auto_pickup_header, c_white, "]");

        mvwprintz(w_auto_pickup_header, 2, 12 + 9, c_white, "[");
        wprintz(w_auto_pickup_header, (iCurrentPage == 2) ? hilite(c_white) : c_white, "Charakter");
        wprintz(w_auto_pickup_header, c_white, "]");

        /*mvwprintz(w_auto_pickup_header, 2, 12 + 21, c_white, "[");
        wprintz(w_auto_pickup_header, (iCurrentPage == 3) ? hilite(c_white) : c_white, "Options");
        wprintz(w_auto_pickup_header, c_white, "]");*/

        wrefresh(w_auto_pickup_header);

        // Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_auto_pickup, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w_auto_pickup, i, j, c_black, ' ');
                }
            }
        }

        if (iCurrentPage == 1 || iCurrentPage == 2) {
            if (iCurrentPage == 2 && g->u.name == "") {
                vAutoPickupRules[2].clear();
                mvwprintz(w_auto_pickup, 8, 15, c_white, "Please load a character first to use this page!");
            }

            if (vAutoPickupRules[iCurrentPage].size() > iContentHeight) {
                iStartPos = iCurrentLine - (iContentHeight - 1) / 2;

                if (iStartPos < 0) {
                    iStartPos = 0;
                } else if (iStartPos + iContentHeight > vAutoPickupRules[iCurrentPage].size()) {
                    iStartPos = vAutoPickupRules[iCurrentPage].size() - iContentHeight;
                }
            }

            // display auto pickup
            for (int i = iStartPos; i < vAutoPickupRules[iCurrentPage].size(); i++) {
                if (i >= iStartPos && i < iStartPos + ((iContentHeight > vAutoPickupRules[iCurrentPage].size()) ? vAutoPickupRules[iCurrentPage].size() : iContentHeight)) {
                    nc_color cLineColor = (vAutoPickupRules[iCurrentPage][i].bActive) ? c_white : c_ltgray;

                    sTemp.str("");
                    sTemp << i + 1;
                    mvwprintz(w_auto_pickup, i - iStartPos, 0, cLineColor, sTemp.str().c_str());
                    mvwprintz(w_auto_pickup, i - iStartPos, 4, cLineColor, "");

                    if (iCurrentLine == i) {
                        wprintz(w_auto_pickup, c_yellow, ">> ");
                    } else {
                        wprintz(w_auto_pickup, c_yellow, "   ");
                    }

                    wprintz(w_auto_pickup, (iCurrentLine == i && iCurrentCol == 1) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[iCurrentPage][i].sRule == "") ? "<empty rule>" : vAutoPickupRules[iCurrentPage][i].sRule).c_str());

                    mvwprintz(w_auto_pickup, i - iStartPos, 52, (iCurrentLine == i && iCurrentCol == 2) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[iCurrentPage][i].bExclude) ? "E" : "I"));
                }
            }

            wrefresh(w_auto_pickup);

        } else if (iCurrentPage == 3) {
            wborder(w_auto_pickup_options, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
            wrefresh(w_auto_pickup);
            wrefresh(w_auto_pickup_options);
        }

        ch = input();

        if (iCurrentPage == 3) {
            switch(ch) {
                case '\t': //Switch to next Page
                    iCurrentPage++;
                    if (iCurrentPage > 3) {
                        iCurrentPage = 1;
                        iCurrentLine = 0;
                    }
                    break;
            }
        } else if (iCurrentPage == 1 || iCurrentPage == 2) {
            if (iCurrentPage == 2 && g->u.name == "" && ch != '\t') {
                //Only allow loaded games to use the char sheet
            } else if (vAutoPickupRules[iCurrentPage].size() > 0 || ch == 'a' || ch == '\t') {
                switch(ch) {
                    case 'j': //move down
                        iCurrentLine++;
                        iCurrentCol = 1;
                        if (iCurrentLine >= vAutoPickupRules[iCurrentPage].size()) {
                            iCurrentLine = 0;
                        }
                        break;
                    case 'k': //move up
                        iCurrentLine--;
                        iCurrentCol = 1;
                        if (iCurrentLine < 0) {
                            iCurrentLine = vAutoPickupRules[iCurrentPage].size()-1;
                        }
                        break;
                    case 'a': //add new rule
                    case 'A':
                        bStuffChanged = true;
                        vAutoPickupRules[iCurrentPage].push_back(cPickupRules("", true, false));
                        iCurrentLine = vAutoPickupRules[iCurrentPage].size()-1;
                        break;
                    case 'r': //remove rule
                    case 'R':
                        bStuffChanged = true;
                        vAutoPickupRules[iCurrentPage].erase(vAutoPickupRules[iCurrentPage].begin() + iCurrentLine);
                        if (iCurrentLine > vAutoPickupRules[iCurrentPage].size()-1) {
                            iCurrentLine--;
                        }
                        break;
                    case 'c': //copy rule
                    case 'C':
                        bStuffChanged = true;
                        vAutoPickupRules[iCurrentPage].push_back(cPickupRules(vAutoPickupRules[iCurrentPage][iCurrentLine].sRule, vAutoPickupRules[iCurrentPage][iCurrentLine].bActive, vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude));
                        iCurrentLine = vAutoPickupRules[iCurrentPage].size()-1;
                        break;
                    case 'm': //move rule global <-> character
                    case 'M':
                        if ((iCurrentPage == 1 && g->u.name != "") || iCurrentPage == 2) {
                            bStuffChanged = true;
                            //copy over
                            vAutoPickupRules[(iCurrentPage == 1) ? 2 : 1].push_back(cPickupRules(vAutoPickupRules[iCurrentPage][iCurrentLine].sRule, vAutoPickupRules[iCurrentPage][iCurrentLine].bActive, vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude));

                            //remove old
                            vAutoPickupRules[iCurrentPage].erase(vAutoPickupRules[iCurrentPage].begin() + iCurrentLine);
                            iCurrentLine = vAutoPickupRules[(iCurrentPage == 1) ? 2 : 1].size()-1;
                            iCurrentPage = (iCurrentPage == 1) ? 2 : 1;
                        }
                        break;
                    case '\t': //Switch to next Page
                        iCurrentPage++;
                        if (iCurrentPage > 2) {
                            iCurrentPage = 1;
                            iCurrentLine = 0;
                        }
                        break;
                    case '\n': //Edit Col in current line
                        bStuffChanged = true;
                        if (iCurrentCol == 1) {
                            vAutoPickupRules[iCurrentPage][iCurrentLine].sRule = string_input_popup("Pickup Rule:", 30, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule);

                        } else if (iCurrentCol == 2) {
                            vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude = !vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude;
                        }
                        break;
                    case 'e': //enable rule
                    case 'E':
                        bStuffChanged = true;
                        vAutoPickupRules[iCurrentPage][iCurrentLine].bActive = true;
                        break;
                    case 'd': //disable rule
                    case 'D':
                        bStuffChanged = true;
                        vAutoPickupRules[iCurrentPage][iCurrentLine].bActive = false;
                        break;
                    case 'h': //move left
                        iCurrentCol--;
                        if (iCurrentCol < 1) {
                            iCurrentCol = iTotalCols;
                        }
                        break;
                    case 'l': //move right
                        iCurrentCol++;
                        if (iCurrentCol > iTotalCols) {
                            iCurrentCol = 1;
                        }
                        break;
                    case '+': //move rule up
                        bStuffChanged = true;
                        if (iCurrentLine < vAutoPickupRules[iCurrentPage].size()-1) {
                            std::swap(vAutoPickupRules[iCurrentPage][iCurrentLine], vAutoPickupRules[iCurrentPage][iCurrentLine+1]);
                            iCurrentLine++;
                            iCurrentCol = 1;
                        }
                        break;
                    case '-': //move rule down
                        bStuffChanged = true;
                        if (iCurrentLine > 0) {
                            std::swap(vAutoPickupRules[iCurrentPage][iCurrentLine], vAutoPickupRules[iCurrentPage][iCurrentLine-1]);
                            iCurrentLine--;
                            iCurrentCol = 1;
                        }
                        break;
                }
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if (bStuffChanged) {
        if(query_yn("Save changes?")) {
            save_auto_pickup();
        } else {
            load_auto_pickup();
        }
    }

    werase(w_auto_pickup);
    werase(w_auto_pickup_border);
}

void load_auto_pickup(bool bLoadCharacter)
{
    /*
    if (g->u.name == "") {
        debugmsg("No character loaded.");
    } else {
        debugmsg(("Character " + g->u.name + " loaded.").c_str());
    }
    */

    std::ifstream fin;
    std::string sFile = "data/auto_pickup.txt";

    if (bLoadCharacter) {
        sFile = "save/" + g->u.name + ".apu.txt";
    }

    fin.open(sFile.c_str());
    if(!fin.is_open()) {
        fin.close();
        create_default_auto_pickup();
        fin.open(sFile.c_str());
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create " << sFile << "\n";
            return;
        }
    }

    vAutoPickupRules[(bLoadCharacter) ? 2 : 1].clear();

    while(!fin.eof()) {
        std::string sLine;
        fin >> sLine;

        if(sLine == "") { // Empty line, chomp it
            getline(fin, sLine);

        } else if(sLine[0] == '#') { // # indicates a comment
            getline(fin, sLine);

        } else {
            int iNum = std::count(sLine.begin(), sLine.end(), ';');

            if(iNum != 2) {
                /*int iNum = std::count(sLine.begin(), sLine.end(), ' ');

                if(iNum == 1) { //its an option! hurray

                } else {*/
                    DebugLog() << "Bad Rule: " << sLine << "\n";
                    getline(fin, sLine);
                //}

            } else {
                std::string sRule = "";
                bool bActive = true;
                bool bExclude = false;

                int iPos = 0;
                int iCol = 1;
                do {
                    iPos = sLine.find(";");

                    std::string sTemp = (iPos == std::string::npos) ? sLine : sLine.substr(0, iPos);

                    if (iCol == 1) {
                        sRule = sTemp;

                    } else if (iCol == 2) {
                        bActive = (sTemp == "T" || sTemp == "True") ? true : false;

                    } else if (iCol == 3) {
                        bExclude = (sTemp == "T" || sTemp == "True") ? true : false;
                    }

                    iCol++;

                    if (iPos != std::string::npos) {
                        sLine = sLine.substr(iPos+1, sLine.size());
                    }

                } while(iPos != std::string::npos);

                vAutoPickupRules[(bLoadCharacter) ? 2 : 1].push_back(cPickupRules(sRule, bActive, bExclude));
            }
        }

        if(fin.peek() == '\n') {
            getline(fin, sLine);    // Chomp
        }
    }

    fin.close();

    merge_vector();
}

void merge_vector()
{
    vAutoPickupRules[0].clear();

    for (int i=1; i <= 2; i++) {
        for (int j=0; j < vAutoPickupRules[i].size(); j++) {
            vAutoPickupRules[0].push_back(cPickupRules(vAutoPickupRules[i][j].sRule, vAutoPickupRules[i][j].bActive, vAutoPickupRules[i][j].bExclude));
        }
    }
}

std::string auto_pickup_header()
{
    return "# This is the auto pickup rules file. The format is\n\
# <pickup rule>;<dis/enabled>;<in/exclude>\n\n\
# <pickup rule> Simple text. No other special characters except spaces and *\n\
# <dis/enabled> Can be T(rue) of F(alse)\n\
# <in/exclude> Can be T(rue) of F(alse)\n\
#\n\
# If # is at the start of a line, it is considered a comment and is ignored.\n\
# In-line commenting is not allowed. I think.\n\
#\n\
# If you want to restore the default auto pickup rules file, simply delete this file.\n\
# A new auto_pickup.txt will be created next time you play.\
\n\n";
}

void save_auto_pickup()
{
    std::ofstream fout;
    fout.open("data/auto_pickup.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << auto_pickup_header() << std::endl;
    for(int i = 0; i < vAutoPickupRules[1].size(); i++) {
        fout << vAutoPickupRules[1][i].sRule << ";";
        fout << (vAutoPickupRules[1][i].bActive ? "T" : "F") << ";";
        fout << (vAutoPickupRules[1][i].bExclude ? "T" : "F");
        fout << "\n";
    }

    merge_vector();
}

void create_default_auto_pickup()
{
    std::ofstream fout;
    fout.open("data/auto_pickup.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << auto_pickup_header();
    fout.close();
}
