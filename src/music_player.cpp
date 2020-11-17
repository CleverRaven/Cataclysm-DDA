#if defined(SDL_SOUND) && !defined(__ANDROID__)
#include <algorithm>

#if defined(_MSC_VER) && defined(USE_VCPKG)
#    include <SDL2/SDL_mixer.h>
#else
#    include <SDL_mixer.h>
#endif

#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "cursesdef.h"
#include "input.h"
#include "music_player.h"
#include "options.h"
#include "output.h"
#include "rng.h"
#include "sdlsound.h"
#include "ui_manager.h"

#if __cplusplus > 201703L
#include <filesystem> // C++17 standard header file name
namespace fs = std::filesystem;
#else
// c++14
#include <experimental/filesystem> // Header file for pre-standard implementation
namespace fs = std::experimental::filesystem;
#endif


/*---------------private function declarations begin-----------------*/

/* Get paths of all files in in directory*/
static void get_music_files_list( const std::string &dir, std::vector<fs::path> &paths_list );

/* Get random music file path from directory &dir */
static std::string get_random_music_file( const std::string &dir );

/* Get all supported formats by SDL Mixer */
static std::string get_supported_formats();

/* load support for the OGG FLAC MP3 sample/music formats */
static void init_formats();

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
    point pos( ( TERMX - w_width ) / 2, ( TERMY - w_height ) / 2 );

    catacurses::window w_player;
    catacurses::window w_music_list;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {

        pos.x = clamp( ( TERMX - w_width ) / 2, 0, TERMX - w_width );
        pos.y = clamp( ( TERMY - w_height ) / 2, 0, TERMY - w_height );

        w_player = catacurses::newwin( w_height, w_width, pos );
        w_music_list = catacurses::newwin( w_list_height, w_list_width, pos + point( 2, 2 ) );

        ui.position( pos, pos + point( w_width, w_height ) );
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

    int selected_button = 2;
    std::vector<std::string> buttons = { " |<< ", " # ", " > ", " || ", " >>| " };

    int selected_list = 0;
    std::vector<fs::path> file_list;
    static fs::path playing_file;
    std::string music_dir = "music/";
    get_music_files_list( music_dir, file_list );
    int number_files = static_cast<int>( file_list.size() ) - 1;
    int volume = get_option<int>( "MUSIC_VOLUME" );

    const nc_color music_info_color = c_light_green_cyan;
    const nc_color selected_music_color = music_info_color;
    const nc_color music_list_color = c_black_cyan;
    const nc_color text_color = c_light_gray_yellow;
    const nc_color buttons_color = c_light_green;

    point p1;
    int offset = 0;
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_player );
        werase( w_music_list );

        // window player
        draw_border( w_player );
        // background painting
        p1.x = 0;
        std::string color_line( w_width, ' ' );
        for( p1.y = 1; p1.y < w_height - 3; p1.y++ ) {
            mvwprintz( w_player, p1, text_color, color_line );
        }
        // top panel
        mvwprintz( w_player, point_zero, music_info_color, color_line );
        std::string title = _( "CataCurses Music Player v0.9-beta" );
        int title_len = static_cast<int>( title.size() );
        mvwprintz( w_player, point_zero, music_info_color, title );
        if( Mix_PlayingMusic() ) {
            trim_and_print( w_player, point( title_len, 0 ), w_width - title_len - 1, music_info_color,
                            string_format( " - %s ",
                                           playing_file.filename().string() ) );
        }
        right_print( w_player, 0, 0, c_dark_gray_red, "X" );
        // music info
        p1.x = w_width - 35;
        p1.y = 2;
        offset = 11;
        if( Mix_PlayingMusic() ) {
            if( Mix_PausedMusic() ) {
                mvwprintz( w_player, point( p1.x + offset, p1.y++ ), text_color, _( "Pausing" ) );
            } else {
                mvwprintz( w_player, point( p1.x + offset, p1.y++ ), text_color, _( "Playing" ) );
            }
        }
        p1.y = 3;
        mvwprintz( w_player, point( p1.x, p1.y++ ), text_color, _( "Title:" ) );
        mvwprintz( w_player, point( p1.x, p1.y++ ), text_color, _( "Volume:" ) );
        p1.x = w_width - 24;
        p1.y = 3;
        if( Mix_PlayingMusic() ) {
            trim_and_print( w_player, point( p1.x, p1.y++ ), 22, music_info_color, string_format( " %s ",
                            playing_file.filename().string() ) );
            mvwprintz( w_player, point( p1.x, p1.y++ ), music_info_color, string_format( " %d ", volume ) );
        }
        // Buttons
        p1.x = 8;
        p1.y = 22;
        offset = 10;
        for( int i = 0 ; i < static_cast<int>( buttons.size() ); i++ ) {
            nc_color btn_color = ( i == selected_button ) ? c_light_green_green : buttons_color;
            mvwprintz( w_player, point( p1.x += offset, p1.y ), btn_color, buttons.at( i ) );
        }
        // buttons borders
        p1.y = 21;
        for( p1.x = 0; p1.x < w_width; p1.x++ ) {
            mvwputch( w_player, p1, BORDER_COLOR, LINE_OXOX );
        }
        mvwputch( w_player, point( 0, p1.y ), BORDER_COLOR, LINE_OXXO );
        mvwputch( w_player, point( w_width - 1, p1.y ), BORDER_COLOR, LINE_OOXX );
        // supported formats
        mvwprintz( w_player, point( 2, 19 ), text_color, _( "Formats supported:" ) );
        wprintz( w_player, text_color, get_supported_formats() );


        // window music list
        // background painting
        for( p1.y = 0; p1.y < w_list_height; p1.y++ ) {
            mvwprintz( w_music_list, point( 0, p1.y ), selected_music_color, color_line );
        }
        if( !file_list.empty() ) {
            // print music list
            p1.x = 1;
            for( p1.y = 0;
                 p1.y < std::min( w_list_height, number_files - selected_list + 1 );
                 p1.y++ ) {
                nc_color clr = ( p1.y == 0 ) ? selected_music_color : music_list_color;
                trim_and_print( w_music_list, p1, w_list_width - 3, clr,
                                file_list.at( p1.y + selected_list ).filename().string() );
            }
            // music list scrollbar
            if( number_files > w_list_height ) {
                scrollbar()
                .offset_x( w_list_width - 1 )
                .offset_y( 0 )
                .content_size( number_files )
                .viewport_pos( selected_list )
                .viewport_size( w_list_height )
                .slot_color( c_black_green )
                .bar_color( c_light_green_green )
                .scroll_to_last( true )
                .apply( w_music_list );
            }
        } else {
            p1.y = 4;
            center_print( w_music_list, p1.y++, music_list_color, _( "Put your music files" ) );
            center_print( w_music_list, p1.y++, music_list_color,
                          string_format( _( "into %s directory." ), colorize( music_dir, selected_music_color ) ) );
        }
        wnoutrefresh( w_player );
        wnoutrefresh( w_music_list );
    } );

    do {
        if( action == "ANY_INPUT" ) {

        } else if( action == "UP" ) {
            selected_list--;
            selected_list = clamp( selected_list, 0, number_files );
        } else if( action == "DOWN" ) {
            selected_list++;
            selected_list = clamp( selected_list, 0, number_files );
        } else if( action == "PAGE_DOWN" ) {
            selected_list += std::min( w_list_height, number_files );
            selected_list = clamp( selected_list, 0, number_files );
        } else if( action == "PAGE_UP" ) {
            selected_list -= std::min( w_list_height, number_files );
            selected_list = clamp( selected_list, 0, number_files );
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
                    if( !file_list.empty() ) {
                        stop_music();
                        selected_list--;
                        selected_list = clamp( selected_list, 0, number_files );
                        playing_file = file_list.at( selected_list );
                        if( p.can_hear( p.pos(), volume ) ) {
                            play_music_path( playing_file.string(), 100 );
                        }
                    }
                    break;
                case 1:
                    // button stop
                    stop_music();
                    break;
                case 2:
                    // button play
                    if( !file_list.empty() ) {
                        stop_music();
                        playing_file = file_list.at( selected_list );
                        if( p.can_hear( p.pos(), volume ) ) {
                            play_music_path( playing_file.string(), 100 );
                        }
                    }
                    break;
                case 3:
                    // button pause
                    if( Mix_PausedMusic() ) {
                        Mix_ResumeMusic();
                    } else {
                        Mix_PauseMusic();
                    }
                    break;
                case 4:
                    // button next track
                    if( !file_list.empty() ) {
                        stop_music();
                        selected_list++;
                        selected_list = clamp( selected_list, 0, number_files );
                        playing_file = file_list.at( selected_list );
                        if( p.can_hear( p.pos(), volume ) ) {
                            play_music_path( playing_file.string(), 100 );
                        }
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

void music_player_next_music( const Character &p )
{
    if( !p.is_player() ) {
        return;
    }
    int volume = get_option<int>( "MUSIC_VOLUME" );
    // If no paying music then play it
    if( !Mix_PlayingMusic() && p.can_hear( p.pos(), volume ) ) {
        play_music_path( get_random_music_file( "music/" ), 100 );
    }
}

void music_player_stop()
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
    std::sort( paths_list.begin(), paths_list.end() );
}

std::string get_random_music_file( const std::string &dir )
{
    std::vector<fs::path> files_list;
    get_music_files_list( dir, files_list );

    // shuffle music list
    std::shuffle( files_list.begin(), files_list.end(), rng_get_engine() );
    return files_list.front().string();
}

std::string get_supported_formats()
{
    init_formats();
    std::string formats;
    int max = Mix_GetNumMusicDecoders();
    for( int i = 0; i < max; ++i ) {
        std::string format;
        formats += " " + format.append( Mix_GetMusicDecoder( i ) );
    }
    return formats;
}

void init_formats()
{
    // 0x00000020 is MIX_INIT_MID flag but clang-tidy abuse it
    int flags = 0x00000020 | MIX_INIT_OGG | MIX_INIT_FLAC | MIX_INIT_MP3;
    Mix_Init( flags );
}
#endif
