/*! \file imtui.h
 *  \brief Enter description here.
 */

#pragma once

// simply expose the existing Dear ImGui API
#include "imgui/imgui.h"

#include <cstring>
#include <cstdint>

namespace ImTui {

using TChar = unsigned char;
using TColor = unsigned char;

// single screen cell
// 0x0000FFFF - char
// 0x00FF0000 - foreground color
// 0xFF000000 - background color
using TCell = uint32_t;

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

}
