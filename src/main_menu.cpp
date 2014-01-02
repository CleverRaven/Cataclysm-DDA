#include "game.h"
#include "keypress.h"
#include "debug.h"
#include "input.h"
#include "mapbuffer.h"
#include "cursesdef.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "catacharset.h"
#include "get_version.h"
#include "help.h"
#include "options.h"
#include "worldfactory.h"

#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

// Template are disabled, as they might need
// mods to be loaded to work correctly (professions can be
// part of a mod)
#define ENABLE_CHAR_TEMPLATES 0

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
extern worldfactory *world_generator;

void game::print_menu(WINDOW *w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY,
                      bool bShowDDA)
{
    // Clear Lines
    werase(w_open);

    // Define window size
    int window_width = getmaxx(w_open);
    int window_height = getmaxy(w_open);

    // Draw horizontal line
    for (int i = 1; i < window_width - 1; ++i) {
        mvwputch(w_open, window_height - 2, i, c_white, LINE_OXOX);
    }

    center_print(w_open, window_height - 1, c_red,
                 _("Please report bugs to kevin.granade@gmail.com or post on the forums."));

    int iLine = 0;
    const int iOffsetX1 = 3 + (window_width - FULL_SCREEN_WIDTH) / 2;
    const int iOffsetX2 = 4 + (window_width - FULL_SCREEN_WIDTH) / 2;
    const int iOffsetX3 = 18 + (window_width - FULL_SCREEN_WIDTH) / 2;

    const nc_color cColor1 = c_ltcyan;
    const nc_color cColor2 = c_ltblue;
    const nc_color cColor3 = c_ltblue;

    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "_________            __                   .__                            ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "\\_   ___ \\ _____   _/  |_ _____     ____  |  |   ___.__   ______  _____  ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "/    \\  \\/ \\__  \\  \\   __\\\\__  \\  _/ ___\\ |  |  <   |  | /  ___/ /     \\ ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "\\     \\____ / __ \\_ |  |   / __ \\_\\  \\___ |  |__ \\___  | \\___ \\ |  Y Y  \\");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, " \\______  /(____  / |__|  (____  / \\___  >|____/ / ____|/____  >|__|_|  /");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "        \\/      \\/             \\/      \\/        \\/          \\/       \\/ ");

    if (bShowDDA) {
        if (FULL_SCREEN_HEIGHT > 24) {
            ++iLine;
        }
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "________                   .__      ________                           ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "\\______ \\  _____   _______ |  | __  \\______ \\  _____    ___.__   ______");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, " |    |  \\ \\__  \\  \\_  __ \\|  |/ /   |    |  \\ \\__  \\  <   |  | /  ___/");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, " |    `   \\ / __ \\_ |  | \\/|    <    |    `   \\ / __ \\_ \\___  | \\___ \\ ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "/_______  /(____  / |__|   |__|_ \\  /_______  /(____  / / ____|/____  >");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "        \\/      \\/              \\/          \\/      \\/  \\/          \\/ ");

        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "   _____   .__                         .___");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "  /  _  \\  |  |__    ____  _____     __| _/");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, " /  /_\\  \\ |  |  \\ _/ __ \\ \\__  \\   / __ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "/    |    \\|   Y  \\\\  ___/  / __ \\_/ /_/ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "\\____|__  /|___|  / \\___  >(____  /\\____ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "        \\/      \\/      \\/      \\/      \\/ ");
        iLine++;
        center_print(w_open, iLine++, cColor3, _("Version: %s"), getVersionString());
    }

    std::vector<std::string> vMenuItems;
    vMenuItems.push_back(pgettext("Main Menu", "<M>OTD"));
    vMenuItems.push_back(pgettext("Main Menu", "<N>ew Game"));
    vMenuItems.push_back(pgettext("Main Menu", "Lo<a>d"));
    vMenuItems.push_back(pgettext("Main Menu", "<W>orld"));
    vMenuItems.push_back(pgettext("Main Menu", "<S>pecial"));
    vMenuItems.push_back(pgettext("Main Menu", "<O>ptions"));
    vMenuItems.push_back(pgettext("Main Menu", "H<e>lp"));
    vMenuItems.push_back(pgettext("Main Menu", "<C>redits"));
    vMenuItems.push_back(pgettext("Main Menu", "<Q>uit"));

    int menu_length = 0;
    for (int pos = 0; pos < vMenuItems.size(); pos++) {
        // adds (width + 2) if there are no shortcut symbols "<" & ">", and just width otherwise
        menu_length += utf8_width(vMenuItems[pos].c_str()) +
                       (vMenuItems[pos].find_first_of("<") == std::string::npos ? 2 : 0);
    }
    int spacing = (window_width - menu_length) / vMenuItems.size() - 1;
    spacing = (spacing < 1 ? 1 : spacing);
    const int adj_offset = (window_width - menu_length - spacing * vMenuItems.size() ) / 2 - 1;
    print_menu_items(w_open, vMenuItems, iSel, iMenuOffsetY, iMenuOffsetX + adj_offset, spacing);

    refresh();
    wrefresh(w_open);
    refresh();
}

