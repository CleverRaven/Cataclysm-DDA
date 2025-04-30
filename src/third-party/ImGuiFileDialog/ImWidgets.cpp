// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * Copyright 2020 Stephane Cuillerdier (aka Aiekick)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// ############################# DISCLAIMER #############################
// ############ THIS WIDGETS LIB IS BASED ON IMGUIPACK ##################
// ############# https://github.com/aiekick/ImGuiPack ###################
// ######################################################################

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "ImWidgets.h"

#ifdef IMGUI_INTERNAL_INCLUDE
#include IMGUI_INTERNAL_INCLUDE
#else  // IMGUI_INTERNAL_INCLUDE
#include <imgui_internal.h>
#endif  // IMGUI_INTERNAL_INCLUDE

#ifdef CUSTOM_IMWIDGETS_CONFIG
#include CUSTOM_IMWIDGETS_CONFIG
#else
#include "ImWidgetConfigHeader.h"
#endif  // CUSTOM_IMWIDGETS_CONFIG

#define STARTING_CUSTOMID 125

#ifdef GLFW3
#include <GLFW/glfw3.h>
#endif  // GLFW3

#ifdef WIN32
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <direct.h>
#ifndef stat
#define stat _stat
#endif  // stat
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif  // S_IFDIR
#ifndef GetCurrentDir
#define GetCurrentDir _getcwd
#endif  // GetCurrentDir
#ifndef SetCurrentDir
#define SetCurrentDir _chdir
#endif  // GetCurrentDir
#elif defined(UNIX)
#include <cstdlib>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef APPLE
#include <dlfcn.h>
#include <sys/syslimits.h>  // PATH_MAX
#endif                      // APPLE
#ifdef STDC_HEADERS
#include <stddef.h>
#include <stdlib.h>
#else  // STDC_HEADERS
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif  // HAVE_STDLIB_H
#endif  // STDC_HEADERS
#ifdef HAVE_STRING_H
#include <string.h>
#endif  // HAVE_STRING_H
#ifndef GetCurrentDir
#define GetCurrentDir getcwd
#endif  // GetCurrentDir
#ifndef SetCurrentDir
#define SetCurrentDir chdir
#endif  // SetCurrentDir
#ifndef S_IFDIR
#define S_IFDIR __S_IFDIR
#endif  // S_IFDIR
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif  // MAX_PATH
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif  // MAX_PATH
#endif  // UNIX

static void OpenUrl(const std::string& vUrl) {
#ifdef WIN32
    ShellExecute(nullptr, nullptr, vUrl.c_str(), nullptr, nullptr, SW_SHOW);
#elif defined(LINUX)
    char buffer[MAX_PATH] = {};
    snprintf(buffer, MAX_PATH, "<mybrowser> %s", vUrl.c_str());
    std::system(buffer);
#elif defined(APPLE)
    // std::string sCmdOpenWith = "open -a Firefox " + vUrl;
    std::string sCmdOpenWith = "open " + vUrl;
    std::system(sCmdOpenWith.c_str());
#endif
}

static void OpenFile(const std::string& vFile) {
#if defined(WIN32)
    auto* result = ShellExecute(nullptr, "", vFile.c_str(), nullptr, nullptr, SW_SHOW);
    if (result < (HINSTANCE)32)  //-V112
    {
        // try to open an editor
        result = ShellExecute(nullptr, "edit", vFile.c_str(), nullptr, nullptr, SW_SHOW);
        if (result == (HINSTANCE)SE_ERR_ASSOCINCOMPLETE || result == (HINSTANCE)SE_ERR_NOASSOC) {
            // open associating dialog
            const std::string sCmdOpenWith = "shell32.dll,OpenAs_RunDLL \"" + vFile + "\"";
            result                         = ShellExecute(nullptr, "", "rundll32.exe", sCmdOpenWith.c_str(), nullptr, SW_NORMAL);
        }
        if (result < (HINSTANCE)32)  // open in explorer //-V112
        {
            const std::string sCmdExplorer = "/select,\"" + vFile + "\"";
            ShellExecute(nullptr, "", "explorer.exe", sCmdExplorer.c_str(), nullptr, SW_NORMAL);  // ce serait peut etre mieu d'utilsier la commande system comme dans SelectFile
        }
    }
#elif defined(LINUX)
    int pid = fork();
    if (pid == 0) {
        execl("/usr/bin/xdg-open", "xdg-open", vFile.c_str(), (char*)0);
    }
#elif defined(APPLE)
    // std::string command = "open -a Tincta " + vFile;
    std::string command = "open " + vFile;
    std::system(command.c_str());
#endif
}

