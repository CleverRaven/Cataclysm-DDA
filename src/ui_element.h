#pragma once
#ifndef CATA_SRC_UI_ELEMENT_H
#define CATA_SRC_UI_ELEMENT_H

#include <string>

struct point;
namespace catacurses
{
class window;
}// namespace catacurses

class ui_element
{
    public:
        virtual ~ui_element() = default;
        virtual void draw( const catacurses::window &w, const point &p, const int width, const int height,
                           const bool selected ) = 0;
        virtual std::string get_category() {
            return std::string();
        };
};

#endif // CATA_SRC_UI_ELEMENT_H
