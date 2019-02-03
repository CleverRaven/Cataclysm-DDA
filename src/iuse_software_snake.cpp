#include "iuse_software_snake.h"

#include <map>
#include <string>
#include <vector>

#include "catacharset.h"  // utf8_width()
#include "cursesdef.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

snake_game::snake_game() = default;

void snake_game::print_score( const catacurses::window &w_snake, int iScore )
{
    mvwprintz( w_snake, 0, 5, c_white, string_format( _( "Score: %d" ), iScore ).c_str() );
}

void snake_game::print_header( const catacurses::window &w_snake, bool show_shortcut )
{
    draw_border( w_snake, BORDER_COLOR, _( "S N A K E" ), c_white );
    if( show_shortcut ) {
        std::string shortcut = _( "<q>uit" );
        shortcut_print( w_snake, 0, FULL_SCREEN_WIDTH - utf8_width( shortcut ) - 2,
                        c_white, c_light_green, shortcut );
    }
}

void snake_game::snake_over( const catacurses::window &w_snake, int iScore )
{
    werase( w_snake );
    print_header( w_snake, false );

    // Body of dead snake
    size_t body_length = 3;
    for( size_t i = 1; i <= body_length; i++ ) {
        for( size_t j = 0; j <= 1; j++ ) {
            mvwprintz( w_snake, i, 4 + j * 65, c_green, "|   |" );
        }
    }

    // Head of dead snake
    mvwprintz( w_snake, body_length + 1, 3, c_green, "(     )" );
    mvwprintz( w_snake, body_length + 1, 4, c_dark_gray, "x   x" );
    mvwprintz( w_snake, body_length + 2, 3, c_green, " \\___/ " );
    mvwputch( w_snake, body_length + 3, 6, c_red, '|' );
    mvwputch( w_snake, body_length + 4, 6, c_red, '^' );

    // Tail of dead snake
    mvwprintz( w_snake, body_length + 1, 70, c_green, "\\ /" );
    mvwputch( w_snake, body_length + 2, 71, c_green, 'v' );

    std::vector<std::string> game_over_text;
    game_over_text.push_back( "  ________    _____      _____   ___________       " );
    game_over_text.push_back( " /  _____/   /  _  \\    /     \\  \\_   _____/    " );
    game_over_text.push_back( "/   \\  ___  /  /_\\  \\  /  \\ /  \\  |    __)_   " );
    game_over_text.push_back( "\\    \\_\\  \\/    |    \\/    Y    \\ |        \\" );
    game_over_text.push_back( " \\______  /\\____|__  /\\____|__  //_______  /    " );
    game_over_text.push_back( "        \\/         \\/         \\/         \\/    " );
    game_over_text.push_back( " ________ ____   _________________________         " );
    game_over_text.push_back( " \\_____  \\\\   \\ /   /\\_   _____/\\______   \\ " );
    game_over_text.push_back( "  /   |   \\\\   Y   /  |    __)_  |       _/      " );
    game_over_text.push_back( " /    |    \\\\     /   |        \\ |    |   \\    " );
    game_over_text.push_back( " \\_______  / \\___/   /_______  / |____|_  /      " );
    game_over_text.push_back( "         \\/                  \\/         \\/      " );

    for( size_t i = 0; i < game_over_text.size(); i++ ) {
        mvwprintz( w_snake, i + 3, 17, c_light_red, game_over_text[i].c_str() );
    }

    center_print( w_snake, 17, c_yellow, string_format( _( "TOTAL SCORE: %d" ), iScore ) );
    //@todo: print actual bound keys
    center_print( w_snake, 21, c_white, _( "Press 'q' or ESC to exit." ) );
    wrefresh( w_snake );
}