/////////////////////////////////////
/////////////////////////////////////

namespace ImGui {

float CustomStyle::puContrastRatio       = 3.0f;
ImU32 CustomStyle::puContrastedTextColor = 0;
int CustomStyle::pushId                  = STARTING_CUSTOMID;
int CustomStyle::minorNumber             = 0;
int CustomStyle::majorNumber             = 0;
int CustomStyle::buildNumber             = 0;
ImVec4 CustomStyle::GoodColor            = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);
ImVec4 CustomStyle::BadColor             = ImVec4(0.8f, 0.2f, 0.2f, 0.8f);
ImVec4 CustomStyle::ImGuiCol_Symbol      = ImVec4(0, 0, 0, 1);
void CustomStyle::Init() {
    puContrastedTextColor = GetColorU32(ImVec4(0, 0, 0, 1));
    puContrastRatio       = 3.0f;
}
void CustomStyle::ResetCustomId() {
    pushId = STARTING_CUSTOMID;
}

void SetContrastRatio(float vRatio) {
    CustomStyle::puContrastRatio = vRatio;
}

void SetContrastedTextColor(ImU32 vColor) {
    CustomStyle::puContrastedTextColor = vColor;
}

/////////////////////////////////////
/////////////////////////////////////

int IncPUSHID() {
    return ++CustomStyle::pushId;
}

int GetPUSHID() {
    return CustomStyle::pushId;
}

void SetPUSHID(int vID) {
    CustomStyle::pushId = vID;
}

ImVec4 GetGoodOrBadColorForUse(bool vUsed) {
    if (vUsed) return CustomStyle::GoodColor;
    return CustomStyle::BadColor;
}

