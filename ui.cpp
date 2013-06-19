#include "ui.h"
#include "output.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"
#ifdef debuguimenu
#define dprint(a,...)      mvprintw(a,0,__VA_ARGS__)
#else
#define dprint(a,...)      void()
#endif



////////////////////////////////////
std::vector<std::string> foldstring ( std::string str, int width ) {
    std::vector<std::string> lines;
    if ( width < 1 ) {
        lines.push_back( str );
        return lines;
    }

    int linepos = width;
    int linestart = 0;

    while( linepos < str.length() ) {
        int crpos = str.find('\n', linestart);
        if (crpos != -1 && crpos <= linepos) {
            lines.push_back( str.substr( linestart, crpos-linestart ) );
            linepos = crpos + width + 1;
            linestart = crpos + 1;
        } else {
            int spacepos = str.rfind(' ', linepos);
            if ( spacepos == -1 ) spacepos = str.find(' ', linepos);
            if ( spacepos < linestart ) {
                spacepos = linestart + width;
                if( spacepos < str.length() ) {
                    lines.push_back( str.substr( linestart, width ) );
                    linepos = spacepos + width;
                    linestart = spacepos;
                }
            } else {
                lines.push_back( str.substr( linestart, spacepos-linestart ) );
                linepos = spacepos + width + 1;
                linestart = spacepos + 1;
            }
        }
    }
    lines.push_back( str.substr( linestart ) );
    return lines;
};

