#pragma once
#ifndef CATA_SRC_UI_H
#define CATA_SRC_UI_H

#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "input.h"
#include "memory_fast.h"
#include "optional.h"
#include "pimpl.h"
#include "point.h"
#include "string_formatter.h"

class translation;

////////////////////////////////////////////////////////////////////////////////////
/**
 * uilist constants
 */
const int UILIST_ERROR = -1024;
const int UILIST_WAIT_INPUT = -1025;
const int UILIST_UNBOUND = -1026;
const int UILIST_CANCEL = -1027;
const int UILIST_TIMEOUT = -1028;
const int UILIST_ADDITIONAL = -1029;
const int MENU_AUTOASSIGN = -1;

class string_input_popup;
class ui_adaptor;

catacurses::window new_centered_win( int nlines, int ncols );

/**
 * mvwzstr: line of text with horizontal offset and color
 */

struct mvwzstr {
    int left = 0;
    nc_color color = c_unset;
    std::string txt;
    int sym = 0;
};

/**
 * uilist_entry: entry line for uilist
 */
struct uilist_entry {
    int retval;                 // return this int
    bool enabled;               // darken, and forbid scrolling if hilight_disabled is false
    bool force_color = false;   // Never darken this option
    // cata::nullopt: automatically assign an unassigned hotkey
    // input_event(): disable hotkey
    cata::optional<input_event> hotkey;
    std::string txt;            // what it says on the tin
    std::string desc;           // optional, possibly longer, description
    std::string ctxt;           // second column text
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;

    // In the following constructors, int K only support letters (a-z, A-Z) and
    // digits (0-9), MENU_AUTOASSIGN, and 0 or ' ' (disable hotkey). Other
    // values may not work under keycode mode.
    explicit uilist_entry( const std::string &T );
    uilist_entry( const std::string &T, const std::string &D );
    uilist_entry( const std::string &T, int K );
    uilist_entry( const std::string &T, const cata::optional<input_event> &K );
    uilist_entry( int R, bool E, int K, const std::string &T );
    uilist_entry( int R, bool E, const cata::optional<input_event> &K,
                  const std::string &T );
    uilist_entry( int R, bool E, int K, const std::string &T, const std::string &D );
    uilist_entry( int R, bool E, int K, const std::string &T, const std::string &D,
                  const std::string &C );
    uilist_entry( int R, bool E, const cata::optional<input_event> &K,
                  const std::string &T, const std::string &D,
                  const std::string &C );
    uilist_entry( int R, bool E, int K, const std::string &T,
                  const nc_color &H, const nc_color &C );
    template<typename Enum, typename... Args,
             typename = std::enable_if_t<std::is_enum<Enum>::value>>
    explicit uilist_entry( Enum e, Args && ... args ) :
        uilist_entry( static_cast<int>( e ), std::forward<Args>( args )... )
    {}

    inclusive_rectangle<point> drawn_rect;
};

/**
 * Generic multi-function callback for highlighted items, key presses, and window control. Example:
 *
 * class monmenu_cb: public uilist_callback {
 *   public:
 *   bool key(int ch, int num, uilist * menu) {
 *     if ( ch == 'k' && num > 0 ) {
 *       std::vector<monster> * game_z=static_cast<std::vector<monster>*>(myptr);
 *       game_z[num]->dead = true;
 *     }
 *   }
 *   void refresh( uilist *menu ) {
 *       if( menu->selected >= 0 && static_cast<size_t>( menu->selected ) < game_z.size() ) {
 *           mvwprintz( menu->window, 0, 0, c_red, "( %s )",game_z[menu->selected]->name() );
 *           wnoutrefresh( menu->window );
 *       }
 *   }
 * }
 * uilist monmenu;
 * for( size_t i = 0; i < z.size(); ++i ) {
 *   monmenu.addentry( z[i].name );
 * }
 * monmenu_cb * cb;
 * cb->setptr( &g->z );
 * monmenu.callback = cb
 * monmenu.query();
 *
 */
class uilist;

/**
* uilist::query() handles most input events first,
* and then passes the event to the callback if it can't handle it.
*
* The callback returninig a boolean false signifies that the callback can't "handle the
* event completely". This is unchanged before or after the PR.
* @{
*/
class uilist_callback
{
    public:

        /**
        * After a new item is selected, call this once
        */
        virtual void select( uilist * ) {}
        virtual bool key( const input_context &, const input_event &/*key*/, int /*entnum*/,
                          uilist * ) {
            return false;
        }
        virtual void refresh( uilist * ) {}
        virtual ~uilist_callback() = default;
};
/*@}*/
/**
 * uilist: scrolling vertical list menu
 */

class uilist // NOLINT(cata-xy)
{
    public:
        class size_scalar
        {
            public:
                struct auto_assign {
                };

                size_scalar &operator=( auto_assign );
                size_scalar &operator=( int val );
                size_scalar &operator=( const std::function<int()> &fun );

                friend class uilist;

            private:
                std::function<int()> fun;
        };

        class pos_scalar
        {
            public:
                struct auto_assign {
                };

                pos_scalar &operator=( auto_assign );
                pos_scalar &operator=( int val );
                // the parameter to the function is the corresponding size vector element
                // (width for x, height for y)
                pos_scalar &operator=( const std::function<int( int )> &fun );

                friend class uilist;

            private:
                std::function<int( int )> fun;
        };

