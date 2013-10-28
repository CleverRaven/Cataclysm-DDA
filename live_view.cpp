#include "live_view.h"
#include "output.h"
#include "game.h"

#define START_LINE 1
#define START_COLUMN 1

live_view::live_view() : w_live_view(NULL), enabled(false), compact_view(false)
{

}

live_view::~live_view()
{
    delwin(w_live_view);
}

void live_view::init(game *g, int start_x, int start_y, int width, int height)
{
    enabled = true;
    if (w_live_view != NULL) {
        delwin(w_live_view);
    }

    this->width = width;
    this->height = height;
    this->g = g;
    w_live_view = newwin(height, width, start_y, start_x);
    hide();
}

void live_view::show(const int x, const int y)
{
    if (!enabled || w_live_view == NULL) {
        return;
    }

    bool did_hide = hide(false); // Clear window if it's visible

    if (!g->u_see(x,y)) {
        if (did_hide) {
            wrefresh(w_live_view);
        }
        return;
    }

    map &m = g->m;
    mvwprintz(w_live_view, 0, START_COLUMN, c_white, "< ");
    wprintz(w_live_view, c_green, _("Mouse View"));
    wprintz(w_live_view, c_white, " >");
    int line = START_LINE;

    g->print_all_tile_info(x, y, w_live_view, START_COLUMN, line, true);

    if (m.can_put_items(x, y)) {
        std::vector<item> &items = m.i_at(x, y);
        print_items(items, line);
    }

    int full_height = w_live_view->height;
    if (line < w_live_view->height - 1) {
        w_live_view->height = (line > 11) ? line : 11;
    }
    wborder(w_live_view, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
        LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    w_live_view->height = full_height;

    w_live_view->inuse = true;
    wrefresh(w_live_view);
}

bool live_view::hide(bool refresh /*= true*/, bool force /*= false*/)
{
    if (!enabled || (!w_live_view->inuse && !force)) {
        return false;
    }

    werase(w_live_view);
    w_live_view->inuse = false;
    if (refresh) {
        wrefresh(w_live_view);
    }

    return true;
}

void live_view::print_items(std::vector<item> &items, int &line) const
{
    std::map<std::string, int> item_names;
    for (int i = 0; i < items.size(); i++) {
        std::string name = items[i].tname(g);
        if (item_names.find(name) == item_names.end()) {
            item_names[name] = 0;
        }
        item_names[name] += 1;
    }

    int last_line = height - START_LINE - 1;
    bool will_overflow = line-1 + item_names.size() > last_line;

    for (std::map<std::string, int>::iterator it = item_names.begin(); 
         it != item_names.end() && (!will_overflow || line < last_line); it++) {
        mvwprintz(w_live_view, line++, START_COLUMN, c_white, it->first.c_str());
        if (it->second > 1) {
            wprintz(w_live_view, c_white, _(" [%d]"), it->second);
        }
    }

    if (will_overflow) {
        mvwprintz(w_live_view, line++, START_COLUMN, c_yellow, _("More items here..."));
    }
}
