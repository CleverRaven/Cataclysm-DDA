#ifndef CATA_UI_H
#define CATA_UI_H

#include "output.h"
#include "enums.h"

#include <list>
#include <string>

typedef int nc_color;

// so we can pass around dimensions.
struct rect {
    // Size of the rect.
    size_t size_x, size_y;

    // Position of the rect.
    unsigned int x, y;

    rect( size_t size_x, size_t size_y, unsigned int x, unsigned int y );
};

class ui_element {
    private:
        bool show = true;
    public:
        // protected seems to work different from C#, so this has to be public.
        rect el_rect;

        ui_element(size_t size_x, size_t size_y);
        ui_element(size_t size_x, size_t size_y, unsigned int x, unsigned int y);
        virtual ~ui_element() = default;

        virtual void draw( WINDOW * ) = 0;
        virtual ui_element *clone() const = 0;

        virtual void set_visible(bool visible);
        virtual bool is_visible() const;
};

// composite pattern
class ui_window {
    protected:
        rect el_rect;
        WINDOW *win;

        std::list<ui_element *> children;
    public:
        ui_window(size_t size_x, size_t size_y);
        ui_window(size_t size_x, size_t size_y, unsigned int x, unsigned int y);
        ~ui_window();

        virtual void draw();

        void add_child( ui_element *child );
        void add_child( const ui_element &child );
};

// similar to window but different
class ui_panel : public ui_element {
    public:
        std::list<ui_element *> children;
    public:
        ui_panel(size_t size_x, size_t size_y);
        ui_panel(size_t size_x, size_t size_y, unsigned int x, unsigned int y);
        virtual ui_element *clone() const override;
        ~ui_panel();

        void add_child( ui_element *child );
        void add_child( const ui_element &child );

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
        bordered_panel(size_t size_x, size_t size_y);
        bordered_panel(size_t size_x, size_t size_y, unsigned int x, unsigned int y);
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
        health_bar(size_t size_x);
        health_bar(size_t size_x, unsigned int x, unsigned int y);
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
        smiley_indicator();
        smiley_indicator(unsigned int x, unsigned int y);
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
                   size_t size_x, size_t size_y);
        tile_panel(tripoint center, std::function<const char_tile(int, int, int)> tile_at,
                   size_t size_x, size_t size_y, unsigned int x, unsigned int y);
        virtual ui_element *clone() const override;

        virtual void draw( WINDOW * ) override;

        void set_center(tripoint new_center);
        const tripoint &get_center() const;
};

#endif // CATA_UI_H
