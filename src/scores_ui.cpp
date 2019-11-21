#include "scores_ui.h"

#include "cursesdef.h"
#include "kill_tracker.h"
#include "input.h"
#include "output.h"
#include "event_statistics.h"
#include "stats_tracker.h"

static std::string get_scores_text( stats_tracker &stats )
{
    std::ostringstream os;
    std::vector<const score *> valid_scores = stats.valid_scores();
    for( const score *scr : valid_scores ) {
        os << scr->description( stats ) << '\n';
    }
    if( valid_scores.empty() ) {
        os << _( "This game has no valid scores.\n" );
    }
    os << _( "\nNote that only scores that existed when you started this game and still exist now "
             "will appear here." );
    return os.str();
}

void show_scores_ui( stats_tracker &stats, const kill_tracker &kills )
{
    catacurses::window w = new_centered_win( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH );

    enum class tab_mode {
        scores,
        kills,
        num_tabs,
        first_tab = scores,
    };

    tab_mode tab = static_cast<tab_mode>( 0 );
    input_context ctxt( "SCORES" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    catacurses::window w_view = catacurses::newwin( getmaxy( w ) - 4, getmaxx( w ) - 1,
                                point( getbegx( w ), getbegy( w ) + 3 ) );
    scrolling_text_view view( w_view );
    bool new_tab = true;

    while( true ) {
        werase( w );

        const std::vector<std::pair<tab_mode, std::string>> tabs = {
            { tab_mode::scores, _( "SCORES" ) },
            { tab_mode::kills, _( "KILLS" ) },
        };
        draw_tabs( w, tabs, tab );
        draw_border_below_tabs( w );

        if( new_tab ) {
            switch( tab ) {
                case tab_mode::scores:
                    view.set_text( get_scores_text( stats ) );
                    break;
                case tab_mode::kills:
                    view.set_text( kills.get_kills_text() );
                    break;
                case tab_mode::num_tabs:
                    assert( false );
                    break;
            }
        }

        wrefresh( w );

        view.draw( c_white );

        const std::string action = ctxt.handle_input();
        new_tab = false;
        if( action == "RIGHT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) + 1 );
            if( tab >= tab_mode::num_tabs ) {
                tab = tab_mode::first_tab;
            }
            new_tab = true;
        } else if( action == "LEFT" ) {
            tab = static_cast<tab_mode>( static_cast<int>( tab ) - 1 );
            if( tab < tab_mode::first_tab ) {
                tab = static_cast<tab_mode>( static_cast<int>( tab_mode::num_tabs ) - 1 );
            }
            new_tab = true;
        } else if( action == "DOWN" ) {
            view.scroll_down();
        } else if( action == "UP" ) {
            view.scroll_up();
        } else if( action == "CONFIRM" ) {
            break;
        } else if( action == "QUIT" ) {
            break;
        }
    }
}
