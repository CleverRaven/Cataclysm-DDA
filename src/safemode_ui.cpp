#include "safemode_ui.h"

#include "game.h"
#include "player.h"
#include "output.h"
#include "debug.h"
#include "catacharset.h"
#include "translations.h"
#include "cata_utility.h"
#include "path_info.h"
#include "filesystem.h"
#include "input.h"
#include "mtype.h"
#include "generic_factory.h"
#include "worldfactory.h"
#include "monstergenerator.h"
#include "debug.h"

#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>

safemode &get_safemode()
{
    static safemode single_instance;
    return single_instance;
}

void safemode::show()
{
    show( _( " SAFEMODE MANAGER " ), true );
}

void safemode::show( const std::string &custom_name, bool is_safemode )
{
    auto vRulesOld = vRules;

    const int iHeaderHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 2 - iHeaderHeight;

    const int iOffsetX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    const int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    enum enCols : int {
        col_Rule,
        col_Attitude,
        col_Dist,
        col_IE,
    };

    std::map<int, int> mapPos;
    mapPos[col_Rule] = 4;
    mapPos[col_Attitude] = 50;
    mapPos[col_Dist] = 61;
    mapPos[col_IE] = 68;

    const int iTotalCols = mapPos.size();

    WINDOW *w_help = newwin( ( FULL_SCREEN_HEIGHT / 2 ) - 2, FULL_SCREEN_WIDTH * 3 / 4,
                             7 + iOffsetY + ( FULL_SCREEN_HEIGHT / 2 ) / 2, iOffsetX + 19 / 2 );
    WINDOW_PTR w_helpptr( w_help );

    WINDOW *w_border = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX );
    WINDOW_PTR w_borderptr( w_border );
    WINDOW *w_header = newwin( iHeaderHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                               1 + iOffsetX );
    WINDOW_PTR w_headerptr( w_header );
    WINDOW *w = newwin( iContentHeight, FULL_SCREEN_WIDTH - 2, iHeaderHeight + 1 + iOffsetY,
                        1 + iOffsetX );
    WINDOW_PTR wptr( w );

    draw_border( w_border, BORDER_COLOR, custom_name );

    mvwputch( w_border, 3,  0, c_ltgray, LINE_XXXO ); // |-
    mvwputch( w_border, 3, 79, c_ltgray, LINE_XOXX ); // -|

    for( auto &mapLine : mapPos ) {
        mvwputch( w_border, FULL_SCREEN_HEIGHT - 1, mapLine.second + 1, c_ltgray,
                  LINE_XXOX ); // _|_
    }

    wrefresh( w_border );

    int tmpx = 0;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<A>dd" ) ) + 2;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<R>emove" ) ) + 2;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<C>opy" ) ) + 2;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<M>ove" ) ) + 2;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<E>nable" ) ) + 2;
    tmpx += shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<D>isable" ) ) + 2;
    shortcut_print( w_header, 0, tmpx, c_white, c_ltgreen, _( "<T>est" ) );
    tmpx = 0;
    tmpx += shortcut_print( w_header, 1, tmpx, c_white, c_ltgreen,
                            _( "<+-> Move up/down" ) ) + 2;
    tmpx += shortcut_print( w_header, 1, tmpx, c_white, c_ltgreen, _( "<Enter>-Edit" ) ) + 2;
    shortcut_print( w_header, 1, tmpx, c_white, c_ltgreen, _( "<Tab>-Switch Page" ) );

    for( int i = 0; i < 78; i++ ) {
        mvwputch( w_header, 2, i, c_ltgray, LINE_OXOX ); // Draw line under header
    }

    for( auto &mapLine : mapPos ) {
        mvwputch( w_header, 2, mapLine.second, c_ltgray, LINE_OXXX );
        mvwputch( w_header, 3, mapLine.second, c_ltgray, LINE_XOXO );
    }

    mvwprintz( w_header, 3, 1, c_white, "#" );
    mvwprintz( w_header, 3, mapPos[col_Rule] + 4, c_white,
               _( "Rules" ) ); //Creaturename with wildcard or NPC
    mvwprintz( w_header, 3, mapPos[col_Attitude] + 2, c_white,
               _( "Attitude" ) ); //Hostile/Neutral/Friendly
    mvwprintz( w_header, 3, mapPos[col_Dist] + 2, c_white, _( "Dist" ) ); //Proximity distance
    mvwprintz( w_header, 3, mapPos[col_IE] + 2, c_white, _( "I/E" ) ); //Include or Exclude rule

    wrefresh( w_header );

    int iTab = GLOBAL_TAB;
    int iLine = 0;
    int iColumn = 0;
    int iStartPos = 0;
    bool bStuffChanged = false;
    input_context ctxt( "SAFEMODE" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "ADD_RULE" );
    ctxt.register_action( "REMOVE_RULE" );
    ctxt.register_action( "COPY_RULE" );
    ctxt.register_action( "ENABLE_RULE" );
    ctxt.register_action( "DISABLE_RULE" );
    ctxt.register_action( "MOVE_RULE_UP" );
    ctxt.register_action( "MOVE_RULE_DOWN" );
    ctxt.register_action( "TEST_RULE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    if( is_safemode ) {
        ctxt.register_action( "SWITCH_SAFEMODE_OPTION" );
        ctxt.register_action( "SWAP_RULE_GLOBAL_CHAR" );
    }

    std::ostringstream sTemp;

    while( true ) {
        int locx = 17;
        locx += shortcut_print( w_header, 2, locx, c_white,
                                ( iTab == GLOBAL_TAB ) ? hilite( c_white ) : c_white, _( "[<Global>]" ) ) + 1;
        shortcut_print( w_header, 2, locx, c_white,
                        ( iTab == CHARACTER_TAB ) ? hilite( c_white ) : c_white, _( "[<Character>]" ) );

        locx = 55;
        mvwprintz( w_header, 0, locx, c_white, _( "Safemode enabled:" ) );
        locx += shortcut_print( w_header, 1, locx,
                                ( ( OPTIONS["SAFEMODE"] ) ? c_ltgreen : c_ltred ), c_white,
                                ( ( OPTIONS["SAFEMODE"] ) ? _( "True" ) : _( "False" ) ) );
        locx += shortcut_print( w_header, 1, locx, c_white, c_ltgreen, "  " );
        locx += shortcut_print( w_header, 1, locx, c_white, c_ltgreen, _( "<S>witch" ) );
        shortcut_print( w_header, 1, locx, c_white, c_ltgreen, "  " );

        wrefresh( w_header );

        // Clear the lines
        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < 79; j++ ) {
                mvwputch( w, i, j, c_black, ' ' );
            }

            for( auto &mapLine : mapPos ) {
                mvwputch( w, i, mapLine.second, c_ltgray, LINE_XOXO );
            }
        }

        const bool currentPageNonEmpty = !vRules[iTab].empty();

        if( iTab == CHARACTER_TAB && g->u.name == "" ) {
            vRules[CHARACTER_TAB].clear();
            mvwprintz( w, 8, 15, c_white, _( "Please load a character first to use this page!" ) );
        }

        draw_scrollbar( w_border, iLine, iContentHeight, vRules[iTab].size(), 5 );
        wrefresh( w_border );

        calcStartPos( iStartPos, iLine, iContentHeight, vRules[iTab].size() );

        // display safemode
        for( int i = iStartPos; i < ( int )vRules[iTab].size(); i++ ) {
            if( i >= iStartPos &&
                i < iStartPos + ( ( iContentHeight > ( int )vRules[iTab].size() ) ?
                                  ( int )vRules[iTab].size() : iContentHeight ) ) {
                nc_color cLineColor = ( vRules[iTab][i].bActive ) ? c_white : c_ltgray;

                sTemp.str( "" );
                sTemp << i + 1;
                mvwprintz( w, i - iStartPos, 1, cLineColor, "%s", sTemp.str().c_str() );
                mvwprintz( w, i - iStartPos, 5, cLineColor, "" );

                if( iLine == i ) {
                    wprintz( w, c_yellow, ">> " );
                } else {
                    wprintz( w, c_yellow, "   " );
                }

                wprintz( w, ( iLine == i && iColumn == col_Rule ) ? hilite( cLineColor ) : cLineColor,
                         "%s", ( ( vRules[iTab][i].sRule == "" ) ? _( "<empty rule>" ) : vRules[iTab][i].sRule ).c_str()
                       );

                auto attCreature = Creature::get_creature_attitude( ( Creature::Attitude )
                                   vRules[iTab][i].attCreature );
                mvwprintz( w, i - iStartPos, mapPos[col_Attitude] + 2,
                           ( iLine == i && iColumn == col_Attitude ) ? hilite( cLineColor ) : cLineColor,
                           "%s", ( attCreature.first ).c_str()
                         );

                mvwprintz( w, i - iStartPos, mapPos[col_Dist] + 2,
                           ( iLine == i && iColumn == col_Dist ) ? hilite( cLineColor ) : cLineColor,
                           "%s", ( !vRules[iTab][i].bExclude ) ? to_string( vRules[iTab][i].iProxyDist ).c_str() : "---"
                         );

                mvwprintz( w, i - iStartPos, mapPos[col_IE] + 2,
                           ( iLine == i && iColumn == col_IE ) ? hilite( cLineColor ) : cLineColor,
                           "%s", ( ( vRules[iTab][i].bExclude ) ? rm_prefix( _( "Exclude" ) ).c_str() : rm_prefix(
                                       _( "Include" ) ).c_str() )
                         );
            }
        }

        wrefresh( w );

        const std::string action = ctxt.handle_input();

        if( action == "NEXT_TAB" ) {
            iTab++;
            if( iTab >= MAX_TAB ) {
                iTab = 0;
                iLine = 0;
            }
        } else if( action == "PREV_TAB" ) {
            iTab--;
            if( iTab < 0 ) {
                iTab = MAX_TAB - 1;
                iLine = 0;
            }
        } else if( action == "QUIT" ) {
            break;
        } else if( iTab == CHARACTER_TAB && g->u.name.empty() ) {
            //Only allow loaded games to use the char sheet
        } else if( action == "DOWN" ) {
            iLine++;
            if( iLine >= ( int )vRules[iTab].size() ) {
                iLine = 0;
            }
        } else if( action == "UP" ) {
            iLine--;
            if( iLine < 0 ) {
                iLine = vRules[iTab].size() - 1;
            }
        } else if( action == "ADD_RULE" ) {
            bStuffChanged = true;
            vRules[iTab].push_back( cRules( "", true, false, Creature::A_HOSTILE,
                                            OPTIONS["SAFEMODEPROXIMITY"] ) );
            iLine = vRules[iTab].size() - 1;
        } else if( action == "REMOVE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            vRules[iTab].erase( vRules[iTab].begin() + iLine );
            if( iLine > ( int )vRules[iTab].size() - 1 ) {
                iLine--;
            }
            if( iLine < 0 ) {
                iLine = 0;
            }
        } else if( action == "COPY_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            vRules[iTab].push_back( cRules(
                                        vRules[iTab][iLine].sRule,
                                        vRules[iTab][iLine].bActive,
                                        vRules[iTab][iLine].bExclude,
                                        vRules[iTab][iLine].attCreature,
                                        vRules[iTab][iLine].iProxyDist ) );
            iLine = vRules[iTab].size() - 1;
        } else if( action == "SWAP_RULE_GLOBAL_CHAR" && currentPageNonEmpty ) {
            if( ( iTab == GLOBAL_TAB && g->u.name != "" ) || iTab == CHARACTER_TAB ) {
                bStuffChanged = true;
                //copy over
                vRules[( iTab == GLOBAL_TAB ) ? CHARACTER_TAB : GLOBAL_TAB].push_back( cRules(
                            vRules[iTab][iLine].sRule,
                            vRules[iTab][iLine].bActive,
                            vRules[iTab][iLine].bExclude,
                            vRules[iTab][iLine].attCreature,
                            vRules[iTab][iLine].iProxyDist ) );

                //remove old
                vRules[iTab].erase( vRules[iTab].begin() + iLine );
                iLine = vRules[( iTab == GLOBAL_TAB ) ? CHARACTER_TAB : GLOBAL_TAB].size() - 1;
                iTab = ( iTab == GLOBAL_TAB ) ? CHARACTER_TAB : GLOBAL_TAB;
            }
        } else if( action == "CONFIRM" && currentPageNonEmpty ) {
            bStuffChanged = true;
            if( iColumn == col_Rule ) {
                fold_and_print( w_help, 1, 1, 999, c_white,
                                _(
                                    "* is used as a Wildcard. A few Examples:\n"
                                    "\n"
                                    "zombie          matches the monster name exactly\n"
                                    "acidic zo*      matches monsters beginning with 'acidic zo'\n"
                                    "*mbie           matches monsters ending with 'mbie'\n"
                                    "*cid*zo*ie      multiple * are allowed\n"
                                    "AcI*zO*iE       case insensitive search\n"
                                    "" )
                              );

                draw_border( w_help );
                wrefresh( w_help );
                vRules[iTab][iLine].sRule = wildcard_trim_rule( string_input_popup( _( "Safemode Rule:" ),
                                            30, vRules[iTab][iLine].sRule ) );
            } else if( iColumn == col_IE ) {
                vRules[iTab][iLine].bExclude = !vRules[iTab][iLine].bExclude;
            } else if( iColumn == col_Attitude ) {
                vRules[iTab][iLine].attCreature++;
                if( vRules[iTab][iLine].attCreature >= Creature::A_MAX ) {
                    vRules[iTab][iLine].attCreature = 0;
                }
            } else if( iColumn == col_Dist && !vRules[iTab][iLine].bExclude ) {
                const auto text = string_input_popup( _( "Proximity Distance (0=max viewdistance)" ),
                                                      4,
                                                      to_string( vRules[iTab][iLine].iProxyDist ),
                                                      _( "Option: " ) + OPTIONS["SAFEMODEPROXIMITY"].getValue() +
                                                      " " + OPTIONS["SAFEMODEPROXIMITY"].getDefaultText(),
                                                      "",
                                                      3,
                                                      true
                                                    );
                if( text.empty() ) {
                    vRules[iTab][iLine].iProxyDist = OPTIONS["SAFEMODEPROXIMITY"];
                } else {
                    //Let the options class handle the validity of the new value
                    auto tempOption = OPTIONS["SAFEMODEPROXIMITY"];
                    tempOption.setValue( text.c_str() );
                    vRules[iTab][iLine].iProxyDist = tempOption;
                }
            }
        } else if( action == "ENABLE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            vRules[iTab][iLine].bActive = true;
        } else if( action == "DISABLE_RULE" && currentPageNonEmpty ) {
            bStuffChanged = true;
            vRules[iTab][iLine].bActive = false;
        } else if( action == "LEFT" ) {
            iColumn--;
            if( iColumn < 0 ) {
                iColumn = iTotalCols - 1;
            }
        } else if( action == "RIGHT" ) {
            iColumn++;
            if( iColumn >= iTotalCols ) {
                iColumn = 0;
            }
        } else if( action == "MOVE_RULE_UP" && currentPageNonEmpty ) {
            bStuffChanged = true;
            if( iLine < ( int )vRules[iTab].size() - 1 ) {
                std::swap( vRules[iTab][iLine],
                           vRules[iTab][iLine + 1] );
                iLine++;
                iColumn = 0;
            }
        } else if( action == "MOVE_RULE_DOWN" && currentPageNonEmpty ) {
            bStuffChanged = true;
            if( iLine > 0 ) {
                std::swap( vRules[iTab][iLine],
                           vRules[iTab][iLine - 1] );
                iLine--;
                iColumn = 0;
            }
        } else if( action == "TEST_RULE" && currentPageNonEmpty ) {
            test_pattern( iTab, iLine );
        } else if( action == "SWITCH_OPTION" ) {
            OPTIONS["SAFEMODE"].setNext();
            get_options().save();
        }
    }

    if( !bStuffChanged ) {
        return;
    }

    if( query_yn( _( "Save changes?" ) ) ) {
        if( is_safemode ) {
            save_global();
            if( g->u.name != "" ) {
                save_character();
            }
        } else {
            create_rules();
        }
    } else {
        vRules = vRulesOld;
    }
}

