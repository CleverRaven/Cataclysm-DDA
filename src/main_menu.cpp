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

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
extern worldfactory *world_generator;

void game::print_menu(WINDOW* w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY, bool bShowDDA)
{
    //Clear Lines
    werase(w_open);

    for (int i = 1; i < FULL_SCREEN_WIDTH-1; ++i) {
        mvwputch(w_open, FULL_SCREEN_HEIGHT-2, i, c_white, LINE_OXOX);
    }

    mvwprintz(w_open, FULL_SCREEN_HEIGHT-1, 5, c_red, _("Please report bugs to kevin.granade@gmail.com or post on the forums."));

    int iLine = 0;
    const int iOffsetX1 = 3;
    const int iOffsetX2 = 4;
    const int iOffsetX3 = 18;

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

    print_menu_items(w_open, vMenuItems, iSel, iMenuOffsetY, iMenuOffsetX);

    refresh();
    wrefresh(w_open);
    refresh();
}

void game::print_menu_items(WINDOW* w_in, std::vector<std::string> vItems, int iSel, int iOffsetY, int iOffsetX)
{
    mvwprintz(w_in, iOffsetY, iOffsetX, c_black, "");

    for (int i=0; i < vItems.size(); i++) {
        wprintz(w_in, c_ltgray, "[");
        if (iSel == i) {
            shortcut_print(w_in, h_white, h_white, vItems[i].c_str());
        } else {
            shortcut_print(w_in, c_ltgray, c_white, vItems[i].c_str());
        }
        wprintz(w_in, c_ltgray, "] ");
    }
}

WORLDPTR game::pick_world_to_play()
{
    return world_generator->pick_world();
}

