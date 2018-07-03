#include "auto_pickup.h"
#include "game.h"
#include "player.h"
#include "output.h"
#include "json.h"
#include "debug.h"
#include "item_factory.h"
#include "translations.h"
#include "cata_utility.h"
#include "path_info.h"
#include "string_formatter.h"
#include "filesystem.h"
#include "input.h"
#include "options.h"
#include "itype.h"
#include "string_input_popup.h"

#include <stdlib.h>
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
    show( _(" AUTO PICKUP MANAGER "), true );
}

void auto_pickup::show( const std::string &custom_name, bool is_autopickup )
{
    auto vRulesOld = vRules;

    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    const int iTotalCols = 2;

    catacurses::window w_help = catacurses::newwin( ( FULL_SCREEN_HEIGHT / 2 ) - 2, FULL_SCREEN_WIDTH * 3 / 4,
                                        7 + iOffsetY + (FULL_SCREEN_HEIGHT / 2) / 2, iOffsetX + 19 / 2);

    catacurses::window w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX );
    catacurses::window w_header = catacurses::newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                          1 + iOffsetX);
    catacurses::window w = catacurses::newwin( iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY,
                                   1 + iOffsetX);

    /**
     * All of the stuff in this lambda needs to be drawn (1) initially, and
     * (2) after closing the HELP_KEYBINDINGS window (since it mangles the screen)
    */
    const auto initial_draw = [&]() {
        // Redraw the border
        draw_border( w_border, BORDER_COLOR, custom_name );
        mvwputch( w_border, 3,  0, c_light_gray, LINE_XXXO) ; // |-
        mvwputch( w_border, 3, 79, c_light_gray, LINE_XOXX ); // -|
        mvwputch( w_border, FULL_SCREEN_HEIGHT - 1, 5, c_light_gray, LINE_XXOX ); // _|_
        mvwputch( w_border, FULL_SCREEN_HEIGHT - 1, 51, c_light_gray, LINE_XXOX );
        mvwputch( w_border, FULL_SCREEN_HEIGHT - 1, 61, c_light_gray, LINE_XXOX );
        wrefresh( w_border );

        // Redraw the header
        int tmpx = 0;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<A>dd" ) ) + 2;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<R>emove" ) ) + 2;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<C>opy" ) ) + 2;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _("<M>ove" ) ) + 2;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<E>nable" ) ) + 2;
        tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<D>isable" ) ) + 2;
        if( !g->u.name.empty() ) {
            shortcut_print( w_header, 0, tmpx, c_white, c_light_green, _( "<T>est" ) );
        }
        tmpx = 0;
        tmpx += shortcut_print( w_header, 1, tmpx, c_white, c_light_green,
                                _( "<+-> Move up/down" ) ) + 2;
        tmpx += shortcut_print( w_header, 1, tmpx, c_white, c_light_green, _( "<Enter>-Edit" ) ) + 2;
        shortcut_print( w_header, 1, tmpx, c_white, c_light_green, _( "<Tab>-Switch Page" ) );

        for( int i = 0; i < 78; i++ ) {
            if( i == 4 || i == 50 || i == 60 ) {
                mvwputch( w_header, 2, i, c_light_gray, LINE_OXXX );
                mvwputch( w_header, 3, i, c_light_gray, LINE_XOXO );
            } else {
                mvwputch( w_header, 2, i, c_light_gray, LINE_OXOX ); // Draw line under header
            }
        }
        mvwprintz( w_header, 3, 1, c_white, "#" );
        mvwprintz( w_header, 3, 8, c_white, _( "Rules" ) );
        mvwprintz( w_header, 3, 52, c_white, _( "I/E" ) );
        wrefresh( w_header );
    };

    initial_draw();
    int iTab = GLOBAL_TAB;
    int iLine = 0;
    int iColumn = 1;
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
    ctxt.register_action("ENABLE_RULE");
    ctxt.register_action("DISABLE_RULE");
    ctxt.register_action("MOVE_RULE_UP");
    ctxt.register_action("MOVE_RULE_DOWN");
    ctxt.register_action("TEST_RULE");
    ctxt.register_action("HELP_KEYBINDINGS");

    if( is_autopickup ) {
        ctxt.register_action("SWITCH_AUTO_PICKUP_OPTION");
        ctxt.register_action("SWAP_RULE_GLOBAL_CHAR");
    }

    std::ostringstream sTemp;

    while(true) {
        int locx = 17;
        locx += shortcut_print(w_header, 2, locx, c_white,
                               (iTab == GLOBAL_TAB) ? hilite(c_white) : c_white, _("[<Global>]")) + 1;
        shortcut_print(w_header, 2, locx, c_white,
                       (iTab == CHARACTER_TAB) ? hilite(c_white) : c_white, _("[<Character>]"));

        locx = 55;
        mvwprintz(w_header, 0, locx, c_white, _("Auto pickup enabled:"));
        locx += shortcut_print(w_header, 1, locx,
                               (get_option<bool>( "AUTO_PICKUP" ) ? c_light_green : c_light_red), c_white,
                               (get_option<bool>( "AUTO_PICKUP" ) ? _("True") : _("False")));
        locx += shortcut_print(w_header, 1, locx, c_white, c_light_green, "  ");
        locx += shortcut_print(w_header, 1, locx, c_white, c_light_green, _("<S>witch"));
        shortcut_print(w_header, 1, locx, c_white, c_light_green, "  ");

        wrefresh(w_header);

        // Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if( j == 4 || j == 50 || j == 60 ) {
                    mvwputch(w, i, j, c_light_gray, LINE_XOXO);
                } else {
                    mvwputch(w, i, j, c_black, ' ');
                }
            }
        }

        const bool currentPageNonEmpty = !vRules[iTab].empty();

        if( iTab == CHARACTER_TAB && g->u.name.empty() ) {
            vRules[CHARACTER_TAB].clear();
            mvwprintz(w, 8, 15, c_white,
                      _("Please load a character first to use this page!"));
        }

        draw_scrollbar(w_border, iLine, iContentHeight,
                       vRules[iTab].size(), 5);
        wrefresh(w_border);

        calcStartPos(iStartPos, iLine, iContentHeight,
                     vRules[iTab].size());

        // display auto pickup
        for (int i = iStartPos; i < (int)vRules[iTab].size(); i++) {
            if (i >= iStartPos &&
                i < iStartPos + ((iContentHeight > (int)vRules[iTab].size()) ?
                                 (int)vRules[iTab].size() : iContentHeight)) {
                nc_color cLineColor = (vRules[iTab][i].bActive) ?
                                      c_white : c_light_gray;

                sTemp.str("");
                sTemp << i + 1;
                mvwprintz( w, i - iStartPos, 1, cLineColor, sTemp.str() );
                mvwprintz(w, i - iStartPos, 5, cLineColor, "");

                if (iLine == i) {
                    wprintz(w, c_yellow, ">> ");
                } else {
                    wprintz(w, c_yellow, "   ");
                }

                wprintz(w, (iLine == i &&
                                        iColumn == 1) ? hilite(cLineColor) : cLineColor, "%s",
                        ( ( vRules[iTab][i].sRule.empty() ) ? _( "<empty rule>" ) :
                         vRules[iTab][i].sRule).c_str() );

                mvwprintz(w, i - iStartPos, 52, (iLine == i &&
                          iColumn == 2) ? hilite(cLineColor) : cLineColor, "%s",
                          ((vRules[iTab][i].bExclude) ? _("Exclude") :  _("Include")));
            }
        }

        wrefresh(w);

        const std::string action = ctxt.handle_input();

        if (action == "NEXT_TAB") {
            iTab++;
            if (iTab >= MAX_TAB) {
                iTab = 0;
                iLine = 0;
            }
        } else if (action == "PREV_TAB") {
            iTab--;
            if (iTab < 0) {
                iTab = MAX_TAB - 1;
                iLine = 0;
            }
        } else if (action == "QUIT") {
            break;
        } else if (iTab == CHARACTER_TAB && g->u.name.empty()) {
            //Only allow loaded games to use the char sheet
        } else if (action == "DOWN") {
            iLine++;
            iColumn = 1;
            if (iLine >= (int)vRules[iTab].size()) {
                iLine = 0;
            }
        } else if (action == "UP") {
            iLine--;
            iColumn = 1;
            if (iLine < 0) {
                iLine = vRules[iTab].size() - 1;
            }
        } else if (action == "REMOVE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iTab].erase(vRules[iTab].begin() + iLine);
            if (iLine > (int)vRules[iTab].size() - 1) {
                iLine--;
            }
            if(iLine < 0){
                iLine = 0;
            }
        } else if (action == "COPY_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iTab].push_back(cRules(
                        vRules[iTab][iLine].sRule,
                        vRules[iTab][iLine].bActive,
                        vRules[iTab][iLine].bExclude));
            iLine = vRules[iTab].size() - 1;
        } else if (action == "SWAP_RULE_GLOBAL_CHAR" && currentPageNonEmpty) {
            if( ( iTab == GLOBAL_TAB && !g->u.name.empty() ) || iTab == CHARACTER_TAB ) {
                bStuffChanged = true;
                //copy over
                vRules[(iTab == GLOBAL_TAB) ? CHARACTER_TAB : GLOBAL_TAB].push_back(cRules(
                            vRules[iTab][iLine].sRule,
                            vRules[iTab][iLine].bActive,
                            vRules[iTab][iLine].bExclude));

                //remove old
                vRules[iTab].erase(vRules[iTab].begin() + iLine);
                iLine = vRules[(iTab == GLOBAL_TAB) ? CHARACTER_TAB : GLOBAL_TAB].size() - 1;
                iTab = (iTab == GLOBAL_TAB) ? CHARACTER_TAB : GLOBAL_TAB;
            }
        } else if( ( action == "ADD_RULE" ) || ( action == "CONFIRM" && currentPageNonEmpty ) ) {
            const int old_iLine = iLine;
            if( action == "ADD_RULE" ) {
                 vRules[iTab].push_back( cRules( "", true, false ) );
                 iLine = vRules[iTab].size() - 1;
            }

            if( iColumn == 1 || action == "ADD_RULE" ) {
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
                const std::string r = string_input_popup()
                                      .title( _( "Pickup Rule:" ) )
                                      .width( 30 )
                                      .text( vRules[iTab][iLine].sRule )
                                      .query_string();
                // If r is empty, then either (1) The player ESC'ed from the window (changed their mind), or
                // (2) Explicitly entered an empty rule- which isn't allowed since "*" should be used
                // to include/exclude everything
                if( !r.empty() ) {
                    vRules[iTab][iLine].sRule = wildcard_trim_rule( r );
                    bStuffChanged = true;
                } else if( action == "ADD_RULE" ) {
                    vRules[iTab].pop_back();
                    iLine = old_iLine;
                }
            } else if (iColumn == 2) {
                bStuffChanged = true;
                vRules[iTab][iLine].bExclude = !vRules[iTab][iLine].bExclude;
            }
        } else if (action == "ENABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iTab][iLine].bActive = true;
        } else if (action == "DISABLE_RULE" && currentPageNonEmpty) {
            bStuffChanged = true;
            vRules[iTab][iLine].bActive = false;
        } else if (action == "LEFT") {
            iColumn--;
            if (iColumn < 1) {
                iColumn = iTotalCols;
            }
        } else if (action == "RIGHT") {
            iColumn++;
            if (iColumn > iTotalCols) {
                iColumn = 1;
            }
        } else if (action == "MOVE_RULE_UP" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iLine < (int)vRules[iTab].size() - 1) {
                std::swap(vRules[iTab][iLine],
                          vRules[iTab][iLine + 1]);
                iLine++;
                iColumn = 1;
            }
        } else if (action == "MOVE_RULE_DOWN" && currentPageNonEmpty) {
            bStuffChanged = true;
            if (iLine > 0) {
                std::swap(vRules[iTab][iLine],
                          vRules[iTab][iLine - 1]);
                iLine--;
                iColumn = 1;
            }
        } else if (action == "TEST_RULE" && currentPageNonEmpty && !g->u.name.empty() ) {
            test_pattern(iTab, iLine);
        } else if (action == "SWITCH_AUTO_PICKUP_OPTION") {
            // @todo: Now that NPCs use this function, it could be used for them too
            get_options().get_option( "AUTO_PICKUP" ).setNext();
            get_options().save();
        } else if( action == "HELP_KEYBINDINGS" ) {
            initial_draw(); // de-mangle parts of the screen
        }
    }

    if( !bStuffChanged ) {
        return;
    }

    if( query_yn( _("Save changes?") ) ) {
        // NPC pickup rules don't need to be saved explicitly
        if( is_autopickup ) {
            save_global();
            if( !g->u.name.empty() ) {
                save_character();
            }
        }

        ready = false;
    } else {
        vRules = vRulesOld;
    }
}

