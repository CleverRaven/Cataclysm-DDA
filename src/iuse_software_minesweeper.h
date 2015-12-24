#ifndef SOFTWARE_MINESWEEPER_H
#define SOFTWARE_MINESWEEPER_H

#include "cursesdef.h"

#include <string>
#include <map>

class minesweeper_game
{
    private:
        bool check_win();
        void new_level( WINDOW *w_minesweeper );
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
