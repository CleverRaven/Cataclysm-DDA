#include "tabbed_listinfo_ui.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <optional>

#include "color.h"
#include "input_enums.h"
#include "options.h"

//TODO: these static functions are identical to character creator's
cataimgui::bounds tabbed_listinfo_ui::uilist_reset_desired_bounds()
{
    return { 0, -1.0f, 0.33f * ImGui::GetMainViewport()->WorkSize.x, -1.0f };
}

uilist_entry tabbed_listinfo_ui::get_uilist_entry( const std::string &name )
{
    const bool screen_reader_mode = get_option<bool>( "SCREEN_READER_MODE" );
    uilist_entry cc_entry( screen_reader_mode ? std::string() : name );
    cc_entry.hotkey = input_event();
    cc_entry.text_color = c_white;

    return cc_entry;
}

void tabbed_listinfo_ui::set_info_scroll( bool &scrolled_down, bool &scrolled_up )
{
    const ImVec2 &viewport = ImGui::GetMainViewport()->WorkSize;
    if( scrolled_down ) {
        scrolled_down = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() + viewport.y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    } else if( scrolled_up ) {
        scrolled_up = false;
        ImGui::SetScrollY( std::clamp( ImGui::GetScrollY() - viewport.y, 0.0f,
                                       ImGui::GetScrollMaxY() ) );
    }
}
