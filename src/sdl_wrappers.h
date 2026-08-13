#pragma once
#ifndef CATA_SRC_SDL_WRAPPERS_H
#define CATA_SRC_SDL_WRAPPERS_H

// IWYU pragma: begin_exports
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
// IWYU pragma: end_exports

#include <atomic>
#include <memory>
#include <string>
#include <vector>

struct point;

// Use CataFlipMode at call sites.
using CataFlipMode = SDL_FlipMode;
// Short aliases for the SDL_KMOD_* modifier masks.
inline constexpr SDL_Keymod KMOD_CTRL  = SDL_KMOD_CTRL;
inline constexpr SDL_Keymod KMOD_SHIFT = SDL_KMOD_SHIFT;
inline constexpr SDL_Keymod KMOD_ALT   = SDL_KMOD_ALT;

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
        SDL_DestroySurface( ptr );
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

// Deferred-disposal list for GPU texture handles that must outlive an
// interrupted or pre-rebuild operation. Each handle's deleter consults a shared
// gate: while the gate is set, SDL_DestroyTexture is skipped because the
// originating renderer is being torn down and reclaims the texture itself.
// drain_live_renderer() destroys on the live renderer; abandon() and the
// destructor release without destroying. Single-threaded, hence a plain vector.
class gpu_handle_graveyard
{
    public:
        using gate = std::shared_ptr<std::atomic<bool>>;

        gpu_handle_graveyard();
        ~gpu_handle_graveyard();
        gpu_handle_graveyard( const gpu_handle_graveyard & ) = delete;
        gpu_handle_graveyard &operator=( const gpu_handle_graveyard & ) = delete;
        gpu_handle_graveyard( gpu_handle_graveyard && ) = default;
        gpu_handle_graveyard &operator=( gpu_handle_graveyard && ) = default;

        // Adopt a raw handle, wrapping it in a shared_ptr that shares this
        // graveyard's current gate.
        void add( SDL_Texture *raw );
        // Adopt a handle already wrapped against this graveyard's gate.
        void adopt( std::shared_ptr<SDL_Texture> handle );
        bool empty() const {
            return handles_.empty();
        }
        // The gate every adopted handle's deleter consults; callers that build
        // handles destined for this graveyard wrap them against it.
        const gate &current_gate() const {
            return gate_;
        }
        // Destroy the handles on the still-live renderer, then re-arm a fresh
        // gate so the graveyard is reusable.
        void drain_live_renderer();
        // Release the handles without SDL_DestroyTexture because the originating
        // renderer is being destroyed, then re-arm a fresh gate.
        void abandon();

    private:
        std::vector<std::shared_ptr<SDL_Texture>> handles_;
        gate gate_;
};

// Wrap a raw texture handle in a shared_ptr whose deleter skips
// SDL_DestroyTexture while `g` is set. Ownership of `raw` transfers to the
// returned handle.
std::shared_ptr<SDL_Texture> make_gated_texture( SDL_Texture *raw, gpu_handle_graveyard::gate g );

namespace cata_shader
{
class variant_pass;
} // namespace cata_shader

// RAII render-target swap: binds `target`, restores the prior target on
// destruction. A non-null `vp` is flushed before the swap so shader
// GPU state is not left bound across the SetRenderTarget transition. Check
// is_valid() before drawing. On any failure here (flush or SDL_SetRenderTarget)
// the ctor raises the global recovery latch, or observes it if another callsite
// already did; boundary_intact() goes false and the caller must abort the frame.
class scoped_render_target
{
    public:
        scoped_render_target( const SDL_Renderer_Ptr &renderer, SDL_Texture *target,
                              cata_shader::variant_pass *vp = nullptr );
        ~scoped_render_target();

        scoped_render_target( const scoped_render_target & ) = delete;
        scoped_render_target &operator=( const scoped_render_target & ) = delete;
        scoped_render_target( scoped_render_target && ) = delete;
        scoped_render_target &operator=( scoped_render_target && ) = delete;

        bool is_valid() const {
            return valid_;
        }

