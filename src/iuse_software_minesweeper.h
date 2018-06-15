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
        int iMaxX = 0;
        int iMaxY = 0;
        int iMinX = 0;
        int iMinY = 0;
        int iLevelX = 0;
        int iLevelY = 0;
        int iOffsetX = 0;
        int iOffsetY = 0;
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

#endif
