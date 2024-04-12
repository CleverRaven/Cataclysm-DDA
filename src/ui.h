#pragma once
#ifndef CATA_SRC_UI_H
#define CATA_SRC_UI_H

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "color.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "input_context.h"
#include "memory_fast.h"
#include "pimpl.h"
#include "point.h"
#include "string_formatter.h"

class scrollbar;
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
    // std::nullopt: automatically assign an unassigned hotkey
    // input_event(): disable hotkey
    std::optional<input_event> hotkey;
    std::string txt;            // what it says on the tin
    std::string desc;           // optional, possibly longer, description
    std::string ctxt;           // second column text
    nc_color hotkey_color;
    nc_color text_color;
    mvwzstr extratxt;

    // In the following constructors, int key only support letters (a-z, A-Z) and
    // digits (0-9), MENU_AUTOASSIGN, and 0 or ' ' (disable hotkey). Other
    // values may not work under keycode mode.

    /**
    * @param txt string that will be displayed on the entry first column
    */
    explicit uilist_entry( const std::string &txt );
    /**
    * @param txt string that will be displayed on the entry first column
    * @param desc entry description if menu desc_enabled is true
    * @see uilist::desc_enabled
    */
    uilist_entry( const std::string &txt, const std::string &desc );
    /**
    * @param txt string that will be displayed on the entry first column
    * @param key hotkey character that when pressed will return this entry return value
    */
    uilist_entry( const std::string &txt, int key );
    /**
    * @param txt string that will be displayed on the entry first column
    * @param key hotkey character that when pressed will return this entry return value
    */
    uilist_entry( const std::string &txt, const std::optional<input_event> &key );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    */
    uilist_entry( int retval, bool enabled, int key, const std::string &txt );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    */
    uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                  const std::string &txt );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    * @param desc entry description if menu desc_enabled is true
    * @see uilist::desc_enabled
    */
    uilist_entry( int retval, bool enabled, int key, const std::string &txt, const std::string &desc );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    * @param desc entry description if menu desc_enabled is true
    * @see uilist::desc_enabled
    */
    uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                  const std::string &txt, const std::string &desc );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    * @param desc entry description if menu desc_enabled is true
    * @param column string that will be displayed on the entry second column
    * @see uilist::desc_enabled
    */
    uilist_entry( int retval, bool enabled, int key, const std::string &txt, const std::string &desc,
                  const std::string &column );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    * @param desc entry description if menu desc_enabled is true
    * @param column string that will be displayed on the entry second column
    * @see uilist::desc_enabled
    */
    uilist_entry( int retval, bool enabled, const std::optional<input_event> &key,
                  const std::string &txt, const std::string &desc,
                  const std::string &column );
    /**
    * @param retval return value of this option when selected during menu query
    * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
    * @param key hotkey character that when pressed will return this entry return value
    * @param txt string that will be displayed on the entry first column
    * @param keycolor color of the hotkey character
    * @param txtcolor entry text string color
    */
    uilist_entry( int retval, bool enabled, int key, const std::string &txt,
                  const nc_color &keycolor, const nc_color &txtcolor );
    template<typename Enum, typename... Args,
             typename = std::enable_if_t<std::is_enum_v<Enum>>>
                                         explicit uilist_entry( Enum e, Args && ... args ) :
                                             uilist_entry( static_cast<int>( e ), std::forward<Args>( args )... )
    {}

    std::optional<inclusive_rectangle<point>> drawn_rect;
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

        // Calculate sizes, populate arrays
        void calc_data();

        // Calls calc_data() and initialize the window
        void setup();
        // initialize the window or reposition it after screen size change.
        void reposition( ui_adaptor &ui );
        void show( ui_adaptor &ui );
        bool scrollby( int scrollby );
        void query( bool loop = true, int timeout = -1 );
        void filterlist();
        // In add_entry/add_entry_desc/add_entry_col, int k only support letters
        // (a-z, A-Z) and digits (0-9), MENU_AUTOASSIGN, and 0 or ' ' (disable
        // hotkey). Other values may not work under keycode mode.

        /**
        * @param txt string that will be displayed on the entry first column
        */
        void addentry( const std::string &txt );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        */
        void addentry( int retval, bool enabled, int key, const std::string &txt );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        */
        void addentry( int retval, bool enabled, const std::optional<input_event> &key,
                       const std::string &txt );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        * @param args list of parameters for string_format to format txt
        */
        template<typename K, typename ...Args>
        void addentry( const int retval, const bool enabled, K &&key, const char *const format,
                       Args &&... args ) {
            return addentry( retval, enabled, std::forward<K>( key ),
                             string_format( format, std::forward<Args>( args )... ) );
        }
        /**
        * @param txt string that will be displayed on the entry first column
        * @param desc entry description if menu desc_enabled is true
        * @see uilist::desc_enabled
        */
        void addentry_desc( const std::string &txt, const std::string &desc );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        * @param desc entry description if menu desc_enabled is true
        * @see uilist::desc_enabled
        */
        void addentry_desc( int retval, bool enabled, int key, const std::string &txt,
                            const std::string &desc );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        * @param desc entry description if menu desc_enabled is true
        * @see uilist::desc_enabled
        */
        void addentry_desc( int retval, bool enabled, const std::optional<input_event> &key,
                            const std::string &txt,
                            const std::string &desc );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        * @param column string that will be displayed on the entry second column
        * @param desc entry description if menu desc_enabled is true
        * @see uilist::desc_enabled
        */
        void addentry_col( int retval, bool enabled, int key, const std::string &txt,
                           const std::string &column,
                           const std::string &desc = "" );
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param key hotkey character that when pressed will return this entry return value
        * @param txt string that will be displayed on the entry first column
        * @param column string that will be displayed on the entry second column
        * @param desc entry description if menu desc_enabled is true
        * @see uilist::desc_enabled
        */
        void addentry_col( int retval, bool enabled, const std::optional<input_event> &key,
                           const std::string &txt, const std::string &column,
                           const std::string &desc = std::string() );
        void settext( const std::string &str );

        void add_category( const std::string &key, const std::string &name );
        void set_category( const std::string &key );
        void set_category_filter( const std::function<bool( const uilist_entry &, const std::string & )>
                                  &fun );

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
        std::unique_ptr<scrollbar> uilist_scrollbar;
        void apply_scrollbar();
        // This function assumes it's being called from `query` and should
        // not be made public.
        void inputfilter();
        enum class handle_mouse_result_t {
            unhandled, handled, confirmed
        };
        handle_mouse_result_t handle_mouse( const input_context &ctxt,
                                            const std::string &ret_act,
                                            bool loop );

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
        // return retval on "CONFIRM" action, default true
        bool allow_confirm = true;
        // return UILIST_ADDITIONAL if the input action is inside `additional_actions`
        // and unhandled by callback, default false.
        bool allow_additional = false;
        bool hilight_disabled = false;

    private:
        report_color_error _color_error = report_color_error::yes;
        input_context create_main_input_context() const;
        input_context create_filter_input_context() const;

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

        int fselected = 0; // -1 as sentinel value for no filtered entries to select from

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
        int category_lines = 0;

        bool started = false;

        bool recalc_start = false;

        std::vector<std::pair<std::string, std::string>> categories;
        std::function<bool( const uilist_entry &, const std::string & )> category_filter;
        int current_category = 0;

        int find_entry_by_coordinate( const point &p ) const;

    public:
        // Results
        // TODO change to getters
        std::string ret_act;
        input_event ret_evt;
        int ret = 0;
        int selected = 0;

        void set_selected( int index );
};

