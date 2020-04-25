#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H
#define CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H

#include <vector>
#include <utility>

#include "cursesdef.h"

class lightson_game
{
    private:
        catacurses::window w_border;
        catacurses::window w;
        // rows, columns
        std::pair< int, int > level_size;
        std::vector< bool > level;
        std::vector< std::pair< int, int > > change_coords;
        // row, column
        std::pair< int, int > position;
        bool win;

        void new_level();
        void reset_level();
        void generate_change_coords( int changes );
        void draw_level();
        bool check_win();
        void toggle_lights();

    public:
        int start_game();
        lightson_game() = default;
};

#endif // CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H
