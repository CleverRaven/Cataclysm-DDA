#include "game.h"
#include "options.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>

bool trigdist;
bool use_tiles;

std::map<std::string, cOpt> OPTIONS;
std::vector<std::pair<std::string, std::string> > vPages;
std::map<int, std::vector<std::string> > mPageItems;

void initOptions() {
    vPages.clear();
    vPages.push_back(std::make_pair("general", _("General")));
    vPages.push_back(std::make_pair("interface", _("Interface")));
    vPages.push_back(std::make_pair("debug", _("Debug")));

    OPTIONS.clear();

    OPTIONS["USE_CELSIUS"] =            cOpt("interface", _("Use Celsius"),
                                             _("Switch between Celsius and Fahrenheit."),
                                             "Fahrenheit,Celsius", "Fahrenheit"
                                            );

    OPTIONS["USE_METRIC_SPEEDS"] =      cOpt("interface", _("Use Metric Speeds"),
                                             _("Switch between Km/h and mph."),
                                             "mph,km/h", "mph"
                                            );

    OPTIONS["USE_METRIC_WEIGHTS"] =     cOpt("interface", _("Use Metric Weights"),
                                             _("Switch between kg and lbs."),
                                             "lbs,kg", "lbs"
                                            );

    OPTIONS["FORCE_CAPITAL_YN"] =       cOpt("interface", _("Force Y/N in prompts"),
                                             _("If true, Y/N prompts are case- sensitive and y and n are not accepted."),
                                             true
                                            );

    OPTIONS["NO_BRIGHT_BACKGROUNDS"] =  cOpt("interface", _("No Bright Backgrounds"),
                                            _("If true, bright backgrounds are not used--some consoles are not compatible."),
                                             false
                                            );

    OPTIONS["24_HOUR"] =                cOpt("interface", _("24 Hour Time"),
                                             _("12h: AM/PM, eg: 7:31 AM - Military: 24h Military, eg: 0731 - 24h: Normal 24h, eg: 7:31"),
                                             "12h,Military,24h", "12h"
                                            );

    OPTIONS["SNAP_TO_TARGET"] =         cOpt("interface", _("Snap to Target"),
                                             _("If true, automatically follow the crosshair when firing/throwing."),
                                             false
                                            );

    OPTIONS["SAFEMODE"] =               cOpt("general", _("Safemode on by default"),
                                             _("If true, safemode will be on after starting a new game or loading."),
                                             true
                                            );

    OPTIONS["SAFEMODEPROXIMITY"] =      cOpt("general", _("Safemode proximity distance"),
                                             _("If safemode is enabled, distance to hostiles when safemode should show a warning. 0 = Max player viewdistance."),
                                             0, 50, 0
                                            );

    OPTIONS["AUTOSAFEMODE"] =           cOpt("general", _("Auto-Safemode on by default"),
                                             _("If true, auto-safemode will be on after starting a new game or loading."),
                                             false
                                            );

    OPTIONS["AUTOSAFEMODETURNS"] =      cOpt("general", _("Turns to reenable safemode"),
                                             _("Number of turns after safemode is reenabled if no hostiles are in safemodeproximity distance."),
                                             1, 100, 50
                                            );

    OPTIONS["AUTOSAVE"] =               cOpt("general", _("Periodically Autosave"),
                                             _("If true, game will periodically save the map."),
                                             false
                                            );

    OPTIONS["AUTOSAVE_TURNS"] =         cOpt("general", _("Game turns between autosaves"),
                                             _("Number of game turns between autosaves"),
                                             1, 30, 5
                                            );

    OPTIONS["AUTOSAVE_MINUTES"] =       cOpt("general", _("Real minutes between autosaves"),
                                             _("Number of real time minutes between autosaves"),
                                             0, 127, 5
                                            );

    OPTIONS["RAIN_ANIMATION"] =         cOpt("interface", _("Rain animation"),
                                             _("If true, will display weather animations."),
                                             true
                                            );

    OPTIONS["CIRCLEDIST"] =             cOpt("general", _("Circular distances"),
                                             _("If true, the game will calculate range in a realistic way: light sources will be circles diagonal movement will cover more ground and take longer. If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall."),
                                             false
                                            );

    OPTIONS["QUERY_DISASSEMBLE"] =      cOpt("interface", _("Query on disassembly"),
                                             _("If true, will query before disassembling items."),
                                             true
                                            );

    OPTIONS["DROP_EMPTY"] =             cOpt("general", _("Drop empty containers"),
                                             _("Set to drop empty containers after use. No: Don't drop any. - Watertight: All except watertight containers. - All: Drop all containers."),
                                             "No,Watertight,All", "No"
                                            );

    OPTIONS["SKILL_RUST"] =             cOpt("debug", _("Skill Rust"),
                                             _("Set the level of skill rust. Vanilla: Vanilla Cataclysm - Capped: Capped at skill levels 2 - Int: Intelligence dependent - IntCap: Intelligence dependent, capped - Off: None at all."),
                                             "Vanilla,Capped,Int,IntCap,Off", "Vanilla"
                                            );

    OPTIONS["DELETE_WORLD"] =           cOpt("general", _("Delete World"),
                                             _("Delete world upon player death."),
                                             "No,Yes,Query", "No"
                                            );

    OPTIONS["INITIAL_POINTS"] =         cOpt("debug", _("Initial points"),
                                             _("Initial points available on character generation."),
                                             0, 25, 6
                                            );

    OPTIONS["MAX_TRAIT_POINTS"] =       cOpt("debug", _("Maximum trait points"),
                                             _("Maximum trait points available for character generation."),
                                             0, 25, 12
                                            );

    OPTIONS["SPAWN_DENSITY"] =          cOpt("general", _("Spawn rate scaling factor"),
                                             _("A scaling factor that determines density of monster spawns."),
                                             0.0, 50.0, 1.0, 0.1
                                            );

    OPTIONS["CITY_SIZE"] =              cOpt("general", _("Size of cities"),
                                             _("A number determining how large cities are. Warning, large numbers lead to very slow mapgen."),
                                             4, 16, 4
                                            );

    OPTIONS["INITIAL_TIME"] =           cOpt("debug", _("Initial time"),
                                             _("Initial starting time of day on character generation."),
                                             0, 23, 8
                                            );

    OPTIONS["VIEWPORT_X"] =             cOpt("interface", _("Viewport width"),
                                             _("SDL ONLY: Set the expansion of the viewport along the X axis. Requires restart. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["VIEWPORT_Y"] =             cOpt("interface", _("Viewport height"),
                                             _("SDL ONLY: Set the expansion of the viewport along the Y axis. Requires restart. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["SIDEBAR_STYLE"] =          cOpt("interface", _("Sidebar style"),
                                             _("Switch between the standard or a narrower and taller sidebar. Requires restart."),
                                             "Standard,Narrow", "Standard"
                                            );

    OPTIONS["MOVE_VIEW_OFFSET"] =       cOpt("interface", _("Move view offset"),
                                             _("Move view by how many squares per keypress."),
                                             1, 50, 1
                                            );

    OPTIONS["STATIC_SPAWN"] =           cOpt("debug", _("Static spawn"),
                                             _("Spawn zombies at game start instead of during game. Must reset world directory after changing for it to take effect."),
                                             true
                                            );

    OPTIONS["CLASSIC_ZOMBIES"] =        cOpt("debug", _("Classic zombies"),
                                             _("Only spawn classic zombies and natural wildlife. Requires a reset of save folder to take effect. This disables certain buildings."),
                                             false
                                            );

    OPTIONS["REVIVE_ZOMBIES"] =         cOpt("debug", _("Revive zombies"),
                                             _("Allow zombies to revive after a certain amount of time."),
                                             true
                                            );

    OPTIONS["SEASON_LENGTH"] =          cOpt("debug", _("Season length"),
                                             _("Season length, in days."),
                                             14, 127, 14
                                            );

    OPTIONS["STATIC_NPC"] =             cOpt("debug", _("Static npcs"),
                                             _("If true, the game will spawn static NPC at the start of the game, requires world reset."),
                                             false
                                            );

    OPTIONS["RANDOM_NPC"] =             cOpt("debug", _("Random npcs"),
                                             _("If true, the game will randomly spawn NPC during gameplay."),
                                             false
                                            );

    OPTIONS["RAD_MUTATION"] =           cOpt("general", _("Mutations by radiation"),
                                             _("If true, radiation causes the player to mutate."),
                                             true
                                            );

    OPTIONS["SAVE_SLEEP"] =             cOpt("interface", _("Ask to save before sleeping"),
                                             _("If true, game will ask to save the map before sleeping."),
                                             false
                                            );

    OPTIONS["HIDE_CURSOR"] =            cOpt("interface", _("Hide Mouse Cursor"),
                                             _("Always: Cursor is always shown. Hidden: Cursor is hidden. HiddenKB: Cursor is hidden on keyboard input and unhidden on mouse movement."),
                                             "Always,Hidden,HiddenKB", "Always"
                                            );

    OPTIONS["MENU_SCROLL"] =            cOpt("interface", _("Centered menu scrolling"),
                                             _("If true, menus will start scrolling in the center of the list, and keep the list centered."),
                                             true
                                            );

    OPTIONS["AUTO_PICKUP"] =            cOpt("general", _("Enable item Auto Pickup"),
                                             _("Enable item auto pickup. Change pickup rules with the Auto Pickup Manager in the Help Menu ?3"),
                                             false
                                            );

    OPTIONS["AUTO_PICKUP_ZERO"] =       cOpt("general", _("Auto Pickup 0 Vol light items"),
                                             _("Auto pickup items with 0 Volume, and weight less than or equal to [option] * 50 grams. '0' disables this option"),
                                             0, 20, 0
                                            );

    OPTIONS["AUTO_PICKUP_SAFEMODE"] =   cOpt("general", _("Auto Pickup Safemode"),
                                             _("Auto pickup is disabled as long as you can see monsters nearby. This is affected by Safemode proximity distance."),
                                             false
                                            );

    OPTIONS["DANGEROUS_PICKUPS"] =      cOpt("general", _("Dangerous pickups"),
                                             _("If false will cause player to drop new items that cause them to exceed the weight limit."),
                                             false
                                            );

    OPTIONS["SHIFT_LIST_ITEM_VIEW"] =   cOpt("interface", _("Shift list item view"),
                                             _("If true, shift the view toward the selected item if it is outside of your current viewport."),
                                             true
                                            );

    OPTIONS["USE_TILES"] =              cOpt("interface", _("Use Tiles"),
                                             _("If true, replaces some TTF rendered text with Tiles. Only applicable on SDL builds. Requires restart."),
                                             true
                                             );

    for (std::map<std::string, cOpt>::iterator iter = OPTIONS.begin(); iter != OPTIONS.end(); ++iter) {
        for (int i=0; i < vPages.size(); ++i) {
            if (vPages[i].first == (iter->second).getPage()) {
                mPageItems[i].push_back(iter->first);
                break;
            }
        }
    }
}

void game::show_options()
{
    std::map<std::string, cOpt> OPTIONS_OLD = OPTIONS;

    const int iTooltipHeight = 3;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[60] = true;
    //mapLines[68] = true;

    WINDOW* w_options_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    WINDOW* w_options_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW* w_options_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY, 1 + iOffsetX);
    WINDOW* w_options = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    wborder(w_options_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwputch(w_options_border, 4,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_options_border, 4, 79, c_ltgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_options_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, c_ltgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_options_border, 0, 36, c_ltred, _(" OPTIONS "));
    wrefresh(w_options_border);

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_options_header, 0, i, c_ltgray, LINE_OXXX);
        } else {
            mvwputch(w_options_header, 0, i, c_ltgray, LINE_OXOX); // Draw header line
        }
    }

    //mvwprintz(w_options_header, 3, 0, c_white, "#");
    //mvwprintz(w_options_header, 3, 7, c_white, _("Option"));
    //mvwprintz(w_options_header, 3, 52, c_white, _("Value"));

    wrefresh(w_options_header);

    int iCurrentPage = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;
    bool bStuffChanged = false;
    char ch = ' ';

    std::stringstream sTemp;

    do {
        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_options, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w_options, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_options_tooltip, i, j, c_black, ' ');
                }
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, mPageItems[iCurrentPage].size());

        //Draw options
        for (int i = iStartPos; i < iStartPos + ((iContentHeight > mPageItems[iCurrentPage].size()) ? mPageItems[iCurrentPage].size() : iContentHeight); i++) {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << i + 1;
            mvwprintz(w_options, i - iStartPos, 0, c_white, sTemp.str().c_str());
            mvwprintz(w_options, i - iStartPos, 4, c_white, "");

            if (iCurrentLine == i) {
                wprintz(w_options, c_yellow, ">> ");
            } else {
                wprintz(w_options, c_yellow, "   ");
            }

            wprintz(w_options, c_white, "%s", (OPTIONS[mPageItems[iCurrentPage][i]].getMenuText()).c_str());

            if (OPTIONS[mPageItems[iCurrentPage][i]].getValue() == "False") {
                cLineColor = c_ltred;
            }

            mvwprintz(w_options, i - iStartPos, 62, (iCurrentLine == i) ? hilite(cLineColor) : cLineColor, "%s", (OPTIONS[mPageItems[iCurrentPage][i]].getValue()).c_str());
        }

        //Draw Scrollbar
        draw_scrollbar(w_options_border, iCurrentLine, iContentHeight, mPageItems[iCurrentPage].size(), 5);

        //Draw Tabs
        mvwprintz(w_options_header, 0, 7, c_white, "");
        for (int i = 0; i < vPages.size(); i++) {
            if (mPageItems[i].size() > 0) { //skip empty pages
                wprintz(w_options_header, c_white, "[");
                wprintz(w_options_header, (iCurrentPage == i) ? hilite(c_ltgreen) : c_ltgreen, (vPages[i].second).c_str());
                wprintz(w_options_header, c_white, "]");
                wputch(w_options_header, c_white, LINE_OXOX);
            }
        }

        wrefresh(w_options_header);

        fold_and_print(w_options_tooltip, 0, 0, 78, c_white, "%s", (OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip() + "  #Default: " + OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText()).c_str());
        wrefresh(w_options_tooltip);

        wrefresh(w_options);

        ch = input();

        if (mPageItems[iCurrentPage].size() > 0 || ch == '\t') {
            switch(ch) {
                case 'j': //move down
                    iCurrentLine++;
                    if (iCurrentLine >= mPageItems[iCurrentPage].size()) {
                        iCurrentLine = 0;
                    }
                    break;
                case 'k': //move up
                    iCurrentLine--;
                    if (iCurrentLine < 0) {
                        iCurrentLine = mPageItems[iCurrentPage].size()-1;
                    }
                    break;
                case 'l': //set to prev value
                    OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setNext();
                    bStuffChanged = true;
                    break;
                case 'h': //set to next value
                    OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setPrev();
                    bStuffChanged = true;
                    break;
                case '>':
                case '\t': //Switch to next Page
                    iCurrentLine = 0;
                    do { //skip empty pages
                        iCurrentPage++;
                        if (iCurrentPage >= vPages.size()) {
                            iCurrentPage = 0;
                        }
                    } while(mPageItems[iCurrentPage].size() == 0);

                    break;
                case '<':
                    iCurrentLine = 0;
                    do { //skip empty pages
                        iCurrentPage--;
                        if (iCurrentPage < 0) {
                            iCurrentPage = vPages.size()-1;
                        }
                    } while(mPageItems[iCurrentPage].size() == 0);
                    break;
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if (bStuffChanged) {
        if(query_yn(_("Save changes?"))) {
            save_options();
        } else {
            OPTIONS = OPTIONS_OLD;
        }
    }

    werase(w_options);
    werase(w_options_border);
    werase(w_options_header);
    werase(w_options_tooltip);
}

void load_options()
{
    std::ifstream fin;
    fin.open("data/options.txt");
    if(!fin.is_open()) {
        fin.close();
        save_options();
        fin.open("data/options.txt");
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create ./data/options.txt\n";
            return;
        }
    }

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if(sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1) {
            int iPos = sLine.find(' ');
            OPTIONS[sLine.substr(0, iPos)].setValue(sLine.substr(iPos+1, sLine.length()));
        }
    }

    fin.close();

    trigdist = OPTIONS["CIRCLEDIST"]; // cache to global due to heavy usage.
    use_tiles = OPTIONS["USE_TILES"]; // cache to global due to heavy usage.
}

std::string options_header()
{
    return "\
# This is the options file.  It works similarly to keymap.txt: the format is\n\
# <option name> <option value>\n\
# <option value> may be any number, positive or negative.  If you use a\n\
# negative sign, do not put a space between it and the number.\n\
#\n\
# If # is at the start of a line, it is considered a comment and is ignored.\n\
# In-line commenting is not allowed.  I think.\n\
#\n\
# If you want to restore the default options, simply delete this file.\n\
# A new options.txt will be created next time you play.\n\
\n\
";
}

void save_options()
{
    std::ofstream fout;
    fout.open("data/options.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << options_header() << std::endl;

    for(int j = 0; j < vPages.size(); j++) {
        for(int i = 0; i < mPageItems[j].size(); i++) {
            fout << "#" << OPTIONS[mPageItems[j][i]].getTooltip() << std::endl;
            fout << "#Default: " << OPTIONS[mPageItems[j][i]].getDefaultText() << std::endl;
            fout << mPageItems[j][i] << " " << OPTIONS[mPageItems[j][i]].getValue() << std::endl << std::endl;
        }
    }

    fout.close();
}