// viewport mode :
// if not called from a ImGui Window, will return ImVec2(0,0)
// if its your case, you need to set the GLFWwindow
// no issue withotu viewport
ImVec2 GetLocalMousePos(GLFWWindow* vWin) {
#if defined(IMGUI_HAS_VIEWPORT) && defined(GLFW3)
    if (vWin) {
        double mouse_x, mouse_y;
        glfwGetCursorPos(vWin, &mouse_x, &mouse_y);
        return ImVec2((float)mouse_x, (float)mouse_y);
    } else {
        ImGuiContext& g = *GImGui;
        auto viewport   = g.CurrentViewport;
        if (!viewport && g.CurrentWindow) viewport = g.CurrentWindow->Viewport;
        if (viewport && viewport->PlatformHandle) {
            double mouse_x, mouse_y;
            glfwGetCursorPos((GLFWwindow*)viewport->PlatformHandle, &mouse_x, &mouse_y);
            return ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
#else
    return GetMousePos();
#endif
    return ImVec2(0.0f, 0.0f);
}

/////////////////////////////////////
/////////////////////////////////////

// contrast from 1 to 21
// https://www.w3.org/TR/WCAG20/#relativeluminancedef
float CalcContrastRatio(const ImU32& backgroundColor, const ImU32& foreGroundColor) {
    const float sa0           = (float)((backgroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    const float sa1           = (float)((foreGroundColor >> IM_COL32_A_SHIFT) & 0xFF);
    static float sr           = 0.2126f / 255.0f;
    static float sg           = 0.7152f / 255.0f;
    static float sb           = 0.0722f / 255.0f;
    const float contrastRatio = (sr * sa0 * ((backgroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa0 * ((backgroundColor >> IM_COL32_G_SHIFT) & 0xFF) + sb * sa0 * ((backgroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f) /
                                (sr * sa1 * ((foreGroundColor >> IM_COL32_R_SHIFT) & 0xFF) + sg * sa1 * ((foreGroundColor >> IM_COL32_G_SHIFT) & 0xFF) + sb * sa1 * ((foreGroundColor >> IM_COL32_B_SHIFT) & 0xFF) + 0.05f);
    if (contrastRatio < 1.0f) return 1.0f / contrastRatio;
    return contrastRatio;
}

bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& maxContrastRatio) {
    const float contrastRatio = CalcContrastRatio(GetColorU32(backGroundColor), GetColorU32(foreGroundColor));
    if (contrastRatio < maxContrastRatio) {
        PushStyleColor(foreGroundColor, invertedColor);
        return true;
    }
    return false;
}

bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio) {
    const float contrastRatio = CalcContrastRatio(GetColorU32(backGroundColor), GetColorU32(foreGroundColor));
    if (contrastRatio < maxContrastRatio) {
        PushStyleColor(foreGroundColor, invertedColor);

        return true;
    }
    return false;
}

bool PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio) {
    const float contrastRatio = CalcContrastRatio(backGroundColor, GetColorU32(foreGroundColor));
    if (contrastRatio < maxContrastRatio) {
        PushStyleColor(foreGroundColor, invertedColor);
        return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://github.com/ocornut/imgui/issues/3710

inline void PathInvertedRect(ImDrawList* vDrawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, ImDrawFlags rounding_corners) {
    if (!vDrawList) return;

    rounding = ImMin(rounding, ImFabs(b.x - a.x) * (((rounding_corners & ImDrawFlags_RoundCornersTop) == ImDrawFlags_RoundCornersTop) || ((rounding_corners & ImDrawFlags_RoundCornersBottom) == ImDrawFlags_RoundCornersBottom) ? 0.5f : 1.0f) - 1.0f);
    rounding = ImMin(rounding, ImFabs(b.y - a.y) * (((rounding_corners & ImDrawFlags_RoundCornersLeft) == ImDrawFlags_RoundCornersLeft) || ((rounding_corners & ImDrawFlags_RoundCornersRight) == ImDrawFlags_RoundCornersRight) ? 0.5f : 1.0f) - 1.0f);

    if (rounding <= 0.0f || rounding_corners == 0) {
        return;
    } else {
        const float rounding_tl = (rounding_corners & ImDrawFlags_RoundCornersTopLeft) ? rounding : 0.0f;
        vDrawList->PathLineTo(a);
        vDrawList->PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        vDrawList->PathFillConvex(col);

        const float rounding_tr = (rounding_corners & ImDrawFlags_RoundCornersTopRight) ? rounding : 0.0f;
        vDrawList->PathLineTo(ImVec2(b.x, a.y));
        vDrawList->PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        vDrawList->PathFillConvex(col);

        const float rounding_br = (rounding_corners & ImDrawFlags_RoundCornersBottomRight) ? rounding : 0.0f;
        vDrawList->PathLineTo(ImVec2(b.x, b.y));
        vDrawList->PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        vDrawList->PathFillConvex(col);

        const float rounding_bl = (rounding_corners & ImDrawFlags_RoundCornersBottomLeft) ? rounding : 0.0f;
        vDrawList->PathLineTo(ImVec2(a.x, b.y));
        vDrawList->PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
        vDrawList->PathFillConvex(col);
    }
}

bool RadioButtonLabeled(float vWidth, const char* label, bool active, bool disabled) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g         = *GImGui;
    const ImGuiStyle& style = g.Style;
    float w                 = vWidth;
    const ImGuiID id        = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, nullptr, true);
    if (w < 0.0f) w = GetContentRegionAvail().x;
    if (IS_FLOAT_EQUAL(w, 0.0f)) w = label_size.x + style.FramePadding.x * 2.0f;
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id)) return false;

    // check
    bool pressed          = false;
    ImGuiCol colUnderText = ImGuiCol_Button;
    if (!disabled) {
        bool hovered, held;
        pressed = ButtonBehavior(total_bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

        colUnderText = ImGuiCol_FrameBg;
        window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : colUnderText), style.FrameRounding);
        if (active) {
            colUnderText = ImGuiCol_Button;
            window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : colUnderText), style.FrameRounding);
        }
    }

    // circle shadow + bg
    if (style.FrameBorderSize > 0.0f) {
        window->DrawList->AddRect(total_bb.Min + ImVec2(1, 1), total_bb.Max, GetColorU32(ImGuiCol_BorderShadow), style.FrameRounding);
        window->DrawList->AddRect(total_bb.Min, total_bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding);
    }

    if (label_size.x > 0.0f) {
        const bool pushed = PushStyleColorWithContrast(colUnderText, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);
        RenderTextClipped(total_bb.Min, total_bb.Max, label, nullptr, &label_size, ImVec2(0.5f, 0.5f));
        if (pushed) PopStyleColor();
    }

    return pressed;
}

bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool active, bool disabled, ImFont* vLabelFont) {
    if (vLabelFont) PushFont(vLabelFont);
    const bool change = RadioButtonLabeled(vWidth, label, active, disabled);
    if (vLabelFont) PopFont();
    if (help)
        if (IsItemHovered()) SetTooltip("%s", help);
    return change;
}

bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool* active, bool disabled, ImFont* vLabelFont) {
    if (vLabelFont) PushFont(vLabelFont);
    const bool change = RadioButtonLabeled(vWidth, label, *active, disabled);
    if (vLabelFont) PopFont();
    if (change) *active = !*active;
    if (help)
        if (IsItemHovered()) SetTooltip("%s", help);
    return change;
}

bool RadioButtonLabeled(ImVec2 vSize, const char* label, bool active, bool disabled) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g         = *GImGui;
    const ImGuiStyle& style = g.Style;
    float w                 = vSize.x;
    float h                 = vSize.y;
    const ImGuiID id        = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, nullptr, true);
    if (w < 0.0f) w = GetContentRegionAvail().x;
    if (h < 0.0f) w = GetContentRegionAvail().y;
    if (IS_FLOAT_EQUAL(w, 0.0f)) w = label_size.x + style.FramePadding.x * 2.0f;
    if (IS_FLOAT_EQUAL(h, 0.0f)) h = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, h));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id)) return false;

    // check
    bool pressed          = false;
    ImGuiCol colUnderText = ImGuiCol_Button;
    if (!disabled) {
        bool hovered, held;
        pressed = ButtonBehavior(total_bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

        colUnderText = ImGuiCol_FrameBg;
        window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : colUnderText), style.FrameRounding);
        if (active) {
            colUnderText = ImGuiCol_Button;
            window->DrawList->AddRectFilled(total_bb.Min, total_bb.Max, GetColorU32((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : colUnderText), style.FrameRounding);
        }
    }

    // circle shadow + bg
    if (style.FrameBorderSize > 0.0f) {
        window->DrawList->AddRect(total_bb.Min + ImVec2(1, 1), total_bb.Max, GetColorU32(ImGuiCol_BorderShadow), style.FrameRounding);
        window->DrawList->AddRect(total_bb.Min, total_bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding);
    }

    if (label_size.x > 0.0f) {
        const bool pushed = PushStyleColorWithContrast(colUnderText, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);
        RenderTextClipped(total_bb.Min, total_bb.Max, label, nullptr, &label_size, ImVec2(0.5f, 0.5f));
        if (pushed) PopStyleColor();
    }

    return pressed;
}

