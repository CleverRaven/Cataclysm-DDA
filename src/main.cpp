/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#include "cursesdef.h"
#include <ctime>
#include "game.h"
#include "color.h"
#include "options.h"
#include "mapbuffer.h"
#include "debug.h"
#include "item_factory.h"
#include "monstergenerator.h"
#include <sys/stat.h>
#include <cstdlib>
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
#ifdef ENABLE_LOGGING
    setupDebug();
#endif
    int seed = time(NULL);
    bool verifyexit = false;
    bool check_all_mods = false;
    // set locale to system default
    setlocale(LC_ALL, "");
#ifdef LOCALIZE
    bindtextdomain("cataclysm-dda", "lang/mo");
    bind_textdomain_codeset("cataclysm-dda", "UTF-8");
    textdomain("cataclysm-dda");
#endif

    //args: world seeding only.
    argc--;
    argv++;
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
            verifyexit = true;
        } else if(std::string(argv[0]) == "--check-mods") {
            argc--;
            check_all_mods = true;
        } else { // ignore unknown args.
            argc--;
        }
        argv++;
    }

    // ncurses stuff
    initOptions();
    load_options(); // For getting size options
    initscr(); // Initialize ncurses
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
            debugmsg(error_message.c_str());
        }
        exit_handler(-999);
    }

    // Now we do the actuall game

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

void exit_handler(int s) {
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
            DebugLog() << "main.cpp:exit_handler(): system(\"clear\"): error returned\n";
        }

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
