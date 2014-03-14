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
#include "file_wrapper.h"
#include <sys/stat.h>
#include <cstdlib>
#include <signal.h>
#include <map>
#include "path_info.h"
#ifdef LOCALIZE
#include <libintl.h>
#endif
#include "translations.h"
#if (defined OSX_SDL_FW)
#include "SDL.h"
#elif (defined OSX_SDL_LIBS)
#include "SDL/SDL.h"
#endif
#ifdef __linux__
#include <unistd.h>
#endif // __linux__

void exit_handler(int s);
void set_base_path(std::string path);
void set_user_dir(const char *ud = "");
void set_standard_filenames(void);

std::map<std::string, std::string> FILENAMES; // create map where we will store the FILENAMES

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

    // Set default file paths
#ifdef PREFIX
#define Q(STR) #STR
#define QUOTE(STR) Q(STR)
    set_base_path(std::string(QUOTE(PREFIX)));
#else
    set_base_path("");
#endif
    set_user_dir();
    set_standard_filenames();

    // Process CLI arguments
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
                FILENAMES["base_path"] = std::string(argv[0]);
                set_base_path(std::string(argv[0]));
                set_standard_filenames();
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--datadir") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["datadir"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--savedir") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["savedir"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--optionfile") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["optionfile"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--keymapfile") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["keymapfile"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--autopickupfile") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["autopickupfile"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--motdfile") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["motdfile"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--typeface") {
            argc--;
            argv++;
            if(argc) {
                FILENAMES["typeface"] = std::string(argv[0]);
                argc--;
                argv++;
            }
        } else { // ignore unknown args.
            argc--;
            argv++;
        }
    }

    // set locale to system default
    setlocale(LC_ALL, "");
#ifdef LOCALIZE
    const char *locale_dir;
#ifdef __linux__
    if (!FILENAMES["base_path"].empty()) {
        locale_dir = std::string(FILENAMES["base_path"] + "share/locale").c_str();
    } else {
        locale_dir = "lang/mo";
    }
#else
    locale_dir = "lang/mo";
#endif // __linux__

    bindtextdomain("cataclysm-dda", locale_dir);
    bind_textdomain_codeset("cataclysm-dda", "UTF-8");
    textdomain("cataclysm-dda");
#endif // LOCALIZE

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
        if (!assure_dir_exist(FILENAMES["user_dir"].c_str())) {
            debugmsg("Can't open or create %s. Check permissions.",
                     FILENAMES["user_dir"].c_str());
            exit_handler(-999);
        }
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

void set_base_path(std::string path)
{
    if (!path.empty()) {
        char ch;
        ch = path.at(path.length() - 1);
        if (ch != '/' || ch != '\\') {
            path.push_back('/');
        }
    }

    FILENAMES.insert(std::pair<std::string,std::string>("base_path", path));
}

void set_user_dir(const char *ud)
{
    std::string dir = std::string(ud);

    if (dir.empty()) {
        const char *user_dir;
#if (defined _WIN32 || defined WINDOW)
        user_dir = getenv("LOCALAPPDATA");
        // On Windows userdir without dot
        dir = std::string(user_dir) + "/cataclysm-dda/";
#else
        user_dir = getenv("HOME");
        dir = std::string(user_dir) + "/.cataclysm-dda/";
#endif
    }

    FILENAMES.insert(std::pair<std::string,std::string>("user_dir", dir));
}

