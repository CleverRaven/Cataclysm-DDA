#include "ui_manager.h"

#include <vector>

#include "cursesdef.h"
#include "point.h"
#include "sdltiles.h"

static std::vector<std::reference_wrapper<ui_adaptor>> ui_stack;

ui_adaptor::ui_adaptor() : disabling_uis_below( false ), invalidated( false ),
    deferred_resize( false )
{
    ui_stack.emplace_back( *this );
}

ui_adaptor::ui_adaptor( ui_adaptor::disable_uis_below ) : disabling_uis_below( true ),
    invalidated( false ), deferred_resize( false )
{
    ui_stack.emplace_back( *this );
}

ui_adaptor::~ui_adaptor()
{
    for( auto it = ui_stack.rbegin(); it < ui_stack.rend(); ++it ) {
        if( &it->get() == this ) {
            ui_stack.erase( std::prev( it.base() ) );
            // TODO avoid invalidating portions that do not need to be redrawn
            ui_manager::invalidate( dimensions );
            break;
        }
    }
}

void ui_adaptor::position_from_window( const catacurses::window &win )
{
    const rectangle old_dimensions = dimensions;
    // ensure position is updated before calling invalidate
#ifdef TILES
    const window_dimensions dim = get_window_dimensions( win );
    dimensions = rectangle( dim.window_pos_pixel, dim.window_pos_pixel + dim.window_size_pixel );
#else
    const point origin( getbegx( win ), getbegy( win ) );
    dimensions = rectangle( origin, origin + point( getmaxx( win ), getmaxy( win ) ) );
#endif
    invalidated = true;
    ui_manager::invalidate( old_dimensions );
}

void ui_adaptor::position( const point &topleft, const point &size )
{
    position_from_window( catacurses::newwin( size.y, size.x, topleft ) );
}

void ui_adaptor::on_redraw( const redraw_callback_t &fun )
{
    redraw_cb = fun;
}

void ui_adaptor::on_screen_resize( const screen_resize_callback_t &fun )
{
    screen_resized_cb = fun;
}

void ui_adaptor::mark_resize() const
{
    deferred_resize = true;
}

static bool contains( const rectangle &lhs, const rectangle &rhs )
{
    return rhs.p_min.x >= lhs.p_min.x && rhs.p_max.x <= lhs.p_max.x &&
           rhs.p_min.y >= lhs.p_min.y && rhs.p_max.y <= lhs.p_max.y;
}

static bool overlap( const rectangle &lhs, const rectangle &rhs )
{
    return lhs.p_min.x < rhs.p_max.x && lhs.p_min.y < rhs.p_max.y &&
           rhs.p_min.x < lhs.p_max.x && rhs.p_min.y < lhs.p_max.y;
}

void ui_adaptor::invalidate( const rectangle &rect )
{
    if( rect.p_min.x >= rect.p_max.x || rect.p_min.y >= rect.p_max.y ) {
        return;
    }
    // TODO avoid invalidating portions that do not need to be redrawn
    for( auto it_upper = ui_stack.cbegin(); it_upper < ui_stack.cend(); ++it_upper ) {
        const ui_adaptor &ui_upper = it_upper->get();
        if( !ui_upper.invalidated && overlap( ui_upper.dimensions, rect ) ) {
            // invalidated by `rect`
            ui_upper.invalidated = true;
        }
        for( auto it_lower = ui_stack.cbegin(); it_lower < it_upper; ++it_lower ) {
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

void ui_adaptor::redraw()
{
    // apply deferred resizing
    auto first = ui_stack.rbegin();
    for( ; first != ui_stack.rend(); ++first ) {
        if( first->get().disabling_uis_below ) {
            break;
        }
    }
    for( auto it = first == ui_stack.rend() ? ui_stack.begin() : std::prev( first.base() );
         it != ui_stack.end(); ++it ) {
        ui_adaptor &ui = *it;
        if( ui.deferred_resize ) {
            if( ui.screen_resized_cb ) {
                ui.screen_resized_cb( ui );
            }
            ui.deferred_resize = false;
        }
    }

    // redraw invalidated uis
    // TODO refresh only when all stacked UIs are drawn
    if( !ui_stack.empty() ) {
        ui_stack.back().get().invalidated = true;
        auto first = ui_stack.crbegin();
        for( ; first != ui_stack.crend(); ++first ) {
            if( first->get().disabling_uis_below ) {
                break;
            }
        }
        for( auto it = first == ui_stack.crend() ? ui_stack.cbegin() : std::prev( first.base() );
             it != ui_stack.cend(); ++it ) {
            const ui_adaptor &ui = *it;
            if( ui.invalidated ) {
                if( ui.redraw_cb ) {
                    ui.redraw_cb( ui );
                }
                ui.invalidated = false;
            }
        }
    }
}

void ui_adaptor::screen_resized()
{
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
        catacurses::refresh();
    } );
}

namespace ui_manager
{

void invalidate( const rectangle &rect )
{
    ui_adaptor::invalidate( rect );
}

void redraw()
{
    ui_adaptor::redraw();
}

void screen_resized()
{
    ui_adaptor::screen_resized();
}

} // namespace ui_manager
