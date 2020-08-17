#if defined(TILES)

#include "cursesdef.h" // IWYU pragma: associated
#include "sdltiles.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>
#if defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL_image.h>
#   include <SDL2/SDL_syswm.h>
#else
#ifdef _WIN32
#   include <SDL_syswm.h>
#endif
#endif

#include "avatar.h"
#include "cata_tiles.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "color_loader.h"
#include "cursesport.h"
#include "debug.h"
#include "filesystem.h"
#include "font_loader.h"
#include "game.h"
#include "game_ui.h"
#include "get_version.h"
#include "hash_utils.h"
#include "input.h"
#include "runtime_handlers.h"
#include "json.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "sdl_wrappers.h"
#include "sdl_geometry.h"
#include "sdl_font.h"
#include "sdlsound.h"
#include "string_formatter.h"
#include "ui_manager.h"
#include "wcwidth.h"

#if defined(__linux__)
#   include <cstdlib> // getenv()/setenv()
#endif

#if defined(_WIN32)
#   if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#       include "platform_win.h"
#   endif
#   include <shlwapi.h>
#endif

#if defined(__ANDROID__)
#include <jni.h>

#include "action.h"
#include "inventory.h"
#include "map.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "worldfactory.h"
#endif

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

//***********************************
//Globals                           *
//***********************************

std::unique_ptr<cata_tiles> tilecontext;
static uint32_t lastupdate = 0;
static uint32_t interval = 25;
static bool needupdate = false;

palette_array windowsPalette;

static Font_Ptr font;
static Font_Ptr map_font;
static Font_Ptr overmap_font;

static SDL_Window_Ptr window;
static SDL_Renderer_Ptr renderer;
static SDL_PixelFormat_Ptr format;
static SDL_Texture_Ptr display_buffer;
static GeometryRenderer_Ptr geometry;
#if defined(__ANDROID__)
static SDL_Texture_Ptr touch_joystick;
#endif
static int WindowWidth;        //Width of the actual window, not the curses window
static int WindowHeight;       //Height of the actual window, not the curses window
// input from various input sources. Each input source sets the type and
// the actual input value (key pressed, mouse button clicked, ...)
// This value is finally returned by input_manager::get_input_event.
static input_event last_input;

static constexpr int ERR = -1;
static int inputdelay;         //How long getch will wait for a character to be typed
static Uint32 delaydpad =
    std::numeric_limits<Uint32>::max();     // Used for entering diagonal directions with d-pad.
static Uint32 dpad_delay =
    100;   // Delay in milliseconds between registering a d-pad event and processing it.
static bool dpad_continuous = false;  // Whether we're currently moving continuously with the dpad.
static int lastdpad = ERR;      // Keeps track of the last dpad press.
static int queued_dpad = ERR;   // Queued dpad press, for individual button presses.
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;
bool fullscreen;
int scaling_factor;

static SDL_Joystick *joystick; // Only one joystick for now.

using cata_cursesport::curseline;
using cata_cursesport::cursecell;
static std::vector<curseline> oversized_framebuffer;
static std::vector<curseline> terminal_framebuffer;
static std::weak_ptr<void> winBuffer; //tracking last drawn window to fix the framebuffer
static int fontScaleBuffer; //tracking zoom levels to fix framebuffer w/tiles

//***********************************
//Non-curses, Window functions      *
//***********************************

static bool operator==( const cata_cursesport::WINDOW *const lhs, const catacurses::window &rhs )
{
    return lhs == rhs.get();
}

static void ClearScreen()
{
    SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    RenderClear( renderer );
}

static void InitSDL()
{
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    int ret;

#if defined(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING)
    SDL_SetHint( SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1" );
#endif

#if defined(__linux__)
    // https://bugzilla.libsdl.org/show_bug.cgi?id=3472#c5
    if( SDL_COMPILEDVERSION == SDL_VERSIONNUM( 2, 0, 5 ) ) {
        const char *xmod = getenv( "XMODIFIERS" );
        if( xmod && strstr( xmod, "@im=ibus" ) != nullptr ) {
            setenv( "XMODIFIERS", "@im=none", 1 );
        }
    }
#endif

    ret = SDL_Init( init_flags );
    throwErrorIf( ret != 0, "SDL_Init failed" );

    ret = TTF_Init();
    throwErrorIf( ret != 0, "TTF_Init failed" );

    // cata_tiles won't be able to load the tiles, but the normal SDL
    // code will display fine.
    ret = IMG_Init( IMG_INIT_PNG );
    printErrorIf( ( ret & IMG_INIT_PNG ) != IMG_INIT_PNG,
                  "IMG_Init failed to initialize PNG support, tiles won't work" );

    ret = SDL_InitSubSystem( SDL_INIT_JOYSTICK );
    printErrorIf( ret != 0, "Initializing joystick subsystem failed" );

    //SDL2 has no functionality for INPUT_DELAY, we would have to query it manually, which is expensive
    //SDL2 instead uses the OS's Input Delay.

    atexit( SDL_Quit );
}

static bool SetupRenderTarget()
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
    display_buffer.reset( SDL_CreateTexture( renderer.get(), SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_TARGET, WindowWidth / scaling_factor, WindowHeight / scaling_factor ) );
    if( printErrorIf( !display_buffer, "Failed to create window buffer" ) ) {
        return false;
    }
    if( printErrorIf( SDL_SetRenderTarget( renderer.get(), display_buffer.get() ) != 0,
                      "SDL_SetRenderTarget failed" ) ) {
        return false;
    }
    ClearScreen();

    return true;
}

//Registers, creates, and shows the Window!!
static void WinCreate()
{
    std::string version = string_format( "Cataclysm: Bright Nights - %s", getVersionString() );

    // Common flags used for fulscreen and for windowed
    int window_flags = 0;
    WindowWidth = TERMINAL_WIDTH * fontwidth * scaling_factor;
    WindowHeight = TERMINAL_HEIGHT * fontheight * scaling_factor;
    window_flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    if( get_option<std::string>( "SCALING_MODE" ) != "none" ) {
        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, get_option<std::string>( "SCALING_MODE" ).c_str() );
    }

#if !defined(__ANDROID__)
    if( get_option<std::string>( "FULLSCREEN" ) == "fullscreen" ) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
        fullscreen = true;
        SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );
    } else if( get_option<std::string>( "FULLSCREEN" ) == "windowedbl" ) {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        fullscreen = true;
        SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );
    } else if( get_option<std::string>( "FULLSCREEN" ) == "maximized" ) {
        window_flags |= SDL_WINDOW_MAXIMIZED;
    }
#endif

    int display = std::stoi( get_option<std::string>( "DISPLAY" ) );
    if( display < 0 || display >= SDL_GetNumVideoDisplays() ) {
        display = 0;
    }

#if defined(__ANDROID__)
    // Bugfix for red screen on Samsung S3/Mali
    // https://forums.libsdl.org/viewtopic.php?t=11445
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );

    // Fix Back button crash on Android 9
#if defined(SDL_HINT_ANDROID_TRAP_BACK_BUTTON )
    const bool trap_back_button = get_option<bool>( "ANDROID_TRAP_BACK_BUTTON" );
    SDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, trap_back_button ? "1" : "0" );
#endif

    // Prevent mouse|touch input confusion
#if defined(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH)
    SDL_SetHint( SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1" );
#else
    SDL_SetHint( SDL_HINT_MOUSE_TOUCH_EVENTS, "0" );
    SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
#endif
#endif

    ::window.reset( SDL_CreateWindow( version.c_str(),
                                      SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                                      SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                                      WindowWidth,
                                      WindowHeight,
                                      window_flags
                                    ) );
    throwErrorIf( !::window, "SDL_CreateWindow failed" );

#if !defined(__ANDROID__)
    // On Android SDL seems janky in windowed mode so we're fullscreen all the time.
    // Fullscreen mode is now modified so it obeys terminal width/height, rather than
    // overwriting it with this calculation.
    if( window_flags & SDL_WINDOW_FULLSCREEN || window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP
        || window_flags & SDL_WINDOW_MAXIMIZED ) {
        SDL_GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );
        // Ignore previous values, use the whole window, but nothing more.
        TERMINAL_WIDTH = WindowWidth / fontwidth / scaling_factor;
        TERMINAL_HEIGHT = WindowHeight / fontheight / scaling_factor;
    }
#endif
    // Initialize framebuffer caches
    terminal_framebuffer.resize( TERMINAL_HEIGHT );
    for( int i = 0; i < TERMINAL_HEIGHT; i++ ) {
        terminal_framebuffer[i].chars.assign( TERMINAL_WIDTH, cursecell( "" ) );
    }

    oversized_framebuffer.resize( TERMINAL_HEIGHT );
    for( int i = 0; i < TERMINAL_HEIGHT; i++ ) {
        oversized_framebuffer[i].chars.assign( TERMINAL_WIDTH, cursecell( "" ) );
    }

    const Uint32 wformat = SDL_GetWindowPixelFormat( ::window.get() );
    format.reset( SDL_AllocFormat( wformat ) );
    throwErrorIf( !format, "SDL_AllocFormat failed" );

    int renderer_id = -1;
#if !defined(__ANDROID__)
    bool software_renderer = get_option<std::string>( "RENDERER" ).empty();
    std::string renderer_name;
    if( software_renderer ) {
        renderer_name = "software";
    } else {
        renderer_name = get_option<std::string>( "RENDERER" );
    }

    const int numRenderDrivers = SDL_GetNumRenderDrivers();
    for( int i = 0; i < numRenderDrivers; i++ ) {
        SDL_RendererInfo ri;
        SDL_GetRenderDriverInfo( i, &ri );
        if( renderer_name == ri.name ) {
            renderer_id = i;
            DebugLog( D_INFO, DC_ALL ) << "Active renderer: " << renderer_id << "/" << ri.name;
            break;
        }
    }
#else
    bool software_renderer = get_option<bool>( "SOFTWARE_RENDERING" );
#endif

#if defined(SDL_HINT_RENDER_BATCHING)
    SDL_SetHint( SDL_HINT_RENDER_BATCHING, get_option<bool>( "RENDER_BATCHING" ) ? "1" : "0" );
#endif
    if( !software_renderer ) {
        dbg( D_INFO ) << "Attempting to initialize accelerated SDL renderer.";

        renderer.reset( SDL_CreateRenderer( ::window.get(), renderer_id, SDL_RENDERER_ACCELERATED |
                                            SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE ) );
        if( printErrorIf( !renderer,
                          "Failed to initialize accelerated renderer, falling back to software rendering" ) ) {
            software_renderer = true;
        } else if( !SetupRenderTarget() ) {
            dbg( D_ERROR ) <<
                           "Failed to initialize display buffer under accelerated rendering, falling back to software rendering.";
            software_renderer = true;
            display_buffer.reset();
            renderer.reset();
        }
    }

    if( software_renderer ) {
        if( get_option<bool>( "FRAMEBUFFER_ACCEL" ) ) {
            SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );
        }
        renderer.reset( SDL_CreateRenderer( ::window.get(), -1,
                                            SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE ) );
        throwErrorIf( !renderer, "Failed to initialize software renderer" );
        throwErrorIf( !SetupRenderTarget(),
                      "Failed to initialize display buffer under software rendering, unable to continue." );
    }

    SDL_SetWindowMinimumSize( ::window.get(), fontwidth * FULL_SCREEN_WIDTH * scaling_factor,
                              fontheight * FULL_SCREEN_HEIGHT * scaling_factor );

#if defined(__ANDROID__)
    // TODO: Not too sure why this works to make fullscreen on Android behave. :/
    if( window_flags & SDL_WINDOW_FULLSCREEN || window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP
        || window_flags & SDL_WINDOW_MAXIMIZED ) {
        SDL_GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );
    }

    // Load virtual joystick texture
    touch_joystick = CreateTextureFromSurface( renderer, load_image( "android/joystick.png" ) );
#endif

    ClearScreen();

    // Errors here are ignored, worst case: the option does not work as expected,
    // but that won't crash
    if( get_option<std::string>( "HIDE_CURSOR" ) != "show" && SDL_ShowCursor( -1 ) ) {
        SDL_ShowCursor( SDL_DISABLE );
    } else {
        SDL_ShowCursor( SDL_ENABLE );
    }

    // Initialize joysticks.
    int numjoy = SDL_NumJoysticks();

    if( get_option<bool>( "ENABLE_JOYSTICK" ) && numjoy >= 1 ) {
        if( numjoy > 1 ) {
            dbg( D_WARNING ) <<
                             "You have more than one gamepads/joysticks plugged in, only the first will be used.";
        }
        joystick = SDL_JoystickOpen( 0 );
        printErrorIf( joystick == nullptr, "SDL_JoystickOpen failed" );
        if( joystick ) {
            printErrorIf( SDL_JoystickEventState( SDL_ENABLE ) < 0,
                          "SDL_JoystickEventState(SDL_ENABLE) failed" );
        }
    } else {
        joystick = nullptr;
    }

    // Set up audio mixer.
    init_sound();

    DebugLog( D_INFO, DC_ALL ) << "USE_COLOR_MODULATED_TEXTURES is set to " <<
                               get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" );
    //initialize the alternate rectangle texture for replacing SDL_RenderFillRect
    if( get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" ) && !software_renderer ) {
        geometry = std::make_unique<ColorModulatedGeometryRenderer>( renderer );
    } else {
        geometry = std::make_unique<DefaultGeometryRenderer>();
    }
}

static void WinDestroy()
{
#if defined(__ANDROID__)
    touch_joystick.reset();
#endif

    shutdown_sound();
    tilecontext.reset();

    if( joystick ) {
        SDL_JoystickClose( joystick );

        joystick = nullptr;
    }
    geometry.reset();
    format.reset();
    display_buffer.reset();
    renderer.reset();
    ::window.reset();
}

/// Converts a color from colorscheme to SDL_Color.
inline const SDL_Color &color_as_sdl( const unsigned char color )
{
    return windowsPalette[color];
}

#if defined(__ANDROID__)
void draw_terminal_size_preview();
void draw_quick_shortcuts();
void draw_virtual_joystick();

static bool quick_shortcuts_enabled = true;

// For previewing the terminal size with a transparent rectangle overlay when user is adjusting it in the settings
static int preview_terminal_width = -1;
static int preview_terminal_height = -1;
static uint32_t preview_terminal_change_time = 0;

extern "C" {

    static bool visible_display_frame_dirty = false;
    static bool has_visible_display_frame = false;
    static SDL_Rect visible_display_frame;

    JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_onNativeVisibleDisplayFrameChanged(
        JNIEnv *env, jclass jcls, jint left, jint top, jint right, jint bottom )
    {
        ( void )env; // unused
        ( void )jcls; // unused
        has_visible_display_frame = true;
        visible_display_frame_dirty = true;
        visible_display_frame.x = left;
        visible_display_frame.y = top;
        visible_display_frame.w = right - left;
        visible_display_frame.h = bottom - top;
    }

} // "C"