void set_standard_filenames(void)
{
    const char *share_dir;
#if !(defined _WIN32 || defined WINDOW)
    if (!FILENAMES["base_path"].empty()) {
        share_dir = "share/cataclysm-dda/";
    } else {
        share_dir = "data/";
    }
#else
    share_dir = "data/";
#endif
    // Shared directories
    FILENAMES.insert(std::pair<std::string,std::string>("gfxdir", FILENAMES["base_path"] + share_dir + "gfx/"));
    FILENAMES.insert(std::pair<std::string,std::string>("luadir", FILENAMES["base_path"] + share_dir + "lua/"));
    // FIXME temporaly datadir == basedir. Should be removed later.
    FILENAMES.insert(std::pair<std::string,std::string>("datadir", FILENAMES["base_path"] + share_dir));
    FILENAMES.insert(std::pair<std::string,std::string>("autoexeclua", FILENAMES["luadir"] + "autoexec.lua"));
    FILENAMES.insert(std::pair<std::string,std::string>("fontdir", FILENAMES["datadir"] + "font/"));
    FILENAMES.insert(std::pair<std::string,std::string>("rawdir", FILENAMES["datadir"] + "raw/"));
    FILENAMES.insert(std::pair<std::string,std::string>("jsondir", FILENAMES["datadir"] + "json/"));
    FILENAMES.insert(std::pair<std::string,std::string>("moddir", FILENAMES["datadir"] + "mods/"));
    FILENAMES.insert(std::pair<std::string,std::string>("recycledir", FILENAMES["datadir"] + "recycling/"));
    FILENAMES.insert(std::pair<std::string,std::string>("namesdir", FILENAMES["datadir"] + "names/"));

    // Shared files
    FILENAMES.insert(std::pair<std::string,std::string>("motd", FILENAMES.find("datadir")->second + "motd"));
    FILENAMES.insert(std::pair<std::string,std::string>("credits", FILENAMES["datadir"] + "credits"));
    // TODO Load localized names
    FILENAMES.insert(std::pair<std::string,std::string>("names", FILENAMES["namesdir"] + "en.json"));
    FILENAMES.insert(std::pair<std::string,std::string>("colors", FILENAMES["rawdir"] + "colors.json"));
    FILENAMES.insert(std::pair<std::string,std::string>("keybindings", FILENAMES["rawdir"] + "keybindings.json"));
    FILENAMES.insert(std::pair<std::string,std::string>("sokoban", FILENAMES["rawdir"] + "sokoban.txt"));
    FILENAMES.insert(std::pair<std::string,std::string>("defaulttilejson", FILENAMES["gfx"] + "tile_config.json"));
    FILENAMES.insert(std::pair<std::string,std::string>("defaulttilepng", FILENAMES["gfx"] + "tinytile.png"));
    FILENAMES.insert(std::pair<std::string,std::string>("mods-dev-default", FILENAMES["moddir"] + "dev-default-mods.json"));

    // User directories
    FILENAMES.insert(std::pair<std::string,std::string>("savedir", FILENAMES["user_dir"] + "save/"));
    FILENAMES.insert(std::pair<std::string,std::string>("memorialdir", FILENAMES["user_dir"] + "memorial/"));
    FILENAMES.insert(std::pair<std::string,std::string>("templatedir", FILENAMES["user_dir"] + "templates/"));

    // User files
    FILENAMES.insert(std::pair<std::string,std::string>("options", FILENAMES["user_dir"] + "options.txt"));
    FILENAMES.insert(std::pair<std::string,std::string>("keymap", FILENAMES["user_dir"] + "keymap.txt"));
    FILENAMES.insert(std::pair<std::string,std::string>("debug", FILENAMES["user_dir"] + "debug.log"));
    FILENAMES.insert(std::pair<std::string,std::string>("fontlist", FILENAMES["user_dir"] + "fontlist.txt"));
    FILENAMES.insert(std::pair<std::string,std::string>("fontdata", FILENAMES["user_dir"] + "FONTDATA"));
    FILENAMES.insert(std::pair<std::string,std::string>("autopickup", FILENAMES["user_dir"] + "auto_pickup.txt"));
    FILENAMES.insert(std::pair<std::string,std::string>("mods-user-default", FILENAMES["user_dir"] + "user-default-mods.json"));
    /*
    FILENAMES.insert(std::pair<std::string,std::string>("mainlua", FILENAMES["datadir"] + "main.lua"));
    FILENAMES.insert(std::pair<std::string,std::string>("typeface", FILENAMES["fontdir"] + "fixedsys.ttf"));
    */
}
