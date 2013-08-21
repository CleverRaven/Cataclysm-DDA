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

#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

void game::print_menu(WINDOW* w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY, bool bShowDDA)
{
    //Clear Lines
    werase(w_open);

    for (int i = 1; i < 79; i++)
        mvwputch(w_open, 23, i, c_white, LINE_OXOX);

    mvwprintz(w_open, 24, 5, c_red, _("Please report bugs to kevin.granade@gmail.com or post on the forums."));

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
        iLine++;
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "________                   .__      ________                           ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "\\______ \\  _____   _______ |  | __  \\______ \\  _____    ___.__   ______");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, " |    |  \\ \\__  \\  \\_  __ \\|  |/ /   |    |  \\ \\__  \\  <   |  | /  ___/");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, " |    `   \\ / __ \\_ |  | \\/|    <    |    `   \\ / __ \\_ \\___  | \\___ \\ ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "/_______  /(____  / |__|   |__|_ \\  /_______  /(____  / / ____|/____  >");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "        \\/      \\/              \\/          \\/      \\/  \\/          \\/ ");

        iLine++;
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "   _____   .__                         .___");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "  /  _  \\  |  |__    ____  _____     __| _/");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, " /  /_\\  \\ |  |  \\ _/ __ \\ \\__  \\   / __ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "/    |    \\|   Y  \\\\  ___/  / __ \\_/ /_/ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "\\____|__  /|___|  / \\___  >(____  /\\____ | ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "        \\/      \\/      \\/      \\/      \\/ ");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "Version: %s",getVersionString());
    }

    std::vector<std::string> vMenuItems;
    vMenuItems.push_back(_("<M>OTD"));
    vMenuItems.push_back(_("<N>ew Game"));
    vMenuItems.push_back(_("<L>oad"));
    vMenuItems.push_back(_("<R>eset"));
    vMenuItems.push_back(_("<S>pecial"));
    vMenuItems.push_back(_("<O>ptions"));
    vMenuItems.push_back(_("<H>elp"));
    vMenuItems.push_back(_("<C>redits"));
    vMenuItems.push_back(_("<Q>uit"));

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

