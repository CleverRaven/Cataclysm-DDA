#include "ui.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <iterator>
#include <memory>

#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "game.h"
#include "ime.h"
#include "input.h"
#include "output.h"
#include "player.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"

#if defined(__ANDROID__)
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

uilist::uilist( const std::string &hotkeys_override )
{
    init();
    if( !hotkeys_override.empty() ) {
        hotkeys = hotkeys_override;
    }
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
    assert( !test_mode ); // uilist should not be used in tests where there's no place for it
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
    keypress = 0;          // last keypress from (int)getch()
    window = catacurses::window();         // our window
    keymap.clear();        // keymap[int] == index, for entries[index]
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
    allow_cancel = true;     // allow canceling with "QUIT" action
    allow_additional = false; // do not return on unhandled additional actions
    hilight_disabled =
        false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    vshift = 0;              // scrolling menu offset
    vmax = 0;                // max entries area rows
    callback = nullptr;         // * uilist_callback
    filter.clear();          // filter string. If "", show everything
    fentries.clear();        // fentries is the actual display after filtering, and maps displayed entry number to actual entry number
    fselected = 0;           // fentries[selected]
    filtering = true;        // enable list display filtering via '/' or '.'
    filtering_nocase = true; // ignore case when filtering
    max_entry_len = 0;
    max_column_len = 0;      // for calculating space for second column

    hotkeys = DEFAULT_HOTKEYS;
    input_category = "UILIST";
    additional_actions.clear();
}

/**
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uilist::filterlist()
{
    bool notfiltering = ( !filtering || filter.empty() );
    int num_entries = entries.size();
    // TODO: && is_all_lc( filter )
    bool nocase = filtering_nocase;
    std::string fstr;
    fstr.reserve( filter.size() );
    if( nocase ) {
        transform( filter.begin(), filter.end(), std::back_inserter( fstr ), tolower );
    } else {
        fstr = filter;
    }
    fentries.clear();
    fselected = -1;
    int f = 0;
    for( int i = 0; i < num_entries; i++ ) {
        if( notfiltering || ( !nocase && static_cast<int>( entries[i].txt.find( filter ) ) != -1 ) ||
            lcmatch( entries[i].txt, fstr ) ) {
            fentries.push_back( i );
            if( i == selected && ( hilight_disabled || entries[i].enabled ) ) {
                fselected = f;
            } else if( i > selected && fselected == -1 && ( hilight_disabled || entries[i].enabled ) ) {
                // Past the previously selected entry, which has been filtered out,
                // choose another nearby entry instead.
                fselected = f;
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
    input_context ctxt( input_category );
    ctxt.register_updown();
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "ANY_INPUT" );
    filter_popup = std::make_unique<string_input_popup>();
    filter_popup->context( ctxt ).text( filter )
    .max_length( 256 )
    .window( window, point( 4, w_height - 1 ), w_width - 4 );
    ime_sentry sentry;
    do {
        ui_manager::redraw();
        filter = filter_popup->query_string( false );
        if( !filter_popup->canceled() ) {
            const std::string action = ctxt.input_to_action( ctxt.get_raw_input() );
            if( !scrollby( scroll_amount_from_action( action ) ) ) {
                filterlist();
            }
        }
    } while( !filter_popup->confirmed() && !filter_popup->canceled() );

    if( filter_popup->canceled() ) {
        filterlist();
    }

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

/**
 * Calculate sizes, populate arrays, initialize window
 */
