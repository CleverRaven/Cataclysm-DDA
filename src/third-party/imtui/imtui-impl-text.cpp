/*! \file imtui-impl-text.cpp
 *  \brief Enter description here.
 */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imtui.h"
#include "imtui-impl-text.h"

#include <cmath>
#include <vector>

#define ABS(x) ((x >= 0) ? x : -x)

void ScanLine(int x1, int y1, int x2, int y2, int ymax, std::vector<int> & xrange) {
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    sx = x2 - x1;
    sy = y2 - y1;

    if (sx > 0) dx1 = 1;
    else if (sx < 0) dx1 = -1;
    else dx1 = 0;

    if (sy > 0) dy1 = 1;
    else if (sy < 0) dy1 = -1;
    else dy1 = 0;

    m = ABS(sx);
    n = ABS(sy);
    dx2 = dx1;
    dy2 = 0;

    if (m < n)
    {
        m = ABS(sy);
        n = ABS(sx);
        dx2 = 0;
        dy2 = dy1;
    }

    x = x1; y = y1;
    cnt = m + 1;
    k = n / 2;

    while (cnt--) {
        if ((y >= 0) && (y < ymax)) {
            if (x < xrange[2*y+0]) xrange[2*y+0] = x;
            if (x > xrange[2*y+1]) xrange[2*y+1] = x;
        }

        k += n;
        if (k < m) {
            x += dx2;
            y += dy2;
        } else {
            k -= m;
            x += dx1;
            y += dy1;
        }
    }
}

static std::vector<int> g_xrange;

void drawTriangle(ImVec2 p0, ImVec2 p1, ImVec2 p2, unsigned char col, ImTui::TScreen * screen) {
    int ymin = std::min(std::min(std::min((float) screen->size(), p0.y), p1.y), p2.y);
    int ymax = std::max(std::max(std::max(0.0f, p0.y), p1.y), p2.y);

    int ydelta = ymax - ymin + 1;

    if ((int) g_xrange.size() < 2*ydelta) {
        g_xrange.resize(2*ydelta);
    }

    for (int y = 0; y < ydelta; y++) {
        g_xrange[2*y+0] = 999999;
        g_xrange[2*y+1] = -999999;
    }

    ScanLine(p0.x, p0.y - ymin, p1.x, p1.y - ymin, ydelta, g_xrange);
    ScanLine(p1.x, p1.y - ymin, p2.x, p2.y - ymin, ydelta, g_xrange);
    ScanLine(p2.x, p2.y - ymin, p0.x, p0.y - ymin, ydelta, g_xrange);

    for (int y = 0; y < ydelta; y++) {
        if (g_xrange[2*y+1] >= g_xrange[2*y+0]) {
            int x = g_xrange[2*y+0];
            int len = 1 + g_xrange[2*y+1] - g_xrange[2*y+0];

            while (len--) {
                if (x >= 0 && x < screen->nx && y + ymin >= 0 && y + ymin < screen->ny) {
                    auto & cell = screen->data[(y + ymin)*screen->nx + x];
                    cell &= 0x00FF0000;
                    cell |= ' ';
                    cell |= ((ImTui::TCell)(col) << 24);
                }
                ++x;
            }
        }
    }
}

inline ImTui::TColor rgbToAnsi256(ImU32 col, bool doAlpha) {
    ImTui::TColor r = col & 0x000000FF;
    ImTui::TColor g = (col & 0x0000FF00) >> 8;
    ImTui::TColor b = (col & 0x00FF0000) >> 16;

    if (r == g && g == b) {
        if (doAlpha) {
            ImTui::TColor a = (col & 0xFF000000) >> 24;
            r = (float(r)*a)/255.0f;
        }
        if (r < 8) {
            return 16;
        }

        if (r > 248) {
            return 231;
        }

        return std::round((float(r - 8) / 247) * 24) + 232;
    }

    if (doAlpha) {
        ImTui::TColor a = (col & 0xFF000000) >> 24;
        float scale = float(a)/255.0f;
        r = std::round(r*scale);
        g = std::round(g*scale);
        b = std::round(b*scale);
    }

    ImTui::TColor res = 16
        + (36 * std::round((float(r) / 255.0f) * 5.0f))
        + (6 * std::round((float(g) / 255.0f) * 5.0f))
        + std::round((float(b) / 255.0f) * 5.0f);

    return res;
}

