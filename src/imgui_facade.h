#pragma once
#ifdef IMGUI_DISABLE
#include <cstddef>

namespace ImGui
{
    void NewLine();
    bool TableSetColumnIndex(int column_n);
}
#endif // IMGUI_DISABLE
