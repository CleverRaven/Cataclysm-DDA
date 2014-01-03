#include "game.h"
#include "options.h"
#include "output.h"
#include "debug.h"
#include "keypress.h"
#include "translations.h"
#include "file_finder.h"
#include "cursesdef.h"
#ifdef SDLTILES
#include "cata_tiles.h"
#endif // SDLTILES

#include <stdlib.h>
#include <fstream>
#include <string>
#include <locale>
#include <sstream>

bool trigdist;
bool use_tiles;

bool used_tiles_changed;
#ifdef SDLTILES
extern cata_tiles *tilecontext;
#endif // SDLTILES

std::map<std::string, cOpt> OPTIONS;
std::map<std::string, cOpt> ACTIVE_WORLD_OPTIONS;
options_data optionsdata; // store extranious options data that doesn't need to be in OPTIONS, 
std::vector<std::pair<std::string, std::string> > vPages;
std::map<int, std::vector<std::string> > mPageItems;
std::map<std::string, std::string> optionNames;
int iWorldOptPage;

options_data::options_data() {
    enable_json("DEFAULT_REGION");
    // to allow class based init_data functions to add values to a 'string' type option, add:
    //   enable_json("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON");
    // also, in options.h, add this before 'class options_data'
    //   class my_class;
    // and inside options_data above public:
    //   friend class my_class;
    // then, in the my_class::load_json (or post-json setup) method:
    //   optionsdata.addme("OPTION_KEY_THAT_GETS_STRING_ENTRIES_ADDED_VIA_JSON", "thisvalue");
}

//Default constructor
cOpt::cOpt() {
    sType = "VOID";
    sPage = "";
}

//string constructor
cOpt::cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const std::string sItemsIn, std::string sDefaultIn) {
    sPage = sPageIn;
    sMenuText = sMenuTextIn;
    sTooltip = sTooltipIn;
    sType = "string";

    std::stringstream ssTemp(sItemsIn);
    std::string sItem;
    while (std::getline(ssTemp, sItem, ',')) {
        vItems.push_back(sItem);
    }

    if (getItemPos(sDefaultIn) == -1) {
        sDefaultIn = vItems[0];
    }

    sDefault = sDefaultIn;
    sSet = sDefaultIn;
}

//bool constructor
cOpt::cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const bool bDefaultIn) {
    sPage = sPageIn;
    sMenuText = sMenuTextIn;
    sTooltip = sTooltipIn;
    sType = "bool";

    bDefault = bDefaultIn;
    bSet = bDefaultIn;
}

//int constructor
cOpt::cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn, const int iMinIn, int iMaxIn, int iDefaultIn) {
    sPage = sPageIn;
    sMenuText = sMenuTextIn;
    sTooltip = sTooltipIn;
    sType = "int";

    if (iMinIn > iMaxIn) {
        iMaxIn = iMinIn;
    }

    iMin = iMinIn;
    iMax = iMaxIn;

    if (iDefaultIn < iMinIn || iDefaultIn > iMaxIn) {
        iDefaultIn = iMinIn ;
    }

    iDefault = iDefaultIn;
    iSet = iDefaultIn;
}

//float constructor
cOpt::cOpt(const std::string sPageIn, const std::string sMenuTextIn, const std::string sTooltipIn,
     const float fMinIn, float fMaxIn, float fDefaultIn, float fStepIn) {
    sPage = sPageIn;
    sMenuText = sMenuTextIn;
    sTooltip = sTooltipIn;
    sType = "float";

    if (fMinIn > fMaxIn) {
        fMaxIn = fMinIn;
    }

    fMin = fMinIn;
    fMax = fMaxIn;
    fStep = fStepIn;

    if (fDefaultIn < fMinIn || fDefaultIn > fMaxIn) {
        fDefaultIn = fMinIn ;
    }

    fDefault = fDefaultIn;
    fSet = fDefaultIn;
}

//helper functions
std::string cOpt::getPage() {
    return sPage;
}

std::string cOpt::getMenuText() {
    return sMenuText;
}

std::string cOpt::getTooltip() {
    return sTooltip;
}

std::string cOpt::getType() {
    return sType;
}

std::string cOpt::getValue() {
    if (sType == "string") {
        return sSet;

    } else if (sType == "bool") {
        return (bSet) ? "true" : "false";

    } else if (sType == "int") {
        std::stringstream ssTemp;
        ssTemp << iSet;
        return ssTemp.str();

    } else if (sType == "float") {
        std::stringstream ssTemp;
        ssTemp.imbue(std::locale("C"));
        const int precision = (fStep >= 0.09) ? 1 : (fStep >= 0.009) ? 2 : (fStep >= 0.0009) ? 3 : 4;
        ssTemp.precision(precision);
        ssTemp << std::fixed << fSet;
        return ssTemp.str();
    }

    return "";
}