void auto_pickup::test_pattern(const int iTab, const int iRow)
{
    std::vector<std::string> vMatchingItems;
    std::string sItemName = "";

    if ( vRules[iTab][iRow].sRule.empty() ) {
        return;
    }

    //Loop through all itemfactory items
    //APU now ignores prefixes, bottled items and suffix combinations still not generated
    for( const itype *e : item_controller->all() ) {
        sItemName = e->nname(1);
        if (vRules[iTab][iRow].bActive &&
            wildcard_match(sItemName, vRules[iTab][iRow].sRule)) {
            vMatchingItems.push_back(sItemName);
        }
    }

    const int iOffsetX = 15 + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    const int iOffsetY = 5 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0);

    int iStartPos = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 8;
    const int iContentWidth = FULL_SCREEN_WIDTH - 30;
    std::ostringstream sTemp;

    catacurses::window w_test_rule_border = catacurses::newwin( iContentHeight + 2, iContentWidth, iOffsetY, iOffsetX );
    catacurses::window w_test_rule_content = catacurses::newwin( iContentHeight, iContentWidth - 2, 1 + iOffsetY, 1 + iOffsetX );

    int nmatch = vMatchingItems.size();
    std::string buf = string_format(ngettext("%1$d item matches: %2$s", "%1$d items match: %2$s",
                                    nmatch), nmatch, vRules[iTab][iRow].sRule.c_str());
    draw_border( w_test_rule_border, BORDER_COLOR, buf, hilite( c_white ) );
    center_print( w_test_rule_border, iContentHeight + 1, red_background( c_white ),
                  _( "Won't display bottled and suffixes=(fits)" ) );
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
                mvwprintz( w_test_rule_content, i - iStartPos, 0, cLineColor, sTemp.str() );
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