SDL_Rect get_android_render_rect( float DisplayBufferWidth, float DisplayBufferHeight )
{
    // If the display buffer aspect ratio is wider than the display,
    // draw it at the top of the screen so it doesn't get covered up
    // by the virtual keyboard. Otherwise just center it.
    SDL_Rect dstrect;
    float DisplayBufferAspect = DisplayBufferWidth / ( float )DisplayBufferHeight;
    float WindowHeightLessShortcuts = ( float )WindowHeight;
    if( !get_option<bool>( "ANDROID_SHORTCUT_OVERLAP" ) && quick_shortcuts_enabled ) {
        WindowHeightLessShortcuts -= get_option<int>( "ANDROID_SHORTCUT_HEIGHT" );
    }
    float WindowAspect = WindowWidth / ( float )WindowHeightLessShortcuts;
    if( WindowAspect < DisplayBufferAspect ) {
        dstrect.x = 0;
        dstrect.y = 0;
        dstrect.w = WindowWidth;
        dstrect.h = WindowWidth / DisplayBufferAspect;
    } else {
        dstrect.x = 0.5f * ( WindowWidth - ( WindowHeightLessShortcuts * DisplayBufferAspect ) );
        dstrect.y = 0;
        dstrect.w = WindowHeightLessShortcuts * DisplayBufferAspect;
        dstrect.h = WindowHeightLessShortcuts;
    }

    // Make sure the destination rectangle fits within the visible area
    if( get_option<bool>( "ANDROID_KEYBOARD_SCREEN_SCALE" ) && has_visible_display_frame ) {
        int vdf_right = visible_display_frame.x + visible_display_frame.w;
        int vdf_bottom = visible_display_frame.y + visible_display_frame.h;
        if( vdf_right < dstrect.x + dstrect.w ) {
            dstrect.w = vdf_right - dstrect.x;
        }
        if( vdf_bottom < dstrect.y + dstrect.h ) {
            dstrect.h = vdf_bottom - dstrect.y;
        }
    }
    return dstrect;
}

#endif

void refresh_display()
{
    needupdate = false;
    lastupdate = SDL_GetTicks();

    if( test_mode ) {
        return;
    }

    // Select default target (the window), copy rendered buffer
    // there, present it, select the buffer as target again.
    SetRenderTarget( renderer, nullptr );
#if defined(__ANDROID__)
    SDL_Rect dstrect = get_android_render_rect( TERMINAL_WIDTH * fontwidth,
                       TERMINAL_HEIGHT * fontheight );
    SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    RenderClear( renderer );
    RenderCopy( renderer, display_buffer, NULL, &dstrect );
#else
    RenderCopy( renderer, display_buffer, nullptr, nullptr );
#endif
#if defined(__ANDROID__)
    draw_terminal_size_preview();
    draw_quick_shortcuts();
    draw_virtual_joystick();
#endif
    SDL_RenderPresent( renderer.get() );
    SetRenderTarget( renderer, display_buffer );
}

// only update if the set interval has elapsed
static void try_sdl_update()
{
    uint32_t now = SDL_GetTicks();
    if( now - lastupdate >= interval ) {
        refresh_display();
    } else {
        needupdate = true;
    }
}

//for resetting the render target after updating texture caches in cata_tiles.cpp
void set_displaybuffer_rendertarget()
{
    SetRenderTarget( renderer, display_buffer );
}

static void invalidate_framebuffer( std::vector<curseline> &framebuffer, const point &p, int width,
                                    int height )
{
    for( int j = 0, fby = p.y; j < height; j++, fby++ ) {
        std::fill_n( framebuffer[fby].chars.begin() + p.x, width, cursecell( "" ) );
    }
}

static void invalidate_framebuffer( std::vector<curseline> &framebuffer )
{
    for( curseline &i : framebuffer ) {
        std::fill_n( i.chars.begin(), i.chars.size(), cursecell( "" ) );
    }
}

void reinitialize_framebuffer()
{
    static int prev_height = -1;
    static int prev_width = -1;
    //Re-initialize the framebuffer with new values.
    const int new_height = std::max( { TERMY, OVERMAP_WINDOW_HEIGHT, TERRAIN_WINDOW_HEIGHT } );
    const int new_width = std::max( { TERMX, OVERMAP_WINDOW_WIDTH, TERRAIN_WINDOW_WIDTH } );
    if( new_height != prev_height || new_width != prev_width ) {
        prev_height = new_height;
        prev_width = new_width;
        oversized_framebuffer.resize( new_height );
        for( int i = 0; i < new_height; i++ ) {
            oversized_framebuffer[i].chars.assign( new_width, cursecell( "" ) );
        }
        terminal_framebuffer.resize( new_height );
        for( int i = 0; i < new_height; i++ ) {
            terminal_framebuffer[i].chars.assign( new_width, cursecell( "" ) );
        }
    }
}

static void invalidate_framebuffer_proportion( cata_cursesport::WINDOW *win )
{
    const int oversized_width = std::max( TERMX, std::max( OVERMAP_WINDOW_WIDTH,
                                          TERRAIN_WINDOW_WIDTH ) );
    const int oversized_height = std::max( TERMY, std::max( OVERMAP_WINDOW_HEIGHT,
                                           TERRAIN_WINDOW_HEIGHT ) );

    // check if the framebuffers/windows have been prepared yet
    if( oversized_height == 0 || oversized_width == 0 ) {
        return;
    }
    if( !g || win == nullptr ) {
        return;
    }
    if( win == g->w_overmap || win == g->w_terrain ) {
        return;
    }

    // track the dimensions for conversion
    const point termpixel( win->pos.x * font->width, win->pos.y * font->height );
    const int termpixel_x2 = termpixel.x + win->width * font->width - 1;
    const int termpixel_y2 = termpixel.y + win->height * font->height - 1;

    if( map_font != nullptr && map_font->width != 0 && map_font->height != 0 ) {
        const int mapfont_x = termpixel.x / map_font->width;
        const int mapfont_y = termpixel.y / map_font->height;
        const int mapfont_x2 = std::min( termpixel_x2 / map_font->width, oversized_width - 1 );
        const int mapfont_y2 = std::min( termpixel_y2 / map_font->height, oversized_height - 1 );
        const int mapfont_width = mapfont_x2 - mapfont_x + 1;
        const int mapfont_height = mapfont_y2 - mapfont_y + 1;
        invalidate_framebuffer( oversized_framebuffer, point( mapfont_x, mapfont_y ), mapfont_width,
                                mapfont_height );
    }

    if( overmap_font != nullptr && overmap_font->width != 0 && overmap_font->height != 0 ) {
        const int overmapfont_x = termpixel.x / overmap_font->width;
        const int overmapfont_y = termpixel.y / overmap_font->height;
        const int overmapfont_x2 = std::min( termpixel_x2 / overmap_font->width, oversized_width - 1 );
        const int overmapfont_y2 = std::min( termpixel_y2 / overmap_font->height,
                                             oversized_height - 1 );
        const int overmapfont_width = overmapfont_x2 - overmapfont_x + 1;
        const int overmapfont_height = overmapfont_y2 - overmapfont_y + 1;
        invalidate_framebuffer( oversized_framebuffer, point( overmapfont_x, overmapfont_y ),
                                overmapfont_width,
                                overmapfont_height );
    }
}

// clear the framebuffer when werase is called on certain windows that don't use the main terminal font
void cata_cursesport::handle_additional_window_clear( WINDOW *win )
{
    if( !g ) {
        return;
    }
    if( win == g->w_terrain || win == g->w_overmap ) {
        invalidate_framebuffer( oversized_framebuffer );
    }
}

void clear_window_area( const catacurses::window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    geometry->rect( renderer, point( win->pos.x * fontwidth, win->pos.y * fontheight ),
                    win->width * fontwidth, win->height * fontheight, color_as_sdl( catacurses::black ) );
}

static bool draw_window( Font_Ptr &font, const catacurses::window &w, const point &offset )
{
    if( scaling_factor > 1 ) {
        SDL_RenderSetLogicalSize( renderer.get(), WindowWidth / scaling_factor,
                                  WindowHeight / scaling_factor );
    }

    cata_cursesport::WINDOW *const win = w.get<cata_cursesport::WINDOW>();
    //Keeping track of the last drawn window
    const cata_cursesport::WINDOW *winBuffer = static_cast<cata_cursesport::WINDOW *>
            ( ::winBuffer.lock().get() );
    if( !fontScaleBuffer ) {
        fontScaleBuffer = tilecontext->get_tile_width();
    }
    const int fontScale = tilecontext->get_tile_width();
    //This creates a problem when map_font is different from the regular font
    //Specifically when showing the overmap
    //And in some instances of screen change, i.e. inventory.
    bool oldWinCompatible = false;

    // clear the oversized buffer proportionally
    invalidate_framebuffer_proportion( win );

    // use the oversize buffer when dealing with windows that can have a different font than the main text font
    bool use_oversized_framebuffer = g && ( w == g->w_terrain || w == g->w_overmap );

    std::vector<curseline> &framebuffer = use_oversized_framebuffer ? oversized_framebuffer :
                                          terminal_framebuffer;

    /*
    Let's try to keep track of different windows.
    A number of windows are coexisting on the screen, so don't have to interfere.

    g->w_terrain, g->w_minimap, g->w_HP, g->w_status, g->w_status2, g->w_messages,
     g->w_location, and g->w_minimap, can be buffered if either of them was
     the previous window.

    g->w_overmap and g->w_omlegend are likewise.

    Everything else works on strict equality because there aren't yet IDs for some of them.
    */
    if( g && ( w == g->w_terrain || w == g->w_minimap ) ) {
        if( winBuffer == g->w_terrain || winBuffer == g->w_minimap ) {
            oldWinCompatible = true;
        }
    } else if( g && ( w == g->w_overmap || w == g->w_omlegend ) ) {
        if( winBuffer == g->w_overmap || winBuffer == g->w_omlegend ) {
            oldWinCompatible = true;
        }
    } else {
        if( win == winBuffer ) {
            oldWinCompatible = true;
        }
    }

    // TODO: Get this from UTF system to make sure it is exactly the kind of space we need
    static const std::string space_string = " ";

    bool update = false;
    for( int j = 0; j < win->height; j++ ) {
        if( !win->line[j].touched ) {
            continue;
        }

        const int fby = win->pos.y + j;
        if( fby >= static_cast<int>( framebuffer.size() ) ) {
            // prevent indexing outside the frame buffer. This might happen for some parts of the window. FIX #28953.
            break;
        }

        update = true;
        win->line[j].touched = false;
        for( int i = 0; i < win->width; i++ ) {
            const int fbx = win->pos.x + i;
            if( fbx >= static_cast<int>( framebuffer[fby].chars.size() ) ) {
                // prevent indexing outside the frame buffer. This might happen for some parts of the window.
                break;
            }

            const cursecell &cell = win->line[j].chars[i];

            const int drawx = offset.x + i * font->width;
            const int drawy = offset.y + j * font->height;
            if( drawx + font->width > WindowWidth || drawy + font->height > WindowHeight ) {
                // Outside of the display area, would not render anyway
                continue;
            }

            // Avoid redrawing an unchanged tile by checking the framebuffer cache
            // TODO: handle caching when drawing normal windows over graphical tiles
            cursecell &oldcell = framebuffer[fby].chars[fbx];

            if( oldWinCompatible && cell == oldcell && fontScale == fontScaleBuffer ) {
                continue;
            }
            oldcell = cell;

            if( cell.ch.empty() ) {
                continue; // second cell of a multi-cell character
            }

            // Spaces are used a lot, so this does help noticeably
            if( cell.ch == space_string ) {
                geometry->rect( renderer, point( drawx, drawy ), font->width, font->height,
                                color_as_sdl( cell.BG ) );
                continue;
            }
            const int codepoint = UTF8_getch( cell.ch );
            const catacurses::base_color FG = cell.FG;
            const catacurses::base_color BG = cell.BG;
            int cw = ( codepoint == UNKNOWN_UNICODE ) ? 1 : utf8_width( cell.ch );
            if( cw < 1 ) {
                // utf8_width() may return a negative width
                continue;
            }
            bool use_draw_ascii_lines_routine = get_option<bool>( "USE_DRAW_ASCII_LINES_ROUTINE" );
            unsigned char uc = static_cast<unsigned char>( cell.ch[0] );
            switch( codepoint ) {
                case LINE_XOXO_UNICODE:
                    uc = LINE_XOXO_C;
                    break;
                case LINE_OXOX_UNICODE:
                    uc = LINE_OXOX_C;
                    break;
                case LINE_XXOO_UNICODE:
                    uc = LINE_XXOO_C;
                    break;
                case LINE_OXXO_UNICODE:
                    uc = LINE_OXXO_C;
                    break;
                case LINE_OOXX_UNICODE:
                    uc = LINE_OOXX_C;
                    break;
                case LINE_XOOX_UNICODE:
                    uc = LINE_XOOX_C;
                    break;
                case LINE_XXXO_UNICODE:
                    uc = LINE_XXXO_C;
                    break;
                case LINE_XXOX_UNICODE:
                    uc = LINE_XXOX_C;
                    break;
                case LINE_XOXX_UNICODE:
                    uc = LINE_XOXX_C;
                    break;
                case LINE_OXXX_UNICODE:
                    uc = LINE_OXXX_C;
                    break;
                case LINE_XXXX_UNICODE:
                    uc = LINE_XXXX_C;
                    break;
                case UNKNOWN_UNICODE:
                    use_draw_ascii_lines_routine = true;
                    break;
                default:
                    use_draw_ascii_lines_routine = false;
                    break;
            }
            geometry->rect( renderer, point( drawx, drawy ), font->width * cw, font->height,
                            color_as_sdl( BG ) );
            if( use_draw_ascii_lines_routine ) {
                font->draw_ascii_lines( renderer, geometry, uc, point( drawx, drawy ), FG );
            } else {
                font->OutputChar( renderer, geometry, cell.ch, point( drawx, drawy ), FG );
            }
        }
    }
    win->draw = false; //We drew the window, mark it as so
    //Keeping track of last drawn window and tilemode zoom level
    ::winBuffer = w.weak_ptr();
    fontScaleBuffer = tilecontext->get_tile_width();

    return update;
}

static bool draw_window( Font_Ptr &font, const catacurses::window &w )
{
    cata_cursesport::WINDOW *const win = w.get<cata_cursesport::WINDOW>();
    // Use global font sizes here to make this independent of the
    // font used for this window.
    return draw_window( font, w, point( win->pos.x * ::fontwidth, win->pos.y * ::fontheight ) );
}

