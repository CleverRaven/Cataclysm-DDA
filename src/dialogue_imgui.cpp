#include "dialogue_imgui.h"

#include <algorithm>
#include <imgui/imgui_internal.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "cata_imgui.h"
#include "debug.h"
#include "dialogue.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "output.h"
#include "string_formatter.h"
#include "talker.h"
#include "text.h"
#include "translation.h"
#include "ui_manager.h"

/*
* This is roughly what the window should look like at full layout.
* I've annotated the various parts to make it easier for you to identify what's doing each thing.
* Each of the entries in capital letters(except HORIZON) is a child window with its own dedicated draw function.
*
* The HORIZON line is the midway point between the DIALOGUE_HISTORY and DIALOGUE_RESPONSES child windows.
* See horizontal_separator_pos_y() for more.
*
*         ---------------------------------------
*         | SIDEBAR  |  DIALOGUE HISTORY        |
*         |(Portrait)|                          |
*         | (name)   |                          |
*         |          |                          |
*         |(Wield)   |                          |
*         |(Wear)    |                          |
*         |(Vis Mut) |                          |
*         |          |                          |
*         |(Stats)   |                          |
*         |(Traits)  |--------HORIZON-----------|
*         |          |  DIALOGUE RESPONSES      |
*         |(Opinions)|                          |
*         |          |                          |
*         |          |                          |
*         ---------------------------------------
*
*
*/

static int topic_category( const talk_topic &the_topic )
{
    const std::string &topic = the_topic.id;
    // TODO: ideally, this would be a property of the topic itself.
    // How this works: each category has a set of topics that belong to it, each set is checked
    // for the given topic and if a set contains, the category number is returned.
    static const std::unordered_set<std::string> topic_1 = { {
            "TALK_MISSION_START", "TALK_MISSION_DESCRIBE", "TALK_MISSION_OFFER",
            "TALK_MISSION_ACCEPTED", "TALK_MISSION_REJECTED", "TALK_MISSION_ADVICE",
            "TALK_MISSION_INQUIRE", "TALK_MISSION_SUCCESS", "TALK_MISSION_SUCCESS_LIE",
            "TALK_MISSION_FAILURE", "TALK_MISSION_REWARD", "TALK_MISSION_END",
            "TALK_MISSION_DESCRIBE_URGENT"
        }
    };
    if( topic_1.count( topic ) > 0 ) {
        return 1;
    }
    static const std::unordered_set<std::string> topic_2 = { {
            "TALK_SHARE_EQUIPMENT", "TALK_GIVE_EQUIPMENT", "TALK_DENY_EQUIPMENT"
        }
    };
    if( topic_2.count( topic ) > 0 ) {
        return 2;
    }
    static const std::unordered_set<std::string> topic_3 = { {
            "TALK_SUGGEST_FOLLOW", "TALK_AGREE_FOLLOW", "TALK_DENY_FOLLOW",
        }
    };
    if( topic_3.count( topic ) > 0 ) {
        return 3;
    }
    static const std::unordered_set<std::string> topic_4 = { {
            "TALK_COMBAT_ENGAGEMENT",
        }
    };
    if( topic_4.count( topic ) > 0 ) {
        return 4;
    }
    static const std::unordered_set<std::string> topic_5 = { {
            "TALK_COMBAT_COMMANDS",
        }
    };
    if( topic_5.count( topic ) > 0 ) {
        return 5;
    }
    static const std::unordered_set<std::string> topic_6 = { {
            "TALK_TRAIN", "TALK_TRAIN_START", "TALK_TRAIN_FORCE",
            "TALK_TRAIN_NPC_START", "TALK_TRAIN_NPC_FORCE"
        }
    };
    if( topic_6.count( topic ) > 0 ) {
        return 6;
    }
    static const std::unordered_set<std::string> topic_7 = { {
            "TALK_MISC_RULES",
        }
    };
    if( topic_7.count( topic ) > 0 ) {
        return 7;
    }
    static const std::unordered_set<std::string> topic_8 = { {
            "TALK_AIM_RULES",
        }
    };
    if( topic_8.count( topic ) > 0 ) {
        return 8;
    }
    static const std::unordered_set<std::string> topic_9 = { {
            "TALK_FRIEND", "TALK_GIVE_ITEM", "TALK_USE_ITEM",
        }
    };
    if( topic_9.count( topic ) > 0 ) {
        return 9;
    }
    return -1; // Not grouped with other topics
}

static float border_size()
{
    return 4.0f;
}

// This is the midway point between the DIALOGUE_HISTORY and DIALOGUE_RESPONSES child windows.
static float horizontal_separator_pos_y( const float window_height )
{
    return window_height * 0.6;
}