int getfoldedwidth (std::vector<std::string> foldedstring) {
    int ret=0;
    for ( int i=0; i < foldedstring.size() ; i++ ) {
        if ( foldedstring[i].size() > ret ) ret=foldedstring[i].size();
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
    if (options.size() == 0) {
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

uimenu::operator int() const {
    int r = ret + shift_retval;
    return r;
}

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
    hilight_color = h_white; // highlight for up/down selection bar
    hotkey_color = c_ltgreen; // hotkey text to the right of menu entry's text
    disabled_color = c_dkgray; // disabled menu entry
    return_invalid = false;  // return 0-(int)invalidKeyCode
    hilight_full = true;     // render hilight_color background over the entire line (minus padding)
    hilight_disabled = false; // if false, hitting 'down' onto a disabled entry will advance downward to the first enabled entry
    shift_retval = 0;        // for legacy menu/vec_menu
}

void uimenu::show() {
    if (!started) {
        bool w_auto = (w_width == -1 || w_width == -2 );
        bool w_autofold = ( w_width == -2);
        int realtextwidth = 0;

        if ( w_auto ) {
            w_width = 4;
        }

        bool h_auto = (w_height == -1);
        if ( h_auto ) {
            w_height = 4;
        }

        std::vector<int> autoassign;
        autoassign.clear();
        int pad = pad_left + pad_right + 2;
        for ( int i = 0; i < entries.size(); i++ ) {
            if(entries[ i ].enabled) {
                if( entries[ i ].hotkey > 0 ) {
                    keymap[ entries[ i ].hotkey ] = i;
                } else if ( entries[ i ].hotkey == -1 ) {
                    autoassign.push_back(i);
                }
                if ( entries[ i ].retval == -1 ) {
                    entries[ i ].retval = i;
                }
                if ( w_auto && w_width < entries[ i ].txt.size() + pad + 4 ) {
                    w_width = entries[ i ].txt.size() + pad + 4;
                }
            } else {
                if ( w_auto && w_width < entries[ i ].txt.size() + pad + 4 ) {
                    w_width = entries[ i ].txt.size() + pad + 4;    // todo: or +5 if header
                }
            }
        }
        if ( autoassign.size() > 0 ) {
            for ( int a = 0; a < autoassign.size(); a++ ) {
                int palloc = autoassign[ a ];
                if ( palloc < 9 ) {
                    entries[ palloc ].hotkey = palloc + 49; // 1-9;
                } else if ( palloc == 9 ) {
                    entries[ palloc ].hotkey = palloc + 39; // 0;
                } else if ( palloc < 36 ) {
                    entries[ palloc ].hotkey = palloc + 87; // a-z
                } else if ( palloc < 61 ) {
                    entries[ palloc ].hotkey = palloc + 29; // A-Z
                }
                if ( palloc < 61 ) {
                    keymap[ entries[ palloc ].hotkey ] = palloc;
                }
            }
        }

        if (w_auto && w_width > TERMX) {
            w_width = TERMX;
        }

        if(text.size() > 0 ) {
            if ( textwidth == -1 ) {
                if ( w_autofold || !w_auto ) {
                   realtextwidth = w_width - 4;
                } else {
                   realtextwidth = text.size();
                   if ( text.size() + 4 > w_width ) {
                       if ( realtextwidth + 4 > TERMX ) {
                           realtextwidth = TERMX - 4;
                       }
                       w_width = realtextwidth + 4;
                   }
                }
            } else if ( textwidth != -1 ) {
                realtextwidth = textwidth;
            }
            textformatted = foldstring(text, realtextwidth);     
        }

        if (h_auto) {
            w_height = 2 + textformatted.size() + entries.size();
        }

        if (h_auto && w_height > TERMY) {
            w_height = TERMY;
        }

        if (w_x == -1) {
            w_x = int((TERMX - w_width) / 2);
        }
        if (w_y == -1) {
            w_y = int((TERMY - w_height) / 2);
        }

        window = newwin(w_height, w_width, w_y, w_x);
        werase(window);
        wattron(window, border_color);
        wborder(window, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        wattroff(window, border_color);
        started = true;
    }
    std::string padspaces = std::string(w_width - 2 - pad_left - pad_right, ' ');
    for ( int i = 0; i < textformatted.size(); i++ ) {
        mvwprintz(window, 1+i, 2, text_color, "%s", textformatted[i].c_str());
    }
    int estart = textformatted.size() + 1; // todo fold text + get offset
    for ( int i = 0; i < entries.size(); i++ ) {
        nc_color co = ( i == selected ? hilight_color : ( entries[ i ].enabled ? text_color : disabled_color ) );
        if ( hilight_full ) {
            mvwprintz(window, estart + i, pad_left + 1, co , "%s", padspaces.c_str());
        }
        if(entries[ i ].enabled && entries[ i ].hotkey > 33 && entries[ i ].hotkey < 126 ) {
            mvwprintz(window, estart + i, pad_left + 2, ( i == selected ? hilight_color : hotkey_color ) , "%c", entries[ i ].hotkey);
        }
        mvwprintz(window, estart + i, pad_left + 4, co, "%s", entries[ i ].txt.c_str() );
    }
    wrefresh(window);
}

void uimenu::query(bool loop) {
    keypress = 0;
    if ( entries.size() < 1 ) {
        return;
    }
    //int last_selected = selected;
    int startret = ret;
    do {
        show();
        keypress = getch();
        if ( keypress == KEY_UP ) {
            selected--;
            int iter = ( hilight_disabled ? 1 : entries.size() );
            while ( iter > 0 ) {
                iter--;
                if( selected < 0 ) {
                    selected = entries.size() - 1;
                }
                if ( entries[ selected ].enabled == false ) {
                    selected--;
                } else {
                    iter = 0;
                }
            }
            // todo: scroll_callback(this, selected, last_selected );
        } else if ( keypress == KEY_DOWN ) {
            selected++;
            int iter = entries.size();
            if ( hilight_disabled == true ) {
                iter = 1;
            }
            while ( iter > 0 ) {
                iter--;
                if( selected >= entries.size() ) {
                    selected = 0;
                }
                if ( entries[ selected ].enabled == false ) {
                    selected++;
                } else {
                    iter = 0;
                }
            }
            // todo: scroll_callback(this, selected, last_selected );
        } else if ( keypress == '\n' || keypress == KEY_ENTER || keymap.find(keypress) != keymap.end() ) {
            if ( keymap.find(keypress) != keymap.end() ) { //!( keypress == '\n' || keypress == KEY_RIGHT || keypress == KEY_ENTER) ) {
                selected = keymap[ keypress ];
            }
            if( entries[ selected ].enabled ) {
                ret = entries[ selected ].retval; // valid
            } else if ( return_invalid ) {
                ret = 0 - entries[ selected ].retval; // disabled
            }
        } else {
            if ( return_invalid ) {
                ret = -1;
            }
        }
    } while ( loop & (ret == startret ) );
}

uimenu::~uimenu() {
    //dprint(3,"death: ret=%d, w_x=%d, w_y=%d, w_width=%d, w_height=%d", ret, w_x, w_y, w_width, w_height );
    werase(window);
    wrefresh(window);
    delwin(window);
    window = NULL;
    init();
}