        // False when the renderer may be in an undefined state (see class
        // doc). The global recovery latch is raised by the time this returns
        // false, and the caller must abort the enclosing frame.
        bool boundary_intact() const {
            return boundary_intact_;
        }

        // Eagerly restore the prior target (re-flushing vp). Idempotent:
        // caches its result. Returns false if the scope was never valid, the
        // flush or SDL_SetRenderTarget fails, or recovery was already latched.
        bool restore();

    private:
        // Latch the global recovery embargo and clear boundary_intact_ after an
        // unsafe SDL boundary outcome.
        void mark_boundary_lost();

        SDL_Renderer *renderer_ = nullptr;
        SDL_Texture *prior_target_ = nullptr;
        cata_shader::variant_pass *vp_ = nullptr;
        bool valid_ = false;
        bool restored_ = false;
        bool restore_attempted_ = false;
        bool last_restore_ok_ = false;
        bool boundary_intact_ = true;
};

// Outcome of permanent_render_target_bind. This helper does NOT short-circuit
// on the recovery latch -- the coordinator needs it during rebuild transitions.
// - ok: SDL accepted the bind.
// - refused_pre_switch: a precondition (null renderer) blocked any SDL call;
//   the prior target is still bound and intact.
// - failed_in_switch: variant_pass::flush() failed (undefined shader bind, or
//   the abandoned_pending_rebind_ embargo) OR SDL_SetRenderTarget returned false
//   (SDL may have mutated target state before failing). The renderer is left
//   undefined and the helper raises the recovery latch.
enum class bind_result {
    ok,
    refused_pre_switch,
    failed_in_switch,
};

// Sticky renderer-boundary recovery latch, mirrored here so this header's
// helpers can consult it without the full sdltiles.h chain. Defined in sdltiles.cpp.
bool renderer_boundary_recovery_pending();
// Raise that latch from code without the sdltiles.h chain.
void renderer_boundary_signal_recovery_required();

// Bind a render target permanently, with no auto-restore, for transitions
// where the prior target is not meaningful. Flushes variant_pass
// when `vp` is non-null.
bind_result permanent_render_target_bind( const SDL_Renderer_Ptr &renderer, SDL_Texture *target,
        cata_shader::variant_pass *vp = nullptr );
void RenderClear( const SDL_Renderer_Ptr &renderer );
SDL_Surface_Ptr CreateRGBSurface( Uint32 flags, int width, int height, int depth, Uint32 Rmask,
                                  Uint32 Gmask, Uint32 Bmask, Uint32 Amask );
void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, Uint8 alpha );
void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, Uint8 alpha );
void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *texture,
                   const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                   double angle, const SDL_Point *center, CataFlipMode flip );
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

// SDL exposes displays as SDL_DisplayID arrays. These wrappers present an
// index-based interface instead, mapping internally.
int GetNumVideoDisplays();
const char *GetDisplayName( int displayIndex );
bool GetDesktopDisplayMode( int displayIndex, SDL_DisplayMode *mode );

// SDL_GetRenderDriver returns the driver name directly.
int GetNumRenderDrivers();
const char *GetRenderDriverName( int index );

// Name via SDL_GetRendererName, capabilities via SDL_GetRendererProperties.
const char *GetRendererName( const SDL_Renderer_Ptr &renderer );
bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer );
bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h );

void RenderPresent( const SDL_Renderer_Ptr &renderer );
void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );
// Wraps SDL_SetRenderLogicalPresentation(r, w, h, mode).
// Callers convert event coordinates explicitly (window_to_display_buffer_coords)
// since the input pipeline runs against the window target, not the buffer.
void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, int w, int h );
void RenderSetScale( const SDL_Renderer_Ptr &renderer, float scaleX, float scaleY );
// SDL_RenderReadPixels returns an SDL_Surface*; the wrapper copies data out.
bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       Uint32 format, void *pixels, int pitch );
void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h );
// The texture currently bound as the renderer's target, or NULL for the
// default window target.
SDL_Texture *GetRenderTarget( const SDL_Renderer_Ptr &renderer );

// SDL_GetTicks returns Uint64; the wrapper narrows to uint32_t.
uint32_t GetTicks();

