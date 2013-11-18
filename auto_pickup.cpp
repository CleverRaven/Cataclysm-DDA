#include "game.h"
#include "auto_pickup.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"
#include "item_factory.h"
#include "catacharset.h"
#include "translations.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <locale>

std::map<std::string, std::string> mapAutoPickupItems;
std::vector<cPickupRules> vAutoPickupRules[5];

void show_auto_pickup()
{
    save_reset_changes(false);

    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT-2-iHeaderHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[50] = true;
    mapLines[54] = true;

    const int iTotalCols = mapLines.size()-1;

    WINDOW* w_auto_pickup_options = newwin(FULL_SCREEN_HEIGHT/2, FULL_SCREEN_WIDTH/2, iOffsetY + (FULL_SCREEN_HEIGHT/2)/2, iOffsetX + (FULL_SCREEN_WIDTH/2)/2);
    WINDOW* w_auto_pickup_help = newwin((FULL_SCREEN_HEIGHT/2)-2, FULL_SCREEN_WIDTH * 3/4, 8 + iOffsetY + (FULL_SCREEN_HEIGHT/2)/2, iOffsetX + 19/2);

    WINDOW* w_auto_pickup_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW* w_auto_pickup_header = newwin(iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW* w_auto_pickup = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY, 1 + iOffsetX);

    wborder(w_auto_pickup_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwputch(w_auto_pickup_border, 3,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_auto_pickup_border, 3, 79, c_ltgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_auto_pickup_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, c_ltgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_auto_pickup_border, 0, 29, c_ltred, _(" AUTO PICKUP MANAGER "));
    wrefresh(w_auto_pickup_border);

    int tmpx = 0;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<A>dd"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<R>emove"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<C>opy"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<M>ove"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<E>nable"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<D>isable"))+2;
    shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<T>est"));
    tmpx = 0;
    tmpx += shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen, _("<+-> Move up/down"))+2;
    tmpx += shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen, _("<Enter>-Edit"))+2;
    shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen, _("<Tab>-Switch Page"));

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXXX);
            mvwputch(w_auto_pickup_header, 3, i, c_ltgray, LINE_XOXO);
        } else {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXOX); // Draw line under header
        }
    }

    mvwprintz(w_auto_pickup_header, 3, 0, c_white, "#");
    mvwprintz(w_auto_pickup_header, 3, 7, c_white, _("Rules"));
    mvwprintz(w_auto_pickup_header, 3, 51, c_white, _("I/E"));

    wrefresh(w_auto_pickup_header);

    int iCurrentPage = 1;
    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    bool bStuffChanged = false;
    char ch = ' ';

    std::stringstream sTemp;

    do {
        int locx = 17;
        locx += shortcut_print(w_auto_pickup_header, 2, locx, c_white, (iCurrentPage == 1) ? hilite(c_white) : c_white, _("[<Global>]"))+1;
        shortcut_print(w_auto_pickup_header, 2, locx, c_white, (iCurrentPage == 2) ? hilite(c_white) : c_white, _("[<Character>]"));

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
                mvwprintz(w_auto_pickup, 8, 15, c_white, _("Please load a character first to use this page!"));
            }

            //Draw Scrollbar
            draw_scrollbar(w_auto_pickup_border, iCurrentLine, iContentHeight, vAutoPickupRules[iCurrentPage].size(), 5);

            calcStartPos(iStartPos, iCurrentLine, iContentHeight, vAutoPickupRules[iCurrentPage].size());

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

                    wprintz(w_auto_pickup, (iCurrentLine == i && iCurrentCol == 1) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[iCurrentPage][i].sRule == "") ? _("<empty rule>") : vAutoPickupRules[iCurrentPage][i].sRule).c_str());

                    mvwprintz(w_auto_pickup, i - iStartPos, 52, (iCurrentLine == i && iCurrentCol == 2) ? hilite(cLineColor) : cLineColor, "%s", ((vAutoPickupRules[iCurrentPage][i].bExclude) ? rm_prefix(_("<Exclude>E")).c_str() : rm_prefix(_("<Include>I")).c_str()));
                }
            }

            wrefresh(w_auto_pickup);

        } else if (iCurrentPage == 3) {
            wborder(w_auto_pickup_options, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);

            mvwprintz(w_auto_pickup_options, 5, 10, c_white, _("Under construction!"));

            wrefresh(w_auto_pickup);
            wrefresh(w_auto_pickup_options);
        }

        ch = (char)input();

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
                            fold_and_print(w_auto_pickup_help, 1, 1, 999, c_white,
                                _(
                                "* is used as a Wildcard. A few Examples:\n"
                                "\n"
                                "wood arrow    matches the itemname exactly\n"
                                "wood ar*      matches items beginning with wood ar\n"
                                "*rrow         matches items ending with rrow\n"
                                "*avy fle*fi*arrow     multible * are allowed\n"
                                "heAVY*woOD*arrOW      case insesitive search\n"
                                "")
                            );

                            wborder(w_auto_pickup_help, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
                            wrefresh(w_auto_pickup_help);
                            vAutoPickupRules[iCurrentPage][iCurrentLine].sRule = trim_rule(string_input_popup(_("Pickup Rule:"), 30, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule));
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
                    case 't': //test rule
                    case 'T':
                        test_pattern(iCurrentPage, iCurrentLine);
                        break;
                }
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if (bStuffChanged) {
        if(query_yn(_("Save changes?"))) {
            save_auto_pickup(false);

            if (g->u.name != "") {
                save_auto_pickup(true);
            }
        } else {
            save_reset_changes(true);
        }
    }

    werase(w_auto_pickup);
    werase(w_auto_pickup_border);
    werase(w_auto_pickup_header);
    werase(w_auto_pickup_options);
    werase(w_auto_pickup_help);
}

