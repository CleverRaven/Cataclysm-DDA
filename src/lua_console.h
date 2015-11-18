#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "output.h"
#include "input.h"

#include <map>
#include <string>
#include <vector>
#include <utility>

struct WINDOW;

class lua_console {
    public:
        lua_console();
        void dispose();
        void run();
    private:
        const int width = TERMX;
        const int lines = 10; // 5 lines

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

        instance_invokable<lua_console> quit_callback = instance_invokable<lua_console>(this, quit);
        instance_invokable<lua_console> scroll_up_callback = instance_invokable<lua_console>(this, scroll_up);
        instance_invokable<lua_console> scroll_down_callback = instance_invokable<lua_console>(this, scroll_down);
};

#endif // LUA_CONSOLE_H