float dialogue_imgui_impl::sidebar_width() const
{
    const int num_characters_line_in_ASCII_portrait = 28; // Known fact
    const int num_characters_width = num_characters_line_in_ASCII_portrait + 4; // Add some padding
    // Portraits are supposed to be 128x128. The extra size is to account for borders and any small padding cases we may have overlooked etc
    const float min_width = std::max( 128.0f + border_size() * 4,
                                      num_characters_width * ImGui::CalcTextSize( "0" ).x );
    const float max_width = window_width * 0.3;
    const float actual_width = std::max( min_width, max_width );
    return actual_width;
}

// Width for just the history/responses children (they use the same width).
// Not to be confused with `window_width` which is a class variable for the entire parent window's width.
float dialogue_imgui_impl::dialogue_window_width() const
{
    // Two child windows side by side with 4 total borders, plus an extra border's worth of space to not let everything feel cramped.
    const float total_horizontal_padding = border_size() * 5;
    return ImGui::GetWindowSize().x - sidebar_width() - total_horizontal_padding;
}

void dialogue_imgui_impl::draw_controls()
{
    // We might be trading (non-imgui screen).
    // Toggled on/off in dialogue::opt_imgui().
    if( hide_ui ) {
        ImGuiWindow *w = ImGui::GetCurrentWindowRead();
        ImGui::SetWindowHiddenAndSkipItemsForCurrentFrame( w );
    }

    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );
    draw_dialogue_sidebar();
    ImGui::SetNextWindowPos( ImVec2( sidebar_width() + border_size() * 3,
                                     border_size() * 2 ) );
    draw_dialogue_history();
    ImGui::SetNextWindowPos( ImVec2( sidebar_width() + border_size() * 3,
                                     horizontal_separator_pos_y( window_height ) + border_size() * 2 ) );
    draw_dialogue_responses();
}

cataimgui::bounds dialogue_imgui_impl::get_bounds()
{
    // This allows us to occupy all space except the sidebar.
    ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    return { 0,
             0,
             window_width,
             viewport.y
           };
}

