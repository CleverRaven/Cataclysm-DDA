#include "dialogue_imgui.h"

#include <array>
#include <string>
#include <vector>

#include "cata_imgui.h"
#include "catacharset.h"
#include "debug.h"
#include "imgui/imgui.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "ui_manager.h"

/*
* This is roughly what the window should look like at full layout.
* I've annotated the various parts to make it easier for you to identify what's doing each thing.
* Each of the entries in capital letters is a child window with its own dedicated draw function.
*
*         ---------------------------------------
*         | SIDEBAR  |  DIALOGUE HISTORY        |
*         |(Portrait)|                          |
*         | (name)   |                          |
*         |          |                          |
*         | (Wield)  |                          |
*         | (Wear)   |                          |
*         |          |                          |
*         |(Traits)  |                          |
*         |(Stats)   | -------------------------|
*         |          |  DIALOGUE RESPONSES      |
*         |(Opinions)|                          |
*         |          |                          |
*         |          |                          |
*         ---------------------------------------
*
*
*/

void dialogue_imgui_impl::draw_controls()
{
    ImGui::SetWindowSize( ImVec2( window_width, window_height ), ImGuiCond_Once );
    draw_dialogue_sidebar();
    // Draw history on the "same line" as the sidebar, but to the right.
    ImGui::SameLine();
    // Now that we have the position, we need to save it...
    ImVec2 topleft_dialogue_non_sidebar = ImGui::GetCursorScreenPos();
    draw_dialogue_history();
    // In order to place this next child correctly we use the saved position and offset the window.
    topleft_dialogue_non_sidebar.y += window_height * 0.6;
    ImGui::SetNextWindowPos( topleft_dialogue_non_sidebar );
    draw_dialogue_responses();
}

void dialogue_imgui::draw_dialogue_imgui()
{
    input_context ctxt( "DIALOGUE" );
    dialogue_imgui_impl p_impl;

    ctxt.register_updown();
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "CONFIRM", to_translation( "Select dialogue option" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );

    ctxt.set_timeout( 10 );

    while( true ) {
        ui_manager::redraw_invalidated();


        p_impl.last_action = ctxt.handle_input();

        if( p_impl.last_action == "QUIT" || !p_impl.get_is_open() ) {
            break;
        }
    }
}

void dialogue_imgui_impl::draw_dialogue_sidebar() const
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 4.0f );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    if( ImGui::BeginChild( "##DIALOGUE_SIDEBAR", ImVec2( window_width * 0.29, window_height * 0.95 ),
                           child_flags ) ) {
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
        cataimgui::draw_colored_text( "Your best friend (Not Liam, the other one)" );
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_history() const
{

    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 4.0f );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    if( ImGui::BeginChild( "##DIALOGUE_HISTORY", ImVec2( window_width * 0.68,
                           ( window_height * 0.59 ) ), child_flags ) ) {
        cataimgui::draw_colored_text( "Hello there! My name is __________" );
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void dialogue_imgui_impl::draw_dialogue_responses() const
{
    ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 4.0f );
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    if( ImGui::BeginChild( "##DIALOGUE_RESPONSES", ImVec2( window_width * 0.68,
                           ( window_height * 0.38 ) ), child_flags ) ) {
        cataimgui::draw_colored_text( "A. Well, aren't you a handsome fellow!" );
        cataimgui::draw_colored_text( "B. So... beautiful..." );
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}