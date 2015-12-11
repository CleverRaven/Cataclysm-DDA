#include "lua_console.h"

#include "catalua.h"
#include "catacharset.h"

lua_console::lua_console() : cWin( newwin( lines, width, 0, 0 ) ), iWin( newwin( 1, width,lines, 0 ) )
{
}

lua_console::~lua_console()
{
    werase( cWin );
    werase( iWin );
    delwin( cWin );
    delwin( iWin );
}

std::string lua_console::get_input()
{
    long key = 0;
    int pos = -1;
    std::map<long, std::function<void()>> callbacks {
        { KEY_ESCAPE, [this]() { this->quit(); } },
        { KEY_NPAGE, [this]() { this->scroll_up(); } },
        { KEY_PPAGE, [this]() { this->scroll_down(); } } };
    return string_input_win( iWin, "", width, 0, 0, width, true, key, pos,
                             "LUA", 0, lines, true, false, callbacks );
}

void lua_console::draw()
{
    werase( cWin );

    // Some juggling to make sure text is aligned with the bottom of the console.
    int stack_size = text_stack.size() - scroll;
    for( int i = lines; i > lines - stack_size && i >= 0; i-- ) {
        auto line = text_stack[stack_size - 1 - ( lines - i )];
        mvwprintz( cWin, i - 1, 0, line.second, "%s", line.first.c_str() );
    }

    wrefresh( cWin );
}

void lua_console::quit()
{
    done = true;
}

void lua_console::scroll_down()
{
    scroll = std::min( std::max( ((int) text_stack.size()) - lines, 0 ), scroll + 1 );
    draw();
}

void lua_console::scroll_up()
{
    scroll = std::max( 0, scroll - 1 );
    draw();
}

void lua_console::read_stream( std::stringstream &stream, nc_color text_color )
{
    std::string line;
    while( std::getline( stream, line ) ) {
        for( auto str : foldstring(line, width) ) {
            text_stack.push_back({str, text_color});
        }
    }
    stream.str( std::string() ); // empty the buffer
    stream.clear();
}

void lua_console::run()
{
    while( !done ) {
        draw();

        std::string input = get_input();

#ifdef LUA
        call_lua( input );

        read_stream( lua_output_stream, c_white );
        read_stream( lua_error_stream, c_red );
#else
        text_stack.push_back( {"This build does not support lua.", c_red} );
#endif // LUA
    }
}
