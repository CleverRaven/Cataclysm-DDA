#include "game.h"
#include "player.h"
#include "auto_pickup.h"
#include "output.h"
#include "debug.h"
#include "item_factory.h"
#include "catacharset.h"
#include "translations.h"
#include "path_info.h"
#include "filesystem.h"
#include "input.h"
#include "worldfactory.h"
#include "itype.h"

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>

auto_pickup &get_auto_pickup()
{
    static auto_pickup single_instance;
    return single_instance;
}

void auto_pickup::show()
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

    WINDOW *w_help = newwin((FULL_SCREEN_HEIGHT / 2) - 2, FULL_SCREEN_WIDTH * 3 / 4,
                                        7 + iOffsetY + (FULL_SCREEN_HEIGHT / 2) / 2, iOffsetX + 19 / 2);
    WINDOW_PTR w_helpptr( w_help );

    WINDOW *w_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW_PTR w_borderptr( w_border );
    WINDOW *w_header = newwin(iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                          1 + iOffsetX);
    WINDOW_PTR w_headerptr( w_header );
    WINDOW *w = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY,
                                   1 + iOffsetX);
    WINDOW_PTR wptr( w );

    draw_border(w_border);
    mvwputch(w_border, 3,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_border, 3, 79, c_ltgray, LINE_XOXX); // -|

    for( auto &mapLine : mapLines ) {
        mvwputch( w_border, FULL_SCREEN_HEIGHT - 1, mapLine.first + 1, c_ltgray,
                  LINE_XXOX ); // _|_
    }

    mvwprintz(w_border, 0, 29, c_ltred, _(" AUTO PICKUP MANAGER "));
    wrefresh(w_border);

    int tmpx = 0;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<A>dd")) + 2;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<R>emove")) + 2;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<C>opy")) + 2;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<M>ove")) + 2;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<E>nable")) + 2;
    tmpx += shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<D>isable")) + 2;
    shortcut_print(w_header, 0, tmpx, c_white, c_ltgreen, _("<T>est"));
    tmpx = 0;
    tmpx += shortcut_print(w_header, 1, tmpx, c_white, c_ltgreen,
                           _("<+-> Move up/down")) + 2;
    tmpx += shortcut_print(w_header, 1, tmpx, c_white, c_ltgreen, _("<Enter>-Edit")) + 2;
    shortcut_print(w_header, 1, tmpx, c_white, c_ltgreen, _("<Tab>-Switch Page"));

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_header, 2, i, c_ltgray, LINE_OXXX);
            mvwputch(w_header, 3, i, c_ltgray, LINE_XOXO);
        } else {
            mvwputch(w_header, 2, i, c_ltgray, LINE_OXOX); // Draw line under header
        }
    }

    mvwprintz(w_header, 3, 1, c_white, "#");
    mvwprintz(w_header, 3, 8, c_white, _("Rules"));
    mvwprintz(w_header, 3, 51, c_white, _("I/E"));

    wrefresh(w_header);

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
        locx += shortcut_print(w_header, 2, locx, c_white,
                               (iCurrentPage == 1) ? hilite(c_white) : c_white, _("[<Global>]")) + 1;
        shortcut_print(w_header, 2, locx, c_white,
                       (iCurrentPage == 2) ? hilite(c_white) : c_white, _("[<Character>]"));

        locx = 55;
        mvwprintz(w_header, 0, locx, c_white, _("Auto pickup enabled:"));
        locx += shortcut_print(w_header, 1, locx,
                               ((OPTIONS["AUTO_PICKUP"]) ? c_ltgreen : c_ltred), c_white,
                               ((OPTIONS["AUTO_PICKUP"]) ? _("True") : _("False")));
        locx += shortcut_print(w_header, 1, locx, c_white, c_ltgreen, "  ");
        locx += shortcut_print(w_header, 1, locx, c_white, c_ltgreen, _("<S>witch"));
        shortcut_print(w_header, 1, locx, c_white, c_ltgreen, "  ");

        wrefresh(w_header);

        // Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w, i, j, c_black, ' ');
                }
            }
        }

        const bool currentPageNonEmpty = !vRules[iCurrentPage].empty();

        if (iCurrentPage == 2 && g->u.name == "") {
            vRules[2].clear();
            mvwprintz(w, 8, 15, c_white,
                      _("Please load a character first to use this page!"));
        }

        draw_scrollbar(w_border, iCurrentLine, iContentHeight,
                       vRules[iCurrentPage].size(), 5);
        wrefresh(w_border);

        calcStartPos(iStartPos, iCurrentLine, iContentHeight,
                     vRules[iCurrentPage].size());

        // display auto pickup
        for (int i = iStartPos; i < (int)vRules[iCurrentPage].size(); i++) {
            if (i >= iStartPos &&
                i < iStartPos + ((iContentHeight > (int)vRules[iCurrentPage].size()) ?
                                 (int)vRules[iCurrentPage].size() : iContentHeight)) {
                nc_color cLineColor = (vRules[iCurrentPage][i].bActive) ?
                                      c_white : c_ltgray;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz(w, i - iStartPos, 1, cLineColor, "%s", sTemp.str().c_str());
                mvwprintz(w, i - iStartPos, 5, cLineColor, "");

                if (iCurrentLine == i) {
                    wprintz(w, c_yellow, ">> ");
                } else {
                    wprintz(w, c_yellow, "   ");
                }

                wprintz(w, (iCurrentLine == i &&
                                        iCurrentCol == 1) ? hilite(cLineColor) : cLineColor, "%s",
                        ((vRules[iCurrentPage][i].sRule == "") ? _("<empty rule>") :
                         vRules[iCurrentPage][i].sRule).c_str());

                mvwprintz(w, i - iStartPos, 52, (iCurrentLine == i &&
                          iCurrentCol == 2) ? hilite(cLineColor) : cLineColor, "%s",
                          ((vRules[iCurrentPage][i].bExclude) ? rm_prefix(_("<Exclude>E")).c_str() : rm_prefix(
                               _("<Include>I")).c_str()));
            }
        }

        wrefresh(w);

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
            if (iCurrentLine >= (int)vRules[iCurrentPage].size()) {
                iCurrentLine = 0;
            }
        } else if (action == "UP") {
            iCurrentLine--;
            iCurrentCol = 1;
            if (iCurrentLine < 0) {
                iCurrentLine = vRules[iCurrentPage].size() - 1;
            }
        } else if (action == "ADD_RULE") {
            bStuffChanged = true;
            vRules[iCurrentPage].push_back(cRules("", true, false));
            iCurrentLine = vRules[iCurrentPage].size() - 1;
        } else if (action == "REMOVE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iCurrentPage].erase(vRules[iCurrentPage].begin() + iCurrentLine);
            if (iCurrentLine > (int)vRules[iCurrentPage].size() - 1) {
                iCurrentLine--;
            }
            if(iCurrentLine < 0){
                iCurrentLine = 0;
            }
        } else if (action == "COPY_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iCurrentPage].push_back(cRules(
                        vRules[iCurrentPage][iCurrentLine].sRule,
                        vRules[iCurrentPage][iCurrentLine].bActive,
                        vRules[iCurrentPage][iCurrentLine].bExclude));
            iCurrentLine = vRules[iCurrentPage].size() - 1;
        } else if (action == "SWAP_RULE_GLOBAL_CHAR" && currentPageNonEmpty) {
            if ((iCurrentPage == 1 && g->u.name != "") || iCurrentPage == 2) {
                bStuffChanged = true;
                //copy over
                vRules[(iCurrentPage == 1) ? 2 : 1].push_back(cRules(
                            vRules[iCurrentPage][iCurrentLine].sRule,
                            vRules[iCurrentPage][iCurrentLine].bActive,
                            vRules[iCurrentPage][iCurrentLine].bExclude));

                //remove old
                vRules[iCurrentPage].erase(vRules[iCurrentPage].begin() + iCurrentLine);
                iCurrentLine = vRules[(iCurrentPage == 1) ? 2 : 1].size() - 1;
                iCurrentPage = (iCurrentPage == 1) ? 2 : 1;
            }
        } else if (action == "CONFIRM" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iCurrentCol == 1) {
                fold_and_print(w_help, 1, 1, 999, c_white,
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

                draw_border(w_help);
                wrefresh(w_help);
                vRules[iCurrentPage][iCurrentLine].sRule = trim_rule(string_input_popup(_("Pickup Rule:"),
                        30, vRules[iCurrentPage][iCurrentLine].sRule));
            } else if (iCurrentCol == 2) {
                vRules[iCurrentPage][iCurrentLine].bExclude =
                    !vRules[iCurrentPage][iCurrentLine].bExclude;
            }
        } else if (action == "ENABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iCurrentPage][iCurrentLine].bActive = true;
        } else if (action == "DISABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iCurrentPage][iCurrentLine].bActive = false;
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
            if (iCurrentLine < (int)vRules[iCurrentPage].size() - 1) {
                std::swap(vRules[iCurrentPage][iCurrentLine],
                          vRules[iCurrentPage][iCurrentLine + 1]);
                iCurrentLine++;
                iCurrentCol = 1;
            }
        } else if (action == "MOVE_RULE_DOWN" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iCurrentLine > 0) {
                std::swap(vRules[iCurrentPage][iCurrentLine],
                          vRules[iCurrentPage][iCurrentLine - 1]);
                iCurrentLine--;
                iCurrentCol = 1;
            }
        } else if (action == "TEST_RULE" && currentPageNonEmpty) {
            test_pattern(iCurrentPage, iCurrentLine);
        } else if (action == "SWITCH_OPTION") {
            OPTIONS["AUTO_PICKUP"].setNext();
            get_options().save((g->u.name != ""));
        }
    }

    if (bStuffChanged) {
        if(query_yn(_("Save changes?"))) {
            save(false);

            if (g->u.name != "") {
                save(true);
            }
        } else {
            save_reset_changes(true);
        }
    }
}

