#include "string_editor_window.h"

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#if defined(__ANDROID__)
#include <SDL_keyboard.h>
#include "options.h"
#endif

#include "wcwidth.h"

static bool is_linebreak( const uint32_t uc )
{
    return uc == '\n';
}

static bool is_breaking( const uint32_t uc )
{
    return uc == ' ';
}

struct folded_line {
    int cpts_start;
    int cpts_end;
    std::string str;
};

// fold text without truncating spaces or parsing color tags
class folded_text
{
    private:
        int line_width = 0;
        std::vector<folded_line> lines;

    public:
        folded_text( const std::string &str, const int line_width );
        const std::vector<folded_line> &get_lines() const;
        // get the display coordinates of the codepoint at index `cpt_idx`
        point codepoint_coordinates( const int cpt_idx ) const;
};

folded_text::folded_text( const std::string &str, const int line_width )
    : line_width( line_width )
{
    // string pointer, remaining bytes, total codepoints, and total display width ...
    // ... before current processed character
    const char *src = str.c_str();
    int bytes = str.length();
    int cpts = 0;
    int width = 0;
    // ... before current line start
    const char *src_start = src;
    int bytes_start = bytes;
    int cpts_start = cpts;
    int width_start = width;
    // ... after last non-breaking character
    const char *src_nonbreak = src_start;
    // ... after last breaking character following at least one non-breaking character
    const char *src_break = src_start;
    int bytes_break = bytes_start;
    int cpts_break = cpts_start;
    int width_break = width_start;
    while( bytes > 0 ) {
        // ... before current processed character
        const char *const src_curr = src;
        const int bytes_curr = bytes;
        const int cpts_curr = cpts;
        const int width_curr = width;
        // ... after current processed character
        const uint32_t uc = UTF8_getch( &src, &bytes );
        const bool linebreak = is_linebreak( uc );
        // assuming character has non-negative width
        const int cw = linebreak ? 0 : std::max( 0, mk_wcwidth( uc ) );
        cpts += 1;
        width += cw;
        // if the characters so far do not fit in a single line
        // (including an end-of-line cursor if the last character is a line break)
        if( width > width_start + line_width
            || ( width == width_start + line_width && linebreak ) ) {
            if( src_break > src_start ) {
                // break after previous breaking character in the line if any
                lines.emplace_back( folded_line {
                    cpts_start, cpts_break,
                    std::string( src_start, src_break )
                } );
                src_start = src_break;
                bytes_start = bytes_break;
                cpts_start = cpts_break;
                width_start = width_break;
            } else {
                // otherwise break before the current character
                lines.emplace_back( folded_line {
                    cpts_start, cpts_curr,
                    std::string( src_start, src_curr )
                } );
                src_start = src_curr;
                bytes_start = bytes_curr;
                cpts_start = cpts_curr;
                width_start = width_curr;
            }
        }
        // always break on line breaks
        if( linebreak ) {
            lines.emplace_back( folded_line {
                cpts_start, cpts,
                std::string( src_start, src )
            } );
            src_start = src;
            bytes_start = bytes;
            cpts_start = cpts;
            width_start = width;
        }
        // record position of breaking and non-breaking characters
        if( is_breaking( uc ) ) {
            // only record a breaking character if it follows at least one
            // non-breaking character after the start of the line
            if( src_nonbreak > src_start ) {
                src_break = src;
                bytes_break = bytes;
                cpts_break = cpts;
                width_break = width;
            }
        } else {
            src_nonbreak = src;
        }
    }
    // remaining characters (or empty line if the string is empty or the last
    // character is line break)
    lines.emplace_back( folded_line {
        cpts_start, cpts,
        std::string( src_start, src )
    } );
}

const std::vector<folded_line> &folded_text::get_lines() const
{
    return lines;
}

