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
#include "argparse.h"

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

class pathname_arg_handler : public LambdaArgHandler {
    public:
        pathname_arg_handler(const std::string &name,
            const std::string &param_description,
            const std::string &description,
            const std::string &pathname_key,
            const std::string &help_group = "")
            : LambdaArgHandler(name, param_description, description, [pathname_key](const std::string &param) {
              PATH_INFO::update_pathname(pathname_key.c_str(), param);
              return true;
            }, help_group) {}
};

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

    {
        // Process CLI arguments
        ArgParser parser;
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("seed", "<string of letters and or numbers>",
                "Sets the random number generator's seed value",
                [&seed](const std::string &param){
                  unsigned char const *hash_input = (unsigned char const *)param.c_str();
                  seed = djb2_hash(hash_input);
                  return true;
                }));
        parser.AddHandler(
            std::make_shared<FlagHandler>("jsonverify", "checks the cdda json files", verifyexit));
        parser.AddHandler(
            std::make_shared<FlagHandler>("check-mods", "checks the json files belonging to cdda mods", verifyexit));
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("basepath", "<path>",
                "the base path for the game data subdirectories",
                [](const std::string &param){
                  PATH_INFO::init_base_path(param);
                  PATH_INFO::set_standard_filenames();
                  return true;
                }));

        std::string map_sharing = "Map sharing";
        parser.AddHandler(
            std::make_shared<LambdaFlagHandler>("shared",
                "activates the map-sharing mode",
                [](){
                  MAP_SHARING::setSharing(true);
                  MAP_SHARING::setCompetitive(true);
                  MAP_SHARING::setWorldmenu(false);
                  return true;
                }, map_sharing));
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("username", "<name>",
                "tells the map-sharing code to use this name for your character.",
                [](const std::string &param){
                  MAP_SHARING::setUsername(param);
                  return true;
                }, map_sharing));
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("addadmin", "<username>",
                "tells the map-sharing code to use this name for your character and give you "
                    "access to the cheat functions.",
                [](const std::string &param){
                  MAP_SHARING::addAdmin(param);
                  return true;
                }, map_sharing));
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("adddebugger", "<username>",
                "informs map-sharing code that you're running inside a debugger",
                [](const std::string &param){
                  MAP_SHARING::addDebugger(param);
                  return true;
                }, map_sharing));
        parser.AddHandler(
            std::make_shared<LambdaFlagHandler>("competitive",
                "instructs map-sharing code to disable access to the in-game cheat functions",
                [](){
                  MAP_SHARING::setCompetitive(true);
                  return true;
                }, map_sharing));

        std::string user_directory_group = "User directories";
        parser.AddHandler(
            std::make_shared<LambdaArgHandler>("userdir", "<path>",
                "base path for user-overrides to files from the ./data directory and named below",
                [](const std::string &param){
                  PATH_INFO::init_user_dir(param.c_str());
                  PATH_INFO::set_standard_filenames();
                  return true;
                }, user_directory_group));

        // The following arguments are dependent on one or more of the previous flags and are run
        // in a second pass.
        parser.AddHandler(2,
            std::make_shared<LambdaFlagHandler>("worldmenu",
                "enables the world menu in the map-sharing code",
                [](){
                  MAP_SHARING::setWorldmenu(true);
                  return true;
                }, map_sharing));

        parser.AddHandler(2,
            std::make_shared<LambdaArgHandler>("datadir", "<directory name>",
                "sub directory from which game data is loaded",
                [](const std::string &param){
                  PATH_INFO::update_pathname("datadir", param);
                  PATH_INFO::update_datadir();
                  return true;
                }));

        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("savedir", "<directory name>",
                "subdirectory for game saves", "savedir", user_directory_group));
        parser.AddHandler(2,
            std::make_shared<LambdaArgHandler>("configdir", "<directory name>",
                "subdirectory for game configuration",
                [](const std::string &param){
                  PATH_INFO::update_pathname("config_dir", param);
                  PATH_INFO::update_config_dir();
                  return true;
                }, user_directory_group));
        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("memorialdir", "<directory name>",
                "subdirectory for memorials",
                "memorialdir", user_directory_group));
        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("optionfile", "<filename>",
                "name of the options file within the configdir", "options", user_directory_group));
        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("keymapfile", "<filename>",
                "name of the keymap file within the configdir", "keymap", user_directory_group));
        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("autopickupfile", "<filename>",
                "name of the autopickup options file within the configdir", "autopickup"));
        parser.AddHandler(2,
            std::make_shared<pathname_arg_handler>("motdfile", "<filename>",
                "name of the message of the day file within the motd directory", "motd"));

        if (!parser.ParseArgs(argc, (const char**)argv)) {
            parser.PrintUserOutput();
            exit(1);
        }

        if (parser.ShouldExit()) {
            parser.PrintUserOutput();
            exit(0);
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
