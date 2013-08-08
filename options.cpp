#include "game.h"
#include "options.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <algorithm>

bool trigdist;

void load_options(){};
void save_options(){};

std::map<std::string, cOpt> OPTIONS;

void initOptions() {
    OPTIONS["USE_CELSIUS"] =            cOpt("General", _("Use Celsius"),
                                         _("If true, use Celcius not Fahrenheit. Default is fahrenheit"),
                                         false
                                        );

    OPTIONS["USE_METRIC_SPEEDS"] =      cOpt("General", _("Use Metric Speeds"),
                                             _("If true, use Km/h not mph. Default is mph"),
                                             false
                                            );

    OPTIONS["USE_METRIC_WEIGHTS"] =     cOpt("General", _("Use Metric Weights"),
                                             _("If true, use kg not lbs. Default is lbs"),
                                             false
                                            );

    OPTIONS["FORCE_CAPITAL_YN"] =       cOpt("General", _("Force Y/N in prompts"),
                                             _("If true, y/n prompts are case- sensitive and y and n are not accepted. Default is true"),
                                             true
                                            );

    OPTIONS["NO_BRIGHT_BACKGROUNDS"] =  cOpt("General", _("No Bright Backgrounds"),
                                            _("If true, bright backgrounds are not used--some consoles are not compatible. Default is false"),
                                             false
                                            );

    OPTIONS["24_HOUR"] =                cOpt("General", _("24 Hour Time"),
                                             _("12h/24h Time: 0 - AM/PM (default)  eg: 7:31 AM 1 - 24h military     eg: 0731 2 - 24h normal       eg: 7:31"),
                                             0, 2, 0
                                            );

    OPTIONS["SNAP_TO_TARGET"] =         cOpt("General", _("Snap to Target"),
                                             _("If true, automatically follow the crosshair when firing/throwing. Default is false"),
                                             false
                                            );

    OPTIONS["SAFEMODE"] =               cOpt("General", _("Safemode on by default"),
                                             _("If true, safemode will be on after starting a new game or loading. Default is true"),
                                             true
                                            );

    OPTIONS["SAFEMODEPROXIMITY"] =      cOpt("General", _("Safemode proximity distance"),
                                             _("If safemode is enabled, distance to hostiles when safemode should show a warning. 0=Viewdistance, and the default"),
                                             0, 50, 0
                                            );

    OPTIONS["AUTOSAFEMODE"] =           cOpt("General", _("Auto-Safemode on by default"),
                                             _("If true, auto-safemode will be on after starting a new game or loading. Default is false"),
                                             false
                                            );

    OPTIONS["AUTOSAFEMODETURNS"] =      cOpt("General", _("Turns to reenable safemode"),
                                             _("Number of turns after safemode is reenabled if no hostiles are in safemodeproximity distance. Default is 50"),
                                             0, 50, 0
                                            );

    OPTIONS["AUTOSAVE"] =               cOpt("General", _("Periodically Autosave"),
                                             _("If true, game will periodically save the map Default is false"),
                                             false
                                            );

    OPTIONS["AUTOSAVE_TURNS"] =         cOpt("General", _("Game minutes between autosaves"),
                                             _("Number of game minutes between autosaves"),
                                             0, 30, 5
                                            );

    OPTIONS["AUTOSAVE_MINUTES"] =       cOpt("General", _("Real minutes between autosaves"),
                                             _("Minimum number of real time minutes between autosaves"),
                                             0, 127, 5
                                            );

    OPTIONS["GRADUAL_NIGHT_LIGHT"] =    cOpt("General", _("Gradual night light"),
                                             _("If true will add nice gradual-lighting should only make a difference during the night. Default is true"),
                                             true
                                            );

    OPTIONS["RAIN_ANIMATION"] =         cOpt("General", _("Rain animation"),
                                             _("If true, will display weather animations. Default is true"),
                                             true
                                            );

    OPTIONS["CIRCLEDIST"] =             cOpt("General", _("Circular distances"),
                                             _("If true, the game will calculate range in a realistic way: light sources will be circles diagonal movement will cover more ground and take longer. If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall."),
                                             false
                                            );

    OPTIONS["QUERY_DISASSEMBLE"] =      cOpt("General", _("Query on disassembly"),
                                             _("If true, will query before disassembling items. Default is true"),
                                             true
                                            );

    OPTIONS["DROP_EMPTY"] =             cOpt("General", _("Drop empty containers"),
                                             _("Set to drop empty containers after use. 0 - don't drop any (default) 1 - all except watertight containers 2 - all containers"),
                                             0, 2, 0
                                            );

    OPTIONS["SKILL_RUST"] =             cOpt("General", _("Skill Rust"),
                                             _("Set the level of skill rust. 0 - vanilla Cataclysm (default) 1 - capped at skill levels 2 - intelligence dependent 3 - intelligence dependent, capped 4 - none at all"),
                                             0, 4, 0
                                            );

    OPTIONS["DELETE_WORLD"] =           cOpt("General", _("Delete World"),
                                             _("Delete saves upon player death. 0 - no (default) 1 - yes 2 - query"),
                                             0, 2, 0
                                            );

    OPTIONS["INITIAL_POINTS"] =         cOpt("General", _("Initial points"),
                                             _("Initial points available on character generation. Default is 6"),
                                             0, 25, 6
                                            );

    OPTIONS["MAX_TRAIT_POINTS"] =       cOpt("General", _("Maximum trait points"),
                                             _("Maximum trait points available for character generation. Default is 12"),
                                             0, 25, 12
                                            );

    OPTIONS["INITIAL_TIME"] =           cOpt("General", _("Initial time"),
                                             _("Initial starting time of day on character generation. Default is 8:00"),
                                             0, 23, 8
                                            );

    OPTIONS["VIEWPORT_X"] =             cOpt("General", _("Viewport width"),
                                             _("WINDOWS ONLY: Set the expansion of the viewport along the X axis. Requires restart. Default is 12. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["VIEWPORT_Y"] =             cOpt("General", _("Viewport height"),
                                             _("WINDOWS ONLY: Set the expansion of the viewport along the Y axis. Requires restart. Default is 12. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["SIDEBAR_STYLE"] =          cOpt("General", _("Sidebar style"),
                                             _("Sidebar style. 0 is standard, 1 is narrower and taller."),
                                             0, 1, 0
                                            );

    OPTIONS["MOVE_VIEW_OFFSET"] =       cOpt("General", _("Move view offset"),
                                             _("Move view by how many squares per keypress. Default is 1"),
                                             1, 50, 1
                                            );

    OPTIONS["STATIC_SPAWN"] =           cOpt("General", _("Static spawn"),
                                             _("Season length, in days. Default is 14"),
                                             14, 127, 14
                                            );

    OPTIONS["CLASSIC_ZOMBIES"] =        cOpt("General", _("Classic zombies"),
                                             _("Spawn zombies at game start instead of during game. Must reset world directory after changing for it to take effect. Default is true"),
                                             true
                                            );

    OPTIONS["REVIVE_ZOMBIES"] =         cOpt("General", _("Revive zombies"),
                                             _("Only spawn classic zombies and natural wildlife. Requires a reset of save folder to take effect. This disables certain buildings. Default is false"),
                                             false
                                            );

    OPTIONS["SEASON_LENGTH"] =          cOpt("General", _("Season length"),
                                             _("Allow zombies to revive after a certain amount of time. Default is true"),
                                             true
                                            );

    OPTIONS["STATIC_NPC"] =             cOpt("General", _("Static npcs"),
                                             _("If true, the game will spawn static NPC at the start of the game, requires world reset. Default is false"),
                                             false
                                            );

    OPTIONS["RANDOM_NPC"] =             cOpt("General", _("Random npcs"),
                                             _("If true, the game will randomly spawn NPC during gameplay. Default is false"),
                                             false
                                            );

    OPTIONS["RAD_MUTATION"] =           cOpt("General", _("Mutations by radiation"),
                                             _("If true, radiation causes the player to mutate. Default is true"),
                                             true
                                            );

    OPTIONS["SAVE_SLEEP"] =             cOpt("General", _("Ask to save before sleeping"),
                                             _("If true, game will ask to save the map before sleeping. Default is false"),
                                             false
                                            );

    OPTIONS["HIDE_CURSOR"] =            cOpt("General", _("Hide Mouse Cursor"),
                                             _("If 0, cursor is always shown. If 1, cursor is hidden. If 2, cursor is hidden on keyboard input and unhidden on mouse movement. Default is 0."),
                                             0, 2, 0
                                            );

    OPTIONS["AUTO_PICKUP"] =            cOpt("General", _("Enable item Auto Pickup"),
                                             _("Enable item auto pickup. Change pickup rules with the Auto Pickup Manager in the Help Menu ?3"),
                                             false
                                            );

    OPTIONS["AUTO_PICKUP_ZERO"] =       cOpt("General", _("Auto Pickup 0 Vol/Weight"),
                                             _("Auto pickup items with 0 Volume or Weight"),
                                             false
                                            );

    OPTIONS["AUTO_PICKUP_SAFEMODE"] =   cOpt("General", _("Auto Pickup Safemode"),
                                             _("Auto pickup is disabled as long as you can see monsters nearby. This is affected by Safemode proximity distance."),
                                             false
                                            );

    OPTIONS["DANGEROUS_PICKUPS"] =      cOpt("General", _("Dangerous pickups"),
                                             _("If false will cause player to drop new items that cause them to exceed the weight limit. Default is false."),
                                             false
                                            );

    OPTIONS["SORT_CRAFTING"] =          cOpt("General", _("Sort Crafting menu"),
                                             _("If true, the crafting menus will display recipes that you can craft before other recipes"),
                                             false
                                            );
}

void game::show_options()
{
    std::vector<std::string> vCategory;
    vCategory.push_back("General");
    vCategory.push_back("UI");
    vCategory.push_back("Gameplay");
    vCategory.push_back("Debug");

    /*
    OPTIONS["use_celsius"] = cOpt("General", "Use Celsius",
                               "Use Celcius or Fahrenheit. Default is Fahrenheit",
                               "Celsius,Fahrenheit", "Fahrenheit"
                              );

    OPTIONS["use_celsius_int"] = cOpt("General", "Use Celsius",
                                   "Use Celcius or Fahrenheit. Default is Fahrenheit",
                                   0, 10, 0
                                  );

    OPTIONS["use_celsius_bool"] = cOpt("General", "Use Celsius",
                                    "Use Celcius or Fahrenheit. Default is Fahrenheit",
                                    true
                                   );

    debugmsg(!OPTIONS["use_celsius_bool"] ? "bTure" : "bFalse");

    if (OPTIONS["use_celsius"] == "Fahrenheit") {
        debugmsg("if (OPTIONS[\"use_celsius\"] == \"Fahrenheit\") {");
    }

    if (OPTIONS["use_celsius"] != "Celsius") {
        debugmsg("if (OPTIONS[\"use_celsius\"] != \"Celsius\") {");
    }

    OPTIONS["use_celsius"].set("Celsius");

    if (OPTIONS["use_celsius"]) {
        debugmsg("if (OPTIONS[\"use_celsius\"]) {");
    }

    if (!OPTIONS["use_celsius"]) {
        debugmsg("if (!OPTIONS[\"use_celsius\"]) {");
    }

    if (OPTIONS["use_celsius_bool"]) {
        debugmsg("if (OPTIONS[\"use_celsius_bool\"]) {");
    }

    if (!OPTIONS["use_celsius_bool"]) {
        debugmsg("if (!OPTIONS[\"use_celsius_bool\"]) {");
    }

    if (OPTIONS["use_celsius_int"] <= 5) {
        debugmsg("if (OPTIONS[\"use_celsius_int\"] <= 2) {");
    }

    if (OPTIONS["use_celsius_int"] >= 5) {
        debugmsg("if (OPTIONS[\"use_celsius_int\"] >= 2) {");
    }

    if (OPTIONS["use_celsius_bool"] == true) {
        debugmsg("if (OPTIONS[\"use_celsius_bool\"] == true) {");
    }

    if (OPTIONS["use_celsius_bool"] != true) {
        debugmsg("if (OPTIONS[\"use_celsius_bool\"] != true) {");
    }
    */

/*
    // Remember what the options were originally so we can restore them if player cancels.
    std::map<std::string, cOpt> OPTIONS_OLD = OPTIONS;

    WINDOW* w_options_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                      (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                                      (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    WINDOW* w_options = newwin(FULL_SCREEN_HEIGHT-2, FULL_SCREEN_WIDTH-2,
                               1 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0),
                               1 + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0));

    int offset = 1;
    const int MAX_LINE = 22;
    int line = 0;
    char ch = ' ';
    bool changed_options = false;
    bool needs_refresh = true;
    wborder(w_options_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwprintz(w_options_border, 0, 36, c_ltred, _(" OPTIONS "));
    wrefresh(w_options_border);
    do {
// TODO: change instructions
        if(needs_refresh) {
            werase(w_options);
            // because those texts use their own \n, do not fold so use a large enough width like 999
            fold_and_print(w_options, 0, 40, 999, c_white, _("Use up/down keys to scroll through\navailable options.\nUse left/right keys to toggle.\nPress ESC or q to return."));
// highlight options for option descriptions
            fold_and_print(w_options, 5, 40, 999, c_white, option_desc(option_key(offset + line)).c_str());
            needs_refresh = false;
        }

// Clear the lines
        for(int i = 0; i < 25; i++) {
            mvwprintz(w_options, i, 0, c_black, "                                        ");
        }
        int valid_option_count = 0;

// display options
        for(int i = 0; i < 26 && offset + i < NUM_OPTION_KEYS; i++) {
            valid_option_count++;
            mvwprintz(w_options, i, 0, c_white, "%s: ",
                      option_name(option_key(offset + i)).c_str());

            if(option_is_bool(option_key(offset + i))) {
                bool on = OPTIONS[ option_key(offset + i) ];
                if(i == line) {
                    mvwprintz(w_options, i, 30, hilite(c_ltcyan), (on ? _("True") : _("False")));
                } else {
                    mvwprintz(w_options, i, 30, (on ? c_ltgreen : c_ltred), (on ? _("True") : _("False")));
                }
            } else {
                char option_val = OPTIONS[ option_key(offset + i) ];
                if(i == line) {
                    mvwprintz(w_options, i, 30, hilite(c_ltcyan), "%d", option_val);
                } else {
                    mvwprintz(w_options, i, 30, c_ltgreen, "%d", option_val);
                }
            }
        }
        wrefresh(w_options);
        ch = input();
        needs_refresh = true;

        switch(ch) {
// move up and down
        case 'j':
            line++;
            if(line > MAX_LINE/2 && offset + 1 < NUM_OPTION_KEYS - MAX_LINE) {
                ++offset;
                --line;
            }
            if(line > MAX_LINE) {
                line = 0;
                offset = 1;
            }
            break;
        case 'k':
            line--;
            if(line < MAX_LINE/2 && offset > 1) {
                --offset;
                ++line;
            }
            if(line < 0) {
                line = MAX_LINE;
                offset = NUM_OPTION_KEYS - MAX_LINE - 1;
            }
            break;
// toggle options with left/right keys
        case 'h':
            if(option_is_bool(option_key(offset + line))) {
                OPTIONS[ option_key(offset + line) ] = !(OPTIONS[ option_key(offset + line) ]);
            } else {
                OPTIONS[ option_key(offset + line) ]--;
                if((OPTIONS[ option_key(offset + line) ]) < option_min_options(option_key(offset + line))) {
                    OPTIONS[ option_key(offset + line) ] = option_max_options(option_key(offset + line)) - 1;
                }
            }
            changed_options = true;
            break;
        case 'l':
            if(option_is_bool(option_key(offset + line))) {
                OPTIONS[ option_key(offset + line) ] = !(OPTIONS[ option_key(offset + line) ]);
            } else {
                OPTIONS[ option_key(offset + line) ]++;
                if((OPTIONS[ option_key(offset + line) ]) >= option_max_options(option_key(offset + line))) {
                    OPTIONS[ option_key(offset + line) ] = option_min_options(option_key(offset + line));
                }
            }
            changed_options = true;
            break;
        }
        if(changed_options && OPTIONS["SEASON_LENGTH"] < 1) { OPTIONS["SEASON_LENGTH"]=option_max_options(OPT_SEASON_LENGTH)-1; }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if(changed_options)
    {
        if(query_yn(_("Save changes?")))
        {
            save_options();
            trigdist=(OPTIONS["CIRCLEDIST"] ? true : false);
        }
        else
        {
            // Player wants to keep the old options. Revert!
            OPTIONS = OPTIONS_OLD;
        }
    }
    werase(w_options);
    */
}

/*
void load_options()
{
    std::ifstream fin;
    fin.open("data/options.txt");
    if(!fin.is_open()) {
        fin.close();
        create_default_options();
        fin.open("data/options.txt");
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create ./data/options.txt\n";
            return;
        }
    }

    while(!fin.eof()) {
        std::string id;
        fin >> id;
        if(id == "") {
            getline(fin, id);    // Empty line, chomp it
        } else if(id[0] == '#') { // # indicates a comment
            getline(fin, id);
        } else {
            option_key key = lookup_option_key(id);
            if(key == OPT_NULL) {
                DebugLog() << "Bad option: " << id << "\n";
                getline(fin, id);
            } else if(option_is_bool(key)) {
                std::string val;
                fin >> val;
                if(val == "T") {
                    OPTIONS[key] = 1.;
                } else {
                    OPTIONS[key] = 0.;
                }
            } else {
                std::string check;
                double val;
                fin >> check;

                if(check == "T" || check == "F") {
                    val = (check == "T") ? 1.: 0.;
                } else {
                    val = atoi(check.c_str());
                }

                // Sanitize option values that are out of range.
                val = std::min(val, (double)option_max_options(key));
                val = std::max(val, (double)option_min_options(key));

                OPTIONS[key] = val;
            }
        }
        if(fin.peek() == '\n') {
            getline(fin, id);    // Chomp
        }
    }
    fin.close();
    trigdist=false; // cache to global due to heavy usage.
    if(OPTIONS["CIRCLEDIST"]) { trigdist=true; }
}

void create_default_options()
{
    std::ofstream fout;
    fout.open("data/options.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << options_header() << "\n\
# If true, use C not F\n\
use_celsius F\n\
# If true, use Km/h not mph\
use_metric_speeds F\n\
# If true, use kg not lbs\
use_metric_weights F\n\
# If true, y/n prompts are case-sensitive, y and n are not accepted\n\
force_capital_yn T\n\
# If true, bright backgrounds are not used--some consoles are not compatible\n\
no_bright_backgrounds F\n\
# 12h/24h Time: 0 = AM/PM, 1 = 24h military, 2 = 24h normal\n\
24_hour 0\n\
# If true, automatically follow the crosshair when firing/throwing\n\
snap_to_target F\n\
# If true, safemode will be on after starting a new game or loading\n\
safemode T\n\
# If safemode is enabled, distance to hostiles when safemode should show a warning (0=Viewdistance)\n\
safemodeproximity 0\n\
# If true, auto-safemode will be on after starting a new game or loading\n\
autosafemode F\n\
# Number of turns after safemode is reenabled when no zombies are in safemodeproximity distance\n\
autosafemodeturns 50\n\
# If true, game will periodically save the map\n\
autosave F\n\
# Minutes between autosaves\n\
autosave_turns 30\n\
# Minimum real time minutes between autosaves\n\
autosave_minutes 5\n\
# If true will add nice gradual-lighting (should only make a difference @night)\n\
gradual_night_light T\n\
# If true, will display weather animations\n\
rain_animation T\n\
# If true, compute distance with real math\n\
circledist F\n\
# If true, will query beefore disassembling items\n\
query_disassemble T\n\
# Player will automatically drop empty containers after use\n\
# 0 - don't drop any, 1 - drop all except watertight containers, 2 - drop all containers\n\
drop_empty 0\n\
# Hide Mouse Cursor\n\
# 0 - Cursor always shown, 1 - Cursor always hidden, 2 - Cursor shown on mouse input and hidden on keyboard input\n\
hide_cursor 0\n\
# Level of skill rust: 0 - vanilla Cataclysm; 1 - vanilla, capped at skill levels; 2 - intelligence dependent; 3 - intelligence dependent, capped; 4 - none at all\n\
skill_rust 0\n\
# Delete world after player death: 0 - no, 1 - yes, 2 - query\n\
delete_world 0\n\
# Initial points available in character generation\n\
initial_points 6\n\
# Maximum trait points allowed in character generation\n\
max_trait_points 12\n\
# Initial time at character generation\n\
initial_time 8\n\
# The width of the terrain window in characters.\n\
viewport_x 12\n\
# The height of the terrain window, which is also the height of the main window, in characters.\n\
viewport_y 12\n\
# How many squares to shift the view when using move view keys (HJKLYUBN).\n\
move_view_offset 1\n\
# Spawn zombies at game start instead of during the game.  You must create a new world after changing\n\
static_spawn T\n\
# Only spawn classic zombies and natural wildlife.  You must create a new world after changing\n\
classic_zombies F\n\
# Allow zombies to revive after a certain amount of time.\n\
revive_zombies T\n\
# Season length in days\n\
season_length 14\n\
# Spawn static NPCs at start. Requires reset after changing.\n\
static_npc F\n\
# Spawn random NPCs during gameplay.\n\
random_npc F\n\
# Radiation causes mutations.\n\
rad_mutation T\n\
# Ask to save before sleeping.\n\
save_sleep F\n\
# Enable Auto Pickup. Manage pickup rules in the help menu. ?3\n\
auto_pickup F\n\
# Auto pickup items with 0 Volume or Weight\n\
auto_pickup_zero F\n\
# Auto pickup is disabled as long as you can see monsters nearby.\n\
auto_pickup_safemode F\n\
# Pickup items that exceed dangerous weight limit 0\n\
dangerous_pickups F\n\
# Sort the crafting menu so things you can craft are at the front of the menu\n\
sort_crafting T\n\
";
    fout.close();
}

std::string options_header()
{
    return "\
# This is the options file.  It works similarly to keymap.txt: the format is\n\
# <option name> <option value>\n\
# <option value> may be any number, positive or negative.  If you use a\n\
# negative sign, do not put a space between it and the number.\n\
#\n\
# If # is at the start of a line, it is considered a comment and is ignored.\n\
# In-line commenting is not allowed.  I think.\n\
#\n\
# If you want to restore the default options, simply delete this file.\n\
# A new options.txt will be created next time you play.\n\
\n\
";
}

void save_options()
{
    std::ofstream fout;
    fout.open("data/options.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << options_header() << std::endl;
    for(int i = 1; i < NUM_OPTION_KEYS; i++) {
        option_key key = option_key(i);
        fout << option_string(key) << " ";
        if(option_is_bool(key)) {
            fout << (OPTIONS[key] ? "T" : "F");
        } else {
            fout << OPTIONS[key];
        }
        fout << "\n";
    }
}
*/
