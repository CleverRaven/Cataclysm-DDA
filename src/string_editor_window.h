#pragma once
#ifndef CATA_SRC_STRING_EDITOR_WINDOW_H
#define CATA_SRC_STRING_EDITOR_WINDOW_H

#include <map>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "ui.h"
#include "ui_manager.h"
#include "wcwidth.h"

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
        /*window it is schown in*/
        catacurses::window _win;
        /*max X and Y size*/
        point _max;
        /*current text*/
        utf8_wrapper _utext;
        /*foldedtext for display*/
        std::vector<std::string> _foldedtext;

        /*default color*/
        nc_color _string_color = c_magenta;
        nc_color _cursor_color = h_light_gray;

        bool _ignore_custom_actions = true;

        /*position of Courser in _utext or as coordinates */
        int _position = 0;
        int _xposition = 0;
        int _yposition = 0;

        std::unique_ptr<input_context> ctxt_ptr;
        input_context *ctxt = nullptr;
        bool _handled = false;

        std::map<long, std::function<bool()>> callbacks;
    public:
        string_editor_window( catacurses::window &win, std::string text );

        bool handled() const;

        /*loop, user imput is handelt. returns the moditided string*/
        const std::string &query_string( const bool loop );

    private:

        /*print the editor*/
        void print_editor();

        void create_context();

        /*move the coursour*/
        void coursour_left( int n = 1 );
        void coursour_right( int n = 1 );
        void coursour_up( int n = 1 );
        void coursour_down( int n = 1 );

        /*returns line and position in folded text for position in text*/
        std::pair<int, int> get_line_and_position( std::vector<std::string> foldedtext,
                int position );
};
#endif // CATA_SRC_STRING_EDITOR_WINDOW_H
