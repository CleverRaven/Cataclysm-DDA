#ifndef CATA_UI_H
#define CATA_UI_H

#include "output.h"
#include "enums.h"
#include "cata_utility.h"
#include "input.h"

#include <utility>
#include <list>
#include <string>
#include <map>
#include <algorithm>

typedef int nc_color;

/**
 * @defgroup Cata_ui Cataclysm's ui framework.
 *
 * The ui framework is implemented with the composite pattern.
 * This allows us to nest ui elements within another. We only need to call 'draw'
 * on the top level object, and the composite pattern takes care of the rest.
 *
 * @{
 */

/**
* @brief Class that represents a rectangle.
*
*  Holds dimensions and position.
*/
struct ui_rect {
    /**
    * The x and y size of the rectangle
    */
    size_t size_x, size_y;

    /**
    * The x and y position as offsets to the anchor (within the parent)
    */
    int x, y;

    ui_rect( size_t size_x, size_t size_y, int x, int y );
};

/**
* @brief Observer pattern event system.
*
* Subscribe listeners to an event, fire it with ```()```
*/
template<typename... Ts>
class ui_event {
        std::vector<std::function<void(Ts...)>> listeners;
    public:
        void subscribe( std::function<void(Ts...)> listener )
        {
            listeners.push_back( listener );
        }

        void operator()(Ts... args)
        {
            for( auto listener : listeners ) {
                listener( args... );
            }
        }

        void operator+=( std::function<void(Ts...)> listener )
        {
            listeners.push_back( listener );
        }
};

/**
* A scoped enum used by ui elements to anchor themselves inside a window.
*/
enum class ui_anchor {
    top_left,
    top_center,
    top_right,
    center_left,
    center_center,
    center_right,
    bottom_left,
    bottom_center,
    bottom_right
};

class ui_window;

/**
* @brief Most basic ui element.
*
* This abstract class is used to implement the frameworks's composite pattern.
*/
class ui_element {
    friend class ui_window; // only ui_window can call draw
    private:
        const ui_window *parent = nullptr; /**< The parent is where ```get_win()```gets it's window from */
        void set_parent( const ui_window *parent );

        ui_anchor anchor = ui_anchor::top_left; /**< This elements current anchoring inside the parent */

        unsigned int anchored_x, anchored_y; /**< Internal kept values to store anchor adjusted position */

        bool show = true; /**< Indicates to ui_window if this element should be drawn, and whether it should receive input */
        ui_rect rect; /**< Holds the elements size and position */
        virtual void calc_anchored_values();
    protected:
        virtual void draw( std::vector<WINDOW *> &render_batch );
        virtual void draw( WINDOW *win ) = 0;
        virtual WINDOW *get_win() const; /**< Getter for a window to draw with */
        virtual void send_action( const std::string &action ) {}
    public:
        ui_element( const ui_rect &rect, ui_anchor anchor = ui_anchor::top_left );
        ui_element( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );
        virtual ~ui_element() = default;

        const ui_rect &get_rect() const;
        virtual void set_rect( const ui_rect &new_rect ); /**< Virtual setter for ```rect```, so derived classes can do extra calculations */

        ui_anchor get_anchor() const;
        virtual void set_anchor( ui_anchor new_anchor ); /**< Virtual setter for ```anchor```, so derived classes can do extra calculations */

        bool is_visible() const;
        virtual void set_visible( bool visible ); /**< Virtual setter for ```show```, so derived classes can do extra calculations */

        unsigned int get_ax() const; /**< Getter for anchor adjusted x position */
        unsigned int get_ay() const; /**< Getter for anchor adjusted y position */

        /**
        * @name Relatives
        *
        * A set of methods used to place this ui_element relative to the passed element.
        * The ```x``` and ```y``` parameters are used as offsets.
        */
        //@{
        void above( const ui_element &other, int x = 0, int y = 0 );
        void below( const ui_element &other, int x = 0, int y = 0 );
        void after( const ui_element &other, int x = 0, int y = 0 );
        void before( const ui_element &other, int x = 0, int y = 0 );
        //@}
};

/**
* @brief The basis for a ui composition.
*
* This is the class in the framework that holds nested elements.
* It is also the only class in the framework with a public 'draw' function.
*/
class ui_window : public ui_element {
    private:
        unsigned int global_x, global_y;

        WINDOW *win = nullptr;
        void draw_children( std::vector<WINDOW *> &render_batch );
        std::vector<ui_element *> children;

        void calc_anchored_values() override;

        input_context ctxt;
    protected:
        void draw( std::vector<WINDOW *> &render_batch ) override;
        void draw( WINDOW *win ) override { on_draw( win ); }
    public:
        ui_window( const ui_rect &rect, ui_anchor anchor = ui_anchor::top_left );
        ui_window( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );
        ~ui_window() override;

        ui_window( const ui_window & ) = delete;
        ui_window &operator=( const ui_window & ) = delete;
        ui_window( ui_window && ) = delete;
        ui_window &operator=( ui_window && ) = delete;

        void draw();

        WINDOW *get_win() const override;

        input_context &get_input_context();
        virtual std::string handle_input();

        /**
        * Event meant to let a client do low-level drawing on the window, without the use of ui_elements.
        */
        ui_event<WINDOW *> on_draw;

        /**
        * @brief Creates a copy of the passed ```ui_element```and stores it in it's list of children.
        *
        * The copying is a safety precaution, so you can add the same element to multiple windows
        * without the same pointer becoming managed by multiple windows.
        * @param child The ```ui_element``` to copy from.
        * @return Returns a pointer to the copy it made (so you can control it later)
        */
        template<typename T, typename... Args>
        T *create_child( Args&&... args )
        {
            auto child = new T( std::forward<Args>(args)... );
            children.push_back( child );
            child->set_parent( this );
            return child;
        }
};

