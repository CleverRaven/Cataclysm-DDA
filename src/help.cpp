#include "help.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "action.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "input_context.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"

help &get_help()
{
    static help single_instance;
    return single_instance;
}

void help::load()
{
    read_from_file_optional_json( PATH_INFO::help(), [&]( const JsonValue & jsin ) {
        deserialize( jsin );
    } );
}

void help::deserialize( const JsonArray &ja )
{
    for( JsonObject jo : ja ) {
        if( jo.get_string( "type" ) != "help" ) {
            debugmsg( "object with type other than \"type\" found in help text file" );
            continue;
        }

        std::vector<translation> messages;
        jo.read( "messages", messages );

        translation name;
        jo.read( "name", name );

        help_texts[ jo.get_int( "order" ) ] = std::make_pair( name, messages );
    }
}

std::string help::get_dir_grid()
{
    static const std::array<action_id, 9> movearray = {{
            ACTION_MOVE_FORTH_LEFT, ACTION_MOVE_FORTH, ACTION_MOVE_FORTH_RIGHT,
            ACTION_MOVE_LEFT,  ACTION_PAUSE,  ACTION_MOVE_RIGHT,
            ACTION_MOVE_BACK_LEFT, ACTION_MOVE_BACK, ACTION_MOVE_BACK_RIGHT
        }
    };

    std::string movement = "<LEFTUP_0>  <UP_0>  <RIGHTUP_0>   <LEFTUP_1>  <UP_1>  <RIGHTUP_1>\n"
                           " \\ | /     \\ | /\n"
                           "  \\|/       \\|/\n"
                           "<LEFT_0>--<pause_0>--<RIGHT_0>   <LEFT_1>--<pause_1>--<RIGHT_1>\n"
                           "  /|\\       /|\\\n"
                           " / | \\     / | \\\n"
                           "<LEFTDOWN_0>  <DOWN_0>  <RIGHTDOWN_0>   <LEFTDOWN_1>  <DOWN_1>  <RIGHTDOWN_1>";

    for( action_id dir : movearray ) {
        std::vector<input_event> keys = keys_bound_to( dir, /*maximum_modifier_count=*/0 );
        for( size_t i = 0; i < 2; i++ ) {
            movement = string_replace( movement, "<" + action_ident( dir ) + string_format( "_%d>", i ),
                                       i < keys.size()
                                       ? string_format( "<color_light_blue>%s</color>",
                                               keys[i].short_description() )
                                       : "<color_red>?</color>" );
        }
    }

    return movement;
}

std::map<int, inclusive_rectangle<point>> help::draw_menu( const catacurses::window &win,
                                       int selected ) const
{
    std::map<int, inclusive_rectangle<point>> opt_map;

    werase( win );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    int y = fold_and_print( win, point( 1, 0 ), getmaxx( win ) - 2, c_white,
                            _( "Please press one of the following for help on that topic:\n"
                               "Press ESC to return to the game." ) ) + 1;

    size_t half_size = help_texts.size() / 2 + 1;
    int second_column = divide_round_up( getmaxx( win ), 2 );
    size_t i = 0;
    for( const auto &text : help_texts ) {
        const std::string cat_name = text.second.first.translated();
        const int cat_width = utf8_width( remove_color_tags( shortcut_text( c_white, cat_name ) ) );
        if( i < half_size ) {
            second_column = std::max( second_column, cat_width + 4 );
        }

        const point sc_start( i < half_size ? 1 : second_column, y + i % half_size );
        shortcut_print( win, sc_start, selected == text.first ? hilite( c_white ) : c_white,
                        selected == text.first ? hilite( c_light_blue ) : c_light_blue, cat_name );
        ++i;

        opt_map.emplace( text.first,
                         inclusive_rectangle<point>( sc_start, sc_start + point( cat_width - 1, 0 ) ) );
    }

    wnoutrefresh( win );

    return opt_map;
}

std::string help::get_note_colors()
{
    std::string text = _( "Note colors: " );
    for( const auto &color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        //~ %1$s: note color abbreviation, %2$s: note color name
        text += string_format( pgettext( "note color", "%1$s:%2$s, " ),
                               colorize( color_pair.first, color_pair.second.color ),
                               color_pair.second.name );
    }

    return text;
}