bool auto_pickup::has_rule(const std::string &sRule)
{
    for( auto &elem : vRules[CHARACTER_TAB] ) {
        if( sRule.length() == elem.sRule.length() && ci_find_substr( sRule, elem.sRule ) != -1 ) {
            return true;
        }
    }
    return false;
}

void auto_pickup::add_rule(const std::string &sRule)
{
    vRules[CHARACTER_TAB].push_back(cRules(sRule, true, false));
    create_rule(sRule);

    if (!get_option<bool>( "AUTO_PICKUP" ) &&
        query_yn(_("Autopickup is not enabled in the options. Enable it now?")) ) {
        get_options().get_option( "AUTO_PICKUP" ).setNext();
        get_options().save();
    }
}

void auto_pickup::remove_rule(const std::string &sRule)
{
    for (auto it = vRules[CHARACTER_TAB].begin();
         it != vRules[CHARACTER_TAB].end(); ++it) {
        if (sRule.length() == it->sRule.length() &&
            ci_find_substr(sRule, it->sRule) != -1) {
            vRules[CHARACTER_TAB].erase(it);
            ready = false;
            break;
        }
    }
}

bool auto_pickup::empty() const
{
    for( int i = GLOBAL_TAB; i < MAX_TAB; i++ ) {
        if ( !vRules[i].empty() ) {
            return false;
        }
    }

    return true;
}

