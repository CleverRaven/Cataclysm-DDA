/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui.h"
#include "imgui/imgui_internal.h"
#include "imtui-impl-ncurses.h"

#define NCURSES_NOMACROS
#if !defined(__APPLE__)
#define NCURSES_WIDECHAR 1
#endif

#ifdef _WIN32
#define NCURSES_MOUSE_VERSION
#include <curses.h>
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

// SDL_Renderer data
struct ImTui_ImplNCurses_Data {
    std::vector<WINDOW *> imtui_wins;
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImTui_ImplNCurses_Data *ImTui_ImplNCurses_GetBackendData()
{
    return ImGui::GetCurrentContext() ? ( ImTui_ImplNCurses_Data * )
           ImGui::GetIO().BackendRendererUserData : nullptr;
}

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

void ImTui_ImplNcurses_Init( float fps_active, float fps_idle )
{
    if( ImGui::GetCurrentContext()->IO.BackendPlatformUserData == nullptr ) {
        ImGui::GetCurrentContext()->IO.BackendPlatformUserData = new ImTui::ImplImtui_Data();
    }
    if( ImGui::GetIO().BackendRendererUserData == nullptr ) {
        ImGui::GetIO().BackendRendererUserData = new ImTui_ImplNCurses_Data();
    }

    if( fps_idle < 0.0 ) {
        fps_idle = fps_active;
    }
    fps_idle = std::min( fps_active, fps_idle );
    g_vsync = VSync( fps_active, fps_idle );

    //imtui_win = newwin(LINES, COLS, 0, 0);

    ImGui::GetIO().KeyRepeatDelay = 0.050;
    ImGui::GetIO().KeyRepeatRate = 0.050;

    int screenSizeX = 0;
    int screenSizeY = 0;

    getmaxyx( stdscr, screenSizeY, screenSizeX );
    ImGui::GetIO().DisplaySize = ImVec2( screenSizeX, screenSizeY );
}

void ImTui_ImplNcurses_Shutdown()
{
    // ref #11 : https://github.com/ggerganov/imtui/issues/11
    printf( "\033[?1003l\n" ); // Disable mouse movement events, as l = low

    ImTui_ImplNCurses_Data *data = ImTui_ImplNCurses_GetBackendData();
    if( data ) {
        delete data;
        ImGui::GetIO().BackendRendererUserData = nullptr;
    }
}


struct ImGuiKeyTranslation {
    ImGuiKey key;
    bool shift;
    ImGuiKeyTranslation( ImGuiKey k, bool s = false ) :
        key( k ),
        shift( s )
    {}
};


// returns ImGuiKey_None if it doesn't have an ImGui_Key counterpart
// or if it is a character compatible with `ImGuiIO::addInputCharacter()`
static ImGuiKeyTranslation ncurses_special_key_to_imgui_key( int key )
{
    switch( key ) {
        case KEY_DOWN:
            return ImGuiKeyTranslation( ImGuiKey_DownArrow );
        case KEY_UP:
            return ImGuiKeyTranslation( ImGuiKey_UpArrow );
        case KEY_LEFT:
            return ImGuiKeyTranslation( ImGuiKey_LeftArrow );
        case KEY_RIGHT:
            return ImGuiKeyTranslation( ImGuiKey_RightArrow );
        case KEY_HOME:
            return ImGuiKeyTranslation( ImGuiKey_Home );
        case KEY_BACKSPACE:
            return ImGuiKeyTranslation( ImGuiKey_Backspace );
        case KEY_F( 1 ):
            return ImGuiKeyTranslation( ImGuiKey_F1 );
        case KEY_F( 2 ):
            return ImGuiKeyTranslation( ImGuiKey_F2 );
        case KEY_F( 3 ):
            return ImGuiKeyTranslation( ImGuiKey_F3 );
        case KEY_F( 4 ):
            return ImGuiKeyTranslation( ImGuiKey_F4 );
        case KEY_F( 5 ):
            return ImGuiKeyTranslation( ImGuiKey_F5 );
        case KEY_F( 6 ):
            return ImGuiKeyTranslation( ImGuiKey_F6 );
        case KEY_F( 7 ):
            return ImGuiKeyTranslation( ImGuiKey_F7 );
        case KEY_F( 8 ):
            return ImGuiKeyTranslation( ImGuiKey_F8 );
        case KEY_F( 9 ):
            return ImGuiKeyTranslation( ImGuiKey_F9 );
        case KEY_F( 10 ):
            return ImGuiKeyTranslation( ImGuiKey_F10 );
        case KEY_F( 11 ):
            return ImGuiKeyTranslation( ImGuiKey_F11 );
        case KEY_F( 12 ):
            return ImGuiKeyTranslation( ImGuiKey_F12 );
        case KEY_F( 13 ):
            return ImGuiKeyTranslation( ImGuiKey_F13 );
        case KEY_F( 14 ):
            return ImGuiKeyTranslation( ImGuiKey_F14 );
        case KEY_F( 15 ):
            return ImGuiKeyTranslation( ImGuiKey_F15 );
        case KEY_F( 16 ):
            return ImGuiKeyTranslation( ImGuiKey_F16 );
        case KEY_F( 17 ):
            return ImGuiKeyTranslation( ImGuiKey_F17 );
        case KEY_F( 18 ):
            return ImGuiKeyTranslation( ImGuiKey_F18 );
        case KEY_F( 19 ):
            return ImGuiKeyTranslation( ImGuiKey_F19 );
        case KEY_F( 20 ):
            return ImGuiKeyTranslation( ImGuiKey_F20 );
        case KEY_F( 21 ):
            return ImGuiKeyTranslation( ImGuiKey_F21 );
        case KEY_F( 22 ):
            return ImGuiKeyTranslation( ImGuiKey_F22 );
        case KEY_F( 23 ):
            return ImGuiKeyTranslation( ImGuiKey_F23 );
        case KEY_F( 24 ):
            return ImGuiKeyTranslation( ImGuiKey_F24 );
        case KEY_DC:
            return ImGuiKeyTranslation( ImGuiKey_Delete );
        case KEY_IC:
            return ImGuiKeyTranslation( ImGuiKey_Insert );
        case KEY_NPAGE:
            return ImGuiKeyTranslation( ImGuiKey_PageUp );
        case KEY_PPAGE:
            return ImGuiKeyTranslation( ImGuiKey_PageDown );
        case KEY_ENTER:
            return ImGuiKeyTranslation( ImGuiKey_Enter );
        case KEY_PRINT:
            return ImGuiKeyTranslation( ImGuiKey_PrintScreen );
        case KEY_A1:
            return ImGuiKeyTranslation( ImGuiKey_Keypad7 );
        case KEY_A3:
            return ImGuiKeyTranslation( ImGuiKey_Keypad9 );
        case KEY_B2:
            return ImGuiKeyTranslation( ImGuiKey_Keypad5 );
        case KEY_C1:
            return ImGuiKeyTranslation( ImGuiKey_Keypad1 );
        case KEY_C3:
            return ImGuiKeyTranslation( ImGuiKey_Keypad3 );
        case KEY_END:
            return ImGuiKeyTranslation( ImGuiKey_End );
        case KEY_SDC:
            return ImGuiKeyTranslation( ImGuiKey_Delete, true );
        case KEY_SEND:
            return ImGuiKeyTranslation( ImGuiKey_End, true );
        case KEY_SHOME:
            return ImGuiKeyTranslation( ImGuiKey_Home, true );
        case KEY_SIC:
            return ImGuiKeyTranslation( ImGuiKey_Insert, true );
        case KEY_SPRINT:
            return ImGuiKeyTranslation( ImGuiKey_PrintScreen, true );
        case KEY_SLEFT:
            return ImGuiKeyTranslation( ImGuiKey_LeftArrow, true );
        case KEY_SRIGHT:
            return ImGuiKeyTranslation( ImGuiKey_RightArrow, true );
        case KEY_SR:
            return ImGuiKeyTranslation( ImGuiKey_UpArrow, true );
        case KEY_SF:
            return ImGuiKeyTranslation( ImGuiKey_DownArrow, true );
        default:
            return ImGuiKeyTranslation( ImGuiKey_None );
    }
}


bool ImTui_ImplNcurses_NewFrame( std::vector<std::pair<int, ImTui::mouse_event>> key_events )
{
    bool hasInput = false;

    int screenSizeX = 0;
    int screenSizeY = 0;

    getmaxyx( stdscr, screenSizeY, screenSizeX );

    ImGuiIO &io = ImGui::GetIO();

    io.DisplaySize = ImVec2( screenSizeX, screenSizeY );

    static int mx = 0;
    static int my = 0;
    static int lbut = 0;
    static int rbut = 0;
    static unsigned long mstate = 0;
    static char input[3];

    input[2] = 0;



    io.KeyCtrl = false;
    io.KeyShift = false;

    for( int k = ( int )ImGuiKey_NamedKey_BEGIN; k < ( int )ImGuiKey_NamedKey_END; k++ ) {
        ImGuiKey key = ( ImGuiKey )k;
        // AddKeyEvent doesn't allow alias keys
        if( !ImGui::IsAliasKey( key ) && ImGui::IsKeyDown( key ) ) {
            io.AddKeyEvent( key, false );
        }
    }

    for( std::pair<int, ImTui::mouse_event> key_event_pair : key_events ) {
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
            io.KeyCtrl |= ( ( mstate & 0x0F000000 ) == 0x01000000 );
        } else if( c != ERR ) {
            input[0] = ( c & 0x000000FF );
            input[1] = ( c & 0x0000FF00 ) >> 8;
            //printf("c = %d, c0 = %d, c1 = %d xxx\n", c, input[0], input[1]);
            if( c < 127 ) {
                if( !ImGui::IsKeyDown( ImGuiKey_Enter ) ) {
                    io.AddInputCharactersUTF8( input );
                }
            } else {
                ImGuiKeyTranslation trans = ncurses_special_key_to_imgui_key( c );
                if( trans.key == ImGuiKey_None ) {
                    io.AddInputCharacter( c );
                } else {
                    io.AddKeyEvent( trans.key, true );
                    io.KeyShift = trans.shift;
                }
            }
        }

        hasInput = true;
    }
    if( ( mstate & 0xf ) == 0x1 ) {
        lbut = 0;
        rbut = 0;
    }

