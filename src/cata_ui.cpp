#include "cata_ui.h"

#include "catacharset.h"
#include "output.h"
#include "input.h"

#include "debug.h"

#include <cmath>
#include <array>

window_buffer::window_buffer(size_t size_x, size_t size_y, unsigned int x, unsigned int y) : current(newwin(size_y, size_x, y, x)), buffer(newwin(size_y, size_x, y, x))
{
}

window_buffer::~window_buffer()
{
    delwin(current);
    delwin(buffer);
}

WINDOW *window_buffer::get_buffer() const
{
    return buffer;
}

void window_buffer::flush()
{
    werase(current);
    wrefresh(buffer);

    WINDOW *tmp = current;
    current = buffer;
    buffer = tmp;
}

ui_rect::ui_rect( size_t size_x, size_t size_y, int x, int y ) : size_x( size_x ), size_y( size_y ), x( x ), y( y )
{
}

ui_window::ui_window( size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) : ui_element(size_x, size_y, x, y, anchor),
                      global_x(ui_element::anchored_x), global_y(ui_element::anchored_y), win(newwin(size_y, size_x, global_y, global_x))
{
}

ui_window::~ui_window()
{
    for( auto child : children ) {
        delete child;
    }

    delwin(win);
}

void ui_window::draw()
{
    werase(win);
    local_draw();
    draw_children();
    wrefresh(win);
    draw_window_children();
}

void ui_window::draw_children()
{
    for( auto &child : children ) {
        if( child->is_visible() && !child->is_window() ) {
            child->draw();
        }
    }
}

void ui_window::draw_window_children()
{
    for( auto &child : children ) {
        if( child->is_visible() && child->is_window() ) {
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

    delwin(win);
    win = newwin(rect.size_y, rect.size_x, global_y, global_x);
    //win = window_buffer(rect.size_x, rect.size_y, global_x, global_y);
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
            int char_health = current_health - (i * points_per_char);
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
    set_state(neutral);
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
    ui_element::set_rect(new_rect);
    num_tiles = new_rect.size_x * new_rect.size_y;
    delete[] tiles;
    tiles = new T[num_tiles];
}

template<class T>
tile_panel<T>::~tile_panel()
{
    delete[] tiles;
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
            tiles[y * get_rect().size_x + x].draw(win, x + get_ax(), y + get_ay());
        }
    }
}

template<class T>
void tile_panel<T>::set_tile(const T &tile, unsigned int x, unsigned int y)
{
    if(x >= get_rect().size_x || y >= get_rect().size_y) {
        return; // TODO: give feedback
    }

    int index = x * get_rect().size_y + y; // TODO: why does this need to be reversed?

    tiles[index] = tile; // Does this call T's copy constructor? or maybe of a derived type?
}

void ui_tile::draw( WINDOW *win, int x, int y ) const
{
    mvwputch(win, x, y, color, sym);
}

ui_tile::ui_tile(long sym, nc_color color) : sym(sym), color(color)
{
}

