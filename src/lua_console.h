#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "output.h"
#include "input.h"
#include "cursesdef.h"

#include <map>
#include <string>
#include <vector>
#include <utility>

class lua_console {
    public:
        lua_console();
        void dispose();
        void run();
    private:
        const int width = TERMX;
        const int lines = 10;

        WINDOW *cWin;
        WINDOW *iWin;

        std::vector<std::pair<std::string, nc_color>> text_stack;
        std::string get_input();
        void print( std::string text );
        void draw();

        bool done = false;

        int scroll = 0;

        void quit();
        void scroll_up();
        void scroll_down();

        std::map<long, std::function<void()>> callbacks {
            {KEY_ESCAPE, [this](){ this->quit(); }},
            {KEY_NPAGE, [this](){ this->scroll_up(); }},
            {KEY_PPAGE, [this](){ this->scroll_down(); }}
        };
};

#endif // LUA_CONSOLE_H
