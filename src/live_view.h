#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "output.h" //WINDOW_PTR
#include "enums.h"

class live_view
{
    public:
        live_view() = default;

        void init();
        int draw( WINDOW *win, int max_height );
        void refresh();
        void show( const tripoint &mouse_position );
        void hide();

    private:
        tripoint mouse_position;

        bool enabled = false;
};

#endif
