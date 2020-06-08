#pragma once
#ifndef CATA_SRC_UI_MANAGER_H
#define CATA_SRC_UI_MANAGER_H

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

        struct disable_uis_below {
        };

        ui_adaptor();
        // ui_adaptor constructed this way will block any uis below from being
        // redrawn or resized until it is deconstructed. It is used for `debug_msg`.
        ui_adaptor( disable_uis_below );
        ui_adaptor( const ui_adaptor &rhs ) = delete;
        ui_adaptor( ui_adaptor &&rhs ) = delete;
        ~ui_adaptor();

        ui_adaptor &operator=( const ui_adaptor &rhs ) = delete;
        ui_adaptor &operator=( ui_adaptor &&rhs ) = delete;

        // If win is null, the function has the same effect as position( point_zero, point_zero )
        void position_from_window( const catacurses::window &win );
        // Set the position and size of the ui to that of an imaginary normal
        // catacurses::window, except that size can be zero.
        // Note that `topleft` and `size` are in console cells on both tiles
        // and curses build.
        void position( const point &topleft, const point &size );
        // Set redraw and resizing callbacks. These callbacks should NOT call
        // `debugmsg`, construct new `ui_adaptor` instances, deconstruct old
        // `ui_adaptor` instances, call `redraw`, or call `screen_resized`.
        //
        // The redraw callback should also not call `position_from_window`,
        // otherwise it may cause UI glitch if the window position changes.
        void on_redraw( const redraw_callback_t &fun );
        void on_screen_resize( const screen_resize_callback_t &fun );

        // Mark this ui_adaptor for resizing the next time `redraw()` is called.
        // This is useful for deferring initialization of the UI when explicit
        // initialization is not possible or wanted.
        void mark_resize() const;

        // Invalidate this UI so it gets redrawn the next time screen is refreshed,
        // unless an upper UI completely occludes this UI. May also cause upper UIs
        // to redraw.
        void invalidate_ui() const;

        // Reset all callbacks and dimensions
        void reset();

        static void invalidate( const rectangle &rect );
        static void redraw();
        static void redraw_invalidated();
        static void screen_resized();
    private:
        static void invalidation_consistency_and_optimization();

        // pixel dimensions in tiles, console cell dimensions in curses
        rectangle dimensions;
        redraw_callback_t redraw_cb;
        screen_resize_callback_t screen_resized_cb;

        bool disabling_uis_below;

        mutable bool invalidated;
        mutable bool deferred_resize;
};

// Helper class that fills the background and obscures all UIs below. It stays
// on the UI stack until its lifetime ends.
class background_pane
{
    public:
        background_pane();
    private:
        ui_adaptor ui;
};

// export static funcs of ui_adaptor with a more coherent scope name
namespace ui_manager
{
// rect is the pixel dimensions in tiles or console cell dimensions in curses
void invalidate( const rectangle &rect );
// invalidate the top window and redraw all invalidated windows
void redraw();
// redraw all invalidated windows without invalidating the top window
void redraw_invalidated();
void screen_resized();
} // namespace ui_manager

#endif // CATA_SRC_UI_MANAGER_H
