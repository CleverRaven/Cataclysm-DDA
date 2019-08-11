#include "ui.h"

#include <assert.h>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <memory>

#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "output.h"
#include "player.h"
#include "string_input_popup.h"

#if defined(__ANDROID__)
#include <SDL_keyboard.h>

#include "options.h"
#endif

/**
* \defgroup UI "The UI Menu."
* @{
*/

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
    : uilist( MENU_AUTOASSIGN, MENU_AUTOASSIGN, MENU_AUTOASSIGN, msg, opts )
{
}

uilist::uilist( const std::string &msg, const std::vector<std::string> &opts )
    : uilist( MENU_AUTOASSIGN, MENU_AUTOASSIGN, MENU_AUTOASSIGN, msg, opts )
{
}

uilist::uilist( const std::string &msg, std::initializer_list<const char *const> opts )
    : uilist( MENU_AUTOASSIGN, MENU_AUTOASSIGN, MENU_AUTOASSIGN, msg, opts )
{
}

uilist::uilist( int startx, int width, int starty, const std::string &msg,
                const std::vector<uilist_entry> &opts )
{
    init();
    w_x = startx;
    w_y = starty;
    w_width = width;
    text = msg;
    entries = opts;
    query();
}

uilist::uilist( int startx, int width, int starty, const std::string &msg,
                const std::vector<std::string> &opts )
{
    init();
    w_x = startx;
    w_y = starty;
    w_width = width;
    text = msg;
    for( const auto &opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
}

uilist::uilist( int startx, int width, int starty, const std::string &msg,
                std::initializer_list<const char *const> opts )
{
    init();
    w_x = startx;
    w_y = starty;
    w_width = width;
    text = msg;
    for( auto opt : opts ) {
        entries.emplace_back( opt );
    }
    query();
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
    w_x = MENU_AUTOASSIGN;              // starting position
    w_y = MENU_AUTOASSIGN;              // -1 = auto center
    w_width = MENU_AUTOASSIGN;          // MENU_AUTOASSIGN = based on text width or max entry width, -2 = based on max entry, folds text
    w_height =
        MENU_AUTOASSIGN; // -1 = autocalculate based on number of entries + number of lines in text // FIXME: scrolling list with offset
    ret = UILIST_WAIT_INPUT;
    text.clear();          // header text, after (maybe) folding, populates:
    textformatted.clear(); // folded to textwidth
    textwidth = MENU_AUTOASSIGN; // if unset, folds according to w_width
    textalign = MENU_ALIGN_LEFT; // TODO:
    title.clear();         // Makes use of the top border, no folding, sets min width if w_width is auto
    keypress = 0;          // last keypress from (int)getch()
    window = catacurses::window();         // our window
    keymap.clear();        // keymap[int] == index, for entries[index]
    selected = 0;          // current highlight, for entries[index]
    entries.clear();       // uilist_entry(int returnval, bool enabled, int keycode, std::string text, ... TODO: submenu stuff)
    started = false;       // set to true when width and key calculations are done, and window is generated.
    pad_left = 0;          // make a blank space to the left
    pad_right = 0;         // or right
    desc_enabled = false;  // don't show option description by default
    desc_lines = 6;        // default number of lines for description
    footer_text.clear();   // takes precedence over per-entry descriptions.
    border = true;         // TODO: always true.
    border_color = c_magenta; // border color
    text_color = c_light_gray;  // text color
    title_color = c_green;  // title color
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_light_green; // hotkey text to the right of menu entry's text
    disabled_color = c_dark_gray; // disabled menu entry
    allow_disabled = false;  // disallow selecting disabled options
    allow_anykey = false;    // do not return on unbound keys
    allow_cancel = true;     // allow cancelling with "QUIT" action
    hilight_full = true;     // render hilight_color background over the entire line (minus padding)
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
    max_entry_len = 0;       // does nothing but can be read
    max_column_len = 0;      // for calculating space for second column

    scrollbar_auto =
        true;   // there is no force-on; true will only render scrollbar if entries > vertical height
    scrollbar_nopage_color =
        c_light_gray;    // color of '|' line for the entire area that isn't current page.
    scrollbar_page_color = c_cyan_cyan; // color of the '|' line for whatever's the current page.
    scrollbar_side = -1;     // -1 == choose left unless taken, then choose right

    hotkeys = DEFAULT_HOTKEYS;
    input_category = "UILIST";
    additional_actions.clear();
}

/**
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uilist::filterlist()
{
    bool notfiltering = ( ! filtering || filter.empty() );
    int num_entries = entries.size();
    bool nocase = filtering_nocase; // TODO: && is_all_lc( filter )
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
            if( i == selected ) {
                fselected = f;
            } else if( i > selected && fselected == -1 ) {
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
}

/**
 * Call string_input_win / ui_element_input::input_filter and filter the entries list interactively
 */
std::string uilist::inputfilter()
{
    std::string identifier; // TODO: uilist.filter_identifier ?
    mvwprintz( window, w_height - 1, 2, border_color, "< " );
    mvwprintz( window, w_height - 1, w_width - 3, border_color, " >" );
    /*
    //debatable merit
        std::string origfilter = filter;
        int origselected = selected;
        int origfselected = fselected;
        int origvshift = vshift;
    */
    string_input_popup popup;
    popup.text( filter )
    .max_length( 256 )
    .window( window, 4, w_height - 1, w_width - 4 )
    .identifier( identifier );
    input_event event;
#if defined(__ANDROID__)
    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
        SDL_StartTextInput();
    }
#endif
    do {
        // filter=filter_input->query(filter, false);
        filter = popup.query_string( false );
        event = popup.context().get_raw_input();
        // key = filter_input->keypress;
        if( event.get_first_input() != KEY_ESCAPE ) {
            if( !scrollby( scroll_amount_from_key( event.get_first_input() ) ) ) {
                filterlist();
            }
            show();
        }
    } while( event.get_first_input() != '\n' && event.get_first_input() != KEY_ESCAPE );

    if( event.get_first_input() == KEY_ESCAPE ) {
        /*
        //perhaps as an option
                filter = origfilter;
                selected = origselected;
                fselected = origfselected;
                vshift = origvshift;
        */
        filterlist();
    }

    wattron( window, border_color );
    for( int i = 1; i < w_width - 1; i++ ) {
        mvwaddch( window, point( i, w_height - 1 ), LINE_OXOX );
    }
    wattroff( window, border_color );

    return filter;
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
    bool w_auto = ( w_width == -1 || w_width == -2 );
    bool w_autofold = ( w_width == -2 );

    // Space for a line between text and entries. Only needed if there is actually text.
    const int text_separator_line = text.empty() ? 0 : 1;
    if( w_auto ) {
        w_width = 4;
        if( !title.empty() ) {
            w_width = title.size() + 5;
        }
    }

    bool h_auto = ( w_height == -1 );
    if( h_auto ) {
        w_height = 4;
    }

    if( desc_enabled && !( w_auto && h_auto ) ) {
        desc_enabled = false; // give up
        debugmsg( "desc_enabled without w_auto and h_auto (h: %d, w: %d)", static_cast<int>( h_auto ),
                  static_cast<int>( w_auto ) );
    }

    max_entry_len = 0;
    max_column_len = 0;
    std::vector<int> autoassign;
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
                w_width = txtwidth + pad + 4 + clen;    // TODO: or +5 if header
            }
        }
        if( desc_enabled ) {
            const int min_width = std::min( TERMX, std::max( w_width, descwidth_final ) ) - 4;
            const int max_width = TERMX - 4;
            int descwidth = find_minimum_fold_width( footer_text.empty() ? entries[i].desc : footer_text,
                            desc_lines, min_width, max_width );
            descwidth += 4; // 2x border + 2x ' ' pad
            if( descwidth_final < descwidth ) {
                descwidth_final = descwidth;
            }
        }
        if( entries[ i ].text_color == c_red_red ) {
            entries[ i ].text_color = text_color;
        }
        fentries.push_back( i );
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
            debugmsg( "description would exceed terminal width (%d vs %d available)", descwidth_final, TERMX );
        } else if( descwidth_final > w_width ) {
            w_width = descwidth_final;
        }

    }

    if( !text.empty() ) {
        int twidth = utf8_width( remove_color_tags( text ) );
        bool formattxt = true;
        int realtextwidth = 0;
        if( textwidth == -1 ) {
            if( w_autofold || !w_auto ) {
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
        if( vmax < 1 ) {
            if( textformatted.empty() ) {
                popup( "Can't display menu options, 0 %d available screen rows are occupied\nThis is probably a bug.\n",
                       TERMY );
            } else {
                popup( "Can't display menu options, %zu %d available screen rows are occupied by\n'%s\n(snip)\n%s'\nThis is probably a bug.\n",
                       textformatted.size(), TERMY, textformatted[ 0 ].c_str(),
                       textformatted[ textformatted.size() - 1 ].c_str() );
            }
        }
    }

    if( w_x == -1 ) {
        w_x = static_cast<int>( ( TERMX - w_width ) / 2 );
    }
    if( w_y == -1 ) {
        w_y = static_cast<int>( ( TERMY - w_height ) / 2 );
    }

    if( scrollbar_side == -1 ) {
        scrollbar_side = ( pad_left > 0 ? 1 : 0 );
    }
    if( static_cast<int>( entries.size() ) <= vmax ) {
        scrollbar_auto = false;
    }
    window = catacurses::newwin( w_height, w_width, point( w_x, w_y ) );
    if( !window ) {
        debugmsg( "Window not created; probably trying to use uilist in test mode." );
        abort();
    }

    fselected = selected;
    if( fselected < 0 ) {
        fselected = selected = 0;
    } else if( fselected >= static_cast<int>( entries.size() ) ) {
        fselected = selected = static_cast<int>( entries.size() ) - 1;
    }
    if( !entries.empty() && !entries[fselected].enabled ) {
        for( size_t i = 0; i < entries.size(); ++i ) {
            if( entries[i].enabled ) {
                fselected = selected = i;
                break;
            }
        }
    }
    started = true;
}

void uilist::apply_scrollbar()
{
    if( !scrollbar_auto ) {
        return;
    }

    int sbside = ( scrollbar_side == 0 ? 0 : w_width - 1 );
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
    .slot_color( scrollbar_nopage_color )
    .bar_color( scrollbar_page_color )
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
        mvwprintz( window, 0, 1, border_color, "< " );
        wprintz( window, title_color, title );
        wprintz( window, border_color, " >" );
    }

    const int pad_size = std::max( 0, w_width - 2 - pad_left - pad_right );
    std::string padspaces = std::string( pad_size, ' ' );
    const int text_lines = textformatted.size();
    int estart = 1;
    if( !textformatted.empty() ) {
        for( int i = 0; i < text_lines; i++ ) {
            trim_and_print( window, 1 + i, 2, getmaxx( window ) - 4, text_color, textformatted[i] );
        }

        mvwputch( window, text_lines + 1, 0, border_color, LINE_XXXO );
        for( int i = 1; i < w_width - 1; ++i ) {
            mvwputch( window, text_lines + 1, i, border_color, LINE_OXOX );
        }
        mvwputch( window, text_lines + 1, w_width - 1, border_color, LINE_XOXX );
        estart += text_lines + 1; // +1 for the horizontal line.
    }

    calcStartPos( vshift, fselected, vmax, fentries.size() );

    for( int fei = vshift, si = 0; si < vmax; fei++, si++ ) {
        if( fei < static_cast<int>( fentries.size() ) ) {
            int ei = fentries [ fei ];
            nc_color co = ( ei == selected ?
                            hilight_color :
                            ( entries[ ei ].enabled || entries[ei].force_color ?
                              entries[ ei ].text_color :
                              disabled_color )
                          );

            if( hilight_full ) {
                mvwprintz( window, estart + si, pad_left + 1, co, padspaces );
            }
            if( entries[ ei ].hotkey >= 33 && entries[ ei ].hotkey < 126 ) {
                const nc_color hotkey_co = ei == selected ? hilight_color : hotkey_color;
                mvwprintz( window, estart + si, pad_left + 2, entries[ ei ].enabled ? hotkey_co : co,
                           "%c", entries[ ei ].hotkey );
            }
            if( padspaces.size() > 3 ) {
                // padspaces's length indicates the maximal width of the entry, it is used above to
                // activate the highlighting, it is used to override previous text there, but in both
                // cases printing starts at pad_left+1, here it starts at pad_left+4, so 3 cells less
                // to be used.
                const auto entry = utf8_wrapper( ei == selected ? remove_color_tags( entries[ ei ].txt ) :
                                                 entries[ ei ].txt );
                trim_and_print( window, estart + si, pad_left + 4,
                                max_entry_len, co, "%s", entry.c_str() );

                if( max_column_len && !entries[ ei ].ctxt.empty() ) {
                    const auto centry = utf8_wrapper( ei == selected ? remove_color_tags( entries[ ei ].ctxt ) :
                                                      entries[ ei ].ctxt );
                    trim_and_print( window, estart + si, getmaxx( window ) - max_column_len - 2,
                                    max_column_len, co, "%s", centry.c_str() );
                }
            }
            mvwzstr menu_entry_extra_text = entries[ei].extratxt;
            if( !menu_entry_extra_text.txt.empty() ) {
                mvwprintz( window, estart + si, pad_left + 1 + menu_entry_extra_text.left,
                           menu_entry_extra_text.color, menu_entry_extra_text.txt );
            }
            if( menu_entry_extra_text.sym != 0 ) {
                mvwputch( window, estart + si, pad_left + 1 + menu_entry_extra_text.left,
                          menu_entry_extra_text.color, menu_entry_extra_text.sym );
            }
            if( callback != nullptr && ei == selected ) {
                callback->select( ei, this );
            }
        } else {
            mvwprintz( window, estart + si, pad_left + 1, c_light_gray, padspaces );
        }
    }

    if( desc_enabled ) {
        // draw border
        mvwputch( window, w_height - desc_lines - 2, 0, border_color, LINE_XXXO );
        for( int i = 1; i < w_width - 1; ++i ) {
            mvwputch( window, w_height - desc_lines - 2, i, border_color, LINE_OXOX );
        }
        mvwputch( window, w_height - desc_lines - 2, w_width - 1, border_color, LINE_XOXX );

        // clear previous desc the ugly way
        for( int y = desc_lines + 1; y > 1; --y ) {
            for( int x = 2; x < w_width - 2; ++x ) {
                mvwputch( window, w_height - y, x, text_color, " " );
            }
        }

        if( static_cast<size_t>( selected ) < entries.size() ) {
            fold_and_print( window, w_height - desc_lines - 1, 2, w_width - 4, text_color,
                            footer_text.empty() ? entries[selected].desc : footer_text );
        }
    }

    if( !filter.empty() ) {
        mvwprintz( window, w_height - 1, 2, border_color, "< %s >", filter );
        mvwprintz( window, w_height - 1, 4, text_color, filter );
    }
    apply_scrollbar();

    this->refresh( true );
}