void auto_pickup::test_pattern(const int iCurrentPage, const int iCurrentLine)
{
    std::vector<std::string> vMatchingItems;
    std::string sItemName = "";

    if (vRules[iCurrentPage][iCurrentLine].sRule == "") {
        return;
    }

    //Loop through all itemfactory items
    //APU now ignores prefixes, bottled items and suffix combinations still not generated
    for( auto &p : item_controller->get_all_itypes() ) {
        sItemName = p.second->nname(1);
        if (vRules[iCurrentPage][iCurrentLine].bActive &&
            match(sItemName, vRules[iCurrentPage][iCurrentLine].sRule)) {
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
                                    nmatch), nmatch, vRules[iCurrentPage][iCurrentLine].sRule.c_str());
    mvwprintz(w_test_rule_border, 0, iContentWidth / 2 - utf8_width(buf) / 2, hilite(c_white),
              "%s", buf.c_str());

    mvwprintz(w_test_rule_border, iContentHeight + 1, 1, red_background(c_white),
              _("Won't display bottled and suffixes=(fits)"));

    wrefresh(w_test_rule_border);

    int iLine = 0;

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

        calcStartPos(iStartPos, iLine, iContentHeight, vMatchingItems.size());

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

                if (iLine == i) {
                    wprintz(w_test_rule_content, c_yellow, ">> ");
                } else {
                    wprintz(w_test_rule_content, c_yellow, "   ");
                }

                wprintz(w_test_rule_content, (iLine == i) ? hilite(cLineColor) : cLineColor,
                        vMatchingItems[i].c_str());
            }
        }

        wrefresh(w_test_rule_content);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            iLine++;
            if (iLine >= (int)vMatchingItems.size()) {
                iLine = 0;
            }
        } else if (action == "UP") {
            iLine--;
            if (iLine < 0) {
                iLine = vMatchingItems.size() - 1;
            }
        } else {
            break;
        }
    }
}