void test_pattern(int iCurrentPage, int iCurrentLine)
{
    std::vector<std::string> vMatchingItems;
    std::string sItemName = "";

    if (vAutoPickupRules[iCurrentPage][iCurrentLine].sRule == "") {
        return;
    }

    //Loop through all itemfactory items
    //TODO: somehow generate damaged, fitting or container items
    for (unsigned i = 0; i < standard_itype_ids.size(); i++) {
        sItemName = item_controller->find_template(standard_itype_ids[i])->name;
        if (vAutoPickupRules[iCurrentPage][iCurrentLine].bActive && auto_pickup_match(sItemName, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule)) {
            vMatchingItems.push_back(sItemName);
        }
    }

    const int iOffsetX = 15 + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    const int iOffsetY = 5 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0);

    int iStartPos = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 8;
    const int iContentWidth = FULL_SCREEN_WIDTH - 30;
    char ch;
    std::stringstream sTemp;

    WINDOW* w_test_rule_border = newwin(iContentHeight + 2, iContentWidth, iOffsetY, iOffsetX);
    WINDOW* w_test_rule_content = newwin(iContentHeight, iContentWidth - 2, 1 + iOffsetY, 1 + iOffsetX);

    wborder(w_test_rule_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);

    int nmatch = vMatchingItems.size();
    std::string buf = string_format(ngettext("%1$d item matches: %2$s", "%1$d items match: %2$s", nmatch), nmatch, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule.c_str());
    mvwprintz(w_test_rule_border, 0, iContentWidth/2 - utf8_width(buf.c_str())/2, hilite(c_white), buf.c_str());

    mvwprintz(w_test_rule_border, iContentHeight + 1, 1, red_background(c_white), _("Won't display damaged, fits and can/bottle items"));

    wrefresh(w_test_rule_border);

    iCurrentLine = 0;

    do {
        // Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                mvwputch(w_test_rule_content, i, j, c_black, ' ');
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, vMatchingItems.size());

        // display auto pickup
        for (int i = iStartPos; i < vMatchingItems.size(); i++) {
            if (i >= iStartPos && i < iStartPos + ((iContentHeight > vMatchingItems.size()) ? vMatchingItems.size() : iContentHeight)) {
                nc_color cLineColor = c_white;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz(w_test_rule_content, i - iStartPos, 0, cLineColor, sTemp.str().c_str());
                mvwprintz(w_test_rule_content, i - iStartPos, 4, cLineColor, "");

                if (iCurrentLine == i) {
                    wprintz(w_test_rule_content, c_yellow, ">> ");
                } else {
                    wprintz(w_test_rule_content, c_yellow, "   ");
                }

                wprintz(w_test_rule_content, (iCurrentLine == i) ? hilite(cLineColor) : cLineColor, vMatchingItems[i].c_str());
            }
        }

        wrefresh(w_test_rule_content);

        ch = (char)input();

        switch(ch) {
            case 'j': //move down
                iCurrentLine++;
                if (iCurrentLine >= vMatchingItems.size()) {
                    iCurrentLine = 0;
                }
                break;
            case 'k': //move up
                iCurrentLine--;
                if (iCurrentLine < 0) {
                    iCurrentLine = vMatchingItems.size()-1;
                }
                break;
        }
    } while(ch == 'j' || ch == 'k');

    werase(w_test_rule_border);
    werase(w_test_rule_content);
}

