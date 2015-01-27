#include "game.h"
#include "rng.h"
#include "input.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "options.h"
#include "auto_pickup.h"
#include "gamemode.h"
#include "mapbuffer.h"
#include "debug.h"
#include "editmap.h"
#include "bodypart.h"
#include "map.h"
#include "uistate.h"
#include "item_group.h"
#include "json.h"
#include "artifact.h"
#include "overmapbuffer.h"
#include "trap.h"
#include "mapdata.h"
#include "catacharset.h"
#include "translations.h"
#include "init.h"
#include "help.h"
#include "action.h"
#include "monstergenerator.h"
#include "monattack.h"
#include "worldfactory.h"
#include "filesystem.h"
#include "mod_manager.h"
#include "path_info.h"
#include "mapbuffer.h"
#include "mapsharing.h"
#include "messages.h"
#include "pickup.h"
#include "weather_gen.h"
#include "start_location.h"
#include "debug.h"
#include "catalua.h"
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <vector>
#include <locale>
#include <cassert>

//TODO replace these includes with filesystem.h
#include <sys/stat.h>

#ifdef _MSC_VER
#   include "wdirent.h"
#   include <direct.h>
#else
#   include <unistd.h>
#   include <dirent.h>
#endif

#if (defined _WIN32 || defined __WIN32__)
#   include "platform_win.h"
#   include <tchar.h>
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
void intro();
nc_color sev(int a); // Right now, ONLY used for scent debugging....

//The one and only game instance
game *g;
extern worldfactory *world_generator;
input_context get_default_mode_input_context();

uistatedata uistate;

bool is_valid_in_w_terrain(int x, int y)
{
    return x >= 0 && x < TERRAIN_WINDOW_WIDTH && y >= 0 && y < TERRAIN_WINDOW_HEIGHT;
}

// This is the main game set-up process.
game::game() :
    new_game(false),
    uquit(QUIT_NO),
    w_terrain(NULL),
    w_overmap(NULL),
    w_omlegend(NULL),
    w_minimap(NULL),
    w_HP(NULL),
    w_messages(NULL),
    w_location(NULL),
    w_status(NULL),
    w_status2(NULL),
    w_blackspace(NULL),
    dangerous_proximity(5),
    safe_mode(SAFE_MODE_ON),
    mostseen(0),
    gamemode(NULL),
    lookHeight(13),
    tileset_zoom(16)
{
    world_generator = new worldfactory();
    // do nothing, everything that was in here is moved to init_data() which is called immediately after g = new game; in main.cpp
    // The reason for this move is so that g is not uninitialized when it gets to installing the parts into vehicles.
}

// Load everything that will not depend on any mods
void game::load_static_data()
{
#ifdef LUA
    init_lua();                 // Set up lua                       (SEE catalua.cpp)
#endif
    // UI stuff, not mod-specific per definition
    inp_mngr.init();            // Load input config JSON
    // Init mappings for loading the json stuff
    DynamicDataLoader::get_instance();
    // Only need to load names once, they do not depend on mods
    init_names();
    narrow_sidebar = OPTIONS["SIDEBAR_STYLE"] == "narrow";
    fullscreen = false;
    was_fullscreen = false;

    // These functions do not load stuff from json.
    // The content they load/initialize is hardcoded into the program.
    // Therefore they can be loaded here.
    // If this changes (if they load data from json), they have to
    // be moved to game::load_mod or game::load_core_data
    init_body_parts();
    init_ter_bitflags_map();
    init_vpart_bitflag_map();
    init_colormap();
    init_mapgen_builtin_functions();
    init_fields();
    init_morale();
    init_diseases();             // Set up disease lookup table
    init_savedata_translation_tables();
    init_npctalk();
    init_artifacts();
    init_weather();
    init_weather_anim();
    init_faction_data();

    // --- move/delete everything below
    // TODO: move this to player class
    moveCount = 0;
}

void game::check_all_mod_data()
{
    mod_manager *mm = world_generator->get_mod_manager();
    dependency_tree &dtree = mm->get_tree();
    if (mm->mod_map.empty()) {
        // If we don't have any mods, test core data only
        load_core_data();
        DynamicDataLoader::get_instance().finalize_loaded_data();
    }
    for (mod_manager::t_mod_map::iterator a = mm->mod_map.begin(); a != mm->mod_map.end(); ++a) {
        MOD_INFORMATION *mod = a->second;
        if (!dtree.is_available(mod->ident)) {
            debugmsg("Skipping mod %s (%s)", mod->name.c_str(), dtree.get_node(mod->ident)->s_errors().c_str());
            continue;
        }
        std::vector<std::string> deps = dtree.get_dependents_of_X_as_strings(mod->ident);
        if (!deps.empty()) {
            // mod is dependency of another mod(s)
            // When those mods get checked, they will pull in
            // this mod, so there is no need to check this mod now.
            continue;
        }
        popup_nowait("checking mod %s", mod->name.c_str());
        // Reset & load core data, than load dependencies
        // and the actual mod and finally finalize all.
        load_core_data();
        deps = dtree.get_dependencies_of_X_as_strings(mod->ident);
        for( auto &dep : deps ) {
            // assert(mm->has_mod(deps[i]));
            // ^^ dependency tree takes care of that case
            MOD_INFORMATION *dmod = mm->mod_map[dep];
            load_data_from_dir(dmod->path);
        }
        load_data_from_dir(mod->path);
        DynamicDataLoader::get_instance().finalize_loaded_data();
    }
}

void game::load_core_data()
{
    // core data can be loaded only once and must be first
    // anyway.
    DynamicDataLoader::get_instance().unload_data();

    load_data_from_dir(FILENAMES["jsondir"]);
}

void game::load_data_from_dir(const std::string &path)
{
#ifdef LUA
    // Process a preload file before the .json files,
    // so that custom IUSE's can be defined before
    // the items that need them are parsed

    lua_loadmod(lua_state, path, "preload.lua");
#endif

    try {
        DynamicDataLoader::get_instance().load_data_from_path(path);
    } catch (std::string &err) {
        debugmsg("Error loading data from json: %s", err.c_str());
    }

#ifdef LUA
    // main.lua will be executed after JSON, allowing to
    // work with items defined by mod's JSON

    lua_loadmod(lua_state, path, "main.lua");
#endif
}

game::~game()
{
    DynamicDataLoader::get_instance().unload_data();
    MAPBUFFER.reset();
    delete gamemode;
    delwin(w_terrain);
    delwin(w_minimap);
    delwin(w_HP);
    delwin(w_messages);
    delwin(w_location);
    delwin(w_status);
    delwin(w_status2);

    delete world_generator;
}

// Fixed window sizes
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7

#if (defined TILES)
// defined in sdltiles.cpp
void to_map_font_dimension(int &w, int &h);
void from_map_font_dimension(int &w, int &h);
void to_overmap_font_dimension(int &w, int &h);
void reinitialize_framebuffer();
#else
// unchanged, nothing to be translated without tiles
void to_map_font_dimension(int &, int &) { }
void from_map_font_dimension(int &, int &) { }
void to_overmap_font_dimension(int &, int &) { }
//in pure curses, the framebuffer won't need reinitializing
void reinitialize_framebuffer() { }
#endif


void game::init_ui()
{
    // clear the screen
    static bool first_init = true;

    if (first_init) {
        clear();

        // set minimum FULL_SCREEN sizes
        FULL_SCREEN_WIDTH = 80;
        FULL_SCREEN_HEIGHT = 24;
        // print an intro screen, making sure the terminal is the correct size
        intro();

        first_init = false;
    }

    int sidebarWidth = narrow_sidebar ? 45 : 55;

    // First get TERMX, TERMY
#if (defined TILES || defined _WIN32 || defined __WIN32__)
    TERMX = get_terminal_width();
    TERMY = get_terminal_height();
#else
    getmaxyx(stdscr, TERMY, TERMX);

    // try to make FULL_SCREEN_HEIGHT symmetric according to TERMY
    if (TERMY % 2) {
        FULL_SCREEN_HEIGHT = 25;
    } else {
        FULL_SCREEN_HEIGHT = 24;
    }

    // now that TERMX and TERMY are set,
    // check if sidebar style needs to be overridden
    sidebarWidth = use_narrow_sidebar() ? 45 : 55;
    if(fullscreen) {
        sidebarWidth = 0;
    }
#endif
    // remove some space for the sidebar, this is the maximal space
    // (using standard font) that the terrain window can have
    TERRAIN_WINDOW_HEIGHT = TERMY;
    TERRAIN_WINDOW_WIDTH = TERMX - sidebarWidth;
    TERRAIN_WINDOW_TERM_WIDTH = TERRAIN_WINDOW_WIDTH;
    TERRAIN_WINDOW_TERM_HEIGHT = TERRAIN_WINDOW_HEIGHT;

    /**
     * In tiles mode w_terrain can have a different font (with a different
     * tile dimension) or can be drawn by cata_tiles which uses tiles that again
     * might have a different dimension then the normal font used everywhere else.
     *
     * TERRAIN_WINDOW_WIDTH/TERRAIN_WINDOW_HEIGHT defines how many squares can
     * be displayed in w_terrain (using it's specific tile dimension), not
     * including partially drawn squares at the right/bottom. You should
     * use it whenever you want to draw specific squares in that window or to
     * determine whether a specific square is draw on screen (or outside the screen
     * and needs scrolling).
     *
     * TERRAIN_WINDOW_TERM_WIDTH/TERRAIN_WINDOW_TERM_HEIGHT defines the size of
     * w_terrain in the standard font dimension (the font that everything else uses).
     * You usually don't have to use it, expect for positioning of windows,
     * because the window positions use the standard font dimension.
     *
     * VIEW_OFFSET_X/VIEW_OFFSET_Y is the position of w_terrain on screen,
     * it is (as every window position) in standard font dimension.
     * As the sidebar is located right of w_terrain it also controls its position.
     * It is used to move everything into the center of the screen,
     * when the screen is larger than what the game requires.
     *
     * The code here calculates size available for w_terrain, caps it at
     * max_view_size (the maximal view range than any character can have at
     * any time).
     * It is stored in TERRAIN_WINDOW_*.
     * If w_terrain does not occupy the whole available area, VIEW_OFFSET_*
     * are set to move everything into the middle of the screen.
     */
    to_map_font_dimension(TERRAIN_WINDOW_WIDTH, TERRAIN_WINDOW_HEIGHT);
    static const int max_view_size = DAYLIGHT_LEVEL * 2 + 1;
    if (TERRAIN_WINDOW_WIDTH > max_view_size) {
        VIEW_OFFSET_X = (TERRAIN_WINDOW_WIDTH - max_view_size) / 2;
        TERRAIN_WINDOW_TERM_WIDTH = max_view_size * TERRAIN_WINDOW_TERM_WIDTH / TERRAIN_WINDOW_WIDTH;
        TERRAIN_WINDOW_WIDTH = max_view_size;
    } else {
        VIEW_OFFSET_X = 0;
    }
    if (TERRAIN_WINDOW_HEIGHT > max_view_size) {
        VIEW_OFFSET_Y = (TERRAIN_WINDOW_HEIGHT - max_view_size) / 2;
        TERRAIN_WINDOW_TERM_HEIGHT = max_view_size * TERRAIN_WINDOW_TERM_HEIGHT / TERRAIN_WINDOW_HEIGHT;
        TERRAIN_WINDOW_HEIGHT = max_view_size;
    } else {
        VIEW_OFFSET_Y = 0;
    }
    // VIEW_OFFSET_* are in standard font dimension.
    from_map_font_dimension(VIEW_OFFSET_X, VIEW_OFFSET_Y);

    // Position of the player in the terrain window, it is always in the center
    POSX = TERRAIN_WINDOW_WIDTH / 2;
    POSY = TERRAIN_WINDOW_HEIGHT / 2;

    // Set up the main UI windows.
    w_terrain = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    werase(w_terrain);

    /**
     * Doing the same thing as above for the overmap
     */
    static const int OVERMAP_LEGEND_WIDTH = 28;
    OVERMAP_WINDOW_HEIGHT = TERMY;
    OVERMAP_WINDOW_WIDTH = TERMX - OVERMAP_LEGEND_WIDTH;
    to_overmap_font_dimension(OVERMAP_WINDOW_WIDTH, OVERMAP_WINDOW_HEIGHT);

    //Bring the framebuffer to the maximum required dimensions
    //Otherwise it segfaults when the overmap needs a bigger buffer size than it provides
    reinitialize_framebuffer();

    int minimapX, minimapY; // always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
    int hpX, hpY, hpW, hpH;
    int messX, messY, messW, messH;
    int locX, locY, locW, locH;
    int statX, statY, statW, statH;
    int stat2X, stat2Y, stat2W, stat2H;
    int mouseview_y, mouseview_h;

    if (use_narrow_sidebar()) {
        // First, figure out how large each element will be.
        hpH = 7;
        hpW = 14;
        statH = 7;
        statW = sidebarWidth - MINIMAP_WIDTH - hpW;
        locH = 1;
        locW = sidebarWidth;
        stat2H = 2;
        stat2W = sidebarWidth;
        messH = TERMY - (statH + locH + stat2H);
        messW = sidebarWidth;

        // Now position the elements relative to each other.
        minimapX = 0;
        minimapY = 0;
        hpX = minimapX + MINIMAP_WIDTH;
        hpY = 0;
        locX = 0;
        locY = minimapY + MINIMAP_HEIGHT;
        statX = hpX + hpW;
        statY = 0;
        stat2X = 0;
        stat2Y = locY + locH;
        messX = 0;
        messY = stat2Y + stat2H;

        mouseview_y = messY + 7;
        mouseview_h = TERMY - mouseview_y - 5;
    } else {
        // standard sidebar style
        minimapX = 0;
        minimapY = 0;
        messX = MINIMAP_WIDTH;
        messY = 0;
        messW = sidebarWidth - messX;
        messH = TERMY - 5; // 1 for w_location + 4 for w_stat, w_messages starts at 0
        hpX = 0;
        hpY = MINIMAP_HEIGHT;
        // under the minimap, but down to the same line as w_location (which is under w_messages)
        // so it erases the space between w_terrain and (w_messages and w_location)
        hpH = messH - MINIMAP_HEIGHT + 1;
        hpW = 7;
        locX = MINIMAP_WIDTH;
        locY = messY + messH;
        locH = 1;
        locW = sidebarWidth - locX;
        statX = 0;
        statY = locY + locH;
        statH = 4;
        statW = sidebarWidth;

        // The default style only uses one status window.
        stat2X = 0;
        stat2Y = statY + statH;
        stat2H = 1;
        stat2W = sidebarWidth;

        mouseview_y = stat2Y + stat2H;
        mouseview_h = TERMY - mouseview_y;
    }

    int _y = VIEW_OFFSET_Y;
    int _x = TERMX - VIEW_OFFSET_X - sidebarWidth;

    w_minimap = newwin(MINIMAP_HEIGHT, MINIMAP_WIDTH, _y + minimapY, _x + minimapX);
    werase(w_minimap);

    w_HP = newwin(hpH, hpW, _y + hpY, _x + hpX);
    werase(w_HP);

    w_messages = newwin(messH, messW, _y + messY, _x + messX);
    werase(w_messages);

    w_location = newwin(locH, locW, _y + locY, _x + locX);
    werase(w_location);

    w_status = newwin(statH, statW, _y + statY, _x + statX);
    werase(w_status);

    int mouse_view_x = _x + minimapX;
    int mouse_view_width = sidebarWidth;
    if (mouseview_h < lookHeight) {
        // Not enough room below the status bar, just use the regular lookaround area
        get_lookaround_dimensions(mouse_view_width, mouseview_y, mouse_view_x);
        mouseview_h = lookHeight;
        liveview.compact_view = true;
        if (!use_narrow_sidebar()) {
            // Second status window must now take care of clearing the area to the
            // bottom of the screen.
            stat2H = std::max( 1, TERMY - stat2Y );
        }
    }
    liveview.init(mouse_view_x, mouseview_y, sidebarWidth, mouseview_h);

    w_status2 = newwin(stat2H, stat2W, _y + stat2Y, _x + stat2X);
    werase(w_status2);
}

void game::toggle_sidebar_style(void)
{
    narrow_sidebar = !narrow_sidebar;
    init_ui();
    refresh_all();
}

void game::toggle_fullscreen(void)
{
#ifndef TILES
    if (TERMX > 121 || TERMY > 121) {
        return;
    }
    fullscreen = !fullscreen;
    init_ui();
    refresh_all();
#endif
}

// temporarily switch out of fullscreen for functions that rely
// on displaying some part of the sidebar
void game::temp_exit_fullscreen(void)
{
    if (fullscreen) {
        was_fullscreen = true;
        toggle_fullscreen();
    } else {
        was_fullscreen = false;
    }
}

void game::reenter_fullscreen(void)
{
    if (was_fullscreen) {
        if (!fullscreen) {
            toggle_fullscreen();
        }
    }
}

/*
 * Initialize more stuff after mapbuffer is loaded.
 */
void game::setup()
{
    m = map(); // reset the main map

    load_world_modfiles(world_generator->active_world);

    next_npc_id = 1;
    next_faction_id = 1;
    next_mission_id = 1;
    // Clear monstair values
    monstairx = -1;
    monstairy = -1;
    monstairz = -1;
    last_target = -1;  // We haven't targeted any monsters yet
    last_target_was_npc = false;
    new_game = true;
    uquit = QUIT_NO;   // We haven't quit the game

    weather = WEATHER_CLEAR; // Start with some nice weather...
    // Weather shift in 30
    nextweather = HOURS((int)OPTIONS["INITIAL_TIME"]) + MINUTES(30);

    turnssincelastmon = 0; //Auto safe mode init
    autosafemode = OPTIONS["AUTOSAFEMODE"];
    safemodeveh =
        OPTIONS["SAFEMODEVEH"]; //Vehicle safemode check, in practice didn't trigger when needed

    footsteps.clear();
    footsteps_source.clear();
    clear_zombies();
    coming_to_stairs.clear();
    active_npc.clear();
    factions.clear();
    active_missions.clear();
    items_dragged.clear();
    Messages::clear_messages();
    events.clear();

    SCT.vSCT.clear(); //Delete pending messages

    // reset kill counts
    kills.clear();
    // Set the scent map to 0
    for( auto &elem : grscent ) {
        for( auto &elem_j : elem ) {
            elem_j = 0;
        }
    }

    load_auto_pickup(false); // Load global auto pickup rules

    remoteveh_cache_turn = INT_MIN;
    remoteveh_cache = nullptr;
    // back to menu for save loading, new game etc
}

bool game::has_gametype() const
{
    return gamemode && gamemode->id() != SGAME_NULL;
}

special_game_id game::gametype() const
{
    return gamemode != nullptr ? gamemode->id() : SGAME_NULL;
}

// Set up all default values for a new game
void game::start_game(std::string worldname)
{
    if (gamemode == NULL) {
        gamemode = new special_game();
    }

    new_game = true;
    start_calendar();
    nextweather = calendar::turn;
    weatherSeed = rand();
    safe_mode = (OPTIONS["SAFEMODE"] ? SAFE_MODE_ON : SAFE_MODE_OFF);
    mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.

    init_autosave();

    clear();
    refresh();
    popup_nowait(_("Please wait as we build your world"));
    // Init some factions.
    if (!load_master(worldname)) { // Master data record contains factions.
        create_factions();
    }
    u.setID( assign_npc_id() ); // should be as soon as possible, but *after* load_master

    const start_location &start_loc = *start_location::find( u.start_location );
    start_loc.setup( cur_om, levx, levy, levz );

    // Start the overmap with out immediate neighborhood visible
    overmap_buffer.reveal(point(om_global_location().x, om_global_location().y), OPTIONS["DISTANCE_INITIAL_VISIBILITY"], 0);
    // Init the starting map at this location.
    m.load( levx, levy, levz, true, cur_om );
    m.build_map_cache();
    // Do this after the map cache has been build!
    start_loc.place_player( u );

    u.moves = 0;
    u.process_turn(); // process_turn adds the initial move points
    nextspawn = int(calendar::turn);
    temperature = 65; // Springtime-appropriate?
    update_weather(); // Springtime-appropriate, definitely.
    u.next_climate_control_check = 0;  // Force recheck at startup
    u.last_climate_control_ret = false;

    //Reset character pickup rules
    vAutoPickupRules[2].clear();
    //Put some NPCs in there!
    create_starting_npcs();
    //Load NPCs. Set nearby npcs to active.
    load_npcs();
    //spawn the monsters
    m.spawn_monsters( true ); // Static monsters

    //Create mutation_category_level
    u.set_highest_cat_level();
    //Calc mutation drench protection stats
    u.drench_mut_calc();
    if (scen->has_flag("FIRE_START")){
            m.add_field(u.pos().x + 5, u.pos().y + 3, field_from_ident("fd_fire"), 3 );
            m.add_field(u.pos().x + 7, u.pos().y + 6, field_from_ident("fd_fire"), 3 );
            m.add_field(u.pos().x + 3, u.pos().y + 4, field_from_ident("fd_fire"), 3 );
    }
    if (scen->has_flag("INFECTED")){
        u.add_effect("infected", 1, random_body_part(), true);
    }
    if (scen->has_flag("BAD_DAY")){
        u.add_effect("flu", 10000);
        u.add_effect("drunk", 2700 - (12 * u.str_max));
        u.add_morale(MORALE_FEELING_BAD,-100,50,50,50);
    }
    //~ %s is player name
    u.add_memorial_log(pgettext("memorial_male", "%s began their journey into the Cataclysm."),
                       pgettext("memorial_female", "%s began their journey into the Cataclysm."),
                       u.name.c_str());
}

void game::create_factions()
{
    faction tmp;
    std::vector<std::string> faction_vector = tmp.all_json_factions();
    for(std::vector<std::string>::reverse_iterator it = faction_vector.rbegin(); it != faction_vector.rend(); ++it) {
        tmp = faction(it[0].c_str());
        tmp.randomize();
        tmp.load_faction_template(it[0].c_str());
        factions.push_back(tmp);
    }
}

//Make any nearby overmap npcs active, and put them in the right location.
void game::load_npcs()
{
    const int radius = int(MAPSIZE / 2) - 1;
    // uses submap coordinates
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(radius);
    for( auto temp : npcs ) {

        if (temp->is_active()) {
            continue;
        }
            const tripoint p = temp->global_sm_location();
            add_msg( m_debug, "game::load_npcs: Spawning static NPC, %d:%d (%d:%d)",
                     levx, levy, p.x, p.y);
        temp->place_on_map();
        // In the rare case the npc was marked for death while
        // it was on the overmap. Kill it.
        if (temp->marked_for_death) {
            temp->die( nullptr );
        } else {
            active_npc.push_back(temp);
        }
    }
}

void game::create_starting_npcs()
{
    if (!ACTIVE_WORLD_OPTIONS["STATIC_NPC"]) {
        return; //Do not generate a starting npc.
    }

    //We don't want more than one starting npc per shelter
    const int radius = 1;
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(radius);
    if (npcs.size() >= 1) {
        return; //There is already an NPC in this shelter
    }

    npc *tmp = new npc();
    tmp->normalize();
    tmp->randomize((one_in(2) ? NC_DOCTOR : NC_NONE));
    // spawn the npc in the overmap, sets its overmap and submap coordinates
    tmp->spawn_at( get_abs_levx(), get_abs_levy(), levz );
    tmp->setx( SEEX * int(MAPSIZE / 2) + SEEX );
    tmp->sety( SEEY * int(MAPSIZE / 2) + 6 );
    tmp->form_opinion(&u);
    tmp->attitude = NPCATT_NULL;
    //This sets the npc mission. This NPC remains in the shelter.
    tmp->mission = NPC_MISSION_SHELTER;
    tmp->chatbin.first_topic = TALK_SHELTER;
    //one random shelter mission.
    tmp->chatbin.missions.push_back(
        reserve_random_mission(ORIGIN_OPENER_NPC, om_location(), tmp->getID()));
}

bool game::cleanup_at_end()
{
    draw_sidebar();
    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE) {
        // Put (non-hallucinations) into the overmap so they are not lost.
        while( num_zombies() > 0 ) {
            despawn_monster( 0 );
        }
        // Save the factions', missions and set the NPC's overmap coords
        // Npcs are saved in the overmap.
        save_factions_missions_npcs(); //missions need to be saved as they are global for all saves.

        // save artifacts.
        save_artifacts();

        // and the overmap, and the local map.
        save_maps(); //Omap also contains the npcs who need to be saved.
    }

    // Clear the future weather for future projects
    weather_log.clear();

    if (uquit == QUIT_DIED || uquit == QUIT_SUICIDE) {
        std::vector<std::string> vRip;

        int iMaxWidth = 0;
        int iNameLine = 0;
        int iInfoLine = 0;

        if (u.has_amount("holybook_bible1", 1) || u.has_amount("holybook_bible2", 1) ||
            u.has_amount("holybook_bible3", 1)) {
            if (!(u.has_trait("CANNIBAL") || u.has_trait("PSYCHOPATH"))) {
                vRip.push_back("               _______  ___");
                vRip.push_back("              <       `/   |");
                vRip.push_back("               >  _     _ (");
                vRip.push_back("              |  |_) | |_) |");
                vRip.push_back("              |  | \\ | |   |");
                vRip.push_back("   ______.__%_|            |_________  __");
                vRip.push_back(" _/                                  \\|  |");
                iNameLine = vRip.size();
                vRip.push_back("|                                        <");
                vRip.push_back("|                                        |");
                iMaxWidth = vRip[vRip.size() - 1].length();
                vRip.push_back("|                                        |");
                vRip.push_back("|_____.-._____              __/|_________|");
                vRip.push_back("              |            |");
                iInfoLine = vRip.size();
                vRip.push_back("              |            |");
                vRip.push_back("              |           <");
                vRip.push_back("              |            |");
                vRip.push_back("              |   _        |");
                vRip.push_back("              |__/         |");
                vRip.push_back("             % / `--.      |%");
                vRip.push_back("         * .%%|          -< @%%%");
                vRip.push_back("         `\\%`@|            |@@%@%%");
                vRip.push_back("       .%%%@@@|%     `   % @@@%%@%%%%");
                vRip.push_back("  _.%%%%%%@@@@@@%%%__/\\%@@%%@@@@@@@%%%%%%");

            } else {
                vRip.push_back("               _______  ___");
                vRip.push_back("              |       \\/   |");
                vRip.push_back("              |            |");
                vRip.push_back("              |            |");
                iInfoLine = vRip.size();
                vRip.push_back("              |            |");
                vRip.push_back("              |            |");
                vRip.push_back("              |            |");
                vRip.push_back("              |            |");
                vRip.push_back("              |           <");
                vRip.push_back("              |   _        |");
                vRip.push_back("              |__/         |");
                vRip.push_back("   ______.__%_|            |__________  _");
                vRip.push_back(" _/                                   \\| \\");
                iNameLine = vRip.size();
                vRip.push_back("|                                         <");
                vRip.push_back("|                                         |");
                iMaxWidth = vRip[vRip.size() - 1].length();
                vRip.push_back("|                                         |");
                vRip.push_back("|_____.-._______            __/|__________|");
                vRip.push_back("             % / `_-.   _  |%");
                vRip.push_back("         * .%%|  |_) | |_)< @%%%");
                vRip.push_back("         `\\%`@|  | \\ | |   |@@%@%%");
                vRip.push_back("       .%%%@@@|%     `   % @@@%%@%%%%");
                vRip.push_back("  _.%%%%%%@@@@@@%%%__/\\%@@%%@@@@@@@%%%%%%");
            }
        } else {
            vRip.push_back("           _________  ____           ");
            vRip.push_back("         _/         `/    \\_         ");
            vRip.push_back("       _/      _     _      \\_.      ");
            vRip.push_back("     _%\\      |_) | |_)       \\_     ");
            vRip.push_back("   _/ \\/      | \\ | |           \\_   ");
            vRip.push_back(" _/                               \\_ ");
            vRip.push_back("|                                   |");
            iNameLine = vRip.size();
            vRip.push_back(" )                                 < ");
            vRip.push_back("|                                   |");
            vRip.push_back("|                                   |");
            vRip.push_back("|   _                               |");
            vRip.push_back("|__/                                |");
            iMaxWidth = vRip[vRip.size() - 1].length();
            vRip.push_back(" / `--.                             |");
            vRip.push_back("|                                  ( ");
            iInfoLine = vRip.size();
            vRip.push_back("|                                   |");
            vRip.push_back(" \\_                               _/");
            vRip.push_back("   \\_%                         ._/  ");
            vRip.push_back("   @`\\_                       _/%%  ");
            vRip.push_back("  %@%@%\\_              *    _/%`%@% ");
            vRip.push_back(" %@@@.%@%\\%%           `\\ %%.%%@@%@");
            vRip.push_back("@%@@%%%%%@@@@@@%%%%%%%%@@%%@@@%%%@%%@");
        }

        const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
        const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

        WINDOW *w_rip = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
        draw_border(w_rip);

        for (unsigned int iY = 0; iY < vRip.size(); ++iY) {
            for (unsigned int iX = 0; iX < vRip[iY].length(); ++iX) {
                char cTemp = vRip[iY][iX];
                if (cTemp != ' ') {
                    nc_color ncColor = c_ltgray;

                    if (cTemp == '%') {
                        ncColor = c_green;

                    } else if (cTemp == '_' || cTemp == '|') {
                        ncColor = c_white;

                    } else if (cTemp == '@') {
                        ncColor = c_brown;

                    } else if (cTemp == '*') {
                        ncColor = c_red;
                    }

                    mvwputch(w_rip, iY + 1, iX + (FULL_SCREEN_WIDTH / 2) - (iMaxWidth / 2), ncColor, vRip[iY][iX]);
                }
            }
        }

        std::string sTemp;
        std::stringstream ssTemp;

        int days_survived = int(calendar::turn.get_turn() / DAYS(1));
        int days_adventured = int((calendar::turn.get_turn() - calendar::start.get_turn()) / DAYS(1));

        for (int lifespan = 0; lifespan < 2; ++lifespan) {
            // Show the second, "Adventured", lifespan
            // only if it's different from the first.
            if (lifespan && days_adventured == days_survived) {
                continue;
            }

            sTemp = lifespan ? _("Adventured:") : _("Survived:");
            mvwprintz(w_rip, iInfoLine++, (FULL_SCREEN_WIDTH / 2) - 5, c_ltgray, (sTemp + " ").c_str());

            int iDays = lifespan ? days_adventured : days_survived;
            ssTemp << iDays;
            wprintz(w_rip, c_magenta, ssTemp.str().c_str());
            ssTemp.str("");

            sTemp = (iDays == 1) ? _("day") : _("days");
            wprintz(w_rip, c_white, (" " + sTemp).c_str());
        }

        int iTotalKills = 0;

        const std::map<std::string, mtype *> monids = MonsterGenerator::generator().get_all_mtypes();
        for( const auto &monid : monids ) {
            if( kill_count( monid.first ) > 0 ) {
                iTotalKills += kill_count( monid.first );
            }
        }

        ssTemp << iTotalKills;

        sTemp = _("Kills:");
        mvwprintz(w_rip, 1 + iInfoLine++, (FULL_SCREEN_WIDTH / 2) - 5, c_ltgray, (sTemp + " ").c_str());
        wprintz(w_rip, c_magenta, ssTemp.str().c_str());

        sTemp = _("In memory of:");
        mvwprintz(w_rip, iNameLine++, (FULL_SCREEN_WIDTH / 2) - (sTemp.length() / 2), c_ltgray,
                  sTemp.c_str());

        sTemp = u.name;
        mvwprintz(w_rip, iNameLine++, (FULL_SCREEN_WIDTH / 2) - (sTemp.length() / 2), c_white,
                  sTemp.c_str());

        sTemp = _("Last Words:");
        mvwprintz(w_rip, iNameLine++, (FULL_SCREEN_WIDTH / 2) - (sTemp.length() / 2), c_ltgray,
                  sTemp.c_str());

        long cInput = '\n';
        int iPos = -1;
        int iStartX = (FULL_SCREEN_WIDTH / 2) - ((iMaxWidth - 4) / 2);
        std::string sLastWords = string_input_win(w_rip, "", iMaxWidth - 4 - 1,
                                 iStartX, iNameLine, iStartX + iMaxWidth - 4 - 1,
                                 true, cInput, iPos);

        death_screen();
        if (uquit == QUIT_SUICIDE) {
            u.add_memorial_log(pgettext("memorial_male", "%s committed suicide."),
                               pgettext("memorial_female", "%s committed suicide."),
                               u.name.c_str());
        } else {
            u.add_memorial_log(pgettext("memorial_male", "%s was killed."),
                               pgettext("memorial_female", "%s was killed."),
                               u.name.c_str());
        }
        if (!sLastWords.empty()) {
            u.add_memorial_log( _("Last words: %s"), sLastWords.c_str(), _("Last words: %s"), sLastWords.c_str() );
        }
        // Struck the save_player_data here to forestall Weirdness
        move_save_to_graveyard();
        write_memorial_file(sLastWords);
        u.memorial_log.clear();
        std::vector<std::string> characters = list_active_characters();
        // remove current player from the active characters list, as they are dead
        std::vector<std::string>::iterator curchar = std::find(characters.begin(),
                characters.end(), u.name);
        if (curchar != characters.end()) {
            characters.erase(curchar);
        }
        if (characters.empty()) {
            if (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] == "yes" ||
                (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] == "query" &&
                 query_yn(_("Delete saved world?")))) {
                delete_world(world_generator->active_world->world_name, true);
            }
        } else if (ACTIVE_WORLD_OPTIONS["DELETE_WORLD"] != "no") {
            std::stringstream message;
            std::string tmpmessage;
            for( auto &character : characters ) {
                tmpmessage += "\n  ";
                tmpmessage += character;
            }
            message << string_format(_("World retained. Characters remaining:%s"),tmpmessage.c_str());
            popup(message.str(), PF_NONE);
        }
        if (gamemode) {
            delete gamemode;
            gamemode = new special_game; // null gamemode or something..
        }
    }
    MAPBUFFER.reset();
    overmap_buffer.clear();
    return true;
}

static int veh_lumi(vehicle *veh)
{
    float veh_luminance = 0.0;
    float iteration = 1.0;
    std::vector<int> light_indices = veh->all_parts_with_feature(VPFLAG_CONE_LIGHT);
    for( auto &light_indice : light_indices ) {
        veh_luminance += ( veh->part_info( light_indice ).bonus / iteration );
        iteration = iteration * 1.1;
    }
    // Calculation: see lightmap.cpp
    return LIGHT_RANGE((veh_luminance * 3));
}

void game::calc_driving_offset(vehicle *veh)
{
    if (veh == nullptr || !OPTIONS["DRIVING_VIEW_OFFSET"]) {
        set_driving_view_offset(point(0, 0));
        return;
    }
    const int g_light_level = (int)light_level();
    const int light_sight_range = u.sight_range(g_light_level);
    int sight = light_sight_range;
    if (veh->lights_on) {
        sight = std::max(veh_lumi(veh), sight);
    }

    // velocity at or below this results in no offset at all
    static const float min_offset_vel = 10 * 100;
    // velocity at or above this results in maximal offset
    static const float max_offset_vel = 70 * 100;
    // The maximal offset will leave at least this many tiles
    // between the PC and the edge of the main window.
    static const int border_range = 2;
    float velocity = veh->velocity;
    rl_vec2d offset = veh->move_vec();
    if (!veh->skidding && std::abs(veh->cruise_velocity - veh->velocity) < 14 * 100 &&
        veh->player_in_control(&u)) {
        // Use the cruise controlled velocity, but only if
        // it is not too different from the actual velocity.
        // The actual velocity changes too often (see above slowdown).
        // Using it makes would make the offset change far too often.
        offset = veh->face_vec();
        velocity = veh->cruise_velocity;
    }
    float rel_offset;
    if (std::fabs(velocity) < min_offset_vel) {
        rel_offset = 0;
    } else if (std::fabs(velocity) > max_offset_vel) {
        rel_offset = (velocity > 0) ? 1 : -1;
    } else {
        rel_offset = (velocity - min_offset_vel) / (max_offset_vel - min_offset_vel);
    }
    // Squeeze into the corners, by making the offset vector longer,
    // the PC is still in view as long as both offset.x and
    // offset.y are <= 1
    if (std::fabs(offset.x) > std::fabs(offset.y) && std::fabs(offset.x) > 0.2) {
        offset.y /= std::fabs(offset.x);
        offset.x = (offset.x > 0) ? +1 : -1;
    } else if (std::fabs(offset.y) > 0.2) {
        offset.x /= std::fabs(offset.y);
        offset.y = offset.y > 0 ? +1 : -1;
    }
    point max_offset((getmaxx(w_terrain) + 1) / 2 - border_range - 1,
                     (getmaxy(w_terrain) + 1) / 2 - border_range - 1);
    offset.x *= rel_offset;
    offset.y *= rel_offset;
    offset.x *= max_offset.x;
    offset.y *= max_offset.y;
    // [ ----@---- ] sight=6
    // [ --@------ ] offset=2
    // [ -@------# ] offset=3
    // can see sights square in every direction, total visible area is
    // (2*sight+1)x(2*sight+1), but the window is only
    // getmaxx(w_terrain) x getmaxy(w_terrain)
    // The area outside of the window is maxoff (sight-getmax/2).
    // If that value is <= 0, the whole visible area fits the window.
    // don't apply the view offset at all.
    // If the offset is > maxoff, only apply at most maxoff, everything
    // above leads to invisible area in front of the car.
    // It will display (getmax/2+offset) squares in one direction and
    // (getmax/2-offset) in the opposite direction (centered on the PC).
    const point maxoff((sight * 2 + 1 - getmaxx(w_terrain)) / 2,
                       (sight * 2 + 1 - getmaxy(w_terrain)) / 2);
    if (maxoff.x <= 0) {
        offset.x = 0;
    } else if (offset.x > 0 && offset.x > maxoff.x) {
        offset.x = maxoff.x;
    } else if (offset.x < 0 && -offset.x > maxoff.x) {
        offset.x = -maxoff.x;
    }
    if (maxoff.y <= 0) {
        offset.y = 0;
    } else if (offset.y > 0 && offset.y > maxoff.y) {
        offset.y = maxoff.y;
    } else if (offset.y < 0 && -offset.y > maxoff.y) {
        offset.y = -maxoff.y;
    }

    // Turn the offset into a vector that increments the offset toward the desired position
    // instead of setting it there instantly, should smooth out jerkiness.
    const point offset_difference(offset.x - driving_view_offset.x,
                                  offset.y - driving_view_offset.y);

    const point offset_sign((offset_difference.x < 0) ? -1 : 1,
                            (offset_difference.y < 0) ? -1 : 1);
    // Shift the current offset in the direction of the calculated offset by one tile
    // per draw event, but snap to calculated offset if we're close enough to avoid jitter.
    offset.x = (std::abs(offset_difference.x) > 1) ?
               (driving_view_offset.x + offset_sign.x) : offset.x;
    offset.y = (std::abs(offset_difference.y) > 1) ?
               (driving_view_offset.y + offset_sign.y) : offset.y;

    set_driving_view_offset(point(offset.x, offset.y));
}

// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool game::do_turn()
{
    if (is_game_over()) {
        return cleanup_at_end();
    }
    // Actual stuff
    if (new_game) {
        new_game = false;
    } else {
        gamemode->per_turn();
        calendar::turn.increment();
    }
    process_events();
    process_missions();
    if (calendar::turn.hours() == 0 && calendar::turn.minutes() == 0 &&
        calendar::turn.seconds() == 0) { // Midnight!
        cur_om->process_mongroups();
#ifdef LUA
        lua_callback(lua_state, "on_day_passed");
#endif
    }

    if (calendar::turn % 50 == 0) { //move hordes every 5 min
        cur_om->move_hordes();
        // Hordes that reached the reality bubble need to spawn,
        // make them spawn in invisible areas only.
        m.spawn_monsters( false );
    }

    // Check if we've overdosed... in any deadly way.
    if (u.stim > 250) {
        add_msg(m_bad, _("You have a sudden heart attack!"));
        u.add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        u.hp_cur[hp_torso] = 0;
    } else if (u.stim < -200 || u.pkill > 240) {
        add_msg(m_bad, _("Your breathing stops completely."));
        u.add_memorial_log(pgettext("memorial_male", "Died of a drug overdose."),
                           pgettext("memorial_female", "Died of a drug overdose."));
        u.hp_cur[hp_torso] = 0;
    } else if (u.has_effect("jetinjector")) {
            if (u.get_effect_dur("jetinjector") > 400) {
                if (!(u.has_trait("NOPAIN"))) {
                    add_msg(m_bad, _("Your heart spasms painfully and stops."));
                } else {
                    add_msg(_("Your heart spasms and stops."));
                }
                u.add_memorial_log(pgettext("memorial_male", "Died of a healing stimulant overdose."),
                                   pgettext("memorial_female", "Died of a healing stimulant overdose."));
                u.hp_cur[hp_torso] = 0;
            }
    } else if (u.has_effect("datura") && u.get_effect_dur("datura") > 14000 && one_in(512)) {
        if (!(u.has_trait("NOPAIN"))) {
            add_msg(m_bad, _("Your heart spasms painfully and stops, dragging you back to reality as you die."));
        } else {
            add_msg(_("You dissolve into beautiful paroxysms of energy.  Life fades from your nebulae and you are no more."));
        }
        u.add_memorial_log(pgettext("memorial_male", "Died of datura overdose."),
                           pgettext("memorial_female", "Died of datura overdose."));
        u.hp_cur[hp_torso] = 0;
    }
    // Check if we're starving or have starved
    if (u.hunger >= 3000) {
        if (u.hunger >= 6000) {
            add_msg(m_bad, _("You have starved to death."));
            u.add_memorial_log(pgettext("memorial_male", "Died of starvation."),
                               pgettext("memorial_female", "Died of starvation."));
            u.hp_cur[hp_torso] = 0;
        } else if (u.hunger >= 5000 && calendar::turn % 20 == 0) {
            add_msg(m_warning, _("Food..."));
        } else if (u.hunger >= 4000 && calendar::turn % 20 == 0) {
            add_msg(m_warning, _("You are STARVING!"));
        } else if (calendar::turn % 20 == 0) {
            add_msg(m_warning, _("Your stomach feels so empty..."));
        }
    }

    // Check if we're dying of thirst
    if (u.thirst >= 600) {
        if (u.thirst >= 1200) {
            add_msg(m_bad, _("You have died of dehydration."));
            u.add_memorial_log(pgettext("memorial_male", "Died of thirst."),
                               pgettext("memorial_female", "Died of thirst."));
            u.hp_cur[hp_torso] = 0;
        } else if (u.thirst >= 1000 && calendar::turn % 20 == 0) {
            add_msg(m_warning, _("Even your eyes feel dry..."));
        } else if (u.thirst >= 800 && calendar::turn % 20 == 0) {
            add_msg(m_warning, _("You are THIRSTY!"));
        } else if (calendar::turn % 20 == 0) {
            add_msg(m_warning, _("Your mouth feels so dry..."));
        }
    }

    // Check if we're falling asleep, unless we're sleeping
    if (u.fatigue >= 600 && !u.in_sleep_state()) {
        if (u.fatigue >= 1000) {
            add_msg(m_bad, _("Survivor sleep now."));
            u.add_memorial_log(pgettext("memorial_male", "Succumbed to lack of sleep."),
                               pgettext("memorial_female", "Succumbed to lack of sleep."));
            u.fatigue -= 10;
            u.try_to_sleep();
        } else if (u.fatigue >= 800 && calendar::turn % 10 == 0) {
            add_msg(m_warning, _("Anywhere would be a good place to sleep..."));
        } else if (calendar::turn % 50 == 0) {
            add_msg(m_warning, _("You feel like you haven't slept in days."));
        }
    }

    // Even if we're not Exhausted, we really should be feeling lack/sleep earlier
    // Penalties start at Dead Tired and go from there
    if (u.fatigue >= 383 && !u.in_sleep_state()) {
        if (u.fatigue >= 700) {
            if (calendar::turn % 50 == 0) {
                add_msg(m_warning, _("You're too tired to stop yawning."));
                u.add_effect("lack_sleep", 50);
            }
            if (one_in(50 + u.int_cur)) {
                // Rivet's idea: look out for microsleeps!
                u.fall_asleep(5);
            }
        } else if (u.fatigue >= 575) {
            if (calendar::turn % 50 == 0) {
                add_msg(m_warning, _("How much longer until bedtime?"));
                u.add_effect("lack_sleep", 50);
            }
            if (one_in(100 + u.int_cur)) {
                u.fall_asleep(5);
            }
        } else if (u.fatigue >= 383 && calendar::turn % 50 == 0) {
            add_msg(m_warning, _("*yawn* You should really get some sleep."));
            u.add_effect("lack_sleep", 50);
        }
    }

    if (calendar::turn % 50 == 0) { // Hunger, thirst, & fatigue up every 5 minutes
        if ((!u.has_trait("LIGHTEATER") || !one_in(3)) &&
            (!u.has_bionic("bio_recycler") || calendar::turn % 300 == 0) &&
            !(u.has_trait("DEBUG_LS"))) {
            u.hunger++;
            if (u.has_trait("HUNGER")) {
                if (one_in(2)) {
                    u.hunger++;
                }
            }
            if (u.has_trait("MET_RAT")) {
                if (!one_in(3)) {
                    u.hunger++;
                }
            }
            if (u.has_trait("HUNGER2")) {
                u.hunger++;
            }
            if (u.has_trait("HUNGER3")) {
                u.hunger += 2;
            }
        }
        if ((!u.has_bionic("bio_recycler") || calendar::turn % 100 == 0) &&
            (!u.has_trait("PLANTSKIN") || !one_in(5)) &&
            (!u.has_trait("DEBUG_LS")) ) {
            u.thirst++;
            if (u.has_trait("THIRST")) {
                if (one_in(2)) {
                    u.thirst++;
                }
            }
            if (u.has_trait("THIRST2")) {
                u.thirst++;
            }
            if (u.has_trait("THIRST3")) {
                u.thirst += 2;
            }
        }
        // Don't increase fatigue if sleeping or trying to sleep or if we're at the cap.
        if (u.fatigue < 1050 && !u.in_sleep_state() && !u.has_trait("DEBUG_LS") ) {
            u.fatigue++;
            // Wakeful folks don't always gain fatigue!
            if (u.has_trait("WAKEFUL")) {
                if (one_in(6)) {
                    u.fatigue--;
                }
            }
            if (u.has_trait("WAKEFUL2")) {
                if (one_in(4)) {
                    u.fatigue--;
                }
            }
            // You're looking at over 24 hours to hit Tired here
            if (u.has_trait("WAKEFUL3")) {
                if (one_in(2)) {
                    u.fatigue--;
                }
            }
            // Sleepy folks gain fatigue faster; Very Sleepy is twice as fast as typical
            if (u.has_trait("SLEEPY")) {
                if (one_in(3)) {
                    u.fatigue++;
                }
            }
            if (u.has_trait("MET_RAT")) {
                if (one_in(2)) {
                    u.fatigue++;
                }
            }
            if (u.has_trait("SLEEPY2")) {
                u.fatigue++;
            }
        }
        if (u.fatigue == 192 && !u.in_sleep_state()) {
            if (u.activity.type == ACT_NULL) {
                add_msg(m_warning, _("You're feeling tired.  %s to lie down for sleep."),
                        press_x(ACTION_SLEEP).c_str());
            } else {
                cancel_activity_query(_("You're feeling tired."));
            }
        }
        if (u.stim < 0) {
            u.stim++;
        }
        if (u.stim > 0) {
            u.stim--;
        }
        if (u.pkill > 0) {
            u.pkill--;
        }
        if (u.pkill < 0) {
            u.pkill++;
        }
        if (u.has_bionic("bio_solar") && is_in_sunlight(u.posx(), u.posy())) {
            u.charge_power(25);
        }
        // Huge folks take penalties for cramming themselves in vehicles
        if ((u.has_trait("HUGE") || u.has_trait("HUGE_OK")) && u.in_vehicle) {
            add_msg(m_bad, _("You're cramping up from stuffing yourself in this vehicle."));
            u.pain += 2 * rng(2, 3);
            u.focus_pool -= 1;
        }

        int dec_stom_food = u.stomach_food * 0.2;
        int dec_stom_water = u.stomach_water * 0.2;
        dec_stom_food = dec_stom_food < 10 ? 10 : dec_stom_food;
        dec_stom_water = dec_stom_water < 10 ? 10 : dec_stom_water;
        u.stomach_food -= dec_stom_food;
        u.stomach_water -= dec_stom_water;
        u.stomach_food = u.stomach_food < 0 ? 0 : u.stomach_food;
        u.stomach_water = u.stomach_water < 0 ? 0 : u.stomach_water;
    }

    if (calendar::turn % 300 == 0) { // Pain up/down every 30 minutes
        if (u.pain > 0) {
            u.pain -= 1 + int(u.pain / 10);
        } else if (u.pain < 0) {
            u.pain = 0;
        }
        // Mutation healing effects
        if (u.has_trait("FASTHEALER2") && one_in(5)) {
            u.healall(1);
        }
        if (u.has_trait("REGEN") && one_in(2)) {
            u.healall(1);
        }
        if (u.has_trait("ROT2") && one_in(5)) {
            u.hurtall(1);
        }
        if (u.has_trait("ROT3") && one_in(2)) {
            u.hurtall(1);
        }

        if (u.radiation > 0 && one_in(3)) {
            u.radiation--;
        }
        u.get_sick();
        // Freakishly Huge folks tire quicker
        if (u.has_trait("HUGE") && !u.in_sleep_state()) {
            add_msg(m_info, _("<whew> You catch your breath."));
            u.fatigue++;
        }
    }
    // auto-learning. This is here because skill-increases happens all over the place:
    // SkillLevel::readBook (has no connection to the skill or the player),
    // player::read, player::practice, ...
    if( u.skillLevel( "unarmed" ) >= 2 ) {
        if( std::find( u.ma_styles.begin(), u.ma_styles.end(), "style_brawling" ) == u.ma_styles.end() ) {
            u.ma_styles.push_back( "style_brawling" );
            add_msg( m_info, _( "You learned a new style." ) );
        }
    }

    if (calendar::turn % 3600 == 0)
    {
        u.update_health();
    }

    // Auto-save if autosave is enabled
    if (OPTIONS["AUTOSAVE"] &&
        calendar::turn % ((int)OPTIONS["AUTOSAVE_TURNS"]) == 0 &&
        !u.is_dead_state()) {
        autosave();
    }

    update_weather();

    // The following happens when we stay still; 10/40 minutes overdue for spawn
    if ((!u.has_trait("INCONSPICUOUS") && calendar::turn > nextspawn + 100) ||
        (u.has_trait("INCONSPICUOUS") && calendar::turn > nextspawn + 400)) {
        spawn_mon(-1 + 2 * rng(0, 1), -1 + 2 * rng(0, 1));
        nextspawn = calendar::turn;
    }

    process_activity();

    if (!u.in_sleep_state()) {
        if (u.moves > 0 || uquit == QUIT_WATCH) {
            while (u.moves > 0 || uquit == QUIT_WATCH) {
                cleanup_dead();
                if (u.activity.type == ACT_NULL) {
                    draw();
                }

                if (handle_action()) {
                    ++moves_since_last_save;
                    u.action_taken();
                }

                if (is_game_over()) {
                    return cleanup_at_end();
                }

                if (uquit == QUIT_WATCH) {
                    break;
                }
                if( u.activity.type != ACT_NULL ) {
                    process_activity();
                }
            }
        } else {
            handle_key_blocking_activity();
        }
    }

    if( driving_view_offset.x != 0 || driving_view_offset.y != 0 ) {
        // Still have a view offset, but might not be driving anymore,
        // or the option has been deactivated,
        // might also happen when someone dives from a moving car.
        // or when using the handbrake.
        vehicle *veh = m.veh_at(u.posx(), u.posy());
        if (veh == 0) {
            calc_driving_offset(0); // reset to (0,0)
        } else {
            calc_driving_offset(veh);
        }
    }
    update_scent();

    m.vehmove();
    // Process power and fuel consumption for all vehicles, including off-map ones.
    // m.vehmove used to do this, but now it only give them moves instead.
    for( auto &elem : MAPBUFFER ) {
        tripoint sm_loc = elem.first;
        point sm_topleft = overmapbuffer::sm_to_ms_copy(sm_loc.x, sm_loc.y);
        point in_reality = m.getlocal(sm_topleft);

        submap *sm = elem.second;

        for( auto &_i : sm->vehicles ) {
            auto veh = _i;

            veh->power_parts( sm_loc );
            veh->idle( sm_loc.z == levz && m.inbounds(in_reality.x, in_reality.y) );
        }
    }
    m.process_fields();
    m.process_active_items();
    m.creature_in_field( u );

    // Update vision caches for monsters. If this turns out to be expensive,
    // consider a stripped down cache just for monsters.
    m.build_map_cache();
    monmove();
    update_stair_monsters();
    u.process_turn();
    u.process_active_items();

    if (levz >= 0 && !u.is_underwater()) {
        weather_effect weffect;
        (weffect.*(weather_data[weather].effect))();
    }

    if (u.has_effect("sleep") && int(calendar::turn) % 300 == 0) {
        draw();
        refresh();
    }

    u.update_bodytemp();
    u.update_body_wetness();
    rustCheck();
    if (calendar::turn % 10 == 0) {
        u.update_morale();
    }

    return false;
}

void game::set_driving_view_offset(const point &p)
{
    // remove the previous driving offset,
    // store the new offset and apply the new offset.
    u.view_offset_x -= driving_view_offset.x;
    u.view_offset_y -= driving_view_offset.y;
    driving_view_offset.x = p.x;
    driving_view_offset.y = p.y;
    u.view_offset_x += driving_view_offset.x;
    u.view_offset_y += driving_view_offset.y;
}

void game::rustCheck()
{
    for (std::vector<const Skill*>::iterator aSkill = ++Skill::skills.begin();
         aSkill != Skill::skills.end(); ++aSkill) {
        if (u.rust_rate() <= rng(0, 1000)) {
            continue;
        }

        if ((*aSkill)->is_combat_skill() &&
            ((u.has_trait("PRED2") && one_in(4)) ||
             (u.has_trait("PRED3") && one_in(2)) ||
             (u.has_trait("PRED4") && x_in_y(2, 3)))) {
            // Their brain is optimized to remember this
            if (one_in(15600)) {
                // They've already passed the roll to avoid rust at
                // this point, but print a message about it now and
                // then.
                //
                // 13 combat skills, 600 turns/hr, 7800 tests/hr.
                // This means PRED2/PRED3/PRED4 think of hunting on
                // average every 8/4/3 hours, enough for immersion
                // without becoming an annoyance.
                //
                add_msg(_("Your heart races as you recall your most recent hunt."));
                u.stim++;
            }
            continue;
        }

        bool charged_bio_mem = u.has_active_bionic("bio_memory") && u.power_level > 25;
        int oldSkillLevel = u.skillLevel(*aSkill);

        if (u.skillLevel(*aSkill).rust(charged_bio_mem)) {
            u.power_level -= 25;
        }
        int newSkill = u.skillLevel(*aSkill);
        if (newSkill < oldSkillLevel) {
            add_msg(m_bad, _("Your skill in %s has reduced to %d!"),
                    (*aSkill)->name().c_str(), newSkill);
        }
    }
}

void game::process_events()
{
    for( auto it = events.begin(); it != events.end(); ) {
        it->per_turn();
        if (it->turn <= int(calendar::turn)) {
            it->actualize();
            it = events.erase(it);
        } else {
            it++;
        }
    }
}

void game::process_activity()
{
    if (u.activity.type == ACT_NULL) {
        return;
    }
    if (int(calendar::turn) % 50 == 0) {
        draw();
    }
    while( u.moves > 0 && u.activity.type != ACT_NULL ) {
        activity_on_turn();
        if (u.activity.moves_left <= 0) { // We finished our activity!
            activity_on_finish();
        }
    }
}

void on_turn_activity_pickaxe(player *p);
void on_finish_activity_pickaxe(player *p);

void on_turn_activity_burrow(player *p);
void on_finish_activity_burrow(player *p);

void game::activity_on_turn()
{
    switch (u.activity.type) {
    case ACT_WAIT:
    case ACT_WAIT_WEATHER:
        // Based on time, not speed
        u.activity.moves_left -= 100;
        u.rooted();
        u.pause();
        break;
    case ACT_PICKAXE:
        // Based on speed, not time
        u.activity.moves_left -= u.moves;
        u.moves = 0;
        on_turn_activity_pickaxe(&u);
        break;
    case ACT_BURROW:
        // Based on speed, not time
        u.activity.moves_left -= u.moves;
        u.moves = 0;
        on_turn_activity_burrow(&u);
        break;
    case ACT_AIM:
        if( u.activity.index == 0 ) {
            plfire(false);
        }
        break;
    case ACT_GAME:
        // Takes care of u.activity.moves_left
        activity_on_turn_game();
        break;
    case ACT_VIBE:
        // Takes care of u.activity.moves_left
        activity_on_turn_vibe();
        break;
    case ACT_REFILL_VEHICLE:
        // Takes care of u.activity.moves_left
        activity_on_turn_refill_vehicle();
        break;
    case ACT_PULP:
        // does not really use u.activity.moves_left, stops itself when finished
        activity_on_turn_pulp();
        break;
    case ACT_FISH:
        // Based on time, not speed--or it should be
        // (Being faster doesn't make the fish bite quicker)
        u.activity.moves_left -= 100;
        u.rooted();
        u.pause();
        break;
    case ACT_DROP:
        activity_on_turn_drop();
        break;
    case ACT_STASH:
        activity_on_turn_stash();
        break;
    case ACT_PICKUP:
        activity_on_turn_pickup();
        break;
    case ACT_MOVE_ITEMS:
        activity_on_turn_move_items();
        break;
    case ACT_ADV_INVENTORY:
        u.cancel_activity();
        advanced_inv();
        break;
    case ACT_START_FIRE:
        u.activity.moves_left -= 100; // based on time
        if (u.i_at(u.activity.position).has_flag("LENS")) { // if using a lens, handle potential changes in weather
            activity_on_turn_start_fire_lens();
        }
        u.rooted();
        u.pause();
        break;
    case ACT_FILL_LIQUID:
        activity_on_turn_fill_liquid();
        break;
    default:
        // Based on speed, not time
        if( u.moves <= u.activity.moves_left ) {
            u.activity.moves_left -= u.moves;
            u.moves = 0;
        } else {
            u.moves -= u.activity.moves_left;
            u.activity.moves_left = 0;
        }
    }
}

void game::activity_on_turn_game()
{
    //Gaming takes time, not speed
    u.activity.moves_left -= 100;

    item &game_item = u.i_at(u.activity.position);

    //Deduct 1 battery charge for every minute spent playing
    if (int(calendar::turn) % 10 == 0) {
        game_item.charges--;
        u.add_morale(MORALE_GAME, 1, 100); //1 points/min, almost 2 hours to fill
    }
    if (game_item.charges == 0) {
        u.activity.moves_left = 0;
        add_msg(m_info, _("The %s runs out of batteries."), game_item.tname().c_str());
    }

    u.rooted();
    u.pause();
}

void game::activity_on_turn_vibe()
{
    //Using a vibrator takes time, not speed
    u.activity.moves_left -= 100;

    item &vibrator_item = u.i_at(u.activity.position);

    if ((u.is_wearing("rebreather")) || (u.is_wearing("rebreather_xl")) ||
        (u.is_wearing("mask_h20survivor"))) {
        u.activity.moves_left = 0;
        add_msg(m_bad, _("You have trouble breathing, and stop."));
    }

    //Deduct 1 battery charge for every minute using the vibrator
    if (int(calendar::turn) % 10 == 0) {
        vibrator_item.charges--;
        u.add_morale(MORALE_FEELING_GOOD, 4, 320); //4 points/min, one hour to fill
        // 1:1 fatigue:morale ratio, so maxing the morale is possible but will take
        // you pretty close to Dead Tired from a well-rested state.
        u.fatigue += 4;
    }
    if (vibrator_item.charges == 0) {
        u.activity.moves_left = 0;
        add_msg(m_info, _("The %s runs out of batteries."), vibrator_item.tname().c_str());
    }
    if (u.fatigue >= 383) { // Dead Tired: different kind of relaxation needed
        u.activity.moves_left = 0;
        add_msg(m_info, _("You're too tired to continue."));
    }

    // Vibrator requires that you be able to move around, stretch, etc, so doesn't play
    // well with roots.  Sorry.  :-(

    u.pause();
}

void game::activity_on_turn_fill_liquid()
{
    //Filling a container takes time, not speed
    u.activity.moves_left -= 100;

    item *container = &u.i_at(u.activity.position);
    item water = item(u.activity.str_values[0], u.activity.values[1]);
    water.poison = u.activity.values[0];
    // Fill up 10 charges per time
    water.charges = 10;

    if (handle_liquid(water, true, true, NULL, container) == false) {
        u.activity.moves_left = 0;
    }

    u.rooted();
    u.pause();
}

void game::activity_on_turn_refill_vehicle()
{
    vehicle *veh = NULL;
    veh = m.veh_at(u.activity.placement.x, u.activity.placement.y);
    if (!veh) {  // Vehicle must've moved or something!
        u.activity.moves_left = 0;
        return;
    }
    bool fuel_pumped = false;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if( m.ter(u.posx() + i, u.posy() + j) == t_gas_pump ||
                m.ter_at(u.posx() + i, u.posy() + j).id == "t_gas_pump_a" ||
                m.ter(u.posx() +i, u.posy() + j) == t_diesel_pump ) {
                auto maybe_gas = m.i_at(u.posx() + i, u.posy() + j);
                for( auto gas = maybe_gas.begin(); gas != maybe_gas.end(); ) {
                    if( gas->type->id == "gasoline" || gas->type->id == "diesel" ) {
                        fuel_pumped = true;
                        int lack = std::min( veh->fuel_capacity(gas->type->id) -
                                             veh->fuel_left(gas->type->id),  200 );
                        if (gas->charges > lack) {
                            veh->refill(gas->type->id, lack);
                            gas->charges -= lack;
                            u.activity.moves_left -= 100;
                            gas++;
                        } else {
                            add_msg(m_bad, _("With a clang and a shudder, the pump goes silent."));
                            veh->refill (gas->type->id, gas->charges);
                            gas = maybe_gas.erase( gas );
                            u.activity.moves_left = 0;
                        }
                        i = 2;
                        j = 2;
                        break;
                    }
                }
            }
        }
    }
    if( !fuel_pumped ) {
        // Can't find any fuel, give up.
        debugmsg("Can't find any fuel, cancelling pumping.");
        u.cancel_activity();
        return;
    }
    u.pause();
}

void game::activity_on_turn_start_fire_lens()
{
    float natural_light_level = this->natural_light_level();
    // if the weather changes, we cannot start a fire with a lens. abort activity
    if (!((weather == WEATHER_CLEAR) || (weather == WEATHER_SUNNY)) || !( natural_light_level >= 60 )) {
        add_msg(m_bad, _("There is not enough sunlight to start a fire now. You stop trying."));
        u.cancel_activity();
    } else if (natural_light_level != u.activity.values.back()) { // when lighting changes we recalculate the time needed
        float previous_natural_light_level = u.activity.values.back();
        u.activity.values.pop_back();
        u.activity.values.push_back(natural_light_level); // update light level
        iuse tmp;
        float progress_left = float(u.activity.moves_left)/float(tmp.calculate_time_for_lens_fire(&u, previous_natural_light_level));
        u.activity.moves_left = int(progress_left*(tmp.calculate_time_for_lens_fire(&u, natural_light_level))); // update moves left
    }
}

void game::activity_on_finish()
{
    switch (u.activity.type) {
    case ACT_RELOAD:
        activity_on_finish_reload();
        break;
    case ACT_READ:
        u.do_read(&(u.i_at(u.activity.position)));
        if (u.activity.type == ACT_NULL) {
            add_msg(_("You finish reading."));
        }
        break;
    case ACT_WAIT:
    case ACT_WAIT_WEATHER:
        add_msg(_("You finish waiting."));
        u.activity.type = ACT_NULL;
        break;
    case ACT_CRAFT:
        u.complete_craft();
        u.activity.type = ACT_NULL;
        break;
    case ACT_LONGCRAFT:
        u.complete_craft();
        u.activity.type = ACT_NULL;
        {
            int batch_size = u.activity.values.front();
            if( u.making_would_work( u.lastrecipe, batch_size ) ) {
                u.make_all_craft(u.lastrecipe, batch_size);
            }
        }
        break;
    case ACT_FORAGE:
        forage();
        u.activity.type = ACT_NULL;
        break;
    case ACT_DISASSEMBLE:
        u.complete_disassemble();
        u.activity.type = ACT_NULL;
        break;
    case ACT_BUTCHER:
        complete_butcher(u.activity.index);
        u.activity.type = ACT_NULL;
        break;
    case ACT_LONGSALVAGE:
        longsalvage();
        break;
    case ACT_VEHICLE:
        activity_on_finish_vehicle();
        break;
    case ACT_BUILD:
        complete_construction();
        u.activity.type = ACT_NULL;
        break;
    case ACT_TRAIN:
        activity_on_finish_train();
        break;
    case ACT_FIRSTAID:
        activity_on_finish_firstaid();
        break;
    case ACT_FISH:
        activity_on_finish_fish();
        break;
    case ACT_PICKAXE:
        on_finish_activity_pickaxe(&u);
        u.activity.type = ACT_NULL;
        break;
    case ACT_BURROW:
        on_finish_activity_burrow(&u);
        u.activity.type = ACT_NULL;
        break;
    case ACT_VIBE:
        add_msg(m_good, _("You feel much better."));
        u.activity.type = ACT_NULL;
        break;
    case ACT_MAKE_ZLAVE:
        activity_on_finish_make_zlave();
        u.activity.type = ACT_NULL;
        break;
    case ACT_PICKUP:
    case ACT_MOVE_ITEMS:
        // Do nothing, the only way this happens is if we set this activity after
        // entering the advanced inventory menu as an activity, and we want it to play out.
        break;
    case ACT_START_FIRE:
        activity_on_finish_start_fire();
        break;
    case ACT_HOTWIRE_CAR:
        activity_on_finish_hotwire();
    case ACT_AIM:
        // Aim bails itself by resetting itself every turn,
        // you only re-enter if it gets set again.
        break;
    default:
        u.activity.type = ACT_NULL;
    }
    if (u.activity.type == ACT_NULL) {
        // Make sure data of previous activity is cleared
        u.activity = player_activity();
        if( !u.backlog.empty() && u.backlog.front().auto_resume ) {
            u.activity = u.backlog.front();
            u.backlog.pop_front();
        }
    }
}

void game::activity_on_finish_make_zlave()
{
    static const int full_pulp_threshold = 4;

    auto items = m.i_at(u.posx(), u.posy());
    std::string corpse_name = u.activity.str_values[0];
    item *body = NULL;

    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( it->display_name() == corpse_name ) {
            body = &*it;
        }
    }

    if (body == NULL) {
        add_msg(m_info, _("There's no corpse to make into a zombie slave!"));
        return;
    }

    int success = u.activity.values[0];

    if (success > 0) {

        u.practice("firstaid", rng(2, 5));
        u.practice("survival", rng(2, 5));

        u.add_msg_if_player(m_good,
                            _("You slice muscles and tendons, and remove body parts until you're confident the zombie won't be able to attack you when it reainmates."));

        body->set_var( "zlave", "zlave" );
        //take into account the chance that the body yet can regenerate not as we need.
        if (one_in(10)) {
            body->set_var( "zlave", "mutilated" );
        }

    } else {

        if (success > -20) {

            u.practice("firstaid", rng(3, 6));
            u.practice("survival", rng(3, 6));

            u.add_msg_if_player(m_warning,
                                _("You hack into the corpse and chop off some body parts.  You think the zombie won't be able to attack when it reanimates."));

            success += rng(1, 20);

            if (success > 0 && !one_in(5)) {
                body->set_var( "zlave", "zlave" );
            } else {
                body->set_var( "zlave", "mutilated" );
            }

        } else {

            u.practice("firstaid", rng(1, 8));
            u.practice("survival", rng(1, 8));

            int pulp = rng(1, full_pulp_threshold);

            body->damage += pulp;

            if (body->damage >= full_pulp_threshold) {
                body->damage = full_pulp_threshold;
                body->active = false;

                u.add_msg_if_player(m_warning, _("You cut up the corpse too much, it is thoroughly pulped."));
            } else {
                u.add_msg_if_player(m_warning, _("You cut into the corpse trying to make it unable to attack, but you don't think you have it right."));
            }
        }
    }
}

void game::activity_on_finish_reload()
{
    item *reloadable = NULL;
    {
        int reloadable_pos;
        std::stringstream ss(u.activity.name);
        ss >> reloadable_pos;
        reloadable = &u.i_at(reloadable_pos);
    }
    if (reloadable->reload(u, u.activity.position)) {
        if (reloadable->is_gun() && reloadable->has_flag("RELOAD_ONE")) {
            if (reloadable->ammo_type() == "bolt") {
                add_msg(_("You insert a bolt into your %s."),
                        reloadable->tname().c_str());
            } else {
                add_msg(_("You insert a cartridge into your %s."),
                        reloadable->tname().c_str());
            }
            u.recoil = std::max(MIN_RECOIL, (MIN_RECOIL + u.recoil) / 2);
        } else {
            add_msg(_("You reload your %s."), reloadable->tname().c_str());
            u.recoil = MIN_RECOIL;
        }
    } else {
        add_msg(m_info, _("Can't reload your %s."), reloadable->tname().c_str());
    }
    u.activity.type = ACT_NULL;
}

void game::activity_on_finish_train()
{
    const Skill* skill = Skill::skill(u.activity.name);
    if (skill == NULL) {
        // Trained martial arts,
        add_msg(m_good, _("You learn %s."), martialarts[u.activity.name].name.c_str());
        //~ %s is martial art
        u.add_memorial_log(pgettext("memorial_male", "Learned %s."),
                           pgettext("memorial_female", "Learned %s."),
                           martialarts[u.activity.name].name.c_str());
        u.add_martialart(u.activity.name);
    } else {
        int new_skill_level = u.skillLevel(skill) + 1;
        u.skillLevel(skill).level(new_skill_level);
        add_msg(m_good, _("You finish training %s to level %d."),
                skill->name().c_str(),
                new_skill_level);
        if (new_skill_level % 4 == 0) {
            //~ %d is skill level %s is skill name
            u.add_memorial_log(pgettext("memorial_male", "Reached skill level %1$d in %2$s."),
                               pgettext("memorial_female", "Reached skill level %1$d in %2$s."),
                               new_skill_level, skill->name().c_str());
        }
    }
    u.activity.type = ACT_NULL;
}

void game::activity_on_finish_firstaid()
{
    item &it = u.i_at(u.activity.position);
    iuse tmp;
    tmp.completefirstaid(&u, &it, false, u.pos());
    u.reduce_charges(u.activity.position, 1);
    // Erase activity and values.
    u.activity.type = ACT_NULL;
    u.activity.values.clear();
}

void game::activity_on_finish_start_fire()
{
    item &it = u.i_at(u.activity.position);
    iuse tmp;
    tmp.resolve_firestarter_use(&u, &it, u.activity.placement);
    u.activity.type = ACT_NULL;
}

void game::activity_on_finish_hotwire()
{
    //Grab this now, in case the vehicle gets shifted
    vehicle *veh = m.veh_at(u.activity.values[0], u.activity.values[1]);
    if (veh) {
        int mech_skill = u.activity.values[2];
        if (mech_skill > (int)rng(1,6)){
            //success
            veh->is_locked = false;
            add_msg(_("This wire will start the engine."));
        } else if (mech_skill > (int)rng(0,4)) {
            //soft fail
            veh->is_locked = false;
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("This wire will probably start the engine."));
        } else if (veh->is_alarm_on){
            veh->is_locked = false;
            add_msg(_("By process of elimination, this wire will start the engine."));
        } else {
            //hard fail
            veh->is_alarm_on = veh->has_security_working();
            add_msg(_("The red wire always starts the engine, doesn't it?"));
        }
    } else {
        dbg(D_ERROR) << "game:process_activity: ACT_HOTWIRE_CAR: vehicle not found";
        debugmsg("process_activity ACT_HOTWIRE_CAR: vehicle not found");
    }
    u.activity.type = ACT_NULL;

}

void game::activity_on_finish_fish()
{
    item &it = u.i_at(u.activity.position);
    int sSkillLevel = 0; //given values to avoid possible null errors if the item used has not any of the flags.
    int fishChance = 20;
    if (it.has_flag("FISH_POOR")) {
        sSkillLevel = u.skillLevel("survival") + dice(1, 6);
        fishChance = dice(1, 20);
    } else if (it.has_flag("FISH_GOOD")) {
        sSkillLevel = u.skillLevel("survival")*1.5 + dice(1, 6) + 3; //much better chances with a good fishing implement
        fishChance = dice(1, 20);
    }
    rod_fish(sSkillLevel,fishChance);
    u.practice("survival", rng(5, 15));
    u.activity.type = ACT_NULL;
}

void game::rod_fish(int sSkillLevel, int fishChance) // fish-with-rod fish catching function
{
    if (sSkillLevel > fishChance) {
        std::vector<monster*> fishables = get_fishable(60); //get the nearby fish list.
        //if the vector is empty (no fish around) the player is still given a small chance to get a (let us say it was hidden) fish
        if (fishables.size() < 1){
            if (one_in(20)) {
            item fish;
            std::vector<std::string> fish_group = MonsterGroupManager::GetMonstersFromGroup("GROUP_FISH");
            std::string fish_mon = fish_group[rng(1, fish_group.size()) - 1];
            fish.make_corpse( fish_mon, calendar::turn );
            m.add_item_or_charges(u.posx(), u.posy(), fish);
            u.add_msg_if_player(m_good, _("You caught a %s."), GetMType(fish_mon)->nname().c_str());
            } else {
                u.add_msg_if_player(_("You didn't catch anything."));
            }
        } else {
            catch_a_monster(fishables, u.posx(), u.posy(), &u, 30000);
        }

    } else {
        u.add_msg_if_player(_("You didn't catch anything."));
    }
}

void game::catch_a_monster(std::vector<monster*> &catchables, int posx, int posy, player *p, int catch_duration) // catching function
{
    int index = rng(1, catchables.size()) - 1; //get a random monster from the vector
    //spawn the corpse, rotten by a part of the duration
    item fish;
    fish.make_corpse( catchables[index]->type, calendar::turn + int(rng(0, catch_duration)) );
    m.add_item_or_charges(posx, posy, fish);
    u.add_msg_if_player(m_good, _("You caught a %s."), catchables[index]->type->nname().c_str());
    //quietly kill the catched
    catchables[index]->no_corpse_quiet = true;
    catchables[index]->die( p );
    catchables.erase (catchables.begin()+index);
}

void game::activity_on_finish_vehicle()
{
    //Grab this now, in case the vehicle gets shifted
    vehicle *veh = m.veh_at(u.activity.values[0], u.activity.values[1]);
    complete_vehicle();
    // complete_vehicle set activity type to NULL if the vehicle
    // was completely dismantled, otherwise the vehicle still exist and
    // is to be examined again.
    if (u.activity.type == ACT_NULL) {
        return;
    }
    u.activity.type = ACT_NULL;
    if (u.activity.values.size() < 7) {
        dbg(D_ERROR) << "game:process_activity: invalid ACT_VEHICLE values: "
                     << u.activity.values.size();
        debugmsg("process_activity invalid ACT_VEHICLE values:%d",
                 u.activity.values.size());
    } else {
        if (veh) {
            refresh_all();
            exam_vehicle(*veh, u.activity.values[0], u.activity.values[1],
                         u.activity.values[2], u.activity.values[3]);
            return;
        } else {
            dbg(D_ERROR) << "game:process_activity: ACT_VEHICLE: vehicle not found";
            debugmsg("process_activity ACT_VEHICLE: vehicle not found");
        }
    }
}

void game::cancel_activity()
{
    u.cancel_activity();
}

bool game::cancel_activity_or_ignore_query(const char *reason, ...)
{
    if (u.activity.type == ACT_NULL) {
        return false;
    }
    va_list ap;
    va_start(ap, reason);
    const std::string text = vstring_format(reason, ap);
    va_end(ap);

    bool force_uc = OPTIONS["FORCE_CAPITAL_YN"];
    int ch = (int)' ';

    std::string stop_message = text + u.activity.get_stop_phrase() +
                               _(" (Y)es, (N)o, (I)gnore further distractions and finish.");

    do {
        ch = popup(stop_message, PF_GET_KEY);
    } while (ch != '\n' && ch != ' ' && ch != KEY_ESCAPE &&
             ch != 'Y' && ch != 'N' && ch != 'I' &&
             (force_uc || (ch != 'y' && ch != 'n' && ch != 'i')));

    if (ch == 'Y' || ch == 'y') {
        u.cancel_activity();
    } else if (ch == 'I' || ch == 'i') {
        return true;
    }
    return false;
}

bool game::cancel_activity_query(const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    const std::string text = vstring_format(message, ap);
    va_end(ap);

    if (ACT_NULL == u.activity.type) {
        if (u.has_destination()) {
            add_msg(m_warning, _("%s. Auto-move canceled"), text.c_str());
            u.clear_destination();
        }
        return false;
    }
    if (query_yn("%s%s", text.c_str(), u.activity.get_stop_phrase().c_str())) {
        u.cancel_activity();
        return true;
    }
    return false;
}

void game::update_weather()
{
    if (calendar::turn >= nextweather) {
        if(!has_generator) {
            weather_generator weatherGen(weatherSeed);
            has_generator = true;
        }
//        debugmsg("Generating weather for turn %d", int(calendar::turn));
        w_point w = weatherGen.get_weather(u.pos(), calendar::turn);
        weather_type old_weather = weather;
        weather = weatherGen.get_weather_conditions(w);
        if (weather == WEATHER_SUNNY && calendar::turn.is_night()) { weather = WEATHER_CLEAR; }
        temperature = w.temperature;
        lightning_active = false;
        nextweather += 50; // Check weather each 50 turns.
        if (weather != old_weather && weather_data[weather].dangerous &&
            levz >= 0 && m.is_outside(u.posx(), u.posy())
            && !u.has_activity(ACT_WAIT_WEATHER)) {
            cancel_activity_query(_("The weather changed to %s!"), weather_data[weather].name.c_str());
        }

        if (weather != old_weather && u.has_activity(ACT_WAIT_WEATHER)) {
            u.assign_activity(ACT_WAIT_WEATHER, 0, 0);
        }
    }
}

int game::get_temperature()
{
    return temperature + m.temperature(u.posx(), u.posy());
}

int game::assign_mission_id()
{
    int ret = next_mission_id;
    next_mission_id++;
    return ret;
}

void game::give_mission(mission_id type)
{
    mission tmp = mission_types[type].create();
    active_missions.push_back(tmp);
    u.active_missions.push_back(tmp.uid);
    u.active_mission = u.active_missions.size() - 1;
    mission_start m_s;
    mission *miss = find_mission(tmp.uid);
    (m_s.*miss->type->start)(miss);
}

void game::assign_mission(int id)
{
    u.active_missions.push_back(id);
    u.active_mission = u.active_missions.size() - 1;
    mission_start m_s;
    mission *miss = find_mission(id);
    (m_s.*miss->type->start)(miss);
}

int game::reserve_mission(mission_id type, int npc_id)
{
    mission tmp = mission_types[type].create(npc_id);
    active_missions.push_back(tmp);
    return tmp.uid;
}

int game::reserve_random_mission(mission_origin origin, point p, int npc_id)
{
    std::vector<int> valid;
    mission_place place;
    for( auto &elem : mission_types ) {
        for( std::vector<mission_origin>::iterator orig = elem.origins.begin();
             orig != elem.origins.end(); ++orig ) {
            if( *orig == origin && ( place.*elem.place )( p.x, p.y ) ) {
                valid.push_back( elem.id );
                break;
            }
        }
    }

    if (valid.empty()) {
        return -1;
    }

    int index = valid[rng(0, valid.size() - 1)];

    return reserve_mission(mission_id(index), npc_id);
}

npc *game::find_npc(int id)
{
    return overmap_buffer.find_npc(id);
}

int game::kill_count(std::string mon)
{
    if (kills.find(mon) != kills.end()) {
        return kills[mon];
    }
    return 0;
}

void game::increase_kill_count(const std::string &mtype_id)
{
    kills[mtype_id]++;
}

mission *game::find_mission(int id)
{
    for( auto &elem : active_missions ) {
        if( elem.uid == id ) {
            return &elem;
        }
    }
    dbg(D_ERROR) << "game:find_mission: " << id << " - it's NULL!";
    debugmsg("game::find_mission(%d) - it's NULL!", id);
    return NULL;
}

mission_type *game::find_mission_type(int id)
{
    for( auto &elem : active_missions ) {
        if( elem.uid == id ) {
            return elem.type;
        }
    }
    return NULL;
}

bool game::mission_complete(int id, int npc_id)
{
    mission *miss = find_mission(id);
    if (miss == NULL) {
        return false;
    }
    mission_type *type = miss->type;
    switch (type->goal) {
    case MGOAL_GO_TO: {
        // TODO: target does not contain a z-component, targets are assume to be on z=0
        const tripoint cur_pos = om_global_location();
        return (rl_dist(cur_pos.x, cur_pos.y, miss->target.x, miss->target.y) <= 1);
    }
    break;

    case MGOAL_GO_TO_TYPE: {
        oter_id cur_ter = overmap_buffer.ter(om_global_location());
        if (cur_ter == miss->type->target_id) {
            return true;
        }
        return false;
    }
    break;

    case MGOAL_FIND_ITEM:
        if (!u.has_amount(type->item_id, miss->item_count)) {
            if (u.has_amount(type->item_id, 1) && u.has_charges(type->item_id, miss->item_count))
                return true;
            else
                return false;
        }
        if (miss->npc_id != -1 && miss->npc_id != npc_id) {
            return false;
        }
        return true;

    case MGOAL_FIND_ANY_ITEM:
        return (u.has_mission_item(miss->uid) &&
                (miss->npc_id == -1 || miss->npc_id == npc_id));

    case MGOAL_FIND_MONSTER:
        if (miss->npc_id != -1 && miss->npc_id != npc_id) {
            return false;
        }
        for (size_t i = 0; i < num_zombies(); i++) {
            if (zombie(i).mission_id == miss->uid) {
                return true;
            }
        }
        return false;

    case MGOAL_RECRUIT_NPC: {
        npc *p = find_npc(miss->target_npc_id);
        return (p != NULL && p->attitude == NPCATT_FOLLOW);
    }

    case MGOAL_RECRUIT_NPC_CLASS: {
        std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(100);
        for( auto &npcs_it : npcs ) {
            if( ( npcs_it )->myclass == miss->recruit_class ) {
                if( ( npcs_it )->attitude == NPCATT_FOLLOW ) {
                    return true;
                }
            }
        }
        return false;
    }

    case MGOAL_FIND_NPC:
        return (miss->npc_id == npc_id);

    case MGOAL_ASSASSINATE:
        return (miss->step >= 1);

    case MGOAL_KILL_MONSTER:
        return (miss->step >= 1);

    case MGOAL_KILL_MONSTER_TYPE:
        debugmsg("%d kill count", kill_count(miss->monster_type));
        debugmsg("%d goal", miss->monster_kill_goal);
        if (kill_count(miss->monster_type) >= miss->monster_kill_goal) {
            return true;
        }
        return false;

    default:
        return false;
    }
    return false;
}

bool game::mission_failed(int id)
{
    mission *miss = find_mission(id);
    if (miss == NULL) {
        return true;    //If the mission is null it is failed.
    }
    return (miss->failed);
}

void game::wrap_up_mission(int id)
{
    mission *miss = find_mission(id);
    if (miss == NULL) {
        return;
    }
    u.completed_missions.push_back( id );
    for( auto it = u.active_missions.begin(); it != u.active_missions.end();) {
        if (*it == id) {
            it = u.active_missions.erase(it);
        } else {
            it++;
        }
    }
    switch (miss->type->goal) {
    case MGOAL_FIND_ITEM:
        if( item::count_by_charges( miss->type->item_id ) ) {
            u.use_charges(miss->type->item_id, miss->item_count);
        } else {
            u.use_amount(miss->type->item_id, miss->item_count);
        }
        break;
    case MGOAL_FIND_ANY_ITEM:
        u.remove_mission_items(miss->uid);
        break;
    default:
        //Suppress warnings
        break;
    }
    mission_end endfunc;
    (endfunc.*miss->type->end)(miss);
}

void game::fail_mission(int id)
{
    mission *miss = find_mission(id);
    if (miss == NULL) {
        return;
    }
    miss->failed = true;
    u.failed_missions.push_back( id );
    for (std::vector<int>::iterator it = u.active_missions.begin();
         it != u.active_missions.end();) {
        if (*it == id) {
            it = u.active_missions.erase(it);
        } else {
            it++;
        }
    }
    mission_fail failfunc;
    (failfunc.*miss->type->fail)(miss);
}

void game::mission_step_complete(int id, int step)
{
    mission *miss = find_mission(id);
    if (miss == NULL) {
        return;
    }
    miss->step = step;
    switch (miss->type->goal) {
    case MGOAL_FIND_ITEM:
    case MGOAL_FIND_MONSTER:
    case MGOAL_ASSASSINATE:
    case MGOAL_KILL_MONSTER: {
        npc *p = find_npc(miss->npc_id);
        if (p != NULL) {
            tripoint t = p->global_omt_location();
            miss->target.x = t.x;
            miss->target.y = t.y;
        } else {
            miss->target = overmap::invalid_point;
        }
        break;
    }
    default:
        //Suppress warnings
        break;
    }
}

void game::process_missions()
{
    for( auto &elem : active_missions ) {
        if( elem.deadline > 0 && !elem.failed && int( calendar::turn ) > elem.deadline ) {
            fail_mission( elem.uid );
        }
    }
}

void game::handle_key_blocking_activity()
{
    // If player is performing a task and a monster is dangerously close, warn them
    // regardless of previous safemode warnings
    if( u.activity.type != ACT_NULL && u.activity.type != ACT_AIM &&
        u.activity.moves_left > 0 && !u.activity.warned_of_proximity ) {
        Creature *hostile_critter = is_hostile_very_close();
        if (hostile_critter != nullptr) {
            u.activity.warned_of_proximity = true;
            if ( cancel_activity_query(_("You see %s approaching!"),
                    hostile_critter->disp_name().c_str()) ) {
                return;
            }
        }
    }

    if (u.activity.moves_left > 0 && u.activity.is_abortable()) {
        input_context ctxt = get_default_mode_input_context();
        timeout(1);
        const std::string action = ctxt.handle_input();
        timeout(-1);
        if (action == "pause") {
            cancel_activity_query(_("Confirm:"));
        } else if (action == "player_data") {
            u.disp_info();
            refresh_all();
        } else if (action == "messages") {
            Messages::display_messages();
            refresh_all();
        } else if (action == "help") {
            display_help();
            refresh_all();
        }
    }
}

/* item submenu for 'i' and '/'
* It use draw_item_info to draw item info and action menu
*
* @param pos position of item in inventory
* @param iStartX Left coord of the item info window
* @param iWidth width of the item info window (height = height of terminal)
* @param position It is position of the action menu. Default 0
*       -2 - near the right edge of the terminal window
*       -1 - left before item info window
*       0 - right after item info window
*       1 - near the left edge of the terminal window
* @return getch
*/
int game::inventory_item_menu(int pos, int iStartX, int iWidth, int position)
{
    int cMenu = (int)'+';

    if (u.has_item(pos)) {
        item &oThisItem = u.i_at(pos);
        std::vector<iteminfo> vThisItem, vDummy, vMenu;

        const int iOffsetX = 2;
        const bool bHPR = hasPickupRule(oThisItem.tname());
        const hint_rating rate_drop_item = u.weapon.has_flag("NO_UNWIELD") ? HINT_CANT : HINT_GOOD;

        int max_text_length = 0;
        int length = 0;
        vMenu.push_back(iteminfo("MENU", "", "iOffsetX", iOffsetX));
        vMenu.push_back(iteminfo("MENU", "", "iOffsetY", 0));
        length = utf8_width(_("<a>ctivate"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "a", _("<a>ctivate"), u.rate_action_use(&oThisItem)));
        length = utf8_width(_("<R>ead"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "R", _("<R>ead"), u.rate_action_read(&oThisItem)));
        length = utf8_width(_("<E>at"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "E", _("<E>at"), u.rate_action_eat(&oThisItem)));
        length = utf8_width(_("<W>ear"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "W", _("<W>ear"), u.rate_action_wear(&oThisItem)));
        length = utf8_width(_("<w>ield"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "w", _("<w>ield")));
        length = utf8_width(_("<t>hrow"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "t", _("<t>hrow")));
        length = utf8_width(_("<T>ake off"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "T", _("<T>ake off"), u.rate_action_takeoff(&oThisItem)));
        length = utf8_width(_("<d>rop"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "d", _("<d>rop"), rate_drop_item));
        length = utf8_width(_("<U>nload"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "U", _("<U>nload"), u.rate_action_unload( oThisItem )));
        length = utf8_width(_("<r>eload"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "r", _("<r>eload"), u.rate_action_reload(&oThisItem)));
        length = utf8_width(_("<D>isassemble"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "D", _("<D>isassemble"), u.rate_action_disassemble(&oThisItem)));
        length = utf8_width(_("<=> reassign"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", "=", _("<=> reassign")));
        length = utf8_width(_("<-> Autopickup"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        length = utf8_width(_("<+> Autopickup"));
        if (length > max_text_length) {
            max_text_length = length;
        }
        vMenu.push_back(iteminfo("MENU", (bHPR) ? "-" : "+",
                                 (bHPR) ? _("<-> Autopickup") : _("<+> Autopickup"), (bHPR) ? HINT_IFFY : HINT_GOOD));

        int offset_line = 0;
        int max_line = 0;
        const std::string str = oThisItem.info(true, &vThisItem);
        const std::string item_name = oThisItem.tname();
        WINDOW *w = newwin(TERMY - VIEW_OFFSET_Y * 2, iWidth, VIEW_OFFSET_Y, iStartX + VIEW_OFFSET_X);
        WINDOW_PTR wptr( w );

        wmove(w, 1, 2);
        wprintz(w, c_white, "%s", item_name.c_str());
        max_line = fold_and_print_from(w, 3, 2, iWidth - 4, offset_line, c_white, str);
        if (max_line > TERMY - VIEW_OFFSET_Y * 2 - 5) {
            wmove(w, 1, iWidth - 3);
            if (offset_line == 0) {
                wprintz(w, c_white, "vv");
            } else if (offset_line > 0 && offset_line + (TERMY - VIEW_OFFSET_Y * 2) - 5 < max_line) {
                wprintz(w, c_white, "^v");
            } else {
                wprintz(w, c_white, "^^");
            }
        }
        draw_border(w);
        wrefresh(w);
        const int iMenuStart = iOffsetX;
        const int iMenuItems = vMenu.size() - 1;
        int iSelected = iOffsetX - 1;
        int popup_width = max_text_length + 2;
        int popup_x = 0;
        switch (position) {
        case -2:
            popup_x = 0;
            break; //near the right edge of the terminal window
        case -1:
            popup_x = iStartX - popup_width;
            break; //left before item info window
        case 0:
            popup_x = iStartX + iWidth;
            break; //right after item info window
        case 1:
            popup_x = TERMX - popup_width;
            break; //near the left edge of the terminal window
        }

        do {
            cMenu = draw_item_info(popup_x, popup_width, 0, vMenu.size() + iOffsetX * 2,
                                   "", vMenu, vDummy,
                                   iSelected >= iOffsetX && iSelected <= iMenuItems ? iSelected : -1);

            switch (cMenu) {
            case 'a':
                use_item(pos);
                break;
            case 'E':
                eat(pos);
                break;
            case 'W':
                wear(pos);
                break;
            case 'w':
                wield(pos);
                break;
            case 't':
                plthrow(pos);
                break;
            case 'T':
                takeoff(pos);
                break;
            case 'd':
                drop(pos);
                break;
            case 'U':
                unload(pos);
                break;
            case 'r':
                reload(pos);
                break;
            case 'R':
                u.read(pos);
                break;
            case 'D':
                u.disassemble(pos);
                break;
            case '=':
                reassign_item(pos);
                break;
            case KEY_UP:
                iSelected--;
                break;
            case KEY_DOWN:
                iSelected++;
                break;
            case '>':
                if (offset_line + (TERMY - VIEW_OFFSET_Y * 2) - 5 < max_line) {
                    offset_line++;
                }
                break;
            case '<':
                if (offset_line > 0) {
                    offset_line--;
                }
                break;
            case '+':
                if (!bHPR) {
                    addPickupRule(oThisItem.tname());
                    add_msg(m_info, _("'%s' added to character pickup rules."), oThisItem.tname().c_str());
                }
                break;
            case '-':
                if (bHPR) {
                    removePickupRule(oThisItem.tname());
                    add_msg(m_info, _("'%s' removed from character pickup rules."), oThisItem.tname().c_str());
                }
                break;
            default:
                break;
            }
            if (iSelected < iMenuStart - 1) { // wraparound, but can be hidden
                iSelected = iMenuItems;
            } else if (iSelected > iMenuItems + 1) {
                iSelected = iMenuStart;
            }
            werase(w);
            if (max_line > TERMY - VIEW_OFFSET_Y * 2 - 5) {
                wmove(w, 1, iWidth - 3);
                if (offset_line == 0) {
                    wprintz(w, c_white, "vv");
                } else if (offset_line > 0 && offset_line + (TERMY - VIEW_OFFSET_Y * 2) - 5 < max_line) {
                    wprintz(w, c_white, "^v");
                } else {
                    wprintz(w, c_white, "^^");
                }
            }
            wmove(w, 1, 2);
            wprintz(w, c_white, "%s", item_name.c_str());
            fold_and_print_from(w, 3, 2, iWidth - 4, offset_line, c_white, str);
            draw_border(w);
            wrefresh(w);
        } while (cMenu == KEY_DOWN || cMenu == KEY_UP || cMenu == '>' || cMenu == '<');
    }
    return cMenu;
}

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview(input_context &ctxt, std::string &action)
{
    do {
        action = ctxt.handle_input();
        if (action == "MOUSE_MOVE") {
            int mx, my;
            if (!ctxt.get_coordinates(w_terrain, mx, my)) {
                hide_mouseview();
            } else {
                liveview.show(mx, my);
            }
        }
    } while (action == "MOUSE_MOVE"); // Freeze animation when moving the mouse

    if (action != "TIMEOUT" && ctxt.get_raw_input().get_first_input() != ERR) {
        // Keyboard event, break out of animation loop
        hide_mouseview();
        return false;
    }

    // Mouse movement or un-handled key
    return true;
}

// Hides the mouse hover box and redraws what was under it
void game::hide_mouseview()
{
    if (liveview.hide()) {
        draw_sidebar(); // Redraw anything hidden by mouseview
    }
}

#ifdef SDLTILES
void rescale_tileset(int size);
#endif

input_context get_default_mode_input_context()
{
    input_context ctxt("DEFAULTMODE");
    // Because those keys move the character, they don't pan, as their original name says
    ctxt.register_action("UP", _("Move North"));
    ctxt.register_action("RIGHTUP", _("Move Northeast"));
    ctxt.register_action("RIGHT", _("Move East"));
    ctxt.register_action("RIGHTDOWN", _("Move Southeast"));
    ctxt.register_action("DOWN", _("Move South"));
    ctxt.register_action("LEFTDOWN", _("Move Southwest"));
    ctxt.register_action("LEFT", _("Move West"));
    ctxt.register_action("LEFTUP", _("Move Northwest"));
    ctxt.register_action("pause");
    ctxt.register_action("LEVEL_DOWN", _("Descend Stairs"));
    ctxt.register_action("LEVEL_UP", _("Ascend Stairs"));
    ctxt.register_action("center");
    ctxt.register_action("shift_n");
    ctxt.register_action("shift_ne");
    ctxt.register_action("shift_e");
    ctxt.register_action("shift_se");
    ctxt.register_action("shift_s");
    ctxt.register_action("shift_sw");
    ctxt.register_action("shift_w");
    ctxt.register_action("shift_nw");
    ctxt.register_action("open");
    ctxt.register_action("close");
    ctxt.register_action("smash");
    ctxt.register_action("examine");
    ctxt.register_action("advinv");
    ctxt.register_action("pickup");
    ctxt.register_action("grab");
    ctxt.register_action("butcher");
    ctxt.register_action("chat");
    ctxt.register_action("look");
    ctxt.register_action("peek");
    ctxt.register_action("listitems");
    ctxt.register_action("zones");
    ctxt.register_action("inventory");
    ctxt.register_action("compare");
    ctxt.register_action("organize");
    ctxt.register_action("apply");
    ctxt.register_action("apply_wielded");
    ctxt.register_action("wear");
    ctxt.register_action("take_off");
    ctxt.register_action("eat");
    ctxt.register_action("read");
    ctxt.register_action("wield");
    ctxt.register_action("pick_style");
    ctxt.register_action("reload");
    ctxt.register_action("unload");
    ctxt.register_action("throw");
    ctxt.register_action("fire");
    ctxt.register_action("fire_burst");
    ctxt.register_action("select_fire_mode");
    ctxt.register_action("drop");
    ctxt.register_action("drop_adj");
    ctxt.register_action("bionics");
    ctxt.register_action("mutations");
    ctxt.register_action("sort_armor");
    ctxt.register_action("wait");
    ctxt.register_action("craft");
    ctxt.register_action("recraft");
    ctxt.register_action("long_craft");
    ctxt.register_action("construct");
    ctxt.register_action("disassemble");
    ctxt.register_action("sleep");
    ctxt.register_action("control_vehicle");
    ctxt.register_action("safemode");
    ctxt.register_action("autosafe");
    ctxt.register_action("ignore_enemy");
    ctxt.register_action("save");
    ctxt.register_action("quicksave");
    ctxt.register_action("quit");
    ctxt.register_action("player_data");
    ctxt.register_action("map");
    ctxt.register_action("missions");
    ctxt.register_action("factions");
    ctxt.register_action("kills");
    ctxt.register_action("morale");
    ctxt.register_action("messages");
    ctxt.register_action("help");
    ctxt.register_action("debug");
    ctxt.register_action("debug_scent");
    ctxt.register_action("debug_mode");
    ctxt.register_action("zoom_out");
    ctxt.register_action("zoom_in");
    ctxt.register_action("toggle_sidebar_style");
    ctxt.register_action("toggle_fullscreen");
    ctxt.register_action("action_menu");
    ctxt.register_action("item_action_menu");
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("COORDINATE");
    ctxt.register_action("MOUSE_MOVE");
    ctxt.register_action("SELECT");
    ctxt.register_action("SEC_SELECT");
    return ctxt;
}

input_context game::get_player_input(std::string &action)
{
    input_context ctxt = get_default_mode_input_context();
    // register QUIT action so it catches q/Q/etc instead of just Q
    if(uquit == QUIT_WATCH) {
        ctxt.register_action("QUIT");
    }

    if (OPTIONS["ANIMATIONS"]) {
        int iStartX = (TERRAIN_WINDOW_WIDTH > 121) ? (TERRAIN_WINDOW_WIDTH - 121) / 2 : 0;
        int iStartY = (TERRAIN_WINDOW_HEIGHT > 121) ? (TERRAIN_WINDOW_HEIGHT - 121) / 2 : 0;
        int iEndX = (TERRAIN_WINDOW_WIDTH > 121) ? TERRAIN_WINDOW_WIDTH - (TERRAIN_WINDOW_WIDTH - 121) / 2 :
                    TERRAIN_WINDOW_WIDTH;
        int iEndY = (TERRAIN_WINDOW_HEIGHT > 121) ? TERRAIN_WINDOW_HEIGHT - (TERRAIN_WINDOW_HEIGHT - 121) /
                    2 : TERRAIN_WINDOW_HEIGHT;

        if (fullscreen) {
            iStartX = 0;
            iStartY = 0;
            iEndX = TERMX;
            iEndY = TERMY;
        }

        //x% of the Viewport, only shown on visible areas
        const int dropCount = int(iEndX * iEndY * mapWeatherAnim[weather].fFactor);
        const int offset_x = (u.posx() + u.view_offset_x) - getmaxx(w_terrain) / 2;
        const int offset_y = (u.posy() + u.view_offset_y) - getmaxy(w_terrain) / 2;

        const bool bWeatherEffect = (mapWeatherAnim[weather].cGlyph != '?');

        weather_printable wPrint;
        wPrint.colGlyph = mapWeatherAnim[weather].colGlyph;
        wPrint.cGlyph = mapWeatherAnim[weather].cGlyph;
        wPrint.wtype = weather;
        wPrint.vdrops.clear();
        wPrint.startx = iStartX;
        wPrint.starty = iStartY;
        wPrint.endx = iEndX;
        wPrint.endy = iEndY;

        inp_mngr.set_timeout(125);
        // Force at least one animation frame if the player is dead.
        while( handle_mouseview(ctxt, action) || uquit == QUIT_WATCH ) {
            if (bWeatherEffect && OPTIONS["ANIMATION_RAIN"]) {
                /*
                Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
                Get tile information from above's weather information:
                WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
                WEATHER_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
                WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
                */

                //Erase previous drops from w_terrain
                for( auto &elem : wPrint.vdrops ) {
                    m.drawsq( w_terrain, u, elem.first + offset_x, elem.second + offset_y, false,
                              true, u.posx() + u.view_offset_x, u.posy() + u.view_offset_y );
                }

                wPrint.vdrops.clear();

                const int light_sight_range = u.sight_range( light_level() );
                for (int i = 0; i < dropCount; i++) {
                    const int iRandX = rng(iStartX, iEndX - 1);
                    const int iRandY = rng(iStartY, iEndY - 1);
                    const int mapx = iRandX + offset_x;
                    const int mapy = iRandY + offset_y;
                    const int distance = rl_dist( u.posx(), u.posy(), mapx, mapy );

                    if( m.is_outside( mapx, mapy ) &&
                        ( m.light_at( mapx, mapy ) > LL_LOW ||
                          distance <= light_sight_range ) &&
                        m.pl_sees( mapx, mapy, distance ) &&
                        !critter_at(mapx, mapy) ) {
                        // Supress if a critter is there
                        wPrint.vdrops.push_back(std::make_pair(iRandX, iRandY));
                    }
                }
            }
            // don't bother calculating SCT if we won't show it
            if (uquit != QUIT_WATCH && OPTIONS["ANIMATION_SCT"]) {
#ifdef TILES
                if (!use_tiles) {
#endif
                    for( auto &elem : SCT.vSCT ) {
                        //Erase previous text from w_terrain
                        if( elem.getStep() > 0 ) {
                            for( size_t i = 0; i < elem.getText().length(); ++i ) {
                                if( u.sees( elem.getPosX() + i, elem.getPosY() ) ) {
                                    m.drawsq( w_terrain, u, elem.getPosX() + i, elem.getPosY(),
                                              false, true, u.posx() + u.view_offset_x,
                                              u.posy() + u.view_offset_y );
                                } else {
                                    const int iDY =
                                        POSY + ( elem.getPosY() - ( u.posy() + u.view_offset_y ) );
                                    const int iDX =
                                        POSX + ( elem.getPosX() - ( u.posx() + u.view_offset_x ) );

                                    if (u.has_effect("boomered")) {
                                        mvwputch(w_terrain, iDY, iDX + i, c_magenta, '#');

                                    } else {
                                        mvwputch(w_terrain, iDY, iDX + i, c_black, ' ');
                                    }
                                }
                            }
                        }
                    }
#ifdef TILES
                }
#endif

                SCT.advanceAllSteps();

                //Check for creatures on all drawing positions and offset if necessary
                for (auto iter = SCT.vSCT.rbegin(); iter != SCT.vSCT.rend(); ++iter) {
                    const direction oCurDir = iter->getDirecton();

                    for (int i = 0; i < (int)iter->getText().length(); ++i) {
                        const int dex = mon_at(iter->getPosX() + i, iter->getPosY());

                        if (dex != -1 && u.sees(zombie(dex))) {
                            i = -1;

                            int iPos = iter->getStep() + iter->getStepOffset();
                            for (auto iter2 = iter; iter2 != SCT.vSCT.rend(); ++iter2) {
                                if (iter2->getDirecton() == oCurDir &&
                                    iter2->getStep() + iter2->getStepOffset() <= iPos) {
                                    if (iter2->getType() == "hp") {
                                        iter2->advanceStepOffset();
                                    }

                                    iter2->advanceStepOffset();
                                    iPos = iter2->getStep() + iter2->getStepOffset();
                                }
                            }
                        }
                    }
                }
            }
            draw_weather(wPrint);
            if(uquit != QUIT_WATCH) {
                draw_sct();
            }
            if( uquit == QUIT_WATCH ) {
                // Display "press X to continue" text at top of main window
                std::string message = string_format( _("Press %s to accept your fate..."),
                        ctxt.get_desc("QUIT").c_str() );
                mvwprintz( w_terrain, 0, (TERRAIN_WINDOW_WIDTH / 2) - (message.length() / 2), c_white,
                           message.c_str() );
                wrefresh(w_terrain);
                break;
            }
            wrefresh(w_terrain);
        }
        inp_mngr.set_timeout(-1);
    } else {
        while (handle_mouseview(ctxt, action)) {};
    }

    return ctxt;
}

void game::rcdrive(int dx, int dy)
{
    std::stringstream car_location_string( u.get_value( "remote_controlling" ) );
    if( car_location_string.str() == "" ) {
        //no turned radio car found
        u.add_msg_if_player(m_warning, _("No radio car connected."));
        return;
    }
    int cx, cy, cz;
    car_location_string >> cx >> cy >> cz;

    auto rc_pairs = m.get_rc_items( cx, cy, cz );
    auto rc_pair = rc_pairs.begin();
    for( ; rc_pair != rc_pairs.end(); ++rc_pair ) {
        if( rc_pair->second->type->id == "radio_car_on" && rc_pair->second->active ) {
            break;
        }
    }
    if( rc_pair == rc_pairs.end() ) {
        u.add_msg_if_player(m_warning, _("No radio car connected."));
        u.remove_value( "remote_controlling" );
        return;
    }
    item *rc_car = rc_pair->second;

    if( m.move_cost(cx + dx, cy + dy) == 0 || !m.can_put_items(cx + dx, cy + dy) ||
        m.has_furn(cx + dx, cy + dy) ) {
        sound(cx + dx, cy + dy, 7, _("sound of a collision with an obstacle."));
        return;
    } else if( m.add_item_or_charges(cx + dx, cy + dy, *rc_car ) ) {
        //~ Sound of moving a remote controlled car
        sound(cx, cy, 6, _("zzz..."));
        u.moves -= 50;
        m.i_rem( cx, cy, rc_car );
        car_location_string.clear();
        car_location_string << cx + dx << ' ' << cy + dy << ' ' << cz;
        u.set_value( "remote_controlling", car_location_string.str() );
        return;
    }
}

vehicle *game::remoteveh()
{
    if( calendar::turn == remoteveh_cache_turn ) {
        return remoteveh_cache;
    }
    remoteveh_cache_turn = calendar::turn;
    std::stringstream remote_veh_string( u.get_value( "remote_controlling_vehicle" ) );
    if( remote_veh_string.str() == "" || 
        ( !u.has_bionic( "bio_remote" ) && !u.has_active_item( "radiocontrol" ) ) ) {
        remoteveh_cache = nullptr;
    } else {
        int vx, vy;
        remote_veh_string >> vx >> vy;
        vehicle *veh = m.veh_at( vx, vy );
        if( veh->fuel_left( "battery", true ) > 0 ) {
            remoteveh_cache = veh;
        } else {
            remoteveh_cache = nullptr;
        }
    }
    return remoteveh_cache;
}

void game::setremoteveh(vehicle *veh)
{
    remoteveh_cache_turn = calendar::turn;
    remoteveh_cache = veh;
    if( veh == nullptr || ( !u.has_active_bionic( "bio_remote" ) && !u.has_active_item( "radiocontrol" ) ) ) {
        u.remove_value( "remote_controlling_vehicle" );
        return;
    }

    std::stringstream remote_veh_string;
    remote_veh_string << veh->global_x() << ' ' << veh->global_y();
    u.set_value( "remote_controlling_vehicle", remote_veh_string.str() );
}

void use_item_menu(player &p);
bool game::handle_action()
{
    std::string action;
    input_context ctxt;
    action_id act = ACTION_NULL;
    // Check if we have an auto-move destination
    if (u.has_destination()) {
        act = u.get_next_auto_move_direction();
        if (act == ACTION_NULL) {
            add_msg(m_info, _("Auto-move cancelled"));
            u.clear_destination();
            return false;
        }
    } else {
        // No auto-move, ask player for input
        ctxt = get_player_input(action);
    }

    int veh_part;
    vehicle *veh = m.veh_at( u.posx(), u.posy(), veh_part );
    bool veh_ctrl = !u.is_dead_state() &&
        ( ( veh && veh->player_in_control(&u) ) || remoteveh() != nullptr );

    // If performing an action with right mouse button, co-ordinates
    // of location clicked.
    int mouse_action_x = -1;
    int mouse_action_y = -1;

    // do not allow mouse actions while dead
    if(!u.is_dead_state()) {
        if (act == ACTION_NULL) {
            if (action == "SELECT" || action == "SEC_SELECT") {
                // Mouse button click
                if (veh_ctrl) {
                    // No mouse use in vehicle
                    return false;
                }

                int mx, my;
                if (!ctxt.get_coordinates(w_terrain, mx, my) || !u.sees(mx, my)) {
                    // Not clicked in visible terrain
                    return false;
                }

                if (action == "SELECT") {
                    bool new_destination = true;
                    if (!destination_preview.empty()) {
                        point final_destination = destination_preview.back();
                        if (final_destination.x == mx && final_destination.y == my) {
                            // Second click
                            new_destination = false;
                            u.set_destination(destination_preview);
                            destination_preview.clear();
                            act = u.get_next_auto_move_direction();
                            if (act == ACTION_NULL) {
                                // Something went wrong
                                u.clear_destination();
                                return false;
                            }
                        }
                    }

                    if (new_destination) {
                        destination_preview = m.route(u.posx(), u.posy(), mx, my, false);
                        return false;
                    }
                } else if (action == "SEC_SELECT") {
                    // Right mouse button

                    bool had_destination_to_clear = !destination_preview.empty();
                    u.clear_destination();
                    destination_preview.clear();

                    if (had_destination_to_clear) {
                        return false;
                    }

                    mouse_action_x = mx;
                    mouse_action_y = my;
                    int mouse_selected_mondex = mon_at(mx, my);
                    if (mouse_selected_mondex != -1) {
                        monster &critter = critter_tracker.find(mouse_selected_mondex);
                        if (!u.sees(critter)) {
                            add_msg(_("Nothing relevant here."));
                            return false;
                        }

                        if (!u.weapon.is_gun()) {
                            add_msg(m_info, _("You are not wielding a ranged weapon."));
                            return false;
                        }

                        //TODO: Add weapon range check. This requires weapon to be reloaded.

                        act = ACTION_FIRE;
                    } else if (std::abs(mx - u.posx()) <= 1 && std::abs(my - u.posy()) <= 1 &&
                               m.close_door(mx, my, !m.is_outside(u.posx(), u.posy()), true)) {
                        // Can only close doors when adjacent to it.
                        act = ACTION_CLOSE;
                    } else {
                        int dx = abs(u.posx() - mx);
                        int dy = abs(u.posy() - my);
                        if (dx < 2 && dy < 2) {
                            if (dy == 0 && dx == 0) {
                                // Clicked on self
                                act = ACTION_PICKUP;
                            } else {
                                // Clicked adjacent tile
                                act = ACTION_EXAMINE;
                            }
                        } else {
                            add_msg(_("Nothing relevant here."));
                            return false;
                        }
                    }
                }
            }
        }
    }

    if( act == ACTION_NULL ) {
        // No auto-move action, no mouse clicks.
        u.clear_destination();
        destination_preview.clear();

        act = look_up_action(action);
        if( act == ACTION_NULL ) {
            add_msg(m_info, _("Unknown command: '%c'"), (int)ctxt.get_raw_input().get_first_input());
        }
    }

    if( act == ACTION_ACTIONMENU ) {
        act = handle_action_menu();
        if (act == ACTION_NULL) {
            return false;
        }
    }

    // This has no action unless we're in a special game mode.
    gamemode->pre_action(act);

    int soffset = (int)OPTIONS["MOVE_VIEW_OFFSET"];
    int soffsetr = 0 - soffset;

    int before_action_moves = u.moves;

    // Use to track if auto-move should be cancelled due to a failed
    // move or obstacle
    bool continue_auto_move = false;

    // quit prompt check (ACTION_QUIT only grabs 'Q')
    if(uquit == QUIT_WATCH && action == "QUIT") {
        uquit = QUIT_DIED;
        return false;
    }

    // I have split the switch(act) into two, so one can be done
    // for the deathcam as well. It will still be run if you are
    // alive, so no worries there. KA101 suggested (quite aptly)
    // that the user should be able to look around, so this
    // allows that. -Davek
    if( uquit == QUIT_WATCH || !u.is_dead_state() ) {
        switch(act) {
        case ACTION_CENTER:
            u.view_offset_x = driving_view_offset.x;
            u.view_offset_y = driving_view_offset.y;
            break;

        case ACTION_SHIFT_N:
            u.view_offset_y += soffsetr;
            break;

        case ACTION_SHIFT_NE:
            u.view_offset_x += soffset;
            u.view_offset_y += soffsetr;
            break;

        case ACTION_SHIFT_E:
            u.view_offset_x += soffset;
            break;

        case ACTION_SHIFT_SE:
            u.view_offset_x += soffset;
            u.view_offset_y += soffset;
            break;

        case ACTION_SHIFT_S:
            u.view_offset_y += soffset;
            break;

        case ACTION_SHIFT_SW:
            u.view_offset_x += soffsetr;
            u.view_offset_y += soffset;
            break;

        case ACTION_SHIFT_W:
            u.view_offset_x += soffsetr;
            break;

        case ACTION_SHIFT_NW:
            u.view_offset_x += soffsetr;
            u.view_offset_y += soffsetr;
            break;

        case ACTION_LOOK:
            look_around();
            break;

        default:
            break;
        }
    }

    // actions allowed only while alive
    if(!u.is_dead_state()) {
        switch (act) {
        case ACTION_NULL:
        case NUM_ACTIONS:
            break; // dummy entries
        case ACTION_ACTIONMENU:
            break; // handled above

        case ACTION_PAUSE:
            if( check_save_mode_allowed() ) {
                u.pause();
            }
            break;

        case ACTION_MOVE_N:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(0, -1);
            } else if (veh_ctrl) {
                pldrive(0, -1);
            } else {
                continue_auto_move = plmove(0, -1);
            }
            break;

        case ACTION_MOVE_NE:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(1, -1);
            } else if (veh_ctrl) {
                pldrive(1, -1);
            } else {
                continue_auto_move = plmove(1, -1);
            }
            break;

        case ACTION_MOVE_E:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(1, 0);
            } else if (veh_ctrl) {
                pldrive(1, 0);
            } else {
                continue_auto_move = plmove(1, 0);
            }
            break;

        case ACTION_MOVE_SE:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(1, 1);
            } else if (veh_ctrl) {
                pldrive(1, 1);
            } else {
                continue_auto_move = plmove(1, 1);
            }
            break;

        case ACTION_MOVE_S:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(0, 1);
            } else if (veh_ctrl) {
                pldrive(0, 1);
            } else {
                continue_auto_move = plmove(0, 1);
            }
            break;

        case ACTION_MOVE_SW:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(-1, 1);
            } else if (veh_ctrl) {
                pldrive(-1, 1);
            } else {
                continue_auto_move = plmove(-1, 1);
            }
            break;

        case ACTION_MOVE_W:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(-1, 0);
            } else if (veh_ctrl) {
                pldrive(-1, 0);
            } else {
                continue_auto_move = plmove(-1, 0);
            }
            break;

        case ACTION_MOVE_NW:
            moveCount++;

            if( u.get_value( "remote_controlling" ) != "" ) {
                rcdrive(-1, -1);
            } else if (veh_ctrl) {
                pldrive(-1, -1);
            } else {
                continue_auto_move = plmove(-1, -1);
            }
            break;

        case ACTION_MOVE_DOWN:
            if (!u.in_vehicle) {
                vertical_move(-1, false);
            }
            break;

        case ACTION_MOVE_UP:
            if (!u.in_vehicle) {
                vertical_move(1, false);
            }
            break;

        case ACTION_OPEN:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't open things while you're in your shell."));
            } else {
                open();
            }
            break;

        case ACTION_CLOSE:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't close things while you're in your shell."));
            } else {
                close(mouse_action_x, mouse_action_y);
            }
            break;

        case ACTION_SMASH:
            if (veh_ctrl) {
                handbrake();
            } else if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't smash things while you're in your shell."));
            } else {
                smash();
            }
            break;

        case ACTION_EXAMINE:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't examine your surroundings while you're in your shell."));
            } else {
                examine(mouse_action_x, mouse_action_y);
            }
            break;

        case ACTION_ADVANCEDINV:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't move mass quantities while you're in your shell."));
            } else {
                advanced_inv();
            }
            break;

        case ACTION_PICKUP:
            Pickup::pick_up(u.posx(), u.posy(), 1);
            break;

        case ACTION_GRAB:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't grab things while you're in your shell."));
            } else {
                grab();
            }
            break;

        case ACTION_BUTCHER:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't butcher while you're in your shell."));
            } else {
                butcher();
            }
            break;

        case ACTION_CHAT:
            chat();
            break;

        case ACTION_PEEK:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't peek around corners while you're in your shell."));
            } else {
                peek();
            }
            break;

        case ACTION_LIST_ITEMS: {
            int iRetItems = -1;
            int iRetMonsters = -1;
            int startas = uistate.list_item_mon;
            temp_exit_fullscreen();
            do {
                if (startas != 2) { // last mode 2 = list_monster
                    startas = 0;      // but only for the first bit of the loop
                    iRetItems = list_items(iRetMonsters);
                } else {
                    iRetItems = -2;   // so we'll try list_items if list_monsters found 0
                }
                if (iRetItems != -1 || startas == 2) {
                    startas = 0;
                    iRetMonsters = list_monsters(iRetItems);
                    if (iRetMonsters == 2) {
                        iRetItems = -1; // will fire, exit loop
                    } else if (iRetMonsters == -1 && iRetItems == -2) {
                        iRetItems = -1; // exit if requested on list_monsters firstrun
                    }
                }
            } while (iRetItems != -1 && iRetMonsters != -1 && !(iRetItems == 0 && iRetMonsters == 0));

            if (iRetItems == 0 && iRetMonsters == 0) {
                add_msg(m_info, _("You don't see any items or monsters around you!"));
            } else if (iRetMonsters == 2) {
                refresh_all();
                plfire(false);
            }
            refresh_all();
            reenter_fullscreen();
        }
        break;

        case ACTION_ZONES:
            zones_manager();
            break;

        case ACTION_INVENTORY: {
            int cMenu = ' ';
            int position = INT_MIN;
            do {
                position = inv(_("Inventory:"), position);
                cMenu = inventory_item_menu(position);
            } while (cMenu == ' ' || cMenu == '.' || cMenu == 'q' || cMenu == '\n' ||
                     cMenu == KEY_ESCAPE || cMenu == KEY_LEFT || cMenu == '=');
            refresh_all();
        }
        break;

        case ACTION_COMPARE:
            compare();
            break;

        case ACTION_ORGANIZE:
            reassign_item();
            break;

        case ACTION_USE:
            // Shell-users are presumed to be able to mess with their inventories, etc
            // while in the shell.  Eating, gear-changing, and item use are OK.
            use_item();
            break;

        case ACTION_USE_WIELDED:
            use_wielded_item();
            break;

        case ACTION_WEAR:
            wear();
            break;

        case ACTION_TAKE_OFF:
            takeoff();
            break;

        case ACTION_EAT:
            eat();
            break;

        case ACTION_READ:
            // Shell-users are presumed to have the book just at an opening and read it that way
            read();
            break;

        case ACTION_WIELD:
            wield();
            break;

        case ACTION_PICK_STYLE:
            u.pick_style();
            break;

        case ACTION_RELOAD:
            reload();
            break;

        case ACTION_UNLOAD:
            unload(u.weapon);
            break;

        case ACTION_THROW:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't effectively throw while you're in your shell."));
            } else {
                plthrow();
            }
            break;

        case ACTION_FIRE:
            // Shell-users may fire a *single-handed* weapon out a port, if need be.
            plfire(false, mouse_action_x, mouse_action_y);
            break;

        case ACTION_FIRE_BURST:
            plfire(true, mouse_action_x, mouse_action_y);
            break;

        case ACTION_SELECT_FIRE_MODE:
            u.weapon.next_mode();
            break;

        case ACTION_DROP:
            // You CAN drop things to your own tile while in the shell.
            drop();
            break;

        case ACTION_DIR_DROP:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't drop things to another tile while you're in your shell."));
            } else {
                drop_in_direction();
            }
            break;
        case ACTION_BIONICS:
            u.power_bionics();
            refresh_all();
            break;
        case ACTION_MUTATIONS:
            u.power_mutations();
            refresh_all();
            break;

        case ACTION_SORT_ARMOR:
            u.sort_armor();
            refresh_all();
            break;

        case ACTION_WAIT:
            wait();
            break;

        case ACTION_CRAFT:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't craft while you're in your shell."));
            } else {
                u.craft();
            }
            break;

        case ACTION_RECRAFT:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't craft while you're in your shell."));
            } else {
                u.recraft();
            }
            break;

        case ACTION_LONGCRAFT:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't craft while you're in your shell."));
            } else {
                u.long_craft();
            }
            break;

        case ACTION_DISASSEMBLE:
            if (u.controlling_vehicle) {
                add_msg(m_info, _("You can't disassemble items while driving."));
            } else {
                u.disassemble();
                refresh_all();
            }
            break;

        case ACTION_CONSTRUCT:
            if (u.in_vehicle) {
                add_msg(m_info, _("You can't construct while in a vehicle."));
            } else if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't construct while you're in your shell."));
            } else {
                construction_menu();
            }
            break;

        case ACTION_SLEEP:
            if (veh_ctrl) {
                add_msg(m_info, _("Vehicle control has moved, %s"),
                        press_x(ACTION_CONTROL_VEHICLE, _("new binding is "),
                                _("new default binding is '^'.")).c_str());
            } else {
                uimenu as_m;
                as_m.text = _("Are you sure you want to sleep?");
                as_m.entries.push_back(uimenu_entry(0, true,
                                                    (OPTIONS["FORCE_CAPITAL_YN"] ? 'Y' : 'y'),
                                                    _("Yes.")));

                if (OPTIONS["SAVE_SLEEP"]) {
                    as_m.entries.push_back(uimenu_entry(1, (moves_since_last_save),
                                                        (OPTIONS["FORCE_CAPITAL_YN"] ? 'S' : 's'),
                                                        _("Yes, and save game before sleeping.")));
                }
                as_m.entries.push_back(uimenu_entry(2, true, (OPTIONS["FORCE_CAPITAL_YN"] ?
                                                    'N' : 'n'), _("No.")));

                if (u.has_item_with_flag("ALARMCLOCK") && (u.hunger < -60)) {
                    as_m.text =
                        _("You're engorged to hibernate. The alarm would only attract attention. Enter hibernation?");
                }
                if( (u.has_item_with_flag("ALARMCLOCK") || u.has_bionic("bio_watch")) &&
                    !(u.hunger < -60) ) {
                    as_m.entries.push_back(uimenu_entry(3, true, '3',
                                                        _("Set alarm to wake up in 3 hours.")));
                    as_m.entries.push_back(uimenu_entry(4, true, '4',
                                                        _("Set alarm to wake up in 4 hours.")));
                    as_m.entries.push_back(uimenu_entry(5, true, '5',
                                                        _("Set alarm to wake up in 5 hours.")));
                    as_m.entries.push_back(uimenu_entry(6, true, '6',
                                                        _("Set alarm to wake up in 6 hours.")));
                    as_m.entries.push_back(uimenu_entry(7, true, '7',
                                                        _("Set alarm to wake up in 7 hours.")));
                    as_m.entries.push_back(uimenu_entry(8, true, '8',
                                                        _("Set alarm to wake up in 8 hours.")));
                    as_m.entries.push_back(uimenu_entry(9, true, '9',
                                                        _("Set alarm to wake up in 9 hours.")));
                }
                /* Calculate key and window variables, generate window,
                   and loop until we get a valid answer. */
                as_m.query();

                bool bSleep = false;
                if (as_m.ret == 0) {
                    bSleep = true;
                } else if (as_m.ret == 1) {
                    quicksave();
                    bSleep = true;
                } else if (as_m.ret >= 3 && as_m.ret <= 9) {
                    u.add_effect("alarm_clock", 600 * as_m.ret);
                    bSleep = true;
                }

                if (bSleep) {
                    u.moves = 0;
                    u.try_to_sleep();
                }
            }
            break;

        case ACTION_CONTROL_VEHICLE:
            if (u.has_active_mutation("SHELL2")) {
                add_msg(m_info, _("You can't operate a vehicle while you're in your shell."));
            } else {
                control_vehicle();
            }
            break;

        case ACTION_TOGGLE_SAFEMODE:
            if (safe_mode == SAFE_MODE_OFF ) {
                safe_mode = SAFE_MODE_ON;
                mostseen = 0;
                add_msg(m_info, _("Safe mode ON!"));
            } else {
                turnssincelastmon = 0;
                safe_mode = SAFE_MODE_OFF;
                if (autosafemode) {
                    add_msg(m_info, _("Safe mode OFF! (Auto safe mode still enabled!)"));
                } else {
                    add_msg(m_info, _("Safe mode OFF!"));
                }
            }
            break;

        case ACTION_TOGGLE_AUTOSAFE:
            if (autosafemode) {
                add_msg(m_info, _("Auto safe mode OFF!"));
                autosafemode = false;
            } else {
                add_msg(m_info, _("Auto safe mode ON"));
                autosafemode = true;
            }
            break;

        case ACTION_IGNORE_ENEMY:
            if (safe_mode == SAFE_MODE_STOP) {
                add_msg(m_info, _("Ignoring enemy!"));
                for( auto &elem : new_seen_mon ) {
                    monster &critter = critter_tracker.find( elem );
                    critter.ignoring = rl_dist( u.pos(), critter.pos() );
                }
                safe_mode = SAFE_MODE_ON;
            }
            break;

        case ACTION_QUIT:
            if (query_yn(_("Commit suicide?"))) {
                if (query_yn(_("REALLY commit suicide?"))) {
                    u.moves = 0;
                    u.place_corpse();
                    uquit = QUIT_SUICIDE;
                }
            }
            break;

        case ACTION_SAVE:
            if (query_yn(_("Save and quit?"))) {
                if(save()) {
                    u.moves = 0;
                    uquit = QUIT_SAVED;
                }
            }
            break;

        case ACTION_QUICKSAVE:
            quicksave();
            return false;

        case ACTION_PL_INFO:
            u.disp_info();
            refresh_all();
            break;

        case ACTION_MAP:
            draw_overmap();
            break;

        case ACTION_MISSIONS:
            list_missions();
            break;

        case ACTION_KILLS:
            disp_kills();
            break;

        case ACTION_FACTIONS:
            list_factions(_("FACTIONS:"));
            break;

        case ACTION_MORALE:
            u.disp_morale();
            refresh_all();
            break;

        case ACTION_MESSAGES:
            Messages::display_messages();
            refresh_all();
            break;

        case ACTION_HELP:
            display_help();
            refresh_all();
            break;

        case ACTION_DEBUG:
            if (MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger()) {
                break;    //don't do anything when sharing and not debugger
            }
            debug();
            refresh_all();
            break;

        case ACTION_TOGGLE_SIDEBAR_STYLE:
            toggle_sidebar_style();
            break;

        case ACTION_TOGGLE_FULLSCREEN:
            toggle_fullscreen();
            break;

        case ACTION_DISPLAY_SCENT:
            if (MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger()) {
                break;    //don't do anything when sharing and not debugger
            }
            display_scent();
            break;

        case ACTION_TOGGLE_DEBUG_MODE:
            if (MAP_SHARING::isCompetitive() && !MAP_SHARING::isDebugger()) {
                break;    //don't do anything when sharing and not debugger
            }
            debug_mode = !debug_mode;
            if( debug_mode ) {
                add_msg( m_info, _("Debug mode ON!") );
            } else {
                add_msg( m_info, _("Debug mode OFF!") );
            }
            break;

        case ACTION_ZOOM_IN:
            zoom_in();
            break;

        case ACTION_ZOOM_OUT:
            zoom_out();
            break;

        case ACTION_ITEMACTION:
            item_action_menu();
            break;
        default:
            break;
        }
    }

    if (!continue_auto_move) {
        u.clear_destination();
    }

    gamemode->post_action(act);

    u.movecounter = (!u.is_dead_state() ? (before_action_moves - u.moves) : 0);
    dbg(D_INFO) << string_format("%s: [%d] %d - %d = %d", action_ident(act).c_str(),
                                 int(calendar::turn), before_action_moves, u.movecounter, u.moves);
    return (!u.is_dead_state());
}

#define SCENT_RADIUS 40

int &game::scent(int x, int y)
{
    if (x < (SEEX * MAPSIZE / 2) - SCENT_RADIUS || x >= (SEEX * MAPSIZE / 2) + SCENT_RADIUS ||
        y < (SEEY * MAPSIZE / 2) - SCENT_RADIUS || y >= (SEEY * MAPSIZE / 2) + SCENT_RADIUS) {
        nulscent = 0;
        return nulscent; // Out-of-bounds - null scent
    }
    return grscent[x][y];
}

void game::update_scent()
{
    static point player_last_position = point(u.posx(), u.posy());
    static int player_last_moved = calendar::turn;
    // Stop updating scent after X turns of the player not moving.
    // Once wind is added, need to reset this on wind shifts as well.
    if (u.posx() == player_last_position.x && u.posy() == player_last_position.y) {
        if (player_last_moved + 1000 < calendar::turn) {
            return;
        }
    } else {
        player_last_position = point(u.posx(), u.posy());
        player_last_moved = calendar::turn;
    }

    // note: the next four intermediate matrices need to be at least
    // [2*SCENT_RADIUS+3][2*SCENT_RADIUS+1] in size to hold enough data
    // The code I'm modifying used [SEEX * MAPSIZE]. I'm staying with that to avoid new bugs.

    // These two matrices are transposed so that x addresses are contiguous in memory
    int sum_3_scent_y[SEEY * MAPSIZE][SEEX * MAPSIZE];  //intermediate variable
    int squares_used_y[SEEY * MAPSIZE][SEEX * MAPSIZE]; //intermediate variable

    // these are for caching flag lookups
    bool blocks_scent[SEEX * MAPSIZE][SEEY * MAPSIZE]; // currently only TFLAG_WALL blocks scent
    bool reduces_scent[SEEX * MAPSIZE][SEEY * MAPSIZE];


    // for loop constants
    const int scentmap_minx = u.posx() - SCENT_RADIUS;
    const int scentmap_maxx = u.posx() + SCENT_RADIUS;
    const int scentmap_miny = u.posy() - SCENT_RADIUS;
    const int scentmap_maxy = u.posy() + SCENT_RADIUS;

    const int diffusivity = 100; // decrease this to reduce gas spread. Keep it under 125 for
    // stability. This is essentially a decimal number * 1000.

    if (!u.has_active_bionic("bio_scent_mask")) {
        grscent[u.posx()][u.posy()] = u.scent;
    }

    // Sum neighbors in the y direction.  This way, each square gets called 3 times instead of 9
    // times. This cost us an extra loop here, but it also eliminated a loop at the end, so there
    // is a net performance improvement over the old code. Could probably still be better.
    // note: this method needs an array that is one square larger on each side in the x direction
    // than the final scent matrix. I think this is fine since SCENT_RADIUS is less than
    // SEEX*MAPSIZE, but if that changes, this may need tweaking.
    for (int x = scentmap_minx - 1; x <= scentmap_maxx + 1; ++x) {
        for (int y = scentmap_miny; y <= scentmap_maxy; ++y) {
            // cache expensive flag checks, once per tile.
            if (y == scentmap_miny) {  // Setting y-1 y-0, when we are at the top row...
                for (int i = y - 1; i <= y; ++i) {
                    blocks_scent[x][i] = m.has_flag(TFLAG_WALL, x, i);
                    reduces_scent[x][i] = m.has_flag(TFLAG_REDUCE_SCENT, x, i);
                }
            }
            blocks_scent[x][y + 1] = m.has_flag(TFLAG_WALL, x, y + 1); // ...so only y+1 here.
            reduces_scent[x][y + 1] = m.has_flag(TFLAG_REDUCE_SCENT, x, y + 1);

            // remember the sum of the scent val for the 3 neighboring squares that can defuse into
            sum_3_scent_y[y][x] = 0;
            squares_used_y[y][x] = 0;
            for (int i = y - 1; i <= y + 1; ++i) {
                if (! blocks_scent[x][i]) {
                    if (reduces_scent[x][i]) {
                        // only 20% of scent can diffuse on REDUCE_SCENT squares
                        sum_3_scent_y[y][x] += 2 * grscent[x][i];
                        squares_used_y[y][x] += 2;
                    } else {
                        sum_3_scent_y[y][x] += 10 * grscent[x][i];
                        squares_used_y[y][x] += 10;
                    }
                }
            }
        }
    }
    for (int x = scentmap_minx; x <= scentmap_maxx; ++x) {
        for (int y = scentmap_miny; y <= scentmap_maxy; ++y) {
            if (! blocks_scent[x][y]) {
                // to how many neighboring squares do we diffuse out? (include our own square
                // since we also include our own square when diffusing in)
                int squares_used = squares_used_y[y][x - 1]
                                   + squares_used_y[y][x]
                                   + squares_used_y[y][x + 1];

                int this_diffusivity;
                if (! reduces_scent[x][y]) {
                    this_diffusivity = diffusivity;
                } else {
                    this_diffusivity = diffusivity / 5; //less air movement for REDUCE_SCENT square
                }
                int temp_scent;
                // take the old scent and subtract what diffuses out
                temp_scent = grscent[x][y] * (10 * 1000 - squares_used * this_diffusivity);
                // neighboring walls and reduce_scent squares absorb some scent
                temp_scent -= grscent[x][y] * this_diffusivity * (90 - squares_used) / 5;
                // we've already summed neighboring scent values in the y direction in the previous
                // loop. Now we do it for the x direction, multiply by diffusion, and this is what
                // diffuses into our current square.
                grscent[x][y] =
                    (temp_scent
                     + this_diffusivity * (sum_3_scent_y[y][x - 1]
                                           + sum_3_scent_y[y][x]
                                           + sum_3_scent_y[y][x + 1])
                    ) / (1000 * 10);


                const int fslime = m.get_field_strength(point(x, y), fd_slime) * 10;
                if (fslime > 0 && grscent[x][y] < fslime) {
                    grscent[x][y] = fslime;
                }
                if (grscent[x][y] > 10000) {
                    dbg(D_ERROR) << "game:update_scent: Wacky scent at " << x << ","
                                 << y << " (" << grscent[x][y] << ")";
                    debugmsg("Wacky scent at %d, %d (%d)", x, y, grscent[x][y]);
                    grscent[x][y] = 0; // Scent should never be higher
                }
            } else { // this cell blocks scent
                grscent[x][y] = 0;
            }
        }
    }
}

bool game::is_game_over()
{
    if (uquit == QUIT_WATCH) {
        // deny player movement and dodging
        u.moves = 0;
        // prevent pain from updating
        u.pain = 0;
        // prevent dodging
        u.dodges_left = 0;
        return false;
    }
    if (uquit == QUIT_DIED) {
        if (u.in_vehicle) {
            m.unboard_vehicle(u.posx(), u.posy());
        }
        u.place_corpse();
        return true;
    }
    if (uquit == QUIT_SUICIDE) {
        if (u.in_vehicle) {
            m.unboard_vehicle(u.posx(), u.posy());
        }
        return true;
    }
    if (uquit != QUIT_NO) {
        return true;
    }
    // is_dead_state() already checks hp_torso && hp_head, no need to for loop it
    if(u.is_dead_state()) {
        if(OPTIONS["DEATHCAM"] == "always") {
            uquit = QUIT_WATCH;
        } else if(OPTIONS["DEATHCAM"] == "ask") {
            uquit = query_yn("Watch the last moments of your life...?") ? QUIT_WATCH : QUIT_DIED;
        } else if(OPTIONS["DEATHCAM"] == "never") {
            uquit = QUIT_DIED;
        } else {
            // Something funky happened here, just die.
            dbg(D_ERROR) << "no deathcam option given to OPTIONS[], defaulting to QUIT_DIED";
            uquit = QUIT_DIED;
        }
        return is_game_over();
    }
    return false;
}

void game::death_screen()
{
    gamemode->game_over();
    Messages::display_messages();
    disp_kills();
}

void game::move_save_to_graveyard()
{
    auto const &save_dir      = world_generator->active_world->world_path;
    auto const &graveyard_dir = FILENAMES["graveyarddir"];
    auto const &prefix        = base64_encode(u.name) + ".";
    
    if (!assure_dir_exist(graveyard_dir)) {
        debugmsg("could not create graveyard path '%s'", graveyard_dir.c_str());
    }

    auto const save_files = get_files_from_path(prefix, save_dir);
    if (save_files.empty()) {
        debugmsg("could not find save files in '%s'", save_dir.c_str());
    }

    for (auto const &src_path : save_files) {
        auto const dst_path = graveyard_dir +
            src_path.substr(src_path.rfind('/'), std::string::npos);

        if (rename_file(src_path, dst_path)) {
            continue;
        }

        debugmsg("could not rename file '%s' to '%s'", src_path.c_str(), dst_path.c_str());

        if (remove_file(src_path)) {
            continue;
        }

        debugmsg("could not remove file '%s'", src_path.c_str());
    }
}

bool game::load_master(std::string worldname)
{
    std::ifstream fin;
    std::stringstream datafile;
    datafile << world_generator->all_worlds[worldname]->world_path << "/master.gsav";
    fin.open(datafile.str().c_str(), std::ifstream::in | std::ifstream::binary);
    if (!fin.is_open()) {
        return false;
    }

    unserialize_master(fin);
    fin.close();
    return true;
}

void game::load_uistate(std::string worldname)
{
    std::stringstream savefile;
    savefile << world_generator->all_worlds[worldname]->world_path << "/uistate.json";

    std::ifstream fin;
    fin.open(savefile.str().c_str(), std::ifstream::in | std::ifstream::binary);
    if (!fin.good()) {
        fin.close();
        return;
    }
    try {
        JsonIn jsin(fin);
        uistate.deserialize(jsin);
    } catch (std::string e) {
        dbg(D_ERROR) << "load_uistate: " << e;
    }
    fin.close();
}

void game::load(std::string worldname, std::string name)
{
    std::ifstream fin;
    std::string worldpath = world_generator->all_worlds[worldname]->world_path;
    worldpath += "/";
    std::stringstream playerfile;
    playerfile << worldpath << name << ".sav";
    fin.open(playerfile.str().c_str(), std::ifstream::in | std::ifstream::binary);
    // First, read in basic game state information.
    if (!fin.is_open()) {
        dbg(D_ERROR) << "game:load: No save game exists!";
        debugmsg("No save game exists!");
        return;
    }
    // Now load up the master game data; factions (and more?)
    load_master(worldname);
    u = player();
    u.name = base64_decode(name);
    u.ret_null = item("null", 0);
    u.weapon = item("null", 0);
    unserialize(fin);
    fin.close();

    // Stair handling.
    if (!coming_to_stairs.empty()) {
        monstairx = -1;
        monstairy = -1;
        monstairz = 999;
    }

    // weather
    std::string wfile = std::string(worldpath + base64_encode(u.name) + ".weather");
    fin.open(wfile.c_str());
    if (fin.is_open()) {
        weather_log.clear();
        load_weather(fin);
    }
    fin.close();
    nextweather = int(calendar::turn);
    // log
    std::string mfile = std::string(worldpath + base64_encode(u.name) + ".log");
    fin.open(mfile.c_str());
    if (fin.is_open()) {
        u.load_memorial_file(fin);
    }
    fin.close();
    // Now that the player's worn items are updated, their sight limits need to be
    // recalculated. (This would be cleaner if u.worn were private.)
    u.recalc_sight_limits();

    if (gamemode == NULL) {
        gamemode = new special_game();
    }

    load_auto_pickup(true); // Load character auto pickup rules
    u.load_zones(); // Load character world zones
    load_uistate(worldname);

    update_map(&u);

    // legacy, needs to be here as we access the map.
    if( u.getID() == 0 || u.getID() == -1 ) {
        // player does not have a real id, so assign a new one,
        u.setID( assign_npc_id() );
        // The vehicle stores the IDs of the boarded players, so update it, too.
        if( u.in_vehicle ) {
            int vpart;
            vehicle *veh = m.veh_at( u.posx(), u.posy(), vpart );
            if( veh != nullptr ) {
                vpart = veh->part_with_feature( vpart, "BOARDABLE" );
                if( vpart >= 0 ) {
                    veh->parts[vpart].passenger_id = u.getID();
                }
            }
        }
    }

    u.reset();
    draw();
}

void game::load_world_modfiles(WORLDPTR world)
{
    popup_nowait(_("Please wait while the world data loads"));
    load_core_data();
    if (world != NULL) {
        load_artifacts(world->world_path + "/artifacts.gsav");
        mod_manager *mm = world_generator->get_mod_manager();
        // this code does not care about mod dependencies,
        // it assumes that those dependencies are static and
        // are resolved during the creation of the world.
        // That means world->active_mod_order contains a list
        // of mods in the correct order.
        for( const auto &mod_ident : world->active_mod_order ) {
            if (mm->has_mod(mod_ident)) {
                MOD_INFORMATION *mod = mm->mod_map[mod_ident];
                if( !mod->obsolete ) {
                    // Silently ignore mods marked as obsolete.
                    load_data_from_dir(mod->path);
                }
            } else {
                debugmsg("the world uses an unknown mod %s", mod_ident.c_str());
            }
        }
        // Load additional mods from that world-specific folder
        load_data_from_dir(world->world_path + "/mods");
    }
    DynamicDataLoader::get_instance().finalize_loaded_data();
}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    std::string masterfile = world_generator->active_world->world_path + "/master.gsav";
    try {
        std::ofstream fout;
        fout.exceptions(std::ios::badbit | std::ios::failbit);

        fopen_exclusive(fout, masterfile.c_str());
        if (!fout.is_open()) {
            return true; //trick code into thinking that everything went okay
        }

        serialize_master(fout);
        fclose_exclusive(fout, masterfile.c_str());
        return true;
    } catch (std::ios::failure &) {
        popup(_("Failed to save factions to %s"), masterfile.c_str());
        return false;
    }
}

bool game::save_artifacts()
{
    std::string artfilename = world_generator->active_world->world_path + "/artifacts.gsav";
    return ::save_artifacts( artfilename );
}

bool game::save_maps()
{
    try {
        m.save();
        overmap_buffer.save(); // can throw std::ios::failure
        MAPBUFFER.save(); // can throw std::ios::failure
        return true;
    } catch (std::ios::failure &) {
        popup(_("Failed to save the maps"));
        return false;
    }
}

bool game::save_uistate()
{
    std::string savefile = world_generator->active_world->world_path + "/uistate.json";
    try {
        std::ofstream fout;
        fout.exceptions(std::ios::badbit | std::ios::failbit);

        fopen_exclusive(fout, savefile.c_str());
        if (!fout.is_open()) {
            return true; //trick game into thinking it was saved
        }

        fout << uistate.serialize();
        fclose_exclusive(fout, savefile.c_str());
        return true;
    } catch (std::ios::failure &) {
        popup(_("Failed to save uistate to %s"), savefile.c_str());
        return false;
    }
}

bool game::save_player_data()
{
    const std::string playerfile = world_generator->active_world->world_path + "/" + base64_encode(u.name);
    try {
        std::ofstream fout;
        fout.exceptions(std::ios::failbit | std::ios::badbit);

        fout.open(std::string(playerfile + ".sav").c_str());
        serialize(fout);
        fout.close();
        // weather
        fout.open(std::string(playerfile + ".weather").c_str());
        save_weather(fout);
        fout.close();
        // log
        fout.open(std::string(playerfile + ".log").c_str());
        fout << u.dump_memorial();
        fout.close();
        return true;
    } catch (std::ios::failure &err) {
        popup(_("Failed to save player data"));
        return false;
    }
}

bool game::save()
{
    try {
        if( !save_player_data() ) {
            return false;
        }
        if (!save_factions_missions_npcs()) {
            return false;
        }
        if (!save_artifacts()) {
            return false;
        }
        if (!save_maps()) {
            return false;
        }
        if (!save_auto_pickup(true)) { // Save character auto pickup rules
            return false;
        }
        if (!save_uistate()) {
            return false;
        }
        return true;
    } catch (std::ios::failure &err) {
        popup(_("Failed to save game data"));
        return false;
    }
}

// Helper predicate to exclude files from deletion when resetting a world directory.
static bool isForbidden(std::string candidate)
{
    if (candidate.find("worldoptions.txt") != std::string::npos ||
        candidate.find("mods.json") != std::string::npos) {
        return true;
    }
    return false;
}

// If delete_folder is true, just delete all the files and directories of a world folder.
// If it's false, just avoid deleting the two config files and the directory itself.
void game::delete_world(std::string worldname, bool delete_folder)
{
    std::string worldpath = world_generator->all_worlds[worldname]->world_path;
    std::set<std::string> directory_paths;

    auto file_paths = get_files_from_path("", worldpath, true, true);
    if (!delete_folder) {
        std::vector<std::string>::iterator forbidden = find_if(file_paths.begin(), file_paths.end(),
                isForbidden);
        while (forbidden != file_paths.end()) {
            file_paths.erase(forbidden);
            forbidden = find_if(file_paths.begin(), file_paths.end(), isForbidden);
        }
    }
    for( auto &file_path : file_paths ) {
        // strip to path and remove worldpath from it
        std::string part = file_path.substr( worldpath.size(),
                                             file_path.find_last_of( "/\\" ) - worldpath.size() );
        int last_separator = part.find_last_of("/\\");
        while (last_separator != (int)std::string::npos && part.size() > 1) {
            directory_paths.insert(part);
            part = part.substr(0, last_separator);
            last_separator = part.find_last_of("/\\");
        }
    }

#if (defined _WIN32 || defined __WIN32__)
    for (std::vector<std::string>::iterator file = file_paths.begin();
         file != file_paths.end(); ++file) {
        DeleteFile(file->c_str());
    }
    for (std::set<std::string>::reverse_iterator it = directory_paths.rbegin();
         it != directory_paths.rend(); ++it) {
        RemoveDirectory(std::string(worldpath + *it).c_str());
    }
    if (delete_folder) {
        RemoveDirectory(worldpath.c_str());
    }
#else
    // Delete files, order doesn't matter.
    for( auto &file_path : file_paths ) {
        (void)unlink( file_path.c_str() );
    }
    // Delete directories -- directories are ordered deepest to shallowest.
    for (std::set<std::string>::reverse_iterator it = directory_paths.rbegin();
         it != directory_paths.rend(); ++it) {
        remove(std::string(worldpath + *it).c_str());
    }
    if (delete_folder) {
        remove(worldpath.c_str());
    }
#endif
}

std::vector<std::string> game::list_active_characters()
{
    std::vector<std::string> saves;
    std::vector<std::string> worldsaves = world_generator->active_world->world_saves;
    for( auto &worldsave : worldsaves ) {
        saves.push_back( base64_decode( worldsave ) );
    }
    return saves;
}

/**
 * Writes information about the character out to a text file timestamped with
 * the time of the file was made. This serves as a record of the character's
 * state at the time the memorial was made (usually upon death) and
 * accomplishments in a human-readable format.
 */
void game::write_memorial_file(std::string sLastWords)
{

    //Open the file first
    DIR *dir = opendir(FILENAMES["memorialdir"].c_str());
    if (!dir) {
#if (defined _WIN32 || defined __WIN32__)
        mkdir(FILENAMES["memorialdir"].c_str());
#else
        mkdir(FILENAMES["memorialdir"].c_str(), 0777);
#endif
        dir = opendir(FILENAMES["memorialdir"].c_str());
        if (!dir) {
            dbg(D_ERROR) << "game:write_memorial_file: Unable to make memorial directory.";
            debugmsg("Could not make '%s' directory", FILENAMES["memorialdir"].c_str());
            return;
        }
    }

    //To ensure unique filenames and to sort files, append a timestamp
    time_t rawtime;
    time(&rawtime);
    std::string timestamp = ctime(&rawtime);

    //Fun fact: ctime puts a \n at the end of the timestamp. Get rid of it.
    size_t end = timestamp.find_last_of('\n');
    timestamp = timestamp.substr(0, end);

    //Colons are not usable in paths, so get rid of them
    for( auto &elem : timestamp ) {
        if( elem == ':' ) {
            elem = '-';
        }
    }

    // Use "C" locale to determine printable characters, and replace with '_'
    // note that spaces will also be translated to '_' as well
    std::locale locl("C");
    std::string player_name;
    for( auto character : u.name ) {
        // Any printable, except space, as we want to convert those to '_'
        if( std::isgraph( character, locl ) ) {
            player_name.push_back( character );
        } else {
            player_name.push_back('_');
        }
        // Constant '5' takes into account '-' and '.txt'
        unsigned int max_fn_len = (FILENAME_MAX - timestamp.length() - 5);
        if( player_name.size() >= max_fn_len ) {
            // Shorten string to appropriate length - 1, so can add '~'
            player_name.resize(max_fn_len - 1);
            // Add '~' if player's name is shortened
            player_name.push_back('~');
            break;
        }
    }

    std::string memorial_file_path = FILENAMES["memorialdir"] +
        player_name + "-" + timestamp + ".txt";

    std::ofstream memorial_file;
    memorial_file.open(memorial_file_path.c_str());

    u.memorial(memorial_file, sLastWords);

    if (!memorial_file.is_open()) {
        dbg(D_ERROR) << "game:write_memorial_file: Unable to open " << memorial_file_path;
        debugmsg("Could not open memorial file '%s'", memorial_file_path.c_str());
    }

    //Cleanup
    memorial_file.close();
    closedir(dir);
}

void game::add_event(event_type type, int on_turn, int faction_id, int x, int y)
{
    event tmp(type, on_turn, faction_id, x, y);
    events.push_back(tmp);
}

struct terrain {
    ter_id ter;
    terrain(ter_id tid) : ter(tid) {};
    terrain(std::string sid)
    {
        ter = t_null;
        if (termap.find(sid) == termap.end()) {
            debugmsg("terrain '%s' does not exist.", sid.c_str());
        } else {
            ter = termap[sid].loadid;
        }
    };
};

bool game::event_queued(event_type type)
{
    for( auto &e : events ) {
        if( e.type == type ) {
            return true;
        }
    }
    return false;
}

#include "savegame.h"
void game::debug()
{
    int action = menu(true, // cancelable
                      _("Debug Functions - Using these is CHEATING!"),
                      _("Wish for an item"),       // 1
                      _("Teleport - Short Range"), // 2
                      _("Teleport - Long Range"),  // 3
                      _("Reveal map"),             // 4
                      _("Spawn NPC"),              // 5
                      _("Spawn Monster"),          // 6
                      _("Check game state..."),    // 7
                      _("Kill NPCs"),              // 8
                      _("Mutate"),                 // 9
                      _("Spawn a vehicle"),        // 10
                      _("Increase all skills"),    // 11
                      _("Learn all melee styles"), // 12
                      _("Unlock all recipes"),     // 13
                      _("Check NPC"),              // 14
                      _("Spawn Artifact"),         // 15
                      _("Spawn Clairvoyance Artifact"), //16
                      _("Map editor"), // 17
                      _("Change weather"),         // 18
                      _("Remove all monsters"),    // 19
                      _("Display hordes"), // 20
                      _("Test Item Group"), // 21
                      _("Damage Self"), //22
#ifdef LUA
                      _("Lua Command"), // 23
#endif
                      _("Cancel"),
                      NULL);
    int veh_num;
    std::vector<std::string> opts;
    switch (action) {
    case 1:
        wishitem(&u);
        break;

    case 2:
        teleport(&u, false);
        break;

    case 3: {
        tripoint tmp = overmap::draw_overmap();
        if (tmp != overmap::invalid_tripoint) {
            //First offload the active npcs.
            active_npc.clear();
            while( num_zombies() > 0 ) {
                despawn_monster( 0 );
            }
            m.clear_vehicle_cache();
            m.vehicle_list.clear();

            const int nlevx = tmp.x * 2 - int(MAPSIZE / 2);
            const int nlevy = tmp.y * 2 - int(MAPSIZE / 2);
            cur_om = &overmap_buffer.get_om_global(tmp.x, tmp.y);
            levx = nlevx - cur_om->pos().x * OMAPX * 2;
            levy = nlevy - cur_om->pos().y * OMAPY * 2;
            levz = tmp.z;
            m.load(levx, levy, levz, true, cur_om);
            load_npcs();
            m.spawn_monsters( true ); // Static monsters
            update_overmap_seen();
            draw_minimap();
        }
    }
    break;
    case 4:
        debugmsg(ngettext("%d radio tower", "%d radio towers", cur_om->radios.size()),
                 cur_om->radios.size());
        for (int i = 0; i < OMAPX; i++) {
            for (int j = 0; j < OMAPY; j++) {
                for (int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++) {
                    cur_om->seen(i, j, k) = true;
                }
            }
        }
        add_msg(m_good, _("Current overmap revealed."));
        break;

    case 5: {
        npc *temp = new npc();
        temp->normalize();
        temp->randomize();
        temp->spawn_at( get_abs_levx(), get_abs_levy(), levz );
        temp->setx( u.posx() - 4 );
        temp->sety( u.posy() - 4 );
        temp->form_opinion(&u);
        temp->mission = NPC_MISSION_NULL;
        int mission_index = reserve_random_mission(ORIGIN_ANY_NPC,
                            om_location(), temp->getID());
        if (mission_index != -1) {
            temp->chatbin.missions.push_back(mission_index);
        }
        load_npcs();
    }
    break;

    case 6:
        wishmonster();
        break;

    case 7: {
        std::string s;
        s = _("Location %d:%d in %d:%d, %s\n");
        s += _("Current turn: %d; Next spawn %d.\n%s\n");
        s += ngettext("%d monster exists.\n", "%d monsters exist.\n", num_zombies());
        s += ngettext("%d currently active NPC.\n", "%d currently active NPCs.\n", active_npc.size());
        s += ngettext("%d event planned.", "%d events planned", events.size());
        popup_top(
            s.c_str(),
            u.posx(), u.posy(), get_abs_levx(), get_abs_levy(),
            otermap[overmap_buffer.ter(om_global_location())].name.c_str(),
            int(calendar::turn), int(nextspawn),
            (ACTIVE_WORLD_OPTIONS["RANDOM_NPC"] == "true" ? _("NPCs are going to spawn.") :
             _("NPCs are NOT going to spawn.")),
            num_zombies(), active_npc.size(), events.size());
        if (!active_npc.empty()) {
            for( auto &elem : active_npc ) {
                tripoint t = ( elem )->global_sm_location();
                add_msg( m_info, _( "%s: map (%d:%d) pos (%d:%d)" ), ( elem )->name.c_str(), t.x,
                         t.y, ( elem )->posx(), ( elem )->posy() );
            }

            add_msg(m_info, _("(you: %d:%d)"), u.posx(), u.posy());
        }
        disp_NPCs();
        break;
    }
    case 8:
        for( auto &elem : active_npc ) {
            add_msg( _( "%s's head implodes!" ), ( elem )->name.c_str() );
            ( elem )->hp_cur[bp_head] = 0;
        }
        break;

    case 9:
        wishmutate(&u);
        break;

    case 10:
        if (m.veh_at(u.posx(), u.posy())) {
            dbg(D_ERROR) << "game:load: There's already vehicle here";
            debugmsg("There's already vehicle here");
        } else {
            std::vector<std::string> veh_strings;
            for( auto &elem : vtypes ) {
                if( elem.first != "custom" ) {
                    veh_strings.push_back( elem.second->type );
                    //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
                    opts.push_back( string_format( _( "%s (%s)" ), elem.second->name.c_str(),
                                                   elem.second->type.c_str() ) );
                }
            }
            opts.push_back(std::string(_("Cancel")));
            veh_num = menu_vec(false, _("Choose vehicle to spawn"), opts) + 1;
            veh_num -= 2;
            if (veh_num < (int)opts.size() - 1) {
                //Didn't pick Cancel
                std::string selected_opt = veh_strings[veh_num];
                vehicle *veh = m.add_vehicle(selected_opt, u.posx(), u.posy(), -90, 100, 0);
                if (veh != NULL) {
                    m.board_vehicle(u.posx(), u.posy(), &u);
                }
            }
        }
        break;

    case 11: {
        wishskill(&u);
    }
    break;

    case 12:
        add_msg(m_info, _("Martial arts debug."));
        add_msg(_("Your eyes blink rapidly as knowledge floods your brain."));
        for( auto &style : martialarts ) {
            if (style.first != "style_none") {
                u.add_martialart(style.first);
            }
        }
        add_msg(m_good, _("You now know a lot more than just 10 styles of kung fu."));
        break;

    case 13: {
        add_msg(m_info, _("Recipe debug."));
        add_msg(_("Your eyes blink rapidly as knowledge floods your brain."));
        for( auto &recipes_cat_iter : recipes ) {
            for( auto cur_recipe : recipes_cat_iter.second ) {

                if (!(u.learned_recipes.find(cur_recipe->ident) != u.learned_recipes.end()))  {
                    u.learn_recipe( (recipe *)cur_recipe );
                }
            }
        }
        add_msg(m_good, _("You know how to craft that now."));
    }
    break;

    case 14: {
        point pos = look_around();
        int npcdex = npc_at(pos.x, pos.y);
        if (npcdex == -1) {
            popup(_("No NPC there."));
        } else {
            std::stringstream data;
            npc *p = active_npc[npcdex];
            uimenu nmenu;
            nmenu.return_invalid = true;
            data << p->name << " " << (p->male ? _("Male") : _("Female")) << std::endl;

            data << npc_class_name(p->myclass) << "; " <<
                 npc_attitude_name(p->attitude) << std::endl;
            if (p->has_destination()) {
                data << string_format(_("Destination: %d:%d (%s)"),
                        p->goal.x, p->goal.y,
                        otermap[overmap_buffer.ter(p->goal)].name.c_str()) << std::endl;
            } else {
                data << _("No destination.") << std::endl;
            }
            data << string_format(_("Trust: %d"), p->op_of_u.trust) << " "
                 << string_format(_("Fear: %d"), p->op_of_u.fear) << " "
                 << string_format(_("Value: %d"), p->op_of_u.value) << " "
                 << string_format(_("Anger: %d"), p->op_of_u.anger) << " "
                 << string_format(_("Owed: %d"), p->op_of_u.owed) << std::endl;

            data << string_format(_("Aggression: %d"), int(p->personality.aggression)) << " "
                 << string_format(_("Bravery: %d"), int(p->personality.bravery)) << " "
                 << string_format(_("Collector: %d"), int(p->personality.collector)) << " "
                 << string_format(_("Altruism: %d"), int(p->personality.altruism)) << std::endl;

            nmenu.text = data.str();
            nmenu.addentry(0, true, 's', "%s", _("Edit [s]kills"));
            nmenu.addentry(1, true, 'i', "%s", _("Grant [i]tems"));
            nmenu.addentry(2, true, 'h', "%s", _("Cause [h]urt (to torso)"));
            nmenu.addentry(3, true, 'p', "%s", _("Cause [p]ain"));
            nmenu.addentry(4, true, '@', "%s", _("Status Window [@]"));
            nmenu.addentry(5, true, 'q', "%s", _("[q]uit"));
            nmenu.selected = 0;
            nmenu.query();
            switch (nmenu.ret) {
            case 0:
                wishskill(p);
                break;
            case 1:
                wishitem(p);
                break;
            case 2:
                p->apply_damage( nullptr, bp_torso, 20 );
                break;
            case 3:
                p->mod_pain(20);
                break;
            case 4:
                p->disp_info();
                break;
            default:
                break;
            }
        }
    }
    break;

    case 15: {
        point center = look_around();
        artifact_natural_property prop =
            artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
        m.create_anomaly(center.x, center.y, prop);
        m.spawn_natural_artifact(center.x, center.y, prop);
    }
    break;

    case 16:
        u.i_add(item(architects_cube(), calendar::turn));
        break;

    case 17: {
        point coord = look_debug();
    }
    break;

    case 18: {
        debugmsg("This menu is disabled in this version. Sorry!");
        break;
        const int weather_offset = 1;
        uimenu weather_menu;
        weather_menu.text = _("Select new weather pattern:");
        weather_menu.return_invalid = true;
        for (int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++) {
            weather_menu.addentry(weather_id + weather_offset, true, -1, weather_data[weather_id].name);
        }
        weather_menu.addentry(-10, true, 'v', _("View weather log"));
        weather_menu.addentry(-11, true, 'd', _("View last 800 hours of decay"));
        weather_menu.query();

        if (weather_menu.ret > 0 && weather_menu.ret <= NUM_WEATHER_TYPES) {
            add_msg(m_info, "%d", weather_menu.selected);

            int selected_weather = weather_menu.selected + 1;
            weather = (weather_type)selected_weather;
        } else if (weather_menu.ret == -10) {
            uimenu weather_log_menu;
            int pweather = 0;
            int cweather = 0;
            std::map<int, weather_segment>::iterator pit = weather_log.lower_bound(int(calendar::turn));
            --pit;
            if (pit != weather_log.end()) {
                cweather = pit->first;
            }
            if (cweather > 5) {
                --pit;
                if (pit != weather_log.end()) {
                    pweather = pit->first;
                }
            }
            weather_log_menu.text = string_format(_("turn: %d, next: %d, current: %d, prev: %d"),
                                                  int(calendar::turn), int(nextweather),
                                                  cweather, pweather );
            for (std::map<int, weather_segment>::const_iterator it = weather_log.begin();
                 it != weather_log.end(); ++it) {
                weather_log_menu.addentry(-1, true, -1, "%dd%dh %6d %15s[%d] %2d",
                                          it->second.deadline.days(), it->second.deadline.hours(),
                                          it->first,
                                          weather_data[int(it->second.weather)].name.c_str(),
                                          it->second.weather,
                                          (int)it->second.temperature
                                         );
                if (it->first == cweather) {
                    weather_log_menu.entries.back().text_color = c_yellow;
                }
            }
            weather_log_menu.query();

        } else if (weather_menu.ret == -11) {
            uimenu decay_test_menu;



            std::map<int, weather_segment>::iterator pit = weather_log.lower_bound(int(calendar::turn));
            --pit;

            calendar diffturn;

            decay_test_menu.text = string_format("turn: %d, next: %d",
                                                 int(calendar::turn), int(nextweather) );

            const int ne = 3;
            std::string examples[ne] = { "milk", "meat", "bread", };
            int esz[ne];
            int exsp[ne];
            std::string spstr = "";
            for (int i = 0; i < ne; i++) {
                const auto ity = item::find_type( examples[i] );
                exsp[i] = dynamic_cast<it_comest *>(ity)->spoils;
                esz[i] = examples[i].size();
                spstr = string_format("%s | %s", spstr.c_str(), examples[i].c_str());
            }

            decay_test_menu.addentry(-1, true, -1,
                                     "  age  day hour   turn |   w-end  temp |    rot  hr %% %s",
                                     spstr.c_str());

            for (int i = 0; i < 800; i++) {
                spstr = "";
                diffturn = int(calendar::turn) - (i * 600);
                pit = weather_log.lower_bound(int(diffturn));
                int prt = get_rot_since(int(diffturn), int(calendar::turn), u.pos());
                int perc = (get_rot_since(int(diffturn), int(diffturn) + 600, u.pos()) * 100) / 600;
                int frt = int(calendar::turn) - int(diffturn);
                for (int e = 0; e < ne; e++) {
                    spstr = string_format("%s | %c %c%s", spstr.c_str(),
                                          exsp[e] > prt ? 'Y' : ' ',
                                          exsp[e] > frt ? 'Y' : ' ',
                                          std::string(esz[e] - 3, ' ').c_str()
                                         );
                }
                decay_test_menu.addentry( -1, true, -1,
                                          "%4dh %3dd%3dh %7d | %7d: %3df | %7d %3d%% %s", 0 - i,
                                          diffturn.days(), diffturn.hours(), int(diffturn),
                                          int(pit->second.deadline), pit->second.temperature,
                                          prt, perc, spstr.c_str() );
            }
            decay_test_menu.query();
        }
    }
    break;

    case 19: {
        for (size_t i = 0; i < num_zombies(); i++) {
            // Use the normal death functions, useful for testing death
            // and for getting a corpse.
            zombie(i).die( nullptr );
        }
        cleanup_dead();
    }
    break;
    case 20: {
        // display hordes on the map
        overmap::draw_overmap(om_global_location(), true);
    }
    break;
    case 21: {
        item_group::debug_spawn();
    }
    break;

    // Damage Self
    case 22: {
        int dbg_damage = query_int("Damage self for how much? hp: %d", u.hp_cur[hp_torso]);
        u.hp_cur[hp_torso] -= dbg_damage;
        u.die(NULL);
        draw_sidebar();
    }
    break;

#ifdef LUA
    case 23: {
        std::string luacode = string_input_popup(_("Lua:"), 60, "");
        call_lua(luacode);
    }
    break;
#endif

    }
    erase();
    refresh_all();
}

void game::draw_overmap()
{
    overmap::draw_overmap();
    refresh_all();
}

void game::disp_kills()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       std::max(0, (TERMY - FULL_SCREEN_HEIGHT) / 2),
                       std::max(0, (TERMX - FULL_SCREEN_WIDTH) / 2));

    std::vector<std::string> data;
    int totalkills = 0;
    const int colum_width = (getmaxx(w) - 2) / 3; // minus border
    for( auto &elem : kills ) {
        const mtype *m = MonsterGenerator::generator().get_mtype( elem.first );
        std::ostringstream buffer;
        buffer << "<color_" << string_from_color(m->color) << ">";
        buffer << m->sym << " " << m->nname();
        buffer << "</color>";
        const int w = colum_width - utf8_width(m->nname().c_str());
        buffer.width(w - 3); // gap between cols, monster sym, space
        buffer.fill(' ');
        buffer << elem.second;
        buffer.width(0);
        data.push_back(buffer.str());
        totalkills += elem.second;
    }
    std::ostringstream buffer;
    if (data.empty()) {
        buffer << _("You haven't killed any monsters yet!");
    } else {
        buffer << string_format(_("KILL COUNT: %d"), totalkills);
    }
    display_table(w, buffer.str(), 3, data);

    werase(w);
    wrefresh(w);
    delwin(w);
    refresh_all();
}

inline bool npc_dist_to_player(const npc *a, const npc *b)
{
    const tripoint ppos = g->om_global_location();
    const tripoint apos = a->global_omt_location();
    const tripoint bpos = b->global_omt_location();
    return square_dist(ppos.x, ppos.y, apos.x, apos.y) < square_dist(ppos.x, ppos.y, bpos.x, bpos.y);
}

void game::disp_NPCs()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    const tripoint ppos = om_global_location();
    mvwprintz(w, 0, 0, c_white, _("Your position: %d:%d"), ppos.x, ppos.y);
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(100);
    std::sort(npcs.begin(), npcs.end(), npc_dist_to_player);
    for (size_t i = 0; i < 20 && i < npcs.size(); i++) {
        const tripoint apos = npcs[i]->global_omt_location();
        mvwprintz(w, i + 2, 0, c_white, "%s: %d:%d", npcs[i]->name.c_str(),
                  apos.x, apos.y);
    }
    wrefresh(w);
    getch();
    werase(w);
    wrefresh(w);
    delwin(w);
}

faction *game::list_factions(std::string title)
{
    std::vector<faction *> valfac; // Factions that we know of.
    for( auto &elem : factions ) {
        if( elem.known_by_u ) {
            valfac.push_back( &elem );
        }
    }
    if (valfac.empty()) { // We don't know of any factions!
        popup(_("You don't know of any factions.  Press Spacebar..."));
        return NULL;
    }

    WINDOW *w_list = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                            ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    WINDOW *w_info = newwin(FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE,
                            1 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            MAX_FAC_NAME_SIZE + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));

    int maxlength = FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE;
    size_t sel = 0;
    bool redraw = true;

    input_context ctxt("FACTIONS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    faction *cur_frac = NULL;
    while (true) {
        cur_frac = valfac[sel];
        if (redraw) {
            // Init w_list content
            werase(w_list);
            draw_border(w_list);
            mvwprintz(w_list, 1, 1, c_white, "%s", title.c_str());
            for (size_t i = 0; i < valfac.size(); i++) {
                nc_color col = (i == sel ? h_white : c_white);
                mvwprintz(w_list, i + 2, 1, col, "%s", valfac[i]->name.c_str());
            }
            wrefresh(w_list);
            // Init w_info content
            // fac_*_text() is in faction.cpp
            werase(w_info);
            mvwprintz(w_info, 0, 0, c_white,
                      _("Ranking: %s"), fac_ranking_text(cur_frac->likes_u).c_str());
            mvwprintz(w_info, 1, 0, c_white,
                      _("Respect: %s"), fac_respect_text(cur_frac->respects_u).c_str());
            fold_and_print(w_info, 3, 0, maxlength, c_white, cur_frac->describe());
            wrefresh(w_info);
            redraw = false;
        }
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            mvwprintz(w_list, sel + 2, 1, c_white, "%s", cur_frac->name.c_str());
            if (sel == valfac.size() - 1) {
                sel = 0;    // Wrap around
            } else {
                sel++;
            }
            redraw = true;
        } else if (action == "UP") {
            mvwprintz(w_list, sel + 2, 1, c_white, "%s", cur_frac->name.c_str());
            if (sel == 0) {
                sel = valfac.size() - 1;    // Wrap around
            } else {
                sel--;
            }
            redraw = true;
        } else if (action == "QUIT") {
            cur_frac = NULL;
            break;
        } else if (action == "CONFIRM") {
            break;
        }
    }
    werase(w_list);
    werase(w_info);
    delwin(w_list);
    delwin(w_info);
    refresh_all();
    return cur_frac;
}

void game::list_missions()
{
    WINDOW *w_missions = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                                (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    int tab = 0;
    size_t selection = 0;
    input_context ctxt("MISSIONS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    while (true) {
        werase(w_missions);
        std::vector<int> umissions;
        switch (tab) {
        case 0:
            umissions = u.active_missions;
            break;
        case 1:
            umissions = u.completed_missions;
            break;
        case 2:
            umissions = u.failed_missions;
            break;
        }

        for (int i = 1; i < FULL_SCREEN_WIDTH - 1; i++) {
            mvwputch(w_missions, 2, i, BORDER_COLOR, LINE_OXOX);
            mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX);

            if (i > 2 && i < FULL_SCREEN_HEIGHT - 1) {
                mvwputch(w_missions, i, 0, BORDER_COLOR, LINE_XOXO);
                mvwputch(w_missions, i, 30, BORDER_COLOR, LINE_XOXO);
                mvwputch(w_missions, i, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXO);
            }
        }

        draw_tab(w_missions, 7, _("ACTIVE MISSIONS"), (tab == 0) ? true : false);
        draw_tab(w_missions, 30, _("COMPLETED MISSIONS"), (tab == 1) ? true : false);
        draw_tab(w_missions, 56, _("FAILED MISSIONS"), (tab == 2) ? true : false);

        mvwputch(w_missions, 2, 0, BORDER_COLOR, LINE_OXXO); // |^
        mvwputch(w_missions, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX); // ^|

        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO); // |
        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOOX); // _|

        mvwputch(w_missions, 2, 30, BORDER_COLOR, (tab == 1) ? LINE_XOXX : LINE_XXXX); // + || -|
        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, 30, BORDER_COLOR, LINE_XXOX); // _|_

        for (size_t i = 0; i < umissions.size(); i++) {
            mission *miss = find_mission(umissions[i]);
            nc_color col = c_white;
            if (static_cast<int>(i) == u.active_mission && tab == 0) {
                col = c_ltred;
            }
            if (selection == i) {
                mvwprintz(w_missions, 3 + i, 1, hilite(col), "%s", miss->name().c_str());
            } else {
                mvwprintz(w_missions, 3 + i, 1, col, "%s", miss->name().c_str());
            }
        }

        if (selection < umissions.size()) {
            mission *miss = find_mission(umissions[selection]);
            mvwprintz(w_missions, 4, 31, c_white, "%s", miss->description.c_str());
            if (miss->deadline != 0)
                mvwprintz(w_missions, 5, 31, c_white, _("Deadline: %d (%d)"),
                          miss->deadline, int(calendar::turn));
            if (miss->target != overmap::invalid_point) {
                const tripoint pos = om_global_location();
                // TODO: target does not contain a z-component, targets are assumed to be on z=0
                mvwprintz(w_missions, 6, 31, c_white, _("Target: (%d, %d)   You: (%d, %d)"),
                          miss->target.x, miss->target.y, pos.x, pos.y);
            }
        } else {
            std::string nope;
            switch (tab) {
            case 0:
                nope = _("You have no active missions!");
                break;
            case 1:
                nope = _("You haven't completed any missions!");
                break;
            case 2:
                nope = _("You haven't failed any missions!");
                break;
            }
            mvwprintz(w_missions, 4, 31, c_ltred, "%s", nope.c_str());
        }

        wrefresh(w_missions);
        const std::string action = ctxt.handle_input();
        if (action == "RIGHT") {
            tab++;
            if (tab == 3) {
                tab = 0;
            }
        } else if (action == "LEFT") {
            tab--;
            if (tab < 0) {
                tab = 2;
            }
        } else if (action == "DOWN") {
            selection++;
            if (selection >= umissions.size()) {
                selection = 0;
            }
        } else if (action == "UP") {
            if (selection == 0) {
                selection = umissions.size() - 1;
            } else {
                selection--;
            }
        } else if (action == "CONFIRM") {
            u.active_mission = selection;
            break;
        } else if (action == "QUIT") {
            break;
        }
    }

    werase(w_missions);
    delwin(w_missions);
    refresh_all();
}

void game::calculate_footstep_markers(std::vector<point> &result)
{
    result.reserve(footsteps.size());
    for (size_t i = 0; i < footsteps.size(); i++) {
        if( !u.sees( footsteps_source[i] ) ) {
            std::vector<point> unseen_points;
            for( auto &elem : footsteps[i] ) {
                if( !u.sees( elem ) ) {
                    unseen_points.push_back( elem );
                }
            }
            if (unseen_points.size() > 0) {
                result.push_back(unseen_points[rng(0, unseen_points.size() - 1)]);
            }
        }
    }
    footsteps.clear();
    footsteps_source.clear();
}

// draws footsteps that have been created by monsters moving about
void game::draw_footsteps()
{
    if (is_draw_tiles_mode()) {
        return; // already done by cata_tiles
    }
    std::vector<point> markers;
    calculate_footstep_markers(markers);
    const int offset_y = POSY - (u.posy() + u.view_offset_y);
    const int offset_x = POSX - (u.posx() + u.view_offset_x);
    for (std::vector<point>::const_iterator a = markers.begin(); a != markers.end(); ++a) {
        mvwputch(w_terrain, offset_y + a->y, offset_x + a->x, c_yellow, '?');
    }
    wrefresh(w_terrain);
}

void game::draw()
{
    // Draw map
    werase(w_terrain);
    draw_ter();
    draw_footsteps();
    draw_sidebar();
}

void game::draw_sidebar()
{
    if (fullscreen) {
        return;
    }

    // w_status2 is not used with the wide sidebar (wide == !narrow)
    // Don't draw anything on it (no werase, wrefresh) in this case to avoid flickering
    // (it overlays other windows)
    const bool sideStyle = use_narrow_sidebar();

    // Draw Status
    draw_HP();
    werase(w_status);
    if( sideStyle ) {
        werase(w_status2);
    }
    if (!liveview.compact_view) {
        liveview.hide(true, false);
    }
    u.disp_status(w_status, w_status2);

    WINDOW *time_window = sideStyle ? w_status2 : w_status;
    wmove(time_window, sideStyle ? 0 : 1, sideStyle ? 15 : 41);
    if ((u.has_item_with_flag("WATCH") || u.has_bionic("bio_watch"))) {
        wprintz(time_window, c_white, "%s", calendar::turn.print_time().c_str());
    } else {
        std::vector<std::pair<char, nc_color> > vGlyphs;
        vGlyphs.push_back(std::make_pair('_', c_red));
        vGlyphs.push_back(std::make_pair('_', c_cyan));
        vGlyphs.push_back(std::make_pair('.', c_brown));
        vGlyphs.push_back(std::make_pair(',', c_blue));
        vGlyphs.push_back(std::make_pair('+', c_yellow));
        vGlyphs.push_back(std::make_pair('c', c_ltblue));
        vGlyphs.push_back(std::make_pair('*', c_yellow));
        vGlyphs.push_back(std::make_pair('C', c_white));
        vGlyphs.push_back(std::make_pair('+', c_yellow));
        vGlyphs.push_back(std::make_pair('c', c_ltblue));
        vGlyphs.push_back(std::make_pair('.', c_brown));
        vGlyphs.push_back(std::make_pair(',', c_blue));
        vGlyphs.push_back(std::make_pair('_', c_red));
        vGlyphs.push_back(std::make_pair('_', c_cyan));

        const int iHour = calendar::turn.hours();
        wprintz(time_window, c_white, "[");
        bool bAddTrail = false;

        for (int i = 0; i < 14; i += 2) {
            if (iHour >= 8 + i && iHour <= 13 + (i / 2)) {
                wputch(time_window, hilite(c_white), ' ');

            } else if (iHour >= 6 + i && iHour <= 7 + i) {
                wputch(time_window, hilite(vGlyphs[i].second), vGlyphs[i].first);
                bAddTrail = true;

            } else if (iHour >= (18 + i) % 24 && iHour <= (19 + i) % 24) {
                wputch(time_window, vGlyphs[i + 1].second, vGlyphs[i + 1].first);

            } else if (bAddTrail && iHour >= 6 + (i / 2)) {
                wputch(time_window, hilite(c_white), ' ');

            } else {
                wputch(time_window, c_white, ' ');
            }
        }

        wprintz(time_window, c_white, "]");
    }

    const oter_id &cur_ter = overmap_buffer.ter(om_global_location());

    std::string tername = otermap[cur_ter].name;
    werase(w_location);
    mvwprintz(w_location, 0, 0, otermap[cur_ter].color, "%s", utf8_truncate(tername, 14).c_str());

    if (levz < 0) {
        mvwprintz(w_location, 0, 18, c_ltgray, _("Underground"));
    } else {
        mvwprintz(w_location, 0, 18, weather_data[weather].color, "%s", weather_data[weather].name.c_str());
    }

    if (u.worn_with_flag("THERMOMETER")) {
        wprintz( w_location, c_white, " %s", print_temperature( get_temperature() ).c_str());
    }

    wrefresh(w_location);

    //Safemode coloring
    WINDOW *day_window = sideStyle ? w_status2 : w_status;
    mvwprintz(day_window, 0, sideStyle ? 0 : 41, c_white, _("%s, day %d"),
              season_name_uc[calendar::turn.get_season()].c_str(), calendar::turn.days() + 1);
    if (safe_mode != SAFE_MODE_OFF || autosafemode != 0) {
        int iPercent = int((turnssincelastmon * 100) / OPTIONS["AUTOSAFEMODETURNS"]);
        wmove(w_status, sideStyle ? 4 : 1, getmaxx(w_status) - 4);
        const char *letters[] = { "S", "A", "F", "E" };
        for (int i = 0; i < 4; i++) {
            nc_color c = (safe_mode == SAFE_MODE_OFF && iPercent < (i + 1) * 25) ? c_red : c_green;
            wprintz(w_status, c, letters[i]);
        }
    }
    wrefresh(w_status);
    if( sideStyle ) {
        wrefresh(w_status2);
    }

    werase(w_messages);
    int maxlength = getmaxx(w_messages);

    // Print monster info and start our output below it.
    const int topline = mon_info(w_messages) + 2;

    int line = getmaxy(w_messages) - 1;
    Messages::display_messages(w_messages, 0, topline, maxlength, line);

    wrefresh(w_messages);

    draw_minimap();
}

bool game::isBetween(int test, int down, int up)
{
    if (test > down && test < up) {
        return true;
    } else {
        return false;
    }
}

void game::draw_critter(const Creature &critter, const point &center)
{
    const int my = POSY + ( critter.posy() - center.y );
    const int mx = POSX + ( critter.posx() - center.x );
    if( !is_valid_in_w_terrain( mx, my ) ) {
        return;
    }
    if( u.sees( critter ) || &critter == &u ) {
        critter.draw( w_terrain, center.x, center.y, false );
        return;
    }

    if( u.sees_with_infrared( critter ) ) {
        mvwputch( w_terrain, my, mx, c_red, '?' );
    }
}
void game::draw_ter(int posx, int posy)
{
    draw_ter(posx, posy, false);
}

void game::draw_ter(int posx, int posy, bool looking)
{
    // posx/posy default to -999
    if (posx == -999) {
        posx = u.posx() + u.view_offset_x;
    }
    if (posy == -999) {
        posy = u.posy() + u.view_offset_y;
    }
    const point center( posx, posy );

    ter_view_x = posx;
    ter_view_y = posy;

    m.build_map_cache();
    m.draw( w_terrain, center );

    // Draw monsters
    for (size_t i = 0; i < num_zombies(); i++) {
        draw_critter( critter_tracker.find( i ), center );
    }

    // Draw NPCs
    for( const npc* n : active_npc ) {
        draw_critter( *n, center );
    }

    if (u.has_active_bionic("bio_scent_vision")) {
        for (int realx = posx - POSX; realx <= posx + POSX; realx++) {
            for (int realy = posy - POSY; realy <= posy + POSY; realy++) {
                if (scent(realx, realy) != 0) {
                    int tempx = posx - realx, tempy = posy - realy;
                    if (!(isBetween(tempx, -2, 2) && isBetween(tempy, -2, 2))) {
                        if (mon_at(realx, realy) != -1) {
                            mvwputch(w_terrain, realy + POSY - posy,
                                     realx + POSX - posx, c_white, '?');
                        } else {
                            mvwputch(w_terrain, realy + POSY - posy,
                                     realx + POSX - posx, c_magenta, '#');
                        }
                    }
                }
            }
        }
    }

    if (!destination_preview.empty()) {
        // Draw auto-move preview trail
        point final_destination = destination_preview.back();
        point center = point(u.posx() + u.view_offset_x, u.posy() + u.view_offset_y);
        draw_line(final_destination.x, final_destination.y, center, destination_preview);
        mvwputch(w_terrain, POSY + (final_destination.y - (u.posy() + u.view_offset_y)),
                 POSX + (final_destination.x - (u.posx() + u.view_offset_x)), c_white, 'X');
    }

    if (u.controlling_vehicle && !looking) {
        draw_veh_dir_indicator();
    }
    if(uquit == QUIT_WATCH) {
        // This should remove the flickering the bar recieves
        input_context ctxt("DEFAULTMODE");
        std::string message = string_format( _("Press %s to accept your fate..."),
                ctxt.get_desc("QUIT").c_str() );
        mvwprintz( w_terrain, 0, (TERRAIN_WINDOW_WIDTH / 2) - (message.length() / 2), c_white,
                   message.c_str() );
    }
    wrefresh(w_terrain);

    if (u.has_effect("visuals") || (u.get_effect_int("hot", bp_head) > 1)) {
        hallucinate(posx, posy);
    }
}

void game::draw_veh_dir_indicator(void)
{
    // don't draw indicator if doing look_around()
    if (OPTIONS["VEHICLE_DIR_INDICATOR"]) {
        vehicle *veh = m.veh_at(u.posx(), u.posy());
        if (!veh) {
            debugmsg("game::draw_veh_dir_indicator: no vehicle!");
            return;
        }
        rl_vec2d face = veh->face_vec();
        float r = 10.0;
        int x = static_cast<int>(r * face.x);
        int y = static_cast<int>(r * face.y);
        mvwputch(w_terrain, POSY + y - u.view_offset_y, POSX + x - u.view_offset_x, c_white, 'X');
    }
}

void game::refresh_all()
{
    m.reset_vehicle_cache();
    draw();
    refresh();
}

void game::draw_HP()
{
    werase(w_HP);
    nc_color color;
    std::string health_bar = "";

    // The HP window can be in "tall" mode (7x14) or "wide" mode (14x7).
    bool wide = (getmaxy(w_HP) == 7);
    int hpx = wide ? 7 : 0;
    int hpy = wide ? 0 : 1;
    int dy = wide ? 1 : 2;
    for (int i = 0; i < num_hp_parts; i++) {
        get_HP_Bar(u.hp_cur[i], u.hp_max[i], color, health_bar);

        wmove(w_HP, i * dy + hpy, hpx);
        if (u.has_trait("SELFAWARE")) {
            wprintz(w_HP, color, "%3d  ", u.hp_cur[i]);
        } else {
            wprintz(w_HP, color, "%s", health_bar.c_str());

            //Add the trailing symbols for a not-quite-full health bar
            int bar_remainder = 5;
            while (bar_remainder > (int)health_bar.size()) {
                --bar_remainder;
                wprintz(w_HP, c_white, ".");
            }
        }
    }

    static const char *body_parts[] = { _("HEAD"), _("TORSO"), _("L ARM"),
                                        _("R ARM"), _("L LEG"), _("R LEG"), _("POWER")
                                      };
    static body_part part[] = { bp_head, bp_torso, bp_arm_l,
                                bp_arm_r, bp_leg_l, bp_leg_r, num_bp
                              };
    int num_parts = sizeof(body_parts) / sizeof(body_parts[0]);
    for (int i = 0; i < num_parts; i++) {
        const char *str = body_parts[i];
        wmove(w_HP, i * dy, 0);
        if (wide) {
            wprintz(w_HP, limb_color(&u, part[i]), " ");
        }
        wprintz(w_HP, limb_color(&u, part[i]), str);
        if (!wide) {
            wprintz(w_HP, limb_color(&u, part[i]), ":");
        }
    }

    int powx = hpx;
    int powy = wide ? 6 : 13;
    if (u.max_power_level == 0) {
        wmove(w_HP, powy, powx);
        if (wide)
            for (int i = 0; i < 2; i++) {
                wputch(w_HP, c_ltgray, LINE_OXOX);
            }
        else {
            wprintz(w_HP, c_ltgray, " --   ");
        }
    } else {
        if (u.power_level == u.max_power_level) {
            color = c_blue;
        } else if (u.power_level >= u.max_power_level * .5) {
            color = c_ltblue;
        } else if (u.power_level > 0) {
            color = c_yellow;
        } else {
            color = c_red;
        }
        mvwprintz(w_HP, powy, powx, color, "%-3d", u.power_level);
    }
    wrefresh(w_HP);
}

nc_color game::limb_color(player *p, body_part bp, bool bleed, bool bite, bool infect)
{
    if (bp == num_bp) {
        return c_ltgray;
    }

    int color_bit = 0;
    nc_color i_color = c_ltgray;
    if (bleed && p->has_effect("bleed", bp)) {
        color_bit += 1;
    }
    if (bite && p->has_effect("bite", bp)) {
        color_bit += 10;
    }
    if (infect && p->has_effect("infected", bp)) {
        color_bit += 100;
    }
    switch (color_bit) {
    case 1:
        i_color = c_red;
        break;
    case 10:
        i_color = c_blue;
        break;
    case 100:
        i_color = c_green;
        break;
    case 11:
        i_color = c_magenta;
        break;
    case 101:
        i_color = c_yellow;
        break;
    }
    return i_color;
}

void game::draw_minimap()
{
    // Draw the box
    werase(w_minimap);
    draw_border(w_minimap);

    const tripoint curs = om_global_location();
    const int cursx = curs.x;
    const int cursy = curs.y;
    bool drew_mission = false;
    point targ;
    if (u.active_mission >= 0 && u.active_mission < (int)u.active_missions.size()) {
        targ = find_mission(u.active_missions[u.active_mission])->target;
        if (targ == overmap::invalid_point) {
            drew_mission = true;
        }
    } else {
        drew_mission = true;
    }

    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            const int omx = cursx + i;
            const int omy = cursy + j;
            nc_color ter_color;
            long ter_sym;
            const bool seen = overmap_buffer.seen(omx, omy, levz);
            const bool vehicle_here = overmap_buffer.has_vehicle(omx, omy, levz);
            if (overmap_buffer.has_note(omx, omy, levz)) {
                const std::string &note = overmap_buffer.note(omx, omy, levz);
                ter_color = c_yellow;
                if (note.length() >= 2 && note[1] == ':') {
                    ter_sym = note[0];
                } else if (note.length() >= 4 && note[3] == ':') {
                    ter_sym = note[2];
                } else {
                    ter_sym = 'N';
                }
                if (note.length() >= 2 && note[1] == ';') {
                    if (note[0] == 'r') {
                        ter_color = c_ltred;
                    }
                    if (note[0] == 'R') {
                        ter_color = c_red;
                    }
                    if (note[0] == 'g') {
                        ter_color = c_ltgreen;
                    }
                    if (note[0] == 'G') {
                        ter_color = c_green;
                    }
                    if (note[0] == 'b') {
                        ter_color = c_ltblue;
                    }
                    if (note[0] == 'B') {
                        ter_color = c_blue;
                    }
                    if (note[0] == 'W') {
                        ter_color = c_white;
                    }
                    if (note[0] == 'C') {
                        ter_color = c_cyan;
                    }
                    if (note[0] == 'P') {
                        ter_color = c_pink;
                    }
                } else if (note.length() >= 4 && note[3] == ';') {
                    if (note[2] == 'r') {
                        ter_color = c_ltred;
                    }
                    if (note[2] == 'R') {
                        ter_color = c_red;
                    }
                    if (note[2] == 'g') {
                        ter_color = c_ltgreen;
                    }
                    if (note[2] == 'G') {
                        ter_color = c_green;
                    }
                    if (note[2] == 'b') {
                        ter_color = c_ltblue;
                    }
                    if (note[2] == 'B') {
                        ter_color = c_blue;
                    }
                    if (note[2] == 'W') {
                        ter_color = c_white;
                    }
                    if (note[2] == 'C') {
                        ter_color = c_cyan;
                    }
                    if (note[2] == 'P') {
                        ter_color = c_pink;
                    }
                } else {
                    ter_color = c_yellow;
                }
            } else if (!seen) {
                ter_sym = ' ';
                ter_color = c_black;
            } else if (vehicle_here) {
                ter_color = c_cyan;
                ter_sym = 'c';
            } else {
                const oter_id &cur_ter = overmap_buffer.ter(omx, omy, levz);
                ter_sym = otermap[cur_ter].sym;
                if (overmap_buffer.is_explored(omx, omy, levz)) {
                    ter_color = c_dkgray;
                } else {
                    ter_color = otermap[cur_ter].color;
                }
            }
            if (!drew_mission && targ.x == omx && targ.y == omy) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: target does not contain a z-component, targets are assume to be on z=0
                drew_mission = true;
                if (i != 0 || j != 0) {
                    ter_color = red_background(ter_color);
                }
            }
            if (i == 0 && j == 0) {
                mvwputch_hi(w_minimap, 3, 3, ter_color, ter_sym);
            } else {
                mvwputch(w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
            }
        }
    }

    // Print arrow to mission if we have one!
    if (!drew_mission) {
        double slope;
        if (cursx != targ.x) {
            slope = double(targ.y - cursy) / double(targ.x - cursx);
        }
        if (cursx == targ.x || fabs(slope) > 3.5) { // Vertical slope
            if (targ.y > cursy) {
                mvwputch(w_minimap, 6, 3, c_red, '*');
            } else {
                mvwputch(w_minimap, 0, 3, c_red, '*');
            }
        } else {
            int arrowx = 3, arrowy = 3;
            if (fabs(slope) >= 1.) { // y diff is bigger!
                arrowy = (targ.y > cursy ? 6 : 0);
                arrowx = int(3 + 3 * (targ.y > cursy ? slope : (0 - slope)));
                if (arrowx < 0) {
                    arrowx = 0;
                }
                if (arrowx > 6) {
                    arrowx = 6;
                }
            } else {
                arrowx = (targ.x > cursx ? 6 : 0);
                arrowy = int(3 + 3 * (targ.x > cursx ? slope : (0 - slope)));
                if (arrowy < 0) {
                    arrowy = 0;
                }
                if (arrowy > 6) {
                    arrowy = 6;
                }
            }
            mvwputch(w_minimap, arrowy, arrowx, c_red, '*');
        }
    }
    wrefresh(w_minimap);
}

void game::hallucinate(const int x, const int y)
{
    const int rx = x - POSX;
    const int ry = y - POSY;
    for (int i = 0; i <= TERRAIN_WINDOW_WIDTH; i++) {
        for (int j = 0; j <= TERRAIN_WINDOW_HEIGHT; j++) {
            if (one_in(10)) {
                char ter_sym = terlist[m.ter(i + rx + rng(-2, 2),
                                             j + ry + rng(-2, 2))].sym;
                nc_color ter_col = terlist[m.ter(i + rx + rng(-2, 2),
                                                 j + ry + rng(-2, 2))].color;
                mvwputch(w_terrain, j, i, ter_col, ter_sym);
            }
        }
    }
    wrefresh(w_terrain);
}

float game::ground_natural_light_level() const
{
    float ret = (float)calendar::turn.sunlight();
    ret += weather_data[weather].light_modifier;

    return std::max(0.0f, ret);
}

float game::natural_light_level() const
{
    float ret = 0;

    if (levz >= 0) {
        ret = (float)calendar::turn.sunlight();
        ret += weather_data[weather].light_modifier;
    }

    return std::max(0.0f, ret);
}

unsigned char game::light_level()
{
    //already found the light level for now?
    if (calendar::turn == latest_lightlevel_turn) {
        return latest_lightlevel;
    }

    int ret;
    if (levz < 0) { // Underground!
        ret = 1;
    } else {
        ret = calendar::turn.sunlight();
        ret -= weather_data[weather].sight_penalty;
    }
    for( auto &e : events ) {
        // The EVENT_DIM event slowly dims the sky, then relights it
        // EVENT_DIM has an occurrence date of turn + 50, so the first 25 dim it
        if( e.type == EVENT_DIM ) {
            int turns_left = e.turn - int(calendar::turn);
            if (turns_left > 25) {
                ret = (ret * (turns_left - 25)) / 25;
            } else {
                ret = (ret * (25 - turns_left)) / 25;
            }
            break;
        }
    }
    if (ret < 8 && event_queued(EVENT_ARTIFACT_LIGHT)) {
        ret = 8;
    }
    if (ret < 1) {
        ret = 1;
    }

    latest_lightlevel = ret;
    latest_lightlevel_turn = calendar::turn;
    return ret;
}

void game::reset_light_level()
{
    latest_lightlevel = 0;
    latest_lightlevel_turn = 0;
}

//Gets the next free ID, also used for player ID's.
int game::assign_npc_id()
{
    int ret = next_npc_id;
    next_npc_id++;
    return ret;
}

int game::assign_faction_id()
{
    int ret = next_faction_id;
    next_faction_id++;
    return ret;
}

faction *game::faction_by_ident(std::string id)
{
    for( auto &elem : factions ) {
        if( elem.id == id ) {
            return &elem;
        }
    }
    return NULL;
}

Creature *game::is_hostile_nearby()
{
    int distance = (OPTIONS["SAFEMODEPROXIMITY"] <= 0) ? 60 : OPTIONS["SAFEMODEPROXIMITY"];
    return is_hostile_within(distance);
}

Creature *game::is_hostile_very_close()
{
    return is_hostile_within(dangerous_proximity);
}

Creature *game::is_hostile_within(int distance)
{
    for( auto &critter : u.get_visible_creatures( distance ) ) {
        if( u.attitude_to( *critter ) == Creature::A_HOSTILE ) {
            return critter;
        }
    }

    return nullptr;
}

//get the fishable critters around and return these
std::vector<monster*> game::get_fishable(int distance)
{
    std::vector<monster*> unique_fish;
    for (size_t i = 0; i < num_zombies(); i++) {
        monster &critter = critter_tracker.find(i);

        if (critter.has_flag(MF_FISHABLE)) {
            int mondist = rl_dist( u.pos(), critter.pos() );
            if (mondist <= distance) {
            unique_fish.push_back (&critter);
            }
        }
    }

    return unique_fish;
}

// Print monster info to the given window, and return the lowest row (0-indexed)
// to which we printed. This is used to share a window with the message log and
// make optimal use of space.
int game::mon_info(WINDOW *w)
{
    const int width = getmaxx(w);
    const int maxheight = 12;
    const int startrow = use_narrow_sidebar() ? 1 : 0;

    std::string sbuff;
    int newseen = 0;
    const int iProxyDist = (OPTIONS["SAFEMODEPROXIMITY"] <= 0) ? 60 : OPTIONS["SAFEMODEPROXIMITY"];
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::vector<npc*> unique_types[9];
    std::vector<std::string> unique_mons[9];
    // dangerous_types tracks whether we should print in red to warn the player
    bool dangerous[8];
    for( auto &dangerou : dangerous ) {
        dangerou = false;
    }

    int viewx = u.posx() + u.view_offset_x;
    int viewy = u.posy() + u.view_offset_y;
    new_seen_mon.clear();

    for( auto &c : u.get_visible_creatures( SEEX * MAPSIZE ) ) {
        const auto m = dynamic_cast<monster*>( c );
        const auto p = dynamic_cast<npc*>( c );
        const auto dir_to_mon = direction_from( viewx, viewy, c->posx(), c->posy() );
        const int mx = POSX + ( c->posx() - viewx );
        const int my = POSY + ( c->posy() - viewy );
        int index;
        if( is_valid_in_w_terrain( mx, my ) ) {
            index = 8;
        } else {
            index = dir_to_mon;
        }
        if( m != nullptr ) {
            auto &critter = *m;
            if(critter.type->has_flag(MF_VERMIN)) {
                continue;
            }

            monster_attitude matt = critter.attitude(&u);
            if (MATT_ATTACK == matt || MATT_FOLLOW == matt) {
                if (index < 8 && critter.sees( g->u )) {
                    dangerous[index] = true;
                }

                int mondist = rl_dist( u.pos(), critter.pos() );
                if (mondist <= iProxyDist) {
                    bool passmon = false;
                    if (critter.ignoring > 0) {
                        if (safe_mode != SAFE_MODE_ON) {
                            critter.ignoring = 0;
                        } else if (mondist > critter.ignoring / 2 || mondist < 6) {
                            passmon = true;
                        }
                    }
                    if (!passmon) {
                        newseen++;
                        new_seen_mon.push_back( mon_at( critter.posx(), critter.posy() ) );
                    }
                }
            }

            const auto &vec = unique_mons[dir_to_mon];
            if( std::find( vec.begin(), vec.end(), critter.type->id ) == vec.end() ) {
                unique_mons[index].push_back(critter.type->id);
            }
        } else if( p != nullptr ) {
            if (p->attitude == NPCATT_KILL)
                if (rl_dist( u.pos(), p->pos() ) <= iProxyDist) {
                    newseen++;
                }

            unique_types[index].push_back( p );
        }
    }

    if (newseen > mostseen) {
        if (newseen - mostseen == 1) {
            if (!new_seen_mon.empty()) {
                monster &critter = critter_tracker.find(new_seen_mon.back());
                cancel_activity_query(_("%s spotted!"), critter.name().c_str());
                if (u.has_trait("M_DEFENDER")) {
                    if (critter.type->in_species("PLANT")) {
                        add_msg(m_warning, _("We have detected a %s."), critter.name().c_str());
                        if (!u.has_effect("adrenaline_mycus")){
                            u.add_effect("adrenaline_mycus", 300);
                        } else {
                            if (u.get_effect_int("adrenaline_mycus") == 1) {
                                // Triffids present.  We ain't got TIME to adrenaline comedown!
                                u.add_effect("adrenaline_mycus", 150);
                                u.mod_pain(3); // Does take it out of you, though
                                add_msg(m_info, _("Our fibers strain with renewed wrath!"));
                            }
                        }
                    }
                }
            } else {
                //Hostile NPC
                cancel_activity_query(_("Hostile survivor spotted!"));
            }
        } else {
            cancel_activity_query(_("Monsters spotted!"));
        }
        turnssincelastmon = 0;
        if (safe_mode == SAFE_MODE_ON) {
            safe_mode = SAFE_MODE_STOP; // Stop movement!
        }
    } else if (autosafemode && newseen == 0) { // Auto-safemode
        turnssincelastmon++;
        if (turnssincelastmon >= OPTIONS["AUTOSAFEMODETURNS"] && safe_mode == SAFE_MODE_OFF) {
            safe_mode = SAFE_MODE_ON;
        }
    }

    if (newseen == 0 && safe_mode == SAFE_MODE_STOP) {
        safe_mode = SAFE_MODE_ON;
    }

    mostseen = newseen;

    // Print the direction headings
    // Reminder:
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)

    const char *dir_labels[] = {
        _("North:"), _("NE:"), _("East:"), _("SE:"),
        _("South:"), _("SW:"), _("West:"), _("NW:")
    };
    int widths[8];
    for (int i = 0; i < 8; i++) {
        widths[i] = utf8_width(dir_labels[i]);
    }
    int xcoords[8];
    const int ycoords[] = { 0, 0, 1, 2, 2, 2, 1, 0 };
    xcoords[0] = xcoords[4] = width / 3;
    xcoords[1] = xcoords[3] = xcoords[2] = (width / 3) * 2;
    xcoords[5] = xcoords[6] = xcoords[7] = 0;
    xcoords[2] -= utf8_width(_("East:")) - utf8_width(
                      _("NE:"));//for the alignment of the 1,2,3 rows on the right edge
    for (int i = 0; i < 8; i++) {
        nc_color c = unique_types[i].empty() && unique_mons[i].empty() ? c_dkgray
                     : (dangerous[i] ? c_ltred : c_ltgray);
        mvwprintz(w, ycoords[i] + startrow, xcoords[i], c, dir_labels[i]);
    }

    // Print the symbols of all monsters in all directions.
    for (int i = 0; i < 8; i++) {
        int symroom;
        point pr(xcoords[i] + widths[i] + 1, ycoords[i] + startrow);

        // The list of symbols needs a space on each end.
        symroom = (width / 3) - widths[i] - 2;
        const int typeshere_npc = unique_types[i].size();
        const int typeshere_mon = unique_mons[i].size();
        const int typeshere = typeshere_mon + typeshere_npc;
        for (int j = 0; j < typeshere && j < symroom; j++) {
            nc_color c;
            std::string sym;
            if (symroom < typeshere && j == symroom - 1) {
                // We've run out of room!
                c = c_white;
                sym = "+";
            } else if (j < typeshere_npc) {
                switch (unique_types[i][j]->attitude) {
                case NPCATT_KILL:
                    c = c_red;
                    break;
                case NPCATT_FOLLOW:
                    c = c_ltgreen;
                    break;
                case NPCATT_DEFEND:
                    c = c_green;
                    break;
                default:
                    c = c_pink;
                    break;
                }
                sym = "@";
            } else {
                sbuff = unique_mons[i][j - typeshere_npc];
                c = GetMType(sbuff)->color;
                sym = GetMType(sbuff)->sym;
            }
            mvwprintz(w, pr.y, pr.x, c, "%s", sym.c_str());

            pr.x++;
        }
    } // for (int i = 0; i < 8; i++)

    // Now we print their full names!

    std::set<std::string> listed_mons;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr(0, 4 + startrow);

    int lastrowprinted = 2 + startrow;

    // Print monster names, starting with those at location 8 (nearby).
    for (int j = 8; j >= 0 && pr.y < maxheight; j--) {
        // Separate names by some number of spaces (more for local monsters).
        int namesep = (j == 8 ? 2 : 1);
        for (std::vector<std::string>::iterator it = unique_mons[j].begin();
             it != unique_mons[j].end() && pr.y < maxheight; ++it) {
            sbuff = *it;
            // buff < 0 means an NPC!  Don't list those.
            if (listed_mons.find(sbuff) == listed_mons.end()) {
                listed_mons.insert(sbuff);

                std::string name = GetMType(sbuff)->nname();

                // Move to the next row if necessary. (The +2 is for the "Z ").
                if (pr.x + 2 + utf8_width(name.c_str()) >= width) {
                    pr.y++;
                    pr.x = 0;
                }

                if (pr.y < maxheight) { // Don't print if we've overflowed
                    lastrowprinted = pr.y;
                    mvwprintz(w, pr.y, pr.x, GetMType(sbuff)->color, "%s", GetMType(sbuff)->sym.c_str());
                    pr.x += 2; // symbol and space
                    nc_color danger = c_dkgray;
                    if (GetMType(sbuff)->difficulty >= 30) {
                        danger = c_red;
                    } else if (GetMType(sbuff)->difficulty >= 16) {
                        danger = c_ltred;
                    } else if (GetMType(sbuff)->difficulty >= 8) {
                        danger = c_white;
                    } else if (GetMType(sbuff)->agro > 0) {
                        danger = c_ltgray;
                    }
                    mvwprintz(w, pr.y, pr.x, danger, "%s", name.c_str());
                    pr.x += utf8_width(name.c_str()) + namesep;
                }
            }
        }
    }

    return lastrowprinted;
}

void game::cleanup_dead()
{
    for( size_t i = 0; i < num_zombies(); ) {
        monster &critter = critter_tracker.find(i);
        if( critter.is_dead() ) {
            dbg(D_INFO) << string_format("cleanup_dead: critter[%d] %d,%d dead:%c hp:%d %s",
                                         i, critter.posx(), critter.posy(), (critter.is_dead() ? '1' : '0'),
                                         critter.hp, critter.name().c_str());
            remove_zombie( i );
        } else {
            i++;
        }
    }

    //Cleanup any dead npcs.
    //This will remove the npc object, it is assumed that they have been transformed into
    //dead bodies before this.
    for( auto it = active_npc.begin(); it != active_npc.end(); ) {
        npc *n = *it;
        if( n->is_dead() ) {
            n->die( nullptr ); // make sure this has been called to create corpses etc.
            const int npc_id = n->getID();
            it = active_npc.erase( it );
            overmap_buffer.remove_npc( npc_id );
        } else {
            it++;
        }
    }
}

void game::monmove()
{
    cleanup_dead();

    // monster::plan() needs to know about all monsters on the same team as the monster
    mfactions monster_factions; // A map - looks much cleaner than vector here
    auto playerfaction = GetMFact( "player" );
    for (int i = 0, numz = num_zombies(); i < numz; i++) {
        monster &critter = zombie( i );
        if( critter.friendly == 0 ) {
            monster_factions[ critter.faction ].insert( i ); // Only 1 faction per mon at the moment
        } else {
            monster_factions[ playerfaction ].insert( i );
        }
    }

    for (size_t i = 0; i < num_zombies(); i++) {
        monster *critter = &critter_tracker.find(i);
        while (!critter->is_dead() && !critter->can_move_to(critter->posx(), critter->posy())) {
            // If we can't move to our current position, assign us to a new one
                dbg(D_ERROR) << "game:monmove: " << critter->name().c_str()
                             << " can't move to its location! (" << critter->posx()
                             << ":" << critter->posy() << "), "
                             << m.tername(critter->posx(), critter->posy()).c_str();
                add_msg( m_debug, "%s can't move to its location! (%d:%d), %s", critter->name().c_str(),
                         critter->posx(), critter->posy(), m.tername(critter->posx(), critter->posy()).c_str());
            bool okay = false;
            int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
            int startx = critter->posx() - 3 * xdir, endx = critter->posx() + 3 * xdir;
            int starty = critter->posy() - 3 * ydir, endy = critter->posy() + 3 * ydir;
            for (int x = startx; x != endx && !okay; x += xdir) {
                for (int y = starty; y != endy && !okay; y += ydir) {
                    if (critter->can_move_to(x, y) && is_empty(x, y)) {
                        critter->setpos(x, y);
                        okay = true;
                    }
                }
            }
            if (!okay) {
                // die of "natural" cause (overpopulation is natural)
                critter->die( nullptr );
            }
        }

        if (!critter->is_dead()) {
            critter->process_turn();
        }

        m.creature_in_field( *critter );

        while (critter->moves > 0 && !critter->is_dead()) {
            critter->made_footstep = false;
            // Controlled critters don't make their own plans
            if (!critter->has_effect("controlled")) {
                // Formulate a path to follow
                critter->plan( monster_factions );
            }
            critter->move(); // Move one square, possibly hit u
            critter->process_triggers();
            m.creature_in_field( *critter );
        }

        if (!critter->is_dead()) {
            if (u.has_active_bionic("bio_alarm") && u.power_level >= 25 &&
                rl_dist( u.pos(), critter->pos() ) <= 5) {
                u.power_level -= 25;
                add_msg(m_warning, _("Your motion alarm goes off!"));
                cancel_activity_query(_("Your motion alarm goes off!"));
                if (u.in_sleep_state()) {
                    u.wake_up();
                }
            }
            // We might have stumbled out of range of the player; if so, kill us
            if (critter->posx() < 0 - (SEEX * MAPSIZE) / 6 ||
                critter->posy() < 0 - (SEEY * MAPSIZE) / 6 ||
                critter->posx() > (SEEX * MAPSIZE * 7) / 6 ||
                critter->posy() > (SEEY * MAPSIZE * 7) / 6) {
                // Remove the zombie, but don't let it "die", it still exists, just
                // not in the reality bubble.
                despawn_monster( i );
                i--;
            }
        }
    }

    cleanup_dead();

    // Now, do active NPCs.
    for( auto &elem : active_npc ) {
        int turns = 0;
        m.creature_in_field( *elem );
        if( ( elem )->hp_cur[hp_head] <= 0 || ( elem )->hp_cur[hp_torso] <= 0 ) {
            ( elem )->die( nullptr );
        } else {
            ( elem )->process_turn();
            while( !( elem )->is_dead() && ( elem )->moves > 0 && turns < 10 ) {
                int moves = ( elem )->moves;
                ( elem )->move();
                if( moves == ( elem )->moves ) {
                    // Count every time we exit npc::move() without spending any moves.
                    turns++;
                }
            }
            // If we spun too long trying to decide what to do (without spending moves),
            // Invoke cranial detonation to prevent an infinite loop.
            if (turns == 10) {
                add_msg( _( "%s's brain explodes!" ), ( elem )->name.c_str() );
                ( elem )->die( nullptr );
            }
        }
    }
    cleanup_dead();
}

bool game::ambient_sound(int x, int y, int vol, std::string description)
{
    return sound( x, y, vol, description, true );
}

bool game::sound(int x, int y, int vol, std::string description, bool ambient)
{
    // --- Monster sound handling here ---
    // Alert all hordes
    if (vol > 20 && levz == 0) {
        int sig_power = ((vol > 140) ? 140 : vol) - 20;
        cur_om->signal_hordes(levx + (MAPSIZE / 2), levy + (MAPSIZE / 2), sig_power);
    }
    // Alert all monsters (that can hear) to the sound.
    for (int i = 0, numz = num_zombies(); i < numz; i++) {
        monster &critter = critter_tracker.find(i);
        // rl_dist() is faster than critter.has_flag() or critter.can_hear(), so we'll check it first.
        int dist = rl_dist(x, y, critter.posx(), critter.posy());
        int vol_goodhearing = vol * 2 - dist;
        if (vol_goodhearing > 0 && critter.can_hear()) {
            const bool goodhearing = critter.has_flag(MF_GOODHEARING);
            int volume = goodhearing ? vol_goodhearing : (vol - dist);
            // Error is based on volume, louder sound = less error
            if (volume > 0) {
                int max_error = 0;
                if (volume < 2) {
                    max_error = 10;
                } else if (volume < 5) {
                    max_error = 5;
                } else if (volume < 10) {
                    max_error = 3;
                } else if (volume < 20) {
                    max_error = 1;
                }

                int target_x = x + rng(-max_error, max_error);
                int target_y = y + rng(-max_error, max_error);

                int wander_turns = volume * (goodhearing ? 6 : 1);
                critter.wander_to(target_x, target_y, wander_turns);
                critter.process_trigger(MTRIG_SOUND, volume);
            }
        }
    }

    // --- Player stuff below this point ---
    int dist = rl_dist(x, y, u.posx(), u.posy());

    // Mutation/Bionic volume modifiers
    if (u.has_bionic("bio_ears")) {
        vol *= 3.5;
    }
    if (u.has_trait("PER_SLIME")) {
        // Random hearing :-/
        // (when it's working at all, see player.cpp)
        vol *= (rng(1, 2)); // changed from 0.5 to fix Mac compiling error
    }
    if (u.has_trait("BADHEARING")) {
        vol *= .5;
    }
    if (u.has_trait("GOODHEARING")) {
        vol *= 1.25;
    }
    if (u.has_trait("CANINE_EARS")) {
        vol *= 1.5;
    }
    if (u.has_trait("URSINE_EARS") || u.has_trait("FELINE_EARS")) {
        vol *= 1.25;
    }
    if (u.has_trait("LUPINE_EARS")) {
        vol *= 1.75;
    }

    // Too far away, we didn't hear it!
    if (dist > vol) {
        return false;
    }

    if (u.is_deaf()) {
        // Has to be here as well to work for stacking deafness (loud noises prolong deafness)
        if (!(u.has_bionic("bio_ears") || u.worn_with_flag("DEAF") || u.is_wearing("rm13_armor_on")) &&
            rng((vol - dist) / 2, (vol - dist)) >= 150) {
            int duration = std::min(40, (vol - dist - 130) / 4);
            u.add_effect("deaf", duration);
        }
        // We're deaf, can't hear it
        return false;
    }

    // Player volume meter includes all sounds from their tile and adjacent tiles
    if (dist <= 1) {
        u.volume = std::max( u.volume, vol );
    }

    // Check for deafness
    if (!u.has_bionic("bio_ears") && !u.is_wearing("rm13_armor_on") &&
        rng((vol - dist) / 2, (vol - dist)) >= 150) {
        int duration = (vol - dist - 130) / 4;
        u.add_effect("deaf", duration);
    }

    // See if we need to wake someone up
    if (u.has_effect("sleep")) {
        if ((!(u.has_trait("HEAVYSLEEPER") ||
               u.has_trait("HEAVYSLEEPER2")) && dice(2, 15) < vol - dist) ||
            (u.has_trait("HEAVYSLEEPER") && dice(3, 15) < vol - dist) ||
            (u.has_trait("HEAVYSLEEPER2") && dice(6, 15) < vol - dist)) {
            //Not kidding about sleep-thru-firefight
            u.wake_up();
            add_msg(m_warning, _("Something is making noise."));
        } else {
            return false;
        }
    }

    if (!ambient && (x != u.posx() || y != u.posy()) && !m.pl_sees( x, y, dist )) {
        if (u.activity.ignore_trivial != true) {
            std::string query;
            if (description != "") {
                query = string_format(_("Heard %s!"), description.c_str());
            } else {
                query = _("Heard a noise!");
            }

            if (cancel_activity_or_ignore_query(query.c_str())) {
                u.activity.ignore_trivial = true;
                for( auto activity : u.backlog ) {
                    activity.ignore_trivial = true;
                }
            }
        }
    }

    // Only print a description if it exists
    if (description != "") {
        // If it came from us, don't print a direction
        if (x == u.posx() && y == u.posy()) {
            capitalize_letter(description, 0);
            add_msg("%s", description.c_str());
        } else {
            // Else print a direction as well
            std::string direction = direction_name(direction_from(u.posx(), u.posy(), x, y));
            add_msg(m_warning, _("From the %s you hear %s"), direction.c_str(), description.c_str());
        }
    }
    return true;
}

// add_footstep will create a list of locations to draw monster
// footsteps. these will be more or less accurate depending on the
// characters hearing and how close they are
void game::add_footstep(int x, int y, int volume, int distance, monster *source)
{
    if (u.is_deaf()) {
        return;
    } else if (x == u.posx() && y == u.posy()) {
        return;
    } else if (u.sees(x, y)) {
        return;
    }
    int err_offset;
    if (volume / distance < 2) {
        err_offset = 3;
    } else if (volume / distance < 3) {
        err_offset = 2;
    } else {
        err_offset = 1;
    }
    if (u.has_bionic("bio_ears")) {
        err_offset--;
    }
    if (u.has_trait("BADHEARING")) {
        err_offset++;
    }
    if (u.has_trait("GOODHEARING") || u.has_trait("FELINE_EARS")) {
        err_offset--;
    }

    int origx = x, origy = y;
    std::vector<point> point_vector;
    for (x = origx - err_offset; x <= origx + err_offset; x++) {
        for (y = origy - err_offset; y <= origy + err_offset; y++) {
            point_vector.push_back(point(x, y));
        }
    }
    footsteps.push_back(point_vector);
    footsteps_source.push_back(source->pos());
}

void game::do_blast(const int x, const int y, const int power, const int radius, const bool fire)
{
    int dam;
    for (int i = x - radius; i <= x + radius; i++) {
        for (int j = y - radius; j <= y + radius; j++) {
            if (i == x && j == y) {
                dam = 3 * power;
            } else {
                dam = 3 * power / (rl_dist(x, y, i, j));
            }
            m.bash(i, j, dam);
            m.bash(i, j, dam); // Double up for tough doors, etc.

            int mon_hit = mon_at(i, j), npc_hit = npc_at(i, j);
            if (mon_hit != -1) {
                monster &critter = critter_tracker.find(mon_hit);
                critter.apply_damage( nullptr, bp_torso, rng( dam / 2, long( dam * 1.5 ) ) ); // TODO: player's fault?
            }

            int vpart;
            vehicle *veh = m.veh_at(i, j, vpart);
            if (veh) {
                veh->damage(vpart, dam, fire ? 2 : 1, false);
            }

            player *n = nullptr;
            if (npc_hit != -1) {
                n = active_npc[npc_hit];
            } else if( u.posx() == i && u.posy() == j ) {
                add_msg(m_bad, _("You're caught in the explosion!"));
                n = &u;
            }
            if( n != nullptr ) {
                n->deal_damage( nullptr, bp_torso, damage_instance( DT_BASH, rng( dam / 2, long( dam * 1.5 ) ) ) );
                n->deal_damage( nullptr, bp_head, damage_instance( DT_BASH, rng( dam / 3, dam ) ) );
                n->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, rng( dam / 3, dam ) ) );
                n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, rng( dam / 3, dam ) ) );
                n->deal_damage( nullptr, bp_arm_l, damage_instance( DT_BASH, rng( dam / 3, dam ) ) );
                n->deal_damage( nullptr, bp_arm_r, damage_instance( DT_BASH, rng( dam / 3, dam ) ) );
            }
            if (fire) {
                m.add_field(i, j, fd_fire, dam / 10);
            }
        }
    }
}

void game::explosion(int x, int y, int power, int shrapnel, bool fire, bool blast)
{
    int radius = int(sqrt(double(power / 4)));
    int dam;
    int noise = power * (fire ? 2 : 10);

    if (power >= 30) {
        sound(x, y, noise, _("a huge explosion!"));
    } else if (power >= 4) {
        sound(x, y, noise, _("an explosion!"));
    } else {
        sound(x, y, 3, _("a loud pop!"));
    }
    if (blast) {
        do_blast(x, y, power, radius, fire);
        // Draw the explosion
        draw_explosion(x, y, radius, c_red);
    }

    // The rest of the function is shrapnel
    if (shrapnel <= 0 || power < 4) {
        return;
    }
    int sx, sy, t, tx, ty;
    std::vector<point> traj;
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000 * OPTIONS["ANIMATION_DELAY"];
    for (int i = 0; i < shrapnel; i++) {
        sx = rng(x - 2 * radius, x + 2 * radius);
        sy = rng(y - 2 * radius, y + 2 * radius);
        if (m.sees(x, y, sx, sy, 50, t)) {
            traj = line_to(x, y, sx, sy, t);
        } else {
            traj = line_to(x, y, sx, sy, 0);
        }
        // If the randomly chosen spot is the origin, it already points there.
        // Otherwise line_to excludes the origin, so add it.
        if( sx !=x || sy != y ) {
            traj.insert( traj.begin(), point(x, y) );
        }
        for (size_t j = 0; j < traj.size(); j++) {
            dam = rng(power / 2, power * 2);
            draw_bullet(u, traj[j].x, traj[j].y, (int)j, traj, '`', ts);
            tx = traj[j].x;
            ty = traj[j].y;
            const int zid = mon_at(tx, ty);
            const int npcdex = npc_at(tx, ty);
            if (zid != -1) {
                monster &critter = critter_tracker.find(zid);
                dam -= critter.get_armor_cut(bp_torso);
                critter.apply_damage( nullptr, bp_torso, dam );
            } else if( npcdex != -1 ) {
                body_part hit = random_body_part();
                if (hit == bp_eyes || hit == bp_mouth || hit == bp_head) {
                    dam = rng(2 * dam, 5 * dam);
                } else if (hit == bp_torso) {
                    dam = rng(long(1.5 * dam), 3 * dam);
                }
                active_npc[npcdex]->deal_damage( nullptr, hit, damage_instance( DT_CUT, dam ) );
            } else if (tx == u.posx() && ty == u.posy()) {
                body_part hit = random_body_part();
                //~ %s is bodypart name in accusative.
                add_msg(m_bad, _("Shrapnel hits your %s!"), body_part_name_accusative(hit).c_str());
                u.deal_damage( nullptr, hit, damage_instance( DT_CUT, dam ) );
            } else {
                std::set<std::string> shrapnel_effects;
                m.shoot(tx, ty, dam, j == traj.size() - 1, shrapnel_effects);
            }
        }
    }
}

void game::flashbang(int x, int y, bool player_immune)
{
    draw_explosion(x, y, 8, c_white);
    int dist = rl_dist(u.posx(), u.posy(), x, y), t;
    if (dist <= 8 && !player_immune) {
        if (!u.has_bionic("bio_ears") && !u.is_wearing("rm13_armor_on")) {
            u.add_effect("deaf", 40 - dist * 4);
        }
        if (m.sees(u.posx(), u.posy(), x, y, 8, t)) {
            int flash_mod = 0;
            if (u.has_trait("PER_SLIME")) {
                if (one_in(2)) {
                    flash_mod = 3; // Yay, you weren't looking!
                }
            } else if (u.has_trait("PER_SLIME_OK")) {
                flash_mod = 8; // Just retract those and extrude fresh eyes
            } else if (u.has_bionic("bio_sunglasses") || u.is_wearing("rm13_armor_on")) {
                flash_mod = 6;
            }
            u.add_env_effect("blind", bp_eyes, (12 - flash_mod - dist) / 2, 10 - dist);
        }
    }
    for (size_t i = 0; i < num_zombies(); i++) {
        monster &critter = critter_tracker.find(i);
        dist = rl_dist(critter.posx(), critter.posy(), x, y);
        if (dist <= 4) {
            critter.add_effect("stunned", 10 - dist);
        }
        if (dist <= 8) {
            if (critter.has_flag(MF_SEES) && m.sees(critter.posx(), critter.posy(), x, y, 8, t)) {
                critter.add_effect("blind", 18 - dist);
            }
            if (critter.has_flag(MF_HEARS)) {
                critter.add_effect("deaf", 60 - dist * 4);
            }
        }
    }
    sound(x, y, 12, _("a huge boom!"));
    // TODO: Blind/deafen NPC
}

void game::shockwave(int x, int y, int radius, int force, int stun, int dam_mult,
                     bool ignore_player)
{
    draw_explosion(x, y, radius, c_blue);

    sound(x, y, force * force * dam_mult / 2, _("Crack!"));
    for (size_t i = 0; i < num_zombies(); i++) {
        monster &critter = critter_tracker.find(i);
        if (rl_dist(critter.posx(), critter.posy(), x, y) <= radius) {
            add_msg(_("%s is caught in the shockwave!"), critter.name().c_str());
            knockback(x, y, critter.posx(), critter.posy(), force, stun, dam_mult);
        }
    }
    for( auto &elem : active_npc ) {
        if( rl_dist( ( elem )->posx(), ( elem )->posx(), x, y ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), ( elem )->name.c_str() );
            knockback( x, y, ( elem )->posx(), ( elem )->posy(), force, stun, dam_mult );
        }
    }
    if (rl_dist(u.posx(), u.posy(), x, y) <= radius && !ignore_player &&
          (!u.has_trait("LEG_TENT_BRACE") || u.footwear_factor() == 1 ||
          (u.footwear_factor() == .5 && one_in(2)))) {
        add_msg(m_bad, _("You're caught in the shockwave!"));
        knockback(x, y, u.posx(), u.posy(), force, stun, dam_mult);
    }
    return;
}

/* Knockback target at (tx,ty) by force number of tiles in direction from (sx,sy) to (tx,ty)
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback(int sx, int sy, int tx, int ty, int force, int stun, int dam_mult)
{
    std::vector<point> traj;
    traj.clear();
    traj = line_to(sx, sy, tx, ty, 0);
    traj.insert(traj.begin(), point(sx, sy)); // how annoying, line_to() doesn't include the originating point!
    traj = continue_line(traj, force);
    traj.insert(traj.begin(), point(tx, ty)); // how annoying, continue_line() doesn't either!

    knockback(traj, force, stun, dam_mult);
    return;
}

/* Knockback target at traj.front() along line traj; traj should already have considered knockback distance.
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback(std::vector<point> &traj, int force, int stun, int dam_mult)
{
    (void)force; //FIXME: unused but header says it should do something
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    int tx = traj.front().x;
    int ty = traj.front().y;
    const int zid = mon_at(tx, ty);
    if (zid == -1 && npc_at(tx, ty) == -1 && (u.posx() != tx && u.posy() != ty)) {
        debugmsg(_("Nothing at (%d,%d) to knockback!"), tx, ty);
        return;
    }
    int force_remaining = 0;
    if (zid != -1) {
        monster *targ = &critter_tracker.find(zid);
        if (stun > 0) {
            targ->add_effect("stunned", stun);
            add_msg(ngettext("%s was stunned for %d turn!",
                             "%s was stunned for %d turns!", stun),
                    targ->name().c_str(), stun);
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if (m.move_cost(traj[i].x, traj[i].y) == 0) {
                targ->setpos(traj[i - 1]);
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (targ->has_effect("stunned")) {
                        targ->add_effect("stunned", force_remaining);
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    } else {
                        targ->add_effect("stunned", force_remaining);
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                    add_msg(_("%s slammed into an obstacle!"), targ->name().c_str());
                    targ->apply_damage( nullptr, bp_torso, dam_mult * force_remaining );
                }
                m.bash(traj[i].x, traj[i].y, 2 * dam_mult * force_remaining);
                break;
            } else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1 ||
                       (u.posx() == traj[i].x && u.posy() == traj[i].y)) {
                targ->setpos(traj[i - 1]);
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (targ->has_effect("stunned")) {
                        targ->add_effect("stunned", force_remaining);
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    } else {
                        targ->add_effect("stunned", force_remaining);
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                targ->name().c_str(), force_remaining);
                    }
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name().c_str());
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("%s collided with someone else and sent him flying!"),
                                targ->name().c_str());
                    } else {
                        add_msg(_("%s collided with someone else and sent her flying!"),
                                targ->name().c_str());
                    }
                } else if (u.posx() == traj.front().x && u.posy() == traj.front().y) {
                    add_msg(m_bad, _("%s collided with you and sent you flying!"), targ->name().c_str());
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            targ->setpos(traj[i]);
            if (m.has_flag("LIQUID", targ->posx(), targ->posy()) && !targ->can_drown() && !targ->is_dead()) {
                targ->die( nullptr );
                if (u.sees(*targ)) {
                    add_msg(_("The %s drowns!"), targ->name().c_str());
                }
            }
            if (!m.has_flag("LIQUID", targ->posx(), targ->posy()) && targ->has_flag(MF_AQUATIC) &&
                !targ->is_dead()) {
                targ->die( nullptr );
                if (u.sees(*targ)) {
                    add_msg(_("The %s flops around and dies!"), targ->name().c_str());
                }
            }
        }
    } else if (npc_at(tx, ty) != -1) {
        npc *targ = active_npc[npc_at(tx, ty)];
        if (stun > 0) {
            targ->add_effect("stunned", stun);
            add_msg(ngettext("%s was stunned for %d turn!",
                             "%s was stunned for %d turns!", stun),
                    targ->name.c_str(), stun);
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if (m.move_cost(traj[i].x, traj[i].y) == 0) { // oops, we hit a wall!
                targ->setx( traj[i - 1].x );
                targ->sety( traj[i - 1].y );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (targ->has_effect("stunned")) {
                        targ->add_effect("stunned", force_remaining);
                        if (targ->has_effect("stunned"))
                            add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                             "%s was stunned AGAIN for %d turns!",
                                             force_remaining),
                                    targ->name.c_str(), force_remaining);
                    } else {
                        targ->add_effect("stunned", force_remaining);
                        if (targ->has_effect("stunned"))
                            add_msg(ngettext("%s was stunned for %d turn!",
                                             "%s was stunned for %d turns!",
                                             force_remaining),
                                    targ->name.c_str(), force_remaining);
                    }
                    add_msg(_("%s took %d damage! (before armor)"), targ->name.c_str(), dam_mult * force_remaining);
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_arm_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_arm_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_torso, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_head, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_hand_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        targ->deal_damage( nullptr, bp_hand_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                }
                m.bash(traj[i].x, traj[i].y, 2 * dam_mult * force_remaining);
                break;
            } else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1 ||
                       (u.posx() == traj[i].x && u.posy() == traj[i].y)) {
                targ->setx( traj[i - 1].x );
                targ->sety( traj[i - 1].y );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (targ->has_effect("stunned")) {
                        add_msg(ngettext("%s was stunned AGAIN for %d turn!",
                                         "%s was stunned AGAIN for %d turns!",
                                         force_remaining),
                                targ->name.c_str(), force_remaining);
                    } else {
                        add_msg(ngettext("%s was stunned for %d turn!",
                                         "%s was stunned for %d turns!",
                                         force_remaining),
                                targ->name.c_str(), force_remaining);
                    }
                    targ->add_effect("effectstunned", force_remaining);
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name.c_str());
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("%s collided with someone else and sent him flying!"),
                                targ->name.c_str());
                    } else {
                        add_msg(_("%s collided with someone else and sent her flying!"),
                                targ->name.c_str());
                    }
                } else if (u.posx() == traj.front().x && u.posy() == traj.front().y &&
                           (u.has_trait("LEG_TENT_BRACE") && (!u.footwear_factor() ||
                            (u.footwear_factor() == .5 && one_in(2))))) {
                    add_msg(_("%s collided with you, and barely dislodges your tentacles!"), targ->name.c_str());
                    force_remaining = 1;
                } else if (u.posx() == traj.front().x && u.posy() == traj.front().y) {
                    add_msg(m_bad, _("%s collided with you and sent you flying!"), targ->name.c_str());
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            targ->setx( traj[i].x );
            targ->sety( traj[i].y );
        }
    } else if (u.posx() == tx && u.posy() == ty) {
        if (stun > 0) {
            u.add_effect("stunned", stun);
            add_msg(m_bad, ngettext("You were stunned for %d turn!",
                                    "You were stunned for %d turns!",
                                    stun),
                    stun);
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if (m.move_cost(traj[i].x, traj[i].y) == 0) { // oops, we hit a wall!
                u.setx( traj[i - 1].x );
                u.sety( traj[i - 1].y );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (u.has_effect("stunned")) {
                        add_msg(m_bad, ngettext("You were stunned AGAIN for %d turn!",
                                                "You were stunned AGAIN for %d turns!",
                                                force_remaining),
                                force_remaining);
                    } else {
                        add_msg(m_bad, ngettext("You were stunned for %d turn!",
                                                "You were stunned for %d turns!",
                                                force_remaining),
                                force_remaining);
                    }
                    u.add_effect("stunned", force_remaining);
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_arm_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_arm_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_torso, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_head, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_hand_l, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                    if (one_in(2)) {
                        u.deal_damage( nullptr, bp_hand_r, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                    }
                }
                m.bash(traj[i].x, traj[i].y, 2 * dam_mult * force_remaining);
                break;
            } else if (mon_at(traj[i].x, traj[i].y) != -1 || npc_at(traj[i].x, traj[i].y) != -1) {
                u.setx( traj[i - 1].x );
                u.sety( traj[i - 1].y );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (u.has_effect("stunned")) {
                        add_msg(m_bad, ngettext("You were stunned AGAIN for %d turn!",
                                                "You were stunned AGAIN for %d turns!",
                                                force_remaining),
                                force_remaining);
                    } else {
                        add_msg(m_bad, ngettext("You were stunned for %d turn!",
                                                "You were stunned for %d turns!",
                                                force_remaining),
                                force_remaining);
                    }
                    u.add_effect("stunned", force_remaining);
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front().x, traj.front().y) != -1) {
                    add_msg(_("You collided with something and sent it flying!"));
                } else if (npc_at(traj.front().x, traj.front().y) != -1) {
                    if (active_npc[npc_at(traj.front().x, traj.front().y)]->male) {
                        add_msg(_("You collided with someone and sent him flying!"));
                    } else {
                        add_msg(_("You collided with someone and sent her flying!"));
                    }
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            if (m.has_flag("LIQUID", u.posx(), u.posy()) && force_remaining < 1) {
                plswim(u.posx(), u.posy());
            } else {
                u.setx( traj[i].x );
                u.sety( traj[i].y );
            }
        }
    }
    return;
}

void game::use_computer(int x, int y)
{
    if (u.has_trait("ILLITERATE")) {
        add_msg(m_info, _("You can not read a computer screen!"));
        return;
    }

    if (u.has_trait("HYPEROPIC") && !u.is_wearing("glasses_reading")
        && !u.is_wearing("glasses_bifocal") && !u.has_effect("contacts")) {
        add_msg(m_info, _("You'll need to put on reading glasses before you can see the screen."));
        return;
    }

    computer *used = m.computer_at(x, y);

    if (used == NULL) {
        dbg(D_ERROR) << "game:use_computer: Tried to use computer at (" << x
                     << ", " << y << ") - none there";
        debugmsg("Tried to use computer at (%d, %d) - none there", x, y);
        return;
    }

    used->use();

    refresh_all();
}

void game::resonance_cascade(int x, int y)
{
    int maxglow = 100 - 5 * trig_dist(x, y, u.posx(), u.posy());
    int minglow = 60 - 5 * trig_dist(x, y, u.posx(), u.posy());
    MonsterGroupResult spawn_details;
    monster invader;
    if (minglow < 0) {
        minglow = 0;
    }
    if (maxglow > 0) {
        u.add_effect("teleglow", rng(minglow, maxglow) * 100);
    }
    int startx = (x < 8 ? 0 : x - 8), endx = (x + 8 >= SEEX * 3 ? SEEX * 3 - 1 : x + 8);
    int starty = (y < 8 ? 0 : y - 8), endy = (y + 8 >= SEEY * 3 ? SEEY * 3 - 1 : y + 8);
    for (int i = startx; i <= endx; i++) {
        for (int j = starty; j <= endy; j++) {
            switch (rng(1, 80)) {
            case 1:
            case 2:
                emp_blast(i, j);
                break;
            case 3:
            case 4:
            case 5:
                for (int k = i - 1; k <= i + 1; k++) {
                    for (int l = j - 1; l <= j + 1; l++) {
                        field_id type = fd_null;
                        switch (rng(1, 7)) {
                        case 1:
                            type = fd_blood;
                            break;
                        case 2:
                            type = fd_bile;
                            break;
                        case 3:
                        case 4:
                            type = fd_slime;
                            break;
                        case 5:
                            type = fd_fire;
                            break;
                        case 6:
                        case 7:
                            type = fd_nuke_gas;
                            break;
                        }
                        if (!one_in(3)) {
                            m.add_field(k, l, type, 3);
                        }
                    }
                }
                break;
            case  6:
            case  7:
            case  8:
            case  9:
            case 10:
                m.add_trap(i, j, tr_portal);
                break;
            case 11:
            case 12:
                m.add_trap(i, j, tr_goo);
                break;
            case 13:
            case 14:
            case 15:
                spawn_details = MonsterGroupManager::GetResultFromGroup("GROUP_NETHER");
                invader = monster(GetMType(spawn_details.name), i, j);
                add_zombie(invader);
                break;
            case 16:
            case 17:
            case 18:
                m.destroy(i, j);
                break;
            case 19:
                explosion(i, j, rng(1, 10), rng(0, 1) * rng(0, 6), one_in(4));
                break;
            }
        }
    }
}

void game::scrambler_blast(int x, int y)
{
    int mondex = mon_at(x, y);
    if (mondex != -1) {
        monster &critter = critter_tracker.find(mondex);
        if (critter.has_flag(MF_ELECTRONIC)) {
            critter.make_friendly();
        }
        add_msg(m_warning, _("The %s sparks and begins searching for a target!"), critter.name().c_str());
    }
}

void game::emp_blast(int x, int y)
{
    int rn;
    if (m.has_flag("CONSOLE", x, y)) {
        add_msg(_("The %s is rendered non-functional!"), m.tername(x, y).c_str());
        m.ter_set(x, y, t_console_broken);
        return;
    }
    // TODO: More terrain effects.
    if (m.ter(x, y) == t_card_science || m.ter(x, y) == t_card_military) {
        rn = rng(1, 100);
        if (rn > 92 || rn < 40) {
            add_msg(_("The card reader is rendered non-functional."));
            m.ter_set(x, y, t_card_reader_broken);
        }
        if (rn > 80) {
            add_msg(_("The nearby doors slide open!"));
            for (int i = -3; i <= 3; i++) {
                for (int j = -3; j <= 3; j++) {
                    if (m.ter(x + i, y + j) == t_door_metal_locked) {
                        m.ter_set(x + i, y + j, t_floor);
                    }
                }
            }
        }
        if (rn >= 40 && rn <= 80) {
            add_msg(_("Nothing happens."));
        }
    }
    int mondex = mon_at(x, y);
    if (mondex != -1) {
        monster &critter = critter_tracker.find(mondex);
        if (critter.has_flag(MF_ELECTRONIC)) {
            int deact_chance = 0;
            const auto mon_item_id = critter.type->revert_to_itype;
            switch( critter.get_size() ) {
                case MS_TINY:
                    deact_chance = 6;
                    break;
                case MS_SMALL:
                    deact_chance = 3;
                    break;
                default:
                    // Currently not used, I have no idea what chances bigger bots should have,
                    // Maybe export this to json?
                    break;
            }
            if( !mon_item_id.empty() && deact_chance != 0 && one_in( deact_chance ) ) {
                add_msg(_("The %s beeps erratically and deactivates!"), critter.name().c_str());
                m.add_item_or_charges( x, y, critter.to_item() );
                for( auto & ammodef : critter.ammo ) {
                    if( ammodef.second > 0 ) {
                        m.spawn_item( x, y, ammodef.first, 1, ammodef.second, calendar::turn );
                    }
                }
                remove_zombie(mondex);
            } else {
                add_msg(_("The EMP blast fries the %s!"), critter.name().c_str());
                int dam = dice(10, 10);
                critter.apply_damage( nullptr, bp_torso, dam );
                if( !critter.is_dead() && one_in( 6 ) ) {
                    critter.make_friendly();
                }
            }
        } else {
            add_msg(_("The %s is unaffected by the EMP blast."), critter.name().c_str());
        }
    }
    if (u.posx() == x && u.posy() == y) {
        if (u.power_level > 0) {
            add_msg(m_bad, _("The EMP blast drains your power."));
            int max_drain = (u.power_level > 1000 ? 1000 : u.power_level);
            u.charge_power(0 - rng(1 + max_drain / 3, max_drain));
        }
        // TODO: More effects?
        //e-handcuffs effects
        if (u.weapon.type->id == "e_handcuffs" && u.weapon.charges > 0){
            u.weapon.item_tags.erase("NO_UNWIELD");
            u.weapon.charges = 0;
            u.weapon.active = false;
            add_msg(m_good, _("The %s on your wrists spark briefly, then release your hands!"), u.weapon.tname().c_str());
        }
    }
    // Drain any items of their battery charge
    for( auto it = m.i_at( x, y ).begin(); it != m.i_at( x, y ).end(); ++it ) {
        if( it->is_tool() && ( dynamic_cast<it_tool *>( it->type ) )->ammo == "battery" ) {
            it->charges = 0;
        }
    }
    // TODO: Drain NPC energy reserves
}

int game::npc_at(const int x, const int y) const
{
    for (size_t i = 0; i < active_npc.size(); i++) {
        if (active_npc[i]->posx() == x && active_npc[i]->posy() == y && !active_npc[i]->is_dead()) {
            return (int)i;
        }
    }
    return -1;
}

int game::npc_by_id(const int id) const
{
    for (size_t i = 0; i < active_npc.size(); i++) {
        if (active_npc[i]->getID() == id) {
            return (int)i;
        }
    }
    return -1;
}

Creature *game::critter_at(int x, int y)
{
    const int mindex = mon_at(x, y);
    if (mindex != -1) {
        return &zombie(mindex);
    }
    if (x == u.posx() && y == u.posy()) {
        return &u;
    }
    const int nindex = npc_at(x, y);
    if (nindex != -1) {
        return active_npc[nindex];
    }
    return NULL;
}

bool game::add_zombie(monster &critter)
{
    if( !m.inbounds( critter.posx(), critter.posy() ) ) {
        dbg( D_ERROR ) << "added a critter with out-of-bounds position: " << critter.posx() << "," << critter.posy() << " - " << critter.disp_name();
    }
    return critter_tracker.add(critter);
}

size_t game::num_zombies() const
{
    return critter_tracker.size();
}

monster &game::zombie(const int idx)
{
    return critter_tracker.find(idx);
}

bool game::update_zombie_pos(const monster &critter, const int newx, const int newy)
{
    return critter_tracker.update_pos(critter, point( newx, newy ) );
}

void game::remove_zombie(const int idx)
{
    if( last_target == idx && !last_target_was_npc ) {
        last_target = -1;
    } else if( last_target > idx && !last_target_was_npc ) {
        last_target--;
    }
    critter_tracker.remove(idx);
}

void game::clear_zombies()
{
    critter_tracker.clear();
}

/**
 * Attempts to spawn a hallucination somewhere close to the player. Returns
 * false if the hallucination couldn't be spawned for whatever reason, such as
 * a monster already in the target square.
 * @return Whether or not a hallucination was successfully spawned.
 */
bool game::spawn_hallucination()
{
    monster phantasm(MonsterGenerator::generator().get_valid_hallucination());
    phantasm.hallucination = true;
    phantasm.spawn(u.posx() + rng(-10, 10), u.posy() + rng(-10, 10));

    //Don't attempt to place phantasms inside of other monsters
    if (mon_at(phantasm.posx(), phantasm.posy()) == -1) {
        return critter_tracker.add(phantasm);
    } else {
        return false;
    }
}

int game::mon_at(const int x, const int y) const
{
    return critter_tracker.mon_at( point( x, y ) );
}

int game::mon_at(point p) const
{
    return critter_tracker.mon_at(p);
}

void game::rebuild_mon_at_cache()
{
    critter_tracker.rebuild_cache();
}

bool game::is_empty(const int x, const int y)
{
    return ((m.move_cost(x, y) > 0 || m.has_flag("LIQUID", x, y)) &&
            npc_at(x, y) == -1 && mon_at(x, y) == -1 &&
            (u.posx() != x || u.posy() != y));
}

bool game::is_in_sunlight(int x, int y)
{
    return (m.is_outside(x, y) && light_level() >= 40 &&
            (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}

bool game::is_sheltered(int x, int y)
{
    bool is_inside = false;
    bool is_underground = false;
    bool is_in_vehicle = false;
    int vpart = -1;
    vehicle *veh = m.veh_at(x, y, vpart);

    if (!m.is_outside(x, y))
        is_inside = true;
    if (levz < 0)
        is_underground = true;
    if (veh && veh->is_inside(vpart))
        is_in_vehicle = true;

    if (is_inside || is_underground || is_in_vehicle)
        return true;
    else
        return false;
}

bool game::is_in_ice_lab(point location)
{
    oter_id cur_ter = cur_om->ter(location.x, location.y, levz);
    bool is_in_ice_lab = false;

    if (cur_ter == "ice_lab" || cur_ter == "ice_lab_stairs" ||
        cur_ter == "ice_lab_core" || cur_ter == "ice_lab_finale") {
        is_in_ice_lab = true;
    }

    return is_in_ice_lab;
}

bool game::revive_corpse(int x, int y, int n)
{
    if ((int)m.i_at(x, y).size() <= n) {
        debugmsg("Tried to revive a non-existent corpse! (%d, %d), #%d of %d", x, y, n, m.i_at(x,
                 y).size());
        return false;
    }
    if( !revive_corpse( x, y, &m.i_at(x, y)[n] ) ) {
        return false;
    }
    m.i_rem(x, y, n);
    return true;
}

bool game::revive_corpse(int x, int y, item *it)
{
    if (it == NULL || !it->is_corpse()) {
        debugmsg("Tried to revive a non-corpse.");
        return false;
    }
    if (critter_at(x, y) != NULL) {
        // Someone is in the way, try again later
        return false;
    }
    int burnt_penalty = it->burnt;
    monster critter(it->get_mtype(), x, y);
    critter.set_speed_base( int(critter.get_speed_base() * 0.8) - (burnt_penalty / 2) );
    critter.hp = int(critter.hp * 0.7) - burnt_penalty;
    if (it->damage > 0) {
        critter.set_speed_base( critter.get_speed_base() / (it->damage + 1) );
        critter.hp /= it->damage + 1;
    }
    critter.no_extra_death_drops = true;

    if (it->get_var( "zlave" ) == "zlave"){
        critter.add_effect("pacified", 1, num_bp, true);
        critter.add_effect("pet", 1, num_bp, true);
    }

    add_zombie(critter);
    return true;
}

void game::open()
{
    int openx, openy;
    if (!choose_adjacent_highlight(_("Open where?"), openx, openy, ACTION_OPEN)) {
        return;
    }

    u.moves -= 100;

    int vpart;
    vehicle *veh = m.veh_at(openx, openy, vpart);

    if (veh) {
        int openable = veh->next_part_to_open(vpart);
        if (openable >= 0) {
            const vehicle *player_veh = m.veh_at(u.posx(), u.posy());
            bool outside = !player_veh || player_veh != veh;
            if (!outside) {
                veh->open(openable);
            } else {
                // Outside means we check if there's anything in that tile outside-openable.
                // If there is, we open everything on tile. This means opening a closed,
                // curtained door from outside is possible, but it will magically open the
                // curtains as well.
                int outside_openable = veh->next_part_to_open(vpart, true);
                if (outside_openable == -1) {
                    const char *name = veh->part_info(openable).name.c_str();
                    add_msg(m_info, _("That %s can only opened from the inside."), name);
                    u.moves += 100;
                } else {
                    veh->open_all_at(openable);
                }
            }
        } else {
            // If there are any OPENABLE parts here, they must be already open
            int already_open = veh->part_with_feature(vpart, "OPENABLE");
            if (already_open >= 0) {
                const char *name = veh->part_info(already_open).name.c_str();
                add_msg(m_info, _("That %s is already open."), name);
            }
            u.moves += 100;
        }
        return;
    }

    bool didit = m.open_door(openx, openy, !m.is_outside(u.posx(), u.posy()));

    if (!didit) {
        const std::string terid = m.get_ter(openx, openy);
        if (terid.find("t_door") != std::string::npos) {
            if (terid.find("_locked") != std::string::npos) {
                add_msg(m_info, _("The door is locked!"));
                return;
            } else if (!termap[terid].close.empty() && termap[terid].close != "t_null") {
                // if the following message appears unexpectedly, the prior check was for t_door_o
                add_msg(m_info, _("That door is already open."));
                u.moves += 100;
                return;
            }
        }
        add_msg(m_info, _("No door there."));
        u.moves += 100;
    }
}

void game::close(int closex, int closey)
{
    if (closex == -1) {
        if (!choose_adjacent_highlight(_("Close where?"), closex, closey, ACTION_CLOSE)) {
            return;
        }
    }

    bool didit = false;
    const bool inside = !m.is_outside(u.posx(), u.posy());

    auto items_in_way = m.i_at(closex, closey);
    int vpart;
    vehicle *veh = m.veh_at(closex, closey, vpart);
    int zid = mon_at(closex, closey);
    if (zid != -1) {
        monster &critter = critter_tracker.find(zid);
        add_msg(m_info, _("There's a %s in the way!"), critter.name().c_str());
    } else if (veh) {
        int openable = veh->next_part_to_close(vpart);
        if (openable >= 0) {
            const char *name = veh->part_info(openable).name.c_str();
            if (veh->part_info(openable).has_flag("OPENCLOSE_INSIDE")) {
                const vehicle *in_veh = m.veh_at(u.posx(), u.posy());
                if (!in_veh || in_veh != veh) {
                    add_msg(m_info, _("That %s can only closed from the inside."), name);
                    return;
                }
            }
            if (veh->parts[openable].open) {
                veh->close(openable);
                didit = true;
            } else {
                add_msg(m_info, _("That %s is already closed."), name);
            }
        }
    } else if (closex == u.posx() && closey == u.posy()) {
        add_msg(m_info, _("There's some buffoon in the way!"));
    } else if (m.has_furn(closex, closey) && m.furn_at(closex, closey).close.empty()) {
        add_msg(m_info, _("There's a %s in the way!"), m.furnname(closex, closey).c_str());
    } else if (!m.close_door(closex, closey, inside, true)) {
        // ^^ That checks if the PC could close something there, it
        // does not actually do anything.
        std::string door_name;
        if (m.has_furn(closex, closey)) {
            door_name = furnlist[m.furn(closex, closey)].name;
        } else {
            door_name = terlist[m.ter(closex, closey)].name;
        }
        // Print a message that we either can not close whatever is there
        // or (if we're outside) that we can only close it from the
        // inside.
        if (!inside && m.close_door(closex, closey, true, true)) {
            add_msg(m_info, _("You cannot close the %s from outside. You must be inside the building."),
                    door_name.c_str());
        } else {
            add_msg(m_info, _("You cannot close the %s."), door_name.c_str());
        }
    } else {
        // Scoot up to 10 volume of items out of the way, only counting items that are vol >= 1.
        if (m.furn(closex, closey) != f_safe_o && !items_in_way.empty()) {
            int total_item_volume = 0;
            if (items_in_way.size() > 10) {
                add_msg(m_info, _("Too many items to push out of the way!"));
                return;
            }
            for( auto &elem : items_in_way ) {
                // Don't even count tiny items.
                if( elem.volume() < 1 ) {
                    continue;
                }
                if( elem.volume() > 10 ) {
                    add_msg( m_info, _( "There's a %s in the way that is too big to just nudge out "
                                        "of the way." ),
                             elem.tname().c_str() );
                    return;
                }
                total_item_volume += elem.volume();
                if (total_item_volume > 10) {
                    add_msg(m_info, _("There is too much stuff in the way."));
                    return;
                }
            }
            add_msg(_("You push %s out of the way."), items_in_way.size() == 1 ?
                    items_in_way[0].tname().c_str() : _("some stuff"));
            u.moves -= items_in_way.size() * 10;
        }

        didit = m.close_door(closex, closey, inside, false);
        if (didit && m.has_flag_ter_or_furn("NOITEM", closex, closey)) {
            // Just plopping items back on their origin square will displace them to adjacent squares
            // since the door is closed now.
            for( auto &elem : items_in_way ) {
                m.add_item_or_charges( closex, closey, elem );
            }
            m.i_clear(closex, closey);
        }
    }

    if (didit) {
        u.moves -= 90;
    }
}

void game::smash()
{
    const int move_cost = int(u.weapon.is_null() ? 80 : u.weapon.attack_time() * 0.8);
    bool didit = false;
    int smashskill = int(u.str_cur + u.weapon.type->melee_dam);
    int smashx, smashy;

    if (!choose_adjacent(_("Smash where?"), smashx, smashy)) {
        return;
    }

    if( m.get_field( point( smashx, smashy ), fd_web ) != nullptr ) {
        m.remove_field( smashx, smashy, fd_web );
        sound( smashx, smashy, 2, "" );
        add_msg( m_info, _( "You brush aside some webs." ) );
        u.moves -= 100;
        return;
    }
    static const int full_pulp_threshold = 4;
    for (auto it = m.i_at(smashx, smashy).begin(); it != m.i_at(smashx, smashy).end(); ++it) {
        if (it->is_corpse() && it->damage < full_pulp_threshold) {
            // do activity forever. ACT_PULP stops itself
            u.assign_activity(ACT_PULP, INT_MAX, 0);
            u.activity.placement = point(smashx, smashy);
            return; // don't smash terrain if we've smashed a corpse
        }
    }
    didit = m.bash(smashx, smashy, smashskill).first;
    if (didit) {
        u.handle_melee_wear();
        u.moves -= move_cost;
        if (u.skillLevel("melee") == 0) {
            u.practice("melee", rng(0, 1) * rng(0, 1));
        }
        if (u.weapon.made_of("glass") &&
            rng(0, u.weapon.volume() + 3) < u.weapon.volume()) {
            add_msg(m_bad, _("Your %s shatters!"), u.weapon.tname().c_str());
            for( auto &elem : u.weapon.contents ) {
                m.add_item_or_charges( u.posx(), u.posy(), elem );
            }
            sound(u.posx(), u.posy(), 24, "");
            u.deal_damage( nullptr, bp_hand_r, damage_instance( DT_CUT, rng( 0, u.weapon.volume() ) ) );
            if (u.weapon.volume() > 20) {
                // Hurt left arm too, if it was big
                u.deal_damage( nullptr, bp_hand_l, damage_instance( DT_CUT, rng( 0, long( u.weapon.volume() * .5 ) ) ) );
            }
            u.remove_weapon();
        }
        if (smashskill < m.bash_resistance(smashx, smashy) && one_in(10)) {
            if (m.has_furn(smashx, smashy) && m.furn_at(smashx, smashy).bash.str_min != -1) {
                // %s is the smashed furniture
                add_msg(m_neutral, _("You don't seem to be damaging the %s."), m.furnname(smashx, smashy).c_str());
            } else {
                // %s is the smashed terrain
                add_msg(m_neutral, _("You don't seem to be damaging the %s."), m.tername(smashx, smashy).c_str());
            }
        }
    } else {
        add_msg(_("There's nothing there to smash!"));
    }
}

void game::activity_on_turn_pulp()
{
    const int smashx = u.activity.placement.x;
    const int smashy = u.activity.placement.y;
    static const int full_pulp_threshold = 4;
    const int move_cost = int(u.weapon.is_null() ? 80 : u.weapon.attack_time() * 0.8);

    // numbers logic: a str 8 character with a butcher knife (4 bash, 18 cut)
    // should have at least a 50% chance of damaging an intact zombie corpse (75 volume).
    // a str 8 character with a baseball bat (28 bash, 0 cut) should have around a 25% chance.

    int cut_power = u.weapon.type->melee_cut;
    // stabbing weapons are a lot less effective at pulping
    if (u.weapon.has_flag("STAB") || u.weapon.has_flag("SPEAR")) {
        cut_power /= 2;
    }
    double pulp_power = sqrt((double)(u.str_cur + u.weapon.type->melee_dam)) *
                        sqrt((double)(cut_power + 1));
    pulp_power = std::min(pulp_power, (double)u.str_cur);
    pulp_power *= 20; // constant multiplier to get the chance right
    int moves = 0;
    int &num_corpses = u.activity.index; // use this to collect how many corpse are pulped
    auto corpse_pile = m.i_at(smashx, smashy);
    for( auto corpse = corpse_pile.begin(); corpse != corpse_pile.end(); ++corpse ) {
        if (!(corpse->is_corpse() && corpse->damage < full_pulp_threshold)) {
            continue; // no corpse or already pulped
        }
        int damage = pulp_power / corpse->volume();
        //Determine corpse's blood type.
        field_id type_blood = corpse->get_mtype()->bloodType();
        do {
            moves += move_cost;
            // Increase damage as we keep smashing,
            // to insure that we eventually smash the target.
            if( x_in_y(pulp_power, corpse->volume()) ) {
                corpse->damage++;
                u.handle_melee_wear();
            }
            // Splatter some blood around
            if (type_blood != fd_null) {
                for (int x = smashx - 1; x <= smashx + 1; x++) {
                    for (int y = smashy - 1; y <= smashy + 1; y++) {
                        if (!one_in(damage + 1) && type_blood != fd_null) {
                            m.add_field(x, y, type_blood, 1);
                        }
                    }
                }
            }
            if( corpse->damage >= full_pulp_threshold ) {
                corpse->damage = full_pulp_threshold;
                corpse->active = false;
                num_corpses++;
            }
            if (moves >= u.moves) {
                // enough for this turn;
                u.moves -= moves;
                return;
            }
        } while( corpse->damage < full_pulp_threshold );
    }
    // If we reach this, all corpses have been pulped, finish the activity
    u.activity.moves_left = 0;
    // TODO: Factor in how long it took to do the smashing.
    add_msg(ngettext("The corpse is thoroughly pulped.",
                     "The corpses are thoroughly pulped.", num_corpses));
}

void game::use_item(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_activatable(_("Use item:"));
    }

    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }
    refresh_all();
    u.use(pos);
    u.invalidate_crafting_inventory();
}

void game::use_wielded_item()
{
    u.use_wielded();
}

bool game::vehicle_near()
{
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (m.veh_at(u.posx() + dx, u.posy() + dy)) {
                return true;
            }
        }
    }
    return false;
}

bool game::refill_vehicle_part(vehicle &veh, vehicle_part *part, bool test)
{
    vpart_info part_info = vehicle_part_types[part->id];
    if (!part_info.has_flag("FUEL_TANK")) {
        return false;
    }
    item *it = nullptr; // the container or the fuel item,
    item *p_itm = nullptr; // always the actual fuel item
    long min_charges = -1;
    bool in_container = false;

    std::string ftype = part_info.fuel_type;
    itype_id itid = default_ammo(ftype);
    if (u.weapon.is_container() && !u.weapon.contents.empty() &&
        u.weapon.contents[0].type->id == itid) {
        it = &u.weapon;
        p_itm = &u.weapon.contents[0];
        min_charges = u.weapon.contents[0].charges;
        in_container = true;
    } else if (u.weapon.type->id == itid) {
        it = &u.weapon;
        p_itm = it;
        min_charges = u.weapon.charges;
    } else {
        it = &u.inv.item_or_container(itid);
        if (!it->is_null()) {
            if (it->type->id == itid) {
                p_itm = it;
            } else {
                //ah, must be a container of the thing
                p_itm = &(it->contents[0]);
                in_container = true;
            }
            min_charges = p_itm->charges;
        }
    }
    // Check for p_itm->type->id == itid is already done above
    if( p_itm == nullptr || it->is_null()) {
        return false;
    } else if (test) {
        return true;
    }

    int fuel_per_charge = 1; //default for gasoline
    if (ftype == "plutonium") {
        fuel_per_charge = 1000;
    } else if (ftype == "plasma") {
        fuel_per_charge = 100;
    }
    int max_fuel = part_info.size;
    int charge_difference = (max_fuel - part->amount) / fuel_per_charge;
    if (charge_difference < 1) {
        charge_difference = 1;
    }
    bool rem_itm = min_charges <= charge_difference;
    long used_charges = rem_itm ? min_charges : charge_difference;
    part->amount += used_charges * fuel_per_charge;
    if (part->amount > max_fuel) {
        part->amount = max_fuel;
    }

    if (ftype == "battery") {
        add_msg(_("You recharge %s's battery."), veh.name.c_str());
        if (part->amount == max_fuel) {
            add_msg(m_good, _("The battery is fully charged."));
        }
    } else if (ftype == "gasoline" || ftype == "diesel") {
        add_msg(_("You refill %s's fuel tank."), veh.name.c_str());
        if (part->amount == max_fuel) {
            add_msg(m_good, _("The tank is full."));
        }
    } else if (ftype == "plutonium") {
        add_msg(_("You refill %s's reactor."), veh.name.c_str());
        if (part->amount == max_fuel) {
            add_msg(m_good, _("The reactor is full."));
        }
    }

    p_itm->charges -= used_charges;
    if (rem_itm) {
        if (in_container) {
            it->contents.erase(it->contents.begin());
        } else if (&u.weapon == it) {
            u.remove_weapon();
        } else {
            u.inv.remove_item(u.get_item_position(it));
        }
    }
    return true;
}

bool game::pl_refill_vehicle(vehicle &veh, int part, bool test)
{
    return refill_vehicle_part(veh, &veh.parts[part], test);
}

void game::handbrake()
{
    vehicle *veh = m.veh_at(u.posx(), u.posy());
    if (!veh) {
        return;
    }
    add_msg(_("You pull a handbrake."));
    veh->cruise_velocity = 0;
    if (veh->last_turn != 0 && rng(15, 60) * 100 < abs(veh->velocity)) {
        veh->skidding = true;
        add_msg(m_warning, _("You lose control of %s."), veh->name.c_str());
        veh->turn(veh->last_turn > 0 ? 60 : -60);
    } else {
        int braking_power = abs( veh->velocity ) / 2 + 10 * 100;
        if( abs( veh->velocity ) < braking_power ) {
            veh->stop();
        } else {
            int sgn = veh->velocity > 0 ? 1 : -1;
            veh->velocity = sgn * ( abs( veh->velocity ) - braking_power );
        }
    }
    u.moves = 0;
}

void game::exam_vehicle(vehicle &veh, int examx, int examy, int cx, int cy)
{
    (void)examx;
    (void)examy; // not currently used
    veh_interact vehint;
    vehint.ddx = cx;
    vehint.ddy = cy;
    vehint.exec(&veh);
    if (vehint.sel_cmd != ' ') {
        int time = 200;
        int skill = u.skillLevel("mechanics");
        int diff = 1;
        if (vehint.sel_vpart_info != NULL) {
            diff = vehint.sel_vpart_info->difficulty + 3;
        }
        int setup = (calendar::turn == veh.last_repair_turn ? 0 : 1);
        int setuptime = std::max(setup * 3000, setup * 6000 - skill * 400);
        int dmg = 1000;
        if (vehint.sel_cmd == 'r') {
            dmg = 1000 - vehint.sel_vehicle_part->hp * 1000 / vehint.sel_vpart_info->durability;
        }
        int mintime = 300 + diff * dmg;
        // sel_cmd = Install Repair reFill remOve Siphon Drainwater Changetire reName
        // Note that even if letters are remapped in keybindings sel_cmd will still use the above.
        // Stored in activity.index and used in the complete_vehicle() callback to finish task.
        switch (vehint.sel_cmd) {
        case 'i':
            time = setuptime + std::max(mintime, 5000 * diff - skill * 2500);;
        case 'r':
            time = setuptime + std::max(mintime, (8 * diff - skill * 4) * dmg);;
        case 'o':
            time = setuptime + std::max(mintime, 4000 * diff - skill * 2000);;
        case 'c':
            time = setuptime + std::max(mintime, 6000 * diff - skill * 4000);;
        }
        u.activity = player_activity(ACT_VEHICLE, time, (int)vehint.sel_cmd, INT_MIN, "");
        u.activity.values.push_back(veh.global_x());    // values[0]
        u.activity.values.push_back(veh.global_y());    // values[1]
        u.activity.values.push_back(vehint.ddx);   // values[2]
        u.activity.values.push_back(vehint.ddy);   // values[3]
        u.activity.values.push_back(-vehint.ddx);   // values[4]
        u.activity.values.push_back(-vehint.ddy);   // values[5]
        // values[6]
        u.activity.values.push_back(veh.index_of_part(vehint.sel_vehicle_part));
        u.activity.values.push_back(vehint.sel_type); // int. might make bitmask
        if (vehint.sel_vpart_info != NULL) {
            u.activity.str_values.push_back(vehint.sel_vpart_info->id);
        } else {
            u.activity.str_values.push_back("null");
        }
        u.moves = 0;
    }
    refresh_all();
}

bool game::forced_gate_closing(int x, int y, ter_id door_type, int bash_dmg)
{
    const std::string &door_name = terlist[door_type].name;
    int kbx = x; // Used when player/monsters are knocked back
    int kby = y; // and when moving items out of the way
    for (int i = 0; i < 20; i++) {
        const int x_ = x + rng(-1, +1);
        const int y_ = y + rng(-1, +1);
        if (is_empty(x_, y_)) {
            // invert direction, as game::knockback needs
            // the source of the force that knocks back
            kbx = -x_ + x + x;
            kby = -y_ + y + y;
            break;
        }
    }
    const bool can_see = u.sees(x, y);
    player *npc_or_player = NULL;
    if (x == u.pos().x && y == u.pos().y) {
        npc_or_player = &u;
    } else {
        const int cindex = npc_at(x, y);
        if (cindex != -1) {
            npc_or_player = active_npc[cindex];
        }
    }
    if (npc_or_player != NULL) {
        if (bash_dmg <= 0) {
            return false;
        }
        if (npc_or_player->is_npc() && can_see) {
            add_msg(_("The %s hits the %s."), door_name.c_str(), npc_or_player->name.c_str());
        } else if (npc_or_player->is_player()) {
            add_msg(m_bad, _("The %s hits you."), door_name.c_str());
        }
        // TODO: make the npc angry?
        npc_or_player->hitall(bash_dmg);
        knockback(kbx, kby, x, y, std::max(1, bash_dmg / 10), -1, 1);
        // TODO: perhaps damage/destroy the gate
        // if the npc was really big?
    }
    const int cindex = mon_at(x, y);
    if (cindex != -1) {
        if (bash_dmg <= 0) {
            return false;
        }
        if (can_see) {
            add_msg(_("The %s hits the %s."), door_name.c_str(), zombie(cindex).name().c_str());
        }
        monster &critter = zombie( cindex );
        if (critter.type->size <= MS_SMALL || critter.has_flag(MF_VERMIN)) {
            critter.die_in_explosion( nullptr );
        } else {
            critter.apply_damage( nullptr, bp_torso, bash_dmg );
        }
        if( !critter.is_dead() && critter.type->size >= MS_HUGE ) {
            // big critters simply prevent the gate from closing
            // TODO: perhaps damage/destroy the gate
            // if the critter was really big?
            return false;
        }
        if( !critter.is_dead() ) {
            // Still alive? Move the critter away so the door can close
            knockback(kbx, kby, x, y, std::max(1, bash_dmg / 10), -1, 1);
            if (mon_at(x, y) != -1) {
                return false;
            }
        }
    }
    int vpart = -1;
    vehicle *veh = m.veh_at(x, y, vpart);
    if (veh != NULL) {
        if (bash_dmg <= 0) {
            return false;
        }
        veh->damage(vpart, bash_dmg);
        if (m.veh_at(x, y, vpart) != NULL) {
            // Check again in case all parts at the door tile
            // have been destroyed, if there is still a vehicle
            // there, the door can not be closed
            return false;
        }
    }
    if (bash_dmg < 0 && !m.i_at(x, y).empty()) {
        return false;
    }
    if(bash_dmg == 0) {
        for( auto &elem : m.i_at( x, y ) ) {
            if( elem.made_of( LIQUID ) ) {
                // Liquids are OK, will be destroyed later
                continue;
            } else if( elem.volume() <= 0 ) {
                // Dito for small items, will be moved away
                continue;
            }
            // Everything else prevents the door from closing
            return false;
        }
    }

    m.ter_set(x, y, door_type);
    if (m.has_flag("NOITEM", x, y)) {
        auto items = m.i_at(x, y);
        while (!items.empty()) {
            if (items[0].made_of(LIQUID)) {
                m.i_rem( x, y, 0 );
                continue;
            }
            if (items[0].made_of("glass") && one_in(2)) {
                if (can_see) {
                    add_msg(m_warning, _("A %s shatters!"), items[0].tname().c_str());
                } else {
                    add_msg(m_warning, _("Something shatters!"));
                }
                m.i_rem( x, y, 0 );
                continue;
            }
            m.add_item_or_charges(kbx, kby, items[0]);
            m.i_rem( x, y, 0 );
        }
    }
    return true;
}

// A gate handle is adjacent to a wall section, and next to that wall section on one side or
// another is the gate.  There may be a handle on the other side, but this is optional.
// The gate continues until it reaches a non-floor tile, so they can be arbitrary length.
//
//   |  !|!  -++-++-  !|++++-
//   +   +      !      +
//   +   +   -++-++-   +
//   +   +             +
//   +   +   !|++++-   +
//  !|   |!        !   |
//
// The terrain type of the handle is passed in, and that is used to determine the type of
// the wall and gate.
void game::open_gate(const int examx, const int examy, const ter_id handle_type)
{

    ter_id v_wall_type;
    ter_id h_wall_type;
    ter_id door_type;
    ter_id floor_type;
    const char *pull_message;
    const char *open_message;
    const char *close_message;
    int bash_dmg;

    if (handle_type == t_gates_mech_control) {
        v_wall_type = t_wall_v;
        h_wall_type = t_wall_h;
        door_type = t_door_metal_locked;
        floor_type = t_floor;
        pull_message = _("You turn the handle...");
        open_message = _("The gate is opened!");
        close_message = _("The gate is closed!");
        bash_dmg = 40;
    } else if (handle_type == t_gates_control_concrete) {
        v_wall_type = t_concrete_v;
        h_wall_type = t_concrete_h;
        door_type = t_door_metal_locked;
        floor_type = t_floor;
        pull_message = _("You turn the handle...");
        open_message = _("The gate is opened!");
        close_message = _("The gate is closed!");
        bash_dmg = 40;

    } else if (handle_type == t_barndoor) {
        v_wall_type = t_wall_wood;
        h_wall_type = t_wall_wood;
        door_type = t_door_metal_locked;
        floor_type = t_dirtfloor;
        pull_message = _("You pull the rope...");
        open_message = _("The barn doors opened!");
        close_message = _("The barn doors closed!");
        bash_dmg = 40;

    } else if (handle_type == t_palisade_pulley) {
        v_wall_type = t_palisade;
        h_wall_type = t_palisade;
        door_type = t_palisade_gate;
        floor_type = t_palisade_gate_o;
        pull_message = _("You pull the rope...");
        open_message = _("The palisade gate swings open!");
        close_message = _("The palisade gate swings closed with a crash!");
        bash_dmg = 30;
    } else {
        return;
    }

    add_msg(pull_message);
    u.moves -= 900;

    bool open = false;
    bool close = false;

    for (int wall_x = -1; wall_x <= 1; wall_x++) {
        for (int wall_y = -1; wall_y <= 1; wall_y++) {
            for (int gate_x = -1; gate_x <= 1; gate_x++) {
                for (int gate_y = -1; gate_y <= 1; gate_y++) {
                    if ((wall_x + wall_y == 1 || wall_x + wall_y == -1) &&
                        // make sure wall not diagonally opposite to handle
                        (gate_x + gate_y == 1 || gate_x + gate_y == -1) &&  // same for gate direction
                        ((wall_y != 0 && (m.ter(examx + wall_x, examy + wall_y) == h_wall_type)) ||
                         //horizontal orientation of the gate
                         (wall_x != 0 &&
                          (m.ter(examx + wall_x, examy + wall_y) == v_wall_type)))) { //vertical orientation of the gate

                        int cur_x = examx + wall_x + gate_x;
                        int cur_y = examy + wall_y + gate_y;

                        if (!close &&
                            (m.ter(examx + wall_x + gate_x, examy + wall_y + gate_y) == door_type)) {  //opening the gate...
                            open = true;
                            while (m.ter(cur_x, cur_y) == door_type) {
                                m.ter_set(cur_x, cur_y, floor_type);
                                cur_x = cur_x + gate_x;
                                cur_y = cur_y + gate_y;
                            }
                        }

                        if (!open &&
                            (m.ter(examx + wall_x + gate_x, examy + wall_y + gate_y) == floor_type)) {  //closing the gate...
                            close = true;
                            while (m.ter(cur_x, cur_y) == floor_type) {
                                forced_gate_closing(cur_x, cur_y, door_type, bash_dmg);
                                cur_x = cur_x + gate_x;
                                cur_y = cur_y + gate_y;
                            }
                        }
                    }
                }
            }
        }
    }

    if (open) {
        add_msg(open_message);
    } else if (close) {
        add_msg(close_message);
    } else {
        add_msg(_("Nothing happens."));
    }
}

void game::moving_vehicle_dismount(int tox, int toy)
{
    int vpart;
    vehicle *veh = m.veh_at(u.posx(), u.posy(), vpart);
    if (!veh) {
        debugmsg("Tried to exit non-existent vehicle.");
        return;
    }
    if (u.posx() == tox && u.posy() == toy) {
        debugmsg("Need somewhere to dismount towards.");
        return;
    }
    int d = (45 * (direction_from(u.posx(), u.posy(), tox, toy)) - 90) % 360;
    add_msg(_("You dive from the %s."), veh->name.c_str());
    m.unboard_vehicle(u.posx(), u.posy());
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_creature( &u, d, 30, true );
    // Hit the ground according to vehicle speed
    if (!m.has_flag("SWIMMABLE", u.posx(), u.posy())) {
        if (veh->velocity > 0) {
            fling_creature(&u, veh->face.dir(), veh->velocity / (float)100);
        } else {
            fling_creature(&u, veh->face.dir() + 180, -(veh->velocity) / (float)100);
        }
    }
    return;
}

void game::control_vehicle()
{
    int veh_part = -1;
    vehicle *veh = remoteveh();
    if( veh == nullptr ) {
        veh = m.veh_at(u.posx(), u.posy(), veh_part);
    }

    if( veh != nullptr && veh->player_in_control( &u ) ) {
        veh->use_controls();
    } else if (veh && veh->part_with_feature(veh_part, "CONTROLS") >= 0
               && u.in_vehicle) {
        if (veh->interact_vehicle_locked()){
            u.controlling_vehicle = true;
            add_msg(_("You take control of the %s."), veh->name.c_str());
            if (!veh->engine_on) {
                veh->start_engine();
            }
        }
    } else {
        int examx, examy;
        if (!choose_adjacent(_("Control vehicle where?"), examx, examy)) {
            return;
        }
        veh = m.veh_at(examx, examy, veh_part);
        if (!veh) {
            add_msg(_("No vehicle there."));
            return;
        }
        if (veh->part_with_feature(veh_part, "CONTROLS") < 0) {
            add_msg(m_info, _("No controls there."));
            return;
        }
        if (veh->interact_vehicle_locked()){
            veh->use_controls();
        }
    }
}

bool pet_menu(monster *z)
{
    enum choices {
        cancel,
        swap_pos,
        push_zlave,
        attach_bag,
        drop_all,
        give_items,
        pheromone,
        rope
    };

    uimenu amenu;

    std::string pet_name = "dog";
    if( z->type->in_species("ZOMBIE") ) {
        pet_name = "zombie slave";
    }

    amenu.selected = 0;
    amenu.text = string_format(_("What to do with your %s?"), pet_name.c_str());
    amenu.addentry(cancel, true, 'q', _("Cancel"));

    amenu.addentry(swap_pos, true, 's', _("Swap positions"));
    amenu.addentry(push_zlave, true, 'p', _("Push %s"), pet_name.c_str());

    if (z->has_effect("has_bag")) {
        amenu.addentry(give_items, true, 'g', _("Place items into bag"));
        amenu.addentry(drop_all, true, 'd', _("Drop all items"));
    } else {
        amenu.addentry(attach_bag, true, 'b', _("Attach bag"));
    }

    if (z->has_effect("tied")) {
        amenu.addentry(rope, true, 'r', _("Untie"));
    } else {
        if (g->u.has_amount("rope_6", 1)) {
            amenu.addentry(rope, true, 'r', _("Tie"));
        } else {
            amenu.addentry(rope, false, 'r', _("You need a short rope"));
        }
    }

    if( z->type->in_species("ZOMBIE") ) {
        amenu.addentry(pheromone, true, 't', _("Tear out pheromone ball"));
    }

    amenu.query();
    int choice = amenu.ret;

    if (cancel == choice) {
        return false;
    }

    if (swap_pos == choice) {
        g->u.moves -= 150;

        if (!one_in((g->u.str_cur + g->u.dex_cur) / 6)) {

            bool t = z->has_effect("tied");
            if (t) {
                z->remove_effect("tied");
            }

            int x = z->posx(), y = z->posy();
            z->move_to(g->u.posx(), g->u.posy(), true);
            g->u.setx( x );
            g->u.sety( y );

            if (t) {
                z->add_effect("tied", 1, num_bp, true);
            }

            add_msg(_("You swap positions with your %s."), pet_name.c_str());

            return true;
        } else {
            add_msg(_("You fail to budge your %s!"), pet_name.c_str());

            return true;
        }
    }

    if (push_zlave == choice) {

        g->u.moves -= 30;

        if (!one_in(g->u.str_cur)) {
            add_msg(_("You pushed the %s."), pet_name.c_str());
        } else {
            add_msg(_("You pushed the %s, but it resisted."), pet_name.c_str());
            return true;
        }

        int deltax = z->posx() - g->u.posx(), deltay = z->posy() - g->u.posy();

        z->move_to(z->posx() + deltax, z->posy() + deltay);

        return true;
    }

    if (attach_bag == choice) {
        int pos = g->inv_type(_("Bag item:"), IC_ARMOR);
        if (pos == INT_MIN) {
            add_msg(_("Never mind."));
            return true;
        }

        item *it = &g->u.i_at(pos);

        if (!it->is_armor()) {
            add_msg(_("This is not a bag!"));
            return true;
        }

        if( it->get_storage() <= 0 ) {
            add_msg(_("This is not a bag!"));
            return true;
        }

        z->add_item(*it);

        add_msg(_("You mount the %s on your %s, ready to store gear."),
                it->display_name().c_str(),  pet_name.c_str());

        g->u.i_rem(pos);

        z->add_effect("has_bag", 1, num_bp, true);

        g->u.moves -= 200;

        return true;
    }

    if (drop_all == choice) {
        for (std::vector<item>::iterator it = z->inv.begin();
             it != z->inv.end(); ++it) {
            g->m.add_item_or_charges(z->posx(), z->posy(), *it);
        }

        z->inv.clear();

        z->remove_effect("has_bag");

        add_msg(_("You dump the contents of the %s's bag on the ground."), pet_name.c_str());

        g->u.moves -= 200;
        return true;
    }

    if (give_items == choice) {

        if (z->inv.empty()) {
            add_msg(_("There is no container on your %s to put things in!"), pet_name.c_str());
            return true;
        }

        item *it = &z->inv[0];

        if (!it->is_armor()) {
            add_msg(_("There is no container on your %s to put things in!"), pet_name.c_str());
            return true;
        }

        int max_cap = it->get_storage();
        int max_weight = z->weight_capacity() - it->weight();

        if (z->inv.size() > 1) {
            for (auto &i : z->inv) {
                max_cap -= i.volume();
                max_weight -= i.weight();
            }
        }

        if (max_weight <= 0) {
            add_msg(_("%s is overburdened. You can't transfer your %s"),
                    pet_name.c_str(), it->tname(1).c_str());
            return true;
        }
        if (max_cap <= 0) {
            add_msg(_("There's no room in your %s's %s for that, it's too bulky!"),
                    pet_name.c_str(), it->tname(1).c_str() );
            return true;
        }

        bool success = g->make_drop_activity( ACT_STASH, z->pos() );
        if( success ) {
            z->add_effect("controlled", 5);
        }
        return success;
    }

    if (pheromone == choice && query_yn(_("Really kill the zombie slave?"))) {

        z->apply_damage( &g->u, bp_torso, 100 ); // damage the monster (and its corpse)
        z->die(&g->u); // and make sure it's really dead

        g->u.moves -= 150;

        if (!one_in(3)) {
            g->u.add_msg_if_player(_("You tear out the pheromone ball from the zombie slave."));

            item ball("pheromone", 0);
            iuse pheromone;
            pheromone.pheromone(&(g->u), &ball, true, g->u.pos());
        }

    }

    if (rope == choice) {
        if (z->has_effect("tied")) {
            z->remove_effect("tied");
            item rope_6("rope_6", 0);
            g->u.i_add(rope_6);
        } else {
            z->add_effect("tied", 1, num_bp, true);
            g->u.use_amount( "rope_6", 1 );
        }

        return true;
    }

    return true;
}

void game::examine(int examx, int examy)
{
    int veh_part = 0;
    vehicle *veh = NULL;
    const int curz = levz;

    if (examx == -1) {
        // if we are driving a vehicle, examine the
        // current tile without asking.
        veh = m.veh_at(u.posx(), u.posy(), veh_part);
        if (veh && veh->player_in_control(&u)) {
            examx = u.posx();
            examy = u.posy();
        } else  if (!choose_adjacent_highlight(_("Examine where?"), examx, examy, ACTION_EXAMINE)) {
            return;
        }
    }

    veh = m.veh_at(examx, examy, veh_part);
    if (veh) {
        int vpcargo = veh->part_with_feature(veh_part, "CARGO", false);
        int vpkitchen = veh->part_with_feature(veh_part, "KITCHEN", true);
        int vpfaucet = veh->part_with_feature(veh_part, "FAUCET", true);
        int vpweldrig = veh->part_with_feature(veh_part, "WELDRIG", true);
        int vpcraftrig = veh->part_with_feature(veh_part, "CRAFTRIG", true);
        int vpchemlab = veh->part_with_feature(veh_part, "CHEMLAB", true);
        int vpcontrols = veh->part_with_feature(veh_part, "CONTROLS", true);
        auto here_ground = m.i_at(examx, examy);
        if( (vpcargo >= 0 && !veh->get_items(vpcargo).empty()) || vpkitchen >= 0 ||
            vpfaucet >= 0 || vpweldrig >= 0 || vpcraftrig >= 0 || vpchemlab >= 0 ||
            vpcontrols >= 0 || !here_ground.empty() ) {
            Pickup::pick_up(examx, examy, 0);
        } else if (u.controlling_vehicle) {
            add_msg(m_info, _("You can't do that while driving."));
        } else if (abs(veh->velocity) > 0) {
            add_msg(m_info, _("You can't do that on a moving vehicle."));
        } else {
            exam_vehicle(*veh, examx, examy);
        }
        return;
    }

    if (m.has_flag("CONSOLE", examx, examy)) {
        use_computer(examx, examy);
        return;
    }
    const furn_t *xfurn_t = &furnlist[m.furn(examx, examy)];
    const ter_t *xter_t = &terlist[m.ter(examx, examy)];
    iexamine xmine;
    const int player_x = u.posx();
    const int player_y = u.posy();

    if (m.has_furn(examx, examy)) {
        (xmine.*xfurn_t->examine)(&u, &m, examx, examy);
    } else {
        (xmine.*xter_t->examine)(&u, &m, examx, examy);
    }

    // Did the player get moved? Bail out if so; our examx and examy probably
    // aren't valid anymore.
    if (player_x != u.posx() || player_y != u.posy()) {
        return;
    }

    if (curz != levz) {
        // triggered an elevator
        return;
    }

    bool none = true;
    if (xter_t->examine != &iexamine::none || xfurn_t->examine != &iexamine::none) {
        none = false;
    }

    if (critter_at(examx, examy) != NULL) {
        Creature *c = critter_at(examx, examy);
        monster *mon = dynamic_cast<monster *>(c);

        if (mon != NULL && mon->has_effect("pet")) {
            if (pet_menu(mon)) {
                return;
            }
        }
    }

    if (m.has_flag("SEALED", examx, examy)) {
        if (none) {
            if (m.has_flag("UNSTABLE", examx, examy)) {
                add_msg(_("The %s is too unstable to remove anything."), m.name(examx, examy).c_str());
            } else {
                add_msg(_("The %s is firmly sealed."), m.name(examx, examy).c_str());
            }
        }
    } else {
        //examx,examy has no traps, is a container and doesn't have a special examination function
        if (m.tr_at(examx, examy) == tr_null && m.i_at(examx, examy).empty() &&
            m.has_flag("CONTAINER", examx, examy) && none) {
            add_msg(_("It is empty."));
        } else if (!veh) {
            Pickup::pick_up(examx, examy, 0);
        }
    }

    //check for disarming traps last to avoid disarming query black box issue.
    if(m.tr_at(examx, examy) != tr_null) {
        xmine.trap(&u, &m, examx, examy);
        if(m.tr_at(examx, examy) == tr_null) {
            Pickup::pick_up(examx, examy, 0);    // After disarming a trap, pick it up.
        }
    }
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carefully peeking around a corner, hence the large move cost.
void game::peek(int peekx, int peeky)
{
    int prevx, prevy;

    if (peekx == 0 && peeky == 0) {
        if (!choose_adjacent(_("Peek where?"), peekx, peeky)) {
            return;
        }

        if (m.move_cost(peekx, peeky) == 0) {
            return;
        }
    }

    u.moves -= 200;
    prevx = u.posx();
    prevy = u.posy();
    u.setx( peekx );
    u.sety( peeky );
    look_around();
    u.setx( prevx );
    u.sety( prevy );
    draw_ter();
}
////////////////////////////////////////////////////////////////////////////////////////////
point game::look_debug()
{
    editmap *edit = new editmap();
    point ret = edit->edit();
    delete edit;
    edit = 0;
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////

void game::print_all_tile_info(int lx, int ly, WINDOW *w_look, int column, int &line,
                               bool mouse_hover)
{
    print_terrain_info(lx, ly, w_look, column, line);
    print_fields_info(lx, ly, w_look, column, line);
    print_trap_info(lx, ly, w_look, column, line);
    print_object_info(lx, ly, w_look, column, line, mouse_hover);
}

void game::print_terrain_info(int lx, int ly, WINDOW *w_look, int column, int &line)
{
    int ending_line = line + 3;
    std::string tile = m.tername(lx, ly);
    if (m.has_furn(lx, ly)) {
        furn_t furn = m.furn_at(lx, ly);
        tile += "; " + furn.name;
        if (furn.has_flag("PLANT")) {
            // Plant types are defined by seeds.
            item plantType = m.i_at(lx, ly)[0];
            if (plantType.typeId() != "fungal_seeds") {
                // We rely on the seeds we care about to be
                // id'd as seed_*.
                tile += " (" + plantType.typeId().substr(5) + ")";
            }
        }
    }

    if (m.move_cost(lx, ly) == 0) {
        mvwprintw(w_look, line, column, _("%s; Impassable"), tile.c_str());
    } else {
        mvwprintw(w_look, line, column, _("%s; Movement cost %d"), tile.c_str(),
                  m.move_cost(lx, ly) * 50);
    }

    std::string signage = m.get_signage(lx, ly);
    if (signage.size() > 0 && signage.size() < 36) {
        mvwprintw(w_look, ++line, column, _("Sign: %s"), signage.c_str());
    } else if (signage.size() > 0) {
        // Truncate to width of window as a guesstimate.
        mvwprintw(w_look, ++line, column, _("Sign: %s..."), signage.substr(0, 32).c_str());
    }

    mvwprintw(w_look, ++line, column, "%s", m.features(lx, ly).c_str());
    if (line < ending_line) {
        line = ending_line;
    }
}

void game::print_fields_info(int lx, int ly, WINDOW *w_look, int column, int &line)
{
    const field &tmpfield = m.field_at(lx, ly);
    for( auto &fld : tmpfield ) {
        const field_entry *cur = &fld.second;
        mvwprintz(w_look, line++, column, fieldlist[cur->getFieldType()].color[cur->getFieldDensity() - 1],
                  "%s",
                  fieldlist[cur->getFieldType()].name[cur->getFieldDensity() - 1].c_str());
    }
}

void game::print_trap_info(int lx, int ly, WINDOW *w_look, const int column, int &line)
{
    trap_id trapid = m.tr_at(lx, ly);
    if (trapid == tr_null) {
        return;
    }
    if (traplist[trapid]->can_see(u, lx, ly)) {
        mvwprintz(w_look, line++, column, traplist[trapid]->color, "%s", traplist[trapid]->name.c_str());
    }
}

void game::print_object_info(int lx, int ly, WINDOW *w_look, const int column, int &line,
                             bool mouse_hover)
{
    int veh_part = 0;
    vehicle *veh = m.veh_at(lx, ly, veh_part);
    const Creature *critter = critter_at( lx, ly );
    if( critter != nullptr && ( u.sees( *critter ) || critter == &u ) ) {
        if( !mouse_hover ) {
            critter->draw( w_terrain, lx, ly, true );
        }
        line = critter->print_info( w_look, line, 6, column );
    } else if (veh) {
        mvwprintw(w_look, line++, column, _("There is a %s there. Parts:"), veh->name.c_str());
        line = veh->print_part_desc(w_look, line, (mouse_hover) ? getmaxx(w_look) : 48, veh_part);
        if (!mouse_hover) {
            m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
        }
    } else if (!mouse_hover) {
        m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
    }
    handle_multi_item_info(lx, ly, w_look, column, line, mouse_hover);
}

void game::handle_multi_item_info(int lx, int ly, WINDOW *w_look, const int column, int &line,
                                  bool mouse_hover)
{
    if (m.sees_some_items(lx, ly, u)) {
        if (mouse_hover) {
            // items are displayed from the live view, don't do this here
            return;
        }
        auto items = m.i_at(lx, ly);
        mvwprintw(w_look, line++, column, _("There is a %s there."), items[0].tname().c_str());
        if (items.size() > 1) {
            mvwprintw(w_look, line++, column, _("There are other items there as well."));
        }
    } else if (m.has_flag("CONTAINER", lx, ly) && !m.could_see_items(lx, ly, u)) {
        mvwprintw(w_look, line++, column, _("You cannot see what is inside of it."));
    }
}

void game::get_lookaround_dimensions(int &lookWidth, int &begin_y, int &begin_x) const
{
    lookWidth = getmaxx(w_messages);
    begin_y = TERMY - lookHeight + 1;
    if (getbegy(w_messages) < begin_y) {
        begin_y = getbegy(w_messages);
    }
    begin_x = getbegx(w_messages);
}

bool game::checkZone(const std::string p_sType, const int p_iX, const int p_iY)
{
    return u.Zones.hasZone(p_sType, m.getabs(p_iX, p_iY));
}

void game::zones_manager_shortcuts(WINDOW *w_info)
{
    werase(w_info);

    int tmpx = 1;
    tmpx += shortcut_print(w_info, 1, tmpx, c_white, c_ltgreen, _("<A>dd")) + 2;
    tmpx += shortcut_print(w_info, 1, tmpx, c_white, c_ltgreen, _("<R>emove")) + 2;
    tmpx += shortcut_print(w_info, 1, tmpx, c_white, c_ltgreen, _("<E>nable")) + 2;
    tmpx += shortcut_print(w_info, 1, tmpx, c_white, c_ltgreen, _("<D>isable")) + 2;

    tmpx = 1;
    tmpx += shortcut_print(w_info, 2, tmpx, c_white, c_ltgreen, _("<+-> Move up/down")) + 2;
    tmpx += shortcut_print(w_info, 2, tmpx, c_white, c_ltgreen, _("<Enter>-Edit")) + 2;

    tmpx = 1;
    tmpx += shortcut_print(w_info, 3, tmpx, c_white, c_ltgreen, _("Show on <M>ap")) + 2;

    wrefresh(w_info);
}

void game::zones_manager_draw_borders(WINDOW *w_border, WINDOW *w_info_border,
                                      const int iInfoHeight, const int width)
{
    for (int i = 1; i < TERMX; ++i) {
        if (i < width) {
            mvwputch(w_border, 0, i, c_ltgray, LINE_OXOX); // -
            mvwputch(w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, c_ltgray, LINE_OXOX); // -
        }

        if (i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2) {
            mvwputch(w_border, i, 0, c_ltgray, LINE_XOXO); // |
            mvwputch(w_border, i, width - 1, c_ltgray, LINE_XOXO); // |
        }
    }

    mvwputch(w_border, 0, 0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w_border, 0, width - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, c_ltgray,
             LINE_XOXX); // -|

    mvwprintz(w_border, 0, 2, c_white, _("Zones manager"));
    wrefresh(w_border);

    for (int j = 0; j < iInfoHeight - 1; ++j) {
        mvwputch(w_info_border, j, 0, c_ltgray, LINE_XOXO);
        mvwputch(w_info_border, j, width - 1, c_ltgray, LINE_XOXO);
    }

    for (int j = 0; j < width - 1; ++j) {
        mvwputch(w_info_border, iInfoHeight - 1, j, c_ltgray, LINE_OXOX);
    }

    mvwputch(w_info_border, iInfoHeight - 1, 0, c_ltgray, LINE_XXOO);
    mvwputch(w_info_border, iInfoHeight - 1, width - 1, c_ltgray, LINE_XOOX);
    wrefresh(w_info_border);
}

void game::zones_manager()
{
    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    u.view_offset_x = 0;
    u.view_offset_y = 0;

    const int offset_x = (u.posx() + u.view_offset_x) - getmaxx(w_terrain) / 2;
    const int offset_y = (u.posy() + u.view_offset_y) - getmaxy(w_terrain) / 2;

    draw_ter();

    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    WINDOW *w_zones = newwin(TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2, width - 2, VIEW_OFFSET_Y + 1,
                             TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW *w_zones_border = newwin(TERMY - iInfoHeight - VIEW_OFFSET_Y * 2, width, VIEW_OFFSET_Y,
                                    TERMX - width - VIEW_OFFSET_X);
    WINDOW *w_zones_info = newwin(iInfoHeight - 1, width - 2, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                  TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW *w_zones_info_border = newwin(iInfoHeight, width, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                         TERMX - width - VIEW_OFFSET_X);

    zones_manager_draw_borders(w_zones_border, w_zones_info_border, iInfoHeight, width);
    zones_manager_shortcuts(w_zones_info);

    std::string action;
    input_context ctxt("ZONES_MANAGER");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("ADD_ZONE");
    ctxt.register_action("REMOVE_ZONE");
    ctxt.register_action("MOVE_ZONE_UP");
    ctxt.register_action("MOVE_ZONE_DOWN");
    ctxt.register_action("SHOW_ZONE_ON_MAP");
    ctxt.register_action("ENABLE_ZONE");
    ctxt.register_action("DISABLE_ZONE");

    int iZonesNum = u.Zones.size();
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2;
    int iStartPos = 0;
    int iActive = 0;
    bool bBlink = false;
    bool bRedrawInfo = true;
    bool bStuffChanged = false;

    do {
        if (action == "ADD_ZONE") {
            zones_manager_draw_borders(w_zones_border, w_zones_info_border, iInfoHeight, width);
            werase(w_zones_info);

            mvwprintz(w_zones_info, 3, 2, c_white, _("Select first point."));
            wrefresh(w_zones_info);

            point pFirst = look_around(w_zones_info, point(-999, -999));
            point pSecond = point(-1, -1);

            if (pFirst.x != -1 && pFirst.y != -1) {
                mvwprintz(w_zones_info, 3, 2, c_white, _("Select second point."));
                wrefresh(w_zones_info);

                pSecond = look_around(w_zones_info, pFirst);
            }

            if (pSecond.x != -1 && pSecond.y != -1) {
                werase(w_zones_info);
                wrefresh(w_zones_info);

                u.Zones.add("", "", false, true,
                            m.getabs(std::min(pFirst.x, pSecond.x), std::min(pFirst.y, pSecond.y)),
                            m.getabs(std::max(pFirst.x, pSecond.x), std::max(pFirst.y, pSecond.y))
                           );

                iZonesNum = u.Zones.size();
                iActive = iZonesNum - 1;

                u.Zones.vZones[iActive].setName();
                u.Zones.vZones[iActive].setZoneType(u.Zones.getZoneTypes());
            }

            draw_ter();
            bBlink = false;
            bRedrawInfo = true;

            zones_manager_draw_borders(w_zones_border, w_zones_info_border, iInfoHeight, width);
            zones_manager_shortcuts(w_zones_info);

        } else if (u.Zones.size() > 0) {
            if (action == "UP") {
                iActive--;
                if (iActive < 0) {
                    iActive = iZonesNum - 1;
                }
                draw_ter();
                bBlink = false;
                bRedrawInfo = true;

            } else if (action == "DOWN") {
                iActive++;
                if (iActive >= iZonesNum) {
                    iActive = 0;
                }
                draw_ter();
                bBlink = false;
                bRedrawInfo = true;

            } else if (action == "REMOVE_ZONE") {
                if (iActive < (int)u.Zones.size()) {
                    u.Zones.remove(iActive);
                    iActive--;

                    if (iActive < 0) {
                        iActive = 0;
                    }

                    iZonesNum = u.Zones.size();

                    draw_ter();
                    wrefresh(w_terrain);
                }
                bBlink = false;
                bRedrawInfo = true;
                bStuffChanged = true;

            } else if (action == "CONFIRM") {
                uimenu as_m;
                as_m.text = _("What do you want to change:");
                as_m.entries.push_back(uimenu_entry(1, true, '1', _("Edit name")));
                as_m.entries.push_back(uimenu_entry(2, true, '2', _("Edit type")));
                //as_m.entries.push_back(uimenu_entry(3, true, '3', _("Move position left/top") ));
                //as_m.entries.push_back(uimenu_entry(4, true, '4', _("Move coordinates right/bottom") ));
                as_m.entries.push_back(uimenu_entry(5, true, 'q', _("Cancel")));
                as_m.query();

                switch (as_m.ret) {
                case 1:
                    u.Zones.vZones[iActive].setName();
                    bStuffChanged = true;
                    break;
                case 2:
                    u.Zones.vZones[iActive].setZoneType(u.Zones.getZoneTypes());
                    bStuffChanged = true;
                    break;
                case 3:
                    //pos lt
                    break;
                case 4:
                    //pos rb
                    break;
                default:
                    break;
                }

                as_m.reset();

                draw_ter();

                bBlink = false;
                bRedrawInfo = true;

                zones_manager_draw_borders(w_zones_border, w_zones_info_border, iInfoHeight, width);
                zones_manager_shortcuts(w_zones_info);

            } else if (action == "MOVE_ZONE_UP" && u.Zones.size() > 1) {
                if (iActive < (int)u.Zones.size() - 1) {
                    std::swap(u.Zones.vZones[iActive],
                              u.Zones.vZones[iActive + 1]);
                    iActive++;
                }
                bBlink = false;
                bRedrawInfo = true;
                bStuffChanged = true;

            } else if (action == "MOVE_ZONE_DOWN" && u.Zones.size() > 1) {
                if (iActive > 0) {
                    std::swap(u.Zones.vZones[iActive],
                              u.Zones.vZones[iActive - 1]);
                    iActive--;
                }
                bBlink = false;
                bRedrawInfo = true;
                bStuffChanged = true;

            } else if (action == "SHOW_ZONE_ON_MAP") {
                //show zone position on overmap;
                point pOMPlayer = overmapbuffer::ms_to_omt_copy(m.getabs(u.posx(), u.posy()));
                point pOMZone = overmapbuffer::ms_to_omt_copy(u.Zones.vZones[iActive].getCenterPoint());
                overmap::draw_overmap(tripoint(pOMPlayer.x, pOMPlayer.y),
                                      false,
                                      tripoint(pOMZone.x, pOMZone.y),
                                      iActive
                                     );

                zones_manager_draw_borders(w_zones_border, w_zones_info_border, iInfoHeight, width);
                zones_manager_shortcuts(w_zones_info);

                draw_ter();

                bRedrawInfo = true;

            } else if (action == "ENABLE_ZONE") {
                u.Zones.vZones[iActive].setEnabled(true);

                bRedrawInfo = true;
                bStuffChanged = true;

            } else if (action == "DISABLE_ZONE") {
                u.Zones.vZones[iActive].setEnabled(false);

                bRedrawInfo = true;
                bStuffChanged = true;
            }
        }

        if (iZonesNum == 0) {
            werase(w_zones);
            wrefresh(w_zones_border);
            mvwprintz(w_zones, 5, 2, c_white, _("No Zones defined."));

        } else if (bRedrawInfo) {
            bRedrawInfo = false;
            werase(w_zones);

            calcStartPos(iStartPos, iActive, iMaxRows, iZonesNum);

            //Draw Scrollbar
            draw_scrollbar(w_zones_border, iActive, iMaxRows, iZonesNum, 1);

            int iNum = 0;

            point pointPlayer = m.getabs(u.posx(), u.posy());

            //Display saved zones
            for (auto &i : u.Zones.vZones) {
                if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iZonesNum) ? iZonesNum : iMaxRows)) {
                    nc_color colorLine = (i.getEnabled()) ? c_white : c_ltgray;

                    if (iNum == iActive) {
                        mvwprintz(w_zones, iNum - iStartPos, 0, c_yellow, "%s", ">>");
                        colorLine = (i.getEnabled()) ? c_ltgreen : c_green;
                    }

                    //Draw Zone name
                    mvwprintz(w_zones, iNum - iStartPos, 3, colorLine, "%s",
                              u.Zones.vZones[iNum].getName().c_str());

                    //Draw Type name
                    mvwprintz(w_zones, iNum - iStartPos, 20, colorLine, "%s",
                              u.Zones.getNameFromType(u.Zones.vZones[iNum].getZoneType()).c_str());

                    point pCenter = i.getCenterPoint();

                    //Draw direction + distance
                    mvwprintz(w_zones, iNum - iStartPos, 35, colorLine, "%*d %s",
                              5, trig_dist(pCenter.x,
                                           pCenter.y,
                                           pointPlayer.x,
                                           pointPlayer.y
                                          ),
                              direction_name_short(direction_from(pCenter.x,
                                                   pCenter.y,
                                                   pointPlayer.x,
                                                   pointPlayer.y
                                                                 )
                                                  ).c_str()
                             );
                }
                iNum++;
            }
        }

        if (iZonesNum > 0) {
            bBlink = !bBlink;

            point pStart = m.getlocal(u.Zones.vZones[iActive].getStartPoint());
            point pEnd = m.getlocal(u.Zones.vZones[iActive].getEndPoint());

            if (bBlink) {
                //draw marked area
                point pOffset = point(offset_x, offset_y); //ASCII
#ifdef TILES
                if (use_tiles) {
                    pOffset = point(0, 0); //TILES
                } else {
                    pOffset = point(-offset_x, -offset_y); //SDL
                }
#endif

                draw_zones(pStart, pEnd, pOffset);
            } else {
                //clear marked area
#ifdef TILES
                if (!use_tiles) {
#endif
                    for (int iY = pStart.y; iY <= pEnd.y; ++iY) {
                        for (int iX = pStart.x; iX <= pEnd.x; ++iX) {
                            if (u.sees(iX, iY)) {
                                m.drawsq(w_terrain, u,
                                         iX,
                                         iY,
                                         false,
                                         false,
                                         u.posx() + u.view_offset_x,
                                         u.posy() + u.view_offset_y);
                            } else {
                                if (u.has_effect("boomered")) {
                                    mvwputch(w_terrain, iY - offset_y, iX - offset_x, c_magenta, '#');

                                } else {
                                    mvwputch(w_terrain, iY - offset_y, iX - offset_x, c_black, ' ');
                                }
                            }
                        }
                    }
#ifdef TILES
                }
#endif
            }

            wrefresh(w_terrain);

            inp_mngr.set_timeout(BLINK_SPEED);
        } else {
            inp_mngr.set_timeout(-1);
        }

        wrefresh(w_zones);
        wrefresh(w_zones_border);

        //Wait for input
        handle_mouseview(ctxt, action);
    } while (action != "QUIT");
    inp_mngr.set_timeout(-1);

    werase(w_zones);
    werase(w_zones_border);
    werase(w_zones_info);
    werase(w_zones_info_border);

    delwin(w_zones);
    delwin(w_zones_border);
    delwin(w_zones_info);
    delwin(w_zones_info_border);

    if (bStuffChanged) {
        if (query_yn(_("Save changes?"))) {
            u.save_zones();
        } else {
            u.load_zones();
        }
    }

    u.view_offset_x = iStoreViewOffsetX;
    u.view_offset_y = iStoreViewOffsetY;

    refresh_all();
}

point game::look_around(WINDOW *w_info, const point pairCoordsFirst)
{
    temp_exit_fullscreen();

    bool bSelectZone = (pairCoordsFirst.x != -1 && pairCoordsFirst.y != -1);
    bool bHasFirstPoint = (pairCoordsFirst.x != -999 && pairCoordsFirst.y != -999);

    const int offset_x = (u.posx() + u.view_offset_x) - getmaxx(w_terrain) / 2;
    const int offset_y = (u.posy() + u.view_offset_y) - getmaxy(w_terrain) / 2;

    int lx = u.posx() + u.view_offset_x, ly = u.posy() + u.view_offset_y;

    if (bSelectZone && bHasFirstPoint) {
        lx = pairCoordsFirst.x;
        ly = pairCoordsFirst.y;
    }

    draw_ter(lx, ly);

    int soffset = (int)OPTIONS["MOVE_VIEW_OFFSET"];
    bool fast_scroll = false;
    bool bBlink = false;

    int lookWidth, lookY, lookX;
    get_lookaround_dimensions(lookWidth, lookY, lookX);

    bool bNewWindow = false;
    if (w_info == NULL) {
        w_info = newwin(lookHeight, lookWidth, lookY, lookX);
        bNewWindow = true;
    }

    dbg( D_PEDANTIC_INFO ) << ": calling handle_input()";

    std::string action;
    input_context ctxt("LOOK");
    ctxt.register_directions();
    ctxt.register_action("COORDINATE");
    ctxt.register_action("SELECT");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("TOGGLE_FAST_SCROLL");

    do {
        if (bNewWindow) {
            werase(w_info);
            draw_border(w_info);
        }

        int junk;
        int off = 1;

        if (bSelectZone) {
            //Select Zone
            if (bHasFirstPoint) {
                bBlink = !bBlink;

                const int dx = pairCoordsFirst.x - offset_x + u.posx() - lx;
                const int dy = pairCoordsFirst.y - offset_y + u.posy() - ly;

                if (bBlink) {
                    const point pStart = point(std::min(dx, POSX), std::min(dy, POSY));
                    const point pEnd = point(std::max(dx, POSX), std::max(dy, POSY));

                    point pOffset = point(0, 0); //ASCII/SDL
#ifdef TILES
                    if (use_tiles) {
                        pOffset = point(offset_x + lx - u.posx(), offset_y + ly - u.posy()); //TILES
                    }
#endif

                    draw_zones(pStart, pEnd, pOffset);

                } else {
#ifdef TILES
                    if (!use_tiles) {
#endif
                        for (int iY = std::min(pairCoordsFirst.y, ly); iY <= std::max(pairCoordsFirst.y, ly); ++iY) {
                            for (int iX = std::min(pairCoordsFirst.x, lx); iX <= std::max(pairCoordsFirst.x, lx); ++iX) {
                                if (u.sees(iX, iY)) {
                                    m.drawsq(w_terrain, u,
                                             iX,
                                             iY,
                                             false,
                                             false,
                                             lx,
                                             ly);
                                } else {
                                    if (u.has_effect("boomered")) {
                                        mvwputch(w_terrain, iY - offset_y - ly + u.posy(), iX - offset_x - lx + u.posx(), c_magenta, '#');

                                    } else {
                                        mvwputch(w_terrain, iY - offset_y - ly + u.posy(), iX - offset_x - lx + u.posx(), c_black, ' ');
                                    }
                                }
                            }
                        }
#ifdef TILES
                    }
#endif
                }

                //Draw first point
                mvwputch_inv(w_terrain, dy, dx, c_ltgreen, 'X');
            }

            //Draw select cursor
            mvwputch_inv(w_terrain, POSY, POSX, c_ltgreen, 'X');

        } else {
            //Look around
            if (u.sees(lx, ly)) {
                print_all_tile_info(lx, ly, w_info, 1, off, false);

            } else if (u.sight_impaired() &&
                       m.light_at(lx, ly) == LL_BRIGHT &&
                       rl_dist(u.posx(), u.posy(), lx, ly) < u.unimpaired_range() &&
                       m.sees(u.posx(), u.posy(), lx, ly, u.unimpaired_range(), junk)) {
                if (u.has_effect("boomered")) {
                    mvwputch_inv(w_terrain, POSY + (ly - u.posy()), POSX + (lx - u.posx()), c_pink, '#');

                } else if (u.has_effect("darkness")) {
                    mvwputch_inv(w_terrain, POSY + (ly - u.posy()), POSX + (lx - u.posx()), c_dkgray, '#');

                } else {
                    mvwputch_inv(w_terrain, POSY + (ly - u.posy()), POSX + (lx - u.posx()), c_ltgray, '#');
                }

                mvwprintw(w_info, 1, 1, _("Bright light."));

            } else {
                mvwputch(w_terrain, POSY, POSX, c_white, 'x');
                mvwprintw(w_info, 1, 1, _("Unseen."));
            }

            if (fast_scroll) {
                // print a light green mark below the top right corner of the w_info window
                mvwprintz(w_info, 1, lookWidth - 1, c_ltgreen, _("F"));
            }

            if( m.has_graffiti_at( lx, ly ) ) {
                mvwprintw(w_info, ++off + 1, 1, _("Graffiti: %s"), m.graffiti_at( lx, ly ).c_str() );
            }

            wrefresh(w_info);
        }

        wrefresh(w_terrain);

        if (bSelectZone && bHasFirstPoint) {
            inp_mngr.set_timeout(BLINK_SPEED);
        }

        //Wait for input
        if (!handle_mouseview(ctxt, action)) {
            // Our coordinates will either be determined by coordinate input(mouse),
            // by a direction key, or by the previous value.
            if (action == "TOGGLE_FAST_SCROLL") {
                fast_scroll = !fast_scroll;

            } else if (!ctxt.get_coordinates(w_terrain, lx, ly)) {
                int dx, dy;
                ctxt.get_direction(dx, dy, action);

                if (dx == -2) {
                    dx = 0;
                    dy = 0;
                } else {
                    if (fast_scroll) {
                        dx *= soffset;
                        dy *= soffset;
                    }
                }

                lx += dx;
                ly += dy;

                //Keep cursor inside the reality bubble
                if (lx < 0) {
                    lx = 0;
                } else if (lx > MAPSIZE * SEEX) {
                    lx = MAPSIZE * SEEX;
                }

                if (ly < 0) {
                    ly = 0;
                } else if (ly > MAPSIZE * SEEY) {
                    ly = MAPSIZE * SEEY;
                }

                draw_ter(lx, ly, true);
            }
        }
    } while (action != "QUIT" && action != "CONFIRM");
    inp_mngr.set_timeout(-1);

    if (bNewWindow) {
        werase(w_info);
        delwin(w_info);
    }
    reenter_fullscreen();

    if (action == "CONFIRM") {
        if (bSelectZone) {
            return point(lx, ly);
        }
        return point(lx, ly);
    }

    return point(-1, -1);
}

bool lcmatch(const std::string &str, const std::string &findstr); // ui.cpp
bool game::list_items_match(item &item, std::string sPattern)
{
    size_t iPos;
    bool hasExclude = false;

    if (sPattern.find("-") != std::string::npos) {
        hasExclude = true;
    }

    do {
        iPos = sPattern.find(",");
        std::string pat = (iPos == std::string::npos) ? sPattern : sPattern.substr(0, iPos);
        bool exclude = false;
        if (pat.substr(0, 1) == "-") {
            exclude = true;
            pat = pat.substr(1, pat.size() - 1);
        } else if (hasExclude) {
            hasExclude = false; //If there are non exclusive items to filter, we flip this back to false.
        }

        std::string namepat = pat;
        std::transform( namepat.begin(), namepat.end(), namepat.begin(), tolower );
        if( lcmatch( item.tname(), namepat ) ) {
            return !exclude;
        }

        if (pat.find("{", 0) != std::string::npos) {
            std::string adv_pat_type = pat.substr(1, pat.find(":") - 1);
            std::string adv_pat_search = pat.substr(pat.find(":") + 1, (pat.find("}") - pat.find(":")) - 1);
            std::transform(adv_pat_search.begin(), adv_pat_search.end(), adv_pat_search.begin(), tolower);
            if (adv_pat_type == "c" && lcmatch(item.get_category().name, adv_pat_search)) {
                return !exclude;
            } else if (adv_pat_type == "m") {
                for (auto material : item.made_of_types()) {
                    if (lcmatch(material->name(), adv_pat_search)) {
                        return !exclude;
                    }
                }
            } else if (adv_pat_type == "dgt" && item.damage > atoi(adv_pat_search.c_str())) {
                return !exclude;
            } else if (adv_pat_type == "dlt" && item.damage < atoi(adv_pat_search.c_str())) {
                return !exclude;
            }
        }

        if (iPos != std::string::npos) {
            sPattern = sPattern.substr(iPos + 1, sPattern.size());
        }

    } while (iPos != std::string::npos);

    return hasExclude;
}

std::vector<map_item_stack> game::find_nearby_items(int iRadius)
{
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> vOrder;

    if (u.has_effect("blind")) {
        return ret;
    }

    std::vector<point> points = closest_points_first(iRadius, u.posx(), u.posy());

    int iLastX = 0;
    int iLastY = 0;

    for( auto &points_p_it : points ) {
        if( points_p_it.y >= u.posy() - iRadius && points_p_it.y <= u.posy() + iRadius &&
            u.sees( points_p_it ) &&
            m.sees_some_items( points_p_it.x, points_p_it.y, u ) ) {

            for( auto &elem : m.i_at( points_p_it.x, points_p_it.y ) ) {
                const std::string name = elem.tname();

                if( temp_items.find( name ) == temp_items.end() ||
                    ( iLastX != points_p_it.x || iLastY != points_p_it.y ) ) {
                    iLastX = points_p_it.x;
                    iLastY = points_p_it.y;

                    if (std::find(vOrder.begin(), vOrder.end(), name) == vOrder.end()) {
                        vOrder.push_back(name);
                        temp_items[name] =
                            map_item_stack( elem, points_p_it.x - u.posx(), points_p_it.y - u.posy() );
                    } else {
                        temp_items[name].addNewPos( points_p_it.x - u.posx(),
                                                    points_p_it.y - u.posy() );
                    }

                } else {
                    temp_items[name].incCount();
                }
            }
        }
    }

    for( auto &elem : vOrder ) {
        ret.push_back( temp_items[elem] );
    }

    return ret;
}

std::vector<map_item_stack> game::filter_item_stacks(std::vector<map_item_stack> stack,
        std::string filter)
{
    std::vector<map_item_stack> ret;

    std::string sFilterTemp = filter;

    for( auto &elem : stack ) {
        if( sFilterTemp == "" || list_items_match( elem.example, sFilterTemp ) ) {
            ret.push_back( elem );
        }
    }
    return ret;
}

std::string game::ask_item_filter(WINDOW *window, int rows)
{
    for (int i = 0; i < rows - 1; i++) {
        mvwprintz(window, i, 1, c_black, "%s", "                                                        ");
    }

    mvwprintz(window, 0, 2, c_white, "%s", _("Type part of an item's name to see"));
    mvwprintz(window, 1, 2, c_white, "%s", _("nearby matching items."));
    mvwprintz(window, 3, 2, c_white, "%s", _("Separate multiple items with ,"));
    mvwprintz(window, 4, 2, c_white, "%s", _("Example: back,flash,aid, ,band"));

    mvwprintz(window, 6, 2, c_white, "%s", _("To exclude items, place - in front"));
    mvwprintz(window, 7, 2, c_white, "%s", _("Example: -pipe,chunk,steel"));

    mvwprintz(window, 9, 2, c_white, "%s", _("Search [c]ategory or [m]aterial:"));
    mvwprintz(window, 10, 2, c_white, "%s", _("Example: {c:food},{m:iron}"));
    wrefresh(window);
    return string_input_popup(_("Filter:"), 55, sFilter,
                              _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "item_filter", 256);
}

std::string game::ask_item_priority_high(WINDOW *window, int rows)
{
    for (int i = 0; i < rows - 1; i++) {
        mvwprintz(window, i, 1, c_black, "%s", "                                                        ");
    }

    mvwprintz(window, 2, 2, c_white, "%s", _("Type part of an item's name to move"));
    mvwprintz(window, 3, 2, c_white, "%s", _("nearby items to the top."));

    mvwprintz(window, 5, 2, c_white, "%s", _("Separate multiple items with ,"));
    mvwprintz(window, 6, 2, c_white, "%s", _("Example: back,flash,aid, ,band"));

    mvwprintz(window, 8, 2, c_white, "%s", _("Search [c]ategory or [m]aterial:"));
    mvwprintz(window, 9, 2, c_white, "%s", _("Example: {c:food},{m:iron}"));
    wrefresh(window);
    return string_input_popup(_("High Priority:"), 55, list_item_upvote,
                              _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "list_item_priority", 256);
}

std::string game::ask_item_priority_low(WINDOW *window, int rows)
{
    for (int i = 0; i < rows - 1; i++) {
        mvwprintz(window, i, 1, c_black, "%s", "                                                        ");
    }

    mvwprintz(window, 2, 2, c_white, "%s", _("Type part of an item's name to move"));
    mvwprintz(window, 3, 2, c_white, "%s", _("nearby items to the bottom."));

    mvwprintz(window, 5, 2, c_white, "%s", _("Separate multiple items with ,"));
    mvwprintz(window, 6, 2, c_white, "%s", _("Example: back,flash,aid, ,band"));

    mvwprintz(window, 8, 2, c_white, "%s", _("Search [c]ategory or [m]aterial:"));
    mvwprintz(window, 9, 2, c_white, "%s", _("Example: {c:food},{m:iron}"));
    wrefresh(window);
    return string_input_popup(_("Low Priority:"), 55, list_item_downvote,
                              _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "list_item_downvote", 256);
}


void game::draw_trail_to_square(int x, int y, bool bDrawX)
{
    //Reset terrain
    draw_ter();

    std::vector<point> vPoint;
    point center = point(u.posx() + u.view_offset_x, u.posy() + u.view_offset_y);
    if (x != 0 || y != 0) {
        //Draw trail
        vPoint = line_to(u.posx(), u.posy(), u.posx() + x, u.posy() + y, 0);
    } else {
        //Draw point
        vPoint.push_back(point(u.posx(), u.posy()));
    }

    draw_line(u.posx() + x, u.posy() + y, center, vPoint);
    if (bDrawX) {
        if (vPoint.empty()) {
            mvwputch(w_terrain, POSY, POSX, c_white, 'X');
        } else {
            mvwputch(w_terrain, POSY + (vPoint[vPoint.size() - 1].y - (u.posy() + u.view_offset_y)),
                     POSX + (vPoint[vPoint.size() - 1].x - (u.posx() + u.view_offset_x)),
                     c_white, 'X');
        }
    }

    wrefresh(w_terrain);
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state(WINDOW *window, int height)
{
    const int width = use_narrow_sidebar() ? 45 : 55;
    for (int i = 1; i < TERMX; i++) {
        if (i < width) {
            mvwputch(window, 0, i, c_ltgray, LINE_OXOX); // -
            mvwputch(window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, i, c_ltgray, LINE_OXOX); // -
        }

        if (i < TERMY - height - VIEW_OFFSET_Y * 2) {
            mvwputch(window, i, 0, c_ltgray, LINE_XOXO); // |
            mvwputch(window, i, width - 1, c_ltgray, LINE_XOXO); // |
        }
    }

    mvwputch(window, 0, 0, c_ltgray, LINE_OXXO); // |^
    mvwputch(window, 0, width - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, 0, c_ltgray, LINE_XXXO); // |-
    mvwputch(window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, width - 1, c_ltgray, LINE_XOXX); // -|

    mvwprintz(window, 0, 2, c_ltgreen, "<Tab>  ");
    wprintz(window, c_white, _("Items"));

    std::vector<std::string> tokens;
    if (sFilter != "") {
        tokens.push_back(_("<R>eset"));
    }

    tokens.push_back(_("<E>xamine"));
    tokens.push_back(_("<C>ompare"));
    tokens.push_back(_("<F>ilter"));
    tokens.push_back(_("<+/->Priority"));

    int gaps = tokens.size() + 1;
    int letters = 0;
    int n = tokens.size();
    for (int i = 0; i < n; i++) {
        letters += utf8_width(tokens[i].c_str()) - 2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = (width - usedwidth) / gaps;
    usedwidth += gap_spaces * gaps;
    int xpos = gap_spaces + (width - usedwidth) / 2;
    const int ypos = TERMY - height - 1 - VIEW_OFFSET_Y * 2;

    for (int i = 0; i < n; i++) {
        xpos += shortcut_print(window, ypos, xpos, c_white, c_ltgreen, tokens[i].c_str()) + gap_spaces;
    }

    refresh_all();
}

//returns the first non priority items.
int game::list_filter_high_priority(std::vector<map_item_stack> &stack, std::string prorities)
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for(auto it = stack.begin(); it != stack.end();) {
        if (prorities == "" || !list_items_match(it->example, prorities)) {
            tempstack.push_back(*it);
            it = stack.erase(it);
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

int game::list_filter_low_priority(std::vector<map_item_stack> &stack, int start,
                                   std::string prorities)
{
    //TODO:optimize if necessary
    std::vector<map_item_stack> tempstack; // temp
    for (auto it = stack.begin() + start; it != stack.end();) {
        if(prorities != "" && list_items_match(it->example, prorities)) {
            tempstack.push_back(*it);
            it = stack.erase(it);
        } else {
            it++;
        }
    }

    int id = stack.size();
    for( auto &elem : tempstack ) {
        stack.push_back( elem );
    }
    return id;
}

void centerlistview(int iActiveX, int iActiveY)
{
    player &u = g->u;
    if (OPTIONS["SHIFT_LIST_ITEM_VIEW"] != "false") {
        int xpos = POSX + iActiveX;
        int ypos = POSY + iActiveY;
        if (OPTIONS["SHIFT_LIST_ITEM_VIEW"] == "centered") {
            int xOffset = TERRAIN_WINDOW_WIDTH / 2;
            int yOffset = TERRAIN_WINDOW_HEIGHT / 2;
            if (!is_valid_in_w_terrain(xpos, ypos)) {
                if (xpos < 0) {
                    u.view_offset_x = xpos - xOffset;
                } else {
                    u.view_offset_x = xpos - (TERRAIN_WINDOW_WIDTH - 1) + xOffset;
                }

                if (xpos < 0) {
                    u.view_offset_y = ypos - yOffset;
                } else {
                    u.view_offset_y = ypos - (TERRAIN_WINDOW_HEIGHT - 1) + yOffset;
                }
            } else {
                u.view_offset_x = 0;
                u.view_offset_y = 0;
            }
        } else {
            if (xpos < 0) {
                u.view_offset_x = xpos;
            } else if (xpos >= TERRAIN_WINDOW_WIDTH) {
                u.view_offset_x = xpos - (TERRAIN_WINDOW_WIDTH - 1);
            } else {
                u.view_offset_x = 0;
            }

            if (ypos < 0) {
                u.view_offset_y = ypos;
            } else if (ypos >= TERRAIN_WINDOW_HEIGHT) {
                u.view_offset_y = ypos - (TERRAIN_WINDOW_HEIGHT - 1);
            } else {
                u.view_offset_y = 0;
            }
        }
    }

}

#define MAXIMUM_ZOOM_LEVEL 4
void game::zoom_in()
{
#ifdef SDLTILES
    if (tileset_zoom > MAXIMUM_ZOOM_LEVEL) {
        tileset_zoom = tileset_zoom / 2;
    } else {
        tileset_zoom = 16;
    }
    rescale_tileset(tileset_zoom);
#endif
}

void game::zoom_out()
{
#ifdef SDLTILES
    if (tileset_zoom == 16) {
        tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        tileset_zoom = tileset_zoom * 2;
    }
    rescale_tileset(tileset_zoom);
#endif
}

int game::list_items(const int iLastState)
{
    int iInfoHeight = std::min(25, TERMY / 2);
    const int width = use_narrow_sidebar() ? 45 : 55;
    WINDOW *w_items = newwin(TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2, width - 2, VIEW_OFFSET_Y + 1,
                             TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW_PTR w_itemsptr( w_items );
    WINDOW *w_items_border = newwin(TERMY - iInfoHeight - VIEW_OFFSET_Y * 2, width, VIEW_OFFSET_Y,
                                    TERMX - width - VIEW_OFFSET_X);
    WINDOW_PTR w_items_borderptr( w_items_border );
    WINDOW *w_item_info = newwin(iInfoHeight - 1, width - 2, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                 TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW_PTR w_item_infoptr( w_item_info );
    WINDOW *w_item_info_border = newwin(iInfoHeight, width, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                        TERMX - width - VIEW_OFFSET_X);
    WINDOW_PTR w_item_info_borderptr( w_item_info_border );

    //Area to search +- of players position.
    const int iRadius = 12 + (u.per_cur * 2);

    //this stores the items found, along with the coordinates
    std::vector<map_item_stack> ground_items = find_nearby_items(iRadius);
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items = (sFilter != "" ?
            filter_item_stacks(ground_items, sFilter) :
            ground_items);
    int highPEnd = list_filter_high_priority(filtered_items, list_item_upvote);
    int lowPStart = list_filter_low_priority(filtered_items, highPEnd, list_item_downvote);
    const int iItemNum = ground_items.size();
    if (iItemNum > 0) {
        uistate.list_item_mon = 1; // remember we've tabbed here
    }
    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    u.view_offset_x = 0;
    u.view_offset_y = 0;

    int iReturn = -1;
    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2;
    int iStartPos = 0;
    int iActiveX = 0;
    int iActiveY = 0;
    int iLastActiveX = INT_MIN;
    int iLastActiveY = INT_MIN;
    bool reset = true;
    bool refilter = true;
    int iFilter = 0;
    int iPage = 0;

    std::string action;
    input_context ctxt("LIST_ITEMS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("LEFT", _("Previous item"));
    ctxt.register_action("RIGHT", _("Next item"));
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("QUIT");
    ctxt.register_action("FILTER");
    ctxt.register_action("RESET_FILTER");
    ctxt.register_action("EXAMINE");
    ctxt.register_action("COMPARE");
    ctxt.register_action("PRIORITY_INCREASE");
    ctxt.register_action("PRIORITY_DECREASE");

    do {
        if (!ground_items.empty() || iLastState == 1) {
            if (action == "COMPARE") {
                compare(iActiveX, iActiveY);
                reset = true;
                refresh_all();
            } else if (action == "FILTER") {
                sFilter = ask_item_filter(w_item_info, iInfoHeight);
                reset = true;
                refilter = true;
            } else if (action == "RESET_FILTER") {
                sFilter = "";
                filtered_items = ground_items;
                iLastActiveX = INT_MIN;
                iLastActiveY = INT_MIN;
                reset = true;
                refilter = true;
            } else if (action == "EXAMINE" && filtered_items.size()) {
                item oThisItem = filtered_items[iActive].example;
                std::vector<iteminfo> vThisItem, vDummy;

                oThisItem.info(true, &vThisItem);
                draw_item_info(0, width - 5, 0, TERMY - VIEW_OFFSET_Y * 2,
                               oThisItem.tname(), vThisItem, vDummy);
                // wait until the user presses a key to wipe the screen

                iLastActiveX = INT_MIN;
                iLastActiveY = INT_MIN;
                reset = true;
            } else if (action == "PRIORITY_INCREASE") {
                std::string temp = ask_item_priority_high(w_item_info, iInfoHeight);
                list_item_upvote = temp;
                refilter = true;
                reset = true;
            } else if (action == "PRIORITY_DECREASE") {
                std::string temp = ask_item_priority_low(w_item_info, iInfoHeight);
                list_item_downvote = temp;
                refilter = true;
                reset = true;
            }

            if (refilter) {
                filtered_items = filter_item_stacks(ground_items, sFilter);
                highPEnd = list_filter_high_priority(filtered_items, list_item_upvote);
                lowPStart = list_filter_low_priority(filtered_items, highPEnd, list_item_downvote);
                iActive = 0;
                iPage = 0;
                iLastActiveX = INT_MIN;
                iLastActiveY = INT_MIN;
                refilter = false;
            }

            if (reset) {
                reset_item_list_state(w_items_border, iInfoHeight);
                reset = false;
            }

            if (action == "UP") {
                iActive--;
                iPage = 0;
                if (iActive < 0) {
                    iActive = iItemNum - iFilter - 1;
                }
            } else if (action == "DOWN") {
                iActive++;
                iPage = 0;
                if (iActive >= iItemNum - iFilter) {
                    iActive = 0;
                }
            } else if (action == "RIGHT") {
                iPage++;
                if (!filtered_items.empty() && iPage >= (int)filtered_items[iActive].vIG.size()) {
                    iPage = filtered_items[iActive].vIG.size() - 1;
                }
            } else if (action == "LEFT") {
                iPage--;
                if (iPage < 0) {
                    iPage = 0;
                }
            } else if (action == "NEXT_TAB" || action == "PREV_TAB") {
                u.view_offset_x = iStoreViewOffsetX;
                u.view_offset_y = iStoreViewOffsetY;
                return 1;
            }

            if (ground_items.empty() && iLastState == 1) {
                reset_item_list_state(w_items_border, iInfoHeight);
                wrefresh(w_items_border);
                mvwprintz(w_items, 10, 2, c_white, _("You dont see any items around you!"));
            } else {
                werase(w_items);

                calcStartPos(iStartPos, iActive, iMaxRows, iItemNum - iFilter);

                int iNum = 0;
                iFilter = ground_items.size() - filtered_items.size();
                iActiveX = 0;
                iActiveY = 0;
                item activeItem;
                std::stringstream sText;
                bool high = true;
                bool low = false;
                int index = 0;
                for (std::vector<map_item_stack>::iterator iter = filtered_items.begin();
                     iter != filtered_items.end();
                     ++iter, ++index) {

                    if (index == highPEnd) {
                        high = false;
                    }
                    if (index == lowPStart) {
                        low = true;
                    }

                    if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iItemNum) ?
                            iItemNum : iMaxRows)) {
                        int iThisPage = 0;

                        if (iNum == iActive) {
                            iThisPage = iPage;

                            iActiveX = iter->vIG[iThisPage].x;
                            iActiveY = iter->vIG[iThisPage].y;

                            activeItem = iter->example;
                        }

                        sText.str("");

                        if (iter->vIG.size() > 1) {
                            sText << "[" << iThisPage + 1 << "/" << iter->vIG.size() << "] (" << iter->totalcount << ") ";
                        }

                        sText << iter->example.tname();

                        if (iter->vIG[iThisPage].count > 1) {
                            sText << " [" << iter->vIG[iThisPage].count << "]";
                        }

                        mvwprintz(w_items, iNum - iStartPos, 1,
                                  ((iNum == iActive) ? c_ltgreen : (high ? c_yellow : (low ? c_red : c_white))),
                                  "%s", (sText.str()).c_str());
                        int numw = iItemNum > 9 ? 2 : 1;
                        mvwprintz(w_items, iNum - iStartPos, width - (6 + numw),
                                  ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                                  numw, trig_dist(0, 0, iter->vIG[iThisPage].x, iter->vIG[iThisPage].y),
                                  direction_name_short(direction_from(0, 0, iter->vIG[iThisPage].x, iter->vIG[iThisPage].y)).c_str()
                                 );
                    }
                    iNum++;
                }

                mvwprintz(w_items_border, 0, (width - 9) / 2 + ((iItemNum - iFilter > 9) ? 0 : 1),
                          c_ltgreen, " %*d", ((iItemNum - iFilter > 9) ? 2 : 1), iActive + 1);
                wprintz(w_items_border, c_white, " / %*d ", ((iItemNum - iFilter > 9) ? 2 : 1), iItemNum - iFilter);

                werase(w_item_info);
                //fold_and_print(w_item_info,1,1,width - 5, c_white, activeItem.info());

                std::vector<iteminfo> vThisItem, vDummy;
                activeItem.info(true, &vThisItem);

                draw_item_info(w_item_info, "", vThisItem, vDummy, 0, true, true);

                //Only redraw trail/terrain if x/y position changed
                if (iActiveX != iLastActiveX || iActiveY != iLastActiveY) {
                    iLastActiveX = iActiveX;
                    iLastActiveY = iActiveY;
                    centerlistview(iActiveX, iActiveY);
                    draw_trail_to_square(iActiveX, iActiveY, true);
                }
                //Draw Scrollbar
                draw_scrollbar(w_items_border, iActive, iMaxRows, iItemNum - iFilter, 1);
            }

            for (int j = 0; j < iInfoHeight - 1; j++) {
                mvwputch(w_item_info_border, j, 0, c_ltgray, LINE_XOXO);
            }

            for (int j = 0; j < iInfoHeight - 1; j++) {
                mvwputch(w_item_info_border, j, width - 1, c_ltgray, LINE_XOXO);
            }

            for (int j = 0; j < width - 1; j++) {
                mvwputch(w_item_info_border, iInfoHeight - 1, j, c_ltgray, LINE_OXOX);
            }

            mvwputch(w_item_info_border, iInfoHeight - 1, 0, c_ltgray, LINE_XXOO);
            mvwputch(w_item_info_border, iInfoHeight - 1, width - 1, c_ltgray, LINE_XOOX);

            wrefresh(w_items);
            wrefresh(w_item_info_border);
            wrefresh(w_item_info);

            refresh();

            action = ctxt.handle_input();
        } else {
            iReturn = 0;
            action = "QUIT";
        }
    } while (action != "QUIT");

    u.view_offset_x = iStoreViewOffsetX;
    u.view_offset_y = iStoreViewOffsetY;

    return iReturn;
}

int game::list_monsters(const int iLastState)
{
    const auto vMonsters = u.get_visible_creatures( DAYLIGHT_LEVEL );
    const int iMonsterNum = vMonsters.size();

    if( vMonsters.empty() && iLastState != 1 ) {
        return 0;
    }

    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    WINDOW *w_monsters = newwin(TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2, width - 2,
                                VIEW_OFFSET_Y + 1, TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW_PTR w_monstersptr( w_monsters );
    WINDOW *w_monsters_border = newwin(TERMY - iInfoHeight - VIEW_OFFSET_Y * 2, width, VIEW_OFFSET_Y,
                                       TERMX - width - VIEW_OFFSET_X);
    WINDOW_PTR w_monsters_borderptr( w_monsters_border );
    WINDOW *w_monster_info = newwin(iInfoHeight - 1, width - 2, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                    TERMX - width + 1 - VIEW_OFFSET_X);
    WINDOW_PTR w_monster_infoptr( w_monster_info );
    WINDOW *w_monster_info_border = newwin(iInfoHeight, width, TERMY - iInfoHeight - VIEW_OFFSET_Y,
                                           TERMX - width - VIEW_OFFSET_X);
    WINDOW_PTR w_monster_info_borderptr( w_monster_info_border );

    uistate.list_item_mon = 2; // remember we've tabbed here

    const int iWeaponRange = u.weapon.gun_range(&u);

    const int iStoreViewOffsetX = u.view_offset_x;
    const int iStoreViewOffsetY = u.view_offset_y;

    u.view_offset_x = 0;
    u.view_offset_y = 0;

    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2 - 1;
    int iStartPos = 0;
    int iActiveX = 0;
    int iActiveY = 0;
    int iLastActiveX = -1;
    int iLastActiveY = -1;
    Creature *cCurMon = nullptr;

    for (int i = 1; i < TERMX; i++) {
        if (i < width) {
            mvwputch(w_monsters_border, 0, i, c_ltgray, LINE_OXOX); // -
            mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, c_ltgray,
                     LINE_OXOX); // -
        }

        if (i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2) {
            mvwputch(w_monsters_border, i, 0, c_ltgray, LINE_XOXO); // |
            mvwputch(w_monsters_border, i, width - 1, c_ltgray, LINE_XOXO); // |
        }
    }

    mvwputch(w_monsters_border, 0, 0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w_monsters_border, 0, width - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, c_ltgray,
             LINE_XXXO); // |-
    mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, c_ltgray,
             LINE_XOXX); // -|

    mvwprintz(w_monsters_border, 0, 2, c_ltgreen, "<Tab> ");
    wprintz(w_monsters_border, c_white, _("Monsters"));

    std::string action;
    input_context ctxt("DEFAULTMODE");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("QUIT");
    ctxt.register_action("look");
    ctxt.register_action("fire");
    ctxt.register_action("HELP_KEYBINDINGS");

    do {
            if (action == "UP") {
                iActive--;
                if (iActive < 0) {
                    iActive = iMonsterNum - 1;
                }
            } else if (action == "DOWN") {
                iActive++;
                if (iActive >= iMonsterNum) {
                    iActive = 0;
                }
            } else if (action == "NEXT_TAB" || action == "PREV_TAB") {
                u.view_offset_x = iStoreViewOffsetX;
                u.view_offset_y = iStoreViewOffsetY;
                return 1;
            } else if (action == "look") {
                point recentered = look_around();
                iLastActiveX = recentered.x;
                iLastActiveY = recentered.y;
            } else if (action == "fire") {
                if( cCurMon != nullptr &&
                    rl_dist( u.pos(), cCurMon->pos() ) <= iWeaponRange) {
                    last_target = mon_at( cCurMon->posx(), cCurMon->posy() );
                    u.view_offset_x = iStoreViewOffsetX;
                    u.view_offset_y = iStoreViewOffsetY;
                    return 2;
                }
            }

            if (vMonsters.empty()) {
                wrefresh(w_monsters_border);
                mvwprintz(w_monsters, 10, 2, c_white, _("You dont see any monsters around you!"));
            } else {
                if( static_cast<size_t>( iActive ) >= vMonsters.size() ) {
                    iActive = 0;
                }
                werase(w_monsters);

                calcStartPos(iStartPos, iActive, iMaxRows, iMonsterNum);

                cCurMon = vMonsters[iActive];
                iActiveX = cCurMon->posx() - u.posx();
                iActiveY = cCurMon->posy() - u.posy();

                const auto endY = std::min<int>( iMaxRows, iMonsterNum - iStartPos );
                for( int y = 0; y < endY; ++y ) {
                    const auto critter = vMonsters[y + iStartPos];
                    const bool selected = ( cCurMon == critter );
                    const auto m = dynamic_cast<monster*>( critter );
                    const auto p = dynamic_cast<npc*>( critter );

                        if( critter->sees( g->u ) ) {
                            mvwprintz(w_monsters, y, 0, c_yellow, "!");
                        }

                        if( m != nullptr ) {
                            mvwprintz(w_monsters, y, 1, selected ? c_ltgreen : c_white, "%s", m->name().c_str());
                        } else {
                            mvwprintz(w_monsters, y, 1, selected ? c_ltgreen : c_white, "%s", critter->disp_name().c_str());
                        }
                        nc_color color = c_white;
                        std::string sText = "";

                        if( m != nullptr ) {
                            m->get_HP_Bar(color, sText);
                        } else {
                            ::get_HP_Bar( critter->get_hp(), critter->get_hp_max(), color, sText, false );
                        }
                        mvwprintz(w_monsters, y, 22, color, "%s", sText.c_str());

                        if( m != nullptr ) {
                            m->get_Attitude(color, sText);
                        } else if( p != nullptr ) {
                            sText = npc_attitude_name( p->attitude );
                            color = p->symbol_color();
                        }
                        mvwprintz(w_monsters, y, 28, color, "%s", sText.c_str());

                        int numw = iMonsterNum > 9 ? 2 : 1;
                        mvwprintz(w_monsters, y, width - (6 + numw),
                                  (selected ? c_ltgreen : c_ltgray), "%*d %s",
                                  numw, trig_dist(0, 0, critter->posx() - u.posx(),
                                                  critter->posy() - u.posy()),
                                  direction_name_short(
                                      direction_from( 0, 0, critter->posx() - u.posx(),
                                                      critter->posy() - u.posy())).c_str() );
                }

                mvwprintz(w_monsters_border, 0, (width - 9) / 2 + ((iMonsterNum > 9) ? 0 : 1),
                          c_ltgreen, " %*d", ((iMonsterNum > 9) ? 2 : 1), iActive + 1);
                wprintz(w_monsters_border, c_white, " / %*d ", ((iMonsterNum > 9) ? 2 : 1), iMonsterNum);

                werase(w_monster_info);

                //print monster info
                cCurMon->print_info(w_monster_info, 1, 11, 1);

                mvwprintz(w_monsters, getmaxy(w_monsters) - 1, 1, c_ltgreen, "%s", ctxt.press_x( "look" ).c_str());
                wprintz(w_monsters, c_ltgray, " %s", _("to look around"));
                if (rl_dist( u.pos(), cCurMon->pos() ) <= iWeaponRange) {
                    wprintz(w_monsters, c_ltgray, "%s", " ");
                    wprintz(w_monsters, c_ltgreen, "%s", ctxt.press_x( "fire" ).c_str());
                    wprintz(w_monsters, c_ltgray, " %s", _("to shoot"));
                }

                //Only redraw trail/terrain if x/y position changed
                if (iActiveX != iLastActiveX || iActiveY != iLastActiveY) {
                    iLastActiveX = iActiveX;
                    iLastActiveY = iActiveY;
                    centerlistview(iActiveX, iActiveY);
                    draw_trail_to_square(iActiveX, iActiveY, false);
                }
                //Draw Scrollbar
                draw_scrollbar(w_monsters_border, iActive, iMaxRows, iMonsterNum, 1);
            }

            for (int j = 0; j < iInfoHeight - 1; j++) {
                mvwputch(w_monster_info_border, j, 0, c_ltgray, LINE_XOXO);
                mvwputch(w_monster_info_border, j, width - 1, c_ltgray, LINE_XOXO);
            }

            for (int j = 0; j < width - 1; j++) {
                mvwputch(w_monster_info_border, iInfoHeight - 1, j, c_ltgray, LINE_OXOX);
            }

            mvwputch(w_monster_info_border, iInfoHeight - 1, 0, c_ltgray, LINE_XXOO);
            mvwputch(w_monster_info_border, iInfoHeight - 1, width - 1, c_ltgray, LINE_XOOX);

            wrefresh(w_monsters);
            wrefresh(w_monster_info_border);
            wrefresh(w_monster_info);

            refresh();

            action = ctxt.handle_input();
    } while (action != "QUIT");

    u.view_offset_x = iStoreViewOffsetX;
    u.view_offset_y = iStoreViewOffsetY;

    return -1;
}

// Establish or release a grab on a vehicle
void game::grab()
{
    int grabx = 0;
    int graby = 0;
    if (0 != u.grab_point.x || 0 != u.grab_point.y) {
        vehicle *veh = m.veh_at(u.posx() + u.grab_point.x, u.posy() + u.grab_point.y);
        if (veh) {
            add_msg(_("You release the %s."), veh->name.c_str());
        } else if (m.has_furn(u.posx() + u.grab_point.x, u.posy() + u.grab_point.y)) {
            add_msg(_("You release the %s."), m.furnname(u.posx() + u.grab_point.x,
                    u.posy() + u.grab_point.y).c_str());
        }
        u.grab_point.x = 0;
        u.grab_point.y = 0;
        u.grab_type = OBJECT_NONE;
        return;
    }
    if (choose_adjacent(_("Grab where?"), grabx, graby)) {
        vehicle *veh = m.veh_at(grabx, graby);
        if (veh != NULL) { // If there's a vehicle, grab that.
            u.grab_point.x = grabx - u.posx();
            u.grab_point.y = graby - u.posy();
            u.grab_type = OBJECT_VEHICLE;
            add_msg(_("You grab the %s."), veh->name.c_str());
        } else if (m.has_furn(grabx, graby)) { // If not, grab furniture if present
            if (m.furn_at(grabx, graby).move_str_req < 0) {
                add_msg(_("You can not grab the %s"), m.furnname(grabx, graby).c_str());
                return;
            }
            u.grab_point.x = grabx - u.posx();
            u.grab_point.y = graby - u.posy();
            u.grab_type = OBJECT_FURNITURE;
            if (!m.can_move_furniture(grabx, graby, &u)) {
                add_msg(_("You grab the %s. It feels really heavy."), m.furnname(grabx, graby).c_str());
            } else {
                add_msg(_("You grab the %s."), m.furnname(grabx, graby).c_str());
            }
        } else { // todo: grab mob? Captured squirrel = pet (or meat that stays fresh longer).
            add_msg(m_info, _("There's nothing to grab there!"));
        }
    } else {
        add_msg(_("Never Mind."));
    }
}

// Handle_liquid returns false if we didn't handle all the liquid.
bool game::handle_liquid(item &liquid, bool from_ground, bool infinite, item *source,
                         item *cont)
{
    if( !liquid.made_of(LIQUID) ) {
        dbg(D_ERROR) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg("Tried to handle_liquid a non-liquid!");
        return false;
    }

    if( (liquid.type->id == "gasoline" || liquid.type->id == "diesel") &&
         vehicle_near() && query_yn(_("Refill vehicle?")) ) {
        int vx = u.posx(), vy = u.posy();
        refresh_all();
        if (!choose_adjacent(_("Refill vehicle where?"), vx, vy)) {
            return false;
        }
        vehicle *veh = m.veh_at(vx, vy);
        if (veh == NULL) {
            add_msg(m_info, _("There isn't any vehicle there."));
            return false;
        }
        const ammotype ftype = liquid.type->id;
        int fuel_cap = veh->fuel_capacity(ftype);
        int fuel_amnt = veh->fuel_left(ftype);
        if (fuel_cap <= 0) {
            add_msg(m_info, _("The %s doesn't use %s."),
                    veh->name.c_str(), ammo_name(ftype).c_str());
            return false;
        } else if (fuel_amnt >= fuel_cap) {
            add_msg(m_info, _("The %s is already full."),
                    veh->name.c_str());
            return false;
        } else if (from_ground && query_yn(_("Pump until full?"))) {
            u.assign_activity(ACT_REFILL_VEHICLE, 2 * (fuel_cap - fuel_amnt));
            u.activity.placement = point(vx, vy);
            return false; // Liquid is not handled by this function, but by the activity!
        }
        const int amt = infinite ? INT_MAX : liquid.charges;
        u.moves -= 100;
        liquid.charges = veh->refill(ftype, amt);
        if (veh->fuel_left(ftype) < fuel_cap) {
            add_msg(_("You refill the %s with %s."),
                    veh->name.c_str(), ammo_name(ftype).c_str());
        } else {
            add_msg(_("You refill the %s with %s to its maximum."),
                    veh->name.c_str(), ammo_name(ftype).c_str());
        }
        // infinite: always handled all, to prevent loops
        return infinite || liquid.charges == 0;
    }

    // Ask to pour rotten liquid (milk!) from the get-go
    int dirx, diry;
    const std::string liqstr = string_format(_("Pour %s where?"), liquid.tname().c_str());
    refresh_all();
    if (!from_ground && liquid.rotten() &&
        choose_adjacent(liqstr, dirx, diry)) {

        if (!m.can_put_items(dirx, diry)) {
            add_msg(m_info, _("You can't pour there!"));
            return false;
        }
        m.add_item_or_charges(dirx, diry, liquid, 1);
        return true;
    }

    if (cont == NULL || cont->is_null()) {
        const std::string text = string_format(_("Container for %s"), liquid.tname().c_str());

        // Check for a container on the ground.
        cont = inv_map_for_liquid(liquid, text);
        if (cont == NULL || cont->is_null()) {
            // No container selected (escaped, ...), ask to pour
            // we asked to pour rotten already
            if (!from_ground && !liquid.rotten() &&
                choose_adjacent(liqstr, dirx, diry)) {

                if (!m.can_put_items(dirx, diry)) {
                    add_msg(m_info, _("You can't pour there!"));
                    return false;
                }
                m.add_item_or_charges(dirx, diry, liquid, 1);
                return true;
            }
            add_msg(_("Never mind."));
            return false;
        }
    }

    if (cont == source) {
        //Source and destination are the same; abort
        add_msg(m_info, _("That's the same container!"));
        return false;

    } else if (liquid.is_ammo() && (cont->is_tool() || cont->is_gun())) {
        // for filling up chainsaws, jackhammers and flamethrowers
        ammotype ammo = "NULL";
        long max = 0;

        if (cont->is_tool()) {
            it_tool *tool = dynamic_cast<it_tool *>(cont->type);
            ammo = tool->ammo;
            max = tool->max_charges;
        } else {
            ammo = cont->type->gun->ammo;
            max = cont->type->gun->clip;
        }

        ammotype liquid_type = liquid.ammo_type();

        if (ammo != liquid_type) {
            add_msg(m_info, _("Your %s won't hold %s."), cont->tname().c_str(),
                    liquid.tname().c_str());
            return false;
        }

        if (max <= 0 || cont->charges >= max) {
            add_msg(m_info, _("Your %s can't hold any more %s."), cont->tname().c_str(),
                    liquid.tname().c_str());
            return false;
        }

        if (cont->charges > 0 && cont->has_curammo() && cont->get_curammo_id() != liquid.typeId()) {
            add_msg(m_info, _("You can't mix loads in your %s."), cont->tname().c_str());
            return false;
        }

        add_msg(_("You pour %s into the %s."), liquid.tname().c_str(), cont->tname().c_str());
        cont->set_curammo( liquid );
        if (infinite) {
            cont->charges = max;
        } else {
            cont->charges += liquid.charges;
            if (cont->charges > max) {
                long extra = cont->charges - max;
                cont->charges = max;
                liquid.charges = extra;
                add_msg(_("There's some left over!"));
                return false;
            }
        }
        return true;

    } else { // filling up normal containers
        std::string err;
        if( !cont->fill_with( liquid, err ) ) {
            add_msg( m_info, err.c_str() );
            return false;
        }

        u.inv.unsort();
        add_msg( _( "You pour %s into the %s." ), liquid.tname().c_str(), cont->tname().c_str() );
        if( !infinite && liquid.charges > 0 ) {
            add_msg( _( "There's some left over!" ) );
        }
        return infinite || liquid.charges <= 0;
    }
    return false;
}

//Move_liquid returns the amount of liquid left if we didn't move all the liquid, otherwise returns sentinel -1, signifies transaction fail.
//One-use, strictly for liquid transactions. Not intended for use with while loops.
int game::move_liquid(item &liquid)
{
    if (!liquid.made_of(LIQUID)) {
        dbg(D_ERROR) << "game:move_liquid: Tried to move_liquid a non-liquid!";
        debugmsg("Tried to move_liquid a non-liquid!");
        return -1;
    }

    //liquid is in fact a liquid.
    const std::string text = string_format(_("Container for %s"), liquid.tname().c_str());
    int pos = inv_for_liquid(liquid, text, false);

    //is container selected?
    if (u.has_item(pos)) {
        item *cont = &(u.i_at(pos));
        if (cont == NULL || cont->is_null()) {
            return -1;
        } else if (liquid.is_ammo() && (cont->is_tool() || cont->is_gun())) {
            // for filling up chainsaws, jackhammers and flamethrowers
            ammotype ammo = "NULL";
            int max = 0;

            if (cont->is_tool()) {
                it_tool *tool = dynamic_cast<it_tool *>(cont->type);
                ammo = tool->ammo;
                max = tool->max_charges;
            } else {
                ammo = cont->type->gun->ammo;
                max = cont->type->gun->clip;
            }

            ammotype liquid_type = liquid.ammo_type();

            if (ammo != liquid_type) {
                add_msg(m_info, _("Your %s won't hold %s."), cont->tname().c_str(),
                        liquid.tname().c_str());
                return -1;
            }

            if (max <= 0 || cont->charges >= max) {
                add_msg(m_info, _("Your %s can't hold any more %s."), cont->tname().c_str(),
                        liquid.tname().c_str());
                return -1;
            }

            if (cont->charges > 0 && cont->has_curammo() && cont->get_curammo_id() != liquid.typeId()) {
                add_msg(m_info, _("You can't mix loads in your %s."), cont->tname().c_str());
                return -1;
            }

            add_msg(_("You pour %s into your %s."), liquid.tname().c_str(),
                    cont->tname().c_str());
            cont->set_curammo( liquid );
            cont->charges += liquid.charges;
            if (cont->charges > max) {
                long extra = cont->charges - max;
                cont->charges = max;
                add_msg(_("There's some left over!"));
                return extra;
            } else {
                return 0;
            }
        } else {
            item tmp_liquid = liquid;
            std::string err;
            if( !cont->fill_with( tmp_liquid, err ) ) {
                add_msg( m_info, "%s", err.c_str() );
                return -1;
            }
            u.inv.unsort();
            if( tmp_liquid.charges == 0 ) {
                add_msg(_("You pour %s into your %s."), liquid.tname().c_str(), cont->type_name().c_str());
            } else {
                add_msg(_("You fill your %s with some of the %s."), cont->type_name().c_str(), liquid.tname().c_str());
                add_msg(_("There's some left over!"));
            }
            return tmp_liquid.charges;
        }
        return -1;
    }
    return -1;
}

void game::drop(int pos)
{
    if (pos == INT_MIN) {
        make_drop_activity( ACT_DROP, u.pos() );
    } else if (pos == -1 && u.weapon.has_flag("NO_UNWIELD")) {
        add_msg(m_info, _("You cannot drop your %s."), u.weapon.tname().c_str());
        return;
    } else {
        std::vector<item> dropped;
        std::vector<item> dropped_worn;
        if (pos <= -2) {
            if (!u.takeoff(pos, false, &dropped_worn)) {
                return;
            }
            u.moves -= 250; // same as game::takeoff
        } else {
            dropped.push_back(u.i_rem(pos));
        }
        drop(dropped, dropped_worn, 0, u.posx(), u.posy());
    }
}

void game::drop_in_direction()
{
    int dirx, diry;
    if (!choose_adjacent(_("Drop where?"), dirx, diry)) {
        return;
    }

    if (!m.can_put_items(dirx, diry)) {
        add_msg(m_info, _("You can't place items there!"));
        return;
    }

    make_drop_activity( ACT_DROP, point(dirx, diry) );
}

bool compare_items_by_lesser_volume(const item &a, const item &b)
{
    return a.volume() < b.volume();
}

// calculate the time (in player::moves) it takes to drop the
// items in dropped and dropped_worn.
// Items in dropped come from the main inventory (or the wielded weapon)
// Items in dropped_worn are cloth that had been worn.
// All items in dropped that fit into the removed storage space
// (freed_volume_capacity) do not take time to drop.
// Example: dropping five 2x4 (volume 5*6) and a worn backpack
// (storage 40) will take only the time for dropping the backpack
// dropping two more 2x4 takes the time for dropping the backpack and
// dropping the remaining 2x4 that does not fit into the backpack.
int game::calculate_drop_cost(std::vector<item> &dropped, const std::vector<item> &dropped_worn,
                              int freed_volume_capacity) const
{
    // Prefer to put small items into the backpack
    std::sort(dropped.begin(), dropped.end(), compare_items_by_lesser_volume);
    int drop_item_cnt = dropped_worn.size();
    int total_volume_dropped = 0;
    for( auto &elem : dropped ) {
        total_volume_dropped += elem.volume();
        if(freed_volume_capacity == 0 || total_volume_dropped > freed_volume_capacity) {
            drop_item_cnt++;
        }
    }
    return drop_item_cnt * 100;
}

void game::drop(std::vector<item> &dropped, std::vector<item> &dropped_worn,
                int freed_volume_capacity, int dirx, int diry)
{
    if (dropped.empty() && dropped_worn.empty()) {
        add_msg(_("Never mind."));
        return;
    }
    const int drop_move_cost = calculate_drop_cost(dropped, dropped_worn, freed_volume_capacity);
    add_msg( m_debug, "Dropping %d+%d items takes %d moves", dropped.size(), dropped_worn.size(),
                 drop_move_cost);

    dropped.insert(dropped.end(), dropped_worn.begin(), dropped_worn.end());

    int veh_part = 0;
    bool to_veh = false;
    vehicle *veh = m.veh_at(dirx, diry, veh_part);
    if (veh) {
        veh_part = veh->part_with_feature(veh_part, "CARGO");
        to_veh = veh_part >= 0;
    }

    bool can_move_there = m.move_cost(dirx, diry) != 0;

    itype_id first = itype_id(dropped[0].type->id);
    bool same = true;
    for (std::vector<item>::iterator it = dropped.begin() + 1;
         it != dropped.end() && same; ++it) {
        if (it->type->id != first) {
            same = false;
        }
    }

    if (dropped.size() == 1 || same) {
        int dropcount = (dropped[0].count_by_charges()) ? dropped[0].charges : 1;

        if (to_veh) {
            add_msg(ngettext("You put your %1$s in the %2$s's %3$s.",
                             "You put your %1$ss in the %2$s's %3$s.",
                             dropcount),
                    dropped[0].tname(dropcount).c_str(),
                    veh->name.c_str(),
                    veh->part_info(veh_part).name.c_str());
        } else if (can_move_there) {
            add_msg(ngettext("You drop your %s on the %s.",
                             "You drop your %s on the %s.", dropcount),
                    dropped[0].tname(dropcount).c_str(),
                    m.name(dirx, diry).c_str());
        } else {
            add_msg(ngettext("You put your %s in the %s.",
                             "You put your %s in the %s.", dropcount),
                    dropped[0].tname(dropcount).c_str(),
                    m.name(dirx, diry).c_str());
        }
    } else {
        if (to_veh) {
            add_msg(_("You put several items in the %s's %s."),
                    veh->name.c_str(), veh->part_info(veh_part).name.c_str());
        } else if (can_move_there) {
            add_msg(_("You drop several items on the %s."),
                    m.name(dirx, diry).c_str());
        } else {
            add_msg(_("You put several items in the %s."),
                    m.name(dirx, diry).c_str());
        }
    }

    if (to_veh) {
        bool vh_overflow = false;
        for( auto &elem : dropped ) {
            vh_overflow = vh_overflow || !veh->add_item( veh_part, elem );
            if (vh_overflow) {
                m.add_item_or_charges( dirx, diry, elem, 1 );
            }
        }
        if (vh_overflow) {
            add_msg (m_warning, _("The trunk is full, so some items fall on the ground."));
        }
    } else {
        for( auto &elem : dropped ) {
            m.add_item_or_charges( dirx, diry, elem, 2 );
        }
    }
    u.moves -= drop_move_cost;
}

void game::reassign_item( int pos )
{
    if( pos == INT_MIN ) {
        pos = inv( _( "Reassign item:" ) );
    }
    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item &change_from = u.i_at( pos );
    if( change_from.is_null() ) {
        return;
    }
    char newch = popup_getkey( _( "%s; enter new letter (press SPACE for none, ESCAPE to cancel)." ),
                               change_from.tname().c_str() );
    if( newch == ' ' ) {
        newch = 0;
    }
    if( newch == KEY_ESCAPE ) {
        add_msg( m_neutral, _( "Never mind." ) );
        return;
    }
    if( newch != 0 && inv_chars.find( newch ) == std::string::npos ) {
        add_msg( m_info, _( "%c is not a valid inventory letter." ), newch );
        return;
    }
    if( change_from.invlet == newch ) {
        // toggle assignment status
        auto iter = u.assigned_invlet.find(newch);
        if( iter == u.assigned_invlet.end() ) {
            u.assigned_invlet[newch] = change_from.typeId();
        } else {
            u.assigned_invlet.erase(iter);
        }
        return;
    }

    const int oldpos = newch == 0 ? INT_MIN : u.invlet_to_position( newch );
    if( oldpos != INT_MIN ) {
        item &change_to = u.i_at( oldpos );
        change_to.invlet = change_from.invlet;
        add_msg( m_info, "%c - %s", change_to.invlet == 0 ? ' ' : change_to.invlet,
                 change_to.tname().c_str() );
    }
    change_from.invlet = newch;
    u.assigned_invlet[newch] = change_from.typeId();
    add_msg( m_info, "%c - %s", newch == 0 ? ' ' : newch, change_from.tname().c_str() );
}

void game::plthrow(int pos)
{

    if (pos == INT_MIN) {
        pos = inv(_("Throw item:"));
        refresh_all();
    }

    int range = u.throw_range(pos);
    if (range < 0) {
        add_msg(m_info, _("You don't have that item."));
        return;
    } else if (range == 0) {
        add_msg(m_info, _("That is too heavy to throw."));
        return;
    }
    item thrown = u.i_at(pos);
    if (pos == -1 && thrown.has_flag("NO_UNWIELD")) {
        // pos == -1 is the weapon, NO_UNWIELD is used for bio_claws_weapon
        add_msg(m_info, _("That's part of your body, you can't throw that!"));
        return;
    }

    if (u.has_effect("relax_gas")) {
        if (one_in(5)) {
            add_msg(m_good, _("You concentrate mightily, and your body obeys!"));
        }
        else {
            u.moves -= rng(2, 5) * 10;
            add_msg(m_bad, _("You can't muster up the effort to throw anything..."));
            return;
        }
    }

    temp_exit_fullscreen();
    m.draw(w_terrain, point(u.posx(), u.posy()));

    int x = u.posx();
    int y = u.posy();

    // pl_target_ui() sets x and y, or returns empty vector if we canceled (by pressing Esc)
    std::vector <point> trajectory = pl_target_ui(x, y, range, &thrown, TARGET_MODE_THROW);
    if (trajectory.empty()) {
        return;
    }

    // Throw a single charge of a stacking object.
    if (thrown.count_by_charges() && thrown.charges > 1) {
        u.i_at(pos).charges--;
        thrown.charges = 1;
    } else {
        u.i_rem(pos);
    }

    // Base move cost on moves per turn of the weapon
    // and our skill.
    int move_cost = thrown.attack_time() / 2;
    int skill_cost = (int)(move_cost / (std::pow(u.skillLevel("throw"), 3.0f) / 400.0 + 1.0));
    int dexbonus = (int)(std::pow(std::max(u.dex_cur - 8, 0), 0.8) * 3.0);

    move_cost += skill_cost;
    move_cost += 20 * u.encumb(bp_torso);
    move_cost -= dexbonus;

    if (u.has_trait("LIGHT_BONES")) {
        move_cost *= .9;
    }
    if (u.has_trait("HOLLOW_BONES")) {
        move_cost *= .8;
    }

    if (move_cost < 25) {
        move_cost = 25;
    }

    u.moves -= move_cost;
    u.practice("throw", 10);

    throw_item(u, x, y, thrown, trajectory);
    reenter_fullscreen();
}

// TODO: Put this into a header (which one?) and maybe move the implementation somewhere else.
/** Comparator object to sort creatures according to their attitude from "u",
 * and (on same attitude) according to their distance to "u".
 */
struct compare_by_dist_attitude {
    const Creature &u;
    bool operator()(Creature *a, Creature *b) const;
};

bool compare_by_dist_attitude::operator()(Creature *a, Creature *b) const
{
    const auto aa = u.attitude_to( *a );
    const auto ab = u.attitude_to( *b );
    if( aa != ab ) {
        return aa < ab;
    }
    return rl_dist( a->pos(), u.pos() ) < rl_dist( b->pos(), u.pos() );
}

std::vector<point> game::pl_target_ui(int &x, int &y, int range, item *relevant, target_mode mode,
                                      int default_target_x, int default_target_y)
{
    // Populate a list of targets with the zombies in range and visible
    const Creature *last_target_critter = NULL;
    if (last_target >= 0 && !last_target_was_npc && size_t(last_target) < num_zombies()) {
        last_target_critter = &zombie(last_target);
    } else if (last_target >= 0 && last_target_was_npc && size_t(last_target) < active_npc.size()) {
        last_target_critter = active_npc[last_target];
    }
    auto mon_targets = u.get_visible_creatures( range );
    std::sort(mon_targets.begin(), mon_targets.end(), compare_by_dist_attitude { u } );
    int passtarget = -1;
    for (size_t i = 0; i < mon_targets.size(); i++) {
        Creature &critter = *mon_targets[i];
        critter.draw(w_terrain, u.posx(), u.posy(), true);
        // no default target, but found the last target
        if (default_target_x == -1 && last_target_critter == &critter) {
            passtarget = i;
            break;
        }
        if (default_target_x == critter.posx() && default_target_y == critter.posy()) {
            passtarget = i;
            break;
        }
    }
    // target() sets x and y, and returns an empty vector if we canceled (Esc)
    std::vector <point> trajectory = target(x, y, u.posx() - range, u.posy() - range,
                                            u.posx() + range, u.posy() + range,
                                            mon_targets, passtarget, relevant, mode);

    if (passtarget != -1) { // We picked a real live target
        // Make it our default for next time
        int id = npc_at(x, y);
        if (id >= 0) {
            last_target = id;
            last_target_was_npc = true;
            if(!active_npc[id]->is_enemy()){
                if (!query_yn(_("Really attack %s?"), active_npc[id]->name.c_str())) {
                    std::vector <point> trajectory_blank;
                    return trajectory_blank; // Cancel the attack
                } else {
                    //The NPC knows we started the fight, used for morale penalty.
                    active_npc[id]->hit_by_player = true;
                }
            }
            active_npc[id]->make_angry();
        } else {
            id = mon_at(x, y);
            if (id >= 0) {
                last_target = id;
                last_target_was_npc = false;
                zombie(last_target).add_effect("hit_by_player", 100);
            }
        }
    }
    return trajectory;
}

void game::plfire(bool burst, int default_target_x, int default_target_y)
{
    if (u.has_effect("relax_gas")) {
        if (one_in(5)) {
            add_msg(m_good, _("Your eyes steel, and you raise your weapon!"));
        } else {
            u.moves -= rng(2, 5) * 10;
            add_msg(m_bad, _("You can't fire your weapon, it's too heavy..."));
            return;
        }
    }
    // draw pistol from a holster if unarmed
    if (!u.is_armed()) {
        // get a list of holsters from worn items
        std::vector<item *> holsters;
        for( auto &worn : u.worn ) {
            if (((worn.type->can_use("HOLSTER_GUN") && !(worn.has_flag("NO_QUICKDRAW"))) || worn.type->can_use("HOLSTER_ANKLE")) &&
                (!worn.contents.empty() && worn.contents[0].is_gun())) {
                holsters.push_back(&worn);
            }
        }
        if (!holsters.empty()) {
            int choice = -1;
            // only one holster found, choose it
            if (holsters.size() == 1) {
                choice = 0;
                // ask player which holster to draw from
            } else {
                std::vector<std::string> choices;
                for( auto i : holsters ) {

                    std::ostringstream ss;
                    ss << string_format(_("%s from %s (%d)"),
                                        i->contents[0].tname().c_str(),
                                        i->type_name(1).c_str(),
                                        i->contents[0].charges);
                    choices.push_back(ss.str());
                }
                choice = (uimenu(false, _("Draw what?"), choices)) - 1;
            }

            if (choice > -1) {
                u.wield_contents(holsters[choice], true,  holsters[choice]->skill(), 13);
                u.add_msg_if_player(_("You pull your %s from its %s and ready it to fire."),
                                    u.weapon.tname().c_str(), holsters[choice]->type_name(1).c_str());
                if (u.weapon.charges <= 0) {
                    u.add_msg_if_player(_("... but it's empty!"));
                    return;
                }
            }
        }
    }

    int reload_pos = INT_MIN;
    if (!u.weapon.is_gun()) {
        return;
    }
    if( u.weapon.is_gunmod() ) {
        add_msg( m_info, _( "The %s must be attached to a gun, it can not be fired separately." ), u.weapon.tname().c_str() );
        return;
    }
    //below prevents fire burst key from fireing in burst mode in semiautos that have been modded
    //should be fine to place this here, plfire(true,*) only once in code
    if (burst && !u.weapon.has_flag("MODE_BURST")) {
        return;
    }

    vehicle *veh = m.veh_at(u.posx(), u.posy());
    if (veh && veh->player_in_control(&u) && u.weapon.is_two_handed(&u)) {
        add_msg(m_info, _("You need a free arm to drive!"));
        return;
    }
    if( !u.weapon.active && !u.weapon.is_in_auxiliary_mode() ) {
        if( u.weapon.activate_charger_gun( u ) ) {
            return;
        }
    }

    if (u.weapon.has_flag("NO_AMMO")) {
        u.weapon.charges = 1;
        u.weapon.set_curammo( "generic_no_ammo" );
    }

    if ((u.weapon.has_flag("STR8_DRAW") && u.str_cur < 4) ||
        (u.weapon.has_flag("STR10_DRAW") && u.str_cur < 5) ||
        (u.weapon.has_flag("STR12_DRAW") && u.str_cur < 6)) {
        add_msg(m_info, _("You're not strong enough to draw the bow!"));
        return;
    }

    if (u.weapon.has_flag("RELOAD_AND_SHOOT") && u.weapon.charges == 0) {
            // find worn quivers
            std::vector<item *> quivers;
            for( auto &worn : u.worn ) {

                if (worn.type->can_use("QUIVER") && !worn.contents.empty()
                    && worn.contents[0].is_ammo() && worn.contents[0].charges > 0
                    && worn.contents[0].ammo_type() == u.weapon.ammo_type() ) {
                    quivers.push_back(&worn);
                }
            }
            // ask which quiver to draw from
            if (!quivers.empty()) {
                int choice = -1;
                //only one quiver found, choose it
                if (quivers.size() == 1) {
                    choice = 0;
                } else {
                    std::vector<std::string> choices;
                    for( auto i : quivers ) {

                        std::ostringstream ss;
                        ss << string_format(_("%s from %s (%d)"),
                                            i->contents[0].tname().c_str(),
                                            i->type_name(1).c_str(),
                                            i->contents[0].charges);
                        choices.push_back(ss.str());
                    }
                    choice = (uimenu(false, _("Draw from which quiver?"), choices)) - 1;
                }

                // draw arrow from quiver
                if (choice > -1) {
                    item *worn = quivers[choice];
                    item &arrows = worn->contents[0];
                    // chance to fail pulling an arrow at lower levels
                    int archery = u.skillLevel("archery");
                    if (archery <= 2 && one_in(10)) {
                        u.moves -= 30;
                        u.add_msg_if_player(_("You try to pull a %s from your %s, but fail!"),
                                            arrows.tname().c_str(), worn->type_name(1).c_str());
                        return;
                    }
                    u.add_msg_if_player(_("You pull a %s from your %s and nock it."),
                                        arrows.tname().c_str(), worn->type_name(1).c_str());
                    reload_pos = u.get_item_position(worn);
                }
            }
        if (reload_pos == INT_MIN) {
            reload_pos = u.weapon.pick_reload_ammo(u, true);
        }
        if (reload_pos == INT_MIN) {
            add_msg(m_info, _("Out of ammo!"));
            return;
        }

        u.weapon.reload(u, reload_pos);
        u.moves -= u.weapon.reload_time(u);
        refresh_all();
    }

    if (u.weapon.num_charges() == 0 && !u.weapon.has_flag("RELOAD_AND_SHOOT") &&
        !u.weapon.has_flag("NO_AMMO")) {
        add_msg(m_info, _("You need to reload!"));
        return;
    }
    if (u.weapon.has_flag("FIRE_100") && u.weapon.num_charges() < 100) {
        add_msg(m_info, _("Your %s needs 100 charges to fire!"), u.weapon.tname().c_str());
        return;
    }
    if (u.weapon.has_flag("FIRE_50") && u.weapon.num_charges() < 50) {
        add_msg(m_info, _("Your %s needs 50 charges to fire!"), u.weapon.tname().c_str());
        return;
    }
    if (u.weapon.has_flag("FIRE_20") && u.weapon.num_charges() < 20) {
        add_msg(m_info, _("Your %s needs 20 charges to fire!"), u.weapon.tname().c_str());
        return;
    }
    const auto gun = u.weapon.type->gun.get();
    if( gun != nullptr && gun->ups_charges > 0 ) {
        const int ups_drain = gun->ups_charges;
        const int adv_ups_drain = std::min( 1, gun->ups_charges * 3 / 5 );
        const int bio_power_drain = std::min( 1, gun->ups_charges / 5 );
        if( !( u.has_charges( "UPS_off", ups_drain ) ||
               u.has_charges( "adv_UPS_off", adv_ups_drain ) ||
               (u.has_bionic( "bio_ups" ) && u.power_level >= bio_power_drain ) ) ) {
            add_msg( m_info,
                     _("You need a UPS with at least %d charges or an advanced UPS with at least %d charges to fire that!"),
                     ups_drain, adv_ups_drain );
            return;
        }
    }

    if (u.weapon.has_flag("MOUNTED_GUN")) {
        int vpart = -1;
        vehicle *veh = m.veh_at(u.posx(), u.posy(), vpart);
        if (!m.has_flag_ter_or_furn("MOUNTABLE", u.posx(), u.posy()) &&
            (veh == NULL || veh->part_with_feature(vpart, "MOUNTABLE") < 0)) {
            add_msg(m_info,
                    _("You need to be standing near acceptable terrain or furniture to use this weapon. A table, a mound of dirt, a broken window, etc."));
            return;
        }
    }

    int range = u.weapon.gun_range(&u);

    temp_exit_fullscreen();
    m.draw(w_terrain, point(u.posx(), u.posy()));

    int x = u.posx();
    int y = u.posy();

    std::vector<point> trajectory = pl_target_ui(x, y, range, &u.weapon, TARGET_MODE_FIRE,
                                                 default_target_x, default_target_y);

    if (trajectory.empty()) {
        if (u.weapon.has_flag("RELOAD_AND_SHOOT")) {
            u.moves += u.weapon.reload_time(u);
            unload(u.weapon);
            u.moves += u.weapon.reload_time(u) / 2; // unloading time
        }
        reenter_fullscreen();
        return;
    }
    draw_ter(); // Recenter our view

    if (u.weapon.get_gun_mode() == "MODE_BURST") {
        burst = true;
    }

    u.fire_gun(x, y, burst);
    reenter_fullscreen();
    //fire(u, x, y, trajectory, burst);
}

void game::butcher()
{
    if (u.controlling_vehicle) {
        add_msg(m_info, _("You can't butcher while driving!"));
        return;
    }

    const int factor = u.butcher_factor();
    bool has_corpse = false;
    bool has_item = false;
    // indices of corpses / items that can be disassembled
    std::vector<int> corpses;
    auto items = m.i_at(u.posx(), u.posy());
    const inventory &crafting_inv = u.crafting_inventory();
    bool has_salvage_tool = u.has_items_with_quality( "CUT", 1, 1 );

    // check if we have a butchering tool
    if( factor > INT_MIN ) {
        // get corpses
        for (size_t i = 0; i < items.size(); i++) {
            if( items[i].is_corpse() ) {
                corpses.push_back(i);
                has_corpse = true;
            }
        }
    }
    // then get items to disassemble
    for (size_t i = 0; i < items.size(); i++) {
        if( !items[i].is_corpse() ) {
            const recipe *cur_recipe = get_disassemble_recipe(items[i].type->id);
            if (cur_recipe != NULL && u.can_disassemble(&items[i], cur_recipe, crafting_inv, false)) {
                corpses.push_back(i);
                has_item = true;
            }
        }
    }
    // Now salvageable items
    size_t salvage_index = corpses.size();
    if( has_salvage_tool ) {
        for( size_t i = 0; i < items.size(); i++ ) {
            if( !items[i].is_corpse() ) {
                if( iuse::valid_to_cut_up( &items[i] ) ) {
                    corpses.push_back(i);
                    has_item = true;
                }
            }
        }
    }

    if (corpses.empty()) {
        if( factor > INT_MIN ) {
            add_msg(m_info, _("There are no corpses here to butcher."));
        } else {
            add_msg(m_info, _("You don't have a sharp item to butcher with."));
        }
        return;
    }

    Creature *hostile_critter = is_hostile_very_close();
    if (hostile_critter != nullptr) {
        if (!query_yn(_("You see %s nearby! Start butchering anyway?"),
                hostile_critter->disp_name().c_str()) ) {
            return;
        }
    }

    int butcher_corpse_index = 0;
    bool multisalvage = corpses.size() - salvage_index > 1;
    if (corpses.size() > 1) {
        uimenu kmenu;
        if( has_item && has_corpse ) {
            kmenu.text = _("Choose corpse to butcher / item to disassemble");
        } else if (has_corpse) {
            kmenu.text = _("Choose corpse to butcher");
        } else {
            kmenu.text = _("Choose item to disassemble");
        }
        kmenu.selected = 0;
        for (size_t i = 0; i < corpses.size(); i++) {
            const item &it = items[corpses[i]];
            int hotkey = -1;
            // First entry gets a hotkey matching the butcher command.
            if (i == 0) {
                const long butcher_key = inp_mngr.get_previously_pressed_key();
                if (butcher_key != 0) {
                    hotkey = butcher_key;
                }
            }
            if (it.is_corpse()) {
                kmenu.addentry(i, true, hotkey, it.get_mtype()->nname());
            } else if( i < salvage_index ) {
                kmenu.addentry(i, true, hotkey, it.tname());
            } else {
                std::stringstream ss;
                ss << _("Cut up") << " " << it.tname();
                kmenu.addentry( i, true, hotkey, ss.str() );
            }
        }
        if( multisalvage ) {
            kmenu.addentry(corpses.size(), true, 'z', _("Cut up all you can"));
        }
        kmenu.addentry(corpses.size() + multisalvage, true, 'q', _("Cancel"));
        kmenu.query();
        if( kmenu.ret == (int)corpses.size() + multisalvage ) {
            return;
        }
        butcher_corpse_index = kmenu.ret;
    }

    if( multisalvage == true && butcher_corpse_index == (int)corpses.size() ) {
        u.assign_activity( ACT_LONGSALVAGE, 0 );
        return;
    }
    const item &dis_item = items[corpses[butcher_corpse_index]];
    if( !dis_item.is_corpse() && butcher_corpse_index < (int)salvage_index) {
        const recipe *cur_recipe = get_disassemble_recipe(dis_item.type->id);
        assert(cur_recipe != NULL); // tested above
        if( !query_dissamble( dis_item ) ) {
            return;
        }
        u.assign_activity(ACT_DISASSEMBLE, cur_recipe->time, cur_recipe->id);
        u.activity.values.push_back(corpses[butcher_corpse_index]);
        u.activity.values.push_back(1);
        return;
    } else if( !dis_item.is_corpse() ) {
        item salvage_tool( "toolset", calendar::turn ); //TODO: Get the actual tool
        iuse::cut_up( &u, &salvage_tool, &items[corpses[butcher_corpse_index]], false );
        return;
    }
    mtype *corpse = dis_item.get_mtype();
    int time_to_cut = 0;
    switch (corpse->size) { // Time in turns to cut up te corpse
    case MS_TINY:
        time_to_cut = 2;
        break;
    case MS_SMALL:
        time_to_cut = 5;
        break;
    case MS_MEDIUM:
        time_to_cut = 10;
        break;
    case MS_LARGE:
        time_to_cut = 18;
        break;
    case MS_HUGE:
        time_to_cut = 40;
        break;
    }
    time_to_cut *= 100; // Convert to movement points
    time_to_cut -= factor * 5; // Penalty for poor tool or benefit for good tool
    if (time_to_cut < 250) {
        time_to_cut = 250;
    }
    u.assign_activity(ACT_BUTCHER, time_to_cut, corpses[butcher_corpse_index]);
}

void game::complete_butcher(int index)
{
    // corpses can disappear (rezzing!), so check for that
    if ((int)m.i_at(u.posx(), u.posy()).size() <= index ||
        !m.i_at(u.posx(), u.posy())[index].is_corpse()) {
        add_msg(m_info, _("There's no corpse to butcher!"));
        return;
    }
    mtype *corpse = m.i_at(u.posx(), u.posy())[index].get_mtype();
    std::vector<item> contents = m.i_at(u.posx(), u.posy())[index].contents;
    int age = m.i_at(u.posx(), u.posy())[index].bday;
    m.i_rem(u.posx(), u.posy(), index);
    int factor = u.butcher_factor();
    int pieces = 0, skins = 0, bones = 0, fats = 0, sinews = 0, feathers = 0;
    double skill_shift = 0.;

    int sSkillLevel = u.skillLevel("survival");

    switch (corpse->size) {
    case MS_TINY:
        pieces = 1;
        skins = 1;
        bones = 1;
        fats = 1;
        sinews = 1;
        feathers = 2;
        break;
    case MS_SMALL:
        pieces = 2;
        skins = 2;
        bones = 4;
        fats = 2;
        sinews = 4;
        feathers = 6;
        break;
    case MS_MEDIUM:
        pieces = 4;
        skins = 4;
        bones = 9;
        fats = 4;
        sinews = 9;
        feathers = 11;
        break;
    case MS_LARGE:
        pieces = 8;
        skins = 8;
        bones = 14;
        fats = 8;
        sinews = 14;
        feathers = 17;
        break;
    case MS_HUGE:
        pieces = 16;
        skins = 16;
        bones = 21;
        fats = 16;
        sinews = 21;
        feathers = 24;
        break;
    }

    skill_shift += rng(0, sSkillLevel - 3);
    skill_shift += rng(0, u.dex_cur - 8) / 4;
    if (u.str_cur < 4) {
        skill_shift -= rng(0, 5 * (4 - u.str_cur)) / 4;
    }
    if( factor < 0 ) {
        skill_shift -= rng( 0, -factor / 5 );
    }

    int practice = 4 + pieces;
    if (practice > 20) {
        practice = 20;
    }
    u.practice("survival", practice);

    pieces += int(skill_shift);
    if (skill_shift < 5)  { // Lose some skins and bones
        skins += ((int)skill_shift - 4);
        bones += ((int)skill_shift - 2);
        fats += ((int)skill_shift - 4);
        sinews += ((int)skill_shift - 8);
        feathers += ((int)skill_shift - 1);
    }

    if (bones > 0) {
         if (corpse->mat == "veggy") {
            m.spawn_item(u.posx(), u.posy(), "plant_sac", bones, 0, age);
            add_msg(m_good, _("You harvest some fluid bladders!"));
        } else if (corpse->has_flag(MF_BONES) && corpse->has_flag(MF_POISON)) {
            m.spawn_item(u.posx(), u.posy(), "bone_tainted", bones / 2, 0, age);
            add_msg(m_good, _("You harvest some salvageable bones!"));
        } else if (corpse->has_flag(MF_BONES) && corpse->has_flag(MF_HUMAN)) {
            m.spawn_item(u.posx(), u.posy(), "bone_human", bones, 0, age);
            add_msg(m_good, _("You harvest some salvageable bones!"));
        } else if (corpse->has_flag(MF_BONES)) {
            m.spawn_item(u.posx(), u.posy(), "bone", bones, 0, age);
            add_msg(m_good, _("You harvest some usable bones!"));
        }
    }

    if (sinews > 0) {
        if (corpse->has_flag(MF_BONES) && !corpse->has_flag(MF_POISON)) {
            m.spawn_item(u.posx(), u.posy(), "sinew", sinews, 0, age);
            add_msg(m_good, _("You harvest some usable sinews!"));
        } else if (corpse->mat == "veggy") {
            m.spawn_item(u.posx(), u.posy(), "plant_fibre", sinews, 0, age);
            add_msg(m_good, _("You harvest some plant fibers!"));
        }
    }

    if ((corpse->has_flag(MF_FUR) || corpse->has_flag(MF_LEATHER) ||
         corpse->has_flag(MF_CHITIN)) && skins > 0) {
        add_msg(m_good, _("You manage to skin the %s!"), corpse->nname().c_str());
        int fur = 0;
        int leather = 0;
        int chitin = 0;

        while (skins > 0) {
            if (corpse->has_flag(MF_CHITIN)) {
                chitin = rng(0, skins);
                skins -= chitin;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_FUR)) {
                fur = rng(0, skins);
                skins -= fur;
                skins = std::max(skins, 0);
            }
            if (corpse->has_flag(MF_LEATHER)) {
                leather = rng(0, skins);
                skins -= leather;
                skins = std::max(skins, 0);
            }
        }

        if (chitin) {
            m.spawn_item(u.posx(), u.posy(), "chitin_piece", chitin, 0, age);
        }
        if (fur) {
            m.spawn_item(u.posx(), u.posy(), "raw_fur", fur, 0, age);
        }
        if (leather) {
            m.spawn_item(u.posx(), u.posy(), "raw_leather", leather, 0, age);
        }
    }

    if (feathers > 0) {
        if (corpse->has_flag(MF_FEATHER)) {
            m.spawn_item(u.posx(), u.posy(), "feather", feathers, 0, age);
            add_msg(m_good, _("You harvest some feathers!"));
        }
    }

    if (fats > 0) {
        if (corpse->has_flag(MF_FAT) && corpse->has_flag(MF_POISON)) {
            m.spawn_item(u.posx(), u.posy(), "fat_tainted", fats, 0, age);
            add_msg(m_good, _("You harvest some gooey fat!"));
        } else if (corpse->has_flag(MF_FAT)) {
            m.spawn_item(u.posx(), u.posy(), "fat", fats, 0, age);
            add_msg(m_good, _("You harvest some fat!"));
        }
    }

    //Add a chance of CBM recovery. For shocker and cyborg corpses.
    if (corpse->has_flag(MF_CBM_CIV)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_common", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Zombie scientist bionics
    if (corpse->has_flag(MF_CBM_SCI)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_sci", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Zombie technician bionics
    if (corpse->has_flag(MF_CBM_TECH)) {
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_tech", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Substation mini-boss bionics
    if (corpse->has_flag(MF_CBM_SUBS)) {
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_subs", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_subs", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    // Payoff for butchering the zombie bio-op
    if (corpse->has_flag(MF_CBM_OP)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            add_msg(m_good, _("You discover a CBM in the %s!"), corpse->nname().c_str());
            //To see if it spawns a battery
            if (rng(0, 1) == 1) { //The battery works
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage_mkII", 1, 0, age);
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
        if (skill_shift >= 0) {
            //To see if it spawns a random additional CBM
            if (rng(0, 1) == 1) { //The CBM works
                m.put_items_from_loc( "bionics_op", u.posx(), u.posy(), age );
            } else { //There is a burnt out CBM
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }

    //Add a chance of CBM power storage recovery.
    if (corpse->has_flag(MF_CBM_POWER)) {
        //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
        if (skill_shift >= 0) {
            //To see if it spawns a battery
            if (one_in(3)) { //The battery works 33% of the time.
                add_msg(m_good, _("You discover a power storage in the %s!"), corpse->nname().c_str());
                m.spawn_item(u.posx(), u.posy(), "bio_power_storage", 1, 0, age);
            } else { //There is a burnt out CBM
                add_msg(m_good, _("You discover a fused lump of bio-circuitry in the %s!"),
                        corpse->nname().c_str());
                m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
            }
        }
    }


    // Recover hidden items
    for( auto &content : contents ) {
        if ((skill_shift + 10) * 5 > rng(0, 100)) {
            add_msg( m_good, _( "You discover a %s in the %s!" ), content.tname().c_str(),
                     corpse->nname().c_str() );
            m.add_item_or_charges( u.posx(), u.posy(), content );
        } else if( content.is_bionic() ) {
            m.spawn_item(u.posx(), u.posy(), "burnt_out_bionic", 1, 0, age);
        }
    }

    if (pieces <= 0) {
        add_msg(m_bad, _("Your clumsy butchering destroys the meat!"));
    } else {
        add_msg(m_good, _("You butcher the corpse."));
        const itype_id meat = corpse->get_meat_itype();
        if( meat == "null" ) {
            return;
        }
        item tmpitem(meat, age);
        tmpitem.set_mtype( corpse );
        while ( pieces > 0 ) {
            pieces--;
            m.add_item_or_charges(u.posx(), u.posy(), tmpitem);
        }
    }
}

void game::longsalvage()
{
    bool has_salvage_tool = u.has_items_with_quality( "CUT", 1, 1 );
    if( !has_salvage_tool ) {
        add_msg(m_bad, _("You no longer have the necessary tools to keep salvaging!"));
    }

    auto items = m.i_at(u.posx(), u.posy());
    item salvage_tool( "toolset", calendar::turn ); // TODO: Use actual tool
    for( auto it = items.begin(); it != items.end(); ++it ) {
        if( iuse::valid_to_cut_up( &*it ) ) {
            iuse::cut_up( &u, &salvage_tool, &*it, false );
            u.assign_activity( ACT_LONGSALVAGE, 0 );
            return;
        }
    }

    add_msg(_("You finish salvaging."));
    u.activity.type = ACT_NULL;
}

void game::forage()
{
    int veggy_chance = rng(1, 100);
    bool found_something = false;

    if (one_in(12)) {
        add_msg(m_good, _("You found some trash!"));
        m.put_items_from_loc( "trash_forest", u.posx(), u.posy(), calendar::turn );
        found_something = true;
    }
    // Compromise: Survival gives a bigger boost, and Peception is leveled a bit.
    if (veggy_chance < ((u.skillLevel("survival") * 1.5) + ((u.per_cur / 2 - 4) + 3))) {
        items_location loc;
        switch (calendar::turn.get_season()) {
        case SPRING:
            loc = "forage_spring";
            break;
        case SUMMER:
            loc = "forage_summer";
            break;
        case AUTUMN:
            loc = "forage_autumn";
            break;
        case WINTER:
            loc = "forage_winter";
            break;
        }
        int cnt = m.put_items_from_loc(loc, u.posx(), u.posy(), calendar::turn); // returns zero if location has no defined items
        if (cnt > 0) {
            add_msg(m_good, _("You found something!"));
            m.ter_set(u.activity.placement.x, u.activity.placement.y, t_dirt);
            found_something = true;
        }
    } else {
        if (one_in(2)) {
            m.ter_set(u.activity.placement.x, u.activity.placement.y, t_dirt);
        }
    }
    if (!found_something) {
        add_msg(_("You didn't find anything."));
    }

    //Determinate maximum level of skill attained by foraging using ones intelligence score
    int max_forage_skill = 0;
    if (u.int_cur < 4) {
        max_forage_skill = 1;
    } else if (u.int_cur < 6) {
        max_forage_skill = 2;
    } else if (u.int_cur < 8) {
        max_forage_skill = 3;
    } else if (u.int_cur < 11) {
        max_forage_skill = 4;
    } else if (u.int_cur < 15) {
        max_forage_skill = 5;
    } else if (u.int_cur < 20) {
        max_forage_skill = 6;
    } else if (u.int_cur < 26) {
        max_forage_skill = 7;
    } else if (u.int_cur > 25) {
        max_forage_skill = 8;
    }
    //Award experience for foraging attempt regardless of success
    u.practice("survival", rng(1, (max_forage_skill * 2) - (u.skillLevel("survival") * 2)),
               max_forage_skill);
}

void game::eat(int pos)
{
    if ((u.has_active_mutation("RUMINANT") || u.has_active_mutation("GRAZER")) &&
        m.ter(u.posx(), u.posy()) == t_underbrush && query_yn(_("Eat underbrush?"))) {
        u.moves -= 400;
        u.hunger -= 10;
        m.ter_set(u.posx(), u.posy(), t_grass);
        add_msg(_("You eat the underbrush."));
        return;
    }
    if (u.has_active_mutation("GRAZER") && m.ter(u.posx(), u.posy()) == t_grass &&
        query_yn(_("Graze?"))) {
        u.moves -= 400;
        if ((u.hunger < 10) || one_in(20 - u.int_cur)) {
            add_msg(_("You eat some of the taller grass, careful to leave some growing."));
            u.hunger -= 2;
        } else {
            add_msg(_("You eat the grass."));
            u.hunger -= 5;
            m.ter_set(u.posx(), u.posy(), t_dirt);
        }
        return;
    }
    if (pos == INT_MIN) {
        pos = inv_type(_("Consume item:"), IC_COMESTIBLE);
    }

    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }

    u.consume(pos);
}

void game::wear(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_type(_("Wear item:"), IC_ARMOR);
    }

    u.wear(pos);
}

void game::takeoff(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_type(_("Take off item:"), IC_NULL);
    }

    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }

    if (u.takeoff(pos)) {
        u.moves -= 250;    // TODO: Make this variable
    } else {
        add_msg(m_info, _("Invalid selection."));
    }
}

void game::reload(int pos)
{
    item *it = &u.i_at(pos);

    // Gun reloading is more complex.
    if (it->is_gun()) {

        // bows etc do not need to reload.
        if (it->has_flag("RELOAD_AND_SHOOT")) {
            add_msg(m_info, _("Your %s does not need to be reloaded, it reloads and fires "
                              "a single motion."), it->tname().c_str());
            return;
        }

        // Make sure the item is actually reloadable
        if (it->ammo_type() == "NULL") {
            add_msg(m_info, _("Your %s does not reload normally."), it->tname().c_str());
            return;
        }

        // See if the gun is fully loaded.
        if (it->charges == it->clip_size()) {
            // Also see if the spare magazine is loaded
            bool magazine_isfull = true;

            for(auto &con : it->contents) {
                if((con.is_gunmod() &&
                        (con.typeId() == "spare_mag" &&
                         con.charges < it->spare_mag_size())) ||
                    (con.is_auxiliary_gunmod() &&
                        (con.charges < con.clip_size()))) {
                    magazine_isfull = false;
                    break;
                }
            }

            if (magazine_isfull) {
                add_msg(m_info, _("Your %s is fully loaded!"), it->tname().c_str());
                return;
            }
        }

        // pick ammo
        int am_pos = it->pick_reload_ammo(u, true);
        if (am_pos == INT_MIN) {
            add_msg(m_info, _("Out of ammo!"));
            refresh_all();
            return;
        }

        // and finally reload.
        std::stringstream ss;
        ss << pos;
        u.assign_activity(ACT_RELOAD, it->reload_time(u), -1, am_pos, ss.str());

    } else if (it->is_tool()) { // tools are simpler
        it_tool *tool = dynamic_cast<it_tool *>(it->type);

        // see if its actually reloadable.
        if (tool->ammo == "NULL") {
            add_msg(m_info, _("You can't reload a %s!"), it->tname().c_str());
            return;
        } else if (it->has_flag("NO_RELOAD")) {
            add_msg(m_info, _("You can't reload a %s!"), it->tname().c_str());
            return;
        }

        // pick ammo
        int am_pos = it->pick_reload_ammo(u, true);

        if (am_pos == INT_MIN) {
            // no ammo, fail reload
            add_msg(m_info, _("Out of %s!"), ammo_name(tool->ammo).c_str());
            return;
        }

        // do the actual reloading
        std::stringstream ss;
        ss << pos;
        u.assign_activity(ACT_RELOAD, it->reload_time(u), -1, am_pos, ss.str());

    } else { // what else is there?
        add_msg(m_info, _("You can't reload a %s!"), it->tname().c_str());
    }

    // all done.
    refresh_all();
}

void game::reload()
{
    if (!u.is_armed()) {
        add_msg(m_info, _("You're not wielding anything."));
    } else {
        reload(-1);
    }
}

// Unload a container, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload(int pos)
{
    // this is necessary to prevent re-selection of the same item later
    item it = (u.inv.remove_item(pos));
    if (!it.is_null()) {
        unload(it);
        u.i_add(it);
    } else {
        item ite;
        if (pos == -1) { // item is wielded as weapon.
            ite = u.weapon;
            u.weapon = item("null", 0); //ret_null;
            unload(ite);
            u.weapon = ite;
            return;
        } else { //this is that opportunity for reselection where the original container is worn, see issue #808
            item &itm = u.i_at(pos);
            if (!itm.is_null()) {
                unload(itm);
            }
        }
    }
}

bool add_or_drop_with_msg( player &u, item &it )
{
    if( it.made_of( LIQUID ) ) {
        return g->handle_liquid( it, false, false, nullptr );
    }
    if( !u.can_pickVolume( it.volume() ) ) {
        add_msg( _( "There's no room in your inventory for the %s, so you drop it." ),
                 it.tname().c_str() );
        g->m.add_item_or_charges( u.posx(), u.posy(), it );
    } else if( !u.can_pickWeight( it.weight(), !OPTIONS["DANGEROUS_PICKUPS"] ) ) {
        add_msg( _( "The %s is too heavy to carry, so you drop it." ), it.tname().c_str() );
        g->m.add_item_or_charges( u.posx(), u.posy(), it );
    } else {
        auto &ni = u.i_add( it );
        add_msg( _( "You put the %s in your inventory." ), ni.tname().c_str() );
        add_msg( m_info, "%c - %s", ni.invlet == 0 ? ' ' : ni.invlet, ni.tname().c_str() );
    }
    return true;
}

void game::unload(item &it)
{
    // NOTE: changes here should also be applied to player::rate_action_unload to make the UI consistent
    if( it.is_container() && !it.contents.empty() ) {
        // TODO (or not): containers and guns use item::contents to store different things:
        // the gunmods / the container contents.
        // Maybe the gunmods should go into their own member?
        u.moves -= 40 * it.contents.size();
        std::vector<item> old_contents = it.contents;
        it.contents.clear();
        for( auto &content : old_contents ) {
            if( !add_or_drop_with_msg( u, content ) ) {
                it.contents.push_back( content );
            }
        }
        return;
    }
    if( it.is_gun() && !it.contents.empty() ) {
        // TODO: same as above: item::contents is shared with containers
        // Try unloading the active gunmod first as this is the most likely what the player wants.
        item *active_gunmod = it.active_gunmod();
        if( active_gunmod != nullptr && active_gunmod->charges > 0 ) {
            unload( *active_gunmod );
            return;
        }
        // Try to unload all the other gunmods.
        for( auto &gunmod : it.contents ) {
            if( gunmod.is_auxiliary_gunmod() && gunmod.charges > 0 ) {
                unload( gunmod );
                return;
            } else if( gunmod.typeId() == "spare_mag" && gunmod.charges > 0 ) {
                // TODO: ^^ this shall be a flag.
                unload( gunmod );
                return;
            }
        }
        // If neither the mods nor the gun itself are loaded, try to remove the mods instead.
        if( it.charges <= 0 ) {
            while( !it.contents.empty() ) {
                u.remove_gunmod( &it, 0 );
            }
            return;
        }
    }

    if( it.is_null() ) {
        add_msg(m_info, _("You're not wielding anything."));
        return;
    }
    // At this point, the contents have been handled, and the item itself must be unloaded.
    if( !it.is_gun() &&
        !it.is_auxiliary_gunmod() &&
        !( it.typeId() == "spare_mag" ) &&
        ( !it.is_tool() || it.ammo_type() == "NULL" ) ) {
        add_msg(m_info, _("You can't unload a %s!"), it.tname().c_str());
        return;
    }
    if( it.charges <= 0 ) {
        if( it.is_tool() ) {
            add_msg( m_info, _( "Your %s isn't charged." ), it.tname().c_str() );
        } else {
            add_msg( m_info, _( "Your %s isn't loaded." ), it.tname().c_str() );
        }
        return;
    }
    if (it.has_flag("NO_UNLOAD")) {
        if (it.has_flag("RECHARGE")) {
            add_msg(m_info, _("You can't unload a rechargeable %s!"), it.tname().c_str());
        } else {
            add_msg(m_info, _("You can't unload a %s!"), it.tname().c_str());
        }
        return;
    }

    u.moves -= int(it.reload_time(u) / 2);
    item *weapon = &it;

    item newam;

    if( weapon->has_curammo() ) {
        newam = item( weapon->get_curammo_id(), calendar::turn );
    } else {
        newam = item(default_ammo(weapon->ammo_type()), calendar::turn);
    }
    if( weapon->ammo_type() == "plutonium" ) {
        int chargesPerPlutonium = 500;
        int chargesRemoved = weapon->charges - (weapon->charges % chargesPerPlutonium);;
        int plutoniumRemoved = chargesRemoved / chargesPerPlutonium;
        if (chargesRemoved < weapon->charges) {
            add_msg(m_info, _("You can't remove partially depleted plutonium!"));
        }
        if (plutoniumRemoved > 0) {
            add_msg(_("You recover %i unused plutonium."), plutoniumRemoved);
            newam.charges = plutoniumRemoved;
            weapon->charges -= chargesRemoved;
        } else {
            return;
        }
    } else {
        newam.charges = weapon->charges;
        weapon->charges = 0;
    }

    if( !add_or_drop_with_msg( u, newam ) ) {
        weapon->charges += newam.charges; // Put it back in
    } else {
        add_msg( _( "You unload your %s." ), weapon->tname().c_str() );
    }
    // null the curammo, but only if we did empty the item
    if (weapon->charges == 0) {
        weapon->unset_curammo();
        // Tools need to be turned off, especially when thy consume charges only every few turns,
        // otherwise they stay active until they would consume the next charge.
        if( weapon->active && weapon->is_tool() && weapon->type->has_use() ) {
            weapon->type->invoke( &u, weapon, false, u.pos() );
        }
    }
}

void game::wield(int pos)
{
    if (u.weapon.has_flag("NO_UNWIELD")) {
        // Bionics can't be unwielded
        add_msg(m_info, _("You cannot unwield your %s."), u.weapon.tname().c_str());
        return;
    }
    if (pos == INT_MIN) {
        pos = inv(_("Wield item:"));
    }

    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }

    // Weapons need invlets to access, give one if not already assigned.
    item &it = u.i_at(pos);
    if (!it.is_null() && it.invlet == 0) {
        u.inv.assign_empty_invlet(it, true);
    }

    bool success = false;
    if (pos == -1) {
        success = u.wield(NULL);
    } else {
        success = u.wield(&(u.i_at(pos)));
    }

    if (success) {
        u.recoil = MIN_RECOIL;
    }
}

void game::read()
{
    int pos = inv_type(_("Read:"), IC_BOOK);

    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }
    draw();
    u.read(pos);
}

void game::chat()
{
    if (u.is_deaf()) {
        add_msg(m_info, _("You can't chat while deaf!"));
        return;
    }

    if (active_npc.empty()) {
        add_msg(_("You talk to yourself for a moment."));
        return;
    }

    std::vector<npc *> available;

    for( auto &elem : active_npc ) {
        if( u.sees( elem->pos() ) &&
            rl_dist( u.pos(), elem->pos() ) <= 24 ) {
            available.push_back( elem );
        }
    }

    if (available.empty()) {
        add_msg(m_info, _("There's no-one close enough to talk to."));
        return;
    } else if (available.size() == 1) {
        available[0]->talk_to_u();
    } else {
        std::vector<std::string> npcs;

        for( auto &elem : available ) {
            npcs.push_back( ( elem )->name );
        }
        npcs.push_back(_("Cancel"));

        int npc_choice = menu_vec(true, _("Who do you want to talk to?"), npcs) - 1;

        if (npc_choice >= 0 && size_t(npc_choice) < available.size()) {
            available[npc_choice]->talk_to_u();
        }
    }
    u.moves -= 100;
    refresh_all();
}

void game::pldrive(int x, int y)
{
    if( !check_save_mode_allowed() ) {
        return;
    }
    vehicle *veh = remoteveh();
    bool remote = true;
    int part = -1;
    if( !veh ) {
        veh = m.veh_at(u.posx(), u.posy(), part);
        remote = false;
    }
    if (!veh) {
        dbg(D_ERROR) << "game::pldrive: can't find vehicle! Drive mode is now off.";
        debugmsg("game::pldrive error: can't find vehicle! Drive mode is now off.");
        u.in_vehicle = false;
        return;
    }
    if( !remote ) {
        int pctr = veh->part_with_feature(part, "CONTROLS");
        if (pctr < 0) {
            add_msg(m_info, _("You can't drive the vehicle from here. You need controls!"));
            return;
        }
    } else {
        if ( veh->all_parts_with_feature( "REMOTE_CONTROLS", true ).size() == 0 ) {
            add_msg(m_info, _("Can't drive this vehicle remotely. It has no working controls."));
            return;
        }
    }

    int thr_amount = 10 * 100;
    if (veh->cruise_on) {
        veh->cruise_thrust(-y * thr_amount);
    } else {
        veh->thrust(-y);
    }
    veh->turn(15 * x);
    if (veh->skidding && veh->valid_wheel_config()) {
        if (rng(0, veh->velocity) < u.dex_cur + u.skillLevel("driving") * 2) {
            add_msg(_("You regain control of the %s."), veh->name.c_str());
            u.practice("driving", veh->velocity / 5);
            veh->velocity = int(veh->forward_velocity());
            veh->skidding = false;
            veh->move.init(veh->turn_dir);
        }
    }
    // Don't spend turns to adjust cruise speed.
    if (x != 0 || !veh->cruise_on) {
        u.moves = 0;
    }

    if (x != 0 && veh->velocity != 0 && one_in(10)) {
        u.practice("driving", 1);
    }
}

bool game::check_save_mode_allowed()
{
    if (u.has_effect("laserlocked")) {
        // Automatic and mandatory safemode.  Make BLOODY sure the player notices!
        safe_mode = SAFE_MODE_STOP;
        add_msg( m_warning,
             _( "You are being laser-targeted--safe mode is on! (%s to turn it off.)" ),
             press_x( ACTION_TOGGLE_SAFEMODE ).c_str() );
        // Effect is only here to hook into safemode, so remove it.
        u.remove_effect("laserlocked");
        return false;
    }
    if( safe_mode != SAFE_MODE_STOP ) {
        return true;
    }
    // Currently driving around, ignore the monster, they have no chance against a proper car anyway (-:
    if( u.controlling_vehicle && !OPTIONS["SAFEMODEVEH"] ) {
        return true;
    }
    // Monsters around and we don't wanna run
    std::string spotted_creature_name;
    if( new_seen_mon.empty() ) {
        // naming consistent with code in game::mon_info
        spotted_creature_name = _( "a hostile survivor" );
    } else {
        spotted_creature_name = zombie( new_seen_mon.back() ).name();
    }
    add_msg( m_warning,
             _( "Spotted %s--safe mode is on! (%s to turn it off or %s to ignore monster.)" ),
             spotted_creature_name.c_str(), press_x( ACTION_TOGGLE_SAFEMODE ).c_str(),
             from_sentence_case( press_x( ACTION_IGNORE_ENEMY ) ).c_str() );
    return false;
}

bool game::disable_robot( const point p )
{
    const int mondex = mon_at( p.x, p.y );
    if( mondex == -1 ) {
        return false;
    }
    monster &critter = zombie( mondex );
    if( critter.friendly == 0 ) {
        // Can only disable / reprogram friendly monsters
        return false;
    }
    const auto mid = critter.type->id;
    const auto mon_item_id = critter.type->revert_to_itype;
    if( !mon_item_id.empty() ) {
        if( !query_yn( _( "Deactivate the %s?" ), critter.name().c_str() ) ) {
            return false;
        }
        u.moves -= 100;
        m.add_item_or_charges( p.x, p.y, critter.to_item() );
        for( auto & ammodef : critter.ammo ) {
            if( ammodef.second > 0 ) {
                m.spawn_item( p.x, p.y, ammodef.first, 1, ammodef.second, calendar::turn );
            }
        }
        remove_zombie( mondex );
        return true;
    }
    // Manhacks are special, they have their own menu here.
    if( mid == "mon_manhack" ) {
        int choice = 0;
        if( critter.has_effect( "docile" ) ) {
            choice = menu( true, _( "Reprogram the manhack?" ), _( "Engage targets." ), _( "Cancel" ), NULL );
        } else {
            choice = menu( true, _( "Reprogram the manhack?" ), _( "Follow me." ), _( "Cancel" ), NULL );
        }
        switch( choice ) {
            case 1:
                if( critter.has_effect( "docile" ) ) {
                    critter.remove_effect( "docile" );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s hovers momentarily as it surveys the area." ),
                                 critter.name().c_str() );
                    }
                } else {
                    critter.add_effect( "docile", 1, num_bp, true );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s lets out a whirring noise and starts to follow you." ),
                                 critter.name().c_str() );
                    }
                }
                u.moves -= 100;
                return true;
        }
    }
    return false;
}

bool game::plmove(int dx, int dy)
{
    if( (!check_save_mode_allowed()) || u.has_active_mutation("SHELL2") ) {
        if ( u.has_active_mutation("SHELL2")) {
            add_msg(m_warning, _("You can't move while in your shell.  Deactivate it to go mobile."));
        }
        return false;
    }
    if (!u.move_effects()) {
        u.moves -= 100;
        return false;
    }
    int x = 0;
    int y = 0;
    if (u.has_effect("stunned")) {
        x = rng(u.posx() - 1, u.posx() + 1);
        y = rng(u.posy() - 1, u.posy() + 1);
    } else {
        x = u.posx() + dx;
        y = u.posy() + dy;
    }

    dbg(D_PEDANTIC_INFO) << "game:plmove: From (" << u.posx() << "," << u.posy() << ") to (" << x << "," <<
                         y << ")";

    if( disable_robot( point( x, y ) ) ) {
        return false;
    }

    // Check if our movement is actually an attack on a monster
    int mondex = mon_at(x, y);
    // Are we displacing a monster?  If it's vermin, always.
    bool displace = false;
    if (mondex != -1) {
        monster &critter = zombie(mondex);
        if (critter.friendly == 0 && !(critter.type->has_flag(MF_VERMIN))) {
            if (u.has_destination()) {
                add_msg(m_warning, _("Monster in the way. Auto-move canceled."));
                add_msg(m_info, _("Click directly on monster to attack."));
                u.clear_destination();
                return false;
            }
            if (u.has_effect("relax_gas")) {
                if (one_in(8)) {
                    add_msg(m_good, _("Your willpower asserts itself, and so do you!"));
                }
                else {
                    u.moves -= rng(2, 8) * 10;
                    add_msg(m_bad, _("You're too pacified to strike anything..."));
                    return false;
                }
            }
            u.melee_attack(critter, true);
            if (critter.is_hallucination()) {
                critter.die( &u );
            }
            draw_hit_mon(x, y, critter, critter.is_dead());
            return false;
        } else if( critter.has_flag( MF_IMMOBILE ) ) {
            add_msg( m_info, _( "You can't displace your %s." ), critter.name().c_str() );
            return false;
        } else {
            displace = true;
        }
    }
    // If not a monster, maybe there's an NPC there
    int npcdex = npc_at(x, y);
    if (npcdex != -1) {
        bool force_attack = false;
        if (!active_npc[npcdex]->is_enemy()) {
            if (!query_yn(_("Really attack %s?"), active_npc[npcdex]->name.c_str())) {
                if (active_npc[npcdex]->is_friend()) {
                    add_msg(_("%s moves out of the way."), active_npc[npcdex]->name.c_str());
                    active_npc[npcdex]->move_away_from(u.posx(), u.posy());
                }

                return false; // Cancel the attack
            } else {
                //The NPC knows we started the fight, used for morale penalty.
                active_npc[npcdex]->hit_by_player = true;
                force_attack = true;
            }
        }

        if (u.has_destination() && !force_attack) {
            add_msg(_("NPC in the way, Auto-move canceled."));
            add_msg(m_info, _("Click directly on NPC to attack."));
            u.clear_destination();
            return false;
        }

        u.melee_attack(*active_npc[npcdex], true);
        active_npc[npcdex]->make_angry();
        return false;
    }

    // GRAB: pre-action checking.
    int vpart0 = -1, vpart1 = -1, dpart = -1;
    vehicle *veh0 = m.veh_at(u.posx(), u.posy(), vpart0);
    vehicle *veh1 = m.veh_at(x, y, vpart1);
    bool pushing_furniture = false;  // moving -into- furniture tile; skip check for move_cost > 0
    bool pulling_furniture = false;  // moving -away- from furniture tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0
    bool pushing_vehicle = false;
    int movecost_modifier =
        0;       // pulling moves furniture into our origin square, so this changes to subtract it.

    if (u.grab_point.x != 0 || u.grab_point.y) {
        if (u.grab_type == OBJECT_VEHICLE) { // default; assume OBJECT_VEHICLE
            vehicle *grabbed_vehicle = m.veh_at(u.posx() + u.grab_point.x, u.posy() + u.grab_point.y);
            // If we're pushing a vehicle, the vehicle tile we'd be "stepping onto" is
            // actually the current tile.
            // If there's a vehicle there, it will actually result in failed movement.
            if (grabbed_vehicle == veh1) {
                pushing_vehicle = true;
                veh1 = veh0;
                vpart1 = vpart0;
            }
        } else if (u.grab_type == OBJECT_FURNITURE) {
            // Determine if furniture grab is valid,
            // and what we're wanting to do with it based on where it is and where we're going.
            point fpos(u.posx() + u.grab_point.x, u.posy() + u.grab_point.y);
            if (m.has_furn(fpos.x, fpos.y)) {
                pushing_furniture = (dx == u.grab_point.x && dy == u.grab_point.y);
                if (!pushing_furniture) {
                    point fdest(fpos.x + dx, fpos.y + dy);
                    pulling_furniture = (fdest.x == u.posx() && fdest.y == u.posy());
                }
                shifting_furniture = (pushing_furniture == false && pulling_furniture == false);
            }
        }
    }
    bool veh_closed_door = false;
    bool outside_vehicle = (!veh0 || veh0 != veh1);
    if (veh1) {
        dpart = veh1->next_part_to_open(vpart1, outside_vehicle);
        veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
    }

    if (veh0 && abs(veh0->velocity) > 100) {
        if (!veh1) {
            if (query_yn(_("Dive from moving vehicle?"))) {
                moving_vehicle_dismount(x, y);
            }
            return false;
        } else if (veh1 != veh0) {
            add_msg(m_info, _("There is another vehicle in the way."));
            return false;
        } else if (veh1->part_with_feature(vpart1, "BOARDABLE") < 0) {
            add_msg(m_info, _("That part of the vehicle is currently unsafe."));
            return false;
        }
    }

    bool toSwimmable = m.has_flag("SWIMMABLE", x, y);
    bool toDeepWater = m.has_flag(TFLAG_DEEP_WATER, x, y);
    bool fromSwimmable = m.has_flag("SWIMMABLE", u.posx(), u.posy());
    bool fromDeepWater = m.has_flag(TFLAG_DEEP_WATER, u.posx(), u.posy());
    bool fromBoat = veh0 && veh0->all_parts_with_feature(VPFLAG_FLOATS).size() > 0;
    bool toBoat = veh1 && veh1->all_parts_with_feature(VPFLAG_FLOATS).size() > 0;

    if (toSwimmable && toDeepWater && !toBoat) { // Dive into water!
        // Requires confirmation if we were on dry land previously
        if ((fromSwimmable && fromDeepWater && !fromBoat) || query_yn(_("Dive into the water?"))) {
            if ((!fromDeepWater || fromBoat) && u.swim_speed() < 500) {
                add_msg(_("You start swimming."));
                add_msg(m_info, _("%s to dive underwater."),
                        press_x(ACTION_MOVE_DOWN).c_str());
            }
            plswim(x, y);
        }
    } else if (m.move_cost(x, y) > 0 || pushing_furniture || shifting_furniture || pushing_vehicle) {
        // move_cost() of 0 = impassible (e.g. a wall)
        u.set_underwater(false);

        //Ask for EACH bad field, maybe not? Maybe say "theres X bad shit in there don't do it."
        const field &tmpfld = m.field_at(x, y);
        for( auto &fld : tmpfld ) {
            const field_entry &cur = fld.second;
            field_id curType = cur.getFieldType();
            bool dangerous = false;

            switch (curType) {
            case fd_smoke:
                dangerous = !(u.get_env_resist(bp_mouth) >= 7);
                break;
            case fd_tear_gas:
            case fd_toxic_gas:
            case fd_gas_vent:
            case fd_relax_gas:
                dangerous = !(u.get_env_resist(bp_mouth) >= 15);
                break;
            case fd_fungal_haze:
                dangerous = (!((u.get_env_resist(bp_mouth) >= 15) &&
                              (u.get_env_resist(bp_eyes) >= 15) ) &&
                              !u.has_trait("M_IMMUNE"));
                break;
            default:
                dangerous = cur.is_dangerous();
                break;
            }
            if ((dangerous) && !query_yn(_("Really step into that %s?"), cur.name().c_str())) {
                return false;
            }
        }

        const trap_id tid = m.tr_at(x, y);
        if (tid != tr_null) {
            const struct trap &t = *traplist[tid];
            if ((t.can_see(u, x, y)) && !t.is_benign() &&
                !query_yn(_("Really step onto that %s?"), t.name.c_str())) {
                return false;
            }
        }

        float drag_multiplier = 1.0;
        vehicle *grabbed_vehicle = NULL;
        if (u.grab_point.x != 0 || u.grab_point.y != 0) {
            // vehicle: pulling, pushing, or moving around the grabbed object.
            if (u.grab_type == OBJECT_VEHICLE) {
                grabbed_vehicle = m.veh_at(u.posx() + u.grab_point.x, u.posy() + u.grab_point.y);
                if (NULL != grabbed_vehicle) {
                    if (grabbed_vehicle == veh0) {
                        add_msg(m_info, _("You can't move %s while standing on it!"), grabbed_vehicle->name.c_str());
                        return false;
                    }
                    drag_multiplier += (float)(grabbed_vehicle->total_mass() * 1000) /
                        (float)(u.weight_capacity() * 5);
                    if (drag_multiplier > 2.0) {
                        add_msg(m_info, _("The %s is too heavy for you to budge!"), grabbed_vehicle->name.c_str());
                        return false;
                    }
                    tileray mdir;

                    int dxVeh = u.grab_point.x * (-1);
                    int dyVeh = u.grab_point.y * (-1);
                    int prev_grab_x = u.grab_point.x;
                    int prev_grab_y = u.grab_point.y;

                    if( abs(dx + dxVeh) == 2 || abs(dy + dyVeh) == 2 ||
                        ((dxVeh + dx) == 0 && (dyVeh + dy) == 0) ) {
                        //We are not moving around the veh
                        if ((dxVeh + dx) == 0 && (dyVeh + dy) == 0) {
                            //we are pushing in the direction of veh
                            dxVeh = dx;
                            dyVeh = dy;
                        } else {
                            u.grab_point.x = dx * (-1);
                            u.grab_point.y = dy * (-1);
                        }

                        if( (abs(dx + dxVeh) == 0 || abs(dy + dyVeh) == 0) &&
                            u.grab_point.x != 0 && u.grab_point.y != 0 ) {
                            //We are moving diagonal while veh is diagonal too and one direction is 0
                            dxVeh = ((dx + dxVeh) == 0) ? 0 : dxVeh;
                            dyVeh = ((dy + dyVeh) == 0) ? 0 : dyVeh;

                            u.grab_point.x = dxVeh * (-1);
                            u.grab_point.y = dyVeh * (-1);
                        }

                        mdir.init(dxVeh, dyVeh);
                        mdir.advance(1);
                        grabbed_vehicle->turn(mdir.dir() - grabbed_vehicle->face.dir());
                        grabbed_vehicle->face = grabbed_vehicle->turn_dir;
                        grabbed_vehicle->precalc_mounts(1, mdir.dir());
                        int imp = 0;
                        std::vector<veh_collision> veh_veh_colls;
                        std::vector<veh_collision> veh_misc_colls;
                        bool can_move = true;
                        // Set player location to illegal value so it can't collide with vehicle.
                        int player_prev_x = u.posx();
                        int player_prev_y = u.posy();
                        u.setx( 0 );
                        u.sety( 0 );
                        if (grabbed_vehicle->collision(veh_veh_colls, veh_misc_colls, dxVeh, dyVeh,
                                                       can_move, imp, true)) {
                            // TODO: figure out what we collided with.
                            add_msg(_("The %s collides with something."), grabbed_vehicle->name.c_str());
                            u.moves -= 10;
                            u.setx( player_prev_x );
                            u.sety( player_prev_y );
                            u.grab_point.x = prev_grab_x;
                            u.grab_point.y = prev_grab_y;
                            return false;
                        }
                        u.setx( player_prev_x );
                        u.sety( player_prev_y );

                        int gx = grabbed_vehicle->global_x();
                        int gy = grabbed_vehicle->global_y();
                        std::vector<int> wheel_indices =
                            grabbed_vehicle->all_parts_with_feature( "WHEEL", false );
                        for( auto p : wheel_indices ) {

                            if( one_in(2) ) {
                                grabbed_vehicle->handle_trap(
                                    gx + grabbed_vehicle->parts[p].precalc[0].x + dxVeh,
                                    gy + grabbed_vehicle->parts[p].precalc[0].y + dyVeh, p );
                            }
                        }
                        m.displace_vehicle(gx, gy, dxVeh, dyVeh);
                    } else {
                        //We are moving around the veh
                        u.grab_point.x = (dx + dxVeh) * (-1);
                        u.grab_point.y = (dy + dyVeh) * (-1);
                    }
                } else {
                    add_msg(m_info, _("No vehicle at grabbed point."));
                    u.grab_point.x = 0;
                    u.grab_point.y = 0;
                    u.grab_type = OBJECT_NONE;
                }
                // Furniture: pull, push, or standing still and nudging object around.
                // Can push furniture out of reach.
            } else if ( u.grab_type == OBJECT_FURNITURE ) {
                point fpos( u.posx() + u.grab_point.x, u.posy() + u.grab_point.y );
                // supposed position of grabbed furniture
                if ( ! m.has_furn( fpos.x, fpos.y ) ) {
                    // where'd it go? We're grabbing thin air so reset.
                    add_msg(m_info, _("No furniture at grabbed point.") );
                    u.grab_point = point(0, 0);
                    u.grab_type = OBJECT_NONE;
                } else {
                    point fdest( fpos.x + dx, fpos.y + dy ); // intended destination of furniture.
                    // Unfortunately, game::is_empty fails for tiles we're standing on,
                    // which will forbid pulling, so:
                    bool canmove = (
                        ( m.move_cost(fdest.x, fdest.y) > 0) &&
                        npc_at(fdest.x, fdest.y) == -1 &&
                        mon_at(fdest.x, fdest.y) == -1 &&
                        m.has_flag("FLAT", fdest.x, fdest.y) &&
                        !m.has_furn(fdest.x, fdest.y) &&
                        m.veh_at(fdest.x, fdest.y) == NULL &&
                        m.tr_at(fdest.x, fdest.y) == tr_null
                        );

                    const furn_t furntype = m.furn_at(fpos.x, fpos.y);
                    int furncost = furntype.movecost;
                    const int src_items = m.i_at(fpos.x, fpos.y).size();
                    const int dst_items = m.i_at(fdest.x, fdest.y).size();
                    bool dst_item_ok = ( ! m.has_flag("NOITEM", fdest.x, fdest.y) &&
                                         ! m.has_flag("SWIMMABLE", fdest.x, fdest.y) &&
                                         ! m.has_flag("DESTROY_ITEM", fdest.x, fdest.y) );
                    bool src_item_ok = ( m.furn_at(fpos.x, fpos.y).has_flag("CONTAINER") ||
                                         m.furn_at(fpos.x, fpos.y).has_flag("SEALED") );

                    int str_req = furntype.move_str_req;
                    // Factor in weight of items contained in the furniture.
                    int furniture_contents_weight = 0;
                    for( auto contained_item : m.i_at( fpos.x, fpos.y ) ) {
                        furniture_contents_weight += contained_item.weight();
                    }
                    str_req += furniture_contents_weight / 4000;

                    if ( ! canmove ) {
                        add_msg( _("The %s collides with something."), furntype.name.c_str() );
                        u.moves -= 50; // "oh was that your foot? Sorry :-O"
                        return false;
                    } else if ( str_req > u.get_str() &&
                                one_in(std::max(20 - str_req - u.get_str(), 2)) ) {
                        add_msg(m_bad, _("You strain yourself trying to move the heavy %s!"),
                                furntype.name.c_str() );
                        u.moves -= 100;
                        u.mod_pain(1); // Hurt ourself.
                        return false; // furniture and or obstacle wins.
                    } else if ( ! src_item_ok && dst_items > 0 ) {
                        add_msg( _("There's stuff in the way.") );
                        u.moves -= 50; // "oh was that your stuffed parrot? Sorry :-O"
                        return false;
                    }

                    if ( pulling_furniture ) {
                        // normalize movecost for pulling:
                        // furniture moves into our current square -then- we move away
                        if ( furncost < 0 ) {
                            // this will make our exit-tile move cost 0
                            movecost_modifier += m.ter_at(fpos.x, fpos.y).movecost;
                            // so add the base cost of our exit-tile's terrain.
                        } else {
                            // or it will think we're walking over the furniture we're pulling
                            movecost_modifier += ( 0 - furncost );
                            // so subtract the base cost of our furniture.
                        }
                    }

                    u.moves -= str_req * 10;
                    // Additional penalty if we can't comfortably move it.
                    if( str_req > u.get_str() ) {
                        int move_penalty = std::pow(str_req, 2.0) + 100.0;
                        if( move_penalty <= 1000 ) {
                            u.moves -= 100;
                            add_msg( m_bad, _("The %s is too heavy for you to budge."),
                                     furntype.name.c_str() );
                            return false;
                        }
                        u.moves -= move_penalty;
                        if (move_penalty > 500) {
                            add_msg( _("Moving the heavy %s is taking a lot of time!"),
                                     furntype.name.c_str() );
                        } else if (move_penalty > 200) {
                            if (one_in(3)) { // Nag only occasionally.
                                add_msg( _("It takes some time to move the heavy %s."),
                                         furntype.name.c_str() );
                            }
                        }
                    }
                    sound(x, y, furntype.move_str_req * 2, _("a scraping noise."));

                    m.furn_set(fdest.x, fdest.y, m.furn(fpos.x, fpos.y));    // finally move it.
                    m.furn_set(fpos.x, fpos.y, f_null);

                    if ( src_items > 0 ) {  // and the stuff inside.
                        if ( dst_item_ok && src_item_ok ) {
                            // Assume contents of both cells are legal, so we can just swap contents.
                            std::list<item> temp;
                            std::move( m.i_at(fpos.x, fpos.y).begin(), m.i_at(fpos.x, fpos.y).end(),
                                       std::back_inserter(temp) );
                            m.i_clear(fpos.x, fpos.y);
                            for( auto item_iter = m.i_at(fdest.x, fdest.y).begin();
                                 item_iter != m.i_at(fdest.x, fdest.y).end(); ++item_iter ) {
                                m.i_at(fpos.x, fpos.y).push_back( *item_iter );
                            }
                            m.i_clear(fdest.x, fdest.y);
                            for( auto item_iter = temp.begin(); item_iter != temp.end(); ++item_iter ) {
                                m.i_at(fdest.x, fdest.y).push_back( *item_iter );
                            }
                        } else {
                            add_msg(_("Stuff spills from the %s!"), furntype.name.c_str() );
                        }
                    }

                    if ( shifting_furniture ) { // we didn't move
                        if ( abs( u.grab_point.x + dx ) < 2 && abs( u.grab_point.y + dy ) < 2 ) {
                            u.grab_point = point ( u.grab_point.x + dx ,
                                                   u.grab_point.y + dy ); // furniture moved relative to us
                        } else { // we pushed furniture out of reach
                            add_msg( _("You let go of the %s"), furntype.name.c_str() );
                            u.grab_point = point (0, 0);
                            u.grab_type = OBJECT_NONE;
                        }
                        return false; // We moved furniture but stayed still.
                    } else if ( pushing_furniture &&
                                m.move_cost(x, y) <= 0 ) { // Not sure how that chair got into a wall, but don't let player follow.
                        add_msg( _("You let go of the %s as it slides past %s"),
                                 furntype.name.c_str(), m.ter_at(x, y).name.c_str() );
                        u.grab_point = point (0, 0);
                        u.grab_type = OBJECT_NONE;
                    }
                }
                // Unsupported!
            } else {
                add_msg(m_info, _("Nothing at grabbed point %d,%d."), u.grab_point.x, u.grab_point.y );
                u.grab_point.x = 0;
                u.grab_point.y = 0;
                u.grab_type = OBJECT_NONE;
            }
        }

        // Calculate cost of moving
        bool diag = trigdist && u.posx() != x && u.posy() != y;
        u.moves -= int(u.run_cost(m.combined_movecost(u.posx(), u.posy(), x, y, grabbed_vehicle,
                                                      movecost_modifier), diag) * drag_multiplier);

        // Adjust recoil down
        u.recoil -= int(u.str_cur / 2) + u.skillLevel("gun");
        u.recoil = std::max( MIN_RECOIL * 2, u.recoil );
        u.recoil = int(u.recoil / 2);
        if ((!u.has_trait("PARKOUR") && m.move_cost(x, y) > 2) ||
            ( u.has_trait("PARKOUR") && m.move_cost(x, y) > 4    )) {
            if (veh1 && m.move_cost(x, y) != 2) {
                add_msg(m_warning, _("Moving past this %s is slow!"), veh1->part_info(vpart1).name.c_str());
            } else {
                add_msg(m_warning, _("Moving past this %s is slow!"), m.name(x, y).c_str());
            }
        }
        if (veh1) {
            vehicle_part *part = &(veh1->parts[vpart1]);
            std::string label = veh1->get_label(part->mount.x, part->mount.y);
            if (label != "") {
                add_msg(m_info, _("Label here: %s"), label.c_str());
            }
        }

        std::string signage = m.get_signage(x, y);
        if (signage.size()) {
            add_msg(m_info, _("The sign says: %s"), signage.c_str());
        }
        if( m.has_graffiti_at( x, y ) ) {
            add_msg(_("Written here: %s"), utf8_truncate(m.graffiti_at( x, y ), 40).c_str());
        }
        if (m.has_flag("ROUGH", x, y) && (!u.in_vehicle)) {
            bool ter_or_furn = m.has_flag_ter( "ROUGH", x, y );
            if (one_in(5) && u.get_armor_bash(bp_foot_l) < rng(2, 5)) {
                add_msg(m_bad, _("You hurt your left foot on the %s!"),
                        ter_or_furn ? m.tername(x, y).c_str() : m.furnname(x, y).c_str() );
                u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
            }
            if (one_in(5) && u.get_armor_bash(bp_foot_r) < rng(2, 5)) {
                add_msg(m_bad, _("You hurt your right foot on the %s!"),
                        ter_or_furn ? m.tername(x, y).c_str() : m.furnname(x, y).c_str() );
                u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
            }
        }
        if( m.has_flag("SHARP", x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur / 2)) &&
            (!u.in_vehicle) && (!u.has_trait("PARKOUR") || one_in(4)) ) {
            bool ter_or_furn = m.has_flag_ter( "SHARP", x, y );
            body_part bp = random_body_part();
            if(u.deal_damage( nullptr, bp, damage_instance( DT_CUT, rng( 1, 4 ) ) ).total_damage() > 0) {
                //~ 1$s - bodypart name in accusative, 2$s is terrain name.
                add_msg(m_bad, _("You cut your %1$s on the %2$s!"),
                        body_part_name_accusative(bp).c_str(),
                        ter_or_furn ? m.tername(x, y).c_str() : m.furnname(x, y).c_str() );
                if ((u.has_trait("INFRESIST")) && (one_in(1024))) {
                u.add_effect("tetanus", 1, num_bp, true);
                } else if ((!u.has_trait("INFIMMUNE") || !u.has_trait("INFRESIST")) && (one_in(256))) {
                  u.add_effect("tetanus", 1, num_bp, true);
                 }
            }
        }
        if (m.has_flag("UNSTABLE", x, y)) {
            u.add_effect("bouldering", 1, num_bp, true);
        } else if (u.has_effect("bouldering")) {
            u.remove_effect("bouldering");
        }
        if (u.has_trait("LEG_TENT_BRACE") && (!u.footwear_factor() ||
                                                 (u.footwear_factor() == .5 && one_in(2)))) {
            // DX and IN are long suits for Cephalopods,
            // so this shouldn't cause too much hardship
            // Presumed that if it's swimmable, they're
            // swimming and won't stick
            if ((!(m.has_flag("SWIMMABLE", x, y)) && (one_in(80 + u.dex_cur + u.int_cur)))) {
                add_msg(_("Your tentacles stick to the ground, but you pull them free."));
                u.fatigue++;
            }
        }
        if (!u.has_artifact_with(AEP_STEALTH) && !u.has_trait("LEG_TENTACLES") &&
            !u.has_trait("DEBUG_SILENT")) {
            if (u.has_trait("LIGHTSTEP") || u.is_wearing("rm13_armor_on")) {
                sound(x, y, 2, "");    // Sound of footsteps may awaken nearby monsters
            } else if (u.has_trait("CLUMSY")) {
                sound(x, y, 10, "");
            } else if (u.has_bionic("bio_ankles")) {
                sound(x, y, 12, "");
            } else {
                sound(x, y, 6, "");
            }
        }
        if (one_in(20) && u.has_artifact_with(AEP_MOVEMENT_NOISE)) {
            sound(u.posx(), u.posy(), 40, _("You emit a rattling sound."));
        }
        // If we moved out of the nonant, we need update our map data
        if (m.has_flag("SWIMMABLE", x, y) && u.has_effect("onfire")) {
            add_msg(_("The water puts out the flames!"));
            u.remove_effect("onfire");
        }
        // displace is set at the top of this function.
        if (displace) { // We displaced a friendly monster!
            // Immobile monsters can't be displaced.
            monster &critter = zombie(mondex);
            critter.move_to(u.posx(), u.posy(),
                            true); // Force the movement even though the player is there right now.
            add_msg(_("You displace the %s."), critter.name().c_str());
        } // displace == true


        if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
            x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2))) {
            update_map(x, y);
        }

        // If the player is in a vehicle, unboard them from the current part
        if (u.in_vehicle) {
            m.unboard_vehicle(u.posx(), u.posy());
        }

        // Move the player
        u.setx( x );
        u.sety( y );
        if (dx != 0 || dy != 0) {
            u.lifetime_stats()->squares_walked++;
        }

        //Autopickup
        if (OPTIONS["AUTO_PICKUP"] && (!OPTIONS["AUTO_PICKUP_SAFEMODE"] || mostseen == 0) &&
            ((m.i_at(u.posx(), u.posy())).size() || OPTIONS["AUTO_PICKUP_ADJACENT"])) {
            Pickup::pick_up(u.posx(), u.posy(), -1);
        }

        // If the new tile is a boardable part, board it
        if (veh1 && veh1->part_with_feature(vpart1, "BOARDABLE") >= 0) {
            m.board_vehicle(u.posx(), u.posy(), &u);
        }

        // Traps!
        // Try to detect.
        u.search_surroundings();
        // We stepped on a trap!
        if (m.tr_at(x, y) != tr_null) {
            trap *tr = traplist[m.tr_at(x, y)];
            if (!u.avoid_trap(tr, x, y)) {
                tr->trigger(&u, x, y);
            }
        }

        // apply martial art move bonuses
        u.ma_onmove_effects();

        // Drench the player if swimmable
        if (m.has_flag("SWIMMABLE", x, y)) {
            u.drench(40, mfb(bp_foot_l) | mfb(bp_foot_r) | mfb(bp_leg_l) | mfb(bp_leg_r));
        }

        // List items here
        if (!m.has_flag("SEALED", x, y)) {
            if (u.has_effect("blind") && !m.i_at(x, y).empty()) {
                add_msg(_("There's something here, but you can't see what it is."));
            } else if (!m.i_at(x, y).empty()) {
                std::vector<std::string> names;
                std::vector<size_t> counts;
                std::vector<item> items;
                for( auto &tmpitem : m.i_at( x, y ) ) {

                    std::string next_tname = tmpitem.tname();
                    std::string next_dname = tmpitem.display_name();
                    bool by_charges = tmpitem.count_by_charges();
                    bool got_it = false;
                    for (size_t i = 0; i < names.size(); ++i) {
                        if (by_charges && next_tname == names[i]) {
                            counts[i] += tmpitem.charges;
                            got_it = true;
                            break;
                        } else if (next_dname == names[i]) {
                            counts[i] += 1;
                            got_it = true;
                            break;
                        }
                    }
                    if (!got_it) {
                        if (by_charges) {
                            names.push_back(tmpitem.tname(tmpitem.charges));
                            counts.push_back(tmpitem.charges);
                        } else {
                            names.push_back(tmpitem.display_name(1));
                            counts.push_back(1);
                        }
                        items.push_back(tmpitem);
                    }
                    if (names.size() > 10) {
                        break;
                    }
                }
                for( size_t i = 0; i < names.size(); ++i ) {
                    if (!items[i].count_by_charges()) {
                        names[i] = items[i].display_name(counts[i]);
                    } else {
                        names[i] = items[i].tname(counts[i]);
                    }
                }
                int and_the_rest = 0;
                for (size_t i = 0; i < names.size(); ++i) {
                    std::string fmt;
                    //~ number of items: "<number> <item>"
                    fmt = ngettext("%1$d %2$s", "%1$d %2$s", counts[i]);
                    names[i] = string_format(fmt, counts[i], names[i].c_str());
                    // Skip the first two.
                    if( i > 1 ) {
                        and_the_rest += counts[i];
                    }
                }
                if( names.size() == 1 ) {
                    add_msg(_("You see here %s."), names[0].c_str());
                } else if( names.size() == 2 ) {
                    add_msg(_("You see here %s and %s."),
                            names[0].c_str(), names[1].c_str());
                } else if( names.size() == 3 ) {
                    add_msg(_("You see here %s, %s, and %s."), names[0].c_str(),
                            names[1].c_str(), names[2].c_str());
                } else if( and_the_rest < 7 ) {
                    add_msg(ngettext("You see here %s, %s and %d more item.",
                                     "You see here %s, %s and %d more items.",
                                     and_the_rest),
                            names[0].c_str(), names[1].c_str(), and_the_rest);
                } else {
                    add_msg(_("You see here %s and many more items."),
                            names[0].c_str());
                }
            }
        }

        if( veh1 && veh1->part_with_feature(vpart1, "CONTROLS") >= 0 && u.in_vehicle ) {
            add_msg(_("There are vehicle controls here."));
            add_msg(m_info, _("%s to drive."),
                    press_x(ACTION_CONTROL_VEHICLE).c_str());
        }

    } else if( u.has_active_bionic("bio_probability_travel") && u.power_level >= 250 ) {
        //probability travel through walls but not water
        int tunneldist = 0;
        // tile is impassable
        while ((m.move_cost(x + tunneldist * (x - u.posx()), y + tunneldist * (y - u.posy())) == 0) ||
               // a monster is there
               ((mon_at(x + tunneldist * (x - u.posx()), y + tunneldist * (y - u.posy())) != -1 ||
                 // so keep tunneling
                 npc_at(x + tunneldist * (x - u.posx()), y + tunneldist * (y - u.posy())) != -1) &&
                // assuming we've already started
                tunneldist > 0)) {
            //add 1 to tunnel distance for each impassable tile in the line
            tunneldist += 1;
            if (tunneldist * 250 > u.power_level) { //oops, not enough energy! Tunneling costs 250 bionic power per impassable tile
                add_msg(_("You try to quantum tunnel through the barrier but are reflected! Try again with more energy!"));
                tunneldist = 0; //we didn't tunnel anywhere
                break;
            }
            if (tunneldist > 24) {
                add_msg(m_info, _("It's too dangerous to tunnel that far!"));
                tunneldist = 0;
                break;    //limit maximum tunneling distance
            }
        }
        if (tunneldist) { //you tunneled
            if (u.in_vehicle) {
                m.unboard_vehicle(u.posx(), u.posy());
            }
            u.power_level -= (tunneldist * 250); //tunneling costs 10 bionic power per impassable tile
            u.moves -= 100; //tunneling costs 100 moves
            //move us the number of tiles we tunneled in the x direction, plus 1 for the last tile.
            u.setx( u.posx() + (tunneldist + 1) * (x - u.posx()) );
            u.sety( u.posy() + (tunneldist + 1) * (y - u.posy()) ); //ditto for y
            add_msg(_("You quantum tunnel through the %d-tile wide barrier!"), tunneldist);
            if (m.veh_at(u.posx(), u.posy(), vpart1) &&
                m.veh_at(u.posx(), u.posy(), vpart1)->part_with_feature(vpart1, "BOARDABLE") >= 0) {
                m.board_vehicle(u.posx(), u.posy(), &u);
            }
        } else { //or you couldn't tunnel due to lack of energy
            u.power_level -= 250; //failure is expensive!
            return false;
        }

    } else if (veh_closed_door) {
        if (outside_vehicle) {
            veh1->open_all_at(dpart);
        } else {
            veh1->open(dpart);
            add_msg(_("You open the %s's %s."), veh1->name.c_str(),
                    veh1->part_info(dpart).name.c_str());
        }
        u.moves -= 100;
    } else { // Invalid move
        if (u.has_effect("blind") || u.has_effect("stunned")) {
            // Only lose movement if we're blind
            add_msg(_("You bump into a %s!"), m.name(x, y).c_str());
            u.moves -= 100;
        } else if (m.furn(x, y) != f_safe_c && m.open_door(x, y, !m.is_outside(u.posx(), u.posy()))) {
            u.moves -= 100;
        } else if (m.ter(x, y) == t_door_locked || m.ter(x, y) == t_door_locked_peep || m.ter(x, y) == t_door_locked_alarm ||
                   m.ter(x, y) == t_door_locked_interior) {
            u.moves -= 100;
            add_msg(_("That door is locked!"));
        } else if (m.ter(x, y) == t_door_bar_locked) {
            u.moves -= 80;
            add_msg(_("You rattle the bars but the door is locked!"));
        }
        return false;
    }

    //Only now can we be sure we actually moved
    on_move_effects();
    return true;
}

void game::on_move_effects()
{
    if (moveCount % 2 == 0) {
        if (u.has_bionic("bio_torsionratchet")) {
            u.charge_power(1);
        }
    }
}

void game::plswim(int x, int y)
{
    if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
        x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2))) {
        update_map(x, y);
    }
    if (!m.has_flag("SWIMMABLE", x, y)) {
        dbg(D_ERROR) << "game:plswim: Tried to swim in "
                     << m.tername(x, y).c_str() << "!";
        debugmsg("Tried to swim in %s!", m.tername(x, y).c_str());
        return;
    }
    if (u.has_effect("onfire")) {
        add_msg(_("The water puts out the flames!"));
        u.remove_effect("onfire");
    }
    int movecost = u.swim_speed();
    u.practice("swimming", u.is_underwater() ? 2 : 1);
    if (movecost >= 500) {
        if (!u.is_underwater() && !(u.shoe_type_count("swim_fins") == 2 ||
                                    (u.shoe_type_count("swim_fins") == 1 && one_in(2)))) {
            add_msg(m_bad, _("You sink like a rock!"));
            u.set_underwater(true);
            u.oxygen = 30 + 2 * u.str_cur;
        }
    }
    if (u.oxygen <= 5 && u.is_underwater()) {
        if (movecost < 500)
            popup(_("You need to breathe! (%s to surface.)"),
                  press_x(ACTION_MOVE_UP).c_str());
        else {
            popup(_("You need to breathe but you can't swim!  Get to dry land, quick!"));
        }
    }
    bool diagonal = (x != u.posx() && y != u.posy());
    if( u.in_vehicle ) {
        m.unboard_vehicle( u.posx(), u.posy() );
    }
    u.setx( x );
    u.sety( y );
    {
        int part;
        const auto veh = m.veh_at( u.posx(), u.posy(), part );
        if( veh != nullptr && veh->part_with_feature( part, VPFLAG_BOARDABLE ) >= 0 ) {
            m.board_vehicle( u.posx(), u.posy(), &u );
        }
    }
    u.moves -= (movecost > 200 ? 200 : movecost)  * (trigdist && diagonal ? 1.41 : 1);
    u.inv.rust_iron_items();

    int drenchFlags = mfb(bp_leg_l) | mfb(bp_leg_r) | mfb(bp_torso) | mfb(bp_arm_l) |
        mfb(bp_arm_r) | mfb(bp_foot_l) | mfb(bp_foot_r);

    if (get_temperature() <= 50) {
        drenchFlags |= mfb(bp_hand_l) | mfb(bp_hand_r);
    }

    if (u.is_underwater()) {
        drenchFlags |= mfb(bp_head) | mfb(bp_eyes) | mfb(bp_mouth) | mfb(bp_hand_l) | mfb(bp_hand_r);
    }
    u.drench(100, drenchFlags);
}

void game::fling_creature(Creature *c, const int &dir, float flvel, bool controlled)
{
    if( c == nullptr ) {
        debugmsg( "game::fling_creature invoked on null target" );
        return;
    }

    int steps = 0;
    const bool is_u = (c == &u);
    int dam1, dam2;

    player *p = dynamic_cast<player*>(c);
    monster *zz = dynamic_cast<monster*>(c);

    tileray tdir(dir);
    int range = flvel / 10;
    int x = c->posx();
    int y = c->posy();
    while (range > 0) {
        c->underwater = false;
        bool seen = is_u || u.sees( *c ); // To avoid redrawing when not seen
        tdir.advance();
        x = c->posx() + tdir.dx();
        y = c->posy() + tdir.dy();
        std::string dname;
        bool thru = true;
        bool slam = false;
        int mondex = mon_at(x, y);
        float previous_velocity = flvel;
        monster *critter = nullptr;

        dam1 = rng( flvel, flvel * 2.0 ) / 3;
        if (controlled) {
            dam1 = std::max((dam1 / 2) - 5, 0);
        }
        if (mondex >= 0) {
            critter = &zombie(mondex);
            slam = true;
            dname = critter->name();
            dam2 = rng( flvel, flvel * 2.0 );
            critter->apply_damage( c, bp_torso, dam2 );
            if( !critter->is_dead() ) {
                thru = false;
            }
        } else if (m.move_cost(x, y) == 0) {
            slam = true;
            int vpart;
            vehicle *veh = m.veh_at(x, y, vpart);
            dname = veh ? veh->part_info(vpart).name : m.tername(x, y).c_str();
            if (m.is_bashable(x, y)) {
                // Only go through if we successfully destroy what we hit
                thru = m.bash(x, y, flvel).second;
            } else {
                thru = false;
            }
        }
        if( slam ) {
            if( thru ) {
                flvel /= 2.0;
            } else {
                flvel = 0.0;
            }
            float velocity_difference = previous_velocity - flvel;
            dam1 = rng( velocity_difference, velocity_difference * 2.0 ) / 3;
            if( thru ) {
                c->add_msg_player_or_npc( _("You are slammed through the %s for %d damage!"),
                                          _("The <npcname> is slammed through the %s!"),
                                          dname.c_str(), dam1 );
            } else {
                c->add_msg_player_or_npc( _("You are slammed against the %s for %d damage!"),
                                          _("The <npcname> is slammed against the %s!"),
                                          dname.c_str(), dam1 );
            }
            if( p != nullptr ) {
                p->hitall(dam1, 40);
            } else {
                zz->apply_damage( critter, bp_torso, dam1 );
            }
        }
        if( thru ) {
            if( p != nullptr ) {
                // If we're flinging the player around, make sure the map stays centered on them.
                if( is_u && ( x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
                    x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)) ) ) {
                    update_map( x, y );
                }
                if (p->in_vehicle) {
                    m.unboard_vehicle(p->posx(), p->posy());
                }
                p->setx( x );
                p->sety( y );
            } else {
                zz->setpos(x, y);
            }
        } else {
            break;
        }
        range--;
        steps++;
        if( seen || u.sees( *c ) ) {
            draw();
        }
    }

    if (!m.has_flag("SWIMMABLE", x, y)) {
        // fall on ground
        dam1 = rng( flvel, flvel * 2.0 ) / 6;
        if (controlled) {
            dam1 = std::max(dam1 / 2 - 5, 0);
        }
        if( p != nullptr ) {
            int dex_reduce = p->dex_cur < 4 ? 4 : p->dex_cur;
            dam1 = dam1 * 8 / dex_reduce;
            if (p->has_trait("PARKOUR")) {
                dam1 /= 2;
            }
            if (dam1 > 0) {
                p->hitall(dam1, 40);
            }
        } else {
            zz->apply_damage( nullptr, bp_torso, dam1 );
        }
        if (is_u) {
            if (dam1 > 0) {
                add_msg(m_bad, _("You fall on the ground for %d damage."), dam1);
            } else if (!controlled) {
                add_msg(_("You land on the ground."));
            }
        }
    } else {
        c->underwater = true;
        if (is_u) {
            if (controlled) {
                add_msg(_("You dive into water."));
            } else {
                add_msg(m_warning, _("You fall into water."));
            }
        }
    }
}

void game::vertical_move(int movez, bool force)
{
    // Check if there are monsters are using the stairs.
    bool slippedpast = false;
    if (!coming_to_stairs.empty()) {
        // TODO: Allow travel if zombie couldn't reach stairs, but spawn him when we go up.
        add_msg(m_warning, _("You try to use the stairs. Suddenly you are blocked by a %s!"),
                coming_to_stairs[0].name().c_str());
        // Roll.
        int dexroll = dice(6, u.dex_cur + u.skillLevel("dodge") * 2);
        int strroll = dice(3, u.str_cur + u.skillLevel("melee") * 1.5);
        if (coming_to_stairs.size() > 4) {
            add_msg(_("The are a lot of them on the %s!"), m.tername(u.posx(), u.posy()).c_str());
            dexroll /= 4;
            strroll /= 2;
        } else if (coming_to_stairs.size() > 1) {
            add_msg(m_warning, _("There's something else behind it!"));
            dexroll /= 2;
        }

        if (dexroll < 14 || strroll < 12) {
            update_stair_monsters();
            u.moves -= 100;
            return;
        }

        if (dexroll >= 14) {
            add_msg(_("You manage to slip past!"));
        } else if (strroll >= 12) {
            add_msg(_("You manage to push past!"));
        }
        slippedpast = true;
        u.moves -= 100;
    }

    // > and < are used for diving underwater.
    if (m.has_flag("SWIMMABLE", u.posx(), u.posy()) && m.has_flag(TFLAG_DEEP_WATER, u.posx(), u.posy())) {
        if (movez == -1) {
            if (u.is_underwater()) {
                add_msg(m_info, _("You are already underwater!"));
                return;
            }
            if (u.worn_with_flag("FLOATATION")) {
                add_msg(m_info, _("You can't dive while wearing a flotation device."));
                return;
            }
            u.set_underwater(true);
            u.oxygen = 30 + 2 * u.str_cur;
            add_msg(_("You dive underwater!"));
        } else {
            if (u.swim_speed() < 500 || u.shoe_type_count("swim_fins") == 2 ||
                  (u.shoe_type_count("swim_fins") == 1 && one_in(2))) {
                u.set_underwater(false);
                add_msg(_("You surface."));
            } else {
                add_msg(m_info, _("You try to surface but can't!"));
            }
        }
        u.moves -= 100;
        return;
    }
    // Force means we're going down, even if there's no staircase, etc.
    // This happens with sinkholes and the like.
    if (!force && ((movez == -1 && !m.has_flag("GOES_DOWN", u.posx(), u.posy())) ||
                   (movez == 1 && !m.has_flag("GOES_UP", u.posx(), u.posy())))) {
        if (movez == -1) {
            add_msg(m_info, _("You can't go down here!"));
        } else {
            add_msg(m_info, _("You can't go up here!"));
        }
        return;
    }

    if (force) {
        // Let go of a grabbed cart.
        u.grab_point.x = 0;
        u.grab_point.y = 0;
    } else if (u.grab_point.x != 0 || u.grab_point.y != 0) {
        // TODO: Warp the cart along with you if you're on an elevator
        add_msg(m_info, _("You can't drag things up and down stairs."));
        return;
    }

    map tmpmap;
    tmpmap.load(levx, levy, levz + movez, false, cur_om);
    // Find the corresponding staircase
    int stairx = -1, stairy = -1;
    bool rope_ladder = false;

    const int omtilesz=SEEX * 2;
    real_coords rc( m.getabs(u.posx(), u.posy()) );

    point omtile_align_start(
        m.getlocal(rc.begin_om_pos())
    );

    if (force) {
        stairx = u.posx();
        stairy = u.posy();
    } else { // We need to find the stairs.
        int best = 999;
        bool danger_lava = false;
        for (int i = omtile_align_start.x; i <= omtile_align_start.x + omtilesz; i++) {
            for (int j = omtile_align_start.y; j <= omtile_align_start.y + omtilesz; j++) {
                if (rl_dist(u.posx(), u.posy(), i, j) <= best &&
                    ((movez == -1 && tmpmap.has_flag("GOES_UP", i, j)) ||
                     (movez == 1 && (tmpmap.has_flag("GOES_DOWN", i, j) ||
                                     tmpmap.ter(i, j) == t_manhole_cover)) ||
                     ((movez == 2 || movez == -2) && tmpmap.ter(i, j) == t_elevator))) {
                    stairx = i;
                    stairy = j;
                    best = rl_dist(u.posx(), u.posy(), i, j);
                }
                // Magic number used as double-shifting "best" added a bit of lag
                if (rl_dist(u.posx(), u.posy(), i, j) <= 3 && (tmpmap.ter(i, j) == t_lava)) {
                    danger_lava = true;
                }
            }
        }

        if (danger_lava && !query_yn(_("There is a LOT of heat coming out of there.  Descend anyway?")) ) {
            return;
        }
        if (stairx == -1 || stairy == -1) { // No stairs found!
            if (movez < 0) {
                if (tmpmap.move_cost(u.posx(), u.posy()) == 0) {
                    popup(_("Halfway down, the way down becomes blocked off."));
                    return;
                } else if (u.has_trait("WEB_RAPPEL")) {
                    if (query_yn(_("There is a sheer drop halfway down. Web-descend?"))) {
                        rope_ladder = true;
                        if ((rng(4, 8)) < (u.skillLevel("dodge"))) {
                            add_msg(_("You attach a web and dive down headfirst, flipping upright and landing on your feet."));
                        } else {
                            add_msg(_("You securely web up and work your way down, lowering yourself safely."));
                        }
                    } else {
                        return;
                    }
                } else if (u.has_trait("VINES2") || u.has_trait("VINES3")) {
                    if (query_yn(_("There is a sheer drop halfway down.  Use your vines to descend?"))) {
                        if (u.has_trait("VINES2")) {
                            if (query_yn(_("Detach a vine?  It'll hurt, but you'll be able to climb back up..."))) {
                                rope_ladder = true;
                                add_msg(m_bad, _("You descend on your vines, though leaving a part of you behind stings."));
                                u.mod_pain(5);
                                u.apply_damage( nullptr, bp_torso, 5 );
                                u.hunger += 10;
                                u.thirst += 10;
                            } else {
                                add_msg(_("You gingerly descend using your vines."));
                            }
                        } else {
                            add_msg(_("You effortlessly lower yourself and leave a vine rooted for future use."));
                            rope_ladder = true;
                            u.hunger += 10;
                            u.thirst += 10;
                        }
                    } else {
                        return;
                    }
                } else if (u.has_amount("grapnel", 1)) {
                    if (query_yn(_("There is a sheer drop halfway down. Climb your grappling hook down?"))) {
                        rope_ladder = true;
                        u.use_amount("grapnel", 1);
                    } else {
                        return;
                    }
                } else if (u.has_amount("rope_30", 1)) {
                    if (query_yn(_("There is a sheer drop halfway down. Climb your rope down?"))) {
                        rope_ladder = true;
                        u.use_amount("rope_30", 1);
                    } else {
                        return;
                    }

                } else if (u.has_amount("bullwhip", 1)) {
                    if (query_yn(_("There is a sheer drop halfway down. Use your whip to lower yourself?"))) {}
                    else {
                        return;
                    }
                } else if (!query_yn(_("There is a sheer drop halfway down.  Jump?"))) {
                    return;
                }
            }
            stairx = u.posx();
            stairy = u.posy();
        }
    }

    if (!force) {
        monstairx = levx;
        monstairy = levy;
        monstairz = levz;
    }
    // Save all monsters that can reach the stairs, remove them from the tracker,
    // then despawn the remaining monsters. Because it's a vertical shift, all
    // monsters are out of the bounds of the map and will despawn.
    for( unsigned int i = 0; i < num_zombies(); ) {
        monster &critter = zombie(i);
        int turns = critter.turns_to_reach(u.posx(), u.posy());
        if (turns < 10 && coming_to_stairs.size() < 8 && critter.will_reach(u.posx(), u.posy())
            && !slippedpast) {
            critter.staircount = 10 + turns;
            coming_to_stairs.push_back(critter);
            remove_zombie( i );
        } else {
            i++;
        }
    }
    shift_monsters( 0, 0, movez );

    // Clear current scents.
    for (int x = u.posx() - SCENT_RADIUS; x <= u.posx() + SCENT_RADIUS; x++) {
        for (int y = u.posy() - SCENT_RADIUS; y <= u.posy() + SCENT_RADIUS; y++) {
            grscent[x][y] = 0;
        }
    }

    // Figure out where we know there are up/down connectors
    // Fill in all the tiles we know about (e.g. subway stations)
    static const int REVEAL_RADIUS = 40;
    const tripoint gpos = om_global_location();
    int z_coord = levz + movez;
    for (int x = -REVEAL_RADIUS; x <= REVEAL_RADIUS; x++) {
        for (int y = -REVEAL_RADIUS; y <= REVEAL_RADIUS; y++) {
            const int cursx = gpos.x + x;
            const int cursy = gpos.y + y;
            if (!overmap_buffer.seen(cursx, cursy, levz)) {
                continue;
            }
            if (overmap_buffer.has_note(cursx, cursy, z_coord)) {
                // Already has a note -> never add an AUTO-note
                continue;
            }
            const oter_id &ter = overmap_buffer.ter(cursx, cursy, levz);
            const oter_id &ter2 = overmap_buffer.ter(cursx, cursy, z_coord);
            if (!!OPTIONS["AUTO_NOTES"]) {
                if (movez == +1 && otermap[ter].known_up && !otermap[ter2].known_down) {
                    overmap_buffer.set_seen(cursx, cursy, z_coord, true);
                    overmap_buffer.add_note(cursx, cursy, z_coord, _(">:W;AUTO: goes down"));
                }
                if (movez == -1 && otermap[ter].known_down && !otermap[ter2].known_up) {
                    overmap_buffer.set_seen(cursx, cursy, z_coord, true);
                    overmap_buffer.add_note(cursx, cursy, z_coord, _("<:W;AUTO: goes up"));
                }
            }
        }
    }

    levz += movez;
    u.moves -= 100;
    m.clear_vehicle_cache();
    m.vehicle_list.clear();
    m.load( levx, levy, levz, true, cur_om );
    u.setx( stairx );
    u.sety( stairy );
    if (rope_ladder) {
        m.ter_set(u.posx(), u.posy(), t_rope_up);
    }
    if (m.ter(stairx, stairy) == t_manhole_cover) {
        m.spawn_item(stairx + rng(-1, 1), stairy + rng(-1, 1), "manhole_cover");
        m.ter_set(stairx, stairy, t_manhole);
    }

    m.spawn_monsters( true );

    if (force) { // Basically, we fell.
        if ((u.has_trait("WINGS_BIRD")) || ((one_in(2)) && (u.has_trait("WINGS_BUTTERFLY")))) {
            add_msg(_("You flap your wings and flutter down gracefully."));
        } else {
            int dam = int((u.str_max / 4) + rng(5, 10)) * rng(1, 3);//The bigger they are
            dam -= rng(u.get_dodge(), u.get_dodge() * 3);
            if (dam <= 0) {
                add_msg(_("You fall expertly and take no damage."));
            } else {
                add_msg(m_bad, _("You fall heavily, taking %d damage."), dam);
                u.hurtall(dam);
            }
        }
    }

    if (m.tr_at(u.posx(), u.posy()) != tr_null) { // We stepped on a trap!
        trap *tr = traplist[m.tr_at(u.posx(), u.posy())];
        if (force || !u.avoid_trap(tr, u.posx(), u.posy())) {
            tr->trigger(&u, u.posx(), u.posy());
        }
    }

    // Clear currently active npcs and reload them
    active_npc.clear();
    load_npcs();
    refresh_all();
}

void game::update_map( player *p )
{
    int x = p->posx();
    int y = p->posy();
    update_map( x, y );
    p->setx( x );
    p->sety( y );
}

void game::update_map(int &x, int &y)
{
    int shiftx = 0, shifty = 0;

    while (x < SEEX * int(MAPSIZE / 2)) {
        x += SEEX;
        shiftx--;
    }
    while (x >= SEEX * (1 + int(MAPSIZE / 2))) {
        x -= SEEX;
        shiftx++;
    }
    while (y < SEEY * int(MAPSIZE / 2)) {
        y += SEEY;
        shifty--;
    }
    while (y >= SEEY * (1 + int(MAPSIZE / 2))) {
        y -= SEEY;
        shifty++;
    }

    m.shift(shiftx, shifty);
    levx += shiftx;
    levy += shifty;

    real_coords rc( m.getabs( 0, 0 ) );
    if( cur_om->pos() != rc.abs_om ) {
        // lev[xy] must stay relative to cur_om, if we change cur_om, we have to change lev[xy]
        levx += ( cur_om->pos().x - rc.abs_om.x ) * OMAPX * 2;
        levy += ( cur_om->pos().y - rc.abs_om.y ) * OMAPY * 2;
        cur_om = &overmap_buffer.get( rc.abs_om.x, rc.abs_om.y );
    }

    // Shift monsters if we're actually shifting
    if (shiftx || shifty) {
        shift_monsters( shiftx, shifty, 0 );
        u.shift_destination(-shiftx * SEEX, -shifty * SEEY);
    }

    // Shift NPCs
    for (std::vector<npc *>::iterator it = active_npc.begin();
         it != active_npc.end();) {
        (*it)->shift(shiftx, shifty);
        if( (*it)->posx() < 0 - SEEX * 2 || (*it)->posy() < 0 - SEEX * 2 ||
            (*it)->posx() > SEEX * (MAPSIZE + 2) || (*it)->posy() > SEEY * (MAPSIZE + 2) ) {
            //Remove the npc from the active list. It remains in the overmap list.
            it = active_npc.erase(it);
        } else {
            it++;
        }
    }
    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();

    // Spawn monsters if appropriate
    m.spawn_monsters( false ); // Static monsters
    if (calendar::turn >= nextspawn) {
        spawn_mon(shiftx, shifty);
    }

    // Shift scent
    unsigned int newscent[SEEX * MAPSIZE][SEEY * MAPSIZE];
    for (int i = 0; i < SEEX * MAPSIZE; i++) {
        for (int j = 0; j < SEEY * MAPSIZE; j++) {
            newscent[i][j] = scent(i + (shiftx * SEEX), j + (shifty * SEEY));
        }
    }
    for (int i = 0; i < SEEX * MAPSIZE; i++) {
        for (int j = 0; j < SEEY * MAPSIZE; j++) {
            scent(i, j) = newscent[i][j];
        }
    }

    // Make sure map cache is consistent since it may have shifted.
    m.build_map_cache();

    // Update what parts of the world map we can see
    update_overmap_seen();
}

tripoint game::om_global_location() const
{
    const int cursx = (levx + int(MAPSIZE / 2)) / 2 + cur_om->pos().x * OMAPX;
    const int cursy = (levy + int(MAPSIZE / 2)) / 2 + cur_om->pos().y * OMAPY;

    return tripoint(cursx, cursy, levz);
}

void game::update_overmap_seen()
{
    const tripoint ompos = om_global_location();
    const int dist = u.overmap_sight_range(light_level());
    // We can always see where we're standing
    overmap_buffer.set_seen(ompos.x, ompos.y, ompos.z, true);
    for (int x = ompos.x - dist; x <= ompos.x + dist; x++) {
        for (int y = ompos.y - dist; y <= ompos.y + dist; y++) {
            const std::vector<point> line = line_to(ompos.x, ompos.y, x, y, 0);
            int sight_points = dist;
            for (std::vector<point>::const_iterator it = line.begin();
                 it != line.end() && sight_points >= 0; ++it) {
                const oter_id &ter = overmap_buffer.ter(it->x, it->y, ompos.z);
                const int cost = otermap[ter].see_cost;
                sight_points -= cost;
            }
            if (sight_points >= 0) {
                overmap_buffer.set_seen(x, y, ompos.z, true);
            }
        }
    }
}

point game::om_location() const
{
    point ret;
    ret.x = int((levx + int(MAPSIZE / 2)) / 2);
    ret.y = int((levy + int(MAPSIZE / 2)) / 2);

    return ret;
}

void game::replace_stair_monsters()
{
    for( auto &elem : coming_to_stairs ) {
        elem.staircount = 0;
        add_zombie( elem );
    }

    coming_to_stairs.clear();
}

//TODO: abstract out the location checking code
//TODO: refactor so zombies can follow up and down stairs instead of this mess
void game::update_stair_monsters()
{

    // Search for the stairs closest to the player.
    std::vector<int> stairx, stairy;
    std::vector<int> stairdist;

    const bool from_below = monstairz < levz;

    if (!coming_to_stairs.empty()) {
        for (int x = 0; x < SEEX * MAPSIZE; x++) {
            for (int y = 0; y < SEEY * MAPSIZE; y++) {
                if( ( from_below && m.has_flag( "GOES_DOWN", x, y ) ) ||
                    ( !from_below && m.has_flag( "GOES_UP", x, y ) ) ) {
                    stairx.push_back(x);
                    stairy.push_back(y);
                    stairdist.push_back(rl_dist(x, y, u.posx(), u.posy()));
                }
            }
        }
        if (stairdist.empty()) {
            return;         // Found no stairs?
        }

        // Find closest stairs.
        size_t si = 0;
        for (size_t i = 0; i < stairdist.size(); i++) {
            if (stairdist[i] < stairdist[si]) {
                si = i;
            }
        }

        // Find up to 4 stairs for distance stairdist[si] +1
        int nearest[4] = { 0 };
        int found = 0;
        nearest[found++] = si;
        for (size_t i = 0; i < stairdist.size(); i++) {
            if ((i != si) && (stairdist[i] <= stairdist[si] + 1)) {
                nearest[found++] = i;
                if (found == 4) {
                    break;
                }
            }
        }
        // Randomize the stair choice
        si = nearest[rng( 0, found - 1 )];

        // Attempt to spawn zombies.
        for (size_t i = 0; i < coming_to_stairs.size(); i++) {
            int mposx = stairx[si];
            int mposy = stairy[si];
            monster &critter = coming_to_stairs[i];

            // We might be not be visible.
            if (!(critter.posx() < 0 - (SEEX * MAPSIZE) / 6 ||
                  critter.posy() < 0 - (SEEY * MAPSIZE) / 6 ||
                  critter.posx() > (SEEX * MAPSIZE * 7) / 6 ||
                  critter.posy() > (SEEY * MAPSIZE * 7) / 6)) {

                coming_to_stairs[i].staircount -= 4;
                // Let the player know zombies are trying to come.
                if (u.sees(mposx, mposy)) {
                    std::stringstream dump;
                    if (coming_to_stairs[i].staircount > 4) {
                        dump << string_format(_("You see a %s on the stairs"), critter.name().c_str());
                    } else {
                        //~ The <monster> is almost at the <bottom/top> of the <terrain type>!
                        if (coming_to_stairs[i].staircount > 0) {
                            dump << (from_below ?
                                     string_format(_("The %s is almost at the top of the %s!"),
                                                   critter.name().c_str(),
                                                   m.tername(mposx, mposy).c_str()) :
                                     string_format(_("The %s is almost at the bottom of the %s!"),
                                                   critter.name().c_str(),
                                                   m.tername(mposx, mposy).c_str()));
                        }
                        add_msg(m_warning, dump.str().c_str());
                    }
                } else {
                    sound(mposx, mposy, 5, _("a sound nearby from the stairs!"));
                }

                if (is_empty(mposx, mposy) && coming_to_stairs[i].staircount <= 0) {
                    critter.setpos(mposx, mposy, true);
                    critter.staircount = 0;
                    add_zombie(critter);
                    if (u.sees(mposx, mposy)) {
                        if (!from_below) {
                            add_msg(m_warning, _("The %s comes down the %s!"),
                                    critter.name().c_str(),
                                    m.tername(mposx, mposy).c_str());
                        } else {
                            add_msg(m_warning, _("The %s comes up the %s!"),
                                    critter.name().c_str(),
                                    m.tername(mposx, mposy).c_str());
                        }
                    }
                    coming_to_stairs.erase(coming_to_stairs.begin() + i);
                } else if (u.posx() == mposx && u.posy() == mposy && critter.staircount <= 0) {
                    // Monster attempts to push player of stairs
                    int pushx = -1;
                    int pushy = -1;
                    int tries = 0;

                    // the critter is now right on top of you and will attack unless
                    // it can find a square to push you into with one of his tries.
                    const int creature_push_attempts = 9;
                    const int player_throw_resist_chance = 3;

                    critter.setpos(mposx, mposy, true);
                    while (tries < creature_push_attempts) {
                        tries++;
                        pushx = rng(-1, 1), pushy = rng(-1, 1);
                        int iposx = mposx + pushx;
                        int iposy = mposy + pushy;
                        if ((pushx != 0 || pushy != 0) && (mon_at(iposx, iposy) == -1) &&
                            critter.can_move_to(iposx, iposy)) {
                            bool resiststhrow = (u.is_throw_immune()) ||
                                                (u.has_trait("LEG_TENT_BRACE"));
                            if (resiststhrow && one_in(player_throw_resist_chance)) {
                                u.moves -= 25; // small charge for avoiding the push altogether
                                add_msg(_("The %s fails to push you back!"),
                                        critter.name().c_str());
                                return; //judo or leg brace prevent you from getting pushed at all
                            }
                            // not accounting for tentacles latching on, so..
                            // something is about to happen, lets charge a move
                            u.moves -= 100;
                            if (resiststhrow && (u.is_throw_immune())) {
                                //we have a judoka who isn't getting pushed but counterattacking now.
                                mattack defend;
                                defend.thrown_by_judo(&critter, -1);
                                return;
                            }
                            std::string msg = "";
                            if (!(resiststhrow) && (u.get_dodge() + rng(0, 3) < 12)) {
                                // dodge 12 - never get downed
                                // 11.. avoid 75%; 10.. avoid 50%; 9.. avoid 25%
                                u.add_effect("downed", 2);
                                msg = _("The %s pushed you back hard!");
                            } else {
                                msg = _("The %s pushed you back!");
                            }
                            add_msg(m_warning, msg.c_str(), critter.name().c_str());
                            u.setx( u.posx() + pushx );
                            u.sety( u.posy() + pushy );
                            return;
                        }
                    }
                    add_msg(m_warning,
                            _("The %s tried to push you back but failed! It attacks you!"),
                            critter.name().c_str());
                    critter.melee_attack(u, false);
                    u.moves -= 100;
                    return;
                } else if ((critter.staircount <= 0) && (mon_at(mposx, mposy) != -1)) {
                    // Monster attempts to displace a monster from the stairs
                    monster &other = critter_tracker.find(mon_at(mposx, mposy));
                    critter.setpos(mposx, mposy, true);

                    // the critter is now right on top of another and will push it
                    // if it can find a square to push it into inside of his tries.
                    const int creature_push_attempts = 9;
                    const int creature_throw_resist = 4;

                    int tries = 0;
                    int pushx = 0;
                    int pushy = 0;
                    while (tries < creature_push_attempts) {
                        tries++;
                        pushx = rng(-1, 1);
                        pushy = rng(-1, 1);
                        int iposx = mposx + pushx;
                        int iposy = mposy + pushy;
                        if ((pushx == 0 && pushy == 0) || ((iposx == u.posx()) && (iposy == u.posy()))) {
                            continue;
                        }
                        if ((mon_at(iposx, iposy) == -1) && other.can_move_to(iposx, iposy)) {
                            other.setpos(iposx, iposy, false);
                            other.moves -= 100;
                            std::string msg = "";
                            if (one_in(creature_throw_resist)) {
                                other.add_effect("downed", 2);
                                msg = _("The %s pushed the %s hard.");
                            } else {
                                msg = _("The %s pushed the %s.");
                            };
                            add_msg(msg.c_str(), critter.name().c_str(), other.name().c_str());
                            return;
                        }
                    }
                    return;
                } else {
                    critter.setpos(mposx, mposy, true);
                }
            }
        }
    }

    if (coming_to_stairs.empty()) {
        monstairx = -1;
        monstairy = -1;
        monstairz = 999;
    }
}

void game::despawn_monster(int mondex)
{
    monster &critter = zombie( mondex );
    if( !critter.is_hallucination() ) {
        // hallucinations aren't stored, they come and go as they like,
        overmap_buffer.despawn_monster( critter );
    }
    remove_zombie( mondex );
}

void game::shift_monsters( const int shiftx, const int shifty, const int shiftz )
{
    // If either shift argument is non-zero, we're shifting.
    if( shiftx == 0 && shifty == 0 && shiftz == 0 ) {
        return;
    }
    for( unsigned int i = 0; i < num_zombies(); ) {
        monster &critter = zombie( i );
        if( shiftx != 0 || shifty != 0 ) {
            critter.shift( shiftx, shifty );
        }
        // TODO: with z-levels, this can be removed, instead shift the critter
        // along shiftz, too. Than make the 3D-inbounds check.
        if( shiftz == 0 && m.inbounds( critter.posx(), critter.posy() ) ) {
            i++;
            // We're inbounds, so don't despawn after all.
            continue;
        }
        // Either a vertical shift or the critter is now outside of the reality bubble,
        // anyway: it must be saved and removed.
        despawn_monster( i );
    }
    // The order in which zombies are shifted may cause zombies to briefly exist on
    // the same square. This messes up the mon_at cache, so we need to rebuild it.
    rebuild_mon_at_cache();
}

void game::spawn_mon(int /*shiftx*/, int /*shifty*/)
{
    // Create a new NPC?
    if (ACTIVE_WORLD_OPTIONS["RANDOM_NPC"] && one_in(100 + 15 * cur_om->npcs.size())) {
        npc *tmp = new npc();
        tmp->normalize();
        tmp->randomize();
        //tmp->stock_missions();
        // Create the NPC in one of the outermost submaps,
        // hopefully far away to be invisible to the player,
        // to prevent NPCs appearing out of thin air.
        // This can be changed to let the NPC spawn further away,
        // so it does not became active immediately.
        int msx = get_abs_levx();
        int msy = get_abs_levy();
        switch (rng(0, 4)) { // on which side of the map to spawn
        case 0:
            msy += rng(0, MAPSIZE - 1);
            break;
        case 1:
            msx += MAPSIZE - 1;
            msy += rng(0, MAPSIZE - 1);
            break;
        case 2:
            msx += rng(0, MAPSIZE - 1);
            break;
        case 3:
            msy += MAPSIZE - 1;
            msx += rng(0, MAPSIZE - 1);
            break;
        }
        // adds the npc to the correct overmap.
        tmp->spawn_at( msx, msy, levz );
        tmp->form_opinion(&u);
        tmp->mission = NPC_MISSION_NULL;
        int mission_index = reserve_random_mission(ORIGIN_ANY_NPC, om_location(), tmp->getID());
        if (mission_index != -1) {
            tmp->chatbin.missions.push_back(mission_index);
        }
        // This will make the new NPC active
        load_npcs();
    }
}

void game::wait()
{
    const bool bHasWatch = (u.has_item_with_flag("WATCH") || u.has_bionic("bio_watch"));

    uimenu as_m;
    as_m.text = _("Wait for how long?");
    as_m.entries.push_back(uimenu_entry(1, true, '1',
                                        (bHasWatch) ? _("5 Minutes") : _("Wait 300 heartbeats")));
    as_m.entries.push_back(uimenu_entry(2, true, '2',
                                        (bHasWatch) ? _("30 Minutes") : _("Wait 1800 heartbeats")));

    if (bHasWatch) {
        as_m.entries.push_back(uimenu_entry(3, true, '3', _("1 hour")));
        as_m.entries.push_back(uimenu_entry(4, true, '4', _("2 hours")));
        as_m.entries.push_back(uimenu_entry(5, true, '5', _("3 hours")));
        as_m.entries.push_back(uimenu_entry(6, true, '6', _("6 hours")));
    }

    as_m.entries.push_back(uimenu_entry(7, true, 'd', _("Wait till dawn")));
    as_m.entries.push_back(uimenu_entry(8, true, 'n', _("Wait till noon")));
    as_m.entries.push_back(uimenu_entry(9, true, 'k', _("Wait till dusk")));
    as_m.entries.push_back(uimenu_entry(10, true, 'm', _("Wait till midnight")));
    as_m.entries.push_back(uimenu_entry(11, true, 'w', _("Wait till weather changes")));

    as_m.entries.push_back(uimenu_entry(12, true, 'x', _("Exit")));
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    const int iHour = calendar::turn.hours();

    int time = 0;
    activity_type actType = ACT_WAIT;

    switch (as_m.ret) {
    case 1:
        time = 5000;
        break;
    case 2:
        time = 30000;
        break;
    case 3:
        time = 60000;
        break;
    case 4:
        time = 120000;
        break;
    case 5:
        time = 180000;
        break;
    case 6:
        time = 360000;
        break;
    case 7:
        time = 60000 * ((iHour <= 6) ? 6 - iHour : 24 - iHour + 6);
        break;
    case 8:
        time = 60000 * ((iHour <= 12) ? 12 - iHour : 12 - iHour + 6);
        break;
    case 9:
        time = 60000 * ((iHour <= 18) ? 18 - iHour : 18 - iHour + 6);
        break;
    case 10:
        time = 60000 * ((iHour <= 24) ? 24 - iHour : 24 - iHour + 6);
        break;
    case 11:
        time = 999999999;
        actType = ACT_WAIT_WEATHER;
        break;
    default:
        return;
    }

    u.assign_activity(actType, time, 0);
    u.rooted_message();
}

void game::gameover()
{
    erase();
    gamemode->game_over();
    mvprintw(0, 35, _("GAME OVER"));
    inv(_("Inventory:"));
}

bool game::game_quit()
{
    return (uquit == QUIT_MENU);
}

bool game::game_error()
{
    return (uquit == QUIT_ERROR);
}

void game::teleport(player *p, bool add_teleglow)
{
    if (p == NULL) {
        p = &u;
    }
    int newx, newy, tries = 0;
    bool is_u = (p == &u);

    if (add_teleglow) {
        p->add_effect("teleglow", 300);
    }
    do {
        newx = p->posx() + rng(0, SEEX * 2) - SEEX;
        newy = p->posy() + rng(0, SEEY * 2) - SEEY;
        tries++;
    } while (tries < 15 && m.move_cost(newx, newy) == 0);
    bool can_see = (is_u || u.sees(newx, newy));
    if (p->in_vehicle) {
        m.unboard_vehicle(p->posx(), p->posy());
    }
    p->setx( newx );
    p->sety( newy );
    if (m.move_cost(newx, newy) == 0) { //Teleported into a wall
        if (can_see) {
            if (is_u) {
                add_msg(_("You teleport into the middle of a %s!"),
                        m.name(newx, newy).c_str());
                p->add_memorial_log(pgettext("memorial_male", "Teleported into a %s."),
                                    pgettext("memorial_female", "Teleported into a %s."),
                                    m.name(newx, newy).c_str());
            } else {
                add_msg(_("%s teleports into the middle of a %s!"),
                        p->name.c_str(), m.name(newx, newy).c_str());
            }
        }
        p->apply_damage( nullptr, bp_torso, 500 );
    } else if (can_see) {
        const int i = mon_at(newx, newy);
        if (i != -1) {
            monster &critter = zombie(i);
            if (is_u) {
                add_msg(_("You teleport into the middle of a %s!"),
                        critter.name().c_str());
                u.add_memorial_log(pgettext("memorial_male", "Telefragged a %s."),
                                   pgettext("memorial_female", "Telefragged a %s."),
                                   critter.name().c_str());
            } else {
                add_msg(_("%s teleports into the middle of a %s!"),
                        p->name.c_str(), critter.name().c_str());
            }
            critter.die_in_explosion( p );
        }
    }
    if( is_u ) {
        update_map( p );
    }
}

void game::nuke(int x, int y)
{
    // TODO: nukes hit above surface, not critter = 0
    tinymap tmpmap;
    tmpmap.load_abs(x * 2, y * 2, 0, false);
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (!one_in(10)) {
                tmpmap.make_rubble(i, j, f_rubble_rock, true, t_dirt, true);
            }
            if (one_in(3)) {
                tmpmap.add_field(i, j, fd_nuke_gas, 3);
            }
            tmpmap.adjust_radiation(i, j, rng(20, 80));
        }
    }
    tmpmap.save();
    overmap_buffer.ter(x, y, 0) = "crater";
    // Kill any npcs on that omap location.
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_omt(x, y, 0, 0);
    for( auto &npc : npcs ) {
        npc->marked_for_death = true;
    }
}

bool game::spread_fungus(int x, int y)
{
    int growth = 1;
    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (i == x && j == y) {
                continue;
            }
            if (m.has_flag("FUNGUS", i, j)) {
                growth += 1;
            }
        }
    }

    bool converted = false;
    if (!m.has_flag_ter("FUNGUS", x, y)) {
        // Terrain conversion
        if (m.has_flag_ter("DIGGABLE", x, y)) {
            if (x_in_y(growth * 10, 100)) {
                m.ter_set(x, y, t_fungus);
                converted = true;
            }
        } else if (m.has_flag("FLAT", x, y)) {
            if (m.has_flag("INDOORS", x, y)) {
                if (x_in_y(growth * 10, 500)) {
                    m.ter_set(x, y, t_fungus_floor_in);
                    converted = true;
                }
            } else if (m.has_flag("SUPPORTS_ROOF", x, y)) {
                if (x_in_y(growth * 10, 1000)) {
                    m.ter_set(x, y, t_fungus_floor_sup);
                    converted = true;
                }
            } else {
                if (x_in_y(growth * 10, 2500)) {
                    m.ter_set(x, y, t_fungus_floor_out);
                    converted = true;
                }
            }
        } else if (m.has_flag("SHRUB", x, y)) {
            if (x_in_y(growth * 10, 200)) {
                m.ter_set(x, y, t_shrub_fungal);
                converted = true;
            } else if (x_in_y(growth, 1000)) {
                m.ter_set(x, y, t_marloss);
                converted = true;
            }
        } else if (m.has_flag("THIN_OBSTACLE", x, y)) {
            if (x_in_y(growth * 10, 150)) {
                m.ter_set(x, y, t_fungus_mound);
                converted = true;
            }
        } else if (m.has_flag("YOUNG", x, y)) {
            if (x_in_y(growth * 10, 500)) {
                m.ter_set(x, y, t_tree_fungal_young);
                converted = true;
            }
        } else if (m.has_flag("WALL", x, y)) {
            if (x_in_y(growth * 10, 5000)) {
                converted = true;
                if (m.ter_at(x, y).sym == LINE_OXOX) {
                    m.ter_set(x, y, t_fungus_wall_h);
                } else if (m.ter_at(x, y).sym == LINE_XOXO) {
                    m.ter_set(x, y, t_fungus_wall_v);
                } else {
                    m.ter_set(x, y, t_fungus_wall);
                }
            }
        }
        // Furniture conversion
        if (converted) {
            if (m.has_flag("FLOWER", x, y)) {
                m.furn_set(x, y, f_flower_fungal);
            } else if (m.has_flag("ORGANIC", x, y)) {
                if (m.furn_at(x, y).movecost == -10) {
                    m.furn_set(x, y, f_fungal_mass);
                } else {
                    m.furn_set(x, y, f_fungal_clump);
                }
            } else if (m.has_flag("PLANT", x, y)) {
                for (size_t k = 0; k < m.i_at(x, y).size(); k++) {
                    m.i_rem(x, y, k);
                }
                item seeds("fungal_seeds", int(calendar::turn));
                m.add_item_or_charges(x, y, seeds);
            }
        }
        return true;
    } else {
        // Everything is already fungus
        if (growth == 9) {
            return false;
        }
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                // One spread on average
                if (!m.has_flag("FUNGUS", i, j) && one_in(9 - growth)) {
                    //growth chance is 100 in X simplified
                    if (m.has_flag("DIGGABLE", i, j)) {
                        m.ter_set(i, j, t_fungus);
                        converted = true;
                    } else if (m.has_flag("FLAT", i, j)) {
                        if (m.has_flag("INDOORS", i, j)) {
                            if (one_in(5)) {
                                m.ter_set(i, j, t_fungus_floor_in);
                                converted = true;
                            }
                        } else if (m.has_flag("SUPPORTS_ROOF", i, j)) {
                            if (one_in(10)) {
                                m.ter_set(i, j, t_fungus_floor_sup);
                                converted = true;
                            }
                        } else {
                            if (one_in(25)) {
                                m.ter_set(i, j, t_fungus_floor_out);
                                converted = true;
                            }
                        }
                    } else if (m.has_flag("SHRUB", i, j)) {
                        if (one_in(2)) {
                            m.ter_set(i, j, t_shrub_fungal);
                            converted = true;
                        } else if (one_in(25)) {
                            m.ter_set(i, j, t_marloss);
                            converted = true;
                        }
                    } else if (m.has_flag("THIN_OBSTACLE", i, j)) {
                        if (x_in_y(10, 15)) {
                            m.ter_set(i, j, t_fungus_mound);
                            converted = true;
                        }
                    } else if (m.has_flag("YOUNG", i, j)) {
                        if (one_in(5)) {
                            if (m.get_field_strength(point (x, y), fd_fungal_haze) != 0) {
                                if (one_in(3)) { // young trees are Vulnerable
                                    m.ter_set(i, j, t_fungus);
                                    m.add_spawn("mon_fungal_blossom", 1, x, y);
                                    if (u.sees(x, y)) {
                                    add_msg(m_warning, _("The young tree blooms forth into a fungal blossom!"));
                                    }
                                } else if (one_in(2)) {
                                    m.ter_set(i, j, t_marloss_tree);
                                }
                            } else {
                                m.ter_set(i, j, t_tree_fungal_young);
                            }
                            converted = true;
                        }
                    } else if (m.has_flag("TREE", i, j)) {
                        if (one_in(10)) {
                            if (m.get_field_strength(point (x, y), fd_fungal_haze) != 0) {
                                if (one_in(4)) {
                                    m.ter_set(i, j, t_fungus);
                                    m.add_spawn("mon_fungal_blossom", 1, x, y);
                                    if (u.sees(x, y)) {
                                    add_msg(m_warning, _("The tree blooms forth into a fungal blossom!"));
                                    }
                                } else if (one_in(3)) {
                                    m.ter_set(i, j, t_marloss_tree);
                                }
                            } else {
                                m.ter_set(i, j, t_tree_fungal);
                            }
                            converted = true;
                        }
                    } else if (m.has_flag("WALL", i, j)) {
                        if (one_in(50)) {
                            converted = true;
                            if (m.ter_at(i, j).sym == LINE_OXOX) {
                                m.ter_set(i, j, t_fungus_wall_h);
                            } else if (m.ter_at(i, j).sym == LINE_XOXO) {
                                m.ter_set(i, j, t_fungus_wall_v);
                            } else {
                                m.ter_set(i, j, t_fungus_wall);
                            }
                        }
                    }

                    if (converted) {
                        if (m.has_flag("FLOWER", i, j)) {
                            m.furn_set(i, j, f_flower_fungal);
                        } else if (m.has_flag("ORGANIC", i, j)) {
                            if (m.furn_at(i, j).movecost == -10) {
                                m.furn_set(i, j, f_fungal_mass);
                            } else {
                                m.furn_set(i, j, f_fungal_clump);
                            }
                        } else if (m.has_flag("PLANT", i, j)) {
                            for (size_t k = 0; k < m.i_at(i, j).size(); k++) {
                                m.i_rem(i, j, k);
                            }
                            item seeds("fungal_seeds", int(calendar::turn));
                            m.add_item_or_charges(x, y, seeds);
                        }
                    }
                }
            }
        }
        return false;
    }
}

std::vector<faction *> game::factions_at(int x, int y)
{
    std::vector<faction *> ret;
    for( auto &elem : factions ) {
        if( trig_dist( x, y, elem.mapx, elem.mapy ) <= elem.size ) {
            ret.push_back( &( elem ) );
        }
    }
    return ret;
}

nc_color sev(int a)
{
    switch (a) {
    case 0:
        return c_cyan;
    case 1:
        return c_ltcyan;
    case 2:
        return c_ltblue;
    case 3:
        return c_blue;
    case 4:
        return c_ltgreen;
    case 5:
        return c_green;
    case 6:
        return c_yellow;
    case 7:
        return c_pink;
    case 8:
        return c_ltred;
    case 9:
        return c_red;
    case 10:
        return c_magenta;
    case 11:
        return c_brown;
    case 12:
        return c_cyan_red;
    case 13:
        return c_ltcyan_red;
    case 14:
        return c_ltblue_red;
    case 15:
        return c_blue_red;
    case 16:
        return c_ltgreen_red;
    case 17:
        return c_green_red;
    case 18:
        return c_yellow_red;
    case 19:
        return c_pink_red;
    case 20:
        return c_magenta_red;
    case 21:
        return c_brown_red;
    }
    return c_dkgray;
}

void game::display_scent()
{
    int div = query_int(_("Set the Scent Map sensitivity to (0 to cancel)?"));
    if (div < 1) {
        add_msg(_("Never mind."));
        return;
    };
    draw_ter();
    for (int x = u.posx() - getmaxx(w_terrain) / 2; x <= u.posx() + getmaxx(w_terrain) / 2; x++) {
        for (int y = u.posy() - getmaxy(w_terrain) / 2; y <= u.posy() + getmaxy(w_terrain) / 2; y++) {
            int sn = scent(x, y) / (div * 2);
            mvwprintz(w_terrain, getmaxy(w_terrain) / 2 + y - u.posy(), getmaxx(w_terrain) / 2 + x - u.posx(),
                      sev(sn / 10), "%d",
                      sn % 10);
        }
    }
    wrefresh(w_terrain);
    getch();
}

void game::init_autosave()
{
    moves_since_last_save = 0;
    last_save_timestamp = time(NULL);
}

void game::quicksave()
{
    //Don't autosave if the player hasn't done anything since the last autosave/quicksave,
    if (!moves_since_last_save) {
        return;
    }
    add_msg(m_info, _("Saving game, this may take a while"));

    time_t now = time(NULL);    //timestamp for start of saving procedure

    //perform save
    save();
    //Now reset counters for autosaving, so we don't immediately autosave after a quicksave or autosave.
    moves_since_last_save = 0;
    last_save_timestamp = now;
}

void game::autosave()
{
    //Don't autosave if the min-autosave interval has not passed since the last autosave/quicksave.
    if (time(NULL) < last_save_timestamp + (60 * OPTIONS["AUTOSAVE_MINUTES"])) {
        return;
    }
    quicksave();    //Driving checks are handled by quicksave()
}

void intro()
{
    int maxx, maxy;
    getmaxyx(stdscr, maxy, maxx);
    const int minHeight = FULL_SCREEN_HEIGHT;
    const int minWidth = FULL_SCREEN_WIDTH;
    WINDOW *tmp = newwin(minHeight, minWidth, 0, 0);
    while (maxy < minHeight || maxx < minWidth) {
        werase(tmp);
        if (maxy < minHeight && maxx < minWidth) {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("Whoa! Your terminal is tiny! This game requires a minimum terminal size of "
                                                       "%dx%d to work properly. %dx%d just won't do. Maybe a smaller font would help?"),
                           minWidth, minHeight, maxx, maxy);
        } else if (maxx < minWidth) {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("Oh! Hey, look at that. Your terminal is just a little too narrow. This game "
                                                       "requires a minimum terminal size of %dx%d to function. It just won't work "
                                                       "with only %dx%d. Can you stretch it out sideways a bit?"),
                           minWidth, minHeight, maxx, maxy);
        } else {
            fold_and_print(tmp, 0, 0, maxx, c_white, _("Woah, woah, we're just a little short on space here. The game requires a "
                                                       "minimum terminal size of %dx%d to run. %dx%d isn't quite enough! Can you "
                                                       "make the terminal just a smidgen taller?"),
                           minWidth, minHeight, maxx, maxy);
        }
        wgetch(tmp);
        getmaxyx(stdscr, maxy, maxx);
    }
    werase(tmp);
    wrefresh(tmp);
    delwin(tmp);
    erase();
}

bool is_worn(const player &p, const item *it)
{
    return !p.worn.empty() && &p.worn.front() <= it && it <= &p.worn.back();
}

void game::process_artifact(item *it, player *p)
{
    const bool worn = is_worn( *p, it );
    const bool wielded = ( it == &p->weapon );
    std::vector<art_effect_passive> effects;
    if( worn && it->is_armor() ) {
        it_artifact_armor *armor = dynamic_cast<it_artifact_armor *>(it->type);
        effects = armor->effects_worn;
    } else if (it->is_tool()) {
        it_artifact_tool *tool = dynamic_cast<it_artifact_tool *>(it->type);
        effects = tool->effects_carried;
        if (wielded) {
            effects.insert( effects.end(), tool->effects_wielded.begin(), tool->effects_wielded.end() );
        }
        // Recharge it if necessary
        if (it->charges < tool->max_charges) {
            switch (tool->charge_type) {
            case ARTC_NULL:
            case NUM_ARTCS:
                break; // dummy entries
            case ARTC_TIME:
                // Once per hour
                if (calendar::turn.seconds() == 0 && calendar::turn.minutes() == 0) {
                    it->charges++;
                }
                break;
            case ARTC_SOLAR:
                if (calendar::turn.seconds() == 0 && calendar::turn.minutes() % 10 == 0 &&
                    is_in_sunlight(p->posx(), p->posy())) {
                    it->charges++;
                }
                break;
            // Artifacts can inflict pain even on Deadened folks.
            // Some weird Lovecraftian thing.  ;P
            // (So DON'T route them through mod_pain!)
            case ARTC_PAIN:
                if (calendar::turn.seconds() == 0) {
                    add_msg(m_bad, _("You suddenly feel sharp pain for no reason."));
                    p->pain += 3 * rng(1, 3);
                    it->charges++;
                }
                break;
            case ARTC_HP:
                if (calendar::turn.seconds() == 0) {
                    add_msg(m_bad, _("You feel your body decaying."));
                    p->hurtall(1);
                    it->charges++;
                }
                break;
            }
        }
    }

    for (auto &i : effects) {
        switch (i) {
        case AEP_STR_UP:
            p->mod_str_bonus(+4);
            break;
        case AEP_DEX_UP:
            p->mod_dex_bonus(+4);
            break;
        case AEP_PER_UP:
            p->mod_per_bonus(+4);
            break;
        case AEP_INT_UP:
            p->mod_int_bonus(+4);
            break;
        case AEP_ALL_UP:
            p->mod_str_bonus(+2);
            p->mod_dex_bonus(+2);
            p->mod_per_bonus(+2);
            p->mod_int_bonus(+2);
            break;
        case AEP_SPEED_UP: // Handled in player::current_speed()
            break;

        case AEP_IODINE:
            if (p->radiation > 0) {
                p->radiation--;
            }
            break;

        case AEP_SMOKE:
            if (one_in(10)) {
                int x = p->posx() + rng(-1, 1), y = p->posy() + rng(-1, 1);
                if (m.add_field(x, y, fd_smoke, rng(1, 3))) {
                    add_msg(_("The %s emits some smoke."),
                            it->tname().c_str());
                }
            }
            break;

        case AEP_SNAKES:
            break; // Handled in player::hit()

        case AEP_EXTINGUISH:
            for (int x = p->posx() - 1; x <= p->posx() + 1; x++) {
                for (int y = p->posy() - 1; y <= p->posy() + 1; y++) {
                    m.adjust_field_age(point(x, y), fd_fire, -1);
                }
            }

        case AEP_HUNGER:
            if (one_in(100)) {
                p->hunger++;
            }
            break;

        case AEP_THIRST:
            if (one_in(120)) {
                p->thirst++;
            }
            break;

        case AEP_EVIL:
            if (one_in(150)) { // Once every 15 minutes, on average
                p->add_effect("evil", 300);
                if( it->is_armor() ) {
                    if( !worn ) {
                    add_msg(_("You have an urge to wear the %s."),
                            it->tname().c_str());
                    }
                } else if (!wielded) {
                    add_msg(_("You have an urge to wield the %s."),
                            it->tname().c_str());
                }
            }
            break;

        case AEP_SCHIZO:
            break; // Handled in player::suffer()

        case AEP_RADIOACTIVE:
            if (one_in(4)) {
                p->radiation++;
            }
            break;

        case AEP_STR_DOWN:
            p->mod_str_bonus(-3);
            break;

        case AEP_DEX_DOWN:
            p->mod_dex_bonus(-3);
            break;

        case AEP_PER_DOWN:
            p->mod_per_bonus(-3);
            break;

        case AEP_INT_DOWN:
            p->mod_int_bonus(-3);
            break;

        case AEP_ALL_DOWN:
            p->mod_str_bonus(-2);
            p->mod_dex_bonus(-2);
            p->mod_per_bonus(-2);
            p->mod_int_bonus(-2);
            break;

        case AEP_SPEED_DOWN:
            break; // Handled in player::current_speed()

        default:
            //Suppress warnings
            break;
        }
    }
    // Recalculate, as it might have changed (by mod_*_bonus above)
    p->str_cur = p->get_str();
    p->int_cur = p->get_int();
    p->dex_cur = p->get_dex();
    p->per_cur = p->get_per();
}
void game::start_calendar()
{
    calendar::start = HOURS(ACTIVE_WORLD_OPTIONS["INITIAL_TIME"]);
    if( scen->has_flag("SPR_START") || scen->has_flag("SUM_START") ||
        scen->has_flag("AUT_START") || scen->has_flag("WIN_START") ) {
        if( scen->has_flag("SPR_START") ) {
            ; // Do nothing;
        } else if( scen->has_flag("SUM_START") ) {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"]);
        } else if( scen->has_flag("AUT_START") ) {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 2);
        } else if( scen->has_flag("WIN_START") ) {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 3);
        } else {
            debugmsg("The Unicorn");
        }
    } else {
        if( ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "spring" ) {
            ; // Do nothing.
        } else if( ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "summer") {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"]);
        } else if( ACTIVE_WORLD_OPTIONS["INITIAL_SEASON"].getValue() == "autumn" ) {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 2);
        } else {
            calendar::start += DAYS((int)ACTIVE_WORLD_OPTIONS["SEASON_LENGTH"] * 3);
        }
    }
    calendar::turn = calendar::start;
}
void game::add_artifact_messages(std::vector<art_effect_passive> effects)
{
    int net_str = 0, net_dex = 0, net_per = 0, net_int = 0, net_speed = 0;

    for (auto &i : effects) {
        switch (i) {
        case AEP_STR_UP:
            net_str += 4;
            break;
        case AEP_DEX_UP:
            net_dex += 4;
            break;
        case AEP_PER_UP:
            net_per += 4;
            break;
        case AEP_INT_UP:
            net_int += 4;
            break;
        case AEP_ALL_UP:
            net_str += 2;
            net_dex += 2;
            net_per += 2;
            net_int += 2;
            break;
        case AEP_STR_DOWN:
            net_str -= 3;
            break;
        case AEP_DEX_DOWN:
            net_dex -= 3;
            break;
        case AEP_PER_DOWN:
            net_per -= 3;
            break;
        case AEP_INT_DOWN:
            net_int -= 3;
            break;
        case AEP_ALL_DOWN:
            net_str -= 2;
            net_dex -= 2;
            net_per -= 2;
            net_int -= 2;
            break;

        case AEP_SPEED_UP:
            net_speed += 20;
            break;
        case AEP_SPEED_DOWN:
            net_speed -= 20;
            break;

        case AEP_IODINE:
            break; // No message

        case AEP_SNAKES:
            add_msg(m_warning, _("Your skin feels slithery."));
            break;

        case AEP_INVISIBLE:
            add_msg(m_good, _("You fade into invisibility!"));
            break;

        case AEP_CLAIRVOYANCE:
            add_msg(m_good, _("You can see through walls!"));
            break;

        case AEP_SUPER_CLAIRVOYANCE:
            add_msg(m_good, _("You can see through everything!"));
            break;

        case AEP_STEALTH:
            add_msg(m_good, _("Your steps stop making noise."));
            break;

        case AEP_GLOW:
            add_msg(_("A glow of light forms around you."));
            break;

        case AEP_PSYSHIELD:
            add_msg(m_good, _("Your mental state feels protected."));
            break;

        case AEP_RESIST_ELECTRICITY:
            add_msg(m_good, _("You feel insulated."));
            break;

        case AEP_CARRY_MORE:
            add_msg(m_good, _("Your back feels strengthened."));
            break;

        case AEP_HUNGER:
            add_msg(m_warning, _("You feel hungry."));
            break;

        case AEP_THIRST:
            add_msg(m_warning, _("You feel thirsty."));
            break;

        case AEP_EVIL:
            add_msg(m_warning, _("You feel an evil presence..."));
            break;

        case AEP_SCHIZO:
            add_msg(m_bad, _("You feel a tickle of insanity."));
            break;

        case AEP_RADIOACTIVE:
            add_msg(m_warning, _("Your skin prickles with radiation."));
            break;

        case AEP_MUTAGENIC:
            add_msg(m_bad, _("You feel your genetic makeup degrading."));
            break;

        case AEP_ATTENTION:
            add_msg(m_warning, _("You feel an otherworldly attention upon you..."));
            break;

        case AEP_FORCE_TELEPORT:
            add_msg(m_bad, _("You feel a force pulling you inwards."));
            break;

        case AEP_MOVEMENT_NOISE:
            add_msg(m_warning, _("You hear a rattling noise coming from inside yourself."));
            break;

        case AEP_BAD_WEATHER:
            add_msg(m_warning, _("You feel storms coming."));
            break;

        case AEP_SICK:
            add_msg(m_bad, _("You feel unwell."));
            break;
        default:
            //Suppress warnings
            break;
        }
    }

    std::string stat_info = "";
    if (net_str != 0) {
        stat_info += string_format(_("Str %s%d! "),
                                   (net_str > 0 ? "+" : ""), net_str);
    }
    if (net_dex != 0) {
        stat_info += string_format(_("Dex %s%d! "),
                                   (net_dex > 0 ? "+" : ""), net_dex);
    }
    if (net_int != 0) {
        stat_info += string_format(_("Int %s%d! "),
                                   (net_int > 0 ? "+" : ""), net_int);
    }
    if (net_per != 0) {
        stat_info += string_format(_("Per %s%d! "),
                                   (net_per > 0 ? "+" : ""), net_per);
    }

    if (stat_info.length() > 0) {
        add_msg(stat_info.c_str());
    }


    if (net_speed != 0) {
        add_msg(m_info, _("Speed %s%d! "), (net_speed > 0 ? "+" : ""), net_speed);
    }
}

int game::get_abs_levx() const
{
    return levx + cur_om->pos().x * OMAPX * 2;
}

int game::get_abs_levy() const
{
    return levy + cur_om->pos().y * OMAPY * 2;
}

int game::get_abs_levz() const
{
    return levz;
}
