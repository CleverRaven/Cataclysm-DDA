#pragma once
#ifndef CATA_SRC_SDL_WRAPPERS_H
#define CATA_SRC_SDL_WRAPPERS_H

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
// IWYU pragma: begin_exports
#if defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL.h>
#   include <SDL2/SDL_image.h>
#   include <SDL2/SDL_ttf.h>
#   include <SDL2/SDL_mouse.h>
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#   include <SDL.h>
#pragma GCC diagnostic pop
#   include <SDL_image.h>
#   include <SDL_ttf.h>
#   include <SDL_mouse.h>
#endif
// IWYU pragma: end_exports

#include <memory>
#include <string>

struct point;

struct SDL_Renderer_deleter {
    void operator()( SDL_Renderer *const renderer ) {
        SDL_DestroyRenderer( renderer );
    }
};
using SDL_Renderer_Ptr = std::unique_ptr<SDL_Renderer, SDL_Renderer_deleter>;

struct SDL_Window_deleter {
    void operator()( SDL_Window *const window ) {
        SDL_DestroyWindow( window );
    }
};
using SDL_Window_Ptr = std::unique_ptr<SDL_Window, SDL_Window_deleter>;

struct SDL_Texture_deleter {
    void operator()( SDL_Texture *const ptr ) {
        SDL_DestroyTexture( ptr );
    }
};
using SDL_Texture_Ptr = std::unique_ptr<SDL_Texture, SDL_Texture_deleter>;

struct SDL_Surface_deleter {
    void operator()( SDL_Surface *const ptr ) {
        SDL_FreeSurface( ptr );
    }
};
using SDL_Surface_Ptr = std::unique_ptr<SDL_Surface, SDL_Surface_deleter>;

struct TTF_Font_deleter {
    void operator()( TTF_Font *const font ) {
        TTF_CloseFont( font );
    }
};
using TTF_Font_Ptr = std::unique_ptr<TTF_Font, TTF_Font_deleter>;
/**
 * If the @p condition is `true`, an error (including the given @p message
 * and the output of @ref SDL_GetError) is logged to the debug log.
 * @returns \p condition, in other words: return whether an error was logged.
 */
bool printErrorIf( bool condition, const char *message );
/**
 * If the @p condition is `true`, an exception (including the given @p message
 * and the output of @ref SDL_GetError) is thrown.
 */
void throwErrorIf( bool condition, const char *message );
/**
 * Wrappers for SDL functions that does error reporting and that accept our
 * wrapped pointers.
 * Errors are reported via the usual debug log stream (exceptions are noted below).
 *
 * @ref CreateTextureFromSurface returns an empty `SDL_Texture_Ptr` if the function
 * fails (the failure is also logged by the function).
 * @ref load_image throws if the loading fails. Its input must be a valid C-String.
 */
/**@{*/
void RenderCopy( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture,
                 const SDL_Rect *srcrect, const SDL_Rect *dstrect );
SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h );
SDL_Texture_Ptr CreateTextureFromSurface( const SDL_Renderer_Ptr &renderer,
        const SDL_Surface_Ptr &surface );
void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p );
void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *rect, Uint32 color );
void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode );
void SetTextureBlendMode( const std::shared_ptr<SDL_Texture> &texture, SDL_BlendMode blendMode );
bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b );
bool SetTextureColorMod( const std::shared_ptr<SDL_Texture> &texture, Uint32 r, Uint32 g,
                         Uint32 b );
void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode blendMode );
void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode );
SDL_Surface_Ptr load_image( const char *path );
bool SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture );
void RenderClear( const SDL_Renderer_Ptr &renderer );
SDL_Surface_Ptr CreateRGBSurface( Uint32 flags, int width, int height, int depth, Uint32 Rmask,
                                  Uint32 Gmask, Uint32 Bmask, Uint32 Amask );
void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, Uint8 alpha );
void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, Uint8 alpha );
void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *texture,
                   const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                   double angle, const SDL_Point *center, SDL_RendererFlip flip );
void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );
bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer );
int BlitSurface( const SDL_Surface_Ptr &src, const SDL_Rect *srcrect,
                 const SDL_Surface_Ptr &dst, SDL_Rect *dstrect );
