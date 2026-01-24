#include "help.h"

#include <cmath>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "cata_imgui.h"
#include "cata_path.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "input_enums.h"
#include "output.h"
#include "npc.h"
#include "path_info.h"
#include "string_formatter.h"
#include "text.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"

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

enum message_modifier {
    MM_NORMAL = 0, // Normal message
    MM_SUBTITLE, // Formatted subtitle
    MM_MONOFONT, // Forced monofont for fixed space diagrams
    MM_SEPERATOR // ImGui seperator, value is the color to use
};

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

    translation name;
    jo.read( "name", category.name );
    if( jo.has_string( "color" ) ) {
        const std::string color_string = jo.get_string( "color" );
        category.color = color_from_string( color_string, report_color_error::no );
        if( category.color == c_unset ) {
            jo.throw_error_at( "color", "Can't parse color" );
            category.color = c_light_blue;
        }
    }

    for( JsonValue jv : jo.get_array( "messages" ) ) {
        if( jv.test_string() ) {
            category.paragraphs.emplace_back( to_translation( jv.get_string() ), MM_NORMAL );
        } else {
            JsonObject jobj = jv.get_object();
            if( jobj.has_string( "subtitle" ) ) {
                category.paragraphs.emplace_back( to_translation( jobj.get_string( "subtitle" ) ), MM_SUBTITLE );
            } else if( jobj.has_string( "force_monospaced" ) ) {
                category.paragraphs.emplace_back( to_translation( jobj.get_string( "force_monospaced" ) ),
                                                  MM_MONOFONT );
            } else if( jobj.has_string( "seperator" ) ) {
                auto it = category.paragraphs.emplace_back( no_translation( jobj.get_string( "seperator" ) ),
                          MM_SEPERATOR );
                // Verify color now bc later the debugmsg would cause an ImGui CTD, don't use color_from_string's debugmsg though bc that will be useless context wise
                if( color_from_string( it.first.translated(), report_color_error::no ) == c_unset ) {
                    jobj.throw_error_at( "seperator", "Can't parse color" );
                }
            }
        }
    }


    const int modified_order = jo.get_int( "order" ) + current_order_start;
    if( !help_categories.try_emplace( modified_order, category ).second ) {
        jo.throw_error_at( "order", "\"order\" must be unique per source" );
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
    ctxt.register_action( "HELP_KEYBINDINGS" );
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
    //TODO: Add one column display for tiny screens and screen reader users
    selected_option = -1;
    format_title();
    // Split the categories in half
    if( ImGui::BeginTable( "Category Options", 2, ImGuiTableFlags_None ) ) {
        ImGui::TableSetupColumn( "Left Column", ImGuiTableColumnFlags_WidthStretch, 1.0f );
        ImGui::TableSetupColumn( "Right Column", ImGuiTableColumnFlags_WidthStretch, 1.0f );
        int half_size = data.help_categories.size() / 2;
        if( data.help_categories.size() % 2 != 0 ) {
            half_size++;
        }
        auto half_it = data.help_categories.begin();
        std::advance( half_it, half_size );
        auto jt = data.help_categories.begin();
        std::advance( jt, half_size - 1 );
        for( auto it = data.help_categories.begin(); it != half_it; it++ ) {
            ImGui::TableNextColumn();
            draw_category_option( it->first, it->second );
            ImGui::TableNextColumn();
            if( jt != std::prev( data.help_categories.end() ) ) {
                jt++;
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
        if( screen_reader ) {
            //~ Prefix for options that has already been viewed when using a screen reader
            cat_name = _( "(read) " ) + cat_name;
        }
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

void help_window::format_title( std::optional<help_category> category )
{
    //~ Help menu header
    const std::string translated_category_name = !category ? _( "Help" ) : category->name.translated();

    if( screen_reader ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }

    nc_color category_color = !category ? c_light_blue : category->color;

    const float title_length = ImGui::CalcTextSize( remove_color_tags(
                                   translated_category_name ).c_str() ).x;
    ImGui::PushStyleVarX( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleVarY( ImGuiStyleVar_ItemSpacing, 0 );
    ImGui::PushStyleColor( ImGuiCol_Text, category_color );
    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
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

void help_window::format_subtitle( const help_category &category,
                                   const std::string translated_category_name )
{
    if( screen_reader ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }

    const nc_color &category_color = category.color;

    const float title_length = ImGui::CalcTextSize( remove_color_tags(
                                   translated_category_name ).c_str() ).x;
    cataimgui::PushMonoFont();
    const int sep_len = std::ceil( ( title_length / ImGui::CalcTextSize( "═" ).x )  + 4 );
    ImGui::PushStyleColor( ImGuiCol_Text, category_color );
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::Text( "▁" );
        ImGui::SameLine( 0.f, 0.f );
    }
    ImGui::NewLine();
    // Using the matching box character doesn't look good bc there's forced(?) y spacing on NewLine
    ImGui::Text( "▏ " );
    ImGui::SameLine( 0.f, 0.f );
    ImGui::PopStyleColor();
    ImGui::PopFont();
    cataimgui::TextColoredParagraph( c_white, translated_category_name );
    cataimgui::PushMonoFont();
    ImGui::SameLine( 0.f, 0.f );
    ImGui::PushStyleColor( ImGuiCol_Text, category_color );
    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
    ImGui::Text( " ▕" );
    for( int i = sep_len; i > 0; i-- ) {
        ImGui::Text( "▔" );
        ImGui::SameLine( 0.f, 0.f );
    }
    ImGui::PopFont();
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::PopStyleColor( 2 );
}

void help_window::draw_category()
{
    const help_category &cat = data.help_categories[loaded_option];
    format_title( cat );
    // Use a table so we can scroll the category paragraphs without the title
    //TODO: Consider not keeping the title seperate on tiny screens
    ImGui::Indent( one_em );
    if( ImGui::BeginTable( "HELP_PARAGRAPHS", 1, ImGuiTableFlags_ScrollY ) ) {
        cataimgui::set_scroll( s );
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        for( const std::pair<std::string, int> &translated_paragraph :
             translated_paragraphs ) {
            switch( translated_paragraph.second ) {
                case MM_NORMAL:
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    break;
                case MM_SUBTITLE:
                    format_subtitle( cat, translated_paragraph.first );
                    break;
                case MM_MONOFONT:
                    cataimgui::PushMonoFont();
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    ImGui::PopFont();
                    break;
                case MM_SEPERATOR: {
                    //Can't verify here bc the debugmsg would cause an ImGui CTD
                    nc_color seperator_color = color_from_string( translated_paragraph.first, report_color_error::no );
                    if( seperator_color == c_unset ) {
                        seperator_color = cat.color;
                    }
                    ImGui::PushStyleColor( ImGuiCol_Separator, seperator_color );
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                    break;
                }
                default:
                    debugmsg( "Unexpected help message modifier" );
                    continue;
            }
            ImGui::NewLine();
            ImGui::NewLine();
        }
        ImGui::EndTable();
    }
}

cataimgui::bounds help_window::get_bounds()
{
    return {0, 0, 1.0, 1.0};
}

float help_window::get_wrap_width()
{
    return static_cast<float>( str_width_to_pixels( TERMX - 5 ) );
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
    for( const std::pair<translation, int> &paragraph : cat.paragraphs ) {
        auto &translated_paragraph = translated_paragraphs.emplace_back(
                                         paragraph.first.translated(), paragraph.second );
        parse_tags( translated_paragraph.first, get_player_character(), get_player_character() );
    }
}

std::string get_hint()
{
    return SNIPPET.random_from_category( "hint" ).value_or( translation() ).translated();
}
