#include "game.h"
#include "player.h"
#include "output.h"
#include "input.h"
#include "mission.h"
#include "weather.h"
#include "calendar.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers

#include <vector>
#include <string>
#include <map>


void game::list_missions()
{
    WINDOW *w_missions = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                 ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                 ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    enum class tab_mode : int {
        TAB_ACTIVE = 0,
        TAB_COMPLETED,
        TAB_FAILED,
        NUM_TABS,
        FIRST_TAB = 0,
        LAST_TAB = NUM_TABS - 1
    };
    tab_mode tab = tab_mode::FIRST_TAB;
    size_t selection = 0;
    // content ranges from y=3 to FULL_SCREEN_HEIGHT - 2
    const int entries_per_page = FULL_SCREEN_HEIGHT - 4;
    input_context ctxt( "MISSIONS" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    while( true ) {
        werase( w_missions );
        std::vector<mission *> umissions;
        if( tab < tab_mode::FIRST_TAB || tab >= tab_mode::NUM_TABS ) {
            debugmsg( "The sanity check failed because tab=%d", ( int )tab );
            tab = tab_mode::FIRST_TAB;
        }
        switch( tab ) {
            case tab_mode::TAB_ACTIVE:
                umissions = u.get_active_missions();
                break;
            case tab_mode::TAB_COMPLETED:
                umissions = u.get_completed_missions();
                break;
            case tab_mode::TAB_FAILED:
                umissions = u.get_failed_missions();
                break;
            default:
                break;
        }
        if( ( !umissions.empty() && selection >= umissions.size() ) ||
            ( umissions.empty() && selection != 0 ) ) {
            debugmsg( "Sanity check failed: selection=%d, size=%d", ( int )selection, ( int )umissions.size() );
            selection = 0;
        }
        // entries_per_page * page number
        const int top_of_page = entries_per_page * ( selection / entries_per_page );
        const int bottom_of_page =
            std::min( top_of_page + entries_per_page - 1, ( int )umissions.size() - 1 );

        for( int i = 1; i < FULL_SCREEN_WIDTH - 1; i++ ) {
            mvwputch( w_missions, 2, i, BORDER_COLOR, LINE_OXOX );
            mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX );

            if( i > 2 && i < FULL_SCREEN_HEIGHT - 1 ) {
                mvwputch( w_missions, i, 30, BORDER_COLOR, LINE_XOXO );
                mvwputch( w_missions, i, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXO );
            }
        }

        draw_tab( w_missions, 7, _( "ACTIVE MISSIONS" ), tab == tab_mode::TAB_ACTIVE );
        draw_tab( w_missions, 30, _( "COMPLETED MISSIONS" ), tab == tab_mode::TAB_COMPLETED );
        draw_tab( w_missions, 56, _( "FAILED MISSIONS" ), tab == tab_mode::TAB_FAILED );

        mvwputch( w_missions, 2, 0, BORDER_COLOR, LINE_OXXO ); // |^
        mvwputch( w_missions, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX ); // ^|

        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO ); // |
        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR,
                  LINE_XOOX ); // _|

        mvwputch( w_missions, 2, 30, BORDER_COLOR,
                  tab == tab_mode::TAB_COMPLETED ? LINE_XOXX : LINE_XXXX ); // + || -|
        mvwputch( w_missions, FULL_SCREEN_HEIGHT - 1, 30, BORDER_COLOR, LINE_XXOX ); // _|_

        draw_scrollbar( w_missions, selection, entries_per_page, umissions.size(), 3, 0 );

        for( int i = top_of_page; i <= bottom_of_page; i++ ) {
            const auto miss = umissions[i];
            nc_color col = c_white;
            if( u.get_active_mission() == miss ) {
                col = c_ltred;
            }
            const int y = i - top_of_page + 3;
            if( ( int )selection == i ) {
                mvwprintz( w_missions, y, 1, hilite( col ), "%s", miss->name().c_str() );
            } else {
                mvwprintz( w_missions, y, 1, col, "%s", miss->name().c_str() );
            }
        }

        if( selection < umissions.size() ) {
            const auto miss = umissions[selection];
            int y = 4;
            if( !miss->get_description().empty() ) {
                mvwprintz( w_missions, y++, 31, c_white, "%s", miss->get_description().c_str() );
            }
            if( miss->has_deadline() ) {
                const calendar deadline( miss->get_deadline() );
                std::string dl = string_format( season_name_upper( deadline.get_season() ) + ", day " +
                                                to_string( deadline.days() + 1 ) + " " + deadline.print_time() );
                mvwprintz( w_missions, y++, 31, c_white, _( "Deadline: %s" ), dl.c_str() );

                if( tab != tab_mode::TAB_COMPLETED ) {
                    // There's no point in displaying this for a completed mission.
                    // @TODO: But displaying when you completed it would be useful.
                    const int remaining_turns = deadline.get_turn() - calendar::turn;
                    std::string remaining_time;

                    if( remaining_turns <= 0 ) {
                        remaining_time = _( "None!" );
                    } else if( u.has_watch() ) {
                        remaining_time = calendar::print_duration( remaining_turns );
                    } else {
                        remaining_time = calendar::print_approx_duration( remaining_turns );
                    }

                    mvwprintz( w_missions, y++, 31, c_white, _( "Time remaining: %s" ), remaining_time.c_str() );
                }
            }
            if( miss->has_target() ) {
                const tripoint pos = u.global_omt_location();
                // TODO: target does not contain a z-component, targets are assumed to be on z=0
                mvwprintz( w_missions, y++, 31, c_white, _( "Target: (%d, %d)   You: (%d, %d)" ),
                           miss->get_target().x, miss->get_target().y, pos.x, pos.y );
            }
        } else {
            static const std::map< tab_mode, std::string > nope = {
                { tab_mode::TAB_ACTIVE, _( "You have no active missions!" ) },
                { tab_mode::TAB_COMPLETED, _( "You haven't completed any missions!" ) },
                { tab_mode::TAB_FAILED, _( "You haven't failed any missions!" ) }
            };
            mvwprintz( w_missions, 4, 31, c_ltred, "%s", nope.at( tab ).c_str() );
        }

        wrefresh( w_missions );
        const std::string action = ctxt.handle_input();
        if( action == "RIGHT" ) {
            tab = ( tab_mode )( ( int )tab + 1 );
            if( tab >= tab_mode::NUM_TABS ) {
                tab = tab_mode::FIRST_TAB;
            }
            selection = 0;
        } else if( action == "LEFT" ) {
            tab = ( tab_mode )( ( int )tab - 1 );
            if( tab < tab_mode::FIRST_TAB ) {
                tab = tab_mode::LAST_TAB;
            }
            selection = 0;
        } else if( action == "DOWN" ) {
            selection++;
            if( selection >= umissions.size() ) {
                selection = 0;
            }
        } else if( action == "UP" ) {
            if( selection == 0 ) {
                selection = umissions.empty() ? 0 : umissions.size() - 1;
            } else {
                selection--;
            }
        } else if( action == "CONFIRM" ) {
            if( tab == tab_mode::TAB_ACTIVE && selection < umissions.size() ) {
                u.set_active_mission( *umissions[selection] );
            }
            break;
        } else if( action == "QUIT" ) {
            break;
        }
    }

    werase( w_missions );
    delwin( w_missions );
    refresh_all();
}