void safemode::test_pattern( const int iTab, const int iRow )
{
    std::vector<std::string> vMatchingMonsters;
    std::string sMonsterName = "";

    if( vRules[iTab][iRow].sRule == "" ) {
        return;
    }

    if( g->u.name == "" ) {
        popup( _( "No monsters loaded. Please start a game first." ) );
        return;
    }

    //Loop through all monster mtypes
    for( const auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
        sMonsterName = type.nname();
        if( wildcard_match( sMonsterName, vRules[iTab][iRow].sRule ) ) {
            vMatchingMonsters.push_back( sMonsterName );
        }
    }

    const int iOffsetX = 15 + ( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    const int iOffsetY = 5 + ( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 :
                               0 );

    int iStartPos = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 8;
    const int iContentWidth = FULL_SCREEN_WIDTH - 30;
    std::ostringstream sTemp;

    WINDOW *w_test_rule_border = newwin( iContentHeight + 2, iContentWidth, iOffsetY, iOffsetX );
    WINDOW_PTR w_test_rule_borderptr( w_test_rule_border );
    WINDOW *w_test_rule_content = newwin( iContentHeight, iContentWidth - 2, 1 + iOffsetY,
                                          1 + iOffsetX );
    WINDOW_PTR w_test_rule_contentptr( w_test_rule_content );

    draw_border( w_test_rule_border );

    int nmatch = vMatchingMonsters.size();
    std::string buf = string_format( ngettext( "%1$d monster matches: %2$s",
                                     "%1$d monsters match: %2$s",
                                     nmatch ), nmatch, vRules[iTab][iRow].sRule.c_str() );
    mvwprintz( w_test_rule_border, 0, iContentWidth / 2 - utf8_width( buf ) / 2, hilite( c_white ),
               "%s", buf.c_str() );

    mvwprintz( w_test_rule_border, iContentHeight + 1, 1, red_background( c_white ),
               _( "Lists monsters regardless of their attitude." ) );

    wrefresh( w_test_rule_border );

    int iLine = 0;

    input_context ctxt( "SAFEMODE_TEST" );
    ctxt.register_updown();
    ctxt.register_action( "QUIT" );

    while( true ) {
        // Clear the lines
        for( int i = 0; i < iContentHeight; i++ ) {
            for( int j = 0; j < 79; j++ ) {
                mvwputch( w_test_rule_content, i, j, c_black, ' ' );
            }
        }

        calcStartPos( iStartPos, iLine, iContentHeight, vMatchingMonsters.size() );

        // display safemode
        for( int i = iStartPos; i < ( int )vMatchingMonsters.size(); i++ ) {
            if( i >= iStartPos &&
                i < iStartPos + ( ( iContentHeight > ( int )vMatchingMonsters.size() ) ?
                                  ( int )vMatchingMonsters.size() :
                                  iContentHeight ) ) {
                nc_color cLineColor = c_white;

                sTemp.str( "" );
                sTemp << i + 1;
                mvwprintz( w_test_rule_content, i - iStartPos, 0, cLineColor, "%s", sTemp.str().c_str() );
                mvwprintz( w_test_rule_content, i - iStartPos, 4, cLineColor, "" );

                if( iLine == i ) {
                    wprintz( w_test_rule_content, c_yellow, ">> " );
                } else {
                    wprintz( w_test_rule_content, c_yellow, "   " );
                }

                wprintz( w_test_rule_content, ( iLine == i ) ? hilite( cLineColor ) : cLineColor,
                         vMatchingMonsters[i].c_str() );
            }
        }

        wrefresh( w_test_rule_content );

        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            iLine++;
            if( iLine >= ( int )vMatchingMonsters.size() ) {
                iLine = 0;
            }
        } else if( action == "UP" ) {
            iLine--;
            if( iLine < 0 ) {
                iLine = vMatchingMonsters.size() - 1;
            }
        } else {
            break;
        }
    }
}

