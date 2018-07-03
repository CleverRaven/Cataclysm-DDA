#pragma once
#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "enums.h"     // tripoint

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
        void refresh();
        void show( const tripoint &mouse_position );
        void hide();

    private:
        tripoint mouse_position;

        bool enabled = false;
};

#endif