void auto_pickup::merge_vector()
{
    vRules[MERGED].clear();

    for (unsigned i = GLOBAL; i <= CHARACTER; i++) { //Loop through global 1 and character 2
        for (auto it = vRules[i].begin(); it != vRules[i].end(); ++it) {
            if (it->sRule != "") {
                vRules[MERGED].push_back(cRules(it->sRule, it->bActive, it->bExclude));
            }
        }
    }
}

bool auto_pickup::has_rule(const std::string &sRule)
{
    for( auto &elem : vRules[CHARACTER] ) {
        if( sRule.length() == elem.sRule.length() && ci_find_substr( sRule, elem.sRule ) != -1 ) {
            return true;
        }
    }
    return false;
}

void auto_pickup::add_rule(const std::string &sRule)
{
    vRules[CHARACTER].push_back(cRules(sRule, true, false));
    merge_vector();
    create_rules();

    if (!OPTIONS["AUTO_PICKUP"] &&
        query_yn(_("Autopickup is not enabled in the options. Enable it now?")) ) {
        OPTIONS["AUTO_PICKUP"].setNext();
        get_options().save(true);
    }
}

void auto_pickup::remove_rule(const std::string &sRule)
{
    for (auto it = vRules[CHARACTER].begin();
         it != vRules[CHARACTER].end(); ++it) {
        if (sRule.length() == it->sRule.length() &&
            ci_find_substr(sRule, it->sRule) != -1) {
            vRules[CHARACTER].erase(it);
            merge_vector();
            create_rules();
            break;
        }
    }
}