void auto_pickup::create_rule( const std::string &to_match )
{
    for( int i = GLOBAL_TAB; i < MAX_TAB; i++ ) {
        for( auto &elem : vRules[i] ) {
            if( !elem.bExclude ) {
                if( elem.bActive && wildcard_match( to_match, elem.sRule ) ) {
                    map_items[ to_match ] = RULE_WHITELISTED;
                }
            } else {
                if( elem.bActive && wildcard_match( to_match, elem.sRule ) ) {
                    map_items[ to_match ] = RULE_BLACKLISTED;
                }
            }
        }
    }
}

void auto_pickup::refresh_map_items() const
{
    map_items.clear();

    //process include/exclude in order of rules, global first, then character specific
    //if a specific item is being added, all the rules need to be checked now
    //may have some performance issues since exclusion needs to check all items also
    for( int i = GLOBAL_TAB; i < MAX_TAB; i++ ) {
        for( auto &elem : vRules[i] ) {
            if ( !elem.sRule.empty() ) {
                if( !elem.bExclude ) {
                    //Check include patterns against all itemfactory items
                    for( const itype *e : item_controller->all() ) {
                        const std::string &cur_item = e->nname( 1 );
                        if( elem.bActive && wildcard_match( cur_item, elem.sRule ) ) {
                            map_items[ cur_item ] = RULE_WHITELISTED;
                        }
                    }
                } else {
                    //only re-exclude items from the existing mapping for now
                    //new exclusions will process during pickup attempts
                    for (auto iter = map_items.begin(); iter != map_items.end(); ++iter) {
                        if( elem.bActive && wildcard_match( iter->first, elem.sRule ) ) {
                            map_items[ iter->first ] = RULE_BLACKLISTED;
                        }
                    }
                }
            }
        }
    }

    ready = true;
}