void cata_cursesport::curses_drawwindow( const catacurses::window &w )
{
    if( scaling_factor > 1 ) {
        SDL_RenderSetLogicalSize( renderer.get(), WindowWidth / scaling_factor,
                                  WindowHeight / scaling_factor );
    }
    WINDOW *const win = w.get<WINDOW>();
    bool update = false;
    if( g && w == g->w_terrain && use_tiles ) {
        // color blocks overlay; drawn on top of tiles and on top of overlay strings (if any).
        color_block_overlay_container color_blocks;

        // Strings with colors do be drawn with map_font on top of tiles.
        std::multimap<point, formatted_text> overlay_strings;

        // game::w_terrain can be drawn by the tilecontext.
        // skip the normal drawing code for it.
        tilecontext->draw(
            point( win->pos.x * fontwidth, win->pos.y * fontheight ),
            g->ter_view_p,
            TERRAIN_WINDOW_TERM_WIDTH * font->width,
            TERRAIN_WINDOW_TERM_HEIGHT * font->height,
            overlay_strings,
            color_blocks );

        // color blocks overlay
        if( !color_blocks.second.empty() ) {
            SDL_BlendMode blend_mode;
            GetRenderDrawBlendMode( renderer, blend_mode ); // save the current blend mode
            SetRenderDrawBlendMode( renderer, color_blocks.first ); // set the new blend mode
            for( const auto &e : color_blocks.second ) {
                geometry->rect( renderer, e.first, tilecontext->get_tile_width(),
                                tilecontext->get_tile_height(), e.second );
            }
            SetRenderDrawBlendMode( renderer, blend_mode ); // set the old blend mode
        }

        // overlay strings
        point prev_coord;
        int x_offset = 0;
        int alignment_offset = 0;
        for( const auto &iter : overlay_strings ) {
            const point coord = iter.first;
            const formatted_text ft = iter.second;
            const utf8_wrapper text( ft.text );

            // Strings at equal coords are displayed sequentially.
            if( coord != prev_coord ) {
                x_offset = 0;
            }

            // Calculate length of all strings in sequence to align them.
            if( x_offset == 0 ) {
                int full_text_length = 0;
                const auto range = overlay_strings.equal_range( coord );
                for( auto ri = range.first; ri != range.second; ++ri ) {
                    utf8_wrapper rt( ri->second.text );
                    full_text_length += rt.display_width();
                }

                alignment_offset = 0;
                if( ft.alignment == text_alignment::center ) {
                    alignment_offset = full_text_length / 2;
                } else if( ft.alignment == text_alignment::right ) {
                    alignment_offset = full_text_length - 1;
                }
            }

            int width = 0;
            for( size_t i = 0; i < text.size(); ++i ) {
                const int x0 = win->pos.x * fontwidth;
                const int y0 = win->pos.y * fontheight;
                const int x = x0 + ( x_offset - alignment_offset + width ) * map_font->width + coord.x;
                const int y = y0 + coord.y;

                // Clip to window bounds.
                if( x < x0 || x > x0 + ( TERRAIN_WINDOW_TERM_WIDTH - 1 ) * font->width
                    || y < y0 || y > y0 + ( TERRAIN_WINDOW_TERM_HEIGHT - 1 ) * font->height ) {
                    continue;
                }

                // TODO: draw with outline / BG color for better readability
                const uint32_t ch = text.at( i );
                map_font->OutputChar( renderer, geometry, utf32_to_utf8( ch ), point( x, y ), ft.color );
                width += mk_wcwidth( ch );
            }

            prev_coord = coord;
            x_offset = width;
        }

        invalidate_framebuffer( terminal_framebuffer, win->pos,
                                TERRAIN_WINDOW_TERM_WIDTH, TERRAIN_WINDOW_TERM_HEIGHT );

        update = true;
    } else if( g && w == g->w_terrain && map_font ) {
        // When the terrain updates, predraw a black space around its edge
        // to keep various former interface elements from showing through the gaps

        //calculate width differences between map_font and font
        int partial_width = std::max( TERRAIN_WINDOW_TERM_WIDTH * fontwidth - TERRAIN_WINDOW_WIDTH *
                                      map_font->width, 0 );
        int partial_height = std::max( TERRAIN_WINDOW_TERM_HEIGHT * fontheight - TERRAIN_WINDOW_HEIGHT *
                                       map_font->height, 0 );
        //Gap between terrain and lower window edge
        if( partial_height > 0 ) {
            geometry->rect( renderer, point( win->pos.x * map_font->width,
                                             ( win->pos.y + TERRAIN_WINDOW_HEIGHT ) * map_font->height ),
                            TERRAIN_WINDOW_WIDTH * map_font->width + partial_width, partial_height,
                            color_as_sdl( catacurses::black ) );
        }
        //Gap between terrain and sidebar
        if( partial_width > 0 ) {
            geometry->rect( renderer, point( ( win->pos.x + TERRAIN_WINDOW_WIDTH ) * map_font->width,
                                             win->pos.y * map_font->height ),
                            partial_width,
                            TERRAIN_WINDOW_HEIGHT * map_font->height + partial_height,
                            color_as_sdl( catacurses::black ) );
        }
        // Special font for the terrain window
        update = draw_window( map_font, w );
    } else if( g && w == g->w_overmap && overmap_font ) {
        // Special font for the terrain window
        update = draw_window( overmap_font, w );
    } else if( g && w == g->w_pixel_minimap && g->pixel_minimap_option ) {
        // ensure the space the minimap covers is "dirtied".
        // this is necessary when it's the only part of the sidebar being drawn
        // TODO: Figure out how to properly make the minimap code do whatever it is this does
        draw_window( font, w );

        // Make sure the entire minimap window is black before drawing.
        clear_window_area( w );
        tilecontext->draw_minimap(
            point( win->pos.x * fontwidth, win->pos.y * fontheight ),
            tripoint( g->u.pos().xy(), g->ter_view_p.z ),
            win->width * font->width, win->height * font->height );
        update = true;

    } else {
        // Either not using tiles (tilecontext) or not the w_terrain window.
        update = draw_window( font, w );
    }
    if( update ) {
        needupdate = true;
    }
}

static int alt_buffer = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer = 0;
    alt_down = true;
}

static bool add_alt_code( char c )
{
    if( alt_down && c >= '0' && c <= '9' ) {
        alt_buffer = alt_buffer * 10 + ( c - '0' );
        return true;
    }
    return false;
}

static int end_alt_code()
{
    alt_down = false;
    return alt_buffer;
}

static int HandleDPad()
{
    // Check if we have a gamepad d-pad event.
    if( SDL_JoystickGetHat( joystick, 0 ) != SDL_HAT_CENTERED ) {
        // When someone tries to press a diagonal, they likely will
        // press a single direction first. Wait a few milliseconds to
        // give them time to press both of the buttons for the diagonal.
        int button = SDL_JoystickGetHat( joystick, 0 );
        int lc = ERR;
        if( button == SDL_HAT_LEFT ) {
            lc = JOY_LEFT;
        } else if( button == SDL_HAT_DOWN ) {
            lc = JOY_DOWN;
        } else if( button == SDL_HAT_RIGHT ) {
            lc = JOY_RIGHT;
        } else if( button == SDL_HAT_UP ) {
            lc = JOY_UP;
        } else if( button == SDL_HAT_LEFTUP ) {
            lc = JOY_LEFTUP;
        } else if( button == SDL_HAT_LEFTDOWN ) {
            lc = JOY_LEFTDOWN;
        } else if( button == SDL_HAT_RIGHTUP ) {
            lc = JOY_RIGHTUP;
        } else if( button == SDL_HAT_RIGHTDOWN ) {
            lc = JOY_RIGHTDOWN;
        }

        if( delaydpad == std::numeric_limits<Uint32>::max() ) {
            delaydpad = SDL_GetTicks() + dpad_delay;
            queued_dpad = lc;
        }

        // Okay it seems we're ready to process.
        if( SDL_GetTicks() > delaydpad ) {

            if( lc != ERR ) {
                if( dpad_continuous && ( lc & lastdpad ) == 0 ) {
                    // Continuous movement should only work in the same or similar directions.
                    dpad_continuous = false;
                    lastdpad = lc;
                    return 0;
                }

                last_input = input_event( lc, CATA_INPUT_GAMEPAD );
                lastdpad = lc;
                queued_dpad = ERR;

                if( !dpad_continuous ) {
                    delaydpad = SDL_GetTicks() + 200;
                    dpad_continuous = true;
                } else {
                    delaydpad = SDL_GetTicks() + 60;
                }
                return 1;
            }
        }
    } else {
        dpad_continuous = false;
        delaydpad = std::numeric_limits<Uint32>::max();

        // If we didn't hold it down for a while, just
        // fire the last registered press.
        if( queued_dpad != ERR ) {
            last_input = input_event( queued_dpad, CATA_INPUT_GAMEPAD );
            queued_dpad = ERR;
            return 1;
        }
    }

    return 0;
}

static SDL_Keycode sdl_keycode_opposite_arrow( SDL_Keycode key )
{
    switch( key ) {
        case SDLK_UP:
            return SDLK_DOWN;
        case SDLK_DOWN:
            return SDLK_UP;
        case SDLK_LEFT:
            return SDLK_RIGHT;
        case SDLK_RIGHT:
            return SDLK_LEFT;
    }
    return 0;
}

static bool sdl_keycode_is_arrow( SDL_Keycode key )
{
    return static_cast<bool>( sdl_keycode_opposite_arrow( key ) );
}

static int arrow_combo_to_numpad( SDL_Keycode mod, SDL_Keycode key )
{
    if( ( mod == SDLK_UP    && key == SDLK_RIGHT ) ||
        ( mod == SDLK_RIGHT && key == SDLK_UP ) ) {
        return KEY_NUM( 9 );
    }
    if( ( mod == SDLK_UP    && key == SDLK_UP ) ) {
        return KEY_NUM( 8 );
    }
    if( ( mod == SDLK_UP    && key == SDLK_LEFT ) ||
        ( mod == SDLK_LEFT  && key == SDLK_UP ) ) {
        return KEY_NUM( 7 );
    }
    if( ( mod == SDLK_RIGHT && key == SDLK_RIGHT ) ) {
        return KEY_NUM( 6 );
    }
    if( mod == sdl_keycode_opposite_arrow( key ) ) {
        return KEY_NUM( 5 );
    }
    if( ( mod == SDLK_LEFT  && key == SDLK_LEFT ) ) {
        return KEY_NUM( 4 );
    }
    if( ( mod == SDLK_DOWN  && key == SDLK_RIGHT ) ||
        ( mod == SDLK_RIGHT && key == SDLK_DOWN ) ) {
        return KEY_NUM( 3 );
    }
    if( ( mod == SDLK_DOWN  && key == SDLK_DOWN ) ) {
        return KEY_NUM( 2 );
    }
    if( ( mod == SDLK_DOWN  && key == SDLK_LEFT ) ||
        ( mod == SDLK_LEFT  && key == SDLK_DOWN ) ) {
        return KEY_NUM( 1 );
    }
    return 0;
}

static int arrow_combo_modifier = 0;

static int handle_arrow_combo( SDL_Keycode key )
{
    if( !arrow_combo_modifier ) {
        arrow_combo_modifier = key;
        return 0;
    }
    return arrow_combo_to_numpad( arrow_combo_modifier, key );
}

static void end_arrow_combo()
{
    arrow_combo_modifier = 0;
}

/**
 * Translate SDL key codes to key identifiers used by ncurses, this
 * allows the input_manager to only consider those.
 * @return 0 if the input can not be translated (unknown key?),
 * -1 when a ALT+number sequence has been started,
 * or something that a call to ncurses getch would return.
 */
static int sdl_keysym_to_curses( const SDL_Keysym &keysym )
{

    const std::string diag_mode = get_option<std::string>( "DIAG_MOVE_WITH_MODIFIERS_MODE" );

    if( diag_mode == "mode1" ) {
        if( keysym.mod & KMOD_CTRL && sdl_keycode_is_arrow( keysym.sym ) ) {
            return handle_arrow_combo( keysym.sym );
        } else {
            end_arrow_combo();
        }
    }

    if( diag_mode == "mode2" ) {
        //Shift + Cursor Arrow (diagonal clockwise)
        if( keysym.mod & KMOD_SHIFT ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
            }
        }
        //Ctrl + Cursor Arrow (diagonal counter-clockwise)
        if( keysym.mod & KMOD_CTRL ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
            }
        }
    }

    if( diag_mode == "mode3" ) {
        //Shift + Cursor Left/RightArrow
        if( keysym.mod & KMOD_SHIFT ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
            }
        }
        //Ctrl + Cursor Left/Right Arrow
        if( keysym.mod & KMOD_CTRL ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
            }
        }
    }

    if( keysym.mod & KMOD_CTRL && keysym.sym >= 'a' && keysym.sym <= 'z' ) {
        // ASCII ctrl codes, ^A through ^Z.
        return keysym.sym - 'a' + '\1';
    }
    switch( keysym.sym ) {
        // This is special: allow entering a Unicode character with ALT+number
        case SDLK_RALT:
        case SDLK_LALT:
            begin_alt_code();
            return -1;
        // The following are simple translations:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_RETURN2:
            return '\n';
        case SDLK_BACKSPACE:
        case SDLK_KP_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_DELETE:
            return KEY_DC;
        case SDLK_ESCAPE:
            return KEY_ESCAPE;
        case SDLK_TAB:
            if( keysym.mod & KMOD_SHIFT ) {
                return KEY_BTAB;
            }
            return '\t';
        case SDLK_LEFT:
            return KEY_LEFT;
        case SDLK_RIGHT:
            return KEY_RIGHT;
        case SDLK_UP:
            return KEY_UP;
        case SDLK_DOWN:
            return KEY_DOWN;
        case SDLK_PAGEUP:
            return KEY_PPAGE;
        case SDLK_PAGEDOWN:
            return KEY_NPAGE;
        case SDLK_HOME:
            return KEY_HOME;
        case SDLK_END:
            return KEY_END;
        case SDLK_F1:
            return KEY_F( 1 );
        case SDLK_F2:
            return KEY_F( 2 );
        case SDLK_F3:
            return KEY_F( 3 );
        case SDLK_F4:
            return KEY_F( 4 );
        case SDLK_F5:
            return KEY_F( 5 );
        case SDLK_F6:
            return KEY_F( 6 );
        case SDLK_F7:
            return KEY_F( 7 );
        case SDLK_F8:
            return KEY_F( 8 );
        case SDLK_F9:
            return KEY_F( 9 );
        case SDLK_F10:
            return KEY_F( 10 );
        case SDLK_F11:
            return KEY_F( 11 );
        case SDLK_F12:
            return KEY_F( 12 );
        case SDLK_F13:
            return KEY_F( 13 );
        case SDLK_F14:
            return KEY_F( 14 );
        case SDLK_F15:
            return KEY_F( 15 );
        // Every other key is ignored as there is no curses constant for it.
        // TODO: add more if you find more.
        default:
            return 0;
    }
}

bool handle_resize( int w, int h )
{
    if( ( w != WindowWidth ) || ( h != WindowHeight ) ) {
        WindowWidth = w;
        WindowHeight = h;
        TERMINAL_WIDTH = WindowWidth / fontwidth / scaling_factor;
        TERMINAL_HEIGHT = WindowHeight / fontheight / scaling_factor;
        catacurses::stdscr = catacurses::newwin( TERMINAL_HEIGHT, TERMINAL_WIDTH, point_zero );
        SetupRenderTarget();
        game_ui::init_ui();
        ui_manager::screen_resized();
        return true;
    }
    return false;
}

void resize_term( const int cell_w, const int cell_h )
{
    int w = cell_w * fontwidth * scaling_factor;
    int h = cell_h * fontheight * scaling_factor;
    SDL_SetWindowSize( window.get(), w, h );
    SDL_GetWindowSize( window.get(), &w, &h );
    handle_resize( w, h );
}

