#include "lua_console.h"

#include "catalua.h"
#include "catacurse.h"

#include "debug.h"
#include "catacharset.h"

lua_console::lua_console() : cWin(newwin(lines, width, 0, 0)), iWin(newwin(1, width,lines, 0))
{
}

void lua_console::dispose()
{
    werase(cWin);
    werase(iWin);
    delwin(cWin);
    delwin(iWin);
}

std::string lua_console::get_input()
{
    long key = 0;
    int pos = -1;
    return string_input_win(iWin, "", width, 0, 0, width, true, key, pos, "LUA", width, 1, true, false,
        (std::map<long, Invokable *> {
            {KEY_ESCAPE, &quit_callback},
            {KEY_NPAGE, &scroll_up_callback},
            {KEY_PPAGE, &scroll_down_callback}
        }));
}

void lua_console::draw()
{
    werase(cWin);

    // Some juggling to make sure text is aligned with the bottom of the console.
    int stack_size = text_stack.size() - scroll;
    for( int i = lines; i > lines - stack_size && i >= 0; i--) {
        auto line = text_stack[stack_size - 1 - (lines - i)];
        mvwprintz(cWin, i - 1, 0, line.second, line.first.c_str());
    }

    wrefresh(cWin);
}

void lua_console::quit()
{
    done = true;
}

void lua_console::scroll_down()
{
    scroll = std::min(std::max((int) text_stack.size() - lines, 0), scroll + 1);
    draw();
}

void lua_console::scroll_up()
{
    scroll = std::max(0, scroll - 1);
    draw();
}

void lua_console::run()
{
    while( !done ) {
        draw();

        std::string input = get_input();

#ifdef LUA
        call_lua( input );
        std::string line;

        while( std::getline(lua_output_stream, line) ) {
            text_stack.push_back( {line, c_white} );
        }
        lua_output_stream = std::stringstream(); // empty the buffer

        while( std::getline(lua_error_stream, line) ) {
            text_stack.push_back( {line, c_red} );
        }
        lua_error_stream = std::stringstream(); // empty the buffer
#else
        text_stack.push_back( {"This build does not support lua.", c_red} );
#endif // LUA
    }
}
