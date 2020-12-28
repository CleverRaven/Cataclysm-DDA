#pragma once
#ifndef CATA_SRC_POPUP_H
#define CATA_SRC_POPUP_H

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "point.h"
#include "string_formatter.h"

class ui_adaptor;

/**
 * UI class for displaying messages or querying player input with popups.
 *
 * Example:
 *
 * std::string action = query_popup()
 *                      .context( "YESNOCANCEL" ) // input context to use
 *                      .message( "%s", _( "Do you want to save before jumping into the lava?" ) )
 *                      .option( "YES" ) // yes, save before jumping
 *                      .option( "NO" ) // no, don't save before jumping
 *                      .option( "QUIT" ) // NOOO, I didn't mean to jump into lava!
 *                      .cursor( 2 ) // default to the third option `QUIT`
 *                      .query() // do the query, and get the input action and event
 *                      .action // retrieve the input action
 *
 * Please refer to documentation of individual functions for detailed explanation.
 **/
class query_popup
{
    public:
        /**
         * Query result returned by `query_once` and `query`.
         *
         * 'wait_input' indicates whether a selection is confirmed, either by
         * "CONFIRM" action or by the option's corresponding action, or if the
         * popup is canceled by "QUIT" action when `allow_cancel` is set to true.
         * It is also false if `allow_anykey` is set to true, or when an error
         * happened. It is always false when returned by `query`.
         *
         * `action` is the selected action, "QUIT" if `allow_cancel` is set to
         * true and "QUIT" action occurs, or "ANY_INPUT" if `allow_anykey` is
         * set to true and an unknown action occurs. In `query_once`, action
         * can also be other actions such as "LEFT" or "RIGHT" which are used
         * for moving the cursor. If an error occured, such as when the popup
         * is not properly set up, `action` will be "ERROR".
         *
         * `evt` is the actual `input_event` that triggers the action. Note that
         * `action` and `evt` do NOT always correspond to each other. For
         * example, if an action is selected by pressing return ("CONFIRM" action),
         * `action` will be the selected action, while `evt` will correspond to
         * "CONFIRM" action.
         **/
        struct result {
            result();
            result( bool wait_input, const std::string &action, const input_event &evt );

            bool wait_input;
            std::string action;
            input_event evt;
        };

        /**
         * Default construction. Note that context and options are not set in
         * default construction, and calling `query_once` or `query` right after
         * default construction will return { false, "ERROR", {} }.
         **/
        query_popup();

