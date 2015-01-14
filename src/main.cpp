/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#include "cursesdef.h"
#include "game.h"
#include "color.h"
#include "options.h"
#include "debug.h"
#include "monstergenerator.h"
#include "filesystem.h"
#include "path_info.h"
#include "mapsharing.h"

#include <ctime>
#include <signal.h>
#ifdef LOCALIZE
#include <libintl.h>
#endif
#include "translations.h"
#if (defined OSX_SDL_FW)
#include "SDL.h"
#elif (defined OSX_SDL_LIBS)
#include "SDL/SDL.h"
#endif

void exit_handler(int s);

#ifdef USE_WINMAIN
int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc = __argc;
    char **argv = __argv;
#else
int main(int argc, char *argv[])
{
#endif
    int seed = time(NULL);
    bool verifyexit = false;
    bool check_all_mods = false;

    // Set default file paths
#ifdef PREFIX
#define Q(STR) #STR
#define QUOTE(STR) Q(STR)
    PATH_INFO::init_base_path(std::string(QUOTE(PREFIX)));
#else
    PATH_INFO::init_base_path("");
#endif

#ifdef USE_HOME_DIR
    PATH_INFO::init_user_dir();
#else
    PATH_INFO::init_user_dir("./");
#endif
    PATH_INFO::set_standard_filenames();

    MAP_SHARING::setDefaults();

    // Process CLI arguments
    int saved_argc = --argc; // skip program name
    char **saved_argv = ++argv;

    while (argc) {
        if(std::string(argv[0]) == "--seed") {
            argc--;
            argv++;
            if(argc) {
                seed = djb2_hash((unsigned char *)argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--jsonverify") {
            argc--;
            argv++;
            verifyexit = true;
        } else if(std::string(argv[0]) == "--check-mods") {
            argc--;
            argv++;
            check_all_mods = true;
        } else if(std::string(argv[0]) == "--basepath") {
            argc--;
            argv++;
            if(argc) {
                PATH_INFO::init_base_path(std::string(argv[0]));
                PATH_INFO::set_standard_filenames();
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--userdir") {
            argc--;
            argv++;
            if (argc) {
                PATH_INFO::init_user_dir( argv[0] );
                PATH_INFO::set_standard_filenames();
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--username") {
            argc--;
            argv++;
            if (argc) {
                MAP_SHARING::setUsername(std::string(argv[0]));
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--addadmin") {
            argc--;
            argv++;
            if (argc) {
                MAP_SHARING::addAdmin(std::string(argv[0]));
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--adddebugger") {
            argc--;
            argv++;
            if (argc) {
                MAP_SHARING::addDebugger(std::string(argv[0]));
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--shared") {
            argc--;
            argv++;
            MAP_SHARING::setSharing(true);
            MAP_SHARING::setCompetitive(true);
            MAP_SHARING::setWorldmenu(false);
        } else if(std::string(argv[0]) == "--competitive") {
            argc--;
            argv++;
            MAP_SHARING::setCompetitive(true);
        } else { // Skipping other options.
            argc--;
            argv++;
        }
    }
    while (saved_argc) {
        // All paths will decrement saved_argc and do not depend on it before this operation, so it is safe to do it in
        // just one place, avoiding repeated code.
        saved_argc--;
        if(std::string(saved_argv[0]) == "--worldmenu") {
            saved_argv++;
            MAP_SHARING::setWorldmenu(true);
        } else if(std::string(saved_argv[0]) == "--datadir") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("datadir", std::string(saved_argv[0]));
                PATH_INFO::update_datadir();
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--savedir") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("savedir", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--configdir") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("config_dir", std::string(saved_argv[0]));
                PATH_INFO::update_config_dir();
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--memorialdir") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("memorialdir", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--optionfile") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("options", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--keymapfile") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("keymap", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--autopickupfile") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("autopickup", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else if(std::string(saved_argv[0]) == "--motdfile") {
            saved_argv++;
            if(saved_argc) {
                PATH_INFO::update_pathname("motd", std::string(saved_argv[0]));
                saved_argc--;
                saved_argv++;
            }
        } else {
            // Unknown arguments are ignored.
            saved_argv++;
        }
    }

    if (!assure_dir_exist(FILENAMES["user_dir"].c_str())) {
        printf("Can't open or create %s. Check permissions.\n",
               FILENAMES["user_dir"].c_str());
        exit(1);
    }

    setupDebug();
    // Options strings loaded with system locale
    initOptions();
    load_options();

    set_language(true);

    if (initscr() == NULL) { // Initialize ncurses
        DebugLog( D_ERROR, DC_ALL ) << "initscr failed!";
        return 1;
    }
    init_interface();
    noecho();  // Don't echo keypresses
    cbreak();  // C-style breaks (e.g. ^C to SIGINT)
    keypad(stdscr, true); // Numpad is numbers
#if !(defined TILES || defined _WIN32 || defined WINDOWS)
    // For tiles or windows, this is handled already in initscr().
    init_colors();
#endif
    // curs_set(0); // Invisible cursor
    set_escdelay(10); // Make escape actually responsive

    std::srand(seed);

    g = new game;
    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if (verifyexit) {
            if(g->game_error()) {
                exit_handler(-999);
            }
            exit_handler(0);
        }
        if (check_all_mods) {
            // Here we load all the mods and check their
            // consistency (both is done in check_all_mod_data).
            g->init_ui();
            popup_nowait("checking all mods");
            g->check_all_mod_data();
            if(g->game_error()) {
                exit_handler(-999);
            }
            // At this stage, the mods (and core game data)
            // are find and we could start playing, but this
            // is only for verifying that stage, so we exit.
            exit_handler(0);
        }
    } catch(std::string &error_message) {
        if(!error_message.empty()) {
            debugmsg("%s", error_message.c_str());
        }
        exit_handler(-999);
    }

    // Now we do the actual game.

    g->init_ui();
    if(g->game_error()) {
        exit_handler(-999);
    }

    curs_set(0); // Invisible cursor here, because MAPBUFFER.load() is crash-prone

#if (!(defined _WIN32 || defined WINDOWS))
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
#endif

    bool quit_game = false;
    do {
        if(!g->opening_screen()) {
            quit_game = true;
        }
        while (!quit_game && !g->do_turn()) ;
        if (g->game_quit() || g->game_error()) {
            quit_game = true;
        }
    } while (!quit_game);


    exit_handler(-999);

    return 0;
}

void exit_handler(int s)
{
    if (s != 2 || query_yn(_("Really Quit? All unsaved changes will be lost."))) {
        erase(); // Clear screen
        endwin(); // End ncurses
        int ret;
#if (defined _WIN32 || defined WINDOWS)
        ret = system("cls"); // Tell the terminal to clear itself
        ret = system("color 07");
#else
        ret = system("clear"); // Tell the terminal to clear itself
#endif
        if (ret != 0) {
            DebugLog( D_ERROR, DC_ALL ) << "system(\"clear\"): error returned: " << ret;
        }
        deinitDebug();

        if(g != NULL) {
            if(g->game_error()) {
                delete g;
                exit(1);
            } else {
                delete g;
                exit(0);
            }
        }
        exit(0);
    }
}
