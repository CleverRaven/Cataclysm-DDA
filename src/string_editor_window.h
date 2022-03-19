#pragma once
#ifndef CATA_SRC_STRING_EDITOR_WINDOW_H
#define CATA_SRC_STRING_EDITOR_WINDOW_H

#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "output.h"
#include "ui.h"
#include "ui_manager.h"

class folded_text;

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

        std::unique_ptr<input_context> ctxt;

    public:
        string_editor_window( const std::function<catacurses::window()> &create_window,
                              const std::string &text );
        ~string_editor_window();

        /*loop, user input is handled. returns the modified string*/
        const std::string &query_string();

    private:
        /*print the editor*/
        void print_editor();

        void create_context();

        /*move the cursor*/
        void cursor_left();
        void cursor_right();
        void cursor_up();
        void cursor_down();

        /*returns line and position in folded text for position in text*/
        point get_line_and_position();
};
#endif // CATA_SRC_STRING_EDITOR_WINDOW_H
