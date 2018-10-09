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
    std::string dir_grid = get_dir_grid();

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();

        std::vector<std::string> messages;
        jo.read( "messages", messages );

        for( auto &line : messages ) {
            if( line == "<DRAW_NOTE_COLORS>" ) {
                line = string_replace( line, "<DRAW_NOTE_COLORS>", note_colors );
                continue;
            } else if( line == "<HELP_DRAW_DIRECTIONS>" ) {
                line = string_replace( line, "<HELP_DRAW_DIRECTIONS>", dir_grid );
                continue;
            }

            size_t pos = line.find( "<press_", 0, 7 );
            while( pos != std::string::npos ) {
                size_t pos2 = line.find( ">", pos, 1 );

                std::string action = line.substr( pos + 7, pos2 - pos - 7 );
                auto replace = "<color_light_blue>" + press_x( look_up_action( action ), "", "" ) + "</color>";

                if( replace.empty() ) {
                    debugmsg( "Help json: Unknown action: %s", action );
                } else {
                    line = string_replace( line, "<press_" + action + ">", replace );
                }

                pos = line.find( "<press_", pos2, 7 );
            }
        }

        help_texts[jo.get_int( "order" )] = std::make_pair( _( jo.get_string( "name" ).c_str() ), messages );
        hotkeys.push_back( get_hotkeys( jo.get_string( "name" ) ) );
    }
}

std::string help::get_dir_grid()
{
    static const std::array<action_id, 9> movearray = {{
            ACTION_MOVE_NW, ACTION_MOVE_N, ACTION_MOVE_NE,
            ACTION_MOVE_W,  ACTION_PAUSE,  ACTION_MOVE_E,
            ACTION_MOVE_SW, ACTION_MOVE_S, ACTION_MOVE_SE
        }
    };

    std::string movement = "<LEFTUP_0>  <UP_0>  <RIGHTUP_0>   <LEFTUP_1>  <UP_1>  <RIGHTUP_1>\n"\
                           " \\ | /     \\ | /\n"\
                           "  \\|/       \\|/\n"\
                           "<LEFT_0>--<pause_0>--<RIGHT_0>   <LEFT_1>--<pause_1>--<RIGHT_1>\n"\
                           "  /|\\       /|\\\n"\
                           " / | \\     / | \\\n"\
                           "<LEFTDOWN_0>  <DOWN_0>  <RIGHTDOWN_0>   <LEFTDOWN_1>  <DOWN_1>  <RIGHTDOWN_1>";

    for( auto dir : movearray ) {
        std::vector<char> keys = keys_bound_to( dir );
        movement = string_replace( movement, "<" + action_ident( dir ) + "_0>",
                                   string_format( "<color_light_blue>%s</color>", keys[0] ) );
        movement = string_replace( movement, "<" + action_ident( dir ) + "_1>",
                                   string_format( "<color_light_blue>%s</color>", keys[1] ) );
    }

    return movement;
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

        shortcut_print( win, y + i % half_size, ( i < half_size ? 1 : second_column ),
                        c_white, c_light_blue, cat_name );
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
