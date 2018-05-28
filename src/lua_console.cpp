#include "lua_console.h"

#include "catalua.h"
#include "catacharset.h"
#include "input.h"
#include "string_input_popup.h"

#include <map>

lua_console::lua_console() : cWin( catacurses::newwin( lines, width, 0, 0 ) ),
    iWin( catacurses::newwin( 1, width, lines, 0 ) )
{
#ifndef LUA
    text_stack.push_back( {_( "This build does not support Lua." ), c_red} );
#else
    text_stack.push_back( {_( "Welcome to the Lua console! Here you can enter Lua code." ), c_green} );
#endif
    text_stack.push_back( {_( "Press [Esc] to close the Lua console." ), c_blue} );
}

lua_console::~lua_console() = default;

std::string lua_console::get_input()
{
    std::map<long, std::function<bool()>> callbacks {
        {
            KEY_ESCAPE, [this]()
            {
                this->quit();
                return false;
            }
        },
        {
            KEY_NPAGE, [this]()
            {
                this->scroll_up();
                return false;
            }
        },
        {
            KEY_PPAGE, [this]()
            {
                this->scroll_down();
                return false;
            }
        } };
    string_input_popup popup;
    popup.window( iWin, 0, 0, width )
    .max_length( width )
    .identifier( "LUA" );
    popup.callbacks = callbacks;
    popup.query();
    return popup.text();
}

void lua_console::draw()
{
    werase( cWin );

    // Some juggling to make sure text is aligned with the bottom of the console.
    int stack_size = text_stack.size() - scroll;
    for( int i = lines; i > lines - stack_size && i >= 0; i-- ) {
        auto line = text_stack[stack_size - 1 - ( lines - i )];
        mvwprintz( cWin, i - 1, 0, line.second, line.first );
    }

    wrefresh( cWin );
}

void lua_console::quit()
{
    done = true;
}

void lua_console::scroll_down()
{
    scroll = std::min( std::max( ( ( int ) text_stack.size() ) - lines, 0 ), scroll + 1 );
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
        for( auto str : foldstring( line, width ) ) {
            text_stack.push_back( {str, text_color} );
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
        text_stack.push_back( {_( "This build does not support Lua." ), c_red} );
        text_stack.push_back( {_( "Press [Esc] to close the Lua console." ), c_blue} );
#endif // LUA
    }
}
