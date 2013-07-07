#include "game.h"
#include "auto_pickup.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <algorithm>

std::vector<cPickupRules> vAutoPickupRules;

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

    const int iTotalTabs = mapLines.size()-1;

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
    wprintz(w_auto_pickup_header, c_white, "dd Rule     ");

    wprintz(w_auto_pickup_header, c_ltgreen, "E");
    wprintz(w_auto_pickup_header, c_white, "nable Rule   ");

    wprintz(w_auto_pickup_header, c_ltgreen, "+");
    wprintz(w_auto_pickup_header, c_white, " Move Rule up    ");

    mvwprintz(w_auto_pickup_header, 1, 0, c_ltgreen, " R");
    wprintz(w_auto_pickup_header, c_white, "emove Rule  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "D");
    wprintz(w_auto_pickup_header, c_white, "isable Rule  ");

    wprintz(w_auto_pickup_header, c_ltgreen, "-");
    wprintz(w_auto_pickup_header, c_white, " Move Rule down  ");

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

    int iCurrentLine = 0;
    int iCurrentTab = 1;
    int iStartPos = 0;
    char ch = ' ';

    std::stringstream sTemp;

    do {
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

        if (vAutoPickupRules.size() > iContentHeight) {
            iStartPos = iCurrentLine - (iContentHeight - 1) / 2;

            if (iStartPos < 0) {
                iStartPos = 0;
            } else if (iStartPos + iContentHeight > vAutoPickupRules.size()) {
                iStartPos = vAutoPickupRules.size() - iContentHeight;
            }
        }

        // display auto pickup
        for (int i = iStartPos; i < vAutoPickupRules.size(); i++) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > vAutoPickupRules.size()) ? vAutoPickupRules.size() : iContentHeight)) {
                nc_color cLineColor = (vAutoPickupRules[i].bActive) ? c_white : c_ltgray;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz(w_auto_pickup, i - iStartPos, 0, cLineColor, sTemp.str().c_str());
                mvwprintz(w_auto_pickup, i - iStartPos, 4, cLineColor, "");

                if (iCurrentLine == i) {
                    wprintz(w_auto_pickup, c_yellow, ">> ");
                } else {
                    wprintz(w_auto_pickup, c_yellow, "   ");
                }

                wprintz(w_auto_pickup, (iCurrentLine == i && iCurrentTab == 1) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[i].sRule == "") ? "<empty rule>" : vAutoPickupRules[i].sRule).c_str());

                mvwprintz(w_auto_pickup, i - iStartPos, 52, (iCurrentLine == i && iCurrentTab == 2) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[i].bExclude) ? "E" : "I"));
            }
        }

        wrefresh(w_auto_pickup);
        ch = input();

        switch(ch) {
            case 'j': //move down
                iCurrentLine++;
                iCurrentTab = 1;
                if (iCurrentLine >= vAutoPickupRules.size()) {
                    iCurrentLine = 0;
                }
                break;
            case 'k': //move up
                iCurrentLine--;
                iCurrentTab = 1;
                if (iCurrentLine < 0) {
                    iCurrentLine = vAutoPickupRules.size()-1;
                }
                break;
            case 'a': //add new rule
            case 'A':
                vAutoPickupRules.push_back(cPickupRules("", true, false));
                iCurrentLine = vAutoPickupRules.size()-1;
                break;
            case 'r': //remove rule
            case 'R':
                vAutoPickupRules.erase(vAutoPickupRules.begin() + iCurrentLine);
                if (iCurrentLine > vAutoPickupRules.size()-1) {
                    iCurrentLine--;
                }
                break;
            case '\n': //Edit tab in current line
                if (iCurrentTab == 1) {
                    vAutoPickupRules[iCurrentLine].sRule = string_input_popup("Pickup Rule:", 30, vAutoPickupRules[iCurrentLine].sRule);

                } else if (iCurrentTab == 2) {
                    vAutoPickupRules[iCurrentLine].bExclude = !vAutoPickupRules[iCurrentLine].bExclude;
                }
                break;
            case 'e': //enable rule
            case 'E':
                vAutoPickupRules[iCurrentLine].bActive = true;
                break;
            case 'd': //disable rule
            case 'D':
                vAutoPickupRules[iCurrentLine].bActive = false;
                break;
            case 'h': //move left
                iCurrentTab--;
                if (iCurrentTab < 1) {
                    iCurrentTab = iTotalTabs;
                }
                break;
            case 'l': //move right
                iCurrentTab++;
                if (iCurrentTab > iTotalTabs) {
                    iCurrentTab = 1;
                }
                break;
            case '+': //move rule up
                if (iCurrentLine < vAutoPickupRules.size()-1) {
                    std::swap(vAutoPickupRules[iCurrentLine], vAutoPickupRules[iCurrentLine+1]);
                    iCurrentLine++;
                    iCurrentTab = 1;
                }
                break;
            case '-': //move rule down
                if (iCurrentLine > 0) {
                    std::swap(vAutoPickupRules[iCurrentLine], vAutoPickupRules[iCurrentLine-1]);
                    iCurrentLine--;
                    iCurrentTab = 1;
                }
                break;
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if(query_yn("Save changes?")) {
        save_auto_pickup();
    } else {
        load_auto_pickup();
    }

    werase(w_auto_pickup);
    werase(w_auto_pickup_border);
}

void load_auto_pickup()
{
    std::ifstream fin;
    fin.open("data/auto_pickup.txt");
    if(!fin.is_open()) {
        fin.close();
        create_default_auto_pickup();
        fin.open("data/auto_pickup.txt");
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create ./data/auto_pickup.txt\n";
            return;
        }
    }

    vAutoPickupRules.clear();

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
                DebugLog() << "Bad Rule: " << sLine << "\n";
                getline(fin, sLine);

            } else {
                std::string sRule = "";
                bool bActive = true;
                bool bExclude = false;

                int iPos = 0;
                int iTab = 1;
                do {
                    iPos = sLine.find(";");

                    std::string sTemp = (iPos == std::string::npos) ? sLine : sLine.substr(0, iPos);

                    if (iTab == 1) {
                        sRule = sTemp;

                    } else if (iTab == 2) {
                        bActive = (sTemp == "T") ? true : false;

                    } else if (iTab == 3) {
                        bExclude = (sTemp == "T") ? true : false;
                    }

                    iTab++;

                    if (iPos != std::string::npos) {
                        sLine = sLine.substr(iPos+1, sLine.size());
                    }

                } while(iPos != std::string::npos);

                vAutoPickupRules.push_back(cPickupRules(sRule, bActive, bExclude));
            }
        }

        if(fin.peek() == '\n') {
            getline(fin, sLine);    // Chomp
        }
    }

    fin.close();
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
    for(int i = 0; i < vAutoPickupRules.size(); i++) {
        fout << vAutoPickupRules[i].sRule << ";";
        fout << (vAutoPickupRules[i].bActive ? "T" : "F") << ";";
        fout << (vAutoPickupRules[i].bExclude ? "T" : "F");
        fout << "\n";
    }
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
