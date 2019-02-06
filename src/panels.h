#pragma once
#ifndef PANELS_H
#define PANELS_H
#include "color.h"
#include "game.h"
#include "input.h"
#include "string_id.h"
#include <string>

class player;
namespace catacurses
{
class window;
} // namespace catacurses
enum face_type : int {
    face_human = 0,
    face_bird,
    face_bear,
    face_cat,
    num_face_types
};

class window_panel
{
    public:
        window_panel( std::function<void( player &, const catacurses::window & )> draw_func, std::string nm,
                      int ht, int wd, bool default_toggle );

        std::function<void( player &, const catacurses::window & )> draw;
        int get_height() const;
        int get_width() const;
        std::string get_name() const;
        bool toggle;
    private:
        int height;
        int width;
        bool default_toggle;
        std::string name;
};

std::map<std::string, std::vector<window_panel>> initialize_panel_layouts();

void draw_panel_adm( const catacurses::window &w, size_t column = 0, size_t index = 1 );

#endif
