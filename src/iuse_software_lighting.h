#pragma once
#ifndef SOFTWARE_LIGHTING_H
#define SOFTWARE_LIGHTING_H

#include "lighting.h"

#include <array>

namespace catacurses
{
class window;
} // namespace catacurses

class lighting_game
{
    private:
        void new_level( const catacurses::window &w_lighting );
        int iLevelX, iLevelY;

        std::array< std::array<int, N_LIGHTING>, N_LIGHTING> level;

    public:
        int start_game();
        lighting_game();
};

#endif
