#include "game.h"
#include "options.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"

#include <stdlib.h>
#include <fstream>
#include <string>
#include <algorithm>

option_table OPTIONS;

bool trigdist;

option_key lookup_option_key(std::string id);
bool option_is_bool(option_key id);
void create_default_options();
std::string options_header();

void game::show_options()
{
    // Remember what the options were originally so we can restore them if player cancels.
    option_table OPTIONS_OLD = OPTIONS;

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
        if(changed_options && OPTIONS[OPT_SEASON_LENGTH] < 1) { OPTIONS[OPT_SEASON_LENGTH]=option_max_options(OPT_SEASON_LENGTH)-1; }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    if(changed_options)
    {
        if(query_yn(_("Save changes?")))
        {
            save_options();
            trigdist=(OPTIONS[OPT_CIRCLEDIST] ? true : false);
        }
        else
        {
            // Player wants to keep the old options. Revert!
            OPTIONS = OPTIONS_OLD;
        }
    }
    werase(w_options);
}

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
    if(OPTIONS[OPT_CIRCLEDIST]) { trigdist=true; }
}

option_key lookup_option_key(std::string id)
{
    if(id == "use_celsius") {
        return OPT_USE_CELSIUS;
    }
    if(id == "use_metric_system") {
        return OPT_USE_METRIC_SYS;
    }
    if(id == "force_capital_yn") {
        return OPT_FORCE_YN;
    }
    if(id == "no_bright_backgrounds") {
        return OPT_NO_CBLINK;
    }
    if(id == "24_hour") {
        return OPT_24_HOUR;
    }
    if(id == "snap_to_target") {
        return OPT_SNAP_TO_TARGET;
    }
    if(id == "safemode") {
        return OPT_SAFEMODE;
    }
    if(id == "safemodeproximity") {
        return OPT_SAFEMODEPROXIMITY;
    }
    if(id == "autosafemode") {
        return OPT_AUTOSAFEMODE;
    }
    if(id == "autosafemodeturns") {
        return OPT_AUTOSAFEMODETURNS;
    }
    if(id == "autosave") {
        return OPT_AUTOSAVE;
    }
    if(id == "autosave_turns") {
        return OPT_AUTOSAVE_TURNS;
    }
    if(id == "autosave_minutes") {
        return OPT_AUTOSAVE_MINUTES;
    }
    if(id == "gradual_night_light") {
        return OPT_GRADUAL_NIGHT_LIGHT;
    }
    if(id == "rain_animation") {
        return OPT_RAIN_ANIMATION;
    }
    if(id == "circledist") {
        return OPT_CIRCLEDIST;
    }
    if(id == "query_disassemble") {
        return OPT_QUERY_DISASSEMBLE;
    }
    if(id == "drop_empty") {
        return OPT_DROP_EMPTY;
    }
    if(id == "skill_rust") {
        return OPT_SKILL_RUST;
    }
    if(id == "delete_world") {
        return OPT_DELETE_WORLD;
    }
    if(id == "initial_points") {
        return OPT_INITIAL_POINTS;
    }
    if(id == "max_trait_points") {
        return OPT_MAX_TRAIT_POINTS;
    }
    if(id == "initial_time") {
        return OPT_INITIAL_TIME;
    }
    if(id == "viewport_x") {
        return OPT_VIEWPORT_X;
    }
    if(id == "viewport_y") {
        return OPT_VIEWPORT_Y;
    }
    if(id == "move_view_offset") {
        return OPT_MOVE_VIEW_OFFSET;
    }
    if(id == "static_spawn") {
        return OPT_STATIC_SPAWN;
    }
    if(id == "classic_zombies") {
        return OPT_CLASSIC_ZOMBIES;
    }
    if(id == "revive_zombies") {
        return OPT_REVIVE_ZOMBIES;
    }
    if(id == "season_length") {
        return OPT_SEASON_LENGTH;
    }
    if(id == "static_npc") {
        return OPT_STATIC_NPC;
    }
    if(id == "random_npc") {
        return OPT_RANDOM_NPC;
    }
    if(id == "rad_mutation") {
        return OPT_RAD_MUTATION;
    }
    if(id == "save_sleep") {
        return OPT_SAVESLEEP;
    }
    if(id == "hide_cursor") {
        return OPT_HIDE_CURSOR;
    }
    if(id == "auto_pickup") {
        return OPT_AUTO_PICKUP;
    }
    if(id == "auto_pickup_zero") {
        return OPT_AUTO_PICKUP_ZERO;
    }
    if(id == "auto_pickup_safemode") {
        return OPT_AUTO_PICKUP_SAFEMODE;
    }
    if(id == "sort_crafting") {
        return OPT_SORT_CRAFTING;
    }

    return OPT_NULL;
}

