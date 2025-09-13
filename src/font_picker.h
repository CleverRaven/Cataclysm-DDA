#pragma once
#ifndef IMTUI
#include <memory>

class Font;
using Font_Ptr = std::unique_ptr<Font>;

namespace FontPicker
{
// Call _BEFORE_ ImGui::NewFrame()
bool PreNewFrame();

// Call to draw UI
void ShowFontsOptionsWindow();
} // namespace FontPicker

#endif // IMTUI
