/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui.h"
#include "imgui/imgui_internal.h"
#include "imtui-impl-ncurses.h"

#ifdef _WIN32
#define NCURSES_MOUSE_VERSION
#include <pdcurses.h>
#define set_escdelay(X)

#define KEY_OFFSET 0xec00

#define KEY_CODE_YES     (KEY_OFFSET + 0x00) /* If get_wch() gives a key code */

#define KEY_BREAK        (KEY_OFFSET + 0x01) /* Not on PC KBD */
#define KEY_DOWN         (KEY_OFFSET + 0x02) /* Down arrow key */
#define KEY_UP           (KEY_OFFSET + 0x03) /* Up arrow key */
#define KEY_LEFT         (KEY_OFFSET + 0x04) /* Left arrow key */
#define KEY_RIGHT        (KEY_OFFSET + 0x05) /* Right arrow key */
#define KEY_HOME         (KEY_OFFSET + 0x06) /* home key */
#define KEY_BACKSPACE    (8) /* not on pc */
#define KEY_F0           (KEY_OFFSET + 0x08) /* function keys; 64 reserved */
#else
#include <ncurses.h>
#endif

#include <array>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <thread>

WINDOW* imtui_win = nullptr;

namespace
{
struct VSync {
    VSync( double fps_active = 60.0, double fps_idle = 60.0 ) :
        tStepActive_us( 1000000.0 / fps_active ),
        tStepIdle_us( 1000000.0 / fps_idle ) {}

    uint64_t tStepActive_us;
    uint64_t tStepIdle_us;
    uint64_t tLast_us = t_us();
    uint64_t tNext_us = tLast_us;

    inline uint64_t t_us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>
               ( std::chrono::high_resolution_clock::now().time_since_epoch() ).count(); // duh ..
    }

    inline void wait( bool active ) {
        uint64_t tNow_us = t_us();

        auto tStep_us = active ? tStepActive_us : tStepIdle_us;
        auto tNextCur_us = tNext_us + tStep_us;

        while( tNow_us < tNextCur_us - 100 ) {
            if( tNow_us + 0.5 * tStepActive_us < tNextCur_us ) {
                int ch = wgetch( imtui_win );

                if( ch != ERR ) {
                    ungetch( ch );
                    tNextCur_us = tNow_us;

                    return;
                }
            }

            std::this_thread::sleep_for( std::chrono::microseconds(
                                             std::min( ( uint64_t )( 0.9 * tStepActive_us ),
                                                     ( uint64_t )( 0.9 * ( tNextCur_us - tNow_us ) )
                                                     ) ) );

            tNow_us = t_us();
        }

        tNext_us += tStep_us;
    }

    inline float delta_s() {
        uint64_t tNow_us = t_us();
        uint64_t res = tNow_us - tLast_us;
        tLast_us = tNow_us;

        return float( res ) / 1e6f;
    }
};
}

static VSync g_vsync;
static ImTui::TScreen *g_screen = nullptr;

ImTui::TScreen *ImTui_ImplNcurses_Init( float fps_active, float fps_idle )
{
    if( g_screen == nullptr ) {
        g_screen = new ImTui::TScreen();
    }

    if( fps_idle < 0.0 ) {
        fps_idle = fps_active;
    }
    fps_idle = std::min( fps_active, fps_idle );
    g_vsync = VSync( fps_active, fps_idle );

    imtui_win = newwin(LINES, COLS, 0, 0);
    ImGui::GetIO().KeyMap[ImGuiKey_Tab]         = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]   = 260;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]  = 261;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]     = 259;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]   = 258;
    ImGui::GetIO().KeyMap[ImGuiKey_PageUp]      = 339;
    ImGui::GetIO().KeyMap[ImGuiKey_PageDown]    = 338;
    ImGui::GetIO().KeyMap[ImGuiKey_Home]        = 262;
    ImGui::GetIO().KeyMap[ImGuiKey_End]         = 360;
    ImGui::GetIO().KeyMap[ImGuiKey_Insert]      = 331;
    ImGui::GetIO().KeyMap[ImGuiKey_Delete]      = 330;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace]   = 263;
    ImGui::GetIO().KeyMap[ImGuiKey_Space]       = 32;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter]       = 10;
    ImGui::GetIO().KeyMap[ImGuiKey_Escape]      = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = 343;
    ImGui::GetIO().KeyMap[ImGuiKey_A]           = 1;
    ImGui::GetIO().KeyMap[ImGuiKey_C]           = 3;
    ImGui::GetIO().KeyMap[ImGuiKey_V]           = 22;
    ImGui::GetIO().KeyMap[ImGuiKey_X]           = 24;
    ImGui::GetIO().KeyMap[ImGuiKey_Y]           = 25;
    ImGui::GetIO().KeyMap[ImGuiKey_Z]           = 26;

    ImGui::GetIO().KeyRepeatDelay = 0.050;
    ImGui::GetIO().KeyRepeatRate = 0.050;

    int screenSizeX = 0;
    int screenSizeY = 0;

    getmaxyx( stdscr, screenSizeY, screenSizeX );
    ImGui::GetIO().DisplaySize = ImVec2( screenSizeX, screenSizeY );

    return g_screen;
}

