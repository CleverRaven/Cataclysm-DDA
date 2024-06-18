#if !defined(TILES) && defined(_WIN32)
#define UNICODE 1
#ifndef CMAKE
#pragma GCC diagnostic ignored "-Wunused-macros"
#define _UNICODE 1
#endif
#include "cursesport.h" // IWYU pragma: associated

#include <cstdlib>
#include <fstream>

#include "cached_options.h"
#include "cursesdef.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"
#include "init.h"
#include "input.h"
#include "path_info.h"
#include "filesystem.h"
#include "debug.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "string_formatter.h"
#include "color_loader.h"
#include "font_loader.h"
#include "platform_win.h"
#include "mmsystem.h"
#include "ui_manager.h"
#include "wcwidth.h"

//***********************************
//Globals                           *
//***********************************

static constexpr int ERR = -1;
const wchar_t *szWindowClass = L"CataCurseWindow";    //Class name :D
HINSTANCE WindowINST;   //the instance of the window
HWND WindowHandle;      //the handle of the window
HDC WindowDC;           //Device Context of the window, used for backbuffer
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
HDC backbuffer;         //an off-screen DC to prevent flickering, lower CPU
HBITMAP backbit;        //the bitmap that is used in conjunction with the above
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines
HFONT font;             //Handle to the font created by CreateFont
std::array<RGBQUAD, color_loader<RGBQUAD>::COLOR_NAMES_COUNT> windowsPalette;
unsigned char *dcbits;  //the bits of the screen image, for direct access
bool CursorVisible = true; // Showcursor is a somewhat weird function
bool needs_resize = false; // The window needs to be resized
bool initialized = false;

static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;

//***********************************
//Non-curses, Window functions      *
//***********************************

// Declare this locally, because it's not generally cross-compatible in cursesport.h
LRESULT CALLBACK ProcessMessages( HWND__ *hWnd, std::uint32_t Msg, WPARAM wParam, LPARAM lParam );

static std::wstring widen( const std::string &s )
{
    if( s.empty() ) {
        // MultiByteToWideChar can not handle this case
        return std::wstring();
    }
    std::vector<wchar_t> buffer( s.length() );
    const int newlen = MultiByteToWideChar( CP_UTF8, 0, s.c_str(), s.length(),
                                            buffer.data(), buffer.size() );
    // On failure, newlen is 0, returns an empty strings.
    return std::wstring( buffer.data(), newlen );
}

static constexpr uint32_t WndStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX |
                                     WS_MAXIMIZEBOX | WS_SYSMENU | WS_VISIBLE;

// Registers, creates, and shows the Window!!
static bool WinCreate()
{
    // Get current process handle
    WindowINST = GetModuleHandle( nullptr );

    // Register window class
    WNDCLASSEXW WindowClassType   = WNDCLASSEXW();
    WindowClassType.cbSize        = sizeof( WNDCLASSEXW );
    // The procedure that gets msgs
    WindowClassType.lpfnWndProc   = ProcessMessages;
    // hInstance
    WindowClassType.hInstance     = WindowINST;
    // Get first resource
    WindowClassType.hIcon         = LoadIcon( WindowINST, MAKEINTRESOURCE( 0 ) );
    WindowClassType.hIconSm       = LoadIcon( WindowINST, MAKEINTRESOURCE( 0 ) );
    WindowClassType.hCursor       = LoadCursor( nullptr, IDC_ARROW );
    WindowClassType.lpszClassName = szWindowClass;
    if( !RegisterClassExW( &WindowClassType ) ) {
        return false;
    }

    // Adjust window size
    // Basic window, show on creation
    RECT WndRect;
    WndRect.left   = WndRect.top = 0;
    WndRect.right  = WindowWidth;
    WndRect.bottom = WindowHeight;
    AdjustWindowRect( &WndRect, WndStyle, false );

    // Center window
    RECT WorkArea;
    SystemParametersInfo( SPI_GETWORKAREA, 0, &WorkArea, 0 );
    int WindowX = WorkArea.right / 2 - ( WndRect.right - WndRect.left ) / 2;
    int WindowY = WorkArea.bottom / 2 - ( WndRect.bottom - WndRect.top ) / 2;

    // Magic
    WindowHandle = CreateWindowExW( 0, szWindowClass, L"", WndStyle,
                                    WindowX, WindowY,
                                    WndRect.right - WndRect.left,
                                    WndRect.bottom - WndRect.top,
                                    nullptr, nullptr, WindowINST, nullptr );
    return WindowHandle != nullptr;

}

