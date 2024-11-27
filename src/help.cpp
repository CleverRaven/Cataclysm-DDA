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

// Imgui migration planning
////////////////////////////////////////////////////////////////////////
// Full screen category selection menu
// When you click a category/press a hotkey the categories paragraphs are presented with the categories name at the top, Esc to go back to selection
// Category paragraphs must word wrap and be scrollable
// If modded help is present, one tab per mod (overflow to an additional tab if more than 52 entries?), otherwise no tabs
// General read/unread functionality that changes the colour on the selection menu (no permanence for now)
// Split name into name and short desc for each category to make use of the extra space
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
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize )
{
    ctxt = input_context( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
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
    if( !selected_category ) {
        draw_category_selection();
    } else {
        draw_category( category.first, category.second );
    }
}

void help_window::draw_category_selection()
{
    mouse_selected_option = -1;
    //~ Help menu header
    format_title( _( "Help" ) );
    if( ImGui::BeginTable( "Category Options", 3, ImGuiTableFlags_None ) ) {
        ImGui::TableSetupColumn( "Left Column", ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 2.0f ) );
        ImGui::TableSetupColumn( "Right Column", ImGuiTableColumnFlags_WidthStretch,
                                 static_cast<float>( window_width / 2.0f ) );
        int half_size = static_cast<float>( data.help_texts.size() / 2.0f ) + 1;
        auto half_it = data.help_texts.begin();
        std::advance( half_it, half_size );
        auto jt = data.help_texts.begin();
        std::advance( jt, half_size );
        for( auto it = data.help_texts.begin(); it != half_it; it++, jt++ ) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            display_category_option( *it );
            ImGui::TableNextColumn();
            if( jt != data.help_texts.end() ) {
                display_category_option( *jt );
            }
        }
        ImGui::EndTable();
    }
}

void help_window::display_category_option(
    const std::pair<const int, std::pair<translation, std::vector<translation>>> &help_text )
{
    std::string cat_name;
    auto hotkey_it = hotkeys.find( help_text.first );
    if( hotkey_it != hotkeys.end() ) {
        cat_name = hotkey_it->second.short_description();
        cat_name += ": ";
    }
    cat_name += help_text.second.first.translated();
    ImGui::Selectable( remove_color_tags( cat_name ).c_str() );
    if( ImGui::IsItemHovered() ) {
        mouse_selected_option = help_text.first;
    }
}

std::string help_window::seperator( int length, char c )
{
    std::string ret = "<color_light_blue>";
    ret += std::string( length, c );
    ret += "</color>";
    return ret;
}

void help_window::format_title( const std::string translated_category_name )
{
    std::string row3 = "<color_light_blue>║</color> ";
    row3 += translated_category_name;
    row3 += " <color_light_blue>║</color>";
    std::string div = seperator( remove_color_tags( row3 ).length() - 4, '=' );
    cataimgui::draw_colored_text( div );
    cataimgui::draw_colored_text( row3 );
    cataimgui::draw_colored_text( div );
    cataimgui::draw_colored_text( seperator( static_cast<int>( wrap_width ), '_' ) );
    ImGui::NewLine();
}

void help_window::draw_category( translation &category_name, std::vector<translation> &paragraphs )
{
    format_title( category_name.translated() );
    for( const translation &paragraph : paragraphs ) {
        cataimgui::draw_colored_text( parse_tags_help_window( paragraph.translated() ), wrap_width );
        ImGui::NewLine();
    }
    input_context cat_cxt( "DISPLAY_HELP_CATEGORY", keyboard_mode::keychar );
    cat_cxt.register_action( "QUIT" );
    cat_cxt.register_action( "CONFIRM" );
}

// Would ideally be merged with parse_tags()
std::string help_window::parse_tags_help_window( std::string translated_line )
{
    if( translated_line == "<DRAW_NOTE_COLORS>" ) {
        translated_line = get_note_colors();
    } else if( translated_line == "<HELP_DRAW_DIRECTIONS>" ) {
        translated_line = get_dir_grid();
    }
    size_t pos = translated_line.find( "<press_", 0, 7 );
    while( pos != std::string::npos ) {
        size_t pos2 = translated_line.find( ">", pos, 1 );

        std::string action = translated_line.substr( pos + 7, pos2 - pos - 7 );
        std::string replace = "<color_light_blue>" +
                              press_x( look_up_action( action ), "", "" ) + "</color>";

        if( replace.empty() ) {
            debugmsg( "Help json: Unknown action: %s", action );
        } else {
            translated_line = string_replace(
                                  translated_line, "<press_" + std::move( action ) + ">", replace );
        }

        pos = translated_line.find( "<press_", pos2, 7 );
    }
    return translated_line;
}

cataimgui::bounds help_window::get_bounds()
{
    return bounds;
}

void help_window::show()
{
    int selected = -1;
    while( true ) {
        while( !selected_category ) {
            ui_manager::redraw_invalidated();
            std::string action = ctxt.handle_input( 50 );

            if( action == "SELECT" && mouse_selected_option != -1 ) {
                action = "CONFIRM";
                selected = mouse_selected_option;
            }

            if( action == "CONFIRM" && selected != -1 ) {
                auto it = data.help_texts.find( selected );
                selected_category = it != data.help_texts.end();
                if( selected_category ) {
                    category = it->second;
                } else {
                    debugmsg( "Category not found" );
                }
            } else if( action == "QUIT" ) {
                return;
            }
        }
        while( selected_category ) {
            ui_manager::redraw_invalidated();
            std::string action = ctxt.handle_input( 50 );

            if( action == "CONFIRM" || action == "QUIT" ) {
                selected_category = false;
            }
        }
    }

}

std::string get_hint()
{
    return SNIPPET.random_from_category( "hint" ).value_or( translation() ).translated();
}