void dialogue_imgui::draw_dialogue_imgui( bool is_computer, bool is_not_conversation,
        const std::string &remote_name )
{
    input_context ctxt( "DIALOGUE" );
    dialogue_imgui_impl p_impl( conversation, is_computer, is_not_conversation, remote_name );

    // All of these keys end up being captured/used in dialogue::opt_imgui().
    ctxt.register_updown();
    ctxt.register_action( "CONFIRM", to_translation( "Select dialogue option" ) );
    ctxt.register_action( "HOME", to_translation( "Scroll dialogue history down (entire page)" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Scroll dialogue history down (one line)" ) );
    ctxt.register_action( "END", to_translation( "Scroll dialogue history up (entire page)" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Scroll dialogue history up (one line)" ) );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_CONDITIONAL" );
    ctxt.register_action( "DEBUG_DIALOGUE_DL_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_RESP_EFFECT" );
    ctxt.register_action( "DEBUG_DIALOGUE_SHOW_ALL_RESPONSE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    ctxt.set_timeout( 10 );

    while( !conversation->done ) {
        ui_manager::redraw_invalidated();

        conversation->actor( true )->update_missions( conversation->missions_assigned );
        const talk_topic next = conversation->opt_imgui( p_impl, conversation->topic_stack.back(), ctxt );
        if( next.id == "TALK_NONE" ) {
            int cat = topic_category( conversation->topic_stack.back() );
            do {
                conversation->topic_stack.pop_back();
            } while( cat != -1 && topic_category( conversation->topic_stack.back() ) == cat );
        }
        if( next.id == "TALK_DONE" || conversation->topic_stack.empty() ) {
            conversation->actor( true )->say(
                dialog_helper::bye_message( conversation->actor( true )->get_npc() ) );
            conversation->done = true;
        } else if( next.id != "TALK_NONE" ) {
            conversation->add_topic( next );
        }
    }
}

void dialogue_imgui_impl::draw_dialogue_sidebar() const
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {sidebar_width(), ImGui::GetWindowHeight() - ( border_size() * 2 )};
    // TODO: Some of these (portrait, name) want to be centered.
    if( ImGui::BeginChild( "##DIALOGUE_SIDEBAR", child_size, child_flags ) ) {
        // This is a masterpiece, especially with the double backslashes to escape it. Anybody who disagrees is automatically sentenced to 10 months of converting windows to Dear ImGui.
        // -Renech "The Greatest" CDDA
        std::string my_beautiful_NPC_ASCII_portait = string_format(
                    "|--------------------------|\n"
                    "|           _____   -Renech|\n"
                    "|          /     \\         |\n"
                    "|         | 0  0 |         |\n"
                    "|          \\ -  /          |\n"
                    "|          |\\__/|          |\n"
                    "|      ___/------\\___      |\n"
                    "|     / _/        \\_ \\     |\n"
                    "|    / / |  NPC   | \\ \\    |\n"
                    "|   / /  |        |  \\ \\   |\n"
                    "|                          |\n"
                    "|--------------------------|"
                );
        cataimgui::draw_colored_text( my_beautiful_NPC_ASCII_portait );

        // Name of who we're talking to (in big letter)
        cataimgui::PushGuiFont1_5x();
        cataimgui::draw_colored_text( conversation->speaker_name( *this ) );
        cataimgui::PopGuiFont1_5x();

        // Wielding/Wearing/Visible mutations
        ImGui::NewLine();
        ImGui::NewLine();
        // Color for contrast
        if( conversation->actor( false )->can_see() ) {
            cataimgui::TextColoredParagraph( c_blue, conversation->actor( true )->short_description() );
        } else {
            cataimgui::TextColoredParagraph( c_blue, string_format( _( "&You're blind and can't look at %s." ),
                                             conversation->actor( true )->disp_name() ) );
        }

        // Stats estimate
        ImGui::NewLine();
        ImGui::NewLine();
        // Color for contrast
        cataimgui::TextColoredParagraph( c_red,
                                         conversation->actor( true )->evaluation_by( *conversation->actor( false ) ) );
        // Personality traits
        ImGui::NewLine();
        ImGui::NewLine();
        // Color for contrast
        cataimgui::TextColoredParagraph( c_pink, conversation->actor( true )->view_personality_traits() );

        // Opinions of player
        ImGui::NewLine();
        ImGui::NewLine();
        // Color for contrast
        cataimgui::TextColoredParagraph( c_yellow, conversation->actor( true )->opinion_text() );

    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_history()
{

    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {dialogue_window_width(), ( horizontal_separator_pos_y( window_height ) - border_size() * 2 )};
    if( ImGui::BeginChild( "##DIALOGUE_HISTORY", child_size, child_flags ) ) {
        draw_history();
        set_scroll( scroll_to );
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_responses() const
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, border_size() );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImVec2 child_size = {dialogue_window_width(), ( window_height - horizontal_separator_pos_y( window_height ) - border_size() * 2 )};
    if( ImGui::BeginChild( "##DIALOGUE_RESPONSES", child_size, child_flags ) ) {
        draw_responses();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::add_to_history( const std::string &text, const std::string &speaker_name,
        nc_color speaker_color )
{
    add_to_history( speaker_name, speaker_color );
    add_to_history( text );
}

void dialogue_imgui_impl::add_to_history( const std::string &text )
{
    add_to_history( text, default_color() );
}

void dialogue_imgui_impl::add_to_history( const std::string &text, nc_color color )
{
    history.emplace_back( color, text );
}

void dialogue_imgui_impl::draw_history() const
{
    for( const history_message &msg : history ) {
        cataimgui::TextColoredParagraph( msg.color, msg.text );
        ImGui::NewLine();
    }
}

nc_color dialogue_imgui_impl::default_color() const
{
    return is_computer ? c_green : c_white;
}

void dialogue_imgui_impl::set_responses( const std::vector<talk_data> &responses )
{
    response_list = responses;
}

void dialogue_imgui_impl::draw_responses() const
{
    // Head it with the topic ID, if needed
    if( debug_mode ) {
        cataimgui::draw_colored_text( "talk_topic: " + debug_topic_name );
    }

    for( int i = 0; i < static_cast<int>( response_list.size() ); i++ ) {
        const talk_data &talk = response_list[i];
        if( sel_response == i ) {
            cataimgui::draw_colored_text( ">>>>>", c_yellow );
        } else {
            // This should be the same width as the arrows up above, to align with non-selected lines.
            cataimgui::draw_colored_text( "     " );
        }
        ImGui::SameLine();
        cataimgui::draw_colored_text( formatted_hotkey( talk.hotkey_desc, talk.color ) );
        ImGui::SameLine();
        cataimgui::TextColoredParagraph( talk.color, talk.text );
        ImGui::NewLine();
    }

    // Show effects, etc
    if( debug_mode ) {
        for( const std::string &dbg_info : responses_debug ) {
            cataimgui::TextColoredParagraph( c_yellow, dbg_info );
            ImGui::NewLine();
        }
    }
}

void dialogue_imgui_impl::set_responses_debug( const std::vector<std::string> &responses )
{
    responses_debug = responses;
}