int snake_game::start_game()
{
    std::vector<std::pair<int, int> > vSnakeBody;
    std::map<int, std::map<int, bool> > mSnakeBody;

    int iOffsetX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
    int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

    catacurses::window w_snake = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                 iOffsetY, iOffsetX );
    print_header( w_snake );

    //Snake start position
    vSnakeBody.push_back( std::make_pair( FULL_SCREEN_HEIGHT / 2, FULL_SCREEN_WIDTH / 2 ) );
    mSnakeBody[FULL_SCREEN_HEIGHT / 2][FULL_SCREEN_WIDTH / 2] = true;
    mvwputch( w_snake, vSnakeBody[vSnakeBody.size() - 1].first,
              vSnakeBody.back().second, c_white, '#' );

    //Snake start direction
    int iDirY = 0;
    int iDirX = 1;

    //Snake start length
    size_t iSnakeBody = 10;

    //GameSpeed aka inputdelay/timeout
    int iGameSpeed = 100;

    //Score
    int iScore = 0;
    int iFruitPosY = 0;
    int iFruitPosX = 0;

    //Draw Score
    print_score( w_snake, iScore );

    input_context ctxt( "SNAKE" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" );

    do {
        //Check if we hit a border
        if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == 0 ) {
            vSnakeBody.push_back( std::make_pair( vSnakeBody[vSnakeBody.size() - 1].first +
                                                  iDirY + FULL_SCREEN_HEIGHT - 2,
                                                  vSnakeBody[vSnakeBody.size() - 1].second + iDirX ) );

        } else if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == FULL_SCREEN_HEIGHT - 1 ) {
            vSnakeBody.push_back( std::make_pair( vSnakeBody[vSnakeBody.size() - 1].first +
                                                  iDirY - FULL_SCREEN_HEIGHT + 2,
                                                  vSnakeBody[vSnakeBody.size() - 1].second + iDirX ) );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == 0 ) {
            vSnakeBody.push_back( std::make_pair( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                                  vSnakeBody[vSnakeBody.size() - 1].second +
                                                  iDirX + FULL_SCREEN_WIDTH - 2 ) );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == FULL_SCREEN_WIDTH - 1 ) {
            vSnakeBody.push_back( std::make_pair( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                                  vSnakeBody[vSnakeBody.size() - 1].second +
                                                  iDirX - FULL_SCREEN_WIDTH + 2 ) );

        } else {
            vSnakeBody.push_back( std::make_pair( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                                  vSnakeBody[vSnakeBody.size() - 1].second + iDirX ) );
        }

        //Check if we hit ourselves
        if( mSnakeBody[vSnakeBody[vSnakeBody.size() - 1].first]
            [vSnakeBody[vSnakeBody.size() - 1].second] ) {
            //We are dead :(
            break;
        } else {
            //Add new position to map
            mSnakeBody[vSnakeBody[vSnakeBody.size() - 1].first]
            [vSnakeBody[vSnakeBody.size() - 1].second] = true;
        }

        //Have we eaten the forbidden fruit?
        if( vSnakeBody[vSnakeBody.size() - 1].first == iFruitPosY &&
            vSnakeBody[vSnakeBody.size() - 1].second == iFruitPosX ) {
            iScore += 500;
            iSnakeBody += 10;
            iGameSpeed -= 3;

            print_score( w_snake, iScore );

            iFruitPosY = 0;
            iFruitPosX = 0;
        }

        //Check if we are longer than our max size
        if( vSnakeBody.size() > iSnakeBody ) {
            mSnakeBody[vSnakeBody[0].first][vSnakeBody[0].second] = false;
            mvwputch( w_snake, vSnakeBody[0].first, vSnakeBody[0].second, c_black, ' ' );
            vSnakeBody.erase( vSnakeBody.begin(), vSnakeBody.begin() + 1 );
        }

        //Draw Snake
        mvwputch( w_snake, vSnakeBody[vSnakeBody.size() - 1].first,
                  vSnakeBody[vSnakeBody.size() - 1].second, c_white, '#' );
        mvwputch( w_snake, vSnakeBody[vSnakeBody.size() - 2].first,
                  vSnakeBody[vSnakeBody.size() - 2].second, c_light_gray, '#' );

        //On full length add a fruit
        if( iFruitPosX == 0 && iFruitPosY == 0 ) {
            do {
                iFruitPosY = rng( 1, FULL_SCREEN_HEIGHT - 2 );
                iFruitPosX = rng( 1, FULL_SCREEN_WIDTH - 2 );
            } while( mSnakeBody[iFruitPosY][iFruitPosX] );

            mvwputch( w_snake, iFruitPosY, iFruitPosX, c_light_red, '*' );
        }

        wrefresh( w_snake );

        const std::string action = ctxt.handle_input( iGameSpeed );

        if( action == "UP" ) {
            if( iDirY != 1 ) {
                iDirY = -1;
                iDirX = 0;
            }
        } else if( action == "DOWN" ) {
            if( iDirY != -1 ) {
                iDirY = 1;
                iDirX = 0;
            }
        } else if( action == "LEFT" ) {
            if( iDirX != 1 ) {
                iDirY = 0;
                iDirX = -1;
            }
        } else if( action == "RIGHT" ) {
            if( iDirX != -1 ) {
                iDirY = 0;
                iDirX = 1;
            }
        } else if( action == "QUIT" ) {
            return iScore;
        }

    } while( true );

    snake_over( w_snake, iScore );
    while( ctxt.handle_input() != "QUIT" ) {
        // try again
    }

    return iScore;
}
