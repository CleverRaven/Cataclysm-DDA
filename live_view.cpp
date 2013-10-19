#include "live_view.h"
#include "output.h"

live_view::live_view() : w_live_view(NULL), enabled(false)
{

}

live_view::~live_view()
{
    delwin(w_live_view);
}

void live_view::init(int start_x, int start_y, int width, int height)
{
    enabled = true;
    if (w_live_view != NULL) {
        delwin(w_live_view);
    }

    w_live_view = newwin(height, width, start_y, start_x);
    hide();
}

void live_view::show()
{
    if (!enabled || w_live_view == NULL) {
        return;
    }

    hide(false); // Clear window if it's visible

    wborder(w_live_view, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
        LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    mvwprintz(w_live_view, 0, 2, c_white, "< ");
    wprintz(w_live_view, c_green, _("Items Under Mouse"));
    wprintz(w_live_view, c_white, " >");
    w_live_view->inuse = true;
    wrefresh(w_live_view);
}

void live_view::hide(bool refresh /*= true*/, bool force /*= false*/)
{
    if (!enabled || (!w_live_view->inuse && !force)) {
        return;
    }

    werase(w_live_view);
    w_live_view->inuse = false;
    if (refresh) {
        wrefresh(w_live_view);
    }
}
