#ifndef _SOFTWARE_SOKOBAN_H_
#define _SOFTWARE_SOKOBAN_H_

#include <string>
#include <vector>
#include <map>

class sokoban_game
{
    private:
        class cUndo
        {
            public:
                int iOldY;
                int iOldX;
                std::string sTileOld;

                cUndo() {
                    iOldY = 0;
                    iOldX = 0;

                    sTileOld = " ";
                }

                cUndo(const int arg_y, const int arg_x, const std::string arg_tile) {
                    iOldY = arg_y;
                    iOldX = arg_x;
                    sTileOld = arg_tile;
                }

                ~cUndo() {};
        };

        int iCurrentLevel, iNumLevel, iTotalMoves;
        std::map<int, std::map<int, std::string> > mLevel;
        std::map<int, std::map<std::string, int> > mLevelInfo;
        std::vector<std::map<int, std::map<int, std::string> > > vLevel;
        std::vector<cUndo> vUndo;
        std::vector<std::vector<std::pair<int, int> > > vLevelDone;
        std::map<int, bool> mAlreadyWon;

        bool parse_level();
        bool check_win();
        int get_wall_connection(const int iY, const int iX);
        void draw_level(WINDOW *w_sokoban);
        void clear_level(WINDOW *w_sokoban);
        void print_score(WINDOW *w_sokoban, int iScore, int iMoves);
    public:
        int start_game();
        sokoban_game();
};
#endif
