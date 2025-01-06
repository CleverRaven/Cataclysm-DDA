#ifdef IMGUI_DISABLE
#include "imgui_facade.h"
void ImGui::NewLine()
{}

bool ImGui::TableSetColumnIndex([[maybe_unused]] int column_n)
{
    return true;
}
#endif