std::string option_string(option_key key)
{
    switch(key) {
    case OPT_USE_CELSIUS:         return "use_celsius";
    case OPT_USE_METRIC_SYS:      return "use_metric_system";
    case OPT_FORCE_YN:            return "force_capital_yn";
    case OPT_NO_CBLINK:           return "no_bright_backgrounds";
    case OPT_24_HOUR:             return "24_hour";
    case OPT_SNAP_TO_TARGET:      return "snap_to_target";
    case OPT_SAFEMODE:            return "safemode";
    case OPT_SAFEMODEPROXIMITY:   return "safemodeproximity";
    case OPT_AUTOSAFEMODE:        return "autosafemode";
    case OPT_AUTOSAFEMODETURNS:   return "autosafemodeturns";
    case OPT_AUTOSAVE:            return "autosave";
    case OPT_AUTOSAVE_TURNS:      return "autosave_turns";
    case OPT_AUTOSAVE_MINUTES:    return "autosave_minutes";
    case OPT_GRADUAL_NIGHT_LIGHT: return "gradual_night_light";
    case OPT_RAIN_ANIMATION:      return "rain_animation";
    case OPT_CIRCLEDIST:          return "circledist";
    case OPT_QUERY_DISASSEMBLE:   return "query_disassemble";
    case OPT_DROP_EMPTY:          return "drop_empty";
    case OPT_SKILL_RUST:          return "skill_rust";
    case OPT_DELETE_WORLD:        return "delete_world";
    case OPT_INITIAL_POINTS:      return "initial_points";
    case OPT_MAX_TRAIT_POINTS:    return "max_trait_points";
    case OPT_INITIAL_TIME:        return "initial_time";
    case OPT_VIEWPORT_X:          return "viewport_x";
    case OPT_VIEWPORT_Y:          return "viewport_y";
    case OPT_MOVE_VIEW_OFFSET:    return "move_view_offset";
    case OPT_STATIC_SPAWN:        return "static_spawn";
    case OPT_CLASSIC_ZOMBIES:     return "classic_zombies";
    case OPT_REVIVE_ZOMBIES:      return "revive_zombies";
    case OPT_SEASON_LENGTH:       return "season_length";
    case OPT_STATIC_NPC:          return "static_npc";
    case OPT_RANDOM_NPC:          return "random_npc";
    case OPT_RAD_MUTATION:        return "rad_mutation";
    case OPT_SAVESLEEP:           return "save_sleep";
    case OPT_HIDE_CURSOR:         return "hide_cursor";
    case OPT_AUTO_PICKUP:         return "auto_pickup";
    case OPT_AUTO_PICKUP_ZERO:    return "auto_pickup_zero";
    case OPT_AUTO_PICKUP_SAFEMODE:return "auto_pickup_safemode";
    case OPT_SORT_CRAFTING:       return "sort_crafting";
    default:                      return "unknown_option";
    }
    return "unknown_option";
}

