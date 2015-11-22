#ifndef CATA_UI_H
#define CATA_UI_H

#include "output.h"
#include "enums.h"

#include <list>
#include <string>

typedef int nc_color;

// so we can pass around dimensions.
class rect {
    private:
        const rect *parent;

        // Position of the rect relative to the parent.
        // These are private because we need the setters to be used.
        unsigned int x, y;
        unsigned int global_x, global_y;
    public:
        // Size of the rect.
        size_t size_x, size_y;

        rect( size_t size_x, size_t size_y, unsigned int x, unsigned int y );

        void set_parent(const rect *p_rect);

        unsigned int get_x() const;
        void set_x(unsigned int new_x);

        unsigned int get_y() const;
        void set_y(unsigned int new_y);

        unsigned int get_global_x() const;
        unsigned int get_global_y() const;
};

class ui_element {
    protected:
        bool show = true;

        rect el_rect;
    public:
        ui_element(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ~ui_element() = default;

        virtual void draw( WINDOW * ) = 0;
        virtual ui_element *clone() const = 0;

        virtual void set_parent(ui_element *parent);

        const rect &get_rect() const;

        virtual void set_visible(bool visible);
        virtual bool is_visible() const;
};

// composite pattern
class ui_window {
    protected:
        WINDOW *win;

        std::list<ui_element *> children;
    public:
        ui_window(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        ~ui_window();

        virtual void draw();

        template<typename T = ui_element>
        T *add_child( const T &child );
};

// similar to window but different
class ui_panel : public ui_element {
    public:
        std::list<ui_element *> children;
    public:
        ui_panel(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;
        ~ui_panel();

        template<typename T = ui_element>
        T *add_child( const T &child );

        virtual void draw( WINDOW * ) override;
};

class ui_label : public ui_element {
    private:
        std::string text;
    public:
        ui_label( std::string text );
        ui_label( std::string text, unsigned int x, unsigned int y );
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

        void set_text( std::string );

        nc_color text_color = c_white;
};

class bordered_panel : public ui_panel {
    public:
        bordered_panel(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

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
    public:
        health_bar(size_t size_x, unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

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
    public:
        smiley_indicator(unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

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
    public:
        tile_panel(tripoint center, std::function<const char_tile(int, int, int)> tile_at,
                   size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

        void set_center(tripoint new_center);
        const tripoint &get_center() const;
};

class tabbed_panel : public bordered_panel {
    private:
        std::vector<std::string> tabs;
        unsigned int tab_index;

        int draw_tab(const std::string &, bool, int, WINDOW *) const;
    public:
        tabbed_panel(size_t size_x, size_t size_y, unsigned int x = 0, unsigned int y = 0);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

        void add_tab(std::string tab);

        void next_tab();
        void previous_tab();
        const std::string &current_tab() const;
};

#endif // CATA_UI_H
