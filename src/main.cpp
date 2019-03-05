/* Entry point and main loop for Cataclysm
 */

#include <cstring>
#include <ctime>
#include <iostream>
#include <locale>
#include <map>
#if (!(defined _WIN32 || defined WINDOWS))
#include <signal.h>
#endif
#include <stdexcept>
#ifdef LOCALIZE
#include <libintl.h>
#endif

#include "color.h"
#include "crash.h"
#include "cursesdef.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "loading_ui.h"
#include "main_menu.h"
#include "mapsharing.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "rng.h"
#include "translations.h"

#ifdef TILES
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#      include <SDL2/SDL_version.h>
#   else
#      include <SDL_version.h>
#   endif
#endif

#ifdef __ANDROID__
#include <unistd.h>
#include <SDL_system.h>
#include <SDL_filesystem.h>
#include <SDL_keyboard.h>
#include <android/log.h>

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

void exit_handler( int s );

extern bool test_dirty;

namespace
{

struct arg_handler {
    //! Handler function to be invoked when this argument is encountered. The handler will be
    //! called with the number of parameters after the flag was encountered, along with the array
    //! of following parameters. It must return an integer indicating how many parameters were
    //! consumed by the call or -1 to indicate that a required argument was missing.
    typedef std::function<int( int, const char ** )> handler_method;

    const char *flag;  //!< The commandline parameter to handle (e.g., "--seed").
    const char *param_documentation;  //!< Human readable description of this arguments parameter.
    const char *documentation;  //!< Human readable documentation for this argument.
    const char *help_group; //!< Section of the help message in which to include this argument.
    handler_method handler;  //!< The callback to be invoked when this argument is encountered.
};

void printHelpMessage( const arg_handler *first_pass_arguments, size_t num_first_pass_arguments,
                       const arg_handler *second_pass_arguments, size_t num_second_pass_arguments );
}  // namespace

#if defined USE_WINMAIN
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    int argc = __argc;
    char **argv = __argv;
