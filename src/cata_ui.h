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
* @brief Tries to mimic C#'s event system.
*/
template<typename... Ts>
class ui_event {
    std::vector<std::function<void(Ts&&...)>> listeners;
public:
    void subscribe( std::function<void(Ts&&...)> listener )
    {
        listeners.push_back( listener );
    }

    void operator()(Ts&&... args)
    {
        for( auto listener : listeners ) {
            listener( std::forward<Ts>(args)... );
        }
    }

    void operator+=( std::function<void(Ts&&...)> listener )
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
        void calc_anchored_values();
    protected:
        virtual void draw() = 0;
        virtual WINDOW *get_win() const; /**< Getter for a window to draw with */
        virtual void send_action( const std::string &action ) {}
    public:
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
* @brief A container for ui elements to execute tasks on several elements at once.
*
* Specifically used by tabbed_window to hide/show certain elements that belong to a tab.
*/
class ui_group {
    std::list<ui_element *> elements;

    bool show = true;
public:
    void add() {}

    template<typename Head = ui_element *, typename... Tail>
    void add( Head head, const Tail&... tail )
    {
        head->set_visible( show );
        elements.push_back( head );
        add( tail... );
    }

    void for_each( std::function<void(ui_element *)> &&func )
    {
        for( auto e : elements ) {
            func( e );
        }
    }

    void set_visible( bool visible )
    {
        show = visible;

        for( auto e : elements ) {
            e->set_visible( visible );
        }
    }
};

/**
* @brief The basis for a ui composition.
*
* This is the class in the framework that holds nested elements.
* It is also the only class in the framework with a public 'draw' function.
*/
class ui_window {
    private:
        ui_rect rect; /**< Holds the elements size and position */
        WINDOW *win = nullptr;
        void draw_children();
        std::vector<ui_element *> children;

        input_context ctxt;
    protected:
        virtual void local_draw() { on_local_draw(); } /**< Method to draw things that are features of this window (or a derived type) e.g. a border */
        void add_child( ui_element *child );
    public:
        ui_window( const ui_rect &rect );
        ui_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );
        ~ui_window();

        ui_window( const ui_window & ) = delete;
        ui_window &operator=( const ui_window & ) = delete;
        ui_window( ui_window && ) = delete;
        ui_window &operator=( ui_window && ) = delete;

        void draw();

        WINDOW *get_win() const;

        const ui_rect &get_rect() const;
        void set_rect( const ui_rect &new_rect );

        const std::vector<ui_element *> &get_children() const;

        virtual std::string handle_input();

        ui_event<> on_local_draw;

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
            add_child( child );
            return child;
        }
};

/**
* @brief A simple text label
*/
class ui_label : public ui_element {
    protected:
        std::string text;

        virtual void draw() override;
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
        void local_draw() override;
    public:
        bordered_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );

        nc_color border_color = BORDER_COLOR;
};

/**
* @brief Generic form of health bar
*/
class health_bar : public ui_element {
    private:
        unsigned int max_health;
        unsigned int current_health;

        std::string bar_str = "";
        nc_color bar_color = c_green;

        static constexpr unsigned int points_per_char = 2;

        void refresh_bar( bool overloaded, float percentage );
    protected:
        void draw() override;
    public:
        health_bar( size_t size_x, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void set_health_percentage( float percentage );
};

/**
* @brief A smiley that changes color
*/
class smiley_indicator : public ui_element {
    private:
        enum smiley_state {
            very_unhappy,
            unhappy,
            neutral,
            happy,
            very_happy
        };

        smiley_state state = neutral;
        nc_color smiley_color = c_white;
        std::string smiley_str = "";
    protected:
        void draw() override;
    public:
        smiley_indicator( int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void set_state( smiley_state new_state );
};

/**
* @brief Class that represents a basic cataclysm tile.
*
* To draw different kinds off tiles (like ones using a tile set),
* extend from this class and override it's draw method.
*/
class ui_tile {
    public:
        virtual ~ui_tile() = default;
        virtual void draw( WINDOW *, int, int ) const = 0;
};

class char_tile : public ui_tile {
        long sym;
        nc_color color;
    public:
        char_tile( long tile_char = ' ', nc_color tile_color = c_black );
        void draw( WINDOW *, int, int ) const override;
};

/**
* @brief A panel that draws tiles.
*
* The type argument is the type of tile used. (for memory allocation reasons)
*/
template<class T = ui_tile>
class ui_tile_panel : public ui_element {
    private:
        array_2d<T> tiles;
    protected:
        void draw() override;
    public:
        ui_tile_panel( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void set_rect( const ui_rect &new_rect ) override;
        void set_tile( const T &tile, unsigned int x, unsigned int y );
};

/**
* @brief A window with tabs at the top.
*
### Creating tabs
* Create a tab with the ```create_tab``` method. It returns a ```ui_group```. Use this group to let tabbed_window control
* visibility of elements you add to the window.
*/
class tabbed_window : public bordered_window {
    private:
        std::vector<std::pair<std::string, ui_group *>> tabs;
        unsigned int tab_index = 0;
    protected:
        void local_draw() override;
    public:
        tabbed_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );
        ~tabbed_window();

