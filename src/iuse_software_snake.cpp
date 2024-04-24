#include "iuse_software_snake.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "catacharset.h"  // utf8_width()
#include "color.h"
#include "cursesdef.h"
#include "input_context.h"
#include "output.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

snake_game::snake_game() = default;

void snake_game::print_score( const catacurses::window &w_snake, int iScore )
{
    mvwprintz( w_snake, point( 5, 0 ), c_white, string_format( _( "Score: %d" ), iScore ) );
}

void snake_game::print_header( const catacurses::window &w_snake, bool show_shortcut )
{
    draw_border( w_snake, BORDER_COLOR, _( "S N A K E" ), c_white );
    if( show_shortcut ) {
        std::string shortcut = _( "<q>uit" );
        shortcut_print( w_snake, point( FULL_SCREEN_WIDTH - utf8_width( shortcut ) - 2, 0 ),
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
            mvwprintz( w_snake, point( 4 + j * 65, i ), c_green, "|   |" );
        }
    }

    // Head of dead snake
    mvwprintz( w_snake, point( 3, body_length + 1 ), c_green, "(     )" );
    mvwprintz( w_snake, point( 4, body_length + 1 ), c_dark_gray, "x   x" );
    mvwprintz( w_snake, point( 3, body_length + 2 ), c_green, " \\___/ " );
    mvwputch( w_snake, point( 6, body_length + 3 ), c_red, '|' );
    mvwputch( w_snake, point( 6, body_length + 4 ), c_red, '^' );

    // Tail of dead snake
    mvwprintz( w_snake, point( 70, body_length + 1 ), c_green, "\\ /" );
    mvwputch( w_snake, point( 71, body_length + 2 ), c_green, 'v' );

    std::vector<std::string> game_over_text;
    game_over_text.emplace_back( R"(  ________    _____      _____   ___________)" );
    game_over_text.emplace_back( R"( /  _____/   /  _  \    /     \  \_   _____/)" );
    game_over_text.emplace_back( R"(/   \  ___  /  /_\  \  /  \ /  \  |    __)_ )" );
    game_over_text.emplace_back( R"(\    \_\  \/    |    \/    Y    \ |        \)" );
    game_over_text.emplace_back( R"( \______  /\____|__  /\____|__  //_______  /)" );
    game_over_text.emplace_back( R"(        \/         \/         \/         \/ )" );
    game_over_text.emplace_back( R"( ________ ____   _________________________  )" );
    game_over_text.emplace_back( R"( \_____  \\   \ /   /\_   _____/\______   \ )" );
    game_over_text.emplace_back( R"(  /   |   \\   Y   /  |    __)_  |       _/ )" );
    game_over_text.emplace_back( R"( /    |    \\     /   |        \ |    |   \ )" );
    game_over_text.emplace_back( R"( \_______  / \___/   /_______  / |____|_  / )" );
    game_over_text.emplace_back( R"(         \/                  \/         \/  )" );

    for( size_t i = 0; i < game_over_text.size(); i++ ) {
        mvwprintz( w_snake, point( 17, i + 3 ), c_light_red, game_over_text[i] );
    }

    center_print( w_snake, 17, c_yellow, string_format( _( "TOTAL SCORE: %d" ), iScore ) );
    // TODO: print actual bound keys
    center_print( w_snake, 21, c_white, _( "Press 'q' or ESC to exit." ) );
    wnoutrefresh( w_snake );
}

int snake_game::start_game()
{
    std::vector<std::pair<int, int> > vSnakeBody;
    std::map<int, std::map<int, bool> > mSnakeBody;

    catacurses::window w_snake;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                             TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );

        w_snake = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                      iOffset );

        ui.position_from_window( w_snake );
    } );
    ui.mark_resize();

    //Snake start position
    vSnakeBody.emplace_back( FULL_SCREEN_HEIGHT / 2, FULL_SCREEN_WIDTH / 2 );
    mSnakeBody[FULL_SCREEN_HEIGHT / 2][FULL_SCREEN_WIDTH / 2] = true;

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

    input_context ctxt( "SNAKE" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_snake );
        print_header( w_snake );
        for( auto it = vSnakeBody.begin(); it != vSnakeBody.end(); ++it ) {
            const nc_color col = it + 1 == vSnakeBody.end() ? c_white : c_light_gray;
            mvwputch( w_snake, point( it->second, it->first ), col, '#' );
        }
        if( iFruitPosX != 0 && iFruitPosY != 0 ) {
            mvwputch( w_snake, point( iFruitPosX, iFruitPosY ), c_light_red, '*' );
        }
        print_score( w_snake, iScore );
        wnoutrefresh( w_snake );
    } );

    do {
        //Check if we hit a border
        if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == 0 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first +
                                     iDirY + FULL_SCREEN_HEIGHT - 2,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );

        } else if( vSnakeBody[vSnakeBody.size() - 1].first + iDirY == FULL_SCREEN_HEIGHT - 1 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first +
                                     iDirY - FULL_SCREEN_HEIGHT + 2,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == 0 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second +
                                     iDirX + FULL_SCREEN_WIDTH - 2 );

        } else if( vSnakeBody[vSnakeBody.size() - 1].second + iDirX == FULL_SCREEN_WIDTH - 1 ) {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second +
                                     iDirX - FULL_SCREEN_WIDTH + 2 );

        } else {
            vSnakeBody.emplace_back( vSnakeBody[vSnakeBody.size() - 1].first + iDirY,
                                     vSnakeBody[vSnakeBody.size() - 1].second + iDirX );
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

            iFruitPosY = 0;
            iFruitPosX = 0;
        }

        //Check if we are longer than our max size
        if( vSnakeBody.size() > iSnakeBody ) {
            mSnakeBody[vSnakeBody[0].first][vSnakeBody[0].second] = false;
            vSnakeBody.erase( vSnakeBody.begin(), vSnakeBody.begin() + 1 );
        }

        //On full length add a fruit
        if( iFruitPosX == 0 && iFruitPosY == 0 ) {
            do {
                iFruitPosY = rng( 1, FULL_SCREEN_HEIGHT - 2 );
                iFruitPosX = rng( 1, FULL_SCREEN_WIDTH - 2 );
            } while( mSnakeBody[iFruitPosY][iFruitPosX] );
        }

        ui_manager::redraw();

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

    ui.on_redraw( [&]( const ui_adaptor & ) {
        snake_over( w_snake, iScore );
    } );
    do {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "QUIT" ) {
            break;
        }
    } while( true );

    return iScore;
}