void safemode::add_rule( const std::string &sRule, const int attMonster, const int iProxyDist,
                         const rule_state state )
{
    vRules[CHARACTER_TAB].push_back( cRules( sRule, true, ( state == RULE_BLACKLISTED ) ? false : true,
                                     attMonster, iProxyDist ) );
    create_rules();

    if( !OPTIONS["SAFEMODE"] &&
        query_yn( _( "Safemode is not enabled in the options. Enable it now?" ) ) ) {
        OPTIONS["SAFEMODE"].setNext();
        get_options().save();
    }
}

bool safemode::has_rule( const std::string &sRule, const int attMonster )
{
    for( auto &elem : vRules[CHARACTER_TAB] ) {
        if( sRule.length() == elem.sRule.length()
            && ci_find_substr( sRule, elem.sRule ) != -1
            && elem.attCreature == attMonster ) {
            return true;
        }
    }
    return false;
}

void safemode::remove_rule( const std::string &sRule, const int attMonster )
{
    for( auto it = vRules[CHARACTER_TAB].begin();
         it != vRules[CHARACTER_TAB].end(); ++it ) {
        if( sRule.length() == it->sRule.length()
            && ci_find_substr( sRule, it->sRule ) != -1
            && it->attCreature == attMonster ) {
            vRules[CHARACTER_TAB].erase( it );
            create_rules();
            break;
        }
    }
}