void game::print_menu_items(WINDOW *w_in, std::vector<std::string> vItems, int iSel, int iOffsetY,
                            int iOffsetX, int spacing)
{
    mvwprintz(w_in, iOffsetY, iOffsetX, c_black, "");

    for (int i = 0; i < vItems.size(); i++) {
        wprintz(w_in, c_ltgray, "[");
        if (iSel == i) {
            shortcut_print(w_in, h_white, h_white, vItems[i].c_str());
        } else {
            shortcut_print(w_in, c_ltgray, c_white, vItems[i].c_str());
        }
        wprintz(w_in, c_ltgray, "]");
        for (int j = 0; j < spacing; j++) {
            wprintz(w_in, c_ltgray, " ");
        }
    }
}

bool assure_dir_exist(const std::string &path) {
    DIR *dir = opendir(path.c_str());
    if (dir != NULL) {
        closedir(dir);
        return true;
    }
#if (defined _WIN32 || defined __WIN32__)
    return (mkdir("save") == 0);
#else
    return (mkdir("save", 0777) == 0);
#endif
}

bool game::opening_screen()
{
    world_generator->set_active_world(NULL);
    // This actually _loads_ what worlds exist.
    world_generator->get_all_worlds();

    WINDOW *w_background = newwin(TERMY, TERMX, 0, 0);
    werase(w_background);
    wrefresh(w_background);

    // main window should also expand to use available display space.
    // expanding to evenly use up half of extra space, for now.
    int extra_w = ((TERMX - FULL_SCREEN_WIDTH) / 2) - 1;
    int extra_h = ((TERMY - FULL_SCREEN_HEIGHT) / 2) - 1;
    extra_w = (extra_w > 0 ? extra_w : 0);
    extra_h = (extra_h > 0 ? extra_h : 0);
    const int total_w = FULL_SCREEN_WIDTH + extra_w;
    const int total_h = FULL_SCREEN_HEIGHT + extra_h;

    // position of window within main display
    const int x0 = (TERMX - total_w) / 2;
    const int y0 = (TERMY - total_h) / 2;

    WINDOW *w_open = newwin(total_h, total_w, y0, x0);

    const int iMenuOffsetX = 2;
    int iMenuOffsetY = total_h - 3;

    std::vector<std::string> vSubItems;
    vSubItems.push_back(pgettext("Main Menu|New Game", "<C>ustom Character"));
    vSubItems.push_back(pgettext("Main Menu|New Game", "<P>reset Character"));
    vSubItems.push_back(pgettext("Main Menu|New Game", "<R>andom Character"));
    vSubItems.push_back(pgettext("Main Menu|New Game", "Play <N>ow!"));

    std::vector<std::string> vWorldSubItems;
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<C>reate World"));
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<D>elete World"));
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<R>eset World"));

    print_menu(w_open, 0, iMenuOffsetX, iMenuOffsetY);

    std::vector<std::string> savegames, templates;
#if ENABLE_CHAR_TEMPLATES
    dirent *dp;
    DIR *dir;
#endif

    if (!assure_dir_exist("save")) {
        popup(_("Unable to make save directory. Check permissions."));
        return false;
    }

#if ENABLE_CHAR_TEMPLATES
    dir = opendir("data");
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".template") != std::string::npos) {
            templates.push_back(tmp.substr(0, tmp.find(".template")));
        }
    }
    closedir(dir);
