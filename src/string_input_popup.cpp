#include "string_input_popup.h"

#include <cctype>

#include "cata_scope_helpers.h"
#include "catacharset.h"
#include "input.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"
#include "wcwidth.h"

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#if defined(__ANDROID__)
#include <SDL_keyboard.h>

#include "options.h"
#endif

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

string_input_popup::string_input_popup() = default;

string_input_popup::~string_input_popup() = default;

void string_input_popup::create_window()
{
    titlesize = utf8_width( remove_color_tags( _title ) ); // Occupied horizontal space
    if( _max_length <= 0 ) {
        _max_length = _width;
    }
    // 2 for border (top and bottom) and 1 for the input text line.
    w_height = 2 + 1;

    // |"w_width = width + titlesize (this text) + 5": _____  |
    w_width = FULL_SCREEN_WIDTH;
    if( _width <= 0 ) {
        _width = std::max( 5, FULL_SCREEN_WIDTH - titlesize - 5 ); // Default if unspecified
    } else {
        _width = std::min( FULL_SCREEN_WIDTH - 20, _width );
        w_width = _width + titlesize + 5;
    }

    title_split = { _title };
    if( w_width > FULL_SCREEN_WIDTH ) {
        // Out of horizontal space- wrap the title
        titlesize = FULL_SCREEN_WIDTH - _width - 5;
        w_width = FULL_SCREEN_WIDTH;

        for( int wraplen = w_width - 2; wraplen >= titlesize; wraplen-- ) {
            title_split = foldstring( _title, wraplen );
            if( utf8_width( title_split.back() ) <= titlesize ) {
                break;
            }
        }
        title_height = static_cast<int>( title_split.size() ) - 1;
        w_height += title_height;
    }

    if( !_description.empty() ) {
        const int twidth = std::min( utf8_width( remove_color_tags( _description ) ), w_width - 4 );
        description_height = foldstring( _description, twidth ).size();
        w_height += description_height;
        if( w_height > TERMY ) {
            description_height = TERMY - 2 - title_height - 1;
            w_height = TERMY;
        }
    }
    // length of title + border (left) + space
    _startx = titlesize + 1;

    if( _max_length <= 0 ) {
        _max_length = 1024;
    }
    _endx = w_width - 3;

    const int w_y = ( TERMY - w_height ) / 2;
    const int w_x = std::max( ( TERMX - w_width ) / 2, 0 );
    _starty = title_height;
    w_full = catacurses::newwin( w_height, w_width, point( w_x, w_y ) );
    if( !_description.empty() ) {
        w_description = catacurses::newwin( description_height, w_width - 1, point( w_x,
                                            w_y + 1 ) );
        desc_view_ptr = std::make_unique<scrolling_text_view>( w_description );
        desc_view_ptr->set_text( _description );
    }
    w_title_and_entry = catacurses::newwin( w_height - description_height - 2, w_width - 2,
                                            point( w_x + 1, w_y + 1 + description_height ) );

    custom_window = false;
}

void string_input_popup::create_context()
{
    ctxt_ptr = std::make_unique<input_context>( "STRING_INPUT", keyboard_mode::keychar );
    ctxt = ctxt_ptr.get();
    ctxt->register_action( "TEXT.QUIT" );
    ctxt->register_action( "TEXT.CONFIRM" );
    if( !_identifier.empty() ) {
        ctxt->register_action( "HISTORY_UP" );
        ctxt->register_action( "HISTORY_DOWN" );
    }
    ctxt->register_action( "TEXT.LEFT" );
    ctxt->register_action( "TEXT.RIGHT" );
    ctxt->register_action( "TEXT.CLEAR" );
    ctxt->register_action( "TEXT.BACKSPACE" );
    ctxt->register_action( "TEXT.HOME" );
    ctxt->register_action( "TEXT.END" );
    ctxt->register_action( "TEXT.DELETE" );
#if defined(TILES)
    ctxt->register_action( "TEXT.PASTE" );
#endif
    ctxt->register_action( "TEXT.INPUT_FROM_FILE" );
    ctxt->register_action( "HELP_KEYBINDINGS" );
    ctxt->register_action( "ANY_INPUT" );
    for( const auto &act : custom_actions ) {
        ctxt->register_action( act.first, act.second );
    }
}

