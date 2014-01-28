#include "ui.h"
#include "catacharset.h"
#include "output.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <iterator>
#include "keypress.h"
#include "cursesdef.h"
#include "uistate.h"
#include "options.h"

#ifdef debuguimenu
#define dprint(a,...)      mvprintw(a,0,__VA_ARGS__)
#else
#define dprint(a,...)      void()
#endif



////////////////////////////////////
int getfoldedwidth (std::vector<std::string> foldedstring) {
    int ret=0;
    for ( int i=0; i < foldedstring.size() ; i++ ) {
        int width = utf8_width(foldedstring[i].c_str());
        if ( width > ret ) {
            ret=width;
        }
    }
    return ret;
}

////////////////////////////////////
uimenu::uimenu() {
    init();
}

uimenu::uimenu(bool cancancel, const char *mes, ...) {  // here we emulate the old int ret=menu(bool, "header", "option1", "option2", ...);
    init();
    va_list ap;
    va_start(ap, mes);
    char *tmp;
    bool done = false;
    int i = 0;
    text = mes;
    shift_retval = 1;
    return_invalid = cancancel;
    while (!done) {
        tmp = va_arg(ap, char *);
        if (tmp != NULL) {
            std::string strtmp = tmp;
            entries.push_back(uimenu_entry(i, true, MENU_AUTOASSIGN, strtmp ));
        } else {
            done = true;
        }
        i++;
    }
    query();
}

uimenu::uimenu(bool cancelable, const char *mes, std::vector<std::string> options) { // exact usage as menu_vec
    init();
    if (options.empty()) {
        debugmsg("0-length menu (\"%s\")", mes);
        ret = -1;
    } else {
        text = mes;
        shift_retval = 1;
        return_invalid = cancelable;

        for (int i = 0; i < options.size(); i++) {
            entries.push_back(uimenu_entry(i, true, MENU_AUTOASSIGN, options[i] ));
        }
        query();
    }
}

uimenu::uimenu(int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents) { // another quick convenience coonstructor
    init();
    w_x = startx;
    w_y = starty;
    w_width = width;
    text = title;
    entries = ents;
    query();
    //dprint(2,"const: ret=%d w_x=%d w_y=%d w_width=%d w_height=%d, text=%s",ret,w_x,w_y,w_width,w_height, text.c_str() );
}

/*
 * Enables oneshot construction -> running -> exit
 */
uimenu::operator int() const {
    int r = ret + shift_retval;
    return r;
}

/*
 * Sane defaults on initialization
 */
void uimenu::init() {
    w_x = MENU_AUTOASSIGN;              // starting position
    w_y = MENU_AUTOASSIGN;              // -1 = auto center
    w_width = MENU_AUTOASSIGN;          // MENU_AUTOASSIGN = based on text width or max entry width, -2 = based on max entry, folds text
    w_height = MENU_AUTOASSIGN; // -1 = autocalculate based on number of entries + number of lines in text // fixme: scrolling list with offset
    ret = UIMENU_INVALID;  // return this unless a valid selection is made ( -1024 )
    text = "";             // header text, after (maybe) folding, populates:
    textformatted.clear(); // folded to textwidth
    textwidth = MENU_AUTOASSIGN; // if unset, folds according to w_width
    textalign = MENU_ALIGN_LEFT; // todo
    title = "";            // Makes use of the top border, no folding, sets min width if w_width is auto
    keypress = 0;          // last keypress from (int)getch()
    window = NULL;         // our window
    keymap.clear();        // keymap[int] == index, for entries[index]
    selected = 0;          // current highlight, for entries[index]
    entries.clear();       // uimenu_entry(int returnval, bool enabled, int keycode, std::string text, ...todo submenu stuff)
    started = false;       // set to true when width and key calculations are done, and window is generated.
    pad_left = 0;          // make a blank space to the left
    pad_right = 0;         // or right
    border = true;         // todo: always true
    border_color = c_magenta; // border color
    text_color = c_ltgray;  // text color
    title_color = c_green;  // title color
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_ltgreen; // hotkey text to the right of menu entry's text
    disabled_color = c_dkgray; // disabled menu entry
    return_invalid = false;  // return 0-(int)invalidKeyCode
    hilight_full = true;     // render hilight_color background over the entire line (minus padding)
    hilight_disabled = false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    shift_retval = 0;        // for legacy menu/vec_menu
    vshift = 0;              // scrolling menu offset
    vmax = 0;                // max entries area rows
    callback = NULL;         // * uimenu_callback
    filter = "";             // filter string. If "", show everything
    fentries.clear();        // fentries is the actual display after filtering, and maps displayed entry number to actual entry number
    fselected = 0;           // fentries[selected]
    filtering = true;        // enable list display filtering via '/' or '.'
    filtering_nocase = true; // ignore case when filtering
    max_entry_len = 0;       // does nothing but can be read

    scrollbar_auto = true;   // there is no force-on; true will only render scrollbar if entries > vertical height
    scrollbar_nopage_color = c_ltgray;    // color of '|' line for the entire area that isn't current page.
    scrollbar_page_color = c_cyan_cyan; // color of the '|' line for whatever's the current page.
    scrollbar_side = -1;     // -1 == choose left unless taken, then choose right

    last_fsize=-1;
    last_vshift=-1;
}