/**
 *  Hack ui list to behave as a multi column menu
 */
class uimenu
{
    public:
        explicit uimenu( int columns ) {
            cata_assert( columns >= 1 );
            col_count = columns;
        }
        /**
        * @param retval return value of this option when selected during menu query
        * @param enable is entry enabled. disabled entries will be grayed out and won't be selectable
        * @param col_content each string will be displayed on the entry's respective column
        */
        void addentry( int retval, bool enabled, const std::vector<std::string> &col_content );
        void set_selected( int index );
        void set_title( const std::string &title );
        int query();

    private:
        /**
         * Called in query to calculate sizes for multiple columns.
         */
        void finalize_addentries();
        struct col {
            col( int i_retval, bool i_enabled, const std::vector<std::string> &i_col_content ) :
                retval( i_retval ), enabled( i_enabled ), col_content( i_col_content )
            {};
            int retval;
            bool enabled;
            const std::vector<std::string> col_content;
        };
        std::vector<col> cols;
        uilist menu;
        int col_count;

        int suggest_width = 74;  // 80 (for min size) - 6 (for edges)
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

void kill_advanced_inv();
void temp_hide_advanced_inv();

/**
 * Helper for typical UI list navigation with wrap-around
 * Add delta to val. If on bounds, wrap to other bound, otherwise clamp to the range [0,size)
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp_wrap( V val, int delta, S size )
{
    if( size == 0 ) {
        return 0;
    }
    // wrap to last
    if( val <= 0 && delta < 0 ) {
        return size - 1;
    }
    // wrap to first
    // Templating of existing `unsigned int` triggers linter rules against `unsigned long`
    // NOLINTNEXTLINE(cata-no-long)
    if( val >= static_cast<V>( size ) - 1 && delta > 0 ) {
        return 0;
    }
    return std::clamp<V>( val + delta, 0, size - 1 );
}

/**
 * Helper for typical arrow key UI list navigation with wrap-around
 * Add 1/-1 to val, then wrap to the range [0,size)
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp_wrap( V val, bool inc, S size )
{
    return inc_clamp_wrap( val, static_cast<int>( inc ? 1 : -1 ), size );
}

/**
 * Helper for typical arrow key UI list navigation with wrap-around
 * Specialized for enum values
 * Add 1/-1 to val, then wrap to the range [0,size)
 */
template<typename T, typename I>
inline std::enable_if_t<std::is_enum_v<T>, T>
inc_clamp_wrap( T val, I inc, T size )
{
    return static_cast<T>( inc_clamp_wrap( static_cast<int>( val ), inc, static_cast<int>( size ) ) );
}

/**
 * Helper for typical UI list navigation without wrap-around
 * Add delta to val, then clamp to the range [min,max]
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, int delta, S min, S max )
{
    // Templating of existing `unsigned int` triggers linter rules against `unsigned long`
    if constexpr( std::is_unsigned_v<V> ) {
        // NOLINTNEXTLINE(cata-no-long)
        if( delta < 0 && val <= static_cast<V>( -delta ) ) {
            return min;
        }
    }
    return std::clamp<V>( val + delta, min, max );
}

/**
 * Helper for typical arrow key UI list navigation without wrap-around
 * Add 1/-1 to val, then clamp to the range [0,max]
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, bool inc, S max )
{
    // NOLINTNEXTLINE(cata-no-long)
    return inc_clamp( val, inc ? 1 : -1, static_cast<S>( 0 ), max );
}

/**
 * Helper for typical UI list navigation without wrap-around
 * Add delta to val, then clamp to the range [0,max]
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, int delta, S max )
{
    // NOLINTNEXTLINE(cata-no-long)
    return inc_clamp( val, delta, static_cast<S>( 0 ), max );
}

/**
 * Helper for typical arrow key UI list navigation without wrap-around
 * Add 1/-1 to val, then clamp to the range [min,max]
 */
template<typename V, typename S>
inline std::enable_if_t < !std::is_enum_v<V>, V >
inc_clamp( V val, bool inc, S min, S max )
{
    return inc_clamp( val, inc ? 1 : -1, min, max );
}

/**
 * Helper for typical arrow key UI list navigation without wrap-around
 * Specialized for enum values
 * Add 1/-1 to val, then clamp to the range [0,max]
 */
template<typename T, typename I>
inline std::enable_if_t<std::is_enum_v<T>, T>
inc_clamp( T val, I inc, T size )
{
    return static_cast<T>( inc_clamp( static_cast<int>( val ), inc, static_cast<int>( size ) ) );
}

/**
 * Helper for typical UI list navigation with wrap-around
 * Handles up, down, scroll_up and _down, page_up and _down, home, and end
 * Returns true if an action/input was handled
 */
template<typename V, typename S>
bool navigate_ui_list( const std::string &action, V &val, int page_delta, S size, bool wrap );

/**
 * Return indexes [start, end) that should be displayed from list long `num_entries`,
 * given that cursor is at position `cursor_pos` and we have `available_space` spaces.
 *
 * @param focused: if false, behave as if cursor_pos = 0,
 * useful when other menu is displayed and this should show the beggining
 *
 * Example:
 * num_entries = 6, available_space = 3, cursor_pos = 2, focused = true;
 * so choose 3 from indexes [0, 1, 2, 3, 4, 5]
 * return {1, 4}
 */
std::pair<int, int> subindex_around_cursor( int num_entries, int available_space, int cursor_pos,
        bool focused = true );

#endif // CATA_SRC_UI_H
