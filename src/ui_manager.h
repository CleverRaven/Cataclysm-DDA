#pragma once
#ifndef CATA_SRC_UI_MANAGER_H
#define CATA_SRC_UI_MANAGER_H

#include <functional>
#include <memory>

#include "cuboid_rectangle.h"
#include "point.h"

namespace cataimgui
{
class client;
} // namespace cataimgui

extern std::unique_ptr<cataimgui::client> imclient;

namespace catacurses
{
class window;
} // namespace catacurses

/**
 * Adaptor between UI code and the UI management system. Using this class allows
 * UIs to be correctly resized and redrawn when the game window is resized or
 * when exiting from other UIs.
 *
 * Usage example:
 * ```
 *     // Effective in the local scope
 *     ui_adaptor ui;
 *     // Ncurses window for drawing
 *     catacurses::window win;
 *     // Things to do when the game window changes size
 *     ui.on_screen_resize( [&]( ui_adaptor & ui ) {
 *         // Create an ncurses window
 *         win = catacurses::newwin( TERMX / 2, TERMY / 2, point( TERMX / 4, TERMY / 4 ) );
 *         // The window passed to this call must contain all the space the redraw
 *         // callback draws to, to ensure proper refreshing when resizing or exiting
 *         // from other UIs.
 *         ui.position_from_window( win );
 *     } );
 *     // Mark the resize callback to be called on the first redraw
 *     ui.mark_resize();
 *     // Things to do when redrawing the UI
 *     ui.on_redraw( [&]( ui_adaptor & ui ) {
 *         // Clear UI area
 *         werase( win );
 *         // Print things
 *         mvwprintw( win, point::zero, "Hello World!" );
 *         // Record the cursor position for screen readers and IME preview to
 *         // correctly function on curses
 *         ui.record_cursor( win );
 *         // Write to frame buffer
 *         wnoutrefresh( win );
 *     } );
 *
 *     input_context ctxt( "<CATEGORY_NAME>" );
 *     ctxt.register_action( "QUIT" );
 *     while( true ) {
 *         // Invalidate the top UI (that is, this UI) and redraw all
 *         // invalidated UIs (including lower UIs that calls this UI).
 *         // May call the resize callbacks.
 *         ui_manager::redraw();
 *         // Get user input. Note that this may call the resize and redraw
 *         // callbacks multiple times due to screen resize, rendering target
 *         // reset, etc.
 *         const std::string action = ctxt.handle_input();
 *         if( action == "QUIT" ) {
 *             break;
 *         }
 *     }
 * ```
 **/
class ui_adaptor
{
    public:
        bool is_imgui;
        bool is_on_top;
        using redraw_callback_t = std::function<void( ui_adaptor & )>;
        using screen_resize_callback_t = std::function<void( ui_adaptor & )>;

        struct disable_uis_below {
        };

        struct debug_message_ui {
        };

        /**
         * Construct a `ui_adaptor` which is automatically added to the UI stack,
         * and removed from the stack when it is deconstructed. (When declared as
         * a local variable, it is removed from the stack when leaving the local scope.)
         **/
        ui_adaptor();
        /**
         * `ui_adaptor` constructed this way will block any UIs below from being
         * redrawn or resized until it is deconstructed.
         **/
        explicit ui_adaptor( disable_uis_below );
        /**
         * Special constructor used by debug.cpp when showing a debug message
         * popup. If the UI is constructed when a redraw call is in progress,
         * The redraw call will be restarted after this UI is deconstructed and
         * the in progress callback returns, in order to prevent incorrect
         * graphics caused by overwritten screen area and resizing.
         **/
        explicit ui_adaptor( debug_message_ui );
        ui_adaptor( const ui_adaptor &rhs ) = delete;
        ui_adaptor( ui_adaptor &&rhs ) = delete;
        ~ui_adaptor();

        ui_adaptor &operator=( const ui_adaptor &rhs ) = delete;
        ui_adaptor &operator=( ui_adaptor &&rhs ) = delete;

        /**
         * Set the position and size of the UI to that of `win`. This information
         * is used to calculate which UIs need redrawing during resizing and when
         * exiting from other UIs, so do call this function in the resizing
         * callback and ensure `win` contains all the space you will be drawing
         * to. Transparency is not supported. If `win` is null, the function has
         * the same effect as `position( point::zero, point::zero )`
         **/
        void position_from_window( const catacurses::window &win );
        /**
         * Set the position and size of the UI to that of an imaginary
         * `catacurses::window` in normal font, except that the size can be zero.
         * Note that `topleft` and `size` are in console cells on both tiles
         * and curses builds.
         **/
        void position( const point &topleft, const point &size );
        /**
         * like 'position', except topleft and size are given as
         * pixels in tiled builds and console cells on curses builds
         **/
        void position_absolute( const point &topleft, const point &size );
        /**
         * Set redraw and resize callbacks. The resize callback should
         * call `position` or `position_from_window` to set the size of the UI,
         * and (re-)calculate any UI data that is related to the screen size,
         * including `catacurses::window` instances. In most cases, you should
         * also call `mark_resize` along with `on_screen_resize` so the UI is
         * initialized by the resizing callback when redrawn for the first time.
         *
         * The redraw callback should only redraw to the area specified by the
         * `position` or `position_from_window` call. Content drawn outside this
         * area may not render correctly when resizing or exiting from other UIs.
         * Transparency is not currently supported.
         *
         * These callbacks should NOT:
         * - Construct new `ui_adaptor` instances
         * - Deconstruct old `ui_adaptor` instances
         * - Call `redraw` or `screen_resized`
         * - (Redraw callback) call `position_from_window`, `position`, or
         *   `invalidate_ui`
         * - Call any function that does these things, except for `debugmsg`
         *
         * Otherwise, display glitches or even crashes might happen.
         **/
        void on_redraw( const redraw_callback_t &fun );
        /* See `on_redraw`. */
        void on_screen_resize( const screen_resize_callback_t &fun );