        /**
         * Specify the input context. In addition to being used to handle input
         * actions, the input context will also be used to generate option text,
         * so it should always be specified as long as at least one option is
         * specified with `option()`, even if `query_once` or `query` are not
         * called afterwards.
         **/
        query_popup &context( const std::string &cat );
        /**
         * Specify the query message.
         */
        template <typename ...Args>
        query_popup &message( const std::string &fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = string_format( fmt, std::forward<Args>( args )... );
            return *this;
        }
        template <typename ...Args>
        query_popup &message( const char *const fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = string_format( fmt, std::forward<Args>( args )... );
            return *this;
        }
        /**
         * Like query_popup::message, but with waiting symbol prepended to the text.
         **/
        template <typename ...Args>
        query_popup &wait_message( const nc_color &bar_color, const std::string &fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ), bar_color );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const nc_color &bar_color, const char *const fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ), bar_color );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const std::string &fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ) );
            return *this;
        }
        template <typename ...Args>
        query_popup &wait_message( const char *const fmt, Args &&... args ) {
            assert_format( fmt, std::forward<Args>( args )... );
            invalidate_ui();
            text = wait_text( string_format( fmt, std::forward<Args>( args )... ) );
            return *this;
        }
        /**
         * Specify an action as an option. The action must be present in the
         * supplied context, either locally or globally. The same applies to
         * other `option` methods.
         **/
        query_popup &option( const std::string &opt );
        /**
         * Specify an action as an option, and a filter of allowed input events
         * for this action. This is for compatibility with the "FORCE_CAPITAL_YN"
         * option.
         * Note that even if the input event is filtered, it will still select
         * the respective dialog option, without closing the dialog.
         */
        query_popup &option( const std::string &opt,
                             const std::function<bool( const input_event & )> &filter );
        /**
         * Specify whether non-option actions can be returned. Mouse movement
         * is always ignored regardless of this setting.
         **/
        query_popup &allow_anykey( bool allow );
        /**
         * Specify whether an implicit cancel option is allowed. This call does
         * not list the cancel option in the UI. Use `option( "QUIT" )` instead
         * to explicitly list cancel in the UI.
         **/
        query_popup &allow_cancel( bool allow );
        /**
         * Whether to show the popup on the top of the screen
         **/
        query_popup &on_top( bool top );
        /**
         * Whether to show the popup in `FULL_SCREEN_HEIGHT` and `FULL_SCREEN_WIDTH`.
         **/
        query_popup &full_screen( bool full );
        /**
         * Specify starting cursor position.
         **/
        query_popup &cursor( size_t pos );
        /**
         * Specify the default message color.
         **/
        query_popup &default_color( const nc_color &d_color );

        /**
         * Draw the UI. An input context should be provided using `context()`
         * for this function to properly generate option text.
         **/
        void show() const;
        /**
         * Query once and return the result. In order for this method to return
         * valid results, the popup must either have at least one option, or
         * have `allow_cancel` or `allow_anykey` set to true. Otherwise
         * { false, "ERROR", {} } is returned. The same applies to `query`.
         **/
        result query_once();
        /**
         * Query until a valid action or an error happens and return the result.
         */
        result query();

    protected:
        /**
         * Create or get a ui_adaptor on the UI stack to handle redrawing and
         * resizing of the popup.
         */
        std::shared_ptr<ui_adaptor> create_or_get_adaptor( bool disable_below = false );

    private:
        struct query_option {
            query_option( const std::string &action,
                          const std::function<bool( const input_event & )> &filter );

            std::string action;
            std::function<bool( const input_event & )> filter;
        };

        std::string category;
        std::string text;
        std::vector<query_option> options;
        size_t cur;
        nc_color default_text_color;
        bool anykey;
        bool cancel;
        bool ontop;
        bool fullscr;

        struct button {
            button( const std::string &text, const point & );

            std::string text;
            point pos;
        };

        std::weak_ptr<ui_adaptor> adaptor;

        // UI caches
        mutable catacurses::window win;
        mutable std::vector<std::string> folded_msg;
        mutable std::vector<button> buttons;

        static std::vector<std::vector<std::string>> fold_query(
                    const std::string &category,
                    const std::vector<query_option> &options,
                    int max_width, int horz_padding );
        void invalidate_ui() const;
        void init() const;

        template <typename ...Args>
        static void assert_format( const std::string &, Args &&... ) {
            static_assert( sizeof...( Args ) > 0,
                           "Format string should take at least one argument.  "
                           "If your message is not a format string, "
                           "use `message( \"%s\", text )` instead." );
        }

        static std::string wait_text( const std::string &text, const nc_color &bar_color );
        static std::string wait_text( const std::string &text );
};

/**
 * Create a popup on the UI stack that gets displayed but receives no input itself.
 * Call ui_manager::redraw() to redraw the popup along with other UIs on the stack,
 * and refresh_display() to force refresh the display if not receiving input after
 * redraw. The popup stays on the UI stack until its lifetime ends.
 *
 * Example:
 *
 * if( not_loaded ) {
 *     static_popup popup;
 *     popup.message( "Please wait…" );
 *     while( loading ) {
 *         ui_manager::redraw();
 *         refresh_display(); // force redraw since we're not receiving input here
 *         load_part();
 *     }
 * }
 * // Popup removed from UI stack when going out of scope.
 * // Note that the removal is not visible until the next time `ui_manager::redraw`
 * // is called.
 */
class static_popup : public query_popup
{
    public:
        static_popup();

    private:
        std::shared_ptr<ui_adaptor> ui;
};

/**
 * Popup optimized to work as a throbber (indicate some calculation running in background).
 *
 * Create an instance of this class to put the popup on the UI stack.
 *
 * Call `refresh()` to redraw the popup, refresh the game window and poll events.
 * To minimize the cost of repeated UI redraws and window refreshes,
 * actual redraw/refresh/poll rate is kept at maximum 2 times per second,
 * and all UIs under the popup are disabled (are not redrawn or resized).
 *
 * The popup stays on the UI stack until its lifetime ends.
 *
 * Example:
 *
 * ```
 * {
 *     throbber_popup popup( _( "Please wait…" ) );
 *     while( loading ) {
 *         popup.refresh(); // may be safely called 1000s of times per second
 *         load_part();
 *     }
 *     // The popup is removed from UI stack when it goes out of scope.
 *     // Note that the removal is not visible until the next time `ui_manager::redraw` is called.
 * }
 * ```
 */
class throbber_popup : private query_popup
{
    public:
        /**
         * Create a popup on the UI stack.
         * The popup is removed from UI stack when it goes out of scope.
         * @param msg message to display.
         * A spinning animation is appended automatically on the left side of the message.
         */
        throbber_popup( const std::string &msg );

        /**
         * Refresh the popup. Ideally, should be called at least 2 times per second
         * to keep the refresh rate consistent.
         */
        void refresh();

    private:
        std::shared_ptr<ui_adaptor> ui;
        std::string msg;
        std::chrono::steady_clock::time_point last_update;
};

#endif // CATA_SRC_POPUP_H
