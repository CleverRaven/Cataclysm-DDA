#include "ui.h"

#include <cctype>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <set>

#include "avatar.h"
#include "cached_options.h" // IWYU pragma: keep
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "game.h"
#include "input.h"
#include "memory_fast.h"
#include "output.h"
#include "sdltiles.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"

#if defined(__ANDROID__)
#include <jni.h>
#include <SDL_keyboard.h>

#include "options.h"
#endif

catacurses::window new_centered_win( int nlines, int ncols )
{
    int height = std::min( nlines, TERMY );
    int width = std::min( ncols, TERMX );
    point pos( ( TERMX - width ) / 2, ( TERMY - height ) / 2 );
    return catacurses::newwin( height, width, pos );
}

/**
* \defgroup UI "The UI Menu."
* @{
*/

static std::optional<input_event> hotkey_from_char( const int ch )
{
    if( ch == MENU_AUTOASSIGN ) {
        return std::nullopt;
    } else if( ch <= 0 || ch == ' ' ) {
        return input_event();
    }
    switch( input_manager::actual_keyboard_mode( keyboard_mode::keycode ) ) {
        case keyboard_mode::keycode:
            if( ch >= 'A' && ch <= 'Z' ) {
                return input_event( std::set<keymod_t>( { keymod_t::shift } ),
                                    ch - 'A' + 'a', input_event_t::keyboard_code );
            } else {
                return input_event( ch, input_event_t::keyboard_code );
            }
        case keyboard_mode::keychar:
            return input_event( ch, input_event_t::keyboard_char );
    }
    return input_event();
}

uilist_entry::uilist_entry( const std::string &txt )
    : retval( -1 ), enabled( true ), hotkey( std::nullopt ), txt( txt ),
      text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const std::string &txt, const std::string &desc )
    : retval( -1 ), enabled( true ), hotkey( std::nullopt ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const std::string &txt, const int key )
    : retval( -1 ), enabled( true ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const std::string &txt, const std::optional<input_event> &key )
    : retval( -1 ), enabled( true ), hotkey( key ), txt( txt ),
      text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key,
                            const std::string &txt )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt, const std::string &desc )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key, const std::string &txt, const std::string &desc )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      desc( desc ), text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt, const std::string &desc,
                            const std::string &column )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      desc( desc ), ctxt( column ), text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled,
                            const std::optional<input_event> &key,
                            const std::string &txt, const std::string &desc,
                            const std::string &column )
    : retval( retval ), enabled( enabled ), hotkey( key ), txt( txt ),
      desc( desc ), ctxt( column ), text_color( c_red_red )
{
}

uilist_entry::uilist_entry( const int retval, const bool enabled, const int key,
                            const std::string &txt,
                            const nc_color &keycolor, const nc_color &txtcolor )
    : retval( retval ), enabled( enabled ), hotkey( hotkey_from_char( key ) ), txt( txt ),
      hotkey_color( keycolor ), text_color( txtcolor )
{
}

uilist::size_scalar &uilist::size_scalar::operator=( auto_assign )
{
    fun = nullptr;
    return *this;
}

uilist::size_scalar &uilist::size_scalar::operator=( const int val )
{
    fun = [val]() -> int {
        return val;
    };
    return *this;
}

