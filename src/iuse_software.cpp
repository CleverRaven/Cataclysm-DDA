#include "iuse_software.h"

#include <map>
#include <string>

#include "cursesdef.h"
#include "iuse_software_kitten.h"
#include "iuse_software_lightson.h"
#include "iuse_software_minesweeper.h"
#include "iuse_software_snake.h"
#include "iuse_software_sokoban.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"

bool play_videogame( const std::string &function_name,
                     std::map<std::string, std::string> &game_data,
                     int &score )
{
    if( function_name.empty() ) {
        score = 15;
        return true; // generic game
    }
    if( function_name == "robot_finds_kitten" ) {
        catacurses::window bkatwin = catacurses::newwin( 22, 62, ( TERMY - 22 ) / 2, ( TERMX - 62 ) / 2 );
        draw_border( bkatwin );
        wrefresh( bkatwin );
        catacurses::window katwin = catacurses::newwin( 20, 60, ( TERMY - 20 ) / 2, ( TERMX - 60 ) / 2 );
        robot_finds_kitten findkitten( katwin );
        bool foundkitten = findkitten.ret;
        if( foundkitten ) {
            game_data["end_message"] = _( "You found kitten!" );
            game_data["moraletype"] = "MORALE_GAME_FOUND_KITTEN";
            score = 30;
        }

        return foundkitten;
    } else if( function_name == "snake_game" ) {
        snake_game sg;
        int iScore = sg.start_game();

        if( iScore >= 10000 ) {
            score = 30;
        } else if( iScore >= 5000 ) {
            score = 15;
        } else {
            score = 5;
        }

        return true;
    } else if( function_name == "sokoban_game" ) {
        sokoban_game sg;
        int iScore = sg.start_game();

        if( iScore >= 5000 ) {
            score = 30;
        } else if( iScore >= 1000 ) {
            score = 15;
        } else {
            score = 5;
        }

        return true;
    } else if( function_name == "minesweeper_game" ) {
        minesweeper_game mg;
        score = mg.start_game();

        return true;
    } else if( function_name == "lightson_game" ) {
        lightson_game lg;
        int iScore = lg.start_game();
        score = std::min( 15, iScore * 3 );

        return true;
    } else {
        score = -5;
        /* morale/activity workaround >.> */
        game_data["end_message"] = string_format(
                                       _( "You struggle to get '%s' working, and finally give up to play minesweeper." ),
                                       function_name.c_str() );
        // @todo: better messages in morale system //  game_data["moraletype"]="MORALE_GAME_SOFTWARE_PROBLEM";
        return false;
    }
}