#endif

    int sel1 = 1, sel2 = 1, sel3 = 1, layer = 1;
    InputEvent input;
    int chInput;
    bool start = false;

    // Load MOTD and store it in a string
    // Only load it once, it shouldn't change for the duration of the application being open
    static std::vector<std::string> motd;
    if (motd.empty()) {
        std::ifstream motd_file;
        motd_file.open("data/motd");
        if (!motd_file.is_open()) {
            motd.push_back(_("No message today."));
        } else {
            while (!motd_file.eof()) {
                std::string tmp;
                getline(motd_file, tmp);
                if (!tmp.length() || tmp[0] != '#') {
                    motd.push_back(tmp);
                }
            }
        }
    }

    // Load Credits and store it in a string
    // Only load it once, it shouldn't change for the duration of the application being open
    static std::vector<std::string> credits;
    if (credits.empty()) {
        std::ifstream credits_file;
        credits_file.open("data/credits");
        if (!credits_file.is_open()) {
            credits.push_back(_("No message today."));
        } else {
            while (!credits_file.eof()) {
                std::string tmp;
                getline(credits_file, tmp);
                if (!tmp.length() || tmp[0] != '#') {
                    credits.push_back(tmp);
                }
            }
        }
    }

    u = player();

    while(!start) {
        if (layer == 1) {
            print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);

            if (sel1 == 0) { // Print the MOTD.
                for (int i = 0; i < motd.size() && i < 16; i++) {
                    mvwprintz(w_open, i + 6, 8 + extra_w / 2, c_ltred, motd[i].c_str());
                }

                wrefresh(w_open);
                refresh();
            } else if (sel1 == 7) { // Print the Credits.
                for (int i = 0; i < credits.size() && i < 16; i++) {
                    mvwprintz(w_open, i + 6, 8 + extra_w / 2, c_ltred, credits[i].c_str());
                }

                wrefresh(w_open);
                refresh();
            }

            chInput = getch();

            if (chInput == 'm' || chInput == 'M') {
                // MOTD
                sel1 = 0;
                chInput = '\n';
            } else if (chInput == 'n' || chInput == 'N') {
                // New Game
                sel1 = 1;
                chInput = '\n';
            } else if (chInput == 'a' || chInput == 'A') {
                // Load Game
                sel1 = 2;
                chInput = '\n';
            } else if (chInput == 'w' || chInput == 'W') {
                // World
                sel1 = 3;
                chInput = '\n';
            } else if (chInput == 's' || chInput == 'S') {
                // Special Game
                sel1 = 4;
                chInput = '\n';
            } else if (chInput == 'o' || chInput == 'O') {
                // Options
                sel1 = 5;
                chInput = '\n';
            } else if (chInput == 'e' || chInput == 'E' || chInput == '?') {
                // Help
                sel1 = 6;
                chInput = '\n';
            } else if (chInput == 'c' || chInput == 'C') {
                // Credits
                sel1 = 7;
                chInput = '\n';
            } else if (chInput == 'q' || chInput == 'Q' || chInput == KEY_ESCAPE) {
                // Quit
                sel1 = 8;
                chInput = '\n';
            }

            if (chInput == KEY_LEFT || chInput == 'h') {
                if (sel1 > 0) {
                    sel1--;
                } else {
                    sel1 = 8;
                }
            } else if (chInput == KEY_RIGHT || chInput == 'l') {
                if (sel1 < 8) {
                    sel1++;
                } else {
                    sel1 = 0;
                }
            } else if ((chInput == KEY_UP || chInput == 'k' || chInput == '\n') && sel1 > 0 && sel1 != 7) {
                if (sel1 == 5) {
                    show_options();
                } else if (sel1 == 6) {
                    display_help();
                } else if (sel1 == 8) {
                    uquit = QUIT_MENU;
                    delwin(w_open);
                    delwin(w_background);
                    return false;
                } else {
                    sel2 = 0;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);
                }
            }
        } else if (layer == 2) {
            if (sel1 == 1) { // New Character
                print_menu_items(w_open, vSubItems, sel2, iMenuOffsetY - 2, iMenuOffsetX);
                wrefresh(w_open);
                refresh();
                chInput = getch();

                if (chInput == 'c' || chInput == 'C') {
                    sel2 = 0;
                    chInput = '\n'  ;
                } else if (chInput == 'p' || chInput == 'P') {
                    sel2 = 1;
                    chInput = '\n';
                } else if (chInput == 'r' || chInput == 'R') {
                    sel2 = 2;
                    chInput = '\n';
                } else if (chInput == 'n' || chInput == 'N') {
                    sel2 = 3;
                    chInput = '\n';
                }

                if (chInput == KEY_LEFT || chInput == 'h') {
                    sel2--;
                    if (sel2 < 0) {
                        sel2 = vSubItems.size() - 1;
                    }
                }
                if (chInput == KEY_RIGHT || chInput == 'l') {
                    sel2++;
                    if (sel2 >= vSubItems.size()) {
                        sel2 = 0;
                    }
                } else if (chInput == KEY_DOWN || chInput == 'j' || chInput == KEY_ESCAPE) {
                    layer = 1;
                    sel1 = 1;
                }
                if (chInput == KEY_UP || chInput == 'k' || chInput == '\n') {
                    if (sel2 == 0 || sel2 == 2 || sel2 == 3) {
                        // First load the mods, this is done by
                        // loading the world.
                        // Pick a world, supressing prompts if it's "play now" mode.
                        WORLDPTR world = world_generator->pick_world( sel2 != 3 );
                        if (world == NULL) {
                            // TODO: makes this into a simple continue
                            delwin(w_open);
                            return opening_screen();
                        }
                        world_generator->set_active_world(world);
                        load_world_modfiles(world->world_name);

                        setup();
                        if (!u.create((sel2 == 0) ? PLTYPE_CUSTOM :
                                                    ((sel2 == 2) ? PLTYPE_RANDOM : PLTYPE_NOW))) {
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        if (!u.create((sel2 == 0) ? PLTYPE_CUSTOM : ((sel2 == 2)?PLTYPE_RANDOM : PLTYPE_NOW))) {
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        werase(w_background);
                        wrefresh(w_background);

                        load_artifacts(world->world_path + "/artifacts.gsav",
                                       itypes);
                        MAPBUFFER.load(world->world_name);
                        start_game(world->world_name);
                        start = true;
                    } else if (sel2 == 1) {
                        layer = 3;
                        sel3 = 0;
                    }
                }
            } else if (sel1 == 2) { // Load Character
                if (world_generator->all_worldnames.empty()) {
                    mvwprintz(w_open, iMenuOffsetY - 2, 15 + iMenuOffsetX + extra_w / 2,
                              c_red, _("No Worlds found!"));
                } else {
                    for (int i = 0; i < world_generator->all_worldnames.size(); ++i) {
                      int line = iMenuOffsetY - 2 - i;
                      std::string world_name = world_generator->all_worldnames[i];
                      int savegames_count = world_generator->all_worlds[world_name]->world_saves.size();
                      mvwprintz(w_open, line, 15 + iMenuOffsetX + extra_w / 2,
                                (sel2 == i ? h_white : c_white), "%s (%d)", world_name.c_str(), savegames_count);
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (world_generator->all_worldnames.empty() && (input == DirectionS || input == Confirm)) {
                    layer = 1;
                } else if (input == DirectionS) {
                    if (sel2 > 0) {
                        sel2--;
                    } else {
                        sel2 = world_generator->all_worldnames.size() - 1;
                    }
                } else if (input == DirectionN) {
                    if (sel2 < world_generator->all_worldnames.size() - 1) {
                        sel2++;
                    } else {
                        sel2 = 0;
                    }
                } else if (input == DirectionW || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel2 >= 0 && sel2 < world_generator->all_worldnames.size()) {
                        layer = 3;
                        sel3 = 0;
                    }
                }
            } else if (sel1 == 3) {  // World Menu
                // Show options for Create, Destroy, Reset worlds.
                // Create world goes directly to Make World screen.
                // Reset and Destroy ask for world to modify.
                // Reset empties world of everything but options, then makes new world within it.
                // Destroy asks for confirmation, then destroys everything in world and then removes world folder.

                // only show reset / destroy world if there is at least one valid world existing!

                int world_subs_to_display = (world_generator->all_worldnames.size() > 0)? vWorldSubItems.size(): 1;
                std::vector<std::string> world_subs;
                int xoffset = 25 + iMenuOffsetX + extra_w / 2;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for (int i = 0; i < world_subs_to_display; ++i) {
                    world_subs.push_back(vWorldSubItems[i]);
                    xlen += vWorldSubItems[i].size() + 2; // Open and close brackets added
                }
                xlen += world_subs.size() - 1;
                if (world_subs.size() > 1) {
                    xoffset -= 6;
                }
                print_menu_items(w_open, world_subs, sel2, yoffset, xoffset - (xlen / 4));
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionW) {
                    if (sel2 > 0) {
                        --sel2;
                    } else {
                        sel2 = world_subs_to_display - 1;
                    }
                } else if (input == DirectionE) {
                    if (sel2 < world_subs_to_display - 1) {
                        ++sel2;
                    } else {
                        sel2 = 0;
                    }
                } else if (input == DirectionS || input == Cancel) {
                    layer = 1;
                }

                if (input == DirectionN || input == Confirm) {
                    if (sel2 == 0) { // Create world
                        // Open up world creation screen!
                        if (world_generator->make_new_world()) {
                            return opening_screen();
                        } else {
                            layer = 1;
                        }
                    } else if (sel2 == 1 || sel2 == 2) { // Delete World | Reset World
                        layer = 3;
                        sel3 = 0;
                    }
                }
            } else if (sel1 == 4) { // Special game
                std::vector<std::string> special_names;
                int xoffset = 32 + iMenuOffsetX  + extra_w / 2;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for (int i = 1; i < NUM_SPECIAL_GAMES; i++) {
                    std::string spec_name = special_game_name(special_game_id(i));
                    special_names.push_back(spec_name);
                    xlen += spec_name.size() + 2;
                }
                xlen += special_names.size() - 1;
                print_menu_items(w_open, special_names, sel2, yoffset, xoffset - (xlen / 4));

                wrefresh(w_open);
                refresh();
                input = get_input();
                if (input == DirectionW) {
                    if (sel2 > 0) {
                        sel2--;
                    } else {
                        sel2 = NUM_SPECIAL_GAMES - 2;
                    }
                } else if (input == DirectionE) {
                    if (sel2 < NUM_SPECIAL_GAMES - 2) {
                        sel2++;
                    } else {
                        sel2 = 0;
                    }
                } else if (input == DirectionS || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionN || input == Confirm) {
                    if (sel2 >= 0 && sel2 < NUM_SPECIAL_GAMES - 1) {
                        delete gamemode;
                        gamemode = get_special_game( special_game_id(sel2 + 1) );
                        // check world
                        WORLDPTR world = world_generator->make_new_world(special_game_id(sel2 + 1));

                        if (world) {
                            world_generator->set_active_world(world);
                            setup();
                            load_world_modfiles(world->world_name);
                        }

                        if (world == NULL || !gamemode->init()) {
                            delete gamemode;
                            gamemode = NULL;
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        load_artifacts(world->world_path + "/artifacts.gsav",
                                       itypes);
                        start = true;
                    }
                }
            }
        } else if (layer == 3) {
            if (sel1 == 2) { // Load Game
                savegames = world_generator->all_worlds[world_generator->all_worldnames[sel2]]->world_saves;
                if (savegames.empty()) {
                    mvwprintz(w_open, iMenuOffsetY - 2, 19 + 19 + iMenuOffsetX + extra_w / 2,
                              c_red, _("No save games found!"));
                } else {
                    for (int i = 0; i < savegames.size(); i++) {
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19 + 19 + iMenuOffsetX + extra_w / 2,
                                  (sel3 == i ? h_white : c_white),
                                  base64_decode(savegames[i]).c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (savegames.size() == 0 && (input == DirectionS || input == Confirm)) {
                    layer = 2;
                } else if (input == DirectionS) {
                    if (sel3 > 0) {
                        sel3--;
                    } else {
                        sel3 = savegames.size() - 1;
                    }
                } else if (input == DirectionN) {
                    if (sel3 < savegames.size() - 1) {
                        sel3++;
                    } else {
                        sel3 = 0;
                    }
                } else if (input == DirectionW || input == Cancel) {
                    layer = 2;
                    sel3 = 0;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel3 >= 0 && sel3 < savegames.size()) {
                        werase(w_background);
                        wrefresh(w_background);
                        WORLDPTR world = world_generator->all_worlds[world_generator->all_worldnames[sel2]];
                        world_generator->set_active_world(world);
                        load_world_modfiles(world->world_name);

                        load_artifacts(world->world_path + "/artifacts.gsav",
                                       itypes);
                        MAPBUFFER.load(world->world_name);
                        setup();

                        load(world->world_name, savegames[sel3]);
                        start = true;
                    }
                }
            } else if (sel1 == 3) { // Show world names
                int i = 0;
                for (std::vector<std::string>::iterator it = world_generator->all_worldnames.begin();
                     it != world_generator->all_worldnames.end(); ++it) {
                    int savegames_count = world_generator->all_worlds[*it]->world_saves.size();
                    int line = iMenuOffsetY - 4 - i;
                    mvwprintz(w_open, line, 26 + iMenuOffsetX + extra_w / 2,
                              (sel3 == i ? h_white : c_white), "%s (%d)", (*it).c_str(), savegames_count);
                    ++i;
                }
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionS) {
                    if (sel3 > 0) {
                        --sel3;
                    } else {
                        sel3 = world_generator->all_worldnames.size() - 1;
                    }
                } else if (input == DirectionN) {
                    if (sel3 < world_generator->all_worldnames.size() - 1) {
                        ++sel3;
                    } else {
                        sel3 = 0;
                    }
                } else if (input == DirectionW || input == Cancel) {
                    layer = 2;

                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel3 >= 0 && sel3 < world_generator->all_worldnames.size()) {
                        bool query_yes = false;
                        bool do_delete = false;
                        if (sel2 == 1) { // Delete World
                            if (query_yn(_("Delete the world and all saves?"))) {
                                query_yes = true;
                                do_delete = true;
                            }
                        } else if (sel2 == 2) { // Reset World
                            if (query_yn(_("Remove all saves and regenerate world?"))) {
                                query_yes = true;
                                do_delete = false;
                            }
                        }

                        if (query_yes) {
                            delete_world(world_generator->all_worldnames[sel3], do_delete);

                            savegames.clear();
                            MAPBUFFER.reset();
                            MAPBUFFER.make_volatile();
                            overmap_buffer.clear();

                            layer = 2;

                            if (do_delete) {
                                // delete world and all contents
                                world_generator->remove_world(world_generator->all_worldnames[sel3]);
                            } else {
                                // clear out everything but worldoptions from this world
                                world_generator->all_worlds[world_generator->all_worldnames[sel3]]->world_saves.clear();
                            }
                            if (world_generator->all_worldnames.size() == 0) {
                                sel2 = 0; // reset to create world selection
                            }
                        } else {
                            // hacky resolution to the issue of persisting world names on the screen
                            return opening_screen();
                        }
                    }
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
            } else { // Character Templates
                if (templates.size() == 0) {
                    mvwprintz(w_open, iMenuOffsetY - 4, iMenuOffsetX + 20 + extra_w / 2,
                              c_red, _("No templates found!"));
                } else {
                    for (int i = 0; i < templates.size(); i++) {
                        int line = iMenuOffsetY - 4 - i;
                        mvwprintz(w_open, line, 20 + iMenuOffsetX + extra_w / 2,
                                  (sel3 == i ? h_white : c_white), templates[i].c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (input == DirectionS) {
                    if (sel3 > 0) {
                        sel3--;
                    } else {
                        sel3 = templates.size() - 1;
                    }
                } else if (templates.size() == 0 && (input == DirectionN || input == Confirm)) {
                    sel1 = 1;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                } else if (input == DirectionN) {
                    if (sel3 < templates.size() - 1) {
                        sel3++;
                    } else {
                        sel3 = 0;
                    }
                } else if (input == DirectionW  || input == Cancel || templates.size() == 0) {
                    sel1 = 1;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                } else if (input == DirectionE || input == Confirm) {
                    setup();
                    if (!u.create(PLTYPE_TEMPLATE, templates[sel3])) {
                        u = player();
                        delwin(w_open);
                        return (opening_screen());
                    }
                    // check world
                    WORLDPTR world = world_generator->pick_world();
                    if (!world) {
                        u = player();
                        delwin(w_open);
                        return (opening_screen());
                    } else {
                        world_generator->set_active_world(world);
                        load_world_modfiles(world->world_name);
                    }
                    if (!u.create(PLTYPE_TEMPLATE, templates[sel3])) {
                        u = player();
                        delwin(w_open);
                        return (opening_screen());
                    }
                    werase(w_background);
                    wrefresh(w_background);

                    std::string artfilename = world_generator->active_world->world_path + "/artifacts.gsav";
                    load_artifacts(artfilename, itypes);
                    MAPBUFFER.load(world_generator->active_world->world_name);

                    start_game(world_generator->active_world->world_name);
                    start = true;
                }
            }
        }
    }
    delwin(w_open);
    delwin(w_background);
    if (start == false) {
        uquit = QUIT_MENU;
    } else {
        refresh_all();
        draw();
    }
    return start;
}
