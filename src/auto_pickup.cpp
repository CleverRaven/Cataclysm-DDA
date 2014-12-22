#include "game.h"
#include "auto_pickup.h"
#include "output.h"
#include "debug.h"
#include "item_factory.h"
#include "catacharset.h"
#include "translations.h"
#include "path_info.h"
#include "file_wrapper.h"

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>

std::map<std::string, std::string> mapAutoPickupItems;
std::vector<cPickupRules> vAutoPickupRules[5];

void show_auto_pickup()
{
    save_reset_changes(false);

    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    std::map<int, bool> mapLines;
    mapLines[4] = true;
    mapLines[50] = true;
    mapLines[54] = true;

    const int iTotalCols = mapLines.size() - 1;

    WINDOW *w_auto_pickup_options = newwin(FULL_SCREEN_HEIGHT / 2, FULL_SCREEN_WIDTH / 2,
                                           iOffsetY + (FULL_SCREEN_HEIGHT / 2) / 2, iOffsetX + (FULL_SCREEN_WIDTH / 2) / 2);
    WINDOW_PTR w_auto_pickup_optionsptr( w_auto_pickup_options );
    WINDOW *w_auto_pickup_help = newwin((FULL_SCREEN_HEIGHT / 2) - 2, FULL_SCREEN_WIDTH * 3 / 4,
                                        7 + iOffsetY + (FULL_SCREEN_HEIGHT / 2) / 2, iOffsetX + 19 / 2);
    WINDOW_PTR w_auto_pickup_helpptr( w_auto_pickup_help );

    WINDOW *w_auto_pickup_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW_PTR w_auto_pickup_borderptr( w_auto_pickup_border );
    WINDOW *w_auto_pickup_header = newwin(iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                          1 + iOffsetX);
    WINDOW_PTR w_auto_pickup_headerptr( w_auto_pickup_header );
    WINDOW *w_auto_pickup = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY,
                                   1 + iOffsetX);
    WINDOW_PTR w_auto_pickupptr( w_auto_pickup );

    draw_border(w_auto_pickup_border);
    mvwputch(w_auto_pickup_border, 3,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_auto_pickup_border, 3, 79, c_ltgray, LINE_XOXX); // -|

    for( auto &mapLine : mapLines ) {
        mvwputch( w_auto_pickup_border, FULL_SCREEN_HEIGHT - 1, mapLine.first + 1, c_ltgray,
                  LINE_XXOX ); // _|_
    }

    mvwprintz(w_auto_pickup_border, 0, 29, c_ltred, _(" AUTO PICKUP MANAGER "));
    wrefresh(w_auto_pickup_border);

    int tmpx = 0;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<A>dd")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<R>emove")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<C>opy")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<M>ove")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<E>nable")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<D>isable")) + 2;
    shortcut_print(w_auto_pickup_header, 0, tmpx, c_white, c_ltgreen, _("<T>est"));
    tmpx = 0;
    tmpx += shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen,
                           _("<+-> Move up/down")) + 2;
    tmpx += shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen, _("<Enter>-Edit")) + 2;
    shortcut_print(w_auto_pickup_header, 1, tmpx, c_white, c_ltgreen, _("<Tab>-Switch Page"));

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXXX);
            mvwputch(w_auto_pickup_header, 3, i, c_ltgray, LINE_XOXO);
        } else {
            mvwputch(w_auto_pickup_header, 2, i, c_ltgray, LINE_OXOX); // Draw line under header
        }
    }

    mvwprintz(w_auto_pickup_header, 3, 1, c_white, "#");
    mvwprintz(w_auto_pickup_header, 3, 8, c_white, _("Rules"));
    mvwprintz(w_auto_pickup_header, 3, 51, c_white, _("I/E"));

    wrefresh(w_auto_pickup_header);

    int iCurrentPage = 1;
    int iCurrentLine = 0;
    int iCurrentCol = 1;
    int iStartPos = 0;
    bool bStuffChanged = false;
    input_context ctxt("AUTO_PICKUP");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("ADD_RULE");
    ctxt.register_action("REMOVE_RULE");
    ctxt.register_action("COPY_RULE");
    ctxt.register_action("SWAP_RULE_GLOBAL_CHAR");
    ctxt.register_action("ENABLE_RULE");
    ctxt.register_action("DISABLE_RULE");
    ctxt.register_action("MOVE_RULE_UP");
    ctxt.register_action("MOVE_RULE_DOWN");
    ctxt.register_action("TEST_RULE");
    ctxt.register_action("SWITCH_AUTO_PICKUP_OPTION");
    ctxt.register_action("HELP_KEYBINDINGS");

    std::stringstream sTemp;

    while(true) {
        int locx = 17;
        locx += shortcut_print(w_auto_pickup_header, 2, locx, c_white,
                               (iCurrentPage == 1) ? hilite(c_white) : c_white, _("[<Global>]")) + 1;
        shortcut_print(w_auto_pickup_header, 2, locx, c_white,
                       (iCurrentPage == 2) ? hilite(c_white) : c_white, _("[<Character>]"));

        locx = 55;
        mvwprintz(w_auto_pickup_header, 0, locx, c_white, _("Auto pickup enabled:"));
        locx += shortcut_print(w_auto_pickup_header, 1, locx,
                               ((OPTIONS["AUTO_PICKUP"]) ? c_ltgreen : c_ltred), c_white,
                               ((OPTIONS["AUTO_PICKUP"]) ? _("True") : _("False")));
        locx += shortcut_print(w_auto_pickup_header, 1, locx, c_white, c_ltgreen, "  ");
        locx += shortcut_print(w_auto_pickup_header, 1, locx, c_white, c_ltgreen, _("<S>witch"));
        shortcut_print(w_auto_pickup_header, 1, locx, c_white, c_ltgreen, "  ");

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

        const bool currentPageNonEmpty = !vAutoPickupRules[iCurrentPage].empty();

        if (iCurrentPage == 2 && g->u.name == "") {
            vAutoPickupRules[2].clear();
            mvwprintz(w_auto_pickup, 8, 15, c_white,
                      _("Please load a character first to use this page!"));
        }

        //Draw Scrollbar
        draw_scrollbar(w_auto_pickup_border, iCurrentLine, iContentHeight,
                       vAutoPickupRules[iCurrentPage].size(), 5);

        calcStartPos(iStartPos, iCurrentLine, iContentHeight,
                     vAutoPickupRules[iCurrentPage].size());

        // display auto pickup
        for (int i = iStartPos; i < (int)vAutoPickupRules[iCurrentPage].size(); i++) {
            if (i >= iStartPos &&
                i < iStartPos + ((iContentHeight > (int)vAutoPickupRules[iCurrentPage].size()) ?
                                 (int)vAutoPickupRules[iCurrentPage].size() : iContentHeight)) {
                nc_color cLineColor = (vAutoPickupRules[iCurrentPage][i].bActive) ?
                                      c_white : c_ltgray;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz(w_auto_pickup, i - iStartPos, 1, cLineColor, "%s", sTemp.str().c_str());
                mvwprintz(w_auto_pickup, i - iStartPos, 5, cLineColor, "");

                if (iCurrentLine == i) {
                    wprintz(w_auto_pickup, c_yellow, ">> ");
                } else {
                    wprintz(w_auto_pickup, c_yellow, "   ");
                }

                wprintz(w_auto_pickup, (iCurrentLine == i &&
                                        iCurrentCol == 1) ? hilite(cLineColor) : cLineColor, "%s",
                        ((vAutoPickupRules[iCurrentPage][i].sRule == "") ? _("<empty rule>") :
                         vAutoPickupRules[iCurrentPage][i].sRule).c_str());

                mvwprintz(w_auto_pickup, i - iStartPos, 52, (iCurrentLine == i &&
                          iCurrentCol == 2) ? hilite(cLineColor) : cLineColor, "%s",
                          ((vAutoPickupRules[iCurrentPage][i].bExclude) ? rm_prefix(_("<Exclude>E")).c_str() : rm_prefix(
                               _("<Include>I")).c_str()));
            }
        }

        wrefresh(w_auto_pickup);

        const std::string action = ctxt.handle_input();

        if (action == "NEXT_TAB") {
            iCurrentPage++;
            if (iCurrentPage > 2) {
                iCurrentPage = 1;
                iCurrentLine = 0;
            }
        } else if (action == "PREV_TAB") {
            iCurrentPage--;
            if (iCurrentPage < 1) {
                iCurrentPage = 2;
                iCurrentLine = 0;
            }
        } else if (action == "QUIT") {
            break;
        } else if (iCurrentPage == 2 && g->u.name.empty()) {
            //Only allow loaded games to use the char sheet
        } else if (action == "DOWN") {
            iCurrentLine++;
            iCurrentCol = 1;
            if (iCurrentLine >= (int)vAutoPickupRules[iCurrentPage].size()) {
                iCurrentLine = 0;
            }
        } else if (action == "UP") {
            iCurrentLine--;
            iCurrentCol = 1;
            if (iCurrentLine < 0) {
                iCurrentLine = vAutoPickupRules[iCurrentPage].size() - 1;
            }
        } else if (action == "ADD_RULE") {
            bStuffChanged = true;
            vAutoPickupRules[iCurrentPage].push_back(cPickupRules("", true, false));
            iCurrentLine = vAutoPickupRules[iCurrentPage].size() - 1;
        } else if (action == "REMOVE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vAutoPickupRules[iCurrentPage].erase(vAutoPickupRules[iCurrentPage].begin() + iCurrentLine);
            if (iCurrentLine > (int)vAutoPickupRules[iCurrentPage].size() - 1) {
                iCurrentLine--;
            }
        } else if (action == "COPY_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vAutoPickupRules[iCurrentPage].push_back(cPickupRules(
                        vAutoPickupRules[iCurrentPage][iCurrentLine].sRule,
                        vAutoPickupRules[iCurrentPage][iCurrentLine].bActive,
                        vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude));
            iCurrentLine = vAutoPickupRules[iCurrentPage].size() - 1;
        } else if (action == "SWAP_RULE_GLOBAL_CHAR" && currentPageNonEmpty) {
            if ((iCurrentPage == 1 && g->u.name != "") || iCurrentPage == 2) {
                bStuffChanged = true;
                //copy over
                vAutoPickupRules[(iCurrentPage == 1) ? 2 : 1].push_back(cPickupRules(
                            vAutoPickupRules[iCurrentPage][iCurrentLine].sRule,
                            vAutoPickupRules[iCurrentPage][iCurrentLine].bActive,
                            vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude));

                //remove old
                vAutoPickupRules[iCurrentPage].erase(vAutoPickupRules[iCurrentPage].begin() + iCurrentLine);
                iCurrentLine = vAutoPickupRules[(iCurrentPage == 1) ? 2 : 1].size() - 1;
                iCurrentPage = (iCurrentPage == 1) ? 2 : 1;
            }
        } else if (action == "CONFIRM" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iCurrentCol == 1) {
                fold_and_print(w_auto_pickup_help, 1, 1, 999, c_white,
                               _(
                                   "* is used as a Wildcard. A few Examples:\n"
                                   "\n"
                                   "wooden arrow    matches the itemname exactly\n"
                                   "wooden ar*      matches items beginning with wood ar\n"
                                   "*rrow           matches items ending with rrow\n"
                                   "*avy fle*fi*arrow     multiple * are allowed\n"
                                   "heAVY*woOD*arrOW      case insensitive search\n"
                                   "")
                              );

                draw_border(w_auto_pickup_help);
                wrefresh(w_auto_pickup_help);
                vAutoPickupRules[iCurrentPage][iCurrentLine].sRule = trim_rule(string_input_popup(_("Pickup Rule:"),
                        30, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule));
            } else if (iCurrentCol == 2) {
                vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude =
                    !vAutoPickupRules[iCurrentPage][iCurrentLine].bExclude;
            }
        } else if (action == "ENABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vAutoPickupRules[iCurrentPage][iCurrentLine].bActive = true;
        } else if (action == "DISABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vAutoPickupRules[iCurrentPage][iCurrentLine].bActive = false;
        } else if (action == "LEFT") {
            iCurrentCol--;
            if (iCurrentCol < 1) {
                iCurrentCol = iTotalCols;
            }
        } else if (action == "RIGHT") {
            iCurrentCol++;
            if (iCurrentCol > iTotalCols) {
                iCurrentCol = 1;
            }
        } else if (action == "MOVE_RULE_UP" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iCurrentLine < (int)vAutoPickupRules[iCurrentPage].size() - 1) {
                std::swap(vAutoPickupRules[iCurrentPage][iCurrentLine],
                          vAutoPickupRules[iCurrentPage][iCurrentLine + 1]);
                iCurrentLine++;
                iCurrentCol = 1;
            }
        } else if (action == "MOVE_RULE_DOWN" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iCurrentLine > 0) {
                std::swap(vAutoPickupRules[iCurrentPage][iCurrentLine],
                          vAutoPickupRules[iCurrentPage][iCurrentLine - 1]);
                iCurrentLine--;
                iCurrentCol = 1;
            }
        } else if (action == "TEST_RULE" && currentPageNonEmpty) {
            test_pattern(iCurrentPage, iCurrentLine);
        } else if (action == "SWITCH_AUTO_PICKUP_OPTION") {
            OPTIONS["AUTO_PICKUP"].setNext();
            save_options((g->u.name != ""));
        }
    }

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
    for( auto &p : item_controller->get_all_itypes() ) {
        sItemName = p.second->nname(1);
        if (vAutoPickupRules[iCurrentPage][iCurrentLine].bActive &&
            auto_pickup_match(sItemName, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule)) {
            vMatchingItems.push_back(sItemName);
        }
    }

    const int iOffsetX = 15 + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    const int iOffsetY = 5 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0);

    int iStartPos = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 8;
    const int iContentWidth = FULL_SCREEN_WIDTH - 30;
    std::stringstream sTemp;

    WINDOW *w_test_rule_border = newwin(iContentHeight + 2, iContentWidth, iOffsetY, iOffsetX);
    WINDOW_PTR w_test_rule_borderptr( w_test_rule_border );
    WINDOW *w_test_rule_content = newwin(iContentHeight, iContentWidth - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW_PTR w_test_rule_contentptr( w_test_rule_content );

    draw_border(w_test_rule_border);

    int nmatch = vMatchingItems.size();
    std::string buf = string_format(ngettext("%1$d item matches: %2$s", "%1$d items match: %2$s",
                                    nmatch), nmatch, vAutoPickupRules[iCurrentPage][iCurrentLine].sRule.c_str());
    mvwprintz(w_test_rule_border, 0, iContentWidth / 2 - utf8_width(buf.c_str()) / 2, hilite(c_white),
              "%s", buf.c_str());

    mvwprintz(w_test_rule_border, iContentHeight + 1, 1, red_background(c_white),
              _("Won't display damaged, fits and can/bottle items"));

    wrefresh(w_test_rule_border);

    iCurrentLine = 0;

    input_context ctxt("AUTO_PICKUP_TEST");
    ctxt.register_updown();
    ctxt.register_action("QUIT");

    while(true) {
        // Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                mvwputch(w_test_rule_content, i, j, c_black, ' ');
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, vMatchingItems.size());

        // display auto pickup
        for (int i = iStartPos; i < (int)vMatchingItems.size(); i++) {
            if (i >= iStartPos &&
                i < iStartPos + ((iContentHeight > (int)vMatchingItems.size()) ? (int)vMatchingItems.size() :
                                 iContentHeight)) {
                nc_color cLineColor = c_white;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz(w_test_rule_content, i - iStartPos, 0, cLineColor, "%s", sTemp.str().c_str());
                mvwprintz(w_test_rule_content, i - iStartPos, 4, cLineColor, "");

                if (iCurrentLine == i) {
                    wprintz(w_test_rule_content, c_yellow, ">> ");
                } else {
                    wprintz(w_test_rule_content, c_yellow, "   ");
                }

                wprintz(w_test_rule_content, (iCurrentLine == i) ? hilite(cLineColor) : cLineColor,
                        vMatchingItems[i].c_str());
            }
        }

        wrefresh(w_test_rule_content);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            iCurrentLine++;
            if (iCurrentLine >= (int)vMatchingItems.size()) {
                iCurrentLine = 0;
            }
        } else if (action == "UP") {
            iCurrentLine--;
            if (iCurrentLine < 0) {
                iCurrentLine = vMatchingItems.size() - 1;
            }
        } else {
            break;
        }
    }
}