void string_input_popup::show_history( utf8_wrapper &ret )
{
    if( _identifier.empty() ) {
        return;
    }
    std::vector<std::string> &hist = uistate.gethistory( _identifier );
    uilist hmenu;
    hmenu.title = _( "d: delete history" );
    hmenu.allow_anykey = true;
    for( size_t h = 0; h < hist.size(); h++ ) {
        hmenu.addentry( h, true, -2, hist[h] );
    }
    if( !ret.empty() && ( hmenu.entries.empty() ||
                          hmenu.entries[hist.size() - 1].txt != ret.str() ) ) {
        hmenu.addentry( hist.size(), true, -2, ret.str() );
    }

    if( !hmenu.entries.empty() ) {
        hmenu.selected = hmenu.entries.size() - 1;

        hmenu.w_height_setup = [&]() -> int {
            // number of lines that make up the menu window: 2*border+entries
            int height = 2 + hmenu.entries.size();
            if( getbegy( w_full ) < height )
            {
                height = std::max( getbegy( w_full ), 4 );
            }
            return height;
        };
        hmenu.w_x_setup = [&]( int ) -> int {
            return getbegx( w_full );
        };
        hmenu.w_y_setup = [&]( const int height ) -> int {
            return std::max( getbegy( w_full ) - height, 0 );
        };

        bool finished = false;
        do {
            hmenu.query();
            if( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != ret.str() ) {
                ret = hmenu.entries[hmenu.ret].txt;
                if( static_cast<size_t>( hmenu.ret ) < hist.size() ) {
                    hist.erase( hist.begin() + hmenu.ret );
                    hist.push_back( ret.str() );
                }
                _position = ret.size();
                finished = true;
            } else if( hmenu.ret == UILIST_UNBOUND && hmenu.ret_evt.get_first_input() == 'd' ) {
                hist.clear();
                finished = true;
            } else if( hmenu.ret != UILIST_UNBOUND ) {
                finished = true;
            }
        } while( !finished );
    }
}

void string_input_popup::add_to_history( const std::string &value ) const
{
    if( !_identifier.empty() && !value.empty() ) {
        std::vector<std::string> &hist = uistate.gethistory( _identifier );
        if( hist.empty() || hist[hist.size() - 1] != value ) {
            hist.push_back( value );
        }
    }
}

void string_input_popup::update_input_history( utf8_wrapper &ret, bool up )
{
    if( _identifier.empty() ) {
        return;
    }

    std::vector<std::string> &hist = uistate.gethistory( _identifier );

    if( hist.empty() ) {
        return;
    }

    if( hist.size() >= _hist_max_size ) {
        hist.erase( hist.begin(), hist.begin() + ( hist.size() - _hist_max_size ) );
    }

    if( up ) {
        if( _hist_str_ind >= static_cast<int>( hist.size() ) ) {
            return;
        } else if( _hist_str_ind == 0 ) {
            _session_str_entered = ret.str();

            //avoid showing the same result twice (after reopen filter window without reset)
            if( hist.size() > 1 && _session_str_entered == hist.back() ) {
                _hist_str_ind += 1;
            }
        }
    } else {
        if( _hist_str_ind == 1 ) {
            ret = _session_str_entered;
            _position = ret.length();
            //show initial string entered and 'return'
            _hist_str_ind = 0;
            return;
        } else if( _hist_str_ind == 0 ) {
            return;
        }
    }

    _hist_str_ind += up ? 1 : -1;
    ret = hist[hist.size() - _hist_str_ind];
    _position = ret.length();
}

