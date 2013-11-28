#include "live_view.h"
#include "output.h"
#include "game.h"
#include "options.h"

#define START_LINE 1
#define START_COLUMN 1

live_view::live_view() : compact_view(false), w_live_view(NULL), 
    enabled(false), inuse(false), last_height(-1)
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

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    // Because of the way the status UI is done, the live view window must
    // be tall enough to clear the entire height of the viewport below the
    // status bar. This hack allows the border around the live view box to
    // be drawn only as big as it needs to be, while still leaving the
    // window tall enough. Won't work for ncurses in Linux, but that doesn't
    // currently support the mouse. If and when it does, there'll need to
    // be a different code path here that works for ncurses.
    int full_height = w_live_view->height;
    if (line < w_live_view->height - 1) {
        w_live_view->height = (line > 11) ? line : 11;
    }
    last_height = w_live_view->height;
#endif

    draw_border(w_live_view);

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    w_live_view->height = full_height;
#endif

    inuse = true;
    wrefresh(w_live_view);
}

bool live_view::hide(bool refresh /*= true*/, bool force /*= false*/)
{
    if (!enabled || (!inuse && !force)) {
        return false;
    }

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    int full_height = w_live_view->height;
    if (use_narrow_sidebar() && last_height > 0) {
        // When using the narrow sidebar mode, the lower part of the screen
        // is used for the message queue. Best not to obscure too much of it.
        w_live_view->height = last_height;
    }
#endif

    werase(w_live_view);

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    w_live_view->height = full_height;
#endif

    inuse = false;
    last_height = -1;
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