/*
 * case insensitive string::find( string::findstr ). findstr must be lowercased
 */
bool lcmatch(const std::string &str, const std::string &findstr)
{
    std::string ret = "";
    ret.reserve( str.size() );
    transform( str.begin(), str.end(), std::back_inserter(ret), tolower );
    return ( ret.find( findstr ) != -1 );
}

/*
 * repopulate filtered entries list (fentries) and set fselected accordingly
 */
void uimenu::filterlist()
{
    bool notfiltering = ( ! filtering || filter.size() < 1 );
    int num_entries = entries.size();
    bool nocase = (filtering_nocase == true); // todo: && is_all_lc( filter )
    std::string fstr = "";
    fstr.reserve(filter.size());
    if ( nocase ) {
        transform( filter.begin(), filter.end(), std::back_inserter(fstr), tolower );
    } else {
        fstr = filter;
    }
    fentries.clear();
    fselected = -1;
    int f = 0;
    for ( int i = 0; i < num_entries; i++ ) {
        if ( notfiltering
             || ( nocase == false && entries[ i ].txt.find(filter) != -1 )
             || lcmatch(entries[i].txt, fstr)
           ) {
            fentries.push_back( i );
            if ( i == selected ) {
                fselected = f;
            }
            f++;
        }
    }
    if ( fselected == -1 ) {
        fselected = 0;
        vshift = 0;
        if ( fentries.empty() ) {
            selected = -1;
        } else {
            selected = fentries [ 0 ];
        }
    }
}

/*
 * Call string_input_win / ui_element_input::input_filter and filter the entries list interactively
 */
std::string uimenu::inputfilter()
{
    std::string identifier = ""; // todo: uimenu.filter_identifier ?
    long key = 0;
    int spos = -1;
    mvwprintz(window, w_height - 1, 2, border_color, "< ");
    mvwprintz(window, w_height - 1, w_width - 3, border_color, " >");
/*
//debatable merit
    std::string origfilter = filter;
    int origselected = selected;
    int origfselected = fselected;
    int origvshift = vshift;
*/
    do {
        // filter=filter_input->query(filter, false);
        filter=string_input_win(window, filter, 256, 4, w_height - 1, w_width - 4, false, key, spos, identifier, 4, w_height - 1 );
        // key = filter_input->keypress;
        if ( key != KEY_ESCAPE ) {
            if ( scrollby(0, key) == false ) {
                filterlist();
            }
            show();
        }
    } while(key != '\n' && key != KEY_ESCAPE);

    if ( key == KEY_ESCAPE ) {
/*
//perhaps as an option
        filter = origfilter;
        selected = origselected;
        fselected = origfselected;
        vshift = origvshift;
*/
        filterlist();
}

    wattron(window, border_color);
    for( int i = 1; i < w_width - 1; i++ ) {
        mvwaddch(window, w_height-1, i, LINE_OXOX);
    }
    wattroff(window, border_color);

    return filter;
}

