#include "string_input_popup.h"

#include "uistate.h"
#include "input.h"
#include "catacharset.h"
#include "output.h"
#include "ui.h"
#include "compatibility.h"

#include <cstdlib>

string_input_popup::string_input_popup() = default;

string_input_popup::~string_input_popup() = default;

void string_input_popup::create_window()
{
    nc_color title_color = c_ltred;
    nc_color desc_color = c_green;

    const int titlesize = utf8_width( _title );
    if( _max_length <= 0 ) {
        _max_length = _width;
    }
    // 2 for border (top and bottom) and 1 for the input text line.
    int w_height = 2 + 1;
    int w_width = ( _width <= 0 ) ? FULL_SCREEN_WIDTH : _width + titlesize + 5;
    if( w_width > FULL_SCREEN_WIDTH ) {
        w_width = FULL_SCREEN_WIDTH;
    }

    std::vector<std::string> descformatted;
    if( !_description.empty() ) {
        const int twidth = std::min( utf8_width( remove_color_tags( _description ) ), w_width - 4 );
        descformatted = foldstring( _description, twidth );
        w_height += descformatted.size();
    }
    // length of title + border (left) + space
    _startx = titlesize + 2;
    // Below the description and below the top border
    _starty = 1 + descformatted.size();

    if( _max_length <= 0 ) {
        _max_length = 1024;
    }
    _endx = w_width - 3;
    _position = -1;

    const int w_y = ( TERMY - w_height ) / 2;
    const int w_x = std::max( ( TERMX - w_width ) / 2, 0 );
    w_ptr.reset( newwin( w_height, w_width, w_y, w_x ) );
    w = w_ptr.get();

    draw_border( w );

    for( size_t i = 0; i < descformatted.size(); ++i ) {
        trim_and_print( w, 1 + i, 1, w_width - 2, desc_color, "%s", descformatted[i].c_str() );
    }
    trim_and_print( w, _starty, 1, w_width - 2, title_color, "%s", _title.c_str() );
}

void string_input_popup::create_context()
{
    ctxt_ptr.reset( new input_context( "STRING_INPUT" ) );
    ctxt = ctxt_ptr.get();
    ctxt->register_action( "ANY_INPUT" );
}

