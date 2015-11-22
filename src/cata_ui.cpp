#include "cata_ui.h"

#include "catacharset.h"

#include <cmath>
#include <array>

rect::rect( size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( size_t size_x, size_t size_y ) : ui_window( size_x, size_y, 0, 0 )
{
}

ui_window::ui_window( size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : el_rect( rect( size_x, size_y, x, y ) ), win( newwin(size_x, size_y, x, y) )
{
}

ui_window::~ui_window()
{
    while( children.size() > 0 ) {
        delete children.back();
    }
    delwin( win );
}

void ui_window::draw()
{
    werase( win );
    for( auto &child : children ) {
        if( child->is_visible() ) {
            child->draw( win );
        }
    }
    wrefresh( win );
}

void ui_window::add_child( ui_element *child )
{
    if( !child ) {
        return;
    }

    children.push_back( child );
}

void ui_window::add_child( const ui_element &child )
{
    add_child( child.clone() );
}

ui_element::ui_element(size_t size_x, size_t size_y) : ui_element(size_x, size_y, 0, 0)
{
}

ui_element::ui_element(size_t size_x, size_t size_y, unsigned int x, unsigned int y) : el_rect(rect(size_x, size_y, x, y))
{
}

void ui_element::set_visible(bool visible)
{
    show = visible;
}

bool ui_element::is_visible() const
{
    return show;
}

ui_panel::ui_panel(size_t size_x, size_t size_y) : ui_panel(size_x, size_y, 0, 0)
{
}

ui_panel::ui_panel(size_t size_x, size_t size_y, unsigned int x, unsigned int y) : ui_element(size_x, size_y, x, y)
{
}

ui_panel::~ui_panel()
{
    while( children.size() > 0 ) {
        delete children.back();
    }
}

ui_element *ui_panel::clone() const
{
    auto panel_clone = new ui_panel(*this);
    panel_clone->children.clear();

    for(const auto &child : children) {
        panel_clone->children.push_back(child->clone());
    }

    return panel_clone;
}

void ui_panel::add_child( ui_element *child )
{
    if( !child ) {
        return;
    }

    // offset the child's location by parent location.
    child->el_rect.x += el_rect.x;
    child->el_rect.y += el_rect.y;

    // TODO: move/resize child to fit in window.

    children.push_back( child );
}

void ui_panel::add_child( const ui_element &child )
{
    add_child( child.clone() );
}

void ui_panel::draw( WINDOW *win )
{
    for(auto &child : children) {
        if( child->is_visible() ) {
            child->draw( win );
        }
    }
}

ui_label::ui_label( std::string text ) : ui_label( text, 0, 0 )
{
}

ui_label::ui_label( std::string text ,unsigned int x, unsigned int y ) : ui_element( utf8_width( text.c_str() ), 1, x, y ), text( text )
{
}

ui_element *ui_label::clone() const
{
    return new ui_label(*this);
}

void ui_label::draw( WINDOW *win )
{
    mvwprintz( win, el_rect.y, el_rect.x, text_color, text.c_str() );
}

void ui_label::set_text( std::string new_text )
{
    text = new_text;
    el_rect.size_x = new_text.size();
}

bordered_panel::bordered_panel(size_t size_x, size_t size_y) : bordered_panel(size_x, size_y, 0, 0)
{
}

bordered_panel::bordered_panel(size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : ui_panel(size_x, size_y, x, y)
{
}

ui_element *bordered_panel::clone() const
{
    return new bordered_panel(*this);
}

void bordered_panel::draw( WINDOW *win )
{
    wattron(win, border_color);
    wborder(win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff(win, border_color);

    ui_panel::draw( win );
}

health_bar::health_bar(size_t size_x) : health_bar(size_x, 0, 0)
{
}

health_bar::health_bar(size_t size_x, unsigned int x, unsigned int y) : ui_element(size_x, 1, x, y),
                       max_health(size_x * points_per_char), current_health(max_health)
{
    for(unsigned int i = 0; i < el_rect.size_x; i++) {
        bar_str += "|";
    }
}

ui_element *health_bar::clone() const
{
    return new health_bar(*this);
}

void health_bar::draw( WINDOW *win )
{
    mvwprintz( win, el_rect.y, el_rect.x, bar_color, bar_str.c_str() );
}

void health_bar::refresh_bar( bool overloaded, float percentage )
{
    bar_str = "";
    if( overloaded ) {
        for(unsigned int i = 0; i < el_rect.size_x; i++) {
            bar_str += "*";
        }
    } else {
        for(unsigned int i = 0; i < el_rect.size_x; i++) {
            unsigned int char_health = current_health - (i * points_per_char);
            if(char_health <= 0) {
                bar_str += ".";
            } else if(char_health == 1) {
                bar_str += "\\";
            } else {
                bar_str += "|";
            }
        }
    }

    static const std::array<nc_color, 7> colors {
        c_green,
        c_ltgreen,
        c_yellow,
        c_magenta,
        c_ltred,
        c_red,
        c_ltgray
    };

    bar_color = colors[(int) roundf(colors.size() * percentage)];
}

void health_bar::set_health_percentage( float percentage )
{
    bool overloaded = false;
    if(percentage > 1) {
        current_health = max_health;
        overloaded = true;
    } else if( percentage < 0 ) {
        current_health = 0;
    } else {
        current_health = percentage * max_health;
    }

    refresh_bar( overloaded, percentage );
}

smiley_indicator::smiley_indicator() : smiley_indicator(0, 0)
{
}

smiley_indicator::smiley_indicator(unsigned int x, unsigned int y) : ui_element(2, 1, x, y)
{
}

ui_element *smiley_indicator::clone() const
{
    return new smiley_indicator(*this);
}

void smiley_indicator::draw( WINDOW *win )
{
    mvwprintz( win, el_rect.y, el_rect.x, smiley_color, smiley_str.c_str() );
}

void smiley_indicator::set_state( smiley_state new_state )
{
    state = new_state;
    switch(new_state) {
        case very_unhappy:
            smiley_color = c_red;
            smiley_str = "D:";
            break;
        case unhappy:
            smiley_color = c_ltred;
            smiley_str = ":(";
            break;
        case neutral:
            smiley_color = c_white;
            smiley_str = ":|";
            break;
        case happy:
            smiley_color = c_ltgreen;
            smiley_str = ":)";
            break;
        case very_happy:
            smiley_color = c_green;
            smiley_str = ":D";
            break;
    }
}

tile_panel::tile_panel(tripoint center, std::function<const char_tile(int, int, int)> tile_at,
           size_t size_x, size_t size_y) : tile_panel(center, tile_at, size_x, size_y, 0, 0)
{
}

tile_panel::tile_panel(tripoint center, std::function<const char_tile(int, int, int)> tile_at,
           size_t size_x, size_t size_y, unsigned int x, unsigned int y) : ui_element(size_x, size_y, x, y), tile_at(tile_at), center(center)
{
    if(size_x % 2 == 0) {
        el_rect.size_x += 1;
    }
    if(size_y % 2 == 0) {
        el_rect.size_y += 1;
    }

    x_radius = (el_rect.size_x - 1) / 2;
    y_radius = (el_rect.size_y - 1) / 2;
}

ui_element *tile_panel::clone() const
{
    return new tile_panel(*this);
}

void tile_panel::draw( WINDOW *win )
{
    // temporary variables for cleanliness
    unsigned int start_x = center.x - x_radius;
    unsigned int end_x = center.x + x_radius;

    unsigned int start_y = center.y - y_radius;
    unsigned int end_y = center.y + y_radius;

    for(unsigned int x = start_x; x <= end_x; x++) {
        for(unsigned int y = start_y; y <= end_y; y++) {
            tile_at(x, y, center.z).draw( win, el_rect.x + x, el_rect.y + y );
        }
    }
}

void tile_panel::set_center(tripoint new_center)
{
    center = new_center;
}

const tripoint &tile_panel::get_center() const
{
    return center;
}

void char_tile::draw( WINDOW *win, int x, int y ) const
{
    mvwputch(win, x, y, color, sym);
}

char_tile::char_tile(long sym, nc_color color) : sym(sym), color(color)
{
}
