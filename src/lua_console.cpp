#include "lua_console.h"

#include "catalua.h"
#include "catacharset.h"

lua_console::lua_console() : cWin( newwin( lines, width, 0, 0 ) ), iWin( newwin( 1, width,lines, 0 ) )
{
}

void lua_console::dispose()
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
    return string_input_win( iWin, "", width, 0, 0, width, true, key, pos, "LUA", 0, lines, true, false, callbacks );
}

void lua_console::draw()
{
    werase( cWin );

    // Some juggling to make sure text is aligned with the bottom of the console.
    int stack_size = text_stack.size() - scroll;
    for( int i = lines; i > lines - stack_size && i >= 0; i-- ) {
        auto line = text_stack[stack_size - 1 - ( lines - i )];
        nc_color discard = c_white;
        print_colored_text( cWin, i - 1, 0, discard, c_white, line );
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

void lua_console::run()
{
    while( !done ) {
        draw();

        std::string input = get_input();

#ifdef LUA
        call_lua( input );
        std::string line;

        while( std::getline( lua_output_stream, line ) ) {
            for( auto str : foldstring(line, width) ) {
                text_stack.push_back( str );
            }
        }
        lua_output_stream.str( std::string() ); // empty the buffer
        lua_output_stream.clear();

        while( std::getline( lua_error_stream, line ) ) {
            for( auto str : foldstring(line, width) ) {
                text_stack.push_back( "<color_red>" + str + "</color>" );
            }
        }
        lua_error_stream.str( std::string() ); // empty the buffer
        lua_error_stream.clear();
#else
        text_stack.push_back( {"This build does not support lua.", c_red} );
#endif // LUA
    }
}