void uilist::setup()
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
            if( entries[ i ].hotkey > 0 ) {
                keymap[ entries[ i ].hotkey ] = i;
            } else if( entries[ i ].hotkey == -1 && i < 100 ) {
                autoassign.push_back( i );
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
    size_t next_free_hotkey = 0;
    for( auto it = autoassign.begin(); it != autoassign.end() &&
         next_free_hotkey < hotkeys.size(); ++it ) {
        while( next_free_hotkey < hotkeys.size() ) {
            const int setkey = hotkeys[next_free_hotkey];
            next_free_hotkey++;
            if( keymap.count( setkey ) == 0 ) {
                entries[*it].hotkey = setkey;
                keymap[setkey] = *it;
                break;
            }
        }
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

    if( w_auto && w_width > TERMX ) {
        w_width = TERMX;
    }

    vmax = entries.size();
    int additional_lines = 2 + text_separator_line + // add two for top & bottom borders
                           static_cast<int>( textformatted.size() );
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

    window = catacurses::newwin( w_height, w_width, point( w_x, w_y ) );
    if( !window ) {
        abort();
    }

    if( !started ) {
        filterlist();
    }

    started = true;
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
    int sbside = ( pad_left <= 0 ? 0 : w_width - 1 );
    int estart = textformatted.size();
    if( estart > 0 ) {
        estart += 2;
    } else {
        estart = 1;
    }

    scrollbar()
    .offset_x( sbside )
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
void uilist::show()
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

    const int text_lines = textformatted.size();
    int estart = 1;
    if( !textformatted.empty() ) {
        for( int i = 0; i < text_lines; i++ ) {
            trim_and_print( window, point( 2, 1 + i ), getmaxx( window ) - 4, text_color, textformatted[i] );
        }

        mvwputch( window, point( 0, text_lines + 1 ), border_color, LINE_XXXO );
        for( int i = 1; i < w_width - 1; ++i ) {
            mvwputch( window, point( i, text_lines + 1 ), border_color, LINE_OXOX );
        }
        mvwputch( window, point( w_width - 1, text_lines + 1 ), border_color, LINE_XOXX );
        estart += text_lines + 1; // +1 for the horizontal line.
    }

    calcStartPos( vshift, fselected, vmax, fentries.size() );

    const int pad_size = std::max( 0, w_width - 2 - pad_left - pad_right );
    const std::string padspaces = std::string( pad_size, ' ' );

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
            if( entries[ ei ].hotkey >= 33 && entries[ ei ].hotkey < 126 ) {
                const nc_color hotkey_co = ei == selected ? hilight_color : hotkey_color;
                mvwprintz( window, point( pad_left + 2, estart + si ), entries[ ei ].enabled ? hotkey_co : co,
                           "%c", entries[ ei ].hotkey );
            }
            if( pad_size > 3 ) {
                // pad_size indicates the maximal width of the entry, it is used above to
                // activate the highlighting, it is used to override previous text there, but in both
                // cases printing starts at pad_left+1, here it starts at pad_left+4, so 3 cells less
                // to be used.
                const auto entry = utf8_wrapper( ei == selected ? remove_color_tags( entries[ ei ].txt ) :
                                                 entries[ ei ].txt );
                trim_and_print( window, point( pad_left + 4, estart + si ),
                                max_entry_len, co, "%s", entry.c_str() );

                if( max_column_len && !entries[ ei ].ctxt.empty() ) {
                    const auto centry = utf8_wrapper( ei == selected ? remove_color_tags( entries[ ei ].ctxt ) :
                                                      entries[ ei ].ctxt );
                    trim_and_print( window, point( getmaxx( window ) - max_column_len - 2, estart + si ),
                                    max_column_len, co, "%s", centry.c_str() );
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
    if( action == "UP" ) {
        return -1;
    } else if( action == "PAGE_UP" ) {
        return ( -vmax + 1 );
    } else if( action == "SCROLL_UP" ) {
        return -3;
    } else if( action == "DOWN" ) {
        return 1;
    } else if( action == "PAGE_DOWN" ) {
        return vmax - 1;
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

    bool looparound = ( scrollby == -1 || scrollby == 1 );
    bool backwards = ( scrollby < 0 );

    fselected += scrollby;
    if( !looparound ) {
        if( backwards && fselected < 0 ) {
            fselected = 0;
        } else if( fselected >= static_cast<int>( fentries.size() ) ) {
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
        if( fselected >= static_cast<int>( fentries.size() ) ) {
            fselected = 0;
        }
        for( size_t i = 0; i < fentries.size(); ++i ) {
            if( hilight_disabled || entries[ fentries [ fselected ] ].enabled ) {
                break;
            }
            ++fselected;
            if( fselected >= static_cast<int>( fentries.size() ) ) {
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
        current_ui->on_redraw( [this]( const ui_adaptor & ) {
            show();
        } );
        current_ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            reposition( ui );
        } );
        current_ui->mark_resize();
    }
    return current_ui;
}

/**
 * Handle input and update display
 *
 */
void uilist::query( bool loop, int timeout )
{
    keypress = 0;
    if( entries.empty() ) {
        ret = UILIST_ERROR;
        return;
    }
    ret = UILIST_WAIT_INPUT;

    input_context ctxt( input_category );
    ctxt.register_updown();
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    if( allow_cancel ) {
        ctxt.register_action( "QUIT" );
    }
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    for( const auto &additional_action : additional_actions ) {
        ctxt.register_action( additional_action.first, additional_action.second );
    }
    hotkeys = ctxt.get_available_single_char_hotkeys( hotkeys );

    shared_ptr_fast<ui_adaptor> ui = create_or_get_ui_adaptor();

    ui_manager::redraw();

#if defined(__ANDROID__)
    for( const auto &entry : entries ) {
        if( entry.hotkey > 0 && entry.enabled ) {
            ctxt.register_manual_key( entry.hotkey, entry.txt );
        }
    }
#endif

    do {
        ret_act = ctxt.handle_input( timeout );
        const auto event = ctxt.get_raw_input();
        keypress = event.get_first_input();
        const auto iter = keymap.find( keypress );

        if( scrollby( scroll_amount_from_action( ret_act ) ) ) {
            /* nothing */
        } else if( filtering && ret_act == "FILTER" ) {
            inputfilter();
        } else if( iter != keymap.end() ) {
            selected = iter->second;
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if( allow_disabled ) {
                ret = entries[selected].retval; // disabled
            }
            if( callback != nullptr ) {
                callback->select( this );
            }
        } else if( !fentries.empty() && ret_act == "CONFIRM" ) {
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if( allow_disabled ) {
                // disabled
                ret = entries[selected].retval;
            }
        } else if( allow_cancel && ret_act == "QUIT" ) {
            ret = UILIST_CANCEL;
        } else if( ret_act == "TIMEOUT" ) {
            ret = UILIST_TIMEOUT;
        } else {
            // including HELP_KEYBINDINGS, in case the caller wants to refresh their contents
            bool unhandled = callback == nullptr || !callback->key( ctxt, event, selected, this );
            if( unhandled && allow_anykey ) {
                ret = UILIST_UNBOUND;
            } else if( unhandled && allow_additional ) {
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

///@}
/**
 * cleanup
 */
void uilist::reset()
{
    window = catacurses::window();
    init();
}

void uilist::addentry( const std::string &str )
{
    entries.emplace_back( str );
}

void uilist::addentry( int r, bool e, int k, const std::string &str )
{
    entries.emplace_back( r, e, k, str );
}

void uilist::addentry_desc( const std::string &str, const std::string &desc )
{
    entries.emplace_back( str, desc );
}

void uilist::addentry_desc( int r, bool e, int k, const std::string &str, const std::string &desc )
{
    entries.emplace_back( r, e, k, str, desc );
}

void uilist::addentry_col( int r, bool e, int k, const std::string &str, const std::string &column,
                           const std::string &desc )
{
    entries.emplace_back( r, e, k, str, desc, column );
}

void uilist::settext( const std::string &str )
{
    text = str;
}

struct pointmenu_cb::impl_t {
    const std::vector< tripoint > &points;
    int last; // to suppress redrawing
    tripoint last_view; // to reposition the view after selecting
    shared_ptr_fast<game::draw_callback_t> terrain_draw_cb;

    impl_t( const std::vector<tripoint> &pts );
    ~impl_t();

    void select( uilist *menu );
};

pointmenu_cb::impl_t::impl_t( const std::vector<tripoint> &pts ) : points( pts )
{
    last = INT_MIN;
    last_view = g->u.view_offset;
    terrain_draw_cb = make_shared_fast<game::draw_callback_t>( [this]() {
        if( last >= 0 && static_cast<size_t>( last ) < points.size() ) {
            g->draw_trail_to_square( g->u.view_offset, true );
        }
    } );
    g->add_draw_callback( terrain_draw_cb );
}

pointmenu_cb::impl_t::~impl_t()
{
    g->u.view_offset = last_view;
}

void pointmenu_cb::impl_t::select( uilist *const menu )
{
    if( last == menu->selected ) {
        return;
    }
    last = menu->selected;
    if( menu->selected < 0 || menu->selected >= static_cast<int>( points.size() ) ) {
        g->u.view_offset = tripoint_zero;
    } else {
        const tripoint &center = points[menu->selected];
        g->u.view_offset = center - g->u.pos();
        // TODO: Remove this line when it's safe
        g->u.view_offset.z = 0;
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