void string_input_popup::draw( ui_adaptor *const ui, const utf8_wrapper &ret,
                               const utf8_wrapper &edit ) const
{
    if( !custom_window ) {
        werase( w_full );
        draw_border( w_full );
        wnoutrefresh( w_full );

        if( !_title.empty() ) {
            int pos_y = 0;
            for( int i = 0; i < static_cast<int>( title_split.size() ) - 1; i++ ) {
                mvwprintz( w_title_and_entry, point( i, pos_y++ ), _title_color, title_split[i] );
            }
            trim_and_print( w_title_and_entry, point( 0, pos_y ), titlesize, _title_color, title_split.back() );
        }
    }

    const int scrmax = _endx - _startx;
    // remove the scrolled out of view part from the input string
    const utf8_wrapper ds( ret.substr_display( shift, scrmax ) );
    int start_x_edit = _startx;
    // Clear the line
    mvwprintw( w_title_and_entry, point( _startx, _starty ), std::string( std::max( 0, scrmax ),
               ' ' ) );
    // Print the whole input string in default color
    mvwprintz( w_title_and_entry, point( _startx, _starty ), _string_color, "%s", ds.c_str() );
    size_t sx = ds.display_width();
    // Print the cursor in its own color
    point cursor_pos;
    if( _position >= 0 && static_cast<size_t>( _position ) < ret.length() ) {
        utf8_wrapper cursor = ret.substr( _position, 1 );
        size_t a = _position;
        while( a > 0 && cursor.display_width() == 0 ) {
            // A combination code point, move back to the earliest
            // non-combination code point
            a--;
            cursor = ret.substr( a, _position - a + 1 );
        }
        const size_t left_over = ret.substr( 0, a ).display_width() - shift;
        cursor_pos = point( _startx + left_over, _starty );
        mvwprintz( w_title_and_entry, cursor_pos, _cursor_color, "%s", cursor.c_str() );
        start_x_edit += left_over;
    } else if( _max_length > 0
               && ret.display_width() >= static_cast<size_t>( _max_length ) ) {
        cursor_pos = point( _startx + sx, _starty );
        mvwprintz( w_title_and_entry, cursor_pos, _cursor_color, " " );
        start_x_edit += sx;
        sx++; // don't override trailing ' '
    } else {
        cursor_pos = point( _startx + sx, _starty );
        mvwprintz( w_title_and_entry, cursor_pos, _cursor_color, "_" );
        start_x_edit += sx;
        sx++; // don't override trailing '_'
    }
    if( static_cast<int>( sx ) < scrmax ) {
        // could be scrolled out of view when the cursor is at the start of the input
        size_t l = scrmax - sx;
        if( _max_length > 0 ) {
            if( ret.display_width() >= static_cast<size_t>( _max_length ) ) {
                l = 0; // no more input possible!
            } else if( _position == static_cast<int>( ret.length() ) ) {
                // one '_' is already printed, formatted as cursor
                l = std::min<size_t>( l, _max_length - ret.display_width() - 1 );
            } else {
                l = std::min<size_t>( l, _max_length - ret.display_width() );
            }
        }
        if( l > 0 ) {
            mvwprintz( w_title_and_entry, point( _startx + sx, _starty ), _underscore_color, std::string( l,
                       '_' ) );
        }
    }
    if( !edit.empty() ) {
        mvwprintz( w_title_and_entry, point( start_x_edit, _starty ), _cursor_color, "%s", edit.c_str() );
    }
    wnoutrefresh( w_title_and_entry );

    std::unique_ptr<on_out_of_scope> move_cursor_and_refresh;
    if( ui ) {
        ui->set_cursor( w_title_and_entry, cursor_pos );
    } else {
        // This ensures the cursor is set last for calling UIs to record and set
        // for screen readers and IME preview
        move_cursor_and_refresh = std::make_unique<on_out_of_scope>( [&]() {
            wmove( w_title_and_entry, cursor_pos );
            wnoutrefresh( w_title_and_entry );
        } );
    }

    //Draw scrolling description
    if( !custom_window && desc_view_ptr ) {
        desc_view_ptr->draw( _desc_color );
        wnoutrefresh( w_description );
    }
}

void string_input_popup::query( const bool loop, const bool draw_only )
{
    query_string( loop, draw_only );
}

template<typename T>
T query_int_impl( string_input_popup &p, const bool loop, const bool draw_only )
{
    do {
        ret_val<T> result = try_parse_integer<T>( p.query_string( loop, draw_only ), true );
        if( p.canceled() ) {
            return 0;
        }
        if( result.success() ) {
            return result.value();
        }
        popup( result.str() );
    } while( loop );

    return 0;
}

int string_input_popup::query_int( const bool loop, const bool draw_only )
{
    return query_int_impl<int>( *this, loop, draw_only );
}

int64_t string_input_popup::query_int64_t( const bool loop, const bool draw_only )
{
    return query_int_impl<int64_t>( *this, loop, draw_only );
}