        static constexpr int header_size = 3;

        ui_group *create_tab( const std::string &tab );

        void next_tab();
        void previous_tab();
        std::string current_tab() const;

        std::string handle_input() override;
};

/**
* @brief Basically, a list of text.
*
* One of the lines of text is highlighted (selected).
* The list also has a scroll bar.
*/
class ui_vertical_list : public ui_element {
    private:
        std::vector<std::string> text;
        std::vector<unsigned int> to_highlight;
        unsigned int scroll = 0;
        unsigned int window_scroll = 0;
    protected:
        void draw() override;
    public:
        ui_vertical_list( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        nc_color text_color = c_white;
        nc_color bar_color = c_ltblue;
        nc_color highlight_color = c_ltgreen;

        void set_text( std::vector<std::string> text );
        void add_highlight( unsigned int i );
        void add_highlight( const std::string &line );

        void scroll_up();
        void scroll_down();
        std::string current() const;

        void send_action( const std::string &action ) override;

        void clear_highlights() { to_highlight.clear(); }

        bool empty() const
        {
            return text.empty();
        }
};

/**
* @brief A special version of ui_vertiacl_list that binds list items to data.
*/
template<typename D>
class ui_record_list : public ui_vertical_list {
        std::map<std::string, D *> data_map;
    public:
        ui_record_list( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left ) : ui_vertical_list(size_x, size_y, x, y, anchor) {
        }

        void make_records( const std::vector<D *> &data, std::function<std::string(D *)> get_text, std::function<bool(D *)> to_highlight = [](D *){} )
        {
            std::vector<std::string> text;

            for( unsigned int i = 0; i < data.size(); i++ ) {
                std::string e_text = get_text( data[i] );
                text.push_back( e_text );
                data_map[e_text] = data[i];

                if( to_highlight( data[i] ) ) {
                    ui_vertical_list::add_highlight( i );
                }
            }

            set_text( text );

        }

        D *get_cur_data()
        {
            return data_map[current()];
        }

        ui_event<D *> on_select;
        ui_event<D *> on_change;

        void send_action( const std::string &action ) override
        {
            ui_vertical_list::send_action( action );

            if( action == "UP" || action == "DOWN" ) {
                if( data_map.size() > 1 ) {
                    on_change( get_cur_data() );
                }
            } else if( action == "CONFIRM" ) {
                if( !data_map.empty() ) {
                    on_select( get_cur_data() );
                }
            }
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
        void draw() override;
    public:
        ui_horizontal_list( int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        nc_color text_color = c_white;

        void set_text( std::vector<std::string> text );

        void scroll_left();
        void scroll_right();
        const std::string &current() const;

        void send_action( const std::string &action );
};

/**
* @brief A label where letters can have individual colors.
*
* To map letters to a color you can use something like ```my_label[c_green] = "XXX___XXX";```
* This means that the first 3 letters will be green, the next 3 will have the default color
* (at least, if they're not mapped to another color) and the last 3 letters will be green again.
* Note that you don't have to use X's, anything that is NOT an underscore will get the color.
*/
class color_mapped_label : public ui_label {
    private:
        std::map<nc_color, std::string> color_map;
    public:
        color_mapped_label( std::string text, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void draw() override;

        std::string &operator[]( nc_color color );
};

/**
* @brief A simple border. Draws border characters in it's rectangle.
*/
class ui_border : public ui_element {
    private:
        array_2d<long> borders;
        void calc_borders();
    public:
        ui_border( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = ui_anchor::top_left );

        void draw() override;

        void set_rect( const ui_rect &rect ) override;

        nc_color border_color = BORDER_COLOR;
};

///@}

#endif // CATA_UI_H
