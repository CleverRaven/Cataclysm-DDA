#include "ui_manager.h"

#include <functional>
#include <iterator>
#include <optional>
#include <vector>

#include "cached_options.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cursesdef.h"
#include "game_ui.h"
#include "point.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "cata_imgui.h"

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

using ui_stack_t = std::vector<std::reference_wrapper<ui_adaptor>>;

static bool imgui_frame_started = false;
static bool redraw_in_progress = false;
static bool showing_debug_message = false;
static bool restart_redrawing = false;
#if defined( TILES )
static std::optional<SDL_Rect> prev_clip_rect;
#endif
static ui_stack_t ui_stack;

ui_adaptor::ui_adaptor() : is_imgui( false ), disabling_uis_below( false ),
    is_debug_message_ui( false ),
    invalidated( false ), deferred_resize( false )
{
    ui_stack.emplace_back( *this );
}

ui_adaptor::ui_adaptor( ui_adaptor::disable_uis_below ) : is_imgui( false ),
    disabling_uis_below( true ),
    is_debug_message_ui( false ), invalidated( false ), deferred_resize( false )
{
    ui_stack.emplace_back( *this );
}

ui_adaptor::ui_adaptor( ui_adaptor::debug_message_ui ) : is_imgui( false ),
    disabling_uis_below( true ),
    is_debug_message_ui( true ), invalidated( false ), deferred_resize( false )
{
    cata_assert( !showing_debug_message );
    showing_debug_message = true;
    if( redraw_in_progress ) {
        restart_redrawing = true;
    }
#if defined( TILES )
    // Reset the clip rect because the debug message UI might be created in a
    // redraw callback when a clip rect is active. When the UI is deconstructed,
    // restore the previous clip rect to prevent the redraw callback from
    // drawing outside the clip area, which will cause stuck graphics. This
    // alone does not prevent the graphics from becoming borked in other ways,
    // but `ui_manager` will redo the entire redrawing as soon as the redraw
    // callback returns.
    const SDL_Renderer_Ptr &renderer = get_sdl_renderer();
    if( SDL_RenderIsClipEnabled( renderer.get() ) ) {
        prev_clip_rect = SDL_Rect();
        SDL_RenderGetClipRect( renderer.get(), &prev_clip_rect.value() );
        SDL_RenderSetClipRect( renderer.get(), nullptr );
    } else {
        prev_clip_rect = std::nullopt;
    }
#endif
    ui_stack.emplace_back( *this );
}

ui_adaptor::~ui_adaptor()
{
    if( is_debug_message_ui ) {
        cata_assert( showing_debug_message );
        showing_debug_message = false;
#if defined( TILES )
        // See ui_adaptor( debug_message_ui )
        if( prev_clip_rect.has_value() ) {
            const SDL_Renderer_Ptr &renderer = get_sdl_renderer();
            SDL_RenderSetClipRect( renderer.get(), &prev_clip_rect.value() );
        }
#endif
    }
    for( auto it = ui_stack.rbegin(); it < ui_stack.rend(); ++it ) {
        if( &it->get() == this ) {
            ui_stack.erase( std::prev( it.base() ) );
            // TODO avoid invalidating portions that do not need to be redrawn
            ui_manager::invalidate( dimensions, disabling_uis_below );
            break;
        }
    }
}

void ui_adaptor::position_from_window( const catacurses::window &win )
{
    if( !win ) {
        position( point_zero, point_zero );
    } else {
        const rectangle<point> old_dimensions = dimensions;
        // ensure position is updated before calling invalidate
#ifdef TILES
        const window_dimensions dim = get_window_dimensions( win );
        dimensions = rectangle<point>(
                         dim.window_pos_pixel, dim.window_pos_pixel + dim.window_size_pixel );
#else
        const point origin( getbegx( win ), getbegy( win ) );
        dimensions = rectangle<point>( origin, origin + point( getmaxx( win ), getmaxy( win ) ) );
#endif
        invalidated = true;
        ui_manager::invalidate( old_dimensions, false );
    }
}

void ui_adaptor::position( const point &topleft, const point &size )
{
    const rectangle<point> old_dimensions = dimensions;
    // ensure position is updated before calling invalidate
#ifdef TILES
    const window_dimensions dim = get_window_dimensions( topleft, size );
    dimensions = rectangle<point>( dim.window_pos_pixel,
                                   dim.window_pos_pixel + dim.window_size_pixel );
#else
    dimensions = rectangle<point>( topleft, topleft + size );
#endif
    invalidated = true;
    ui_manager::invalidate( old_dimensions, false );
}