void auto_pickup::create_rules(const std::string &sItemNameIn)
{
    if (sItemNameIn == "") {
        mapItems.clear();
    }

    std::string sItemName = "";

    //process include/exclude in order of rules, global first, then character specific
    //the MERGED vector is already in the correct order
    //if a specific item is being added, all the rules need to be checked now
    //may have some performance issues since exclusion needs to check all items also
    for( auto &elem : vRules[MERGED] ) {
        if( !elem.bExclude ) {
            if (sItemNameIn != "") {
                if( elem.bActive && match( sItemNameIn, elem.sRule ) ) {
                    mapItems[sItemNameIn] = "true";
                }
            } else {
                //Check include patterns against all itemfactory items
                for( auto &p : item_controller->get_all_itypes() ) {
                    sItemName = p.second->nname(1);
                    if( elem.bActive && match( sItemName, elem.sRule ) ) {
                        mapItems[sItemName] = "true";
                    }
                }
            }
        } else {
            if (sItemNameIn != "") {
                if( elem.bActive && match( sItemNameIn, elem.sRule ) ) {
                    mapItems[sItemNameIn] = "false";
                }
            } else {
                //only re-exclude items from the existing mapping for now
                //new exclusions will process during pickup attempts
                for (auto iter = mapItems.begin(); iter != mapItems.end(); ++iter) {
                    if( elem.bActive && match( iter->first, elem.sRule ) ) {
                        mapItems[iter->first] = "false";
                    }
                }
            }
        }
    }
}

/**
 * Stores or retrieves the current ruleset from temporary storage.
 * Used to implement the "cancel changes" capability ("[N]o, don't save") of the
 * auto-pickup interface.
 *
 * @param bReset false to store, true to retrieve.
 */
void auto_pickup::save_reset_changes(const bool bReset)
{
    for (int i = GLOBAL; i <= CHARACTER; i++) { //Loop through global 1 and character 2
        int destination = i + ((bReset) ? 0 : 2); // if reset, copy to vRules[1,2]
        int source = i + ((bReset) ? 2 : 0); // if reset, copy from vRules[3,4]
                                             // (temp storage from when bReset was false)
        vRules[destination].clear();
        for (auto it = vRules[source].begin(); it != vRules[source].end(); ++it) {
            if (it->sRule != "") {
                vRules[destination].push_back(*it);
            }
        }
    }

    merge_vector();
}

std::string auto_pickup::trim_rule(const std::string &sPatternIn)
{
    std::string sPattern = sPatternIn;
    size_t iPos = 0;

    //Remove all double ** in pattern
    while((iPos = sPattern.find("**")) != std::string::npos) {
        sPattern = sPattern.substr(0, iPos) + sPattern.substr(iPos + 1, sPattern.length() - iPos - 1);
    }

    return sPattern;
}

