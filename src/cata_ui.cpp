#include "cata_ui.h"

#include "catacharset.h"

#include "debug.h"

#include <cmath>
#include <array>

ui_rect::ui_rect( size_t size_x, size_t size_y, int x, int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) : ui_element(size_x, size_y, x, y, anchor),
                      global_x(ui_element::anchored_x), global_y(ui_element::anchored_y), win(newwin(size_y, size_x, global_y, global_x))
{
    // adjust_window() // Does ui_element's constructor call our version of calc_anchored_values
}

ui_window::~ui_window()
{
    for( auto child : children ) {
        delete child;
    }

    delwin( win );
}

void ui_window::draw()
{
    werase( win );
    local_draw();
    wrefresh( win );
    draw_children();
}

void ui_window::draw_children()
{
    for( auto &child : children ) {
        if( child->is_visible() ) {
            child->draw();
        }
    }
}

void ui_window::adjust_window()
{
    global_x = anchored_x;
    global_y = anchored_y;
    if(parent) {
        global_x += parent->global_x;
        global_y += parent->global_y;
    }

    // I don't think we can change a WINDOW's values without creating a new one
    delwin( win );
    win = newwin(rect.size_y, rect.size_x, global_y, global_x);
}

void ui_window::set_parent( const ui_window *parent )
{
    ui_element::set_parent( parent );
    adjust_window();

    for(auto child : children) {
        child->calc_anchored_values();
    }
}

void ui_window::set_rect(const ui_rect &new_rect)
{
    ui_element::set_rect(new_rect);
    adjust_window();

    for(auto child : children) {
        child->calc_anchored_values();
    }
}

void ui_window::set_anchor(ui_anchor new_anchor)
{
    ui_element::set_anchor(new_anchor);
    adjust_window();

    for(auto child : children) {
        child->calc_anchored_values();
    }
}

// This method expects a pointer to a heap allocated ui_element.
// This ui_element will be deleted by the ui_window's deconstructor, so you can add-and-forget.
void ui_window::add_child( ui_element *child )
{
    if(!child) {
        return; // In case we get passed a bad alloc
    }

    children.push_back( child );
    child->set_parent(this);
}

WINDOW *ui_window::get_win() const
{
    return win;
}

size_t ui_window::child_count() const
{
    return children.size();
}

const std::list<ui_element *> &ui_window::get_children() const
{
    return children;
}

ui_element::ui_element(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) :
                       anchor(anchor), anchored_x(x), anchored_y(y), rect(ui_rect(size_x, size_y, x, y))
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

void ui_element::above(const ui_element &other, int x, int y)
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + x;
    rect.y = o_rect.y + o_rect.size_y + y;
    calc_anchored_values();
}

void ui_element::below(const ui_element &other, int x, int y)
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + x;
    rect.y = o_rect.y - o_rect.size_y + y;
    calc_anchored_values();
}

void ui_element::after(const ui_element &other, int x, int y)
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x + o_rect.size_x + x;
    rect.y = o_rect.y + y;
    calc_anchored_values();
}

void ui_element::before(const ui_element &other, int x, int y)
{
    auto o_rect = other.get_rect();
    rect.x = o_rect.x - o_rect.size_x + x;
    rect.y = o_rect.y + y;
    calc_anchored_values();
}

void ui_element::set_rect(const ui_rect &new_rect)
{
    rect = new_rect;
    calc_anchored_values();
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
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2) + rect.x;
                anchored_y = rect.y;
                return;
            case top_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = rect.y;
                return;
            case center_left:
                anchored_x = rect.x;
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2) + rect.y;
                return;
            case center_center:
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2) + rect.x;
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2) + rect.y;
                return;
            case center_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = (p_rect.size_y / 2) - (rect.size_y / 2) + rect.y;
                return;
            case bottom_left:
                anchored_x = rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
                return;
            case bottom_center:
                anchored_x = (p_rect.size_x / 2) - (rect.size_x / 2) + rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
                return;
            case bottom_right:
                anchored_x = p_rect.size_x - rect.size_x + rect.x;
                anchored_y = p_rect.size_y - rect.size_y + rect.y;
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

ui_label::ui_label( std::string text ,int x, int y, ui_anchor anchor ) : ui_element( utf8_width( text.c_str() ), 1, x, y, anchor ), text( text )
{
}

void ui_label::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), text_color, text.c_str() );

    wrefresh(win);
}