bool game::opening_screen()
{
    std::map<std::string, WORLDPTR> worlds;
    if (world_generator){
        world_generator->set_active_world(NULL);
        worlds = world_generator->get_all_worlds();
    }
    WINDOW* w_background = newwin(TERMY, TERMX, 0, 0);

    werase(w_background);
    wrefresh(w_background);

    WINDOW* w_open = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                            (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    const int iMenuOffsetX = 2;
    int iMenuOffsetY = FULL_SCREEN_HEIGHT-3;

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
    dirent *dp;
    DIR *dir;

    dir = opendir("save");
    if (!dir){
        #if (defined _WIN32 || defined __WIN32__)
            mkdir("save");
        #else
            mkdir("save", 0777);
        #endif
        dir = opendir("save");
    }
    if (!dir) {
        dbg(D_ERROR) << "game:opening_screen: Unable to make save directory.";
        debugmsg("Could not make './save' directory");
        endwin();
        exit(1);
    }
    closedir(dir);

    dir = opendir("data");
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".template") != std::string::npos)
            templates.push_back(tmp.substr(0, tmp.find(".template")));
    }
    closedir(dir);

    int sel1 = 1, sel2 = 1, sel3 = 1, layer = 1;
    InputEvent input;
    int chInput;
    bool start = false;

    // Load MOTD and store it in a string
    // Only load it once, it shouldn't change for the duration of the application being open
    static std::vector<std::string> motd;
    if (motd.empty()){
        std::ifstream motd_file;
        motd_file.open("data/motd");
        if (!motd_file.is_open())
            motd.push_back(_("No message today."));
        else {
            while (!motd_file.eof()) {
                std::string tmp;
                getline(motd_file, tmp);
                if (!tmp.length() || tmp[0] != '#')
                    motd.push_back(tmp);
            }
        }
    }

    // Load Credits and store it in a string
    // Only load it once, it shouldn't change for the duration of the application being open
    static std::vector<std::string> credits;
    if (credits.empty()){
        std::ifstream credits_file;
        credits_file.open("data/credits");
        if (!credits_file.is_open())
            credits.push_back(_("No message today."));
        else {
            while (!credits_file.eof()) {
                std::string tmp;
                getline(credits_file, tmp);
                if (!tmp.length() || tmp[0] != '#')
                    credits.push_back(tmp);
            }
        }
    }

    u = player();

    while(!start) {
        if (layer == 1) {
            print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);

            if (sel1 == 0) { // Print the MOTD.
                for (int i = 0; i < motd.size() && i < 16; i++)
                    mvwprintz(w_open, i + 6, 8, c_ltred, motd[i].c_str());

                wrefresh(w_open);
                refresh();
            } else if (sel1 == 7) { // Print the Credits.
                for (int i = 0; i < credits.size() && i < 16; i++)
                    mvwprintz(w_open, i + 6, 8, c_ltred, credits[i].c_str());

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
                if (sel1 > 0)
                    sel1--;
                else
                    sel1 = 8;
            } else if (chInput == KEY_RIGHT || chInput == 'l') {
                if (sel1 < 8)
                    sel1++;
                else
                    sel1 = 0;
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
                print_menu_items(w_open, vSubItems, sel2, iMenuOffsetY-2, iMenuOffsetX);
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
                    if (sel2 < 0)
                        sel2 = vSubItems.size()-1;
                } if (chInput == KEY_RIGHT || chInput == 'l') {
                    sel2++;
                    if (sel2 >= vSubItems.size())
                        sel2 = 0;
                } else if (chInput == KEY_DOWN || chInput == 'j' || chInput == KEY_ESCAPE) {
                    layer = 1;
                    sel1 = 1;
                }
                if (chInput == KEY_UP || chInput == 'k' || chInput == '\n') {
                    if (sel2 == 0 || sel2 == 2 || sel2 == 3) {
                        setup();
                        if (!u.create((sel2 == 0) ? PLTYPE_CUSTOM : ((sel2 == 2)?PLTYPE_RANDOM : PLTYPE_NOW))) {
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        WORLDPTR world = pick_world_to_play();
                        if (!world){
                            u = player();
                            delwin(w_open);
                            return opening_screen();
                        }else{
                            world_generator->set_active_world(world);
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
                if (world_generator->all_worldnames.empty()){
                    mvwprintz(w_open, iMenuOffsetY - 2, 19 + iMenuOffsetX, c_red, _("No Worlds found!"));
                }else {
                    for (int i = 0; i < world_generator->all_worldnames.size(); ++i){
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19 + iMenuOffsetX, (sel2 == i ? h_white : c_white), world_generator->all_worldnames[i].c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (world_generator->all_worldnames.empty() && (input == DirectionS || input == Confirm)) {
                    layer = 1;
                } else if (input == DirectionS) {
                    if (sel2 > 0)
                        sel2--;
                    else
                        sel2 = world_generator->all_worldnames.size() - 1;
                } else if (input == DirectionN) {
                    if (sel2 < world_generator->all_worldnames.size() - 1)
                        sel2++;
                    else
                        sel2 = 0;
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
                // show options for Create, Destroy, Reset worlds. Create world goes directly to Make World screen. Reset and Destroy ask for world to modify.
                // Reset empties world of everything but options, then makes new world within it.
                // Destroy asks for confirmation, then destroys everything in world and then removes world folder

                // only show reset / destroy world if there is at least one valid world existing!

                int world_subs_to_display = (world_generator->all_worldnames.size() > 0)? vWorldSubItems.size(): 1;
                std::vector<std::string> world_subs;
                int xoffset = 25 + iMenuOffsetX;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for (int i = 0; i < world_subs_to_display; ++i)
                {
                    world_subs.push_back(vWorldSubItems[i]);
                    xlen += vWorldSubItems[i].size()+2; // open and close brackets added
                }
                xlen += world_subs.size() - 1;
                if (world_subs.size() > 1){
                    xoffset -= 6;
                }
                print_menu_items(w_open, world_subs, sel2, yoffset, xoffset - (xlen/4));
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionW)
                {
                    if (sel2 > 0)
                    {
                        --sel2;
                    }
                    else
                    {
                        sel2 = world_subs_to_display - 1;
                    }
                }
                else if (input == DirectionE)
                {
                    if (sel2 < world_subs_to_display - 1)
                    {
                        ++sel2;
                    }
                    else
                    {
                        sel2 = 0;
                    }
                }
                else if (input == DirectionS || input == Cancel)
                {
                    layer = 1;
                }

                if (input == DirectionN || input == Confirm)
                {
                    if (sel2 == 0) // Create world
                    {
                        // Open up world creation screen!
                        if (world_generator->make_new_world())
                        {
                            return opening_screen();
                        }else
                        {
                            layer = 1;
                        }
                    }
                    else if (sel2 == 1 || sel2 == 2) // Delete World | Reset World
                    {
                        layer = 3;
                        sel3 = 0;
                    }
                }
            } else if (sel1 == 4) { // Special game
                std::vector<std::string> special_names;
                int xoffset = 32 + iMenuOffsetX;
                int yoffset = iMenuOffsetY - 2;
                int xlen = 0;
                for (int i = 1; i < NUM_SPECIAL_GAMES; i++) {
                    std::string spec_name = special_game_name(special_game_id(i));
                    special_names.push_back(spec_name);
                    xlen += spec_name.size() + 2;
                }
                xlen += special_names.size()-1;
                print_menu_items(w_open, special_names, sel2, yoffset, xoffset - (xlen/4));

                wrefresh(w_open);
                refresh();
                input = get_input();
                if (input == DirectionW) {
                    if (sel2 > 0)
                        sel2--;
                    else
                        sel2 = NUM_SPECIAL_GAMES - 2;
                } else if (input == DirectionE) {
                    if (sel2 < NUM_SPECIAL_GAMES - 2)
                        sel2++;
                    else
                        sel2 = 0;
                } else if (input == DirectionS || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionN || input == Confirm) {
                    if (sel2 >= 0 && sel2 < NUM_SPECIAL_GAMES - 1) {
                        delete gamemode;
                        gamemode = get_special_game( special_game_id(sel2+1) );
                        // check world
                        WORLDPTR world = world_generator->make_new_world(special_game_id(sel2 + 1));

                        if (world)
                        {
                            world_generator->set_active_world(world);
                            setup();
                        }

                        if (world == NULL || !gamemode->init()) {
                            delete gamemode;
                            gamemode = new special_game;
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
            if (sel1 == 2){ // Load Game
                savegames = world_generator->all_worlds[world_generator->all_worldnames[sel2]]->world_saves;
                if (savegames.empty()){
                    mvwprintz(w_open, iMenuOffsetY - 2, 19+19 + iMenuOffsetX, c_red, _("No save games found!"));
                }else{
                    for (int i = 0; i < savegames.size(); i++) {
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19+19 + iMenuOffsetX, (sel3 == i ? h_white : c_white), base64_decode(savegames[i]).c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (savegames.size() == 0 && (input == DirectionS || input == Confirm)) {
                    layer = 2;
                } else if (input == DirectionS) {
                    if (sel3 > 0)
                        sel3--;
                    else
                        sel3 = savegames.size() - 1;
                } else if (input == DirectionN) {
                    if (sel3 < savegames.size() - 1)
                        sel3++;
                    else
                        sel3 = 0;
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

                        load_artifacts(world->world_path + "/artifacts.gsav",
                                       itypes);
                        MAPBUFFER.load(world->world_name);
                        setup();

                        load(world->world_name, savegames[sel3]);
                        start = true;
                    }
                 }
            }else if (sel1 == 3)
            {
                // show world names
                int i = 0;
                for (std::vector<std::string>::iterator it = world_generator->all_worldnames.begin();
                     it != world_generator->all_worldnames.end();
                     ++it)
                {
                    int line = iMenuOffsetY - 4 - i;
                    mvwprintz(w_open, line, 26+iMenuOffsetX, (sel3 == i ? h_white : c_white), (*it).c_str());
                    ++i;
                }
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionS) {
                    if (sel3 > 0)
                        --sel3;
                    else
                        sel3 = world_generator->all_worldnames.size() - 1;
                } else if (input == DirectionN) {
                    if (sel3 < world_generator->all_worldnames.size() - 1)
                        ++sel3;
                    else
                        sel3 = 0;
                } else if (input == DirectionW || input == Cancel) {
                    layer = 2;

                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel3 >= 0 && sel3 < world_generator->all_worldnames.size()) {
                        bool query_yes = false;
                        bool do_delete = false;
                        if (sel2 == 1){ // Delete World
                            if (query_yn(_("Delete the world and all saves?"))) {
                                query_yes = true;
                                do_delete = true;
                            }
                        }
                        else if (sel2 == 2){ // Reset World
                            if (query_yn(_("Remove all saves and regenerate world?"))) {
                                query_yes = true;
                                do_delete = false;
                            }
                        }

                        if (query_yes){
                            delete_world(world_generator->all_worldnames[sel3], do_delete);

                            savegames.clear();
                            MAPBUFFER.reset();
                            MAPBUFFER.make_volatile();
                            overmap_buffer.clear();

                            layer = 2;

                            if (do_delete){
                                // delete world and all contents
                                world_generator->remove_world(world_generator->all_worldnames[sel3]);
                            }else{
                                // clear out everything but worldoptions from this world
                                world_generator->all_worlds[world_generator->all_worldnames[sel3]]->world_saves.clear();
                            }
                            if (world_generator->all_worldnames.size() == 0){
                                sel2 = 0; // reset to create world selection
                            }
                        }else{
                            // hacky resolution to the issue of persisting world names on the screen
                            return opening_screen();
                        }
                    }
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
            }else{ // Character Templates
                if (templates.size() == 0){
                    mvwprintz(w_open, iMenuOffsetY-4, iMenuOffsetX+20, c_red, _("No templates found!"));
                }else {
                    for (int i = 0; i < templates.size(); i++) {
                        int line = iMenuOffsetY - 4 - i;
                        mvwprintz(w_open, line, 20 + iMenuOffsetX, (sel3 == i ? h_white : c_white), templates[i].c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (input == DirectionS) {
                    if (sel3 > 0)
                        sel3--;
                    else
                        sel3 = templates.size() - 1;
                } else if (templates.size() == 0 && (input == DirectionN || input == Confirm)) {
                    sel1 = 1;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                } else if (input == DirectionN) {
                    if (sel3 < templates.size() - 1)
                        sel3++;
                    else
                        sel3 = 0;
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
                    WORLDPTR world = pick_world_to_play();
                    if (!world){
                        u = player();
                        delwin(w_open);
                        return (opening_screen());
                    }else{
                        world_generator->set_active_world(world);
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