bool RadioButtonLabeled(ImVec2 vSize, const char* label, const char* help, bool active, bool disabled, ImFont* vLabelFont) {
    if (vLabelFont) PushFont(vLabelFont);
    const bool change = RadioButtonLabeled(vSize, label, active, disabled);
    if (vLabelFont) PopFont();
    if (help)
        if (IsItemHovered()) SetTooltip("%s", help);
    return change;
}

bool RadioButtonLabeled(ImVec2 vSize, const char* label, const char* help, bool* active, bool disabled, ImFont* vLabelFont) {
    if (vLabelFont) PushFont(vLabelFont);
    const bool change = RadioButtonLabeled(vSize, label, *active, disabled);
    if (vLabelFont) PopFont();
    if (change) *active = !*active;
    if (help)
        if (IsItemHovered()) SetTooltip("%s", help);
    return change;
}

bool SmallContrastedButton(const char* label) {
    ImGuiContext& g        = *GImGui;
    float backup_padding_y = g.Style.FramePadding.y;
    g.Style.FramePadding.y = 0.0f;
    const bool pushed      = PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);
    PushID(++CustomStyle::pushId);
    bool pressed = ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
    PopID();
    if (pushed) PopStyleColor();
    g.Style.FramePadding.y = backup_padding_y;
    return pressed;
}

bool ContrastedButton_For_Dialogs(const char* label, const ImVec2& size_arg) {
    return ContrastedButton(label, nullptr, nullptr, 0.0f, size_arg, ImGuiButtonFlags_None);
}

bool ContrastedButton(const char* label, const char* help, ImFont* imfont, float vWidth, const ImVec2& size_arg, ImGuiButtonFlags flags) {
    const bool pushed = PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);

    if (imfont) PushFont(imfont);

    PushID(++CustomStyle::pushId);

    const bool res = ButtonEx(label, ImVec2(ImMax(vWidth, size_arg.x), size_arg.y), flags | ImGuiButtonFlags_PressedOnClick);

    PopID();

    if (imfont) PopFont();

    if (pushed) PopStyleColor();

    if (help)
        if (IsItemHovered()) SetTooltip("%s", help);

    return res;
}

bool ToggleContrastedButton(const char* vLabelTrue, const char* vLabelFalse, bool* vValue, const char* vHelp, ImFont* vImfont) {
    bool res = false;

    assert(vValue);

    const auto pushed = PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);

    if (vImfont) PushFont(vImfont);

    PushID(++CustomStyle::pushId);

    if (Button(*vValue ? vLabelTrue : vLabelFalse)) {
        *vValue = !*vValue;
        res     = true;
    }

    PopID();

    if (vImfont) PopFont();

    if (pushed) PopStyleColor();

    if (vHelp)
        if (IsItemHovered()) SetTooltip("%s", vHelp);

    return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// COMBO ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float inCalcMaxPopupHeightFromItemCount(int items_count) {
    ImGuiContext& g = *GImGui;
    if (items_count <= 0) return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool BeginContrastedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags) {
    ImGuiContext& g     = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();  // We behave like Begin() and need to consume those values
    if (window->SkipItems) return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id        = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview));  // Can't use both flags together
    if (flags & ImGuiComboFlags_WidthFitPreview) IM_ASSERT((flags & (ImGuiComboFlags_NoPreview | (ImGuiComboFlags)ImGuiComboFlags_CustomPreview)) == 0);

    const float arrow_size    = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size   = CalcTextSize(label, NULL, true);
    const float preview_width = ((flags & ImGuiComboFlags_WidthFitPreview) && (preview_value != NULL)) ? CalcTextSize(preview_value, NULL, true).x : 0.0f;
    const float w             = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : ((flags & ImGuiComboFlags_WidthFitPreview) ? (arrow_size + preview_width + style.FramePadding.x * 2.0f) : CalcItemWidth());
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(bb.Min, bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb)) return false;

    // Open on click
    bool hovered, held;
    bool pressed           = ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open        = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open) {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    // Render shape
    const ImU32 frame_col = GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2  = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavHighlight(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview)) window->DrawList->AddRectFilled(bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & ImGuiComboFlags_NoArrowButton) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
    if (!(flags & ImGuiComboFlags_NoArrowButton)) {
        const bool pushed = PushStyleColorWithContrast(ImGuiCol_Button, ImGuiCol_Text, CustomStyle::puContrastedTextColor, CustomStyle::puContrastRatio);
        ImU32 bg_col      = GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x) RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, ImGuiDir_Down, 1.0f);
        if (pushed) PopStyleColor();
    }
    RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Custom preview
    if (flags & ImGuiComboFlags_CustomPreview) {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    // Render preview and label
    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview)) {
        if (g.LogEnabled) LogSetNextTextDecoration("{", "}");
        RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
    }
    if (label_size.x > 0) RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);

    if (!popup_open) return false;

    g.NextWindowData.Flags = backup_next_window_data_flags;
    return BeginComboPopup(popup_id, bb, flags);
}