void ui_adaptor::position_absolute( const point &topleft, const point &size )
{
    const rectangle<point> old_dimensions = dimensions;
    // ensure position is updated before calling invalidate
    dimensions = rectangle<point>( topleft, topleft + size );
    invalidated = true;
    ui_manager::invalidate( old_dimensions, false );
}

void ui_adaptor::on_redraw( const redraw_callback_t &fun )
{
    redraw_cb = fun;
}

void ui_adaptor::on_screen_resize( const screen_resize_callback_t &fun )
{
    screen_resized_cb = fun;
}

void ui_adaptor::set_cursor( const catacurses::window &w, const point &pos )
{
#if !defined( TILES )
    cursor_type = cursor::custom;
    cursor_pos = point( getbegx( w ), getbegy( w ) ) + pos;
#else
    // Unimplemented
    cursor_type = cursor::disabled;
    static_cast<void>( w );
    static_cast<void>( pos );
#endif
}

void ui_adaptor::record_cursor( const catacurses::window &w )
{
#if !defined( TILES )
    cursor_type = cursor::custom;
    cursor_pos = point( getbegx( w ) + getcurx( w ), getbegy( w ) + getcury( w ) );
#else
    // Unimplemented
    cursor_type = cursor::disabled;
    static_cast<void>( w );
#endif
}

void ui_adaptor::record_term_cursor()
{
#if !defined( TILES ) && !defined(_MSC_VER)
    cursor_type = cursor::custom;
    cursor_pos = point( getcurx( catacurses::newscr ), getcury( catacurses::newscr ) );
#else
    // Unimplemented
    cursor_type = cursor::disabled;
#endif
}

void ui_adaptor::default_cursor()
{
#if !defined( TILES )
    cursor_type = cursor::last;
#else
    // Unimplemented
    cursor_type = cursor::disabled;
#endif
}

void ui_adaptor::disable_cursor()
{
    cursor_type = cursor::disabled;
}

static void restore_cursor( const point &p )
{
#if !defined( TILES ) && !defined(_MSC_VER)
    wmove( catacurses::newscr, p );
#else
    static_cast<void>( p );
#endif
}

void ui_adaptor::mark_resize() const
{
    deferred_resize = true;
}

static bool contains( const rectangle<point> &lhs, const rectangle<point> &rhs )
{
    return rhs.p_min.x >= lhs.p_min.x && rhs.p_max.x <= lhs.p_max.x &&
           rhs.p_min.y >= lhs.p_min.y && rhs.p_max.y <= lhs.p_max.y;
}

static bool overlap( const rectangle<point> &lhs, const rectangle<point> &rhs )
{
    return lhs.p_min.x < rhs.p_max.x && lhs.p_min.y < rhs.p_max.y &&
           rhs.p_min.x < lhs.p_max.x && rhs.p_min.y < lhs.p_max.y;
}

// This function does two things:
// 1. Ensure that any UI that would be overwritten by redrawing a lower invalidated
//    UI also gets redrawn.
// 2. Optimize the invalidated flag so completely occluded UIs will not be redrawn.
//
// The current implementation may still invalidate UIs that in fact do not need to
// be redrawn, but all UIs that need to be redrawn are guaranteed to be invalidated.
void ui_adaptor::invalidation_consistency_and_optimization()
{
    // Only ensure consistency and optimize for UIs not disabled by another UI
    // with `disable_uis_below`, since if a UI is disabled, it does not get
    // resized or redrawn, so the invalidation flag is not cleared, and including
    // the disabled UI in the following calculation would unnecessarily
    // invalidate any upper intersecting UIs.
    auto rfirst = ui_stack.crbegin();
    for( ; rfirst != ui_stack.crend(); ++rfirst ) {
        if( rfirst->get().disabling_uis_below ) {
            break;
        }
    }
    const auto first = rfirst == ui_stack.crend() ? ui_stack.cbegin() : std::prev( rfirst.base() );
    for( auto it_upper = first; it_upper < ui_stack.cend(); ++it_upper ) {
        const ui_adaptor &ui_upper = it_upper->get();
        for( auto it_lower = first; it_lower < it_upper; ++it_lower ) {
            const ui_adaptor &ui_lower = it_lower->get();
            if( !ui_upper.invalidated && ui_lower.invalidated &&
                overlap( ui_upper.dimensions, ui_lower.dimensions ) ) {
                // invalidated by lower invalidated UIs
                ui_upper.invalidated = true;
            }
            if( ui_upper.invalidated && ui_lower.invalidated &&
                contains( ui_upper.dimensions, ui_lower.dimensions ) ) {
                // fully obscured lower UIs do not need to be redrawn.
                ui_lower.invalidated = false;
                // Note: we don't need to re-test ui_lower from earlier iterations
                // during which ui_upper.invalidated hadn't yet been determined to
                // be true, because if the ui_lower would be obscured by ui_upper,
                // it implies that ui_lower would overlap with ui_upper, by which
                // we would have already determined ui_upper.invalidated to be true
                // then.
            }
        }
    }
}

