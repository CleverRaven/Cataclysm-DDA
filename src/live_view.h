#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "output.h" //WINDOW_PTR
#include "enums.h"

class live_view
{
    public:
        live_view() = default;

        void init( int start_x, int start_y, int width, int height );
        void draw();
        void show( const tripoint &mouse_position );
        void hide();
    private:
        WINDOW_PTR w_live_view;
        tripoint mouse_position;

        int width       = 0;
        int height      = 0;
        bool enabled    = false;

        operator WINDOW *() const {
            return w_live_view.get();
        }
};

#endif