        uilist();
        // query() will be called at the end of these convenience constructors
        uilist( const std::string &msg, const std::vector<uilist_entry> &opts );
        uilist( const std::string &msg, const std::vector<std::string> &opts );
        uilist( const std::string &msg, std::initializer_list<const char *const> opts );

        ~uilist();

        // whether to report invalid color tag with debug message.
        void color_error( bool report );

        void init();
        void setup();
        // initialize the window or reposition it after screen size change.
        void reposition( ui_adaptor &ui );
        void show();
        bool scrollby( int scrollby );
        void query( bool loop = true, int timeout = -1 );
        void filterlist();
        // In add_entry/add_entry_desc/add_entry_col, int k only support letters
        // (a-z, A-Z) and digits (0-9), MENU_AUTOASSIGN, and 0 or ' ' (disable
        // hotkey). Other values may not work under keycode mode.
        void addentry( const std::string &str );
        void addentry( int r, bool e, int k, const std::string &str );
        void addentry( int r, bool e, const cata::optional<input_event> &k,
                       const std::string &str );
        template<typename K, typename ...Args>
        void addentry( const int r, const bool e, K &&k, const char *const format, Args &&... args ) {
            return addentry( r, e, std::forward<K>( k ),
                             string_format( format, std::forward<Args>( args )... ) );
        }
        void addentry_desc( const std::string &str, const std::string &desc );
        void addentry_desc( int r, bool e, int k, const std::string &str, const std::string &desc );
        void addentry_col( int r, bool e, int k, const std::string &str, const std::string &column,
                           const std::string &desc = "" );
        void addentry_col( int r, bool e, const cata::optional<input_event> &k,
                           const std::string &str, const std::string &column,
                           const std::string &desc = std::string() );
        void settext( const std::string &str );

        void reset();

        // Can be called before `uilist::query` to keep the uilist on UI stack after
        // `uilist::query` returns. The returned `ui_adaptor` is cleared when the
        // `uilist` is deconstructed.
        //
        // Example:
        //     shared_ptr_fast<ui_adaptor> ui = menu.create_or_get_ui_adaptor();
        //     menu.query()
        //     // before `ui` or `menu` is deconstructed, the menu will always be
        //     // displayed on screen.
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator int() const;

    private:
        int scroll_amount_from_action( const std::string &action );
        void apply_scrollbar();
        // This function assumes it's being called from `query` and should
        // not be made public.
        void inputfilter();

    public:
        // Parameters
        // TODO change to setters
        std::string title;
        std::string text;
        // basically the same as desc, except it doesn't change based on selection
        std::string footer_text;
        std::vector<uilist_entry> entries;

        std::string input_category;
        std::vector<std::pair<std::string, translation>> additional_actions;

        nc_color border_color;
        nc_color text_color;
        nc_color title_color;
        nc_color hilight_color;
        nc_color hotkey_color;
        nc_color disabled_color;

        uilist_callback *callback;

        pos_scalar w_x_setup;
        pos_scalar w_y_setup;
        size_scalar w_width_setup;
        size_scalar w_height_setup;

        int textwidth = 0;

        size_scalar pad_left_setup;
        size_scalar pad_right_setup;

        // Maximum number of lines to be allocated for displaying descriptions.
        // This only serves as a hint, not a hard limit, so the number of lines
        // may still exceed this value when for example the description text is
        // long enough.
        int desc_lines_hint = 0;
        bool desc_enabled = false;

        bool filtering = false;
        bool filtering_nocase = false;

        // return on selecting disabled entry, default false
        bool allow_disabled = false;
        // return UILIST_UNBOUND on keys unbound & unhandled by callback, default false
        bool allow_anykey = false;
        // return UILIST_CANCEL on "QUIT" action, default true
        bool allow_cancel = true;
        // return UILIST_ADDITIONAL if the input action is inside `additional_actions`
        // and unhandled by callback, default false.
        bool allow_additional = false;
        bool hilight_disabled = false;

    private:
        report_color_error _color_error = report_color_error::yes;

    public:
        // Iternal states
        // TODO make private
        std::vector<std::string> textformatted;

        catacurses::window window;
        int w_x = 0;
        int w_y = 0;
        int w_width = 0;
        int w_height = 0;

        int pad_left = 0;
        int pad_right = 0;

        int vshift = 0;

        int fselected = 0;

    private:
        std::vector<int> fentries;
        std::map<input_event, int, std::function<bool( const input_event &, const input_event & )>>
        keymap { input_event::compare_type_mod_code };

        weak_ptr_fast<ui_adaptor> ui;

        std::unique_ptr<string_input_popup> filter_popup;
        std::string filter;

        int max_entry_len = 0;
        int max_column_len = 0;

        int vmax = 0;

        int desc_lines = 0;

        bool started = false;

        uilist_entry *find_entry_by_coordinate( const point &p );

    public:
        // Results
        // TODO change to getters
        std::string ret_act;
        input_event ret_evt;
        int ret = 0;
        int selected = 0;
};

/**
 * Callback for uilist that pairs menu entries with points
 * When an entry is selected, view will be centered on the paired point
 */
class pointmenu_cb : public uilist_callback
{
    private:
        struct impl_t;

        pimpl<impl_t> impl;
    public:
        explicit pointmenu_cb( const std::vector< tripoint > &pts );
        ~pointmenu_cb() override;
        void select( uilist *menu ) override;
};

#endif // CATA_SRC_UI_H