bool IsCursorVisible();
void ShowCursor();
void HideCursor();

// Returns clipboard text as std::string. Handles SDL_free internally.
std::string GetClipboardText();
bool SetClipboardText( const std::string &text );

// SDL reports float coordinates; the wrapper truncates to int.
// Returns window-space coordinates (NOT render-logical); downstream pipeline
// handles scaling separately.
Uint32 GetMouseState( int *x, int *y );

// SDL_GetKeyboardState returns const bool*. Rather than exposing the raw
// array (bool* to Uint8* cast is unsafe), provide a per-scancode query.
bool IsScancodePressed( SDL_Scancode scancode );

// Takes raw SDL_Window* for use with both smart-pointer and raw windows.
void GetWindowSize( SDL_Window *window, int *w, int *h );
void GetWindowSizeInPixels( SDL_Window *window, int *w, int *h );

// Per-texture SDL_SetTextureScaleMode. Accepts game option strings
// ("none"/"nearest"/"linear") and the numeric forms ("0"/"1").
void SetTextureScaleQuality( const SDL_Texture_Ptr &texture, const std::string &quality );
// Store a default scale quality applied by CreateTexture/CreateTextureFromSurface.
void SetDefaultTextureScaleQuality( const std::string &quality );

// Text input is window-scoped; all three take the target SDL_Window*.
void StartTextInput( SDL_Window *window );
void StopTextInput( SDL_Window *window );
bool IsTextInputActive( SDL_Window *window );

// Use these instead of raw SDL flags at call sites. SDL_WINDOW_FULLSCREEN is
// borderless; there is no separate fullscreen-desktop flag.
inline constexpr Uint32 CATA_WINDOW_HIDDEN    = SDL_WINDOW_HIDDEN;
inline constexpr Uint32 CATA_WINDOW_RESIZABLE = SDL_WINDOW_RESIZABLE;
inline constexpr Uint32 CATA_WINDOW_MAXIMIZED = SDL_WINDOW_MAXIMIZED;
inline constexpr Uint32 CATA_WINDOW_HIGH_DPI  = SDL_WINDOW_HIGH_PIXEL_DENSITY;

// Creates a window centered on the given display. Uses CATA_WINDOW_* flags.
// No fullscreen flags -- call SetWindowFullscreen after creation for that.
// Uses SDL_CreateWindowWithProperties to handle maximized+display placement.
SDL_Window_Ptr CreateGameWindow( const char *title, int display, int w, int h, Uint32 flags );

enum class FullscreenMode { windowed, fullscreen_desktop, fullscreen_exclusive };
// Maps to SDL_SetWindowFullscreen(bool) + SDL_SetWindowFullscreenMode, then
// calls SDL_SyncWindow so the state is settled before returning.
bool SetWindowFullscreen( SDL_Window *window, FullscreenMode mode );

// All call SDL_SyncWindow for async-safe behavior.
void RestoreWindow( SDL_Window *window );
void SetWindowSize( SDL_Window *window, int w, int h );
void SetWindowMinimumSize( SDL_Window *window, int w, int h );
void SetWindowTitle( SDL_Window *window, const char *title );

// Takes a driver name rather than an index; vsync via SDL_SetRenderVSync.
// When software == true, passes "software" as the driver name.
SDL_Renderer_Ptr CreateRenderer( const SDL_Window_Ptr &window, const char *driver_name,
                                 bool software, bool vsync );

// Touch finger coordinates. SDL_EVENT_FINGER_* events carry normalized [0,1]
// values; the wrappers multiply by the supplied window dimension to recover
// window-pixel coordinates.
float GetFingerX( const SDL_Event &ev, int windowWidth );
float GetFingerY( const SDL_Event &ev, int windowHeight );

/**@}*/

// Window events are top-level SDL_EVENT_WINDOW_* constants.

// Returns true if the event is a window event.
bool IsWindowEvent( const SDL_Event &ev );
// Returns the window event subtype for use in switch statements.
Uint32 GetWindowEventID( const SDL_Event &ev );

