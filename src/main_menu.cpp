#include "game.h"
#include "debug.h"
#include "input.h"
#include "mapbuffer.h"
#include "cursesdef.h"
#include "overmapbuffer.h"
#include "translations.h"
#include "catacharset.h"
#include "get_version.h"
#include "help.h"
#include "worldfactory.h"
#include "file_wrapper.h"
#include "path_info.h"
#include "mapsharing.h"

#include <fstream>

#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
extern worldfactory *world_generator;

static std::vector<std::string> mmenu_motd;
static std::vector<std::string> mmenu_credits;

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

    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              "_________            __                   .__                            ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              "\\_   ___ \\ _____   _/  |_ _____     ____  |  |   ___.__   ______  _____  ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              "/    \\  \\/ \\__  \\  \\   __\\\\__  \\  _/ ___\\ |  |  <   |  | /  ___/ /     \\ ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              "\\     \\____ / __ \\_ |  |   / __ \\_\\  \\___ |  |__ \\___  | \\___ \\ |  Y Y  \\");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              " \\______  /(____  / |__|  (____  / \\___  >|____/ / ____|/____  >|__|_|  /");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1,
              "        \\/      \\/             \\/      \\/        \\/          \\/       \\/ ");

    if (bShowDDA) {
        if (FULL_SCREEN_HEIGHT > 24) {
            ++iLine;
        }
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  "________                   .__      ________                           ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  "\\______ \\  _____   _______ |  | __  \\______ \\  _____    ___.__   ______");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  " |    |  \\ \\__  \\  \\_  __ \\|  |/ /   |    |  \\ \\__  \\  <   |  | /  ___/");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  " |    `   \\ / __ \\_ |  | \\/|    <    |    `   \\ / __ \\_ \\___  | \\___ \\ ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  "/_______  /(____  / |__|   |__|_ \\  /_______  /(____  / / ____|/____  >");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2,
                  "        \\/      \\/              \\/          \\/      \\/  \\/          \\/ ");

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
    for( auto menu_item : vMenuItems ) {
        // adds width if there are shortcut symbols "<" & ">", and width + 2 otherwise.
        menu_length += utf8_width(menu_item.c_str()) +
            (menu_item.find_first_of("<") != std::string::npos ? 0 : 2);
    }
    // Available free space. -1 width_pos != line_pos. line_pos == width - 1.
    const int free_space = std::max(0, window_width - menu_length - 1 - iMenuOffsetX);
    const int spacing = free_space / ((int)vMenuItems.size() - 1);
    const int width_of_spacing = spacing * (vMenuItems.size() - 1);
    const int adj_offset = std::max(0, (free_space - width_of_spacing) / 2);

    print_menu_items(w_open, vMenuItems, iSel, iMenuOffsetY, adj_offset, spacing);

    refresh();
    wrefresh(w_open);
    refresh();
}

void game::print_menu_items(WINDOW *w_in, std::vector<std::string> vItems, int iSel, int iOffsetY,
                            int iOffsetX, int spacing)
{
    mvwprintz(w_in, iOffsetY, iOffsetX, c_black, "");

    const int items_size = (int)vItems.size();
    for (int i = 0; i < items_size; i++) {
        wprintz(w_in, c_ltgray, "[");
        if (iSel == i) {
            shortcut_print(w_in, h_white, h_white, vItems[i]);
        } else {
            shortcut_print(w_in, c_ltgray, c_white, vItems[i]);
        }
        wprintz(w_in, c_ltgray, "]");
        // Don't print spaces after last item.
        if ( i != (items_size - 1)) {
            wprintz(w_in, c_ltgray, std::string(spacing, ' ').c_str());
        }
    }
}

std::vector<std::string> load_file( const std::string &path, const std::string &alternative_text )
{
    std::ifstream stream( path.c_str() );
    std::vector<std::string> result;
    std::string line;
    while( std::getline( stream, line ) ) {
        if( !line.empty() && line[0] == '#' ) {
            continue;
        }
        result.push_back( line );
    }
    if( result.empty() ) {
        result.push_back( alternative_text );
    }
    return result;
}