bool ContrastedCombo(float vWidth, const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items) {
    ImGuiContext& g = *GImGui;

    if (vWidth > -1) PushItemWidth((float)vWidth);

    // Call the getter to obtain the preview string which is a parameter to BeginCombo()
    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count) items_getter(data, *current_item, &preview_value);

    // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)) SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, inCalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!BeginContrastedCombo(label, preview_value, ImGuiComboFlags_None)) return false;

    // Display items
    // FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
    bool value_changed = false;
    for (int i = 0; i < items_count; ++i) {
        PushID((void*)(intptr_t)i);
        const bool item_selected = (i == *current_item);
        const char* item_text;
        if (!items_getter(data, i, &item_text)) item_text = "*Unknown item*";
        if (Selectable(item_text, item_selected)) {
            value_changed = true;
            *current_item = i;
        }
        if (item_selected) SetItemDefaultFocus();
        PopID();
    }

    EndCombo();
    if (value_changed) MarkItemEdited(g.LastItemData.ID);

    if (vWidth > -1) PopItemWidth();

    return value_changed;
}

// Getter for the old Combo() API: const char*[]
inline bool inItems_ArrayGetter(void* data, int idx, const char** out_text) {
    const char* const* items = (const char* const*)data;
    if (out_text) *out_text = items[idx];
    return true;
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
inline bool inItems_SingleStringGetter(void* data, int idx, const char** out_text) {
    // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
    const char* items_separated_by_zeros = (const char*)data;
    int items_count                      = 0;
    const char* p                        = items_separated_by_zeros;
    while (*p) {
        if (idx == items_count) break;
        p += strlen(p) + 1;
        items_count++;
    }
    if (!*p) return false;
    if (out_text) *out_text = p;
    return true;
}

// Combo box helper allowing to pass an array of strings.
bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* const items[], int items_count, int height_in_items) {
    PushID(++CustomStyle::pushId);

    const bool value_changed = ContrastedCombo(vWidth, label, current_item, inItems_ArrayGetter, (void*)items, items_count, height_in_items);

    PopID();

    return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items) {
    PushID(++CustomStyle::pushId);

    int items_count = 0;
    const char* p   = items_separated_by_zeros;  // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p) {
        p += strlen(p) + 1;
        items_count++;
    }
    bool value_changed = ContrastedCombo(vWidth, label, current_item, inItems_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);

    PopID();

    return value_changed;
}

bool ContrastedComboVectorDefault(float vWidth, const char* label, int* current_item, const std::vector<std::string>& items, int vDefault, int height_in_items) {
    bool change = false;

    if (!items.empty()) {
        PushID(++CustomStyle::pushId);

        change = ContrastedButton(BUTTON_LABEL_RESET, "Reset");
        if (change) *current_item = vDefault;

        SameLine();

        change |= ContrastedCombo(
            vWidth, label, current_item,
            [](void* data, int idx, const char** out_text) {
                *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
                return true;
            },
            (void*)&items, (int32_t)items.size(), height_in_items);

        PopID();
    }

    return change;
}

}  // namespace ImGui