bool game::opening_screen()
{
    WINDOW* w_background = newwin(TERMY, TERMX, 0, 0);

    werase(w_background);
    wrefresh(w_background);

    WINDOW* w_open = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                            (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    const int iMenuOffsetX = 2;
    int iMenuOffsetY = FULL_SCREEN_HEIGHT-3;

    std::vector<std::string> vSubItems;
    vSubItems.push_back(_("<C>ustom Character"));
    vSubItems.push_back(_("<P>reset Character"));
    vSubItems.push_back(_("<R>andom Character"));
    vSubItems.push_back(_("Play <N>ow!"));

    print_menu(w_open, 0, iMenuOffsetX, iMenuOffsetY);

    std::vector<std::string> savegames, templates;
    dirent *dp;
    DIR *dir = opendir("save");
    if (!dir) {
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
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".sav") != std::string::npos)
            savegames.push_back(tmp.substr(0, tmp.find(".sav")));
    }
    closedir(dir);
    dir = opendir("data");
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".template") != std::string::npos)
            templates.push_back(tmp.substr(0, tmp.find(".template")));
    }

    int sel1 = 1, sel2 = 1, layer = 1;
    InputEvent input;
    int chInput;
    bool start = false;

    // Load MOTD and store it in a string
    std::vector<std::string> motd;
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

    // Load Credits and store it in a string
    std::vector<std::string> credits;
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

    while(!start) {
        if (layer == 1) {
            print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);

            if (sel1 == 0) {	// Print the MOTD.
                for (int i = 0; i < motd.size() && i < 16; i++)
                    mvwprintz(w_open, i + 7, 8, c_ltred, motd[i].c_str());

                wrefresh(w_open);
                refresh();
            } else if (sel1 == 7) {	// Print the Credits.
                for (int i = 0; i < credits.size() && i < 16; i++)
                    mvwprintz(w_open, i + 7, 8, c_ltred, credits[i].c_str());

                wrefresh(w_open);
                refresh();
            }

            chInput = getch();

            if (chInput == 'm' || chInput == 'M') {
                sel1 = 0;
                chInput = '\n';
            } else if (chInput == 'n' || chInput == 'N') {
                sel1 = 1;
                chInput = '\n';
            } else if (chInput == 'L') {
                sel1 = 2;
                chInput = '\n';
            } else if (chInput == 'r' || chInput == 'R') {
                sel1 = 3;
                chInput = '\n';
            } else if (chInput == 's' || chInput == 'S') {
                sel1 = 4;
                chInput = '\n';
            } else if (chInput == 'o' || chInput == 'O') {
                sel1 = 5;
                chInput = '\n';
            } else if (chInput == 'H' || chInput == '?') {
                sel1 = 6;
                chInput = '\n';
            } else if (chInput == 'c' || chInput == 'C') {
                sel1 = 7;
                chInput = '\n';
            } else if (chInput == 'q' || chInput == 'Q' || chInput == KEY_ESCAPE) {
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
                    help();
                } else if (sel1 == 8) {
                    uquit = QUIT_MENU;
                    return false;
                } else {
                    sel2 = 0;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);
                }
            }
        } else if (layer == 2) {
            if (sel1 == 1) {	// New Character
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
                        if (!u.create(this, (sel2 == 0) ? PLTYPE_CUSTOM : ((sel2 == 2)?PLTYPE_RANDOM : PLTYPE_NOW))) {
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }

                        werase(w_background);
                        wrefresh(w_background);
                        start_game();
                        start = true;
                    } else if (sel2 == 1) {
                        layer = 3;
                        sel1 = 0;
                    }
                }
            } else if (sel1 == 2) {	// Load Character
                if (savegames.size() == 0)
                    mvwprintz(w_open, iMenuOffsetY - 2, 19 + iMenuOffsetX, c_red, _("No save games found!"));
                else {
                    for (int i = 0; i < savegames.size(); i++) {
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19 + iMenuOffsetX, (sel2 == i ? h_white : c_white), base64_decode(savegames[i]).c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (savegames.size() == 0 && (input == DirectionS || input == Confirm)) {
                    layer = 1;
                } else if (input == DirectionS) {
                    if (sel2 > 0)
                        sel2--;
                    else
                        sel2 = savegames.size() - 1;
                } else if (input == DirectionN) {
                    if (sel2 < savegames.size() - 1)
                        sel2++;
                    else
                        sel2 = 0;
                } else if (input == DirectionW || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel2 >= 0 && sel2 < savegames.size()) {
                        werase(w_background);
                        wrefresh(w_background);
                        load(savegames[sel2]);
                        start = true;
                    }
                }
            } else if (sel1 == 3) {  // Delete world
                if (query_yn(_("Delete the world and all saves?"))) {
                    delete_save();
                    savegames.clear();
                    MAPBUFFER.reset();
                    MAPBUFFER.make_volatile();
                    overmap_buffer.clear();
                }

                layer = 1;
            } else if (sel1 == 4) {	// Special game
                for (int i = 1; i < NUM_SPECIAL_GAMES; i++) {
                    mvwprintz(w_open, iMenuOffsetY-i-1, 34 + iMenuOffsetX, (sel2 == i-1 ? h_white : c_white),
                    special_game_name( special_game_id(i) ).c_str());
                }
                wrefresh(w_open);
                refresh();
                input = get_input();
                if (input == DirectionS) {
                    if (sel2 > 0)
                        sel2--;
                    else
                        sel2 = NUM_SPECIAL_GAMES - 2;
                } else if (input == DirectionN) {
                    if (sel2 < NUM_SPECIAL_GAMES - 2)
                        sel2++;
                    else
                        sel2 = 0;
                } else if (input == DirectionW || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel2 >= 0 && sel2 < NUM_SPECIAL_GAMES - 1) {
                        delete gamemode;
                        gamemode = get_special_game( special_game_id(sel2+1) );
                        if (!gamemode->init(this)) {
                            delete gamemode;
                            gamemode = new special_game;
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        start = true;
                    }
                }
            }
        } else if (layer == 3) {	// Character Templates
            if (templates.size() == 0)
                mvwprintz(w_open, iMenuOffsetY-4, iMenuOffsetX+20, c_red, _("No templates found!"));
            else {
                for (int i = 0; i < templates.size(); i++) {
                    int line = iMenuOffsetY - 4 - i;
                    mvwprintz(w_open, line, 20 + iMenuOffsetX, (sel1 == i ? h_white : c_white), templates[i].c_str());
                }
            }
            wrefresh(w_open);
            refresh();
            input = get_input();
            if (input == DirectionS) {
                if (sel1 > 0)
                    sel1--;
                else
                    sel1 = templates.size() - 1;
            } else if (templates.size() == 0 && (input == DirectionN || input == Confirm)) {
                sel1 = 1;
                layer = 2;
                print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
            } else if (input == DirectionN) {
                if (sel1 < templates.size() - 1)
                    sel1++;
                else
                    sel1 = 0;
            } else if (input == DirectionW  || input == Cancel || templates.size() == 0) {
                sel1 = 1;
                layer = 2;
                print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
            } else if (input == DirectionE || input == Confirm) {
                if (!u.create(this, PLTYPE_TEMPLATE, templates[sel1])) {
                    u = player();
                    delwin(w_open);
                    return (opening_screen());
                }

                werase(w_background);
                wrefresh(w_background);
                start_game();
                start = true;
            }
        }
    }
    delwin(w_open);
    if (start == false)
        uquit = QUIT_MENU;
    return start;
}
