/*! \file imtui-impl-ncurses.h
 *  \brief Enter description here.
 */

#pragma once

#include <vector>

namespace ImTui
{
struct TScreen;
struct mouse_event {
    int x, y, z;
    unsigned int bstate;
};
}

// the interface below allows the user to decide when the application is active or not
// this can be used to reduce the redraw rate, and thus the CPU usage, when suitable
// for example - there is no user input, or the displayed content hasn't changed significantly

// fps_active - specify the redraw rate when the application is active
// fps_idle - specify the redraw rate when the application is not active
ImTui::TScreen *ImTui_ImplNcurses_Init( float fps_active = 60.0, float fps_idle = -1.0 );

void ImTui_ImplNcurses_Shutdown();

// returns true if there is any user input from the keyboard/mouse
bool ImTui_ImplNcurses_NewFrame( std::vector<std::pair<int, ImTui::mouse_event>> key_events );

// active - specify which redraw rate to use: fps_active or fps_idle
void ImTui_ImplNcurses_DrawScreen( bool active = true );

void ImTui_ImplNcurses_UploadColorPair( short p, short f, short b );
void ImTui_ImplNcurses_SetAllocedPairCount( short p );

bool ImTui_ImplNcurses_ProcessEvent();
