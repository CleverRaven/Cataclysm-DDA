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

#pragma once
#pragma warning(disable : 4251)

#ifdef IMGUI_INCLUDE
#include IMGUI_INCLUDE
#else  // IMGUI_INCLUDE
#include <imgui.h>
#endif  // IMGUI_INCLUDE

#include <cstdint>  // types like uint32_t
#include <vector>
#include <string>

#ifdef USE_OPENGL
#include <ctools/cTools.h>
#endif

#define ImWidgets_VERSION "ImWidgets v1.0"

#ifndef IS_FLOAT_DIFFERENT
#define IS_FLOAT_DIFFERENT(a, b) (fabs((a) - (b)) > FLT_EPSILON)
#endif  // IS_FLOAT_DIFFERENT

#ifndef IS_FLOAT_EQUAL
#define IS_FLOAT_EQUAL(a, b) (fabs((a) - (b)) < FLT_EPSILON)
#endif  // IS_FLOAT_EQUAL
struct ImGuiWindow;
struct GLFWWindow;

namespace ImGui {

class IMGUI_API CustomStyle {
public:
    static float puContrastRatio;
    static ImU32 puContrastedTextColor;
    static int pushId;
    static int minorNumber;
    static int majorNumber;
    static int buildNumber;
    static ImVec4 ImGuiCol_Symbol;
    static ImVec4 GoodColor;
    static ImVec4 BadColor;
    static void Init();
    static void ResetCustomId();
};

IMGUI_API int IncPUSHID();
IMGUI_API int GetPUSHID();
IMGUI_API void SetPUSHID(int vID);

IMGUI_API void SetContrastRatio(float vRatio);
IMGUI_API void SetContrastedTextColor(ImU32 vColor);

IMGUI_API float CalcContrastRatio(const ImU32& backGroundColor, const ImU32& foreGroundColor);
IMGUI_API bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImU32& invertedColor, const float& minContrastRatio);
IMGUI_API bool PushStyleColorWithContrast(const ImGuiCol& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio);
IMGUI_API bool PushStyleColorWithContrast(const ImU32& backGroundColor, const ImGuiCol& foreGroundColor, const ImVec4& invertedColor, const float& maxContrastRatio);

IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, bool active, bool disabled);
IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool active, bool disabled = false, ImFont* vLabelFont = nullptr);
IMGUI_API bool RadioButtonLabeled(float vWidth, const char* label, const char* help, bool* active, bool disabled = false, ImFont* vLabelFont = nullptr);

template <typename T>
bool RadioButtonLabeled_BitWize(float vWidth, const char* vLabel, const char* vHelp, T* vContainer, T vFlag,
                                          bool vOneOrZeroAtTime     = false,  // only one selcted at a time
                                          bool vAlwaysOne           = true,   // radio behavior, always one selected
                                          T vFlagsToTakeIntoAccount = (T)0, bool vDisableSelection = false,
                                          ImFont* vLabelFont = nullptr)  // radio witl use only theses flags
{
    bool selected  = *vContainer & vFlag;
    const bool res = RadioButtonLabeled(vWidth, vLabel, vHelp, &selected, vDisableSelection, vLabelFont);
    if (res) {
        if (selected) {
            if (vOneOrZeroAtTime) {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = vFlag;  // set
            } else {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = (T)(*vContainer | vFlag);  // add
            }
        } else {
            if (vOneOrZeroAtTime) {
                if (!vAlwaysOne)
                    *vContainer = (T)(0);  // remove all
            } else
                *vContainer = (T)(*vContainer & ~vFlag);  // remove one
        }
    }
    return res;
}

template <typename T>
bool RadioButtonLabeled_BitWize(float vWidth, const char* vLabelOK, const char* vLabelNOK, const char* vHelp, T* vContainer, T vFlag,
                                          bool vOneOrZeroAtTime     = false,  // only one selcted at a time
                                          bool vAlwaysOne           = true,   // radio behavior, always one selected
                                          T vFlagsToTakeIntoAccount = (T)0, bool vDisableSelection = false,
                                          ImFont* vLabelFont = nullptr)  // radio witl use only theses flags
{
    bool selected     = *vContainer & vFlag;
    const char* label = (selected ? vLabelOK : vLabelNOK);
    const bool res    = RadioButtonLabeled(vWidth, label, vHelp, &selected, vDisableSelection, vLabelFont);
    if (res) {
        if (selected) {
            if (vOneOrZeroAtTime) {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = vFlag;  // set
            } else {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = (T)(*vContainer | vFlag);  // add
            }
        } else {
            if (vOneOrZeroAtTime) {
                if (!vAlwaysOne)
                    *vContainer = (T)(0);  // remove all
            } else
                *vContainer = (T)(*vContainer & ~vFlag);  // remove one
        }
    }
    return res;
}

