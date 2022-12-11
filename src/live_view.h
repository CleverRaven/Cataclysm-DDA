#pragma once
#ifndef CATA_SRC_LIVE_VIEW_H
#define CATA_SRC_LIVE_VIEW_H

#include <memory>

#include "cursesdef.h"
#include "point.h"

class ui_adaptor;

class live_view
{
    public:
        live_view();
        ~live_view();

        void init();
        void show( const tripoint &p );
        bool is_enabled();
        void hide();

    private:
        tripoint mouse_position;

        catacurses::window win;
        std::unique_ptr<ui_adaptor> ui;
};

#endif // CATA_SRC_LIVE_VIEW_H
