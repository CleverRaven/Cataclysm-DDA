#pragma once
#ifndef CATA_SRC_STRING_EDITOR_WINDOW_H
#define CATA_SRC_STRING_EDITOR_WINDOW_H

#include <map>
#include <string>
#include <vector>

#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "ui.h"
#include "ui_manager.h"
#include "cursesdef.h"
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
        int _maxx;
        int _maxy;
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
        input_context* ctxt = nullptr;

        bool _canceled = false;
        bool _confirmed = false;
        bool _handled = false;

        std::map<long, std::function<bool()>> callbacks;
    public:
        string_editor_window(catacurses::window& win, std::string text);

        /*returns line and position in folded text for position in text*/
        std::pair<int, int> get_line_and_position(std::string text, std::vector<std::string> foldedtext, int position); 
        /*returns position in text for line and position in foldedtext*/
        int get_position(int y, int x);

        /*print the editor*/
        void print_editor();


        bool canceled() const;

        bool confirmed() const;

        bool handled() const;

        void create_context();

        
        /*move the coursour*/
        void coursour_left(int n = 1);
        void coursour_right(int n = 1);
        void coursour_up(int n = 1);
        void coursour_down(int n = 1);

        /*loop, user imput is handelt. returns the moditided string*/
        const std::string& query_string(const bool loop);
    

};
#endif