void help::display_help() const
{
    catacurses::window w_help_border;
    catacurses::window w_help;

    ui_adaptor ui;
    const auto init_windows = [&]( ui_adaptor & ui ) {
        w_help_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                            point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                                    TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );
        w_help = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                     point( 1 + static_cast<int>( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ),
                                            1 + static_cast<int>( TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) ) );
        ui.position_from_window( w_help_border );
    };
    init_windows( ui );
    ui.on_screen_resize( init_windows );

    input_context ctxt( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    // for mouse selection
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    // for the menu shortcuts
    ctxt.register_action( "ANY_INPUT" );

    std::string action;
    std::map<int, inclusive_rectangle<point>> opt_map;
    int sel = -1;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        draw_border( w_help_border, BORDER_COLOR, _( "Help" ) );
        wnoutrefresh( w_help_border );
        opt_map = draw_menu( w_help, sel );
    } );

    std::map<int, std::vector<std::string>> hotkeys;
    for( const auto &text : help_texts ) {
        hotkeys.emplace( text.first, get_hotkeys( text.second.first.translated() ) );
    }

    do {
        ui_manager::redraw();

        sel = -1;
        action = ctxt.handle_input();
        std::string sInput = ctxt.get_raw_input().text;

        // Mouse selection
        if( action == "MOUSE_MOVE" || action == "SELECT" ) {
            std::optional<point> coord = ctxt.get_coordinates_text( w_help );
            if( !!coord ) {
                int cnt = run_for_point_in<int, point>( opt_map, *coord,
                [&sel]( const std::pair<int, inclusive_rectangle<point>> &p ) {
                    sel = p.first;
                } );
                if( cnt > 0 && action == "SELECT" ) {
                    auto iter = hotkeys.find( sel );
                    if( iter != hotkeys.end() && !iter->second.empty() ) {
                        sInput = iter->second.front();
                        action = "CONFIRM";
                    }
                }
            }
        }

        for( const auto &hotkey_entry : hotkeys ) {
            auto help_text_it = help_texts.find( hotkey_entry.first );
            if( help_text_it == help_texts.end() ) {
                continue;
            }
            for( const std::string &hotkey : hotkey_entry.second ) {
                if( sInput == hotkey ) {
                    std::vector<std::string> i18n_help_texts;
                    i18n_help_texts.reserve( help_text_it->second.second.size() );
                    std::transform( help_text_it->second.second.begin(), help_text_it->second.second.end(),
                    std::back_inserter( i18n_help_texts ), [&]( const translation & line ) {
                        std::string line_proc = line.translated();
                        if( line_proc == "<DRAW_NOTE_COLORS>" ) {
                            line_proc = get_note_colors();
                        } else if( line_proc == "<HELP_DRAW_DIRECTIONS>" ) {
                            line_proc = get_dir_grid();
                        }
                        size_t pos = line_proc.find( "<press_", 0, 7 );
                        while( pos != std::string::npos ) {
                            size_t pos2 = line_proc.find( ">", pos, 1 );

                            std::string action = line_proc.substr( pos + 7, pos2 - pos - 7 );
                            std::string replace = "<color_light_blue>" +
                                                  press_x( look_up_action( action ), "", "" ) + "</color>";

                            if( replace.empty() ) {
                                debugmsg( "Help json: Unknown action: %s", action );
                            } else {
                                line_proc = string_replace(
                                                line_proc, "<press_" + std::move( action ) + ">", replace );
                            }

                            pos = line_proc.find( "<press_", pos2, 7 );
                        }
                        return line_proc;
                    } );

                    if( !i18n_help_texts.empty() ) {
                        ui.on_screen_resize( nullptr );

                        const auto get_w_help_border = [&]() {
                            init_windows( ui );
                            return w_help_border;
                        };

                        scrollable_text( get_w_help_border, _( "Help" ),
                                         std::accumulate( i18n_help_texts.begin() + 1, i18n_help_texts.end(),
                                                          i18n_help_texts.front(),
                        []( std::string lhs, const std::string & rhs ) {
                            return std::move( lhs ) + "\n\n" + rhs;
                        } ) );

                        ui.on_screen_resize( init_windows );
                    }
                    action = "CONFIRM";
                    break;
                }
            }
        }
    } while( action != "QUIT" );
}

std::string get_hint()
{
    return SNIPPET.random_from_category( "hint" ).value_or( translation() ).translated();
}