tabbed_window::tabbed_window(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) : bordered_window(size_x, size_y, x, y, anchor), tab_index(0)
{
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

    for (unsigned int i = 0; i < get_rect().size_x; i++) {
        mvwputch(win, 2, i, border_color, LINE_OXOX);
    }

    mvwputch(win, 2,  0, border_color, LINE_OXXO); // |^
    mvwputch(win, 2, get_rect().size_x - 1, border_color, LINE_OOXX); // ^|

    int x_offset = 1; // leave space for selection bracket
    for(unsigned int i = 0; i < tabs.size(); i++) {
        x_offset += draw_tab(win, x_offset, tabs[i].first, tab_index == i) + 2;
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
    delete[] uncovered;
}

void auto_bordered_window::set_rect(const ui_rect &new_rect)
{
    ui_window::set_rect(new_rect);
    delete[] uncovered;
    uncovered = new bool[new_rect.size_x * new_rect.size_y];
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

        unsigned int start_x = child->get_ax();
        unsigned int start_y = child->get_ay();

        unsigned int end_x = start_x + c_rect.size_x;
        unsigned int end_y = start_y + c_rect.size_y;

        for(size_t x = start_x; x < end_x; x++) {
            for(size_t y = start_y; y < end_y; y++) {
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
    bool up = is_uncovered(x, y - 1);
    bool right = is_uncovered(x + 1, y);
    bool down = is_uncovered(x, y + 1);

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

ui_vertical_list::ui_vertical_list(size_t size_x, size_t size_y, int x, int y, ui_anchor anchor) : ui_element(size_x, size_y, x, y, anchor)
{
}

void ui_vertical_list::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    unsigned int start_line = window_scroll;
    unsigned int end_line = start_line + get_rect().size_y;

    unsigned int available_space = get_rect().size_x - 2; // 2 for scroll bar and spacer

    for(unsigned int line = start_line; line < end_line && line < text.size(); line++) {
        auto txt = text[line];
        if(txt.size() > available_space) {
            txt = txt.substr(0, available_space);
        }
        if(scroll == line) {
            mvwprintz(win, get_ay() + line - start_line, get_ax() + 2, hilite(text_color), txt.c_str());
        } else {
            mvwprintz(win, get_ay() + line - start_line, get_ax() + 2, text_color, txt.c_str());
        }
    }

    draw_scrollbar(win, scroll, get_rect().size_y, text.size(), get_ay(), get_ax(), bar_color, false);
}

void ui_vertical_list::set_text(std::vector<std::string> text)
{
    this->text = text;
}

void ui_vertical_list::scroll_up()
{
    scroll = (scroll == 0 ? text.size() - 1 : scroll - 1);

    if(scroll == text.size() - 1) {
        window_scroll = scroll - get_rect().size_y + 1;
    } else if(scroll < window_scroll) {
        window_scroll--;
    }
}

void ui_vertical_list::scroll_down()
{
    scroll = (scroll == text.size() - 1 ? 0 : scroll + 1);

    if(scroll > get_rect().size_y + window_scroll - 1) {
        window_scroll++;
    } else if(scroll == 0) {
        window_scroll = 0;
    }
}

const std::string &ui_vertical_list::current() const
{
    return text[scroll];
}

ui_horizontal_list::ui_horizontal_list(size_t size_x, int x, int y, ui_anchor anchor) : ui_element(size_x, 1, x, y, anchor)
{
}

void ui_horizontal_list::draw()
{
    auto win = get_win();
    if(!win) {
        return;
    }

    unsigned int x_offset = 1 + get_ax();

    // TODO: truncate on overflow
    for(unsigned int i = 0; i < text.size(); i++) {
        x_offset += draw_subtab(win, get_ax() + x_offset, get_ay(), text[i], i == scroll) + 2;
    }
}

void ui_horizontal_list::set_text(std::vector<std::string> text)
{
    this->text = text;
}

void ui_horizontal_list::scroll_left()
{
    scroll = (scroll == 0 ? text.size() - 1 : scroll - 1);
}

void ui_horizontal_list::scroll_right()
{
    scroll = (scroll == text.size() - 1 ? 0 : scroll + 1);
}

const std::string &ui_horizontal_list::current() const
{
    return text[scroll];
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

void tab_test()
{
    tabbed_window win(31, 14, 50, 15);
    auto t_win1 = win.create_tab<ui_window>("tab 1");
    auto label1 = new ui_label("window 1", 0, 0, center_center);
    t_win1->add_child(label1);

    auto t_win2 = win.create_tab<ui_window>("tab 2");
    auto label2 = new ui_label("window 2", 0, 0, center_center);
    t_win2->add_child(label2);

    win.draw();

    input_context ctxt;
    ctxt.register_action("QUIT");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");

    while(true) {
        const std::string action = ctxt.handle_input();

        if(action == "PREV_TAB") {
            win.previous_tab();
        } else if(action == "NEXT_TAB") {
            win.next_tab();
        } else if(action == "QUIT") {
            break;
        }

        win.draw();
    }
}

void indicators_test()
{
    bordered_window win(31, 31, 50, 15);

    auto hb = new health_bar(5, 0, 0, center_center);
    win.add_child(hb);
    hb->set_health_percentage(0.5);

    auto si = new smiley_indicator(0, -1, center_center);
    win.add_child(si);

    win.draw();
}

void tile_panel_test()
{
    bordered_window win(31, 31, 50, 15);

    auto tp = new tile_panel<ui_tile>(29, 29, 1, 1);
    win.add_child(tp);

    tp->set_tile(ui_tile('X', c_yellow), 5, 5);
    tp->set_tile(ui_tile('X', c_yellow), 9, 5);
    tp->set_tile(ui_tile('X', c_yellow), 7, 6);

    win.draw();
}

void auto_border_test()
{
    auto_bordered_window win(51, 23, 50, 15);

    win.add_child(new bordered_window(49, 10, 1, 1));
    win.add_child(new bordered_window(49, 10, -1, -1, bottom_right));

    win.draw();
}

void list_test()
{
    std::vector<std::string> text {
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong",
        "1",
        "2",
        "3",
        "loooooooooooong"
    };

    bordered_window win(32, 34, 50, 15);

    auto t_list = new ui_vertical_list(7, 15, 0, 1);
    t_list->set_text(text);

    win.add_child(t_list);

    win.draw();

    input_context ctxt;
    ctxt.register_action("QUIT");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");

    while(true) {
        const std::string action = ctxt.handle_input();

        if(action == "UP") {
            t_list->scroll_up();
        } else if(action == "DOWN") {
            t_list->scroll_down();
        } else if(action == "QUIT") {
            break;
        }

        win.draw();
    }
}

void list_test2()
{
    std::vector<std::string> text {
        "1",
        "2",
        "3"
    };

    bordered_window win(51, 34, 50, 15);

    auto t_list = new ui_horizontal_list(30, 1, 1);
    t_list->set_text(text);

    win.add_child(t_list);

    win.draw();

    input_context ctxt;
    ctxt.register_action("QUIT");
    ctxt.register_action("LEFT");
    ctxt.register_action("RIGHT");

    while(true) {
        const std::string action = ctxt.handle_input();

        if(action == "LEFT") {
            t_list->scroll_left();
        } else if(action == "RIGHT") {
            t_list->scroll_right();
        } else if(action == "QUIT") {
            break;
        }

        win.draw();
    }
}

void ui_test_func()
{
    std::vector<std::string> options {
        "labels and anchors",
        "tabs",
        "indicators",
        "tile_panel",
        "auto borders",
        "v list",
        "h list"
    };

    int selection = menu_vec(false, "Select a sample", options);

    switch(selection) {
        case 1:
            label_test();
            break;
        case 2:
            tab_test();
            break;
        case 3:
            indicators_test();
            break;
        case 4:
            tile_panel_test();
            break;
        case 5:
            auto_border_test();
            break;
        case 6:
            list_test();
            break;
        case 7:
            list_test2();
            break;
        default:
            break;
    }
    //tab_test();
}
