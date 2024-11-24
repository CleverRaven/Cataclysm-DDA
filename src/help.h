#pragma once
#ifndef CATA_SRC_HELP_H
#define CATA_SRC_HELP_H

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cuboid_rectangle.h"

class JsonArray;
class JsonObject;
class translation;
namespace catacurses
{
class window;
}  // namespace catacurses

class help
{
    public:
        void load_from_file();
        void load( const JsonObject &jo, const std::string &src );
        void clear_modded_help();
        void display_help() const;
        // Run at the start of every mod to set the starting point for order
        void set_current_order_start();

    private:
        void deserialize( const JsonArray &ja );
        std::map<int, inclusive_rectangle<point>> draw_menu( const catacurses::window &win,
                                               int selected ) const;
        static std::string get_note_colors();
        static std::string get_dir_grid();
        // Modifier for each mods order
        int current_order_start = 0;
        // The last order key defined by the help file
        int file_order_end = 0;
        std::map<int, std::pair<translation, std::vector<translation>>> help_texts;
};

help &get_help();

std::string get_hint();

#endif // CATA_SRC_HELP_H