Uint32 MapRGB( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b );
Uint32 MapRGBA( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
void GetRGBA( Uint32 pixel, const SDL_Surface_Ptr &surface, Uint8 &r, Uint8 &g, Uint8 &b,
              Uint8 &a );
int SetColorKey( const SDL_Surface_Ptr &surface, int flag, Uint32 key );
int SetSurfaceRLE( const SDL_Surface_Ptr &surface, int flag );
int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, SDL_BlendMode blendMode );
SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, Uint32 pixel_format );
int LockSurface( const SDL_Surface_Ptr &surface );
void UnlockSurface( const SDL_Surface_Ptr &surface );
// Returns the pixel format enum (SDL_PIXELFORMAT_*) for the surface.
// SDL3: surface->format is the enum directly; SDL2: surface->format->format.
Uint32 GetSurfacePixelFormat( const SDL_Surface_Ptr &surface );
TTF_Font_Ptr OpenFontIndex( const char *file, int ptsize, int64_t index );
const char *FontFaceStyleName( const TTF_Font_Ptr &font );
int FontFaces( const TTF_Font_Ptr &font );
int FontHeight( const TTF_Font_Ptr &font );
void SetFontStyle( const TTF_Font_Ptr &font, int style );
SDL_Surface_Ptr RenderUTF8_Solid( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );
SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );
// Project-level helper: can this font produce a glyph for the given codepoint?
// In SDL3_ttf there is no direct TTF_GlyphIsProvided equivalent; this will be
// emulated via glyph metrics or a render attempt.
bool CanRenderGlyph( const TTF_Font_Ptr &font, Uint32 ch );

// SDL3: index-based API replaced by SDL_DisplayID arrays. Wrappers
// present the SDL2-style index interface, mapping internally on SDL3.
int GetNumVideoDisplays();
const char *GetDisplayName( int displayIndex );
bool GetDesktopDisplayMode( int displayIndex, SDL_DisplayMode *mode );

// SDL3: SDL_GetRenderDriverInfo removed; SDL_GetRenderDriver returns name directly.
int GetNumRenderDrivers();
const char *GetRenderDriverName( int index );

// SDL3: SDL_RendererInfo struct removed. Name via SDL_GetRendererName,
// capabilities via SDL_GetRendererProperties.
const char *GetRendererName( const SDL_Renderer_Ptr &renderer );
bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer );
bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h );

// SDL3: various renames, signature changes, behavioral changes.
void RenderPresent( const SDL_Renderer_Ptr &renderer );
void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );
// SDL3: replaced by SDL_SetRenderLogicalPresentation(r, w, h, mode).
// Callers must also use ConvertEventCoordinates (SDL3 does not auto-scale events).
void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, int w, int h );
void RenderSetScale( const SDL_Renderer_Ptr &renderer, float scaleX, float scaleY );
// SDL3: returns SDL_Surface* instead of filling a buffer. Wrapper copies data out.
bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       Uint32 format, void *pixels, int pitch );
// SDL3: renamed to SDL_GetCurrentRenderOutputSize.
void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h );

// SDL3: SDL_GetTicks returns Uint64. Wrapper keeps uint32_t for source compat.
uint32_t GetTicks();

// SDL3: SDL_ShowCursor split into SDL_ShowCursor/SDL_HideCursor/SDL_CursorVisible.
bool IsCursorVisible();
void ShowCursor();
void HideCursor();

// Returns clipboard text as std::string. Handles SDL_free internally on both versions.
std::string GetClipboardText();
bool SetClipboardText( const std::string &text );

// SDL3: returns float coordinates. Wrapper truncates to int.
// Returns window-space coordinates (NOT render-logical); downstream pipeline
// handles scaling separately.
Uint32 GetMouseState( int *x, int *y );

// SDL3: SDL_GetKeyboardState returns const bool*. Rather than exposing the raw
// array (bool* to Uint8* cast is unsafe), provide a per-scancode query.
bool IsScancodePressed( SDL_Scancode scancode );

// Takes raw SDL_Window* for use with both smart-pointer and raw windows.
void GetWindowSize( SDL_Window *window, int *w, int *h );
// Falls back to GetWindowSize on SDL2 < 2.26.
void GetWindowSizeInPixels( SDL_Window *window, int *w, int *h );