void toggle_fullscreen_window()
{
    static int restore_win_w = get_option<int>( "TERMINAL_X" ) * fontwidth * scaling_factor;
    static int restore_win_h = get_option<int>( "TERMINAL_Y" ) * fontheight * scaling_factor;

    if( fullscreen ) {
        if( printErrorIf( SDL_SetWindowFullscreen( window.get(), 0 ) != 0,
                          "SDL_SetWindowFullscreen failed" ) ) {
            return;
        }
        SDL_RestoreWindow( window.get() );
        SDL_SetWindowSize( window.get(), restore_win_w, restore_win_h );
        SDL_SetWindowMinimumSize( window.get(), fontwidth * FULL_SCREEN_WIDTH * scaling_factor,
                                  fontheight * FULL_SCREEN_HEIGHT * scaling_factor );
    } else {
        restore_win_w = WindowWidth;
        restore_win_h = WindowHeight;
        if( printErrorIf( SDL_SetWindowFullscreen( window.get(), SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0,
                          "SDL_SetWindowFullscreen failed" ) ) {
            return;
        }
    }
    int nw = 0;
    int nh = 0;
    SDL_GetWindowSize( window.get(), &nw, &nh );
    handle_resize( nw, nh );
    fullscreen = !fullscreen;
}

#if defined(__ANDROID__)
static float finger_down_x = -1.0f; // in pixels
static float finger_down_y = -1.0f; // in pixels
static float finger_curr_x = -1.0f; // in pixels
static float finger_curr_y = -1.0f; // in pixels
static float second_finger_down_x = -1.0f; // in pixels
static float second_finger_down_y = -1.0f; // in pixels
static float second_finger_curr_x = -1.0f; // in pixels
static float second_finger_curr_y = -1.0f; // in pixels
// when did the first finger start touching the screen? 0 if not touching, otherwise the time in milliseconds.
static uint32_t finger_down_time = 0;
// the last time we repeated input for a finger hold, 0 if not touching, otherwise the time in milliseconds.
static uint32_t finger_repeat_time = 0;
// the last time a single tap was detected. used for double-tap detection.
static uint32_t last_tap_time = 0;
// when did the hardware back button start being pressed? 0 if not touching, otherwise the time in milliseconds.
static uint32_t ac_back_down_time = 0;
// has a second finger touched the screen while the first was touching?
static bool is_two_finger_touch = false;
// did this touch start on a quick shortcut?
static bool is_quick_shortcut_touch = false;
static bool quick_shortcuts_toggle_handled = false;
// the current finger repeat delay - will be somewhere between the min/max values depending on user input
uint32_t finger_repeat_delay = 500;
// should we make sure the sdl surface is visible? set to true whenever the SDL window is shown.
static bool needs_sdl_surface_visibility_refresh = true;

// Quick shortcuts container: maps the touch input context category (std::string) to a std::list of input_events.
using quick_shortcuts_t = std::list<input_event>;
std::map<std::string, quick_shortcuts_t> quick_shortcuts_map;

// A copy of the last known input_context from the input manager. It's important this is a copy, as there are times
// the input manager has an empty input_context (eg. when player is moving over slow objects) and we don't want our
// quick shortcuts to disappear momentarily.
input_context touch_input_context;

std::string get_quick_shortcut_name( const std::string &category )
{
    if( category == "DEFAULTMODE" && g->check_zone( zone_type_id( "NO_AUTO_PICKUP" ), g->u.pos() ) &&
        get_option<bool>( "ANDROID_SHORTCUT_ZONE" ) ) {
        return "DEFAULTMODE____SHORTCUTS";
    }
    return category;
}

float android_get_display_density()
{
    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
    jobject activity = ( jobject )SDL_AndroidGetActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "getDisplayDensity", "()F" );
    jfloat ans = env->CallFloatMethod( activity, method_id );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    return ans;
}

// given the active quick shortcuts, returns the dimensions of each quick shortcut button.
void get_quick_shortcut_dimensions( quick_shortcuts_t &qsl, float &border, float &width,
                                    float &height )
{
    const float shortcut_dimensions_authored_density = 3.0f; // 480p xxhdpi
    float screen_density_scale = android_get_display_density() / shortcut_dimensions_authored_density;
    border = std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_BORDER" ) );
    width = std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_WIDTH_MAX" ) );
    float min_width = std::floor( screen_density_scale * std::min(
                                      get_option<int>( "ANDROID_SHORTCUT_WIDTH_MIN" ),
                                      get_option<int>( "ANDROID_SHORTCUT_WIDTH_MAX" ) ) );
    float usable_window_width = WindowWidth * get_option<int>( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE" ) *
                                0.01f;
    if( width * qsl.size() > usable_window_width ) {
        width *= usable_window_width / ( width * qsl.size() );
        if( width < min_width ) {
            width = min_width;
        }
    }
    width = std::floor( width );
    height = std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_HEIGHT" ) );
}

// Returns the quick shortcut (if any) under the finger's current position, or finger down position if down == true
input_event *get_quick_shortcut_under_finger( bool down = false )
{

    if( !quick_shortcuts_enabled ) {
        return NULL;
    }

    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                 touch_input_context.get_category() )];

    float border, width, height;
    get_quick_shortcut_dimensions( qsl, border, width, height );

    float finger_y = down ? finger_down_y : finger_curr_y;
    if( finger_y < WindowHeight - height ) {
        return NULL;
    }

    int i = 0;
    bool shortcut_right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    float finger_x = down ? finger_down_x : finger_curr_x;
    for( std::list<input_event>::iterator it = qsl.begin(); it != qsl.end(); ++it ) {
        if( ( i + 1 ) * width > WindowWidth * get_option<int>( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE" ) *
            0.01f ) {
            continue;
        }
        i++;
        if( shortcut_right ) {
            if( finger_x > WindowWidth - ( i * width ) ) {
                return &( *it );
            }
        } else {
            if( finger_x < i * width ) {
                return &( *it );
            }
        }
    }

    return NULL;
}

// when pre-populating a quick shortcut list with defaults, ignore these actions (since they're all handleable by native touch operations)
bool ignore_action_for_quick_shortcuts( const std::string &action )
{
    return ( action == "UP"
             || action == "DOWN"
             || action == "LEFT"
             || action == "RIGHT"
             || action == "LEFTUP"
             || action == "LEFTDOWN"
             || action == "RIGHTUP"
             || action == "RIGHTDOWN"
             || action == "QUIT"
             || action == "CONFIRM"
             || action == "MOVE_SINGLE_ITEM" // maps to ENTER
             || action == "MOVE_ARMOR" // maps to ENTER
             || action == "ANY_INPUT"
             || action ==
             "DELETE_TEMPLATE" // strictly we shouldn't have this one, but I don't like seeing the "d" on the main menu by default. :)
           );
}

// Adds a quick shortcut to a quick_shortcut list, setting shortcut_last_used_action_counter accordingly.
void add_quick_shortcut( quick_shortcuts_t &qsl, input_event &event, bool back,
                         bool reset_shortcut_last_used_action_counter )
{
    if( reset_shortcut_last_used_action_counter ) {
        event.shortcut_last_used_action_counter =
            g->get_user_action_counter();    // only used for DEFAULTMODE
    }
    if( back ) {
        qsl.push_back( event );
    } else {
        qsl.push_front( event );
    }
}

// Given a quick shortcut list and a specific key, move that key to the front or back of the list.
void reorder_quick_shortcut( quick_shortcuts_t &qsl, int key, bool back )
{
    for( const auto &event : qsl ) {
        if( event.get_first_input() == key ) {
            input_event event_copy = event;
            qsl.remove( event );
            add_quick_shortcut( qsl, event_copy, back, false );
            break;
        }
    }
}

void reorder_quick_shortcuts( quick_shortcuts_t &qsl )
{
    // Do some manual reordering to make transitions between input contexts more consistent
    // Desired order of keys: < > BACKTAB TAB PPAGE NPAGE . . . . ?
    bool shortcut_right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    if( shortcut_right ) {
        reorder_quick_shortcut( qsl, KEY_PPAGE, false ); // paging control
        reorder_quick_shortcut( qsl, KEY_NPAGE, false );
        reorder_quick_shortcut( qsl, KEY_BTAB, false ); // secondary tabs after that
        reorder_quick_shortcut( qsl, '\t', false );
        reorder_quick_shortcut( qsl, '<', false ); // tabs next
        reorder_quick_shortcut( qsl, '>', false );
        reorder_quick_shortcut( qsl, '?', false ); // help at the start
    } else {
        reorder_quick_shortcut( qsl, KEY_NPAGE, false );
        reorder_quick_shortcut( qsl, KEY_PPAGE, false ); // paging control
        reorder_quick_shortcut( qsl, '\t', false );
        reorder_quick_shortcut( qsl, KEY_BTAB, false ); // secondary tabs after that
        reorder_quick_shortcut( qsl, '>', false );
        reorder_quick_shortcut( qsl, '<', false ); // tabs next
        reorder_quick_shortcut( qsl, '?', false ); // help at the start
    }
}

int choose_best_key_for_action( const std::string &action, const std::string &category )
{
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action, category );
    int best_key = -1;
    for( const auto &events_event : events ) {
        if( events_event.type == CATA_INPUT_KEYBOARD && events_event.sequence.size() == 1 ) {
            bool is_ascii_char = isprint( events_event.sequence.front() ) &&
                                 events_event.sequence.front() < 0xFF;
            bool is_best_ascii_char = best_key >= 0 && isprint( best_key ) && best_key < 0xFF;
            if( best_key < 0 || ( is_ascii_char && !is_best_ascii_char ) ) {
                best_key = events_event.sequence.front();
            }
        }
    }
    return best_key;
}

bool add_key_to_quick_shortcuts( int key, const std::string &category, bool back )
{
    if( key > 0 ) {
        quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
        input_event event = input_event( key, CATA_INPUT_KEYBOARD );
        quick_shortcuts_t::iterator it = std::find( qsl.begin(), qsl.end(), event );
        if( it != qsl.end() ) { // already exists
            ( *it ).shortcut_last_used_action_counter =
                g->get_user_action_counter(); // make sure we refresh shortcut usage
        } else {
            add_quick_shortcut( qsl, event, back,
                                true ); // doesn't exist, add it to the shortcuts and refresh shortcut usage
            return true;
        }
    }
    return false;
}
bool add_best_key_for_action_to_quick_shortcuts( std::string action_str,
        const std::string &category, bool back )
{
    int best_key = choose_best_key_for_action( action_str, category );
    return add_key_to_quick_shortcuts( best_key, category, back );
}

bool add_best_key_for_action_to_quick_shortcuts( action_id action, const std::string &category,
        bool back )
{
    return add_best_key_for_action_to_quick_shortcuts( action_ident( action ), category, back );
}

void remove_action_from_quick_shortcuts( std::string action_str, const std::string &category )
{
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_str, category );
    for( const auto &event : events ) {
        qsl.remove( event );
    }
}

void remove_action_from_quick_shortcuts( action_id action, const std::string &category )
{
    remove_action_from_quick_shortcuts( action_ident( action ), category );
}

// Returns true if an expired action was removed
bool remove_expired_actions_from_quick_shortcuts( const std::string &category )
{
    int remove_turns = get_option<int>( "ANDROID_SHORTCUT_REMOVE_TURNS" );
    if( remove_turns <= 0 ) {
        return false;
    }

    // This should only ever be used on "DEFAULTMODE" category for gameplay shortcuts
    if( category != "DEFAULTMODE" ) {
        return false;
    }

    bool ret = false;
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    quick_shortcuts_t::iterator it = qsl.begin();
    while( it != qsl.end() ) {
        if( g->get_user_action_counter() - ( *it ).shortcut_last_used_action_counter > remove_turns ) {
            it = qsl.erase( it );
            ret = true;
        } else {
            ++it;
        }
    }
    return ret;
}

void remove_stale_inventory_quick_shortcuts()
{
    if( get_option<bool>( "ANDROID_INVENTORY_AUTOADD" ) ) {
        quick_shortcuts_t &qsl = quick_shortcuts_map["INVENTORY"];
        quick_shortcuts_t::iterator it = qsl.begin();
        bool in_inventory;
        int key;
        bool valid;
        while( it != qsl.end() ) {
            key = ( *it ).get_first_input();
            valid = inv_chars.valid( key );
            in_inventory = false;
            if( valid ) {
                in_inventory = g->u.inv.invlet_to_position( key ) != INT_MIN;
                if( !in_inventory ) {
                    // We couldn't find this item in the inventory, let's check worn items
                    for( const auto &item : g->u.worn ) {
                        if( item.invlet == key ) {
                            in_inventory = true;
                            break;
                        }
                    }
                }
                if( !in_inventory ) {
                    // We couldn't find it in worn items either, check weapon held
                    if( g->u.weapon.invlet == key ) {
                        in_inventory = true;
                    }
                }
            }
            if( valid && !in_inventory ) {
                it = qsl.erase( it );
            } else {
                ++it;
            }
        }
    }
}

// Draw preview of terminal size when adjusting values
void draw_terminal_size_preview()
{
    bool preview_terminal_dirty = preview_terminal_width != get_option<int>( "TERMINAL_X" ) * fontwidth
                                  ||
                                  preview_terminal_height != get_option<int>( "TERMINAL_Y" ) * fontheight;
    if( preview_terminal_dirty ||
        ( preview_terminal_change_time > 0 && SDL_GetTicks() - preview_terminal_change_time < 1000 ) ) {
        if( preview_terminal_dirty ) {
            preview_terminal_width = get_option<int>( "TERMINAL_X" ) * fontwidth;
            preview_terminal_height = get_option<int>( "TERMINAL_Y" ) * fontheight;
            preview_terminal_change_time = SDL_GetTicks();
        }
        SetRenderDrawColor( renderer, 255, 255, 255, 255 );
        SDL_Rect previewrect = get_android_render_rect( preview_terminal_width, preview_terminal_height );
        SDL_RenderDrawRect( renderer.get(), &previewrect );
        SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    }
}

