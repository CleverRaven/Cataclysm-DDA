#ifndef UI_H
#define UI_H

#include "enums.h"
#include "output.h"
#include "cursesdef.h"

#include <stdlib.h>
#include <unordered_map>
#include <memory>

//// ui_base ////////////////////////////////////////////////////////////// {{{1
/*  ui_base provides a base pure virtual interface for classes that deal with any
 *  screen drawing and/or screen input. Since this is a pure virtual, one must define
 *  their own `handle_input()' and `draw()' functions.
 */
class input_context;

enum ui_input_code {
    ui_error = -2,
    ui_exit = -1,
    ui_continue = 0,
};

class ui_base
{
    protected:
        // basic window properties, needed for all classes in UI!
        WINDOW_PTR window = nullptr;
        point pos  = {0, 0};
        point size = {0, 0};
        // override "ctxt_name" in derived classes
        const std::string ctxt_name = "default";
        input_context *ctxt;
        // for error display
        std::string error_msg = "";
        int error_code = 0;
        // code to be returned on good exit
        int return_code = 0;

        // set the error code and message
        void set_error(int code, const std::string &msg);
        // register input actions for the above context
        virtual void register_input_actions();
        /*  handle_input() is to be overridden per derived class for one to deal with
         *  keystrokes in its own custom way. The input will be handled via the standard
         *  `input_manager'. You can get the results of the input from XXX and co.
         */
        virtual ui_input_code handle_input(const std::string &action);
        virtual std::string get_input() const;
    public:
        // let the container sort out the position and size
        ui_base();
        // allow one to initialize it straight off
        ui_base(const point &s, const point p) : size(s), pos(p) {};
        virtual ~ui_base();
        // execute ui's `run()' function
        virtual int operator ()();
        virtual int run() = 0;

        // returns the size (size) of the window
        const point &get_size() const;
        // returns the position (pos) of the window
        const point &get_position() const;
        /*  draw() is to be overriden to allow any custom drawing for the derived class.
         *  Since this is a pure virtual interface, you should have access to the variables
         *  and methods of the derived class, so use this as you please when you draw! :-)
         */
        virtual void draw() = 0;
        /*  finish() contains the last remnants of what needs to be drawn or changed for
         *  the derived class. Takes place after a draw().
         */
        virtual void finish();
        // set the size of the window
        virtual void set_size(const point &s) final;
        // set the position of the window
        virtual void set_position(const point &p) final;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_functor /////////////////////////////////////////////////////////// {{{1
typedef std::function<void(void*, void*)> ui_function;
// contains name and respective function for function functionality
struct ui_callback {
    std::string name;
    ui_function func;
    
    // probably will only be used for rem_function()
    ui_callback(const std::string &N) : name(N) {};
    // more verbose struct initialization: 
    //      i.e. ui_callback("func1", func1);
    ui_callback(const std::string &N, ui_function F) : name(N), func(F) {};
};

class ui_functor
{
    protected:
        // for named functions, name is pertinent to the derived class
        std::unordered_map<std::string, ui_function> functions;

        // adds a functor to function, accessible via `name'
        void add_function(const ui_callback &uc);
        // remove function of `name'
        void rem_function(const ui_callback &uc);
        // replace the function accessed by `name'
        void mod_function(const ui_callback &uc);
    public:
        // invoke function "name" with refs to in and out variable vectors
        void invoke(const std::string &name,
                std::vector<void*> &in,
                std::vector<void*> &out);
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_element /////////////////////////////////////////////////////////// {{{1
/*  ui_elements are used to provide specific interface types to the user.
 *  This class is to be derived from for any new type of element of the UI.
 */
class ui_element : public ui_base
{
    private:
        // whether to draw the border or not
        int do_border = true;
        // border color to draw
        nc_color border_color = BORDER_COLOR;
        // draw the element's border
        virtual void draw_border() final;
        // enable/disable the border for the window 
        // (offset will still calculate if there is one, unless do_border == -1)
        void set_border(int should_draw = true);
    public:
        ui_element();
        virtual ~ui_element();

