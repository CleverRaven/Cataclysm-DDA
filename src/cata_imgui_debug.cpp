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
    if( debug_mode ) {
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
    }
}