#elif defined __ANDROID__
extern "C" int SDL_main( int argc, char **argv ) {
#else
int main( int argc, char *argv[] )
{
#endif
    init_crash_handlers();
    int seed = time( NULL );
    bool verifyexit = false;
    bool check_mods = false;
    std::string dump;
    dump_mode dmode = dump_mode::TSV;
    std::vector<std::string> opts;
    std::string world; /** if set try to load first save in this world on startup */

#ifdef __ANDROID__
    // Start the standard output logging redirector
    start_logger( "cdda" );

    // On Android first launch, we copy all data files from the APK into the app's writeable folder so std::io stuff works.
    // Use the external storage so it's publicly modifiable data (so users can mess with installed data, save games etc.)
    std::string external_storage_path( SDL_AndroidGetExternalStoragePath() );
    if( external_storage_path.back() != '/' ) {
        external_storage_path += '/';
    }

    PATH_INFO::init_base_path( external_storage_path );
#else
    // Set default file paths
#ifdef PREFIX
#define Q(STR) #STR
#define QUOTE(STR) Q(STR)
    PATH_INFO::init_base_path( std::string( QUOTE( PREFIX ) ) );
#else
    PATH_INFO::init_base_path( "" );
#endif
#endif

#ifdef __ANDROID__
    PATH_INFO::init_user_dir( external_storage_path.c_str() );
#else
#if (defined USE_HOME_DIR || defined USE_XDG_DIR)
    PATH_INFO::init_user_dir();
#else
    PATH_INFO::init_user_dir( "./" );
#endif
#endif
    PATH_INFO::set_standard_filenames();

    MAP_SHARING::setDefaults();
    {
        const char *section_default = nullptr;
        const char *section_map_sharing = "Map sharing";
        const char *section_user_directory = "User directories";
        const std::array<arg_handler, 12> first_pass_arguments = {{
                {
                    "--seed", "<string of letters and or numbers>",
                    "Sets the random number generator's seed value",
                    section_default,
                    [&seed]( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        const unsigned char *hash_input = ( const unsigned char * ) params[0];
                        seed = djb2_hash( hash_input );
                        return 1;
                    }
                },
                {
                    "--jsonverify", nullptr,
                    "Checks the CDDA json files",
                    section_default,
                    [&verifyexit]( int, const char ** ) -> int {
                        verifyexit = true;
                        return 0;
                    }
                },
                {
                    "--check-mods", "[mods...]",
                    "Checks the json files belonging to CDDA mods",
                    section_default,
                    [&check_mods, &opts]( int n, const char *params[] ) -> int {
                        check_mods = true;
                        test_mode = true;
                        for( int i = 0; i < n; ++i )
                        {
                            opts.emplace_back( params[ i ] );
                        }
                        return 0;
                    }
                },
                {
                    "--dump-stats", "<what> [mode = TSV] [opts...]",
                    "Dumps item stats",
                    section_default,
                    [&dump, &dmode, &opts]( int n, const char *params[] ) -> int {
                        if( n < 1 )
                        {
                            return -1;
                        }
                        test_mode = true;
                        dump = params[ 0 ];
                        for( int i = 2; i < n; ++i )
                        {
                            opts.emplace_back( params[ i ] );
                        }
                        if( n >= 2 )
                        {
                            if( !strcmp( params[ 1 ], "TSV" ) ) {
                                dmode = dump_mode::TSV;
                                return 0;
                            } else if( !strcmp( params[ 1 ], "HTML" ) ) {
                                dmode = dump_mode::HTML;
                                return 0;
                            } else {
                                return -1;
                            }
                        }
                        return 0;
                    }
                },
                {
                    "--world", "<name>",
                    "Load world",
                    section_default,
                    [&world]( int n, const char *params[] ) -> int {
                        if( n < 1 )
                        {
                            return -1;
                        }
                        world = params[0];
                        return 1;
                    }
                },
                {
                    "--basepath", "<path>",
                    "Base path for all game data subdirectories",
                    section_default,
                    []( int num_args, const char **params )
                    {
                        if( num_args < 1 ) {
                            return -1;
                        }
                        PATH_INFO::init_base_path( params[0] );
                        PATH_INFO::set_standard_filenames();
                        return 1;
                    }
                },
                {
                    "--shared", nullptr,
                    "Activates the map-sharing mode",
                    section_map_sharing,
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
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        MAP_SHARING::setUsername( params[0] );
                        return 1;
                    }
                },
                {
                    "--addadmin", "<username>",
                    "Instructs map-sharing code to use this name for your character and give you "
                    "access to the cheat functions.",
                    section_map_sharing,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        MAP_SHARING::addAdmin( params[0] );
                        return 1;
                    }
                },
                {
                    "--adddebugger", "<username>",
                    "Informs map-sharing code that you're running inside a debugger",
                    section_map_sharing,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        MAP_SHARING::addDebugger( params[0] );
                        return 1;
                    }
                },
                {
                    "--competitive", nullptr,
                    "Instructs map-sharing code to disable access to the in-game cheat functions",
                    section_map_sharing,
                    []( int, const char ** ) -> int {
                        MAP_SHARING::setCompetitive( true );
                        return 0;
                    }
                },
                {
                    "--userdir", "<path>",
                    "Base path for user-overrides to files from the ./data directory and named below",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::init_user_dir( params[0] );
                        PATH_INFO::set_standard_filenames();
                        return 1;
                    }
                }
            }
        };

        // The following arguments are dependent on one or more of the previous flags and are run
        // in a second pass.
        const std::array<arg_handler, 9> second_pass_arguments = {{
                {
                    "--worldmenu", nullptr,
                    "Enables the world menu in the map-sharing code",
                    section_map_sharing,
                    []( int, const char ** ) -> int {
                        MAP_SHARING::setWorldmenu( true );
                        return true;
                    }
                },
                {
                    "--datadir", "<directory name>",
                    "Sub directory from which game data is loaded",
                    nullptr,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "datadir", params[0] );
                        PATH_INFO::update_datadir();
                        return 1;
                    }
                },
                {
                    "--savedir", "<directory name>",
                    "Subdirectory for game saves",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "savedir", params[0] );
                        return 1;
                    }
                },
                {
                    "--configdir", "<directory name>",
                    "Subdirectory for game configuration",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "config_dir", params[0] );
                        PATH_INFO::update_config_dir();
                        return 1;
                    }
                },
                {
                    "--memorialdir", "<directory name>",
                    "Subdirectory for memorials",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "memorialdir", params[0] );
                        return 1;
                    }
                },
                {
                    "--optionfile", "<filename>",
                    "Name of the options file within the configdir",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "options", params[0] );
                        return 1;
                    }
                },
                {
                    "--keymapfile", "<filename>",
                    "Name of the keymap file within the configdir",
                    section_user_directory,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "keymap", params[0] );
                        return 1;
                    }
                },
                {
                    "--autopickupfile", "<filename>",
                    "Name of the autopickup options file within the configdir",
                    nullptr,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "autopickup", params[0] );
                        return 1;
                    }
                },
                {
                    "--motdfile", "<filename>",
                    "Name of the message of the day file within the motd directory",
                    nullptr,
                    []( int num_args, const char **params ) -> int {
                        if( num_args < 1 )
                        {
                            return -1;
                        }
                        PATH_INFO::update_pathname( "motd", params[0] );
                        return 1;
                    }
                },
            }
        };

        // Process CLI arguments.
        const size_t num_first_pass_arguments =
            sizeof( first_pass_arguments ) / sizeof( first_pass_arguments[0] );
        const size_t num_second_pass_arguments =
            sizeof( second_pass_arguments ) / sizeof( second_pass_arguments[0] );
        int saved_argc = --argc; // skip program name
        const char **saved_argv = ( const char ** )++argv;
        while( argc ) {
            if( !strcmp( argv[0], "--help" ) ) {
                printHelpMessage( first_pass_arguments.data(), num_first_pass_arguments,
                                  second_pass_arguments.data(), num_second_pass_arguments );
                return 0;
            } else {
                bool arg_handled = false;
                for( size_t i = 0; i < num_first_pass_arguments; ++i ) {
                    auto &arg_handler = first_pass_arguments[i];
                    if( !strcmp( argv[0], arg_handler.flag ) ) {
                        argc--;
                        argv++;
                        int args_consumed = arg_handler.handler( argc, ( const char ** )argv );
                        if( args_consumed < 0 ) {
                            printf( "Failed parsing parameter '%s'\n", *( argv - 1 ) );
                            exit( 1 );
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
        while( saved_argc ) {
            bool arg_handled = false;
            for( size_t i = 0; i < num_second_pass_arguments; ++i ) {
                auto &arg_handler = second_pass_arguments[i];
                if( !strcmp( saved_argv[0], arg_handler.flag ) ) {
                    --saved_argc;
                    ++saved_argv;
                    int args_consumed = arg_handler.handler( saved_argc, saved_argv );
                    if( args_consumed < 0 ) {
                        printf( "Failed parsing parameter '%s'\n", *( argv - 1 ) );
                        exit( 1 );
                    }
                    saved_argc -= args_consumed;
                    saved_argv += args_consumed;
                    arg_handled = true;
                    break;
                }
            }
            // Ignore unknown options.
            if( !arg_handled ) {
                --saved_argc;
                ++saved_argv;
            }
        }
    }

    if( !dir_exist( FILENAMES["datadir"] ) ) {
        printf( "Fatal: Can't find directory \"%s\"\nPlease ensure the current working directory is correct. Perhaps you meant to start \"cataclysm-launcher\"?\n",
                FILENAMES["datadir"].c_str() );
        exit( 1 );
    }

    if( !assure_dir_exist( FILENAMES["user_dir"] ) ) {
        printf( "Can't open or create %s. Check permissions.\n",
                FILENAMES["user_dir"].c_str() );
        exit( 1 );
    }

    setupDebug( DebugOutput::file );

    /**
     * OS X does not populate locale env vars correctly (they usually default to
     * "C") so don't bother trying to set the locale based on them.
     */
#if (!defined MACOSX)
    if( setlocale( LC_ALL, "" ) == NULL ) {
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
#if (!defined MACOSX)
    }
#endif

    get_options().init();
    get_options().load();
    set_language();

#ifdef TILES
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

    // in test mode don't initialize curses to avoid escape sequences being inserted into output stream
    if( !test_mode ) {
        try {
            // set minimum FULL_SCREEN sizes
            FULL_SCREEN_WIDTH = 80;
            FULL_SCREEN_HEIGHT = 24;
            catacurses::init_interface();
        } catch( const std::exception &err ) {
            // can't use any curses function as it has not been initialized
            std::cerr << "Error while initializing the interface: " << err.what() << std::endl;
            DebugLog( D_ERROR, DC_ALL ) << "Error while initializing the interface: " << err.what() << "\n";
            return 1;
        }
    }

    srand( seed );
    rng_set_engine_seed( seed );

    g.reset( new game );
    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if( verifyexit ) {
            exit_handler( 0 );
        }
        if( !dump.empty() ) {
            init_colors();
            exit( g->dump_stats( dump, dmode, opts ) ? 0 : 1 );
        }
        if( check_mods ) {
            init_colors();
            loading_ui ui( false );
            const std::vector<mod_id> mods( opts.begin(), opts.end() );
            exit( g->check_mod_data( mods, ui ) && !test_dirty ? 0 : 1 );
        }
    } catch( const std::exception &err ) {
        debugmsg( "%s", err.what() );
        exit_handler( -999 );
    }

    // Now we do the actual game.

    g->init_ui();

    catacurses::curs_set( 0 ); // Invisible cursor here, because MAPBUFFER.load() is crash-prone

#if (!(defined _WIN32 || defined WINDOWS))
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset( &sigIntHandler.sa_mask );
    sigIntHandler.sa_flags = 0;
    sigaction( SIGINT, &sigIntHandler, NULL );
#endif

#ifdef LOCALIZE
    std::string lang;
#if (defined _WIN32 || defined WINDOWS)
    lang = getLangFromLCID( GetUserDefaultLCID() );
#else
    const char *v = setlocale( LC_ALL, NULL );
    if( v != NULL ) {
        lang = v;

        if( lang == "C" ) {
            lang = "en";
        }
    }
#endif
    if( get_option<std::string>( "USE_LANG" ).empty() && ( lang.empty() ||
            !isValidLanguage( lang ) ) ) {
        select_language();
        set_language();
    }
#endif

    while( true ) {
        if( !world.empty() ) {
            if( !g->load( world ) ) {
                break;
            }
            world.clear(); // ensure quit returns to opening screen

        } else {
            main_menu menu;
            if( !menu.opening_screen() ) {
                break;
            }
        }

        while( !g->do_turn() );
    }

    exit_handler( -999 );
    return 0;
}

namespace
{
void printHelpMessage( const arg_handler *first_pass_arguments,
                       size_t num_first_pass_arguments,
                       const arg_handler *second_pass_arguments,
                       size_t num_second_pass_arguments )
{

    // Group all arguments by help_group.
    std::multimap<std::string, const arg_handler *> help_map;
    for( size_t i = 0; i < num_first_pass_arguments; ++i ) {
        std::string help_group;
        if( first_pass_arguments[i].help_group ) {
            help_group = first_pass_arguments[i].help_group;
        }
        help_map.insert( std::make_pair( help_group, &first_pass_arguments[i] ) );
    }
    for( size_t i = 0; i < num_second_pass_arguments; ++i ) {
        std::string help_group;
        if( second_pass_arguments[i].help_group ) {
            help_group = second_pass_arguments[i].help_group;
        }
        help_map.insert( std::make_pair( help_group, &second_pass_arguments[i] ) );
    }

    printf( "Command line parameters:\n" );
    std::string current_help_group;
    auto it = help_map.begin();
    auto it_end = help_map.end();
    for( ; it != it_end; ++it ) {
        if( it->first != current_help_group ) {
            current_help_group = it->first;
            printf( "\n%s\n", current_help_group.c_str() );
        }

        const arg_handler *handler = it->second;
        printf( "%s", handler->flag );
        if( handler->param_documentation ) {
            printf( " %s", handler->param_documentation );
        }
        printf( "\n" );
        if( handler->documentation ) {
            printf( "\t%s\n", handler->documentation );
        }
    }
}
}  // namespace

void exit_handler( int s )
{
    const int old_timeout = inp_mngr.get_timeout();
    inp_mngr.reset_timeout();
    if( s != 2 || query_yn( _( "Really Quit? All unsaved changes will be lost." ) ) ) {
        catacurses::erase(); // Clear screen

        deinitDebug();

        int exit_status = 0;
        g.reset();

        catacurses::endwin();

        exit( exit_status );
    }
    inp_mngr.set_timeout( old_timeout );
}