const std::string &string_input_popup::query_string( const bool loop, const bool draw_only )
{
    if( !custom_window && !w_full ) {
        create_window();
        _position = -1;
    }
    if( !ctxt ) {
        create_context();
    }

    if( desc_view_ptr ) {
        desc_view_ptr->set_up_navigation( *ctxt, scrolling_key_scheme::no_scheme, true );
    }

    _text_changed = false;
    utf8_wrapper ret( _text );
    utf8_wrapper edit( ctxt->get_edittext() );
    if( _position == -1 ) {
        _position = ret.length();
    }
    const int scrmax = std::max( 0, _endx - _startx );

    std::unique_ptr<ui_adaptor> ui;
    if( !draw_only && !custom_window ) {
        ui = std::make_unique<ui_adaptor>();
        ui->position_from_window( w_full );
        ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            create_window();
            ui.position_from_window( w_full );
        } );
        ui->on_redraw( [&]( ui_adaptor & ui ) {
            draw( &ui, ret, edit );
        } );
    }

    int ch = 0;

    _canceled = false;
    _confirmed = false;
    do {
        if( _text_changed ) {
            ret = utf8_wrapper( _text );
            _text_changed = false;
        }
        if( _position < 0 ) {
            _position = 0;
        }
        if( shift < 0 ) {
            shift = 0;
        }

        const size_t width_to_cursor_start = ret.substr( 0, _position ).display_width();
        size_t width_to_cursor_end = width_to_cursor_start;
        if( static_cast<size_t>( _position ) < ret.length() ) {
            width_to_cursor_end += ret.substr( _position, 1 ).display_width();
        } else {
            width_to_cursor_end += 1;
        }
        // starts scrolling when the cursor is this far from the start or end
        const size_t scroll_width = std::min( 10, scrmax / 5 );
        if( ret.display_width() < static_cast<size_t>( scrmax ) ) {
            shift = 0;
        } else if( width_to_cursor_start < static_cast<size_t>( shift ) + scroll_width ) {
            shift = std::max( width_to_cursor_start, scroll_width ) - scroll_width;
        } else if( width_to_cursor_end > static_cast<size_t>( shift + scrmax ) - scroll_width ) {
            shift = std::min( width_to_cursor_end + scroll_width, ret.display_width() ) - scrmax;
        }
        const utf8_wrapper text_before_start = ret.substr_display( 0, shift );
        const size_t width_before_start = text_before_start.display_width();
        if( width_before_start != static_cast<size_t>( shift ) ) {
            // This prevents a multi-cell character from been split, which is not possible
            // instead scroll a cell further to make that character disappear completely
            const size_t width_at_start = ret.substr( text_before_start.length(), 1 ).display_width();
            shift = width_before_start + width_at_start;
        }

        if( ui ) {
            ui_manager::redraw();
        } else {
            draw( nullptr, ret, edit );
        }

        if( draw_only ) {
            return _text;
        }

        const std::string action = ctxt->handle_input();
        const input_event ev = ctxt->get_raw_input();
        ch = ev.type == input_event_t::keyboard_char ? ev.get_first_input() : 0;
        _handled = true;

        bool next_loop = false;
        for( const auto &cb : callbacks ) {
            if( ( !std::get<0>( cb ).empty() && std::get<0>( cb ) == action ) ||
                ( std::get<1>( cb ) != INT_MIN && std::get<1>( cb ) == ch ) ) {
                if( std::get<2>( cb )() ) {
                    next_loop = true;
                    break;
                }
            }
        }
        if( next_loop ) {
            continue;
        }

        if( desc_view_ptr && desc_view_ptr->handle_navigation( action, *ctxt ) ) {
            // NO FURTHER ACTION REQUIRED
        } else if( action == "TEXT.QUIT" ) {
#if defined(__ANDROID__)
            if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                StopTextInput();
            }
#endif
            _text.clear();
            _position = -1;
            _canceled = true;
            return _text;
        } else if( action == "TEXT.CONFIRM" ) {
            add_to_history( ret.str() );
            _confirmed = true;
            _text = ret.str();
            if( !_hist_use_uilist ) {
                _hist_str_ind = 0;
                _session_str_entered.erase( 0 );
            }
            return _text;
        } else if( action == "HISTORY_UP" ) {
            if( !_identifier.empty() ) {
                if( edit.empty() ) {
                    if( _hist_use_uilist ) {
                        show_history( ret );
                    } else {
                        update_input_history( ret, true );
                    }
                }
            } else {
                _handled = false;
            }
        } else if( action == "HISTORY_DOWN" ) {
            if( !_identifier.empty() ) {
                if( edit.empty() && !_hist_use_uilist ) {
                    update_input_history( ret, false );
                }
            } else {
                _handled = false;
            }
        } else if( action == "TEXT.RIGHT" ) {
            if( edit.empty() && _position + 1 <= static_cast<int>( ret.size() ) ) {
                _position++;
            }
        } else if( action == "TEXT.LEFT" ) {
            if( edit.empty() && _position > 0 ) {
                _position--;
            }
        } else if( action == "TEXT.CLEAR" ) {
            _position = 0;
            ret.erase( 0 );
        } else if( action == "TEXT.BACKSPACE" ) {
            if( _position > 0 && _position <= static_cast<int>( ret.size() ) ) {
                _position--;
                ret.erase( _position, 1 );
            }
        } else if( action == "TEXT.HOME" ) {
            if( edit.empty() ) {
                _position = 0;
            }
        } else if( action == "TEXT.END" ) {
            if( edit.empty() ) {
                _position = ret.size();
            }
        } else if( action == "TEXT.DELETE" ) {
            if( _position < static_cast<int>( ret.size() ) ) {
                ret.erase( _position, 1 );
            }
        } else if( action == "TEXT.PASTE" || action == "TEXT.INPUT_FROM_FILE"
                   || ( action == "ANY_INPUT" && !ev.text.empty() ) ) {
            // paste, input from file, or text input
            // bail out early if already at length limit
            if( _max_length <= 0 || ret.display_width() < static_cast<size_t>( _max_length ) ) {
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
                } else {
                    entered = ev.text;
                }
                if( !entered.empty() ) {
                    utf8_wrapper insertion;
                    const char *str = entered.c_str();
                    int len = entered.length();
                    int width = ret.display_width();
                    while( len > 0 ) {
                        const uint32_t ch = UTF8_getch( &str, &len );
                        // Use mk_wcwidth to filter out control characters
                        if( _only_digits ? ch == '-' || isdigit( ch ) : mk_wcwidth( ch ) >= 0 ) {
                            const int newwidth = mk_wcwidth( ch );
                            if( _max_length <= 0 || width + newwidth <= _max_length ) {
                                insertion.append( utf8_wrapper( utf32_to_utf8( ch ) ) );
                                width += newwidth;
                            } else {
                                break;
                            }
                        }
                    }
                    ret.insert( _position, insertion );
                    _position += insertion.length();
                    edit = utf8_wrapper();
                    ctxt->set_edittext( std::string() );
                }
            }
        } else if( ev.edit_refresh ) {
            edit = utf8_wrapper( ev.edit );
            ctxt->set_edittext( ev.edit );
        } else {
            _handled = false;
        }
    } while( loop );
    _text = ret.str();
    return _text;
}

