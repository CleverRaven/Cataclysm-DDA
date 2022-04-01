#pragma once
#ifndef CATA_SRC_UI_LAYOUT_H
#define CATA_SRC_UI_LAYOUT_H

#include "memory_fast.h"
#include "ui_element.h"

struct point;
namespace catacurses
{
class window;
}// namespace catacurses

class absolute_layout : public ui_element
{
    public:
        absolute_layout() = default;
        absolute_layout( point pos, int width, int height ) : ui_element( pos, width, height ) {};

        void draw( const catacurses::window &w, const point offset ) const override;
        void add_element( shared_ptr_fast<ui_element> &element );
        void remove_element( shared_ptr_fast<ui_element> &element );
    private:
        std::vector<shared_ptr_fast<ui_element>> layout_elements;
};

#endif // CATA_SRC_UI_LAYOUT_H