/**
 * wrefresh + wrefresh callback's window
 */
void uilist::refresh( bool refresh_callback )
{
    wrefresh( window );
    if( refresh_callback && callback != nullptr ) {
        callback->refresh( this );
    }
}

/**
 * redraw borders, which is required in some cases ( look_around() )
 */
void uilist::redraw( bool redraw_callback )
{
    draw_border( window, border_color );
    if( !title.empty() ) {
        mvwprintz( window, 0, 1, border_color, "< " );
        wprintz( window, title_color, title );
        wprintz( window, border_color, " >" );
    }
    if( !filter.empty() ) {
        mvwprintz( window, w_height - 1, 2, border_color, "< %s >", filter );
        mvwprintz( window, w_height - 1, 4, text_color, filter );
    }
    ( void )redraw_callback; // TODO: something
    /*
    // pending tests on if this is needed
        if ( redraw_callback && callback != NULL ) {
            callback->redraw(this);
        }
    */
}

int uilist::scroll_amount_from_key( const int key )
{
    if( key == KEY_UP ) {
        return -1;
    } else if( key == KEY_PPAGE ) {
        return ( -vmax + 1 );
    } else if( key == KEY_DOWN ) {
        return 1;
    } else if( key == KEY_NPAGE ) {
        return vmax - 1;
    } else {
        return 0;
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
    if( ! looparound ) {
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
    }
    return true;
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

    show();

#if defined(__ANDROID__)
    for( const auto &entry : entries ) {
        if( entry.hotkey > 0 && entry.enabled ) {
            ctxt.register_manual_key( entry.hotkey, entry.txt );
        }
    }
#endif

    do {
        const auto action = ctxt.handle_input( timeout );
        const auto event = ctxt.get_raw_input();
        keypress = event.get_first_input();
        const auto iter = keymap.find( keypress );

        if( scrollby( scroll_amount_from_action( action ) ) ) {
            /* nothing */
        } else if( action == "HELP_KEYBINDINGS" ) {
            /* nothing, handled by input_context */
        } else if( filtering && action == "FILTER" ) {
            inputfilter();
        } else if( iter != keymap.end() ) {
            selected = iter->second;
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if( allow_disabled ) {
                ret = entries[selected].retval; // disabled
            }
        } else if( !fentries.empty() && action == "CONFIRM" ) {
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if( allow_disabled ) {
                ret = entries[selected].retval; // disabled
            }
        } else if( allow_cancel && action == "QUIT" ) {
            ret = UILIST_CANCEL;
        } else if( action == "TIMEOUT" ) {
            ret = UILIST_TIMEOUT;
        } else {
            bool unhandled = callback == nullptr || !callback->key( ctxt, event, selected, this );
            if( unhandled && allow_anykey ) {
                ret = UILIST_UNBOUND;
            }
        }

        show();
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

pointmenu_cb::pointmenu_cb( const std::vector< tripoint > &pts ) : points( pts )
{
    last = INT_MIN;
    last_view = g->u.view_offset;
}

void pointmenu_cb::select( int /*num*/, uilist * /*menu*/ )
{
    g->u.view_offset = last_view;
}

void pointmenu_cb::refresh( uilist *menu )
{
    if( last == menu->selected ) {
        return;
    }
    if( menu->selected < 0 || menu->selected >= static_cast<int>( points.size() ) ) {
        last = menu->selected;
        g->u.view_offset = tripoint_zero;
        g->draw_ter();
        wrefresh( g->w_terrain );
        g->draw_panels();
        menu->redraw( false ); // show() won't redraw borders
        menu->show();
        return;
    }

    last = menu->selected;
    const tripoint &center = points[menu->selected];
    g->u.view_offset = center - g->u.pos();
    g->u.view_offset.z = 0; // TODO: Remove this line when it's safe
    g->draw_trail_to_square( g->u.view_offset, true );
    menu->redraw( false );
    menu->show();
}

