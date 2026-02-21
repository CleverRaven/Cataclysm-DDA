#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "cata_imgui.h"
#include "color.h"
#include "font_loader.h"

static ImGuiTableFlags DEBUG_IMGUI_table_flags;

static ImGuiTableColumnFlags DEBUG_IMGUI_column_flags;

namespace cataimgui
{
// Simple forwarder to append debug flags
bool BeginTable( const char *str_id, int columns, ImGuiTableFlags flags = 0,
                 const ImVec2 &outer_size = ImVec2( 0.0f, 0.0f ), float inner_width = 0.0f );
// to match the BeginTable forwarder.
void EndTable();

// Simple forwarder to append debug flags
void TableSetupColumn( const char *label, ImGuiTableColumnFlags flags = 0,
                       float init_width_or_weight = 0.0f, ImGuiID user_id = 0 );

void add_cataimgui_debug_checkboxes();

} // namespace cataimgui
