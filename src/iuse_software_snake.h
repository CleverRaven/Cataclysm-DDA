#pragma once
#ifndef SOFTWARE_SNAKE_H
#define SOFTWARE_SNAKE_H

namespace catacurses
{
class window;
} // namespace catacurses

class snake_game
{
    public:
        snake_game();
        void print_score( const catacurses::window &w_snake, int iScore );
        void print_header( const catacurses::window &w_snake, bool show_shortcut = true );
        void snake_over( const catacurses::window &w_snake, int iScore );
        int start_game();
};

#endif
