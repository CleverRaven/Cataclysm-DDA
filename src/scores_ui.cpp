#include "scores_ui.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "achievement.h"
#include "color.h"
#include "cursesdef.h"
#include "event_statistics.h"
#include "input_context.h"
#include "localized_comparator.h"
#include "kill_tracker.h"
#include "output.h"
#include "past_games_info.h"
#include "point.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"

static std::string get_achievements_text( const achievements_tracker &achievements,
        bool use_conducts, int width )
{
    // Load past game info beforehand because otherwise it may erase an `achievement_tracker`
    // within a call to its method when lazy-loaded, causing dangling pointer.
    get_past_games();
    std::string thing_name = use_conducts ? _( "conducts" ) : _( "achievements" );
    std::string cap_thing_name = use_conducts ? _( "Conducts" ) : _( "Achievements" );
    if( !achievements.is_enabled() ) {
        return string_format(
                   _( "%s are disabled, probably due to use of the debug menu.  If you only used "
                      "the debug menu to work around a game bug, then you can re-enable %s via the "
                      "debug menu (\"Enable achievements\" under the \"Game\" submenu)." ),
                   cap_thing_name, thing_name );
    }
    std::string os;
    std::vector<const achievement *> valid_achievements = achievements.valid_achievements();
    valid_achievements.erase(
        std::remove_if( valid_achievements.begin(), valid_achievements.end(),
    [&]( const achievement * a ) {
        return achievements.is_hidden( a ) || a->is_conduct() != use_conducts;
    } ), valid_achievements.end() );
    using sortable_achievement =
        std::tuple<achievement_completion, std::string, const achievement *>;
    std::vector<sortable_achievement> sortable_achievements;
    std::transform( valid_achievements.begin(), valid_achievements.end(),
                    std::back_inserter( sortable_achievements ),
    [&]( const achievement * ach ) {
        achievement_completion comp = achievements.is_completed( ach->id );
        return std::make_tuple( comp, ach->name().translated(), ach );
    } );
    std::sort( sortable_achievements.begin(), sortable_achievements.end(), localized_compare );
    std::string horizontal_line;
    horizontal_line.reserve( std::string( LINE_OXOX_S ).length() * width );
    for( int i = 0; i < width; i++ ) {
        horizontal_line.append( LINE_OXOX_S );
    }
    for( const sortable_achievement &ach : sortable_achievements ) {
        os += achievements.ui_text_for( std::get<const achievement *>( ach ) );
        os += colorize( horizontal_line, c_magenta );
    }
    if( valid_achievements.empty() ) {
        os += string_format( _( "This game has no valid %s.\n" ), thing_name );
    }
    os += string_format( _( "Note that only %s that existed when you started this game and still "
                            "exist now will appear here." ), thing_name );
    return os;
}

static std::string get_scores_text( stats_tracker &stats )
{
    std::string os;
    std::vector<const score *> valid_scores = stats.valid_scores();
    for( const score *scr : valid_scores ) {
        os += scr->description( stats ) + "\n";
    }
    if( valid_scores.empty() ) {
        os += _( "This game has no valid scores.\n" );
    }
    os += _( "\nNote that only scores that existed when you started this game and still exist now "
             "will appear here." );
    return os;
}

void show_scores_ui( const achievements_tracker &achievements, stats_tracker &stats,
                     const kill_tracker &kills )
{
    catacurses::window w;

    enum class tab_mode : int {
        achievements,
        conducts,
        scores,
        kills,
        num_tabs,
        first_tab = achievements,
    };

    tab_mode tab = static_cast<tab_mode>( 0 );
    input_context ctxt( "SCORES" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    catacurses::window w_view;
    scrolling_text_view view( w_view );
    view.set_up_navigation( ctxt, scrolling_key_scheme::arrow_scroll, true );

    bool new_tab = true;

    ui_adaptor ui;
    const auto &init_windows = [&]( ui_adaptor & ui ) {
        w = new_centered_win( TERMY - 2, FULL_SCREEN_WIDTH );
        w_view = catacurses::newwin( getmaxy( w ) - 4, getmaxx( w ) - 1,
                                     point( getbegx( w ), getbegy( w ) + 3 ) );
        ui.position_from_window( w );
    };
    ui.on_screen_resize( init_windows );
    // initialize explicitly here since w_view is used before first redraw
    init_windows( ui );

    const std::vector<std::pair<tab_mode, std::string>> tabs = {
        { tab_mode::achievements, _( "ACHIEVEMENTS" ) },
        { tab_mode::conducts, _( "CONDUCTS" ) },
        { tab_mode::scores, _( "SCORES" ) },
        { tab_mode::kills, _( "KILLS" ) },
    };

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        draw_tabs( w, tabs, tab );
        draw_border_below_tabs( w );
        wnoutrefresh( w );

        view.draw( c_white );
    } );

    while( true ) {
        if( new_tab ) {
            switch( tab ) {
                case tab_mode::achievements:
                    view.set_text( get_achievements_text( achievements, false, getmaxx( w ) - 2 ) );
                    break;
                case tab_mode::conducts:
                    view.set_text( get_achievements_text( achievements, true, getmaxx( w ) - 2 ) );
                    break;
                case tab_mode::scores:
                    view.set_text( get_scores_text( stats ) );
                    break;
                case tab_mode::kills:
                    view.set_text( kills.get_kills_text() );
                    break;
                case tab_mode::num_tabs:
                    cata_fatal( "Invalid tab" );
                    break;
            }
        }

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        new_tab = false;
        if( view.handle_navigation( action, ctxt ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( action == "LEFT" || action == "PREV_TAB" || action == "RIGHT" || action == "NEXT_TAB" ) {
            // necessary to use inc_clamp_wrap
            static_assert( static_cast<int>( tab_mode::first_tab ) == 0 );
            tab = inc_clamp_wrap( tab, action == "RIGHT" || action == "NEXT_TAB", tab_mode::num_tabs );
            new_tab = true;
        } else if( action == "CONFIRM" || action == "QUIT" ) {
            break;
        }
    }
}