// Draw quick shortcuts on top of the game view
void draw_quick_shortcuts()
{

    if( !quick_shortcuts_enabled ||
        SDL_IsTextInputActive() ||
        ( get_option<bool>( "ANDROID_HIDE_HOLDS" ) && !is_quick_shortcut_touch && finger_down_time > 0 &&
          SDL_GetTicks() - finger_down_time >= static_cast<uint32_t>(
              get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) ) { // player is swipe + holding in a direction
        return;
    }

    bool shortcut_right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    std::string &category = touch_input_context.get_category();
    bool is_default_mode = category == "DEFAULTMODE";
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    if( qsl.empty() || !touch_input_context.get_registered_manual_keys().empty() ) {
        if( category == "DEFAULTMODE" ) {
            const std::string default_gameplay_shortcuts =
                get_option<std::string>( "ANDROID_SHORTCUT_DEFAULTS" );
            for( const auto &c : default_gameplay_shortcuts ) {
                add_key_to_quick_shortcuts( c, category, true );
            }
        } else {
            // This is an empty quick-shortcuts list, let's pre-populate it as best we can from the input context

            // For manual key lists, force-clear them each time since there's no point allowing custom bindings anyway
            if( !touch_input_context.get_registered_manual_keys().empty() ) {
                qsl.clear();
            }

            // First process registered actions
            std::vector<std::string> &registered_actions = touch_input_context.get_registered_actions();
            for( std::vector<std::string>::iterator it = registered_actions.begin();
                 it != registered_actions.end(); ++it ) {
                std::string &action = *it;
                if( ignore_action_for_quick_shortcuts( action ) ) {
                    continue;
                }

                add_best_key_for_action_to_quick_shortcuts( action, category, !shortcut_right );
            }

            // Then process manual keys
            std::vector<input_context::manual_key> &registered_manual_keys =
                touch_input_context.get_registered_manual_keys();
            for( const auto &manual_key : registered_manual_keys ) {
                input_event event( manual_key.key, CATA_INPUT_KEYBOARD );
                add_quick_shortcut( qsl, event, !shortcut_right, true );
            }
        }
    }

    // Only reorder quick shortcuts for non-gameplay lists that are likely to have navigational menu stuff
    if( !is_default_mode ) {
        reorder_quick_shortcuts( qsl );
    }

    float border, width, height;
    get_quick_shortcut_dimensions( qsl, border, width, height );
    input_event *hovered_quick_shortcut = get_quick_shortcut_under_finger();
    SDL_Rect rect;
    bool hovered, show_hint;
    int i = 0;
    for( std::list<input_event>::iterator it = qsl.begin(); it != qsl.end(); ++it ) {
        if( ( i + 1 ) * width > WindowWidth * get_option<int>( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE" ) *
            0.01f ) {
            continue;
        }
        input_event &event = *it;
        std::string text = event.text;
        int key = event.get_first_input();
        float default_text_scale = std::floor( 0.75f * ( height /
                                               font->height ) ); // default for single character strings
        float text_scale = default_text_scale;
        if( text.empty() || text == " " ) {
            text = inp_mngr.get_keyname( key, event.type );
            text_scale = std::min( text_scale, 0.75f * ( width / ( font->width * utf8_width( text ) ) ) );
        }
        hovered = is_quick_shortcut_touch && hovered_quick_shortcut == &event;
        show_hint = hovered &&
                    SDL_GetTicks() - finger_down_time > static_cast<uint32_t>
                    ( get_option<int>( "ANDROID_INITIAL_DELAY" ) );
        std::string hint_text;
        if( show_hint ) {
            if( touch_input_context.get_category() == "INVENTORY" && inv_chars.valid( key ) ) {
                // Special case for inventory items - show the inventory item name as help text
                hint_text = g->u.inv.find_item( g->u.inv.invlet_to_position( key ) ).display_name();
                if( hint_text == "none" ) {
                    // We couldn't find this item in the inventory, let's check worn items
                    for( const auto &item : g->u.worn ) {
                        if( item.invlet == key ) {
                            hint_text = item.display_name();
                            break;
                        }
                    }
                }
                if( hint_text == "none" ) {
                    // We couldn't find it in worn items either, must be weapon held
                    if( g->u.weapon.invlet == key ) {
                        hint_text = g->u.weapon.display_name();
                    }
                }
            } else {
                // All other screens - try and show the action name, either from registered actions or manually registered keys
                hint_text = touch_input_context.get_action_name( touch_input_context.input_to_action( event ) );
                if( hint_text == "ERROR" ) {
                    hint_text = touch_input_context.get_action_name_for_manual_key( key );
                }
            }
            if( hint_text == "ERROR" || hint_text == "none" || hint_text.empty() ) {
                show_hint = false;
            }
        }
        if( shortcut_right )
            rect = { WindowWidth - ( int )( ( i + 1 ) * width + border ), ( int )( WindowHeight - height ), ( int )( width - border * 2 ), ( int )( height ) };
        else
            rect = { ( int )( i * width + border ), ( int )( WindowHeight - height ), ( int )( width - border * 2 ), ( int )( height ) };
        if( hovered ) {
            SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        } else {
            SetRenderDrawColor( renderer, 0, 0, 0,
                                get_option<int>( "ANDROID_SHORTCUT_OPACITY_BG" ) * 0.01f * 255.0f );
        }
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        RenderFillRect( renderer, &rect );
        if( hovered ) {
            // draw a second button hovering above the first one
            if( shortcut_right )
                rect = { WindowWidth - ( int )( ( i + 1 ) * width + border ), ( int )( WindowHeight - height * 2.2f ), ( int )( width - border * 2 ), ( int )( height ) };
            else
                rect = { ( int )( i * width + border ), ( int )( WindowHeight - height * 2.2f ), ( int )( width - border * 2 ), ( int )( height ) };
            SetRenderDrawColor( renderer, 0, 0, 196, 255 );
            RenderFillRect( renderer, &rect );

            if( show_hint ) {
                // draw a backdrop for the hint text
                rect = { 0, ( int )( ( WindowHeight - height ) * 0.5f ), ( int )WindowWidth, ( int )height };
                SetRenderDrawColor( renderer, 0, 0, 0,
                                    get_option<int>( "ANDROID_SHORTCUT_OPACITY_BG" ) * 0.01f * 255.0f );
                RenderFillRect( renderer, &rect );
            }
        }
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
        SDL_RenderSetScale( renderer.get(), text_scale, text_scale );
        int text_x, text_y;
        if( shortcut_right ) {
            text_x = ( WindowWidth - ( i + 0.5f ) * width - ( font->width * utf8_width(
                           text ) ) * text_scale * 0.5f ) / text_scale;
        } else {
            text_x = ( ( i + 0.5f ) * width - ( font->width * utf8_width( text ) ) * text_scale * 0.5f ) /
                     text_scale;
        }
        text_y = ( WindowHeight - ( height + font->height * text_scale ) * 0.5f ) / text_scale;
        font->OutputChar( renderer, geometry, text, point( text_x + 1, text_y + 1 ), 0,
                          get_option<int>( "ANDROID_SHORTCUT_OPACITY_SHADOW" ) * 0.01f );
        font->OutputChar( renderer, geometry, text, point( text_x, text_y ),
                          get_option<int>( "ANDROID_SHORTCUT_COLOR" ),
                          get_option<int>( "ANDROID_SHORTCUT_OPACITY_FG" ) * 0.01f );
        if( hovered ) {
            // draw a second button hovering above the first one
            font->OutputChar( renderer, geometry, text,
                              point( text_x, text_y - ( height * 1.2f / text_scale ) ),
                              get_option<int>( "ANDROID_SHORTCUT_COLOR" ) );
            if( show_hint ) {
                // draw hint text
                text_scale = default_text_scale;
                hint_text = text + " " + hint_text;
                hint_text = remove_color_tags( hint_text );
                const float safe_margin = 0.9f;
                int hint_length = utf8_width( hint_text );
                if( WindowWidth * safe_margin < font->width * text_scale * hint_length ) {
                    text_scale *= ( WindowWidth * safe_margin ) / ( font->width * text_scale *
                                  hint_length );    // scale to fit comfortably
                }
                SDL_RenderSetScale( renderer.get(), text_scale, text_scale );
                text_x = ( WindowWidth - ( ( font->width  * hint_length ) * text_scale ) ) * 0.5f / text_scale;
                text_y = ( WindowHeight - font->height * text_scale ) * 0.5f / text_scale;
                font->OutputChar( renderer, geometry, hint_text, point( text_x + 1, text_y + 1 ), 0,
                                  get_option<int>( "ANDROID_SHORTCUT_OPACITY_SHADOW" ) * 0.01f );
                font->OutputChar( renderer, geometry, hint_text, point( text_x, text_y ),
                                  get_option<int>( "ANDROID_SHORTCUT_COLOR" ),
                                  get_option<int>( "ANDROID_SHORTCUT_OPACITY_FG" ) * 0.01f );
            }
        }
        SDL_RenderSetScale( renderer.get(), 1.0f, 1.0f );
        i++;
        if( ( i + 1 ) * width > WindowWidth ) {
            break;
        }
    }
}

void draw_virtual_joystick()
{

    // Bail out if we don't need to draw the joystick
    if( !get_option<bool>( "ANDROID_SHOW_VIRTUAL_JOYSTICK" ) ||
        finger_down_time <= 0 ||
        SDL_GetTicks() - finger_down_time <= static_cast<uint32_t>
        ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ||
        is_quick_shortcut_touch ||
        is_two_finger_touch ) {
        return;
    }

    SDL_SetTextureAlphaMod( touch_joystick.get(),
                            get_option<int>( "ANDROID_VIRTUAL_JOYSTICK_OPACITY" ) * 0.01f * 255.0f );

    float longest_window_edge = std::max( WindowWidth, WindowHeight );

    SDL_Rect dstrect;

    // Draw deadzone range
    dstrect.w = dstrect.h = ( get_option<float>( "ANDROID_DEADZONE_RANGE" ) ) * longest_window_edge * 2;
    dstrect.x = finger_down_x - dstrect.w / 2;
    dstrect.y = finger_down_y - dstrect.h / 2;
    RenderCopy( renderer, touch_joystick, NULL, &dstrect );

    // Draw repeat delay range
    dstrect.w = dstrect.h = ( get_option<float>( "ANDROID_DEADZONE_RANGE" ) +
                              get_option<float>( "ANDROID_REPEAT_DELAY_RANGE" ) ) * longest_window_edge * 2;
    dstrect.x = finger_down_x - dstrect.w / 2;
    dstrect.y = finger_down_y - dstrect.h / 2;
    RenderCopy( renderer, touch_joystick, NULL, &dstrect );

    // Draw current touch position (50% size of repeat delay range)
    dstrect.w = dstrect.h = dstrect.w / 2;
    dstrect.x = finger_down_x + ( finger_curr_x - finger_down_x ) / 2 - dstrect.w / 2;
    dstrect.y = finger_down_y + ( finger_curr_y - finger_down_y ) / 2 - dstrect.h / 2;
    RenderCopy( renderer, touch_joystick, NULL, &dstrect );

}

float clmp( float value, float low, float high )
{
    return ( value < low ) ? low : ( ( value > high ) ? high : value );
}
float lerp( float t, float a, float b )
{
    return ( 1.0f - t ) * a + t * b;
}

void update_finger_repeat_delay()
{
    float delta_x = finger_curr_x - finger_down_x;
    float delta_y = finger_curr_y - finger_down_y;
    float dist = std::sqrt( delta_x * delta_x + delta_y * delta_y );
    float longest_window_edge = std::max( WindowWidth, WindowHeight );
    float t = clmp( ( dist - ( get_option<float>( "ANDROID_DEADZONE_RANGE" ) * longest_window_edge ) ) /
                    std::max( 0.01f, ( get_option<float>( "ANDROID_REPEAT_DELAY_RANGE" ) ) * longest_window_edge ),
                    0.0f, 1.0f );
    finger_repeat_delay = lerp( std::pow( t, get_option<float>( "ANDROID_SENSITIVITY_POWER" ) ),
                                static_cast<uint32_t>( std::max( get_option<int>( "ANDROID_REPEAT_DELAY_MIN" ),
                                        get_option<int>( "ANDROID_REPEAT_DELAY_MAX" ) ) ),
                                static_cast<uint32_t>( std::min( get_option<int>( "ANDROID_REPEAT_DELAY_MIN" ),
                                        get_option<int>( "ANDROID_REPEAT_DELAY_MAX" ) ) ) );
}

// TODO: Is there a better way to detect when string entry is allowed?
// ANY_INPUT seems close but is abused by code everywhere.
// Had a look through and think I've got all the cases but can't be 100% sure.
bool is_string_input( input_context &ctx )
{
    std::string &category = ctx.get_category();
    return category == "STRING_INPUT"
           || category == "HELP_KEYBINDINGS"
           || category == "NEW_CHAR_DESCRIPTION"
           || category == "WORLDGEN_CONFIRM_DIALOG";
}

int get_key_event_from_string( const std::string &str )
{
    if( !str.empty() ) {
        return str[0];
    }
    return -1;
}
// This function is triggered on finger up events, OR by a repeating timer for touch hold events.
void handle_finger_input( uint32_t ticks )
{

    float delta_x = finger_curr_x - finger_down_x;
    float delta_y = finger_curr_y - finger_down_y;
    float dist = std::sqrt( delta_x * delta_x + delta_y * delta_y ); // in pixel space
    bool handle_diagonals = touch_input_context.is_action_registered( "LEFTUP" );
    bool is_default_mode = touch_input_context.get_category() == "DEFAULTMODE";
    if( dist > ( get_option<float>( "ANDROID_DEADZONE_RANGE" )*std::max( WindowWidth,
                 WindowHeight ) ) ) {
        if( !handle_diagonals ) {
            if( delta_x >= 0 && delta_y >= 0 ) {
                last_input = input_event( delta_x > delta_y ? KEY_RIGHT : KEY_DOWN, CATA_INPUT_KEYBOARD );
            } else if( delta_x < 0 && delta_y >= 0 ) {
                last_input = input_event( -delta_x > delta_y ? KEY_LEFT : KEY_DOWN, CATA_INPUT_KEYBOARD );
            } else if( delta_x >= 0 && delta_y < 0 ) {
                last_input = input_event( delta_x > -delta_y ? KEY_RIGHT : KEY_UP, CATA_INPUT_KEYBOARD );
            } else if( delta_x < 0 && delta_y < 0 ) {
                last_input = input_event( -delta_x > -delta_y ? KEY_LEFT : KEY_UP, CATA_INPUT_KEYBOARD );
            }
        } else {
            if( delta_x > 0 ) {
                if( std::abs( delta_y ) < delta_x * 0.5f ) {
                    // swipe right
                    last_input = input_event( KEY_RIGHT, CATA_INPUT_KEYBOARD );
                } else if( std::abs( delta_y ) < delta_x * 2.0f ) {
                    if( delta_y < 0 ) {
                        // swipe up-right
                        last_input = input_event( JOY_RIGHTUP, CATA_INPUT_GAMEPAD );
                    } else {
                        // swipe down-right
                        last_input = input_event( JOY_RIGHTDOWN, CATA_INPUT_GAMEPAD );
                    }
                } else {
                    if( delta_y < 0 ) {
                        // swipe up
                        last_input = input_event( KEY_UP, CATA_INPUT_KEYBOARD );
                    } else {
                        // swipe down
                        last_input = input_event( KEY_DOWN, CATA_INPUT_KEYBOARD );
                    }
                }
            } else {
                if( std::abs( delta_y ) < -delta_x * 0.5f ) {
                    // swipe left
                    last_input = input_event( KEY_LEFT, CATA_INPUT_KEYBOARD );
                } else if( std::abs( delta_y ) < -delta_x * 2.0f ) {
                    if( delta_y < 0 ) {
                        // swipe up-left
                        last_input = input_event( JOY_LEFTUP, CATA_INPUT_GAMEPAD );

                    } else {
                        // swipe down-left
                        last_input = input_event( JOY_LEFTDOWN, CATA_INPUT_GAMEPAD );
                    }
                } else {
                    if( delta_y < 0 ) {
                        // swipe up
                        last_input = input_event( KEY_UP, CATA_INPUT_KEYBOARD );
                    } else {
                        // swipe down
                        last_input = input_event( KEY_DOWN, CATA_INPUT_KEYBOARD );
                    }
                }
            }
        }
    } else {
        if( ticks - finger_down_time >= static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            // Single tap (repeat) - held, so always treat this as a tap
            // We only allow repeats for waiting, not confirming in menus as that's a bit silly
            if( is_default_mode ) {
                last_input = input_event( get_key_event_from_string( get_option<std::string>( "ANDROID_TAP_KEY" ) ),
                                          CATA_INPUT_KEYBOARD );
            }
        } else {
            if( last_tap_time > 0 &&
                ticks - last_tap_time < static_cast<uint32_t>( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
                // Double tap
                last_input = input_event( is_default_mode ? KEY_ESCAPE : KEY_ESCAPE, CATA_INPUT_KEYBOARD );
                last_tap_time = 0;
            } else {
                // First tap detected, waiting to decide whether it's a single or a double tap input
                last_tap_time = ticks;
            }
        }
    }
}

bool android_is_hardware_keyboard_available()
{
    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
    jobject activity = ( jobject )SDL_AndroidGetActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "isHardwareKeyboardAvailable", "()Z" );
    jboolean ans = env->CallBooleanMethod( activity, method_id );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    return ans;
}

void android_vibrate()
{
    int vibration_ms = get_option<int>( "ANDROID_VIBRATION" );
    if( vibration_ms > 0 && !android_is_hardware_keyboard_available() ) {
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID method_id = env->GetMethodID( clazz, "vibrate", "(I)V" );
        env->CallVoidMethod( activity, method_id, vibration_ms );
        env->DeleteLocalRef( activity );
        env->DeleteLocalRef( clazz );
    }
}
#endif

//Check for any window messages (keypress, paint, mousemove, etc)
static void CheckMessages()
{
    SDL_Event ev;
    bool quit = false;
    bool text_refresh = false;
    bool is_repeat = false;
    if( HandleDPad() ) {
        return;
    }

#if defined(__ANDROID__)
    if( visible_display_frame_dirty ) {
        needupdate = true;
        visible_display_frame_dirty = false;
    }

    uint32_t ticks = SDL_GetTicks();

    // Force text input mode if hardware keyboard is available.
    if( android_is_hardware_keyboard_available() && !SDL_IsTextInputActive() ) {
        SDL_StartTextInput();
    }

    // Make sure the SDL surface view is visible, otherwise the "Z" loading screen is visible.
    if( needs_sdl_surface_visibility_refresh ) {
        needs_sdl_surface_visibility_refresh = false;

        // Call Java show_sdl_surface()
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID method_id = env->GetMethodID( clazz, "show_sdl_surface", "()V" );
        env->CallVoidMethod( activity, method_id );
        env->DeleteLocalRef( activity );
        env->DeleteLocalRef( clazz );
    }

    // Copy the current input context
    if( !input_context::input_context_stack.empty() ) {
        input_context *new_input_context = *--input_context::input_context_stack.end();
        if( new_input_context && *new_input_context != touch_input_context ) {

            // If we were in an allow_text_entry input context, and text input is still active, and we're auto-managing keyboard, hide it.
            if( touch_input_context.allow_text_entry &&
                !new_input_context->allow_text_entry &&
                !is_string_input( *new_input_context ) &&
                SDL_IsTextInputActive() &&
                get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                SDL_StopTextInput();
            }

            touch_input_context = *new_input_context;
            needupdate = true;
        }
    }

    bool is_default_mode = touch_input_context.get_category() == "DEFAULTMODE";
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                 touch_input_context.get_category() )];

    // Don't do this logic if we already need an update, otherwise we're likely to overload the game with too much input on hold repeat events
    if( !needupdate ) {

        // Check action weightings and auto-add any immediate-surrounding actions as quick shortcuts
        // This code is based heavily off action.cpp handle_action_menu() which puts common shortcuts at the top
        if( is_default_mode && get_option<bool>( "ANDROID_SHORTCUT_AUTOADD" ) ) {
            static int last_moves_since_last_save = -1;
            if( last_moves_since_last_save != g->get_moves_since_last_save() ) {
                last_moves_since_last_save = g->get_moves_since_last_save();

                // Actions to add
                std::set<action_id> actions;

                // Actions to remove - we only want to remove things that we're 100% sure won't be useful to players otherwise
                std::set<action_id> actions_remove;

                // Check if we're in a potential combat situation, if so, sort a few actions to the top.
                if( !g->u.get_hostile_creatures( 60 ).empty() ) {
                    // Only prioritize movement options if we're not driving.
                    if( !g->u.controlling_vehicle ) {
                        actions.insert( ACTION_CYCLE_MOVE );
                    }
                    // Only prioritize fire weapon options if we're wielding a ranged weapon.
                    if( g->u.weapon.is_gun() || g->u.weapon.has_flag( "REACH_ATTACK" ) ) {
                        actions.insert( ACTION_FIRE );
                    }
                }

                // If we're already running, make it simple to toggle running to off.
                if( g->u.movement_mode_is( CMM_RUN ) ) {
                    actions.insert( ACTION_TOGGLE_RUN );
                }
                // If we're already crouching, make it simple to toggle crouching to off.
                if( g->u.movement_mode_is( CMM_CROUCH ) ) {
                    actions.insert( ACTION_TOGGLE_CROUCH );
                }

                // We're not already running or in combat, so remove cycle walk/run
                if( std::find( actions.begin(), actions.end(), ACTION_CYCLE_MOVE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CYCLE_MOVE );
                }

                // Check if we can perform one of our actions on nearby terrain. If so,
                // display that action at the top of the list.
                for( int dx = -1; dx <= 1; dx++ ) {
                    for( int dy = -1; dy <= 1; dy++ ) {
                        int x = g->u.posx() + dx;
                        int y = g->u.posy() + dy;
                        int z = g->u.posz();
                        const tripoint pos( x, y, z );

                        // Check if we're near a vehicle, if so, vehicle controls should be top.
                        {
                            const optional_vpart_position vp = g->m.veh_at( pos );
                            vehicle *const veh = veh_pointer_or_null( vp );
                            if( veh ) {
                                const int veh_part = vp ? vp->part_index() : -1;
                                if( veh->part_with_feature( veh_part, "CONTROLS", true ) >= 0 ) {
                                    actions.insert( ACTION_CONTROL_VEHICLE );
                                }
                                const int openablepart = veh->part_with_feature( veh_part, "OPENABLE", true );
                                if( openablepart >= 0 && veh->is_open( openablepart ) && ( dx != 0 ||
                                        dy != 0 ) ) { // an open door adjacent to us
                                    actions.insert( ACTION_CLOSE );
                                }
                                const int curtainpart = veh->part_with_feature( veh_part, "CURTAIN", true );
                                if( curtainpart >= 0 && veh->is_open( curtainpart ) && ( dx != 0 || dy != 0 ) ) {
                                    actions.insert( ACTION_CLOSE );
                                }
                                const int cargopart = veh->part_with_feature( veh_part, "CARGO", true );
                                if( cargopart >= 0 && ( !veh->get_items( cargopart ).empty() ) ) {
                                    actions.insert( ACTION_PICKUP );
                                }
                            }
                        }

                        if( dx != 0 || dy != 0 ) {
                            // Check for actions that work on nearby tiles
                            //if( can_interact_at( ACTION_OPEN, pos ) ) {
                            // don't bother with open since user can just walk into target
                            //}
                            if( can_interact_at( ACTION_CLOSE, pos ) ) {
                                actions.insert( ACTION_CLOSE );
                            }
                            if( can_interact_at( ACTION_EXAMINE, pos ) ) {
                                actions.insert( ACTION_EXAMINE );
                            }
                        } else {
                            // Check for actions that work on own tile only
                            if( can_interact_at( ACTION_BUTCHER, pos ) ) {
                                actions.insert( ACTION_BUTCHER );
                            } else {
                                actions_remove.insert( ACTION_BUTCHER );
                            }

                            if( can_interact_at( ACTION_MOVE_UP, pos ) ) {
                                actions.insert( ACTION_MOVE_UP );
                            } else {
                                actions_remove.insert( ACTION_MOVE_UP );
                            }

                            if( can_interact_at( ACTION_MOVE_DOWN, pos ) ) {
                                actions.insert( ACTION_MOVE_DOWN );
                            } else {
                                actions_remove.insert( ACTION_MOVE_DOWN );
                            }
                        }

                        // Check for actions that work on nearby tiles and own tile
                        if( can_interact_at( ACTION_PICKUP, pos ) ) {
                            actions.insert( ACTION_PICKUP );
                        }
                    }
                }

                // We're not near a vehicle, so remove control vehicle
                if( std::find( actions.begin(), actions.end(), ACTION_CONTROL_VEHICLE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CONTROL_VEHICLE );
                }

                // We're not able to close anything nearby, so remove it
                if( std::find( actions.begin(), actions.end(), ACTION_CLOSE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CLOSE );
                }

                // We're not able to examine anything nearby, so remove it
                if( std::find( actions.begin(), actions.end(), ACTION_EXAMINE ) == actions.end() ) {
                    actions_remove.insert( ACTION_EXAMINE );
                }

                // We're not able to pickup anything nearby, so remove it
                if( std::find( actions.begin(), actions.end(), ACTION_PICKUP ) == actions.end() ) {
                    actions_remove.insert( ACTION_PICKUP );
                }

                // Check if we can't move because of safe mode - if so, add ability to ignore
                if( g && !g->check_safe_mode_allowed( false ) ) {
                    actions.insert( ACTION_IGNORE_ENEMY );
                    actions.insert( ACTION_TOGGLE_SAFEMODE );
                } else {
                    actions_remove.insert( ACTION_IGNORE_ENEMY );
                    actions_remove.insert( ACTION_TOGGLE_SAFEMODE );
                }

                // Check if we're significantly hungry or thirsty - if so, add eat
                if( g->u.get_hunger() > 100 || g->u.get_thirst() > thirst_levels::thirsty ) {
                    actions.insert( ACTION_EAT );
                }

                // Check if we're dead tired - if so, add sleep
                if( g->u.get_fatigue() > fatigue_levels::dead_tired ) {
                    actions.insert( ACTION_SLEEP );
                }

                for( const auto &action : actions ) {
                    if( add_best_key_for_action_to_quick_shortcuts( action, touch_input_context.get_category(),
                            !get_option<bool>( "ANDROID_SHORTCUT_AUTOADD_FRONT" ) ) ) {
                        needupdate = true;
                    }
                }

                size_t old_size = qsl.size();
                for( const auto &action_remove : actions_remove ) {
                    remove_action_from_quick_shortcuts( action_remove, touch_input_context.get_category() );
                }
                if( qsl.size() != old_size ) {
                    needupdate = true;
                }
            }
        }

        if( remove_expired_actions_from_quick_shortcuts( touch_input_context.get_category() ) ) {
            needupdate = true;
        }

        // Toggle quick shortcuts on/off
        if( ac_back_down_time > 0 &&
            ticks - ac_back_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            if( !quick_shortcuts_toggle_handled ) {
                quick_shortcuts_enabled = !quick_shortcuts_enabled;
                quick_shortcuts_toggle_handled = true;
                refresh_display();

                // Display an Android toast message
                {
                    JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
                    jobject activity = ( jobject )SDL_AndroidGetActivity();
                    jclass clazz( env->GetObjectClass( activity ) );
                    jstring toast_message = env->NewStringUTF( quick_shortcuts_enabled ? "Shortcuts visible" :
                                            "Shortcuts hidden" );
                    jmethodID method_id = env->GetMethodID( clazz, "toast", "(Ljava/lang/String;)V" );
                    env->CallVoidMethod( activity, method_id, toast_message );
                    env->DeleteLocalRef( activity );
                    env->DeleteLocalRef( clazz );
                }
            }
        }

        // Handle repeating inputs from touch + holds
        if( !is_quick_shortcut_touch && !is_two_finger_touch && finger_down_time > 0 &&
            ticks - finger_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            if( ticks - finger_repeat_time > finger_repeat_delay ) {
                handle_finger_input( ticks );
                finger_repeat_time = ticks;
                // Prevent repeating inputs on the next call to this function if there is a fingerup event
                while( SDL_PollEvent( &ev ) ) {
                    if( ev.type == SDL_FINGERUP ) {
                        second_finger_down_x = second_finger_curr_x = finger_down_x = finger_curr_x = -1.0f;
                        second_finger_down_y = second_finger_curr_y = finger_down_y = finger_curr_y = -1.0f;
                        is_two_finger_touch = false;
                        finger_down_time = 0;
                        finger_repeat_time = 0;
                        // let the next call decide if needupdate should be true
                        break;
                    }
                }
                return;
            }
        }

        // If we received a first tap and not another one within a certain period, this was a single tap, so trigger the input event
        if( !is_quick_shortcut_touch && !is_two_finger_touch && last_tap_time > 0 &&
            ticks - last_tap_time >= static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            // Single tap
            last_tap_time = ticks;
            last_input = input_event( is_default_mode ? get_key_event_from_string(
                                          get_option<std::string>( "ANDROID_TAP_KEY" ) ) : '\n', CATA_INPUT_KEYBOARD );
            last_tap_time = 0;
            return;
        }

        // ensure hint text pops up even if player doesn't move finger to trigger a FINGERMOTION event
        if( is_quick_shortcut_touch && finger_down_time > 0 &&
            ticks - finger_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            needupdate = true;
        }
    }
#endif

    last_input = input_event();

    bool need_redraw = false;

    while( SDL_PollEvent( &ev ) ) {
        switch( ev.type ) {
            case SDL_WINDOWEVENT:
                switch( ev.window.event ) {
#if defined(__ANDROID__)
                    // SDL will send a focus lost event whenever the app loses focus (eg. lock screen, switch app focus etc.)
                    // If we detect it and the game seems in a saveable state, try and do a quicksave. This is a bit dodgy
                    // as the player could be ANYWHERE doing ANYTHING (a sub-menu, interacting with an NPC/computer etc.)
                    // but it seems to work so far, and the alternative is the player losing their progress as the app is likely
                    // to be destroyed pretty quickly when it goes out of focus due to memory usage.
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        if( world_generator &&
                            world_generator->active_world &&
                            g && g->uquit == QUIT_NO &&
                            get_option<bool>( "ANDROID_QUICKSAVE" ) &&
                            !std::uncaught_exception() ) {
                            g->quicksave();
                        }
                        break;
                    // SDL sends a window size changed event whenever the screen rotates orientation
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        WindowWidth = ev.window.data1;
                        WindowHeight = ev.window.data2;
                        SDL_Delay( 500 );
                        SDL_GetWindowSurface( window.get() );
                        refresh_display();
                        needupdate = true;
                        break;
#endif
                    case SDL_WINDOWEVENT_SHOWN:
                    case SDL_WINDOWEVENT_MINIMIZED:
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                        need_redraw = true;
                        needupdate = true;
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
#if defined(__ANDROID__)
                        needs_sdl_surface_visibility_refresh = true;
                        if( android_is_hardware_keyboard_available() ) {
                            SDL_StopTextInput();
                            SDL_StartTextInput();
                        }
#endif
                        break;
                    case SDL_WINDOWEVENT_RESIZED: {
                        restore_on_out_of_scope<input_event> prev_last_input( last_input );
                        needupdate = handle_resize( ev.window.data1, ev.window.data2 );
                        break;
                    }
                    default:
                        break;
                }
                break;
            case SDL_RENDER_TARGETS_RESET:
                need_redraw = true;
                needupdate = true;
                break;
            case SDL_KEYDOWN: {
#if defined(__ANDROID__)
                // Toggle virtual keyboard with Android back button. For some reason I get double inputs, so ignore everything once it's already down.
                if( ev.key.keysym.sym == SDLK_AC_BACK && ac_back_down_time == 0 ) {
                    ac_back_down_time = ticks;
                    quick_shortcuts_toggle_handled = false;
                }
#endif
                is_repeat = ev.key.repeat;
                //hide mouse cursor on keyboard input
                if( get_option<std::string>( "HIDE_CURSOR" ) != "show" && SDL_ShowCursor( -1 ) ) {
                    SDL_ShowCursor( SDL_DISABLE );
                }
                const int lc = sdl_keysym_to_curses( ev.key.keysym );
                if( lc <= 0 ) {
                    // a key we don't know in curses and won't handle.
                    break;
                } else if( add_alt_code( lc ) ) {
                    // key was handled
                } else {
                    last_input = input_event( lc, CATA_INPUT_KEYBOARD );
#if defined(__ANDROID__)
                    if( !android_is_hardware_keyboard_available() ) {
                        if( !is_string_input( touch_input_context ) && !touch_input_context.allow_text_entry ) {
                            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                SDL_StopTextInput();
                            }

                            // add a quick shortcut
                            if( !last_input.text.empty() || !inp_mngr.get_keyname( lc, CATA_INPUT_KEYBOARD ).empty() ) {
                                qsl.remove( last_input );
                                add_quick_shortcut( qsl, last_input, false, true );
                                refresh_display();
                            }
                        } else if( lc == '\n' || lc == KEY_ESCAPE ) {
                            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                SDL_StopTextInput();
                            }
                        }
                    }
#endif
                }
            }
            break;
            case SDL_KEYUP: {
#if defined(__ANDROID__)
                // Toggle virtual keyboard with Android back button
                if( ev.key.keysym.sym == SDLK_AC_BACK ) {
                    if( ticks - ac_back_down_time <= static_cast<uint32_t>
                        ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
                        if( SDL_IsTextInputActive() ) {
                            SDL_StopTextInput();
                        } else {
                            SDL_StartTextInput();
                        }
                    }
                    ac_back_down_time = 0;
                }
#endif
                is_repeat = ev.key.repeat;
                if( ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT ) {
                    int code = end_alt_code();
                    if( code ) {
                        last_input = input_event( code, CATA_INPUT_KEYBOARD );
                        last_input.text = utf32_to_utf8( code );
                    }
                }
            }
            break;
            case SDL_TEXTINPUT:
                if( !add_alt_code( *ev.text.text ) ) {
                    if( strlen( ev.text.text ) > 0 ) {
                        const unsigned lc = UTF8_getch( ev.text.text );
                        last_input = input_event( lc, CATA_INPUT_KEYBOARD );
#if defined(__ANDROID__)
                        if( !android_is_hardware_keyboard_available() ) {
                            if( !is_string_input( touch_input_context ) && !touch_input_context.allow_text_entry ) {
                                if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                    SDL_StopTextInput();
                                }

                                quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                             touch_input_context.get_category() )];
                                qsl.remove( last_input );
                                add_quick_shortcut( qsl, last_input, false, true );
                                refresh_display();
                            } else if( lc == '\n' || lc == KEY_ESCAPE ) {
                                if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                    SDL_StopTextInput();
                                }
                            }
                        }
