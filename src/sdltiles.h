#pragma once
#ifndef CATA_SRC_SDLTILES_H
#define CATA_SRC_SDLTILES_H

#include "point.h" // IWYU pragma: keep

namespace catacurses
{
class window;
} // namespace catacurses

#if defined(TILES)

#include <cstdint>
#include <memory>
#include <string>

#include "color_loader.h"
#include "coords_fwd.h"
#include "sdl_wrappers.h"
#include "string_id.h"

#if defined(__APPLE__)
// For TARGET_OS_IPHONE macro to test if is on iOS
#include <TargetConditionals.h>
#endif

class cata_tiles;

struct weather_type;

using weather_type_id = string_id<weather_type>;

namespace catacurses
{
class window;
} // namespace catacurses

extern std::shared_ptr<cata_tiles> tilecontext;
extern std::shared_ptr<cata_tiles> closetilecontext;
extern std::shared_ptr<cata_tiles> fartilecontext;
extern std::unique_ptr<cata_tiles> overmap_tilecontext;
extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;
extern int fontheight;
extern int fontwidth;

// This function may refresh the screen, so it should not be used where tiles
// may be displayed. Actually, this is supposed to be called from init.cpp,
// and only from there.
void load_tileset();
void rescale_tileset( int size );
bool save_screenshot( const std::string &file_path );
void toggle_fullscreen_window();

struct window_dimensions {
    point scaled_font_size;
    point window_pos_cell;
    point window_size_cell;
    point window_pos_pixel;
    point window_size_pixel;
};
window_dimensions get_window_dimensions( const catacurses::window &win );
// Get dimensional info of an imaginary normal catacurses::window with the given
// position and size. Unlike real catacurses::window, size can be zero.
window_dimensions get_window_dimensions( const point &pos, const point &size );

const SDL_Renderer_Ptr &get_sdl_renderer();
// Clears the SDL renderer to black. Returns false without clearing when a
// recovery/pause/resize is queued or the buffer bind failed, so the caller can
// keep the clear request armed for a later frame.
bool clear_sdl_window();
// Returns the main game window. Needed by text input wrappers and other
// SDL3 APIs that require an explicit window parameter.
SDL_Window *get_sdl_window();
// Dimensions of the terminal-sized display_buffer (logical game pixels),
// queried from the SDL texture. Differs from window output under SCALING_FACTOR
// or android letterboxing. Zero before the buffer exists.
void get_display_buffer_dims( int *w, int *h );

// Map window coordinates to display_buffer (terminal) pixel coordinates.
// Returns the input unchanged when either dimension source is unavailable.
SDL_Point window_to_display_buffer_coords( SDL_Point window_pt );

// Convert a coord-bearing mouse event (motion/button/wheel) in place to
// display_buffer coordinates. Touch events are left in window coordinates: the
// android shortcut overlay and virtual joystick hit-test against the window.
void convert_event_to_display_buffer_coords( SDL_Event *event );

namespace cata_shader
{
class variant_pass;
} // namespace cata_shader

#if SDL_MAJOR_VERSION >= 3
// Process-lifetime variant_pass owned alongside the renderer (WinCreate to
// WinDestroy). One shared handle so a renderer recreate updates a single pass,
// not per-context copies.
cata_shader::variant_pass *get_shared_variant_pass();
#endif

// True while the active scope failed to bind the buffer target. Per-scope;
// consult before drawing so nothing paints onto an unknown SDL target.
bool display_buffer_scope_is_invalid();

// Sticky renderer-boundary recovery latch (failure sources: see bind_result in
// sdl_wrappers.h). Once set, scope ctors refuse the bind until the coordinator
// clears it. Sticky across scopes, unlike display_buffer_scope_is_invalid.
bool display_buffer_scope_recovery_pending();

// Raise the recovery latch from outside a draw scope (e.g. the present-time
// bind in refresh_display); most helper failures already latch internally.
void display_buffer_scope_signal_recovery_required();

// Clear the latch once the poisoned renderer is gone and a fresh one is wired.
void display_buffer_scope_clear_recovery_required();

// True when a draw must skip the backend paint, for any of: a queued recovery,
// the app paused or resuming, a pending resize, or a latched draw-scope boundary
// failure. Thin accessor so shared redraw paths skip the GPU paint without the
// coordinator header. Callers below reference this set rather than re-listing it.
bool renderer_should_abort_frame();

// Monotonic count of completed renderer-resource rebuilds. Saved alongside any
// retained renderer state (e.g. a clip rect) so a later restore can detect that
// a recovery rebuilt the renderer in between and skip the now-stale restore.
uint64_t renderer_resource_generation();

// Nestable RAII guard: binds display_buffer on entry, restores the idle null
// target on exit. Only the outermost switches; nested scopes are no-ops. On a
// failed bind the scope is inactive and display_buffer_scope_is_invalid() is
// true so draw entry points skip. A caller hitting a mid-scope shader failure
// must call abort_unbind() so the destructor does not switch target with shader
// state still bound.
class display_buffer_draw_scope
{
    public:
        // Default draws refuse the bind when renderer_should_abort_frame, so no
        // SDL_SetRenderTarget runs on a paused or about-to-be-rebuilt renderer.
        // The recovery-blank step passes allow_during_recovery to bind during its
        // own gated work, where the abort latch is deliberately still raised.
        explicit display_buffer_draw_scope( bool allow_during_recovery = false );
        ~display_buffer_draw_scope();
        display_buffer_draw_scope( const display_buffer_draw_scope & ) = delete;
        display_buffer_draw_scope &operator=( const display_buffer_draw_scope & ) = delete;
        display_buffer_draw_scope( display_buffer_draw_scope && ) = delete;
        display_buffer_draw_scope &operator=( display_buffer_draw_scope && ) = delete;

        // Skip the outermost destructor's switch back to the null target.
        // Used after a variant_pass::flush failure to honor the shader
        // bind boundary.
        void abort_unbind();

        // True when the caller should draw inside this scope: the bind succeeded
        // and renderer_should_abort_frame is clear. Folds the bind-validity check
        // and the post-bind abort recheck into one call so a draw path cannot skip
        // the recheck.
        bool should_draw() const;

    private:
        bool outermost_ = false;
        bool active_ = false;
};

#endif // TILES

// Text level, valid only for a point relative to the window, not a point in overall space.
bool window_contains_point_relative( const catacurses::window &win, const point &p );
#endif // CATA_SRC_SDLTILES_H