uilist::size_scalar &uilist::size_scalar::operator=( const std::function<int()> &fun )
{
    this->fun = fun;
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( auto_assign )
{
    fun = nullptr;
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( const int val )
{
    fun = [val]( int ) -> int {
        return val;
    };
    return *this;
}

uilist::pos_scalar &uilist::pos_scalar::operator=( const std::function<int( int )> &fun )
{
    this->fun = fun;
    return *this;
}

uilist::uilist()
{
    init();
}

uilist::uilist( const std::string &msg, const std::vector<uilist_entry> &opts )
{
    init();
    text = msg;
    entries = opts;
    query();
}

uilist::uilist( const std::string &msg, const std::vector<std::string> &opts )
{
    init();
    text = msg;
    for( const std::string &opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
}

uilist::uilist( const std::string &msg, std::initializer_list<const char *const> opts )
{
    init();
    text = msg;
    for( const char *const opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
}

uilist::~uilist()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( current_ui ) {
        current_ui->reset();
    }
}

void uilist::color_error( const bool report )
{
    if( report ) {
        _color_error = report_color_error::yes;
    } else {
        _color_error = report_color_error::no;
    }
}

/*
 * Enables oneshot construction -> running -> exit
 */
uilist::operator int() const
{
    return ret;
}

/**
 * Sane defaults on initialization
 */
void uilist::init()
{
    cata_assert( !test_mode ); // uilist should not be used in tests where there's no place for it
    w_x_setup = pos_scalar::auto_assign {};
    w_y_setup = pos_scalar::auto_assign {};
    w_width_setup = size_scalar::auto_assign {};
    w_height_setup = size_scalar::auto_assign {};
    w_x = 0;
    w_y = 0;
    w_width = 0;
    w_height = 0;
    ret = UILIST_WAIT_INPUT;
    text.clear();          // header text, after (maybe) folding, populates:
    textformatted.clear(); // folded to textwidth
    textwidth = MENU_AUTOASSIGN; // if unset, folds according to w_width
    title.clear();         // Makes use of the top border, no folding, sets min width if w_width is auto
    ret_evt = input_event(); // last input event
    window = catacurses::window();         // our window
    keymap.clear();        // keymap[input_event] == index, for entries[index]
    selected = 0;          // current highlight, for entries[index]
    entries.clear();       // uilist_entry(int returnval, bool enabled, int keycode, std::string text, ... TODO: submenu stuff)
    started = false;       // set to true when width and key calculations are done, and window is generated.
    pad_left_setup = 0;
    pad_right_setup = 0;
    pad_left = 0;          // make a blank space to the left
    pad_right = 0;         // or right
    desc_enabled = false;  // don't show option description by default
    desc_lines_hint = 6;   // default number of lines for description
    desc_lines = 6;
    footer_text.clear();   // takes precedence over per-entry descriptions.
    border_color = c_magenta; // border color
    text_color = c_light_gray;  // text color
    title_color = c_green;  // title color
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_light_green; // hotkey text to the right of menu entry's text
    disabled_color = c_dark_gray; // disabled menu entry
    allow_disabled = false;  // disallow selecting disabled options
    allow_anykey = false;    // do not return on unbound keys
    allow_cancel = true;     // allow canceling with "UILIST.QUIT" action
    allow_confirm = true;     // allow confirming with confirm action
    allow_additional = false; // do not return on unhandled additional actions
    hilight_disabled =
        false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    vshift = 0;              // scrolling menu offset
    vmax = 0;                // max entries area rows
    callback = nullptr;         // * uilist_callback
    filter.clear();          // filter string. If "", show everything
    fentries.clear();        // fentries is the actual display after filtering, and maps displayed entry number to actual entry number
    fselected = 0;           // selected = fentries[fselected]
    filtering = true;        // enable list display filtering via '/' or '.'
    filtering_nocase = true; // ignore case when filtering
    max_entry_len = 0;
    max_column_len = 0;      // for calculating space for second column
    uilist_scrollbar = std::make_unique<scrollbar>();

    categories.clear();
    current_category = 0;
    category_lines = 0;

    input_category = "UILIST";
    additional_actions.clear();
}

input_context uilist::create_main_input_context() const
{
    input_context ctxt( input_category, keyboard_mode::keycode );
    ctxt.register_action( "UILIST.UP" );
    ctxt.register_action( "UILIST.DOWN" );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "HOME", to_translation( "Go to first entry" ) );
    ctxt.register_action( "END", to_translation( "Go to last entry" ) );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    if( allow_cancel ) {
        ctxt.register_action( "UILIST.QUIT" );
    }
    ctxt.register_action( "MOUSE_MOVE" );
    if( allow_confirm ) {
        ctxt.register_action( "CONFIRM" );
        ctxt.register_action( "SELECT" );
    }
    ctxt.register_action( "UILIST.FILTER" );
    if( !categories.empty() ) {
        ctxt.register_action( "UILIST.LEFT" );
        ctxt.register_action( "UILIST.RIGHT" );
    }
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    uilist_scrollbar->set_draggable( ctxt );
    for( const auto &additional_action : additional_actions ) {
        ctxt.register_action( additional_action.first, additional_action.second );
    }
    return ctxt;
}

input_context uilist::create_filter_input_context() const
{
    input_context ctxt( input_category, keyboard_mode::keychar );
    // string input popup actions
    ctxt.register_action( "TEXT.LEFT" );
    ctxt.register_action( "TEXT.RIGHT" );
    ctxt.register_action( "TEXT.QUIT" );
    ctxt.register_action( "TEXT.CONFIRM" );
    ctxt.register_action( "TEXT.CLEAR" );
    ctxt.register_action( "TEXT.BACKSPACE" );
    ctxt.register_action( "TEXT.HOME" );
    ctxt.register_action( "TEXT.END" );
    ctxt.register_action( "TEXT.DELETE" );
#if defined( TILES )
    ctxt.register_action( "TEXT.PASTE" );
#endif
    ctxt.register_action( "TEXT.INPUT_FROM_FILE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );
    // uilist actions
    ctxt.register_action( "UILIST.UP" );
    ctxt.register_action( "UILIST.DOWN" );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "HOME", to_translation( "Go to first entry" ) );
    ctxt.register_action( "END", to_translation( "Go to last entry" ) );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    if( allow_confirm ) {
        ctxt.register_action( "SELECT" );
    }
    ctxt.register_action( "MOUSE_MOVE" );
    return ctxt;
}

/**
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uilist::filterlist()
{
    // TODO: && is_all_lc( filter )
    fentries.clear();
    fselected = -1;
    int f = 0;
    for( size_t i = 0; i < entries.size(); i++ ) {
        bool visible = true;
        if( !categories.empty() && !category_filter( entries[i], categories[current_category].first ) ) {
            continue;
        }
        if( filtering && !filter.empty() ) {
            if( filtering_nocase ) {
                // case-insensitive match
                visible = lcmatch( entries[i].txt, filter );
            } else {
                // case-sensitive match
                visible = entries[i].txt.find( filter ) != std::string::npos;
            }
        }
        if( visible ) {
            fentries.push_back( static_cast<int>( i ) );
            if( hilight_disabled || entries[i].enabled ) {
                if( static_cast<int>( i ) == selected || ( static_cast<int>( i ) > selected && fselected == -1 ) ) {
                    // Either this is selected, or we are past the previously selected entry,
                    // which has been filtered out, so choose another nearby entry instead.
                    fselected = f;
                }
            }
            f++;
        }
    }
    if( fselected == -1 ) {
        fselected = 0;
        vshift = 0;
        if( fentries.empty() ) {
            selected = -1;
        } else {
            selected = fentries [ 0 ];
        }
    } else if( fselected < static_cast<int>( fentries.size() ) ) {
        selected = fentries[fselected];
    } else {
        fselected = selected = -1;
    }
    // scroll to top of screen if all remaining entries fit the screen.
    if( static_cast<int>( fentries.size() ) <= vmax ) {
        vshift = 0;
    }
    if( callback != nullptr ) {
        callback->select( this );
    }
}

void uilist::inputfilter()
{
    input_context ctxt = create_filter_input_context();
    filter_popup = std::make_unique<string_input_popup>();
    filter_popup->context( ctxt ).text( filter )
    .max_length( 256 )
    .window( window, point( 4, w_height - 1 ), w_width - 4 );
    bool loop = true;
    do {
        ui_manager::redraw();
        filter = filter_popup->query_string( false );
        recalc_start = false;
        if( !filter_popup->confirmed() ) {
            const std::string action = ctxt.input_to_action( ctxt.get_raw_input() );
            if( filter_popup->handled() ) {
                filterlist();
                recalc_start = true;
            } else if( scrollby( scroll_amount_from_action( action ) ) ) {
                recalc_start = true;
            } else if( handle_mouse( ctxt, action, true ) == handle_mouse_result_t::confirmed ) {
                loop = false;
            }
        }
    } while( loop && !filter_popup->confirmed() && !filter_popup->canceled() );

    filter_popup.reset();
}

/**
 * Find the minimum width between max( min_width, 1 ) and
 * max( max_width, min_width, 1 ) to fold the string to no more than max_lines,
 * or no more than the minimum number of lines possible, assuming that
 * foldstring( width ).size() decreases monotonously with width.
 **/
static int find_minimum_fold_width( const std::string &str, int max_lines,
                                    int min_width, int max_width )
{
    if( str.empty() ) {
        return std::max( min_width, 1 );
    }
    min_width = std::max( min_width, 1 );
    // max_width could be further limited by the string width, but utf8_width is
    // not handling linebreaks properly.

    if( min_width < max_width ) {
        // If with max_width the string still folds to more than max_lines, find the
        // minimum width that folds the string to such number of lines instead.
        max_lines = std::max<int>( max_lines, foldstring( str, max_width ).size() );
        while( min_width < max_width ) {
            int width = ( min_width + max_width ) / 2;
            // width may equal min_width, but will always be less than max_width.
            int lines = foldstring( str, width ).size();
            // If the current width folds the string to no more than max_lines
            if( lines <= max_lines ) {
                // The minimum width is between min_width and width.
                max_width = width;
            } else {
                // The minimum width is between width + 1 and max_width.
                min_width = width + 1;
            }
            // The new interval will always be smaller than the previous one,
            // so the loop is guaranteed to end.
        }
    }
    return min_width;
}

void uilist::calc_data()
{
    bool w_auto = !w_width_setup.fun;

    // Space for a line between text and entries. Only needed if there is actually text.
    const int text_separator_line = text.empty() ? 0 : 1;
    if( w_auto ) {
        w_width = 4;
        if( !title.empty() ) {
            w_width = utf8_width( title ) + 5;
        }
    } else {
        w_width = w_width_setup.fun();
    }
    const int max_desc_width = w_auto ? TERMX - 4 : w_width - 4;

    bool h_auto = !w_height_setup.fun;
    if( h_auto ) {
        w_height = 4;
    } else {
        w_height = w_height_setup.fun();
    }

    max_entry_len = 0;
    max_column_len = 0;
    desc_lines = desc_lines_hint;
    std::vector<int> autoassign;
    pad_left = pad_left_setup.fun ? pad_left_setup.fun() : 0;
    pad_right = pad_right_setup.fun ? pad_right_setup.fun() : 0;
    int pad = pad_left + pad_right + 2;
    int descwidth_final = 0; // for description width guard
    for( size_t i = 0; i < entries.size(); i++ ) {
        int txtwidth = utf8_width( remove_color_tags( entries[i].txt ) );
        int ctxtwidth = utf8_width( remove_color_tags( entries[i].ctxt ) );
        if( txtwidth > max_entry_len ) {
            max_entry_len = txtwidth;
        }
        if( ctxtwidth > max_column_len ) {
            max_column_len = ctxtwidth;
        }
        int clen = ( ctxtwidth > 0 ) ? ctxtwidth + 2 : 0;
        if( entries[ i ].enabled ) {
            if( !entries[i].hotkey.has_value() ) {
                autoassign.emplace_back( static_cast<int>( i ) );
            } else if( entries[i].hotkey.value() != input_event() ) {
                keymap[entries[i].hotkey.value()] = i;
            }
            if( entries[ i ].retval == -1 ) {
                entries[ i ].retval = i;
            }
            if( w_auto && w_width < txtwidth + pad + 4 + clen ) {
                w_width = txtwidth + pad + 4 + clen;
            }
        } else {
            if( w_auto && w_width < txtwidth + pad + 4 + clen ) {
                // TODO: or +5 if header
                w_width = txtwidth + pad + 4 + clen;
            }
        }
        if( desc_enabled ) {
            const int min_desc_width = std::min( max_desc_width, std::max( w_width, descwidth_final ) - 4 );
            int descwidth = find_minimum_fold_width( footer_text.empty() ? entries[i].desc : footer_text,
                            desc_lines, min_desc_width, max_desc_width );
            descwidth += 4; // 2x border + 2x ' ' pad
            if( descwidth_final < descwidth ) {
                descwidth_final = descwidth;
            }
        }
        if( entries[ i ].text_color == c_red_red ) {
            entries[ i ].text_color = text_color;
        }
    }
    input_context ctxt = create_main_input_context();
    const hotkey_queue &hotkeys = hotkey_queue::alpha_digits();
    input_event hotkey = ctxt.first_unassigned_hotkey( hotkeys );
    for( auto it = autoassign.begin(); it != autoassign.end() &&
         hotkey != input_event(); ++it ) {
        bool assigned = false;
        do {
            if( keymap.count( hotkey ) == 0 ) {
                entries[*it].hotkey = hotkey;
                keymap[hotkey] = *it;
                assigned = true;
            }
            hotkey = ctxt.next_unassigned_hotkey( hotkeys, hotkey );
        } while( !assigned && hotkey != input_event() );
    }

    if( desc_enabled ) {
        if( descwidth_final > TERMX ) {
            desc_enabled = false; // give up
        } else if( descwidth_final > w_width ) {
            w_width = descwidth_final;
        }

    }

    if( !text.empty() ) {
        int twidth = utf8_width( remove_color_tags( text ) );
        bool formattxt = true;
        int realtextwidth = 0;
        if( textwidth == -1 ) {
            if( !w_auto ) {
                realtextwidth = w_width - 4;
            } else {
                realtextwidth = twidth;
                if( twidth + 4 > w_width ) {
                    if( realtextwidth + 4 > TERMX ) {
                        realtextwidth = TERMX - 4;
                    }
                    textformatted = foldstring( text, realtextwidth );
                    formattxt = false;
                    realtextwidth = 10;
                    for( auto &l : textformatted ) {
                        const int w = utf8_width( remove_color_tags( l ) );
                        if( w > realtextwidth ) {
                            realtextwidth = w;
                        }
                    }
                    if( realtextwidth + 4 > w_width ) {
                        w_width = realtextwidth + 4;
                    }
                }
            }
        } else if( textwidth != -1 ) {
            realtextwidth = textwidth;
            if( realtextwidth + 4 > w_width ) {
                w_width = realtextwidth + 4;
            }
        }
        if( formattxt ) {
            textformatted = foldstring( text, realtextwidth );
        }
    }

    // shrink-to-fit
    if( desc_enabled ) {
        desc_lines = 0;
        for( const uilist_entry &ent : entries ) {
            // -2 for borders, -2 for padding
            desc_lines = std::max<int>( desc_lines, foldstring( footer_text.empty() ? ent.desc : footer_text,
                                        w_width - 4 ).size() );
        }
        if( desc_lines <= 0 ) {
            desc_enabled = false;
        }
    }
    if( !categories.empty() ) {
        category_lines = 0;
        for( const std::pair<std::string, std::string> &pair : categories ) {
            // -2 for borders, -2 for padding
            category_lines = std::max<int>( category_lines, foldstring( pair.second, w_width - 4 ).size() );
        }
    }

    if( w_auto && w_width > TERMX ) {
        w_width = TERMX;
    }

    vmax = entries.size();
    int additional_lines = 2 + text_separator_line + // add two for top & bottom borders
                           static_cast<int>( textformatted.size() );
    if( !categories.empty() ) {
        additional_lines += category_lines + 1;
    }
    if( desc_enabled ) {
        additional_lines += desc_lines + 1; // add one for description separator line
    }

    if( h_auto ) {
        w_height = vmax + additional_lines;
    }

    if( w_height > TERMY ) {
        w_height = TERMY;
    }

    if( vmax + additional_lines > w_height ) {
        vmax = w_height - additional_lines;
    }

    if( !w_x_setup.fun ) {
        w_x = static_cast<int>( ( TERMX - w_width ) / 2 );
    } else {
        w_x = w_x_setup.fun( w_width );
    }
    if( !w_y_setup.fun ) {
        w_y = static_cast<int>( ( TERMY - w_height ) / 2 );
    } else {
        w_y  = w_y_setup.fun( w_height );
    }
}

void uilist::setup()
{
    calc_data();

    window = catacurses::newwin( w_height, w_width, point( w_x, w_y ) );
    if( !window ) {
        cata_fatal( "Failed to create uilist window" );
    }

    if( !started ) {
        filterlist();
    }

    started = true;
    recalc_start = true;
}

void uilist::reposition( ui_adaptor &ui )
{
    setup();
    if( filter_popup ) {
        filter_popup->window( window, point( 4, w_height - 1 ), w_width - 4 );
    }
    ui.position_from_window( window );
}

void uilist::apply_scrollbar()
{
    int sbside = pad_left <= 0 ? 0 : w_width - 1;
    int estart = textformatted.size();
    if( estart > 0 ) {
        estart += 2;
    } else {
        estart = 1;
    }
    estart += category_lines > 0 ? category_lines + 1 : 0;

    uilist_scrollbar->offset_x( sbside )
    .offset_y( estart )
    .content_size( fentries.size() )
    .viewport_pos( vshift )
    .viewport_size( vmax )
    .border_color( border_color )
    .arrow_color( border_color )
    .slot_color( c_light_gray )
    .bar_color( c_cyan_cyan )
    .scroll_to_last( false )
    .apply( window );
}

/**
 * Generate and refresh output
 */
void uilist::show( ui_adaptor &ui )
{
    if( !started ) {
        setup();
    }

    werase( window );
    draw_border( window, border_color );
    if( !title.empty() ) {
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( window, point( 1, 0 ), border_color, "< " );
        wprintz( window, title_color, title );
        wprintz( window, border_color, " >" );
    }

    const auto print_line = [&]( int line ) {
        mvwputch( window, point( 0, line ), border_color, LINE_XXXO );
        for( int i = 1; i < w_width - 1; ++i ) {
            mvwputch( window, point( i, line ), border_color, LINE_OXOX );
        }
        mvwputch( window, point( w_width - 1, line ), border_color, LINE_XOXX );
    };
    const int text_lines = textformatted.size();
    int estart = 1;
    if( !textformatted.empty() ) {
        for( int i = 0; i < text_lines; i++ ) {
            trim_and_print( window, point( 2, 1 + i ), getmaxx( window ) - 4,
                            text_color, _color_error, "%s", textformatted[i] );
        }
        print_line( text_lines + 1 );
        estart += text_lines + 1; // +1 for the horizontal line.
    }
    if( !categories.empty() ) {
        mvwprintz( window, point( 1, estart ), c_yellow, "<< %s >>", categories[current_category].second );
        print_line( estart + category_lines );
        estart += category_lines + 1;
    }

    if( recalc_start ) {
        calcStartPos( vshift, fselected, vmax, fentries.size() );
    }

    const int pad_size = std::max( 0, w_width - 2 - pad_left - pad_right );
    const std::string padspaces = std::string( pad_size, ' ' );

    for( uilist_entry &entry : entries ) {
        entry.drawn_rect = std::nullopt;
    }

    // Entry text will be trimmed to this length for printing.  Need spacer at beginning/end, room for second column
    const int entry_space = pad_size - 2 - 1 - ( max_column_len > 0 ? max_column_len + 2 : 0 );
    for( int fei = vshift, si = 0; si < vmax; fei++, si++ ) {
        if( fei < static_cast<int>( fentries.size() ) ) {
            int ei = fentries [ fei ];
            nc_color co = ( ei == selected ?
                            hilight_color :
                            ( entries[ ei ].enabled || entries[ei].force_color ?
                              entries[ ei ].text_color :
                              disabled_color )
                          );

            mvwprintz( window, point( pad_left + 1, estart + si ), co, padspaces );
            if( entries[ei].hotkey.has_value() && entries[ei].hotkey.value() != input_event() ) {
                const nc_color hotkey_co = ei == selected ? hilight_color : hotkey_color;
                mvwprintz( window, point( pad_left + 1, estart + si ), entries[ ei ].enabled ? hotkey_co : co,
                           "%s", right_justify( entries[ei].hotkey.value().short_description(), 2 ) );
            }
            if( pad_size > 3 ) {
                // pad_size indicates the maximal width of the entry, it is used above to
                // activate the highlighting, it is used to override previous text there, but in both
                // cases printing starts at pad_left+1, here it starts at pad_left+4, so 3 cells less
                // to be used.
                const std::string &entry = ei == selected ? remove_color_tags( entries[ ei ].txt ) :
                                           entries[ ei ].txt;
                point p( pad_left + 4, estart + si );
                entries[ei].drawn_rect =
                    inclusive_rectangle<point>( p + point( -3, 0 ), p + point( -4 + pad_size, 0 ) );
                trim_and_print( window, p, entry_space, co, _color_error, "%s", entry );

                if( max_column_len && !entries[ ei ].ctxt.empty() ) {
                    const std::string &centry = ei == selected ? remove_color_tags( entries[ ei ].ctxt ) :
                                                entries[ ei ].ctxt;
                    trim_and_print( window, point( getmaxx( window ) - max_column_len - 2, estart + si ),
                                    max_column_len, co, _color_error, "%s", centry );
                }
            }
            mvwzstr menu_entry_extra_text = entries[ei].extratxt;
            if( !menu_entry_extra_text.txt.empty() ) {
                mvwprintz( window, point( pad_left + 1 + menu_entry_extra_text.left, estart + si ),
                           menu_entry_extra_text.color, menu_entry_extra_text.txt );
            }
            if( menu_entry_extra_text.sym != 0 ) {
                mvwputch( window, point( pad_left + 1 + menu_entry_extra_text.left, estart + si ),
                          menu_entry_extra_text.color, menu_entry_extra_text.sym );
            }
        } else {
            mvwprintz( window, point( pad_left + 1, estart + si ), c_light_gray, padspaces );
        }
    }

    if( desc_enabled ) {
        // draw border
        mvwputch( window, point( 0, w_height - desc_lines - 2 ), border_color, LINE_XXXO );
        for( int i = 1; i < w_width - 1; ++i ) {
            mvwputch( window, point( i, w_height - desc_lines - 2 ), border_color, LINE_OXOX );
        }
        mvwputch( window, point( w_width - 1, w_height - desc_lines - 2 ), border_color, LINE_XOXX );

        // clear previous desc the ugly way
        for( int y = desc_lines + 1; y > 1; --y ) {
            for( int x = 2; x < w_width - 2; ++x ) {
                mvwputch( window, point( x, w_height - y ), text_color, " " );
            }
        }

        if( static_cast<size_t>( selected ) < entries.size() ) {
            fold_and_print( window, point( 2, w_height - desc_lines - 1 ), w_width - 4, text_color,
                            footer_text.empty() ? entries[selected].desc : footer_text );
        }
    }

    if( filter_popup ) {
        mvwprintz( window, point( 2, w_height - 1 ), border_color, "< " );
        mvwprintz( window, point( w_width - 3, w_height - 1 ), border_color, " >" );
        filter_popup->query( /*loop=*/false, /*draw_only=*/true );
        // Record cursor immediately after filter_popup drawing
        ui.record_term_cursor();
    } else {
        if( !filter.empty() ) {
            mvwprintz( window, point( 2, w_height - 1 ), border_color, "< %s >", filter );
            mvwprintz( window, point( 4, w_height - 1 ), text_color, filter );
        }
    }
    apply_scrollbar();

    wnoutrefresh( window );
    if( callback != nullptr ) {
        callback->refresh( this );
    }
}

int uilist::scroll_amount_from_action( const std::string &action )
{
    const int scroll_rate = vmax > 20 ? 10 : 3;
    if( action == "UILIST.UP" ) {
        return -1;
    } else if( action == "PAGE_UP" ) {
        return -scroll_rate;
    } else if( action == "SCROLL_UP" ) {
        return -3;
    } else if( action == "HOME" ) {
        return -fselected;
    } else if( action == "END" ) {
        return fentries.size() - fselected - 1;
    } else if( action == "UILIST.DOWN" ) {
        return 1;
    } else if( action == "PAGE_DOWN" ) {
        return scroll_rate;
    } else if( action == "SCROLL_DOWN" ) {
        return +3;
    } else {
        return 0;
    }
}

/**
 * check for valid scrolling keypress and handle. return false if invalid keypress
 */
bool uilist::scrollby( const int scrollby )
{
    if( scrollby == 0 ) {
        return false;
    }

    bool looparound = scrollby == -1 || scrollby == 1;
    bool backwards = scrollby < 0;
    int recmax = static_cast<int>( fentries.size() );

    fselected += scrollby;
    if( !looparound ) {
        if( backwards && fselected < 0 ) {
            fselected = 0;
        } else if( fselected >= recmax ) {
            fselected = fentries.size() - 1;
        }
    }

    if( backwards ) {
        if( fselected < 0 ) {
            fselected = fentries.size() - 1;
        }
        for( size_t i = 0; i < fentries.size(); ++i ) {
            if( hilight_disabled || entries[ fentries [ fselected ] ].enabled ) {
                break;
            }
            --fselected;
            if( fselected < 0 ) {
                fselected = fentries.size() - 1;
            }
        }
    } else {
        if( fselected >= recmax ) {
            fselected = 0;
        }
        for( size_t i = 0; i < fentries.size(); ++i ) {
            if( hilight_disabled || entries[ fentries [ fselected ] ].enabled ) {
                break;
            }
            ++fselected;
            if( fselected >= recmax ) {
                fselected = 0;
            }
        }
    }
    if( static_cast<size_t>( fselected ) < fentries.size() ) {
        selected = fentries [ fselected ];
        if( callback != nullptr ) {
            callback->select( this );
        }
    }
    return true;
}

shared_ptr_fast<ui_adaptor> uilist::create_or_get_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> current_ui = ui.lock();
    if( !current_ui ) {
        ui = current_ui = make_shared_fast<ui_adaptor>();
        current_ui->on_redraw( [this]( ui_adaptor & ui ) {
            show( ui );
        } );
        current_ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            reposition( ui );
        } );
        current_ui->mark_resize();
    }
    return current_ui;
}

