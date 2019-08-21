#include "game.h" // IWYU pragma: associated

#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

#include "avatar.h"
#include "mission.h"
#include "calendar.h"
// needed for the workaround for the std::to_string bug in some compilers
#include "compatibility.h" // IWYU pragma: keep
#include "input.h"
#include "output.h"
#include "npc.h"
#include "color.h"
#include "debug.h"
#include "string_formatter.h"
#include "translations.h"

void game::list_missions()
{
    catacurses::window w_missions = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                    point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                           TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );

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
            debugmsg( "The sanity check failed because tab=%d", static_cast<int>( tab ) );
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
            debugmsg( "Sanity check failed: selection=%d, size=%d", static_cast<int>( selection ),
                      static_cast<int>( umissions.size() ) );
            selection = 0;
        }
        // entries_per_page * page number
        const int top_of_page = entries_per_page * ( selection / entries_per_page );
        const int bottom_of_page =
            std::min( top_of_page + entries_per_page - 1, static_cast<int>( umissions.size() ) - 1 );

        for( int i = 1; i < FULL_SCREEN_WIDTH - 1; i++ ) {
            mvwputch( w_missions, point( i, 2 ), BORDER_COLOR, LINE_OXOX );
            mvwputch( w_missions, point( i, FULL_SCREEN_HEIGHT - 1 ), BORDER_COLOR, LINE_OXOX );

            if( i > 2 && i < FULL_SCREEN_HEIGHT - 1 ) {
                mvwputch( w_missions, point( 30, i ), BORDER_COLOR, LINE_XOXO );
                mvwputch( w_missions, point( FULL_SCREEN_WIDTH - 1, i ), BORDER_COLOR, LINE_XOXO );
            }
        }

        draw_tab( w_missions, 7, _( "ACTIVE MISSIONS" ), tab == tab_mode::TAB_ACTIVE );
        draw_tab( w_missions, 30, _( "COMPLETED MISSIONS" ), tab == tab_mode::TAB_COMPLETED );
        draw_tab( w_missions, 56, _( "FAILED MISSIONS" ), tab == tab_mode::TAB_FAILED );

        mvwputch( w_missions, point( 0, 2 ), BORDER_COLOR, LINE_OXXO ); // |^
        mvwputch( w_missions, point( FULL_SCREEN_WIDTH - 1, 2 ), BORDER_COLOR, LINE_OOXX ); // ^|

        mvwputch( w_missions, point( 0, FULL_SCREEN_HEIGHT - 1 ), BORDER_COLOR, LINE_XXOO ); // |
        mvwputch( w_missions, point( FULL_SCREEN_WIDTH - 1, FULL_SCREEN_HEIGHT - 1 ), BORDER_COLOR,
                  LINE_XOOX ); // _|

        mvwputch( w_missions, point( 30, 2 ), BORDER_COLOR,
                  tab == tab_mode::TAB_COMPLETED ? LINE_XOXX : LINE_XXXX ); // + || -|
        mvwputch( w_missions, point( 30, FULL_SCREEN_HEIGHT - 1 ), BORDER_COLOR, LINE_XXOX ); // _|_

        draw_scrollbar( w_missions, selection, entries_per_page, umissions.size(), point( 0, 3 ) );

        for( int i = top_of_page; i <= bottom_of_page; i++ ) {
            const auto miss = umissions[i];
            const nc_color col = u.get_active_mission() == miss ? c_light_green : c_white;
            const int y = i - top_of_page + 3;
            trim_and_print( w_missions, point( 1, y ), 28,
                            static_cast<int>( selection ) == i ? hilite( col ) : col,
                            miss->name() );
        }

        if( selection < umissions.size() ) {
            const auto miss = umissions[selection];
            const nc_color col = u.get_active_mission() == miss ? c_light_green : c_white;
            std::string for_npc;
            if( miss->get_npc_id().is_valid() ) {
                npc *guy = g->find_npc( miss->get_npc_id() );
                if( guy ) {
                    for_npc = string_format( _( " for %s" ), guy->disp_name() );
                }
            }

            int y = 3;
            y += fold_and_print( w_missions, point( 31, y ), getmaxx( w_missions ) - 33, col,
                                 miss->name() + for_npc );

            auto format_tokenized_description = []( const std::string description,
            std::vector<std::pair<int, std::string>> rewards ) {
                std::string formatted_description = description;
                for( size_t i = 0; i < rewards.size(); i++ ) {
                    std::regex token("<reward_count:" + rewards[i].second + ">");
                    formatted_description = std::regex_replace( formatted_description, token, string_format( "%d",
                                            rewards[i].first ) );
                }
                return formatted_description;
            };

            y++;
            if( !miss->get_description().empty() ) {
                y += fold_and_print( w_missions, point( 31, y ), getmaxx( w_missions ) - 33, c_white,
                                     format_tokenized_description( miss->get_description(), miss->get_likely_rewards() ) );
            }
            if( miss->has_deadline() ) {
                const time_point deadline = miss->get_deadline();
                mvwprintz( w_missions, point( 31, ++y ), c_white, _( "Deadline: %s" ), to_string( deadline ) );

                if( tab != tab_mode::TAB_COMPLETED ) {
                    // There's no point in displaying this for a completed mission.
                    // @ TODO: But displaying when you completed it would be useful.
                    const time_duration remaining = deadline - calendar::turn;
                    std::string remaining_time;

                    if( remaining <= 0_turns ) {
                        remaining_time = _( "None!" );
                    } else if( u.has_watch() ) {
                        remaining_time = to_string( remaining );
                    } else {
                        remaining_time = to_string_approx( remaining );
                    }

                    mvwprintz( w_missions, point( 31, ++y ), c_white, _( "Time remaining: %s" ), remaining_time );
                }
            }
            if( miss->has_target() ) {
                const tripoint pos = u.global_omt_location();
                // TODO: target does not contain a z-component, targets are assumed to be on z=0
                mvwprintz( w_missions, point( 31, ++y ), c_white, _( "Target: (%d, %d)   You: (%d, %d)" ),
                           miss->get_target().x, miss->get_target().y, pos.x, pos.y );
            }
        } else {
            static const std::map< tab_mode, std::string > nope = {
                { tab_mode::TAB_ACTIVE, _( "You have no active missions!" ) },
                { tab_mode::TAB_COMPLETED, _( "You haven't completed any missions!" ) },
                { tab_mode::TAB_FAILED, _( "You haven't failed any missions!" ) }
            };
            mvwprintz( w_missions, point( 31, 4 ), c_light_red, nope.at( tab ) );
        }

        wrefresh( w_missions );
        const std::string action = ctxt.handle_input();
        if( action == "RIGHT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) + 1 );
            if( tab >= tab_mode::NUM_TABS ) {
                tab = tab_mode::FIRST_TAB;
            }
            selection = 0;
        } else if( action == "LEFT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) - 1 );
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

    refresh_all();
}
