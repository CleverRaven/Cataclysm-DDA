/*! \file imtui.h
 *  \brief Enter description here.
 */

#pragma once

// simply expose the existing Dear ImGui API
#include "imgui/imgui.h"

#include <cstring>
#include <cstdint>
#include <vector>
#include <string>

namespace ImTui {

using TChar = unsigned char;
using TColor = unsigned char;

// single screen cell
struct TCell
{
    TColor fg;
    TColor bg;
    uint32_t ch;
    uint8_t chwidth;
};

struct TScreen {
    int nx = 0;
    int ny = 0;

    int nmax = 0;

    TCell * data = nullptr;
    ~TScreen() {
        if (data) delete [] data;
    }

    inline int size() const { return nx*ny; }

    inline void clear() {
        if (data) {
            memset(data, 0, nx*ny*sizeof(TCell));
        }
    }

    inline void resize(int pnx, int pny) {
        nx = pnx;
        ny = pny;
        if (nx*ny <= nmax) return;

        if (data) delete [] data;

        nmax = nx*ny;
        data = new TCell[nmax];
    }
};

struct ImplImtui_Data
{
    TScreen Screen;
};

static ImplImtui_Data* ImTui_Impl_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImplImtui_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

}
