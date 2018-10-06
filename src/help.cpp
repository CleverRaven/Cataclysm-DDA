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

    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();

        std::vector<std::string> text;
        jo.read( "text", text );

        for( auto &line : text ) {
            size_t pos = line.find( "<press_", 0, 7 );
            while( pos != std::string::npos ) {
                size_t pos2 = line.find( ">", pos, 1 );

                std::string action = line.substr( pos + 7, pos2 - pos - 7 );
                auto replace = press_x( look_up_action( action ), "", "" );

                if( replace.empty() ) {
                    debugmsg("Help json: Unknown action: %s", action);
                } else {
                    line = string_replace( line, "<press_" + action + ">", replace);
                }

                pos = line.find( "<press_", pos2, 7 );
            }
        }

        help_texts[jo.get_int( "pos" )] = std::make_pair( _(jo.get_string( "name" ).c_str() ), text );

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
Press q or ESC to return to the game." ) ) + 1;

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

void help_map( const catacurses::window &win )
{
    werase( win );
    mvwprintz( win, 0, 0, c_light_gray,  _( "MAP SYMBOLS:" ) );
    mvwprintz( win, 1, 0, c_brown,   _( "\
.           Field - Empty grassland, occasional wild fruit." ) );
    mvwprintz( win, 2, 0, c_green,   _( "\
F           Forest - May be dense or sparse. Slow moving; foragable food." ) );
    mvwputch( win,  3,  0, c_dark_gray, LINE_XOXO );
    mvwputch( win,  3,  1, c_dark_gray, LINE_OXOX );
    mvwputch( win,  3,  2, c_dark_gray, LINE_XXOO );
    mvwputch( win,  3,  3, c_dark_gray, LINE_OXXO );
    mvwputch( win,  3,  4, c_dark_gray, LINE_OOXX );
    mvwputch( win,  3,  5, c_dark_gray, LINE_XOOX );
    mvwputch( win,  3,  6, c_dark_gray, LINE_XXXO );
    mvwputch( win,  3,  7, c_dark_gray, LINE_XXOX );
    mvwputch( win,  3,  8, c_dark_gray, LINE_XOXX );
    mvwputch( win,  3,  9, c_dark_gray, LINE_OXXX );
    mvwputch( win,  3, 10, c_dark_gray, LINE_XXXX );

    mvwprintz( win,  3, 12, c_dark_gray,  _( "\
Road - Safe from burrowing animals." ) );
    mvwprintz( win,  4, 0, c_dark_gray,  _( "\
H=          Highway - Like roads, but lined with guard rails." ) );
    mvwprintz( win,  5, 0, c_dark_gray,  _( "\
|-          Bridge - Helps you cross rivers." ) );
    mvwprintz( win,  6, 0, c_blue,    _( "\
R           River - Most creatures can not swim across them, but you may." ) );
    mvwprintz( win,  7, 0, c_dark_gray,  _( "\
O           Parking lot - Empty lot, few items. Mostly useless." ) );
    mvwprintz( win,  8, 0, c_light_green, _( "\
^>v<        House - Filled with a variety of items. Good place to sleep." ) );
    mvwprintz( win,  9, 0, c_light_blue,  _( "\
^>v<        Gas station - A good place to collect gasoline. Risk of explosion." ) );
    mvwprintz( win, 10, 0, c_light_red,   _( "\
^>v<        Pharmacy - The best source for vital medications." ) );
    mvwprintz( win, 11, 0, c_green,   _( "\
^>v<        Grocery store - A good source of canned food and other supplies." ) );
    mvwprintz( win, 12, 0, c_cyan,    _( "\
^>v<        Hardware store - Home to tools, melee weapons and crafting goods." ) );
    mvwprintz( win, 13, 0, c_light_cyan,  _( "\
^>v<        Sporting Goods store - Several survival tools and melee weapons." ) );
    mvwprintz( win, 14, 0, c_magenta, _( "\
^>v<        Liquor store - Alcohol is good for crafting Molotov cocktails." ) );
    mvwprintz( win, 15, 0, c_red,     _( "\
^>v<        Gun store - Firearms and ammunition are very valuable." ) );
    mvwprintz( win, 16, 0, c_blue,    _( "\
^>v<        Clothing store - High-capacity clothing, some light armor." ) );
    mvwprintz( win, 17, 0, c_brown,   _( "\
^>v<        Library - Home to books, both entertaining and informative." ) );
    mvwprintz( win, 18, 0, c_white, _( "\
^>v<        Man-made buildings - The pointed side indicates the front door." ) );
    mvwprintz( win, 19, 0, c_light_gray, _( "\
            There are many others out there... search for them!" ) );
    mvwprintz( win, 20, 0,  c_light_gray,  _( "Note colors: " ) );
    int row = 20;
    int column = utf8_width( _( "Note colors: " ) );
    for( auto color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        std::string color_description = string_format( "%s:%s, ", color_pair.first.c_str(),
                                        _( color_pair.second.c_str() ) );
        int pair_width = utf8_width( color_description );

        if( column + pair_width > getmaxx( win ) ) {
            column = 0;
            row++;
        }
        mvwprintz( win, row, column, get_note_color( color_pair.first ),
                   color_description.c_str() );
        column += pair_width;
    }
    wrefresh( win );
    catacurses::refresh();
    inp_mngr.wait_for_any_key();
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
