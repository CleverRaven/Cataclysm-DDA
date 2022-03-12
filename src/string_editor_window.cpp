#include "string_editor_window.h"

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#include "options.h"
#endif

string_editor_window::string_editor_window( catacurses::window &win, const std::string &text )
{
    _win = win;
    _max.x = getmaxx( win );
    _max.y =  getmaxy( win );
    _utext = utf8_wrapper( text );
    _foldedtext = foldstring( _utext.str(), _max.x - 1 );
}

std::pair<int, int> string_editor_window::get_line_and_position()
{
    int counter = 0;
    for( int i = 0; i < static_cast<int>( _foldedtext.size() ); i++ ) {
        utf8_wrapper linetext( _foldedtext[i] );
        int temp = linetext.display_width();
        //foldstring, cuts " " away, so it is possible to get a huge disconnect between folded and unfolded strings.
        if( !_foldedtext[i].empty() ) {
            temp += ( _foldedtext[i].back() != ' ' ) ? 1 : 0;
        }

        if( counter + temp > _position ) {
            const int lineposition = _position - counter;
            return std::make_pair( i, lineposition );
        } else {
            counter += temp;
        }
    }
    return std::make_pair( static_cast<int>( _foldedtext.size() ), 0 );
}

void string_editor_window::print_editor()
{
    const int ftsize = static_cast<int>( _foldedtext.size() );
    const int middleofpage = _max.y / 2;
    const auto line_position = std::make_pair( _yposition, _xposition );

    int topoflist = 0;
    int bottomoflist = std::min( topoflist + _max.y, ftsize );

    if( _max.y <= ftsize ) {
        if( line_position.first > middleofpage ) {
            topoflist = line_position.first - middleofpage;
            bottomoflist = topoflist + _max.y;
        }
        if( line_position.first + middleofpage >= ftsize ) {
            bottomoflist = ftsize ;
            topoflist = bottomoflist - _max.y;
        }
    }

    for( int i = topoflist; i < bottomoflist; i++ ) {
        const int y = i - topoflist;
        trim_and_print( _win, point( 1, y ), _max.x, c_white, _foldedtext[i] );
        if( i == line_position.first ) {
            std::string c_cursor = " ";
            if( _position < static_cast<int>( _utext.size() ) ) {
                utf8_wrapper cursor = _utext.substr( _position, 1 );
                if( *cursor.c_str() != '\n' ) {
                    c_cursor = cursor.str();
                }
            }

            mvwprintz( _win, point( line_position.second + 1, y ), h_white, "%s", c_cursor );
        }
    }

    if( ftsize > _max.y ) {
        scrollbar()
        .content_size( ftsize )
        .viewport_pos( topoflist )
        .viewport_size( _max.y )
        .apply( _win );
    }
}

bool string_editor_window::handled() const
{
    return _handled;
}

void string_editor_window::create_context()
{
    ctxt = std::make_unique<input_context>();
    ctxt->register_action( "ANY_INPUT" );
}

void string_editor_window::cursor_left()
{
    if( _position > 0 ) {
        _position--;
    } else {
        _position = _utext.size();
    }
}

void string_editor_window::cursor_right()
{
    if( _position + 1 <= static_cast<int>( _utext.size() ) ) {
        _position++;
    } else {
        _position = 0;
    }
}

void string_editor_window::cursor_up()
{
    if( _yposition > 0 ) {
        const int size = utf8_wrapper( _foldedtext[_yposition - 1] ).size();
        if( _xposition < size ) {
            _position -= size;
        } else {
            _position -= _xposition + 1;
        }
    } else {
        _position = _utext.size() - utf8_wrapper( _foldedtext.back() ).size();
    }
}

void string_editor_window::cursor_down()
{
    const int size = utf8_wrapper( _foldedtext[_yposition % _foldedtext.size()] ).size();
    const int nextsize = utf8_wrapper( _foldedtext[( _yposition + 1 ) % _foldedtext.size()] ).size();
    if( size == 0 ) {
        _position++;
    } else if( nextsize == 0 ) {
        _position = _position + size - _xposition + 1;
    } else if( _xposition < nextsize ) {
        _position += size;
    } else {
        _position = _position + size - _xposition + nextsize - 1;

    }
    _position = _position % _utext.size();
}

const std::string &string_editor_window::query_string( bool loop )
{
    if( !ctxt ) {
        create_context();
    }

    utf8_wrapper edit( ctxt->get_edittext() );
    if( _position == -1 ) {
        _position = _utext.length();
    }

    int ch = 0;
    do {
        _foldedtext = foldstring( _utext.str(), _max.x - 1 );
        const auto line_position = get_line_and_position();
        _xposition = line_position.second;
        _yposition = line_position.first;

        werase( _win );
        print_editor();
        wnoutrefresh( _win );

        const std::string action = ctxt->handle_input();
        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;

        _handled = true;
        if( action != "ANY_INPUT" ) {
            _handled = false;
            continue;
        }

        if( ch == KEY_ESCAPE ) {
#if defined(__ANDROID__)
            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                SDL_StopTextInput();
            }
#endif
            return _utext.str();
        } else if( ch == KEY_UP ) {
            cursor_up();
        } else if( ch == KEY_DOWN ) {
            cursor_down();
        } else if( ch == KEY_RIGHT ) {
            cursor_right();
        } else if( ch == KEY_LEFT ) {
            cursor_left();
        } else if( ch == 0x15 ) {                   // ctrl-u: delete all the things
            _position = 0;
            _utext.erase( 0 );
        } else if( ch == KEY_BACKSPACE ) {
            if( _position > 0 && _position <= static_cast<int>( _utext.size() ) ) {
                _position--;
                _utext.erase( _position, 1 );
            }
        } else if( ch == KEY_HOME ) {
            _position = 0;
        } else if( ch == KEY_END ) {
            _position = _utext.size();
        } else if( ch == KEY_DC ) {
            if( _position < static_cast<int>( _utext.size() ) ) {
                _utext.erase( _position, 1 );
            }
        } else if( ch == 0x16 || ch == KEY_F( 2 ) || !ev.text.empty() || ch == KEY_ENTER || ch == '\n' ) {
            // ctrl-v, f2, or _utext input
            // bail out early if already at length limit
            std::string entered;
            if( ch == 0x16 ) {
#if defined(TILES)
                if( edit.empty() ) {
                    char *const clip = SDL_GetClipboardText();
                    if( clip ) {
                        entered = clip;
                        SDL_free( clip );
                    }
                }
#endif
            } else if( ch == KEY_F( 2 ) ) {
                if( edit.empty() ) {
                    entered = get_input_string_from_file();
                }
            } else if( ch == KEY_ENTER || ch == '\n' ) {
                entered = "\n";

            } else {
                entered = ev.text;
            }
            if( !entered.empty() ) {
                utf8_wrapper insertion;
                const char *str = entered.c_str();

                int len = entered.length();
                while( len > 0 ) {
                    const uint32_t ch = UTF8_getch( &str, &len );
                    if( ch != '\r' ) {
                        insertion.append( utf8_wrapper( utf32_to_utf8( ch ) ) );
                    }
                }
                _utext.insert( _position, insertion );
                _position += insertion.length();
                edit = utf8_wrapper();
                ctxt->set_edittext( std::string() );
            }
        } else if( ev.edit_refresh ) {
            edit = utf8_wrapper( ev.edit );
            ctxt->set_edittext( ev.edit );
        } else {
            _handled = false;
        }
    } while( loop );

    return _utext.str();
}