bool safemode::empty() const
{
    for( int i = 0; i < MAX_TAB; i++ ) {
        if( !vRules[i].empty() ) {
            return false;
        }
    }

    return true;
}

void safemode::create_rules()
{
    map_monsters.clear();

    static std::vector<int> vAny = {{Creature::A_HOSTILE, Creature::A_NEUTRAL, Creature::A_FRIENDLY}};

    //process include/exclude in order of rules, global first, then character specific
    //if a specific monster is being added, all the rules need to be checked now
    //may have some performance issues since exclusion needs to check all monsters also
    for( int i = 0; i < MAX_TAB; i++ ) {
        for( auto &elem : vRules[i] ) {
            if( !elem.bExclude ) {
                //Check include patterns against all monster mtypes
                for( const auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
                    const std::string &cur_mon = type.nname();
                    if( elem.bActive && wildcard_match( cur_mon, elem.sRule ) ) {
                        if( elem.attCreature == ( int )Creature::A_ANY ) {
                            for( auto &any : vAny ) {
                                map_monsters[ cur_mon ][ any ] = cRuleState( RULE_BLACKLISTED, elem.iProxyDist );
                            }
                        } else {
                            map_monsters[ cur_mon ][ elem.attCreature ] = cRuleState( RULE_BLACKLISTED, elem.iProxyDist );
                        }
                    }
                }
            } else {
                //exclude monsters from the existing mapping
                for( auto iter = map_monsters.begin(); iter != map_monsters.end(); ++iter ) {
                    if( elem.bActive && wildcard_match( iter->first, elem.sRule ) ) {
                        if( elem.attCreature == ( int )Creature::A_ANY ) {
                            for( auto &any : vAny ) {
                                map_monsters[ iter->first ][ any ] = cRuleState( RULE_WHITELISTED, elem.iProxyDist );
                            }
                        } else {
                            map_monsters[ iter->first ][ elem.attCreature ] = cRuleState( RULE_WHITELISTED, elem.iProxyDist );
                        }
                    }
                }
            }
        }
    }
}