// Replaces SDL_HINT_RENDER_SCALE_QUALITY with per-texture SDL_SetTextureScaleMode
// (available in SDL2 2.0.12+ and SDL3). Accepts game option strings
// ("none"/"nearest"/"linear") and SDL2 hint values ("0"/"1").
void SetTextureScaleQuality( const SDL_Texture_Ptr &texture, const std::string &quality );
// Store a default scale quality applied by CreateTexture/CreateTextureFromSurface.
void SetDefaultTextureScaleQuality( const std::string &quality );

// SDL3: all three take SDL_Window*. SDL2 versions ignore the parameter.
void StartTextInput( SDL_Window *window );
void StopTextInput( SDL_Window *window );
bool IsTextInputActive( SDL_Window *window );

// Use these instead of raw SDL flags at call sites. Raw macros like
// SDL_WINDOW_ALLOW_HIGHDPI / SDL_WINDOW_FULLSCREEN_DESKTOP may not exist
// in SDL3 headers.
// SDL3: SDL_WINDOW_ALLOW_HIGHDPI -> SDL_WINDOW_HIGH_PIXEL_DENSITY
// SDL3: SDL_WINDOW_FULLSCREEN_DESKTOP removed; SDL_WINDOW_FULLSCREEN is borderless
#if SDL_MAJOR_VERSION >= 3
inline constexpr Uint32 CATA_WINDOW_HIDDEN    = SDL_WINDOW_HIDDEN;
inline constexpr Uint32 CATA_WINDOW_RESIZABLE = SDL_WINDOW_RESIZABLE;
inline constexpr Uint32 CATA_WINDOW_MAXIMIZED = SDL_WINDOW_MAXIMIZED;
inline constexpr Uint32 CATA_WINDOW_HIGH_DPI  = SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
inline constexpr Uint32 CATA_WINDOW_HIDDEN    = SDL_WINDOW_HIDDEN;
inline constexpr Uint32 CATA_WINDOW_RESIZABLE = SDL_WINDOW_RESIZABLE;
inline constexpr Uint32 CATA_WINDOW_MAXIMIZED = SDL_WINDOW_MAXIMIZED;
inline constexpr Uint32 CATA_WINDOW_HIGH_DPI  = SDL_WINDOW_ALLOW_HIGHDPI;
#endif

// Creates a window centered on the given display. Uses CATA_WINDOW_* flags.
// No fullscreen flags -- call SetWindowFullscreen after creation for that.
// SDL3: uses SDL_CreateWindowWithProperties to handle maximized+display placement.
SDL_Window_Ptr CreateGameWindow( const char *title, int display, int w, int h, Uint32 flags );

enum class FullscreenMode { windowed, fullscreen_desktop, fullscreen_exclusive };
// SDL3: maps to SDL_SetWindowFullscreen(bool) + SDL_SetWindowFullscreenMode.
// Calls SDL_SyncWindow on SDL3 to ensure state is settled before returning.
bool SetWindowFullscreen( SDL_Window *window, FullscreenMode mode );

// All call SDL_SyncWindow on SDL3 for async-safe behavior.
void RestoreWindow( SDL_Window *window );
void SetWindowSize( SDL_Window *window, int w, int h );
void SetWindowMinimumSize( SDL_Window *window, int w, int h );
void SetWindowTitle( SDL_Window *window, const char *title );

// SDL3: takes name string instead of index, flags removed. Vsync via SDL_SetRenderVSync.
// When software == true, SDL3 path passes "software" as driver name.
SDL_Renderer_Ptr CreateRenderer( const SDL_Window_Ptr &window, const char *driver_name,
                                 bool software, bool vsync );

// SDL2: no-op (logical size auto-scales events).
// SDL3: calls SDL_ConvertEventToRenderCoordinates on all event types.
void ConvertEventCoordinates( const SDL_Renderer_Ptr &renderer, SDL_Event *event );

// SDL2: finger coords are normalized [0,1]; multiply by window dimension.
// SDL3: after ConvertEventCoordinates, coords are already absolute render-logical.
float GetFingerX( const SDL_Event &ev, int windowWidth );
float GetFingerY( const SDL_Event &ev, int windowHeight );

/**@}*/

// SDL2 nests window events under SDL_WINDOWEVENT with subtypes in ev.window.event.
// SDL3 flattens them to top-level SDL_EVENT_WINDOW_* constants.