std::string cOpt::getValueName() {
    if (sType == "string") {
        return optionNames[sSet];

    } else if (sType == "bool") {
        return (bSet) ? _("True") : _("False");
    }

    return getValue();
}

std::string cOpt::getDefaultText() {
    if (sType == "string") {
        std::string sItems = "";
        for (int i = 0; i < vItems.size(); i++) {
            if (sItems != "") {
                sItems += _(", ");
            }
            sItems += optionNames[vItems[i]];
        }
        return string_format(_("Default: %s - Values: %s"), optionNames[sDefault].c_str(), sItems.c_str());

    } else if (sType == "bool") {
        return (bDefault) ? _("Default: True") : _("Default: False");

    } else if (sType == "int") {
        return string_format(_("Default: %d - Min: %d, Max: %d"), iDefault, iMin, iMax);

    } else if (sType == "float") {
        return string_format(_("Default: %.2f - Min: %.2f, Max: %.2f"), fDefault, fMin, fMax);
    }

    return "";
}

int cOpt::getItemPos(const std::string sSearch) {
    if (sType == "string") {
        for (int i = 0; i < vItems.size(); i++) {
            if (vItems[i] == sSearch) {
                return i;
            }
        }
    }

    return -1;
}

//set to next item
void cOpt::setNext() {
    if (sType == "string") {
        int iNext = getItemPos(sSet)+1;
        if (iNext >= vItems.size()) {
            iNext = 0;
        }

        sSet = vItems[iNext];

    } else if (sType == "bool") {
        bSet = !bSet;

    } else if (sType == "int") {
        iSet++;
        if (iSet > iMax) {
            iSet = iMin;
        }

    } else if (sType == "float") {
        fSet += fStep;
        if (fSet > fMax) {
            fSet = fMin;
        }
    }
}

//set to prev item
void cOpt::setPrev() {
    if (sType == "string") {
        int iPrev = getItemPos(sSet)-1;
        if (iPrev < 0) {
            iPrev = vItems.size()-1;
        }

        sSet = vItems[iPrev];

    } else if (sType == "bool") {
        bSet = !bSet;

    } else if (sType == "int") {
        iSet--;
        if (iSet < iMin) {
            iSet = iMax;
        }

    } else if (sType == "float") {
        fSet -= fStep;
        if (fSet < fMin) {
            fSet = fMax;
        }
    }
}

//set value
void cOpt::setValue(std::string sSetIn) {
    if (sType == "string") {
        if (getItemPos(sSetIn) != -1) {
            sSet = sSetIn;
        }

    } else if (sType == "bool") {
        bSet = (sSetIn == "True" || sSetIn == "true" || sSetIn == "T" || sSetIn == "t");

    } else if (sType == "int") {
        iSet = atoi(sSetIn.c_str());

        if ( iSet < iMin || iSet > iMax ) {
            iSet = iDefault;
        }

    } else if (sType == "float") {
        std::istringstream ssTemp(sSetIn);
        ssTemp.imbue(std::locale("C"));
        float tmpFloat;
        ssTemp >> tmpFloat;
        if(ssTemp) {
            fSet = tmpFloat;
        } else {
            debugmsg("invalid floating point option: %s", sSetIn.c_str());
        }
        if ( fSet < fMin || fSet > fMax ) {
            fSet = fDefault;
        }
    }
}

//Set default class behaviour to float
cOpt::operator float() const {
    if (sType == "string") {
        return (sSet != "" && sSet == sDefault) ? 1.0 : 0.0;

    } else if (sType == "bool") {
        return (bSet) ? 1.0 : 0.0;

    } else if (sType == "int") {
        return (float)iSet;

    } else if (sType == "float") {
        return fSet;
    }

    return 0.0;
}

// if (class == "string")
bool cOpt::operator==(const std::string sCompare) const {
    return (sType == "string" && sSet == sCompare);
}

// if (class != "string")
bool cOpt::operator!=(const std::string sCompare) const {
    return !(*this == sCompare);
}