uilist::handle_mouse_result_t uilist::handle_mouse( const input_context &ctxt,
        const std::string &ret_act,
        const bool loop )
{
    handle_mouse_result_t result = handle_mouse_result_t::unhandled;
    int temp_pos = vshift;
    if( uilist_scrollbar->handle_dragging( ret_act, ctxt.get_coordinates_text( catacurses::stdscr ),
                                           temp_pos ) ) {
        scrollby( temp_pos - vshift );
        vshift = temp_pos;
        return handle_mouse_result_t::handled;
    }

    // Only check MOUSE_MOVE when looping internally
    if( !fentries.empty() && ( ret_act == "SELECT" || ( loop && ret_act == "MOUSE_MOVE" ) ) ) {
        result = handle_mouse_result_t::handled;
        const std::optional<point> p = ctxt.get_coordinates_text( window );
        if( p && window_contains_point_relative( window, p.value() ) ) {
            const int new_fselected = find_entry_by_coordinate( p.value() );
            if( new_fselected >= 0 && static_cast<size_t>( new_fselected ) < fentries.size() ) {
                const bool enabled = entries[fentries[new_fselected]].enabled;
                if( enabled || allow_disabled || hilight_disabled ) {
                    // Change the selection to display correctly after this
                    // function is called again.
                    fselected = new_fselected;
                    selected = fentries[fselected];
                    if( callback != nullptr ) {
                        callback->select( this );
                    }
                    if( ret_act == "SELECT" ) {
                        if( enabled || allow_disabled ) {
                            ret = entries[selected].retval;
                            // Treating clicking during filtering as confirmation and stop filtering
                            result = handle_mouse_result_t::confirmed;
                        }
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Handle input and update display
 *
 */
void uilist::query( bool loop, int timeout )
{
#if defined(__ANDROID__)
    bool auto_pos = w_x_setup.fun == nullptr && w_y_setup.fun == nullptr &&
                    w_width_setup.fun == nullptr && w_height_setup.fun == nullptr;

    if( get_option<bool>( "ANDROID_NATIVE_UI" ) && !entries.empty() && auto_pos ) {
        if( !started ) {
            calc_data();
            started = true;
        }
        JNIEnv *env = ( JNIEnv * )SDL_AndroidGetJNIEnv();
        jobject activity = ( jobject )SDL_AndroidGetActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID get_nativeui_method_id = env->GetMethodID( clazz, "getNativeUI",
                                           "()Lcom/cleverraven/cataclysmdda/NativeUI;" );
        jobject native_ui_obj = env->CallObjectMethod( activity, get_nativeui_method_id );
        jclass native_ui_cls( env->GetObjectClass( native_ui_obj ) );
        jmethodID list_menu_method_id = env->GetMethodID( native_ui_cls, "singleChoiceList",
                                        "(Ljava/lang/String;[Ljava/lang/String;[Z)I" );
        jstring jstr_message = env->NewStringUTF( text.c_str() );
        jobjectArray j_options = env->NewObjectArray( entries.size(), env->FindClass( "java/lang/String" ),
                                 env->NewStringUTF( "" ) );
        jbooleanArray j_enabled = env->NewBooleanArray( entries.size() );
        jboolean *n_enabled = new jboolean[entries.size()];
        for( std::size_t i = 0; i < entries.size(); i++ ) {
            std::string entry = remove_color_tags( entries[i].txt );
            if( !entries[i].ctxt.empty() ) {
                std::string ctxt = remove_color_tags( entries[i].ctxt );
                while( !ctxt.empty() && ctxt.back() == '\n' ) {
                    ctxt.pop_back();
                }
                if( !ctxt.empty() ) {
                    str_append( entry, "\n", ctxt );
                }
            }
            if( desc_enabled ) {
                std::string desc = remove_color_tags( entries[i].desc );
                while( !desc.empty() && desc.back() == '\n' ) {
                    desc.pop_back();
                }
                if( !desc.empty() ) {
                    str_append( entry, "\n", desc );
                }
            }
            env->SetObjectArrayElement( j_options, i, env->NewStringUTF( entry.c_str() ) );
            n_enabled[i] = entries[i].enabled;
        }
        env->SetBooleanArrayRegion( j_enabled, 0, entries.size(), n_enabled );
        int j_ret = env->CallIntMethod( native_ui_obj, list_menu_method_id, jstr_message, j_options,
                                        j_enabled );
        env->DeleteLocalRef( j_enabled );
        env->DeleteLocalRef( j_options );
        env->DeleteLocalRef( jstr_message );
        env->DeleteLocalRef( native_ui_cls );
        env->DeleteLocalRef( native_ui_obj );
        env->DeleteLocalRef( clazz );
        env->DeleteLocalRef( activity );
        delete[] n_enabled;
        if( j_ret == -1 ) {
            ret = UILIST_CANCEL;
        } else if( 0 <= j_ret && j_ret < entries.size() ) {
            ret = entries[j_ret].retval;
        } else {
            ret = UILIST_ERROR;
        }
        return;
    }
#endif
    ret_evt = input_event();
    if( entries.empty() ) {
        ret = UILIST_ERROR;
        return;
    }
    ret = UILIST_WAIT_INPUT;

    input_context ctxt = create_main_input_context();

    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();

    ui_manager::redraw();

#if defined(__ANDROID__)
    for( const auto &entry : entries ) {
        if( entry.enabled && entry.hotkey.has_value()
            && entry.hotkey.value() != input_event() ) {
            ctxt.register_manual_key( entry.hotkey.value().get_first_input(), entry.txt );
        }
    }
#endif

    do {
        ret_act = ctxt.handle_input( timeout );
        const input_event event = ctxt.get_raw_input();
        ret_evt = event;
        const auto iter = keymap.find( ret_evt );
        recalc_start = false;

        if( scrollby( scroll_amount_from_action( ret_act ) ) ) {
            recalc_start = true;
        } else if( filtering && ret_act == "UILIST.FILTER" ) {
            inputfilter();
        } else if( !categories.empty() && ( ret_act == "UILIST.LEFT" || ret_act == "UILIST.RIGHT" ) ) {
            current_category += ret_act == "UILIST.LEFT" ? -1 : 1;
            if( current_category < 0 ) {
                current_category = categories.size() - 1;
            } else if( current_category >= static_cast<int>( categories.size() ) ) {
                current_category = 0;
            }
            filterlist();
        } else if( iter != keymap.end() ) {
            const auto it = std::find( fentries.begin(), fentries.end(), iter->second );
            if( it != fentries.end() ) {
                const bool enabled = entries[*it].enabled;
                if( enabled || allow_disabled || hilight_disabled ) {
                    // Change the selection to display correctly when this function
                    // is called again.
                    fselected = std::distance( fentries.begin(), it );
                    selected = *it;
                    if( enabled || allow_disabled ) {
                        ret = entries[selected].retval;
                    }
                    if( callback != nullptr ) {
                        callback->select( this );
                    }
                }
            }
        } else if( handle_mouse( ctxt, ret_act, loop ) != handle_mouse_result_t::unhandled ) {
            // mouse handled, do nothing more
        } else if( allow_confirm && !fentries.empty() && ret_act == "CONFIRM" ) {
            if( entries[ selected ].enabled || allow_disabled ) {
                ret = entries[selected].retval;
            }
        } else if( allow_cancel && ret_act == "UILIST.QUIT" ) {
            ret = UILIST_CANCEL;
        } else if( ret_act == "TIMEOUT" ) {
            ret = UILIST_TIMEOUT;
        } else {
            // including HELP_KEYBINDINGS, in case the caller wants to refresh their contents
            bool unhandled = callback == nullptr || !callback->key( ctxt, event, selected, this );
            if( unhandled && allow_anykey ) {
                ret = UILIST_UNBOUND;
            } else if( unhandled && allow_additional ) {
                recalc_start = true;
                for( const auto &it : additional_actions ) {
                    if( it.first == ret_act ) {
                        ret = UILIST_ADDITIONAL;
                        break;
                    }
                }
            }
        }

        ui_manager::redraw();
    } while( loop && ret == UILIST_WAIT_INPUT );
}

int uilist::find_entry_by_coordinate( const point &p ) const
{
    for( auto it = fentries.begin(); it != fentries.end(); ++it ) {
        const uilist_entry &entry = entries[*it];
        if( entry.drawn_rect && entry.drawn_rect.value().contains( p ) ) {
            return std::distance( fentries.begin(), it );
        }
    }
    return -1;
}

///@}
/**
 * cleanup
 */
void uilist::reset()
{
    window = catacurses::window();
    init();
}

void uilist::addentry( const std::string &txt )
{
    entries.emplace_back( txt );
}

void uilist::addentry( int retval, bool enabled, int key, const std::string &txt )
{
    entries.emplace_back( retval, enabled, key, txt );
}

void uilist::addentry( const int retval, const bool enabled,
                       const std::optional<input_event> &key,
                       const std::string &txt )
{
    entries.emplace_back( retval, enabled, key, txt );
}

void uilist::addentry_desc( const std::string &txt, const std::string &desc )
{
    entries.emplace_back( txt, desc );
}

void uilist::addentry_desc( int retval, bool enabled, int key, const std::string &txt,
                            const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc );
}

void uilist::addentry_desc( int retval, bool enabled, const std::optional<input_event> &key,
                            const std::string &txt, const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc );
}

void uilist::addentry_col( int retval, bool enabled, int key, const std::string &txt,
                           const std::string &column,
                           const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc, column );
}

void uilist::addentry_col( const int retval, const bool enabled,
                           const std::optional<input_event> &key,
                           const std::string &txt, const std::string &column,
                           const std::string &desc )
{
    entries.emplace_back( retval, enabled, key, txt, desc, column );
}

void uilist::settext( const std::string &str )
{
    text = str;
}

void uilist::set_selected( int index )
{
    selected = std::clamp( index, 0, static_cast<int>( entries.size() - 1 ) );
}

void uilist::add_category( const std::string &key, const std::string &name )
{
    categories.emplace_back( key, name );
}

void uilist::set_category( const std::string &key )
{
    const auto it = std::find_if( categories.begin(),
    categories.end(), [key]( std::pair<std::string, std::string> &pair ) {
        return pair.first == key;
    } );
    current_category = std::distance( categories.begin(), it );
}

void uilist::set_category_filter( const
                                  std::function<bool( const uilist_entry &, const std::string & )> &fun )
{
    category_filter = fun;
}

void uimenu::addentry( int retval, bool enabled, const std::vector<std::string> &col_content )
{
    cata_assert( static_cast<int>( col_content.size() ) == col_count );
    cols.emplace_back( retval, enabled, col_content );
}

void uimenu::finalize_addentries()
{
    menu.entries.clear();
    std::vector<int> maxes( col_count, 0 );
    // get max width of each column
    for( col &c : cols ) {
        int i = 0;
        for( const std::string &entry : c.col_content ) {
            maxes[i] = std::max( maxes[i], utf8_width( entry, true ) );
            ++i;
        }
    }
    // adding spacing between columns
    int free_width = suggest_width - std::reduce( maxes.begin(), maxes.end() );
    int spacing = std::min( 3, col_count > 1 ? free_width / ( col_count - 1 ) : 0 );
    if( spacing > 0 ) {
        for( int i = 0; i < col_count - 1; ++i ) {
            maxes[i] += spacing;
        }
    }

    for( col &c : cols ) {
        std::string row;
        int i = 0;
        for( const std::string &entry : c.col_content ) {
            // Pad with spaces
            // Add length of tags to number of spaces to pad with
            // That is length(entry_with_tags) - length(entry_without_tags)
            // Otherwise the entry padding will be shorter by number of chars in tags
            int entry_len_plus_tags = maxes[i] + utf8_width( entry, false ) - utf8_width( entry, true );
            row += string_format( "%-*s", entry_len_plus_tags, entry );
            ++i;
        }
        menu.addentry( c.retval, c.enabled, -1, row );
    }
}

void uimenu::set_selected( int index )
{
    menu.selected = index;
}

void uimenu::set_title( const std::string &title )
{
    menu.title = title;
}

int uimenu::query()
{
    finalize_addentries();
    menu.query();
    return menu.ret;
}

struct pointmenu_cb::impl_t {
    const std::vector< tripoint > &points;
    int last; // to suppress redrawing
    tripoint last_view; // to reposition the view after selecting
    shared_ptr_fast<game::draw_callback_t> terrain_draw_cb;

    explicit impl_t( const std::vector<tripoint> &pts );
    ~impl_t();

    void select( uilist *menu );
};

pointmenu_cb::impl_t::impl_t( const std::vector<tripoint> &pts ) : points( pts )
{
    last = INT_MIN;
    avatar &player_character = get_avatar();
    last_view = player_character.view_offset;
    terrain_draw_cb = make_shared_fast<game::draw_callback_t>( [this, &player_character]() {
        if( last >= 0 && static_cast<size_t>( last ) < points.size() ) {
            g->draw_trail_to_square( player_character.view_offset, true );
        }
    } );
    g->add_draw_callback( terrain_draw_cb );
}

pointmenu_cb::impl_t::~impl_t()
{
    get_avatar().view_offset = last_view;
}

void pointmenu_cb::impl_t::select( uilist *const menu )
{
    if( last == menu->selected ) {
        return;
    }
    last = menu->selected;
    avatar &player_character = get_avatar();
    if( menu->selected < 0 || menu->selected >= static_cast<int>( points.size() ) ) {
        player_character.view_offset = tripoint_zero;
    } else {
        const tripoint &center = points[menu->selected];
        player_character.view_offset = center - player_character.pos();
        // TODO: Remove this line when it's safe
        player_character.view_offset.z = 0;
    }
    g->invalidate_main_ui_adaptor();
}

pointmenu_cb::pointmenu_cb( const std::vector<tripoint> &pts ) : impl( pts )
{
}

pointmenu_cb::~pointmenu_cb() = default;

void pointmenu_cb::select( uilist *const menu )
{
    impl->select( menu );
}

template bool navigate_ui_list<int, int>( const std::string &action, int &val, int page_delta,
        int size, bool wrap );
template bool navigate_ui_list<int, size_t>( const std::string &action, int &val, int page_delta,
        size_t size, bool wrap );
template bool navigate_ui_list<size_t, size_t>( const std::string &action, size_t &val,
        int page_delta, size_t size, bool wrap );
template bool navigate_ui_list<unsigned int, unsigned int>( const std::string &action,
        unsigned int &val,
        int page_delta, unsigned int size, bool wrap );

// Templating of existing `unsigned int` triggers linter rules against `unsigned long`
// NOLINTs below are to address
template<typename V, typename S>
bool navigate_ui_list( const std::string &action, V &val, int page_delta, S size, bool wrap )
{
    if( action == "UP" || action == "SCROLL_UP" || action == "DOWN" || action == "SCROLL_DOWN" ) {
        if( wrap ) {
            // NOLINTNEXTLINE(cata-no-long)
            val = inc_clamp_wrap( val, action == "DOWN" || action == "SCROLL_DOWN", static_cast<V>( size ) );
        } else {
            val = inc_clamp( val, action == "DOWN" || action == "SCROLL_DOWN",
                             // NOLINTNEXTLINE(cata-no-long)
                             static_cast<V>( size ? size - 1 : 0 ) );
        }
    } else if( ( action == "PAGE_UP" || action == "PAGE_DOWN" ) && page_delta ) {
        // page navigation never wraps
        val = inc_clamp( val, action == "PAGE_UP" ? -page_delta : page_delta,
                         // NOLINTNEXTLINE(cata-no-long)
                         static_cast<V>( size ? size - 1 : 0 ) );
    } else if( action == "HOME" ) {
        // NOLINTNEXTLINE(cata-no-long)
        val = static_cast<V>( 0 );
    } else if( action == "END" ) {
        // NOLINTNEXTLINE(cata-no-long)
        val = static_cast<V>( size ? size - 1 : 0 );
    } else {
        return false;
    }
    return true;
}

std::pair<int, int> subindex_around_cursor( int num_entries, int available_space, int cursor_pos,
        bool focused )
{
    if( !focused || num_entries <= available_space ) {
        return { 0, std::min( available_space, num_entries ) };
    }
    int slice_start = std::min( std::max( 0, cursor_pos - available_space / 2 ),
                                num_entries - available_space );
    return {slice_start, slice_start + available_space };
}