#endif
                    } else {
                        // no key pressed in this event
                        last_input = input_event();
                        last_input.type = CATA_INPUT_KEYBOARD;
                    }
                    last_input.text = ev.text.text;
                    text_refresh = true;
                }
                break;
            case SDL_TEXTEDITING: {
                if( strlen( ev.edit.text ) > 0 ) {
                    const unsigned lc = UTF8_getch( ev.edit.text );
                    last_input = input_event( lc, CATA_INPUT_KEYBOARD );
                } else {
                    // no key pressed in this event
                    last_input = input_event();
                    last_input.type = CATA_INPUT_KEYBOARD;
                }
                last_input.edit = ev.edit.text;
                last_input.edit_refresh = true;
                text_refresh = true;
            }
            break;
            case SDL_JOYBUTTONDOWN:
                last_input = input_event( ev.jbutton.button, CATA_INPUT_KEYBOARD );
                break;
            case SDL_JOYAXISMOTION:
                // on gamepads, the axes are the analog sticks
                // TODO: somehow get the "digipad" values from the axes
                break;
            case SDL_MOUSEMOTION:
                if( get_option<std::string>( "HIDE_CURSOR" ) == "show" ||
                    get_option<std::string>( "HIDE_CURSOR" ) == "hidekb" ) {
                    if( !SDL_ShowCursor( -1 ) ) {
                        SDL_ShowCursor( SDL_ENABLE );
                    }

                    // Only monitor motion when cursor is visible
                    last_input = input_event( MOUSE_MOVE, CATA_INPUT_MOUSE );
                }
                break;

            case SDL_MOUSEBUTTONUP:
                switch( ev.button.button ) {
                    case SDL_BUTTON_LEFT:
                        last_input = input_event( MOUSE_BUTTON_LEFT, CATA_INPUT_MOUSE );
                        break;
                    case SDL_BUTTON_RIGHT:
                        last_input = input_event( MOUSE_BUTTON_RIGHT, CATA_INPUT_MOUSE );
                        break;
                }
                break;

            case SDL_MOUSEWHEEL:
                if( ev.wheel.y > 0 ) {
                    last_input = input_event( SCROLLWHEEL_UP, CATA_INPUT_MOUSE );
                } else if( ev.wheel.y < 0 ) {
                    last_input = input_event( SCROLLWHEEL_DOWN, CATA_INPUT_MOUSE );
                }
                break;