    if( ImGui::IsKeyDown( ImGuiKey_A ) ) {
        io.KeyCtrl = true;
    }
    if( ImGui::IsKeyDown( ImGuiKey_C ) ) {
        io.KeyCtrl = true;
    }
    if( ImGui::IsKeyDown( ImGuiKey_V ) ) {
        io.KeyCtrl = true;
    }
    if( ImGui::IsKeyDown( ImGuiKey_X ) ) {
        io.KeyCtrl = true;
    }
    if( ImGui::IsKeyDown( ImGuiKey_Y ) ) {
        io.KeyCtrl = true;
    }
    if( ImGui::IsKeyDown( ImGuiKey_Z ) ) {
        io.KeyCtrl = true;
    }

    io.MousePos.x = mx;
    io.MousePos.y = my;
    io.MouseDown[0] = lbut;
    io.MouseDown[1] = rbut;

    io.DeltaTime = g_vsync.delta_s();

    return hasInput;
}

// state
static short nColPairs = 1;
static int nActiveFrames = 10;
static ImTui::TScreen screenPrev;
static std::array<std::pair<bool, int>, 256 * 256> colPairs;

void ImTui_ImplNcurses_UploadColorPair( short p, short f, short b )
{
    uint16_t imtui_p = b * 256 + f;
    colPairs[imtui_p] = {true, p};
    nColPairs = std::max( p, nColPairs ) + 1;
}