void ImTui_ImplNcurses_Shutdown()
{
    // ref #11 : https://github.com/ggerganov/imtui/issues/11
    printf( "\033[?1003l\n" ); // Disable mouse movement events, as l = low

    if( g_screen ) {
        delete g_screen;
    }

    g_screen = nullptr;
}

bool ImTui_ImplNcurses_NewFrame( std::vector<std::pair<int, ImTui::mouse_event>> key_events )
{
    bool hasInput = false;

    int screenSizeX = 0;
    int screenSizeY = 0;

    getmaxyx( stdscr, screenSizeY, screenSizeX );
    ImGui::GetIO().DisplaySize = ImVec2( screenSizeX, screenSizeY );

    static int mx = 0;
    static int my = 0;
    static int lbut = 0;
    static int rbut = 0;
    static unsigned long mstate = 0;
    static char input[3];

    input[2] = 0;

    auto &keysDown = ImGui::GetIO().KeysDown;
    std::fill( keysDown, keysDown + 512, 0 );

    ImGui::GetIO().KeyCtrl = false;
    ImGui::GetIO().KeyShift = false;

    for( auto key_event_pair : key_events ) {
        int c = key_event_pair.first;

        if( c == KEY_MOUSE ) {
            mx = key_event_pair.second.x;
            my = key_event_pair.second.y;
            mstate = key_event_pair.second.bstate;
            if( ( mstate & 0x000f ) == 0x0002 ) {
                lbut = 1;
            }
            if( ( mstate & 0x000f ) == 0x0001 ) {
                lbut = 0;
            }
            if( ( mstate & 0xf000 ) == 0x2000 ) {
                rbut = 1;
            }
            if( ( mstate & 0xf000 ) == 0x1000 ) {
                rbut = 0;
            }
            //printf("mstate = 0x%016lx\n", mstate);
            ImGui::GetIO().KeyCtrl |= ( ( mstate & 0x0F000000 ) == 0x01000000 );
        } else if( c != ERR ) {
            input[0] = ( c & 0x000000FF );
            input[1] = ( c & 0x0000FF00 ) >> 8;
            //printf("c = %d, c0 = %d, c1 = %d xxx\n", c, input[0], input[1]);
            if( c < 127 ) {
                if( c != ImGui::GetIO().KeyMap[ImGuiKey_Enter] ) {
                    ImGui::GetIO().AddInputCharactersUTF8( input );
                }
            }
            if( c == 330 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Delete]] = true;
            } else if( c == KEY_BACKSPACE || c == KEY_DC || c == 127 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
                // Shift + arrows (probably not portable :()
            } else if( c == 393 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if( c == 402 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if( c == 337 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if( c == 336 ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if( c == KEY_BACKSPACE ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
            } else if( c == KEY_LEFT ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]] = true;
            } else if( c == KEY_RIGHT ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]] = true;
            } else if( c == KEY_UP ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]] = true;
            } else if( c == KEY_DOWN ) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]] = true;
            } else {
                keysDown[c] = true;
            }
        }

        hasInput = true;
    }
    if( ( mstate & 0xf ) == 0x1 ) {
        lbut = 0;
        rbut = 0;
    }

    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_A]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }
    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_C]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }
    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_V]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }
    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_X]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }
    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Y]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }
    if( ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Z]] ) {
        ImGui::GetIO().KeyCtrl = true;
    }

    ImGui::GetIO().MousePos.x = mx;
    ImGui::GetIO().MousePos.y = my;
    ImGui::GetIO().MouseDown[0] = lbut;
    ImGui::GetIO().MouseDown[1] = rbut;

    ImGui::GetIO().DeltaTime = g_vsync.delta_s();

    return hasInput;
}