std::string option_desc(option_key key)
{
    switch(key) {
    case OPT_USE_CELSIUS:         return _("If true, use Celcius not Fahrenheit.\nDefault is fahrenheit");
    case OPT_USE_METRIC_SYS:      return _("If true, use Km/h not mph.\nDefault is mph");
    case OPT_FORCE_YN:            return _("If true, y/n prompts are case-\nsensitive and y and n\nare not accepted.\nDefault is true");
    case OPT_NO_CBLINK:           return _("If true, bright backgrounds are not\nused--some consoles are not\ncompatible.\nDefault is false");
    case OPT_24_HOUR:             return _("12h/24h Time:\n0 - AM/PM (default)  eg: 7:31 AM\n1 - 24h military     eg: 0731\n2 - 24h normal       eg: 7:31");
    case OPT_SNAP_TO_TARGET:      return _("If true, automatically follow the\ncrosshair when firing/throwing.\nDefault is false");
    case OPT_SAFEMODE:            return _("If true, safemode will be on after\nstarting a new game or loading.\nDefault is true");
    case OPT_SAFEMODEPROXIMITY:   return _("If safemode is enabled,\ndistance to hostiles when safemode\nshould show a warning.\n0=Viewdistance, and the default");
    case OPT_AUTOSAFEMODE:        return _("If true, auto-safemode will be on\nafter starting a new game or loading.\nDefault is false");
    case OPT_AUTOSAFEMODETURNS:   return _("Number of turns after safemode\nis reenabled if no hostiles are\nin safemodeproximity distance.\nDefault is 50");
    case OPT_AUTOSAVE:            return _("If true, game will periodically\nsave the map\nDefault is false");
    case OPT_AUTOSAVE_TURNS:      return _("Number of minutes between autosaves");
    case OPT_AUTOSAVE_MINUTES:    return _("Minimum number of real time minutes\nbetween autosaves");
    case OPT_GRADUAL_NIGHT_LIGHT: return _("If true will add nice gradual-lighting\nshould only make a difference\nduring the night.\nDefault is true");
    case OPT_RAIN_ANIMATION:      return _("If true, will display weather\nanimations.\nDefault is true");
    case OPT_CIRCLEDIST:          return _("If true, the game will calculate\nrange in a realistic way:\nlight sources will be circles\ndiagonal movement will\ncover more ground and take\nlonger.\nIf disabled, everything is\nsquare: moving to the northwest\ncorner of a building\ntakes as long as moving\nto the north wall.");
    case OPT_QUERY_DISASSEMBLE:   return _("If true, will query before\ndisassembling items.\nDefault is true");
    case OPT_DROP_EMPTY:          return _("Set to drop empty containers after\nuse.\n0 - don't drop any (default)\n1 - all except watertight containers\n2 - all containers");
    case OPT_SKILL_RUST:          return _("Set the level of skill rust.\n0 - vanilla Cataclysm (default)\n1 - capped at skill levels\n2 - intelligence dependent\n3 - intelligence dependent, capped\n4 - none at all");
    case OPT_DELETE_WORLD:        return _("Delete saves upon player death.\n0 - no (default)\n1 - yes\n2 - query");
    case OPT_INITIAL_POINTS:      return _("Initial points available on character\ngeneration.\nDefault is 6");
    case OPT_MAX_TRAIT_POINTS:    return _("Maximum trait points available for\ncharacter generation.\nDefault is 12");
    case OPT_INITIAL_TIME:        return _("Initial starting time of day on\ncharacter generation.\nDefault is 8:00");
    case OPT_VIEWPORT_X:          return _("WINDOWS ONLY: Set the expansion of the\nviewport along the X axis.\nRequires restart.\nDefault is 12.\nPOSIX systems will use terminal size\nat startup.");
    case OPT_VIEWPORT_Y:          return _("WINDOWS ONLY: Set the expansion of the\nviewport along the Y axis.\nRequires restart.\nDefault is 12.\nPOSIX systems will use terminal size\nat startup.");
    case OPT_MOVE_VIEW_OFFSET:    return _("Move view by how many squares per\nkeypress.\nDefault is 1");
    case OPT_SEASON_LENGTH:       return _("Season length, in days.\nDefault is 14");
    case OPT_STATIC_SPAWN:        return _("Spawn zombies at game start instead of\nduring game. Must reset world\ndirectory after changing for it to\ntake effect.\nDefault is true");
    case OPT_CLASSIC_ZOMBIES:     return _("Only spawn classic zombies and natural\nwildlife. Requires a reset of\nsave folder to take effect.\nThis disables certain buildings.\nDefault is false");
    case OPT_REVIVE_ZOMBIES:      return _("Allow zombies to revive after\na certain amount of time.\nDefault is true");
    case OPT_STATIC_NPC:          return _("If true, the game will spawn static\nNPC at the start of the game,\nrequires world reset.\nDefault is false");
    case OPT_RANDOM_NPC:          return _("If true, the game will randomly spawn\nNPC during gameplay.\nDefault is false");
    case OPT_RAD_MUTATION:        return _("If true, radiation causes the player\nto mutate.\nDefault is true");
    case OPT_SAVESLEEP:           return _("If true, game will ask to save the map\nbefore sleeping. Default is false");
    case OPT_HIDE_CURSOR:         return _("If 0, cursor is always shown. If 1,\ncursor is hidden. If 2, cursor is\nhidden on keyboard input and\nunhidden on mouse movement.\nDefault is 0.");
    case OPT_AUTO_PICKUP:         return _("Enable item auto pickup. Change\npickup rules with the Auto Pickup\nManager in the Help Menu ?3");
    case OPT_AUTO_PICKUP_ZERO:    return _("Auto pickup items with\n0 Volume or Weight");
    case OPT_AUTO_PICKUP_SAFEMODE:return _("Auto pickup is disabled\nas long as you can see\nmonsters nearby.\n\nThis is affected by\nSafemode proximity distance.");
    case OPT_SORT_CRAFTING:       return _("If true, the crafting menus\nwill display recipes that you can\ncraft before other recipes");
    default:                      return " ";
    }
    return "Big ol Bug (options.cpp:option_desc)";
}

