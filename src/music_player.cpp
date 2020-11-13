#include <algorithm>

#if defined(_MSC_VER) && defined(USE_VCPKG)
#    include <SDL2/SDL_mixer.h>
#else
#    include <SDL_mixer.h>
#endif

#include "cata_utility.h"
#include "character.h"
#include "cursesdef.h"
#include "input.h"
#include "music_player.h"
#include "options.h"
#include "output.h"
#include "sdlsound.h"
#include "rng.h"
#include "ui_manager.h"

#if __cplusplus > 201703L
#include <filesystem> // C++17 standard header file name
#else
// c++14
#include <experimental/filesystem> // Header file for pre-standard implementation
#endif

namespace fs = std::experimental::filesystem;


/*---------------private function declarations begin-----------------*/
/* Get paths of all files in in directory*/
void get_music_files_list( const std::string &dir, std::vector<fs::path> &paths_list );
/*----------------private function declarations end-----------------*/

bool music_player_interface( const Character &p )
{
    if( !p.is_player() ) {
        return false;
    }

    const int w_width = 80;
    const int w_height = 24;
    const int w_list_width = w_width - 40;
    const int w_list_height = w_height - 8;
    int x_pos = ( TERMX - w_width ) / 2;
    int y_pos = ( TERMY - w_height ) / 2;
    catacurses::window w_player;
    catacurses::window w_music_list;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {

        x_pos = clamp( ( TERMX - w_width ) / 2, 0, TERMX - w_width );
        y_pos = clamp( ( TERMY - w_height ) / 2, 0, TERMY - w_height );

        w_player = catacurses::newwin( w_height, w_width, point( x_pos, y_pos ) );
        w_music_list = catacurses::newwin( w_list_height, w_list_width, point( x_pos + 2, y_pos + 2 ) );

        ui.position( point( x_pos, y_pos ), point( x_pos + w_width, y_pos + w_height ) );
    } );
    ui.mark_resize();

    std::string action;
    input_context ctxt( "PLAYER", keyboard_mode::keychar );
    ctxt.register_action( "UP" );
    ctxt.register_action( "DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "RIGHT" );
    ctxt.register_action( "LEFT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    int selected_button = 3;
    std::vector<std::string> buttons = { " |<< ", " << ", " # ", " > ", " || ", " >> ", " >>| " };

    int selected_list = 0;
    std::vector<fs::path> file_list;
    static fs::path playing_file;
    get_music_files_list( "music/", file_list );
    int volume = get_option<int>( "MUSIC_VOLUME" );

    const nc_color music_info_color = c_light_green_cyan;
    const nc_color selected_music_color = music_info_color;
    const nc_color music_list_color = c_black_cyan;
    const nc_color text_color = c_light_gray_yellow;
    const nc_color buttons_color = c_light_green;

    int x = 0;
    int y = 0;
    int offset = 0;
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_player );
        werase( w_music_list );

        // window player
        draw_border( w_player );
        // background painting
        std::string color_line( w_width, ' ' );
        for( y = 1; y < w_height - 3; y++ ) {
            mvwprintz( w_player, point( 0, y ), text_color, color_line );
        }
        // top panel
        mvwprintz( w_player, point_zero, music_info_color, color_line );
        std::string title = _( "CataCurses Music Player v0.9-beta" );
        int title_len = static_cast<int>( title.size() );
        mvwprintz( w_player, point_zero, music_info_color, title );
        if( Mix_PlayingMusic() ) {
            trim_and_print( w_player, point( title_len, 0 ), w_width - title_len, music_info_color,
                            string_format( " - %s ",
                                           playing_file.filename().string() ) );
        }
        right_print( w_player, 0, 0, c_dark_gray_red, "X" );
        // music info
        x = w_width - 35;
        y = 2;
        offset = 11;
        mvwprintz( w_player, point( x + offset, y++ ), text_color, _( "Plays" ) );
        mvwprintz( w_player, point( x, y++ ), text_color, _( "Title:" ) );
        mvwprintz( w_player, point( x, y++ ), text_color, _( "Duration:" ) );
        mvwprintz( w_player, point( x, y++ ), text_color, _( "Time:" ) );
        mvwprintz( w_player, point( x, y++ ), text_color, _( "Volume:" ) );
        x = w_width - 24;
        y = 3;
        if( Mix_PlayingMusic() ) {
            trim_and_print( w_player, point( x, y++ ), 22, music_info_color, string_format( " %s ",
                            playing_file.filename().string() ) );
            mvwprintz( w_player, point( x, y++ ), music_info_color, " xx:xx " );
            mvwprintz( w_player, point( x, y++ ), music_info_color, " xx:xx " );
            mvwprintz( w_player, point( x, y++ ), music_info_color, string_format( " %d ", volume ) );
        }
        // music scrollbar
        draw_music_scrollbar( w_player, point( 10, 19 ), music_info_color, 60, 25 );
        // Buttons
        x = -2;
        y = 22;
        offset = 10;
        for( int i = 0 ; i < static_cast<int>( buttons.size() ); i++ ) {
            nc_color btn_color = buttons_color;
            if( i == selected_button ) {
                btn_color = c_light_green_green;
            }
            mvwprintz( w_player, point( x += offset, y ), btn_color, buttons.at( i ) );
        }
        // buttons borders
        y = 21;
        for( x = 0; x < w_width; x++ ) {
            mvwputch( w_player, point( x, y ), BORDER_COLOR, LINE_OXOX );
        }
        mvwputch( w_player, point( 0, y ), BORDER_COLOR, LINE_OXXO );
        mvwputch( w_player, point( w_width - 1, y ), BORDER_COLOR, LINE_OOXX );


        // window music list
        // background painting
        for( y = 0; y < w_height - 8; y++ ) {
            mvwprintz( w_music_list, point( 0, y ), selected_music_color, color_line );
        }
        // print music list
        x = 0;
        y = 0;
        for( y = 0; y < std::min( w_list_height, static_cast<int>( file_list.size() ) - selected_list );
             y++ ) {
            nc_color clr = music_list_color;
            if( y == 0 ) {
                clr = selected_music_color;
            }
            trim_and_print( w_music_list, point( 1, y ), w_list_width - 2, clr,
                            file_list.at( y + selected_list ).filename().string() );
        }
        // music list scrollbar
        draw_scrollbar( w_music_list, selected_list, w_list_height, static_cast<int>( file_list.size() ),
                        point( w_list_width - 1, 0 ), c_black_green );

        wnoutrefresh( w_player );
        wnoutrefresh( w_music_list );
    } );

    do {

        if( action == "ANY_INPUT" ) {

        } else if( action == "UP" ) {
            selected_list--;
            selected_list = clamp( selected_list, 0, static_cast<int>( file_list.size() ) - 1 );
        } else if( action == "DOWN" ) {
            selected_list++;
            selected_list = clamp( selected_list, 0, static_cast<int>( file_list.size() ) - 1 );
        } else if( action == "PAGE_DOWN" ) {

        } else if( action == "PAGE_UP" ) {

        } else if( action == "RIGHT" ) {
            selected_button++;
            selected_button = clamp( selected_button, 0, static_cast<int>( buttons.size() ) - 1 );
        } else if( action == "LEFT" ) {
            selected_button--;
            selected_button = clamp( selected_button, 0, static_cast<int>( buttons.size() ) - 1 );
        } else if( action == "CONFIRM" ) {
            switch( selected_button ) {
                case 0:
                    // button previous track
                    stop_music();
                    selected_list--;
                    selected_list = clamp( selected_list, 0, static_cast<int>( file_list.size() ) - 1 );
                    playing_file = file_list.at( selected_list );
                    if( p.can_hear( p.pos(), volume ) ) {
                        play_music_path( playing_file.string(), 100 );
                    }
                    break;
                case 1:
                    // button revind
                    break;
                case 2:
                    // button stop
                    stop_music();
                    break;
                case 3:
                    // button play
                    stop_music();
                    playing_file = file_list.at( selected_list );
                    if( p.can_hear( p.pos(), volume ) ) {
                        play_music_path( playing_file.string(), 100 );
                    }
                    break;
                case 4:
                    // button pause
                    if( Mix_PausedMusic() ) {
                        Mix_ResumeMusic();
                    } else {
                        Mix_PauseMusic();
                    }
                    break;
                case 5:
                    // button flash forward
                    break;
                case 6:
                    // button next track
                    stop_music();
                    selected_list++;
                    selected_list = clamp( selected_list, 0, static_cast<int>( file_list.size() ) - 1 );
                    playing_file = file_list.at( selected_list );
                    if( p.can_hear( p.pos(), volume ) ) {
                        play_music_path( playing_file.string(), 100 );
                    }
                    break;
                default:
                    break;
            }
        }

        ui_manager::redraw();
        action = ctxt.handle_input();
    } while( action != "QUIT" );

    return Mix_PlayingMusic();
}