void load_auto_pickup(bool bCharacter)
{
    std::ifstream fin;
    std::string sFile = "data/auto_pickup.txt";

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
    }

    fin.open(sFile.c_str());
    if(!fin.is_open()) {
        fin.close();

        create_default_auto_pickup(bCharacter);

        fin.open(sFile.c_str());
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create " << sFile << "\n";
            return;
        }
    }

    vAutoPickupRules[(bCharacter) ? 2 : 1].clear();

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if(sLine != "" && sLine[0] != '#') {
            int iNum = std::count(sLine.begin(), sLine.end(), ';');

            if(iNum != 2) {
                /*int iNum = std::count(sLine.begin(), sLine.end(), ' ');

                if(iNum == 1) { //its an option! hurray

                } else {*/
                    DebugLog() << "Bad Rule: " << sLine << "\n";
                //}

            } else {
                std::string sRule = "";
                bool bActive = true;
                bool bExclude = false;

                size_t iPos = 0;
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

                vAutoPickupRules[(bCharacter) ? 2 : 1].push_back(cPickupRules(sRule, bActive, bExclude));
            }
        }
    }

    fin.close();

    merge_vector();
    createPickupRules();
}

void merge_vector()
{
    vAutoPickupRules[0].clear();

    for (unsigned i = 1; i <= 2; i++) { //Loop through global 1 and character 2
        for (unsigned j = 0; j < vAutoPickupRules[i].size(); j++) {
            if (vAutoPickupRules[i][j].sRule != "") {
                vAutoPickupRules[0].push_back(cPickupRules(vAutoPickupRules[i][j].sRule, vAutoPickupRules[i][j].bActive, vAutoPickupRules[i][j].bExclude));
            }
        }
    }
}

bool hasPickupRule(std::string sRule)
{
    for (unsigned i = 0; i < vAutoPickupRules[2].size(); i++) {
        if (sRule.length() == (vAutoPickupRules[2][i].sRule).length() && ci_find_substr(sRule, vAutoPickupRules[2][i].sRule) != -1) {
            return true;
        }
    }

    return false;
}

void addPickupRule(std::string sRule)
{
    vAutoPickupRules[2].push_back(cPickupRules(sRule, true, false));
    merge_vector();
    createPickupRules();
}

void removePickupRule(std::string sRule)
{
    for (unsigned i = 0; i < vAutoPickupRules[2].size(); i++) {
        if (sRule.length() == (vAutoPickupRules[2][i].sRule).length() && ci_find_substr(sRule, vAutoPickupRules[2][i].sRule) != -1) {
            vAutoPickupRules[2].erase(vAutoPickupRules[2].begin() + i);
            merge_vector();
            createPickupRules();
            break;
        }
    }
}

