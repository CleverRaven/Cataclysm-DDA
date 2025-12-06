#include "cata_imgui_debug.h"

bool    cataimgui::BeginTable( const char *str_id, int columns_count, ImGuiTableFlags flags,
                               const ImVec2 &outer_size, float inner_width )
{
    flags = flags | DEBUG_IMGUI_table_flags;
    return ImGui::BeginTable( str_id, columns_count, flags, outer_size, inner_width );
}

void cataimgui::EndTable()
{
    ImGui::EndTable();
}

void cataimgui::TableSetupColumn( const char *label, ImGuiTableColumnFlags flags,
                                  float init_width_or_weight, ImGuiID user_id )
{
    flags = flags | DEBUG_IMGUI_column_flags;
    ImGui::TableSetupColumn( label, flags, init_width_or_weight, user_id );
}

void cataimgui::add_cataimgui_debug_checkboxes()
{
    // TODO: Replace with collapsibles/tree nodes??
    if( debug_mode ) {
        draw_colored_text( "Not every flag is necessarily here, just anything I thought you might need to use. Feel free to add any missing, or ask if you need help." );
        ImGui::SeparatorText( "TABLE FLAGS" );
        ImGui::CheckboxFlags( "ImGuiTableFlags_SizingFixedFit##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_SizingFixedFit );
        ImGui::CheckboxFlags( "ImGuiTableFlags_SizingFixedSame##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_SizingFixedSame );
        ImGui::CheckboxFlags( "ImGuiTableFlags_SizingStretchProp##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_SizingStretchProp );
        ImGui::CheckboxFlags( "ImGuiTableFlags_SizingStretchSame##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_SizingStretchSame );
        ImGui::CheckboxFlags( "ImGuiTableFlags_NoHostExtendX##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_NoHostExtendX );
        ImGui::CheckboxFlags( "ImGuiTableFlags_PreciseWidths##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_PreciseWidths );
        ImGui::CheckboxFlags( "ImGuiTableFlags_PadOuterX##xx", &DEBUG_IMGUI_table_flags,
                              ImGuiTableFlags_PadOuterX );
        ImGui::NewLine();
        ImGui::SeparatorText( "COLUMN FLAGS" );
        ImGui::CheckboxFlags( "ImGuiTableColumnFlags_WidthStretch##xx", &DEBUG_IMGUI_column_flags,
                              ImGuiTableColumnFlags_WidthStretch );
        ImGui::CheckboxFlags( "ImGuiTableColumnFlags_WidthFixed##xx", &DEBUG_IMGUI_column_flags,
                              ImGuiTableColumnFlags_WidthFixed );
        ImGui::CheckboxFlags( "ImGuiTableColumnFlags_NoResize##xx", &DEBUG_IMGUI_column_flags,
                              ImGuiTableColumnFlags_NoResize );
        ImGui::CheckboxFlags( "ImGuiTableColumnFlags_NoHeaderLabel##xx", &DEBUG_IMGUI_column_flags,
                              ImGuiTableColumnFlags_NoHeaderLabel );
        ImGui::CheckboxFlags( "ImGuiTableColumnFlags_NoHeaderWidth##xx", &DEBUG_IMGUI_column_flags,
                              ImGuiTableColumnFlags_NoHeaderWidth );
        ImGui::NewLine();
        ImGui::SeparatorText( "WINDOW FLAGS" );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoTitleBar##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoTitleBar );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoCollapse##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoCollapse );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoResize##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoResize );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoSavedSettings##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoSavedSettings );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoMove##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoMove );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoNavInputs##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoNavInputs );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoNavFocus##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoNavFocus );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoBringToFrontOnFocus##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoBringToFrontOnFocus );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_AlwaysAutoResize##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_AlwaysAutoResize );
        ImGui::CheckboxFlags( "ImGuiWindowFlags_NoBackground##xx", &default_cataimgui_window_flags,
                              ImGuiWindowFlags_NoBackground );
    }
}
