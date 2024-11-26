#include "help.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cata_imgui.h"
#include "input_context.h"
#include "imgui/imgui.h"
#include "output.h"

#include "action.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "input_context.h"
#include "input_enums.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_helpers.h"
#include "ui_manager.h"

// Imgui planning
////////////////////////////////////////////////////////////////////////
// Tabular full screen menu
// 1 Tab per mod (overflow to an additional tab if more than 52 entries)
// General read/unread functionality
// Short desc for each category?
// Handling for tiny screens (mobile)

class JsonObject;

help &get_help()
{
    static help single_instance;
    return single_instance;
}

void help::load( const JsonObject &jo, const std::string &src )
{
    get_help().load_object( jo, src );
}

void help::reset()
{
    get_help().reset_instance();
}

void help::reset_instance()
{
    current_order_start = 0;
    current_src = "";
    help_texts.clear();
}

void help::load_object( const JsonObject &jo, const std::string &src )
{
    if( src == "dda" ) {
        jo.throw_error( string_format( "Vanilla help must be located in %s",
                                       PATH_INFO::jsondir().generic_u8string() ) );
    }
    if( src != current_src ) {
        current_order_start = help_texts.empty() ? 0 : help_texts.crbegin()->first + 1;
        current_src = src;
    }
    std::vector<translation> messages;
    jo.read( "messages", messages );

    translation name;
    jo.read( "name", name );
    const int modified_order = jo.get_int( "order" ) + current_order_start;
    if( !help_texts.try_emplace( modified_order, std::make_pair( name, messages ) ).second ) {
        jo.throw_error_at( "order", "\"order\" must be unique (per src)" );
    }
}

std::string help_window::get_dir_grid()
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

std::string help_window::get_note_colors()
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

help_window::help_window() : cataimgui::window( "help",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoNavInputs )
{
    input_context ctxt( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    //ctxt.register_action( "NEXT_TAB" );
    //ctxt.register_action( "PREV_TAB" );
    // Mouse selection
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    // Generated shortcuts
    ctxt.register_action( "ANY_INPUT" );
    const hotkey_queue &hkq = hotkey_queue::alphabets();
    input_event next_hotkey = ctxt.first_unassigned_hotkey( hkq );
    for( const auto &text : data.help_texts ) {
        hotkeys.emplace( text.first, next_hotkey );
        next_hotkey = ctxt.next_unassigned_hotkey( hkq, next_hotkey );
    }
}

void help_window::draw_controls()
{
    //if( ImGui::BeginTabBar( "idk" ) ) {
    //    ImGui::EndTabBar();
    //}
    mouse_selected_option = -1;
    size_t ind = 0;
    for( const auto &text : data.help_texts ) {
        std::string cat_name;
        auto hotkey_it = hotkeys.find( text.first );
        if( hotkey_it != hotkeys.end() ) {
            cat_name = hotkey_it->second.short_description();
            cat_name += ": ";
        }
        cat_name += text.second.first.translated();
        ImGui::Button( remove_color_tags( cat_name ).c_str() );
        if( ImGui::IsItemHovered() ) {
            mouse_selected_option = ind;
        }
        if( keyboard_selected_option != last_keyboard_selected_option &&
            keyboard_selected_option == short( ind ) && ImGui::IsWindowFocused() ) {
            ImGui::SetKeyboardFocusHere( -1 );
        }
        ind++;
    }
}

void help_window::draw_category( translation &category_name, std::vector<translation> &paragraphs )
{
    cataimgui::draw_colored_text( category_name.translated() );
    for( const translation &paragraph : paragraphs ) {
        ImGui::NewLine();
        ImGui::NewLine();
        cataimgui::draw_colored_text( paragraph.translated() );
    }
    mark_resized();
    while( true ) {
        ui_manager::redraw_invalidated();
        const std::string action = ctxt.handle_input( 50 );
        if( action == "CONFIRM" || action == "QUIT" ) {
            return;
        }
    }
}

cataimgui::bounds help_window::get_bounds()
{
    return bounds;
}

void help_window::show()
{
    size_t selected = 0;
    while( true ) {
        ui_manager::redraw_invalidated();
        std::string action = ctxt.handle_input( 50 );
        //const input_event event = ctxt.get_raw_input();
        //if( action == "NEXT_TAB" ) {
        //    switch_target = cur_target;
        //    ++switch_target;
        //} else if( action == "PREV_TAB" ) {
        //    switch_target = cur_target;
        //    --switch_target;
        //} else
        if( action == "SELECT" && mouse_selected_option != -1 ) {
            action = "CONFIRM";
            selected = size_t( mouse_selected_option );
        }
        if( action == "CONFIRM" ) {
            if( auto it = data.help_texts.find( selected ); it != data.help_texts.end() ) {
                draw_category( it->second.first, it->second.second );
            }
        } else if( action == "QUIT" ) {
            return;
        }
    }
}

std::string get_hint()
{
    return SNIPPET.random_from_category( "hint" ).value_or( translation() ).translated();
}