rule_state safemode::check_monster( const std::string &sMonsterName, const int attCreature,
                                    const int iDist ) const
{
    const auto iter = map_monsters.find( sMonsterName );
    if( iter != map_monsters.end() ) {
        const auto &tmp = ( iter->second )[attCreature];
        if( tmp.state == RULE_BLACKLISTED ) {
            const int check_dist = ( tmp.proxy_dist == 0 ) ? 50 : tmp.proxy_dist;
            if( iDist <= check_dist ) {
                return RULE_BLACKLISTED;
            }

        } else if( tmp.state == RULE_WHITELISTED ) {
            return RULE_WHITELISTED;
        }
    }

    return RULE_NONE;
}

void safemode::clear_character_rules()
{
    vRules[CHARACTER_TAB].clear();
}

bool safemode::save_character()
{
    return save( true );
}

bool safemode::save_global()
{
    return save( false );
}

bool safemode::save( const bool bCharacter )
{
    bChar = bCharacter;
    auto savefile = FILENAMES["safemode"];

    if( bCharacter ) {
        savefile = world_generator->active_world->world_path + "/" + base64_encode(
                       g->u.name ) + ".sfm.json";
        std::ifstream fin;

        fin.open( ( world_generator->active_world->world_path + "/" +
                    base64_encode( g->u.name ) + ".sav" ).c_str() );
        if( !fin.is_open() ) {
            return true; //Character not saved yet.
        }
        fin.close();
    }

    return write_to_file( savefile, [&]( std::ostream & fout ) {
        JsonOut jout( fout, true );
        serialize( jout );

        if( !bCharacter ) {
            create_rules();
        }
    }, _( "safemode configuration" ) );
}

