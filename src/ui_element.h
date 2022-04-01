#pragma once
#ifndef CATA_SRC_UI_ELEMENT_H
#define CATA_SRC_UI_ELEMENT_H

#include <memory>

#include "cuboid_rectangle.h"

namespace catacurses
{
class window;
}// namespace catacurses

class ui_element
{
    public:
        ui_element() = default;
        ui_element( const ui_element & ) = default;
        ui_element( point pos, int width, int height ) : pos( pos ), width( width ), height( height ) {};
        virtual ~ui_element() = default;
        void draw( const catacurses::window &w, const point offset ) {
            static_cast<const ui_element *>( this )->draw( w, offset );
        }
        virtual void draw( const catacurses::window &w, const point offset ) const = 0;

        point pos;
        int width;
        int height;
};

#endif // CATA_SRC_UI_ELEMENT_H
