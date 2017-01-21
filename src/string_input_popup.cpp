#include "string_input_popup.h"

#include "uistate.h"
#include "input.h"
#include "catacharset.h"
#include "output.h"
#include "ui.h"

std::string string_input_popup( std::string title, int width, std::string input, std::string desc,
                                std::string identifier, int max_length, bool only_digits )
{
    nc_color title_color = c_ltred;
    nc_color desc_color = c_green;

    std::vector<std::string> descformatted;

    int titlesize = utf8_width( title );
    int startx = titlesize + 2;
    if( max_length == 0 ) {
        max_length = width;
    }
    int w_height = 3;
    int iPopupWidth = ( width == 0 ) ? FULL_SCREEN_WIDTH : width + titlesize + 5;
    if( iPopupWidth > FULL_SCREEN_WIDTH ) {
        iPopupWidth = FULL_SCREEN_WIDTH;
    }
    if( !desc.empty() ) {
        int twidth = utf8_width( remove_color_tags( desc ) );
        if( twidth > iPopupWidth - 4 ) {
            twidth = iPopupWidth - 4;
        }
        descformatted = foldstring( desc, twidth );
        w_height += descformatted.size();
    }
    int starty = 1 + descformatted.size();

    if( max_length == 0 ) {
        max_length = 1024;
    }
    int w_y = ( TERMY - w_height ) / 2;
    int w_x = ( ( TERMX > iPopupWidth ) ? ( TERMX - iPopupWidth ) / 2 : 0 );
    WINDOW *w = newwin( w_height, iPopupWidth, w_y,
                        ( ( TERMX > iPopupWidth ) ? ( TERMX - iPopupWidth ) / 2 : 0 ) );

    draw_border( w );

    int endx = iPopupWidth - 3;

    for( size_t i = 0; i < descformatted.size(); ++i ) {
        trim_and_print( w, 1 + i, 1, iPopupWidth - 2, desc_color, "%s", descformatted[i].c_str() );
    }
    trim_and_print( w, starty, 1, iPopupWidth - 2, title_color, "%s", title.c_str() );
    long key = 0;
    int pos = -1;
    std::string ret = string_input_win( w, input, max_length, startx, starty, endx, true, key, pos,
                                        identifier, w_x, w_y, true, only_digits );
    werase( w );
    wrefresh( w );
    delwin( w );
    refresh();
    return ret;
}

std::string string_input_win( WINDOW *w, std::string input, int max_length, int startx, int starty,
                              int endx, bool loop, long &ch, int &pos, std::string identifier,
                              int w_x, int w_y, bool dorefresh, bool only_digits,
                              std::map<long, std::function<void()>> callbacks, std::set<long> ch_code_blacklist )
{
    input_context ctxt( "STRING_INPUT" );
    ctxt.register_action( "ANY_INPUT" );
    std::string action;

    return string_input_win_from_context( w, ctxt, input, max_length, startx, starty, endx, loop,
                                          action, ch, pos, identifier, w_x, w_y, dorefresh,
                                          only_digits, false, callbacks, ch_code_blacklist );
}

