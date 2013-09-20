#ifndef _UI_H_
#define _UI_H_

#include "output.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"
////////////////////////////////////////////////////////////////////////////////////
/*
 * uimenu constants
 */
const int UIMENU_INVALID=-1024;
const nc_color C_UNSET_MASK=c_red_red;
const int MENU_ALIGN_LEFT=-1;
const int MENU_ALIGN_CENTER=0;
const int MENU_ALIGN_RIGHT=1;
const int MENU_WIDTH_ENTRIES=-2;
const int MENU_AUTOASSIGN=-1;

/*
 * mvwzstr: line of text with horizontal offset and color
 */

struct mvwzstr {
    int left;
    nc_color color;
    std::string txt;
};

/*
 * uimenu_entry: entry line for uimenu
 */
struct uimenu_entry {
    int retval;           // return this int
    bool enabled;         // darken, and forbid scrolling if hilight_disabled is false
    int hotkey;           // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
    std::string txt;      // what it says on the tin
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;
    //std::string filtertxt; // possibly useful
    uimenu_entry(std::string T) : retval(-1), enabled(true), hotkey(-1), txt(T) {text_color=C_UNSET_MASK;};
    uimenu_entry(std::string T, int K) : retval(-1), enabled(true), hotkey(K), txt(T) {text_color=C_UNSET_MASK;};
    uimenu_entry(int R, bool E, int K, std::string T) : retval(R), enabled(E), hotkey(K), txt(T) {text_color=C_UNSET_MASK;};
};

/*
 * Virtual base class for windowed ui stuff (like uimenu)
 */
class ui_container {
  public:
    int w_x;
    int w_y;
    int w_width;
    int w_height;
    WINDOW *window;
    virtual void refresh( bool refresh_children = true ) = 0;
};

/*
 * Generic multi-function callback for highlighted items, key presses, and window control. Example:
 *
 * class monmenu_cb: public uimenu_callback {
 *   public:
 *   bool key(int ch, int num, uimenu * menu) {
 *     if ( ch == 'k' && num > 0 ) {
 *       std::vector<monster> * game_z=static_cast<std::vector<monster>*>(myptr);
 *       game_z[num]->dead = true;
 *     }
 *   }
 *   void select(int num, uimenu * menu) {
 *       mvwprintz(menu->window, 0, 0, c_red, "( %s )",game_z[num]->name().c_str() );
 *   }
 * }
 * uimenu monmenu;
 * for(int i=0; i < z.size(); i++ ) {
 *   monmenu.addentry("%s", z[i].name.c_str() );
 * }
 * monmenu_cb * cb;
 * cb->setptr( &g->z );
 * monmenu.callback = cb
 * monmenu.query();
 *
 */
class uimenu;
class uimenu_callback {
    public:
        void * myptr;
        void setptr(void * ptr) {
             myptr = ptr;
        }
        virtual void select(int entnum, uimenu * menu) {};
        virtual bool key(int key, int entnum, uimenu * menu) { return false; };
        virtual void refresh(uimenu * menu) {};
        virtual void redraw(uimenu * menu) {};
        virtual ~uimenu_callback() {};
};
/*
 * uimenu: scrolling vertical list menu
 */
class ui_element;
class ui_element_input;
class uimenu: public ui_container {
  public:
    int ret;
    int selected;
    int keypress;
    std::string text;
    std::vector<std::string> textformatted;
    int textwidth;
    int textalign;
    int max_entry_len;
    std::string title;
    std::vector<uimenu_entry> entries;
    std::map<int, int> keymap;    
    bool border;
    bool filtering;
    bool filtering_nocase;
    nc_color border_color;
    nc_color text_color;
    nc_color title_color;
    nc_color hilight_color;
    nc_color hotkey_color;
    nc_color disabled_color;
    int pad_left; int pad_right;
    bool return_invalid;
    bool hilight_disabled;
    bool hilight_full;
    int shift_retval;
    int vshift;
    int vmax;
    std::string filter;
    std::vector<int> fentries;
    int fselected;
    bool centered_scroll;

    bool scrollbar_auto;
    nc_color scrollbar_nopage_color;
    nc_color scrollbar_page_color;
    int scrollbar_side;

    uimenu_callback * callback;

    uimenu(); // bare init

    uimenu(bool cancancel, const char * message, ...); // legacy menu()
    uimenu(bool cancelable, const char *mes, std::vector<std::string> options); // legacy menu_vec
    uimenu (int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents);

    void init();
    void setup();
    void show();
    bool scrollby(int scrollby=0, const int key=0 );
    void query(bool loop=true);
    void filterlist();
    void apply_scrollbar();
    std::string inputfilter();
    void refresh(bool refresh_callback=true);
    void redraw(bool redraw_callback=true);
    void addentry(std::string str);
    void addentry(const char *format, ...);
    void addentry(int r, bool e, int k, std::string str);
    void addentry(int r, bool e, int k, const char *format, ...);
    void settext(std::string str);
    void settext(const char *format, ...);
    ~uimenu ();
    operator int() const;

    // pending refactor // ui_element_input * filter_input;

  private:
    bool started;
    int last_fsize;
    int last_vshift;
};

#endif
