#pragma once
#ifndef CATA_SRC_LIVE_VIEW_H
#define CATA_SRC_LIVE_VIEW_H

#include "point.h"

namespace catacurses
{
class window;
} // namespace catacurses

class live_view
{
    public:
        live_view() = default;

        void init();
        int draw( const catacurses::window &win, int max_height );
        void show( const tripoint &p );
        bool is_enabled();
        void hide();

    private:
        tripoint mouse_position;

        bool enabled = false;
};

#endif // CATA_SRC_LIVE_VIEW_H
