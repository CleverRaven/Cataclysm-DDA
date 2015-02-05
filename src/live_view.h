#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "map.h"
#include "cursesdef.h"

class item;

class live_view
{
    public:
        live_view();
        ~live_view();

        void init(int start_x, int start_y, int width, int height);
        void show(const int x, const int y);
        bool hide(bool refresh = true, bool force = false);

        bool compact_view;

    private:
        WINDOW *w_live_view;
        int width, height;
        bool enabled;
        int inuse;
        int last_height;

        void print_items( const map_stack &items, int &line ) const;
};

#endif
