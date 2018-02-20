#pragma once
#ifndef SOFTWARE_MINESWEEPER_H
#define SOFTWARE_MINESWEEPER_H

#include <map>

namespace catacurses
{
class window;
} // namespace catacurses

class minesweeper_game
{
    private:
        bool check_win();
        void new_level( const catacurses::window &w_minesweeper );
        int iMaxX, iMaxY, iMinX, iMinY;
        int iLevelX, iLevelY;
        int iOffsetX, iOffsetY;
        int iBombs;

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

#endif