void createPickupRules(const std::string sItemNameIn)
{
    if (sItemNameIn == "") {
        mapAutoPickupItems.clear();
    }

    std::string sItemName = "";

    for (unsigned iPattern = 0; iPattern < vAutoPickupRules[0].size(); iPattern++) { //Includes only
        if (!vAutoPickupRules[0][iPattern].bExclude) {
            if (sItemNameIn != "") {
                if (vAutoPickupRules[0][iPattern].bActive && auto_pickup_match(sItemNameIn, vAutoPickupRules[0][iPattern].sRule)) {
                    mapAutoPickupItems[sItemNameIn] = "true";
                    break;
                }
            } else {
                //Check include paterns against all itemfactory items
                for (unsigned i = 0; i < standard_itype_ids.size(); i++) {
                    sItemName = item_controller->find_template(standard_itype_ids[i])->name;
                    if (vAutoPickupRules[0][iPattern].bActive && auto_pickup_match(sItemName, vAutoPickupRules[0][iPattern].sRule)) {
                        mapAutoPickupItems[sItemName] = "true";
                    }
                }
            }
        }
    }

    for (unsigned iPattern = 0; iPattern < vAutoPickupRules[0].size(); iPattern++) { //Excludes only
        if (vAutoPickupRules[0][iPattern].bExclude) {
            if (sItemNameIn != "") {
                if (vAutoPickupRules[0][iPattern].bActive && auto_pickup_match(sItemNameIn, vAutoPickupRules[0][iPattern].sRule)) {
                    mapAutoPickupItems[sItemNameIn] = "false";
                    return;
                }
            } else {
                //Check exclude paterns against all included items
                for (std::map<std::string, std::string>::iterator iter = mapAutoPickupItems.begin(); iter != mapAutoPickupItems.end(); ++iter) {
                    if (vAutoPickupRules[0][iPattern].bActive && auto_pickup_match(iter->first, vAutoPickupRules[0][iPattern].sRule)) {
                        mapAutoPickupItems[iter->first] = "false";
                    }
                }
            }
        }
    }
}

void save_reset_changes(bool bReset)
{
    for (int i=1; i <= 2; i++) { //Loop through global 1 and character 2
        vAutoPickupRules[i + ((bReset) ? 0: 2)].clear();
        for (unsigned j=0; j < vAutoPickupRules[i + ((bReset) ? 2: 0)].size(); j++) {
            if (vAutoPickupRules[i + ((bReset) ? 2: 0)][j].sRule != "") {
                vAutoPickupRules[i + ((bReset) ? 0: 2)].push_back(cPickupRules(vAutoPickupRules[i + ((bReset) ? 2: 0)][j].sRule, vAutoPickupRules[i + ((bReset) ? 2: 0)][j].bActive, vAutoPickupRules[i + ((bReset) ? 2: 0)][j].bExclude));
            }
        }
    }

    merge_vector();
}

std::string auto_pickup_header(bool bCharacter)
{
    std::string sTemp = (bCharacter) ? "character": "global";
    return "# This is the " + sTemp + " auto pickup rules file. The format is\n\
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

void save_auto_pickup(bool bCharacter)
{
    std::ofstream fout;
    std::string sFile = "data/auto_pickup.txt";

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
        std::ifstream fin;

        fin.open((world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".sav").c_str());
        if(!fin.is_open()) {
            return;
        }
        fin.close();
    }

    fout.open(sFile.c_str());

    if(!fout.is_open()) {
        return;
    }

    fout << auto_pickup_header(bCharacter) << std::endl;
    for (unsigned i = 0; i < vAutoPickupRules[(bCharacter) ? 2 : 1].size(); i++) {
        fout << vAutoPickupRules[(bCharacter) ? 2 : 1][i].sRule << ";";
        fout << (vAutoPickupRules[(bCharacter) ? 2 : 1][i].bActive ? "T" : "F") << ";";
        fout << (vAutoPickupRules[(bCharacter) ? 2 : 1][i].bExclude ? "T" : "F");
        fout << "\n";
    }

    if (!bCharacter) {
        merge_vector();
        createPickupRules();
    }
}

