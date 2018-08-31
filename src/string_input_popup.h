#ifndef STRING_INPUT_POPUP_H
#define STRING_INPUT_POPUP_H

#include "cursesdef.h"

#include <string>
#include <memory>
#include <map>
#include <set>
#include <functional>

class input_context;
struct input_event;
class utf8_wrapper;

/**
 * Shows a window querying the user for input.
 *
 * Returns the input that was entered. If the user cancels the input (e.g. by pressing Escape),
 * an empty string is returned. An empty string may also be returned when the user does not enter
 * any text and confirms the input (by pressing ENTER).
 *
 * Examples:
 * \code
    input = string_input_popup().title("Enter something").query();
    // shows the input field in window w at coordinates (10,20) up to (30,20).
    input = string_input_popup().window(w, 10, 20, 30).query();
 * \endcode
 *
 * @param title The displayed title, describing what to enter. @ref color_tags can be used.
 * @param width Width of the input area where the user input appears.
 * @param input The initially display input. The user can change this.
 * @param desc An optional text (e.h. help or formatting information) which is displayed
 * above the input. Color tags can be used.
 * @param identifier If not empty, this is used to store and retrieve previously entered
 * text. All calls with the same `identifier` share this history, the history is also stored
 * when saving the game (see @ref uistate).
 * @param max_length The maximal length of the text the user can input. More input is simply
 * ignored and the returned string is never longer than this.
 * @param only_digits Whether to only allow digits in the string.
 */
class string_input_popup
{
    private:
        std::string _title;
        std::string _text;
        std::string _description;
        std::string _identifier;
        int _width = 0;
        int _max_length = -1;
        bool _only_digits = false;
        int _startx = 0;
        int _starty = 0;
        int _endx = 0;
        int _position = -1;

        catacurses::window w;

        std::unique_ptr<input_context> ctxt_ptr;
        input_context *ctxt = nullptr;

        bool _canceled = false;

        void query_more( bool loop, bool dorefresh );

        void create_window();
        void create_context();

        void show_history( utf8_wrapper &ret );
        void add_to_history( const std::string &value ) const;
        void draw( const utf8_wrapper &ret, const utf8_wrapper &edit, int shift ) const;

    public:
        string_input_popup();
        ~string_input_popup();
        /**
         * The title: short string before the actual input field.
         * It's optional, default is an empty string.
         */
        string_input_popup &title( std::string value ) {
            _title = value;
            return *this;
        }
        /**
         * Set / get the text that can be modified by the user.
         * Note that cancelling the query makes this an empty string.
         * It's optional default is an empty string.
         */
        /**@{*/
        string_input_popup &text( std::string value ) {
            _text = value;
            return *this;
        }
        const std::string &text() const {
            return _text;
        }
        /**@}*/
        /**
         * Additional help text, shown below the input box.
         * It's optional, default is an empty text.
         */
        string_input_popup &description( std::string value ) {
            _description = value;
            return *this;
        }
        /**
         * An identifier to be used to store / get the input
         * history. If empty (the default), no history will be
         * available, otherwise the history associated with
         * the identifier will be available.
         * If the input is not canceled, the new input is
         * added to the history.
         */
        string_input_popup &identifier( std::string value ) {
            _identifier = value;
            return *this;
        }
        /**
         * Width (in console cells) of the input field itself.
         */
        string_input_popup &width( int value ) {
            _width = value;
            return *this;
        }
        /**
         * Maximal amount of Unicode characters that can be
         * given by the user. The default is something like 1000.
         */
        string_input_popup &max_length( int value ) {
            _max_length = value;
            return *this;
        }
        /**
         * If true, any non-digit input cancels the input. Default is false.
         */
        string_input_popup &only_digits( bool value ) {
            _only_digits = value;
            return *this;
        }
        /**
         * Set the window area where to display the input text. If this is set,
         * the class will not create a separate window and *only* the editable
         * text will be printed at the given part of the given window.
         * Integer parameters define the area (one line) where the editable
         * text is printed.
         */
        string_input_popup &window( const catacurses::window &w, int startx, int starty, int endx );
        /**
         * Set / get the input context that is used to gather user input.
         * The class will create its own context if none is set here.
         */
        /**@{*/
        string_input_popup &context( input_context &ctxt );
        input_context &context() const {
            return *ctxt;
        }
        /**@}*/
        /**
         * Draws the input box, waits for input (if \p loop is true).
         * @return @ref text()
         */
        /**@{*/
        void query( bool loop = true, bool draw_only = false );
        int query_int( bool loop = true, bool draw_only = false );
        long query_long( bool loop = true, bool draw_only = false );
        const std::string &query_string( bool loop = true, bool draw_only = false );
        /**@}*/
        /**
         * Whether the input box was canceled via the ESCAPE key (or similar)
         * If the input was finished via the ENTER key (or similar), this will
         * return `false`.
         */
        bool canceled() const {
            return _canceled;
        }
        /**
         * Edit values in place. This combines: calls to @ref text to set the
         * current value, @ref query to get user input and setting the
         * value back into the parameter object (when the popup was not
         * canceled). Cancelling the popup keeps the value unmodified.
         */
        /**@{*/
        void edit( std::string &text );
        void edit( long &value );
        void edit( int &value );
        /**@}*/

        std::map<long, std::function<bool()>> callbacks;
};

#endif