std::string option_name(option_key key)
{
    switch(key) {
    case OPT_USE_CELSIUS:         return _("Use Celsius");
    case OPT_USE_METRIC_SYS:      return _("Use Metric System");
    case OPT_FORCE_YN:            return _("Force Y/N in prompts");
    case OPT_NO_CBLINK:           return _("No Bright Backgrounds");
    case OPT_24_HOUR:             return _("24 Hour Time");
    case OPT_SNAP_TO_TARGET:      return _("Snap to Target");
    case OPT_SAFEMODE:            return _("Safemode on by default");
    case OPT_SAFEMODEPROXIMITY:   return _("Safemode proximity distance");
    case OPT_AUTOSAFEMODE:        return _("Auto-Safemode on by default");
    case OPT_AUTOSAFEMODETURNS:   return _("Turns to reenable safemode");
    case OPT_AUTOSAVE:            return _("Periodically Autosave");
    case OPT_AUTOSAVE_TURNS:      return _("Game minutes between autosaves");
    case OPT_AUTOSAVE_MINUTES:    return _("Real minutes between autosaves");
    case OPT_GRADUAL_NIGHT_LIGHT: return _("Gradual night light");
    case OPT_RAIN_ANIMATION:      return _("Rain animation");
    case OPT_CIRCLEDIST:          return _("Circular distances");
    case OPT_QUERY_DISASSEMBLE:   return _("Query on disassembly");
    case OPT_DROP_EMPTY:          return _("Drop empty containers");
    case OPT_SKILL_RUST:          return _("Skill Rust");
    case OPT_DELETE_WORLD:        return _("Delete World");
    case OPT_INITIAL_POINTS:      return _("Initial points");
    case OPT_MAX_TRAIT_POINTS:    return _("Maximum trait points");
    case OPT_INITIAL_TIME:        return _("Initial time");
    case OPT_VIEWPORT_X:          return _("Viewport width");
    case OPT_VIEWPORT_Y:          return _("Viewport height");
    case OPT_MOVE_VIEW_OFFSET:    return _("Move view offset");
    case OPT_STATIC_SPAWN:        return _("Static spawn");
    case OPT_CLASSIC_ZOMBIES:     return _("Classic zombies");
    case OPT_REVIVE_ZOMBIES:      return _("Revive zombies");
    case OPT_SEASON_LENGTH:       return _("Season length");
    case OPT_STATIC_NPC:          return _("Static npcs");
    case OPT_RANDOM_NPC:          return _("Random npcs");
    case OPT_RAD_MUTATION:        return _("Mutations by radiation");
    case OPT_SAVESLEEP:           return _("Ask to save before sleeping");
    case OPT_HIDE_CURSOR:         return _("Hide Mouse Cursor");
    case OPT_AUTO_PICKUP:         return _("Enable item Auto Pickup");
    case OPT_AUTO_PICKUP_ZERO:    return _("Auto Pickup 0 Vol/Weight");
    case OPT_AUTO_PICKUP_SAFEMODE:return _("Auto Pickup Safemode");
    case OPT_SORT_CRAFTING:       return _("Sort Crafting menu");
    default:                      return "Unknown Option (options.cpp:option_name)";
    }
    return "Big ol Bug (options.cpp:option_name)";
}