void create_default_auto_pickup(bool bCharacter)
{
    std::ofstream fout;
    std::string sFile = "data/auto_pickup.txt";

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
    }

    fout.open(sFile.c_str());
    if(!fout.is_open()) {
        return;
    }

    fout << auto_pickup_header(bCharacter);
    fout.close();
}

std::string trim_rule(std::string sPattern)
{
    size_t iPos = 0;

    //Remove all double ** in pattern
    while((iPos = sPattern.find("**")) != std::string::npos) {
        sPattern = sPattern.substr(0, iPos) + sPattern.substr(iPos+1, sPattern.length()-iPos-1);
    }

    return sPattern;
}

bool auto_pickup_match(std::string sText, std::string sPattern)
{
    //case insenitive search

    /* Possible patterns
    *
    wooD
    wood*
    *wood
    Wood*aRrOW
    wood*arrow*
    *wood*arrow
    *wood*hard* *x*y*z*arrow*
    */

    if (sText == "") {
        return false;
    } else if (sText == "*") {
        return true;
    }

    size_t iPos;
    std::vector<std::string> vPattern;

    sPattern = trim_rule(sPattern);

    split(sPattern, '*', vPattern);
    size_t iNum = vPattern.size();

    if (iNum == 0) { //should never happen
        return false;
    } else if (iNum == 1) { // no * found
        if (sText.length() == vPattern[0].length() && ci_find_substr(sText, vPattern[0]) != -1) {
            return true;
        }

        return false;
    }

    for (unsigned i=0; i < vPattern.size(); i++) {
        if (i==0 && vPattern[i] != "") { //beginning: ^vPat[i]
            if (sText.length() < vPattern[i].length() || ci_find_substr(sText.substr(0, vPattern[i].length()), vPattern[i]) == -1) {
                //debugmsg(("1: sText: " + sText + " | sPattern: ^" + vPattern[i] + " | no match").c_str());
                return false;
            }

            sText = sText.substr(vPattern[i].length(), sText.length()-vPattern[i].length());
        } else if (i==vPattern.size()-1 && vPattern[i] != "") { //linenend: vPat[i]$
            if (sText.length() < vPattern[i].length() || ci_find_substr(sText.substr(sText.length()-vPattern[i].length(), vPattern[i].length()), vPattern[i]) == -1) {
                //debugmsg(("2: sText: " + sText + " | sPattern: " + vPattern[i] + "$ | no match").c_str());
                return false;
            }
        } else { //inbetween: vPat[i]
            if (vPattern[i] != "") {
                if ((iPos = ci_find_substr(sText, vPattern[i])) == -1) {
                    //debugmsg(("3: sText: " + sText + " | sPattern: " + vPattern[i] + " | no match").c_str());
                    return false;
                }

                sText = sText.substr(iPos+vPattern[i].length(), sText.length()-iPos);
            }
        }
    }

    return true;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    elems.clear();
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    if ( s.substr(s.length()-1, 1) == "*") {
        elems.push_back("");
    }

    return elems;
}

/*
    // string test
    std::string str1 = "FIRST HELLO";
    std::string str2 = "hello";
    int f1 = ci_find_substr( str1, str2 );

    // wstring test
    std::wstring wstr1 = L"ОПЯТЬ ПРИВЕТ";
    std::wstring wstr2 = L"привет";
    int f2 = ci_find_substr( wstr1, wstr2 );
*/

// templated version of my_equal so it could work with both char and wchar_t
template<typename charT>
struct my_equal {
    public:
        my_equal( const std::locale& loc ) : loc_(loc) {}

        bool operator()(charT ch1, charT ch2) {
            return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
        }
    private:
        const std::locale& loc_;
};

// find substring (case insensitive)
template<typename charT>
int ci_find_substr( const charT& str1, const charT& str2, const std::locale& loc )
{
    typename charT::const_iterator it = std::search( str1.begin(), str1.end(), str2.begin(), str2.end(), my_equal<typename charT::value_type>(loc) );
    if ( it != str1.end() ) return it - str1.begin();
    else return -1; // not found
}