point folded_text::codepoint_coordinates( const int cpt_idx ) const
{
    if( lines.empty() ) {
        return point_zero;
    }
    // use upper_bound so the cursor after a full line the width of the window
    // is placed at the start of the next line
    auto it = std::upper_bound( lines.begin(), lines.end(), cpt_idx,
    []( const int p, const folded_line & l ) {
        return p < l.cpts_end;
    } );
    if( it == lines.end() ) {
        // past the last codepoint
        const point pos = point( utf8_wrapper( lines.back().str ).display_width(),
                                 lines.size() - 1 );
        // if the cursor does not fit into the last line, warp to next line instead
        if( pos.x >= line_width ) {
            return point( 0, pos.y + 1 );
        }
        return pos;
    } else {
        const char *src = it->str.c_str();
        int bytes = it->str.length();
        int width = 0;
        for( int i = 0; bytes > 0 && i < cpt_idx - it->cpts_start; ++i ) {
            const uint32_t uc = UTF8_getch( &src, &bytes );
            const bool linebreak = is_linebreak( uc );
            const int cw = linebreak ? 0 : std::max( 0, mk_wcwidth( uc ) );
            width += cw;
        }
        return point( width, std::distance( lines.begin(), it ) );
    }
}

string_editor_window::string_editor_window(
    const std::function<catacurses::window()> &create_window,
    const std::string &text )
{
    _create_window = create_window;
    _utext = utf8_wrapper( text );
}

string_editor_window::~string_editor_window() = default;

point string_editor_window::get_line_and_position()
{
    return _folded->codepoint_coordinates( _position );
}

