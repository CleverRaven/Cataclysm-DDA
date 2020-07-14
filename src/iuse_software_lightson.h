#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H
#define CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H

#include <vector>

#include "cursesdef.h"
#include "point.h"

class lightson_game
{
    private:
        catacurses::window w_border;
        catacurses::window w;
        // rows, columns
        point level_size;
        std::vector<bool> level;
        std::vector<point> change_coords;
        // row, column
        point position;
        bool win;

        void new_level();
        void reset_level();
        void generate_change_coords( int changes );
        void draw_level();
        bool check_win();
        void toggle_lights();
        void toggle_lights_at( const point &pt );
        bool get_value_at( const point &pt );
        void set_value_at( const point &pt, bool value );
        void toggle_value_at( const point &pt );

    public:
        int start_game();
        lightson_game() = default;
};

#endif // CATA_SRC_IUSE_SOFTWARE_LIGHTSON_H