void initOptions() {
    vPages.clear();
    vPages.push_back(std::make_pair("general", _("General")));
    vPages.push_back(std::make_pair("interface", _("Interface")));
    vPages.push_back(std::make_pair("graphics", _("Graphics")));
    vPages.push_back(std::make_pair("debug", _("Debug")));
    iWorldOptPage = vPages.size();
    vPages.push_back(std::make_pair("world_default", _("World Defaults")));

    OPTIONS.clear();

    std::string tileset_names;
    tileset_names = get_tileset_names("gfx");      //get the tileset names and set the optionNames

    optionNames["fahrenheit"] = _("Fahrenheit");
    optionNames["celsius"] = _("Celsius");
    OPTIONS["USE_CELSIUS"] =            cOpt("interface", _("Temperature units"),
                                             _("Switch between Celsius and Fahrenheit."),
                                             "fahrenheit,celsius", "fahrenheit"
                                            );

    optionNames["mph"] = _("mph");
    optionNames["km/h"] = _("km/h");
    OPTIONS["USE_METRIC_SPEEDS"] =      cOpt("interface", _("Speed units"),
                                             _("Switch between km/h and mph."),
                                             "mph,km/h", "mph"
                                            );

    optionNames["lbs"] = _("lbs");
    optionNames["kg"] = _("kg");
    OPTIONS["USE_METRIC_WEIGHTS"] =     cOpt("interface", _("Mass units"),
                                             _("Switch between kg and lbs."),
                                             "lbs,kg", "lbs"
                                            );

    OPTIONS["FORCE_CAPITAL_YN"] =       cOpt("interface", _("Force Y/N in prompts"),
                                             _("If true, Y/N prompts are case-sensitive and y and n are not accepted."),
                                             true
                                            );

    OPTIONS["NO_BRIGHT_BACKGROUNDS"] =  cOpt("graphics", _("No bright backgrounds"),
                                            _("If true, bright backgrounds are not used - some consoles are not compatible."),
                                             false
                                            );

    //~ 12h time, e.g. 11:59pm
    optionNames["12h"] = _("12h");
    //~ Military time, e.g. 2359
    optionNames["military"] = _("Military");
    //~ 24h time, e.g. 23:59
    optionNames["24h"] = _("24h");
    OPTIONS["24_HOUR"] =                cOpt("interface", _("Time format"),
                                             _("12h: AM/PM, eg: 7:31 AM - Military: 24h Military, eg: 0731 - 24h: Normal 24h, eg: 7:31"),
                                             "12h,military,24h", "12h"
                                            );

    OPTIONS["SNAP_TO_TARGET"] =         cOpt("interface", _("Snap to target"),
                                             _("If true, automatically follow the crosshair when firing/throwing."),
                                             false
                                            );

    OPTIONS["SAFEMODE"] =               cOpt("general", _("Safemode on by default"),
                                             _("If true, safemode will be on after starting a new game or loading."),
                                             true
                                            );

    OPTIONS["VEHICLE_ARMOR_COLOR"] =    cOpt("interface", _("Vehicle plating changes part color"),
                                             _("If true, vehicle parts will change color if they are armor plated"),
                                             true
                                            );

    OPTIONS["SAFEMODEPROXIMITY"] =      cOpt("general", _("Safemode proximity distance"),
                                             _("If safemode is enabled, distance to hostiles when safemode should show a warning. 0 = Max player viewdistance."),
                                             0, 50, 0
                                            );

    OPTIONS["SAFEMODEVEH"] =            cOpt("general", _("Safemode when driving"),
                                             _("When true, safemode will alert you to hostiles whilst you are driving a vehicle."),
                                             false
                                            );

    OPTIONS["AUTOSAFEMODE"] =           cOpt("general", _("Auto-safemode on by default"),
                                             _("If true, auto-safemode will be on after starting a new game or loading."),
                                             false
                                            );


    OPTIONS["AUTOSAFEMODETURNS"] =      cOpt("general", _("Turns to reenable safemode"),
                                             _("Number of turns after safemode is reenabled if no hostiles are in safemodeproximity distance."),
                                             1, 100, 50
                                            );

    OPTIONS["AUTOSAVE"] =               cOpt("general", _("Periodically autosave"),
                                             _("If true, game will periodically save the map."),
                                             false
                                            );

    OPTIONS["AUTOSAVE_TURNS"] =         cOpt("general", _("Game turns between autosaves"),
                                             _("Number of game turns between autosaves"),
                                             1, 1000, 5
                                            );

    OPTIONS["AUTOSAVE_MINUTES"] =       cOpt("general", _("Real minutes between autosaves"),
                                             _("Number of real time minutes between autosaves"),
                                             0, 127, 5
                                            );

    OPTIONS["RAIN_ANIMATION"] =         cOpt("graphics", _("Rain animation"),
                                             _("If true, will display weather animations."),
                                             true
                                            );

    OPTIONS["CIRCLEDIST"] =             cOpt("general", _("Circular distances"),
                                             _("If true, the game will calculate range in a realistic way: light sources will be circles diagonal movement will cover more ground and take longer. If disabled, everything is square: moving to the northwest corner of a building takes as long as moving to the north wall."),
                                             false
                                            );

    OPTIONS["QUERY_DISASSEMBLE"] =      cOpt("interface", _("Query on disassembly"),
                                             _("If true, will query before disassembling items."),
                                             true
                                            );

    optionNames["no"] = _("No");
    //~ containers
    optionNames["watertight"] = _("Watertight");
    optionNames["all"] = _("All");
    OPTIONS["DROP_EMPTY"] =             cOpt("general", _("Drop empty containers"),
                                             _("Set to drop empty containers after use. No: Don't drop any. - Watertight: All except watertight containers. - All: Drop all containers."),
                                             "no,watertight,all", "no"
                                            );

    //~ plain, default, normal
    optionNames["vanilla"] = _("Vanilla");
    //~ capped at a value
    optionNames["capped"] = _("Capped");
    //~ based on intelligence
    optionNames["int"] = _("Int");
    //~ based on intelligence and capped
    optionNames["intcap"] = _("IntCap");
    optionNames["off"] = _("Off");
    OPTIONS["SKILL_RUST"] =             cOpt("debug", _("Skill rust"),
                                             _("Set the level of skill rust. Vanilla: Vanilla Cataclysm - Capped: Capped at skill levels 2 - Int: Intelligence dependent - IntCap: Intelligence dependent, capped - Off: None at all."),
                                             "vanilla,capped,int,intcap,off", "vanilla"
                                            );

    optionNames["no"] = _("No");
    optionNames["yes"] = _("Yes");
    optionNames["query"] = _("Query");
    OPTIONS["DELETE_WORLD"] =           cOpt("world_default", _("Delete world"),
                                             _("Delete the world when the last active character dies."),
                                             "no,yes,query", "no"
                                            );

    OPTIONS["INITIAL_POINTS"] =         cOpt("debug", _("Initial points"),
                                             _("Initial points available on character generation."),
                                             0, 25, 6
                                            );

    OPTIONS["MAX_TRAIT_POINTS"] =       cOpt("debug", _("Maximum trait points"),
                                             _("Maximum trait points available for character generation."),
                                             0, 25, 12
                                            );

    OPTIONS["SPAWN_DENSITY"] =          cOpt("world_default", _("Spawn rate scaling factor"),
                                             _("A scaling factor that determines density of monster spawns."),
                                             0.0, 50.0, 1.0, 0.1
                                            );

    OPTIONS["ITEM_SPAWNRATE"] =         cOpt("world_default", _("Item spawn scaling factor"),
                                             _("A scaling factor that determines density of item spawns."),
                                             0.01, 10.0, 1.0, 0.01
                                            );
    std::string region_ids("default");
    optionNames["default"] = "default";

    OPTIONS["DEFAULT_REGION"] =          cOpt("world_default", _("Default region type"),
                                             _("(WIP feature) Determines terrain, shops, plants, and more."),
                                             region_ids, "default"
                                            );

    OPTIONS["CITY_SIZE"] =              cOpt("world_default", _("Size of cities"),
                                             _("A number determining how large cities are. Warning, large numbers lead to very slow mapgen."),
                                             1, 16, 4
                                            );

    OPTIONS["INITIAL_TIME"] =           cOpt("world_default", _("Initial time"),
                                             _("Initial starting time of day on character generation."),
                                             0, 23, 8
                                            );

    optionNames["spring"] = _("Spring");
    optionNames["summer"] = _("Summer");
    optionNames["autumn"] = _("Autumn");
    optionNames["winter"] = _("Winter");
    OPTIONS["INITIAL_SEASON"] =         cOpt("world_default", _("Initial season"),
                                             _("Initial starting season of day on character generation."),
                                             "spring,summer,autumn,winter", "spring");

    OPTIONS["VIEWPORT_X"] =             cOpt("graphics", _("Viewport width"),
                                             _("SDL ONLY: Set the expansion of the viewport along the X axis. Requires restart. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["VIEWPORT_Y"] =             cOpt("graphics", _("Viewport height"),
                                             _("SDL ONLY: Set the expansion of the viewport along the Y axis. Requires restart. POSIX systems will use terminal size at startup."),
                                             12, 93, 12
                                            );

    OPTIONS["INPUT_DELAY"] =             cOpt("graphics", _("Input delay"),
                                             _("SDL ONLY: Determines how many times per second an action will be performed by holding down a key. The delay is in milliseconds. Requires restart."),
                                             1, 500, 60
                                            );

    optionNames["standard"] = _("Standard");
    //~ sidebar style
    optionNames["narrow"] = _("Narrow");
    OPTIONS["SIDEBAR_STYLE"] =          cOpt("interface", _("Sidebar style"),
                                             _("Switch between the standard or a narrower and taller sidebar. Requires restart."),
                                             "standard,narrow", "standard"
                                            );
    //~ style of vehicle interaction menu; vertical is old one.
    optionNames["vertical"] = _("Vertical");
    optionNames["horizontal"] = _("Horizontal");
    optionNames["hybrid"] = _("Hybrid");
    OPTIONS["VEH_MENU_STYLE"] =         cOpt("interface", _("Vehicle menu style"),
                                             _("Switch between two different styles of vehicle interaction menu or combination of them."),
                                             "vertical,horizontal,hybrid", "vertical"
                                            );

    OPTIONS["MOVE_VIEW_OFFSET"] =       cOpt("interface", _("Move view offset"),
                                             _("Move view by how many squares per keypress."),
                                             1, 50, 1
                                            );

    OPTIONS["STATIC_SPAWN"] =           cOpt("world_default", _("Static spawn"),
                                             _("Spawn zombies at game start instead of during game. Must reset world directory after changing for it to take effect."),
                                             true
                                            );

    OPTIONS["CLASSIC_ZOMBIES"] =        cOpt("world_default", _("Classic zombies"),
                                             _("Only spawn classic zombies and natural wildlife. Requires a reset of save folder to take effect. This disables certain buildings."),
                                             false
                                            );

    OPTIONS["BLACK_ROAD"] =             cOpt("world_default", _("Black Road"),
                                             _("If true, spawn zombies at shelters."),
                                             false
                                            );

    OPTIONS["SEASON_LENGTH"] =          cOpt("world_default", _("Season length"),
                                             _("Season length, in days."),
                                             14, 127, 14
                                            );

    OPTIONS["STATIC_NPC"] =             cOpt("world_default", _("Static npcs"),
                                             _("If true, the game will spawn static NPC at the start of the game, requires world reset."),
                                             false
                                            );

    OPTIONS["RANDOM_NPC"] =             cOpt("world_default", _("Random npcs"),
                                             _("If true, the game will randomly spawn NPC during gameplay."),
                                             false
                                            );

    OPTIONS["RAD_MUTATION"] =           cOpt("world_default", _("Mutations by radiation"),
                                             _("If true, radiation causes the player to mutate."),
                                             true
                                            );

    OPTIONS["SAVE_SLEEP"] =             cOpt("interface", _("Ask to save before sleeping"),
                                             _("If true, game will ask to save the map before sleeping."),
                                             false
                                            );

    //~ show mouse cursor
    optionNames["show"] = _("Show");
    //~ hide mouse cursor
    optionNames["hide"] = _("Hide");
    //~ hide mouse cursor when keyboard is used
    optionNames["hidekb"] = _("HideKB");
    OPTIONS["HIDE_CURSOR"] =            cOpt("interface", _("Hide mouse cursor"),
                                             _("Always: Cursor is always shown. Hidden: Cursor is hidden. HiddenKB: Cursor is hidden on keyboard input and unhidden on mouse movement."),
                                             "show,hide,hidekb", "show"
                                            );

    OPTIONS["MENU_SCROLL"] =            cOpt("interface", _("Centered menu scrolling"),
                                             _("If true, menus will start scrolling in the center of the list, and keep the list centered."),
                                             true
                                            );

    OPTIONS["AUTO_PICKUP"] =            cOpt("general", _("Auto pickup enabled"),
                                             _("Enable item auto pickup. Change pickup rules with the Auto Pickup Manager in the Help Menu ?3"),
                                             false
                                            );

    OPTIONS["AUTO_PICKUP_ZERO"] =       cOpt("general", _("Auto pickup 0 vol light items"),
                                             _("Auto pickup items with 0 Volume, and weight less than or equal to [option] * 50 grams. '0' disables this option"),
                                             0, 20, 0
                                            );

    OPTIONS["AUTO_PICKUP_SAFEMODE"] =   cOpt("general", _("Auto pickup safemode"),
                                             _("Auto pickup is disabled as long as you can see monsters nearby. This is affected by Safemode proximity distance."),
                                             false
                                            );

    OPTIONS["DANGEROUS_PICKUPS"] =      cOpt("general", _("Dangerous pickups"),
                                             _("If false will cause player to drop new items that cause them to exceed the weight limit."),
                                             false
                                            );

    optionNames["false"] = _("False");
    optionNames["centered"] = _("Centered");
    optionNames["edge"] = _("To edge");
    OPTIONS["SHIFT_LIST_ITEM_VIEW"] =   cOpt("interface", _("Shift list item view"),
                                             _("Centered or to edge, shift the view toward the selected item if it is outside of your current viewport."),
                                             "false,centered,edge",  "centered"
                                            );

    OPTIONS["USE_TILES"] =              cOpt("graphics", _("Use tiles"),
                                             _("If true, replaces some TTF rendered text with tiles. Only applicable on SDL builds."),
                                             true
                                            );

    OPTIONS["TILES"] =                  cOpt("graphics", _("Choose tileset"),
                                             _("Choose the tileset you want to use. Only applicable on SDL builds."),
                                             tileset_names, "hoder"
                                            );   // populate the options dynamically

    for (std::map<std::string, cOpt>::iterator iter = OPTIONS.begin(); iter != OPTIONS.end(); ++iter) {
        for (unsigned i=0; i < vPages.size(); ++i) {
            if (vPages[i].first == (iter->second).getPage()) {
                mPageItems[i].push_back(iter->first);
                break;
            }
        }
    }
}

void show_options(bool ingame)
{
    std::map<std::string, cOpt> OPTIONS_OLD = OPTIONS;
    std::map<std::string, cOpt> WOPTIONS_OLD = ACTIVE_WORLD_OPTIONS;
    if ( world_generator->active_world == NULL ) {
        ingame = false;
    }

    const int iTooltipHeight = 4;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[60] = true;

    WINDOW* w_options_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    WINDOW* w_options_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY,
                                       1 + iOffsetX);
    WINDOW* w_options_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY,
                                      1 + iOffsetX);
    WINDOW* w_options = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2,
                               iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    draw_border(w_options_border);
    mvwputch(w_options_border, iTooltipHeight + 1,  0, BORDER_COLOR, LINE_XXXO); // |-
    mvwputch(w_options_border, iTooltipHeight + 1, 79, BORDER_COLOR, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_options_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, BORDER_COLOR, LINE_XXOX); // _|_
    }

    mvwprintz(w_options_border, 0, 36, c_ltred, _(" OPTIONS "));
    wrefresh(w_options_border);

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_options_header, 0, i, BORDER_COLOR, LINE_OXXX);
        } else {
            mvwputch(w_options_header, 0, i, BORDER_COLOR, LINE_OXOX); // Draw header line
        }
    }

    wrefresh(w_options_header);

    int iCurrentPage = 0;
    int iLastPage = 0;
    int iCurrentLine = 0;
    int iStartPos = 0;
    bool bStuffChanged = false;
    bool bWorldStuffChanged = false;
    char ch = ' ';

    std::stringstream sTemp;

    used_tiles_changed = false;

    do {
        std::map<std::string, cOpt> & cOPTIONS = ( ingame && iCurrentPage == iWorldOptPage ?
                                                   ACTIVE_WORLD_OPTIONS : OPTIONS );

        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_options, i, j, BORDER_COLOR, LINE_XOXO);
                } else {
                    mvwputch(w_options, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_options_tooltip, i, j, c_black, ' ');
                }
            }
        }

        calcStartPos(iStartPos, iCurrentLine, iContentHeight, mPageItems[iCurrentPage].size());

        //Draw options
        for (int i = iStartPos; i < iStartPos + ((iContentHeight > mPageItems[iCurrentPage].size()) ?
                                                 mPageItems[iCurrentPage].size() : iContentHeight); i++) {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << i + 1;
            mvwprintz(w_options, i - iStartPos, 0, c_white, sTemp.str().c_str());
            mvwprintz(w_options, i - iStartPos, 4, c_white, "");

            if (iCurrentLine == i) {
                wprintz(w_options, c_yellow, ">> ");
            } else {
                wprintz(w_options, c_yellow, "   ");
            }
            wprintz(w_options, c_white, "%s",
                    (cOPTIONS[mPageItems[iCurrentPage][i]].getMenuText()).c_str());

            if (cOPTIONS[mPageItems[iCurrentPage][i]].getValue() == "false") {
                cLineColor = c_ltred;
            }

            mvwprintz(w_options, i - iStartPos, 62, (iCurrentLine == i) ? hilite(cLineColor) :
                      cLineColor, "%s", (cOPTIONS[mPageItems[iCurrentPage][i]].getValueName()).c_str());
        }

        //Draw Scrollbar
        draw_scrollbar(w_options_border, iCurrentLine, iContentHeight,
                       mPageItems[iCurrentPage].size(), iTooltipHeight+2, 0, BORDER_COLOR);

        //Draw Tabs
        mvwprintz(w_options_header, 0, 7, c_white, "");
        for (unsigned i = 0; i < vPages.size(); i++) {
            if (mPageItems[i].size() > 0) { //skip empty pages
                wprintz(w_options_header, c_white, "[");
                if ( ingame && i == iWorldOptPage ) {
                   wprintz(w_options_header,
                           (iCurrentPage == i) ? hilite(c_ltgreen) : c_ltgreen, _("Current world"));
                } else {
                   wprintz(w_options_header, (iCurrentPage == i) ?
                           hilite(c_ltgreen) : c_ltgreen, (vPages[i].second).c_str());
                }
                wprintz(w_options_header, c_white, "]");
                wputch(w_options_header, BORDER_COLOR, LINE_OXOX);
            }
        }

        wrefresh(w_options_header);