void ImTui_ImplText_RenderDrawData(ImDrawData * drawData, ImTui::TScreen * screen) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);

    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    screen->resize(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    screen->clear();

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = drawData->CmdLists[n];

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    float lastCharX = -10000.0f;
                    float lastCharY = -10000.0f;

                    for (unsigned int i = 0; i < pcmd->ElemCount; i += 3) {
                        int vidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 0];
                        int vidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 1];
                        int vidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 2];

                        auto pos0 = cmd_list->VtxBuffer[vidx0].pos;
                        auto pos1 = cmd_list->VtxBuffer[vidx1].pos;
                        auto pos2 = cmd_list->VtxBuffer[vidx2].pos;

                        pos0.x = std::max(std::min(float(clip_rect.z - 1), pos0.x), clip_rect.x);
                        pos1.x = std::max(std::min(float(clip_rect.z - 1), pos1.x), clip_rect.x);
                        pos2.x = std::max(std::min(float(clip_rect.z - 1), pos2.x), clip_rect.x);
                        pos0.y = std::max(std::min(float(clip_rect.w - 1), pos0.y), clip_rect.y);
                        pos1.y = std::max(std::min(float(clip_rect.w - 1), pos1.y), clip_rect.y);
                        pos2.y = std::max(std::min(float(clip_rect.w - 1), pos2.y), clip_rect.y);

                        auto uv0 = cmd_list->VtxBuffer[vidx0].uv;
                        auto uv1 = cmd_list->VtxBuffer[vidx1].uv;
                        auto uv2 = cmd_list->VtxBuffer[vidx2].uv;

                        auto col0 = cmd_list->VtxBuffer[vidx0].col;
                        //auto col1 = cmd_list->VtxBuffer[vidx1].col;
                        //auto col2 = cmd_list->VtxBuffer[vidx2].col;

                        if (uv0.x != uv1.x || uv0.x != uv2.x || uv1.x != uv2.x ||
                            uv0.y != uv1.y || uv0.y != uv2.y || uv1.y != uv2.y) {
                            int vvidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 3];
                            int vvidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 4];
                            int vvidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 5];

                            auto ppos0 = cmd_list->VtxBuffer[vvidx0].pos;
                            auto ppos1 = cmd_list->VtxBuffer[vvidx1].pos;
                            auto ppos2 = cmd_list->VtxBuffer[vvidx2].pos;

                            float x = ((pos0.x + pos1.x + pos2.x + ppos0.x + ppos1.x + ppos2.x)/6.0f);
                            float y = ((pos0.y + pos1.y + pos2.y + ppos0.y + ppos1.y + ppos2.y)/6.0f) + 0.5f;

                            if (std::fabs(y - lastCharY) < 0.5f && std::fabs(x - lastCharX) < 0.5f) {
                                x = lastCharX + 1.0f;
                                y = lastCharY;
                            }

                            lastCharX = x;
                            lastCharY = y;

                            int xx = (x) + 1;
                            int yy = (y) + 0;
                            if (xx < clip_rect.x || xx >= clip_rect.z || yy < clip_rect.y || yy >= clip_rect.w) {
                            } else {
                                auto & cell = screen->data[yy*screen->nx + xx];
                                cell &= 0xFF000000;
                                cell |= (col0 & 0xff000000) >> 24;
                                cell |= ((ImTui::TCell)(rgbToAnsi256(col0, false)) << 16);
                            }
                            i += 3;
                        } else {
                            drawTriangle(pos0, pos1, pos2, rgbToAnsi256(col0, true), screen);
                        }
                    }
                }
            }
        }
    }

}