/*
 * Calculate sizes, populate arrays, initialize window
 */
void uimenu::setup() {
    bool w_auto = (w_width == -1 || w_width == -2 );
    bool w_autofold = ( w_width == -2);

    if ( w_auto ) {
        w_width = 4;
        if ( title.size() > 0 ) w_width = title.size() + 5;
    }

    bool h_auto = (w_height == -1);
    if ( h_auto ) {
        w_height = 4;
    }
    max_entry_len = 0;
    std::vector<int> autoassign;
    autoassign.clear();
    int pad = pad_left + pad_right + 2;
    for ( int i = 0; i < entries.size(); i++ ) {
        int txtwidth = utf8_width(entries[ i ].txt.c_str());
        if ( txtwidth > max_entry_len ) {
            max_entry_len = txtwidth;
        }
        if(entries[ i ].enabled) {
            if( entries[ i ].hotkey > 0 ) {
                keymap[ entries[ i ].hotkey ] = i;
            } else if ( entries[ i ].hotkey == -1 && i < 100 ) {
                autoassign.push_back(i);
            }
            if ( entries[ i ].retval == -1 ) {
                entries[ i ].retval = i;
            }
            if ( w_auto && w_width < txtwidth + pad + 4 ) {
                w_width = txtwidth + pad + 4;
            }
        } else {
            if ( w_auto && w_width < txtwidth + pad + 4 ) {
                w_width = txtwidth + pad + 4;    // todo: or +5 if header
            }
        }
        if ( entries[ i ].text_color == C_UNSET_MASK ) {
            entries[ i ].text_color = text_color;
        }
        fentries.push_back( i );
    }
    if ( !autoassign.empty() ) {
        for ( int a = 0; a < autoassign.size(); a++ ) {
            int setkey=-1;
            if ( a < 9 ) {
                setkey = a + 49; // 1-9;
            } else if ( a == 9 ) {
                setkey = a + 39; // 0;
            } else if ( a < 36 ) {
                setkey = a + 87; // a-z
            } else if ( a < 61 ) {
                setkey = a + 29; // A-Z
            }
            if ( setkey != -1 && keymap.find(setkey) == keymap.end() ) {
                int palloc = autoassign[ a ];
                entries[ palloc ].hotkey = setkey;
                keymap[ setkey ] = palloc;
            }
        }
    }

    if (w_auto && w_width > TERMX) {
        w_width = TERMX;
    }

    if(text.size() > 0 ) {
        int twidth = utf8_width(text.c_str());
        bool formattxt=true;
        int realtextwidth = 0;
        if ( textwidth == -1 ) {
            if ( w_autofold || !w_auto ) {
               realtextwidth = w_width - 4;
            } else {
               realtextwidth = twidth;
               if ( twidth + 4 > w_width ) {
                   if ( realtextwidth + 4 > TERMX ) {
                       realtextwidth = TERMX - 4;
                   }
                   textformatted = foldstring(text, realtextwidth);
                   formattxt=false;
                   realtextwidth = 10;
                   for ( int l=0; l < textformatted.size(); l++ ) {
                       if ( utf8_width(textformatted[l].c_str()) > realtextwidth ) {
                           realtextwidth = utf8_width(textformatted[l].c_str());
                       }
                   }
                   if ( realtextwidth + 4 > w_width ) {
                       w_width = realtextwidth + 4;
                   }
               }
            }
        } else if ( textwidth != -1 ) {
            realtextwidth = textwidth;
        }
        if ( formattxt == true ) {
            textformatted = foldstring(text, realtextwidth);
        }
    }

    if (h_auto) {
        w_height = 3 + textformatted.size() + entries.size();
    }

    if ( w_height > TERMY ) {
        w_height = TERMY;
    }

    vmax = entries.size();
    if ( vmax + 3 + textformatted.size() > w_height ) {
        vmax = w_height - 3 - textformatted.size();
        if ( vmax < 1 ) {
            popup("Can't display menu options, %d %d available screen rows are occupied by\n'%s\n(snip)\n%s'\nThis is probably a bug.\n",
               textformatted.size(),TERMY,textformatted[0].c_str(),textformatted[textformatted.size()-1].c_str()
            );
        }
    }

    if (w_x == -1) {
        w_x = int((TERMX - w_width) / 2);
    }
    if (w_y == -1) {
        w_y = int((TERMY - w_height) / 2);
    }

    if ( scrollbar_side == -1 ) {
        scrollbar_side = ( pad_left > 0 ? 1 : 0 );
    }
    if ( entries.size() <= vmax ) {
        scrollbar_auto = false;
    }
    window = newwin(w_height, w_width, w_y, w_x);

    werase(window);
    draw_border(window, border_color);
    if( title.size() > 0 ) {
        mvwprintz(window, 0, 1, border_color, "< ");
        wprintz(window, title_color, "%s", title.c_str() );
        wprintz(window, border_color, " >");
    }
    fselected = selected;
    started = true;
}

