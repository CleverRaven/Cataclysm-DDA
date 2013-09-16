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
#include "game.h"
#include "output.h"
#include "catacharset.h"
#include "crafting.h"
#include "options.h"
#include "debug.h"
#include "iuse.h"
#include "iuse_software.h"
#include "iuse_software_kitten.h"
#include "iuse_software_snake.h"
#include "iuse_software_sokoban.h"
#include "translations.h"

bool play_videogame(std::string function_name, std::map<std::string, std::string> & game_data, int &score)
{
    if ( function_name == "" ) {
        score = 15;
        return true; // generic game
    }
    if ( function_name == "robot_finds_kitten" ) {
        WINDOW *bkatwin = newwin(22, 62, (TERMY - 22) / 2, (TERMX - 62) / 2);
        wborder(bkatwin, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
        wrefresh(bkatwin);
        WINDOW *katwin = newwin(20, 60, (TERMY - 20) / 2, (TERMX - 60) / 2);
        robot_finds_kitten findkitten(katwin);
        bool foundkitten = findkitten.ret;
        werase(katwin);
        delwin(katwin);
        werase(bkatwin);
        delwin(bkatwin);
        if (foundkitten == true) {
            game_data["end_message"] = _("You found kitten!");
            game_data["moraletype"] = "MORALE_GAME_FOUND_KITTEN";
            score = 30;
        }

        return foundkitten;

        return true;
    /* } else if( function_name == "tetris" ) { */
    } else if ( function_name == "snake_game" ) {
        snake_game sg;
        int iScore = sg.start_game();

        if (iScore >= 10000) {
            score = 50;
        } else if (iScore >= 5000) {
            score = 30;
        } else {
            score = 20;
        }

        return true;
    } else if ( function_name == "sokoban_game" ) {
        sokoban_game sg;
        int iScore = sg.start_game();

        if (iScore >= 5000) {
            score = 50;
        } else if (iScore >= 1000) {
            score = 30;
        } else {
            score = 20;
        }

        return true;
    } else {
        score = -5;
        /* morale/activity workaround >.> */
        game_data["end_message"] = string_format(_("You struggle to get '%s' working, and finally give up to play minesweeper."), function_name.c_str() );
        // todo: better messages in morale system //  game_data["moraletype"]="MORALE_GAME_SOFTWARE_PROBLEM";
        return false;
    }
}