void ui_adaptor::invalidate_ui() const
{
    if( invalidated ) {
        return;
    }
    auto it = ui_stack.cbegin();
    for( ; it < ui_stack.cend(); ++it ) {
        if( &it->get() == this ) {
            break;
        }
    }
    if( it == ui_stack.end() ) {
        return;
    }
    // If an upper UI occludes this UI then nothing gets redrawn
    for( auto it_upper = std::next( it ); it_upper < ui_stack.cend(); ++it_upper ) {
        if( contains( it_upper->get().dimensions, dimensions ) ) {
            return;
        }
    }
    // Always mark this UI for redraw even if it is below another UI with
    // `disable_uis_below`, so when the UI with `disable_uis_below` is removed,
    // this UI is correctly marked for redraw.
    invalidated = true;
    invalidation_consistency_and_optimization();
}

void ui_adaptor::reset()
{
    on_screen_resize( nullptr );
    on_redraw( nullptr );
    position( point_zero, point_zero );
}

void ui_adaptor::invalidate( const rectangle<point> &rect, const bool reenable_uis_below )
{
    if( rect.p_min.x >= rect.p_max.x || rect.p_min.y >= rect.p_max.y ) {
        if( reenable_uis_below ) {
            invalidation_consistency_and_optimization();
        }
        return;
    }
    // Always invalidate every UI, even if it is below another UI with
    // `disable_uis_below`, so when the UI with `disable_uis_below` is removed,
    // UIs below are correctly marked for redraw.
    for( auto it_upper = ui_stack.cbegin(); it_upper < ui_stack.cend(); ++it_upper ) {
        const ui_adaptor &ui_upper = it_upper->get();
        if( !ui_upper.invalidated && overlap( ui_upper.dimensions, rect ) ) {
            // invalidated by `rect`
            ui_upper.invalidated = true;
        }
    }
    invalidation_consistency_and_optimization();
}

bool ui_adaptor::has_imgui()
{
    for( auto ui : ui_stack ) {
        if( ui.get().is_imgui ) {
            return true;
        }
    }
    return false;
}

void ui_adaptor::redraw()
{
    if( !ui_stack.empty() ) {
        ui_stack.back().get().invalidated = true;
    }
    redraw_invalidated();
}