std::string string_input_win_from_context( WINDOW *w, input_context &ctxt, std::string input,
        int max_length, int startx, int starty, int endx,
        bool loop, std::string &action, long &ch, int &pos,
        std::string identifier, int w_x, int w_y, bool dorefresh,
        bool only_digits, bool draw_only, std::map<long, std::function<void()>> callbacks,
        std::set<long> ch_code_blacklist )
{
    utf8_wrapper ret( input );
    nc_color string_color = c_magenta;
    nc_color cursor_color = h_ltgray;
    nc_color underscore_color = c_ltgray;
    if( pos == -1 ) {
        pos = ret.length();
    }
    int scrmax = endx - startx;
    // in output (console) cells, not characters of the string!
    int shift = 0;
    bool redraw = true;

    do {

        if( pos < 0 ) {
            pos = 0;
        }

        const size_t left_shift = ret.substr( 0, pos ).display_width();
        if( ( int )left_shift < shift ) {
            shift = 0;
        } else if( pos < ( int )ret.length() && ( int )left_shift + 1 >= shift + scrmax ) {
            // if the cursor is inside the input string, keep one cell right of
            // the cursor visible, because the cursor might be on a multi-cell
            // character.
            shift = left_shift - scrmax + 2;
        } else if( pos == ( int )ret.length() && ( int )left_shift >= shift + scrmax ) {
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
            mvwprintw( w, starty, startx, std::string( scrmax, ' ' ).c_str() );
            // Print the whole input string in default color
            mvwprintz( w, starty, startx, string_color, "%s", ds.c_str() );
            size_t sx = ds.display_width();
            // Print the cursor in its own color
            if( pos < ( int )ret.length() ) {
                utf8_wrapper cursor = ret.substr( pos, 1 );
                size_t a = pos;
                while( a > 0 && cursor.display_width() == 0 ) {
                    // A combination code point, move back to the earliest
                    // non-combination code point
                    a--;
                    cursor = ret.substr( a, pos - a + 1 );
                }
                size_t left_over = ret.substr( 0, a ).display_width() - shift;
                mvwprintz( w, starty, startx + left_over, cursor_color, "%s", cursor.c_str() );
            } else if( pos == max_length && max_length > 0 ) {
                mvwprintz( w, starty, startx + sx, cursor_color, " " );
                sx++; // don't override trailing ' '
            } else {
                mvwprintz( w, starty, startx + sx, cursor_color, "_" );
                sx++; // don't override trailing '_'
            }
            if( ( int )sx < scrmax ) {
                // could be scrolled out of view when the cursor is at the start of the input
                size_t l = scrmax - sx;
                if( max_length > 0 ) {
                    if( ( int )ret.length() >= max_length ) {
                        l = 0; // no more input possible!
                    } else if( pos == ( int )ret.length() ) {
                        // one '_' is already printed, formated as cursor
                        l = std::min<size_t>( l, max_length - ret.length() - 1 );
                    } else {
                        l = std::min<size_t>( l, max_length - ret.length() );
                    }
                }
                if( l > 0 ) {
                    mvwprintz( w, starty, startx + sx, underscore_color, std::string( l, '_' ).c_str() );
                }
            }
            wrefresh( w );
        }

        if( dorefresh ) {
            wrefresh( w );
        }

        if ( draw_only ) {
            return input;
        }

        bool return_key = false;
        action = ctxt.handle_input();
        const input_event ev = ctxt.get_raw_input();
        ch = ev.type == CATA_INPUT_KEYBOARD ? ev.get_first_input() : 0;

        if( callbacks[ch] ) {
            callbacks[ch]();
        }

        if( ch_code_blacklist.find( ch ) != ch_code_blacklist.end() ) {
            continue;
        }

        if( ch == KEY_ESCAPE ) {
            return "";
        } else if( ch == '\n' ) {
            return_key = true;
        } else if( ch == KEY_UP ) {
            if( !identifier.empty() ) {
                std::vector<std::string> &hist = uistate.gethistory( identifier );
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
                hmenu.w_y = w_y - hmenu.w_height;
                if( hmenu.w_y < 0 ) {
                    hmenu.w_y = 0;
                    hmenu.w_height = std::max( w_y, 4 );
                }
                hmenu.w_x = w_x;

                hmenu.query();
                if( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != ret.str() ) {
                    ret = hmenu.entries[hmenu.ret].txt;
                    if( hmenu.ret < ( int )hist.size() ) {
                        hist.erase( hist.begin() + hmenu.ret );
                        hist.push_back( ret.str() );
                    }
                    pos = ret.size();
                    redraw = true;
                } else if( hmenu.keypress == 'd' ) {
                    hist.clear();
                }
            }
        } else if( ch == KEY_DOWN || ch == KEY_NPAGE || ch == KEY_PPAGE || ch == KEY_BTAB || ch == 9 ) {
            /* absolutely nothing */
        } else if( ch == KEY_RIGHT ) {
            if( pos + 1 <= ( int )ret.size() ) {
                pos++;
            }
            redraw = true;
        } else if( ch == KEY_LEFT ) {
            if( pos > 0 ) {
                pos--;
            }
            redraw = true;
        } else if( ch == 0x15 ) {                      // ctrl-u: delete all the things
            pos = 0;
            ret.erase( 0 );
            redraw = true;
            // Move the cursor back and re-draw it
        } else if( ch == KEY_BACKSPACE ) {
            // but silently drop input if we're at 0, instead of adding '^'
            if( pos > 0 && pos <= ( int )ret.size() ) {
                //TODO: it is safe now since you only input ascii chars
                pos--;
                ret.erase( pos, 1 );
                redraw = true;
            }
        } else if( ch == KEY_HOME ) {
            pos = 0;
            redraw = true;
        } else if( ch == KEY_END ) {
            pos = ret.size();
            redraw = true;
        } else if( ch == KEY_DC ) {
            if( pos < ( int )ret.size() ) {
                ret.erase( pos, 1 );
                redraw = true;
            }
        } else if( ch == KEY_F( 2 ) ) {
            std::string tmp = get_input_string_from_file();
            int tmplen = utf8_width( tmp );
            if( tmplen > 0 && ( tmplen + utf8_width( ret.c_str() ) <= max_length || max_length == 0 ) ) {
                ret.append( tmp );
            }
        } else if( ch == ERR ) {
            // Ignore the error
        } else if( ch != 0 && only_digits && !isdigit( ch ) ) {
            return_key = true;
        } else if( max_length > 0 && ( int )ret.length() >= max_length ) {
            // no further input possible, ignore key
        } else if( !ev.text.empty() ) {
            const utf8_wrapper t( ev.text );
            ret.insert( pos, t );
            pos += t.length();
            redraw = true;
        }

        if( return_key ) { //"/n" return code
            {
                if( !identifier.empty() && !ret.empty() ) {
                    std::vector<std::string> &hist = uistate.gethistory( identifier );
                    if( hist.size() == 0 || hist[hist.size() - 1] != ret.str() ) {
                        hist.push_back( ret.str() );
                    }
                }
                return ret.str();
            }
        }
    } while( loop == true );
    return ret.str();
}