bool ImTui_ImplText_Init() {
    ImGui::GetStyle().Alpha                   = 1.0f;
    ImGui::GetStyle().WindowPadding           = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().WindowRounding          = 0.0f;
    ImGui::GetStyle().WindowBorderSize        = 0.0f;
    ImGui::GetStyle().WindowMinSize           = ImVec2(4.0f, 2.0f);
    ImGui::GetStyle().WindowTitleAlign        = ImVec2(0.0f, 0.0f);
    ImGui::GetStyle().WindowMenuButtonPosition= ImGuiDir_Left;
    ImGui::GetStyle().ChildRounding           = 0.0f;
    ImGui::GetStyle().ChildBorderSize         = 0.0f;
    ImGui::GetStyle().PopupRounding           = 0.0f;
    ImGui::GetStyle().PopupBorderSize         = 0.0f;
    ImGui::GetStyle().FramePadding            = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().FrameRounding           = 0.0f;
    ImGui::GetStyle().FrameBorderSize         = 0.0f;
    ImGui::GetStyle().ItemSpacing             = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().ItemInnerSpacing        = ImVec2(1.0f, 0.0f);
    ImGui::GetStyle().TouchExtraPadding       = ImVec2(0.5f, 0.0f);
    ImGui::GetStyle().IndentSpacing           = 1.0f;
    ImGui::GetStyle().ColumnsMinSpacing       = 1.0f;
    ImGui::GetStyle().ScrollbarSize           = 0.5f;
    ImGui::GetStyle().ScrollbarRounding       = 0.0f;
    ImGui::GetStyle().GrabMinSize             = 0.1f;
    ImGui::GetStyle().GrabRounding            = 0.0f;
    ImGui::GetStyle().TabRounding             = 0.0f;
    ImGui::GetStyle().TabBorderSize           = 0.0f;
    ImGui::GetStyle().ColorButtonPosition     = ImGuiDir_Right;
    ImGui::GetStyle().ButtonTextAlign         = ImVec2(0.5f,0.0f);
    ImGui::GetStyle().SelectableTextAlign     = ImVec2(0.0f,0.0f);
    ImGui::GetStyle().DisplayWindowPadding    = ImVec2(0.0f,0.0f);
    ImGui::GetStyle().DisplaySafeAreaPadding  = ImVec2(0.0f,0.0f);
    ImGui::GetStyle().CellPadding             = ImVec2(1.0f,0.0f);
    ImGui::GetStyle().MouseCursorScale        = 1.0f;
    ImGui::GetStyle().AntiAliasedLines        = false;
    ImGui::GetStyle().AntiAliasedFill         = false;
    ImGui::GetStyle().CurveTessellationTol    = 1.25f;

    ImGui::GetStyle().Colors[ImGuiCol_WindowBg]         = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg]          = ImVec4(0.35, 0.35, 0.35, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15, 0.15, 0.15, 1.0f);
    ImGui::GetStyle().Colors[ImGuiCol_TextSelectedBg]   = ImVec4(0.75, 0.75, 0.75, 0.5f);
    ImGui::GetStyle().Colors[ImGuiCol_NavHighlight]     = ImVec4(0.00, 0.00, 0.00, 0.0f);

    ImFontConfig fontConfig;
    fontConfig.GlyphMinAdvanceX = 1.0f;
    fontConfig.SizePixels = 1.00;
    static const ImWchar ranges[] = {
        0x0020, 0x052F,
        0x1D00, 0x1DFF,
        0x2000, 0x206F,
        0x20A0, 0x20CF,
        0x2100, 0x214F,
        0x2190, 0x22FF,
        //0x0020, 0xCFFF,
        0
    };
    fontConfig.GlyphRanges = ranges;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    // Build atlas
    ImGui::GetIO().Fonts->Build();

    return true;
}

void ImTui_ImplText_Shutdown() {
}

void ImTui_ImplText_NewFrame() {
}
