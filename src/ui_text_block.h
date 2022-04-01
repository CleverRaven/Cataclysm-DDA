#pragma once
#ifndef CATA_SRC_UI_TEXT_BLOCK_H
#define CATA_SRC_UI_TEXT_BLOCK_H

#include "color.h"
#include "ui_element.h"

namespace catacurses
{
class window;
}// namespace catacurses

class text_block : public ui_element
{
    public:
        text_block( std::string text, nc_color color ) : text( text ), color( color ) {};
        text_block( std::string text, nc_color color, point pos, int width, int height ) :
            ui_element( pos, width, height ), text( text ), color( color ) {};

        std::string text;
        nc_color color;

        void draw( const catacurses::window &w, const point offset ) const override;

        bool multiline = false;
};

#endif // CATA_SRC_UI_TEXT_BLOCK_H