bool option_is_bool(option_key id)
{
    switch(id) {
    case OPT_24_HOUR:
    case OPT_SAFEMODEPROXIMITY:
    case OPT_AUTOSAFEMODETURNS:
    case OPT_SKILL_RUST:
    case OPT_DROP_EMPTY:
    case OPT_DELETE_WORLD:
    case OPT_INITIAL_POINTS:
    case OPT_MAX_TRAIT_POINTS:
    case OPT_INITIAL_TIME:
    case OPT_VIEWPORT_X:
    case OPT_VIEWPORT_Y:
    case OPT_SEASON_LENGTH:
    case OPT_MOVE_VIEW_OFFSET:
    case OPT_AUTOSAVE_TURNS:
    case OPT_AUTOSAVE_MINUTES:
    case OPT_HIDE_CURSOR:
        return false;
        break;
    default:
        return true;
    }
    return true;
}

char option_max_options(option_key id)
{
    char ret;
    if(option_is_bool(id)) {
        ret = 2;
    } else
        switch(id) {
        case OPT_24_HOUR:
            ret = 3;
            break;
        case OPT_SAFEMODEPROXIMITY:
            ret = 61;
            break;
        case OPT_AUTOSAFEMODETURNS:
            ret = 101;
            break;
        case OPT_INITIAL_POINTS:
            ret = 25;
            break;
        case OPT_MAX_TRAIT_POINTS:
            ret = 25;
            break;
        case OPT_INITIAL_TIME:
            ret = 24; // 0h to 23h
            break;
        case OPT_DELETE_WORLD:
        case OPT_DROP_EMPTY:
            ret = 3;
            break;
        case OPT_SKILL_RUST:
            ret = 5;
            break;
        case OPT_VIEWPORT_X:
        case OPT_VIEWPORT_Y:
            ret = 93; // TODO Set up min/max values so weird numbers don't have to be used.
            break;
        case OPT_SEASON_LENGTH:
            ret = 127;
            break;
        case OPT_MOVE_VIEW_OFFSET:
            ret = 50; // TODO calculate max for screen size
            break;
        case OPT_AUTOSAVE_TURNS:
        case OPT_AUTOSAVE_MINUTES:
            ret = 127;
            break;
        case OPT_HIDE_CURSOR:
            ret = 3;
            break;
        default:
            ret = 2;
            break;
        }
    return ret;
}

char option_min_options(option_key id)
{
    char ret;
    if(option_is_bool(id)) {
        ret = 0;
    } else
        switch(id) {
        case OPT_MAX_TRAIT_POINTS:
            ret = 3;
            break;
        case OPT_VIEWPORT_X:
        case OPT_VIEWPORT_Y:
            ret = 12; // TODO Set up min/max values so weird numbers don't have to be used.
            break;
        case OPT_AUTOSAVE_TURNS:
            ret = 1;
            break;
        default:
            ret = 0;
            break;
        }
    return ret;
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
use_metric_system F\n\
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
# \n\
# GAMEPLAY OPTIONS: CHANGING THESE OPTIONS WILL AFFECT GAMEPLAY DIFFICULTY! \n\
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
