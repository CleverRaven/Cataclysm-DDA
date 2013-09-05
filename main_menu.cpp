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
#include "options.h"

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
    vMenuItems.push_back(_("<W>orld"));
    //vMenuItems.push_back(_("<R>eset"));
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

int game::worldpick_screen()
{
    DebugLog() << "Entering World Pick Screen\n";
    const int VAL_ESC = -1;
    int worldnum = 0;

    int sel = 0, selpage = 0;

    const int iTooltipHeight = 3;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int num_pages = world_name_keys.size() / iContentHeight + 1; // at least 1 page

    DebugLog() << "\t" << num_pages << " Pages to Generate\n";
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    //mapLines[60] = true;

    std::map<int, std::vector<std::string> > world_pages;

    DebugLog() << "\tGenerating World List\n";
    worldnum = 0;
    for (int i = 0; i < num_pages; ++i)
    {
        world_pages[i] = std::vector<std::string>();
        for (int j = 0; j < iContentHeight; ++j)
        {
            DebugLog() << "\t\tAdding World ["<<world_name_keys[worldnum] << "]\n";
            world_pages[i].push_back(world_name_keys[worldnum++]);

            if (worldnum == world_name_keys.size())
            {
                break;
            }
        }
    }
    DebugLog() << "\tWorld List Generated\n";

    WINDOW* w_worlds_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    WINDOW* w_worlds_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW* w_worlds_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY, 1 + iOffsetX);
    WINDOW* w_worlds = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    wborder(w_worlds_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwputch(w_worlds_border, 4,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_worlds_border, 4, 79, c_ltgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_worlds_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, c_ltgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_worlds_border, 0, 36, c_ltred, _(" WORLD SELECTION "));
    wrefresh(w_worlds_border);

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_worlds_header, 0, i, c_ltgray, LINE_OXXX);
        } else {
            mvwputch(w_worlds_header, 0, i, c_ltgray, LINE_OXOX); // Draw header line
        }
    }

    wrefresh(w_worlds_header);

    DebugLog() << "\tWindows Generated\n";

    char ch = ' ';

    std::stringstream sTemp;

    do {
        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_worlds, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w_worlds, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_worlds_tooltip, i, j, c_black, ' ');
                }
            }
        }

        //Draw World Names
        for (int i = 0; i < world_pages[selpage].size(); ++i)
        {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << i + 1;
            mvwprintz(w_worlds, i, 0, c_white, sTemp.str().c_str());
            mvwprintz(w_worlds, i, 4, c_white, "");


            if (i == sel)
            {
                wprintz(w_worlds, c_yellow, ">> ");
            }
            else
            {
                wprintz(w_worlds, c_yellow, "   ");
            }

            wprintz(w_worlds, c_white, "%s", (world_pages[selpage])[i].c_str());
        }

        //Draw Tabs
        mvwprintz(w_worlds_header, 0, 7, c_white, "");
        for (int i = 0; i < num_pages; ++i) {
            if (world_pages[i].size() > 0) { //skip empty pages
                wprintz(w_worlds_header, c_white, "[");
                wprintz(w_worlds_header, (selpage == i) ? hilite(c_white) : c_white, "Page %d", i+1);
                wprintz(w_worlds_header, c_white, "]");
                wputch(w_worlds_header, c_white, LINE_OXOX);
            }
        }

        wrefresh(w_worlds_header);

        fold_and_print(w_worlds_tooltip, 0, 0, 78, c_white, "Pick a world to enter game");
        wrefresh(w_worlds_tooltip);

        wrefresh(w_worlds);

        ch = input();

        if (world_pages[selpage].size() > 0 || ch == '\t') {
            switch(ch) {
                case 'j': //move down
                    sel++;
                    if (sel >= world_pages[selpage].size()) {
                        sel = 0;
                    }
                    break;
                case 'k': //move up
                    sel--;
                    if (sel < 0) {
                        sel = world_pages[selpage].size()-1;
                    }
                    break;
                case '>':
                case '\t': //Switch to next Page
                    sel = 0;
                    do { //skip empty pages
                        selpage++;
                        if (selpage >= world_pages.size()) {
                            selpage = 0;
                        }
                    } while(world_pages[selpage].size() == 0);

                    break;
                case '<':
                    sel = 0;
                    do { //skip empty pages
                        selpage--;
                        if (selpage < 0) {
                            selpage = world_pages.size()-1;
                        }
                    } while(world_pages[selpage].size() == 0);
                    break;
                case '\n':
                    // we are wanting to get out of this by confirmation, so ask if we want to load the level [y/n prompt] and if yes exit
                    std::stringstream querystring;
                    querystring << "Do you want to start the game in world [" << world_pages[selpage][sel] << "]?";
                    if (query_yn(querystring.str().c_str()))
                    {
                        werase(w_worlds);
                        werase(w_worlds_border);
                        werase(w_worlds_header);
                        werase(w_worlds_tooltip);
                        return sel + selpage * iContentHeight;
                    }
                    break;
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    werase(w_worlds);
    werase(w_worlds_border);
    werase(w_worlds_header);
    werase(w_worlds_tooltip);
    // we are assumed to have quit, so return -1
    return -1;
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

std::map<std::string, std::vector<std::string> > get_save_data()
{
    std::string save_dir = "save";

    struct dirent *dp, *sdp;
    struct stat _buff;
    DIR *save = opendir(save_dir.c_str()), *world;

    std::map<std::string, std::vector<std::string> > world_savegames;

    world_savegames.clear();

    // make sure save directory exists, if not try to create it
    if (!save) {
        #if (defined _WIN32 || defined __WIN32__)
            mkdir("save");
        #else
            mkdir("save", 0777);
        #endif
        save = opendir("save");
    }
    // if save directory still does not exist, exit with message
    if (!save) {
        dbg(D_ERROR) << "game:opening_screen: Unable to make save directory.";
        debugmsg("Could not make './save' directory");
        endwin();
        exit(1);
    }

    // read through save directory looking for folders. These folders are the World folders containing the save data
    while ((dp = readdir(save)))
    {
        if (stat(dp->d_name, &_buff) != 0x4)
        {
            //DebugLog() << "Potential Folder Found: "<< dp->d_name << "\n";
            // ignore "." and ".."
            if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
            {
                std::string subdirname = save_dir + "/" + dp->d_name;
                //subdirname.append(dp->d_name);
                // now that we have the world directory found and saved, we need to populate it with appropriate save file information
                world = opendir(subdirname.c_str());
                if (world == NULL)
                {
                    //DebugLog() << "Could not open directory: "<< dp->d_name << "\n";
                    continue;
                }
                else
                {
                    //worldlist.push_back(dp->d_name);
                    world_savegames[dp->d_name] = std::vector<std::string>();
                }
                while ((sdp = readdir(world)))
                {
                    std::string tmp = sdp->d_name;
                    if (tmp.find(".sav") != std::string::npos)
                    {
                        world_savegames[dp->d_name].push_back(tmp.substr(0, tmp.find(".sav")));
                    }
                }
                // close the world directory
                closedir(world);
            }
        }
    }

    // list worlds and savegame data in DebugLog
    /*
    for (std::map<std::string, std::vector<std::string> >::iterator it = world_savegames.begin(); it != world_savegames.end(); ++it)
    {
        DebugLog() << "World: "<< it->first << "\n";
        for (std::vector<std::string>::iterator sit = it->second.begin(); sit != it->second.end(); ++sit)
        {
            DebugLog() << "\t" << base64_decode(*sit) << "\n";
        }
    }
    //*/

    // return save data
    return world_savegames;
}

int game::pick_world_to_play()
{
    if (world_name_keys.size() == 0)
    {
        if (query_yn("No worlds exist, would you like to make one?"))
        {
            return worldgen_screen();
        }
        else
        {
            return -1;
        }
    }
    else if (world_name_keys.size() == 1)
    {
        return 0; // use that world!
    }
    else
    {
        return worldpick_screen();
        // pick a world
        // for now, return -1 because need to make a new window to select with (probably) >.<
        //return -1;
    }
}

int game::worldgen_screen()
{
    const std::string wdef = "world_default";
    // all we really need to do in here is create the worldoptions.txt file, I think. And then make sure that the world options get loaded properly
    WINDOW* w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                    (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                    (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);

    int tab = 0;

    // read in current world default options
    std::vector<std::string> world_option_names;
    /*
    for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it)
    {
        if (it->second.sPage == wdef)
        {
            world_option_names.push_back(it->first);
        }
    }
    */
    // set up map from the defaults
    std::map<std::string, cOpt> worldops;
    for (std::vector<std::string>::iterator it = world_option_names.begin(); it != world_option_names.end(); ++it)
    {
        worldops[*it] = OPTIONS[*it];
    }

    std::string world_name = "__WORLD_NAME__";

    do
    {
        werase(w);
        wrefresh(w);
        switch(tab)
        {
            case 0: break;
            case 1: break;
        }
    }while (tab >= 0 && tab < 2);

    delwin(w);

    if (tab < 0)
    {
        return -1;
    }

    // make directory
    std::stringstream worldfolder;
    worldfolder << "save/" << world_name;
    DIR *dir = opendir(worldfolder.str().c_str());
    if (!dir) {
        #if (defined _WIN32 || defined __WIN32__)
            mkdir(worldfolder.str().c_str());
        #else
            mkdir(worldfolder.str().c_str(), 0777);
        #endif
        dir = opendir(worldfolder.str().c_str());
        if (!dir) {
            dbg(D_ERROR) << "game:worldgen_screen: Unable to make world directory.";
            debugmsg("Could not make './save/<world>' directory");
            return -1;
        }
    }
    save_world_options(world_name, worldops);
    return -1;
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

    std::vector<std::string> world_sub_items;
    world_sub_items.push_back(_("<C>reate World"));
    world_sub_items.push_back(_("<D>elete World"));
    world_sub_items.push_back(_("<R>eset World"));

    print_menu(w_open, 0, iMenuOffsetX, iMenuOffsetY);

    std::vector<std::string> savegames, templates;

    world_name_keys.clear();
    world_save_data.clear();
    active_world.clear();


    struct dirent *dp;
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

    // get world folders, along with their savegame data
    world_save_data = get_save_data();

    // vector to make it easier to get and cache the working world
    for (std::map<std::string, std::vector<std::string> >::iterator it = world_save_data.begin(); it != world_save_data.end(); ++it)
    {
        world_name_keys.push_back(it->first);
    }

    dir = opendir("data");
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".template") != std::string::npos)
            templates.push_back(tmp.substr(0, tmp.find(".template")));
    }

    int sel1 = 1, sel2 = 1, sel3 = 1, layer = 1;
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

            if (chInput == 'm' || chInput == 'M') { // MOTD
                sel1 = 0;
                chInput = '\n';
            } else if (chInput == 'n' || chInput == 'N') { // New Game
                sel1 = 1;
                chInput = '\n';
            } else if (chInput == 'L') { // Load Game
                sel1 = 2;
                chInput = '\n';
            } else if (chInput == 'w' || chInput == 'W') { // World
                sel1 = 3;
                chInput = '\n';
            } else if (chInput == 's' || chInput == 'S') { // Special
                sel1 = 4;
                chInput = '\n';
            } else if (chInput == 'o' || chInput == 'O') { // Options
                sel1 = 5;
                chInput = '\n';
            } else if (chInput == 'H' || chInput == '?') { // Help
                sel1 = 6;
                chInput = '\n';
            } else if (chInput == 'c' || chInput == 'C') { // Credits
                sel1 = 7;
                chInput = '\n';
            } else if (chInput == 'q' || chInput == 'Q' || chInput == KEY_ESCAPE) { // Quit
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
                        if (!u.create(this, (sel2 == 0) ? PLTYPE_CUSTOM : ((sel2 == 2)?PLTYPE_RANDOM : PLTYPE_NOW))){
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        // check world
                        int picked_world = pick_world_to_play();
                        if (picked_world == -1)
                        {
                            u = player();
                            delwin(w_open);
                            return (opening_screen());
                        }
                        else
                        {
                            active_world = world_name_keys[picked_world];
                        }

                        werase(w_background);
                        wrefresh(w_background);
                        //start_game();
                        start_game_from(active_world);
                        start = true;
                    } else if (sel2 == 1) {
                        layer = 3;
                        sel1 = 0;
                    }
                }
            } else if (sel1 == 2) {	// Load Character
                if (world_name_keys.size() == 0)
                {
                    // no worlds to display
                    mvwprintz(w_open, iMenuOffsetY - 2, 19 + iMenuOffsetX, c_red, _("No save games found!"));
                }
                else
                {
                    // show world names
                    int i = 0;
                    for (std::vector<std::string>::iterator it = world_name_keys.begin();
                         it != world_name_keys.end();
                         ++it)
                    {
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19+iMenuOffsetX, (sel2 == i ? h_white : c_white), (*it).c_str());
                        ++i;
                    }
                    /*
                    for (int i = 0; i < savegames.size(); i++) {
                        int line = iMenuOffsetY - 2 - i;
                        mvwprintz(w_open, line, 19 + iMenuOffsetX, (sel2 == i ? h_white : c_white), base64_decode(savegames[i]).c_str());
                    }*/
                }
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (world_name_keys.size() == 0 && (input == DirectionS || input == Confirm)) {
                    layer = 1;
                }
                else if (input == DirectionS) {
                    if (sel2 > 0)
                        sel2--;
                    else
                        sel2 = world_name_keys.size() - 1;
                } else if (input == DirectionN) {
                    if (sel2 < world_name_keys.size() - 1)
                        sel2++;
                    else
                        sel2 = 0;
                } else if (input == DirectionW || input == Cancel) {
                    layer = 1;
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel2 >= 0 && sel2 < world_name_keys.size()) {
                        layer = 3;
                        sel3 = 0;
                        //werase(w_background);
                        //wrefresh(w_background);
                        // for now can't load the world_save_data
                        //load(world_save_data[sel2]);
                        //start = true;
                    }
                }

            } else if (sel1 == 3) {  // World Menu
                /*
                if (query_yn(_("Delete the world and all saves?"))) {
                    delete_save();
                    savegames.clear();
                    MAPBUFFER.reset();
                    MAPBUFFER.make_volatile();
                    overmap_buffer.clear();
                }
                */
                // show options for Create, Destroy, Reset worlds. Create world goes directly to Make World screen. Reset and Destroy ask for world to modify.
                // Reset empties world of everything but options, then makes new world within it.
                // Destroy asks for confirmation, then destroys everything in world and then removes world folder

                // only show reset / destroy world if there is at least one valid world existing!

                int world_subs_to_display = (world_name_keys.size() > 0)? world_sub_items.size(): 1;
                for (int i = 0; i < world_subs_to_display; ++i)
                {
                    int line = iMenuOffsetY - 2 - i;
                    //mvwprintz(w_open, line, 26 + iMenuOffsetX, (sel2 == i ? h_white : c_white), world_sub_items[i].c_str());
                    //shortcut_print(w_in, h_white, h_white, vItems[i].c_str());
                    //mvwprintz(w_open, line, 26 + iMenuOffsetX, h_white, (sel2 == i ? h_white : c_white), world_sub_items[i].c_str());
                    mvwprintz(w_open, line, 26 + iMenuOffsetX, h_white, "");
                    shortcut_print(w_open, (sel2 == i? h_white:c_ltgray), (sel2 == i ? h_white : c_white), world_sub_items[i].c_str());
                }

                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionS)
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
                else if (input == DirectionN)
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
                else if (input == DirectionW)
                {
                    layer = 1;
                }

                if (input == DirectionE || input == Confirm)
                {
                    if (sel2 == 0) // Create world
                    {
                        // Open up world creation screen!
                    }
                    else if (sel2 == 1 || sel2 == 2) // Delete World | Reset World
                    {
                        layer = 3;
                        sel3 = 0;
                    }
                }

                //layer = 1;
            } else if (sel1 == 4)
            {	// Special game
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
        } else if (layer == 3) {
            if (sel1 == 2) // Load Game
            {
                // sel2 is the world index
                savegames = world_save_data[world_name_keys[sel2]];

                if (savegames.size() == 0)
                    mvwprintz(w_open, iMenuOffsetY - 2, 19+19 + iMenuOffsetX, c_red, _("No save games found!"));
                else {
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
                        active_world = world_name_keys[sel2];

                        load_from(active_world, savegames[sel3]);
                        start = true;
                    }
                }
            }
            else if (sel1 == 3)
            {
                // show world names
                int i = 0;
                for (std::vector<std::string>::iterator it = world_name_keys.begin();
                     it != world_name_keys.end();
                     ++it)
                {
                    int line = iMenuOffsetY - 3 - i;
                    mvwprintz(w_open, line, 40+iMenuOffsetX, (sel3 == i ? h_white : c_white), (*it).c_str());
                    ++i;
                }
                wrefresh(w_open);
                refresh();
                input = get_input();

                if (input == DirectionS) {
                    if (sel3 > 0)
                        --sel3;
                    else
                        sel3 = world_name_keys.size() - 1;
                } else if (input == DirectionN) {
                    if (sel3 < world_name_keys.size() - 1)
                        ++sel3;
                    else
                        sel3 = 0;
                } else if (input == DirectionW || input == Cancel) {
                    layer = 2;

                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (input == DirectionE || input == Confirm) {
                    if (sel3 >= 0 && sel3 < world_name_keys.size()) {

                        if (sel2 == 1) // Delete World
                        {
                            if (query_yn(_("Delete the world and all saves?"))) {
                                delete_world(world_name_keys[sel3], true);
                                //delete_save();
                                savegames.clear();
                                MAPBUFFER.reset();
                                MAPBUFFER.make_volatile();
                                overmap_buffer.clear();

                                layer = 2;
                                // remove from master map
                                world_save_data.erase(world_name_keys[sel3]);
                                // remove from key vector
                                world_name_keys.erase(world_name_keys.begin() + sel3);

                                return opening_screen();
                            }
                        }
                        else if (sel2 == 2) // Reset World
                        {
                            if (query_yn(_("Remove all saves and regenerate world?"))) {
                                delete_world(world_name_keys[sel3], false);
                                //delete_save();
                                savegames.clear();
                                MAPBUFFER.reset();
                                MAPBUFFER.make_volatile();
                                overmap_buffer.clear();

                                layer = 2;
                                // remove from master map -- just resetting, don't need to kill it!
                                //world_save_data.erase(world_name_keys[sel3]);
                                // remove from key vector -- just resetting, don't need to kill it!
                                //world_name_keys.erase(world_name_keys.begin() + sel3);

                                return opening_screen();
                            }
                        }

                    }

                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
            }
            else
            {
                // Character Templates
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
                    // check world
                    int picked_world = pick_world_to_play();
                    if (picked_world == -1)
                    {
                        u = player();
                        delwin(w_open);
                        return (opening_screen());
                    }
                    else
                    {
                        active_world = world_name_keys[picked_world];
                    }

                    werase(w_background);
                    wrefresh(w_background);
                    //start_game();
                    start_game_from(active_world);
                    start = true;
                }
            }
        }
    }
    delwin(w_open);
    if (start == false)
        uquit = QUIT_MENU;
    return start;
}
