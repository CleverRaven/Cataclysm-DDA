/* Entry point and main loop for Cataclysm
 */

// IWYU pragma: no_include <sys/signal.h>
#include <algorithm>
#include <array>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#if defined(_WIN32)
#include "platform_win.h"
#else
#include <csignal>
#endif

#include <flatbuffers/util.h>

#include "cached_options.h"
#include "cata_path.h"
#include "color.h"
#include "compatibility.h"
#include "crash.h"
#include "cursesdef.h"
#include "debug.h"
#include "do_turn.h"
#include "event.h"
#include "event_bus.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "get_version.h"
#include "help.h"
#include "input.h"
#include "loading_ui.h"
#include "main_menu.h"
#include "mapsharing.h"
#include "memory_fast.h"
#include "options.h"
#include "ordered_static_globals.h"
#include "output.h"
#include "path_info.h"
#include "rng.h"
#include "system_locale.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#if defined(MACOSX)
#   include <unistd.h> // getpid()
#endif

#if defined(PREFIX)
#   undef PREFIX
#   include "prefix.h"
#endif

class ui_adaptor;

#if defined(TILES)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#      include <SDL2/SDL_version.h>
#   else
#      include <SDL_version.h>
#   endif
#endif

#if defined(__ANDROID__)
#include <SDL_filesystem.h>
#include <SDL_keyboard.h>
#include <SDL_system.h>
#include <android/log.h>
#include <unistd.h>

// Taken from: https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/
// Force Android standard output to adb logcat output

static int pfd[2];
static pthread_t thr;
static const char *tag = "cdda";

static void *thread_func( void * )
{
    ssize_t rdsz;
    char buf[128];
    for( ;; ) {
        if( ( ( rdsz = read( pfd[0], buf, sizeof buf - 1 ) ) > 0 ) ) {
            if( buf[rdsz - 1] == '\n' ) {
                --rdsz;
            }
            buf[rdsz] = 0;  /* add null-terminator */
            __android_log_write( ANDROID_LOG_DEBUG, tag, buf );
        }
    }
    return 0;
}

int start_logger( const char *app_name )
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf( stdout, 0, _IOLBF, 0 );
    setvbuf( stderr, 0, _IONBF, 0 );

    /* create the pipe and redirect stdout and stderr */
    pipe( pfd );
    dup2( pfd[1], 1 );
    dup2( pfd[1], 2 );

    /* spawn the logging thread */
    if( pthread_create( &thr, 0, thread_func, 0 ) == -1 ) {
        return -1;
    }
    pthread_detach( thr );
    return 0;
}

#endif //__ANDROID__

namespace
{

#if defined(_WIN32)
// Used only if AttachConsole() works
FILE *CONOUT;
#endif
void exit_handler( int s )
{
    const int old_timeout = inp_mngr.get_timeout();
    inp_mngr.reset_timeout();
    if( s != 2 || query_yn( _( "Really Quit?  All unsaved changes will be lost." ) ) ) {
        deinitDebug();

        int exit_status = 0;
        g.reset();

        catacurses::endwin();

#if defined(__ANDROID__)
        // Avoid capturing SIGABRT on exit on Android in crash report
        // Can be removed once the SIGABRT on exit problem is fixed
        signal( SIGABRT, SIG_DFL );
#endif

#if !defined(_WIN32)
        if( s == 2 ) {
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = SIG_DFL;
            sigemptyset( &sigIntHandler.sa_mask );
            sigIntHandler.sa_flags = 0;
            sigaction( SIGINT, &sigIntHandler, nullptr );
            kill( getpid(), s );
        } else
#endif
        {
            exit( exit_status );
        }
    }
    inp_mngr.set_timeout( old_timeout );
    ui_manager::redraw_invalidated();
    catacurses::doupdate();
}

struct arg_handler {
    //! Handler function to be invoked when this argument is encountered. The handler will be
    //! called with the number of parameters after the flag was encountered, along with the array
    //! of following parameters. It must return an integer indicating how many parameters were
    //! consumed by the call or -1 to indicate that a required argument was missing.
    using handler_method = std::function<int ( int, const char ** )>;

