#ifndef UI_H
#define UI_H

#include "enums.h"
#include "output.h"
#include "cursesdef.h"

#include <stdlib.h>
#include <unordered_map>
#include <memory>
//// ui_base ////////////////////////////////////////////////////////////////// {{{1
class ui_base
{
    protected:
        WINDOW_PTR window;
        point pos = {0, 0};
        point size = {0, 0};
    public:
        ui_base();
        virtual ~ui_base() = 0;

        virtual void handle_input() = 0;
        virtual void draw() = 0;
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_element /////////////////////////////////////////////////////////////// {{{1
class ui_element : public ui_base
{
    private:
        std::vector<ui_element*> children;
    protected:
        const ui_element *parent = nullptr;
        virtual void set_parent(const ui_element *ue) final;

        // add an element to this container
        virtual void add_element(ui_element &ue) final {add_element(&ue);}
        virtual void add_element(ui_element *ue) final;
        // remove an element from this container
        virtual void rem_element(ui_element &ue) final {add_element(&ue);}
        virtual void rem_element(ui_element *ue) final;
    public:
        ui_element() {};
        ui_element(ui_element &ue) {add_element(ue);}
        ui_element(ui_element *ue) {add_element(ue);}
        virtual ~ui_element();

        virtual void refresh() final;
        virtual void draw();
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_scrollable //////////////////////////////////////////////////////////// {{{1
class ui_scrollable
{
    protected:
        int cur_line = 0;
    public:
        int get_line() const
        {
            return cur_line;
        }
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_canvas //////////////////////////////////////////////////////////////// {{{1
class ui_canvas : public ui_element
{
    public:
        virtual void draw();
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_list ////////////////////////////////////////////////////////////////// {{{1
class ui_list : public ui_element, public ui_scrollable
{
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_button //////////////////////////////////////////////////////////////// {{{1
class ui_button : public ui_element
{
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_text ////////////////////////////////////////////////////////////////// {{{1
class ui_text : public ui_element, public ui_scrollable
{
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_scrollbar ///////////////////////////////////////////////////////////// {{{1
class ui_scrollbar : public ui_element
{
    private:
        unsigned int num_items = 0;
        point offset = {0, 0};
        nc_color bar_color = c_white;
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////////// }}}1
//// ui_container ///////////////////////////////////////////////////////////// {{{1
class ui_container : public ui_base
{
};
/////////////////////////////////////////////////////////////////////////////// }}}1



//// work in progress ///////////////////////////////////////////////////////// {{{1
/*
 * mvwzstr: line of text with horizontal offset and color
 */

struct mvwzstr {
    int left = 0;
    nc_color color = c_unset;
    std::string txt;
    long sym = 0;
};

/*
 * uimenu constants
 */
const int UIMENU_INVALID = -1024;
const int MENU_ALIGN_LEFT = -1;
const int MENU_ALIGN_CENTER = 0;
const int MENU_ALIGN_RIGHT = 1;
const int MENU_WIDTH_ENTRIES = -2;
const int MENU_AUTOASSIGN = -1;

/*
 * uimenu_entry: entry line for uimenu
 */
struct uimenu_entry {
    int retval;           // return this int
    bool enabled;         // darken, and forbid scrolling if hilight_disabled is false
    int hotkey;           // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
    std::string txt;      // what it says on the tin
    std::string desc;     // optional, possibly longer, description
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;