        /**
         * Automatically set the termianl cursor to a window position after
         * drawing is done. The cursor position of the last drawn UI takes
         * effect. This ensures the cursor is always set to the desired position
         * even if some subsequent drawing code moves the cursor, so screen
         * readers (and in the case of text input, the IME preview) can
         * correctly function. This is only supposed to be called from the
         * `on_redraw` callback on the `ui_adaptor` argument of the callback.
         * Currently only supports curses (on tiles, the windows may have
         * different cell sizes, so the current implementation of recording the
         * on-screen position does not work, and screen readers do not work with
         * the current tiles implementation anyway).
         */
        void set_cursor( const catacurses::window &win, const point &pos );
        /**
         * Record the current window cursor position and restore it when drawing
         * is done. The most recent cursor position is recorded regardless of
         * whether the terminal cursor is updated by refreshing the window. See
         * also `set_cursor`.
         */
        void record_cursor( const catacurses::window &win );
        /**
         * Record the current terminal cursor position (where the cursor was
         * placed when refreshing the last window) and restore it when drawing
         * is done. See also `set_cursor`.
         */
        void record_term_cursor();
        /**
         * Use the terminal cursor position when the redraw callback returns.
         * This is the default. See also `set_cursor`.
         */
        void default_cursor();
        /**
         * Do not set cursor for this `ui_adaptor`. If any higher or lower UI
         * sets the cursor, that cursor position is used instead. If no UI sets
         * the cursor, the final cursor position when drawing is done is used.
         * See also `set_cursor`.
         */
        void disable_cursor();

        /**
         * Mark this `ui_adaptor` for resizing the next time it is redrawn.
         * This is normally called alongside `on_screen_resize` to initialize
         * the UI on the first redraw. You should also use this function to
         * explicitly request a reinitialization if any value the screen resize
         * callback depends on (apart from the screen size) has changed.
         **/
        void mark_resize() const;

        /**
         * Invalidate this UI so it gets redrawn the next redraw unless an upper
         * UI completely occludes this UI. May also cause upper UIs to redraw.
         * Can be used to mark lower UIs for redrawing when their associated data
         * has changed.
         **/
        void invalidate_ui() const;

        /**
         * Reset all callbacks and dimensions. Will cause invalidation of the
         * previously specified screen area.
         **/
        void reset();

        void shutdown();

        /* See the `ui_manager` namespace */
        static void invalidate( const rectangle<point> &rect, bool reenable_uis_below );
        static bool has_imgui();
        static void redraw();
        static void redraw_invalidated();
        static void screen_resized();
    private:
        static void invalidation_consistency_and_optimization();

        // pixel dimensions in tiles, console cell dimensions in curses
        rectangle<point> dimensions;
        redraw_callback_t redraw_cb;
        screen_resize_callback_t screen_resized_cb;

        // For optimization purposes, these value may or may not be reset or
        // modified before, during, or after drawing, so they do not necessarily
        // indicate what the current cursor position would be, and therefore
        // should not be made public or have a public getter.
        enum class cursor {
            last, custom, disabled
        };
        cursor cursor_type = cursor::last;
        point cursor_pos;

        bool disabling_uis_below;
        bool is_debug_message_ui;
        bool is_shutting_down = false;

        mutable bool invalidated;
        mutable bool deferred_resize;
};

/**
 * Helper class that fills the background and obscures all UIs below. It stays
 * on the UI stack until its lifetime ends.
 **/
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
/**
 * Invalidate a portion of the screen when a UI is resized, closed, etc.
 * Not supposed to be directly called by the user.
 * rect is the pixel dimensions in tiles or console cell dimensions in curses
 **/
void invalidate( const rectangle<point> &rect, bool reenable_uis_below );
/**
 * Invalidate the top window and redraw all invalidated windows.
 * Note that `ui_manager` may redraw multiple times when a `ui_adaptor` callback
 * calls `debugmsg`, the game window is resized, or the system requests a redraw,
 * so any data that may change after a resize or on each redraw should be
 * calculated within the respective callbacks.
 **/
void redraw();
/**
 * Redraw all invalidated windows without invalidating the top window.
 **/
void redraw_invalidated();
/**
 * Handle resize of the game window.
 * Not supposed to be directly called by the user.
 **/
void screen_resized();
void invalidate_all_ui_adaptors();
void reset();
} // namespace ui_manager

#endif // CATA_SRC_UI_MANAGER_H