// Normalized window event constants. Use with switch(GetWindowEventID(ev)).
inline constexpr Uint32 CATA_WINDOWEVENT_SHOWN        = SDL_EVENT_WINDOW_SHOWN;
inline constexpr Uint32 CATA_WINDOWEVENT_EXPOSED      = SDL_EVENT_WINDOW_EXPOSED;
inline constexpr Uint32 CATA_WINDOWEVENT_MINIMIZED    = SDL_EVENT_WINDOW_MINIMIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESTORED     = SDL_EVENT_WINDOW_RESTORED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESIZED      = SDL_EVENT_WINDOW_RESIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_LOST   = SDL_EVENT_WINDOW_FOCUS_LOST;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_GAINED = SDL_EVENT_WINDOW_FOCUS_GAINED;
inline constexpr Uint32 CATA_WINDOWEVENT_SAFE_AREA_CHANGED = SDL_EVENT_WINDOW_SAFE_AREA_CHANGED;

inline constexpr Uint32 CATA_RENDER_TARGETS_RESET = SDL_EVENT_RENDER_TARGETS_RESET;

// Renderer device-reset/lost and mobile lifecycle event constants.
inline constexpr Uint32 CATA_RENDER_DEVICE_RESET = SDL_EVENT_RENDER_DEVICE_RESET;
inline constexpr Uint32 CATA_RENDER_DEVICE_LOST = SDL_EVENT_RENDER_DEVICE_LOST;
inline constexpr Uint32 CATA_APP_DIDENTERFOREGROUND = SDL_EVENT_DID_ENTER_FOREGROUND;
inline constexpr Uint32 CATA_APP_WILLENTERBACKGROUND = SDL_EVENT_WILL_ENTER_BACKGROUND;
inline constexpr Uint32 CATA_APP_DIDENTERBACKGROUND = SDL_EVENT_DID_ENTER_BACKGROUND;
inline constexpr Uint32 CATA_WINDOWEVENT_PIXEL_SIZE_CHANGED = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;

// Touch finger ID accessor.
inline SDL_FingerID GetFingerID( const SDL_Event &ev )
{
    return ev.tfinger.fingerID;
}

inline constexpr Uint32 CATA_FINGERMOTION = SDL_EVENT_FINGER_MOTION;
inline constexpr Uint32 CATA_FINGERDOWN   = SDL_EVENT_FINGER_DOWN;
inline constexpr Uint32 CATA_FINGERUP     = SDL_EVENT_FINGER_UP;

inline constexpr Uint32 CATA_KEYDOWN         = SDL_EVENT_KEY_DOWN;
inline constexpr Uint32 CATA_KEYUP           = SDL_EVENT_KEY_UP;
inline constexpr Uint32 CATA_TEXTINPUT       = SDL_EVENT_TEXT_INPUT;
inline constexpr Uint32 CATA_TEXTEDITING     = SDL_EVENT_TEXT_EDITING;
inline constexpr Uint32 CATA_MOUSEMOTION     = SDL_EVENT_MOUSE_MOTION;
inline constexpr Uint32 CATA_MOUSEBUTTONDOWN = SDL_EVENT_MOUSE_BUTTON_DOWN;
inline constexpr Uint32 CATA_MOUSEBUTTONUP   = SDL_EVENT_MOUSE_BUTTON_UP;
inline constexpr Uint32 CATA_MOUSEWHEEL      = SDL_EVENT_MOUSE_WHEEL;
inline constexpr Uint32 CATA_QUIT            = SDL_EVENT_QUIT;

// Key events carry ev.key.key, ev.key.mod and ev.key.scancode directly.
// CataKeysym bundles the three into one accessor result.
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

// Returns are kept raw (void* / const char*) so <jni.h> does not leak into
// this header.
#if defined(__ANDROID__)
inline void *GetAndroidJNIEnv()
{
    return SDL_GetAndroidJNIEnv();
}

inline void *GetAndroidActivity()
{
    return SDL_GetAndroidActivity();
}

inline const char *GetAndroidExternalStoragePath()
{
    return SDL_GetAndroidExternalStoragePath();
}
#endif // __ANDROID__

#endif // CATA_SRC_SDL_WRAPPERS_H
