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

/// <summary>
/// Editor, to let the player edit text.
///
/// Example:
/// string_editor_window ed = string_editor_window(win, text);
/// new_text = ed.query_string(true);
///
/// </summary>
class string_editor_window
{
    private:
        /*window it is shown in*/
        catacurses::window _win;
        /*max X and Y size*/
        point _max;
        /*current text*/
        utf8_wrapper _utext;
        /*foldedtext for display*/
        std::vector<std::string> _foldedtext;

        /*position of cursor in _utext or as coordinates */
        int _position = 0;
        int _xposition = 0;
        int _yposition = 0;

        std::unique_ptr<input_context> ctxt;
        bool _handled = false;

    public:
        string_editor_window( catacurses::window &win, const std::string &text );

        bool handled() const;

        /*loop, user input is handled. returns the modified string*/
        const std::string &query_string( bool loop );

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
        std::pair<int, int> get_line_and_position();
};
#endif // CATA_SRC_STRING_EDITOR_WINDOW_H
