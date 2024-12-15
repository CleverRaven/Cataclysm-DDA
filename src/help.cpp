#include "help.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "action.h"
#include "cata_imgui.h"
#include "color.h"
#include "debug.h"
#include "imgui/imgui.h"
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
    help_categories.clear();
    read_categories.clear();
}

void help::load_object( const JsonObject &jo, const std::string &src )
{
    if( src == "dda" ) {
        jo.throw_error( string_format( "Vanilla help must be located in %s",
                                       PATH_INFO::jsondir().generic_u8string() ) );
    }
    if( src != current_src ) {
        current_order_start = help_categories.empty() ? 0 : help_categories.crbegin()->first + 1;
        current_src = src;
    }
    help_category category;
    jo.read( "messages", category.paragraphs );

    translation name;
    jo.read( "name", category.name );
    const int modified_order = jo.get_int( "order" ) + current_order_start;
    if( !help_categories.try_emplace( modified_order, category ).second ) {
        jo.throw_error_at( "order", "\"order\" must be unique per source" );
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

void help_window::note_colors()
{
    ImGui::TextUnformatted( _( "Note colors: " ) );
    ImGui::SameLine( 0.f, 0.f );
    for( const auto &color_pair : get_note_color_names() ) {
        // The color index is not translatable, but the name is.
        //~ %1$s: note color abbreviation, %2$s: note color name
        cataimgui::TextColoredParagraph( c_white, string_format( pgettext( "note color", "%1$s:%2$s, " ),
                                         colorize( color_pair.first, color_pair.second.color ),
                                         color_pair.second.name ) );
        // TODO: Has a stray comma at the end
        ImGui::SameLine( 0.f, 0.f );
    }
}

help_window::help_window() : cataimgui::window( "help",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse )
{

    ctxt = input_context( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    // Mouse selection
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    // Generated shortcuts
    ctxt.register_action( "ANY_INPUT" );
    // Scrolling open category
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_updown();
    // Switching between categories while open
    ctxt.register_leftright();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    const hotkey_queue &hkq = hotkey_queue::alphabets();
    input_event next_hotkey = ctxt.first_unassigned_hotkey( hkq );
    for( const auto &text : data.help_categories ) {
        hotkeys.emplace( text.first, next_hotkey );
        next_hotkey = ctxt.next_unassigned_hotkey( hkq, next_hotkey );
    }
}

void help_window::draw_controls()
{
    if( !has_selected_category ) {
        draw_category_selection();
    } else {
        draw_category();
    }
}

void help_window::draw_category_selection()
{
    // TODO: Add one column display for tiny screens and screen reader users
    selected_option = -1;
    //~ Help menu header
    format_title( _( "Help" ) );
    // Split the categories in half
    if( ImGui::BeginTable( "Category Options", 2, ImGuiTableFlags_None ) ) {
        ImGui::TableSetupColumn( "Left Column", ImGuiTableColumnFlags_WidthStretch, 1.0f );
        ImGui::TableSetupColumn( "Right Column", ImGuiTableColumnFlags_WidthStretch, 1.0f );
        int half_size = static_cast<float>( data.help_categories.size() / 2.0f ) + 1;
        auto half_it = data.help_categories.begin();
        std::advance( half_it, half_size );
        auto jt = data.help_categories.begin();
        std::advance( jt, half_size );
        for( auto it = data.help_categories.begin(); it != half_it; it++, jt++ ) {
            ImGui::TableNextColumn();
            draw_category_option( it->first, it->second );
            ImGui::TableNextColumn();
            if( jt != data.help_categories.end() ) {
                draw_category_option( jt->first, jt->second );
            }
        }
        ImGui::EndTable();
    }
}

void help_window::draw_category_option( const int &option, const help_category &category )
{
    std::string cat_name;
    auto hotkey_it = hotkeys.find( option );
    if( hotkey_it != hotkeys.end() ) {
        cat_name = hotkey_it->second.short_description();
        cat_name += ": ";
    }
    cat_name += category.name.translated();
    if( data.read_categories.find( option ) != data.read_categories.end() ) {
        ImGui::PushStyleColor( ImGuiCol_Text, c_light_gray );
        ImGui::Selectable( remove_color_tags( cat_name ).c_str() );
        ImGui::PopStyleColor();
    } else {
        ImGui::Selectable( remove_color_tags( cat_name ).c_str() );
    }
    if( ImGui::IsItemHovered() ) {
        selected_option = option;
    }
}

void help_window::format_title( const std::string translated_category_name )
{
    if( get_option<bool>( "SCREEN_READER_MODE" ) ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }
    const float title_length = ImGui::CalcTextSize( remove_color_tags(
                                   translated_category_name ).c_str() ).x;
    ImGui::PushStyleVarX( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleVarY( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleColor( ImGuiCol_Text, c_light_blue );
    ImGui::PushStyleColor( ImGuiCol_Separator, c_light_blue );
    cataimgui::PushMonoFont();
    const int sep_len = std::ceil( ( title_length / ImGui::CalcTextSize( "═" ).x )  + 2 );
    ImGui::Text( "╔" );
    ImGui::SameLine();
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::Text( "═" );
        ImGui::SameLine();
    }
    const int x = ImGui::GetCursorPosX();
    ImGui::Text( "╗" );
    ImGui::Text( "║ " );
    ImGui::SameLine();
    const ImVec2 text_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX( x );
    ImGui::Text( "║" );
    ImGui::Text( "╚" );
    ImGui::SameLine();
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::Text( "═" );
        ImGui::SameLine();
    }
    ImGui::Text( "╝" );
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    const ImVec2 end_pos = ImGui::GetCursorPos();
    ImGui::PopFont();
    ImGui::PopStyleColor( 2 );
    ImGui::PopStyleVar( 2 );
    ImGui::SetCursorPos( text_pos );
    cataimgui::TextColoredParagraph( c_white, translated_category_name );
    ImGui::SetCursorPos( end_pos );
}

//void help_window::format_subtitle( const std::string translated_category_name )
//{
//    if( get_option<bool>( "SCREEN_READER_MODE" ) ) {
//        cataimgui::TextColoredParagraph( c_white, translated_category_name );
//        ImGui::NewLine();
//        return;
//    }
//    const float title_length = ImGui::CalcTextSize( remove_color_tags(
//                                   translated_category_name ).c_str() ).x;
//    cataimgui::PushMonoFont();
//    const int sep_len = std::ceil( ( title_length / ImGui::CalcTextSize( "═" ).x )  + 4 );
//    ImGui::PushStyleColor( ImGuiCol_Text, c_light_blue );
//    for( int i = sep_len; i > 0; i-- ) {
//        ImGui::Text( "▁" );
//        ImGui::SameLine( 0.f, 0.f );
//    }
//    ImGui::NewLine();
//    // Using the matching box character doesn't look good bc there's forced(?) y spacing on NewLine
//    ImGui::Text( "▏ " );
//    ImGui::SameLine( 0.f, 0.f );
//    ImGui::PopStyleColor();
//    ImGui::PopFont();
//    cataimgui::TextColoredParagraph( c_white, translated_category_name );
//    cataimgui::PushMonoFont();
//    ImGui::SameLine( 0.f, 0.f );
//    ImGui::PushStyleColor( ImGuiCol_Text, c_light_blue );
//    ImGui::Text( " ▕" );
//    for( int i = sep_len; i > 0; i-- ) {
//        ImGui::Text( "▔" );
//        ImGui::SameLine( 0.f, 0.f );
//    }
//    ImGui::PopStyleColor();
//    ImGui::PopFont();
//    ImGui::NewLine();
//    ImGui::PushStyleColor( ImGuiCol_Separator, c_light_blue );
//    ImGui::Separator();
//    ImGui::PopStyleColor();
//}

void help_window::draw_category()
{
    const help_category &cat = data.help_categories[loaded_option];
    format_title( cat.name.translated() );
    // Use a table so we can scroll the category paragraphs without the title
    if( ImGui::BeginTable( "HELP_PARAGRAPHS", 1,
                           ImGuiTableFlags_ScrollY ) ) {
        cataimgui::set_scroll( s );
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        for( const std::string &translated_paragraph : translated_paragraphs ) {
            if( translated_paragraph == "<DRAW_NOTE_COLORS>" ) {
                note_colors();
                continue;
            } else if( translated_paragraph == "<HELP_DRAW_DIRECTIONS>" ) {
                static const std::string dir_grid = get_dir_grid();
                cataimgui::draw_colored_text( dir_grid );
                continue;
            }
            cataimgui::TextColoredParagraph( c_white, translated_paragraph );
            ImGui::NewLine();
            ImGui::NewLine();
        }
        ImGui::EndTable();
    }
}

// Would ideally be merged with parse_tags()?
void help_window::parse_tags_help_window()
{
    for( std::string &translated_paragraph : translated_paragraphs ) {
        if( translated_paragraph == "<DRAW_NOTE_COLORS>" ) {
            continue;
        } else if( translated_paragraph == "<HELP_DRAW_DIRECTIONS>" ) {
            continue;
        }
        size_t pos = translated_paragraph.find( "<press_", 0, 7 );
        while( pos != std::string::npos ) {
            size_t pos2 = translated_paragraph.find( ">", pos, 1 );

            std::string action = translated_paragraph.substr( pos + 7, pos2 - pos - 7 );
            std::string replace = "<color_light_blue>" +
                                  press_x( look_up_action( action ), "", "" ) + "</color>";

            if( replace.empty() ) {
                debugmsg( "Help json: Unknown action: %s", action );
            } else {
                translated_paragraph = string_replace( translated_paragraph, "<press_" + std::move( action ) + ">",
                                                       replace );
            }

            pos = translated_paragraph.find( "<press_", pos2, 7 );
        }
    }
}

cataimgui::bounds help_window::get_bounds()
{
    return {0, 0, 1.0, 1.0};
}

void help_window::show()
{
    while( true ) {
        while( !has_selected_category ) {
            ui_manager::redraw_invalidated();
            std::string action = ctxt.handle_input( 50 );
            input_event input = ctxt.get_raw_input();

            for( const auto &hotkey_entry : hotkeys ) {
                if( input == hotkey_entry.second ) {
                    action = "CONFIRM";
                    selected_option = hotkey_entry.first;
                    break;
                }
            }

            if( selected_option != -1 && ( action == "SELECT" || action == "CONFIRM" ) ) {
                has_selected_category = data.help_categories.find( selected_option ) != data.help_categories.end();
                if( has_selected_category ) {
                    loaded_option = selected_option;
                    swap_translated_paragraphs();
                    data.read_categories.insert( loaded_option );
                } else {
                    debugmsg( "Category not found: option %s", selected_option );
                }
            } else if( action == "QUIT" ) {
                return;
            }
        }
        while( has_selected_category ) {
            ui_manager::redraw_invalidated();
            std::string action = ctxt.handle_input( 50 );

            if( action == "UP" ) {
                s = cataimgui::scroll::line_up;
            } else if( action == "DOWN" ) {
                s = cataimgui::scroll::line_down;
            } else if( action == "PAGE_UP" ) {
                s = cataimgui::scroll::page_up;
            } else if( action == "PAGE_DOWN" ) {
                s = cataimgui::scroll::page_down;
            } else if( action == "CONFIRM" || action == "QUIT" ) {
                has_selected_category = false;
            } else if( action == "LEFT" || action == "PREV_TAB" ) {
                auto it = data.help_categories.find( loaded_option );
                loaded_option = it != data.help_categories.begin() ? ( --it )->first :
                                data.help_categories.rbegin()->first;
                swap_translated_paragraphs();
                data.read_categories.insert( loaded_option );
            } else if( action == "RIGHT" || action == "NEXT_TAB" ) {
                auto it = data.help_categories.find( loaded_option );
                it++;
                loaded_option = it != data.help_categories.end() ? it->first : data.help_categories.begin()->first;
                swap_translated_paragraphs();
                data.read_categories.insert( loaded_option );
            }
        }
    }
}

void help_window::swap_translated_paragraphs()
{
    translated_paragraphs.clear();
    const help_category &cat = data.help_categories[loaded_option];
    for( const translation &paragraph : cat.paragraphs ) {
        translated_paragraphs.emplace_back( paragraph.translated() );
    }
    parse_tags_help_window();
}

std::string get_hint()
{
    return SNIPPET.random_from_category( "hint" ).value_or( translation() ).translated();
}