        void set_border_color(nc_color c)
        {
            border_color = c;
        }
        void draw() override;
        void finish() override;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_scrollbar ///////////////////////////////////////////////////////// {{{1
class ui_scrollbar : public ui_element
{
    private:
        unsigned int num_items = 0;
        nc_color bar_color = c_white;
    public:
        // inheriting class must define this for scrollbar to work
        virtual const int get_line() const = 0;
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_tabbed ////////////////////////////////////////////////////////////// {{{1

class ui_tabbed : public ui_base
{
    private:
        int active_tab = 0;
        std::vector<std::string> labels;
        // returns the index of the label in `labels'
        size_t find_label(const std::string &name);
    protected:
        // set the names of the labels, clears the prior labels
        void set_labels(const char *first_label, ...);
        template <typename T> void set_labels(T v);
        // add a label to the end of the list
        void add_label(const std::string &name);
        // remove a label by its name
        void rem_label(const std::string &name);
        // modify a label from old_name to new_name
        void mod_label(const std::string &old_name, const std::string &new_name);
        // draw the tabs for the element
        void draw_tabs();
    public:
        ui_tabbed();
        ui_tabbed(const char *start, ...);

        const std::vector<std::string> &get_labels() const;
        virtual void finish() override;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_canvas //////////////////////////////////////////////////////////// {{{1
class ui_canvas : public ui_element
{
    public:
        virtual void draw();
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_list ////////////////////////////////////////////////////////////// {{{1
struct ui_list_entry {
    std::string name;
    bool enabled;
    nc_color text_color;
    int return_code;

    ui_list_entry(const std::string &N) : name(N) {};
    ui_list_entry(const std::string &N, bool E) : name(N), enabled(E) {};
    ui_list_entry(const std::string &N, bool E, nc_color C) :
        name(N), enabled(E), text_color(C) {};
    ui_list_entry(const std::string &N, bool E, nc_color C, int R) : 
        name(N), enabled(E), text_color(C), return_code(R) {};
};

class ui_list : public ui_element, public ui_scrollbar
{
    private:
        std::vector<ui_list_entry> entries;
        size_t find_entry(const std::string &name) const;
    public:
        const int get_line() const override;
        void draw() override;
        virtual void finish() override final;

        // adds an entry with the given attributes
        void add_entry(const std::string &name, bool enabled = true, 
                nc_color text_color = COLOR_WHITE, int return_code = -1);
        void add_entry(const ui_list_entry &entry);
        // removes the named entry 
        void rem_entry(const std::string &name);
        void rem_entry(const ui_list_entry &entry);
        // modifies an existing entry
        void mod_entry(const std::string &name, bool enabled = true,
                nc_color text_color = COLOR_WHITE);
        void mod_entry(const ui_list_entry &entry);
        // returns the entry by name
        ui_list_entry get_entry(const std::string &name) const;
        // clear all entries
        void clear();
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_button //////////////////////////////////////////////////////////// {{{1
class ui_button : public ui_element
{
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_text ////////////////////////////////////////////////////////////// {{{1
class ui_text : public ui_element, public ui_scrollbar
{
    public:
        void draw() override;
};
/////////////////////////////////////////////////////////////////////////// }}}1
//// ui_container ///////////////////////////////////////////////////////// {{{1
class ui_container : public ui_base
{
    private:
        ui_base *child;
    public:
        ui_container(ui_base *like) : child(like) /* innocence! :-) */ {};
        void refresh();
};
/////////////////////////////////////////////////////////////////////////// }}}1
////             ////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1
////             ////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1
////             ////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1
////             ////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1
////             ////////////////////////////////////////////////////////// {{{1
/////////////////////////////////////////////////////////////////////////// }}}1

/////////////////////////////////////////////////////////////////////////// }}}1


//// work in progress ///////////////////////////////////////////////////// {{{1
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

/////////////////////////////////////////////////////////////////////////// }}}1
#endif
