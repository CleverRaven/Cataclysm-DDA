#include "imgui_demo.h"

#include <imgui/imgui.h>
#include <string>

#include "cata_imgui.h"
#include "color.h"
#include "input_context.h"
#include "text.h"
#include "translations.h"
#include "ui_manager.h"


imgui_demo_ui::imgui_demo_ui(): cataimgui::window( _( "ImGui Demo Screen" ) )
{
    std::string text =
        "Some long text that will wrap around nicely.  <color_red>Some red text in the middle.</color>  Some long text that will wrap around nicely.";
    stuff = std::make_shared<cataimgui::Paragraph>();
    stuff->append_colored_text( text, c_white );
}

cataimgui::bounds imgui_demo_ui::get_bounds()
{
    return { -1.f, -1.f, ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y };
}

static void draw_lorem( const std::shared_ptr<cataimgui::Paragraph> &stuff )
{

#ifdef TUI
    ImGui::SetNextWindowPos( { 0, 0 }, ImGuiCond_Once );
    ImGui::SetNextWindowSize( { 60, 40 }, ImGuiCond_Once );
#else
    ImGui::SetNextWindowSize( { 620, 900 }, ImGuiCond_Once );
#endif
    if( ImGui::Begin( "test" ) ) {
#ifdef TUI
        static float wrap_width = 50.0f;
        ImGui::SliderFloat( "Wrap width", &wrap_width, 1, 60, "%.0f" );
        float marker_size = 0.0f;
#else
        static float wrap_width = 200.0f;
        ImGui::SliderFloat( "Wrap width", &wrap_width, -20, 600, "%.0f" );
        float marker_size = ImGui::GetTextLineHeight();
#endif

#define WRAP_START()                                                           \
    ImDrawList *draw_list = ImGui::GetWindowDrawList();                        \
    ImVec2 pos = ImGui::GetCursorScreenPos();                                  \
    ImVec2 marker_min = ImVec2(pos.x + wrap_width, pos.y);                     \
    ImVec2 marker_max =                                                        \
            ImVec2(pos.x + wrap_width + marker_size, pos.y + marker_size);         \
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrap_width);

#define WRAP_END()                                                             \
    draw_list->AddRectFilled(marker_min, marker_max,                           \
                             IM_COL32(255, 0, 255, 255));                      \
    ImGui::PopTextWrapPos();

        // three sentences in a nicely–wrapped paragraph
        if( ImGui::CollapsingHeader( "Plain wrapped text" ) ) {
            WRAP_START();
            ImGui::TextWrapped( "%s",
                                "Some long text that will wrap around nicely.  Some red text in the middle.  Some long text that will wrap around nicely." );
            WRAP_END();
            ImGui::NewLine();
        }

        if( ImGui::CollapsingHeader( "Styled Paragraph" ) ) {
            WRAP_START();
            cataimgui::TextStyled( stuff, wrap_width );
            WRAP_END();
            ImGui::NewLine();
        }

        if( ImGui::CollapsingHeader( "Unstyled Paragraph" ) ) {
            WRAP_START();
            cataimgui::TextUnstyled( stuff, wrap_width );
            WRAP_END();
            ImGui::NewLine();
        }

        if( ImGui::CollapsingHeader( "Styled Paragraph, no allocations" ) ) {
            WRAP_START();
            cataimgui::TextParagraph( c_white, "Some long text that will wrap around nicely.", wrap_width );
            cataimgui::TextParagraph( c_red, "  Some red text in the middle.", wrap_width );
            cataimgui::TextParagraph( c_white, "  Some long text that will wrap around nicely.", wrap_width );
            ImGui::NewLine();
            WRAP_END();
            ImGui::NewLine();
        }

        if( ImGui::CollapsingHeader( "Naive attempt" ) ) {
            WRAP_START();
            // same three sentences, but the color breaks the wrapping
            ImGui::TextUnformatted( "Some long text that will wrap around nicely." );
            ImGui::SameLine();
            ImGui::TextColored( c_red, "%s", "Some red text in the middle." );
            ImGui::SameLine();
            ImGui::TextUnformatted( "Some long text that will wrap around nicely." );
            WRAP_END();
        }
    }
    ImGui::End();
}

void imgui_demo_ui::draw_controls()
{
#ifndef TUI
    ImGui::ShowDemoWindow();
#endif
    draw_lorem( stuff );
}

void imgui_demo_ui::init()
{
    // The demo makes it's own screen.  Don't get in the way
    force_to_back = true;
}

void imgui_demo_ui::run()
{
    init();

    input_context ctxt( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    std::string action;

    ui_manager::redraw();

    while( is_open ) {
        ui_manager::redraw();
        action = ctxt.handle_input( 5 );
        if( action == "QUIT" ) {
            break;
        }
    }
}