string_input_popup &string_input_popup::window( const catacurses::window &w, const point &start,
        int endx )
{
    if( !custom_window && this->w_full ) {
        // default window already created
        return *this;
    }
    this->w_title_and_entry = w;
    _startx = start.x;
    _starty = start.y;
    _endx = endx;
    custom_window = true;
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
    only_digits( false );
    text( value );
    query();
    if( !canceled() ) {
        value = text();
    }
}

template<typename T>
static void edit_integer( string_input_popup &p, T &value )
{
    p.only_digits( true );
    while( true ) {
        p.text( std::to_string( value ) );
        p.query();
        if( p.canceled() ) {
            break;
        }
        ret_val<T> parsed_val = try_parse_integer<T>( p.text(), true );
        if( parsed_val.success() ) {
            value = parsed_val.value();
            break;
        } else {
            popup( parsed_val.str() );
        }
    }
}

// NOLINTNEXTLINE(cata-no-long)
void string_input_popup::edit( long &value )
{
    edit_integer<long>( *this, value );
}

void string_input_popup::edit( int &value )
{
    edit_integer<int>( *this, value );
}

string_input_popup &string_input_popup::text( const std::string &value )
{
    _text = value;
    const size_t u8size = utf8_wrapper( _text ).size();
    if( _position < 0 || static_cast<size_t>( _position ) > u8size ) {
        _position = u8size;
    }
    _text_changed = true;
    return *this;
}

void string_input_popup::add_callback( const std::string &action,
                                       const std::function<bool()> &callback_func )
{
    callbacks.emplace_back( action, INT_MIN, callback_func );
}

void string_input_popup::add_callback( int input, const std::function<bool()> &callback_func )
{
    callbacks.emplace_back( "", input, callback_func );
}
