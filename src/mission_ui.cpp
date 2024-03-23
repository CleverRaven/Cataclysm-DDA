#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "color.h"
#include "debug.h"
#include "input_context.h"
#include "mission.h"
#include "npc.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

void game::list_missions()
{
    constexpr int MAX_CHARS_PER_MISSION_ROW_NAME{ 38 };

    catacurses::window w_missions;

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
    int entries_per_page = 0;
    input_context ctxt( "MISSIONS" );
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // rectangular coordinates of each tab
    // used for mouse controls
    std::map<tab_mode, inclusive_rectangle<point>> tabs_coords;
    std::map<size_t, inclusive_rectangle<point>> mission_row_coords;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w_missions = new_centered_win( TERMY, FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2 );

        // content ranges from y=3 to TERMY - 2
        entries_per_page = TERMY - 4;

        ui.position_from_window( w_missions );
    } );
    ui.mark_resize();

    std::vector<mission *> umissions;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_missions );
        // entries_per_page * page number
        const int top_of_page = entries_per_page * ( selection / entries_per_page );
        const int bottom_of_page =
            std::min( top_of_page + entries_per_page - 1, static_cast<int>( umissions.size() ) - 1 );

        for( int i = 3; i < TERMY - 1; i++ ) {
            mvwputch( w_missions, point( 40, i ), BORDER_COLOR, LINE_XOXO );
        }

        const std::vector<std::pair<tab_mode, std::string>> tabs = {
            { tab_mode::TAB_ACTIVE, _( "ACTIVE MISSIONS" ) },
            { tab_mode::TAB_COMPLETED, _( "COMPLETED MISSIONS" ) },
            { tab_mode::TAB_FAILED, _( "FAILED MISSIONS" ) },
        };
        tabs_coords = draw_tabs( w_missions, tabs, tab );
        draw_border_below_tabs( w_missions );
        int x1 = 2;
        int x2 = 2;
        for( const std::pair<tab_mode, std::string> &t : tabs ) {
            x2 = x1 + utf8_width( t.second ) + 1;
            if( t.first == tab ) {
                break;
            }
            x1 = x2 + 2;
        }
        mvwputch( w_missions, point( 40, 2 ), BORDER_COLOR, x1 < 40 && 40 < x2 ? ' ' : LINE_OXXX ); // ^|^
        mvwputch( w_missions, point( 40, TERMY - 1 ), BORDER_COLOR, LINE_XXOX ); // _|_

        draw_scrollbar( w_missions, selection, entries_per_page, umissions.size(), point( 0, 3 ) );

        mission_row_coords.clear();
        dialogue d( get_talker_for( get_avatar() ), nullptr, {} );
        auto format_tokenized_description = [&d]( const std::string & description,
        const talk_effect_fun_t::likely_rewards_t &rewards ) {
            std::string formatted_description = description;
            for( const auto &reward : rewards ) {
                std::string token = "<reward_count:" + itype_id( reward.second.evaluate( d ) ).str() + ">";
                formatted_description = string_replace( formatted_description, token, string_format( "%g",
                                                        reward.first.evaluate( d ) ) );
            }
            parse_tags( formatted_description, get_avatar(), get_avatar() );
            return formatted_description;
        };
        for( int i = top_of_page; i <= bottom_of_page; i++ ) {
            mission *miss = umissions[i];
            const nc_color col = u.get_active_mission() == miss ? c_light_green : c_white;
            const int y = i - top_of_page + 3;
            trim_and_print( w_missions, point( 1, y ), MAX_CHARS_PER_MISSION_ROW_NAME,
                            static_cast<int>( selection ) == i ? hilite( col ) : col,
                            format_tokenized_description( miss->name(), miss->get_likely_rewards() ) );
            inclusive_rectangle<point> rec( point( 1, y ), point( 1 + MAX_CHARS_PER_MISSION_ROW_NAME - 1, y ) );
            mission_row_coords.emplace( i, rec );
        }

        if( selection < umissions.size() ) {
            mission *miss = umissions[selection];
            const nc_color col = u.get_active_mission() == miss ? c_light_green : c_white;
            std::string for_npc;
            if( miss->get_npc_id().is_valid() ) {
                npc *guy = g->find_npc( miss->get_npc_id() );
                if( guy ) {
                    for_npc = string_format( _( " for %s" ), guy->disp_name() );
                }
            }

            int y = 3;

            y += fold_and_print( w_missions, point( 41, y ), getmaxx( w_missions ) - 43, col,
                                 format_tokenized_description( miss->name(), miss->get_likely_rewards() ) + for_npc );
            y++;
            if( !miss->get_description().empty() ) {
                y += fold_and_print( w_missions, point( 41, y ), getmaxx( w_missions ) - 43, c_white,
                                     format_tokenized_description( miss->get_description(), miss->get_likely_rewards() ) );
            }
            if( miss->has_deadline() ) {
                const time_point deadline = miss->get_deadline();
                mvwprintz( w_missions, point( 41, ++y ), c_white, _( "Deadline: %s" ), to_string( deadline ) );

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

                    mvwprintz( w_missions, point( 41, ++y ), c_white, _( "Time remaining: %s" ), remaining_time );
                }
            }
            if( miss->has_target() ) {
                const tripoint_abs_omt pos = u.global_omt_location();
                // TODO: target does not contain a z-component, targets are assumed to be on z=0
                mvwprintz( w_missions, point( 41, ++y ), c_white, _( "Target: %s   You: %s" ),
                           miss->get_target().to_string(), pos.to_string() );
            }
        } else {
            static const std::map< tab_mode, std::string > nope = {
                { tab_mode::TAB_ACTIVE, translate_marker( "You have no active missions!" ) },
                { tab_mode::TAB_COMPLETED, translate_marker( "You haven't completed any missions!" ) },
                { tab_mode::TAB_FAILED, translate_marker( "You haven't failed any missions!" ) }
            };
            mvwprintz( w_missions, point( 41, 4 ), c_light_red, _( nope.at( tab ) ) );
        }

        wnoutrefresh( w_missions );
    } );

    while( true ) {
        umissions.clear();
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
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "LEFT" || action == "PREV_TAB" || action == "RIGHT" || action == "NEXT_TAB" ) {
            // necessary to use inc_clamp_wrap
            static_assert( static_cast<int>( tab_mode::FIRST_TAB ) == 0 );
            tab = inc_clamp_wrap( tab, action == "RIGHT" || action == "NEXT_TAB", tab_mode::NUM_TABS );
            selection = 0;
        } else if( navigate_ui_list( action, selection, 10, umissions.size(), true ) ) {
        } else if( action == "CONFIRM" ) {
            if( tab == tab_mode::TAB_ACTIVE && selection < umissions.size() ) {
                u.set_active_mission( *umissions[selection] );
            }
            break;
        } else if( action == "SELECT" ) {
            // get clicked coord
            std::optional<point> coord = ctxt.get_coordinates_text( w_missions );
            if( coord.has_value() ) {

                for( auto &it : tabs_coords ) {
                    if( it.second.contains( coord.value() ) ) {
                        tab = it.first;
                    }
                }

                for( auto &it : mission_row_coords ) {
                    if( it.second.contains( coord.value() ) ) {
                        selection = it.first;
                        if( tab == tab_mode::TAB_ACTIVE && selection < umissions.size() ) {
                            u.set_active_mission( *umissions[selection] );
                        }
                    }
                }
            }
        } else if( action == "MOUSE_MOVE" ) {
            // get clicked coord
            std::optional<point> coord = ctxt.get_coordinates_text( w_missions );
            if( coord.has_value() ) {

                for( auto &it : mission_row_coords ) {
                    if( it.second.contains( coord.value() ) ) {
                        selection = it.first;
                    }
                }
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }
}
