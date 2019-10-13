#pragma once
#ifndef SOFTWARE_SOKOBAN_H
#define SOFTWARE_SOKOBAN_H

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace catacurses
{
class window;
} // namespace catacurses

class sokoban_game
{
    private:
        class cUndo
        {
            public:
                int iOldY = 0;
                int iOldX = 0;
                std::string sTileOld;

                cUndo() : iOldY( 0 ), iOldX( 0 ), sTileOld( " " ) {}

                cUndo( const int &arg_y, const int &arg_x, const std::string &arg_tile ) : iOldY( arg_y ),
                    iOldX( arg_x ), sTileOld( arg_tile ) {}
        };

        int iCurrentLevel = 0;
        int iNumLevel = 0;
        int iTotalMoves = 0;
        std::map<int, std::map<int, std::string> > mLevel;
        std::map<int, std::map<std::string, size_t> > mLevelInfo;
        std::vector<std::map<int, std::map<int, std::string> > > vLevel;
        std::vector<cUndo> vUndo;
        std::vector<std::vector<std::pair<int, int> > > vLevelDone;
        std::map<int, bool> mAlreadyWon;

        void parse_level( std::istream &fin );
        bool check_win();
        int get_wall_connection( int iY, int iX );
        void draw_level( const catacurses::window &w_sokoban );
        void clear_level( const catacurses::window &w_sokoban );
        void print_score( const catacurses::window &w_sokoban, int iScore, int iMoves );
    public:
        int start_game();
        sokoban_game();
};

#endif