// Unregisters, releases the DC if needed, and destroys the window.
static void WinDestroy()
{
    if( ( WindowDC != nullptr ) && ( ReleaseDC( WindowHandle, WindowDC ) == 0 ) ) {
        WindowDC = nullptr;
    }
    if( WindowHandle != nullptr && ( !( DestroyWindow( WindowHandle ) ) ) ) {
        WindowHandle = nullptr;
    }
    if( !( UnregisterClassW( szWindowClass, WindowINST ) ) ) {
        WindowINST = nullptr;
    }
}

// Creates a backbuffer to prevent flickering
static void create_backbuffer()
{
    if( WindowDC != nullptr ) {
        ReleaseDC( WindowHandle, WindowDC );
    }
    if( backbuffer != nullptr ) {
        ReleaseDC( WindowHandle, backbuffer );
    }
    WindowDC   = GetDC( WindowHandle );
    backbuffer = CreateCompatibleDC( WindowDC );

    BITMAPINFO bmi = BITMAPINFO();
    bmi.bmiHeader.biSize         = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth        = WindowWidth;
    bmi.bmiHeader.biHeight       = -WindowHeight;
    bmi.bmiHeader.biPlanes       = 1;
    bmi.bmiHeader.biBitCount     = 8;
    bmi.bmiHeader.biCompression  = BI_RGB; // Raw RGB
    bmi.bmiHeader.biSizeImage    = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed      = color_loader<RGBQUAD>::COLOR_NAMES_COUNT; // Colors in the palette
    bmi.bmiHeader.biClrImportant = color_loader<RGBQUAD>::COLOR_NAMES_COUNT; // Colors in the palette
    backbit = CreateDIBSection( nullptr, &bmi, DIB_RGB_COLORS, reinterpret_cast<void **>( &dcbits ),
                                nullptr,
                                0 );
    DeleteObject( SelectObject( backbuffer, backbit ) ); //load the buffer into DC
}

bool handle_resize( int, int )
{
    if( !initialized ) {
        return false;
    }
    needs_resize = false;
    RECT WndRect;
    if( GetClientRect( WindowHandle, &WndRect ) ) {
        TERMINAL_WIDTH = WndRect.right / fontwidth;
        TERMINAL_HEIGHT = WndRect.bottom / fontheight;
        WindowWidth = TERMINAL_WIDTH * fontwidth;
        WindowHeight = TERMINAL_HEIGHT * fontheight;
        catacurses::stdscr = catacurses::newwin( TERMINAL_HEIGHT, TERMINAL_WIDTH, point_zero );
        catacurses::resizeterm();
        create_backbuffer();
        SetBkMode( backbuffer, TRANSPARENT ); //Transparent font backgrounds
        SelectObject( backbuffer, font ); //Load our font into the DC
        color_loader<RGBQUAD>().load( windowsPalette );
        if( SetDIBColorTable( backbuffer, 0, windowsPalette.size(), windowsPalette.data() ) == 0 ) {
            throw std::runtime_error( "SetDIBColorTable failed" );
        }
        ui_manager::screen_resized();
    }

    return true;
}

void resize_term( const int cell_w, const int cell_h )
{
    RECT WndRect;
    WndRect.left = WndRect.top = 0;
    WndRect.right = cell_w * fontwidth * get_scaling_factor();
    WndRect.bottom = cell_h * fontheight * get_scaling_factor();
    if( !AdjustWindowRect( &WndRect, WndStyle, false ) ) {
        return;
    }
    if( !SetWindowPos( WindowHandle, nullptr, 0, 0,
                       WndRect.right - WndRect.left, WndRect.bottom - WndRect.top,
                       SWP_NOMOVE | SWP_NOZORDER ) ) {
        return;
    }
    GetClientRect( WindowHandle, &WndRect );
    handle_resize( WndRect.right - WndRect.left, WndRect.bottom - WndRect.top );
}

