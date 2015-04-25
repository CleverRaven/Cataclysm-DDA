#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "output.h" //WINDOW_PTR

class live_view
{
public:
    live_view() = default;

    void init(int start_x, int start_y, int width, int height);
    void show(int x, int y);
    bool hide(bool refresh = true, bool force = false);
    bool is_compact() const;
    void set_compact(bool value);
private:
    WINDOW_PTR w_live_view;

    int width       = 0;
    int height      = 0;
    int last_height = -1;

    bool inuse        = false;
    bool enabled      = false;
    bool compact_view = false;

    operator WINDOW*() const {
        return w_live_view.get();
    }
};

#endif
