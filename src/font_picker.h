#pragma once
#ifndef IMTUI
#include <memory>

class Font;
using Font_Ptr = std::unique_ptr<Font>;

class FontPickerWindow;

class FontPicker
{
    protected:
        friend FontPickerWindow;
        bool WantRebuild = false;
    public:
        // Call _BEFORE_ ImGui::NewFrame()
        bool PreNewFrame();

        // Call to draw UI
        void ShowFontsOptionsWindow();
};

extern FontPicker font_editor;
#endif // IMTUI
