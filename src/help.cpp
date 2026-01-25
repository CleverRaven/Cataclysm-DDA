#include "help.h"

#include <cmath>
#include <iterator>
#include <string>
#include <vector>

#include "cata_imgui.h"
#include "cata_path.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "input_context.h"
#include "input_enums.h"
#include "npc.h"
#include "output.h"
#include "path_info.h"
#include "string_formatter.h"
#include "text.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"

help_data &get_help()
{
    static help_data single_instance;
    return single_instance;
}

void help_data::reset()
{
    current_order_start = 0;
    current_src = "";
    _help_categories.clear();
    _read_categories.clear();
}

void help_data::load( const JsonObject &jo, const std::string &src )
{
    if( src == "dda" ) {
        jo.throw_error( string_format( "Vanilla help must be located in %s",
                                       PATH_INFO::jsondir().generic_u8string() ) );
    }
    if( src != current_src ) {
        current_order_start = _help_categories.empty() ? 0 : _help_categories.crbegin()->first + 1;
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
                translation subtitle;
                mandatory( jobj, false, "subtitle", subtitle );
                category.paragraphs.emplace_back( subtitle, MM_SUBTITLE );
            } else if( jobj.has_string( "force_monospaced" ) ) {
                translation force_monospaced;
                mandatory( jobj, false, "force_monospaced", force_monospaced );
                category.paragraphs.emplace_back( force_monospaced, MM_MONOFONT );
            } else if( jobj.has_string( "separator" ) ) {
                auto it = category.paragraphs.emplace_back( no_translation( jobj.get_string( "separator" ) ),
                          MM_SEPARATOR );
                // Verify color now bc later the debugmsg would cause an ImGui CTD, don't use color_from_string's debugmsg though bc that will be useless context wise
                if( color_from_string( it.first.translated(), report_color_error::no ) == c_unset ) {
                    jobj.throw_error_at( "separator", "Can't parse color" );
                }
            }
        }
    }


    const int modified_order = jo.get_int( "order" ) + current_order_start;
    if( !_help_categories.try_emplace( modified_order, category ).second ) {
        jo.throw_error_at( "order", "\"order\" must be unique per source" );
    }
}

bool help_window::is_space_for_two_cat()
{
    static std::optional<float> two_column_min_width;
    if( !two_column_min_width ) {
        const std::map<const int, const help_category> &cats = data.get_help_categories();
        float longest = 0;
        for( const auto &cat : cats ) {
            float cat_x = ImGui::CalcTextSize( cat.second.name.translated().c_str() ).x;
            if( longest < cat_x ) {
                longest = cat_x;
            }
        }
        two_column_min_width = ( 2 * longest ) + 20.0f; // Guarentee some padding
    }
    return ImGui::GetContentRegionAvail().x > *two_column_min_width;
}