bool auto_pickup::match(const std::string &sTextIn, const std::string &sPatternIn)
{
    std::string sText = sTextIn;
    std::string sPattern = sPatternIn;

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
        return (sText.length() == vPattern[0].length() && ci_find_substr(sText, vPattern[0]) != -1);
    }

    for (auto it = vPattern.begin(); it != vPattern.end(); ++it) {
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

std::vector<std::string> &auto_pickup::split(const std::string &s, char delim, std::vector<std::string> &elems)
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

// find substring (case insensitive)
template<typename charT>
int auto_pickup::ci_find_substr( const charT &str1, const charT &str2, const std::locale &loc )
{
    typename charT::const_iterator it = std::search( str1.begin(), str1.end(), str2.begin(), str2.end(),
                                        my_equal<typename charT::value_type>(loc) );
    if ( it != str1.end() ) {
        return it - str1.begin();
    } else {
        return -1;    // not found
    }
}

const std::string &auto_pickup::check_item(const std::string &sItemName)
{
    return mapItems[sItemName];
}

void auto_pickup::clear_character_rules()
{
    vRules[2].clear();
}

bool auto_pickup::save_character()
{
    return save(true);
}

bool auto_pickup::save_global()
{
    return save(false);
}

bool auto_pickup::save(const bool bCharacter)
{
    bChar = bCharacter;
    auto savefile = FILENAMES["autopickup"];

    try {
        if (bCharacter) {
            savefile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.json";
            std::ifstream fin;

            fin.open((world_generator->active_world->world_path + "/" +
                      base64_encode(g->u.name) + ".sav").c_str());
            if(!fin.is_open()) {
                return true; //Character not saved yet.
            }
            fin.close();
        }

        std::ofstream fout;
        fout.exceptions(std::ios::badbit | std::ios::failbit);

        fout.open(savefile.c_str());

        if(!fout.is_open()) {
            return true; //trick game into thinking it was saved
        }

        JsonOut jout( fout, true );
        serialize(jout);

        if(!bCharacter) {
            merge_vector();
            create_rules();
        }

        fout.close();
        return true;

    } catch(std::ios::failure &) {
        popup(_("Failed to save autopickup to %s"), savefile.c_str());
        return false;
    }

    return false;
}

void auto_pickup::load_character()
{
    load(true);
}

void auto_pickup::load_global()
{
    load(false);
}

void auto_pickup::load(const bool bCharacter)
{
    bChar = bCharacter;

    std::ifstream fin;
    std::string sFile = FILENAMES["autopickup"];
    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.json";
    }

    fin.open(sFile.c_str(), std::ifstream::in | std::ifstream::binary);

    if( !fin.good() ) {
        if (load_legacy(bCharacter)) {
            if (save(bCharacter)) {
                remove_file(sFile);
            }
        }
    } else {
        try {
            JsonIn jsin(fin);
            deserialize(jsin);
        } catch( const JsonError &e ) {
            DebugLog(D_ERROR, DC_ALL) << "auto_pickup::load: " << e;
        }
    }

    fin.close();
    merge_vector();
    create_rules();
}

void auto_pickup::serialize(JsonOut &json) const
{
    json.start_array();

    for( auto &elem : vRules[( bChar ) ? CHARACTER : GLOBAL] ) {
        json.start_object();

        json.member( "rule", elem.sRule );
        json.member( "active", elem.bActive );
        json.member( "exclude", elem.bExclude );

        json.end_object();
    }

    json.end_array();
}

void auto_pickup::deserialize(JsonIn &jsin)
{
    vRules[(bChar) ? CHARACTER : GLOBAL].clear();

    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject jo = jsin.get_object();

        const std::string sRule = jo.get_string("rule");
        const bool bActive = jo.get_bool("active");
        const bool bExclude = jo.get_bool("exclude");

        vRules[(bChar) ? CHARACTER : GLOBAL].push_back(cRules(sRule, bActive, bExclude));
    }
}

bool auto_pickup::load_legacy(const bool bCharacter)
{
    std::ifstream fin;
    std::string sFile = FILENAMES["legacy_autopickup2"];

    if (bCharacter) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode(g->u.name) + ".apu.txt";
    }

    fin.open(sFile.c_str());
    if(!fin.is_open()) {
        if( !bCharacter ) {
            fin.open(FILENAMES["legacy_autopickup"].c_str());

            if( !fin.is_open() ) {
                DebugLog( D_ERROR, DC_ALL ) << "Could neither read nor create " << sFile;
                return false;
            }
        } else {
            return false;
        }
    }

    vRules[(bCharacter) ? CHARACTER : GLOBAL].clear();

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
                        bActive = sTemp == "T" || sTemp == "True";

                    } else if (iCol == 3) {
                        bExclude = sTemp == "T" || sTemp == "True";
                    }

                    iCol++;

                    if (iPos != std::string::npos) {
                        sLine = sLine.substr(iPos + 1, sLine.size());
                    }

                } while(iPos != std::string::npos);

                vRules[(bCharacter) ? CHARACTER : GLOBAL].push_back(cRules(sRule, bActive, bExclude));
            }
        }
    }

    return true;
}