    //std::string filtertxt; // possibly useful
    uimenu_entry(std::string T) : retval(-1), enabled(true), hotkey(-1), txt(T)
    {
        text_color = c_red_red;
    };
    uimenu_entry(std::string T, std::string D) : retval(-1), enabled(true), hotkey(-1), txt(T), desc(D)
    {
        text_color = c_red_red;
    };
    uimenu_entry(std::string T, int K) : retval(-1), enabled(true), hotkey(K), txt(T)
    {
        text_color = c_red_red;
    };
    uimenu_entry(int R, bool E, int K, std::string T) : retval(R), enabled(E), hotkey(K), txt(T)
    {
        text_color = c_red_red;
    };
    uimenu_entry(int R, bool E, int K, std::string T, std::string D) : retval(R), enabled(E), hotkey(K), txt(T), desc(D)
    {
        text_color = c_red_red;
    };
    uimenu_entry(int R, bool E, int K, std::string T, nc_color H, nc_color C) : retval(R), enabled(E), hotkey(K), txt(T),
        hotkey_color(H), text_color(C) {};
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
 * for( size_t i = 0; i < z.size(); ++i ) {
 *   monmenu.addentry("%s", z[i].name.c_str() );
 * }
 * monmenu_cb * cb;
 * cb->setptr( &g->z );
 * monmenu.callback = cb
 * monmenu.query();
 *
 */
class uimenu;
class uimenu_callback
{
    public:
        void *myptr;
        void setptr(void *ptr)
        {
            myptr = ptr;
        }
        virtual void select(int /*entnum*/, uimenu *) {};
        virtual bool key(int /*key*/, int /*entnum*/, uimenu *)
        {
            return false;
        };
        virtual void refresh(uimenu *) {};
        virtual void redraw(uimenu *) {};
        virtual ~uimenu_callback() {};
};
/*
 * uimenu: scrolling vertical list menu
 */
class uimenu: public ui_base
{
    public:
        int ret;
        int selected;
        int keypress;
        std::string text;
        std::vector<std::string> textformatted;
        int textwidth;
        int textalign;
        int max_entry_len;
        int max_desc_len;
        std::string title;
        std::vector<uimenu_entry> entries;
        std::map<int, int> keymap;
        bool desc_enabled;
        int desc_lines;
        bool border;
        bool filtering;
        bool filtering_nocase;
        nc_color border_color;
        nc_color text_color;
        nc_color title_color;
        nc_color hilight_color;
        nc_color hotkey_color;
        nc_color disabled_color;
        int pad_left;
        int pad_right;
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

        uimenu_callback *callback;

        uimenu(); // bare init

        uimenu(bool cancancel, const char *message, ...);  // legacy menu()
        uimenu(bool cancelable, const char *mes, const std::vector<std::string> options); // legacy menu_vec
        uimenu(bool cancelable, const char *mes, const std::vector<std::string> &options, const std::string &hotkeys);
        uimenu(bool cancelable, int startx, int width, int starty, std::string title,
               std::vector<uimenu_entry> ents);
        uimenu(int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents);

        void init();
        void setup();
        void show();
        bool scrollby(int scrollby = 0, const int key = 0 );
        void query(bool loop = true);
        void filterlist();
        void apply_scrollbar();
        std::string inputfilter();
        void refresh(bool refresh_callback = true);
        void redraw(bool redraw_callback = true);
        void addentry(std::string str);
        void addentry(const char *format, ...);
        void addentry(int r, bool e, int k, std::string str);
        void addentry(int r, bool e, int k, const char *format, ...);
        void addentry_desc(std::string str, std::string desc);
        void addentry_desc(int r, bool e, int k, std::string str, std::string desc);
        void settext(std::string str);
        void settext(const char *format, ...);

        void reset();
        ~uimenu ();

        operator int() const;

        // pending refactor // ui_element_input * filter_input;

    private:
        bool started;
        int last_fsize;
        int last_vshift;
        std::string hotkeys;
};

// Callback for uimenu that pairs menu entries with points
// When an entry is selected, view will be centered on the paired point
class pointmenu_cb : public uimenu_callback {
    private:
        const std::vector< tripoint > &points;
        int last; // to suppress redrawing
        tripoint last_view; // to reposition the view after selecting
    public:
        pointmenu_cb( const std::vector< tripoint > &pts );
        ~pointmenu_cb() { };
        void select( int num, uimenu *menu ) override;
        void refresh( uimenu *menu ) override;
};

/////////////////////////////////////////////////////////////////////////////// }}}1
#endif