// Copied from sdlcurses.cpp
#define ALT_BUFFER_SIZE 8
static char alt_buffer[ALT_BUFFER_SIZE] = {};
static int alt_buffer_len = 0;
static bool alt_down = false;
static bool shift_down = false;

static void begin_alt_code()
{
    alt_buffer[0] = '\0';
    alt_down = true;
    alt_buffer_len = 0;
}

static void add_alt_code( char c )
{
    // Not exactly how it works, but acceptable
    if( c >= '0' && c <= '9' ) {
        if( alt_buffer_len < ALT_BUFFER_SIZE - 1 ) {
            alt_buffer[alt_buffer_len] = c;
            alt_buffer[++alt_buffer_len] = '\0';
        }
    }
}

static int end_alt_code()
{
    alt_down = false;
    return atoi( alt_buffer );
}

// This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages( HWND__ *hWnd, unsigned int Msg,
                                  WPARAM wParam, LPARAM lParam )
{
    uint16_t MouseOver;
    switch( Msg ) {
        case WM_DEADCHAR:
        case WM_CHAR:
            lastchar = static_cast<int>( wParam );
            switch( lastchar ) {
                case VK_TAB:
                    lastchar = ( shift_down ) ? KEY_BTAB : '\t';
                    break;
                case VK_RETURN:
                    // Reroute ENTER key for compatibility purposes
                    lastchar = 10;
                    break;
                case VK_BACK:
                    // Reroute BACKSPACE key for compatibility purposes
                    lastchar = 127;
                    break;
            }
            return 0;

        case WM_KEYDOWN:
            // Here we handle non-character input
            switch( wParam ) {
                case VK_SHIFT:
                    shift_down = true;
                    break;
                case VK_LEFT:
                    lastchar = KEY_LEFT;
                    break;
                case VK_RIGHT:
                    lastchar = KEY_RIGHT;
                    break;
                case VK_UP:
                    lastchar = KEY_UP;
                    break;
                case VK_DOWN:
                    lastchar = KEY_DOWN;
                    break;
                case VK_NEXT:
                    lastchar = KEY_NPAGE;
                    break;
                case VK_PRIOR:
                    lastchar = KEY_PPAGE;
                    break;
                case VK_HOME:
                    lastchar = KEY_HOME;
                    break;
                case VK_END:
                    lastchar = KEY_END;
                    break;
                case VK_F1:
                    lastchar = KEY_F( 1 );
                    break;
                case VK_F2:
                    lastchar = KEY_F( 2 );
                    break;
                case VK_F3:
                    lastchar = KEY_F( 3 );
                    break;
                case VK_F4:
                    lastchar = KEY_F( 4 );
                    break;
                case VK_F5:
                    lastchar = KEY_F( 5 );
                    break;
                case VK_F6:
                    lastchar = KEY_F( 6 );
                    break;
                case VK_F7:
                    lastchar = KEY_F( 7 );
                    break;
                case VK_F8:
                    lastchar = KEY_F( 8 );
                    break;
                case VK_F9:
                    lastchar = KEY_F( 9 );
                    break;
                case VK_F10:
                    lastchar = KEY_F( 10 );
                    break;
                case VK_F11:
                    lastchar = KEY_F( 11 );
                    break;
                case VK_F12:
                    lastchar = KEY_F( 12 );
                    break;
                default:
                    break;
            }
            return 0;

        case WM_KEYUP:
            if( wParam == VK_SHIFT ) {
                shift_down = false;
                return 0;
            }

            // LeftAlt hack
            if( !GetAsyncKeyState( VK_LMENU ) && alt_down ) {
                if( int code = end_alt_code() ) {
                    lastchar = code;
                }
            }
            return 0;

        case WM_SIZE:
            WINDOWPLACEMENT win_placement;
            win_placement.length = sizeof( win_placement );
            if( GetWindowPlacement( hWnd, &win_placement ) && win_placement.showCmd != SW_SHOWMINIMIZED ) {
                needs_resize = true;
            }
            return 0;

        case WM_SYSCHAR:
            add_alt_code( static_cast<char>( wParam ) );
            return 0;

        case WM_SYSKEYDOWN:
            // LeftAlt hack
            if( GetAsyncKeyState( VK_LMENU ) && !alt_down ) {
                begin_alt_code();
            }
            break;

        case WM_SETCURSOR:
            MouseOver = LOWORD( lParam );
            if( get_option<std::string>( "HIDE_CURSOR" ) == "hide" ) {
                if( MouseOver == HTCLIENT && CursorVisible ) {
                    CursorVisible = false;
                    ShowCursor( false );
                } else if( MouseOver != HTCLIENT && !CursorVisible ) {
                    CursorVisible = true;
                    ShowCursor( true );
                }
            } else if( !CursorVisible ) {
                CursorVisible = true;
                ShowCursor( true );
            }
            break;

        case WM_ERASEBKGND:
            // Don't erase background
            return 1;

        case WM_PAINT:
            BitBlt( WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0, SRCCOPY );
            ValidateRect( WindowHandle, nullptr );
            return 0;

        case WM_DESTROY:
            // A messy exit, but easy way to escape game loop
            exit( 0 );
    }

    return DefWindowProcW( hWnd, Msg, wParam, lParam );
}

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB( int x, int y, int y2, int thickness, unsigned char color )
{
    int j;
    for( j = y; j < y2; j++ ) {
        memset( &dcbits[x + j * WindowWidth], color, thickness );
    }
}
inline void HorzLineDIB( int x, int y, int x2, int thickness, unsigned char color )
{
    int j;
    for( j = y; j < y + thickness; j++ ) {
        memset( &dcbits[x + j * WindowWidth], color, x2 - x );
    }
}
inline void FillRectDIB( int x, int y, int width, int height, unsigned char color )
{
    int j;
    for( j = y; j < y + height; j++ )
        //NOTE TO FUTURE: this breaks if j is negative. Apparently it doesn't break if j is too large, though?
    {
        memset( &dcbits[x + j * WindowWidth], color, width );
    }
}

