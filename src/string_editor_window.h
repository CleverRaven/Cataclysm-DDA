#pragma once
#ifndef CATA_SRC_STRING_EDITOR_WINDOW_H
#define CATA_SRC_STRING_EDITOR_WINDOW_H

#include <functional>
#include <string>

#include "catacharset.h"
#include "cursesdef.h"
#include "point.h"

class folded_text;

struct ime_preview_range;

class input_context;
class ui_adaptor;

/// <summary>
/// Editor, to let the player edit text.
///
/// Example:
/// string_editor_window ed = string_editor_window( create_window, text );
/// new_text = ed.query_string();
///
/// </summary>
class string_editor_window
{
    private:
        /*window it is shown in*/
        catacurses::window _win;
        /*callback to create a window during initialization and after screen resize*/
        std::function<catacurses::window()> _create_window;
        /*max X and Y size*/
        point _max;
        /*current text*/
        utf8_wrapper _utext;
        /*folded text for display*/
        std::unique_ptr<folded_text> _folded;

        /*codepoint index of cursor in _utext*/
        int _position = -1;
        /*display coordinates of cursor*/
        point _cursor_display;
        /*desired x coordinate of cursor when moving cursor up or down*/
        int _cursor_desired_x = -1;
        /*IME preview range*/
        std::unique_ptr<ime_preview_range> _ime_preview_range;

        std::unique_ptr<input_context> ctxt;

    public:
        string_editor_window( const std::function<catacurses::window()> &create_window,
                              const std::string &text );
        ~string_editor_window();

        /*loop, user input is handled. returns whether user confirmed input and
          the modified string*/
        std::pair<bool, std::string> query_string();

    private:
        /*print the editor*/
        void print_editor( ui_adaptor &ui );

        void create_context();

        /*move the cursor*/
        void cursor_leftright( int diff );
        void cursor_updown( int diff );

        /*returns line and position in folded text for position in text*/
        point get_line_and_position( int position, bool zero_x );
};
#endif // CATA_SRC_STRING_EDITOR_WINDOW_H
