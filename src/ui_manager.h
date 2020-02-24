#pragma once
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <functional>

#include "point.h"

namespace catacurses
{
class window;
} // namespace catacurses

class ui_adaptor
{
    public:
        using redraw_callback_t = std::function<void( const ui_adaptor & )>;
        using screen_resize_callback_t = std::function<void( ui_adaptor & )>;

        ui_adaptor();
        ui_adaptor( const ui_adaptor &rhs ) = delete;
        ui_adaptor( ui_adaptor &&rhs ) = delete;
        ~ui_adaptor();

        ui_adaptor &operator=( const ui_adaptor &rhs ) = delete;
        ui_adaptor &operator=( ui_adaptor &&rhs ) = delete;

        void position_from_window( const catacurses::window &win );
        void on_redraw( const redraw_callback_t &fun );
        void on_screen_resize( const screen_resize_callback_t &fun );

        static void invalidate( const rectangle &rect );
        static void redraw();
        static void screen_resized();
    private:
        // pixel dimensions in tiles, console cell dimensions in curses
        rectangle dimensions;
        redraw_callback_t redraw_cb;
        screen_resize_callback_t screen_resized_cb;

        mutable bool invalidated;
};

// export static funcs of ui_adaptor with a more coherent scope name
namespace ui_manager
{
// rect is the pixel dimensions in tiles or console cell dimensions in curses
void invalidate( const rectangle &rect );
// invalidate the top window and redraw all invalidated windows
void redraw();
void screen_resized();
} // namespace ui_manager

#endif
