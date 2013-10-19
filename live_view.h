#ifndef _LIVE_VIEW_H_
#define _LIVE_VIEW_H_

#include "catacurse.h"

class live_view
{
public:
    live_view();
    ~live_view();

    void init(int start_x, int start_y, int width, int height);
    void show();
    void hide(bool refresh = true, bool force = false);

private:
    WINDOW *w_live_view;
    int start_x, start_y;
    bool enabled;
};
#endif