void load_auto_pickup(bool bCharacter)
{
    std::ifstream fin;
    std::string sFile = FILENAMES["autopickup"];

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
    }

    bool legacy_autopickup_loaded = false;
    fin.open(sFile.c_str());
    if(!fin.is_open()) {
        if( !bCharacter ) {
            fin.open(FILENAMES["legacy_autopickup"].c_str());
        }
        if( !fin.is_open() ) {
            assure_dir_exist(FILENAMES["config_dir"]);
            create_default_auto_pickup(bCharacter);
            fin.open(sFile.c_str());
        } else {
            legacy_autopickup_loaded = true;
        }

        if(!fin.is_open()) {
            DebugLog( D_ERROR, DC_ALL ) << "Could neither read nor create " << sFile;
            return;
        }
    }

    vAutoPickupRules[(bCharacter) ? APU_CHARACTER : APU_GLOBAL].clear();

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if(sLine != "" && sLine[0] != '#') {
            int iNum = std::count(sLine.begin(), sLine.end(), ';');

            if(iNum != 2) {
                DebugLog( D_ERROR, DC_ALL ) << "Bad Rule: " << sLine;
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
                        sLine = sLine.substr(iPos + 1, sLine.size());
                    }

                } while(iPos != std::string::npos);

                vAutoPickupRules[(bCharacter) ? APU_CHARACTER : APU_GLOBAL].push_back(cPickupRules(sRule, bActive,
                        bExclude));
            }
        }
    }

    fin.close();
    merge_vector();
    createPickupRules();
    if( legacy_autopickup_loaded ) {
        assure_dir_exist(FILENAMES["config_dir"]);
        save_auto_pickup( bCharacter );
    }
}