void cata_cursesport::curses_drawwindow( const catacurses::window &w )
{

    WINDOW *const win = w.get<WINDOW>();
    int i = 0;
    int j = 0;
    int drawx = 0;
    int drawy = 0;
    wchar_t tmp;

    for( j = 0; j < win->height; j++ ) {
        if( win->line[j].touched ) {
            win->line[j].touched = false;

            for( i = 0; i < win->width; i++ ) {
                const cursecell &cell = win->line[j].chars[i];
                if( cell.ch.empty() ) {
                    // second cell of a multi-cell character
                    continue;
                }
                drawx = ( ( win->pos.x + i ) * fontwidth );
                drawy = ( ( win->pos.y + j ) * fontheight ); //-j;
                if( drawx + fontwidth > WindowWidth || drawy + fontheight > WindowHeight ) {
                    // Outside of the display area, would not render anyway
                    continue;
                }

                int FG = cell.FG;
                int BG = cell.BG;
                FillRectDIB( drawx, drawy, fontwidth, fontheight, BG );
                static const std::string space_string = " ";
                // Spaces don't need any drawing except background
                if( cell.ch == space_string ) {
                    continue;
                }

                tmp = UTF8_getch( cell.ch );
                if( tmp != UNKNOWN_UNICODE ) {

                    int color = RGB( windowsPalette[FG].rgbRed, windowsPalette[FG].rgbGreen,
                                     windowsPalette[FG].rgbBlue );
                    SetTextColor( backbuffer, color );

                    int cw = mk_wcwidth( tmp );
                    if( cw > 1 ) {
                        FillRectDIB( drawx + fontwidth * ( cw - 1 ), drawy, fontwidth, fontheight, BG );
                        i += cw - 1;
                    }
                    if( tmp ) {
                        const std::wstring utf16 = widen( cell.ch );
                        ExtTextOutW( backbuffer, drawx, drawy, 0, nullptr, utf16.c_str(), utf16.length(), nullptr );
                    }
                } else {
                    switch( static_cast<unsigned char>( win->line[j].chars[i].ch[0] ) ) {
                        // box bottom/top side (horizontal line)
                        case LINE_OXOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            break;
                        // box left/right side (vertical line)
                        case LINE_XOXO_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            break;
                        // box top left
                        case LINE_OXXO_C:
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;
                        // box top right
                        case LINE_OOXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;
                        // box bottom right
                        case LINE_XOOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight + 1, 2, FG );
                            break;
                        // box bottom left
                        case LINE_XXOO_C:
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight + 1, 2, FG );
                            break;
                        // box bottom north T (left, right, up)
                        case LINE_XXOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight, 2, FG );
                            break;
                        // box bottom east T (up, right, down)
                        case LINE_XXXO_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            break;
                        // box bottom south T (left, right, down)
                        case LINE_OXXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;
                        // box X (left down up right)
                        case LINE_XXXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            break;
                        // box bottom east T (left, down, up)
                        case LINE_XOXX_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            break;
                        default:
                            break;
                    }//switch (tmp)
                }//(tmp < 0)
            }//for (i=0;i<win->width;i++)
        }
    }// for (j=0;j<win->height;j++)
    // We drew the window, mark it as so
    win->draw = false;
}