rule_state auto_pickup::check_item( const std::string &sItemName ) const
{
    if( !ready ) {
        refresh_map_items();
    }

    const auto iter = map_items.find( sItemName );
    if( iter != map_items.end() ) {
        return iter->second;
    }

    return RULE_NONE;
}

void auto_pickup::clear_character_rules()
{
    vRules[CHARACTER_TAB].clear();
    ready = false;
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

        if (bCharacter) {
            savefile = g->get_player_base_save_path() + ".apu.json";

            const std::string player_save = g->get_player_base_save_path() + ".sav";
            if( !file_exist( player_save ) ) {
                return true; //Character not saved yet.
            }
        }

    return write_to_file( savefile, [&]( std::ostream &fout ) {
        JsonOut jout( fout, true );
        serialize(jout);
    }, _( "autopickup configuration" ) );
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

    std::string sFile = FILENAMES["autopickup"];
    if (bCharacter) {
        sFile = g->get_player_base_save_path() + ".apu.json";
    }

    if( !read_from_file_optional_json( sFile, [this]( JsonIn &jsin ) { deserialize( jsin ); } ) ) {
        if (load_legacy(bCharacter)) {
            if (save(bCharacter)) {
                remove_file(sFile);
            }
        }
    }

    ready = false;
}

void auto_pickup::serialize(JsonOut &json) const
{
    json.start_array();

    for( auto &elem : vRules[( bChar ) ? CHARACTER_TAB : GLOBAL_TAB] ) {
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
    vRules[(bChar) ? CHARACTER_TAB : GLOBAL_TAB].clear();
    ready = false;

    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject jo = jsin.get_object();

        const std::string sRule = jo.get_string("rule");
        const bool bActive = jo.get_bool("active");
        const bool bExclude = jo.get_bool("exclude");

        vRules[(bChar) ? CHARACTER_TAB : GLOBAL_TAB].push_back(cRules(sRule, bActive, bExclude));
    }
}

bool auto_pickup::load_legacy(const bool bCharacter)
{
    std::string sFile = FILENAMES["legacy_autopickup2"];

    if (bCharacter) {
        sFile = g->get_player_base_save_path() + ".apu.txt";
    }

    auto &rules = vRules[(bCharacter) ? CHARACTER_TAB : GLOBAL_TAB];

    using namespace std::placeholders;
    const auto& reader = std::bind( &auto_pickup::load_legacy_rules, this, std::ref( rules ), _1 );
    if( !read_from_file_optional( sFile, reader ) ) {
        if( !bCharacter ) {
            return read_from_file_optional( FILENAMES["legacy_autopickup"], reader );
        } else {
            return false;
        }
    }

    return true;
}

void auto_pickup::load_legacy_rules( std::vector<cRules> &rules, std::istream &fin )
{
    rules.clear();
    ready = false;

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if(!sLine.empty() && sLine[0] != '#') {
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
                    iPos = sLine.find( ';' );

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

                rules.push_back(cRules(sRule, bActive, bExclude));
            }
        }
    }
}