void uimenu::apply_scrollbar()
{
    if ( ! scrollbar_auto ) {
        return;
    }
    if ( last_vshift != vshift || last_fsize != fentries.size() ) {
        last_vshift=vshift;
        last_fsize=fentries.size();

        int sbside = ( scrollbar_side == 0 ? 0 : w_width );
        int estart = textformatted.size() + 1;

        if ( !fentries.empty() && vmax < fentries.size() ) {
            wattron(window, border_color);
            mvwaddch(window, estart, sbside, '^');
            wattroff(window, border_color);

            wattron(window, scrollbar_nopage_color);
            for( int i = estart + 1; i < estart + vmax - 1; i++ ) {
                mvwaddch(window, i, sbside, LINE_XOXO);
            }
            wattroff(window, scrollbar_nopage_color);

            wattron(window, border_color);
            mvwaddch(window, estart + vmax - 1, sbside, 'v');
            wattroff(window, border_color);

            int svmax = vmax - 2;
            int fentriessz = fentries.size() - vmax;
            int sbsize = (vmax * svmax) / fentries.size();
            if ( sbsize < 2 ) {
                sbsize=2;
            }
            int svmaxsz = svmax - sbsize;
            int sbstart = ( vshift * svmaxsz ) / fentriessz;
            int sbend = sbstart + sbsize;

            wattron(window, scrollbar_page_color);
            for ( int i = sbstart; i < sbend; i++ ) {
                mvwaddch(window, i + estart + 1, sbside, LINE_XOXO);
            }
            wattroff(window, scrollbar_page_color);

        } else {
            wattron(window, border_color);
            for( int i = estart; i < estart + vmax; i++ ) {
                mvwaddch(window, i, sbside, LINE_XOXO);
            }
            wattroff(window, border_color);
        }
    }
}
/*
 * Generate and refresh output
 */