void music_player_next_music( void )
{
    // If no paying music then play it
    if( !Mix_PlayingMusic() ) {
        play_music_path( get_random_music_file( "music/" ), 100 );
    }
}

void music_player_stop( void )
{
    stop_music();
}

void get_music_files_list( const std::string &dir, std::vector<fs::path> &paths_list )
{
    if( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
        return;
    }

    // get all music paths in list
    for( auto &p : fs::recursive_directory_iterator( dir ) ) {
        std::string f_ext = p.path().extension().string();
        bool is_music = f_ext == ".wav" || f_ext == ".flac" || f_ext == ".mp3" || f_ext == ".ogg" ||
                        f_ext == ".midi";
        if( is_music ) {
            paths_list.push_back( p.path() );
        }
    }
    // shuffle music list
    static auto eng = cata_default_random_engine(
                          std::chrono::system_clock::now().time_since_epoch().count() );
    std::shuffle( paths_list.begin(), paths_list.end(), eng );
}

std::string get_random_music_file( const std::string &dir )
{
    if( !fs::exists( dir ) || !fs::is_directory( dir ) ) {
        return "";
    }

    // get all music paths in list
    std::vector<std::string> path_list;
    for( auto &p : fs::recursive_directory_iterator( dir ) ) {
        std::string f_ext = p.path().extension().string();
        bool is_music = f_ext == ".wav" || f_ext == ".flac" || f_ext == ".mp3" || f_ext == ".ogg" ||
                        f_ext == ".midi";
        if( is_music ) {
            path_list.push_back( p.path().string() );
        }
    }
    // shuffle music list
    static auto eng = cata_default_random_engine(
                          std::chrono::system_clock::now().time_since_epoch().count() );
    std::shuffle( path_list.begin(), path_list.end(), eng );
    return path_list.front();
}

void draw_music_scrollbar( const catacurses::window &window, const point &start_point,
                           const nc_color &scrollbar_color, const int &length, const int &percent )
{
    if( length <= 0 ) {
        return;
    }

    // add <--->
    mvwputch( window, start_point, scrollbar_color, '<' );
    for( int x = 1; x < length - 1; x++ ) {
        wputch( window, scrollbar_color, LINE_OXOX );
    }
    wputch( window, scrollbar_color, '>' );

    // add slider
    int pr = clamp( percent, 0, 100 );
    int pos = 1 + ( length - 1 ) * pr / 100;
    mvwputch( window, point( start_point.x + pos, start_point.y ), scrollbar_color, LINE_XXXX );
}