/**
* @brief A simple text label
*/
class ui_label : public ui_element {
    protected:
        std::string text;

        virtual void draw( WINDOW *win ) override;
    public:
        ui_label( std::string text, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void set_text( std::string );

        nc_color text_color = c_white;
};

/**
* @brief A window with a border
*/
class bordered_window : public ui_window {
    protected:
        void draw( WINDOW *win ) override;
    public:
        bordered_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );

        using ui_window::draw;

        nc_color border_color = BORDER_COLOR;
};

/**
* @brief A window with tabs at the top.
*
### Creating tabs
* Create a tab with the ```create_tab``` method.
* It returns a ```ui_window *``` (Or something else if you pass it a template argument).
* Use this window to let tabbed_window control visibility of elements you add to the window.
*/
class tabbed_window : public bordered_window {
    private:
        std::vector<std::pair<std::string, ui_element *>> tabs;
        unsigned int tab_index = 0;
    protected:
        void draw( WINDOW *win ) override;
    public:
        tabbed_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );

        using ui_window::draw;

        static constexpr int header_size = 3;

        template<typename T = ui_window, typename... Ts>
        T *create_tab( const std::string &tab, Ts&&... args )
        {
            auto tab_win = create_child<T>( get_rect().size_x - 2, get_rect().size_y - header_size - 1, 1, -1,
                                            ui_anchor::bottom_left, std::forward<Ts>(args)... );
            tabs.push_back({tab, tab_win});
            tab_win->set_visible( tabs.size() == 1 );
            return tab_win;
        }

        ui_event<> on_scroll;

        void next_tab();
        void previous_tab();
        std::string current_tab() const;

        std::string handle_input() override;
};

constexpr auto default_draw_func = []( nc_color color, std::string txt )
    {
        return std::pair<std::string, nc_color>{txt, color}; // perform no transformation
    };

/**
* @brief Basically, a list of text.
*
* One of the lines of text is highlighted (selected).
* The list also has a scroll bar.
*/
template<typename D = std::string, typename Draw_Func = decltype(default_draw_func), D default_current = "">
class ui_vertical_list : public ui_element {
    private:
        std::vector<D> items;
        unsigned int scroll = 0;
        unsigned int window_scroll = 0;

        Draw_Func get_text_and_color;
    protected:
        void draw( WINDOW *win ) override
        {
            int start_line = 0;
            calcStartPos( start_line, scroll, get_rect().size_y, items.size() );
            unsigned int end_line = start_line + get_rect().size_y;

            size_t available_space = get_rect().size_x - 2; // 2 for scroll bar and spacer

            for( unsigned int line = start_line; line < end_line && line < items.size(); line++ ) {
                auto t_c = get_text_and_color( text_color, items[line] );
                std::string txt = t_c.first;
                string_truncate( txt, available_space );

                mvwprintz( win, get_ay() + line - start_line, get_ax() + 2, (scroll == line ? hilite(t_c.second) : t_c.second), "%s", txt.c_str() );
            }

            draw_scrollbar( win, scroll, get_rect().size_y, items.size(), get_ay(), get_ax(), bar_color, false );
        }
    public:
        ui_vertical_list( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left, Draw_Func d = default_draw_func ) :
                          ui_element( size_x, size_y, x, y, anchor ), get_text_and_color( d )
        {
        }

        nc_color text_color = c_white;
        nc_color bar_color = c_ltblue;

        void set_items( std::vector<D> items )
        {
            this->items = items;
        }

        void scroll_up()
        {
            if( items.size() > 1 ) {
                scroll = ( scroll == 0 ? items.size() - 1 : scroll - 1 );

                if( scroll == items.size() - 1 ) {
                    window_scroll = scroll - get_rect().size_y + 1;
                } else if( scroll < window_scroll ) {
                    window_scroll--;
                }
                on_scroll();
            }
        }

        void scroll_down()
        {
            if( items.size() > 1 ) {
                scroll = ( scroll == items.size() - 1 ? 0 : scroll + 1 );

                if( scroll > get_rect().size_y + window_scroll - 1 ) {
                    window_scroll++;
                } else if( scroll == 0 ) {
                    window_scroll = 0;
                }
                on_scroll();
            }
        }

        D current() const
        {
            if( items.empty() ) {
                return default_current;
            }
            return items[scroll];
        }

        ui_event<> on_select;
        ui_event<> on_scroll;

        void send_action( const std::string &action ) override
        {
            if( action == "UP" ) {
                scroll_up();
            } else if( action == "DOWN" ) {
                scroll_down();
            } else if( action == "CONFIRM" ) {
                if( !items.empty() ) {
                    on_select();
                }
            }
        }

        bool empty() const
        {
            return items.empty();
        }
};

/**
* @brief A horizontal list of text
*/
class ui_horizontal_list : public ui_element {
    private:
        std::vector<std::string> text;
        unsigned int scroll = 0;
    protected:
        void draw( WINDOW *win ) override;
    public:
        ui_horizontal_list( int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        nc_color text_color = c_white;

        void set_text( std::vector<std::string> text );

        void scroll_left();
        void scroll_right();
        const std::string &current() const;

        ui_event<> on_select;
        ui_event<> on_scroll;

        void send_action( const std::string &action ) override;
};

/**
* @brief A simple border. Draws border characters in it's rectangle.
*/
class ui_border : public ui_element {
    private:
        array_2d<long> borders;
        void calc_borders();
    protected:
        void draw( WINDOW *win ) override;
    public:
        ui_border( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void set_rect( const ui_rect &rect ) override;

        nc_color border_color = BORDER_COLOR;
};

///@}

#endif // CATA_UI_H