// state
static short nColPairs = 1;
static int nActiveFrames = 10;
static ImTui::TScreen screenPrev;
static std::array<std::pair<bool, int>, 256 * 256> colPairs;

bool is_in_bounds( int x, int y )
{
    ImGuiContext *ctxt = ImGui::GetCurrentContext();
    // skip the first window, since ImGui seems to always have a "dummy" window that takes up most of the screen
    for( int index = 1; index < ctxt->Windows.size(); index++ ) {
        ImGuiWindow *win = ctxt->Windows[index];
        if(win->Collapsed || !win->Active) {
            continue;
        }
        if( x >= win->Pos.x && x < ( win->Pos.x + win->Size.x )  &&
            y >= win->Pos.y && y < ( win->Pos.y + win->Size.y ) ) {
            return true;
        }
    }
    return false;
}

void ImTui_ImplNcurses_UploadColorPair( short p, short f, short b )
{
    uint16_t imtui_p = b * 256 + f;
    colPairs[imtui_p] = {true, p};
    nColPairs = std::max( p, nColPairs ) + 1;
}

void ImTui_ImplNcurses_SetAllocedPairCount( short p )
{
    nColPairs = std::max(nColPairs, p) + 1;
}

void ImTui_ImplNcurses_DrawScreen( bool active )
{
    if( active ) {
        nActiveFrames = 10;
    }


    std::vector<ImRect> window_bounds;
    for( ImGuiWindow *win : ImGui::GetCurrentContext()->Windows ) {
        window_bounds.push_back( win->OuterRectClipped );
    }
    int nx = g_screen->nx;
    int ny = g_screen->ny;

    bool compare = true;

    if( screenPrev.nx != nx || screenPrev.ny != ny ) {
        screenPrev.resize( nx, ny );
        compare = false;
    }

    int ic = 0;
    wmove(imtui_win, 0, 0);
    for( int y = 0; y < ny; ++y ) {
        bool wmove_needed = true;
        constexpr int no_lastp = 0x7FFFFFFF;
        int lastp = no_lastp;
        for( int x = 0; x < nx; ++x ) {
            if( !is_in_bounds( x, y ) ) {
                wmove_needed = true;
                continue;
            }
            const auto cell = g_screen->data[y * nx + x];
            const uint16_t f = ( cell & 0x00FF0000 ) >> 16;
            const uint16_t b = ( cell & 0xFF000000 ) >> 24;
            const uint16_t p = b * 256 + f;

            if( wmove_needed ) {
                wmove( imtui_win, y, x );
            }
            if( lastp != ( int ) p ) {
                if( colPairs[p].first == false ) {
                    init_pair( nColPairs, f, b );
                    colPairs[p].first = true;
                    colPairs[p].second = nColPairs;
                    ++nColPairs;
                }
                wattron( imtui_win, COLOR_PAIR( colPairs[p].second ) );
                lastp = p;
            }

            const uint16_t c = cell & 0x0000FFFF;
            waddch( imtui_win, c );
        }
        if( lastp != no_lastp ) {
            wattroff( imtui_win, COLOR_PAIR( colPairs[lastp].second ) );
        }

        if( compare ) {
            memcpy( screenPrev.data + y * nx, g_screen->data + y * nx, nx * sizeof( ImTui::TCell ) );
        }
    }
    wrefresh(imtui_win);

    if( !compare ) {
        memcpy( screenPrev.data, g_screen->data, nx * ny * sizeof( ImTui::TCell ) );
    }

    //g_vsync.wait( nActiveFrames -- > 0 );
}

bool ImTui_ImplNcurses_ProcessEvent()
{
    return true;
}
