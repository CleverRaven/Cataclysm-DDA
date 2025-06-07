#include "string_editor_window.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#if defined(__ANDROID__)
#include <SDL_keyboard.h>

#include "cata_utility.h"
#include "options.h"
#endif

#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "output.h"
#include "ui_manager.h"
#include "unicode.h"
#include "wcwidth.h"

static bool is_linebreak( const uint32_t uc )
{
    return uc == '\n';
}

static bool break_before( const uint32_t uc )
{
    return is_cjk_or_emoji( uc );
}

static bool break_after( const uint32_t uc )
{
    return uc == ' ' || is_cjk_or_emoji( uc );
}

static bool is_word( const uint32_t uc )
{
    return uc != ' ';
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
        std::vector<folded_line> lines;

    public:
        folded_text( const std::string &str, int line_width );
        const std::vector<folded_line> &get_lines() const;
        // get the display coordinates of the codepoint at index `cpt_idx`
        point codepoint_coordinates( int cpt_idx, bool zero_x ) const;
};

struct ime_preview_range {
    int begin;
    int end;
    point display_first;
    point display_last;
};

folded_text::folded_text( const std::string &str, const int line_width )
{
    // string pointer, remaining bytes, total codepoints, and total display width ...
    // ... before current processed character
    const char *src = str.c_str();
    int bytes = str.length();
    int cpts = 0;
    int width = 0;
    // ... before current line start
    const char *src_start = src;
    int cpts_start = cpts;
    int width_start = width;
    // ... after the last word character
    const char *src_word = src_start;
    // ... at the last breaking position
    const char *src_break = src_start;
    int cpts_break = cpts_start;
    int width_break = width_start;
    while( bytes > 0 ) {
        // ... before current processed character
        const char *const src_curr = src;
        const int cpts_curr = cpts;
        const int width_curr = width;
        // ... after current processed character
        const uint32_t uc = UTF8_getch( &src, &bytes );
        const bool linebreak = is_linebreak( uc );
        // assuming character has non-negative width
        const int cw = linebreak ? 0 : std::max( 0, mk_wcwidth( uc ) );
        cpts += 1;
        width += cw;
        // can we break before the current character?
        if( break_before( uc ) && src_curr > src_start
            // break with at least one word character before
            && src_word > src_start ) {
            src_break = src_curr;
            cpts_break = cpts_curr;
            width_break = width_curr;
        }
        // if the characters so far do not fit in a single line
        if( width > width_start + line_width ) {
            if( src_break > src_start ) {
                // break at the last breaking position in the line if any
                lines.emplace_back( folded_line {
                    cpts_start, cpts_break,
                    std::string( src_start, src_break )
                } );
                src_start = src_break;
                cpts_start = cpts_break;
                width_start = width_break;
            } else if( src_curr > src_start ) {
                // otherwise break before the current character, but ensure
                // each line has at least one character
                lines.emplace_back( folded_line {
                    cpts_start, cpts_curr,
                    std::string( src_start, src_curr )
                } );
                src_start = src_curr;
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
            cpts_start = cpts;
            width_start = width;
        }
        if( is_word( uc ) ) {
            src_word = src;
        }
        // can we break after the current character?
        if( break_after( uc ) && src > src_start
            // break with at least one word character before
            && src_word > src_start ) {
            src_break = src;
            cpts_break = cpts;
            width_break = width;
        }
    }
    // remaining characters (empty line if the string is empty or the last
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

point folded_text::codepoint_coordinates( const int cpt_idx, const bool zero_x ) const
{
    if( lines.empty() ) {
        return point::zero;
    }
    // find the line before the cursor position
    auto it = std::lower_bound( lines.begin(), lines.end(), cpt_idx,
    []( const folded_line & l, const int p ) {
        return l.cpts_end < p;
    } );
    if( it == lines.end() ) {
        // past the last codepoint, shouldn't happen
        return point::zero;
    }
    int y = std::distance( lines.begin(), it );
    // if zero_x is true and the line is not the last line, cursor at the end of
    // the line is moved to the start of the next line
    if( zero_x && static_cast<size_t>( y ) + 1 < lines.size()
        && cpt_idx == it->cpts_end ) {
        return point( 0, y + 1 );
    }
    // otherwise, calculate the width until cpt_idx
    int x = 0;
    const char *src = it->str.c_str();
    int bytes = it->str.length();
    for( int i = 0; bytes > 0 && i < cpt_idx - it->cpts_start; ++i ) {
        const uint32_t uc = UTF8_getch( &src, &bytes );
        if( is_linebreak( uc ) ) {
            x = 0;
            ++y;
        } else {
            x += std::max( 0, mk_wcwidth( uc ) );
        }
    }
    return point( x, y );
}

string_editor_window::string_editor_window(
    const std::function<catacurses::window()> &create_window,
    const std::string &text )
{
    _create_window = create_window;
    _utext = utf8_wrapper( text );
}

string_editor_window::~string_editor_window() = default;

point string_editor_window::get_line_and_position( const int position, const bool zero_x )
{
    return _folded->codepoint_coordinates( position, zero_x );
}

void string_editor_window::print_editor( ui_adaptor &ui )
{
    const point focus = _ime_preview_range ? _ime_preview_range->display_last : _cursor_display;
    const int ftsize = _folded->get_lines().size();
    const int middleofpage = _max.y / 2;

    int topoflist = 0;
    int bottomoflist = std::min( topoflist + _max.y, ftsize );

    if( _max.y <= ftsize ) {
        if( focus.y > middleofpage ) {
            topoflist = focus.y - middleofpage;
            bottomoflist = topoflist + _max.y;
        }
        if( focus.y + middleofpage >= ftsize ) {
            bottomoflist = ftsize ;
            topoflist = bottomoflist - _max.y;
        }
    }

    for( int i = topoflist; i < bottomoflist; i++ ) {
        const int y = i - topoflist;
        const folded_line &line = _folded->get_lines()[i];
        mvwprintz( _win, point( 1, y ), c_white, "%s", line.str );
        if( !_ime_preview_range && i == _cursor_display.y ) {
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
            const point cursor_pos( _cursor_display.x + 1, y );
            mvwprintz( _win, cursor_pos, h_white, "%s", utf32_to_utf8( c_cursor ) );
            ui.set_cursor( _win, cursor_pos );
        }
        if( _ime_preview_range && i >= _ime_preview_range->display_first.y
            && i <= _ime_preview_range->display_last.y ) {
            const int beg = std::max( 0, _ime_preview_range->begin - line.cpts_start );
            const int end = std::min( _ime_preview_range->end, line.cpts_end ) - line.cpts_start;
            const utf8_wrapper preview = utf8_wrapper( line.str ).substr( beg, end - beg );
            const point disp = i == _ime_preview_range->display_first.y
                               ? point( _ime_preview_range->display_first.x + 1, y )
                               : point( 1, y );
            mvwprintz( _win, disp, c_dark_gray_white, "%s", preview.str() );
        }
    }
    if( _ime_preview_range ) {
        const point cursor_pos = _ime_preview_range->display_last + point( 1, -topoflist );
        ui.set_cursor( _win, cursor_pos );
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
    ctxt = std::make_unique<input_context>( "STRING_EDITOR", keyboard_mode::keychar );
    ctxt->register_action( "TEXT.QUIT" );
    ctxt->register_action( "TEXT.CONFIRM" );
    ctxt->register_action( "TEXT.LEFT" );
    ctxt->register_action( "TEXT.RIGHT" );
    ctxt->register_action( "TEXT.UP" );
    ctxt->register_action( "TEXT.DOWN" );
    ctxt->register_action( "TEXT.CLEAR" );
    ctxt->register_action( "TEXT.BACKSPACE" );
    ctxt->register_action( "TEXT.HOME" );
    ctxt->register_action( "TEXT.END" );
    ctxt->register_action( "TEXT.PAGE_UP" );
    ctxt->register_action( "TEXT.PAGE_DOWN" );
    ctxt->register_action( "TEXT.DELETE" );
#if defined(TILES)
    ctxt->register_action( "TEXT.PASTE" );
#endif
    ctxt->register_action( "TEXT.INPUT_FROM_FILE" );
    ctxt->register_action( "HELP_KEYBINDINGS" );
    ctxt->register_action( "ANY_INPUT" );
}

void string_editor_window::cursor_leftright( const int diff )
{
    const int size = _utext.size();
    if( diff < 0 && _position <= 0 ) {
        // Warp to end
        _position = size;
    } else if( diff > 0 && _position >= size ) {
        // Warp to start
        _position = 0;
    } else {
        // Move at most `diff` codepoints without warping
        _position = clamp( _position + diff, 0, size );
    }
}

void string_editor_window::cursor_updown( const int diff )
{
    if( diff != 0 && !_folded->get_lines().empty() ) {
        const int size = _folded->get_lines().size();
        int new_y = 0;
        if( diff < 0 && _cursor_display.y <= 0 ) {
            // Warp to last line
            new_y = size - 1;
        } else if( diff > 0 && _cursor_display.y >= size - 1 ) {
            // Warp to first line
            new_y = 0;
        } else {
            // Move at most `diff` lines without warping
            new_y = clamp( _cursor_display.y + diff, 0, size - 1 );
        }
        const folded_line &new_line = _folded->get_lines()[new_y];
        utf8_wrapper ustr( new_line.str );
        if( !ustr.empty() && is_linebreak( ustr.at( ustr.size() - 1 ) ) ) {
            ustr = ustr.substr( 0, ustr.size() - 1 );
        }
        // Put cursor at tbe largest x coordinate in the line less or equal to
        // the desired position.
        const int offset = ustr.substr_display( 0, _cursor_desired_x ).size();
        _position = new_line.cpts_start + offset;
    }
}

std::pair<bool, std::string> string_editor_window::query_string()
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
        _cursor_desired_x = -1;
        refold = true;
        ui.position_from_window( _win );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( ui_adaptor & ui ) {
        if( refold ) {
            utf8_wrapper text = _utext;
            if( !edit.empty() ) {
                text.insert( _position, edit );
            }
            _folded = std::make_unique<folded_text>( text.str(), _max.x - 2 );
            refold = false;
            reposition = true;
        }
        if( reposition ) {
            _cursor_display = get_line_and_position( _position, _cursor_desired_x == 0 );
            if( _cursor_desired_x < 0 ) {
                _cursor_desired_x = _cursor_display.x;
            }
            if( edit.empty() ) {
                _ime_preview_range.reset();
            } else {
                _ime_preview_range = std::make_unique<ime_preview_range>( ime_preview_range {
                    _position, _position + static_cast<int>( edit.size() ),
                    _cursor_display, get_line_and_position( _position + edit.size() - 1, true )
                } );
            }
            reposition = false;
        }
        werase( _win );
        print_editor( ui );
        wnoutrefresh( _win );
    } );

#if defined(__ANDROID__)
    on_out_of_scope stop_text_input( []() {
        if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
            StopTextInput();
        }
    } );
#endif

    int ch = 0;
    do {
        ui_manager::redraw();

        const std::string action = ctxt->handle_input();
        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;

        if( action == "TEXT.QUIT" ) {
            return { false, _utext.str() };
        } else if( action == "TEXT.CONFIRM" ) {
            return { true, _utext.str() };
        } else if( action == "TEXT.UP" ) {
            if( edit.empty() ) {
                cursor_updown( -1 );
                reposition = true;
            }
        } else if( action == "TEXT.DOWN" ) {
            if( edit.empty() ) {
                cursor_updown( 1 );
                reposition = true;
            }
        } else if( action == "TEXT.RIGHT" ) {
            if( edit.empty() ) {
                cursor_leftright( 1 );
                _cursor_desired_x = -1;
                reposition = true;
            }
        } else if( action == "TEXT.LEFT" ) {
            if( edit.empty() ) {
                cursor_leftright( -1 );
                _cursor_desired_x = -1;
                reposition = true;
            }
        } else if( action == "TEXT.CLEAR" ) {
            _position = 0;
            _cursor_desired_x = -1;
            _utext.erase( 0 );
            refold = true;
        } else if( action == "TEXT.BACKSPACE" ) {
            if( _position > 0 && _position <= static_cast<int>( _utext.size() ) ) {
                _position--;
                _cursor_desired_x = -1;
                _utext.erase( _position, 1 );
                refold = true;
            }
        } else if( action == "TEXT.HOME" ) {
            if( edit.empty()
                && static_cast<size_t>( _cursor_display.y ) < _folded->get_lines().size() ) {
                _position = _folded->get_lines()[_cursor_display.y].cpts_start;
                // put the cursor at line start rather than the previous line end
                _cursor_desired_x = 0;
                reposition = true;
            }
        } else if( action == "TEXT.END" ) {
            if( edit.empty()
                && static_cast<size_t>( _cursor_display.y ) < _folded->get_lines().size() ) {
                _position = _folded->get_lines()[_cursor_display.y].cpts_end;
                const utf8_wrapper ustr( _folded->get_lines()[_cursor_display.y].str );
                if( is_linebreak( ustr.at( ustr.size() - 1 ) ) ) {
                    --_position;
                }
                _cursor_desired_x = -1;
                reposition = true;
            }
        } else if( action == "TEXT.PAGE_UP" ) {
            if( edit.empty() ) {
                cursor_updown( -_max.y );
                reposition = true;
            }
        } else if( action == "TEXT.PAGE_DOWN" ) {
            if( edit.empty() ) {
                cursor_updown( _max.y );
                reposition = true;
            }
        } else if( action == "TEXT.DELETE" ) {
            if( _position < static_cast<int>( _utext.size() ) ) {
                _cursor_desired_x = -1;
                _utext.erase( _position, 1 );
                refold = true;
            }
        } else if( action == "TEXT.PASTE" || action == "TEXT.INPUT_FROM_FILE"
                   || !ev.text.empty() || ch == KEY_ENTER || ch == '\n' ) {
            // paste, input from file, or text input
            std::string entered;
            if( action == "TEXT.PASTE" ) {
#if defined(TILES)
                if( edit.empty() ) {
                    char *const clip = SDL_GetClipboardText();
                    if( clip ) {
                        entered = clip;
                        SDL_free( clip );
                    }
                }
#endif
            } else if( action == "TEXT.INPUT_FROM_FILE" ) {
                if( edit.empty() ) {
                    entered = get_input_string_from_file();
                }
            } else if( ch == KEY_ENTER || ch == '\n' ) {
                if( edit.empty() ) {
                    entered = "\n";
                }
            } else {
                entered = ev.text;
            }
            if( !entered.empty() ) {
                utf8_wrapper insertion;
                const char *str = entered.c_str();

                int len = entered.length();
                while( len > 0 ) {
                    const uint32_t ch = UTF8_getch( &str, &len );
                    // Use mk_wcwidth to filter out control characters
                    if( ch == '\n' || mk_wcwidth( ch ) >= 0 ) {
                        insertion.append( utf8_wrapper( utf32_to_utf8( ch ) ) );
                    }
                }
                _utext.insert( _position, insertion );
                _position += insertion.length();
                _cursor_desired_x = -1;
                edit = utf8_wrapper();
                refold = true;
                ctxt->set_edittext( std::string() );
            }
        } else if( ev.edit_refresh ) {
            edit = utf8_wrapper( ev.edit );
            refold = true;
            ctxt->set_edittext( ev.edit );
        }
    } while( true );

    return { false, _utext.str() };
}
