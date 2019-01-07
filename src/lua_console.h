#pragma once
#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include <string>
#include <utility>
#include <vector>

#include "cursesdef.h"
#include "output.h"

class nc_color;

class lua_console
{
    public:
        lua_console();
        ~lua_console();
        void run();
    private:
        const int width = TERMX;
        const int lines = 10;

        catacurses::window cWin;
        catacurses::window iWin;

        std::vector<std::pair<std::string, nc_color>> text_stack;
        std::string get_input();
        void draw();

        bool done = false;

        int scroll = 0;

        void read_stream( std::stringstream &stream, nc_color text_color );

        void quit();
        void scroll_up();
        void scroll_down();
};

#endif // LUA_CONSOLE_H
