#pragma once
#ifndef CATA_SRC_IUSE_SOFTWARE_MINESWEEPER_H
#define CATA_SRC_IUSE_SOFTWARE_MINESWEEPER_H

#include <map>

#include "point.h"

namespace catacurses
{
class window;
} // namespace catacurses

class minesweeper_game
{
    private:
        bool check_win();
        void new_level( const catacurses::window &w_minesweeper );
        point max;
        point min;
        point level;
        point offset;
        int iBombs = 0;

        std::map<int, std::map<int, int> > mLevel;

        enum reveal {
            bomb = -1,
            unknown,
            flag,
            seen
        };

        std::map<int, std::map<int, reveal> > mLevelReveal;

    public:
        int start_game();
        minesweeper_game();
};

#endif // CATA_SRC_IUSE_SOFTWARE_MINESWEEPER_H
