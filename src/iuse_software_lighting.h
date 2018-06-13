#pragma once
#ifndef SOFTWARE_LIGHTING_H
#define SOFTWARE_LIGHTING_H

#include <vector>

namespace catacurses
{
class window;
} // namespace catacurses

class lighting_game
{
    private:
        void new_level( const catacurses::window &w_lighting );
        int iLevelX, iLevelY;

        std::vector< std::vector<int> > mLevel;

    public:
        int start_game();
        lighting_game();
};

#endif