#if defined(__ANDROID__)
            case SDL_FINGERMOTION:
                if( ev.tfinger.fingerId == 0 ) {
                    if( !is_quick_shortcut_touch ) {
                        update_finger_repeat_delay();
                    }
                    needupdate = true; // ensure virtual joystick and quick shortcuts redraw as we interact
                    finger_curr_x = ev.tfinger.x * WindowWidth;
                    finger_curr_y = ev.tfinger.y * WindowHeight;

                    if( get_option<bool>( "ANDROID_VIRTUAL_JOYSTICK_FOLLOW" ) && !is_two_finger_touch ) {
                        // If we've moved too far from joystick center, offset joystick center automatically
                        float delta_x = finger_curr_x - finger_down_x;
                        float delta_y = finger_curr_y - finger_down_y;
                        float dist = std::sqrt( delta_x * delta_x + delta_y * delta_y );
                        float max_dist = ( get_option<float>( "ANDROID_DEADZONE_RANGE" ) +
                                           get_option<float>( "ANDROID_REPEAT_DELAY_RANGE" ) ) * std::max( WindowWidth, WindowHeight );
                        if( dist > max_dist ) {
                            float delta_ratio = ( dist / max_dist ) - 1.0f;
                            finger_down_x += delta_x * delta_ratio;
                            finger_down_y += delta_y * delta_ratio;
                        }
                    }

                } else if( ev.tfinger.fingerId == 1 ) {
                    second_finger_curr_x = ev.tfinger.x * WindowWidth;
                    second_finger_curr_y = ev.tfinger.y * WindowHeight;
                }
                break;
            case SDL_FINGERDOWN:
                if( ev.tfinger.fingerId == 0 ) {
                    finger_down_x = finger_curr_x = ev.tfinger.x * WindowWidth;
                    finger_down_y = finger_curr_y = ev.tfinger.y * WindowHeight;
                    finger_down_time = ticks;
                    finger_repeat_time = 0;
                    is_quick_shortcut_touch = get_quick_shortcut_under_finger() != NULL;
                    if( !is_quick_shortcut_touch ) {
                        update_finger_repeat_delay();
                    }
                    needupdate = true; // ensure virtual joystick and quick shortcuts redraw as we interact
                } else if( ev.tfinger.fingerId == 1 ) {
                    if( !is_quick_shortcut_touch ) {
                        second_finger_down_x = second_finger_curr_x = ev.tfinger.x * WindowWidth;
                        second_finger_down_y = second_finger_curr_y = ev.tfinger.y * WindowHeight;
                        is_two_finger_touch = true;
                    }
                }
                break;
            case SDL_FINGERUP:
                if( ev.tfinger.fingerId == 0 ) {
                    finger_curr_x = ev.tfinger.x * WindowWidth;
                    finger_curr_y = ev.tfinger.y * WindowHeight;
                    if( is_quick_shortcut_touch ) {
                        input_event *quick_shortcut = get_quick_shortcut_under_finger();
                        if( quick_shortcut ) {
                            last_input = *quick_shortcut;
                            if( get_option<bool>( "ANDROID_SHORTCUT_MOVE_FRONT" ) ) {
                                quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                             touch_input_context.get_category() )];
                                reorder_quick_shortcut( qsl, quick_shortcut->get_first_input(), false );
                            }
                            quick_shortcut->shortcut_last_used_action_counter = g->get_user_action_counter();
                        } else {
                            // Get the quick shortcut that was originally touched
                            quick_shortcut = get_quick_shortcut_under_finger( true );
                            if( quick_shortcut &&
                                ticks - finger_down_time <= static_cast<uint32_t>( get_option<int>( "ANDROID_INITIAL_DELAY" ) )
                                &&
                                finger_curr_y < finger_down_y &&
                                finger_down_y - finger_curr_y > std::abs( finger_down_x - finger_curr_x ) ) {
                                // a flick up was detected, remove the quick shortcut!
                                quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                             touch_input_context.get_category() )];
                                qsl.remove( *quick_shortcut );
                            }
                        }
                    } else {
                        if( is_two_finger_touch ) {
                            // handle zoom in/out
                            if( is_default_mode ) {
                                float x1 = ( finger_curr_x - finger_down_x );
                                float y1 = ( finger_curr_y - finger_down_y );
                                float d1 = std::sqrt( x1 * x1 + y1 * y1 );

                                float x2 = ( second_finger_curr_x - second_finger_down_x );
                                float y2 = ( second_finger_curr_y - second_finger_down_y );
                                float d2 = std::sqrt( x2 * x2 + y2 * y2 );

                                float longest_window_edge = std::max( WindowWidth, WindowHeight );

                                if( std::max( d1, d2 ) < get_option<float>( "ANDROID_DEADZONE_RANGE" ) * longest_window_edge ) {
                                    last_input = input_event( get_key_event_from_string(
                                                                  get_option<std::string>( "ANDROID_2_TAP_KEY" ) ), CATA_INPUT_KEYBOARD );
                                } else {
                                    float dot = ( x1 * x2 + y1 * y2 ) / ( d1 * d2 ); // dot product of two finger vectors, -1 to +1
                                    if( dot > 0.0f ) { // both fingers mostly heading in same direction, check for double-finger swipe gesture
                                        float dratio = d1 / d2;
                                        const float dist_ratio = 0.3f;
                                        if( dratio > dist_ratio &&
                                            dratio < ( 1.0f /
                                                       dist_ratio ) ) { // both fingers moved roughly the same distance, so it's a double-finger swipe!
                                            float xavg = 0.5f * ( x1 + x2 );
                                            float yavg = 0.5f * ( y1 + y2 );
                                            if( xavg > 0 && xavg > std::abs( yavg ) ) {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_2_SWIPE_LEFT_KEY" ) ), CATA_INPUT_KEYBOARD );
                                            } else if( xavg < 0 && -xavg > std::abs( yavg ) ) {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_2_SWIPE_RIGHT_KEY" ) ), CATA_INPUT_KEYBOARD );
                                            } else if( yavg > 0 && yavg > std::abs( xavg ) ) {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_2_SWIPE_DOWN_KEY" ) ), CATA_INPUT_KEYBOARD );
                                            } else {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_2_SWIPE_UP_KEY" ) ), CATA_INPUT_KEYBOARD );
                                            }
                                        }
                                    } else {
                                        // both fingers heading in opposite direction, check for zoom gesture
                                        float down_x = finger_down_x - second_finger_down_x;
                                        float down_y = finger_down_y - second_finger_down_y;
                                        float down_dist = std::sqrt( down_x * down_x + down_y * down_y );

                                        float curr_x = finger_curr_x - second_finger_curr_x;
                                        float curr_y = finger_curr_y - second_finger_curr_y;
                                        float curr_dist = std::sqrt( curr_x * curr_x + curr_y * curr_y );

                                        const float zoom_ratio = 0.9f;
                                        if( curr_dist < down_dist * zoom_ratio ) {
                                            last_input = input_event( get_key_event_from_string(
                                                                          get_option<std::string>( "ANDROID_PINCH_IN_KEY" ) ), CATA_INPUT_KEYBOARD );
                                        } else if( curr_dist > down_dist / zoom_ratio ) {
                                            last_input = input_event( get_key_event_from_string(
                                                                          get_option<std::string>( "ANDROID_PINCH_OUT_KEY" ) ), CATA_INPUT_KEYBOARD );
                                        }
                                    }
                                }
                            }
                        } else if( ticks - finger_down_time <= static_cast<uint32_t>(
                                       get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
                            handle_finger_input( ticks );
                        }
                    }
                    second_finger_down_x = second_finger_curr_x = finger_down_x = finger_curr_x = -1.0f;
                    second_finger_down_y = second_finger_curr_y = finger_down_y = finger_curr_y = -1.0f;
                    is_two_finger_touch = false;
                    finger_down_time = 0;
                    finger_repeat_time = 0;
                    needupdate = true; // ensure virtual joystick and quick shortcuts are updated properly
                    refresh_display(); // as above, but actually redraw it now as well
                } else if( ev.tfinger.fingerId == 1 ) {
                    if( is_two_finger_touch ) {
                        // on second finger release, just remember the x/y position so we can calculate delta once first finger is done
                        // is_two_finger_touch will be reset when first finger lifts (see above)
                        second_finger_curr_x = ev.tfinger.x * WindowWidth;
                        second_finger_curr_y = ev.tfinger.y * WindowHeight;
                    }
                }

                break;
#endif

            case SDL_QUIT:
                quit = true;
                break;
        }
        if( text_refresh && !is_repeat ) {
            break;
        }
    }
    if( need_redraw ) {
        restore_on_out_of_scope<input_event> prev_last_input( last_input );
        // FIXME: SDL_RENDER_TARGETS_RESET only seems to be fired after the first redraw
        // when restoring the window after system sleep, rather than immediately
        // on focus gain. This seems to mess up the first redraw and
        // causes black screen that lasts ~0.5 seconds before the screen
        // contents are redrawn in the following code.
        window_dimensions dim = get_window_dimensions( catacurses::stdscr );
        ui_manager::invalidate( rectangle( point_zero, dim.window_size_pixel ), false );
        ui_manager::redraw();
    }
    if( needupdate ) {
        try_sdl_update();
    }
    if( quit ) {
        exit_handler( 0 );
    }
}

//***********************************
//Pseudo-Curses Functions           *
//***********************************

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

