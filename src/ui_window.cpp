#include "ui_window.h"

#include "ui_manager.h"

template<typename T>
shared_ptr_fast<ui_adaptor> ui_window<T>::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            draw();
        } );
        current_ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            resize( ui );
        } );
        current_ui->mark_resize();
    }
    return current_ui;
}

template<typename T>
void ui_window<T>::add_element( shared_ptr_fast<ui_element> &element )
{
    layout.add_element( element );
}

template<typename T>
void ui_window<T>::draw()
{
    werase( window );
    layout.draw( window, point_zero );
    wnoutrefresh( window );
}

template<typename T>
void ui_window<T>::resize( ui_adaptor &ui )
{
    // todo: size recalculation (callback?)
    point size( width(), height() );
    point pos( x(), y() );
    window = catacurses::newwin( size.y, size.x, pos );
    // layout position is relative to window area
    layout.pos = point_zero;
    layout.width = width();
    layout.height = height();

    ui.position( pos, size );
}