    std::string_view flag;  //!< The commandline parameter to handle (e.g., "--seed").
    std::string_view param_documentation;  //!< Human readable description of this arguments parameter.
    std::string_view documentation;  //!< Human readable documentation for this argument.
    std::string_view help_group; //!< Section of the help message in which to include this argument.
    int num_args; //!< How many further arguments are expected for this parameter (usually 0 or 1).
    handler_method handler;  //!< The callback to be invoked when this argument is encountered.
};

template<typename FirstPassArgs, typename SecondPassArgs>
void printHelpMessage( const FirstPassArgs &first_pass_arguments,
                       const SecondPassArgs &second_pass_arguments )
{
    // Group all arguments by help_group.
    std::multimap<std::string, const arg_handler *> help_map;
    for( const arg_handler &handler : first_pass_arguments ) {
        help_map.emplace( handler.help_group, &handler );
    }
    for( const arg_handler &handler : second_pass_arguments ) {
        help_map.emplace( handler.help_group, &handler );
    }

    std::cout << "Command line parameters:\n";
    std::string current_help_group;
    for( std::pair<const std::string, const arg_handler *> &help_entry : help_map ) {
        if( help_entry.first != current_help_group ) {
            current_help_group = help_entry.first;
            std::cout << "\n" << current_help_group << "\n";
        }

        const arg_handler *handler = help_entry.second;
        std::cout << handler->flag << " " << handler->param_documentation;
        if( !handler->documentation.empty() ) {
            std::cout << "\n    " << handler->documentation << "\n";
        }
    }
    std::cout << std::endl;
}

/**
 * Displays current application version and compile options values
 */
void printVersionMessage()
{
#if defined(TILES)
    const bool hasTiles = true;
#else
    const bool hasTiles = false;
#endif

#if defined(SDL_SOUND)
    const bool hasSound = true;
#else
    const bool hasSound = false;
#endif

    printf( "Cataclysm Dark Days Ahead: %s\n\n"
            "%ctiles, %csound\n\n"
            "data dir: %s\nuser dir: %s\n",
            getVersionString(),
            hasTiles ? '+' : '-',
            hasSound ? '+' : '-',
            PATH_INFO::datadir().c_str(),
            PATH_INFO::user_dir().c_str() );
}

void process_args( const char **argv, int argc, const std::vector<arg_handler> &arg_handlers )
{
    while( argc ) {
        bool arg_handled = false;
        for( const arg_handler &handler : arg_handlers ) {
            if( handler.flag == argv[0] ) {
                argc--;
                argv++;
                if( argc < handler.num_args ) {
                    std::cout << "Missing expected argument to command line parameter " << handler.flag << std::endl;
                    std::exit( 1 );
                }
                int args_consumed = handler.handler( argc, argv );
                if( args_consumed < 0 ) {
                    printf( "Failed parsing parameter '%s'\n", *( argv - 1 ) );
                    std::exit( 1 );
                }
                argc -= args_consumed;
                argv += args_consumed;
                arg_handled = true;
                break;
            }
        }
        // Skip other options.
        if( !arg_handled ) {
            --argc;
            ++argv;
        }
    }
}

struct cli_opts {
    int seed = time( nullptr );
    bool verifyexit = false;
    bool check_mods = false;
    std::vector<std::string> opts;
    std::string world; /** if set try to load first save in this world on startup */
    bool disable_ascii_art = false;
};

cli_opts parse_commandline( int argc, const char **argv )
{
    cli_opts result;

    constexpr std::string_view section_default;
    constexpr std::string_view section_map_sharing = "Map sharing";
    constexpr std::string_view section_user_directory = "User directories";
    constexpr std::string_view section_accessibility = "Accessibility";
    const std::vector<arg_handler> first_pass_arguments = {{
            {
                "--seed", "<string of letters and or numbers>",
                "Sets the random number generator's seed value",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    const unsigned char *hash_input = reinterpret_cast<const unsigned char *>( params[0] );
                    result.seed = djb2_hash( hash_input );
                    return 1;
                }
            },
            {
                "--jsonverify", {},
                "Checks the CDDA json files",
                section_default,
                0,
                [&result]( int, const char ** ) -> int {
                    result.verifyexit = true;
                    return 0;
                }
            },
            {
                "--check-mods", "[mod…]",
                "Checks the json files belonging to given CDDA mod",
                section_default,
                1,
                [&result]( int n, const char **params ) -> int {
                    result.check_mods = true;
                    test_mode = true;
                    for( int i = 0; i < n; ++i )
                    {
                        result.opts.emplace_back( params[ i ] );
                    }
                    return 0;
                }
            },
            {
                "--world", "<name>",
                "Load world",
                section_default,
                1,
                [&result]( int, const char **params ) -> int {
                    result.world = params[0];
                    return 1;
                }
            },
            {
                "--basepath", "<path>",
                "Base path for all game data subdirectories",
                section_default,
                1,
                []( int, const char **params )
                {
                    PATH_INFO::init_base_path( params[0] );
                    PATH_INFO::set_standard_filenames();
                    return 1;
                }
            },
            {
                "--shared", {},
                "Activates the map-sharing mode",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setSharing( true );
                    MAP_SHARING::setCompetitive( true );
                    MAP_SHARING::setWorldmenu( false );
                    return 0;
                }
            },
            {
                "--username", "<name>",
                "Instructs map-sharing code to use this name for your character.",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::setUsername( params[0] );
                    return 1;
                }
            },
            {
                "--addadmin", "<username>",
                "Instructs map-sharing code to use this name for your character and give you "
                "access to the cheat functions.",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::addAdmin( params[0] );
                    return 1;
                }
            },
            {
                "--adddebugger", "<username>",
                "Informs map-sharing code that you're running inside a debugger",
                section_map_sharing,
                1,
                []( int, const char **params ) -> int {
                    MAP_SHARING::addDebugger( params[0] );
                    return 1;
                }
            },
            {
                "--competitive", {},
                "Instructs map-sharing code to disable access to the in-game cheat functions",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setCompetitive( true );
                    return 0;
                }
            },
            {
                "--userdir", "<path>",
                // NOLINTNEXTLINE(cata-text-style): the dot is not a period
                "Base path for user-overrides to files from the ./data directory and named below",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::init_user_dir( params[0] );
                    PATH_INFO::set_standard_filenames();
                    return 1;
                }
            },
            {
                "--disable-ascii-art", {},
                "Disable aesthetic ascii art in menus and descriptions.",
                section_accessibility,
                0,
                [&result]( int, const char ** ) -> int {
                    result.disable_ascii_art = true;
                    return 0;
                }
            }
        }
    };

    // The following arguments are dependent on one or more of the previous flags and are run
    // in a second pass.
    const std::vector<arg_handler> second_pass_arguments = {{
            {
                "--worldmenu", {},
                "Enables the world menu in the map-sharing code",
                section_map_sharing,
                0,
                []( int, const char ** ) -> int {
                    MAP_SHARING::setWorldmenu( true );
                    return true;
                }
            },
            {
                "--datadir", "<directory name>",
                "Sub directory from which game data is loaded",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_datadir( params[0] );
                    return 1;
                }
            },
            {
                "--savedir", "<directory name>",
                "Subdirectory for game saves",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_savedir( params[0] );
                    return 1;
                }
            },
            {
                "--configdir", "<directory name>",
                "Subdirectory for game configuration",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_config_dir( params[0] );
                    return 1;
                }
            },
            {
                "--memorialdir", "<directory name>",
                "Subdirectory for memorials",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_memorialdir( params[0] );
                    return 1;
                }
            },
            {
                "--optionfile", "<filename>",
                "Name of the options file within the configdir",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_options( params[0] );
                    return 1;
                }
            },
            {
                "--keymapfile", "<filename>",
                "Name of the keymap file within the configdir",
                section_user_directory,
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_keymap( params[0] );
                    return 1;
                }
            },
            {
                "--autopickupfile", "<filename>",
                "Name of the autopickup options file within the configdir",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_autopickup( params[0] );
                    return 1;
                }
            },
            {
                "--motdfile", "<filename>",
                "Name of the message of the day file within the motd directory",
                {},
                1,
                []( int, const char **params ) -> int {
                    PATH_INFO::set_motd( params[0] );
                    return 1;
                }
            },
        }
    };

    if( std::count( argv, argv + argc, std::string( "--help" ) ) ) {
        printHelpMessage( first_pass_arguments, second_pass_arguments );
        std::exit( 0 );
    }

    if( std::count( argv, argv + argc, std::string( "--version" ) ) ) {
        printVersionMessage();
        std::exit( 0 );
    }

    // skip program name
    --argc;
    ++argv;

    process_args( argv, argc, first_pass_arguments );
    process_args( argv, argc, second_pass_arguments );

    return result;
}