void uimenu::show() {
    if (!started) {
        setup();
    }
    std::string padspaces = std::string(w_width - 2 - pad_left - pad_right, ' ');
    const int text_lines = textformatted.size();
    for ( int i = 0; i < text_lines; i++ ) {
        mvwprintz(window, 1+i, 2, text_color, "%s", textformatted[i].c_str());
    }
    
    mvwputch(window, text_lines + 1, 0, border_color, LINE_XXXO);
    for ( int i = 1; i < w_width - 1; ++i) {
        mvwputch(window, text_lines + 1, i, border_color, LINE_OXOX);
    }
    mvwputch(window, text_lines + 1, w_width - 1, border_color, LINE_XOXX);

    int estart = text_lines + 2;

    if( OPTIONS["MENU_SCROLL"] ) {
        if (fentries.size() > vmax) {
            vshift = fselected - (vmax - 1) / 2;

            if (vshift < 0) {
                vshift = 0;
            } else if (vshift + vmax > fentries.size()) {
                vshift = fentries.size() - vmax;
            }
         }
    } else {
        if( fselected < vshift ) {
            vshift = fselected;
        } else if( fselected >= vshift + vmax ) {
            vshift = 1 + fselected - vmax;
        }
    }

    for ( int fei = vshift, si=0; si < vmax; fei++,si++ ) {
        if ( fei < fentries.size() ) {
            int ei=fentries [ fei ];
            nc_color co = ( ei == selected ?
               hilight_color :
               ( entries[ ei ].enabled ?
                  entries[ ei ].text_color :
               disabled_color )
            );

            if ( hilight_full ) {
               mvwprintz(window, estart + si, pad_left + 1, co , "%s", padspaces.c_str());
            }
            if(entries[ ei ].enabled && entries[ ei ].hotkey > 33 && entries[ ei ].hotkey < 126 ) {
               mvwprintz(window, estart + si, pad_left + 2, ( ei == selected ? hilight_color : hotkey_color ) , "%c", entries[ ei ].hotkey);
            }
            mvwprintz(window, estart + si, pad_left + 4, co, "%s", entries[ ei ].txt.c_str() );
            if ( entries[ ei ].extratxt.txt.size() > 0 ) {
                mvwprintz(window, estart + si, pad_left + 1 + entries[ ei ].extratxt.left, entries[ ei ].extratxt.color, "%s", entries[ ei ].extratxt.txt.c_str() );
            }
            if ( callback != NULL && ei == selected ) {
                callback->select(ei,this);
            }
        } else {
            mvwprintz(window, estart + si, pad_left + 1, c_ltgray , "%s", padspaces.c_str());
        }
    }

    if ( filter.size() > 0 ) {
        mvwprintz(window,w_height-1,2,border_color,"< %s >",filter.c_str() );
        mvwprintz(window,w_height-1,4,text_color,"%s",filter.c_str());
    }
    apply_scrollbar();

    this->refresh(true);
}

/*
 * wrefresh + wrefresh callback's window
 */
void uimenu::refresh( bool refresh_callback ) {
    wrefresh(window);
    if ( refresh_callback && callback != NULL ) {
        callback->refresh(this);
    }
}

/*
 * redraw borders, which is required in some cases ( look_around() )
 */
void uimenu::redraw( bool redraw_callback ) {
    draw_border(window, border_color);
    if( title.size() > 0 ) {
        mvwprintz(window, 0, 1, border_color, "< ");
        wprintz(window, title_color, "%s", title.c_str() );
        wprintz(window, border_color, " >");
    }
    if ( filter.size() > 0 ) {
        mvwprintz(window,w_height-1,2,border_color,"< %s >",filter.c_str() );
        mvwprintz(window,w_height-1,4,text_color,"%s",filter.c_str());
    }
    (void)redraw_callback; // TODO
/*
// pending tests on if this is needed
    if ( redraw_callback && callback != NULL ) {
        callback->redraw(this);
    }
*/
}

/*
 * check for valid scrolling keypress and handle. return false if invalid keypress
 */
