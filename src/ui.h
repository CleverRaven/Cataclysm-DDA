#pragma once
#ifndef UI_H
#define UI_H

#include "enums.h"
#include <stdlib.h>
#include "color.h"
#include "cursesdef.h"
#include "string_formatter.h"

#include <vector>
#include <string>
#include <map>
#include <utility>

////////////////////////////////////////////////////////////////////////////////////
/**
 * uimenu constants
 */
const int UIMENU_INVALID = -1024;
const int MENU_ALIGN_LEFT = -1;
const int MENU_ALIGN_CENTER = 0;
const int MENU_ALIGN_RIGHT = 1;
const int MENU_WIDTH_ENTRIES = -2;
const int MENU_AUTOASSIGN = -1;

struct input_event;
class input_context;

/**
 * mvwzstr: line of text with horizontal offset and color
 */

struct mvwzstr {
    int left = 0;
    nc_color color = c_unset;
    std::string txt;
    long sym = 0;
};

/**
 * uimenu_entry: entry line for uimenu
 */
struct uimenu_entry {
    int retval;                 // return this int
    bool enabled;               // darken, and forbid scrolling if hilight_disabled is false
    bool force_color = false;   // Never darken this option
    int hotkey;                 // keycode from (int)getch(). -1: automagically pick first free character: 1-9 a-z A-Z
    std::string txt;            // what it says on the tin
    std::string desc;           // optional, possibly longer, description
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;

    //std::string filtertxt; // possibly useful
    uimenu_entry( std::string T ) : retval( -1 ), enabled( true ), hotkey( -1 ), txt( T ) {
        text_color = c_red_red;
    };
    uimenu_entry( std::string T, std::string D ) : retval( -1 ), enabled( true ), hotkey( -1 ),
        txt( T ), desc( D ) {
        text_color = c_red_red;
    };
    uimenu_entry( std::string T, int K ) : retval( -1 ), enabled( true ), hotkey( K ), txt( T ) {
        text_color = c_red_red;
    };
    uimenu_entry( int R, bool E, int K, std::string T ) : retval( R ), enabled( E ), hotkey( K ),
        txt( T ) {
        text_color = c_red_red;
    };
    uimenu_entry( int R, bool E, int K, std::string T, std::string D ) : retval( R ), enabled( E ),
        hotkey( K ), txt( T ), desc( D ) {
        text_color = c_red_red;
    };
    uimenu_entry( int R, bool E, int K, std::string T, nc_color H, nc_color C ) : retval( R ),
        enabled( E ), hotkey( K ), txt( T ),
        hotkey_color( H ), text_color( C ) {};
};

/**
 * Virtual base class for windowed ui stuff (like uimenu)
 */
class ui_container
{
    public:
        virtual ~ui_container() = default;

    public:
        int w_x;
        int w_y;
        int w_width;
        int w_height;
        catacurses::window window;
        virtual void refresh( bool refresh_children = true ) = 0;
};

/**
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
 *   monmenu.addentry( z[i].name );
 * }
 * monmenu_cb * cb;
 * cb->setptr( &g->z );
 * monmenu.callback = cb
 * monmenu.query();
 *
 */
class uimenu;
/**
* uimenu::query() handles most input events first,
* and then passes the event to the callback if it can't handle it.
*
* The callback returninig a boolean false signifies that the callback can't "handle the
* event completely". This is unchanged before or after the PR.
* @{
*/
class uimenu_callback
{
    public:
        virtual void select( int /*entnum*/, uimenu * ) {};
        virtual bool key( const input_context &, const input_event &/*key*/, int /*entnum*/,
                          uimenu * ) {
            return false;
        };
        virtual void refresh( uimenu * ) {};
        virtual void redraw( uimenu * ) {};
        virtual ~uimenu_callback() = default;
};
/*@}*/
/**
 * uimenu: scrolling vertical list menu
 */
class ui_element;
class ui_element_input;
class uimenu: public ui_container
{
    public:
        int ret;
        int selected;
        int keypress;
        std::string text;
        std::vector<std::string> textformatted;
        std::string input_category;
        std::vector< std::pair<std::string, std::string> > additional_actions;
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

        uimenu( const std::string &hotkeys = "" ); // bare init

        uimenu( bool cancancel, const char *message, ... ); // legacy menu()
        uimenu( bool cancelable, const char *mes,
                const std::vector<std::string> options ); // legacy menu_vec
        uimenu( bool cancelable, const char *mes, const std::vector<std::string> &options,
                const std::string &hotkeys );
        uimenu( bool cancelable, int startx, int width, int starty, std::string title,
                std::vector<uimenu_entry> ents );
        uimenu( int startx, int width, int starty, std::string title, std::vector<uimenu_entry> ents );

        void init();
        void setup();
        void show();
        bool scrollby( int scrollby );
        int scroll_amount_from_key( const int key );
        int scroll_amount_from_action( const std::string &action );
        void query( bool loop = true );
        void filterlist();
        void apply_scrollbar();
        std::string inputfilter();
        void refresh( bool refresh_callback = true ) override;
        void redraw( bool redraw_callback = true );
        void addentry( std::string str );
        void addentry( int r, bool e, int k, std::string str );
        // K is templated so it matches a `char` literal and a `long` value.
        // Using a fixed type (either `char` or `long`) will lead to ambiguity with the
        // other overload when called with the wrong type.
        template<typename K, typename ...Args>
        void addentry( const int r, const bool e, K k, const char *const format, Args &&... args ) {
            return addentry( r, e, k, string_format( format, std::forward<Args>( args )... ) );
        }
        void addentry_desc( std::string str, std::string desc );
        void addentry_desc( int r, bool e, int k, std::string str, std::string desc );
        void settext( std::string str );

        void reset();

        operator int() const;

        // pending refactor // ui_element_input * filter_input;

    private:
        bool started;
        int last_fsize;
        int last_vshift;
        std::string hotkeys;
};

/**
 * Callback for uimenu that pairs menu entries with points
 * When an entry is selected, view will be centered on the paired point
 */
class pointmenu_cb : public uimenu_callback
{
    private:
        const std::vector< tripoint > &points;
        int last; // to suppress redrawing
        tripoint last_view; // to reposition the view after selecting
    public:
        pointmenu_cb( const std::vector< tripoint > &pts );
        ~pointmenu_cb() override = default;
        void select( int num, uimenu *menu ) override;
        void refresh( uimenu *menu ) override;
};

#endif