void game::mmenu_refresh_motd()
{
    mmenu_motd = load_file(PATH_INFO::find_translated_file( "motddir", ".motd", "motd" ), _( "No message today." ) );
}

void game::mmenu_refresh_credits()
{
    mmenu_credits = load_file(PATH_INFO::find_translated_file( "creditsdir", ".credits", "credits" ), _( "No message today." ) );
}

bool game::opening_screen()
{
    // Play title music, whoo!
    play_music("title");

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
    if(!MAP_SHARING::isSharing()) { // "Play Now" function doesn't play well together with shared maps
        vSubItems.push_back(pgettext("Main Menu|New Game", "Play <N>ow!"));
    }


    std::vector<std::string> vWorldSubItems;
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<C>reate World"));
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<D>elete World"));
    vWorldSubItems.push_back(pgettext("Main Menu|World", "<R>eset World"));

    print_menu(w_open, 0, iMenuOffsetX, iMenuOffsetY);

    std::vector<std::string> savegames, templates;
    dirent *dp;
    DIR *dir;

    if (!assure_dir_exist(FILENAMES["savedir"])) {
        popup(_("Unable to make save directory. Check permissions."));
        return false;
    }

    if (!assure_dir_exist(FILENAMES["templatedir"].c_str())) {
        popup(_("Unable to make templates directory. Check permissions."));
        return false;
    }
    dir = opendir(FILENAMES["templatedir"].c_str());
    while ((dp = readdir(dir))) {
        std::string tmp = dp->d_name;
        if (tmp.find(".template") != std::string::npos) {
            templates.push_back(tmp.substr(0, tmp.find(".template")));
        }
    }
    closedir(dir);

    int sel1 = 1, sel2 = 1, sel3 = 1, layer = 1;
    input_context ctxt("MAIN_MENU");
    ctxt.register_cardinal();
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
    // for the menu shortcuts
    ctxt.register_action("ANY_INPUT");
    bool start = false;

    // Load MOTD and Credits, load it once as it shouldn't change for the duration of the application being open
    mmenu_refresh_motd();
    mmenu_refresh_credits();

    u = player();

    while(!start) {
        print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1 == 0 || sel1 == 7) ? false : true);

        if (layer == 1) {
            if (sel1 == 0) { // Print the MOTD.
                for (size_t i = 0; i < mmenu_motd.size() && i < 16; i++) {
                    mvwprintz(w_open, i + 6, 8 + extra_w / 2, c_ltred, mmenu_motd[i].c_str());
                }

                wrefresh(w_open);
                refresh();
            } else if (sel1 == 7) { // Print the Credits.
                for (size_t i = 0; i < mmenu_credits.size() && i < 16; i++) {
                    mvwprintz(w_open, i + 6, 8 + extra_w / 2, c_ltred, mmenu_credits[i].c_str());
                }

                wrefresh(w_open);
                refresh();
            }

            std::string action = ctxt.handle_input();
            const long chInput = ctxt.get_raw_input().get_first_input();
            if (chInput == 'm' || chInput == 'M') {
                // MOTD
                sel1 = 0;
                action = "CONFIRM";
            } else if (chInput == 'n' || chInput == 'N') {
                // New Game
                sel1 = 1;
                action = "CONFIRM";
            } else if (chInput == 'a' || chInput == 'A') {
                // Load Game
                sel1 = 2;
                action = "CONFIRM";
            } else if (chInput == 'w' || chInput == 'W') {
                // World
                sel1 = 3;
                action = "CONFIRM";
            } else if (chInput == 's' || chInput == 'S') {
                // Special Game
                sel1 = 4;
                action = "CONFIRM";
            } else if (chInput == 'o' || chInput == 'O') {
                // Options
                sel1 = 5;
                action = "CONFIRM";
            } else if (chInput == 'e' || chInput == 'E' || chInput == '?') {
                // Help
                sel1 = 6;
                action = "CONFIRM";
            } else if (chInput == 'c' || chInput == 'C') {
                // Credits
                sel1 = 7;
                action = "CONFIRM";
            } else if (action == "QUIT") {
                // Quit
                sel1 = 8;
                action = "CONFIRM";
            } else if (action == "LEFT") {
                if (sel1 > 0) {
                    sel1--;
                } else {
                    sel1 = 8;
                }
            } else if (action == "RIGHT") {
                if (sel1 < 8) {
                    sel1++;
                } else {
                    sel1 = 0;
                }
            }
            if ((action == "UP" || action == "CONFIRM") && sel1 > 0 && sel1 != 7) {
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
                if (MAP_SHARING::isSharing() &&
                    world_generator->all_worlds.empty()) { //don't show anything when there are no worlds (will not work if there are special maps)
                    layer = 1;
                    sel1 = 1;
                    continue;
                }

                print_menu_items(w_open, vSubItems, sel2, iMenuOffsetY - 2, iMenuOffsetX);
                wrefresh(w_open);
                refresh();

                std::string action = ctxt.handle_input();
                const long chInput = ctxt.get_raw_input().get_first_input();
                if (chInput == 'c' || chInput == 'C') {
                    sel2 = 0;
                    action = "CONFIRM";
                } else if (chInput == 'p' || chInput == 'P') {
                    sel2 = 1;
                    action = "CONFIRM";
                } else if (chInput == 'r' || chInput == 'R') {
                    sel2 = 2;
                    action = "CONFIRM";
                } else if (chInput == 'n' || chInput == 'N') {
                    sel2 = 3;
                    action = "CONFIRM";
                } else if (action == "LEFT") {
                    sel2--;
                    if (sel2 < 0) {
                        sel2 = vSubItems.size() - 1;
                    }
                } else if (action == "RIGHT") {
                    sel2++;
                    if (sel2 >= (int)vSubItems.size()) {
                        sel2 = 0;
                    }
                } else if (action == "DOWN" || action == "QUIT") {
                    layer = 1;
                    sel1 = 1;
                }
                if (action == "UP" || action == "CONFIRM") {
                    if (sel2 == 0 || sel2 == 2 || sel2 == 3) {
                        // First load the mods, this is done by
                        // loading the world.
                        // Pick a world, supressing prompts if it's "play now" mode.
                        WORLDPTR world = world_generator->pick_world( sel2 != 3 );
                        if (world == NULL) {
                            continue;
                        }
                        world_generator->set_active_world(world);
                        setup();
                        if (!u.create((sel2 == 0) ? PLTYPE_CUSTOM :
                                      ((sel2 == 2) ? PLTYPE_RANDOM : PLTYPE_NOW))) {
                            u = player();
                            continue;
                        }
                        werase(w_background);
                        wrefresh(w_background);

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
                    for (int i = 0; i < (int)world_generator->all_worldnames.size(); ++i) {
                      int line = iMenuOffsetY - 2 - i;
                      std::string world_name = world_generator->all_worldnames[i];
                      int savegames_count = world_generator->all_worlds[world_name]->world_saves.size();
                      nc_color color1, color2;
                      if(world_name == "TUTORIAL" || world_name == "DEFENSE") {
                         color1 = c_ltcyan;
                         color2 = h_ltcyan;
                      } else {
                         color1 = c_white;
                         color2 = h_white;
                      }
                      mvwprintz(w_open, line, 15 + iMenuOffsetX + extra_w / 2,
                                (sel2 == i ? color2 : color1 ), "%s (%d)",
                                world_name.c_str(), savegames_count);
                    }
                }
                wrefresh(w_open);
                refresh();
                const std::string action = ctxt.handle_input();
                if (world_generator->all_worldnames.empty() && (action == "DOWN" || action == "CONFIRM")) {
                    layer = 1;
                } else if (action == "DOWN") {
                    if (sel2 > 0) {
                        sel2--;
                    } else {
                        sel2 = world_generator->all_worldnames.size() - 1;
                    }
                } else if (action == "UP") {
                    if (sel2 < (int)world_generator->all_worldnames.size() - 1) {
                        sel2++;
                    } else {
                        sel2 = 0;
                    }
                } else if (action == "LEFT" || action == "QUIT") {
                    layer = 1;
                } else if (action == "RIGHT" || action == "CONFIRM") {
                    if (sel2 >= 0 && sel2 < (int)world_generator->all_worldnames.size()) {
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

                if(MAP_SHARING::isSharing() && !MAP_SHARING::isWorldmenu() && !MAP_SHARING::isAdmin()) {
                    layer = 1;
                    popup(_("Only the admin can change worlds."));
                    continue;
                }

                int world_subs_to_display = (!world_generator->all_worldnames.empty()) ? vWorldSubItems.size() : 1;
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
                std::string action = ctxt.handle_input();
                const long chInput = ctxt.get_raw_input().get_first_input();

                if (chInput == 'c' || chInput == 'C') {
                    sel2 = 0;
                    action = "CONFIRM";
                } else if ((chInput == 'd' || chInput == 'D') && (world_subs_to_display > 1)) {
                    sel2 = 1;
                    action = "CONFIRM";
                } else if ((chInput == 'r' || chInput == 'R') && (world_subs_to_display > 1)) {
                    sel2 = 2;
                    action = "CONFIRM";
                }

                if (action == "LEFT") {
                    if (sel2 > 0) {
                        --sel2;
                    } else {
                        sel2 = world_subs_to_display - 1;
                    }
                } else if (action == "RIGHT") {
                    if (sel2 < world_subs_to_display - 1) {
                        ++sel2;
                    } else {
                        sel2 = 0;
                    }
                } else if (action == "DOWN" || action == "QUIT") {
                    layer = 1;
                }

                if (action == "UP" || action == "CONFIRM") {
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
                if(MAP_SHARING::isSharing()) { // Thee can't save special games, therefore thee can't share them
                    layer = 1;
                    popup(_("Special games don't work with shared maps."));
                    continue;
                }

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
                std::string action = ctxt.handle_input();
                if (action == "LEFT") {
                    if (sel2 > 0) {
                        sel2--;
                    } else {
                        sel2 = NUM_SPECIAL_GAMES - 2;
                    }
                } else if (action == "RIGHT") {
                    if (sel2 < NUM_SPECIAL_GAMES - 2) {
                        sel2++;
                    } else {
                        sel2 = 0;
                    }
                } else if (action == "DOWN" || action == "QUIT") {
                    layer = 1;
                }
                if (action == "UP" || action == "CONFIRM") {
                    if (sel2 >= 0 && sel2 < NUM_SPECIAL_GAMES - 1) {
                        delete gamemode;
                        gamemode = get_special_game( special_game_id(sel2 + 1) );
                        // check world
                        WORLDPTR world = world_generator->make_new_world(special_game_id(sel2 + 1));
                        if (world == NULL) {
                            continue;
                        }
                        world_generator->set_active_world(world);
                        setup();
                        if (!gamemode->init()) {
                            delete gamemode;
                            gamemode = NULL;
                            u = player();
                            continue;
                        }
                        start = true;
                    }
                }
            }
        } else if (layer == 3) {
            bool available = false;
            if (sel1 == 2) { // Load Game
                savegames = world_generator->all_worlds[world_generator->all_worldnames[sel2]]->world_saves;
                if (savegames.empty()) {
                    mvwprintz(w_open, iMenuOffsetY - 2, 19 + 19 + iMenuOffsetX + extra_w / 2,
                              c_red, _("No save games found!"));
                } else {
                    for( std::vector<std::string>::iterator it = savegames.begin();
                         it != savegames.end(); ) {
                        std::string savename = base64_decode(*it);
                        if( MAP_SHARING::isSharing() && savename != MAP_SHARING::getUsername() ) {
                            it = savegames.erase(it);
                        } else {
                            // calculates the index from distance between it and savegames.begin()
                            int i = it - savegames.begin();
                            available = true;
                            int line = iMenuOffsetY - 2 - i;
                            mvwprintz(w_open, line, 19 + 19 + iMenuOffsetX + extra_w / 2,
                                      (sel3 == i ? h_white : c_white),
                                      base64_decode(*it).c_str());
                            ++it;
                        }
                    }
                    if (!available) {
                        mvwprintz(w_open, iMenuOffsetY - 2, 19 + 19 + iMenuOffsetX + extra_w / 2,
                                  c_red, _("No save games found!"));
                    }
                }
                wrefresh(w_open);
                refresh();
                std::string action = ctxt.handle_input();
                if (savegames.empty() && (action == "DOWN" || action == "CONFIRM")) {
                    layer = 2;
                } else if (action == "DOWN") {
                    if (sel3 > 0) {
                        sel3--;
                    } else {
                        sel3 = savegames.size() - 1;
                    }
                } else if (action == "UP") {
                    if (sel3 < (int)savegames.size() - 1) {
                        sel3++;
                    } else {
                        sel3 = 0;
                    }
                } else if (action == "LEFT" || action == "QUIT") {
                    layer = 2;
                    sel3 = 0;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (action == "RIGHT" || action == "CONFIRM") {
                    if (sel3 >= 0 && sel3 < (int)savegames.size()) {
                        werase(w_background);
                        wrefresh(w_background);
                        WORLDPTR world = world_generator->all_worlds[world_generator->all_worldnames[sel2]];
                        world_generator->set_active_world(world);
                        setup();
                        MAPBUFFER.load(world->world_name);

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
                    nc_color color1, color2;
                    if(*it == "TUTORIAL" || *it == "DEFENSE") {
                       color1 = c_ltcyan;
                       color2 = h_ltcyan;
                    } else {
                       color1 = c_white;
                       color2 = h_white;
                    }
                    mvwprintz(w_open, line, 26 + iMenuOffsetX + extra_w / 2,
                              (sel3 == i ? color2 : color1), "%s (%d)", (*it).c_str(), savegames_count);
                    ++i;
                }
                wrefresh(w_open);
                refresh();
                std::string action = ctxt.handle_input();

                if (action == "DOWN") {
                    if (sel3 > 0) {
                        --sel3;
                    } else {
                        sel3 = world_generator->all_worldnames.size() - 1;
                    }
                } else if (action == "UP") {
                    if (sel3 < (int)world_generator->all_worldnames.size() - 1) {
                        ++sel3;
                    } else {
                        sel3 = 0;
                    }
                } else if (action == "LEFT" || action == "QUIT") {
                    layer = 2;

                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                }
                if (action == "RIGHT" || action == "CONFIRM") {
                    if (sel3 >= 0 && sel3 < (int)world_generator->all_worldnames.size()) {
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
                            overmap_buffer.clear();

                            layer = 2;

                            if (do_delete) {
                                // delete world and all contents
                                world_generator->remove_world(world_generator->all_worldnames[sel3]);
                            } else {
                                // clear out everything but worldoptions from this world
                                world_generator->all_worlds[world_generator->all_worldnames[sel3]]->world_saves.clear();
                            }
                            if (world_generator->all_worldnames.empty()) {
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
                if (templates.empty()) {
                    mvwprintz(w_open, iMenuOffsetY - 4, iMenuOffsetX + 20 + extra_w / 2,
                              c_red, _("No templates found!"));
                } else {
                    for (int i = 0; i < (int)templates.size(); i++) {
                        int line = iMenuOffsetY - 4 - i;
                        mvwprintz(w_open, line, 20 + iMenuOffsetX + extra_w / 2,
                                  (sel3 == i ? h_white : c_white), templates[i].c_str());
                    }
                }
                wrefresh(w_open);
                refresh();
                std::string action = ctxt.handle_input();
                if (action == "DOWN") {
                    if (sel3 > 0) {
                        sel3--;
                    } else {
                        sel3 = templates.size() - 1;
                    }
                } else if (templates.empty() && (action == "UP" || action == "CONFIRM")) {
                    sel1 = 1;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                } else if (action == "UP") {
                    if (sel3 < (int)templates.size() - 1) {
                        sel3++;
                    } else {
                        sel3 = 0;
                    }
                } else if (action == "LEFT"  || action == "QUIT" || templates.empty()) {
                    sel1 = 1;
                    layer = 2;
                    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY);
                } else if (action == "RIGHT" || action == "CONFIRM") {
                    WORLDPTR world = world_generator->pick_world();
                    if (world == NULL) {
                        u = player();
                        continue;
                    }
                    world_generator->set_active_world(world);
                    setup();
                    if (!u.create(PLTYPE_TEMPLATE, templates[sel3])) {
                        u = player();
                        continue;
                    }
                    werase(w_background);
                    wrefresh(w_background);
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
