#include "ui_manager.h"

#include <vector>

#include "cursesdef.h"
#include "point.h"
#include "sdltiles.h"

static std::vector<std::reference_wrapper<ui_adaptor>> ui_stack;

ui_adaptor::ui_adaptor() : invalidated( false )
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

void ui_adaptor::on_redraw( const redraw_callback_t &fun )
{
    redraw_cb = fun;
}

void ui_adaptor::on_screen_resize( const screen_resize_callback_t &fun )
{
    screen_resized_cb = fun;
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
    for( auto it = ui_stack.crbegin(); it < ui_stack.crend(); ++it ) {
        const ui_adaptor &ui = it->get();
        if( overlap( ui.dimensions, rect ) ) {
            ui.invalidated = true;
            if( contains( ui.dimensions, rect ) ) {
                break;
            }
        }
    }
}

void ui_adaptor::redraw()
{
    // TODO refresh only when all stacked UIs are drawn
    if( !ui_stack.empty() ) {
        ui_stack.back().get().invalidated = true;
        for( const ui_adaptor &ui : ui_stack ) {
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
        if( ui.screen_resized_cb ) {
            ui.screen_resized_cb( ui );
        }
    }
    redraw();
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
