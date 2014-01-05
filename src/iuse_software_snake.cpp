#include <string>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#include "keypress.h"
#include "output.h"
#include "catacharset.h"
#include "options.h"
#include "debug.h"
#include "iuse_software_snake.h"

snake_game::snake_game()
{
}

void snake_game::print_score(WINDOW *w_snake, int iScore)
{
    std::stringstream ssTemp;
    ssTemp << "Score: " << iScore;

    mvwprintz(w_snake, 0, 5, c_white, ssTemp.str().c_str());
}

int snake_game::start_game()
{
    std::vector<std::pair<int, int> > vSnakeBody;
    std::map<int, std::map<int, bool> > mSnakeBody;

    int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    WINDOW* w_snake = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    draw_border(w_snake);
    mvwprintz(w_snake, 0, FULL_SCREEN_WIDTH-10, c_ltgreen, "q");
    wprintz(w_snake, c_white, "uit");
    mvwprintz(w_snake, 0, (FULL_SCREEN_WIDTH/2)-5, c_white, "S N A K E");

    //Snake start position
    vSnakeBody.push_back(std::make_pair(FULL_SCREEN_HEIGHT/2, FULL_SCREEN_WIDTH/2));
    mSnakeBody[FULL_SCREEN_HEIGHT/2][FULL_SCREEN_WIDTH/2] = true;
    mvwputch(w_snake, vSnakeBody[vSnakeBody.size()-1].first, vSnakeBody.back().second, c_white, '#');

    //Snake start direction
    int iDirY = 0;
    int iDirX = 1;

    //Snake start length
    int iSnakeBody = 10;

    //Gamespeed aka inputdelay/timeout
    int iGamespeed = 100;

    //Score
    int iScore = 0;
    int iFruitPosY = 0;
    int iFruitPosX = 0;

    //Draw Score
    print_score(w_snake, iScore);

    int input = '.';

    do {
        //Check if we hit a border
        if (vSnakeBody[vSnakeBody.size()-1].first + iDirY == 0) {
            vSnakeBody.push_back(std::make_pair(vSnakeBody[vSnakeBody.size()-1].first + iDirY + FULL_SCREEN_HEIGHT - 2, vSnakeBody[vSnakeBody.size()-1].second + iDirX));

        } else if (vSnakeBody[vSnakeBody.size()-1].first + iDirY == FULL_SCREEN_HEIGHT - 1) {
            vSnakeBody.push_back(std::make_pair(vSnakeBody[vSnakeBody.size()-1].first + iDirY - FULL_SCREEN_HEIGHT + 2, vSnakeBody[vSnakeBody.size()-1].second + iDirX));

        } else if (vSnakeBody[vSnakeBody.size()-1].second + iDirX == 0) {
            vSnakeBody.push_back(std::make_pair(vSnakeBody[vSnakeBody.size()-1].first + iDirY, vSnakeBody[vSnakeBody.size()-1].second + iDirX + FULL_SCREEN_WIDTH - 2));

        } else if (vSnakeBody[vSnakeBody.size()-1].second + iDirX == FULL_SCREEN_WIDTH - 1) {
            vSnakeBody.push_back(std::make_pair(vSnakeBody[vSnakeBody.size()-1].first + iDirY, vSnakeBody[vSnakeBody.size()-1].second + iDirX - FULL_SCREEN_WIDTH + 2));

        } else {
            vSnakeBody.push_back(std::make_pair(vSnakeBody[vSnakeBody.size()-1].first + iDirY, vSnakeBody[vSnakeBody.size()-1].second + iDirX));
        }

        //Check if we hit ourself
        if (mSnakeBody[vSnakeBody[vSnakeBody.size()-1].first][vSnakeBody[vSnakeBody.size()-1].second]) {
            //We are dead :(
            return iScore;
        } else {
            //Add new position to map
            mSnakeBody[vSnakeBody[vSnakeBody.size()-1].first][vSnakeBody[vSnakeBody.size()-1].second] = true;
        }

        //Have we eaten the forbidden fruit?
        if (vSnakeBody[vSnakeBody.size()-1].first == iFruitPosY && vSnakeBody[vSnakeBody.size()-1].second == iFruitPosX) {
            iScore += 500;
            iSnakeBody += 10;
            iGamespeed -= 3;

            print_score(w_snake, iScore);

            iFruitPosY = 0;
            iFruitPosX = 0;
        }

        //Check if we are longer than our max size
        if (vSnakeBody.size() > iSnakeBody) {
            mSnakeBody[vSnakeBody[0].first][vSnakeBody[0].second] = false;
            mvwputch(w_snake, vSnakeBody[0].first, vSnakeBody[0].second, c_black, ' ');
            vSnakeBody.erase(vSnakeBody.begin(), vSnakeBody.begin()+1);
        }

        //Draw Snake
        mvwputch(w_snake, vSnakeBody[vSnakeBody.size()-1].first, vSnakeBody[vSnakeBody.size()-1].second, c_white, '#');
        mvwputch(w_snake, vSnakeBody[vSnakeBody.size()-2].first, vSnakeBody[vSnakeBody.size()-2].second, c_ltgray, '#');

        //On full length add a fruit
        if (iFruitPosY == 0 && iFruitPosY == 0) {
            do {
                iFruitPosY = rng(1, FULL_SCREEN_HEIGHT - 2);
                iFruitPosX = rng(1, FULL_SCREEN_WIDTH - 2);
            } while (mSnakeBody[iFruitPosY][iFruitPosX]);

            mvwputch(w_snake, iFruitPosY, iFruitPosX, c_ltred, '*');
        }

        wrefresh(w_snake);

        //Check input
        timeout(iGamespeed);
        input = getch();
        timeout(-1);

        switch (input) {
            case KEY_UP: /* up */
                if (iDirY != 1) {
                    iDirY = -1;
                    iDirX = 0;
                }
                break;
            case KEY_DOWN: /* down */
                if (iDirY != -1) {
                    iDirY = 1;
                    iDirX = 0;
                }
                break;
            case KEY_LEFT: /* left */
                if (iDirX != 1) {
                    iDirY = 0;
                    iDirX = -1;
                }
                break;
            case KEY_RIGHT: /* right */
                if (iDirX != -1) {
                    iDirY = 0;
                    iDirX = 1;
                }
                break;
            case 'q':
                return iScore;
                break;
            default:
                break;
        }

    } while (true);

    return iScore;
}