void merge_vector()
{
    vAutoPickupRules[APU_MERGED].clear();

    for (unsigned i = APU_GLOBAL; i <= APU_CHARACTER; i++) { //Loop through global 1 and character 2
        for (std::vector<cPickupRules>::iterator it = vAutoPickupRules[i].begin();
             it != vAutoPickupRules[i].end(); ++it) {
            if (it->sRule != "") {
                vAutoPickupRules[APU_MERGED].push_back(cPickupRules(it->sRule,
                                                       it->bActive, it->bExclude));
            }
        }
    }
}

bool hasPickupRule(std::string sRule)
{
    for( auto &elem : vAutoPickupRules[APU_CHARACTER] ) {
        if( sRule.length() == elem.sRule.length() && ci_find_substr( sRule, elem.sRule ) != -1 ) {
            return true;
        }
    }
    return false;
}

void addPickupRule(std::string sRule)
{
    vAutoPickupRules[APU_CHARACTER].push_back(cPickupRules(sRule, true, false));
    merge_vector();
    createPickupRules();

    if (!OPTIONS["AUTO_PICKUP"] &&
        query_yn(_("Autopickup is not enabled in the options. Enable it now?")) ) {
        OPTIONS["AUTO_PICKUP"].setNext();
        save_options(true);
    }
}