const std::string &string_input_popup::query( const bool loop, const bool draw_only )
{
    if( !w ) {
        create_window();
    }
    if( !ctxt ) {
        create_context();
    }
    utf8_wrapper ret( _text );
    nc_color string_color = c_magenta;
    nc_color cursor_color = h_ltgray;
    nc_color underscore_color = c_ltgray;
    if( _position == -1 ) {
        _position = ret.length();
    }
    int scrmax = _endx - _startx;
    // in output (console) cells, not characters of the string!
    int shift = 0;
    bool redraw = true;

    int ch = 0;

    do {

        if( _position < 0 ) {
            _position = 0;
        }

        const size_t left_shift = ret.substr( 0, _position ).display_width();
        if( ( int )left_shift < shift ) {
            shift = 0;
        } else if( _position < ( int )ret.length() && ( int )left_shift + 1 >= shift + scrmax ) {
            // if the cursor is inside the input string, keep one cell right of
            // the cursor visible, because the cursor might be on a multi-cell
            // character.
            shift = left_shift - scrmax + 2;
        } else if( _position == ( int )ret.length() && ( int )left_shift >= shift + scrmax ) {
            // cursor is behind the end of the input string, keep the
            // trailing '_' visible (always a single cell character)
            shift = left_shift - scrmax + 1;
        } else if( shift < 0 ) {
            shift = 0;
        }
        const size_t xleft_shift = ret.substr_display( 0, shift ).display_width();
        if( ( int )xleft_shift != shift ) {
            // This prevents a multi-cell character from been split, which is not possible
            // instead scroll a cell further to make that character disappear completely
            shift++;
        }

        if( redraw ) {
            redraw = false;
            // remove the scrolled out of view part from the input string
            const utf8_wrapper ds( ret.substr_display( shift, scrmax ) );
            // Clear the line
            mvwprintw( w, _starty, _startx, std::string( scrmax, ' ' ).c_str() );
            // Print the whole input string in default color
            mvwprintz( w, _starty, _startx, string_color, "%s", ds.c_str() );
            size_t sx = ds.display_width();
            // Print the cursor in its own color
            if( _position < ( int )ret.length() ) {
                utf8_wrapper cursor = ret.substr( _position, 1 );
                size_t a = _position;
                while( a > 0 && cursor.display_width() == 0 ) {
                    // A combination code point, move back to the earliest
                    // non-combination code point
                    a--;
                    cursor = ret.substr( a, _position - a + 1 );
                }
                size_t left_over = ret.substr( 0, a ).display_width() - shift;
                mvwprintz( w, _starty, _startx + left_over, cursor_color, "%s", cursor.c_str() );
            } else if( _position == _max_length && _max_length > 0 ) {
                mvwprintz( w, _starty, _startx + sx, cursor_color, " " );
                sx++; // don't override trailing ' '
            } else {
                mvwprintz( w, _starty, _startx + sx, cursor_color, "_" );
                sx++; // don't override trailing '_'
            }
            if( ( int )sx < scrmax ) {
                // could be scrolled out of view when the cursor is at the start of the input
                size_t l = scrmax - sx;
                if( _max_length > 0 ) {
                    if( ( int )ret.length() >= _max_length ) {
                        l = 0; // no more input possible!
                    } else if( _position == ( int )ret.length() ) {
                        // one '_' is already printed, formated as cursor
                        l = std::min<size_t>( l, _max_length - ret.length() - 1 );
                    } else {
                        l = std::min<size_t>( l, _max_length - ret.length() );
                    }
                }
                if( l > 0 ) {
                    mvwprintz( w, _starty, _startx + sx, underscore_color, std::string( l, '_' ).c_str() );
                }
            }
            wrefresh( w );
        }

        wrefresh( w );

        if( draw_only ) {
            return _text;
        }

        bool return_key = false;
        const std::string action = ctxt->handle_input();
        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == CATA_INPUT_KEYBOARD ? ev.get_first_input() : 0;

        if( callbacks[ch] ) {
            callbacks[ch]();
        }

        if( ch_code_blacklist.find( ch ) != ch_code_blacklist.end() ) {
            continue;
        }

        if( ch == KEY_ESCAPE ) {
            _text = std::string();
            _canceled = true;
            return _text;
        } else if( ch == '\n' ) {
            return_key = true;
        } else if( ch == KEY_UP ) {
            if( !_identifier.empty() ) {
                std::vector<std::string> &hist = uistate.gethistory( _identifier );
                uimenu hmenu;
                hmenu.title = _( "d: delete history" );
                hmenu.return_invalid = true;
                for( size_t h = 0; h < hist.size(); h++ ) {
                    hmenu.addentry( h, true, -2, hist[h].c_str() );
                }
                if( !ret.empty() && ( hmenu.entries.empty() ||
                                      hmenu.entries[hist.size() - 1].txt != ret.str() ) ) {
                    hmenu.addentry( hist.size(), true, -2, ret.str() );
                    hmenu.selected = hist.size();
                } else {
                    hmenu.selected = hist.size() - 1;
                }
                // number of lines that make up the menu window: title,2*border+entries
                hmenu.w_height = 3 + hmenu.entries.size();
                hmenu.w_y = getbegy( w ) - hmenu.w_height;
                if( hmenu.w_y < 0 ) {
                    hmenu.w_y = 0;
                    hmenu.w_height = std::max( getbegy( w ), 4 );
                }
                hmenu.w_x = getbegx( w );

                hmenu.query();
                if( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != ret.str() ) {
                    ret = hmenu.entries[hmenu.ret].txt;
                    if( hmenu.ret < ( int )hist.size() ) {
                        hist.erase( hist.begin() + hmenu.ret );
                        hist.push_back( ret.str() );
                    }
                    _position = ret.size();
                    redraw = true;
                } else if( hmenu.keypress == 'd' ) {
                    hist.clear();
                }
            }
        } else if( ch == KEY_DOWN || ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_BTAB || ch == 9 ) {
            /* absolutely nothing */
        } else if( ch == KEY_RIGHT ) {
            if( _position + 1 <= ( int )ret.size() ) {
                _position++;
            }
            redraw = true;
        } else if( ch == KEY_LEFT ) {
            if( _position > 0 ) {
                _position--;
            }
            redraw = true;
        } else if( ch == 0x15 ) {                      // ctrl-u: delete all the things
            _position = 0;
            ret.erase( 0 );
            redraw = true;
            // Move the cursor back and re-draw it
        } else if( ch == KEY_BACKSPACE ) {
            // but silently drop input if we're at 0, instead of adding '^'
            if( _position > 0 && _position <= ( int )ret.size() ) {
                //TODO: it is safe now since you only input ascii chars
                _position--;
                ret.erase( _position, 1 );
                redraw = true;
            }
        } else if( ch == KEY_HOME ) {
            _position = 0;
            redraw = true;
        } else if( ch == KEY_END ) {
            _position = ret.size();
            redraw = true;
        } else if( ch == KEY_DC ) {
            if( _position < ( int )ret.size() ) {
                ret.erase( _position, 1 );
                redraw = true;
            }
        } else if( ch == KEY_F( 2 ) ) {
            std::string tmp = get_input_string_from_file();
            int tmplen = utf8_width( tmp );
            if( tmplen > 0 && ( tmplen + utf8_width( ret.c_str() ) <= _max_length || _max_length == 0 ) ) {
                ret.append( tmp );
            }
        } else if( ch == ERR ) {
            // Ignore the error
        } else if( ch != 0 && _only_digits && !isdigit( ch ) ) {
            return_key = true;
        } else if( _max_length > 0 && ( int )ret.length() >= _max_length ) {
            // no further input possible, ignore key
        } else if( !ev.text.empty() ) {
            const utf8_wrapper t( ev.text );
            ret.insert( _position, t );
            _position += t.length();
            redraw = true;
        }

        if( return_key ) { //"/n" return code
            {
                if( !_identifier.empty() && !ret.empty() ) {
                    std::vector<std::string> &hist = uistate.gethistory( _identifier );
                    if( hist.size() == 0 || hist[hist.size() - 1] != ret.str() ) {
                        hist.push_back( ret.str() );
                    }
                }
                _text = ret.str();
                return _text;
            }
        }
    } while( loop == true );
    _text = ret.str();
    return _text;
}

string_input_popup &string_input_popup::window( WINDOW *w, int startx, int starty, int endx )
{
    w_ptr.reset();
    this->w = w;
    _startx = startx;
    _starty = starty;
    _endx = endx;
    return *this;
}

string_input_popup &string_input_popup::context( input_context &ctxt )
{
    ctxt_ptr.reset();
    this->ctxt = &ctxt;
    return *this;
}

void string_input_popup::edit( std::string &value )
{
    only_digits( true );
    text( value );
    query();
    if( !canceled() ) {
        value = text();
    }
}

void string_input_popup::edit( long &value )
{
    only_digits( true );
    text( to_string( value ) );
    query();
    if( !canceled() ) {
        value = std::atol( text().c_str() );
    }
}

void string_input_popup::edit( int &value )
{
    only_digits( true );
    text( to_string( value ) );
    query();
    if( !canceled() ) {
        value = std::atoi( text().c_str() );
    }
}
