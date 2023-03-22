#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_SNAKE_H
#define CATA_SRC_IUSE_SOFTWARE_SNAKE_H


#include "input.h"

namespace catacurses
{
class window;
} // namespace catacurses

class snake_game
{
    public:
        snake_game();
        void print_score( const catacurses::window &w_snake, int iScore );
        void print_header( const catacurses::window &w_snake, const input_context &ctxt,
                           bool show_shortcut = true );
        void snake_over( const catacurses::window &w_snake, int iScore, const input_context &ctxt );
        int start_game();
};

#endif // CATA_SRC_IUSE_SOFTWARE_SNAKE_H