// Check for any window messages (keypress, paint, mousemove, etc)
static void CheckMessages()
{
    MSG msg;
    while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    if( needs_resize ) {
        restore_on_out_of_scope<int> prev_lastchar( lastchar );
        handle_resize( 0, 0 );
        refresh_display();
    }
}

// Calculates the new width of the window
int projected_window_width()
{
    return get_option<int>( "TERMINAL_X" ) * fontwidth;
}

// Calculates the new height of the window
int projected_window_height()
{
    return get_option<int>( "TERMINAL_Y" ) * fontheight;
}

int get_terminal_width()
{
    return TERMINAL_WIDTH;
}

int get_terminal_height()
{
    return TERMINAL_HEIGHT;
}

//***********************************
//Pseudo-Curses Functions           *
//***********************************

// Basic Init, create the font, backbuffer, etc
void catacurses::init_interface()
{
    lastchar = -1;
    inputdelay = -1;

    font_loader fl;
    fl.load();
    ::fontwidth = fl.fontwidth;
    ::fontheight = fl.fontheight;
    halfwidth = fontwidth / 2;
    halfheight = fontheight / 2;
    TERMINAL_WIDTH = get_option<int>( "TERMINAL_X" );
    TERMINAL_HEIGHT = get_option<int>( "TERMINAL_Y" );
    WindowWidth = TERMINAL_WIDTH * fontwidth;
    WindowHeight = TERMINAL_HEIGHT * fontheight;

    // Create the actual window, register it, etc
    WinCreate();
    // Set Sleep resolution to 1ms
    timeBeginPeriod( 1 );
    // Let the message queue handle setting up the window
    CheckMessages();

    create_backbuffer();

    // Load private fonts
    if( SetCurrentDirectoryW( L"data\\font" ) ) {
        WIN32_FIND_DATAW findData;
        for( HANDLE findFont = FindFirstFileW( L".\\*", &findData ); findFont != INVALID_HANDLE_VALUE; ) {
            // Skip folders
            if( !( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
                AddFontResourceExW( findData.cFileName, FR_PRIVATE, nullptr );
            }
            if( !FindNextFileW( findFont, &findData ) ) {
                FindClose( findFont );
                break;
            }
        }
        SetCurrentDirectoryW( L"..\\.." );
    }

    // Use desired font, if possible
    cata_assert( !fl.typeface.empty() );
    font = CreateFontW( fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        PROOF_QUALITY, FF_MODERN, widen( fl.typeface.front() ).c_str() );

    // Transparent font backgrounds
    SetBkMode( backbuffer, TRANSPARENT );
    // Load our font into the DC
    SelectObject( backbuffer, font );

    color_loader<RGBQUAD>().load( windowsPalette );
    if( SetDIBColorTable( backbuffer, 0, windowsPalette.size(), windowsPalette.data() ) == 0 ) {
        throw std::runtime_error( "SetDIBColorTable failed" );
    }
    init_colors();

    stdscr = newwin( get_option<int>( "TERMINAL_Y" ), get_option<int>( "TERMINAL_X" ), point_zero );
    //newwin calls `new WINDOW`, and that will throw, but not return nullptr.

    initialized = true;
}

bool catacurses::supports_256_colors()
{
    return COLORS >= 256;
}

// A very accurate and responsive timer (NEVER use GetTickCount)
static uint64_t GetPerfCount()
{
    uint64_t Count;
    QueryPerformanceCounter( reinterpret_cast<PLARGE_INTEGER>( &Count ) );
    return Count;
}

void input_manager::pump_events()
{
    if( test_mode ) {
        return;
    }

    // Handle all events, but ignore any keypress
    CheckMessages();

    lastchar = ERR;
    previously_pressed_key = 0;
}

// we can probably add support for keycode mode, but wincurse is deprecated
// so we just ignore the mode argument.
input_event input_manager::get_input_event( const keyboard_mode /*preferred_keyboard_mode*/ )
{
    if( test_mode ) {
        // input should be skipped in caller's code
        throw std::runtime_error( "input_manager::get_input_event called in test mode" );
    }

    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there
    uint64_t Frequency;
    QueryPerformanceFrequency( reinterpret_cast<PLARGE_INTEGER>( &Frequency ) );
    wrefresh( catacurses::stdscr );
    InvalidateRect( WindowHandle, nullptr, true );
    lastchar = ERR;
    if( inputdelay < 0 ) {
        for( ; lastchar == ERR; Sleep( 1 ) ) {
            CheckMessages();
        }
    } else if( inputdelay > 0 ) {
        for( uint64_t t0 = GetPerfCount(), t1 = 0; t1 < ( t0 + inputdelay * Frequency / 1000 );
             t1 = GetPerfCount() ) {
            CheckMessages();
            if( lastchar != ERR ) {
                break;
            }
            Sleep( 1 );
        }
    } else {
        CheckMessages();
    }

    if( lastchar != ERR && get_option<std::string>( "HIDE_CURSOR" ) == "hidekb" && CursorVisible ) {
        CursorVisible = false;
        ShowCursor( false );
    }

    previously_pressed_key = 0;
    input_event rval;
    if( lastchar == ERR ) {
        if( input_timeout > 0 ) {
            rval.type = input_event_t::timeout;
        } else {
            rval.type = input_event_t::error;
        }
    } else {
        // == Unicode DELETE
        if( lastchar == 127 ) {
            previously_pressed_key = KEY_BACKSPACE;
            return input_event( KEY_BACKSPACE, input_event_t::keyboard_char );
        }
        rval.type = input_event_t::keyboard_char;
        rval.text = utf32_to_utf8( lastchar );
        previously_pressed_key = lastchar;
        // for compatibility only add the first byte, not the code point
        // as it would  conflict with the special keys defined by ncurses
        rval.add_input( lastchar );
    }

    return rval;
}

bool gamepad_available()
{
    return false;
}

std::optional<tripoint> input_context::get_coordinates( const catacurses::window &, const point &,
        bool center_cursor ) const
{
    // TODO: implement this properly
    return std::nullopt;
}

// Ends the terminal, destroy everything
void catacurses::endwin()
{
    DeleteObject( font );
    WinDestroy();
    // Unload it
    RemoveFontResourceExA( "data\\font", FR_PRIVATE, nullptr );
}

template<>
RGBQUAD color_loader<RGBQUAD>::from_rgb( const int r, const int g, const int b )
{
    RGBQUAD result;
    // Blue
    result.rgbBlue = b;
    // Green
    result.rgbGreen = g;
    // Red
    result.rgbRed = r;
    //The Alpha, is not used, so just set it to 0
    result.rgbReserved = 0;
    return result;
}

void input_manager::set_timeout( const int t )
{
    input_timeout = t;
    inputdelay = t;
}

int get_scaling_factor()
{
    return 1;
}

HWND getWindowHandle()
{
    return WindowHandle;
}

void refresh_display()
{
    RedrawWindow( WindowHandle, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
}

void set_title( const std::string &title )
{
    if( WindowHandle != nullptr ) {
        SetWindowTextW( WindowHandle, widen( title ).c_str() );
    }
}

#endif