void string_editor_window::print_editor()
{
    const int ftsize = std::max( static_cast<int>( _folded->get_lines().size() ),
                                 _cursor_display.y + 1 );
    const int middleofpage = _max.y / 2;

    int topoflist = 0;
    int bottomoflist = std::min( topoflist + _max.y, ftsize );

    if( _max.y <= ftsize ) {
        if( _cursor_display.y > middleofpage ) {
            topoflist = _cursor_display.y - middleofpage;
            bottomoflist = topoflist + _max.y;
        }
        if( _cursor_display.y + middleofpage >= ftsize ) {
            bottomoflist = ftsize ;
            topoflist = bottomoflist - _max.y;
        }
    }

    for( int i = topoflist; i < bottomoflist; i++ ) {
        const int y = i - topoflist;
        if( static_cast<size_t>( i ) < _folded->get_lines().size() ) {
            const folded_line &line = _folded->get_lines()[i];
            mvwprintz( _win, point( 1, y ), c_white, "%s", line.str );
            if( i == _cursor_display.y ) {
                uint32_t c_cursor = 0;
                const char *src = line.str.c_str();
                int len = line.str.length();
                int cpts = line.cpts_start;
                // display the cursor as the first non-zero-width character after
                // the cursor position if any
                while( len > 0 && ( cpts <= _position || mk_wcwidth( c_cursor ) < 1 ) ) {
                    c_cursor = UTF8_getch( &src, &len );
                    cpts += 1;
                }
                // but display cursor as space at end of line
                if( cpts <= _position || c_cursor == 0
                    || is_linebreak( c_cursor ) || mk_wcwidth( c_cursor ) < 1 ) {
                    c_cursor = ' ';
                }
                mvwprintz( _win, point( _cursor_display.x + 1, y ), h_white, "%s", utf32_to_utf8( c_cursor ) );
            }
        } else if( i == _cursor_display.y ) {
            // cursor past the end of text
            mvwprintz( _win, point( _cursor_display.x + 1, y ), h_white, " " );
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

void string_editor_window::create_context()
{
    ctxt = std::make_unique<input_context>( "default", keyboard_mode::keychar );
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
    // move cursor up while trying to keep the x coordinate
    if( _cursor_display.y > 0 ) {
        const folded_line &prev_line = _folded->get_lines()[_cursor_display.y - 1];
        _position = prev_line.cpts_start;
        _position += utf8_wrapper( prev_line.str ).substr_display( 0, _cursor_display.x ).size();
        // -1 because cursor past the last character is shown in the next line
        _position = std::min( _position, prev_line.cpts_end - 1 );
    } else if( !_folded->get_lines().empty() ) {
        const folded_line &last_line = _folded->get_lines().back();
        _position = last_line.cpts_start;
        _position += utf8_wrapper( last_line.str ).substr_display( 0, _cursor_display.x ).size();
        // not -1 because the cursor past the last character is shown in the same
        // line (except when the cursor would be past the right window border, but
        // the original cursor position is always less than the window width)
        _position = std::min( _position, last_line.cpts_end );
    }
}

void string_editor_window::cursor_down()
{
    // move cursor down while trying to keep the x coordinate
    if( _folded->get_lines().empty() ) {
        // do nothing
    } else if( static_cast<size_t>( _cursor_display.y + 2 ) == _folded->get_lines().size() ) {
        const folded_line &last_line = _folded->get_lines().back();
        _position = last_line.cpts_start;
        _position += utf8_wrapper( last_line.str ).substr_display( 0, _cursor_display.x ).size();
        // not -1 because the cursor past the last character is shown in the same
        // line (except when the cursor would be past the right window border, but
        // the original cursor position is always less than the window width)
        _position = std::min( _position, last_line.cpts_end );
    } else {
        const folded_line *next_line = nullptr;
        if( static_cast<size_t>( _cursor_display.y + 2 ) < _folded->get_lines().size() ) {
            next_line = &_folded->get_lines()[_cursor_display.y + 1];
        } else {
            next_line = &_folded->get_lines().front();
        }
        _position = next_line->cpts_start;
        _position += utf8_wrapper( next_line->str ).substr_display( 0, _cursor_display.x ).size();
        // -1 because cursor past the last character is shown in the next line
        _position = std::min( _position, next_line->cpts_end - 1 );
    }
}

const std::string &string_editor_window::query_string()
{
    if( !ctxt ) {
        create_context();
    }

    utf8_wrapper edit( ctxt->get_edittext() );
    if( _position == -1 ) {
        _position = _utext.length();
    }

    // fold the text
    bool refold = true;
    // calculate the cursor position
    bool reposition = true;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        _win = _create_window();
        _max.x = getmaxx( _win );
        _max.y =  getmaxy( _win );
        refold = true;
        ui.position_from_window( _win );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( refold ) {
            _folded = std::make_unique<folded_text>( _utext.str(), _max.x - 1 );
            refold = false;
            reposition = true;
        }
        if( reposition ) {
            _cursor_display = get_line_and_position();
            reposition = false;
        }
        werase( _win );
        print_editor();
        wnoutrefresh( _win );
    } );

    int ch = 0;
    do {
        ui_manager::redraw();

        const std::string action = ctxt->handle_input();

        if( action != "ANY_INPUT" ) {
            continue;
        }

        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;

        if( ch == KEY_ESCAPE ) {
#if defined(__ANDROID__)
            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                SDL_StopTextInput();
            }
#endif
            return _utext.str();
        } else if( ch == KEY_UP ) {
            cursor_up();
            reposition = true;
        } else if( ch == KEY_DOWN ) {
            cursor_down();
            reposition = true;
        } else if( ch == KEY_RIGHT ) {
            cursor_right();
            reposition = true;
        } else if( ch == KEY_LEFT ) {
            cursor_left();
            reposition = true;
        } else if( ch == 0x15 ) {                   // ctrl-u: delete all the things
            _position = 0;
            _utext.erase( 0 );
            refold = true;
        } else if( ch == KEY_BACKSPACE ) {
            if( _position > 0 && _position <= static_cast<int>( _utext.size() ) ) {
                _position--;
                _utext.erase( _position, 1 );
                refold = true;
            }
        } else if( ch == KEY_HOME ) {
            _position = 0;
            reposition = true;
        } else if( ch == KEY_END ) {
            _position = _utext.size();
            reposition = true;
        } else if( ch == KEY_DC ) {
            if( _position < static_cast<int>( _utext.size() ) ) {
                _utext.erase( _position, 1 );
                refold = true;
            }
        } else if( ch == 0x16 || ch == KEY_F( 2 ) || !ev.text.empty() || ch == KEY_ENTER || ch == '\n' ) {
            // ctrl-v, f2, or _utext input
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
                refold = true;
                edit = utf8_wrapper();
                ctxt->set_edittext( std::string() );
            }
        } else if( ev.edit_refresh ) {
            edit = utf8_wrapper( ev.edit );
            ctxt->set_edittext( ev.edit );
        }
    } while( true );

    return _utext.str();
}
