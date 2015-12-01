#ifndef CATA_UI_H
#define CATA_UI_H

#include "output.h"
#include "enums.h"
#include "cata_utility.h"

#include <utility>
#include <list>
#include <string>
#include <map>

typedef int nc_color;

/**
 * @defgroup Cata_ui Cataclysm's ui framework.
 *
 * The ui framework is implemented with the composite pattern.
 * This allows us to nest ui elements within another. We only need to call 'draw'
 * on the top level object, and the composite pattern takes care of the rest.
 *
 *
 ### Implementing your own ui_element.
 * To implement an effective ui_element sub-class you must implement a 'clone' method.
 * This method is used by ui_window's copy constructor to clone it's children (without knowing the internal type at runtime).
 * The implementation can just be
 ```
 ui_element *my_element::clone() const
 {
    return new my_element( *this );
 }
 ```
 * and that will be sufficient. Override the class's copy constructor
 * if you want to clone internal items too (like in ui_window).
 *
 * Next you have to implement a draw method. To do that
 * you probably want a window to draw on, you can use ```get_win()``` for that, just be sure to check if it's not null,
 * which it technically can be if the parent is not set. It's also important (at least, if you want to use anchors) that
 * you use ```get_ax()``` and ```get_ay()``` to retrieve anchor adjusted x and y positions (respectively)
 * rather than using ```get_rect().x``` and ```get_rect().y```. You don't need to manually call the draw method, that's done
 * by the window you add it to. On that note, you should probably keep it 'protected' to avoid doing that accidentally.
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

enum ui_anchor {
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
        void set_parent( const ui_window *parent ); /**< Virtual setter for ```parent```, so derived classes can do extra calculations */

        ui_anchor anchor = top_left; /**< This elements current anchoring inside the parent */

        unsigned int anchored_x, anchored_y; /**< Internal kept values to store anchor adjusted position */

        bool show = true; /**< Indicates to ui_window if this element should be drawn */
        ui_rect rect; /**< Holds the elements size and position */
        void calc_anchored_values();
    protected:
        virtual void draw() = 0;
        virtual WINDOW *get_win() const; /**< Getter for a window to draw with */
    public:
        ui_element( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = top_left );
        virtual ui_element *clone() const = 0;  // Currently only needed to copy a ui_window
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
class ui_window {
    private:
        ui_rect rect; /**< Holds the elements size and position */
        WINDOW *win = nullptr;
        void draw_children();
        std::list<ui_element *> children;
    protected:
        virtual void local_draw() {} /**< Method to draw things that are features of this window (or a derived type) e.g. a border */
        void add_child( ui_element *child );
    public:
        ui_window( const ui_rect &rect );
        ui_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );
        ui_window( const ui_window &other ); /**< Override of the standard copy constructor. We have to clone all nested elements too (not just the pointers) */
        ~ui_window();

        void draw();

        WINDOW *get_win() const;

        const ui_rect &get_rect() const;
        void set_rect( const ui_rect &new_rect );

        const std::list<ui_element *> &get_children() const;

        /**
        * @brief Creates a copy of the passed ```ui_element```and stores it in it's list of children.
        *
        * The copying is a safety precaution, so you can add the same element to multiple windows
        * without the same pointer becoming managed by multiple windows.
        * @param child The ```ui_element``` to copy from.
        * @return Returns a pointer to the copy it made (so you can control it later)
        */
        template<class T = ui_element>
        T *create_child( const T &child );
};

/**
* @brief A simple text label
*/
class ui_label : public ui_element {
    protected:
        std::string text;

        virtual void draw() override;
    public:
        ui_label( std::string text, int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

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
* \brief Generic form of health bar
*/
class health_bar : public ui_element {
    private:
        unsigned int max_health;
        unsigned int current_health;

        std::string bar_str = "";
        nc_color bar_color = c_green;

        static const unsigned int points_per_char = 2;

        void refresh_bar( bool overloaded, float percentage );
    protected:
        void draw() override;
    public:
        health_bar( size_t size_x, int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

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
        smiley_indicator( int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

        void set_state( smiley_state new_state );
};

/**
* @brief Class that represents a basic cataclysm tile.
*
* This basic class has just a color and symbol, and a virtual
* draw function. To draw other kinds off tiles (like ones using a tile set),
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
        ui_tile_panel( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

        void set_rect( const ui_rect &new_rect ) override;
        void set_tile( const T &tile, unsigned int x, unsigned int y );
};

/**
* @brief A window with tabs at the top.
*
### Creating tabs
* Sometimes when you create a tab you want to also hook up some controls. But (if done naively)
* you would have to keep a pointer to every element you want to control, in a list and keep track
* of the current tab you're on to know which element to control. But there is a better way! ```create_tab```
* takes a type argument with the ```ui_window``` constraint. So what you could do, if you don't
* want to keep track of things, is to extend a ```ui_window``` class and within that, do tab specific setup,
* and have tab specific fields (e.g. if you have a list you want to control in the tab, you would have a list field).
* and pass that class's type as a type argument. You can then request the current tab (with ```current_tab```)
* cast the pointer to the type of your subclass (it doesn't get sliced) and access your list field
* to send controls to. Then, you would only have to keep track of the types mapped to the different
* tabs (through the tab's name), which is only needed if you have different kind of tabs.
*/
class tabbed_window : public bordered_window {
    private:
        std::vector<std::string> tabs;
        unsigned int tab_index = 0;
    protected:
        void local_draw() override;
    public:
        tabbed_window( size_t size_x, size_t size_y, int x = 0, int y = 0 );
        static constexpr int header_size = 3;

        void add_tab( const std::string &tab );

        void next_tab();
        void previous_tab();
        std::string current_tab() const;
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
        unsigned int scroll = 0;
        unsigned int window_scroll = 0;
    protected:
        void draw() override;
    public:
        ui_vertical_list( size_t size_x, size_t size_y, int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

        nc_color text_color = c_white;
        nc_color bar_color = c_ltblue;

        void set_text( std::vector<std::string> text );

        void scroll_up();
        void scroll_down();
        const std::string &current() const;
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
        ui_horizontal_list( int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

        nc_color text_color = c_white;

        void set_text( std::vector<std::string> text );

        void scroll_left();
        void scroll_right();
        const std::string &current() const;
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
        color_mapped_label( std::string text, int x = 0, int y = 0, ui_anchor anchor = top_left );
        ui_element *clone() const override;

        void draw() override;

        std::string &operator[]( nc_color color );
};

///@}

void ui_test_func();

#endif // CATA_UI_H