void removePickupRule(std::string sRule)
{
    for (std::vector<cPickupRules>::iterator it = vAutoPickupRules[APU_CHARACTER].begin();
         it != vAutoPickupRules[APU_CHARACTER].end(); ++it) {
        if (sRule.length() == it->sRule.length() &&
            ci_find_substr(sRule, it->sRule) != -1) {
            vAutoPickupRules[APU_CHARACTER].erase(it);
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

    //Includes only
    for( auto &elem : vAutoPickupRules[APU_MERGED] ) {
        if( !elem.bExclude ) {
            if (sItemNameIn != "") {
                if( elem.bActive && auto_pickup_match( sItemNameIn, elem.sRule ) ) {
                    mapAutoPickupItems[sItemNameIn] = "true";
                    break;
                }
            } else {
                //Check include paterns against all itemfactory items
                for( auto &p : item_controller->get_all_itypes() ) {
                    sItemName = p.second->nname(1);
                    if( elem.bActive && auto_pickup_match( sItemName, elem.sRule ) ) {
                        mapAutoPickupItems[sItemName] = "true";
                    }
                }
            }
        }
    }

    //Excludes only
    for( auto &elem : vAutoPickupRules[APU_MERGED] ) {
        if( elem.bExclude ) {
            if (sItemNameIn != "") {
                if( elem.bActive && auto_pickup_match( sItemNameIn, elem.sRule ) ) {
                    mapAutoPickupItems[sItemNameIn] = "false";
                    return;
                }
            } else {
                //Check exclude paterns against all included items
                for (std::map<std::string, std::string>::iterator iter =
                         mapAutoPickupItems.begin();
                     iter != mapAutoPickupItems.end(); ++iter) {
                    if( elem.bActive && auto_pickup_match( iter->first, elem.sRule ) ) {
                        mapAutoPickupItems[iter->first] = "false";
                    }
                }
            }
        }
    }
}

bool checkExcludeRules(const std::string sItemNameIn)
{
    for( auto &elem : vAutoPickupRules[APU_MERGED] ) {
        if( elem.bExclude && elem.bActive && auto_pickup_match( sItemNameIn, elem.sRule ) ) {
            return false;
        }
    }

    return true;
}

void save_reset_changes(bool bReset)
{
    for (int i = APU_GLOBAL; i <= APU_CHARACTER; i++) { //Loop through global 1 and character 2
        vAutoPickupRules[i + ((bReset) ? 0 : 2)].clear();
        for (std::vector<cPickupRules>::iterator it =
                 vAutoPickupRules[i + ((bReset) ? 2 : 0)].begin();
             it != vAutoPickupRules[i + ((bReset) ? 2 : 0)].end(); ++it) {
            if (it->sRule != "") {
                vAutoPickupRules[i + ((bReset) ? 0 : 2)].push_back(cPickupRules(
                            it->sRule, it->bActive, it->bExclude));
            }
        }
    }

    merge_vector();
}

std::string auto_pickup_header(bool bCharacter)
{
    std::string sTemp = (bCharacter) ? "character" : "global";
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

bool save_auto_pickup(bool bCharacter)
{
    std::ofstream fout;
    std::string sFile = FILENAMES["autopickup"];

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
        std::ifstream fin;

        fin.open((world_generator->active_world->world_path + "/" +
                  base64_encode(g->u.name) + ".sav").c_str());
        if(!fin.is_open()) {
            return true;
        }
        fin.close();
    }

    fout.exceptions(std::ios::badbit | std::ios::failbit);
    try {
        assure_dir_exist(FILENAMES["config_dir"]);
        fout.open(sFile.c_str());

        fout << auto_pickup_header(bCharacter) << std::endl;
        for( auto &elem : vAutoPickupRules[( bCharacter ) ? APU_CHARACTER : APU_GLOBAL] ) {
            fout << elem.sRule << ";";
            fout << ( elem.bActive ? "T" : "F" ) << ";";
            fout << ( elem.bExclude ? "T" : "F" );
            fout << "\n";
        }

        if (!bCharacter) {
            merge_vector();
            createPickupRules();
        }
        fout.close();
        return true;
    } catch(std::ios::failure &) {
        popup(_("Failed to write autopickup rules to %s"), sFile.c_str());
        return false;
    }
}

void create_default_auto_pickup(bool bCharacter)
{
    std::ofstream fout;
    std::string sFile = FILENAMES["autopickup"];

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
        sPattern = sPattern.substr(0, iPos) + sPattern.substr(iPos + 1, sPattern.length() - iPos - 1);
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

    int iPos;
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

    for (std::vector<std::string>::iterator it = vPattern.begin();
         it != vPattern.end(); ++it) {
        if (it == vPattern.begin() && *it != "") { //beginning: ^vPat[i]
            if (sText.length() < it->length() ||
                ci_find_substr(sText.substr(0, it->length()), *it) == -1) {
                return false;
            }

            sText = sText.substr(it->length(), sText.length() - it->length());
        } else if (it == vPattern.end() - 1 && *it != "") { //linenend: vPat[i]$
            if (sText.length() < it->length() ||
                ci_find_substr(sText.substr(sText.length() - it->length(),
                                            it->length()), *it) == -1) {
                return false;
            }
        } else { //inbetween: vPat[i]
            if (*it != "") {
                if ((iPos = (int)ci_find_substr(sText, *it)) == -1) {
                    return false;
                }

                sText = sText.substr(iPos + (int)it->length(), (int)sText.length() - iPos);
            }
        }
    }

    return true;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    elems.clear();
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    if ( s.substr(s.length() - 1, 1) == "*") {
        elems.push_back("");
    }

    return elems;
}

// templated version of my_equal so it could work with both char and wchar_t
template<typename charT>
struct my_equal {
    public:
        my_equal( const std::locale &loc ) : loc_(loc) {}

        bool operator()(charT ch1, charT ch2)
        {
            return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
        }
    private:
        const std::locale &loc_;
};

// find substring (case insensitive)
template<typename charT>
int ci_find_substr( const charT &str1, const charT &str2, const std::locale &loc )
{
    typename charT::const_iterator it = std::search( str1.begin(), str1.end(), str2.begin(), str2.end(),
                                        my_equal<typename charT::value_type>(loc) );
    if ( it != str1.end() ) {
        return it - str1.begin();
    } else {
        return -1;    // not found
    }
}