#if (defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
        if (mPageItems[iCurrentPage][iCurrentLine] == "VIEWPORT_X") {
            int new_viewport_x, new_window_width;
            std::stringstream value_conversion(OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getValueName());

            value_conversion >> new_viewport_x;
            new_window_width = projected_window_width(new_viewport_x);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           "%s #%s -- The window will be %d pixels wide with the selected value.",
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str(),
                           new_window_width);
        } else if (mPageItems[iCurrentPage][iCurrentLine] == "VIEWPORT_Y") {
            int new_viewport_y, new_window_height;
            std::stringstream value_conversion(OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getValueName());

            value_conversion >> new_viewport_y;
            new_window_height = projected_window_height(new_viewport_y);

            fold_and_print(w_options_tooltip, 0, 0, 78, c_white,
                           "%s #%s -- The window will be %d pixels tall with the selected value.",
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str(),
                           new_window_height);
        } else
#endif
        {
            fold_and_print(w_options_tooltip, 0, 0, 78, c_white, "%s #%s",
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getTooltip().c_str(),
                           OPTIONS[mPageItems[iCurrentPage][iCurrentLine]].getDefaultText().c_str());
        }

        if ( iCurrentPage != iLastPage ) {
            iLastPage = iCurrentPage;
            if ( ingame && iCurrentPage == iWorldOptPage ) {
                mvwprintz( w_options_tooltip, 3, 3, c_ltred, "%s", _("Note: ") );
                wprintz(  w_options_tooltip, c_white, "%s",
                          _("Some of these options may produce unexpected results if changed."));
            }
        }
        wrefresh(w_options_tooltip);

        wrefresh(w_options);

        ch = input();

        if (mPageItems[iCurrentPage].size() > 0 || ch == '\t') {
            switch(ch) {
                case 'j': //move down
                    iCurrentLine++;
                    if (iCurrentLine >= mPageItems[iCurrentPage].size()) {
                        iCurrentLine = 0;
                    }
                    break;
                case 'k': //move up
                    iCurrentLine--;
                    if (iCurrentLine < 0) {
                        iCurrentLine = mPageItems[iCurrentPage].size()-1;
                    }
                    break;
                case 'l': //set to prev value
                    cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setNext();
                    bStuffChanged = true;
                    if ( iCurrentPage == iWorldOptPage ) {
                        bWorldStuffChanged = true;
                    }
                    break;
                case 'h': //set to next value
                    cOPTIONS[mPageItems[iCurrentPage][iCurrentLine]].setPrev();
                    bStuffChanged = true;
                    if ( iCurrentPage == iWorldOptPage ) {
                        bWorldStuffChanged = true;
                    }
                    break;
                case '>':
                case '\t': //Switch to next Page
                    iCurrentLine = 0;
                    iStartPos = 0;
                    iCurrentPage++;
                    if (iCurrentPage >= vPages.size()) {
                        iCurrentPage = 0;
                    }
                    break;
                case '<':
                    iCurrentLine = 0;
                    iStartPos = 0;
                    iCurrentPage--;
                    if (iCurrentPage < 0) {
                        iCurrentPage = vPages.size()-1;
                    }
                    break;
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    used_tiles_changed = (OPTIONS_OLD["TILES"].getValue() != OPTIONS["TILES"].getValue()) ||
        (OPTIONS_OLD["USE_TILES"] != OPTIONS["USE_TILES"]);

    if (bStuffChanged) {
        if(query_yn(_("Save changes?"))) {
            save_options(ingame && bWorldStuffChanged);
        } else {
            used_tiles_changed = false;
            OPTIONS = OPTIONS_OLD;
            if (ingame && bWorldStuffChanged) {
                ACTIVE_WORLD_OPTIONS = WOPTIONS_OLD;
            }
        }
    }
#ifdef SDLTILES
    if (used_tiles_changed){
        tilecontext->reinit("gfx");
    }
#endif // SDLTILES
    delwin(w_options);
    delwin(w_options_border);
    delwin(w_options_header);
    delwin(w_options_tooltip);
}

void load_options()
{
    std::ifstream fin;
    fin.open("data/options.txt");
    if(!fin.is_open()) {
        fin.close();
        save_options();
        fin.open("data/options.txt");
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create ./data/options.txt\n";
            return;
        }
    }

    std::string sLine;
    while(!fin.eof()) {
        getline(fin, sLine);

        if(sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1) {
            int iPos = sLine.find(' ');
            const std::string loadedvar = sLine.substr(0, iPos);
            const std::string loadedval = sLine.substr(iPos+1, sLine.length());
            // option with values from post init() might get clobbered
            optionsdata.add_retry(loadedvar, loadedval); // stash it until update();
            
            OPTIONS[ loadedvar ].setValue( loadedval );
        }
    }

    fin.close();

    trigdist = OPTIONS["CIRCLEDIST"]; // cache to global due to heavy usage.
    use_tiles = OPTIONS["USE_TILES"]; // cache to global due to heavy usage.
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

void save_options(bool ingame)
{
    std::ofstream fout;
    fout.open("data/options.txt");
    if(!fout.is_open()) {
        return;
    }

    fout << options_header() << std::endl;

    for(int j = 0; j < vPages.size(); j++) {
        bool update_wopt = (ingame && j == iWorldOptPage );
        for(int i = 0; i < mPageItems[j].size(); i++) {
            fout << "#" << OPTIONS[mPageItems[j][i]].getTooltip() << std::endl;
            fout << "#" << OPTIONS[mPageItems[j][i]].getDefaultText() << std::endl;
            fout << mPageItems[j][i] << " " << OPTIONS[mPageItems[j][i]].getValue() << std::endl << std::endl;
            if ( update_wopt ) {
                world_generator->active_world->world_options[ mPageItems[j][i] ] = ACTIVE_WORLD_OPTIONS[ mPageItems[j][i] ];
            }
        }
    }

    fout.close();
    if ( ingame ) {
        world_generator->save_world( world_generator->active_world, false );
    }
    trigdist = OPTIONS["CIRCLEDIST"]; // update trigdist as well
    use_tiles = OPTIONS["USE_TILES"]; // and use_tiles
}

bool use_narrow_sidebar()
{
    return (TERMY < 25 || OPTIONS["SIDEBAR_STYLE"] == "narrow");
}

std::string get_tileset_names(std::string dir_path)
{
    const std::string defaultTilesets = "hoder,deon";

    const std::string filename = "tileset.txt";                             // tileset-info-file
    std::vector<std::string> files;
    files = file_finder::get_files_from_path(filename, dir_path, true);     // search it

    std::string tileset_names;
    bool first_tileset_name = true;

    for(std::vector<std::string>::iterator it = files.begin(); it !=files.end(); ++it) {
        std::ifstream fin;
        fin.open(it->c_str());
        if(!fin.is_open()) {
            fin.close();
            DebugLog() << "\tCould not read ."<<*it;
            optionNames["deon"] = _("Deon's");          // just setting some standards
            optionNames["hoder"] = _("Hoder's");
            return defaultTilesets;
        }
        // should only have 2 values inside it, otherwise is going to only load the last 2 values
        std::string tileset_name;

        while(!fin.eof()) {
            std::string sOption;
            fin >> sOption;

            //DebugLog() << "\tCurrent: " << sOption << " -- ";

            if(sOption == "") {
                getline(fin, sOption);    // Empty line, chomp it
                //DebugLog() << "Empty line, skipping\n";
            } else if(sOption[0] == '#') { // # indicates a comment
                getline(fin, sOption);
                //DebugLog() << "Comment line, skipping\n";
            } else {
                if (sOption.find("NAME") != std::string::npos) {
                    tileset_name = "";
                    fin >> tileset_name;
                    if(first_tileset_name)
                    {
                        first_tileset_name = false;
                        tileset_names += tileset_name;
                    } else {
                        tileset_names += std::string(",");
                        tileset_names += tileset_name;
                    }
                } else if (sOption.find("VIEW") != std::string::npos) {
                    std::string viewName = "";
                    fin >> viewName;
                    optionNames[tileset_name] = viewName;
                    break;
                }
            }
        }
        fin.close();
    }

    if(tileset_names == "") {
        optionNames["deon"] = _("Deon's");          // more standards
        optionNames["hoder"] = _("Hoder's");
        return defaultTilesets;

    }

    return tileset_names;
}

void options_data::enable_json(const std::string & lvar) {
    post_json_verify[ lvar ] = std::string( 1, 001 ); // because "" might be valid
}

void options_data::add_retry(const std::string & lvar, const::std::string & lval) {
    static const std::string blank_value( 1, 001 ); 
    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() && it->second == blank_value ) {
        // initialized with impossible value: valid
        post_json_verify[ lvar ] = lval;
    }
}

void options_data::add_value( const std::string & lvar, const std::string & lval, std::string lvalname ) {
    static const std::string blank_value( 1, 001 );

    std::map<std::string, std::string>::const_iterator it = post_json_verify.find(lvar);
    if ( it != post_json_verify.end() ) {
        std::map<std::string, cOpt>::iterator ot = OPTIONS.find(lvar);
        if ( ot != OPTIONS.end() && ot->second.sType == "string" ) {
            for(std::vector<std::string>::const_iterator eit = ot->second.vItems.begin();
                eit != ot->second.vItems.end(); ++eit) {
                if ( *eit == lval ) { // already in
                    return;
                }
            }
            ot->second.vItems.push_back(lval);
            if ( optionNames.find(lval) == optionNames.end() ) {
                optionNames[ lval ] = ( lvalname == "" ? lval : lvalname );
            }
            // our value was saved, then set to default, so set it again.
            if ( it->second == lval ) {
                OPTIONS[ lvar ].setValue( lval );
            }
        }

    }
}