IMGUI_API bool RadioButtonLabeled(ImVec2 vSize, const char* label, bool active, bool disabled);
IMGUI_API bool RadioButtonLabeled(ImVec2 vSize, const char* label, const char* help, bool active, bool disabled = false, ImFont* vLabelFont = nullptr);
IMGUI_API bool RadioButtonLabeled(ImVec2 vSize, const char* label, const char* help, bool* active, bool disabled = false, ImFont* vLabelFont = nullptr);

template <typename T>
bool RadioButtonLabeled_BitWize(ImVec2 vSize, const char* vLabel, const char* vHelp, T* vContainer, T vFlag,
                                          bool vOneOrZeroAtTime     = false,  // only one selcted at a time
                                          bool vAlwaysOne           = true,   // radio behavior, always one selected
                                          T vFlagsToTakeIntoAccount = (T)0, bool vDisableSelection = false,
                                          ImFont* vLabelFont = nullptr)  // radio witl use only theses flags
{
    bool selected  = *vContainer & vFlag;
    const bool res = RadioButtonLabeled(vSize, vLabel, vHelp, &selected, vDisableSelection, vLabelFont);
    if (res) {
        if (selected) {
            if (vOneOrZeroAtTime) {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = vFlag;  // set
            } else {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = (T)(*vContainer | vFlag);  // add
            }
        } else {
            if (vOneOrZeroAtTime) {
                if (!vAlwaysOne)
                    *vContainer = (T)(0);  // remove all
            } else
                *vContainer = (T)(*vContainer & ~vFlag);  // remove one
        }
    }
    return res;
}

template <typename T>
bool RadioButtonLabeled_BitWize(ImVec2 vSize, const char* vLabelOK, const char* vLabelNOK, const char* vHelp, T* vContainer, T vFlag,
                                          bool vOneOrZeroAtTime     = false,  // only one selcted at a time
                                          bool vAlwaysOne           = true,   // radio behavior, always one selected
                                          T vFlagsToTakeIntoAccount = (T)0, bool vDisableSelection = false,
                                          ImFont* vLabelFont = nullptr)  // radio witl use only theses flags
{
    bool selected     = *vContainer & vFlag;
    const char* label = (selected ? vLabelOK : vLabelNOK);
    const bool res    = RadioButtonLabeled(vSize, label, vHelp, &selected, vDisableSelection, vLabelFont);
    if (res) {
        if (selected) {
            if (vOneOrZeroAtTime) {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = vFlag;  // set
            } else {
                if (vFlagsToTakeIntoAccount) {
                    if (vFlag & vFlagsToTakeIntoAccount) {
                        *vContainer = (T)(*vContainer & ~vFlagsToTakeIntoAccount);  // remove these flags
                        *vContainer = (T)(*vContainer | vFlag);                     // add
                    }
                } else
                    *vContainer = (T)(*vContainer | vFlag);  // add
            }
        } else {
            if (vOneOrZeroAtTime) {
                if (!vAlwaysOne)
                    *vContainer = (T)(0);  // remove all
            } else
                *vContainer = (T)(*vContainer & ~vFlag);  // remove one
        }
    }
    return res;
}

IMGUI_API bool ContrastedButton_For_Dialogs(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
IMGUI_API bool ContrastedButton(const char* label, const char* help = nullptr, ImFont* imfont = nullptr, float vWidth = 0.0f, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);
IMGUI_API bool ToggleContrastedButton(const char* vLabelTrue, const char* vLabelFalse, bool* vValue, const char* vHelp = nullptr, ImFont* vImfont = nullptr);
IMGUI_API bool SmallContrastedButton(const char* label);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// COMBO ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IMGUI_API bool BeginContrastedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedCombo(float vWidth, const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);
IMGUI_API bool ContrastedComboVectorDefault(float vWidth, const char* label, int* current_item, const std::vector<std::string>& items, int vDefault, int height_in_items = -1);

}  // namespace ImWidgets