static void init_term_size_and_scaling_factor()
{
    scaling_factor = 1;
    int terminal_x = get_option<int>( "TERMINAL_X" );
    int terminal_y = get_option<int>( "TERMINAL_Y" );

#if !defined(__ANDROID__)

    if( get_option<std::string>( "SCALING_FACTOR" ) == "2" ) {
        scaling_factor = 2;
    } else if( get_option<std::string>( "SCALING_FACTOR" ) == "4" ) {
        scaling_factor = 4;
    }

    if( scaling_factor > 1 ) {

        int max_width, max_height;

        int current_display_id = std::stoi( get_option<std::string>( "DISPLAY" ) );
        SDL_DisplayMode current_display;

        if( SDL_GetDesktopDisplayMode( current_display_id, &current_display ) == 0 ) {
            if( get_option<std::string>( "FULLSCREEN" ) == "no" ) {

                // Make a maximized test window to determine maximum windowed size
                SDL_Window_Ptr test_window;
                test_window.reset( SDL_CreateWindow( "test_window",
                                                     SDL_WINDOWPOS_CENTERED_DISPLAY( current_display_id ),
                                                     SDL_WINDOWPOS_CENTERED_DISPLAY( current_display_id ),
                                                     FULL_SCREEN_WIDTH * fontwidth,
                                                     FULL_SCREEN_HEIGHT * fontheight,
                                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_MAXIMIZED
                                                   ) );

                SDL_GetWindowSize( test_window.get(), &max_width, &max_height );

                // If the video subsystem isn't reset the test window messes things up later
                test_window.reset();
                SDL_QuitSubSystem( SDL_INIT_VIDEO );
                SDL_InitSubSystem( SDL_INIT_VIDEO );

            } else {
                // For fullscreen or window borderless maximum size is the display size
                max_width = current_display.w;
                max_height = current_display.h;
            }
        } else {
            dbg( D_WARNING ) << "Failed to get current Display Mode, assuming infinite display size.";
            max_width = INT_MAX;
            max_height = INT_MAX;
        }

        if( terminal_x * fontwidth > max_width ||
            FULL_SCREEN_WIDTH * fontwidth * scaling_factor > max_width ) {
            if( FULL_SCREEN_WIDTH * fontwidth * scaling_factor > max_width ) {
                dbg( D_WARNING ) << "SCALING_FACTOR set too high for display size, resetting to 1";
                scaling_factor = 1;
                terminal_x = max_width / fontwidth;
                terminal_y = max_height / fontheight;
                get_options().get_option( "SCALING_FACTOR" ).setValue( "1" );
            } else {
                terminal_x = max_width / fontwidth;
            }
        }

        if( terminal_y * fontheight > max_height ||
            FULL_SCREEN_HEIGHT * fontheight * scaling_factor > max_height ) {
            if( FULL_SCREEN_HEIGHT * fontheight * scaling_factor > max_height ) {
                dbg( D_WARNING ) << "SCALING_FACTOR set too high for display size, resetting to 1";
                scaling_factor = 1;
                terminal_x = max_width / fontwidth;
                terminal_y = max_height / fontheight;
                get_options().get_option( "SCALING_FACTOR" ).setValue( "1" );
            } else {
                terminal_y = max_height / fontheight;
            }
        }

        terminal_x -= terminal_x % scaling_factor;
        terminal_y -= terminal_y % scaling_factor;

        terminal_x = std::max( FULL_SCREEN_WIDTH * scaling_factor, terminal_x );
        terminal_y = std::max( FULL_SCREEN_HEIGHT * scaling_factor, terminal_y );

        get_options().get_option( "TERMINAL_X" ).setValue(
            std::max( FULL_SCREEN_WIDTH * scaling_factor, terminal_x ) );
        get_options().get_option( "TERMINAL_Y" ).setValue(
            std::max( FULL_SCREEN_HEIGHT * scaling_factor, terminal_y ) );

        get_options().save();
    }

#endif //__ANDROID__

    TERMINAL_WIDTH = terminal_x / scaling_factor;
    TERMINAL_HEIGHT = terminal_y / scaling_factor;
}

//Basic Init, create the font, backbuffer, etc
void catacurses::init_interface()
{
    last_input = input_event();
    inputdelay = -1;

    InitSDL();

    get_options().init();
    get_options().load();

    font_loader fl;
    fl.load();
    fl.fontwidth = get_option<int>( "FONT_WIDTH" );
    fl.fontheight = get_option<int>( "FONT_HEIGHT" );
    fl.fontsize = get_option<int>( "FONT_SIZE" );
    fl.fontblending = get_option<bool>( "FONT_BLENDING" );
    fl.map_fontsize = get_option<int>( "MAP_FONT_SIZE" );
    fl.map_fontwidth = get_option<int>( "MAP_FONT_WIDTH" );
    fl.map_fontheight = get_option<int>( "MAP_FONT_HEIGHT" );
    fl.overmap_fontsize = get_option<int>( "OVERMAP_FONT_SIZE" );
    fl.overmap_fontwidth = get_option<int>( "OVERMAP_FONT_WIDTH" );
    fl.overmap_fontheight = get_option<int>( "OVERMAP_FONT_HEIGHT" );
    ::fontwidth = fl.fontwidth;
    ::fontheight = fl.fontheight;

    init_term_size_and_scaling_factor();

    WinCreate();

    dbg( D_INFO ) << "Initializing SDL Tiles context";
    tilecontext = std::make_unique<cata_tiles>( renderer, geometry );
    try {
        tilecontext->load_tileset( get_option<std::string>( "TILES" ), true );
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to check for tileset: " << err.what();
        // use_tiles is the cached value of the USE_TILES option.
        // most (all?) code refers to this to see if cata_tiles should be used.
        // Setting it to false disables this from getting used.
        use_tiles = false;
    }

    color_loader<SDL_Color>().load( windowsPalette );
    init_colors();

    // initialize sound set
    load_soundset();

    font = std::make_unique<FontFallbackList>( renderer, format, fl.fontwidth, fl.fontheight,
            windowsPalette, fl.typeface, fl.fontsize, fl.fontblending );
    map_font = std::make_unique<FontFallbackList>( renderer, format, fl.map_fontwidth,
               fl.map_fontheight,
               windowsPalette, fl.map_typeface, fl.map_fontsize, fl.fontblending );
    overmap_font = std::make_unique<FontFallbackList>( renderer, format, fl.overmap_fontwidth,
                   fl.overmap_fontheight,
                   windowsPalette, fl.overmap_typeface, fl.overmap_fontsize, fl.fontblending );
    stdscr = newwin( get_terminal_height(), get_terminal_width(), point_zero );
    //newwin calls `new WINDOW`, and that will throw, but not return nullptr.

#if defined(__ANDROID__)
    // Make sure we initialize preview_terminal_width/height to sensible values
    preview_terminal_width = TERMINAL_WIDTH * fontwidth;
    preview_terminal_height = TERMINAL_HEIGHT * fontheight;
#endif
}

// This is supposed to be called from init.cpp, and only from there.
void load_tileset()
{
    if( !tilecontext || !use_tiles ) {
        return;
    }
    tilecontext->load_tileset( get_option<std::string>( "TILES" ) );
    tilecontext->do_tile_loading_report();
}

//Ends the terminal, destroy everything
void catacurses::endwin()
{
    tilecontext.reset();
    font.reset();
    map_font.reset();
    overmap_font.reset();
    WinDestroy();
}

template<>
SDL_Color color_loader<SDL_Color>::from_rgb( const int r, const int g, const int b )
{
    SDL_Color result;
    result.b = b;       //Blue
    result.g = g;       //Green
    result.r = r;       //Red
    result.a = 0xFF;    // Opaque
    return result;
}

void input_manager::set_timeout( const int t )
{
    input_timeout = t;
    inputdelay = t;
}

// This is how we're actually going to handle input events, SDL getch
// is simply a wrapper around this.
input_event input_manager::get_input_event()
{
    previously_pressed_key = 0;
    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there

    wrefresh( catacurses::stdscr );

    if( inputdelay < 0 ) {
        do {
            CheckMessages();
            if( last_input.type != CATA_INPUT_ERROR ) {
                break;
            }
            SDL_Delay( 1 );
        } while( last_input.type == CATA_INPUT_ERROR );
    } else if( inputdelay > 0 ) {
        uint32_t starttime = SDL_GetTicks();
        uint32_t endtime = 0;
        bool timedout = false;
        do {
            CheckMessages();
            endtime = SDL_GetTicks();
            if( last_input.type != CATA_INPUT_ERROR ) {
                break;
            }
            SDL_Delay( 1 );
            timedout = endtime >= starttime + inputdelay;
            if( timedout ) {
                last_input.type = CATA_INPUT_TIMEOUT;
            }
        } while( !timedout );
    } else {
        CheckMessages();
    }

    if( last_input.type == CATA_INPUT_MOUSE ) {
        SDL_GetMouseState( &last_input.mouse_pos.x, &last_input.mouse_pos.y );
    } else if( last_input.type == CATA_INPUT_KEYBOARD ) {
        previously_pressed_key = last_input.get_first_input();
#if defined(__ANDROID__)
        android_vibrate();
#endif
    }
#if defined(__ANDROID__)
    else if( last_input.type == CATA_INPUT_GAMEPAD ) {
        android_vibrate();
    }
#endif

    return last_input;
}

bool gamepad_available()
{
    return joystick != nullptr;
}

void rescale_tileset( int size )
{
    tilecontext->set_draw_scale( size );
    g->mark_main_ui_adaptor_resize();
}

static window_dimensions get_window_dimensions( const catacurses::window &win,
        const point &pos, const point &size )
{
    window_dimensions dim;
    if( use_tiles && g && win == g->w_terrain ) {
        // tiles might have different dimensions than standard font
        dim.scaled_font_size.x = tilecontext->get_tile_width();
        dim.scaled_font_size.y = tilecontext->get_tile_height();
    } else if( map_font && g && win == g->w_terrain ) {
        // map font (if any) might differ from standard font
        dim.scaled_font_size.x = map_font->width;
        dim.scaled_font_size.y = map_font->height;
    } else if( overmap_font && g && win == g->w_overmap ) {
        dim.scaled_font_size.x = overmap_font->width;
        dim.scaled_font_size.y = overmap_font->height;
    } else {
        dim.scaled_font_size.x = fontwidth;
        dim.scaled_font_size.y = fontheight;
    }

    // multiplied by the user's specified scaling factor regardless of whether tiles are in use
    dim.scaled_font_size *= get_scaling_factor();

    if( win ) {
        cata_cursesport::WINDOW *const pwin = win.get<cata_cursesport::WINDOW>();
        dim.window_pos_cell = pwin->pos;
        dim.window_size_cell.x = pwin->width;
        dim.window_size_cell.y = pwin->height;
    } else {
        dim.window_pos_cell = pos;
        dim.window_size_cell = size;
    }

    // the window position is *always* in standard font dimensions!
    dim.window_pos_pixel = point( dim.window_pos_cell.x * fontwidth,
                                  dim.window_pos_cell.y * fontheight );
    // But the size of the window is in the font dimensions of the window.
    dim.window_size_pixel.x = dim.window_size_cell.x * dim.scaled_font_size.x;
    dim.window_size_pixel.y = dim.window_size_cell.y * dim.scaled_font_size.y;

    return dim;
}

window_dimensions get_window_dimensions( const catacurses::window &win )
{
    return get_window_dimensions( win, point_zero, point_zero );
}

window_dimensions get_window_dimensions( const point &pos, const point &size )
{
    return get_window_dimensions( {}, pos, size );
}

cata::optional<tripoint> input_context::get_coordinates( const catacurses::window &capture_win_ )
{
    if( !coordinate_input_received ) {
        return cata::nullopt;
    }

    const catacurses::window &capture_win = capture_win_ ? capture_win_ : g->w_terrain;
    const window_dimensions dim = get_window_dimensions( capture_win );

    const int &fw = dim.scaled_font_size.x;
    const int &fh = dim.scaled_font_size.y;
    const point &win_min = dim.window_pos_pixel;
    const point &win_size = dim.window_size_pixel;
    const point win_max = win_min + win_size;

    // Translate mouse coordinates to map coordinates based on tile size
    // Check if click is within bounds of the window we care about
    const rectangle win_bounds( win_min, win_max );
    if( !win_bounds.contains_inclusive( coordinate ) ) {
        return cata::nullopt;
    }

    point view_offset;
    if( capture_win == g->w_terrain ) {
        view_offset = g->ter_view_p.xy();
    }

    const point screen_pos = coordinate - win_min;
    point p;
    if( tile_iso && use_tiles ) {
        const float win_mid_x = win_min.x + win_size.x / 2.0f;
        const float win_mid_y = -win_min.y + win_size.y / 2.0f;
        const int screen_col = std::round( ( screen_pos.x - win_mid_x ) / ( fw / 2.0 ) );
        const int screen_row = std::round( ( screen_pos.y - win_mid_y ) / ( fw / 4.0 ) );
        const point selected( ( screen_col - screen_row ) / 2, ( screen_row + screen_col ) / 2 );
        p = view_offset + selected;
    } else {
        const point selected( screen_pos.x / fw, screen_pos.y / fh );
        p = view_offset + selected - dim.window_size_cell / 2;
    }

    return tripoint( p, g->get_levz() );
}

int get_terminal_width()
{
    return TERMINAL_WIDTH;
}

int get_terminal_height()
{
    return TERMINAL_HEIGHT;
}

int get_scaling_factor()
{
    return scaling_factor;
}

static int map_font_width()
{
    if( use_tiles && tilecontext ) {
        return tilecontext->get_tile_width();
    }
    return ( map_font ? map_font.get() : font.get() )->width;
}

static int map_font_height()
{
    if( use_tiles && tilecontext ) {
        return tilecontext->get_tile_height();
    }
    return ( map_font ? map_font.get() : font.get() )->height;
}

static int overmap_font_width()
{
    return ( overmap_font ? overmap_font.get() : font.get() )->width;
}

static int overmap_font_height()
{
    return ( overmap_font ? overmap_font.get() : font.get() )->height;
}

void to_map_font_dim_width( int &w )
{
    w = ( w * fontwidth ) / map_font_width();
}

void to_map_font_dim_height( int &h )
{
    h = ( h * fontheight ) / map_font_height();
}

void to_map_font_dimension( int &w, int &h )
{
    to_map_font_dim_width( w );
    to_map_font_dim_height( h );
}

void from_map_font_dimension( int &w, int &h )
{
    w = ( w * map_font_width() + fontwidth - 1 ) / fontwidth;
    h = ( h * map_font_height() + fontheight - 1 ) / fontheight;
}

void to_overmap_font_dimension( int &w, int &h )
{
    w = ( w * fontwidth ) / overmap_font_width();
    h = ( h * fontheight ) / overmap_font_height();
}

bool is_draw_tiles_mode()
{
    return use_tiles;
}

/** Saves a screenshot of the current viewport, as a PNG file, to the given location.
* @param file_path: A full path to the file where the screenshot should be saved.
* @returns `true` if the screenshot generation was successful, `false` otherwise.
*/
bool save_screenshot( const std::string &file_path )
{
    // Note: the viewport is returned by SDL and we don't have to manage its lifetime.
    SDL_Rect viewport;

    // Get the viewport size (width and heigth of the screen)
    SDL_RenderGetViewport( renderer.get(), &viewport );

    // Create SDL_Surface with depth of 32 bits (note: using zeros for the RGB masks sets a default value, based on the depth; Alpha mask will be 0).
    SDL_Surface_Ptr surface = CreateRGBSurface( 0, viewport.w, viewport.h, 32, 0, 0, 0, 0 );

    // Get data from SDL_Renderer and save them into surface
    if( printErrorIf( SDL_RenderReadPixels( renderer.get(), nullptr, surface->format->format,
                                            surface->pixels, surface->pitch ) != 0,
                      "save_screenshot: cannot read data from SDL_Renderer." ) ) {
        return false;
    }

    // Save screenshot as PNG file
    if( printErrorIf( IMG_SavePNG( surface.get(), file_path.c_str() ) != 0,
                      std::string( "save_screenshot: cannot save screenshot file: " + file_path ).c_str() ) ) {
        return false;
    }

    return true;
}

#ifdef _WIN32
HWND getWindowHandle()
{
    SDL_SysWMinfo info;
    SDL_VERSION( &info.version );
    SDL_GetWindowWMInfo( ::window.get(), &info );
    return info.info.win.window;
}
#endif

#endif // TILES
