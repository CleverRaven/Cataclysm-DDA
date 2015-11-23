#include "cata_ui.h"

#include "catacharset.h"

#include <cmath>
#include <array>

ui_rect::ui_rect( size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : ui_element(size_x, size_y, x, y),
                      global_x(x), global_y(y), win(newwin(size_y, size_x, global_y, global_x))
{
}

ui_window::ui_window(const ui_window &other) : ui_element(other.rect.size_x, other.rect.size_y, other.rect.x, other.rect.y), global_x(other.global_x),
                                               global_y(other.global_y), win(newwin(rect.size_y, rect.size_x, rect.y, rect.x))
{
    for( auto child : other.children ) {
        add_child(*child);
    }
}

ui_element *ui_window::clone() const
{
    return new ui_window(*this);
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
    local_draw();
    wrefresh( win );
}

void ui_window::local_draw()
{
    for( auto &child : children ) {
        if( child->is_visible() ) {
            child->draw();
        }
    }
}

void ui_window::set_parent( const ui_window *parent )
{
    ui_element::set_parent( parent );
    global_x = parent->global_x + rect.x;
    global_y = parent->global_y + rect.y;

    delwin( win );
    win = newwin(rect.size_y, rect.size_x, global_y, global_x);
}

template<typename T>
T *ui_window::add_child( const T &child )
{
    auto child_clone = child.clone();
    children.push_back( child_clone );
    child_clone->set_parent(this);
    return (T *) child_clone;
}

WINDOW *ui_window::get_win() const
{
    return win;
}

ui_element::ui_element(size_t size_x, size_t size_y, unsigned int x, unsigned int y) : anchored_x(x), anchored_y(y), rect(ui_rect(size_x, size_y, x, y))
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

void ui_element::set_anchor(ui_anchor new_anchor)
{
    anchor = new_anchor;
    calc_anchored_values();
}

void ui_element::calc_anchored_values()
{
    if(parent) {
        auto p_rect = parent->get_rect();
        switch(anchor) {
            case top_left:
                break;
            case top_center:
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2);
                anchored_y = rect.y;
                return;
            case top_right:
                anchored_x = p_rect.size_x - rect.size_x;
                anchored_y = rect.y;
                return;
            case center_left:
                anchored_x = rect.x;
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2);
                return;
            case center_center:
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2);
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2);
                return;
            case center_right:
                anchored_x = p_rect.size_x - rect.size_x;
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2);
                return;
            case bottom_left:
                anchored_x = rect.x;
                anchored_y = p_rect.size_y - rect.size_y;
                return;
            case bottom_center:
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2);
                anchored_y = p_rect.size_y - rect.size_y;
                return;
            case bottom_right:
                anchored_x = p_rect.size_x - rect.size_x;
                anchored_y = p_rect.size_y - rect.size_y;
                return;
        }
    }
    anchored_x = rect.x;
    anchored_y = rect.y;
}

unsigned int ui_element::get_ax() const
{
    return anchored_x;
}

unsigned int ui_element::get_ay() const
{
    return anchored_y;
}

void ui_element::set_parent(const ui_window *parent)
{
    this->parent = parent;
    calc_anchored_values();
}

const ui_rect &ui_element::get_rect() const
{
    return rect;
}

WINDOW *ui_element::get_win() const
{
    if(parent) {
        return parent->get_win();
    }
    return nullptr;
}

ui_label::ui_label( std::string text ,unsigned int x, unsigned int y ) : ui_element( utf8_width( text.c_str() ), 1, x, y ), text( text )
{
}

ui_element *ui_label::clone() const
{
    return new ui_label(*this);
}

void ui_label::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), text_color, text.c_str() );
}

void ui_label::set_text( std::string new_text )
{
    text = new_text;
    rect.size_x = new_text.size();
}

bordered_window::bordered_window(size_t size_x, size_t size_y, unsigned int x, unsigned int y ) : ui_window(size_x, size_y, x, y)
{
}

ui_element *bordered_window::clone() const
{
    return new bordered_window(*this);
}

void bordered_window::local_draw()
{
    wattron(win, border_color);
    wborder(win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff(win, border_color);

    ui_window::local_draw();
}

health_bar::health_bar(size_t size_x, unsigned int x, unsigned int y) : ui_element(size_x, 1, x, y),
                       max_health(size_x * points_per_char), current_health(max_health)
{
    for(unsigned int i = 0; i < rect.size_x; i++) {
        bar_str += "|";
    }
}

ui_element *health_bar::clone() const
{
    return new health_bar(*this);
}

void health_bar::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), bar_color, bar_str.c_str() );
}