void safemode::load_character()
{
    load( true );
}

void safemode::load_global()
{
    load( false );
}

void safemode::load( const bool bCharacter )
{
    bChar = bCharacter;

    std::ifstream fin;
    std::string sFile = FILENAMES["safemode"];
    if( bCharacter ) {
        sFile = world_generator->active_world->world_path + "/" + base64_encode( g->u.name ) + ".sfm.json";
    }

    fin.open( sFile.c_str(), std::ifstream::in | std::ifstream::binary );

    if( fin.good() ) {
        try {
            JsonIn jsin( fin );
            deserialize( jsin );
        } catch( const JsonError &e ) {
            DebugLog( D_ERROR, DC_ALL ) << "safemode::load: " << e;
        }
    }

    fin.close();
    create_rules();
}

void safemode::serialize( JsonOut &json ) const
{
    json.start_array();

    for( auto &elem : vRules[( bChar ) ? CHARACTER_TAB : GLOBAL_TAB] ) {
        json.start_object();

        json.member( "rule", elem.sRule );
        json.member( "active", elem.bActive );
        json.member( "exclude", elem.bExclude );
        json.member( "attitude", elem.attCreature );
        json.member( "proxy_dist", elem.iProxyDist );

        json.end_object();
    }

    json.end_array();
}

void safemode::deserialize( JsonIn &jsin )
{
    vRules[( bChar ) ? CHARACTER_TAB : GLOBAL_TAB].clear();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();

        const std::string sRule = jo.get_string( "rule" );
        const bool bActive = jo.get_bool( "active" );
        const bool bExclude = jo.get_bool( "exclude" );
        const Creature::Attitude attCreature = ( Creature::Attitude ) jo.get_int( "attitude" );
        const int iProxyDist = jo.get_int( "proxy_dist" );

        vRules[( bChar ) ? CHARACTER_TAB : GLOBAL_TAB].push_back(
            cRules( sRule, bActive, bExclude, attCreature, iProxyDist )
        );
    }
}