bool assure_essential_dirs_exist()
{
    using namespace PATH_INFO;
    std::vector<std::string> essential_paths{
        config_dir(),
        savedir(),
        templatedir(),
        user_font(),
        user_sound().get_unrelative_path().u8string(),
        user_gfx().get_unrelative_path().u8string()
    };
    for( const std::string &path : essential_paths ) {
        if( !assure_dir_exist( path ) ) {
            popup( _( "Unable to make directory \"%s\".  Check permissions." ), path );
            return false;
        }
    }
    return true;
}

}  // namespace

#if defined(USE_WINMAIN)
int APIENTRY WinMain( _In_ HINSTANCE /* hInstance */, _In_opt_ HINSTANCE /* hPrevInstance */,
                      _In_ LPSTR /* lpCmdLine */, _In_ int /* nCmdShow */ )
{
    int argc = __argc;
    char **argv = __argv;
#elif defined(__ANDROID__)
extern "C" int SDL_main( int argc, char **argv ) {
#else
int main( int argc, const char *argv[] )
{
#endif
    ordered_static_globals();
    init_crash_handlers();
    reset_floating_point_mode();
#if defined(FLATBUFFERS_LOCALE_INDEPENDENT) && (FLATBUFFERS_LOCALE_INDEPENDENT > 0)
    flatbuffers::ClassicLocale::Get();
#endif

    on_out_of_scope json_member_reporting_guard{ [] {
            // Disable reporting unvisited members if stack unwinding leaves main early.
            Json::globally_report_unvisited_members( false );
        } };

#if defined(_WIN32) and defined(TILES)
    const HANDLE std_output { GetStdHandle( STD_OUTPUT_HANDLE ) }, std_error { GetStdHandle( STD_ERROR_HANDLE ) };
    if( std_output != INVALID_HANDLE_VALUE and std_error != INVALID_HANDLE_VALUE ) {
        if( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
            if( std_output == nullptr ) {
                freopen_s( &CONOUT, "CONOUT$", "w", stdout );
            }
            if( std_error == nullptr ) {
                freopen_s( &CONOUT, "CONOUT$", "w", stderr );
            }
        }
    }
#endif
#if defined(__ANDROID__)
    // Start the standard output logging redirector
    start_logger( "cdda" );

    // On Android first launch, we copy all data files from the APK into the app's writeable folder so std::io stuff works.
    // Use the external storage so it's publicly modifiable data (so users can mess with installed data, save games etc.)
    std::string external_storage_path( SDL_AndroidGetExternalStoragePath() );

    PATH_INFO::init_base_path( external_storage_path );
#else
    // Set default file paths
#if defined(PREFIX)
    PATH_INFO::init_base_path( std::string( PREFIX ) );
#else
    PATH_INFO::init_base_path( "" );
#endif
#endif

#if defined(__ANDROID__)
    PATH_INFO::init_user_dir( external_storage_path );
#else
#   if defined(USE_HOME_DIR) || defined(USE_XDG_DIR)
    PATH_INFO::init_user_dir( "" );
#   else
    PATH_INFO::init_user_dir( "." );
#   endif
#endif
    PATH_INFO::set_standard_filenames();

    MAP_SHARING::setDefaults();

    cli_opts cli = parse_commandline( argc, const_cast<const char **>( argv ) );

    if( !dir_exist( PATH_INFO::datadir() ) ) {
        printf( "Fatal: Can't find data directory \"%s\"\nPlease ensure the current working directory is correct or specify data directory with --datadir.  Perhaps you meant to start \"cataclysm-launcher\"?\n",
                PATH_INFO::datadir().c_str() );
        exit( 1 );
    }

    if( !assure_dir_exist( PATH_INFO::user_dir() ) ) {
        printf( "Can't open or create %s. Check permissions.\n",
                PATH_INFO::user_dir().c_str() );
        exit( 1 );
    }

    setupDebug( DebugOutput::file );
    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    json_error_output_colors = json_error_output_colors_t::color_tags;

    /**
     * OS X does not populate locale env vars correctly (they usually default to
     * "C") so don't bother trying to set the locale based on them.
     */
#if !defined(MACOSX)
    if( setlocale( LC_ALL, "" ) == nullptr ) {
        DebugLog( D_WARNING, D_MAIN ) << "Error while setlocale(LC_ALL, '').";
    } else {
#endif
        try {
            std::locale::global( std::locale( "" ) );
        } catch( const std::exception & ) {
            // if user default locale retrieval isn't implemented by system
            try {
                // default to basic C locale
                std::locale::global( std::locale::classic() );
            } catch( const std::exception &err ) {
                debugmsg( "%s", err.what() );
                exit_handler( -999 );
            }
        }
#if !defined(MACOSX)
    }
#endif

    DebugLog( D_INFO, DC_ALL ) << "[main] C locale set to " << setlocale( LC_ALL, nullptr );
    DebugLog( D_INFO, DC_ALL ) << "[main] C++ locale set to " << std::locale().name();

#if defined(TILES)
    SDL_version compiled;
    SDL_VERSION( &compiled );
    DebugLog( D_INFO, DC_ALL ) << "SDL version used during compile is "
                               << static_cast<int>( compiled.major ) << "."
                               << static_cast<int>( compiled.minor ) << "."
                               << static_cast<int>( compiled.patch );

    SDL_version linked;
    SDL_GetVersion( &linked );
    DebugLog( D_INFO, DC_ALL ) << "SDL version used during linking and in runtime is "
                               << static_cast<int>( linked.major ) << "."
                               << static_cast<int>( linked.minor ) << "."
                               << static_cast<int>( linked.patch );
#endif

#if !defined(TILES)
    get_options().init();
    get_options().load();
#endif

    // in test mode don't initialize curses to avoid escape sequences being inserted into output stream
    if( !test_mode ) {
        try {
            // set minimum FULL_SCREEN sizes
            FULL_SCREEN_WIDTH = EVEN_MINIMUM_TERM_WIDTH;
            FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT;
            catacurses::init_interface();
        } catch( const std::exception &err ) {
            // can't use any curses function as it has not been initialized
            std::cerr << "Error while initializing the interface: " << err.what() << std::endl;
            DebugLog( D_ERROR, DC_ALL ) << "Error while initializing the interface: " << err.what() << "\n";
            return 1;
        }
    } else if( cli.check_mods ) {
        get_options().init();
        get_options().load();
    }

    set_language_from_options();

    rng_set_engine_seed( cli.seed );

    game_ui::init_ui();

    g = std::make_unique<game>();

    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if( cli.verifyexit ) {
            exit_handler( 0 );
        }
        if( cli.check_mods ) {
            init_colors();
            loading_ui ui( false );
            const std::vector<mod_id> mods( cli.opts.begin(), cli.opts.end() );
            exit( g->check_mod_data( mods, ui ) && !debug_has_error_been_observed() ? 0 : 1 );
        }
    } catch( const std::exception &err ) {
        debugmsg( "%s", err.what() );
        exit_handler( -999 );
    }

    // Override existing settings from cli  options
    if( cli.disable_ascii_art ) {
        get_options().get_option( "ENABLE_ASCII_ART" ).setValue( "false" );
        get_options().get_option( "ENABLE_ASCII_TITLE" ).setValue( "false" );
    }

    // Now we do the actual game.

#if defined(DEBUG_CURSES_CURSOR)
    catacurses::curs_set( 2 );
#else
    // I have no clue what this comment is on about
    // Any value works well enough for debugging at least
    catacurses::curs_set( 0 ); // Invisible cursor here, because MAPBUFFER.load() is crash-prone
#endif

#if !defined(_WIN32)
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset( &sigIntHandler.sa_mask );
    sigIntHandler.sa_flags = 0;
    sigaction( SIGINT, &sigIntHandler, nullptr );
#endif

#if defined(LOCALIZE)
    if( get_option<std::string>( "USE_LANG" ).empty() && !SystemLocale::Language().has_value() ) {
        select_language();
        set_language_from_options();
    }
#endif
    replay_buffered_debugmsg_prompts();

    if( !assure_essential_dirs_exist() ) {
        exit_handler( -999 );
        return 0;
    }

    main_menu::queued_world_to_load = std::move( cli.world );

    get_help().load();

    while( true ) {
        main_menu menu;
        if( !menu.opening_screen() ) {
            break;
        }

        shared_ptr_fast<ui_adaptor> ui = g->create_or_get_main_ui_adaptor();
        get_event_bus().send<event_type::game_begin>( getVersionString() );
        while( !do_turn() );
    }

    exit_handler( -999 );
    return 0;
}