void ImTui_ImplNcurses_SetAllocedPairCount( short p )
{
    nColPairs = std::max( nColPairs, p ) + 1;
}

void create_destroy_curses_windows( std::vector<WINDOW *> &windows )
{
    size_t cursesWinIdx = 0;
    int imguiWinIdx = 0;
    for( ; imguiWinIdx < GImGui->Windows.Size; imguiWinIdx++ ) {
        ImGuiWindow *imwin = GImGui->Windows[imguiWinIdx];
        if( imwin->Collapsed || !imwin->Active ) {
            continue;
        }
        if( windows.size() <= cursesWinIdx ) {
            windows.push_back( newwin( short( imwin->Size.y ), short( imwin->Size.x ), short( imwin->Pos.y ),
                                       short( imwin->Pos.x ) ) );
        } else {
            WINDOW *win = windows[cursesWinIdx];
            if( getbegx( win ) != short( imwin->Pos.x ) || getbegy( win ) != short( imwin->Pos.y ) ||
                getmaxx( win ) != short( imwin->Size.x ) || getmaxy( win ) != imwin->Size.y ) {
                delwin( win );
                windows[cursesWinIdx] = newwin( short( imwin->Size.y ), short( imwin->Size.x ),
                                                short( imwin->Pos.y ), short( imwin->Pos.x ) );
            }
        }
        cursesWinIdx++;
    }
    if( cursesWinIdx < int( windows.size() ) ) {
        size_t lastIndex = cursesWinIdx;
        for( ; cursesWinIdx < windows.size(); cursesWinIdx++ ) {
            if( windows[cursesWinIdx] != nullptr ) {
                delwin( windows[cursesWinIdx] );
            }
        }
        windows.erase( windows.begin() + lastIndex, windows.end() );
    }
}
wchar_t strTmp[] = {0, 0};
void ImTui_ImplNcurses_DrawScreen( bool active )
{
    ImTui::ImplImtui_Data *bd = ImTui::ImTui_Impl_GetBackendData();
    ImTui_ImplNCurses_Data *rd = ImTui_ImplNCurses_GetBackendData();
    if( active ) {
        nActiveFrames = 10;
    }

    ImTui::TScreen &g_screen = bd->Screen;
    create_destroy_curses_windows( rd->imtui_wins );
    ImFont *font = nullptr;
    for( WINDOW *cursesWin : rd->imtui_wins ) {
        int nx = g_screen.nx;
        int ny = g_screen.ny;

        for( int y = getbegy( cursesWin ); y <= ( getbegy( cursesWin ) + getmaxy( cursesWin ) ); ++y ) {
            constexpr int no_lastp = 0x7FFFFFFF;
            int lastp = no_lastp;
            wmove( cursesWin, y - getbegy( cursesWin ), 0 );
            for( int x = getbegx( cursesWin ); x <= ( getbegx( cursesWin ) +  getmaxx( cursesWin ) ); ++x ) {
                const auto cell = g_screen.data[y * nx + x];
                const uint16_t f = cell.fg;
                const uint16_t b = cell.bg;
                const uint16_t p = b * 256 + f;

                if( lastp != ( int ) p ) {
                    if( colPairs[p].first == false ) {
                        init_pair( nColPairs, f, b );
                        colPairs[p].first = true;
                        colPairs[p].second = nColPairs;
                        ++nColPairs;
                    }
                    wattron( cursesWin, COLOR_PAIR( colPairs[p].second ) );
                    lastp = p;
                }
                strTmp[0] = cell.ch;
                waddwstr( cursesWin, strTmp );

                if( cell.chwidth > 1 ) {
                    x += ( cell.chwidth - 1 );
                }
            }
            if( lastp != no_lastp ) {
                wattroff( cursesWin, COLOR_PAIR( colPairs[lastp].second ) );
            }
        }

        wnoutrefresh( cursesWin );
    }

}

bool ImTui_ImplNcurses_ProcessEvent()
{
    return true;
}
