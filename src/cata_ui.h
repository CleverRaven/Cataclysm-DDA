#ifndef CATA_UI_H
#define CATA_UI_H

#include "output.h"
#include "enums.h"

#include <list>
#include <string>

typedef int nc_color;

// so we can pass around dimensions.
struct ui_rect {
    // Size of the rect.
    size_t size_x, size_y;
    //position of the rect. (within the parent)
    unsigned int x, y;

    ui_rect( size_t size_x, size_t size_y, unsigned int x, unsigned int y );
};

class ui_window;

class ui_element {
    friend class ui_window; // so we don't have to make draw, clone and set_parent public
    private:
        const ui_window *parent;
        virtual void set_parent(const ui_window *parent);
    protected:
        bool show = true;
        ui_rect rect;

        virtual void draw() = 0;
        virtual ui_element *clone() const = 0;
        virtual WINDOW *get_win() const;
    public:
        ui_element(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ~ui_element() = default;

        const ui_rect &get_rect() const;

        void set_visible(bool visible);
        bool is_visible() const;
};

// composite pattern
class ui_window : public ui_element {
    friend class ui_element;
    private:
        void set_parent( const ui_window *parent ) override;

        int global_x;
        int global_y;

        std::list<ui_element *> children;
    protected:
        WINDOW *win;

        virtual ui_element *clone() const override;
        virtual WINDOW *get_win() const override;
        virtual void draw_internal();
    public:
        ui_window(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        ui_window(const ui_window &);
        ~ui_window() override;

        void draw() override;

        template<typename T = ui_element>
        T *add_child( const T &child );
};

class ui_label : public ui_element {
    private:
        std::string text;
    protected:
        virtual ui_element *clone() const override;
        virtual void draw() override;
    public:
        ui_label( std::string text );
        ui_label( std::string text, unsigned int x, unsigned int y );

        void set_text( std::string );

        nc_color text_color = c_white;
};

class bordered_window : public ui_window {
    protected:
        virtual ui_element *clone() const override;
        virtual void draw_internal() override;
    public:
        bordered_window(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);

        nc_color border_color = BORDER_COLOR;
};

class health_bar : public ui_element {
    private:
        unsigned int max_health;
        unsigned int current_health;

        std::string bar_str = "";
        nc_color bar_color = c_green;

        static const unsigned int points_per_char = 2;

        void refresh_bar( bool overloaded, float percentage );
    protected:
        virtual ui_element *clone() const override;
        virtual void draw() override;
    public:
        health_bar(size_t size_x, unsigned int x = 0, unsigned int y = 0);

        void set_health_percentage( float percentage );
};


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
        virtual ui_element *clone() const override;
        virtual void draw() override;
    public:
        smiley_indicator(unsigned int x = 0, unsigned int y = 0);

        void set_state( smiley_state new_state );
};

class char_tile {
    public:
        long sym;
        nc_color color;

        char_tile(long tile_char, nc_color tile_color);

        void draw( WINDOW *, int, int ) const;
};

class tile_panel : public ui_element {
    private:
        std::function<const char_tile(int, int, int)> tile_at;
        tripoint center;

        unsigned int x_radius;
        unsigned int y_radius;
    protected:
        virtual ui_element *clone() const override;
        virtual void draw() override;
    public:
        tile_panel(tripoint center, std::function<const char_tile(int, int, int)> tile_at,
                   size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);

        void set_center(tripoint new_center);
        const tripoint &get_center() const;
};

class tabbed_window : public bordered_window {
    private:
        std::vector<std::string> tabs;
        unsigned int tab_index;

        int draw_tab(const std::string &, bool, int, WINDOW *) const;
    protected:
        virtual ui_element *clone() const override;
        virtual void draw_internal() override;
    public:
        tabbed_window(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);

        void add_tab(std::string tab);

        void next_tab();
        void previous_tab();
        const std::string &current_tab() const;
};

#endif // CATA_UI_H