bool uimenu::scrollby(int scrollby, const int key) {
    if ( key != 0 ) {
        if ( key == KEY_UP ) {
            scrollby=-1;
        } else if ( key == KEY_PPAGE ) {
            scrollby=(-vmax + 1);
        } else if ( key == KEY_DOWN ) {
            scrollby=1;
        } else if ( key == KEY_NPAGE ) {
            scrollby=vmax - 1;
            } else {
            return false;
            }
    } else if ( scrollby == 0 ) {
        return false;
    }

    bool looparound = ( scrollby == -1 || scrollby == 1 );
    bool backwards = ( scrollby < 0 );

    fselected += scrollby;
    if ( ! looparound ) {
        if ( backwards && fselected < 0 ) {
            fselected = 0;
        } else if ( fselected >= fentries.size() ) {
            fselected = fentries.size()-1;
        }
    }

    int iter = ( hilight_disabled ? 1 : fentries.size() );

    if ( backwards ) {
            while ( iter > 0 ) {
                iter--;
            if( fselected < 0 ) {
                fselected = fentries.size() - 1;
                }
            if ( entries[ fentries [ fselected ] ].enabled == false ) {
                fselected--;
                } else {
                    iter = 0;
                }
            }
            } else {
            while ( iter > 0 ) {
                iter--;
            if( fselected >= fentries.size() ) {
                fselected = 0;
                }
            if ( entries[ fentries [ fselected ] ].enabled == false ) {
                fselected++;
                } else {
                    iter = 0;
                }
            }
    }
    selected = fentries [ fselected ];
    return true;
}

/*
 * Handle input and update display
 */
void uimenu::query(bool loop) {
    keypress = 0;
    if ( entries.empty() ) {
        return;
    }
    int startret = UIMENU_INVALID;
    ret = UIMENU_INVALID;
    bool keycallback = (callback != NULL );

    show();
    do {
        bool skiprefresh = false;
        bool skipkey = false;
        keypress = getch();

        if ( scrollby(0, keypress) == true ) {
            /* nothing */
        } else if ( filtering && ( keypress == '/' || keypress == '.' ) ) {
            inputfilter();
        } else if ( !fentries.empty() && ( keypress == '\n' || keypress == KEY_ENTER || keymap.find(keypress) != keymap.end() ) ) {
            if ( keymap.find(keypress) != keymap.end() ) {
                selected = keymap[ keypress ];//fixme ?
            }
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if ( return_invalid ) {
                ret = 0 - entries[ selected ].retval; // disabled
            }
        } else if ( keypress == KEY_ESCAPE && return_invalid) { //break loop with ESCAPE key
            break;
        } else {
            if ( keycallback ) {
                skipkey = callback->key( keypress, selected, this );
            }
            if ( ! skipkey && return_invalid ) {
                ret = -1;
            }
        }

        if ( skiprefresh==false ) {
            show();
        }
    } while ( loop && (ret == startret ) );
}

/*
 * cleanup
 */
uimenu::~uimenu() {
    werase(window);
    wrefresh(window);
    delwin(window);
    window = NULL;
    init();
}

void uimenu::addentry(std::string str) {
   entries.push_back(str);
}

void uimenu::addentry(int r, bool e, int k, std::string str) {
   entries.push_back(uimenu_entry(r, e, k, str));
}

void uimenu::addentry(const char *format, ...) {
   char buf[4096];
   va_list ap;
   va_start(ap, format);
   int safe=vsnprintf(buf, sizeof(buf), format, ap);
   if ( safe >= 4096 || safe < 0 ) {
     popup("BUG: Increase buf[4096] in ui.cpp");
     return;
   }
   entries.push_back(std::string(buf));
}

void uimenu::addentry(int r, bool e, int k, const char *format, ...) {
   char buf[4096];
   int rv=r; bool en=e; int ke=k;
   va_list ap;
   va_start(ap, format);
   int safe=vsnprintf(buf, sizeof(buf), format, ap);
   if ( safe >= 4096 || safe < 0 ) {
     popup("BUG: Increase buf[4096] in ui.cpp");
     return;
   }
   entries.push_back(uimenu_entry(rv, en, ke, std::string(buf)));
}

void::uimenu::settext(std::string str) {
   text = str;
}

void uimenu::settext(const char *format, ...) {
   char buf[16384];
   va_list ap;
   va_start(ap, format);
   int safe=vsnprintf(buf, sizeof(buf), format, ap);
   if ( safe >= 16384 || safe < 0 ) {
     popup("BUG: Increase buf[16384] in ui.cpp");
     return;
   }
   text = std::string(buf);
}