void health_bar::refresh_bar( bool overloaded, float percentage )
{
    bar_str = "";
    if( overloaded ) {
        for(unsigned int i = 0; i < rect.size_x; i++) {
            bar_str += "*";
        }
    } else {
        for(unsigned int i = 0; i < rect.size_x; i++) {
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

smiley_indicator::smiley_indicator(unsigned int x, unsigned int y) : ui_element(2, 1, x, y)
{
}

ui_element *smiley_indicator::clone() const
{
    return new smiley_indicator(*this);
}

void smiley_indicator::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), smiley_color, smiley_str.c_str() );
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

tile_panel::tile_panel(tripoint center, std::function<const ui_tile(int, int, int)> tile_at,
                       size_t size_x, size_t size_y, unsigned int x, unsigned int y)
                       : ui_element(size_x, size_y, x, y), tile_at(tile_at), center(center)
{
    if(size_x % 2 == 0) {
        rect.size_x += 1;
    }
    if(size_y % 2 == 0) {
        rect.size_y += 1;
    }

    x_radius = (rect.size_x - 1) / 2;
    y_radius = (rect.size_y - 1) / 2;
}

ui_element *tile_panel::clone() const
{
    return new tile_panel(*this);
}

void tile_panel::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    // temporary variables for cleanliness
    unsigned int start_x = center.x - x_radius;
    unsigned int end_x = center.x + x_radius;

    unsigned int start_y = center.y - y_radius;
    unsigned int end_y = center.y + y_radius;

    for(unsigned int x = start_x; x <= end_x; x++) {
        for(unsigned int y = start_y; y <= end_y; y++) {
            tile_at(x, y, center.z).draw( win, get_ax() + x, get_ay() + y );
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

void ui_tile::draw( WINDOW *win, int x, int y ) const
{
    mvwputch(win, x, y, color, sym);
}

ui_tile::ui_tile(long sym, nc_color color) : sym(sym), color(color)
{
}

tabbed_window::tabbed_window(size_t size_x, size_t size_y, unsigned int x, unsigned int y) : bordered_window(size_x, size_y, x, y)
{
}

ui_element *tabbed_window::clone() const
{
    return new tabbed_window(*this);
}

// returns the width of the tab
int tabbed_window::draw_tab(const std::string &tab, bool selected, int x_offset) const
{
    int gy = get_ay();
    int gx = get_ax() + x_offset;

    int width = utf8_width(tab.c_str()) + 2;
    //print top border
    for(int x = gx; x < gx + width; x++) {
        mvwputch(win, gy, x, border_color, LINE_OXOX);
    }

    //print text and 2 borders
    mvwputch(win, gy + 1, gx, border_color, LINE_XOXO);
    mvwprintz(win, gy + 1, gx + 1, (selected ? hilite(border_color) : border_color), tab.c_str());
    mvwputch(win, gy + 1, gx + width, border_color, LINE_XOXO);

    if(selected) {
        // print selection braces
        mvwputch(win, gy + 1, gx - 1, border_color, '<');
        mvwputch(win, gy + 1, gx + width + 1, border_color, '>');

        // print bottom tab corners
        mvwputch(win, gy + 2, gx, border_color, LINE_XOOX);
        mvwputch(win, gy + 2, gx + width, border_color, LINE_XXOO);
    } else {
        // print T
        mvwputch(win, gy + 2, gx, border_color, LINE_XXOX);
        // print bottom border
        for(int x = gx + 1; x < gx + width - 1; x++) {
            mvwputch(win, gy + 2, x, border_color, LINE_OXOX);
        }
        // print T
        mvwputch(win, gy + 2, gx + width, border_color, LINE_XXOX);
    }
    return width;
}

void tabbed_window::local_draw()
{
    bordered_window::local_draw();
    //erase the top 3 rows
    for(int y = 0; y < 3; y++) {
        for(int x = 0; x < rect.size_x; x++){
            mvwputch(win, y, x, border_color, ' ');
        }
    }

    int x_offset = 1; // leave space for selection bracket
    for(unsigned int i = 0; i < tabs.size(); i++) {
        x_offset += draw_tab(tabs[i], tab_index == i, x_offset) + 1;
    }
}

void tabbed_window::add_tab(std::string tab)
{
    tabs.push_back(tab);
}

void tabbed_window::next_tab()
{
    if( tab_index == tabs.size() - 1 ){
        tab_index = 0;
    } else {
        tab_index++;
    }
}

void tabbed_window::previous_tab()
{
    if( tab_index == 0 ){
        tab_index = tabs.size() - 1;
    } else {
        tab_index--;
    }
}


const std::string &tabbed_window::current_tab() const
{
    return tabs[tab_index];
}