help_window::help_window() : cataimgui::window( "help",
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavFocus )
{

    ctxt = input_context( "DISPLAY_HELP", keyboard_mode::keychar );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    // Mouse selection
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    // Generated shortcuts
    ctxt.register_action( "ANY_INPUT" );
    // Switching between category options and scrolling open category
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_updown();
    // Switching between category options and open categories
    ctxt.register_leftright();
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    const hotkey_queue &hkq = hotkey_queue::alphabets();
    input_event next_hotkey = ctxt.first_unassigned_hotkey( hkq );
    for( const std::pair<const int, const help_category> &option_category :
         data.get_help_categories() ) {
        hotkeys.emplace( option_category.first, next_hotkey );
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
    selected_option = -1;
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    if( !is_space_for_two_cat() ) {
        format_title();
        if( ImGui::BeginChild( "Category Options" ) ) {
            for( const auto &cat : cats ) {
                draw_category_option( cat.first, cat.second );
            }
            ImGui::EndChild();
        }
    } else {
        format_title();
        // Split the categories in half
        if( ImGui::BeginTable( "Category Options", 2, ImGuiTableFlags_None ) ) {
            ImGui::TableSetupColumn( "Left Column" );
            ImGui::TableSetupColumn( "Right Column" );
            int half_size = cats.size() / 2;
            if( cats.size() % 2 != 0 ) {
                half_size++;
            }
            auto half_it = cats.begin();
            std::advance( half_it, half_size );
            auto jt = cats.begin();
            std::advance( jt, half_size - 1 );
            for( auto it = cats.begin(); it != half_it; it++ ) {
                ImGui::TableNextColumn();
                draw_category_option( it->first, it->second );
                ImGui::TableNextColumn();
                if( jt != std::prev( cats.end() ) ) {
                    jt++;
                    draw_category_option( jt->first, jt->second );
                }
            }
            ImGui::EndTable();
        }
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
    if( data.is_read( option ) ) {
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

void help_window::draw_category()
{
    const help_category &cat = data.get_help_category( loaded_option );
    const std::string prev_cat_name =
        data.get_help_category( previous_option( loaded_option ) ).name.translated();
    const std::string next_cat_name =
        data.get_help_category( next_option( loaded_option ) ).name.translated();
    const bool scroll_title = ImGui::GetContentRegionAvail().y < 500.0f;
    const bool show_footer = is_space_for_two_cat();

    int child_rows = TERMY;
    child_rows -= scroll_title ? 0 : 5;
    child_rows -= show_footer ? 5 : 0;

    if( !scroll_title ) {
        format_title( cat );
    }

    ImGui::Indent( one_em );
    if( ImGui::BeginChild( "HELP_PARAGRAPHS",
                           ImVec2( -1, static_cast<float>( str_height_to_pixels( child_rows ) ) ) ) ) {

        cataimgui::set_scroll( s );
        if( scroll_title ) {
            cataimgui::TextColoredParagraph( c_white, cat.name.translated() );
            ImGui::NewLine();
            ImGui::NewLine();
        }
        for( const std::pair<std::string, message_modifier> &translated_paragraph :
             translated_paragraphs ) {
            switch( translated_paragraph.second ) {
                case MM_NORMAL:
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    break;
                case MM_SUBTITLE:
                    format_subtitle( translated_paragraph.first, cat.color );
                    break;
                case MM_MONOFONT:
                    cataimgui::PushMonoFont();
                    cataimgui::draw_colored_text( translated_paragraph.first, c_white, get_wrap_width() );
                    ImGui::PopFont();
                    break;
                case MM_SEPARATOR: {
                    // Can't verify here bc the debugmsg would cause an ImGui CTD
                    nc_color separator_color = color_from_string( translated_paragraph.first, report_color_error::no );
                    if( separator_color == c_unset ) {
                        separator_color = cat.color;
                    }
                    ImGui::PushStyleColor( ImGuiCol_Separator, separator_color );
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
        ImGui::EndChild();
    }
    if( show_footer ) {
        format_footer( prev_cat_name, next_cat_name, cat.color );
    }
}

cataimgui::bounds help_window::get_bounds()
{
    return {0, 0, 1.0, 1.0};
}

float help_window::get_wrap_width()
{
    return static_cast<float>( str_width_to_pixels( TERMX - 6 ) );
}

void help_window::show()
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();
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
                has_selected_category = cats.find( selected_option ) != cats.end();
                if( has_selected_category ) {
                    loaded_option = selected_option;
                    swap_translated_paragraphs();
                    data.mark_read( loaded_option );
                    selected_option = -1;
                    window_flags |= ImGuiWindowFlags_NoNavInputs;
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

            if( selected_option != -1 ) {
                if( action == "SELECT" || action == "CONFIRM" ) {
                    s = cataimgui::scroll::begin;
                    loaded_option = selected_option;
                    swap_translated_paragraphs();
                    data.mark_read( loaded_option );
                    continue;
                }
                selected_option = -1;
            }

            if( action == "UP" ) {
                s = cataimgui::scroll::line_up;
            } else if( action == "DOWN" ) {
                s = cataimgui::scroll::line_down;
            } else if( action == "PAGE_UP" ) {
                s = cataimgui::scroll::page_up;
            } else if( action == "PAGE_DOWN" ) {
                s = cataimgui::scroll::page_down;
            } else if( action == "CONFIRM" || action == "QUIT" ) {
                s = cataimgui::scroll::begin;
                has_selected_category = false;
                window_flags = window_flags & ~ImGuiWindowFlags_NoNavInputs;
            } else if( action == "LEFT" || action == "PREV_TAB" ) {
                s = cataimgui::scroll::begin;
                loaded_option = previous_option( loaded_option );
                swap_translated_paragraphs();
                data.mark_read( loaded_option );
            } else if( action == "RIGHT" || action == "NEXT_TAB" ) {
                s = cataimgui::scroll::begin;
                loaded_option = next_option( loaded_option );
                swap_translated_paragraphs();
                data.mark_read( loaded_option );
            }
        }
    }
}

void help_window::swap_translated_paragraphs()
{
    Character &u = get_player_character();
    translated_paragraphs.clear();
    const help_category &cat = data.get_help_category( loaded_option );

    for( const auto &[untranslated_paragraph, modifier] : cat.paragraphs ) {
        std::string &translated_paragraph = translated_paragraphs.emplace_back(
                                                untranslated_paragraph.translated(), modifier ).first;
        parse_tags( translated_paragraph, u, u );
    }
}

int help_window::previous_option( int option )
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    auto it = cats.find( option );
    cata_assert( it != cats.end() );
    if( --it == --cats.begin() ) {
        it = --cats.end();
    }
    return it->first;
}

int help_window::next_option( int option )
{
    const std::map<const int, const help_category> &cats = data.get_help_categories();
    auto it = cats.find( option );
    cata_assert( it != cats.end() );
    if( ++it == cats.end() ) {
        it = cats.begin();
    }
    return it->first;
}

void help_window::format_title( std::optional<help_category> category ) const
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

void help_window::format_subtitle( const std::string_view &translated_category_name,
                                   const nc_color &category_color ) const
{
    if( screen_reader ) {
        cataimgui::TextColoredParagraph( c_white, translated_category_name );
        ImGui::NewLine();
        return;
    }

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

void help_window::format_footer( const std::string &prev, const std::string &next,
                                 const nc_color &category_color )
{
    if( screen_reader ) {
        const std::string screen_reader_footer = string_format(
                    //~ Help menu footer when using screen reader
                    _( "Previous category: %s. Next category: %s" ), prev, next );
        cataimgui::TextColoredParagraph( c_white, screen_reader_footer );
        return;
    }

    const float next_size = ImGui::CalcTextSize( next.c_str() ).x;
    const float brackets_size = ImGui::CalcTextSize( "<]" ).x;
    const float next_x = ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - next_size -
                         brackets_size - one_em;
    ImGui::PushStyleColor( ImGuiCol_Separator, category_color );
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::PushStyleColor( ImGuiCol_Text, c_yellow );
    ImGui::Text( "<" );
    ImGui::SameLine( 0.f, 0.f );
    ImGui::PopStyleColor( 1 );
    ImGui::Text( "%s", prev.c_str() );
    if( ImGui::IsItemHovered() ) {
        selected_option = previous_option( loaded_option );
    }
    ImGui::SameLine( 0.f, 0.f );
    ImGui::PushStyleColor( ImGuiCol_Text, c_yellow );
    ImGui::Text( "]" );
    ImGui::PopStyleColor( 1 );
    ImGui::SameLine( next_x );
    ImGui::PushStyleColor( ImGuiCol_Text, c_yellow );
    ImGui::Text( "[" );
    ImGui::PopStyleColor( 1 );
    ImGui::SameLine( 0.f, 0.f );
    ImGui::Text( "%s", next.c_str() );
    if( ImGui::IsItemHovered() ) {
        selected_option = next_option( loaded_option );
    }
    ImGui::PushStyleColor( ImGuiCol_Text, c_yellow );
    ImGui::SameLine( 0.f, 0.f );
    ImGui::Text( ">" );
    ImGui::PopStyleColor( 2 );
}