void ui_label::set_text( std::string new_text )
{
    text = new_text;
    set_rect(ui_rect(new_text.size(), get_rect().size_y, get_rect().x, get_rect().y));
}

bordered_window::bordered_window(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor ) : ui_window(size_x, size_y, x, y, anchor)
{
}

void bordered_window::local_draw()
{
    auto win = get_win(); // never null

    wattron(win, border_color);
    wborder(win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wattroff(win, border_color);
}

health_bar::health_bar(size_t size_x, int x, int y, ui_anchor anchor) : ui_element(size_x, 1, x, y, anchor),
                       max_health(size_x * points_per_char), current_health(max_health)
{
    for(unsigned int i = 0; i < get_rect().size_x; i++) {
        bar_str += "|";
    }
}

void health_bar::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), bar_color, bar_str.c_str() );
    wrefresh(win);
}

void health_bar::refresh_bar( bool overloaded, float percentage )
{
    bar_str = "";
    if( overloaded ) {
        for(unsigned int i = 0; i < get_rect().size_x; i++) {
            bar_str += "*";
        }
    } else {
        for(unsigned int i = 0; i < get_rect().size_x; i++) {
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

smiley_indicator::smiley_indicator(int x, int y, ui_anchor anchor) : ui_element(2, 1, x, y, anchor)
{
}

void smiley_indicator::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    mvwprintz( win, get_ay(), get_ax(), smiley_color, smiley_str.c_str() );

    wrefresh(win);
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

template<class T>
tile_panel<T>::tile_panel(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor)
                       : ui_element(size_x, size_y, x, y, anchor)
{
    num_tiles = size_x * size_y;
    tiles = new T[num_tiles];
}

template<class T>
void tile_panel<T>::set_rect(const ui_rect &new_rect)
{
    ui_window::set_rect(new_rect);
    num_tiles = new_rect.size_x * new_rect.size_y;
    tiles = realloc(tiles, num_tiles * sizeof(T));
}

template<class T>
tile_panel<T>::~tile_panel()
{
    free(tiles);
}

template<class T>
void tile_panel<T>::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    for(unsigned int x = 0; x < get_rect().size_x; x++) {
        for(unsigned int y = 0; y < get_rect().size_y; y++) {
            tiles[y * get_rect().size_x + x].draw(win, x, y);
        }
    }

    wrefresh(win);
}

template<class T>
void tile_panel<T>::set_tile(const T &tile, unsigned int x, unsigned int y)
{
    if(x >= get_rect().size_x || y >= get_rect().size_y) {
        return; // TODO: give feedback
    }

    int index = y * get_rect().size_x + x;

    delete tiles[index];
    tiles[index] = tile; // Does this call T's copy constructor? or maybe of a derived type?
}

void ui_tile::draw( WINDOW *win, int x, int y ) const
{
    mvwputch(win, x, y, color, sym);
}

ui_tile::ui_tile(long sym, nc_color color) : sym(sym), color(color)
{
}

tabbed_window::tabbed_window(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) : bordered_window(size_x, size_y, x, y, anchor)
{
}

// returns the width of the tab
int tabbed_window::draw_tab(const std::string &tab, bool selected, int x_offset) const
{
    auto win = get_win(); // never null
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
    auto win = get_win(); // never null
    //erase the top 3 rows
    for(unsigned int y = 0; y < 3; y++) {
        for(unsigned int x = 0; x < get_rect().size_x; x++){
            mvwputch(win, y, x, border_color, ' ');
        }
    }

    int x_offset = 1; // leave space for selection bracket
    for(unsigned int i = 0; i < tabs.size(); i++) {
        x_offset += draw_tab(tabs[i].first, tab_index == i, x_offset) + 1;
    }
}

template<class T>
T *tabbed_window::create_tab(std::string tab)
{
    T *tab_win = new T(get_rect().size_x - 2, get_rect().size_y - 4, 1, -1, bottom_left);
    tabs.push_back({tab, tab_win});
    tab_win->set_visible(tabs.size() == 1 ? true : false);
    add_child(tab_win);
    return tab_win;
}

void tabbed_window::next_tab()
{
    tabs[tab_index].second->set_visible(false);
    if( tab_index == tabs.size() - 1 ){
        tab_index = 0;
    } else {
        tab_index++;
    }
    tabs[tab_index].second->set_visible(true);
}

void tabbed_window::previous_tab()
{
    tabs[tab_index].second->set_visible(false);
    if( tab_index == 0 ){
        tab_index = tabs.size() - 1;
    } else {
        tab_index--;
    }
    tabs[tab_index].second->set_visible(true);
}

const std::pair<std::string, ui_window *> &tabbed_window::current_tab() const
{
    return tabs[tab_index];
}

auto_bordered_window::auto_bordered_window(size_t size_x, size_t size_y, int x , int y, ui_anchor anchor) : ui_window(size_x, size_y, x, y, anchor)
{
    uncovered = new bool[size_x * size_y];
    recalc_uncovered();
}

auto_bordered_window::~auto_bordered_window()
{
    free(uncovered);
}

void auto_bordered_window::set_rect(const ui_rect &new_rect)
{
    ui_window::set_rect(new_rect);
    uncovered = (bool *) realloc(uncovered, new_rect.size_x * new_rect.size_y * sizeof(bool));
    recalc_uncovered();
}

void auto_bordered_window::add_child( ui_element *child )
{
    ui_window::add_child(child);
    recalc_uncovered();
}

void auto_bordered_window::recalc_uncovered()
{
    for(size_t x = 0; x < get_rect().size_x; x++) {
        for(size_t y = 0; y < get_rect().size_y; y++) {
            uncovered[y * get_rect().size_x + x] = true;
        }
    }

    for(auto &child : get_children()) {
        auto c_rect = child->get_rect();
        for(size_t x = c_rect.x; x < c_rect.size_x + c_rect.x; x++) {
            for(size_t y = c_rect.y; y < c_rect.size_y + c_rect.y; y++) {
                if(x < get_rect().size_x && y < get_rect().size_y) {
                    uncovered[y * get_rect().size_x + x] = false;
                }
            }
        }
    }
}

bool auto_bordered_window::is_uncovered(int x, int y) const
{
    if(x < 0 || y < 0 || (unsigned int) x >= get_rect().size_x || (unsigned int) y >= get_rect().size_y) {
        return false;
    }
    return uncovered[y * get_rect().size_x + x];
}

long auto_bordered_window::get_border_char(unsigned int x, unsigned int y) const
{
    bool left = is_uncovered(x - 1, y);
    bool up = is_uncovered(x, y + 1);
    bool right = is_uncovered(x + 1, y);
    bool down = is_uncovered(x, y - 2);

    long ret = ' ';

    if(left) {
        ret = LINE_OXOX; // '-'
    }

    if(up) {
        if(left) {
            ret = LINE_XOOX; // '_|'
        } else {
            ret = LINE_XOXO; // '|'
        }
    }

    if(right) {
        if(up) {
            if(left) {
                ret = LINE_XXOX; // '_|_'
            } else {
                ret = LINE_XXOO; // '|_'
            }
        } else {
            ret = LINE_OXOX; // '-'
        }
    }

    if(down) {
        if(right) {
            if(up) {
                if(left) {
                    ret = LINE_XXXX; // '-|-'
                } else {
                    ret = LINE_XXXO; // '|-'
                }
            } else {
                if(left) {
                    ret = LINE_OXXX; // '^|^'
                } else {
                    ret = LINE_OXXO; // '|^'
                }
            }
        } else {
            if(left) {
                if(up) {
                    ret = LINE_XOXX; // '-|'
                } else {
                    ret = LINE_OOXX; // '^|'
                }
            } else {
                ret = LINE_XOXO; // '|'
            }
        }
    }

    return ret;
}

void auto_bordered_window::local_draw()
{
    ui_window::local_draw();

    auto win = get_win(); // never null
    for(unsigned int x = 0; x < get_rect().size_x; x++){
        for(unsigned int y = 0; y < get_rect().size_y; y++) {
            if(is_uncovered(x, y)) {
                mvwputch(win, y, x, border_color, get_border_char(x, y));
            }
        }
    }
}

void label_test()
{
    auto lable1 = new ui_label("some", 0, 0, top_left);
    lable1->text_color = c_red;
    auto lable2 = new ui_label("anchored", 0, 0, center_center);
    lable2->text_color = c_ltblue;
    auto lable3 = new ui_label("labels", 0, 0, bottom_right);
    lable3->text_color = c_ltgreen;

    bordered_window win(31, 13, 50, 15);
    win.add_child(lable1);
    win.add_child(lable2);
    win.add_child(lable3);
    win.draw();
}

void ui_test_func()
{
    tabbed_window win(31, 14, 50, 15);
    auto t_win = win.create_tab<bordered_window>("my tab");
    auto label = new ui_label("test", 0, 0, center_center);
    t_win->add_child(label);
    win.draw();
}