// Returns true if the event is a window event.
bool IsWindowEvent( const SDL_Event &ev );
// Returns the window event subtype for use in switch statements.
Uint32 GetWindowEventID( const SDL_Event &ev );

// Normalized window event constants. Use with switch(GetWindowEventID(ev)).
#if SDL_MAJOR_VERSION >= 3
inline constexpr Uint32 CATA_WINDOWEVENT_SHOWN        = SDL_EVENT_WINDOW_SHOWN;
inline constexpr Uint32 CATA_WINDOWEVENT_EXPOSED      = SDL_EVENT_WINDOW_EXPOSED;
inline constexpr Uint32 CATA_WINDOWEVENT_MINIMIZED    = SDL_EVENT_WINDOW_MINIMIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESTORED     = SDL_EVENT_WINDOW_RESTORED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESIZED      = SDL_EVENT_WINDOW_RESIZED;
// SIZE_CHANGED removed in SDL3; use RESIZED instead.
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_LOST   = SDL_EVENT_WINDOW_FOCUS_LOST;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_GAINED = SDL_EVENT_WINDOW_FOCUS_GAINED;
#else
inline constexpr Uint32 CATA_WINDOWEVENT_SHOWN        = SDL_WINDOWEVENT_SHOWN;
inline constexpr Uint32 CATA_WINDOWEVENT_EXPOSED      = SDL_WINDOWEVENT_EXPOSED;
inline constexpr Uint32 CATA_WINDOWEVENT_MINIMIZED    = SDL_WINDOWEVENT_MINIMIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESTORED     = SDL_WINDOWEVENT_RESTORED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESIZED      = SDL_WINDOWEVENT_RESIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_LOST   = SDL_WINDOWEVENT_FOCUS_LOST;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_GAINED = SDL_WINDOWEVENT_FOCUS_GAINED;
#endif

#if SDL_MAJOR_VERSION >= 3
inline constexpr Uint32 CATA_RENDER_TARGETS_RESET = SDL_EVENT_RENDER_TARGETS_RESET;
#else
inline constexpr Uint32 CATA_RENDER_TARGETS_RESET = SDL_RENDER_TARGETS_RESET;
#endif

// Touch finger ID accessor. SDL3 renames fingerId -> fingerID.
inline SDL_FingerID GetFingerID( const SDL_Event &ev )
{
#if SDL_MAJOR_VERSION >= 3
    return ev.tfinger.fingerID;
#else
    return ev.tfinger.fingerId;
#endif
}

// Touch event renames. SDL3: SDL_FINGER* -> SDL_EVENT_FINGER_*.
#if SDL_MAJOR_VERSION >= 3
inline constexpr Uint32 CATA_FINGERMOTION = SDL_EVENT_FINGER_MOTION;
inline constexpr Uint32 CATA_FINGERDOWN   = SDL_EVENT_FINGER_DOWN;
inline constexpr Uint32 CATA_FINGERUP     = SDL_EVENT_FINGER_UP;
#else
inline constexpr Uint32 CATA_FINGERMOTION = SDL_FINGERMOTION;
inline constexpr Uint32 CATA_FINGERDOWN   = SDL_FINGERDOWN;
inline constexpr Uint32 CATA_FINGERUP     = SDL_FINGERUP;
#endif

// SDL3 removes SDL_Keysym from key events: ev.key.keysym.sym -> ev.key.key,
// ev.key.keysym.mod -> ev.key.mod, ev.key.keysym.scancode -> ev.key.scancode.
// CataKeysym provides a version-independent accessor.
struct CataKeysym {
    SDL_Keycode sym;
    Uint16 mod;
    SDL_Scancode scancode;
};
CataKeysym GetKeysym( const SDL_Event &ev );

/**
 * Comparison operators which SDL lacks being a C-ish lib.
 */
/**@{*/

inline bool operator==( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return
        lhs.r == rhs.r &&
        lhs.g == rhs.g &&
        lhs.b == rhs.b &&
        lhs.a == rhs.a;
}

inline bool operator!=( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return !operator==( lhs, rhs );
}

inline bool operator==( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return
        lhs.x == rhs.x &&
        lhs.y == rhs.y &&
        lhs.w == rhs.w &&
        lhs.h == rhs.h;
}

inline bool operator!=( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return !operator==( lhs, rhs );
}

/**@}*/

#endif // CATA_SRC_SDL_WRAPPERS_H
