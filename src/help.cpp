#include "help.h"

#include "action.h"
#include "catacharset.h"
#include "input.h"
#include "output.h"
#include "cursesdef.h"
#include "translations.h"
#include "text_snippets.h"
#include "json.h"
#include "path_info.h"
#include <cmath>  // max in help_main
#include <vector>
#include <algorithm>

help &get_help()
{
    static help single_instance;
    return single_instance;
}

void help::load()
{
    read_from_file_optional_json( FILENAMES["help"], [&]( JsonIn & jsin ) {
        deserialize( jsin );
    } );
}

void help::deserialize( JsonIn &jsin )
{
    hotkeys.clear();

    std::string note_colors = get_note_colors();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();

        std::vector<std::string> text;
        jo.read( "text", text );

        for( auto &line : text ) {

            line = string_replace( line, "<DRAW_NOTE_COLORS>", note_colors );

            size_t pos = line.find( "<press_", 0, 7 );
            while( pos != std::string::npos ) {
                size_t pos2 = line.find( ">", pos, 1 );

                std::string action = line.substr( pos + 7, pos2 - pos - 7 );
                auto replace = press_x( look_up_action( action ), "", "" );

                if( replace.empty() ) {
                    debugmsg( "Help json: Unknown action: %s", action );
                } else {
                    line = string_replace( line, "<press_" + action + ">", replace );
                }

                pos = line.find( "<press_", pos2, 7 );
            }
        }

        help_texts[jo.get_int( "pos" )] = std::make_pair( _( jo.get_string( "name" ).c_str() ), text );

        hotkeys.push_back( get_hotkeys( jo.get_string( "name" ) ) );
    }
}

void help_draw_dir( const catacurses::window &win, int line_y )
{
    std::array<action_id, 9> movearray = {{
            ACTION_MOVE_NW, ACTION_MOVE_N, ACTION_MOVE_NE,
            ACTION_MOVE_W,  ACTION_PAUSE,  ACTION_MOVE_E,
            ACTION_MOVE_SW, ACTION_MOVE_S, ACTION_MOVE_SE
        }
    };
    mvwprintz( win, line_y + 1, 0, c_white, _( "\
  \\ | /     \\ | /\n\
   \\|/       \\|/ \n\
  -- --     -- --  \n\
   /|\\       /|\\ \n\
  / | \\     / | \\" ) );
    for( int acty = 0; acty < 3; acty++ ) {
        for( int actx = 0; actx < 3; actx++ ) {
            std::vector<char> keys = keys_bound_to( movearray[acty * 3 + actx] );
            if( !keys.empty() ) {
                mvwputch( win, acty * 3 + line_y, actx * 3 + 1, c_light_blue, keys[0] );
                if( keys.size() > 1 ) {
                    mvwputch( win, acty * 3 + line_y, actx * 3 + 11, c_light_blue, keys[1] );
                }
            }
        }
    }
}

void help::draw_menu( const catacurses::window &win )
{
    werase( win );
    int y = fold_and_print( win, 0, 1, getmaxx( win ) - 2, c_white, _( "\
Please press one of the following for help on that topic:\n\
Press ESC to return to the game." ) ) + 1;

    std::vector<std::string> headers;

    size_t half_size = help_texts.size() / 2;
    int second_column = getmaxx( win ) / 2;
    for( size_t i = 0; i < help_texts.size(); i++ ) {
        auto &cat_name = help_texts[i].first;
        if( i < half_size ) {
            second_column = std::max( second_column, utf8_width( cat_name ) + 4 );
        }
        mvwprintz( win, y + i % half_size, ( i < half_size ? 1 : second_column ),
                   c_white, cat_name.c_str() );
    }

    wrefresh( win );
}

std::string help::get_note_colors()
{
    std::string text = _( "Note colors: " );
    for( auto color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        text += string_format( "<color_%s>%s:%s</color>, ",
                               string_from_color( get_note_color( color_pair.first ) ),
                               color_pair.first.c_str(), _( color_pair.second.c_str() ) );
    }

    return text;
}

void help::display_help()
{
    catacurses::window w_help_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                                       ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );
    catacurses::window w_help = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                1 + ( int )( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                                1 + ( int )( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ) );

    bool needs_refresh = true;

    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    // for the menu shortcuts
    ctxt.register_action( "ANY_INPUT" );

    std::string action = "";

    do {
        if( needs_refresh ) {
            draw_border( w_help_border, BORDER_COLOR, _( " HELP " ) );
            wrefresh( w_help_border );
            draw_menu( w_help );
            catacurses::refresh();
            needs_refresh = false;
        };

        /*
        #ifdef __ANDROID__
        input_context ctxt( "DISPLAY_HELP" );
        for( long key = 'a'; key <= 'p'; ++key ) {
            ctxt.register_manual_key( key );
        }
        for( long key = '1'; key <= '4'; ++key ) {
            ctxt.register_manual_key( key );
        }
        #endif
        */

        action = ctxt.handle_input();
        std::string sInput = ctxt.get_raw_input().text;
        for( size_t i = 0; i < hotkeys.size(); ++i ) {
            for( auto hotkey : hotkeys[i] ) {
                if( sInput == hotkey ) {
                    multipage( w_help, help_texts[i].second );
                    action = "CONFIRM";
                    break;
                }
            }
        }

        needs_refresh = true;
    } while( action != "QUIT" );
}

std::string get_hint()
{
    return SNIPPET.get( SNIPPET.assign( "hint" ) );
}