void ui_adaptor::redraw_invalidated( )
{
    if( test_mode || ui_stack.empty() ) {
        return;
    }
    // This boolean is needed when a debug error is thrown inside redraw_invalidated
    if( !imgui_frame_started ) {
        imclient->new_frame();
    }
    imgui_frame_started = true;

    restore_on_out_of_scope<bool> prev_redraw_in_progress( redraw_in_progress );
    restore_on_out_of_scope<bool> prev_restart_redrawing( restart_redrawing );
    redraw_in_progress = true;

    do {
        // Changed by the resize and redraw callbacks if they call `debugmsg`.
        // When this becomes true, the `debugmsg` call would have already changed
        // the resize and redraw flags according to the resized and redrawn states
        // so far, so we restart redrawing using the changed flags to redraw
        // the area invalidated by the debug message popup.
        restart_redrawing = false;

        // Find the first enabled UI. From now on enabling and disabling UIs
        // have no effect until the end of this call.
        auto first = ui_stack.rbegin();
        for( ; first != ui_stack.rend(); ++first ) {
            if( first->get().disabling_uis_below ) {
                break;
            }
        }

        // Avoid a copy if possible to improve performance. `ui_stack_orig`
        // always contains the original UI stack, and `first_enabled` always points
        // to elements of `ui_stack_orig`.
        std::unique_ptr<ui_stack_t> ui_stack_copy;
        auto first_enabled = first == ui_stack.rend() ? ui_stack.begin() : std::prev( first.base() );
        ui_stack_t *ui_stack_orig = &ui_stack;

        // Apply deferred resizing.
        bool needs_resize = false;
        for( auto it = first_enabled; !needs_resize && it != ui_stack_orig->end(); ++it ) {
            ui_adaptor &ui = *it;
            if( ui.deferred_resize && ui.screen_resized_cb ) {
                needs_resize = true;
            }
        }
        if( needs_resize ) {
            if( !ui_stack_copy ) {
                // Callbacks may modify the UI stack; make a copy of the original one.
                ui_stack_copy = std::make_unique<ui_stack_t>( *ui_stack_orig );
                first_enabled = ui_stack_copy->begin() + ( first_enabled - ui_stack_orig->begin() );
                ui_stack_orig = &*ui_stack_copy;
            }
            for( auto it = first_enabled; !restart_redrawing && it != ui_stack_orig->end(); ++it ) {
                ui_adaptor &ui = *it;
                if( ui.deferred_resize ) {
                    if( ui.screen_resized_cb ) {
                        ui.screen_resized_cb( ui );
                    }
                    if( !restart_redrawing ) {
                        ui.deferred_resize = false;
                    }
                }
            }
        }

        // Redraw invalidated UIs.
        bool needs_redraw = false;
        if( !restart_redrawing ) {
            for( auto it = first_enabled; !needs_redraw && it != ui_stack_orig->end(); ++it ) {
                const ui_adaptor &ui = *it;
                if( ( ui.invalidated || ui.is_imgui ) && ui.redraw_cb ) {
                    needs_redraw = true;
                }
            }
        }
        if( !restart_redrawing && needs_redraw ) {
            if( !ui_stack_copy ) {
                // Callbacks may change the UI stack; make a copy of the original one.
                ui_stack_copy = std::make_unique<ui_stack_t>( *ui_stack_orig );
                first_enabled = ui_stack_copy->begin() + ( first_enabled - ui_stack_orig->begin() );
                ui_stack_orig = &*ui_stack_copy;
            }
            std::optional<point> cursor_pos;
            auto top_ui = std::prev( ui_stack_orig->end() );
            for( auto it = first_enabled; !restart_redrawing && it != ui_stack_orig->end(); ++it ) {
                ui_adaptor &ui = *it;
                ui.is_on_top = it == top_ui;
                if( ui.invalidated || ui.is_imgui ) {
                    if( ui.redraw_cb ) {
                        ui.default_cursor();
                        ui.redraw_cb( ui );
                        if( ui.cursor_type == cursor::last ) {
                            ui.record_term_cursor();
                            cata_assert( ui.cursor_type != cursor::last );
                        }
                        if( ui.cursor_type == cursor::custom ) {
                            cursor_pos = ui.cursor_pos;
                        }
                    }
                    if( !restart_redrawing ) {
                        ui.invalidated = false;
                    }
                }
            }
            if( !restart_redrawing && cursor_pos.has_value() ) {
                restore_cursor( cursor_pos.value() );
            }
        }
    } while( restart_redrawing );
#if defined(EMSCRIPTEN)
    emscripten_sleep( 1 );
#endif

    imclient->end_frame();
    imgui_frame_started = false;

    // if any ImGui window needed to calculate the size of its contents,
    //  it needs an extra frame to draw. We do that here.
    if( imclient->auto_size_frame_active() ) {
        redraw_invalidated();
    }
}

void ui_adaptor::screen_resized()
{
    // Always mark every UI for resize even if it is below another UI with
    // `disable_uis_below`, so when the UI with `disable_uis_below` is removed,
    // UIs below are correctly marked for resize.
    for( ui_adaptor &ui : ui_stack ) {
        ui.deferred_resize = true;
    }
    redraw();
}

background_pane::background_pane()
{
    ui.on_screen_resize( []( ui_adaptor & ui ) {
        ui.position_from_window( catacurses::stdscr );
    } );
    ui.position_from_window( catacurses::stdscr );
    ui.on_redraw( []( const ui_adaptor & ) {
        catacurses::erase();
        wnoutrefresh( catacurses::stdscr );
    } );
}

namespace ui_manager
{

void invalidate( const rectangle<point> &rect, const bool reenable_uis_below )
{
    ui_adaptor::invalidate( rect, reenable_uis_below );
}

void redraw()
{
    ui_adaptor::redraw();
}

void redraw_invalidated()
{
    ui_adaptor::redraw_invalidated();
}

void screen_resized()
{
    ui_adaptor::screen_resized();
}
void invalidate_all_ui_adaptors()
{
    for( ui_adaptor &adaptor : ui_stack ) {
        adaptor.invalidate_ui();
    }
}
} // namespace ui_manager
