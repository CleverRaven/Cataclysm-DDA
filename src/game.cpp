#include "game.h"

#include "coordinate_conversions.h"
#include "rng.h"
#include "input.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "veh_interact.h"
#include "veh_type.h"
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
#include "mondefense.h"
#include "monfaction.h"
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
#include "lua_console.h"
#include "sounds.h"
#include "iuse_actor.h"
#include "mutation.h"
#include "mtype.h"
#include "map.h"
#include "map_iterator.h"
#include "overmap.h"
#include "omdata.h"
#include "crafting.h"
#include "construction.h"
#include "lightmap.h"
#include "npc.h"
#include "npc_class.h"
#include "scenario.h"
#include "mission.h"
#include "compatibility.h"
#include "mongroup.h"
#include "morale_types.h"
#include "worldfactory.h"
#include "material.h"
#include "martialarts.h"
#include "event.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "vehicle.h"
#include "submap.h"
#include "mapgen_functions.h"
#include "clzones.h"
#include "item_location.h"
#include "weather.h"
#include "faction.h"
#include "enums.h"
#include "live_view.h"
#include "recipe_dictionary.h"
#include "cata_utility.h"
#include "pathfinding.h"
#include "projectile.h"
#include "gates.h"
#include "item_factory.h"
#include "scent_map.h"
#include "safemode_ui.h"
#include "game_constants.h"

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <locale>
#include <cassert>
#include <iterator>
#include <ctime>
#include <cstring>
#include <chrono>

#ifdef TILES
#include "cata_tiles.h"
#endif // TILES

#if !(defined _WIN32 || defined WINDOWS || defined TILES)
#include <langinfo.h>
#endif

#if (defined _WIN32 || defined __WIN32__)
#   include "platform_win.h"
#   include <tchar.h>
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const int core_version = 3;

/** Will be set to true when running unit tests */
bool test_mode = false;

const mtype_id mon_fungal_blossom( "mon_fungal_blossom" );
const mtype_id mon_manhack( "mon_manhack" );

const skill_id skill_melee( "melee" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_driving( "driving" );
const skill_id skill_firstaid( "firstaid" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id PLANT( "PLANT" );

const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_amigara( "amigara" );
const efftype_id effect_blind( "blind" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_bouldering( "bouldering" );
const efftype_id effect_contacts( "contacts" );
const efftype_id effect_controlled( "controlled" );
const efftype_id effect_deaf( "deaf" );
const efftype_id effect_docile( "docile" );
const efftype_id effect_downed( "downed" );
const efftype_id effect_drunk( "drunk" );
const efftype_id effect_evil( "evil" );
const efftype_id effect_flu( "flu" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_has_bag( "has_bag" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_laserlocked( "laserlocked" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_pet( "pet" );
const efftype_id effect_relax_gas( "relax_gas" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_tied( "tied" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_winded( "winded" );

void advanced_inv(); // player_activity.cpp
void intro();

//The one and only game instance
game *g;
#ifdef TILES
extern std::unique_ptr<cata_tiles> tilecontext;
#endif // TILES
input_context get_default_mode_input_context();

uistatedata uistate;

bool is_valid_in_w_terrain(int x, int y)
{
    return x >= 0 && x < TERRAIN_WINDOW_WIDTH && y >= 0 && y < TERRAIN_WINDOW_HEIGHT;
}

class user_turn {

private:
    std::chrono::time_point<std::chrono::steady_clock> user_turn_start;
public:
    user_turn() {
        user_turn_start = std::chrono::steady_clock::now();
    }

    bool has_timeout_elapsed() {
        float turn_duration = get_option<float>( "TURN_DURATION" );
        // Magic number 0.005 chosen due to option menu's 2 digit precision and
        // the option menu UI rounding <= 0.005 down to "0.00" in the display.
        // This conditional will catch values (e.g. 0.003) that the options menu
        // would round down to "0.00" in the options menu display. This prevents
        // the user from being surprised by floating point rounding near zero.
        if( turn_duration <= 0.005 ) {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::milliseconds elapsed_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>( now - user_turn_start );
        return elapsed_ms.count() >= 1000.0 * turn_duration;
    }
};

// This is the main game set-up process.
game::game() :
    map_ptr( new map() ),
    u_ptr( new player() ),
    liveview_ptr( new live_view() ),
    liveview( *liveview_ptr ),
    scent_ptr( new scent_map() ),
    new_game(false),
    uquit(QUIT_NO),
    m( *map_ptr ),
    u( *u_ptr ),
    scent( *scent_ptr ),
    critter_tracker( new Creature_tracker() ),
    weather( WEATHER_CLEAR ),
    lightning_active( false ),
    weather_precise( new w_point() ),
    w_terrain(NULL),
    w_overmap(NULL),
    w_omlegend(NULL),
    w_minimap(NULL),
    w_pixel_minimap(NULL),
    w_HP(NULL),
    w_messages(NULL),
    w_location(NULL),
    w_status(NULL),
    w_status2(NULL),
    w_blackspace(NULL),
    dangerous_proximity(5),
    pixel_minimap_option(0),
    safe_mode(SAFE_MODE_ON),
    safe_mode_warning_logged(false),
    mostseen(0),
    gamemode(),
    user_action_counter(0),
    lookHeight(13),
    tileset_zoom(16),
    weather_override( WEATHER_NULL )
{
    world_generator.reset( new worldfactory() );
    // do nothing, everything that was in here is moved to init_data() which is called immediately after g = new game; in main.cpp
    // The reason for this move is so that g is not uninitialized when it gets to installing the parts into vehicles.
}

// Load everything that will not depend on any mods
void game::load_static_data()
{
    // UI stuff, not mod-specific per definition
    inp_mngr.init();            // Load input config JSON
    // Init mappings for loading the json stuff
    DynamicDataLoader::get_instance();
    // Only need to load names once, they do not depend on mods
    init_names();
    narrow_sidebar = get_option<std::string>( "SIDEBAR_STYLE" ) == "narrow";
    right_sidebar = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    fullscreen = false;
    was_fullscreen = false;

    // These functions do not load stuff from json.
    // The content they load/initialize is hardcoded into the program.
    // Therefore they can be loaded here.
    // If this changes (if they load data from json), they have to
    // be moved to game::load_mod or game::load_core_data
    init_mapgen_builtin_functions();
    init_fields();
    init_savedata_translation_tables();
    init_artifacts();
    init_faction_data();

    get_auto_pickup().load_global();
    get_safemode().load_global();

    // --- move/delete everything below
    // TODO: move this to player class
    moveCount = 0;
}

bool game::check_mod_data( const std::vector<std::string> &opts )
{
    auto &mods = world_generator->get_mod_manager()->mod_map;
    auto &tree = world_generator->get_mod_manager()->get_tree();

    // deduplicated list of mods to check
    std::set<std::string> check( opts.begin(), opts.end() );

    // if no specific mods specified check everything
    if( check.empty() ) {
        for( const auto &e : mods ) {
            check.emplace( e.first );
        }
    }

    if( check.empty() ) {
        // if no loadable mods then test core data only
        try {
            load_core_data();
        } catch( const std::exception &err ) {
            std::cerr << "Error loading data from json: " << err.what() << std::endl;
        }
        DynamicDataLoader::get_instance().finalize_loaded_data();
    }

    for( const auto &e : check ) {
        auto iter = mods.find( e );
        if( iter == mods.end() ) {
            std::cerr << "Unknown mod: " << e << std::endl;
            return false;
        }

        MOD_INFORMATION &mod = *iter->second;

        if( !tree.is_available( mod.ident ) ) {
            std::cerr << "Missing dependencies: " << mod.name << "\n"
                      << tree.get_node( mod.ident )->s_errors() << std::endl;
            return false;
        }

        std::cout << "Checking mod " << mod.name << " [" << mod.ident << "]" << std::endl;

        try {
            load_core_data();

            // Load any dependencies
            for( auto &dep : tree.get_dependencies_of_X_as_strings( mod.ident ) ) {
                load_data_from_dir( mods[dep]->path, mods[dep]->ident );
            }

            // Load mod itself
            load_data_from_dir( mod.path, mod.ident );
        } catch( const std::exception &err ) {
            std::cerr << "Error loading data: " << err.what() << std::endl;
        }

        DynamicDataLoader::get_instance().finalize_loaded_data();
    }

    return !g->game_error();
}

void game::load_core_data()
{
    // core data can be loaded only once and must be first
    // anyway.
    DynamicDataLoader::get_instance().unload_data();

    init_lua();
    load_data_from_dir( FILENAMES[ "jsondir" ], "core" );
}

void game::load_data_from_dir( const std::string &path, const std::string &src )
{
    // Process a preload file before the .json files,
    // so that custom IUSE's can be defined before
    // the items that need them are parsed
    lua_loadmod( path, "preload.lua" );

    DynamicDataLoader::get_instance().load_data_from_path( path, src );

    // main.lua will be executed after JSON, allowing to
    // work with items defined by mod's JSON
    lua_loadmod( path, "main.lua" );
}

game::~game()
{
    MAPBUFFER.reset();
    delwin(w_terrain);
    delwin(w_minimap);
    delwin(w_pixel_minimap);
    delwin(w_HP);
    delwin(w_messages_short);
    delwin(w_messages_long);
    delwin(w_location);
    delwin(w_status);
    delwin(w_status2);
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

#ifdef TILES
        //class variable to track the option being active
        //only set once, toggle action is used to change during game
        pixel_minimap_option = get_option<bool>( "PIXEL_MINIMAP" );
#endif // TILES
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
     * It was used to move everything into the center of the screen,
     * when the screen was larger than what the game required. They are currently
     * set to zero to prevent the other game windows from being truncated if
     * w_terrain is too small for the current window.
     *
     * The code here calculates size available for w_terrain, caps it at
     * max_view_size (the maximal view range than any character can have at
     * any time).
     * It is stored in TERRAIN_WINDOW_*.
     * If w_terrain does not occupy the whole available area, VIEW_OFFSET_*
     * are set to move everything into the middle of the screen.
     */
    to_map_font_dimension(TERRAIN_WINDOW_WIDTH, TERRAIN_WINDOW_HEIGHT);

    VIEW_OFFSET_X = 0;
    VIEW_OFFSET_Y = 0;

    // VIEW_OFFSET_* are in standard font dimension.
    from_map_font_dimension(VIEW_OFFSET_X, VIEW_OFFSET_Y);

    // Position of the player in the terrain window, it is always in the center
    POSX = TERRAIN_WINDOW_WIDTH / 2;
    POSY = TERRAIN_WINDOW_HEIGHT / 2;

    // Set up the main UI windows.
    w_terrain = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH,
                       VIEW_OFFSET_Y, right_sidebar ? VIEW_OFFSET_X :
                       VIEW_OFFSET_X + sidebarWidth);
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
    int messX, messY, messW, messHshort, messHlong;
    int locX, locY, locW, locH;
    int statX, statY, statW, statH;
    int stat2X, stat2Y, stat2W, stat2H;
    int pixelminimapW, pixelminimapH, pixelminimapX, pixelminimapY;

    bool pixel_minimap_custom_height = false;

#ifdef TILES
    pixel_minimap_custom_height = get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) > 0;
#endif // TILES

    if (use_narrow_sidebar()) {
        // First, figure out how large each element will be.
        hpH = 7;
        hpW = 14;
        statH = 7;
        statW = sidebarWidth - MINIMAP_WIDTH - hpW;
        locH = 2;
        locW = sidebarWidth;
        stat2H = 2;
        stat2W = sidebarWidth;
        pixelminimapW = sidebarWidth;
        pixelminimapH = (pixelminimapW / 2);
        if (pixel_minimap_custom_height && pixelminimapH > get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) ) {
            pixelminimapH = get_option<int>( "PIXEL_MINIMAP_HEIGHT" );
        }
        messHshort = TERRAIN_WINDOW_TERM_HEIGHT - (statH + locH + stat2H + pixelminimapH);
        messW = sidebarWidth;
        if (messHshort < 9) {
            pixelminimapH -= 9 - messHshort;
            messHshort = 9;
        }
        messHlong = TERRAIN_WINDOW_TERM_HEIGHT - (statH + locH + stat2H);

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
        pixelminimapX = 0;
        pixelminimapY = messY + messHshort;
    } else {
        // standard sidebar style
        locH = 2;
        statX = 0;
        statH = 4;
        minimapX = 0;
        minimapY = 0;
        messX = MINIMAP_WIDTH;
        messY = 0;
        messW = sidebarWidth - messX;
        pixelminimapW = messW;
        pixelminimapH = (pixelminimapW / 2);
        if (pixel_minimap_custom_height && pixelminimapH > get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) ) {
            pixelminimapH = get_option<int>( "PIXEL_MINIMAP_HEIGHT" );
        }
        messHshort = TERRAIN_WINDOW_TERM_HEIGHT - (locH + statH + pixelminimapH); // 1 for w_location + 4 for w_stat, w_messages starts at 0
        if (messHshort < 9) {
            pixelminimapH -= 9 - messHshort;
            messHshort = 9;
        }
        messHlong = TERRAIN_WINDOW_TERM_HEIGHT - (locH + statH);
        pixelminimapX = MINIMAP_WIDTH;
        pixelminimapY = messHshort;
        hpX = 0;
        hpY = MINIMAP_HEIGHT;
        // under the minimap, but down to the same line as w_location (which is under w_messages)
        // so it erases the space between w_terrain and (w_messages and w_location)
        hpH = messHshort + pixelminimapH - MINIMAP_HEIGHT + 3;
        hpW = 7;
        locX = MINIMAP_WIDTH;
        locY = messY + messHshort + pixelminimapH;
        locW = sidebarWidth - locX;
        statY = locY + locH;
        statW = sidebarWidth;

        // The default style only uses one status window.
        stat2X = 0;
        stat2Y = statY + statH;
        stat2H = 1;
        stat2W = sidebarWidth;
    }

    int _y = VIEW_OFFSET_Y;
    int _x = right_sidebar ? TERMX - VIEW_OFFSET_X - sidebarWidth : VIEW_OFFSET_X;

    w_minimap = newwin(MINIMAP_HEIGHT, MINIMAP_WIDTH, _y + minimapY, _x + minimapX);
    werase(w_minimap);

    w_HP = newwin(hpH, hpW, _y + hpY, _x + hpX);
    werase(w_HP);

    w_messages_short = newwin(messHshort, messW, _y + messY, _x + messX);
    werase(w_messages_short);

    w_messages_long = newwin(messHlong, messW, _y + messY, _x + messX);
    werase(w_messages_long);

    w_pixel_minimap = newwin(pixelminimapH, pixelminimapW, _y + pixelminimapY, _x + pixelminimapX);
    werase(w_pixel_minimap);

    w_messages = w_messages_short;
    if (!pixel_minimap_option) {
        w_messages = w_messages_long;
    }

    w_location = newwin(locH, locW, _y + locY, _x + locX);
    werase(w_location);

    w_status = newwin(statH, statW, _y + statY, _x + statX);
    werase(w_status);

    w_status2 = newwin(stat2H, stat2W, _y + stat2Y, _x + stat2X);
    werase(w_status2);

    liveview.init();
}

void game::toggle_sidebar_style(void)
{
    narrow_sidebar = !narrow_sidebar;
#ifdef TILES
    tilecontext->reinit_minimap();
#endif // TILES
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

void game::toggle_pixel_minimap(void)
{
#ifdef TILES
    if (pixel_minimap_option) {
        clear_window_area(w_pixel_minimap);
    }
    pixel_minimap_option = !pixel_minimap_option;
    init_ui();
    refresh_all();
#endif // TILES
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
    popup_status( _( "Please wait while the world data loads..." ), _( "Loading core data" ) );
    load_core_data();

    for( int i = get_world_option<int>( "CORE_VERSION" ); i < core_version; ++i ) {
        popup_status( _( "Please wait while the world data loads..." ),
                      _( "Applying legacy migration (%i/%i)" ), i, core_version - 1 );

        load_data_from_dir( FILENAMES["legacydir"] + to_string( i ), "legacy" );
    }

    load_world_modfiles(world_generator->active_world);

    m =  map( get_world_option<bool>( "ZLEVELS" ) );

    next_npc_id = 1;
    next_faction_id = 1;
    next_mission_id = 1;
    last_target = -1;  // We haven't targeted any monsters yet
    last_target_was_npc = false;
    new_game = true;
    uquit = QUIT_NO;   // We haven't quit the game
    bVMonsterLookFire = true;

    weather = WEATHER_CLEAR; // Start with some nice weather...
    // Weather shift in 30
    nextweather = HOURS( get_option<int>( "INITIAL_TIME" ) ) + MINUTES(30);

    turnssincelastmon = 0; //Auto safe mode init
    autosafemode = get_option<bool>( "AUTOSAFEMODE" );
    safemodeveh =
        get_option<bool>( "SAFEMODEVEH" ); //Vehicle safemode check, in practice didn't trigger when needed

    sounds::reset_sounds();
    clear_zombies();
    coming_to_stairs.clear();
    active_npc.clear();
    mission_npc.clear();
    factions.clear();
    mission::clear_all();
    Messages::clear_messages();
    events.clear();

    SCT.vSCT.clear(); //Delete pending messages

    // reset kill counts
    kills.clear();
    scent.reset();

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
    return gamemode ? gamemode->id() : SGAME_NULL;
}

void game::load_map( tripoint pos_sm )
{
    m.load( pos_sm.x, pos_sm.y, pos_sm.z, true );
}

// Set up all default values for a new game
bool game::start_game(std::string worldname)
{
    if( !gamemode ) {
        gamemode.reset( new special_game() );
    }

    seed = rand();
    new_game = true;
    start_calendar();
    nextweather = calendar::turn;
    safe_mode = (get_option<bool>( "SAFEMODE" ) ? SAFE_MODE_ON : SAFE_MODE_OFF);
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

    const start_location &start_loc = u.start_location.obj();
    const tripoint omtstart = start_loc.find_player_initial_location();
    if( omtstart == overmap::invalid_tripoint ) {
        return false;
    }
    start_loc.prepare_map( omtstart);

    if( scen->has_map_special() ) {
        // Specials can add monster spawn points and similar and should be done before the main
        // map is loaded.
        start_loc.add_map_special( omtstart, scen->get_map_special() );
    }
    tripoint lev = omt_to_sm_copy( omtstart );
    // The player is centered in the map, but lev[xyz] refers to the top left point of the map
    lev.x -= MAPSIZE / 2;
    lev.y -= MAPSIZE / 2;
    load_map( lev );

    m.build_map_cache( get_levz() );
    // Do this after the map cache has been built!
    start_loc.place_player( u );
    // ...but then rebuild it, because we want visibility cache to avoid spawning monsters in sight
    m.build_map_cache( get_levz() );
    // Start the overmap with out immediate neighborhood visible, this needs to be after place_player
    overmap_buffer.reveal( point(u.global_omt_location().x, u.global_omt_location().y), get_option<int>( "DISTANCE_INITIAL_VISIBILITY" ), 0);

    u.moves = 0;
    u.process_turn(); // process_turn adds the initial move points
    u.stamina = u.get_stamina_max();
    nextspawn = int(calendar::turn);
    temperature = 65; // Springtime-appropriate?
    update_weather(); // Springtime-appropriate, definitely.
    u.next_climate_control_check = 0;  // Force recheck at startup
    u.last_climate_control_ret = false;

    //Reset character safe mode/pickup rules
    get_auto_pickup().clear_character_rules();
    get_safemode().clear_character_rules();

    //Put some NPCs in there!
    create_starting_npcs();
    //Load NPCs. Set nearby npcs to active.
    load_npcs();
    // Spawn the monsters
    const bool spawn_near =
        get_world_option<bool>( "BLACK_ROAD" ) || g->scen->has_flag("SUR_START");
    m.spawn_monsters( !spawn_near ); // Static monsters

    // Make sure that no monsters are near the player
    // This can happen in lab starts
    if( !spawn_near ) {
        for( size_t i = 0; i < num_zombies(); ) {
            if( rl_dist( zombie( i ).pos(), u.pos() ) <= 5 ||
                m.clear_path( zombie( i ).pos(), u.pos(), 40, 1, 100 ) ) {
                remove_zombie( i );
            } else {
                i++;
            }
        }
    }

    //Create mutation_category_level
    u.set_highest_cat_level();
    //Calc mutation drench protection stats
    u.drench_mut_calc();
    if ( scen->has_flag("FIRE_START") ){
        start_loc.burn( omtstart, 3, 3 );
    }
    if (scen->has_flag("INFECTED")){
        u.add_effect( effect_infected, 1, random_body_part(), true);
    }
    if (scen->has_flag("BAD_DAY")){
        u.add_effect( effect_flu, 10000 );
        u.add_effect( effect_drunk, 2700 );
        u.add_morale( MORALE_FEELING_BAD, -100, -100, MINUTES( 5 ), MINUTES( 5 ) );
    }
    if(scen->has_flag("HELI_CRASH")) {
        start_loc.handle_heli_crash( u );
    }

    // Now that we're done handling coordinates, ensure the player's submap is in the center of the map
    update_map( u );

    //~ %s is player name
    u.add_memorial_log(pgettext("memorial_male", "%s began their journey into the Cataclysm."),
                       pgettext("memorial_female", "%s began their journey into the Cataclysm."),
                       u.name.c_str());
   lua_callback("on_new_player_created");

    return true;
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
    std::vector<npc *> just_added;
    for( auto temp : npcs ) {
        if( temp->is_active() ) {
            continue;
        }

        const tripoint sm_loc = temp->global_sm_location();
        // NPCs who are out of bounds before placement would be pushed into bounds
        // This can cause NPCs to teleport around, so we don't want that
        if( sm_loc.x < get_levx() || sm_loc.x >= get_levx() + MAPSIZE ||
            sm_loc.y < get_levy() || sm_loc.y >= get_levy() + MAPSIZE ) {
            continue;
        }

        add_msg( m_debug, "game::load_npcs: Spawning static NPC, %d:%d:%d (%d:%d:%d)",
                 get_levx(), get_levy(), get_levz(), sm_loc.x, sm_loc.y, sm_loc.z );
        temp->place_on_map();
        if( !m.inbounds( temp->pos() ) ) {
            continue;
        }
        // In the rare case the npc was marked for death while
        // it was on the overmap. Kill it.
        if (temp->marked_for_death) {
            temp->die( nullptr );
        } else {
            if (temp->my_fac != NULL)
                temp->my_fac->known_by_u = true;
            active_npc.push_back( temp );
            just_added.push_back( temp );
        }
    }

    for( auto npc : just_added ) {
        npc->on_load();
    }

    npcs_dirty = false;
}

void game::unload_npcs()
{
    for( auto npc : active_npc ) {
        npc->on_unload();
    }

    active_npc.clear();
}

void game::reload_npcs()
{
    // TODO: Make it not invoke the "on_unload" command for the NPCs that will be loaded anyway
    // and not invoke "on_load" for those NPCs that avoided unloading this way.
    unload_npcs();
    load_npcs();
}

//Pulls the NPCs that were dumped into the world map on save back into mission_npcs
void game::load_mission_npcs()
{
    const int radius = int(MAPSIZE / 2) - 1;
    // uses submap coordinates
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(radius);
    for( auto temp : npcs ) {
        if (temp->companion_mission != ""){
            mission_npc.push_back(temp);
            overmap_buffer.hide_npc( temp->getID() );
        }
    }

    reload_npcs();
}

void game::create_starting_npcs()
{
    if( !get_world_option<bool>( "STATIC_NPC" ) ) {
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
    tmp->randomize( one_in(2) ? NC_DOCTOR : NC_NONE );
    // spawn the npc in the overmap, sets its overmap and submap coordinates
    tmp->spawn_at( get_levx(), get_levy(), get_levz() );
    tmp->setx( SEEX * int(MAPSIZE / 2) + SEEX );
    tmp->sety( SEEY * int(MAPSIZE / 2) + 6 );
    tmp->form_opinion( u );
    tmp->attitude = NPCATT_NULL;
    //This sets the npc mission. This NPC remains in the shelter.
    tmp->mission = NPC_MISSION_SHELTER;
    tmp->chatbin.first_topic = "TALK_SHELTER";
    //one random shelter mission.
    tmp->add_new_mission( mission::reserve_random( ORIGIN_OPENER_NPC, tmp->global_omt_location(), tmp->getID() ) );
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
            vRip.push_back("|                                   |");
            vRip.push_back("|     %                         .   |");
            vRip.push_back("|  @`                            %% |");
            vRip.push_back("| %@%@%\\                *      %`%@%|");
            vRip.push_back("%%@@@.%@%\\%%           `\\ %%.%%@@%@");
            vRip.push_back("@%@@%%%%%@@@@@@%%%%%%%%@@%%@@@%%%@%%@");
        }

        const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
        const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

        WINDOW *w_rip = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
        draw_border(w_rip);

        sfx::do_player_death_hurt( g->u, 1 );
        sfx::fade_audio_group(1, 2000);
        sfx::fade_audio_group(2, 2000);
        sfx::fade_audio_group(3, 2000);
        sfx::fade_audio_group(4, 2000);

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

        for( const auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
            if( kill_count( type.id ) > 0 ) {
                iTotalKills += kill_count( type.id );
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
            u.add_memorial_log(pgettext("memorial_male", "Last words: %s"),
                               pgettext("memorial_female", "Last words: %s"),
                               sLastWords.c_str() );
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
            if (get_world_option<std::string>( "DELETE_WORLD" ) == "yes" ||
                (get_world_option<std::string>( "DELETE_WORLD" ) == "query" &&
                 query_yn(_("Delete saved world?")))) {
                delete_world(world_generator->active_world->world_name, true);
            }
        } else if (get_world_option<std::string>( "DELETE_WORLD" ) != "no") {
            std::stringstream message;
            std::string tmpmessage;
            for( auto &character : characters ) {
                tmpmessage += "\n  ";
                tmpmessage += character;
            }
            message << string_format(_("World retained. Characters remaining:%s"),tmpmessage.c_str());
            popup(message.str(), PF_NONE);
        }
        if( gamemode ) {
            gamemode.reset( new special_game() ); // null gamemode or something..
        }
    }

    //clear all sound channels
    sfx::fade_audio_channel( -1, 300 );
    sfx::fade_audio_group(1, 300);
    sfx::fade_audio_group(2, 300);
    sfx::fade_audio_group(3, 300);
    sfx::fade_audio_group(4, 300);

    MAPBUFFER.reset();
    overmap_buffer.clear();
    return true;
}

static int veh_lumi( vehicle &veh )
{
    float veh_luminance = 0.0;
    float iteration = 1.0;
    auto lights = veh.lights( true );

    for( const auto pt : lights ) {
        const auto &vp = pt->info();
        if( vp.has_flag( VPFLAG_CONE_LIGHT ) ) {
            veh_luminance += vp.bonus / iteration;
            iteration = iteration * 1.1;
        }
    }
    // Calculation: see lightmap.cpp
    return LIGHT_RANGE((veh_luminance * 3));
}

void game::calc_driving_offset(vehicle *veh)
{
    if (veh == nullptr || !get_option<bool>( "DRIVING_VIEW_OFFSET" ) ) {
        set_driving_view_offset(point(0, 0));
        return;
    }
    const int g_light_level = (int)light_level( u.posz() );
    const int light_sight_range = u.sight_range(g_light_level);
    int sight = std::max( veh_lumi( *veh ), light_sight_range );

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
        veh->player_in_control(u)) {
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
    if( new_game ) {
        new_game = false;
    } else {
        gamemode->per_turn();
        calendar::turn.increment();
    }

    if( npcs_dirty ) {
        load_npcs();
    }

    process_events();
    mission::process_all();
    if (calendar::turn.hours() == 0 && calendar::turn.minutes() == 0 &&
        calendar::turn.seconds() == 0) { // Midnight!
        overmap_buffer.process_mongroups();
        lua_callback("on_day_passed");
    }

    // Run a LUA callback once per minute
    if (calendar::turn.seconds() == 0) {
        lua_callback("on_minute_passed");
    }

    // Move hordes every 5 min
    if( calendar::once_every(MINUTES(5)) ) {
        overmap_buffer.move_hordes();
        // Hordes that reached the reality bubble need to spawn,
        // make them spawn in invisible areas only.
        m.spawn_monsters( false );
    }

    u.update_body();

    // Auto-save if autosave is enabled
    if (get_option<bool>( "AUTOSAVE" ) &&
        calendar::once_every(get_option<int>( "AUTOSAVE_TURNS" ) ) &&
        !u.is_dead_state()) {
        autosave();
    }

    update_weather();
    reset_light_level();

    // The following happens when we stay still; 10/40 minutes overdue for spawn
    if ((!u.has_trait("INCONSPICUOUS") && calendar::turn > nextspawn + 100) ||
        (u.has_trait("INCONSPICUOUS") && calendar::turn > nextspawn + 400)) {
        spawn_mon(-1 + 2 * rng(0, 1), -1 + 2 * rng(0, 1));
        nextspawn = calendar::turn;
    }

    process_activity();

    // Process sound events into sound markers for display to the player.
    sounds::process_sound_markers( &u );

    if (!u.in_sleep_state()) {
        if (u.moves > 0 || uquit == QUIT_WATCH) {
            while (u.moves > 0 || uquit == QUIT_WATCH) {
                cleanup_dead();
                // Process any new sounds the player caused during their turn.
                sounds::process_sound_markers( &u );
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
            // Reset displayed sound markers now that the turn is over.
            // We only want this to happen if the player had a chance to examine the sounds.
            sounds::reset_markers();
        } else {
            handle_key_blocking_activity();
        }
    }

    if( driving_view_offset.x != 0 || driving_view_offset.y != 0 ) {
        // Still have a view offset, but might not be driving anymore,
        // or the option has been deactivated,
        // might also happen when someone dives from a moving car.
        // or when using the handbrake.
        vehicle *veh = m.veh_at(u.pos());
        if (veh == 0) {
            calc_driving_offset(0); // reset to (0,0)
        } else {
            calc_driving_offset(veh);
        }
    }

    // No-scent debug mutation has to be processed here or else it takes time to start working
    if( !u.has_active_bionic( "bio_scent_mask" ) &&
        !u.has_trait( "DEBUG_NOSCENT" ) ) {
        scent.set( u.pos(), u.scent );
        overmap_buffer.set_scent( u.global_omt_location(),  u.scent );
    }
    scent.update( u.pos(), m );

    // We need floor cache before checking falling 'n stuff
    m.build_floor_caches();

    m.process_falling();
    m.vehmove();

    // Process power and fuel consumption for all vehicles, including off-map ones.
    // m.vehmove used to do this, but now it only give them moves instead.
    for( auto &elem : MAPBUFFER ) {
        tripoint sm_loc = elem.first;
        point sm_topleft = sm_to_ms_copy(sm_loc.x, sm_loc.y);
        point in_reality = m.getlocal(sm_topleft);

        submap *sm = elem.second;

        const bool in_bubble_z = m.has_zlevels() || sm_loc.z == get_levz();
        for( auto &veh : sm->vehicles ) {
            veh->power_parts();
            veh->idle( in_bubble_z && m.inbounds(in_reality.x, in_reality.y) );
        }
    }
    m.process_fields();
    m.process_active_items();
    m.creature_in_field( u );

    // Apply sounds from previous turn to monster and NPC AI.
    sounds::process_sounds();
    // Update vision caches for monsters. If this turns out to be expensive,
    // consider a stripped down cache just for monsters.
    m.build_map_cache( get_levz(), true );
    monmove();
    update_stair_monsters();
    u.process_turn();
    if (u.moves < 0) {
        draw();
    }
    u.process_active_items();

    if (get_levz() >= 0 && !u.is_underwater()) {
        weather_data(weather).effect();
    }

    if( u.has_effect( effect_sleep) && calendar::once_every(MINUTES(30)) ) {
        draw();
        refresh();
    }

    u.update_bodytemp();
    u.update_body_wetness( *weather_precise );
    u.apply_wetness_morale( temperature );
    rustCheck();

    if( calendar::once_every( MINUTES( 1 ) ) ) {
        u.update_morale();
    }

    if( calendar::once_every( SECONDS( 90 ) ) ) {
        u.check_and_recover_morale();
    }

    sfx::remove_hearing_loss();
    sfx::do_danger_music();
    sfx::do_fatigue();

    return false;
}

void game::set_driving_view_offset(const point &p)
{
    // remove the previous driving offset,
    // store the new offset and apply the new offset.
    u.view_offset.x -= driving_view_offset.x;
    u.view_offset.y -= driving_view_offset.y;
    driving_view_offset.x = p.x;
    driving_view_offset.y = p.y;
    u.view_offset.x += driving_view_offset.x;
    u.view_offset.y += driving_view_offset.y;
}

void game::rustCheck()
{
    for (auto const &aSkill : Skill::skills) {
        if (u.rust_rate() <= rng(0, 1000)) {
            continue;
        }

        if (aSkill.is_combat_skill() &&
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
        int oldSkillLevel = u.get_skill_level(aSkill.ident());

        if (u.get_skill_level(aSkill.ident()).rust(charged_bio_mem)) {
            u.charge_power(-25);
        }
        int newSkill = u.get_skill_level(aSkill.ident());
        if (newSkill < oldSkillLevel) {
            add_msg(m_bad, _("Your skill in %s has reduced to %d!"),
                    aSkill.name().c_str(), newSkill);
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
    if( u.activity.type == ACT_NULL ) {
        return;
    }

    if( calendar::once_every(MINUTES(5)) ) {
        draw();
    }

    while( u.moves > 0 && u.activity.type != ACT_NULL ) {
        u.activity.do_turn( &u );
    }
}

void game::catch_a_monster(std::vector<monster*> &catchables, const tripoint &pos, player *p, int catch_duration) // catching function
{
    int index = rng(1, catchables.size()) - 1; //get a random monster from the vector
    //spawn the corpse, rotten by a part of the duration
    m.add_item_or_charges( pos, item::make_corpse( catchables[index]->type->id, calendar::turn + int( rng( 0, catch_duration ) ) ) );
    u.add_msg_if_player(m_good, _("You caught a %s."), catchables[index]->type->nname().c_str());
    //quietly kill the catched
    catchables[index]->no_corpse_quiet = true;
    catchables[index]->die( p );
    catchables.erase (catchables.begin()+index);
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

    bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    int ch = (int)' ';

    std::string stop_message = text + " " + u.activity.get_stop_phrase() + " " +
                               _( "(Y)es, (N)o, (I)gnore further distractions and finish." );

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
    if (query_yn("%s %s", text.c_str(), u.activity.get_stop_phrase().c_str())) {
        u.cancel_activity();
        return true;
    }
    return false;
}

const weather_generator &game::get_cur_weather_gen() const
{
    const auto &om = get_cur_om();
    const auto &settings = om.get_settings();
    return settings.weather;
}

unsigned int game::get_seed() const
{
    return seed;
}

void game::set_npcs_dirty()
{
    npcs_dirty = true;
}

void game::update_weather()
{
    if( weather == WEATHER_NULL || calendar::turn >= nextweather ) {
        const weather_generator &weather_gen = get_cur_weather_gen();
        w_point &w = *weather_precise;
        w = weather_gen.get_weather( u.global_square_location(), calendar::turn, seed );
        weather_type old_weather = weather;
        weather = weather_override == WEATHER_NULL ?
            weather_gen.get_weather_conditions( w )
            : weather_override;
        if( weather == WEATHER_SUNNY && calendar::turn.is_night() ) {
            weather = WEATHER_CLEAR;
        }
        sfx::do_ambient();

        temperature = w.temperature;
        lightning_active = false;
        nextweather = calendar::turn + 50; // Check weather each 50 turns.
        if (weather != old_weather && weather_data(weather).dangerous &&
            get_levz() >= 0 && m.is_outside(u.pos())
            && !u.has_activity(ACT_WAIT_WEATHER)) {
            cancel_activity_query(_("The weather changed to %s!"), weather_data(weather).name.c_str());
        }

        if (weather != old_weather && u.has_activity(ACT_WAIT_WEATHER)) {
            u.assign_activity(ACT_WAIT_WEATHER, 0, 0);
        }

        if( weather_data( weather ).sight_penalty !=
            weather_data( old_weather ).sight_penalty ) {
            for( int i = -OVERMAP_DEPTH; i <= OVERMAP_HEIGHT; i++ ) {
                m.set_transparency_cache_dirty( i );
            }
        }
    }
}

int game::get_temperature()
{
    return temperature + m.temperature( u.pos() );
}

int game::assign_mission_id()
{
    int ret = next_mission_id;
    next_mission_id++;
    return ret;
}

npc *game::find_npc(int id)
{
    return overmap_buffer.find_npc(id);
}

int game::kill_count( const mtype_id& mon )
{
    if (kills.find(mon) != kills.end()) {
        return kills[mon];
    }
    return 0;
}

void game::increase_kill_count( const mtype_id& id )
{
    kills[id]++;
}

void game::handle_key_blocking_activity()
{
    // If player is performing a task and a monster is dangerously close, warn them
    // regardless of previous safe mode warnings
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
* @return getch
*/
int game::inventory_item_menu(int pos, int iStartX, int iWidth, const inventory_item_menu_positon position)
{
    int cMenu = (int)'+';

    item &oThisItem = u.i_at( pos );
    if( u.has_item( oThisItem ) ) {
        std::vector<iteminfo> vThisItem, vDummy;

        const bool bHPR = get_auto_pickup().has_rule(oThisItem.tname( 1, false ));
        const hint_rating rate_drop_item = u.weapon.has_flag("NO_UNWIELD") ? HINT_CANT : HINT_GOOD;

        int max_text_length = 0;
        uimenu action_menu;
        const auto addentry = [&]( const char key, const std::string &text, const hint_rating hint ) {
            // The char is used as retval from the uimenu *and* as hotkey.
            action_menu.addentry( key, true, key, text );
            auto &entry = action_menu.entries.back();
            switch( hint ) {
                case HINT_CANT:
                    entry.text_color = c_ltgray;
                    break;
                case HINT_IFFY:
                    entry.text_color = c_ltred;
                    break;
                case HINT_GOOD:
                    entry.text_color = c_ltgreen;
                    break;
            }
            max_text_length = std::max( max_text_length, utf8_width( text ) );
        };
        addentry( 'a', pgettext("action", "activate"), u.rate_action_use( oThisItem ) );
        addentry( 'R', pgettext("action", "read"), u.rate_action_read( oThisItem ) );
        addentry( 'E', pgettext("action", "eat"), u.rate_action_eat( oThisItem ) );
        addentry( 'W', pgettext("action", "wear"), u.rate_action_wear( oThisItem ) );
        addentry( 'w', pgettext("action", "wield"), HINT_GOOD );
        addentry( 't', pgettext("action", "throw"), HINT_GOOD );
        addentry( 'c', pgettext("action", "change side"), u.rate_action_change_side( oThisItem ) );
        addentry( 'T', pgettext("action", "take off"), u.rate_action_takeoff( oThisItem ) );
        addentry( 'd', pgettext("action", "drop"), rate_drop_item );
        addentry( 'U', pgettext("action", "unload"), u.rate_action_unload( oThisItem ) );
        addentry( 'r', pgettext("action", "reload"), u.rate_action_reload( oThisItem ) );
        addentry( 'p', pgettext("action", "part reload"), u.rate_action_reload( oThisItem ) );
        addentry( 'm', pgettext("action", "mend"), u.rate_action_mend( oThisItem ) );
        addentry( 'D', pgettext("action", "disassemble"), u.rate_action_disassemble( oThisItem ) );
        addentry( '=', pgettext("action", "reassign"), HINT_GOOD );
        if( bHPR ) {
            addentry( '-', _("Autopickup"), HINT_IFFY );
        } else {
            addentry( '+', _("Autopickup"), HINT_GOOD );
        }

        int iScrollPos = 0;
        oThisItem.info(true, vThisItem);

        // +2+2 for border and adjacent spaces, +2 for '<hotkey><space>'
        int popup_width = max_text_length + 2+2 + 2;
        int popup_x = 0;
        switch (position) {
        case RIGHT_TERMINAL_EDGE:
            popup_x = 0;
            break;
        case LEFT_OF_INFO:
            popup_x = iStartX - popup_width;
            break;
        case RIGHT_OF_INFO:
            popup_x = iStartX + iWidth;
            break;
        case LEFT_TERMINAL_EDGE:
            popup_x = TERMX - popup_width;
            break;
        }

        // TODO: Ideally the setup of uimenu would be split into calculate variables (size, width...),
        // and actual window creation. This would allow us to let uimenu calculate the width, we can
        // use that to adjust its location afterwards.
        action_menu.w_y = VIEW_OFFSET_Y;
        action_menu.w_x = popup_x + VIEW_OFFSET_X;
        action_menu.w_width = popup_width;
        // Filtering isn't needed, the number of entries is manageable.
        action_menu.filtering = false;
        // Default menu border color is different, this matches the border of the item info window.
        action_menu.border_color = BORDER_COLOR;

        do {
            draw_item_info(iStartX, iWidth, VIEW_OFFSET_X, TERMY - VIEW_OFFSET_Y * 2, oThisItem.tname(), oThisItem.type_name(), vThisItem, vDummy,
                           iScrollPos, true, false, false);
            const int prev_selected = action_menu.selected;
            action_menu.query( false );
            if( action_menu.ret != UIMENU_INVALID ) {
                cMenu = action_menu.ret; /* Remember: hotkey == retval, see addentry above. */
            } else if( action_menu.keypress == KEY_RIGHT ) {
                // Simulate KEY_RIGHT == '\n' (confirm currently selected entry) for compatibility with old version.
                // TODO: ideally this should be done in the uimenu, maybe via a callback.
                cMenu = action_menu.entries[action_menu.selected].retval;
            } else {
                cMenu = action_menu.keypress;
            }

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
            case 'c':
                change_side(pos);
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
            case 'p':
                reload( pos, true );
                break;
            case 'm':
                mend( pos );
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
            case KEY_PPAGE:
                // Prevent the menu from scrolling with this key. TODO: Ideally the menu
                // could be instructed to ignore these two keys instead of scrolling.
                action_menu.selected = prev_selected;
                action_menu.fselected = prev_selected;
                action_menu.vshift = 0;
                iScrollPos--;
                break;
            case KEY_NPAGE:
                // ditto. See KEY_PPAGE.
                action_menu.selected = prev_selected;
                action_menu.fselected = prev_selected;
                action_menu.vshift = 0;
                iScrollPos++;
                break;
            case '+':
                if (!bHPR) {
                    get_auto_pickup().add_rule(oThisItem.tname( 1, false ));
                    add_msg(m_info, _("'%s' added to character pickup rules."), oThisItem.tname( 1, false ).c_str());
                }
                break;
            case '-':
                if (bHPR) {
                    get_auto_pickup().remove_rule(oThisItem.tname( 1, false ));
                    add_msg(m_info, _("'%s' removed from character pickup rules."), oThisItem.tname( 1, false ).c_str());
                }
                break;
            }
        } while (cMenu == KEY_DOWN || cMenu == KEY_UP || cMenu == KEY_PPAGE || cMenu == KEY_NPAGE);
    }
    return cMenu;
}

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview(input_context &ctxt, std::string &action)
{
    tripoint liveview_pos{ tripoint_min };
    do {
        action = ctxt.handle_input();
        if (action == "MOUSE_MOVE") {
            int mx, my;
            const bool are_valid_coordinates = ctxt.get_coordinates(w_terrain, mx, my);
            // TODO: Z
            int mz = g->get_levz();
            if (are_valid_coordinates && ( mx != liveview_pos.x ||
                                           my != liveview_pos.y ||
                                           mz != liveview_pos.z ) ) {
                liveview_pos = tripoint( mx, my, mz );
                liveview.show( liveview_pos );
                draw_sidebar_messages();
            } else if ( !are_valid_coordinates ) {
                liveview_pos = tripoint_min;
                liveview.hide();
                draw_sidebar_messages();
            }
        }
    } while (action == "MOUSE_MOVE"); // Freeze animation when moving the mouse

    if (action != "TIMEOUT" && ctxt.get_raw_input().get_first_input() != ERR) {
        // Keyboard event, break out of animation loop
        liveview.hide();
        return false;
    }

    // Mouse movement or un-handled key
    return true;
}

#ifdef TILES
void rescale_tileset(int size);
#endif

input_context get_default_mode_input_context()
{
    input_context ctxt("DEFAULTMODE");
    // Because those keys move the character, they don't pan, as their original name says
    ctxt.set_iso(true);
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
    ctxt.register_action("toggle_move");
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
    ctxt.register_action("autoattack");
    ctxt.register_action("ignore_enemy");
    ctxt.register_action("whitelist_enemy");
    ctxt.register_action("save");
    ctxt.register_action("quicksave");
#ifndef RELEASE
    ctxt.register_action("quickload");
#endif
    ctxt.register_action("quit");
    ctxt.register_action("player_data");
    ctxt.register_action("map");
    ctxt.register_action("missions");
    ctxt.register_action("factions");
    ctxt.register_action("kills");
    ctxt.register_action("morale");
    ctxt.register_action("messages");
    ctxt.register_action("help");
    ctxt.register_action("open_keybindings");
    ctxt.register_action("open_options");
    ctxt.register_action("open_autopickup");
    ctxt.register_action("open_safemode");
    ctxt.register_action("open_color");
    ctxt.register_action("debug");
    ctxt.register_action("debug_scent");
    ctxt.register_action("debug_mode");
    ctxt.register_action("zoom_out");
    ctxt.register_action("zoom_in");
    ctxt.register_action("toggle_sidebar_style");
    ctxt.register_action("toggle_fullscreen");
    ctxt.register_action("toggle_pixel_minimap");
    ctxt.register_action("action_menu");
    ctxt.register_action("main_menu");
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

    m.update_visibility_cache( u.posz() );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();
    const level_cache &map_cache = m.get_cache_ref( u.posz() );
    const auto &visibility_cache = map_cache.visibility_cache;

    user_turn current_turn;

    if (get_option<bool>( "ANIMATIONS" ) ) {
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
        auto const weather_info = get_weather_animation(weather);
        int offset_x = (u.posx() + u.view_offset.x) - getmaxx(w_terrain) / 2;
        int offset_y = (u.posy() + u.view_offset.y) - getmaxy(w_terrain) / 2;

#ifdef TILES
        if( tile_iso && use_tiles ) {
            iStartX = 0;
            iStartY = 0;
            iEndX = MAPSIZE * SEEX;
            iEndY = MAPSIZE * SEEY;
            offset_x = 0;
            offset_y = 0;
        }
#endif //TILES

        const bool bWeatherEffect = ( weather_info.glyph != '?' );
        const int dropCount = int( iEndX * iEndY * weather_info.factor );

        weather_printable wPrint;
        wPrint.colGlyph = weather_info.color;
        wPrint.cGlyph = weather_info.glyph;
        wPrint.wtype = weather;
        wPrint.vdrops.clear();
        wPrint.startx = iStartX;
        wPrint.starty = iStartY;
        wPrint.endx = iEndX;
        wPrint.endy = iEndY;

        inp_mngr.set_timeout( 125 );
        // Force at least one animation frame if the player is dead.
        while( handle_mouseview(ctxt, action) || uquit == QUIT_WATCH ) {
            if( action == "TIMEOUT" && current_turn.has_timeout_elapsed() ) {
                break;
            }

            if( bWeatherEffect && get_option<bool>( "ANIMATION_RAIN" ) ) {
                /*
                Location to add rain drop animation bits! Since it refreshes w_terrain it can be added to the animation section easily
                Get tile information from above's weather information:
                WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
                WEATHER_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
                WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
                */

#ifdef TILES
                if( !use_tiles ) {
#endif //TILES
                    //If not using tiles, erase previous drops from w_terrain
                    for( auto &elem : wPrint.vdrops ) {
                        const tripoint location( elem.first + offset_x, elem.second + offset_y, get_levz() );
                        const lit_level lighting = visibility_cache[location.x][location.y];
                        wmove( w_terrain, location.y - offset_y, location.x - offset_x );
                        if( !m.apply_vision_effects( w_terrain, lighting, cache ) ) {
                            m.drawsq( w_terrain, u, location, false, true,
                                      u.pos() + u.view_offset,
                                      lighting == LL_LOW, lighting == LL_BRIGHT );
                        }
                    }
#ifdef TILES
                }
#endif //TILES
                wPrint.vdrops.clear();

                for( int i = 0; i < dropCount; i++ ) {
                    const int iRandX = rng( iStartX, iEndX - 1 );
                    const int iRandY = rng( iStartY, iEndY - 1 );
                    const int mapx = iRandX + offset_x;
                    const int mapy = iRandY + offset_y;

                    const tripoint mapp( mapx, mapy, u.posz() );

                    const lit_level lighting = visibility_cache[mapp.x][mapp.y];

                    if( m.is_outside( mapp ) && m.get_visibility( lighting, cache ) == VIS_CLEAR &&
                        !critter_at( mapp, true ) ) {
                        // Suppress if a critter is there
                        wPrint.vdrops.push_back(std::make_pair(iRandX, iRandY));
                    }
                }
            }
            // don't bother calculating SCT if we won't show it
            if (uquit != QUIT_WATCH && get_option<bool>( "ANIMATION_SCT" ) ) {
#ifdef TILES
                if (!use_tiles) {
#endif
                    for( auto &elem : SCT.vSCT ) {
                        //Erase previous text from w_terrain
                        if( elem.getStep() > 0 ) {
                            for( size_t i = 0; i < elem.getText().length(); ++i ) {
                                const tripoint location( elem.getPosX() + i, elem.getPosY(), get_levz() );
                                const lit_level lighting = visibility_cache[location.x][location.y];
                                wmove( w_terrain, location.y - offset_y, location.x - offset_x );
                                if( !m.apply_vision_effects( w_terrain, lighting, cache ) ) {
                                    m.drawsq( w_terrain, u, location, false, true,
                                              u.pos() + u.view_offset,
                                              lighting == LL_LOW, lighting == LL_BRIGHT );
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
                        tripoint tmp( iter->getPosX() + i, iter->getPosY(), get_levz() );
                        const Creature *critter = critter_at( tmp, true );

                        if( critter != nullptr && u.sees( *critter ) ) {
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
                popup(message, PF_NO_WAIT_ON_TOP);
                break;
            }
            wrefresh(w_terrain);

            //updating the pixel minimap here allows red flashing indicators for enemies to actually flicker
            draw_pixel_minimap();
        }
        inp_mngr.set_timeout(-1);
    } else {
        inp_mngr.set_timeout( 125 );
        while( handle_mouseview( ctxt, action ) ) {
            if( action == "TIMEOUT" && current_turn.has_timeout_elapsed() ) {
                break;
            }
        }
        inp_mngr.set_timeout( -1 );
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
        if( rc_pair->second->typeId() == "radio_car_on" && rc_pair->second->active ) {
            break;
        }
    }
    if( rc_pair == rc_pairs.end() ) {
        u.add_msg_if_player(m_warning, _("No radio car connected."));
        u.remove_value( "remote_controlling" );
        return;
    }
    item *rc_car = rc_pair->second;

    if( tile_iso && use_tiles ) {
        rotate_direction_cw(dx,dy);
    }

    tripoint src( cx, cy, cz );
    tripoint dest( cx + dx, cy + dy, cz );
    if( m.impassable(dest) || !m.can_put_items_ter_furn(dest) ||
        m.has_furn(dest) ) {
        sounds::sound(dest, 7, _("sound of a collision with an obstacle."));
        return;
    } else if( !m.add_item_or_charges(dest, *rc_car ).is_null() ) {
        //~ Sound of moving a remote controlled car
        sounds::sound(src, 6, _("zzz..."));
        u.moves -= 50;
        m.i_rem( src, rc_car );
        car_location_string.clear();
        car_location_string << dest.x << ' ' << dest.y << ' ' << dest.z;
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
    if( remote_veh_string.str().empty() ||
        ( !u.has_active_bionic( "bio_remote" ) && !u.has_active_item( "remotevehcontrol" ) ) ) {
        remoteveh_cache = nullptr;
    } else {
        tripoint vp;
        remote_veh_string >> vp.x >> vp.y >> vp.z;
        vehicle *veh = m.veh_at( vp );
        if( veh && veh->fuel_left( "battery", true ) > 0 ) {
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
    if( veh != nullptr && !u.has_active_bionic( "bio_remote" ) && !u.has_active_item( "remotevehcontrol" ) ) {
        debugmsg( "Tried to set remote vehicle without bio_remote or remotevehcontrol" );
        veh = nullptr;
    }

    if( veh == nullptr ) {
        u.remove_value( "remote_controlling_vehicle" );
        return;
    }

    std::stringstream remote_veh_string;
    const tripoint vehpos = veh->global_pos3();
    remote_veh_string << vehpos.x << ' ' << vehpos.y << ' ' << vehpos.z;
    u.set_value( "remote_controlling_vehicle", remote_veh_string.str() );
}

bool game::handle_action()
{
    std::string action;
    input_context ctxt;
    action_id act = ACTION_NULL;
    // Check if we have an auto-move destination
    if (u.has_destination()) {
        act = u.get_next_auto_move_direction();
        if (act == ACTION_NULL) {
            add_msg(m_info, _("Auto-move canceled"));
            u.clear_destination();
            return false;
        }
    } else {
        // No auto-move, ask player for input
        ctxt = get_player_input(action);
    }

    int veh_part;
    vehicle *veh = m.veh_at( u.pos(), veh_part );
    bool veh_ctrl = !u.is_dead_state() &&
        ( ( veh && veh->player_in_control(u) ) || remoteveh() != nullptr );

    // If performing an action with right mouse button, co-ordinates
    // of location clicked.
    tripoint mouse_target = tripoint_min;

    // quit prompt check (ACTION_QUIT only grabs 'Q')
    if(uquit == QUIT_WATCH && action == "QUIT") {
        uquit = QUIT_DIED;
        return false;
    }

    if( act == ACTION_NULL ) {
        act = look_up_action(action);

        if ( act == ACTION_MAIN_MENU ) {
            act = handle_main_menu();
        }

        if( act == ACTION_ACTIONMENU ) {
            // No auto-move actions have or can be set at this point.
            u.clear_destination();
            destination_preview.clear();
            act = handle_action_menu();
            if (act == ACTION_NULL) {
                return false;
            }
        }

        if ( can_action_change_worldstate( act ) ) {
            user_action_counter += 1;
        }

        if( act == ACTION_SELECT || act == ACTION_SEC_SELECT ) {
            // Mouse button click
            if (veh_ctrl) {
                // No mouse use in vehicle
                return false;
            }

            if (u.is_dead_state()) {
                // do not allow mouse actions while dead
                return false;
            }

            int mx, my;
            if (!ctxt.get_coordinates(w_terrain, mx, my) || !u.sees(mx, my)) {
                // Not clicked in visible terrain
                return false;
            }
            mouse_target = tripoint( mx, my, u.posz() );

            if ( act == ACTION_SELECT ) {
                // Note: The following has the potential side effect of
                // setting auto-move destination state in addition to setting
                // act.
                if ( !try_get_left_click_action( act, mouse_target ) ) {
                    return false;
                }
            } else if ( act == ACTION_SEC_SELECT ) {
                if ( !try_get_right_click_action( act, mouse_target ) ) {
                    return false;
                }
            }
        } else if ( act != ACTION_TIMEOUT ) {
            // act has not been set for an auto-move, so clearing possible
            // auto-move destinations. Since initializing an auto-move with
            // the mouse may span across multiple actions, we do not clear the
            // auto-move destination if the action is only a timeout, as this
            // would require the user to double click quicker quicker than the
            // timeout delay.
            u.clear_destination();
            destination_preview.clear();
        }
    }

    if( act == ACTION_NULL ) {
        add_msg(m_info, _("Unknown command: '%c'"), (int)ctxt.get_raw_input().get_first_input());
        return false;
    }

    // This has no action unless we're in a special game mode.
    gamemode->pre_action(act);

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
    int soffsetr = 0 - soffset;

    int before_action_moves = u.moves;

    // Use to track if auto-move should be canceled due to a failed
    // move or obstacle
    bool continue_auto_move = true;

    // These actions are allowed while deathcam is active.
    if( uquit == QUIT_WATCH || !u.is_dead_state() ) {
        switch(act) {
        case ACTION_CENTER:
            u.view_offset.x = driving_view_offset.x;
            u.view_offset.y = driving_view_offset.y;
            break;

        case ACTION_SHIFT_N:
            u.view_offset.y += soffsetr;
            break;

        case ACTION_SHIFT_NE:
            u.view_offset.x += soffset;
            u.view_offset.y += soffsetr;
            break;

        case ACTION_SHIFT_E:
            u.view_offset.x += soffset;
            break;

        case ACTION_SHIFT_SE:
            u.view_offset.x += soffset;
            u.view_offset.y += soffset;
            break;

        case ACTION_SHIFT_S:
            u.view_offset.y += soffset;
            break;

        case ACTION_SHIFT_SW:
            u.view_offset.x += soffsetr;
            u.view_offset.y += soffset;
            break;

        case ACTION_SHIFT_W:
            u.view_offset.x += soffsetr;
            break;

        case ACTION_SHIFT_NW:
            u.view_offset.x += soffsetr;
            u.view_offset.y += soffsetr;
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
        case ACTION_MAIN_MENU:
            break; // handled above

        case ACTION_TIMEOUT:
            if( check_safe_mode_allowed(false) ) {
                u.pause();
            }
            break;

        case ACTION_PAUSE:
            if( check_safe_mode_allowed() ) {
                u.pause();
            }
            break;

        case ACTION_TOGGLE_MOVE:
            u.toggle_move_mode();
            break;

        case ACTION_MOVE_N:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(0, -1);
            } else if (veh_ctrl) {
                pldrive(0, -1);
            } else {
                continue_auto_move = plmove(0, -1);
            }
            break;

        case ACTION_MOVE_NE:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(1, -1);
            } else if (veh_ctrl) {
                pldrive(1, -1);
            } else {
                continue_auto_move = plmove(1, -1);
            }
            break;

        case ACTION_MOVE_E:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(1, 0);
            } else if (veh_ctrl) {
                pldrive(1, 0);
            } else {
                continue_auto_move = plmove(1, 0);
            }
            break;

        case ACTION_MOVE_SE:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(1, 1);
            } else if (veh_ctrl) {
                pldrive(1, 1);
            } else {
                continue_auto_move = plmove(1, 1);
            }
            break;

        case ACTION_MOVE_S:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(0, 1);
            } else if (veh_ctrl) {
                pldrive(0, 1);
            } else {
                continue_auto_move = plmove(0, 1);
            }
            break;

        case ACTION_MOVE_SW:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(-1, 1);
            } else if (veh_ctrl) {
                pldrive(-1, 1);
            } else {
                continue_auto_move = plmove(-1, 1);
            }
            break;

        case ACTION_MOVE_W:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
                rcdrive(-1, 0);
            } else if (veh_ctrl) {
                pldrive(-1, 0);
            } else {
                continue_auto_move = plmove(-1, 0);
            }
            break;

        case ACTION_MOVE_NW:
            moveCount++;

            if( ( u.get_value( "remote_controlling" ) != "" ) && ( ( u.has_active_item("radiocontrol") ) || ( u.has_active_bionic("bio_remote") ) ) ) {
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
            if( u.has_active_mutation( "SHELL2" ) ) {
                add_msg(m_info, _("You can't close things while you're in your shell."));
            } else if( mouse_target != tripoint_min ) {
                close( mouse_target );
            } else {
                close();
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
            } else if( mouse_target != tripoint_min ) {
                examine( mouse_target );
            } else {
                examine();
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
            Pickup::pick_up( u.pos(), 1 );
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

        case ACTION_LIST_ITEMS:
            list_items_monsters();
            break;

        case ACTION_ZONES:
            zones_manager();
            break;

        case ACTION_INVENTORY:
            interactive_inv();
            refresh_all();
            break;

        case ACTION_COMPARE:
            compare();
            break;

        case ACTION_ORGANIZE:
            reassign_item();
            refresh_all();
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
            unload();
            break;

        case ACTION_MEND:
            mend();
            break;

        case ACTION_THROW:
            plthrow();
            break;

        case ACTION_FIRE:
            // Use vehicle turret or draw a pistol from a holster if unarmed
            if( !u.is_armed() ) {

                int part = -1;
                vehicle *veh = m.veh_at( u.pos(), part );

                turret_data turret;
                if( veh && ( turret = veh->turret_query( u.pos() ) ) ) {
                    switch( turret.query() ) {
                        case turret_data::status::no_ammo:
                            add_msg( m_bad, _( "The %s is out of ammo." ), turret.name().c_str() );
                            break;

                        case turret_data::status::no_power:
                            add_msg( m_bad,  _( "The %s is not powered." ), turret.name().c_str() );
                            break;

                        case turret_data::status::ready: {
                            // if more than one firing mode provide callback to cyle through them
                            target_callback switch_mode;
                            if( turret.base()->gun_all_modes().size() > 1 ) {
                                switch_mode = [&turret]( item *obj ) {
                                    obj->gun_cycle_mode();
                                    // currently gun modes dont change ammo but they may in the future
                                    return turret.ammo_current() == "null" ? nullptr :
                                           item::find_type( turret.ammo_current() );
                                };
                            }

                            // if multiple ammo types available provide callback to cycle alternatives
                            target_callback switch_ammo;
                            if( turret.ammo_options().size() > 1 ) {
                                switch_ammo = [&turret]( item * ) {
                                    const auto opts = turret.ammo_options();
                                    auto iter = opts.find( turret.ammo_current() );
                                    turret.ammo_select( ++iter != opts.end() ? *iter : *opts.begin() );
                                    return item::find_type( turret.ammo_current() );
                                };
                            }

                            auto trajectory = pl_target_ui( TARGET_MODE_TURRET_MANUAL, &*turret.base(),
                                                            turret.range(), turret.ammo_data(),
                                                            switch_mode, switch_ammo );

                            if( !trajectory.empty() ) {
                                turret.fire( u, trajectory.back() );
                            }
                            break;
                        }

                        default:
                            debugmsg( "unknown turret status" );
                            break;
                    }
                    break;
                }

                if( veh ) {
                    int vpcontrols = veh->part_with_feature( part, "CONTROLS", true );
                    if( vpcontrols >= 0 && veh->turrets_aim() ) {
                        break;
                    }
                }

                std::vector<std::string> options( 1, _("Cancel") );
                std::vector<std::function<void()>> actions( 1, []{} );

                for( auto &w : u.worn ) {
                    if( w.type->can_use( "holster" ) && !w.has_flag( "NO_QUICKDRAW" ) &&
                        !w.contents.empty() && w.contents.front().is_gun() ) {
                        // draw (first) gun contained in holster
                        options.push_back( string_format( _("%s from %s (%d)" ),
                                                          w.contents.front().tname().c_str(),
                                                          w.type_name().c_str(),
                                                          w.contents.front().ammo_remaining() ) );

                        actions.push_back( [&]{ u.invoke_item( &w, "holster" ); } );

                    } else if( w.is_gun() && w.gunmod_find( "shoulder_strap" ) ) {
                        // wield item currently worn using shoulder strap
                        options.push_back( w.display_name() );
                        actions.push_back( [&]{ u.wield( w ); } );
                    }
                }
                if( options.size() > 1 ) {
                    actions[ ( uimenu( false, _("Draw what?"), options ) ) - 1 ]();
                }
            }

            if( u.weapon.is_gun() ) {
                plfire();

            } else if( u.weapon.has_flag( "REACH_ATTACK" ) ) {
                int range = u.weapon.has_flag( "REACH3" ) ? 3 : 2;
                temp_exit_fullscreen();
                m.draw( w_terrain, u.pos() );
                std::vector<tripoint> trajectory = pl_target_ui( TARGET_MODE_REACH, &u.weapon,range );
                if( !trajectory.empty() ) {
                    u.reach_attack( trajectory.back() );
                }
                draw_ter();
                reenter_fullscreen();
            }
            break;

        case ACTION_FIRE_BURST: {
            auto mode = u.weapon.gun_get_mode_id();
            if( u.weapon.gun_set_mode( "AUTO" ) ) {
                plfire();
                u.weapon.gun_set_mode( mode );
            }
            break;
        }

        case ACTION_SELECT_FIRE_MODE:
            if( u.is_armed() ) {
                u.weapon.gun_cycle_mode();
            }
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
                as_m.return_invalid = true;
                as_m.text = _("Are you sure you want to sleep?");
                as_m.entries.push_back(uimenu_entry(0, true,
                                                    (get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'Y' : 'y'),
                                                    _("Yes.")));

                if (get_option<bool>( "SAVE_SLEEP" ) ) {
                    as_m.entries.push_back(uimenu_entry(1, (moves_since_last_save),
                                                        (get_option<bool>( "FORCE_CAPITAL_YN" ) ? 'S' : 's'),
                                                        _("Yes, and save game before sleeping.")));
                }
                as_m.entries.push_back(uimenu_entry(2, true, (get_option<bool>( "FORCE_CAPITAL_YN" ) ?
                                                    'N' : 'n'), _("No.")));

                if( u.has_alarm_clock() && u.get_hunger() < -60 && u.has_active_mutation( "HIBERNATE" ) ) {
                    as_m.text =
                        _("You're engorged to hibernate. The alarm would only attract attention. Enter hibernation?");
                }
                // List all active items, bionics or mutations so player can deactivate them
                std::vector<std::string> active;
                for ( auto &it : g->u.inv_dump() ) {
                    if ( it->active && it->charges > 0 && it->is_tool_reversible() ) {
                        active.push_back( it->tname() );
                    }
                }
                for ( int i = 0; i < g->u.num_bionics(); i++ ) {
                    bionic const &bio = g->u.bionic_at_index(i);
                    if (!bio.powered) {
                        continue;
                    }

                    // bio_alarm is usefull for waking up during sleeping
                    // turning off bio_leukocyte has 'unpleasant side effects'
                    if( bio.id == "bio_alarm" || bio.id == "bio_leukocyte" ) {
                        continue;
                    }

                    auto const &info = bio.info();
                    if ( info.power_over_time > 0 ) {
                        active.push_back(info.name);
                    }
                }
                for ( auto &mut : g->u.get_mutations() ) {
                    const auto &mdata = mutation_branch::get( mut );
                    if( mdata.cost > 0 && u.has_active_mutation( mut ) ) {
                        active.push_back( mdata.name );
                    }
                }
                std::stringstream data;
                if ( active.size() > 0 ) {
                    data << as_m.text << std::endl;
                    data << _("You may want to deactivate these before you sleep.") << std::endl;
                    data << " " << std::endl;
                    for ( auto &a : active ) {
                        data << a << std::endl;
                    }
                    as_m.text = data.str();
                }
                if( u.has_alarm_clock() &&
                    !( u.get_hunger() < -60 && u.has_active_mutation( "HIBERNATE" ) ) ) {
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
                    u.add_effect( effect_alarm_clock, 600 * as_m.ret);
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
                set_safe_mode( SAFE_MODE_ON );
                mostseen = 0;
                add_msg(m_info, _("Safe mode ON!"));
            } else {
                turnssincelastmon = 0;
                set_safe_mode( SAFE_MODE_OFF );
                if (autosafemode) {
                    add_msg(m_info, _("Safe mode OFF! (Auto safe mode still enabled!)"));
                } else {
                    add_msg(m_info, _("Safe mode OFF!"));
                }
            }
            if( u.has_effect( effect_laserlocked) ) {
                u.remove_effect( effect_laserlocked);
                safe_mode_warning_logged = false;
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
                    monster &critter = critter_tracker->find( elem );
                    critter.ignoring = rl_dist( u.pos(), critter.pos() );
                }
                set_safe_mode( SAFE_MODE_ON );
            } else if( u.has_effect( effect_laserlocked) ) {
                add_msg(m_info, _("Ignoring laser targeting!"));
                u.remove_effect( effect_laserlocked);
                safe_mode_warning_logged = false;
            }
            break;

        case ACTION_WHITELIST_ENEMY:
            if ( safe_mode == SAFE_MODE_STOP && !get_safemode().empty() ) {
                get_safemode().add_rule( get_safemode().lastmon_whitelist, Creature::A_ANY, 0, RULE_WHITELISTED );
                add_msg( m_info, _( "Creature whitelisted: %s" ), get_safemode().lastmon_whitelist.c_str() );
                set_safe_mode( SAFE_MODE_ON );
                mostseen = 0;
            } else {
                get_safemode().show();
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
            refresh_all();
            break;

        case ACTION_SAVE:
            if (query_yn(_("Save and quit?"))) {
                if(save()) {
                    u.moves = 0;
                    uquit = QUIT_SAVED;
                }
            }
            refresh_all();
            break;

        case ACTION_QUICKSAVE:
            quicksave();
            return false;

        case ACTION_QUICKLOAD:
            quickload();
            return false;

        case ACTION_PL_INFO:
            u.disp_info();
            refresh_all();
            break;

        case ACTION_MAP:
            werase( w_terrain );
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

        case ACTION_KEYBINDINGS:
            ctxt.display_help();
            refresh_all();
            break;

        case ACTION_OPTIONS:
            get_options().show( true );
            refresh_all();
            break;

        case ACTION_AUTOPICKUP:
            get_auto_pickup().show();
            refresh_all();
            break;

        case ACTION_SAFEMODE:
            get_safemode().show();
            refresh_all();
            break;

        case ACTION_COLOR:
            all_colors.show_gui();
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

        case ACTION_TOGGLE_PIXEL_MINIMAP:
            toggle_pixel_minimap();
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

        case ACTION_AUTOATTACK:
            autoattack();
            break;

        default:
            break;
        }
    }
    if( !continue_auto_move ) {
        u.clear_destination();
    }

    gamemode->post_action(act);

    u.movecounter = (!u.is_dead_state() ? (before_action_moves - u.moves) : 0);
    dbg(D_INFO) << string_format("%s: [%d] %d - %d = %d", action_ident(act).c_str(),
                                 int(calendar::turn), before_action_moves, u.movecounter, u.moves);
    return (!u.is_dead_state());
}

bool game::try_get_left_click_action( action_id &act, const tripoint &mouse_target ) {
    bool new_destination = true;
    if (!destination_preview.empty()) {
        auto &final_destination = destination_preview.back();
        if (final_destination.x == mouse_target.x && final_destination.y == mouse_target.y) {
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
        destination_preview = m.route( u.pos(), mouse_target, u.get_pathfinding_settings(), u.get_path_avoid() );
        return false;
    }

    return true;
}

bool game::try_get_right_click_action( action_id &act, const tripoint &mouse_target ) {
    const bool cleared_destination = !destination_preview.empty();
    u.clear_destination();
    destination_preview.clear();

    if (cleared_destination) {
        // Produce no-op if auto-move had just been cleared on this action
        // e.g. from a previous single left mouse click. This has the effect
        // of right-click cancelling an auto-move before it is initiated.
        return false;
    }

    const bool is_adjacent = square_dist( mouse_target.x, mouse_target.y, u.posx(), u.posy() ) <= 1;
    const bool is_self = square_dist( mouse_target.x, mouse_target.y, u.posx(), u.posy() ) <= 0;
    int mouse_selected_mondex = mon_at( mouse_target );
    if (mouse_selected_mondex != -1) {
        monster &critter = critter_tracker->find(mouse_selected_mondex);
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
    } else if (is_adjacent && m.close_door( tripoint( mouse_target.x, mouse_target.y, u.posz() ), !m.is_outside(u.pos()), true)) {
        act = ACTION_CLOSE;
    } else if ( is_self ) {
        act = ACTION_PICKUP;
    } else if ( is_adjacent ) {
        act = ACTION_EXAMINE;
    } else {
        add_msg(_("Nothing relevant here."));
        return false;
    }

    return true;
}

bool game::is_game_over()
{
    if (uquit == QUIT_WATCH) {
        // deny player movement and dodging
        u.moves = 0;
        // prevent pain from updating
        u.set_pain( 0 );
        // prevent dodging
        u.dodges_left = 0;
        return false;
    }
    if (uquit == QUIT_DIED) {
        if (u.in_vehicle) {
            m.unboard_vehicle(u.pos());
        }
        u.place_corpse();
        return true;
    }
    if (uquit == QUIT_SUICIDE) {
        if (u.in_vehicle) {
            m.unboard_vehicle(u.pos());
        }
        return true;
    }
    if (uquit != QUIT_NO) {
        return true;
    }
    // is_dead_state() already checks hp_torso && hp_head, no need to for loop it
    if(u.is_dead_state()) {
        if(get_option<std::string>( "DEATHCAM" ) == "always") {
            uquit = QUIT_WATCH;
        } else if(get_option<std::string>( "DEATHCAM" ) == "ask") {
            uquit = query_yn(_("Watch the last moments of your life...?")) ? QUIT_WATCH : QUIT_DIED;
        } else if(get_option<std::string>( "DEATHCAM" ) == "never") {
            uquit = QUIT_DIED;
        } else {
            // Something funky happened here, just die.
            dbg(D_ERROR) << "no deathcam option given to options, defaulting to QUIT_DIED";
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
    disp_NPC_epilogues();
    disp_faction_ends();
}

void game::move_save_to_graveyard()
{
    const std::string &save_dir      = world_generator->active_world->world_path;
    const std::string &graveyard_dir = FILENAMES["graveyarddir"];
    const std::string &prefix        = base64_encode(u.name) + ".";

    if (!assure_dir_exist(graveyard_dir)) {
        debugmsg("could not create graveyard path '%s'", graveyard_dir.c_str());
    }

    auto const save_files = get_files_from_path(prefix, save_dir);
    if (save_files.empty()) {
        debugmsg("could not find save files in '%s'", save_dir.c_str());
    }

    for (auto const &src_path : save_files) {
        const std::string dst_path = graveyard_dir +
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
    using namespace std::placeholders;
    const auto datafile = world_generator->all_worlds[worldname]->world_path + "/master.gsav";
    return read_from_file_optional( datafile, std::bind( &game::unserialize_master, this, _1 ) );
}

void game::load_uistate(std::string worldname)
{
    using namespace std::placeholders;
    const auto savefile = world_generator->all_worlds[worldname]->world_path + "/uistate.json";
    read_from_file_optional( savefile, uistate );
}

void game::load(std::string worldname, std::string name)
{
    using namespace std::placeholders;

    const std::string worldpath = world_generator->all_worlds[worldname]->world_path + "/";
    const std::string playerfile = worldpath + name + ".sav";

    // Now load up the master game data; factions (and more?)
    load_master(worldname);
    u = player();
    u.name = base64_decode(name);
    // This should be initialized more globally (in player/Character constructor)
    u.ret_null = item( "null", 0 );
    u.weapon = item("null", 0);
    if( !read_from_file( playerfile, std::bind( &game::unserialize, this, _1 ) ) ) {
        return;
    }

    read_from_file_optional( worldpath + name + ".weather", std::bind( &game::load_weather, this, _1 ) );
    nextweather = int(calendar::turn);

    read_from_file_optional( worldpath + name + ".log", std::bind( &player::load_memorial_file, &u, _1 ) );

    // Now that the player's worn items are updated, their sight limits need to be
    // recalculated. (This would be cleaner if u.worn were private.)
    u.recalc_sight_limits();

    if( !gamemode ) {
        gamemode.reset( new special_game() );
    }

    safe_mode = get_option<bool>( "SAFEMODE" ) ? SAFE_MODE_ON : SAFE_MODE_OFF;
    mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.

    init_autosave();
    get_auto_pickup().load_character(); // Load character auto pickup rules
    get_safemode().load_character(); // Load character safemode rules
    zone_manager::get_manager().load_zones(); // Load character world zones
    load_uistate(worldname);

    load_mission_npcs(); // Pull mission_npcs back out of the overmap before update_map
    update_map( u );

    // legacy, needs to be here as we access the map.
    if( u.getID() == 0 || u.getID() == -1 ) {
        // player does not have a real id, so assign a new one,
        u.setID( assign_npc_id() );
        // The vehicle stores the IDs of the boarded players, so update it, too.
        if( u.in_vehicle ) {
            int vpart;
            vehicle *veh = m.veh_at( u.pos(), vpart );
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
    erase();
    refresh();

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
                MOD_INFORMATION &mod = *mm->mod_map[mod_ident];
                if( !mod.obsolete ) {
                    // Silently ignore mods marked as obsolete.
                    popup_status( _( "Please wait while the world data loads..." ),
                                  _( "Loading mods (%s)" ), mod.ident.c_str() );
                    load_data_from_dir( mod.path, mod.ident );
                }
            } else {
                debugmsg("the world uses an unknown mod %s", mod_ident.c_str());
            }
        }
        // Load additional mods from that world-specific folder
        load_data_from_dir( world->world_path + "/mods", "custom" );
    }

    erase();
    refresh();
    popup_status( _( "Please wait while the world data loads..." ), _( "Finalizing and verifying" ) );

    DynamicDataLoader::get_instance().finalize_loaded_data();
}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    //Dump all of the NPCs from mission_npc into the world map to be saved
    for( auto *elem : mission_npc ) {
        elem->spawn_at(get_levx(), get_levy(), get_levz());
        elem->setx(u.posx() + 3);
        elem->sety(u.posy() + 3);
        elem->setz( u.posz() );
    }

    std::string masterfile = world_generator->active_world->world_path + "/master.gsav";
    return write_to_file_exclusive( masterfile, [&]( std::ostream &fout ) {
        serialize_master(fout);
    }, _( "factions data" ) );
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
        overmap_buffer.save(); // can throw
        MAPBUFFER.save(); // can throw
        return true;
    } catch( const std::exception &err ) {
        popup( _( "Failed to save the maps: %s" ), err.what() );
        return false;
    }
}

bool game::save_uistate()
{
    std::string savefile = world_generator->active_world->world_path + "/uistate.json";
    return write_to_file_exclusive( savefile, [&]( std::ostream &fout ) {
        fout << uistate.serialize();
    }, _( "uistate data" ) );
}

bool game::save_player_data()
{
    const std::string playerfile = world_generator->active_world->world_path + "/" + base64_encode(u.name);

    const bool saved_data = write_to_file( playerfile + ".sav", [&]( std::ostream &fout ) {
        serialize(fout);
    }, _( "player data" ) );
    const bool saved_weather = write_to_file( playerfile + ".weather", [&]( std::ostream &fout ) {
        save_weather(fout);
    }, _( "weather state" ) );
    const bool saved_log = write_to_file( playerfile + ".log", [&]( std::ostream &fout ) {
        fout << u.dump_memorial();
    }, _( "player memorial" ) );

    return saved_data && saved_weather && saved_log;
}

bool game::save()
{
    try {
        if ( !save_player_data() ||
             !save_factions_missions_npcs() ||
             !save_artifacts() ||
             !save_maps() ||
             !get_auto_pickup().save_character() ||
             !get_safemode().save_character() ||
             !save_uistate()){
            return false;
        } else {
            world_generator->active_world->add_save( base64_encode( u.name ) );
            return true;
        }
    } catch (std::ios::failure &err) {
        popup(_("Failed to save game data"));
        return false;
    }
}

// Helper predicate to exclude files from deletion when resetting a world directory.
static bool isForbidden(std::string candidate)
{
    if (candidate.find(FILENAMES["worldoptions"]) != std::string::npos ||
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
        (void)remove( file_path.c_str() );
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
    const std::string &memorial_dir = FILENAMES["memorialdir"];
    const std::string &memorial_active_world_dir = memorial_dir +  world_generator->active_world->world_name + "/";

    //Check if both dirs exist. Nested assure_dir_exist fails if the first dir of the nested dir does not exist.
    if (!assure_dir_exist(memorial_dir)) {
        dbg(D_ERROR) << "game:write_memorial_file: Unable to make memorial directory.";
        debugmsg("Could not make '%s' directory", memorial_dir.c_str());
        return;
    }

    if (!assure_dir_exist(memorial_active_world_dir)) {
        dbg(D_ERROR) << "game:write_memorial_file: Unable to make active world directory in memorial directory.";
        debugmsg("Could not make '%s' directory", memorial_active_world_dir.c_str());
        return;
    }

    // <name>-YYYY-MM-DD-HH-MM-SS.txt
    //       123456789012345678901234 ~> 24 chars + a null
    constexpr size_t suffix_len   = 24 + 1;
    constexpr size_t max_name_len = FILENAME_MAX - suffix_len;

    size_t const name_len = u.name.size();
    // Here -1 leaves space for the ~
    size_t const truncated_name_len = (name_len >= max_name_len) ? (max_name_len - 1) : name_len;

    std::ostringstream memorial_file_path;
    memorial_file_path << memorial_active_world_dir;

    // Use the default locale to replace non-printable characters with _ in the player name.
    std::locale locale {"C"};
    std::replace_copy_if(std::begin(u.name), std::begin(u.name) + truncated_name_len,
        std::ostream_iterator<char>(memorial_file_path),
        [&](char const c) { return !std::isgraph(c, locale); }, '_');

    // Add a ~ if the player name was actually truncated.
    memorial_file_path << ((truncated_name_len != name_len) ? "~-" : "-");

    // Add a timestamp for uniqueness.
    char buffer[suffix_len] {};
    std::time_t t = std::time(nullptr);
    std::strftime(buffer, suffix_len, "%Y-%m-%d-%H-%M-%S", std::localtime(&t));
    memorial_file_path << buffer;

    memorial_file_path << ".txt";

    const std::string path_string = memorial_file_path.str();
    write_to_file( memorial_file_path.str(), [&]( std::ostream &fout ) {
        u.memorial( fout, sLastWords );
    }, _( "player memorial" ) );
}

void game::add_event(event_type type, int on_turn, int faction_id)
{
    add_event( type, on_turn, faction_id, u.global_sm_location() );
}

void game::add_event(event_type type, int on_turn, int faction_id, const tripoint center )
{
    event tmp( type, on_turn, faction_id, center );
    events.push_back(tmp);
}

bool game::event_queued(event_type type) const
{
    for( const auto &e : events ) {
        if( e.type == type ) {
            return true;
        }
    }
    return false;
}

void game::debug()
{
    int action = menu( true, // cancelable
                       _( "Debug Functions - Using these is CHEATING!" ),
                       _( "Wish for an item" ),       // 1
                       _( "Teleport - Short Range" ), // 2
                       _( "Teleport - Long Range" ),  // 3
                       _( "Reveal map" ),             // 4
                       _( "Spawn NPC" ),              // 5
                       _( "Spawn Monster" ),          // 6
                       _( "Check game state..." ),    // 7
                       _( "Kill NPCs" ),              // 8
                       _( "Mutate" ),                 // 9
                       _( "Spawn a vehicle" ),        // 10
                       _( "Change all skills" ),      // 11
                       _( "Learn all melee styles" ), // 12
                       _( "Unlock all recipes" ),     // 13
                       _( "Edit player/NPC" ),        // 14
                       _( "Spawn Artifact" ),         // 15
                       _( "Spawn Clairvoyance Artifact" ), //16
                       _( "Map editor" ),             // 17
                       _( "Change weather" ),         // 18
                       _( "Kill all monsters" ),    // 19
                       _( "Display hordes" ),         // 20
                       _( "Test Item Group" ),        // 21
                       _( "Damage Self" ),            // 22
                       _( "Show Sound Clustering" ),  // 23
                       _( "Lua Console" ),            // 24
                       _( "Display weather" ),        // 25
                       _( "Display overmap scents" ), // 26
                       _( "Change time" ),            // 27
                       _( "Set automove route" ),     // 28
                       _( "Show mutation category levels" ), // 29
                       _( "Overmap editor" ),         // 30
                       _( "Cancel" ),
                       NULL );
    int veh_num;
    std::vector<std::string> opts;
    refresh_all();
    switch( action ) {
        case 1:
            wishitem( &u );
            break;

        case 2: {
            if( u.in_vehicle ) {
                m.unboard_vehicle( u.pos() );
            }

            auto pt = look_around();
            if( pt == tripoint_min ) {
                break;
            }

            if( m.has_zlevels() && pt.z != get_levz() ) {
                vertical_shift( pt.z );
            }

            u.setpos( pt );
            update_map( u );
            pt = u.pos();
            add_msg( _( "You teleport to point (%d,%d,%d)" ), pt.x, pt.y, pt.z );

            if( m.veh_at( u.pos() ) != nullptr ) {
                m.board_vehicle( u.pos(), &u );
            }
        }
        break;

        case 3: {
            tripoint tmp = overmap::draw_overmap();
            if( tmp != overmap::invalid_tripoint ) {
                //First offload the active npcs.
                unload_npcs();
                while( num_zombies() > 0 ) {
                    despawn_monster( 0 );
                }
                if( u.in_vehicle ) {
                    m.unboard_vehicle( u.pos() );
                }
                const int minz = m.has_zlevels() ? -OVERMAP_DEPTH : get_levz();
                const int maxz = m.has_zlevels() ? OVERMAP_HEIGHT : get_levz();
                for( int z = minz; z <= maxz; z++ ) {
                    m.clear_vehicle_cache( z );
                    m.clear_vehicle_list( z );
                }
                // offset because load_map expects the coordinates of the top left corner, but the
                // player will be centered in the middle of the map.
                const int nlevx = tmp.x * 2 - int( MAPSIZE / 2 );
                const int nlevy = tmp.y * 2 - int( MAPSIZE / 2 );
                u.setz( tmp.z );
                load_map( tripoint( nlevx, nlevy, tmp.z ) );
                load_npcs();
                m.spawn_monsters( true ); // Static monsters
                update_overmap_seen();
                // update weather now as it could be different on the new location
                nextweather = calendar::turn;
                draw_minimap();
            }
        }
        break;
        case 4: {
            auto &cur_om = get_cur_om();
            for( int i = 0; i < OMAPX; i++ ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                        cur_om.seen( i, j, k ) = true;
                    }
                }
            }
            add_msg( m_good, _( "Current overmap revealed." ) );
        }
        break;

        case 5: {
            npc *temp = new npc();
            temp->normalize();
            temp->randomize();
            temp->spawn_at( get_levx(), get_levy(), get_levz() );
            temp->setx( u.posx() - 4 );
            temp->sety( u.posy() - 4 );
            temp->setz( u.posz() );
            temp->form_opinion( u );
            temp->mission = NPC_MISSION_NULL;
            temp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, temp->global_omt_location(),
                                   temp->getID() ) );
            load_npcs();
        }
        break;

        case 6:
            wishmonster();
            break;

        case 7: {
            std::string s;
            s = _( "Location %d:%d in %d:%d, %s\n" );
            s += _( "Current turn: %d; Next spawn %d.\n%s\n" );
            s += ngettext( "%d monster exists.\n", "%d monsters exist.\n", num_zombies() );
            s += ngettext( "%d currently active NPC.\n", "%d currently active NPCs.\n", active_npc.size() );
            s += ngettext( "%d event planned.", "%d events planned", events.size() );
            popup_top(
                s.c_str(),
                u.posx(), u.posy(), get_levx(), get_levy(),
                overmap_buffer.ter( u.global_omt_location() )->name.c_str(),
                int( calendar::turn ), int( nextspawn ),
                ( get_world_option<bool>( "RANDOM_NPC" ) ? _( "NPCs are going to spawn." ) :
                  _( "NPCs are NOT going to spawn." ) ),
                num_zombies(), active_npc.size(), events.size() );
            if( !active_npc.empty() ) {
                for( auto & elem : active_npc ) {
                    tripoint t = ( elem )->global_sm_location();
                    add_msg( m_info, _( "%s: map (%d:%d) pos (%d:%d)" ), ( elem )->name.c_str(), t.x,
                             t.y, ( elem )->posx(), ( elem )->posy() );
                }

                add_msg( m_info, _( "(you: %d:%d)" ), u.posx(), u.posy() );
            }
            disp_NPCs();
            break;
        }
        case 8:
            for( auto & elem : active_npc ) {
                add_msg( _( "%s's head implodes!" ), ( elem )->name.c_str() );
                ( elem )->hp_cur[bp_head] = 0;
            }
            break;

        case 9:
            wishmutate( &u );
            break;

        case 10:
            if( m.veh_at( u.pos() ) ) {
                dbg( D_ERROR ) << "game:load: There's already vehicle here";
                debugmsg( "There's already vehicle here" );
            } else {
                std::vector<vproto_id> veh_strings;
                for( auto & elem : vehicle_prototype::get_all() ) {
                    if( elem != vproto_id( "custom" ) ) {
                        const vehicle_prototype &proto = elem.obj();
                        veh_strings.push_back( elem );
                        //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
                        opts.push_back( string_format( _( "%1$s (%2$s)" ), proto.name.c_str(),
                                                       elem.c_str() ) );
                    }
                }
                opts.push_back( std::string( _( "Cancel" ) ) );
                veh_num = menu_vec( false, _( "Choose vehicle to spawn" ), opts ) + 1;
                veh_num -= 2;
                if( veh_num < ( int )opts.size() - 1 ) {
                    //Didn't pick Cancel
                    const vproto_id &selected_opt = veh_strings[veh_num];
                    tripoint dest = u.pos(); // TODO: Allow picking this when add_vehicle has 3d argument
                    vehicle *veh = m.add_vehicle( selected_opt, dest.x, dest.y, -90, 100, 0 );
                    if( veh != NULL ) {
                        m.board_vehicle( u.pos(), &u );
                    }
                }
            }
            break;

        case 11: {
            wishskill( &u );
        }
        break;

        case 12:
            add_msg( m_info, _( "Martial arts debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( auto & style : all_martialart_types() ) {
                if( style != matype_id( "style_none" ) ) {
                    u.add_martialart( style );
                }
            }
            add_msg( m_good, _( "You now know a lot more than just 10 styles of kung fu." ) );
            break;

        case 13: {
            add_msg( m_info, _( "Recipe debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( const auto &e : recipe_dict ) {
                u.learn_recipe( &e.second );
            }
            add_msg( m_good, _( "You know how to craft that now." ) );
        }
        break;

        case 14: {
            tripoint pos = look_around();
            int npcdex = npc_at( pos );
            if( npcdex == -1 && pos != u.pos() ) {
                popup( _( "No character there." ) );
            } else {
                player &p = npcdex != -1 ? *active_npc[npcdex] : u;
                // The NPC is also required for "Add mission", so has to be in this scope
                npc *np = npcdex != -1 ? active_npc[npcdex] : nullptr;
                uimenu nmenu;
                nmenu.return_invalid = true;

                if( np != nullptr ) {
                    std::stringstream data;
                    data << np->name << " " << ( np->male ? _( "Male" ) : _( "Female" ) ) << std::endl;
                    data << np->myclass.obj().get_name() << "; " <<
                         npc_attitude_name( np->attitude ) << std::endl;
                    if( np->has_destination() ) {
                        data << string_format( _( "Destination: %d:%d:%d (%s)" ),
                                               np->goal.x, np->goal.y, np->goal.z,
                                               overmap_buffer.ter( np->goal )->name.c_str() ) << std::endl;
                    } else {
                        data << _( "No destination." ) << std::endl;
                    }
                    data << string_format( _( "Trust: %d" ), np->op_of_u.trust ) << " "
                         << string_format( _( "Fear: %d" ), np->op_of_u.fear ) << " "
                         << string_format( _( "Value: %d" ), np->op_of_u.value ) << " "
                         << string_format( _( "Anger: %d" ), np->op_of_u.anger ) << " "
                         << string_format( _( "Owed: %d" ), np->op_of_u.owed ) << std::endl;

                    data << string_format( _( "Aggression: %d" ), int( np->personality.aggression ) ) << " "
                         << string_format( _( "Bravery: %d" ), int( np->personality.bravery ) ) << " "
                         << string_format( _( "Collector: %d" ), int( np->personality.collector ) ) << " "
                         << string_format( _( "Altruism: %d" ), int( np->personality.altruism ) ) << std::endl;

                    data << _( "Needs:" ) << std::endl;
                    for( const auto &need : np->needs ) {
                        data << need << std::endl;
                    }

                    nmenu.text = data.str();
                } else {
                    nmenu.text = _( "Player" );
                }

                enum { D_SKILLS, D_STATS, D_ITEMS, D_DELETE_ITEMS, D_ITEM_WORN,
                       D_HP, D_PAIN, D_NEEDS, D_HEALTHY, D_STATUS, D_MISSION, D_TELE,
                       D_MUTATE, D_CLASS
                     };
                nmenu.addentry( D_SKILLS, true, 's', "%s", _( "Edit [s]kills" ) );
                nmenu.addentry( D_STATS, true, 't', "%s", _( "Edit s[t]ats" ) );
                nmenu.addentry( D_ITEMS, true, 'i', "%s", _( "Grant [i]tems" ) );
                nmenu.addentry( D_DELETE_ITEMS, true, 'd', "%s", _( "[d]elete (all) items" ) );
                nmenu.addentry( D_ITEM_WORN, true, 'w', "%s",
                                _( "[w]ear/[w]ield an item from player's inventory" ) );
                nmenu.addentry( D_HP, true, 'h', "%s", _( "Set [h]it points" ) );
                nmenu.addentry( D_PAIN, true, 'p', "%s", _( "Cause [p]ain" ) );
                nmenu.addentry( D_HEALTHY, true, 'a', "%s", _( "Set he[a]lth" ) );
                nmenu.addentry( D_NEEDS, true, 'n', "%s", _( "Set [n]eeds" ) );
                nmenu.addentry( D_MUTATE, true, 'u', "%s", _( "M[u]tate" ) );
                nmenu.addentry( D_STATUS, true, '@', "%s", _( "Status Window [@]" ) );
                nmenu.addentry( D_TELE, true, 'e', "%s", _( "t[e]leport" ) );
                if( p.is_npc() ) {
                    nmenu.addentry( D_MISSION, true, 'm', "%s", _( "Add [m]ission" ) );
                    nmenu.addentry( D_CLASS, true, 'c', "%s", _( "Randomize with [c]lass" ) );
                }
                nmenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
                nmenu.selected = 0;
                nmenu.query();
                switch( nmenu.ret ) {
                    case D_SKILLS:
                        wishskill( &p );
                        break;
                    case D_STATS: {
                        uimenu smenu;
                        smenu.return_invalid = true;
                        smenu.addentry( 0, true, 'S', "%s: %d", _( "Maximum strength" ), p.str_max );
                        smenu.addentry( 1, true, 'D', "%s: %d", _( "Maximum dexterity" ), p.dex_max );
                        smenu.addentry( 2, true, 'I', "%s: %d", _( "Maximum intelligence" ), p.int_max );
                        smenu.addentry( 3, true, 'P', "%s: %d", _( "Maximum perception" ), p.per_max );
                        smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
                        smenu.selected = 0;
                        smenu.query();
                        int *bp_ptr = nullptr;
                        switch( smenu.ret ) {
                            case 0:
                                bp_ptr = &p.str_max;
                                break;
                            case 1:
                                bp_ptr = &p.dex_max;
                                break;
                            case 2:
                                bp_ptr = &p.int_max;
                                break;
                            case 3:
                                bp_ptr = &p.per_max;
                                break;
                            default:
                                break;
                        }

                        if( bp_ptr != nullptr ) {
                            int value;
                            if( query_int( value, "Set the stat to? Currently: %d", *bp_ptr ) && value >= 0 ) {
                                *bp_ptr = value;
                                p.reset_stats();
                            }
                        }
                    }
                    break;
                    case D_ITEMS:
                        wishitem( &p );
                        break;
                    case D_DELETE_ITEMS:
                        if( !query_yn( "Delete all items from the target?" ) ) {
                            break;
                        }
                        for( auto &it : p.worn ) {
                            it.on_takeoff( p );
                        }
                        p.worn.clear();
                        p.inv.clear();
                        p.weapon = p.ret_null;
                        break;
                    case D_ITEM_WORN: {
                        int item_pos = inv_for_filter( "Make target equip:", []( const item &it ) {
                            return it.is_armor();
                        } );
                        item &to_wear = u.i_at( item_pos );
                        if( to_wear.is_armor() ) {
                            p.on_item_wear( to_wear );
                            p.worn.push_back( to_wear );
                        } else if( !to_wear.is_null() ) {
                            p.weapon = to_wear;
                        }
                    }
                    break;
                    case D_HP: {
                        uimenu smenu;
                        smenu.return_invalid = true;
                        smenu.addentry( 0, true, 'q', "%s: %d", _( "Torso" ), p.hp_cur[hp_torso] );
                        smenu.addentry( 1, true, 'w', "%s: %d", _( "Head" ), p.hp_cur[hp_head] );
                        smenu.addentry( 2, true, 'a', "%s: %d", _( "Left arm" ), p.hp_cur[hp_arm_l] );
                        smenu.addentry( 3, true, 's', "%s: %d", _( "Right arm" ), p.hp_cur[hp_arm_r] );
                        smenu.addentry( 4, true, 'z', "%s: %d", _( "Left leg" ), p.hp_cur[hp_leg_l] );
                        smenu.addentry( 5, true, 'x', "%s: %d", _( "Right leg" ), p.hp_cur[hp_leg_r] );
                        smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
                        smenu.selected = 0;
                        smenu.query();
                        int *bp_ptr = nullptr;
                        switch( smenu.ret ) {
                            case 0:
                                bp_ptr = &p.hp_cur[hp_torso];
                                break;
                            case 1:
                                bp_ptr = &p.hp_cur[hp_head];
                                break;
                            case 2:
                                bp_ptr = &p.hp_cur[hp_arm_l];
                                break;
                            case 3:
                                bp_ptr = &p.hp_cur[hp_arm_r];
                                break;
                            case 4:
                                bp_ptr = &p.hp_cur[hp_leg_l];
                                break;
                            case 5:
                                bp_ptr = &p.hp_cur[hp_leg_r];
                                break;
                            default:
                                break;
                        }

                        if( bp_ptr != nullptr ) {
                            int value;
                            if( query_int( value, "Set the hitpoints to? Currently: %d", *bp_ptr ) && value >= 0 ) {
                                *bp_ptr = value;
                                p.reset_stats();
                            }
                        }
                    }
                    break;
                    case D_PAIN: {
                        int value;
                        if( query_int( value, "Cause how much pain? pain: %d", p.get_pain() ) ) {
                            p.mod_pain( value );
                        }
                    }
                    break;
                    case D_NEEDS: {
                        uimenu smenu;
                        smenu.return_invalid = true;
                        smenu.addentry( 0, true, 'h', "%s: %d", _( "Hunger" ), p.get_hunger() );
                        smenu.addentry( 1, true, 't', "%s: %d", _( "Thirst" ), p.get_thirst() );
                        smenu.addentry( 2, true, 'f', "%s: %d", _( "Fatigue" ), p.get_fatigue() );

                        const auto& vits = vitamin::all();
                        for( const auto& v : vits ) {
                            smenu.addentry( -1, true, 0, "%s: %d", v.second.name().c_str(), p.vitamin_get( v.first ) );
                        }

                        smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
                        smenu.selected = 0;
                        smenu.query();

                        switch( smenu.ret ) {
                            int value;
                            case 0:
                                if( query_int( value, "Set hunger to? Currently: %d", p.get_hunger() ) ) {
                                    p.set_hunger( value );
                                }
                                break;

                            case 1:
                                if( query_int( value, "Set thirst to? Currently: %d", p.get_thirst() ) ) {
                                    p.set_thirst( value );
                                }
                                break;

                            case 2:
                                if( query_int( value, "Set fatigue to? Currently: %d", p.get_fatigue() ) ) {
                                    p.set_fatigue( value );
                                }
                                break;

                            default:
                                if( smenu.ret > 2 && smenu.ret < static_cast<int>( vits.size() + 3 ) ) {
                                    auto iter = std::next( vits.begin(), smenu.ret - 3 );
                                    if( query_int( value, "Set %s to? Currently: %d",
                                        iter->second.name().c_str(), p.vitamin_get( iter->first ) ) ) {
                                        p.vitamin_set( iter->first, value );
                                    }
                                }
                        }

                    }
                    break;
                    case D_MUTATE:
                        wishmutate( &p );
                        break;
                    case D_HEALTHY: {
                        uimenu smenu;
                        smenu.return_invalid = true;
                        smenu.addentry( 0, true, 'h', "%s: %d", _( "Health" ), p.get_healthy() );
                        smenu.addentry( 1, true, 'm', "%s: %d", _( "Health modifier" ), p.get_healthy_mod() );
                        smenu.addentry( 999, true, 'q', "%s", _( "[q]uit" ) );
                        smenu.selected = 0;
                        smenu.query();
                        switch( smenu.ret ) {
                            int value;
                            case 0:
                                if( query_int( value, "Set the value to? Currently: %d", p.get_healthy() ) ) {
                                    p.set_healthy( value );
                                }
                                break;
                            case 1:
                                if( query_int( value, "Set the value to? Currently: %d", p.get_healthy_mod() ) ) {
                                    p.set_healthy_mod( value );
                                }
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                    case D_STATUS:
                        p.disp_info();
                        break;
                    case D_MISSION: {
                        uimenu types;
                        types.return_invalid = true;
                        types.text = _( "Choose mission type" );
                        const auto all_missions = mission_type::get_all();
                        std::vector<const mission_type *> mts;
                        for( size_t i = 0; i < all_missions.size(); i++ ) {
                            types.addentry( i, true, -1, all_missions[ i ].name );
                            mts.push_back( &all_missions[ i ] );
                        }

                        types.addentry( INT_MAX, true, -1, _( "Cancel" ) );
                        types.query();
                        if( types.ret >= 0 && types.ret < (int)mts.size() ) {
                            np->add_new_mission( mission::reserve_new( mts[ types.ret ]->id, np->getID() ) );
                        }
                    }
                    break;
                    case D_TELE: {
                        tripoint newpos = look_around();
                        if( newpos != tripoint_min ) {
                            p.setpos( newpos );
                            if( p.is_player() ) {
                                update_map( u );
                            }
                        }
                    }
                    break;
                    case D_CLASS: {
                        uimenu classes;
                        classes.return_invalid = true;
                        classes.text = _( "Choose new class" );
                        std::vector<npc_class_id> ids;
                        size_t i = 0;
                        for( auto &cl : npc_class::get_all() ) {
                            ids.push_back( cl.id );
                            classes.addentry( i, true, -1, cl.get_name() );
                            i++;
                        }

                        classes.addentry( INT_MAX, true, -1, _( "Cancel" ) );
                        classes.query();
                        if( classes.ret < (int)ids.size() && classes.ret >= 0 ) {
                            np->randomize( ids[ classes.ret ] );
                        }
                    }
                }
            }
        }
        break;

        case 15: {
            auto center = look_around();
            if( center != tripoint_min ) {
                artifact_natural_property prop =
                    artifact_natural_property( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) );
                m.create_anomaly( center, prop );
                m.spawn_natural_artifact( center, prop );
            }
        }
        break;

        case 16:
            u.i_add( item( architects_cube(), calendar::turn ) );
            break;

        case 17: {
            tripoint coord = look_debug();
        }
        break;

        case 18: {
            uimenu weather_menu;
            weather_menu.text = _( "Select new weather pattern:" );
            weather_menu.return_invalid = true;
            weather_menu.addentry( 0, true, MENU_AUTOASSIGN, weather_override == WEATHER_NULL ?
                                   _( "Keep normal weather patterns" ) : _( "Disable weather forcing" ) );
            for( int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++ ) {
                weather_menu.addentry( weather_id, true, MENU_AUTOASSIGN,
                                       weather_data( static_cast<weather_type>( weather_id ) ).name );
            }

            weather_menu.addentry( NUM_WEATHER_TYPES, true, MENU_AUTOASSIGN, _( "Cancel" ) );

            weather_menu.query();

            if( weather_menu.ret >= 0 && weather_menu.ret <= NUM_WEATHER_TYPES ) {
                weather_type selected_weather = ( weather_type )weather_menu.selected;
                weather_override = selected_weather;
                nextweather = calendar::turn;
                update_weather();
            }
        }
        break;

        case 19: {
            for( size_t i = 0; i < num_zombies(); i++ ) {
                // Use the normal death functions, useful for testing death
                // and for getting a corpse.
                zombie( i ).die( nullptr );
            }
            cleanup_dead();
        }
        break;
        case 20:
            overmap::draw_hordes();
            break;
        case 21: {
            item_group::debug_spawn();
        }
        break;

        // Damage Self
        case 22: {
            int dbg_damage;
            if( query_int( dbg_damage, _( "Damage self for how much? hp: %d" ), u.hp_cur[hp_torso] ) ) {
                u.hp_cur[hp_torso] -= dbg_damage;
                u.die( NULL );
                draw_sidebar();
            }
        }
        break;

        case 23: {
#ifdef TILES
            const point offset {
                POSX - u.posx() + u.view_offset.x,
                     POSY - u.posy() + u.view_offset.y
            };
            draw_ter();
            auto sounds_to_draw = sounds::get_monster_sounds();
            for( const auto & sound : sounds_to_draw.first ) {
                mvwputch( w_terrain, offset.y + sound.y, offset.x + sound.x, c_yellow, '?' );
            }
            for( const auto & sound : sounds_to_draw.second ) {
                mvwputch( w_terrain, offset.y + sound.y, offset.x + sound.x, c_red, '?' );
            }
            wrefresh( w_terrain );
            getch();
#else
            popup( _( "This binary was not compiled with tiles support." ) );
#endif
        }
        break;

        case 24: {
            lua_console console;
            console.run();
        }
        break;
        case 25:
            overmap::draw_weather();
            break;
        case 26:
            overmap::draw_scents();
            break;
        case 27: {
            auto set_turn = [&]( const int initial, const int factor, const char * const msg ) {
                const auto text = string_input_popup( msg, 20, to_string( initial ), "", "", 20, true );
                if( text.empty() ) {
                    return;
                }
                const int new_value = std::atoi( text.c_str() );
                calendar::turn -= ( initial - new_value ) * factor;
            };

            uimenu smenu;
            smenu.return_invalid = true;
            do {
                const int iSel = smenu.ret;
                smenu.reset();
                smenu.return_invalid = true;
                smenu.addentry( 0, true, 'y', "%s: %d", _( "year" ), calendar::turn.years() );
                smenu.addentry( 1, true, 's', "%s: %d", _( "season" ), int( calendar::turn.get_season() ) );
                smenu.addentry( 2, true, 'd', "%s: %d", _( "day" ), calendar::turn.days() );
                smenu.addentry( 3, true, 'h', "%s: %d", _( "hour" ), calendar::turn.hours() );
                smenu.addentry( 4, true, 'm', "%s: %d", _( "minute" ), calendar::turn.minutes() );
                smenu.addentry( 5, true, 't', "%s: %d", _( "turn" ), calendar::turn.get_turn() );
                smenu.addentry( 6, true, 'q', "%s", _( "quit" ) );
                smenu.selected = iSel;
                smenu.query();

                switch( smenu.ret ) {
                    case 0:
                        set_turn( calendar::turn.years(), DAYS( 1 ) * calendar::turn.year_length(), _( "Set year to?" ) );
                        break;
                    case 1:
                        set_turn( int( calendar::turn.get_season() ), DAYS( 1 ) * calendar::turn.season_length(),
                                  _( "Set season to? (0 = spring)" ) );
                        break;
                    case 2:
                        set_turn( calendar::turn.days(), DAYS( 1 ), _( "Set days to?" ) );
                        break;
                    case 3:
                        set_turn( calendar::turn.hours(), HOURS( 1 ), _( "Set hour to?" ) );
                        break;
                    case 4:
                        set_turn( calendar::turn.minutes(), MINUTES( 1 ), _( "Set minute to?" ) );
                        break;
                    case 5:
                        set_turn( calendar::turn.get_turn(), 1,
                                  string_format( _( "Set turn to? (One day is %i turns)" ), int( DAYS( 1 ) ) ).c_str() );
                        break;
                    default:
                        break;
                }
            } while( smenu.ret != 6 && smenu.ret != UIMENU_INVALID );
        }
        break;
        case 28: {
            tripoint dest = look_around();
            if( dest == tripoint_min || dest == u.pos() ) {
                break;
            }

            auto rt = m.route( u.pos(), dest, u.get_pathfinding_settings(), u.get_path_avoid() );
            u.set_destination( rt );
            if( !u.has_destination() ) {
                popup( "Couldn't find path" );
            }
        }
        break;
        case 29: {
            for( const auto & elem : u.mutation_category_level ) {
                add_msg( "%s: %d", elem.first.c_str(), elem.second );
            }
        }
        break;
        case 30: {
            overmap::draw_editor();
        }
        break;
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

    std::map<std::tuple<std::string,std::string,std::string>,int> kill_counts;

    // map <name, sym, color> to kill count
    for( auto &elem : kills ) {
        const mtype &m = elem.first.obj();
        kill_counts[std::tuple<std::string,std::string,std::string>(
            m.nname(),
            m.sym,
            string_from_color( m.color )
        )] += elem.second;
        totalkills += elem.second;
    }

    for( const auto &entry : kill_counts ) {
        std::ostringstream buffer;
        buffer << "<color_" << std::get<2>( entry.first ) << ">";
        buffer << std::get<1>( entry.first ) << " " << std::get<0>( entry.first );
        buffer << "</color>";
        const int w = colum_width - utf8_width( std::get<0>( entry.first ) );
        buffer.width( w - 3 ); // gap between cols, monster sym, space
        buffer.fill(' ');
        buffer << entry.second;
        buffer.width( 0 );
        data.push_back( buffer.str() );
    }
    std::ostringstream buffer;
    if( data.empty() ) {
        buffer << _( "You haven't killed any monsters yet!" );
    } else {
        buffer << string_format( _( "KILL COUNT: %d" ), totalkills );
    }
    display_table( w, buffer.str(), 3, data );

    werase( w );
    wrefresh( w );
    delwin( w );
    refresh_all();
}

void game::disp_NPC_epilogues()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       std::max(0, (TERMY - FULL_SCREEN_HEIGHT) / 2),
                       std::max(0, (TERMX - FULL_SCREEN_WIDTH) / 2));
    std::vector<std::string> data;
    epilogue epi;
    //This search needs to be expanded to all NPCs
    for( auto &elem : active_npc ) {
        if(elem->is_friend()) {
            if (elem->male){
                epi.random_by_group("male", elem->name);
            } else {
                epi.random_by_group("female", elem->name);
            }
            for( auto &ln : epi.lines ) {
                data.push_back( ln );
            }
            display_table(w, "", 1, data);
        }
        data.clear();
    }

    werase(w);
    wrefresh(w);
    delwin(w);
    refresh_all();
}

void game::disp_faction_ends()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       std::max(0, (TERMY - FULL_SCREEN_HEIGHT) / 2),
                       std::max(0, (TERMX - FULL_SCREEN_WIDTH) / 2));
    std::vector<std::string> data;

    for( auto &elem : factions ) {
        if(elem.known_by_u) {
            if (elem.name == "Your Followers"){
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "" );
                data.push_back( "       You are forgotten among the billions lost in the cataclysm..." );

                display_table(w, "", 1, data);
            } else if (elem.name == "The Old Guard" && elem.power != 100){
                if (elem.power < 150){
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    Locked in an endless battle, the Old Guard was forced to consolidate their");
                    data.push_back( "resources in a handful of fortified bases along the coast.  Without the men" );
                    data.push_back( "or material to rebuild, the soldiers that remained lost all hope..." );
                } else {
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The steadfastness of individual survivors after the cataclysm impressed ");
                    data.push_back( "the tattered remains of the once glorious union.  Spurred on by small ");
                    data.push_back( "successes, a number of operations to re-secure facilities met with limited ");
                    data.push_back( "success.  Forced to eventually consolidate to large bases, the Old Guard left");
                    data.push_back( "these facilities in the hands of the few survivors that remained.  As the ");
                    data.push_back( "years past, little materialized from the hopes of rebuilding civilization...");
                }
                display_table(w, "The Old Guard", 1, data);
            } else if (elem.name == "The Free Merchants" && elem.power != 100){
                if (elem.power < 150){
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    Life in the refugee shelter deteriorated as food shortages and disease ");
                    data.push_back( "destroyed any hope of maintaining a civilized enclave.  The merchants and ");
                    data.push_back( "craftsmen dispersed to found new colonies but most became victims of");
                    data.push_back( "marauding bandits.  Those who survived never found a place to call home...");
                } else {
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The Free Merchants struggled for years to keep themselves fed but their");
                    data.push_back( "once profitable trade routes were plundered by bandits and thugs.  In squalor");
                    data.push_back( "and filth the first generations born after the cataclysm are told stories of");
                    data.push_back( "the old days when food was abundant and the children were allowed to play in");
                    data.push_back( "the sun...");
                }
                display_table(w, "The Free Merchants", 1, data);
            } else if (elem.name == "The Tacoma Commune" && elem.power != 100){
                if (elem.power < 150){
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The fledgling outpost was abandoned a few months later.  The external");
                    data.push_back( "threats combined with low crop yields caused the Free Merchants to withdraw");
                    data.push_back( "their support.  When the exhausted migrants returned to the refugee center");
                    data.push_back( "they were turned away to face the world on their own.");
                } else {
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The commune continued to grow rapidly through the years despite constant");
                    data.push_back( "external threat.  While maintaining a reputation as a haven for all law");
                    data.push_back( "abiding citizens, the commune's leadership remained loyal to the interests of");
                    data.push_back( "the Free Merchants.  Hard labor for little reward remained the price to be");
                    data.push_back( "paid for those who sought the safety of the community.");
                }
                display_table(w, "The Tacoma Commune", 1, data);
            } else if (elem.name == "The Wasteland Scavengers" && elem.power != 100){
                if (elem.power < 150){
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The lone bands of survivors who wandered the now alien world dwindled in");
                    data.push_back( "number through the years.  Unable to compete with the growing number of ");
                    data.push_back( "monstrosities that had adapted to live in their world, those who did survive");
                    data.push_back( "lived in dejected poverty and hopelessness...");
                } else {
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The scavengers who flourished in the opening days of the cataclysm found");
                    data.push_back( "an ever increasing challenge in finding and maintaining equipment from the ");
                    data.push_back( "old world.  Enormous hordes made cities impossible to enter while new ");
                    data.push_back( "eldritch horrors appeared mysteriously near old research labs.  But on the ");
                    data.push_back( "fringes of where civilization once ended, bands of hunter-gatherers began to");
                    data.push_back( "adopt agrarian lifestyles in fortified enclaves...");
                }
                display_table(w, "The Wasteland Scavengers", 1, data);
            } else if (elem.name == "Hell's Raiders" && elem.power != 100){
                if (elem.power < 150){
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    The raiders grew more powerful than any other faction as attrition ");
                    data.push_back( "destroyed the Old Guard.  The ruthless men and women who banded together to");
                    data.push_back( "rob refugees and pillage settlements soon found themselves without enough ");
                    data.push_back( "victims to survive.  The Hell's Raiders were eventually destroyed when ");
                    data.push_back( "infighting erupted into civil war but there were few survivors left to ");
                    data.push_back( "celebrate their destruction.");
                } else {
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "" );
                    data.push_back( "    Fueled by drugs and rage, the Hell's Raiders fought tooth and nail to");
                    data.push_back( "overthrow the last strongholds of the Old Guard.  The costly victories ");
                    data.push_back( "brought the warlords abundant territory and slaves but little in the way of");
                    data.push_back( "stability.  Within weeks, infighting led to civil war as tribes vied for ");
                    data.push_back( "leadership of the faction.  When only one warlord finally secured control,");
                    data.push_back( "there was nothing left to fight for... just endless cities full of the dead.");
                }
                display_table(w, "Hell's Raiders", 1, data);
            }

        }
        data.clear();
    }

    werase(w);
    wrefresh(w);
    delwin(w);
    refresh_all();
}

struct npc_dist_to_player {
    const tripoint ppos;
    npc_dist_to_player() : ppos( g->u.global_omt_location() ) { }
    // Operator overload required to leverage sort API.
    bool operator()( const npc *a, const npc *b ) const {
        const tripoint apos = a->global_omt_location();
        const tripoint bpos = b->global_omt_location();
        return square_dist( ppos.x, ppos.y, apos.x, apos.y ) <
            square_dist( ppos.x, ppos.y, bpos.x, bpos.y );
    }
};

void game::disp_NPCs()
{
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    const tripoint ppos = u.global_omt_location();
    const tripoint &lpos = u.pos();
    mvwprintz( w, 0, 0, c_white, _("Your overmap position: %d, %d, %d"), ppos.x, ppos.y, ppos.z );
    mvwprintz( w, 1, 0, c_white, _("Your local position: %d, %d, %d"), lpos.x, lpos.y, lpos.z );
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_player(100);
    std::sort(npcs.begin(), npcs.end(), npc_dist_to_player() );
    for (size_t i = 0; i < 20 && i < npcs.size(); i++) {
        const tripoint apos = npcs[i]->global_omt_location();
        mvwprintz(w, i + 3, 0, c_white, "%s: %d, %d, %d", npcs[i]->name.c_str(),
                  apos.x, apos.y, apos.z);
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
                      _("Ranking:           %s"), fac_ranking_text(cur_frac->likes_u).c_str());
            mvwprintz(w_info, 1, 0, c_white,
                      _("Respect:           %s"), fac_respect_text(cur_frac->respects_u).c_str());
            mvwprintz(w_info, 2, 0, c_white,
                      _("Wealth:            %s"), fac_wealth_text(cur_frac->wealth, cur_frac->size).c_str());
            mvwprintz(w_info, 3, 0, c_white,
                      _("Food Supply:       %s"), fac_food_supply_text(cur_frac->food_supply, cur_frac->size).c_str());
            mvwprintz(w_info, 4, 0, c_white,
                      _("Combat Ability:    %s"), fac_combat_ability_text(cur_frac->combat_ability).c_str());
            fold_and_print(w_info, 6, 0, maxlength, c_white, cur_frac->describe());
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
        } else if ( action == "HELP_KEYBINDINGS" ) {
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
        std::vector<mission*> umissions;
        switch (tab) {
        case 0:
            umissions = u.get_active_missions();
            break;
        case 1:
            umissions = u.get_completed_missions();
            break;
        case 2:
            umissions = u.get_failed_missions();
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

        draw_tab( w_missions, 7, _( "ACTIVE MISSIONS" ), tab == 0 );
        draw_tab( w_missions, 30, _( "COMPLETED MISSIONS" ), tab == 1 );
        draw_tab( w_missions, 56, _( "FAILED MISSIONS" ), tab == 2 );

        mvwputch(w_missions, 2, 0, BORDER_COLOR, LINE_OXXO); // |^
        mvwputch(w_missions, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX); // ^|

        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO); // |
        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOOX); // _|

        mvwputch(w_missions, 2, 30, BORDER_COLOR, (tab == 1) ? LINE_XOXX : LINE_XXXX); // + || -|
        mvwputch(w_missions, FULL_SCREEN_HEIGHT - 1, 30, BORDER_COLOR, LINE_XXOX); // _|_

        for (size_t i = 0; i < umissions.size(); i++) {
            const auto miss = umissions[i];
            nc_color col = c_white;
            if( u.get_active_mission() == miss ) {
                col = c_ltred;
            }
            if (selection == i) {
                mvwprintz(w_missions, 3 + i, 1, hilite(col), "%s", miss->name().c_str());
            } else {
                mvwprintz(w_missions, 3 + i, 1, col, "%s", miss->name().c_str());
            }
        }

        if (selection < umissions.size()) {
            const auto miss = umissions[selection];
            mvwprintz(w_missions, 4, 31, c_white, "%s", miss->get_description().c_str());
            if( miss->has_deadline() ) {
                // TODO: proper formatting of turns, see calendar class, it has some nice functions
                mvwprintz(w_missions, 5, 31, c_white, _("Deadline: %d (%d)"),
                          int(miss->get_deadline()), int(calendar::turn));
            }
            if( miss->has_target() ) {
                const tripoint pos = u.global_omt_location();
                // TODO: target does not contain a z-component, targets are assumed to be on z=0
                mvwprintz(w_missions, 6, 31, c_white, _("Target: (%d, %d)   You: (%d, %d)"),
                          miss->get_target().x, miss->get_target().y, pos.x, pos.y);
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
            if( tab == 0 && selection < umissions.size() ) {
                u.set_active_mission( *umissions[selection] );
            }
            break;
        } else if (action == "QUIT") {
            break;
        }
    }

    werase(w_missions);
    delwin(w_missions);
    refresh_all();
}

// A little helper to draw footstep glyphs.
static void draw_footsteps( WINDOW *window, const tripoint &offset )
{
    for( const auto &footstep : sounds::get_footstep_markers() ) {
        char glyph = '?';
        if( footstep.z != offset.z ) { // Here z isn't an offset, but a coordinate
            glyph = footstep.z > offset.z ? '^' : 'v';
        }

        mvwputch( window, offset.y + footstep.y, offset.x + footstep.x, c_yellow, glyph );
    }
}

void game::draw()
{
    // Draw map
    werase(w_terrain);

    //temporary fix for updating visibility for minimap
    ter_view_z = ( u.pos() + u.view_offset ).z;
    m.build_map_cache( ter_view_z );
    m.update_visibility_cache( ter_view_z );

    draw_sidebar();
    draw_ter();
    if( !is_draw_tiles_mode() ) {
        wrefresh(w_terrain);
    }
#ifdef TILES
    try_sdl_update();
#endif // TILES
}

void game::draw_pixel_minimap()
{
    // Force a refresh of the pixel minimap.
    // only do so if it is in use
    if(pixel_minimap_option && w_pixel_minimap){
        werase(w_pixel_minimap);
        //trick window into rendering
        mvwputch(w_pixel_minimap, 0, 0, c_black, ' ');
        wrefresh(w_pixel_minimap);
    }
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
    u.disp_status(w_status, w_status2);

    WINDOW *time_window = sideStyle ? w_status2 : w_status;
    wmove(time_window, sideStyle ? 0 : 1, sideStyle ? 15 : 41);
    if ( u.has_watch() ) {
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

    const oter_id &cur_ter = overmap_buffer.ter(u.global_omt_location());

    const std::string &tername = cur_ter->name;
    werase(w_location);
    mvwprintz(w_location, 0, 0, cur_ter->color, "%s", utf8_truncate(tername, 14).c_str());

    if (get_levz() < 0) {
        mvwprintz(w_location, 0, 18, c_ltgray, _("Underground"));
    } else {
        mvwprintz(w_location, 0, 18, weather_data(weather).color, "%s", weather_data(weather).name.c_str());
    }

    if( u.worn_with_flag( "THERMOMETER" ) || u.has_bionic( "bio_meteorologist" ) ) {
        wprintz( w_location, c_white, " %s", print_temperature( get_temperature() ).c_str());
    }

    //moon phase display
    static std::vector<std::string> vMoonPhase = {"(   )", "(  ))", "( | )", "((  )"};

    const int iPhase = int(calendar::turn.moon());
    std::string sPhase = vMoonPhase[iPhase%4];

    if (iPhase > 0) {
        sPhase.insert(5 - ((iPhase > 4) ? iPhase%4 : 0), "</color>");
        sPhase.insert(5 - ((iPhase < 4) ? iPhase+1 : 5), "<color_" + string_from_color(i_black) + ">");
    }

    trim_and_print( w_location, 1, 0, 10, c_white, _("Moon %s"), sPhase.c_str() );

    const auto ll = get_light_level(g->u.fine_detail_vision_mod());
    mvwprintz(w_location, 1, 15, c_ltgray, "%s ", _("Lighting:"));
    wprintz(w_location, ll.second, ll.first.c_str());

    wrefresh(w_location);

    //Safemode coloring
    WINDOW *day_window = sideStyle ? w_status2 : w_status;
    mvwprintz(day_window, 0, sideStyle ? 0 : 41, c_white, _("%s, day %d"),
              season_name_upper(calendar::turn.get_season()).c_str(), calendar::turn.days() + 1);
    if (safe_mode != SAFE_MODE_OFF || autosafemode != 0) {
        int iPercent = turnssincelastmon * 100 / get_option<int>( "AUTOSAFEMODETURNS" );
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

    draw_minimap();
    draw_pixel_minimap();
    draw_sidebar_messages();
}

void game::draw_sidebar_messages()
{
    if (fullscreen) {
        return;
    }

    werase(w_messages);

    // Print liveview or monster info and start log messages output below it.
    int topline = liveview.draw(w_messages, getmaxy(w_messages));
    if ( topline == 0 ) {
        topline = mon_info(w_messages) + 2;
    }
    int line = getmaxy(w_messages) - 1;
    int maxlength = getmaxx(w_messages);
    Messages::display_messages(w_messages, 0, topline, maxlength, line);
    wrefresh(w_messages);
}

void game::draw_critter( const Creature &critter, const tripoint &center )
{
    const int my = POSY + ( critter.posy() - center.y );
    const int mx = POSX + ( critter.posx() - center.x );
    if( !is_valid_in_w_terrain( mx, my ) ) {
        return;
    }
    if( critter.posz() != center.z && m.has_zlevels() ) {
        static const tripoint up_tripoint( 0, 0, 1 );
        if( critter.posz() == center.z - 1 &&
            ( debug_mode || u.sees( critter ) ) &&
            m.valid_move( critter.pos(), critter.pos() + up_tripoint, false, true ) ) {
            // Monster is below
            // TODO: Make this show something more informative than just green 'v'
            // TODO: Allow looking at this mon with look command
            // TODO: Redraw this after weather etc. animations
            mvwputch( w_terrain, my, mx, c_green_cyan, 'v' );
        }
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

void game::draw_ter( const bool draw_sounds )
{
    draw_ter( u.pos() + u.view_offset, false, draw_sounds );
}

void game::draw_ter( const tripoint &center, const bool looking, const bool draw_sounds )
{
    ter_view_x = center.x;
    ter_view_y = center.y;
    ter_view_z = center.z;
    const int posx = center.x;
    const int posy = center.y;

    // TODO: Make it not rebuild the cache all the time (cache point+moves?)
    if( !looking ) {
        // If we're looking, the cache is built at start (entering looking mode)
        m.build_map_cache( center.z );
    }

    m.draw( w_terrain, center );

    if( draw_sounds ) {
        draw_footsteps( w_terrain, {POSX - center.x, POSY - center.y, center.z} );
    }

    // Draw monsters
    for( size_t i = 0; i < num_zombies(); i++ ) {
        draw_critter( critter_tracker->find( i ), center );
    }

    // Draw NPCs
    for( const npc* n : active_npc ) {
        draw_critter( *n, center );
    }

    // Draw player last
    draw_critter( u, center );

    if( u.has_active_bionic("bio_scent_vision") && u.view_offset.z == 0 ) {
        tripoint tmp = center;
        int &realx = tmp.x;
        int &realy = tmp.y;
        for( realx = posx - POSX; realx <= posx + POSX; realx++ ) {
            for( realy = posy - POSY; realy <= posy + POSY; realy++ ) {
                if( scent.get( tmp ) != 0 ) {
                    int tempx = posx - realx;
                    int tempy = posy - realy;
                    if ( !( isBetween( tempx, -2, 2 ) &&
                            isBetween( tempy, -2, 2 ) ) ) {
                        if( mon_at( tmp ) != -1 ) {
                            mvwputch( w_terrain, realy + POSY - posy,
                                      realx + POSX - posx, c_white, '?' );
                        } else {
                            mvwputch( w_terrain, realy + POSY - posy,
                                      realx + POSX - posx, c_magenta, '#' );
                        }
                    }
                }
            }
        }
    }

    if( !destination_preview.empty() && u.view_offset.z == 0 ) {
        // Draw auto-move preview trail
        const tripoint &final_destination = destination_preview.back();
        tripoint line_center = u.pos() + u.view_offset;
        draw_line( final_destination, line_center, destination_preview );
        mvwputch(w_terrain, POSY + (final_destination.y - (u.posy() + u.view_offset.y)),
                 POSX + (final_destination.x - (u.posx() + u.view_offset.x)), c_white, 'X');
    }

    if( u.controlling_vehicle && !looking ) {
        draw_veh_dir_indicator( false );
        draw_veh_dir_indicator( true );
    }
    if(uquit == QUIT_WATCH) {
        // This should remove the flickering the bar receives
        input_context ctxt("DEFAULTMODE");
        std::string message = string_format( _("Press %s to accept your fate..."),
                ctxt.get_desc("QUIT").c_str() );
        popup(message, PF_NO_WAIT_ON_TOP);
    }

    // Place the cursor over the player as is expected by screen readers.
    wmove( w_terrain, POSY + g->u.pos().y - center.y, POSX + g->u.pos().x - center.x );

    wrefresh(w_terrain);

}

tripoint game::get_veh_dir_indicator_location( bool next ) const
{
    if( !get_option<bool>( "VEHICLE_DIR_INDICATOR" ) ) {
        return tripoint_min;
    }
    vehicle *veh = m.veh_at( u.pos() );
    if( !veh ) {
        return tripoint_min;
    }
    rl_vec2d face = next ? veh->dir_vec() : veh->face_vec();
    float r = 10.0;
    return { static_cast<int>(r * face.x), static_cast<int>(r * face.y), u.pos().z };
}

void game::draw_veh_dir_indicator( bool next )
{
    tripoint indicator_offset = get_veh_dir_indicator_location( next );
    if( indicator_offset != tripoint_min ) {
        auto col = next ? c_white : c_dkgray;
        mvwputch( w_terrain, POSY + indicator_offset.y - u.view_offset.y,
                  POSX + indicator_offset.x - u.view_offset.x, col, 'X' );
    }
}

void game::refresh_all()
{
    const int minz = m.has_zlevels() ? -OVERMAP_DEPTH : get_levz();
    const int maxz = m.has_zlevels() ? OVERMAP_HEIGHT : get_levz();
    for( int z = minz; z <= maxz; z++ ) {
        m.reset_vehicle_cache( z );
    }

    draw();
    refresh();
}

void game::draw_HP()
{
    werase(w_HP);

    // The HP window can be in "tall" mode (7x14) or "wide" mode (14x7).
    bool wide = (getmaxy(w_HP) == 7);
    int hpx = wide ? 7 : 0;
    int hpy = wide ? 0 : 1;
    int dy = wide ? 1 : 2;

    bool const is_self_aware = u.has_trait("SELFAWARE");

    for (int i = 0; i < num_hp_parts; i++) {
        auto const &hp = get_hp_bar(u.hp_cur[i], u.hp_max[i]);

        wmove(w_HP, i * dy + hpy, hpx);
        if (is_self_aware) {
            wprintz(w_HP, hp.second, "%3d  ", u.hp_cur[i]);
        } else {
            wprintz(w_HP, hp.second, "%s", hp.first.c_str());

            //Add the trailing symbols for a not-quite-full health bar
            int bar_remainder = 5;
            while (bar_remainder > (int)hp.first.size()) {
                --bar_remainder;
                wprintz(w_HP, c_white, ".");
            }
        }
    }

    static const char *body_parts[] = { _("HEAD"), _("TORSO"), _("L ARM"),
                                        _("R ARM"), _("L LEG"), _("R LEG"),
                                        _("POWER")
                                      };
    static body_part part[] = { bp_head, bp_torso, bp_arm_l,
                                bp_arm_r, bp_leg_l, bp_leg_r, num_bp
                              };
    int num_parts = sizeof(body_parts) / sizeof(body_parts[0]);
    for (int i = 0; i < num_parts; i++) {
        const char *str = body_parts[i];
        wmove(w_HP, i * dy, 0);
        if (wide) {
            wprintz(w_HP, u.limb_color(part[i], true, true, true), " ");
        }
        wprintz(w_HP, u.limb_color(part[i], true, true, true), str);
        if (!wide) {
            wprintz(w_HP, u.limb_color(part[i], true, true, true), ":");
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
        nc_color color = c_red;
        if (u.power_level == u.max_power_level) {
            color = c_blue;
        } else if (u.power_level >= u.max_power_level * .5) {
            color = c_ltblue;
        } else if (u.power_level > 0) {
            color = c_yellow;
        }
        mvwprintz(w_HP, powy, powx, color, "%-3d", u.power_level);
    }
    if( !wide ) {
        mvwprintz(w_HP, 14, hpx, c_white, "%s", _("STA"));
        wmove(w_HP, 15, hpx);
        u.print_stamina_bar(w_HP);
    }
    wrefresh(w_HP);
}

void game::draw_minimap()
{
    // Draw the box
    werase(w_minimap);
    draw_border(w_minimap);

    const tripoint curs = u.global_omt_location();
    const int cursx = curs.x;
    const int cursy = curs.y;
    const tripoint targ = u.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;

    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            const int omx = cursx + i;
            const int omy = cursy + j;
            nc_color ter_color;
            long ter_sym;
            const bool seen = overmap_buffer.seen(omx, omy, get_levz());
            const bool vehicle_here = overmap_buffer.has_vehicle(omx, omy, get_levz());
            if (overmap_buffer.has_note(omx, omy, get_levz())) {

                const std::string &note_text = overmap_buffer.note(omx, omy, get_levz());

                ter_color = c_yellow;
                ter_sym = 'N';

                int symbolIndex = note_text.find(':');
                int colorIndex = note_text.find(';');

                bool symbolFirst = symbolIndex < colorIndex;

                if (colorIndex > -1 && symbolIndex > -1) {
                    if (symbolFirst) {
                        if (colorIndex > 4) {
                            colorIndex = -1;
                        }
                        if (symbolIndex > 1) {
                            symbolIndex = -1;
                            colorIndex = -1;
                        }
                    } else {
                        if (symbolIndex > 4) {
                            symbolIndex = -1;
                        }
                        if (colorIndex > 2) {
                            colorIndex = -1;
                        }
                    }
                } else if (colorIndex > 2) {
                    colorIndex = -1;
                } else if (symbolIndex > 1) {
                    symbolIndex = -1;
                }

                if (symbolIndex > -1) {
                    int symbolStart = 0;
                    if (colorIndex > -1 && !symbolFirst) {
                        symbolStart = colorIndex + 1;
                    }
                    ter_sym = note_text.substr(symbolStart, symbolIndex - symbolStart).c_str()[0];
                }

                if (colorIndex > -1) {

                    int colorStart = 0;

                    if (symbolIndex > -1 && symbolFirst) {
                        colorStart = symbolIndex + 1;
                    }

                    std::string sym = note_text.substr(colorStart, colorIndex - colorStart);

                    if (sym.length() == 2) {
                        if (sym.compare("br") == 0) {
                            ter_color = c_brown;
                        } else if (sym.compare("lg") == 0) {
                            ter_color = c_ltgray;
                        } else if (sym.compare("dg") == 0) {
                            ter_color = c_dkgray;
                        }
                    } else {
                        char colorID = sym.c_str()[0];
                        if (colorID == 'r') {
                            ter_color = c_ltred;
                        } else if (colorID == 'R') {
                            ter_color = c_red;
                        } else if (colorID == 'g') {
                            ter_color = c_ltgreen;
                        } else if (colorID == 'G') {
                            ter_color = c_green;
                        } else if (colorID == 'b') {
                            ter_color = c_ltblue;
                        } else if (colorID == 'B') {
                            ter_color = c_blue;
                        } else if (colorID == 'W') {
                            ter_color = c_white;
                        } else if (colorID == 'C') {
                            ter_color = c_cyan;
                        } else if (colorID == 'c') {
                            ter_color = c_ltcyan;
                        } else if (colorID == 'P') {
                            ter_color = c_pink;
                        } else if (colorID == 'm') {
                            ter_color = c_magenta;
                        }
                    }
                }
            } else if (!seen) {
                ter_sym = ' ';
                ter_color = c_black;
            } else if (vehicle_here) {
                ter_color = c_cyan;
                ter_sym = 'c';
            } else {
                const oter_id &cur_ter = overmap_buffer.ter(omx, omy, get_levz());
                ter_sym = cur_ter->sym;
                if (overmap_buffer.is_explored(omx, omy, get_levz())) {
                    ter_color = c_dkgray;
                } else {
                    ter_color = cur_ter->color;
                }
            }
            if (!drew_mission && targ.x == omx && targ.y == omy) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
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
    if( !drew_mission ) {
        double slope = ( cursx != targ.x ) ? double( targ.y - cursy ) / double( targ.x - cursx ) : 4;

        if( cursx == targ.x || fabs( slope ) > 3.5 ) { // Vertical slope
            if( targ.y > cursy ) {
                mvwputch( w_minimap, 6, 3, c_red, '*' );
            } else {
                mvwputch( w_minimap, 0, 3, c_red, '*' );
            }
        } else {
            int arrowx = 3, arrowy = 3;
            if( fabs( slope ) >= 1. ) { // y diff is bigger!
                arrowy = ( targ.y > cursy ? 6 : 0 );
                arrowx = int( 3 + 3 * ( targ.y > cursy ? slope : ( 0 - slope ) ) );
                if( arrowx < 0 ) {
                    arrowx = 0;
                }
                if( arrowx > 6 ) {
                    arrowx = 6;
                }
            } else {
                arrowx = ( targ.x > cursx ? 6 : 0 );
                arrowy = int( 3 + 3 * ( targ.x > cursx ? slope : ( 0 - slope ) ) );
                if( arrowy < 0 ) {
                    arrowy = 0;
                }
                if( arrowy > 6 ) {
                    arrowy = 6;
                }
            }
            char glyph = '*';
            if( targ.z > u.posz() ) {
                glyph = '^';
            } else if( targ.z < u.posz() ) {
                glyph = 'v';
            }

            mvwputch( w_minimap, arrowy, arrowx, c_red, glyph );
        }
    }
    wrefresh( w_minimap );
}

float game::natural_light_level( const int zlev ) const
{
    if( zlev > OVERMAP_HEIGHT || zlev < 0 ) {
        return LIGHT_AMBIENT_MINIMAL;
    }

    if( latest_lightlevels[zlev] > -std::numeric_limits<float>::max() ) {
        // Already found the light level for now?
        return latest_lightlevels[zlev];
    }

    float ret = LIGHT_AMBIENT_MINIMAL;

    // Sunlight/moonlight related stuff, ignore while underground
    if( zlev >= 0 ) {
        if( !lightning_active ) {
            ret = calendar::turn.sunlight();
        } else {
            // Recent lightning strike has lit the area
            ret = DAYLIGHT_LEVEL;
        }

        ret += weather_data(weather).light_modifier;
    }

    // Artifact light level changes here. Even though some of these only have an effect
    // aboveground it is cheaper performance wise to simply iterate through the entire
    // list once instead of twice.
    float mod_ret = -1;
    // Each artifact change does std::max(mod_ret, new val) since a brighter end value
    // will trump a lower one.
    for( const auto &e : events ) {
        // EVENT_DIM slowly dims the natural sky level, then relights it.
        if( e.type == EVENT_DIM ) {
            int turns_left = e.turn - int(calendar::turn);
            // EVENT_DIM has an occurrence date of turn + 50, so the first 25 dim it,
            if (turns_left > 25) {
                mod_ret = std::max(mod_ret, (ret * (turns_left - 25)) / 25);
            // and the last 25 scale back towards normal.
            } else {
                mod_ret = std::max(mod_ret, (ret * (25 - turns_left)) / 25);
            }
        } else if ( e.type == EVENT_ARTIFACT_LIGHT ) {
            // EVENT_ARTIFACT_LIGHT causes everywhere to become as bright as day.
            mod_ret = std::max<float>( ret, DAYLIGHT_LEVEL );
        }
    }
    // If we had a changed light level due to an artifact event then it overwrites
    // the natural light level.
    if( mod_ret > -1 ) {
        ret = mod_ret;
    }

    // Cap everything to our minimum light level
    ret = std::max<float>( LIGHT_AMBIENT_MINIMAL, ret );

    latest_lightlevels[zlev] = ret;

    return ret;
}

unsigned char game::light_level( const int zlev ) const
{
    const float light = natural_light_level( zlev );
    return LIGHT_RANGE(light);
}

void game::reset_light_level()
{
    for( float &lev : latest_lightlevels ) {
        lev = -std::numeric_limits<float>::max();
    }
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
    int distance = (get_option<int>( "SAFEMODEPROXIMITY" ) <= 0) ? MAX_VIEW_DISTANCE : get_option<int>( "SAFEMODEPROXIMITY" );
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
        monster &critter = critter_tracker->find(i);

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

    int newseen = 0;
    const int iProxyDist = (get_option<int>( "SAFEMODEPROXIMITY" ) <= 0) ? MAX_VIEW_DISTANCE : get_option<int>( "SAFEMODEPROXIMITY" );
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::vector<npc*> unique_types[9];
    std::vector<const mtype*> unique_mons[9];
    // dangerous_types tracks whether we should print in red to warn the player
    bool dangerous[8];
    for( auto &dangerou : dangerous ) {
        dangerou = false;
    }

    tripoint view = u.pos() + u.view_offset;
    new_seen_mon.clear();

    for( auto &c : u.get_visible_creatures( SEEX * MAPSIZE ) ) {
        const auto m = dynamic_cast<monster*>( c );
        const auto p = dynamic_cast<npc*>( c );
        const auto dir_to_mon = direction_from( view.x, view.y, c->posx(), c->posy() );
        const int mx = POSX + ( c->posx() - view.x );
        const int my = POSY + ( c->posy() - view.y );
        int index = 8;
        if( !is_valid_in_w_terrain( mx, my ) ) {
            // for compatibility with old code, see diagram below, it explains the values for index,
            // also might need revisiting one z-levels are in.
            switch( dir_to_mon ) {
                case ABOVENORTHWEST:
                case NORTHWEST:
                case BELOWNORTHWEST:
                    index = 7;
                    break;
                case ABOVENORTH:
                case NORTH:
                case BELOWNORTH:
                    index = 0;
                    break;
                case ABOVENORTHEAST:
                case NORTHEAST:
                case BELOWNORTHEAST:
                    index = 1;
                    break;
                case ABOVEWEST:
                case WEST:
                case BELOWWEST:
                    index = 6;
                    break;
                case ABOVECENTER:
                case CENTER:
                case BELOWCENTER:
                    index = 8;
                    break;
                case ABOVEEAST:
                case EAST:
                case BELOWEAST:
                    index = 2;
                    break;
                case ABOVESOUTHWEST:
                case SOUTHWEST:
                case BELOWSOUTHWEST:
                    index = 5;
                    break;
                case ABOVESOUTH:
                case SOUTH:
                case BELOWSOUTH:
                    index = 4;
                    break;
                case ABOVESOUTHEAST:
                case SOUTHEAST:
                case BELOWSOUTHEAST:
                    index = 3;
                    break;
            }
        }

        rule_state safemode_state = RULE_NONE;
        const bool safemode_empty = get_safemode().empty();

        if( m != nullptr ) {
            //Safemode monster check
            auto &critter = *m;

            const monster_attitude matt = critter.attitude(&u);
            const int mon_dist = rl_dist( u.pos(), critter.pos() );
            safemode_state = get_safemode().check_monster(critter.name(), critter.attitude_to(u), mon_dist);

            if ((!safemode_empty && safemode_state == RULE_BLACKLISTED) || (safemode_empty && (MATT_ATTACK == matt || MATT_FOLLOW == matt))) {
                if (index < 8 && critter.sees( g->u )) {
                    dangerous[index] = true;
                }

                if (!safemode_empty || mon_dist <= iProxyDist) {
                    bool passmon = false;
                    if (critter.ignoring > 0) {
                        if (safe_mode != SAFE_MODE_ON) {
                            critter.ignoring = 0;
                        } else if (mon_dist > critter.ignoring / 2 || mon_dist < 6) {
                            passmon = true;
                        }
                    }

                    if (!passmon) {
                        int news = mon_at( critter.pos(), true );
                        if( news != -1 ) {
                            newseen++;
                            new_seen_mon.push_back( news );
                        } else {
                            debugmsg( "%s at (%d,%d,%d) was not found in the tracker",
                                      critter.disp_name().c_str(),
                                      critter.posx(), critter.posy(), critter.posz() );
                        }
                    }
                }
            }

            auto &vec = unique_mons[index];
            if( std::find( vec.begin(), vec.end(), critter.type ) == vec.end() ) {
                vec.push_back( critter.type );
            }
        } else if( p != nullptr ) {
            //Safe mode NPC check

            const int npc_dist = rl_dist( u.pos(), p->pos() );
            safemode_state = get_safemode().check_monster(get_safemode().npc_type_name(), p->attitude_to( u ), npc_dist);

            if ( ( !safemode_empty && safemode_state == RULE_BLACKLISTED ) || (safemode_empty && p->attitude == NPCATT_KILL ) ) {
                if ( !safemode_empty || npc_dist <= iProxyDist ) {
                    newseen++;
                }
            }
            unique_types[index].push_back( p );
        }
    }

    if (newseen > mostseen) {
        if (newseen - mostseen == 1) {
            if (!new_seen_mon.empty()) {
                monster &critter = critter_tracker->find(new_seen_mon.back());
                cancel_activity_query(_("%s spotted!"), critter.name().c_str());
                if (u.has_trait("M_DEFENDER") && critter.type->in_species( PLANT )) {
                    add_msg(m_warning, _("We have detected a %s."), critter.name().c_str());
                    if (!u.has_effect( effect_adrenaline_mycus)){
                        u.add_effect( effect_adrenaline_mycus, 300 );
                    } else if( u.get_effect_int( effect_adrenaline_mycus ) == 1 ) {
                        // Triffids present.  We ain't got TIME to adrenaline comedown!
                        u.add_effect( effect_adrenaline_mycus, 150);
                        u.mod_pain(3); // Does take it out of you, though
                        add_msg(m_info, _("Our fibers strain with renewed wrath!"));
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
            set_safe_mode( SAFE_MODE_STOP );
        }
    } else if (autosafemode && newseen == 0) { // Auto-safe mode
        turnssincelastmon++;
        if (turnssincelastmon >= get_option<int>( "AUTOSAFEMODETURNS" ) && safe_mode == SAFE_MODE_OFF) {
            set_safe_mode( SAFE_MODE_ON );
        }
    }

    if (newseen == 0 && safe_mode == SAFE_MODE_STOP) {
        set_safe_mode( SAFE_MODE_ON );
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
    //for the alignment of the 1,2,3 rows on the right edge
    xcoords[2] -= utf8_width(_("East:")) - utf8_width(_("NE:"));
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
                default:
                    c = c_pink;
                    break;
                }
                sym = "@";
            } else {
                const mtype& mt = *unique_mons[i][j - typeshere_npc];
                c = mt.color;
                sym = mt.sym;
            }
            mvwprintz(w, pr.y, pr.x, c, "%s", sym.c_str());

            pr.x++;
        }
    } // for (int i = 0; i < 8; i++)

    // Now we print their full names!

    std::set<const mtype*> listed_mons;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr(0, 4 + startrow);

    int lastrowprinted = 2 + startrow;

    // Print monster names, starting with those at location 8 (nearby).
    for (int j = 8; j >= 0 && pr.y < maxheight; j--) {
        // Separate names by some number of spaces (more for local monsters).
        int namesep = (j == 8 ? 2 : 1);
        for( const mtype* type : unique_mons[j] ) {
            if( pr.y >= maxheight ) {
                // no space to print to anyway
                break;
            }
            if( listed_mons.count( type ) > 0 ) {
                // this type is already printed.
                continue;
            }
            listed_mons.insert( type );

            const mtype& mt = *type;
            const std::string name = mt.nname();

            // Move to the next row if necessary. (The +2 is for the "Z ").
            if (pr.x + 2 + utf8_width(name) >= width) {
                pr.y++;
                pr.x = 0;
            }

            if (pr.y < maxheight) { // Don't print if we've overflowed
                lastrowprinted = pr.y;
                mvwprintz(w, pr.y, pr.x, mt.color, "%s", mt.sym.c_str());
                pr.x += 2; // symbol and space
                nc_color danger = c_dkgray;
                if (mt.difficulty >= 30) {
                    danger = c_red;
                } else if (mt.difficulty >= 16) {
                    danger = c_ltred;
                } else if (mt.difficulty >= 8) {
                    danger = c_white;
                } else if (mt.agro > 0) {
                    danger = c_ltgray;
                }
                mvwprintz(w, pr.y, pr.x, danger, "%s", name.c_str());
                pr.x += utf8_width(name) + namesep;
            }
        }
    }

    return lastrowprinted;
}

void game::cleanup_dead()
{
    // Important: `Creature::die` must not be called after creature objects (NPCs, monsters) have
    // been removed, the dying creature could still have a pointer (the killer) to another creature.
    for( size_t i = 0; i < num_zombies(); i++ ) {
        monster &critter = critter_tracker->find(i);
        if( critter.is_dead() ) {
            dbg(D_INFO) << string_format("cleanup_dead: critter[%d] %d,%d dead:%c hp:%d %s",
                                         i, critter.posx(), critter.posy(), (critter.is_dead() ? '1' : '0'),
                                         critter.get_hp(), critter.name().c_str());
            critter.die( nullptr );
        }
    }

    for( auto &n : active_npc ) {
        if( n->is_dead() ) {
            n->die( nullptr ); // make sure this has been called to create corpses etc.
        }
    }

    // From here on, pointers to creatures get invalidated as dead creatures get removed.
    for( size_t i = 0; i < num_zombies(); ) {
        if( critter_tracker->find( i ).is_dead() ) {
            remove_zombie( i );
        } else {
            i++;
        }
    }
    for( auto it = active_npc.begin(); it != active_npc.end(); ) {
        if( (*it)->is_dead() ) {
            overmap_buffer.remove_npc( (*it)->getID() );
            it = active_npc.erase( it );
        } else {
            it++;
        }
    }
}

void game::monmove()
{
    cleanup_dead();

    // Make sure these don't match the first time around.
    tripoint cached_lev = m.get_abs_sub() + tripoint( 1, 0, 0 );

    mfactions monster_factions;
    const auto &playerfaction = mfaction_str_id( "player" );
    for (size_t i = 0; i < num_zombies(); i++) {
        // The first time through, and any time the map has been shifted,
        // recalculate monster factions.
        if( cached_lev != m.get_abs_sub() ) {
            // monster::plan() needs to know about all monsters on the same team as the monster.
            monster_factions.clear();
            for( int i = 0, numz = num_zombies(); i < numz; i++ ) {
                monster &critter = zombie( i );
                if( critter.friendly == 0 ) {
                    // Only 1 faction per mon at the moment.
                    monster_factions[ critter.faction ].insert( i );
                } else {
                    monster_factions[ playerfaction ].insert( i );
                }
            }
            cached_lev = m.get_abs_sub();
        }

        monster &critter = critter_tracker->find(i);
        while (!critter.is_dead() && !critter.can_move_to(critter.pos())) {
            // If we can't move to our current position, assign us to a new one
                dbg(D_ERROR) << "game:monmove: " << critter.name().c_str()
                             << " can't move to its location! (" << critter.posx()
                             << ":" << critter.posy() << ":" << critter.posz() << "), "
                             << m.tername(critter.posx(), critter.posy()).c_str();
                add_msg( m_debug, "%s can't move to its location! (%d,%d,%d), %s", critter.name().c_str(),
                         critter.posx(), critter.posy(), critter.posz(), m.tername(critter.pos()).c_str());
            bool okay = false;
            int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
            int startx = critter.posx() - 3 * xdir, endx = critter.posx() + 3 * xdir;
            int starty = critter.posy() - 3 * ydir, endy = critter.posy() + 3 * ydir;
            int z = critter.posz();
            for (int x = startx; x != endx && !okay; x += xdir) {
                for (int y = starty; y != endy && !okay; y += ydir) {
                    tripoint dest( x, y, z );
                    if (critter.can_move_to( dest ) && is_empty( dest )) {
                        critter.setpos( dest );
                        okay = true;
                    }
                }
            }
            if (!okay) {
                // die of "natural" cause (overpopulation is natural)
                critter.die( nullptr );
            }
        }

        if (!critter.is_dead()) {
            critter.process_turn();
        }

        m.creature_in_field( critter );

        while (critter.moves > 0 && !critter.is_dead()) {
            critter.made_footstep = false;
            // Controlled critters don't make their own plans
            if (!critter.has_effect( effect_controlled)) {
                // Formulate a path to follow
                critter.plan( monster_factions );
            }
            critter.move(); // Move one square, possibly hit u
            critter.process_triggers();
            m.creature_in_field( critter );
        }

        if (!critter.is_dead() &&
            u.has_active_bionic("bio_alarm") &&
            u.power_level >= 25 &&
            rl_dist( u.pos(), critter.pos() ) <= 5 &&
            !critter.is_hallucination()) {
                u.charge_power(-25);
                add_msg(m_warning, _("Your motion alarm goes off!"));
                cancel_activity_query(_("Your motion alarm goes off!"));
                if (u.in_sleep_state()) {
                    u.wake_up();
                }
        }
    }

    cleanup_dead();

    // The remaining monsters are all alive, but may be outside of the reality bubble.
    // If so, despawn them. This is not the same as dying, they will be stored for later and the
    // monster::die function is not called.
    for( size_t i = 0; i < num_zombies(); ) {
        monster &critter = critter_tracker->find( i );
        if( critter.posx() < 0 - ( SEEX * MAPSIZE ) / 6 ||
            critter.posy() < 0 - ( SEEY * MAPSIZE ) / 6 ||
            critter.posx() > ( SEEX * MAPSIZE * 7 ) / 6 ||
            critter.posy() > ( SEEY * MAPSIZE * 7 ) / 6 ) {
            despawn_monster( i );
        } else {
            i++;
        }
    }

    // Now, do active NPCs.
    for( auto np : active_npc ) {
        if( np->is_dead() ) {
            continue;
        }
        int turns = 0;
        m.creature_in_field( *np );
        np->process_turn();
        while( !np->is_dead() && !np->in_sleep_state() && np->moves > 0 && turns < 10 ) {
            int moves = np->moves;
            np->move();
            if( moves == np->moves ) {
                // Count every time we exit npc::move() without spending any moves.
                turns++;
            }

            // Turn on debug mode when in infinite loop
            // It has to be done before the last turn, otherwise
            // there will be no meaningful debug output.
            if( turns == 9 ) {
                debugmsg( "NPC %s entered infinite loop. Turning on debug mode",
                          np->name.c_str() );
                debug_mode = true;
            }
        }

        // If we spun too long trying to decide what to do (without spending moves),
        // Invoke cranial detonation to prevent an infinite loop.
        if( turns == 10 ) {
            add_msg( _( "%s's brain explodes!" ), np->name.c_str() );
            np->die( nullptr );
        }

        if( !np->is_dead() ) {
            np->process_active_items();
            np->update_body();
        }
    }
    cleanup_dead();
}

void game::flashbang( const tripoint &p, bool player_immune)
{
    draw_explosion( p, 8, c_white );
    int dist = rl_dist( u.pos(), p );
    if (dist <= 8 && !player_immune) {
        if (!u.has_bionic("bio_ears") && !u.is_wearing("rm13_armor_on")) {
            u.add_effect( effect_deaf, 40 - dist * 4);
        }
        if( m.sees( u.pos(), p, 8 ) ) {
            int flash_mod = 0;
            if( u.has_trait( "PER_SLIME" ) ) {
                if (one_in(2)) {
                    flash_mod = 3; // Yay, you weren't looking!
                }
            } else if( u.has_trait( "PER_SLIME_OK" ) ) {
                flash_mod = 8; // Just retract those and extrude fresh eyes
            } else if( u.has_bionic( "bio_sunglasses" ) || u.is_wearing( "rm13_armor_on" ) ) {
                flash_mod = 6;
            } else if( u.worn_with_flag( "BLIND" ) || u.worn_with_flag( "FLASH_PROTECTION" ) ) {
                flash_mod = 3; // Not really proper flash protection, but better than nothing
            }
            u.add_env_effect( effect_blind, bp_eyes, (12 - flash_mod - dist) / 2, 10 - dist );
        }
    }
    for( size_t i = 0; i < num_zombies(); i++ ) {
        monster &critter = critter_tracker->find(i);
        dist = rl_dist( critter.pos(), p );
        if( dist <= 8 ) {
            if( dist <= 4 ) {
                critter.add_effect( effect_stunned, 10 - dist);
            }
            if( critter.has_flag(MF_SEES) && m.sees( critter.pos(), p, 8 ) ) {
                critter.add_effect( effect_blind, 18 - dist);
            }
            if( critter.has_flag(MF_HEARS) ) {
                critter.add_effect( effect_deaf, 60 - dist * 4);
            }
        }
    }
    sounds::sound( p, 12, _("a huge boom!"));
    // TODO: Blind/deafen NPC
}

void game::shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult,
                      bool ignore_player )
{
    draw_explosion( p, radius, c_blue );

    sounds::sound( p, force * force * dam_mult / 2, _("Crack!") );
    for (size_t i = 0; i < num_zombies(); i++) {
        monster &critter = critter_tracker->find(i);
        if( rl_dist( critter.pos(), p ) <= radius ) {
            add_msg(_("%s is caught in the shockwave!"), critter.name().c_str());
            knockback( p, critter.pos(), force, stun, dam_mult);
        }
    }
    for( auto &elem : active_npc ) {
        if( rl_dist( ( elem )->pos(), p ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), ( elem )->name.c_str() );
            knockback( p, ( elem )->pos(), force, stun, dam_mult );
        }
    }
    if( rl_dist(u.pos(), p ) <= radius && !ignore_player &&
          (!u.has_trait( "LEG_TENT_BRACE" ) || u.footwear_factor() == 1 ||
          (u.footwear_factor() == .5 && one_in( 2 ) ) ) ) {
        add_msg( m_bad, _("You're caught in the shockwave!") );
        knockback( p, u.pos(), force, stun, dam_mult);
    }
    return;
}

/* Knockback target at t by force number of tiles in direction from s to t
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( const tripoint &s, const tripoint &t, int force, int stun, int dam_mult)
{
    std::vector<tripoint> traj;
    traj.clear();
    traj = line_to( s, t, 0, 0 );
    traj.insert( traj.begin(), s ); // how annoying, line_to() doesn't include the originating point!
    traj = continue_line( traj, force );
    traj.insert( traj.begin(), t ); // how annoying, continue_line() doesn't either!

    knockback( traj, force, stun, dam_mult );
    return;
}

/* Knockback target at traj.front() along line traj; traj should already have considered knockback distance.
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( std::vector<tripoint> &traj, int force, int stun, int dam_mult )
{
    (void)force; //FIXME: unused but header says it should do something
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    tripoint tp = traj.front();
    const int zid = mon_at( tp, true );
    if( zid == -1 && npc_at( tp ) == -1 && u.pos() != tp ) {
        debugmsg(_("Nothing at (%d,%d) to knockback!"), tp.x, tp.y, tp.z );
        return;
    }
    int force_remaining = 0;
    if (zid != -1) {
        monster *targ = &critter_tracker->find(zid);
        if (stun > 0) {
            targ->add_effect( effect_stunned, stun);
            add_msg(_("%s was stunned!"), targ->name().c_str());
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if (m.impassable(traj[i].x, traj[i].y)) {
                targ->setpos(traj[i - 1]);
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    targ->add_effect( effect_stunned, force_remaining);
                    add_msg(_("%s was stunned!"), targ->name().c_str());
                    add_msg(_("%s slammed into an obstacle!"), targ->name().c_str());
                    targ->apply_damage( nullptr, bp_torso, dam_mult * force_remaining );
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if (mon_at(traj[i]) != -1 || npc_at(traj[i]) != -1 ||
                       (u.pos() == traj[i])) {
                targ->setpos(traj[i - 1]);
                force_remaining = traj.size() - i;
                if (stun != 0) {
                     targ->add_effect( effect_stunned, force_remaining);
                     add_msg(_("%s was stunned!"), targ->name().c_str());
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front()) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name().c_str());
                } else if (npc_at(traj.front()) != -1) {
                    if (active_npc[npc_at(traj.front())]->male) {
                        add_msg(_("%s collided with someone else and sent him flying!"),
                                targ->name().c_str());
                    } else {
                        add_msg(_("%s collided with someone else and sent her flying!"),
                                targ->name().c_str());
                    }
                } else if (u.pos() == traj.front()) {
                    add_msg(m_bad, _("%s collided with you and sent you flying!"), targ->name().c_str());
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            targ->setpos(traj[i]);
            if (m.has_flag("LIQUID", targ->pos()) && !targ->can_drown() && !targ->is_dead()) {
                targ->die( nullptr );
                if (u.sees(*targ)) {
                    add_msg(_("The %s drowns!"), targ->name().c_str());
                }
            }
            if (!m.has_flag("LIQUID", targ->pos()) && targ->has_flag(MF_AQUATIC) &&
                !targ->is_dead()) {
                targ->die( nullptr );
                if (u.sees(*targ)) {
                    add_msg(_("The %s flops around and dies!"), targ->name().c_str());
                }
            }
        }
    } else if( npc_at( tp ) != -1 ) {
        npc *targ = active_npc[npc_at( tp )];
        if (stun > 0) {
            targ->add_effect( effect_stunned, stun);
            add_msg(_("%s was stunned!"), targ->name.c_str());
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if (m.impassable(traj[i].x, traj[i].y)) { // oops, we hit a wall!
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    targ->add_effect( effect_stunned, force_remaining);
                    if (targ->has_effect( effect_stunned))
                        add_msg(_("%s was stunned!"), targ->name.c_str());

                    body_part bps[] = {
                        bp_head,
                        bp_arm_l, bp_arm_r,
                        bp_hand_l, bp_hand_r,
                        bp_torso,
                        bp_leg_l, bp_leg_r
                    };
                    for (auto &bp : bps) {
                        if (one_in(2)) {
                            targ->deal_damage( nullptr, bp, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                        }
                    }
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( mon_at(traj[i]) != -1 ||
                       npc_at(traj[i]) != -1 ||
                       u.pos() == traj[i] ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    add_msg(_("%s was stunned!"), targ->name.c_str());
                    targ->add_effect( effect_stunned, force_remaining);
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front()) != -1) {
                    add_msg(_("%s collided with something else and sent it flying!"),
                            targ->name.c_str());
                } else if (npc_at(traj.front()) != -1) {
                    if (active_npc[npc_at(traj.front())]->male) {
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
            targ->setpos( traj[i] );
        }
    } else if( u.pos() == tp ) {
        if (stun > 0) {
            u.add_effect( effect_stunned, stun);
            add_msg(m_bad, ngettext("You were stunned for %d turn!",
                                    "You were stunned for %d turns!",
                                    stun),
                    stun);
        }
        for (size_t i = 1; i < traj.size(); i++) {
            if( m.impassable( traj[i] ) ) { // oops, we hit a wall!
                u.setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (u.has_effect( effect_stunned)) {
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
                    u.add_effect( effect_stunned, force_remaining);
                    body_part bps[] = {
                        bp_head,
                        bp_arm_l, bp_arm_r,
                        bp_hand_l, bp_hand_r,
                        bp_torso,
                        bp_leg_l, bp_leg_r
                    };
                    for (auto &bp : bps) {
                        if (one_in(2)) {
                            u.deal_damage( nullptr, bp, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                        }
                    }
                    u.check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( mon_at( traj[i] ) != -1 || npc_at( traj[i] ) != -1 ) {
                u.setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if (stun != 0) {
                    if (u.has_effect( effect_stunned)) {
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
                    u.add_effect( effect_stunned, force_remaining);
                }
                traj.erase(traj.begin(), traj.begin() + i);
                if (mon_at(traj.front()) != -1) {
                    add_msg(_("You collided with something and sent it flying!"));
                } else if (npc_at(traj.front()) != -1) {
                    if (active_npc[npc_at(traj.front())]->male) {
                        add_msg(_("You collided with someone and sent him flying!"));
                    } else {
                        add_msg(_("You collided with someone and sent her flying!"));
                    }
                }
                knockback(traj, force_remaining, stun, dam_mult);
                break;
            }
            if (m.has_flag("LIQUID", u.pos()) && force_remaining < 1) {
                plswim(u.pos());
            } else {
                u.setpos( traj[i] );
            }
        }
    }
    return;
}

void game::use_computer( const tripoint &p )
{
    if (u.has_trait("ILLITERATE")) {
        add_msg(m_info, _("You can not read a computer screen!"));
        return;
    }
    if( u.is_blind() ) {
        // we don't have screen readers in game
        add_msg( m_info, _( "You can not see a computer screen!" ) );
        return;
    }
    if (u.has_trait("HYPEROPIC") && !u.is_wearing("glasses_reading")
        && !u.is_wearing("glasses_bifocal") && !u.has_effect( effect_contacts)) {
        add_msg(m_info, _("You'll need to put on reading glasses before you can see the screen."));
        return;
    }

    computer *used = m.computer_at( p );

    if (used == NULL) {
        dbg(D_ERROR) << "game:use_computer: Tried to use computer at (" <<
            p.x << ", " << p.y << ", " << p.z << ") - none there";
        debugmsg( "Tried to use computer at (%d, %d, %d) - none there", p.x, p.y, p.z );
        return;
    }

    used->use();

    refresh_all();
}

void game::resonance_cascade( const tripoint &p )
{
    int maxglow = 100 - 5 * trig_dist( p, u.pos() );
    int minglow = 60 - 5 * trig_dist( p, u.pos() );
    MonsterGroupResult spawn_details;
    monster invader;
    if (minglow < 0) {
        minglow = 0;
    }
    if (maxglow > 0) {
        u.add_effect( effect_teleglow, rng(minglow, maxglow) * 100);
    }
    int startx = (p.x < 8 ? 0 : p.x - 8), endx = (p.x + 8 >= SEEX * 3 ? SEEX * 3 - 1 : p.x + 8);
    int starty = (p.y < 8 ? 0 : p.y - 8), endy = (p.y + 8 >= SEEY * 3 ? SEEY * 3 - 1 : p.y + 8);
    tripoint dest( startx, starty, p.z );
    for( int &i = dest.x; i <= endx; i++ ) {
        for( int &j = dest.y; j <= endy; j++ ) {
            switch (rng(1, 80)) {
            case 1:
            case 2:
                emp_blast( dest );
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
                            m.add_field( {k, l, p.z}, type, 3, 0 );
                        }
                    }
                }
                break;
            case  6:
            case  7:
            case  8:
            case  9:
            case 10:
                m.add_trap( dest, tr_portal );
                break;
            case 11:
            case 12:
                m.add_trap( dest, tr_goo );
                break;
            case 13:
            case 14:
            case 15:
                spawn_details = MonsterGroupManager::GetResultFromGroup( mongroup_id( "GROUP_NETHER" ) );
                invader = monster( spawn_details.name, dest );
                add_zombie(invader);
                break;
            case 16:
            case 17:
            case 18:
                m.destroy( dest );
                break;
            case 19:
                explosion( dest, rng(1, 10), rng(0, 1) * rng(0, 6), one_in(4) );
                break;
            }
        }
    }
}

void game::scrambler_blast( const tripoint &p )
{
    int mondex = mon_at( p );
    if (mondex != -1) {
        monster &critter = critter_tracker->find(mondex);
        if (critter.has_flag(MF_ELECTRONIC)) {
            critter.make_friendly();
        }
        add_msg(m_warning, _("The %s sparks and begins searching for a target!"), critter.name().c_str());
    }
}

void game::emp_blast( const tripoint &p )
{
    // TODO: Implement z part
    int x = p.x;
    int y = p.y;
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
    int mondex = mon_at(p);
    if (mondex != -1) {
        monster &critter = critter_tracker->find(mondex);
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
                critter.check_dead_state();
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
            u.charge_power(-rng(1 + max_drain / 3, max_drain));
        }
        // TODO: More effects?
        //e-handcuffs effects
        if (u.weapon.typeId() == "e_handcuffs" && u.weapon.charges > 0){
            u.weapon.item_tags.erase("NO_UNWIELD");
            u.weapon.charges = 0;
            u.weapon.active = false;
            add_msg(m_good, _("The %s on your wrists spark briefly, then release your hands!"), u.weapon.tname().c_str());
        }
    }
    // Drain any items of their battery charge
    for( auto it = m.i_at( x, y ).begin(); it != m.i_at( x, y ).end(); ++it ) {
        if( it->is_tool() && it->ammo_type() == ammotype( "battery" ) ) {
            it->charges = 0;
        }
    }
    // TODO: Drain NPC energy reserves
}

int game::npc_at( const tripoint &p ) const
{
    for( size_t i = 0; i < active_npc.size(); i++ ) {
        if( active_npc[i]->pos() == p && !active_npc[i]->is_dead() ) {
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

Creature *game::critter_at( const tripoint &p, bool allow_hallucination )
{
    const int mindex = mon_at( p, allow_hallucination );
    if( mindex != -1 ) {
        return &zombie( mindex );
    }
    if( p == u.pos() ) {
        return &u;
    }
    const int nindex = npc_at( p );
    if( nindex != -1 ) {
        return active_npc[nindex];
    }
    return nullptr;
}

Creature const* game::critter_at( const tripoint &p, bool allow_hallucination ) const
{
    return const_cast<game*>(this)->critter_at( p, allow_hallucination );
}

bool game::summon_mon( const mtype_id& id, const tripoint &p )
{
    monster mon( id );
    mon.spawn( p );
    return add_zombie(mon, true);
}

// By default don't pin upgrades to current day
bool game::add_zombie(monster &critter)
{
    return add_zombie(critter, false);
}

bool game::add_zombie(monster &critter, bool pin_upgrade)
{
    if( !m.inbounds( critter.pos() ) ) {
        dbg( D_ERROR ) << "added a critter with out-of-bounds position: "
                       << critter.posx() << "," << critter.posy() << ","  << critter.posz()
                       << " - " << critter.disp_name();
    }

    critter.try_upgrade(pin_upgrade);
    if( !pin_upgrade ) {
        critter.on_load();
    }

    critter.last_updated = calendar::turn;
    return critter_tracker->add(critter);
}

size_t game::num_zombies() const
{
    return critter_tracker->size();
}

monster &game::zombie(const int idx)
{
    return critter_tracker->find(idx);
}

bool game::update_zombie_pos( const monster &critter, const tripoint &pos )
{
    return critter_tracker->update_pos( critter, pos );
}

void game::remove_zombie(const int idx)
{
    if( last_target == idx && !last_target_was_npc ) {
        last_target = -1;
    } else if( last_target > idx && !last_target_was_npc ) {
        last_target--;
    }
    critter_tracker->remove(idx);
}

void game::clear_zombies()
{
    critter_tracker->clear();
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
    phantasm.spawn({u.posx() + static_cast<int>(rng(-10, 10)), u.posy() + static_cast<int>(rng(-10, 10)), u.posz()});

    //Don't attempt to place phantasms inside of other monsters
    if( mon_at( phantasm.pos(), true ) == -1 ) {
        return critter_tracker->add(phantasm);
    } else {
        return false;
    }
}

int game::mon_at( const tripoint &p, bool allow_hallucination ) const
{
    const int mon_index = critter_tracker->mon_at( p );
    if( mon_index == -1 ||
        allow_hallucination || !critter_tracker->find( mon_index ).is_hallucination() ) {
        return mon_index;
    }

    return -1;
}

monster *game::monster_at( const tripoint &p, bool allow_hallucination )
{
    return &zombie( mon_at( p, allow_hallucination ) );
}

void game::rebuild_mon_at_cache()
{
    critter_tracker->rebuild_cache();
}

bool game::swap_critters( Creature &a, Creature &b )
{
    if( &a == &b ) {
        // No need to do anything, but print a debugmsg anyway
        debugmsg( "Tried to swap %s with itself", a.disp_name().c_str() );
        return true;
    }

    // Simplify by "sorting" the arguments
    // Only the first argument can be u
    // If swapping player/npc with a monster, monster is second
    bool a_first = a.is_player() ||
                   ( a.is_npc() && !b.is_player() );
    Creature &first =  a_first ? a : b;
    Creature &second = a_first ? b : a;
    // Possible options:
    // both first and second are monsters
    // second is a monster, first is a player or an npc
    // first is a player, second is an npc
    // both first and second are npcs
    if( first.is_monster() ) {
        monster *m1 = dynamic_cast< monster* >( &first );
        monster *m2 = dynamic_cast< monster* >( &second );
        if( m1 == nullptr || m2 == nullptr || m1 == m2 ) {
            debugmsg( "Couldn't swap two monsters" );
            return false;
        }

        critter_tracker->swap_positions( *m1, *m2 );
        return true;
    }

    player *u_or_npc = dynamic_cast< player* >( &first );
    player *other_npc = dynamic_cast< player* >( &second );

    if( u_or_npc->in_vehicle ) {
        g->m.unboard_vehicle( u_or_npc->pos() );
    }

    if( other_npc->in_vehicle ) {
        g->m.unboard_vehicle( other_npc->pos() );
    }

    tripoint temp = second.pos();
    second.setpos( first.pos() );
    first.setpos( temp );

    int part = -1;
    vehicle *veh = g->m.veh_at( u_or_npc->pos(), part );
    if( veh != nullptr && veh->part_with_feature( part, VPFLAG_BOARDABLE ) >= 0 ) {
        g->m.board_vehicle( u_or_npc->pos(), u_or_npc );
    }

    vehicle *oveh = g->m.veh_at( other_npc->pos(), part );
    if( oveh != nullptr && oveh->part_with_feature( part, VPFLAG_BOARDABLE ) >= 0 ) {
        g->m.board_vehicle( other_npc->pos(), other_npc );
    }

    if( first.is_player() ) {
        update_map( *u_or_npc );
    }

    return true;
}

bool game::is_empty( const tripoint &p )
{
    return ( m.passable( p ) || m.has_flag( "LIQUID", p ) ) &&
               critter_at( p ) == nullptr;
}

bool game::is_in_sunlight( const tripoint &p )
{
    return (m.is_outside( p ) && light_level( p.z ) >= 40 &&
            (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}

bool game::is_sheltered( const tripoint &p )
{
    int vpart = -1;
    vehicle *veh = m.veh_at( p, vpart );

    return ( !m.is_outside( p ) ||
             p.z < 0 ||
             ( veh && veh->is_inside(vpart) ) );
}

bool game::revive_corpse( const tripoint &p, item &it )
{
    if (!it.is_corpse()) {
        debugmsg("Tried to revive a non-corpse.");
        return false;
    }
    if( critter_at( p ) != nullptr ) {
        // Someone is in the way, try again later
        return false;
    }
    monster critter( it.get_mtype()->id, p );
    critter.init_from_item( it );
    if( critter.get_hp() < 1 ) {
        // Failed reanimation due to corpse being too burned
        it.active = false;
        return false;
    }

    critter.no_extra_death_drops = true;
    critter.add_effect( effect_downed, 5, num_bp, true );

    if (it.get_var( "zlave" ) == "zlave" ) {
        critter.add_effect( effect_pacified, 1, num_bp, true );
        critter.add_effect( effect_pet, 1, num_bp, true );
    }

    if (it.get_var("no_ammo") == "no_ammo") {
        for (auto &ammo : critter.ammo) {
            ammo.second = 0;
        }
    }

    const bool ret = add_zombie(critter);
    if( !ret ) {
        debugmsg( "Couldn't add a revived monster" );
    }
    if( ret && mon_at( p ) == -1 ) {
        debugmsg( "Revived monster is not where it's supposed to be. Prepare for crash." );
    }
    return ret;
}

void game::open()
{
    tripoint openp;
    if (!choose_adjacent_highlight(_("Open where?"), openp, ACTION_OPEN)) {
        return;
    }

    u.moves -= 100;

    int vpart;
    vehicle *veh = m.veh_at(openp, vpart);

    if( veh != nullptr ) {
        int openable = veh->next_part_to_open(vpart);
        if (openable >= 0) {
            const vehicle *player_veh = m.veh_at(u.pos());
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
                    const std::string name = veh->part_info( openable ).name();
                    add_msg(m_info, _("That %s can only opened from the inside."), name.c_str());
                    u.moves += 100;
                } else {
                    veh->open_all_at(openable);
                }
            }
        } else {
            // If there are any OPENABLE parts here, they must be already open
            int already_open = veh->part_with_feature(vpart, "OPENABLE");
            if (already_open >= 0) {
                const std::string name = veh->part_info( already_open ).name();
                add_msg(m_info, _("That %s is already open."), name.c_str());
            }
            u.moves += 100;
        }
        return;
    }

    bool didit = m.open_door( openp, !m.is_outside( u.pos() ) );

    if (!didit) {
        const ter_str_id tid = m.ter( openp ).id();

        if( tid.str().find("t_door") != std::string::npos ) {
            if( tid.str().find("_locked") != std::string::npos ) {
                add_msg(m_info, _("The door is locked!"));
                return;
            } else if ( tid.obj().close ) {
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

void game::close()
{
    tripoint closep;
    if( choose_adjacent_highlight( _("Close where?"), closep, ACTION_CLOSE ) ) {
        close( closep );
    }
}

void game::close( const tripoint &closep )
{
    using namespace units::literals;
    bool didit = false;
    const bool inside = !m.is_outside( u.pos() );

    auto items_in_way = m.i_at(closep);
    int vpart;
    vehicle *veh = m.veh_at(closep, vpart);
    int zid = mon_at(closep);
    if (zid != -1) {
        monster &critter = critter_tracker->find(zid);
        add_msg(m_info, _("There's a %s in the way!"), critter.name().c_str());
    } else if (veh) {
        int openable = veh->next_part_to_close(vpart);
        if (openable >= 0) {
            if( closep == u.pos() ) {
                add_msg(m_info, _("There's some buffoon in the way!"));
                return;
            }
            const std::string name = veh->part_info(openable).name();
            if (veh->part_info(openable).has_flag("OPENCLOSE_INSIDE")) {
                const vehicle *in_veh = m.veh_at(u.pos());
                if (!in_veh || in_veh != veh) {
                    add_msg(m_info, _("That %s can only closed from the inside."), name.c_str());
                    return;
                }
            }
            if (veh->parts[openable].open) {
                veh->close(openable);
                didit = true;
            } else {
                add_msg(m_info, _("That %s is already closed."), name.c_str());
            }
        }
    } else if( closep == u.pos() ) {
        add_msg(m_info, _("There's some buffoon in the way!"));
    } else if( m.has_furn( closep ) && !m.furn( closep ).obj().close ) {
        // check for open crate
        if( m.furn( closep ) == furn_str_id( "f_crate_o" ) ) {
            add_msg(m_info, _("You'll need to construct a seal to close the crate!"));
        } else {
            add_msg(m_info, _("There's a %s in the way!"), m.furnname(closep).c_str());
        }
    } else if (!m.close_door( closep, inside, true )) {
        // ^^ That checks if the PC could close something there, it
        // does not actually do anything.
        std::string door_name;
        if (m.has_furn(closep)) {
            door_name = m.furn(closep).obj().name;
        } else {
            door_name = m.tername( closep );
        }
        // Print a message that we either can not close whatever is there
        // or (if we're outside) that we can only close it from the
        // inside.
        if (!inside && m.close_door( closep, true, true )) {
            add_msg(m_info, _("You cannot close the %s from outside. You must be inside the building."),
                    door_name.c_str());
        } else {
            add_msg(m_info, _("You cannot close the %s."), door_name.c_str());
        }
    } else {
        // Scoot up to 10 volume of items out of the way, only counting items that are vol >= 1.
        if (m.furn(closep) != f_safe_o && !items_in_way.empty()) {
            units::volume total_item_volume = 0;
            if (items_in_way.size() > 10) {
                add_msg(m_info, _("Too many items to push out of the way!"));
                return;
            }
            for( auto &elem : items_in_way ) {
                // Don't even count tiny items.
                if( elem.volume() < 250_ml ) {
                    continue;
                }
                if( elem.volume() > 2500_ml ) {
                    add_msg( m_info, _( "There's a %s in the way that is too big to just nudge out "
                                        "of the way." ),
                             elem.tname().c_str() );
                    return;
                }
                total_item_volume += elem.volume();
                if (total_item_volume > 2500_ml ) {
                    add_msg(m_info, _("There is too much stuff in the way."));
                    return;
                }
            }
            add_msg(_("You push %s out of the way."), items_in_way.size() == 1 ?
                    items_in_way[0].tname().c_str() : _("some stuff"));
            u.moves -= items_in_way.size() * 10;
        }

        didit = m.close_door( closep, inside, false );
        if (didit && m.has_flag_ter_or_furn("NOITEM", closep)) {
            // Just plopping items back on their origin square will displace them to adjacent squares
            // since the door is closed now.
            for( auto &elem : items_in_way ) {
                m.add_item_or_charges( closep, elem );
            }
            m.i_clear(closep);
        }
    }

    if (didit) {
        u.moves -= 90;
    }
}

void game::smash()
{
    const int move_cost = !u.is_armed() ? 80 : u.weapon.attack_time() * 0.8;
    bool didit = false;
    ///\EFFECT_STR increases smashing capability
    int smashskill = u.str_cur + u.weapon.damage_melee( DT_BASH );
    tripoint smashp;

    const bool allow_floor_bash = debug_mode; // Should later become "true"
    if( !choose_adjacent(_("Smash where?"), smashp, allow_floor_bash ) ) {
        return;
    }

    bool smash_floor = false;
    if( smashp.z != u.posz() ) {
        if( smashp.z > u.posz() ) {
            // TODO: Knock on the ceiling
            return;
        }

        smashp.z = u.posz();
        smash_floor = true;
    }

    if( m.get_field( smashp, fd_web ) != nullptr ) {
        m.remove_field( smashp, fd_web );
        sounds::sound( smashp, 2, "" );
        add_msg( m_info, _( "You brush aside some webs." ) );
        u.moves -= 100;
        return;
    }

    for( const auto &maybe_corpse : m.i_at( smashp ) ) {
        if ( maybe_corpse.is_corpse() && maybe_corpse.damage() < maybe_corpse.max_damage() &&
             maybe_corpse.get_mtype()->has_flag( MF_REVIVES ) ) {
            // do activity forever. ACT_PULP stops itself
            u.assign_activity( ACT_PULP, INT_MAX, 0 );
            u.activity.placement = smashp;
            return; // don't smash terrain if we've smashed a corpse
        }
    }

    didit = m.bash( smashp, smashskill, false, false, smash_floor ).did_bash;
    if( didit ) {
        u.handle_melee_wear();
        u.moves -= move_cost;
        const int mod_sta = ( (u.weapon.weight() / 100 ) + 20) * -1;
        u.mod_stat("stamina", mod_sta);

        if (u.get_skill_level( skill_melee ) == 0) {
            u.practice( skill_melee, rng(0, 1) * rng(0, 1));
        }
        const int vol = u.weapon.volume() / units::legacy_volume_factor;
        if (u.weapon.made_of( material_id( "glass" ) ) &&
            rng(0, vol + 3) < vol) {
            add_msg(m_bad, _("Your %s shatters!"), u.weapon.tname().c_str());
            for( auto &elem : u.weapon.contents ) {
                m.add_item_or_charges( u.pos(), elem );
            }
            sounds::sound(u.pos(), 24, "");
            u.deal_damage( nullptr, bp_hand_r, damage_instance( DT_CUT, rng( 0, vol ) ) );
            if (vol > 20) {
                // Hurt left arm too, if it was big
                u.deal_damage( nullptr, bp_hand_l, damage_instance( DT_CUT, rng( 0, long( vol * .5 ) ) ) );
            }
            u.remove_weapon();
            u.check_dead_state();
        }
        if (smashskill < m.bash_resistance(smashp) && one_in(10)) {
            if (m.has_furn(smashp) && m.furn(smashp).obj().bash.str_min != -1) {
                // %s is the smashed furniture
                add_msg(m_neutral, _("You don't seem to be damaging the %s."), m.furnname(smashp).c_str());
            } else {
                // %s is the smashed terrain
                add_msg(m_neutral, _("You don't seem to be damaging the %s."), m.tername(smashp).c_str());
            }
        }
    } else {
        add_msg(_("There's nothing there to smash!"));
    }
}

void game::use_item(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_for_activatables( u, _( "Use item:" ) );
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

void game::handbrake()
{
    vehicle *veh = m.veh_at(u.pos());
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

void game::exam_vehicle( vehicle &veh, int cx, int cy )
{
    auto act = veh_interact::run( veh, cx, cy );
    if( act ) {
        u.moves = 0;
        u.assign_activity( act );
    }
}

bool game::forced_door_closing( const tripoint &p, const ter_id door_type, int bash_dmg )
{
    // TODO: Z
    const int &x = p.x;
    const int &y = p.y;
    const std::string &door_name = door_type.obj().name;
    int kbx = x; // Used when player/monsters are knocked back
    int kby = y; // and when moving items out of the way
    for (int i = 0; i < 20; i++) {
        const int x_ = x + rng(-1, +1);
        const int y_ = y + rng(-1, +1);
        if (is_empty({x_, y_, get_levz()})) {
            // invert direction, as game::knockback needs
            // the source of the force that knocks back
            kbx = -x_ + x + x;
            kby = -y_ + y + y;
            break;
        }
    }
    const tripoint kbp( kbx, kby, p.z );
    const bool can_see = u.sees(x, y);
    player *npc_or_player = NULL;
    if (x == u.pos().x && y == u.pos().y) {
        npc_or_player = &u;
    } else {
        const int cindex = npc_at(p);
        if (cindex != -1) {
            npc_or_player = active_npc[cindex];
        }
    }
    if (npc_or_player != NULL) {
        if (bash_dmg <= 0) {
            return false;
        }
        if (npc_or_player->is_npc() && can_see) {
            add_msg(_("The %1$s hits the %2$s."), door_name.c_str(), npc_or_player->name.c_str());
        } else if (npc_or_player->is_player()) {
            add_msg(m_bad, _("The %s hits you."), door_name.c_str());
        }
        if( npc_or_player->activity.type != ACT_NULL ) {
            npc_or_player->cancel_activity();
        }
        // TODO: make the npc angry?
        npc_or_player->hitall(bash_dmg, 0, nullptr);
        knockback( kbp, p, std::max(1, bash_dmg / 10), -1, 1);
        // TODO: perhaps damage/destroy the gate
        // if the npc was really big?
    }
    const int cindex = mon_at(p);
    if (cindex != -1) {
        if (bash_dmg <= 0) {
            return false;
        }
        if (can_see) {
            add_msg(_("The %1$s hits the %2$s."), door_name.c_str(), zombie(cindex).name().c_str());
        }
        monster &critter = zombie( cindex );
        if( critter.type->size <= MS_SMALL ) {
            critter.die_in_explosion( nullptr );
        } else {
            critter.apply_damage( nullptr, bp_torso, bash_dmg );
            critter.check_dead_state();
        }
        if( !critter.is_dead() && critter.type->size >= MS_HUGE ) {
            // big critters simply prevent the gate from closing
            // TODO: perhaps damage/destroy the gate
            // if the critter was really big?
            return false;
        }
        if( !critter.is_dead() ) {
            // Still alive? Move the critter away so the door can close
            knockback( kbp, p, std::max(1, bash_dmg / 10), -1, 1);
            if (mon_at(p) != -1) {
                return false;
            }
        }
    }
    int vpart = -1;
    vehicle *veh = m.veh_at( p, vpart );
    if( veh != nullptr ) {
        if (bash_dmg <= 0) {
            return false;
        }
        veh->damage(vpart, bash_dmg);
        if( m.veh_at( p, vpart) != nullptr ) {
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
            } else if( elem.volume() < units::from_milliliter( 250 ) ) {
                // Dito for small items, will be moved away
                continue;
            }
            // Everything else prevents the door from closing
            return false;
        }
    }

    m.ter_set( x, y, door_type );
    if (m.has_flag("NOITEM", x, y)) {
        auto items = m.i_at(x, y);
        while (!items.empty()) {
            if (items[0].made_of(LIQUID)) {
                m.i_rem( x, y, 0 );
                continue;
            }
            if (items[0].made_of( material_id( "glass" ) ) && one_in(2)) {
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

void game::open_gate( const tripoint &p )
{
    gates::open_gate( p, u );
}

void game::moving_vehicle_dismount( const tripoint &dest_loc )
{
    int vpart;
    vehicle *veh = m.veh_at(u.pos(), vpart);
    if( veh == nullptr ) {
        debugmsg("Tried to exit non-existent vehicle.");
        return;
    }
    if( u.pos() == dest_loc ) {
        debugmsg("Need somewhere to dismount towards.");
        return;
    }
    tileray ray( dest_loc.x - u.posx(), dest_loc.y - u.posy() );
    const int d = ray.dir(); // TODO:: make dir() const correct!
    add_msg(_("You dive from the %s."), veh->name.c_str());
    m.unboard_vehicle(u.pos());
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_creature( &u, d, 30, true );
    // Hit the ground according to vehicle speed
    if (!m.has_flag("SWIMMABLE", u.pos())) {
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
        veh = m.veh_at(u.pos(), veh_part);
    }

    if( veh != nullptr && veh->player_in_control( u ) ) {
        veh->use_controls( u.pos() );
    } else if( veh && veh->part_with_feature( veh_part, "CONTROLS" ) >= 0 && u.in_vehicle ) {
        if( !veh->interact_vehicle_locked() ) {
            return;
        }
        if( veh->engine_on ) {
            u.controlling_vehicle = true;
            add_msg( _("You take control of the %s."), veh->name.c_str() );
        } else {
            veh->start_engines( true );
        }
    } else {
        tripoint examp;
        if (!choose_adjacent(_("Control vehicle where?"), examp)) {
            return;
        }
        veh = m.veh_at(examp, veh_part);
        if (!veh) {
            add_msg(_("No vehicle there."));
            return;
        }
        if (veh->interact_vehicle_locked()){
            veh->use_controls( examp );
        }
    }
}

bool pet_menu(monster *z)
{
    enum choices {
        cancel,
        swap_pos,
        push_zlave,
        rename,
        attach_bag,
        drop_all,
        give_items,
        pheromone,
        rope
    };

    uimenu amenu;
    amenu.return_invalid = true;
    std::string pet_name = _("dog");
    if( z->type->in_species( ZOMBIE ) ) {
        pet_name = _("zombie slave");
    }

    amenu.selected = 0;
    amenu.text = string_format(_("What to do with your %s?"), pet_name.c_str());
    amenu.addentry(cancel, true, 'q', _("Cancel"));

    amenu.addentry(swap_pos, true, 's', _("Swap positions"));
    amenu.addentry(push_zlave, true, 'p', _("Push %s"), pet_name.c_str());
    amenu.addentry( rename, true, 'e', _("Rename") );

    if (z->has_effect( effect_has_bag)) {
        amenu.addentry(give_items, true, 'g', _("Place items into bag"));
        amenu.addentry(drop_all, true, 'd', _("Drop all items"));
    } else {
        amenu.addentry(attach_bag, true, 'b', _("Attach bag"));
    }

    if (z->has_effect( effect_tied)) {
        amenu.addentry(rope, true, 'r', _("Untie"));
    } else {
        if (g->u.has_amount("rope_6", 1)) {
            amenu.addentry(rope, true, 'r', _("Tie"));
        } else {
            amenu.addentry(rope, false, 'r', _("You need a short rope"));
        }
    }

    if( z->type->in_species( ZOMBIE ) ) {
        amenu.addentry(pheromone, true, 't', _("Tear out pheromone ball"));
    }

    amenu.query();
    int choice = amenu.ret;

    if (cancel == choice || choice < 0) {
        return false;
    }

    if (swap_pos == choice) {
        g->u.moves -= 150;

        ///\EFFECT_STR increases chance to successfully swap positions with your pet

        ///\EFFECT_DEX increases chance to successfully swap positions with your pet
        if (!one_in((g->u.str_cur + g->u.dex_cur) / 6)) {

            bool t = z->has_effect( effect_tied);
            if (t) {
                z->remove_effect( effect_tied);
            }

            tripoint zp = z->pos();
            z->move_to( g->u.pos(), true);
            g->u.setpos( zp );

            if (t) {
                z->add_effect( effect_tied, 1, num_bp, true);
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

        ///\EFFECT_STR increases chance to successfuly push your pet
        if (!one_in(g->u.str_cur)) {
            add_msg(_("You pushed the %s."), pet_name.c_str());
        } else {
            add_msg(_("You pushed the %s, but it resisted."), pet_name.c_str());
            return true;
        }

        int deltax = z->posx() - g->u.posx(), deltay = z->posy() - g->u.posy();

        z->move_to( tripoint( z->posx() + deltax, z->posy() + deltay, z->posz() ) );

        return true;
    }

    if ( rename == choice ) {
        std::string unique_name = string_input_popup( _("Enter new pet name:"), 20 );
        if( unique_name.length() > 0 ) {
            z->unique_name = unique_name;
        }
        return true;
    }

    if (attach_bag == choice) {
        int pos = g->inv_for_filter( _("Bag item:"), []( const item &it ) {
            return it.is_armor() && it.get_storage() > 0;
        } );

        if (pos == INT_MIN) {
            add_msg(_("Never mind."));
            return true;
        }

        item *it = &g->u.i_at(pos);

        z->add_item(*it);

        add_msg(_("You mount the %1$s on your %2$s, ready to store gear."),
                it->display_name().c_str(),  pet_name.c_str());

        g->u.i_rem(pos);

        z->add_effect( effect_has_bag, 1, num_bp, true);

        g->u.moves -= 200;

        return true;
    }

    if (drop_all == choice) {
        for (std::vector<item>::iterator it = z->inv.begin();
             it != z->inv.end(); ++it) {
            g->m.add_item_or_charges(z->pos(), *it);
        }

        z->inv.clear();

        z->remove_effect( effect_has_bag);

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

        units::volume max_cap = it->get_storage();
        int max_weight = z->weight_capacity() - it->weight();

        if (z->inv.size() > 1) {
            for (auto &i : z->inv) {
                max_cap -= i.volume();
                max_weight -= i.weight();
            }
        }

        if (max_weight <= 0) {
            add_msg(_("%1$s is overburdened. You can't transfer your %2$s"),
                    pet_name.c_str(), it->tname(1).c_str());
            return true;
        }
        if (max_cap <= 0) {
            add_msg(_("There's no room in your %1$s's %2$s for that, it's too bulky!"),
                    pet_name.c_str(), it->tname(1).c_str() );
            return true;
        }

        const auto items_to_stash = g->multidrop();
        if( !items_to_stash.empty() ) {
            g->u.drop( items_to_stash, z->pos(), true );
            z->add_effect( effect_controlled, 5);
            return true;
        }

        return false;
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
        if (z->has_effect( effect_tied)) {
            z->remove_effect( effect_tied);
            item rope_6("rope_6", 0);
            g->u.i_add(rope_6);
        } else {
            z->add_effect( effect_tied, 1, num_bp, true);
            g->u.use_amount( "rope_6", 1 );
        }

        return true;
    }

    return true;
}

bool game::npc_menu( npc &who )
{
    enum choices : int {
        cancel = 0,
        swap_pos,
        push,
        examine_wounds,
        use_item,
        sort_armor,
        attack
    };

    uimenu amenu;
    amenu.return_invalid = true;

    amenu.selected = 0;
    amenu.text = string_format( _("What to do with %s?"), who.disp_name().c_str() );
    amenu.addentry( cancel, true, 'q', _("Cancel") );

    const bool obeys = debug_mode || (who.is_friend() && !who.in_sleep_state());
    amenu.addentry( swap_pos, obeys, 's', _("Swap positions") );
    amenu.addentry( push, obeys, 'p', _("Push away") );
    amenu.addentry( examine_wounds, true, 'w', _("Examine wounds") );
    amenu.addentry( use_item, true, 'i', _("Use item on") );
    amenu.addentry( sort_armor, true, 'r', _("Sort armor") );
    amenu.addentry( attack, true, 'a', _("Attack") );

    amenu.query();

    const int choice = amenu.ret;
    if( choice == cancel ) {
        return false;
    }

    if( choice == swap_pos ) {
        if( !prompt_dangerous_tile( who.pos() ) ) {
            return true;
        }
        // TODO: Make NPCs protest when displaced onto dangerous crap
        add_msg(_("You swap places with %s."), who.name.c_str());
        swap_critters( u, who );
        // TODO: Make that depend on stuff
        u.mod_moves( -200 );
    } else if( choice == push ) {
        // TODO: Make NPCs protest when displaced onto dangerous crap
        tripoint oldpos = who.pos();
        who.move_away_from( u.pos(), true );
        u.mod_moves( -20 );
        if( oldpos != who.pos() ) {
            add_msg(_("%s moves out of the way."), who.name.c_str());
        } else {
            add_msg( m_warning, _("%s has nowhere to go!"), who.name.c_str());
        }
    } else if( choice == examine_wounds ) {
        ///\EFFECT_PER slightly increases precision when examining NPCs' wounds

        ///\EFFECT_FIRSTAID increases precision when examining NPCs' wounds
        const bool precise = u.get_skill_level( skill_firstaid ) * 4 + u.per_cur >= 20;
        who.body_window( precise );
    } else if( choice == use_item ) {
        static const std::string heal_string( "heal" );
        const auto will_accept = [&who]( const item &it ) {
            const auto use_fun = it.get_use( heal_string );
            if( use_fun == nullptr ) {
                return false;
            }

            const auto *actor = dynamic_cast<const heal_actor *>( use_fun->get_actor_ptr() );

            return actor != nullptr &&
                   actor->limb_power >= 0 &&
                   actor->head_power >= 0 &&
                   actor->torso_power >= 0;
        };
        const int pos = inv_for_filter( _("Use which item:"), will_accept );

        if( pos == INT_MIN ) {
            add_msg( _("Never mind") );
            return false;
        }
        item &used = u.i_at( pos );
        bool did_use = u.invoke_item( &used, heal_string, who.pos() );
        if( did_use ) {
            // Note: exiting a body part selection menu counts as use here
            u.mod_moves( -300 );
        }
    } else if( choice == sort_armor ) {
        who.sort_armor();
        u.mod_moves( -100 );
    } else if( choice == attack ) {
        if(query_yn(_("You may be attacked! Proceed?"))) {
            //The NPC knows we started the fight, used for morale penalty.
            if( !who.is_enemy() ) {
                who.hit_by_player = true;
            }

            u.melee_attack( who, true );
            who.make_angry();
        }
    }

    return true;
}

void game::examine()
{
    // if we are driving a vehicle, examine the
    // current tile without asking.
    const vehicle * const veh = m.veh_at( u.pos() );
    if( veh && veh->player_in_control( u ) ) {
        examine( u.pos() );
        return;
    }

    tripoint examp = u.pos();
    if( !choose_adjacent_highlight( _("Examine where?"), examp, ACTION_EXAMINE ) ) {
        return;
    }
    examine( examp );
}

void game::examine( const tripoint &examp )
{
    Creature *c = critter_at( examp );
    if( c != nullptr ) {
        monster *mon = dynamic_cast<monster *>( c );
        if( mon != nullptr && mon->has_effect( effect_pet ) ) {
            if( pet_menu( mon ) ) {
                return;
            }
        }

        npc *np = dynamic_cast<npc*>( c );
        if( np != nullptr ) {
            if( npc_menu( *np ) ) {
                return;
            }
        }
    }

    int veh_part = 0;
    vehicle *veh = nullptr;

    veh = m.veh_at( examp, veh_part );
    if( veh != nullptr ) {
        if( u.controlling_vehicle ) {
            add_msg(m_info, _("You can't do that while driving."));
        } else if( abs(veh->velocity) > 0 ) {
            add_msg(m_info, _("You can't do that on a moving vehicle."));
        } else {
            Pickup::pick_up( examp, 0 );
        }
        return;
    }

    if( m.has_flag( "CONSOLE", examp ) ) {
        use_computer( examp );
        return;
    }
    const furn_t &xfurn_t = m.furn( examp ).obj();
    const ter_t &xter_t = m.ter( examp ).obj();

    const tripoint player_pos = u.pos();

    if( m.has_furn( examp ) ) {
        xfurn_t.examine( u, examp );
    } else {
        xter_t.examine( u, examp );
    }

    // Did the player get moved? Bail out if so; our examp probably
    // isn't valid anymore.
    if( player_pos != u.pos() ) {
        return;
    }

    bool none = true;
    if( xter_t.examine != &iexamine::none || xfurn_t.examine != &iexamine::none ) {
        none = false;
    }

    if( !m.tr_at( examp ).is_null() ) {
        iexamine::trap( u, examp );
        draw_ter();
    }

    // In case of teleport trap or somesuch
    if( player_pos != u.pos() ) {
        return;
    }

    if (m.has_flag("SEALED", examp)) {
        if( none ) {
            if (m.has_flag("UNSTABLE", examp)) {
                add_msg(_("The %s is too unstable to remove anything."), m.name(examp).c_str());
            } else {
                add_msg(_("The %s is firmly sealed."), m.name(examp).c_str());
            }
        }
    } else {
        //examp has no traps, is a container and doesn't have a special examination function
        if( m.tr_at( examp ).is_null() && m.i_at(examp).empty() &&
            m.has_flag("CONTAINER", examp) && none) {
            add_msg(_("It is empty."));
        } else if( veh == nullptr ) {
            Pickup::pick_up( examp, 0);
        }
    }
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carefully peeking around a corner, hence the large move cost.
void game::peek()
{
    tripoint p = u.pos();
    if( !choose_adjacent( _("Peek where?"), p.x, p.y ) ) {
        refresh_all();
        return;
    }

    if( m.impassable( p ) ) {
        return;
    }

    peek( p );
}

void game::peek( const tripoint &p )
{
    u.moves -= 200;
    tripoint prev = u.pos();
    u.setpos( p );
    look_around();
    u.setpos( prev );
    draw_ter();
}
////////////////////////////////////////////////////////////////////////////////////////////
tripoint game::look_debug()
{
    editmap edit;
    return edit.edit();
}
////////////////////////////////////////////////////////////////////////////////////////////

void game::print_all_tile_info( const tripoint &lp, WINDOW *w_look, int column, int &line,
                                const int last_line, bool draw_terrain_indicators,
                                const visibility_variables &cache )
{
    auto visibility = m.get_visibility( m.apparent_light_at( lp, cache ), cache );
    switch( visibility ) {
        case VIS_CLEAR:
            {
                int veh_part = 0;
                const vehicle *veh = m.veh_at( lp, veh_part);
                const Creature *creature = critter_at( lp, true );
                print_terrain_info( lp, w_look, column, line );
                print_fields_info( lp, w_look, column, line );
                print_trap_info( lp, w_look, column, line );
                print_creature_info( creature, w_look, column, line );
                print_vehicle_info( veh, veh_part, w_look, column, line, last_line );
                print_items_info( lp, w_look, column, line, last_line );
                print_graffiti_info( lp, w_look, column, line, last_line );

                if ( draw_terrain_indicators) {
                    if ( creature != nullptr ) {
                        creature->draw( w_terrain, lp, true );
                    } else {
                        m.drawsq( w_terrain, u, lp, true, true, lp );
                    }
                }
            }
            break;
        case VIS_BOOMER:
        case VIS_BOOMER_DARK:
        case VIS_DARK:
        case VIS_LIT:
        case VIS_HIDDEN:
            print_visibility_info( w_look, column, line, visibility );

            if (draw_terrain_indicators) {
                print_visibility_indicator( visibility );
            }
            break;
    }

    auto this_sound = sounds::sound_at( lp );
    if( !this_sound.empty() ) {
        mvwprintw( w_look, ++line, 1, _("You heard %s from here."), this_sound.c_str() );
    } else {
        // Check other z-levels
        tripoint tmp = lp;
        for( tmp.z = -OVERMAP_DEPTH; tmp.z <= OVERMAP_HEIGHT; tmp.z++ ) {
            if( tmp.z == lp.z ) {
                continue;
            }

            auto zlev_sound = sounds::sound_at( tmp );
            if( !zlev_sound.empty() ) {
                mvwprintw( w_look, ++line, 1, tmp.z > lp.z ?
                           _("You heard %s from above.") : _("You heard %s from below."),
                           zlev_sound.c_str() );
            }
        }
    }
}

void game::print_visibility_info( WINDOW *w_look, int column, int &line,
                                  visibility_type visibility )
{
    const char* visibility_message = nullptr;
    switch( visibility ) {
        case VIS_CLEAR:
            visibility_message = _("Clearly visible.");
            break;
        case VIS_BOOMER:
            visibility_message = _("A bright pink blur.");
            break;
        case VIS_BOOMER_DARK:
            visibility_message = _("A pink blur.");
            break;
        case VIS_DARK:
            visibility_message = _("Darkness.");
            break;
        case VIS_LIT:
            visibility_message = _("Bright light.");
            break;
        case VIS_HIDDEN:
            visibility_message = _("Unseen.");
            break;
    }

    mvwprintw(w_look, column, line, visibility_message);
    line += 2;
}

void game::print_terrain_info( const tripoint &lp, WINDOW *w_look, int column, int &line)
{
    int ending_line = line + 3;
    std::string tile = m.tername( lp );
    if( m.has_furn( lp ) ) {
        tile += "; " + m.furnname( lp );
    }

    if (m.impassable( lp )) {
        mvwprintw(w_look, line, column, _("%s; Impassable"), tile.c_str());
    } else {
        mvwprintw(w_look, line, column, _("%s; Movement cost %d"), tile.c_str(),
                  m.move_cost(lp) * 50);

        const auto ll = get_light_level(std::max(1.0, LIGHT_AMBIENT_LIT - m.ambient_light_at(lp) + 1.0));
        mvwprintw(w_look, ++line, column, _("Lighting: "));
        wprintz(w_look, ll.second, ll.first.c_str());
    }

    std::string signage = m.get_signage( lp );
    if (signage.size() > 0 && signage.size() < 36) {
        mvwprintw(w_look, ++line, column, _("Sign: %s"), signage.c_str());
    } else if (signage.size() > 0) {
        // Truncate to width of window as a guesstimate.
        mvwprintw(w_look, ++line, column, _("Sign: %s..."), signage.substr(0, 32).c_str());
    }

    if( m.has_zlevels() && lp.z > -OVERMAP_DEPTH && !m.has_floor( lp ) ) {
        // Print info about stuff below
        tripoint below( lp.x, lp.y, lp.z - 1 );
        std::string tile_below = m.tername( below );
        if( m.has_furn( below ) ) {
            tile_below += "; " + m.furnname( below );
        }

        if( !m.has_floor_or_support( lp ) ) {
            mvwprintw(w_look, ++line, column, _("Below: %s; No support"), tile_below.c_str() );
        } else {
            mvwprintw(w_look, ++line, column, _("Below: %s; Walkable"), tile_below.c_str() );
        }
    }

    mvwprintw(w_look, ++line, column, "%s", m.features( lp ).c_str());
    if (line < ending_line) {
        line = ending_line;
    }
}

void game::print_fields_info( const tripoint &lp, WINDOW *w_look, int column, int &line)
{
    const field &tmpfield = m.field_at( lp );
    for( auto &fld : tmpfield ) {
        const field_entry *cur = &fld.second;
        mvwprintz(w_look, line++, column, fieldlist[cur->getFieldType()].color[cur->getFieldDensity() - 1],
                  "%s",
                  fieldlist[cur->getFieldType()].name[cur->getFieldDensity() - 1].c_str());
    }
}

void game::print_trap_info( const tripoint &lp, WINDOW *w_look, const int column, int &line)
{
    const trap &tr = m.tr_at( lp );
    if( tr.can_see( lp, u )) {
        mvwprintz(w_look, line++, column, tr.color, "%s", tr.name.c_str());
    }
}

void game::print_creature_info( const Creature *creature, WINDOW *w_look,
                                const int column, int &line )
{
    if( creature != nullptr && ( u.sees( *creature ) || creature == &u ) ) {
        line = creature->print_info( w_look, line, 6, column );
    }
}

void game::print_vehicle_info( const vehicle *veh, int veh_part, WINDOW *w_look,
                               const int column, int &line, const int last_line )
{
    if (veh) {
        mvwprintw(w_look, line++, column, _("There is a %s there. Parts:"), veh->name.c_str());
        line = veh->print_part_desc(w_look, line, last_line, getmaxx(w_look), veh_part);
    }
}

void game::print_visibility_indicator( visibility_type visibility )
{
    std::string visibility_indicator;
    nc_color visibility_indicator_color = c_white;
    switch( visibility ) {
        case VIS_CLEAR:
            // Nothing printed when visibility is clear
            return;
        case VIS_BOOMER:
        case VIS_BOOMER_DARK:
            visibility_indicator = '#';
            visibility_indicator_color = c_pink;
            break;
        case VIS_DARK:
            visibility_indicator = '#';
            visibility_indicator_color = c_dkgray;
            break;
        case VIS_LIT:
            visibility_indicator = '#';
            visibility_indicator_color = c_ltgray;
            break;
        case VIS_HIDDEN:
            visibility_indicator = 'x';
            visibility_indicator_color = c_white;
            break;
    }

    mvwputch(w_terrain, POSY, POSX, visibility_indicator_color, visibility_indicator);
}

void game::print_items_info( const tripoint &lp, WINDOW *w_look, const int column, int &line,
                             const int last_line )
{
    if( !m.sees_some_items( lp, u ) ) {
        return;
    } else if ( m.has_flag( "CONTAINER", lp ) && !m.could_see_items( lp, u ) ) {
        mvwprintw( w_look, line++, column, _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( "BLIND" ) ) {
        mvwprintz( w_look, line++, column, c_yellow,
                _( "There's something there, but you can't see what it is." ) );
        return;
    } else {
        std::map<std::string, int> item_names;
        for( auto &item : m.i_at( lp ) ) {
            ++item_names[item.tname()];
        }

        const int max_width = getmaxx( w_look ) - column - 1;
        for( auto const &it : item_names ) {
            if( line >= last_line ) {
                mvwprintz( w_look, line++, column , c_yellow, _( "More items here..." ) );
                break;
            }

            if( it.second > 1 ) {
                trim_and_print( w_look, line++, column, max_width, c_white,
                        pgettext( "%s is the name of the item. %d is the quantity of that item.", "%s [%d]" ),
                        it.first.c_str(), it.second );
            } else {
                trim_and_print( w_look, line++, column, max_width, c_white, "%s", it.first.c_str() );
            }
        }
    }
}

void game::print_graffiti_info( const tripoint &lp, WINDOW *w_look, const int column, int &line,
                             const int last_line )
{
    if (line > last_line) {
        return;
    }
    if( m.has_graffiti_at( lp ) ) {
        mvwprintw(w_look, line++, column, _("Graffiti: %s"), m.graffiti_at( lp ).c_str() );
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

bool game::check_zone( const std::string &type, const tripoint &where ) const
{
    return zone_manager::get_manager().has( type, m.getabs( where ) );
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
    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    const int offset_x = (u.posx() + u.view_offset.x) - getmaxx(w_terrain) / 2;
    const int offset_y = (u.posy() + u.view_offset.y) - getmaxy(w_terrain) / 2;

    draw_ter();

    int zone_ui_height = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width :
                                        VIEW_OFFSET_X;

    WINDOW *w_zones = newwin(TERMY - 2 - zone_ui_height - VIEW_OFFSET_Y * 2, width - 2,
                             VIEW_OFFSET_Y + 1, offsetX + 1);
    WINDOW *w_zones_border = newwin(TERMY - zone_ui_height - VIEW_OFFSET_Y * 2, width,
                                    VIEW_OFFSET_Y, offsetX);
    WINDOW *w_zones_info = newwin(zone_ui_height - 1, width - 2,
                                  TERMY - zone_ui_height - VIEW_OFFSET_Y, offsetX + 1);
    WINDOW *w_zones_info_border = newwin(zone_ui_height, width,
                                         TERMY - zone_ui_height - VIEW_OFFSET_Y, offsetX);

    zones_manager_draw_borders(w_zones_border, w_zones_info_border, zone_ui_height, width);
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
    ctxt.register_action("HELP_KEYBINDINGS");

    auto &zones = zone_manager::get_manager();
    int zone_num = zones.size();
    const int max_rows = TERMY - zone_ui_height - 2 - VIEW_OFFSET_Y * 2;
    int start_index = 0;
    int active_index = 0;
    bool blink = false;
    bool redraw_info = true;
    bool stuff_changed = false;

    do {
        if (action == "ADD_ZONE") {
            zones_manager_draw_borders(w_zones_border, w_zones_info_border, zone_ui_height, width);
            werase(w_zones_info);

            mvwprintz(w_zones_info, 3, 2, c_white, _("Select first point."));
            wrefresh(w_zones_info);

            tripoint first = look_around( w_zones_info, u.pos() + u.view_offset, false, true );
            tripoint second = tripoint_min;

            if( first != tripoint_min ) {
                mvwprintz(w_zones_info, 3, 2, c_white, _("Select second point."));
                wrefresh(w_zones_info);

                second = look_around( w_zones_info, first, true, true );
            }

            if( second != tripoint_min ) {
                werase(w_zones_info);
                wrefresh(w_zones_info);

                zones.add( "", "", false, true,
                            m.getabs( tripoint( std::min(first.x, second.x),
                                                std::min(first.y, second.y),
                                                std::min(first.z, second.z) ) ),
                            m.getabs( tripoint( std::max(first.x, second.x),
                                                std::max(first.y, second.y),
                                                std::max(first.z, second.z) ) )
                           );

                zone_num = zones.size();
                active_index = zone_num - 1;

                zones.zones[active_index].set_name();
                zones.zones[active_index].set_type();
                stuff_changed = true;
            }

            draw_ter();
            blink = false;
            redraw_info = true;

            zones_manager_draw_borders(w_zones_border, w_zones_info_border, zone_ui_height, width);
            zones_manager_shortcuts(w_zones_info);

        } else if (zones.size() > 0) {
            if (action == "UP") {
                active_index--;
                if (active_index < 0) {
                    active_index = zone_num - 1;
                }
                draw_ter();
                blink = false;
                redraw_info = true;

            } else if (action == "DOWN") {
                active_index++;
                if (active_index >= zone_num) {
                    active_index = 0;
                }
                draw_ter();
                blink = false;
                redraw_info = true;

            } else if (action == "REMOVE_ZONE") {
                if (active_index < (int)zones.size()) {
                    zones.remove(active_index);
                    active_index--;

                    if (active_index < 0) {
                        active_index = 0;
                    }

                    zone_num = zones.size();

                    draw_ter();
                    wrefresh(w_terrain);
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

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
                    zones.zones[active_index].set_name();
                    stuff_changed = true;
                    break;
                case 2:
                    zones.zones[active_index].set_type();
                    stuff_changed = true;
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

                blink = false;
                redraw_info = true;

                zones_manager_draw_borders(w_zones_border, w_zones_info_border, zone_ui_height, width);
                zones_manager_shortcuts(w_zones_info);

            } else if (action == "MOVE_ZONE_UP" && zones.size() > 1) {
                if (active_index < (int)zones.size() - 1) {
                    std::swap(zones.zones[active_index],
                              zones.zones[active_index + 1]);
                    active_index++;
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

            } else if (action == "MOVE_ZONE_DOWN" && zones.size() > 1) {
                if (active_index > 0) {
                    std::swap(zones.zones[active_index],
                              zones.zones[active_index - 1]);
                    active_index--;
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

            } else if (action == "SHOW_ZONE_ON_MAP") {
                //show zone position on overmap;
                tripoint player_overmap_position = ms_to_omt_copy( m.getabs( u.pos() ) );
                tripoint zone_overmap = ms_to_omt_copy( zones.zones[active_index].get_center_point() );
                overmap::draw_zones( player_overmap_position, zone_overmap, active_index );

                zones_manager_draw_borders(w_zones_border, w_zones_info_border, zone_ui_height, width);
                zones_manager_shortcuts(w_zones_info);

                draw_ter();

                redraw_info = true;

            } else if (action == "ENABLE_ZONE") {
                zones.zones[active_index].set_enabled(true);

                redraw_info = true;
                stuff_changed = true;

            } else if (action == "DISABLE_ZONE") {
                zones.zones[active_index].set_enabled(false);

                redraw_info = true;
                stuff_changed = true;
            }
        }

        if (zone_num == 0) {
            werase(w_zones);
            wrefresh(w_zones_border);
            mvwprintz(w_zones, 5, 2, c_white, _("No Zones defined."));

        } else if (redraw_info) {
            redraw_info = false;
            werase(w_zones);

            calcStartPos(start_index, active_index, max_rows, zone_num);

            draw_scrollbar(w_zones_border, active_index, max_rows, zone_num, 1);
            wrefresh(w_zones_border);

            int iNum = 0;

            tripoint player_absolute_pos = m.getabs( u.pos() );

            //Display saved zones
            for (auto &i : zones.zones) {
                if (iNum >= start_index && iNum < start_index + ((max_rows > zone_num) ? zone_num : max_rows)) {
                    nc_color colorLine = (i.get_enabled()) ? c_white : c_ltgray;

                    if (iNum == active_index) {
                        mvwprintz(w_zones, iNum - start_index, 0, c_yellow, "%s", ">>");
                        colorLine = (i.get_enabled()) ? c_ltgreen : c_green;
                    }

                    //Draw Zone name
                    mvwprintz(w_zones, iNum - start_index, 3, colorLine, "%s",
                              zones.zones[iNum].get_name().c_str());

                    //Draw Type name
                    mvwprintz(w_zones, iNum - start_index, 20, colorLine, "%s",
                              zones.get_name_from_type(zones.zones[iNum].get_type()).c_str());

                    tripoint center = i.get_center_point();

                    //Draw direction + distance
                    mvwprintz(w_zones, iNum - start_index, 32, colorLine, "%*d %s",
                              5, static_cast<int>( trig_dist( player_absolute_pos, center ) ),
                              direction_name_short( direction_from( player_absolute_pos, center ) ).c_str()
                             );
                }
                iNum++;
            }
        }

        if (zone_num > 0) {
            blink = !blink;

            tripoint start = m.getlocal(zones.zones[active_index].get_start_point());
            tripoint end = m.getlocal(zones.zones[active_index].get_end_point());

            if( blink ) {
                //draw marked area
                tripoint offset = tripoint( offset_x, offset_y, 0 ); //ASCII
#ifdef TILES
                if( use_tiles ) {
                    offset = tripoint( 0, 0, 0 ); //TILES
                } else {
                    offset = tripoint( -offset_x, -offset_y, 0 ); //SDL
                }
#endif

                draw_zones( start, end, offset );
            } else {
                //clear marked area
#ifdef TILES
                if (!use_tiles) {
#endif
                    for (int iY = start.y; iY <= end.y; ++iY) {
                        for (int iX = start.x; iX <= end.x; ++iX) {
                            if (u.sees(iX, iY)) {
                                m.drawsq(w_terrain, u,
                                         tripoint( iX, iY, u.posz() + u.view_offset.z ),
                                         false,
                                         false,
                                         u.pos() + u.view_offset );
                            } else {
                                if (u.has_effect( effect_boomered)) {
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
        action = ctxt.handle_input();
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

    if( stuff_changed ) {
        auto &zones = zone_manager::get_manager();
        if( query_yn( _("Save changes?") ) ) {
            zones.save_zones();
        } else {
            zones.load_zones();
        }

        zones.cache_data();
    }

    u.view_offset = stored_view_offset;

    refresh_all();
}

tripoint game::look_around()
{
    return look_around( nullptr, u.pos() + u.view_offset, false, false );
}

tripoint game::look_around( WINDOW *w_info, const tripoint &start_point,
                            bool has_first_point, bool select_zone )
{
    bVMonsterLookFire = false;
    // TODO: Make this `true`
    const bool allow_zlev_move = m.has_zlevels() &&
        ( debug_mode || fov_3d || u.has_trait( "DEBUG_NIGHTVISION" ) );

    temp_exit_fullscreen();

    const int offset_x = (u.posx() + u.view_offset.x) - getmaxx(w_terrain) / 2;
    const int offset_y = (u.posy() + u.view_offset.y) - getmaxy(w_terrain) / 2;

    tripoint lp = u.pos() + u.view_offset;
    int &lx = lp.x;
    int &ly = lp.y;
    int &lz = lp.z;

    if( select_zone && has_first_point ) {
        lp = start_point;
    }

    draw_ter( lp );

    //change player location to peek location temporarily for minimap update
    tripoint current_pos = u.pos();
    u.setpos(lp);
    draw_pixel_minimap();
    u.setpos(current_pos);

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
    bool fast_scroll = false;
    bool blink = false;

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
    ctxt.set_iso(true);
    ctxt.register_directions();
    ctxt.register_action("COORDINATE");
    ctxt.register_action("LEVEL_UP");
    ctxt.register_action("LEVEL_DOWN");
    ctxt.register_action("TOGGLE_FAST_SCROLL");
    if (select_zone) {
        ctxt.register_action("SELECT");
    } else {
        ctxt.register_action("TRAVEL_TO");
        ctxt.register_action("LIST_ITEMS");
        ctxt.register_action("MOUSE_MOVE");
    }
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");

    const int old_levz = get_levz();

    m.update_visibility_cache( old_levz );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    do {
        if (bNewWindow) {
            werase(w_info);
            draw_border(w_info);

            if (!select_zone) {
                nc_color clr = c_white;
                const std::string colored_key = string_format( "<color_ltgreen>%s</color>",
                                                               ctxt.get_desc( "LIST_ITEMS", 1 ).c_str() );
                print_colored_text( w_info, getmaxy(w_info) - 1, 2, clr, clr,
                                    string_format( _( "Press %s to list items and monsters" ),
                                                   colored_key.c_str() ) );
            }
        }

        if (select_zone) {
            //Select Zone
            if (has_first_point) {
                blink = !blink;

                const int dx = start_point.x - offset_x + u.posx() - lx;
                const int dy = start_point.y - offset_y + u.posy() - ly;

                if (blink) {
                    const tripoint start = tripoint( std::min(dx, POSX), std::min(dy, POSY), lz );
                    const tripoint end = tripoint( std::max(dx, POSX), std::max(dy, POSY), lz );

                    tripoint offset = tripoint( 0, 0, 0 ); //ASCII/SDL
#ifdef TILES
                    if( use_tiles ) {
                        offset = tripoint( offset_x + lx - u.posx(), offset_y + ly - u.posy(), 0 ); //TILES
                    }
#endif

                    draw_zones( start, end, offset );

                } else {
#ifdef TILES
                    if (!use_tiles) {
#endif
                        for (int iY = std::min(start_point.y, ly); iY <= std::max(start_point.y, ly); ++iY) {
                            for (int iX = std::min(start_point.x, lx); iX <= std::max(start_point.x, lx); ++iX) {
                                if (u.sees(iX, iY)) {
                                    m.drawsq(w_terrain, u,
                                             tripoint( iX, iY, lp.z ),
                                             false,
                                             false,
                                             tripoint( lx, ly, u.posz() ) );
                                } else {
                                    if (u.has_effect( effect_boomered)) {
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
            int first_line = 1;
            const int last_line = lookHeight - 2;
            print_all_tile_info( lp, w_info, 1, first_line, last_line, !is_draw_tiles_mode(), cache );

            if (fast_scroll) {
                //~ "Fast Scroll" mark below the top right corner of the info window
                right_print( w_info, 1, 0, c_ltgreen, _( "F" ) );
            }

            wrefresh(w_info);
        }

        if (!is_draw_tiles_mode() && action != "MOUSE_MOVE") {
            // When tiles are disabled, this refresh is required to update the
            // selected terrain square with highlighted ascii coloring. When
            // tiles are enabled, the selected square isn't highlighted using
            // this function, and it is too CPU-intensive to call repeatedly
            // in a mouse event loop. If we did want to highlight the tile
            // selected by the mouse, we could call wrefresh when the mouse
            // hovered over a new tile (rather than on every mouse move
            // event).
            wrefresh(w_terrain);
        }

        if (select_zone && has_first_point) {
            inp_mngr.set_timeout(BLINK_SPEED);
        }

        //Wait for input
        action = ctxt.handle_input();
        if (action == "LIST_ITEMS") {
            list_items_monsters();
            draw_ter( lp, true );

        } else if (action == "TOGGLE_FAST_SCROLL") {
            fast_scroll = !fast_scroll;
        } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
            if( !allow_zlev_move ) {
                continue;
            }

            int new_levz = lp.z + ( action == "LEVEL_UP" ? 1 : -1 );
            if( new_levz > OVERMAP_HEIGHT ) {
                new_levz = OVERMAP_HEIGHT;
            } else if( new_levz < -OVERMAP_DEPTH ) {
                new_levz = -OVERMAP_DEPTH;
            }

            add_msg( m_debug, "levx: %d, levy: %d, levz :%d", get_levx(), get_levy(), new_levz );
            u.view_offset.z = new_levz - u.posz();
            lp.z = new_levz;
            refresh_all();
            draw_ter( lp, true );
        } else if( action == "TRAVEL_TO" ) {
            if( !u.sees( lp ) ) {
                add_msg(_("You can't see that destination."));
                continue;
            }

            auto route = m.route( u.pos(), lp, u.get_pathfinding_settings(), u.get_path_avoid() );
            if( route.size() > 1 ) {
                route.pop_back();
                u.set_destination( route );
            } else {
                add_msg(m_info, _("You can't travel there."));
                continue;
            }
            return { INT_MIN, INT_MIN, INT_MIN };
        } else if (!ctxt.get_coordinates(w_terrain, lx, ly) && action != "MOUSE_MOVE") {
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

            draw_ter( lp, true );
        }
    } while (action != "QUIT" && action != "CONFIRM" && action != "SELECT");

    if( m.has_zlevels() && lp.z != old_levz ) {
        m.build_map_cache( old_levz );
        u.view_offset.z = 0;
    }

    inp_mngr.set_timeout(-1);

    if (bNewWindow) {
        werase(w_info);
        delwin(w_info);
    }
    reenter_fullscreen();
    bVMonsterLookFire = true;

    if( action == "CONFIRM" || action == "SELECT" ) {
        return lp;
    }

    return tripoint( INT_MIN, INT_MIN, INT_MIN );
}

std::vector<map_item_stack> game::find_nearby_items(int iRadius)
{
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> item_order;

    if( u.is_blind() ) {
        return ret;
    }

    std::vector<tripoint> points = closest_tripoints_first( iRadius, u.pos() );

    tripoint last_pos;

    for( auto &points_p_it : points ) {
        if( points_p_it.y >= u.posy() - iRadius && points_p_it.y <= u.posy() + iRadius &&
            u.sees( points_p_it ) &&
            m.sees_some_items( points_p_it, u ) ) {

            for( auto &elem : m.i_at( points_p_it ) ) {
                const std::string name = elem.tname();
                const tripoint relative_pos = points_p_it - u.pos();

                if( std::find( item_order.begin(), item_order.end(), name ) == item_order.end() ) {
                    item_order.push_back( name );
                    temp_items[name] = map_item_stack( &elem, relative_pos );
                } else {
                    temp_items[name].add_at_pos( &elem, relative_pos );
                }
            }
        }
    }

    for( auto &elem : item_order ) {
        ret.push_back( temp_items[elem] );
    }

    return ret;
}

void game::draw_item_filter_rules(WINDOW *window, int rows)
{
    for (int i = 0; i < rows - 1; i++) {
        mvwprintz(window, i, 1, c_black, std::string(getmaxx(window) - 2, ' ').c_str());
    }

    mvwprintz(window, 0, 2, c_white, "%s", _("Type part of an item's name to"));
    mvwprintz(window, 1, 2, c_white, "%s", _("filter it."));

    mvwprintz(window, 3, 2, c_white, "%s", _("Separate multiple items with ,"));
    mvwprintz(window, 4, 2, c_white, "%s", _("Example: back,flash,aid, ,band"));

    mvwprintz(window, 6, 2, c_white, "%s", _("To exclude items, place - in front"));
    mvwprintz(window, 7, 2, c_white, "%s", _("Example: -pipe,-chunk,-steel"));

    mvwprintz(window, 9, 2, c_white, "%s", _("Search [c]ategory or [m]aterial:"));
    mvwprintz(window, 10, 2, c_white, "%s", _("Example: {c:food},{m:iron}"));
    wrefresh(window);
}

std::string game::ask_item_priority_high(WINDOW *window, int rows)
{
    for (int i = 0; i < rows - 1; i++) {
        mvwprintz(window, i, 1, c_black, std::string(getmaxx(window) - 2, ' ').c_str());
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
        mvwprintz(window, i, 1, c_black, std::string(getmaxx(window) - 2, ' ').c_str());
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

void game::draw_trail_to_square( const tripoint &t, bool bDrawX )
{
    //Reset terrain
    draw_ter();

    std::vector<tripoint> pts;
    tripoint center = u.pos() + u.view_offset;
    if( t != tripoint_zero ) {
        //Draw trail
        pts = line_to( u.pos(), u.pos() + t, 0, 0 );
    } else {
        //Draw point
        pts.push_back( u.pos() );
    }

    draw_line( u.pos() + t, center, pts );
    if (bDrawX) {
        char sym = 'X';
        if( t.z > 0 ) {
            sym = '^';
        } else if( t.z < 0 ) {
            sym = 'v';
        }
        if (pts.empty()) {
            mvwputch( w_terrain, POSY, POSX, c_white, sym );
        } else {
            mvwputch( w_terrain, POSY + (pts[pts.size() - 1].y - (u.posy() + u.view_offset.y)),
                      POSX + (pts[pts.size() - 1].x - (u.posx() + u.view_offset.x)),
                      c_white, sym );
        }
    }

    wrefresh(w_terrain);
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state(WINDOW *window, int height, bool bRadiusSort)
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

    mvwprintz(window, 0, 2, c_ltgreen, "<Tab> ");
    wprintz(window, c_white, _("Items"));

    std::string sSort;
    if ( bRadiusSort ) {
        //~ Sort type: distance.
        sSort = _("<s>ort: dist");
    } else {
        //~ Sort type: category.
        sSort = _("<s>ort: cat");
    }

    int letters = utf8_width(sSort);

    shortcut_print(window, 0, getmaxx(window) - letters, c_white, c_ltgreen, sSort.c_str());

    std::vector<std::string> tokens;
    if (sFilter != "") {
        tokens.push_back(_("<R>eset"));
    }

    tokens.push_back(_("<E>xamine"));
    tokens.push_back(_("<C>ompare"));
    tokens.push_back(_("<F>ilter"));
    tokens.push_back(_("<+/->Priority"));

    int gaps = tokens.size() + 1;
    letters = 0;
    int n = tokens.size();
    for (int i = 0; i < n; i++) {
        letters += utf8_width(tokens[i]) - 2; //length ignores < >
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

void centerlistview( const tripoint &active_item_position )
{
    player &u = g->u;
    if (get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false") {
        u.view_offset.z = active_item_position.z;
        int xpos = POSX + active_item_position.x;
        int ypos = POSY + active_item_position.y;
        if (get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered") {
            int xOffset = TERRAIN_WINDOW_WIDTH / 2;
            int yOffset = TERRAIN_WINDOW_HEIGHT / 2;
            if (!is_valid_in_w_terrain(xpos, ypos)) {
                if (xpos < 0) {
                    u.view_offset.x = xpos - xOffset;
                } else {
                    u.view_offset.x = xpos - (TERRAIN_WINDOW_WIDTH - 1) + xOffset;
                }

                if (xpos < 0) {
                    u.view_offset.y = ypos - yOffset;
                } else {
                    u.view_offset.y = ypos - (TERRAIN_WINDOW_HEIGHT - 1) + yOffset;
                }
            } else {
                u.view_offset.x = 0;
                u.view_offset.y = 0;
            }
        } else {
            if (xpos < 0) {
                u.view_offset.x = xpos;
            } else if (xpos >= TERRAIN_WINDOW_WIDTH) {
                u.view_offset.x = xpos - (TERRAIN_WINDOW_WIDTH - 1);
            } else {
                u.view_offset.x = 0;
            }

            if (ypos < 0) {
                u.view_offset.y = ypos;
            } else if (ypos >= TERRAIN_WINDOW_HEIGHT) {
                u.view_offset.y = ypos - (TERRAIN_WINDOW_HEIGHT - 1);
            } else {
                u.view_offset.y = 0;
            }
        }
    }

}

#define MAXIMUM_ZOOM_LEVEL 4
void game::zoom_in()
{
#ifdef TILES
    if (tileset_zoom > MAXIMUM_ZOOM_LEVEL) {
        tileset_zoom = tileset_zoom / 2;
    } else {
        tileset_zoom = 64;
    }
    rescale_tileset(tileset_zoom);
#endif
}

void game::zoom_out()
{
#ifdef TILES
    if (tileset_zoom == 64) {
        tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        tileset_zoom = tileset_zoom * 2;
    }
    rescale_tileset(tileset_zoom);
#endif
}

void game::reset_zoom()
{
#ifdef TILES
    tileset_zoom = 16;
    rescale_tileset(tileset_zoom);
#endif // TILES
}

int game::get_user_action_counter() const
{
    return user_action_counter;
}

void game::list_items_monsters()
{
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
        plfire();
    }
    refresh_all();
    reenter_fullscreen();
}

int game::list_items(const int iLastState)
{
    int iInfoHeight = std::min(25, TERMY / 2);
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width : VIEW_OFFSET_X;

    WINDOW *w_items = newwin(TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2, width - 2,VIEW_OFFSET_Y + 1, offsetX + 1);
    WINDOW_PTR w_itemsptr( w_items );

    WINDOW *w_items_border = newwin(TERMY - iInfoHeight - VIEW_OFFSET_Y * 2, width,VIEW_OFFSET_Y, offsetX);
    WINDOW_PTR w_items_borderptr( w_items_border );

    WINDOW *w_item_info = newwin(iInfoHeight, width, TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX);
    WINDOW_PTR w_item_infoptr( w_item_info );

    //Area to search +- of players position.
    ///\EFFECT_PER increases range of interacting with items on the ground from a list
    const int iRadius = 12 + (u.per_cur * 2);

    bool sort_radius;
    bool addcategory;

    // use previously selected sorting method
    if (uistate.list_item_sort == 1) {
        sort_radius = true;
        addcategory = false;
    } else if (uistate.list_item_sort == 2) {
        sort_radius = false;
        addcategory = true;
    } else { // default state after start
        sort_radius = true;
        addcategory = false;
    }

    // reload filter/priority settings on the first invocation, if they were active
    if (!uistate.list_item_init) {
        if (uistate.list_item_filter_active) {
            sFilter = uistate.list_item_filter;
        }
        if (uistate.list_item_downvote_active) {
            list_item_downvote = uistate.list_item_downvote;
        }
        if (uistate.list_item_priority_active) {
            list_item_upvote = uistate.list_item_priority;
        }
        uistate.list_item_init = true;
    }

    //this stores the items found, along with the coordinates
    std::vector<map_item_stack> ground_items_radius = find_nearby_items(iRadius);
    std::vector<map_item_stack> ground_items = ground_items_radius;
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items = (sFilter != "" ?
            filter_item_stacks(ground_items, sFilter) :
            ground_items);
    int highPEnd = list_filter_high_priority(filtered_items, list_item_upvote);
    int lowPStart = list_filter_low_priority(filtered_items, highPEnd, list_item_downvote);
    int iItemNum = ground_items.size();
    if (iItemNum > 0) {
        uistate.list_item_mon = 1; // remember we've tabbed here
    }

    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    int iReturn = -1;
    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2;
    int iStartPos = 0;
    tripoint active_pos;
    tripoint iLastActive = tripoint_min;
    bool reset = true;
    bool refilter = true;
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    map_item_stack *activeItem = NULL;
    std::map<int, std::string> mSortCategory;

    std::string action;
    input_context ctxt("LIST_ITEMS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("LEFT", _("Previous item"));
    ctxt.register_action("RIGHT", _("Next item"));
    ctxt.register_action("PAGE_DOWN");
    ctxt.register_action("PAGE_UP");
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
    ctxt.register_action("SORT");
    ctxt.register_action("TRAVEL_TO");

    do {
        if (!ground_items.empty() || iLastState == 1) {
            if (action == "COMPARE") {
                compare( active_pos );
                reset = true;
                refresh_all();
            } else if (action == "FILTER") {
                draw_item_filter_rules(w_item_info, iInfoHeight);
                sFilter = string_input_popup(_("Filter:"), 55, sFilter,
                                _("UP: history, CTRL-U clear line, ESC: abort, ENTER: save"), "item_filter", 256);
                reset = true;
                refilter = true;
                addcategory = !sort_radius;
                if ( sFilter != "" ) {
                    uistate.list_item_filter_active = true;
                } else {
                    uistate.list_item_filter_active = false;
                }
            } else if (action == "RESET_FILTER") {
                sFilter = "";
                filtered_items = ground_items;
                iLastActive = tripoint_min;
                reset = true;
                refilter = true;
                uistate.list_item_filter_active = false;
                addcategory = !sort_radius;
            } else if (action == "EXAMINE" && filtered_items.size()) {
                std::vector<iteminfo> vThisItem, vDummy;

                activeItem->example->info(true, vThisItem);
                int iDummySelect = 0;
                draw_item_info(0, width - 5, 0, TERMY - VIEW_OFFSET_Y * 2,
                               activeItem->example->tname(), activeItem->example->type_name(), vThisItem, vDummy, iDummySelect,
                               false, false, true);
                // wait until the user presses a key to wipe the screen

                iLastActive = tripoint_min;
                reset = true;
            } else if (action == "PRIORITY_INCREASE") {
                std::string temp = ask_item_priority_high(w_item_info, iInfoHeight);
                list_item_upvote = temp;
                refilter = true;
                reset = true;
                addcategory = !sort_radius;
                if ( list_item_upvote != "" ) {
                    uistate.list_item_priority_active = true;
                } else {
                    uistate.list_item_priority_active = false;
                }
            } else if (action == "PRIORITY_DECREASE") {
                std::string temp = ask_item_priority_low(w_item_info, iInfoHeight);
                list_item_downvote = temp;
                refilter = true;
                reset = true;
                addcategory = !sort_radius;
                if ( list_item_downvote != "" ) {
                    uistate.list_item_downvote_active = true;
                } else {
                    uistate.list_item_downvote_active = false;
                }
            } else if (action == "SORT") {
                if ( sort_radius ) {
                    sort_radius = false;
                    addcategory = true;
                    std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort );
                    uistate.list_item_sort = 2; // list is sorted by category
                } else {
                    sort_radius = true;
                    ground_items = ground_items_radius;
                    uistate.list_item_sort = 1; // list is sorted by distance
                }

                highPEnd = -1;
                lowPStart = -1;
                iCatSortNum = 0;

                mSortCategory.clear();
                refilter = true;
                reset = true;

            } else if( action == "TRAVEL_TO" ) {
                if( !u.sees( u.pos() + active_pos ) ) {
                    add_msg(_("You can't see that destination."));
                }
                auto route = m.route( u.pos(), u.pos() + active_pos, u.get_pathfinding_settings(), u.get_path_avoid() );
                if( route.size() > 1 ) {
                    route.pop_back();
                    u.set_destination( route );
                    break;
                } else {
                    add_msg(m_info, _("You can't travel there."));
                }
            }

            if ( uistate.list_item_sort == 1 ) {
                ground_items = ground_items_radius;
            } else if ( uistate.list_item_sort == 2 ) {
                std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort );
            }

            if (refilter) {
                refilter = false;

                filtered_items = filter_item_stacks(ground_items, sFilter);
                highPEnd = list_filter_high_priority(filtered_items, list_item_upvote);
                lowPStart = list_filter_low_priority(filtered_items, highPEnd, list_item_downvote);
                iActive = 0;
                page_num = 0;
                iLastActive = tripoint_min;
                iItemNum = filtered_items.size();
            }

            if ( addcategory ) {
                addcategory = false;
                std::string sLastCategoryName = "";
                iCatSortNum = 0;
                mSortCategory.clear();

                if ( highPEnd > 0 ) {
                    mSortCategory[0] = _("HIGH PRIORITY");
                    iCatSortNum++;
                }

                const int iStart = (highPEnd > 0) ? highPEnd : 0;
                const int iEnd = (lowPStart < (int)filtered_items.size()) ? lowPStart : filtered_items.size() ;

                for (int i=iStart; i < iEnd; i++) {
                    if ( filtered_items[i].example->get_category().name != sLastCategoryName ) {
                        sLastCategoryName = filtered_items[i].example->get_category().name;
                        mSortCategory[i + iCatSortNum] = _(sLastCategoryName.c_str());

                        iCatSortNum++;
                    }
                }

                if ( lowPStart < (int)filtered_items.size() ) {
                    mSortCategory[lowPStart + iCatSortNum] = _("LOW PRIORITY");
                    iCatSortNum++;
                }

                if ( mSortCategory[0] != "" ) {
                    iActive++;
                }

                iItemNum = filtered_items.size() + iCatSortNum;
            }

            if (reset) {
                reset_item_list_state(w_items_border, iInfoHeight, sort_radius);
                reset = false;
                iScrollPos = 0;
            }

            if (action == "UP") {
                do {
                    iActive--;
                } while(mSortCategory[iActive] != "");
                iScrollPos = 0;
                page_num = 0;
                if (iActive < 0) {
                    iActive = iItemNum - 1;
                }
            } else if (action == "DOWN") {
                do {
                    iActive++;
                } while(mSortCategory[iActive] != "");
                iScrollPos = 0;
                page_num = 0;
                if (iActive >= iItemNum) {
                    iActive = (mSortCategory[0] != "") ? 1 : 0;
                }
            } else if (action == "RIGHT") {
                page_num++;
                if (!filtered_items.empty() && page_num >= (int)activeItem->vIG.size()) {
                    page_num = activeItem->vIG.size() - 1;
                }
            } else if (action == "LEFT") {
                page_num--;
                if (page_num < 0) {
                    page_num = 0;
                }
            } else if (action == "PAGE_UP") {
                iScrollPos--;
            } else if (action == "PAGE_DOWN") {
                iScrollPos++;
            } else if (action == "NEXT_TAB" || action == "PREV_TAB") {
                u.view_offset = stored_view_offset;
                return 1;
            }

            if (ground_items.empty() && iLastState == 1) {
                reset_item_list_state(w_items_border, iInfoHeight, sort_radius);
                wrefresh(w_items_border);
                mvwprintz(w_items, 10, 2, c_white, _("You don't see any items around you!"));
            } else {
                werase(w_items);

                calcStartPos(iStartPos, iActive, iMaxRows, iItemNum);

                int iNum = 0;
                active_pos = tripoint_zero;
                std::stringstream sText;
                bool high = true;
                bool low = false;
                int index = 0;
                int iCatSortOffset = 0;
                bool bKeepIter = false;

                for (int i=0; i < iStartPos; i++) {
                    if (mSortCategory[i] != "") {
                        iNum++;
                    }
                }

                for (std::vector<map_item_stack>::iterator iter = filtered_items.begin();
                     iter != filtered_items.end(); ++index) {

                    if ( index < highPEnd + iCatSortOffset ) {
                        high = true;
                    } else if ( index >= lowPStart + iCatSortOffset ) {
                        low = true;
                    } else {
                        high = false;
                        low = false;
                    }

                    if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iItemNum) ? iItemNum : iMaxRows)) {
                        int iThisPage = 0;

                        if ( mSortCategory[iNum] != "" ) {
                            iCatSortOffset++;
                            bKeepIter = true;

                            mvwprintz(w_items, iNum - iStartPos, 1, c_magenta, "%s", mSortCategory[iNum].c_str());

                        } else {
                            if (iNum == iActive) {
                                iThisPage = page_num;

                                active_pos = iter->vIG[iThisPage].pos;

                                activeItem = &(*iter);
                            }

                            sText.str("");

                            if (iter->vIG.size() > 1) {
                                sText << "[" << iThisPage + 1 << "/" << iter->vIG.size() << "] (" << iter->totalcount << ") ";
                            }

                            sText << iter->example->tname();

                            if (iter->vIG[iThisPage].count > 1) {
                                sText << " [" << iter->vIG[iThisPage].count << "]";
                            }

                            trim_and_print(w_items, iNum - iStartPos, 1, width - 9,
                                      ((iNum == iActive) ? c_ltgreen : (high ? c_yellow : (low ? c_red : iter->example->color_in_inventory()))),
                                      "%s", (sText.str()).c_str());
                            int numw = iItemNum > 9 ? 2 : 1;
                            mvwprintz(w_items, iNum - iStartPos, width - (6 + numw),
                                      ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                                      numw, square_dist(0, 0, iter->vIG[iThisPage].pos.x, iter->vIG[iThisPage].pos.y),
                                      direction_name_short(direction_from(0, 0, iter->vIG[iThisPage].pos.x, iter->vIG[iThisPage].pos.y)).c_str()
                                     );
                        }
                    }

                    if ( bKeepIter ) {
                        bKeepIter = false;
                    } else {
                        ++iter;
                    }

                    iNum++;
                }

                iNum = 0;
                for (int i=0; i < iActive; i++) {
                    if (mSortCategory[i] != "") {
                        iNum++;
                    }
                }

                mvwprintz(w_items_border, 0, (width - 9) / 2 + ((iItemNum > 9) ? 0 : 1),
                          c_ltgreen, " %*d", ((iItemNum > 9) ? 2 : 1), (iItemNum > 0) ? iActive - iNum + 1 : 0);
                wprintz(w_items_border, c_white, " / %*d ", ((iItemNum > 9) ? 2 : 1), iItemNum - iCatSortNum);

                werase(w_item_info);

                if ( iItemNum > 0 ) {
                    std::vector<iteminfo> vThisItem, vDummy;
                    activeItem->example->info(true, vThisItem);

                    draw_item_info(w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true);

                    //Only redraw trail/terrain if x/y position changed
                    if( active_pos != iLastActive ) {
                        iLastActive = active_pos;
                        centerlistview( active_pos );
                        draw_trail_to_square( active_pos, true );
                    }
                }

                draw_scrollbar(w_items_border, iActive, iMaxRows, iItemNum, 1);
                wrefresh(w_items_border);
            }

            bool bDrawLeft = (ground_items.empty() && iLastState == 1) || filtered_items.empty();
            draw_custom_border(w_item_info, bDrawLeft, true, false, true,
                               LINE_XOXO, LINE_XOXO, true, true);

            wrefresh(w_items);
            wrefresh(w_item_info);

            refresh();

            action = ctxt.handle_input();
        } else {
            iReturn = 0;
            action = "QUIT";
        }
    } while (action != "QUIT");

    u.view_offset = stored_view_offset;

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
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width :
                                        VIEW_OFFSET_X;

    WINDOW *w_monsters = newwin(TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2, width - 2,
                                VIEW_OFFSET_Y + 1, offsetX + 1);
    WINDOW_PTR w_monstersptr( w_monsters );
    WINDOW *w_monsters_border = newwin(TERMY - iInfoHeight - VIEW_OFFSET_Y * 2, width,
                                       VIEW_OFFSET_Y, offsetX);
    WINDOW_PTR w_monsters_borderptr( w_monsters_border );
    WINDOW *w_monster_info = newwin(iInfoHeight - 1, width - 2,
                                    TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX + 1);
    WINDOW_PTR w_monster_infoptr( w_monster_info );
    WINDOW *w_monster_info_border = newwin(iInfoHeight, width,
                                           TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX);
    WINDOW_PTR w_monster_info_borderptr( w_monster_info_border );

    uistate.list_item_mon = 2; // remember we've tabbed here

    const int iWeaponRange = u.gun_engagement_range( u.weapon, player::engagement::maximum );

    const tripoint stored_view_offset = u.view_offset;
    u.view_offset = tripoint_zero;

    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2 - 1;
    int iStartPos = 0;
    tripoint iActivePos;
    tripoint iLastActivePos = tripoint_min;
    Creature *cCurMon = nullptr;

    for (int i = 1; i < TERMX; i++) {
        if (i < width) {
            mvwputch(w_monsters_border, 0, i, BORDER_COLOR, LINE_OXOX); // -
            mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, BORDER_COLOR,
                     LINE_OXOX); // -
        }

        if (i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2) {
            mvwputch(w_monsters_border, i, 0, BORDER_COLOR, LINE_XOXO); // |
            mvwputch(w_monsters_border, i, width - 1, BORDER_COLOR, LINE_XOXO); // |
        }
    }

    mvwputch(w_monsters_border, 0, 0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w_monsters_border, 0, width - 1, BORDER_COLOR, LINE_OOXX); // ^|

    mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, BORDER_COLOR,
             LINE_XXXO); // |-
    mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, BORDER_COLOR,
             LINE_XOXX); // -|

    mvwprintz(w_monsters_border, 0, 2, c_ltgreen, "<Tab> ");
    wprintz(w_monsters_border, c_white, _("Monsters"));

    std::string action;
    input_context ctxt("LIST_MONSTERS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("SAFEMODE_BLACKLIST_ADD");
    ctxt.register_action("SAFEMODE_BLACKLIST_REMOVE");
    ctxt.register_action("QUIT");
    if ( bVMonsterLookFire ) {
        ctxt.register_action("look");
        ctxt.register_action("fire");
    }
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
            u.view_offset = stored_view_offset;
            return 1;
        } else if (action == "SAFEMODE_BLACKLIST_REMOVE") {
            const auto m = dynamic_cast<monster*>( cCurMon );
            const std::string monName = (m != nullptr) ? m->name() : "human";

            if ( get_safemode().has_rule(monName, Creature::A_ANY) ) {
                get_safemode().remove_rule(monName, Creature::A_ANY);
            }
        } else if (action == "SAFEMODE_BLACKLIST_ADD") {
            if ( !get_safemode().empty() ) {
                const auto m = dynamic_cast<monster*>( cCurMon );
                const std::string monName = (m != nullptr) ? m->name() : "human";

                get_safemode().add_rule(monName, Creature::A_ANY, get_option<int>( "SAFEMODEPROXIMITY" ), RULE_BLACKLISTED);
            }
        } else if (action == "look") {
            tripoint recentered = look_around();
            iLastActivePos = recentered;
        } else if (action == "fire") {
            if( cCurMon != nullptr &&
                rl_dist( u.pos(), cCurMon->pos() ) <= iWeaponRange) {
                last_target = mon_at( cCurMon->pos(), true );
                u.view_offset = stored_view_offset;
                return 2;
            }
        }

        if (vMonsters.empty()) {
            wrefresh(w_monsters_border);
            mvwprintz(w_monsters, 10, 2, c_white, _("You don't see any monsters around you!"));
        } else {
            if( static_cast<size_t>( iActive ) >= vMonsters.size() ) {
                iActive = 0;
            }
            werase(w_monsters);

            calcStartPos(iStartPos, iActive, iMaxRows, iMonsterNum);

            cCurMon = vMonsters[iActive];
            iActivePos = cCurMon->pos() - u.pos();

            const auto endY = std::min<int>( iMaxRows, iMonsterNum - iStartPos );
            for( int y = 0; y < endY; ++y ) {
                const auto critter = vMonsters[y + iStartPos];
                const bool selected = ( cCurMon == critter );
                const auto m = dynamic_cast<monster*>( critter );
                const auto p = dynamic_cast<npc*>( critter );

                    if( critter->sees( g->u ) ) {
                        mvwprintz(w_monsters, y, 0, c_yellow, "!");
                    }

                    bool type_npc = false;
                    if( m != nullptr ) {
                        mvwprintz(w_monsters, y, 1, selected ? c_ltgreen : c_white, "%s", m->name().c_str());
                    } else {
                        mvwprintz(w_monsters, y, 1, selected ? c_ltgreen : c_white, "%s", critter->disp_name().c_str());
                        type_npc = true;
                    }

                    if ( selected && !get_safemode().empty() ) {
                        for (int i = 1; i < width-2; i++) {
                            mvwputch(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2,
                                     i, BORDER_COLOR, LINE_OXOX); // -
                        }
                        std::string sSafemode = _("<A>dd do safemode Blacklist");
                        const std::string monName = (type_npc) ? get_safemode().npc_type_name() : m->name();
                        if ( get_safemode().has_rule(monName, Creature::A_ANY) ) {
                            sSafemode = _("<R>emove from safemode Blacklist");
                        }

                        shortcut_print(w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 3,
                        c_white, c_ltgreen, sSafemode.c_str());
                    }

                    nc_color color = c_white;
                    std::string sText;

                    if( m != nullptr ) {
                        m->get_HP_Bar(color, sText);
                    } else {
                        std::tie(sText, color) =
                            ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
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
                                numw, square_dist(0, 0, critter->posx() - u.posx(),
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

            if (bVMonsterLookFire) {
                mvwprintz(w_monsters, getmaxy(w_monsters) - 1, 1, c_ltgreen, "%s", ctxt.press_x( "look" ).c_str());
                wprintz(w_monsters, c_ltgray, " %s", _("to look around"));

                if (rl_dist( u.pos(), cCurMon->pos() ) <= iWeaponRange) {
                    wprintz(w_monsters, c_ltgray, "%s", " ");
                    wprintz(w_monsters, c_ltgreen, "%s", ctxt.press_x( "fire" ).c_str());
                    wprintz(w_monsters, c_ltgray, " %s", _("to shoot"));
                }
            }

            //Only redraw trail/terrain if x/y position changed
            if( iActivePos != iLastActivePos ) {
                iLastActivePos = iActivePos;
                centerlistview( iActivePos );
                draw_trail_to_square( iActivePos, false );
            }

            draw_scrollbar(w_monsters_border, iActive, iMaxRows, iMonsterNum, 1);
            wrefresh(w_monsters_border);
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

    u.view_offset = stored_view_offset;

    return -1;
}

// Establish or release a grab on a vehicle
void game::grab()
{
    tripoint grabp( 0, 0, 0 );
    if( u.grab_type != OBJECT_NONE ) {
        vehicle *veh = m.veh_at( u.pos() + u.grab_point );
        if( veh != nullptr ) {
            add_msg(_("You release the %s."), veh->name.c_str());
        } else if (m.has_furn(u.pos() + u.grab_point)) {
            add_msg(_("You release the %s."), m.furnname(u.pos() + u.grab_point).c_str());
        }

        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
        return;
    }

    if( choose_adjacent(_("Grab where?"), grabp ) ) {
        if( grabp == u.pos() ) {
            add_msg( _("You get a hold of yourself.") );
            u.grab_point = tripoint_zero;
            u.grab_type = OBJECT_NONE;
            return;
        }

        vehicle *veh = m.veh_at( grabp );
        if( veh != nullptr ) { // If there's a vehicle, grab that.
            u.grab_point = grabp - u.pos();
            u.grab_type = OBJECT_VEHICLE;
            add_msg(_("You grab the %s."), veh->name.c_str());
        } else if( m.has_furn( grabp ) ) { // If not, grab furniture if present
            if( m.furn( grabp ).obj().move_str_req < 0 ) {
                add_msg(_("You can not grab the %s"), m.furnname( grabp ).c_str());
                return;
            }
            u.grab_point = grabp - u.pos();
            u.grab_type = OBJECT_FURNITURE;
            if (!m.can_move_furniture( grabp, &u )) {
                add_msg(_("You grab the %s. It feels really heavy."), m.furnname( grabp ).c_str());
            } else {
                add_msg(_("You grab the %s."), m.furnname( grabp ).c_str());
            }
        } else { // todo: grab mob? Captured squirrel = pet (or meat that stays fresh longer).
            add_msg(m_info, _("There's nothing to grab there!"));
        }
    } else {
        add_msg(_("Never mind."));
    }
}

std::vector<vehicle*> nearby_vehicles_for( const itype_id &ft )
{
    std::vector<vehicle*> result;
    for( auto && p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        vehicle * const veh = g->m.veh_at( p );
        // TODO: constify fuel_left and fuel_capacity
        // TODO: add a fuel_capacity_left function
        if( std::find( result.begin(), result.end(), veh ) != result.end() ) {
            continue;
        }
        if( veh != nullptr && veh->fuel_left( ft ) < veh->fuel_capacity( ft ) ) {
            result.push_back( veh );
        }
    }
    return result;
}

void game::handle_all_liquid( item liquid, const int radius )
{
    while( liquid.charges > 0l ) {
        // handle_liquid allows to pour onto the ground, which will handle all the liquid and
        // set charges to 0. This allows terminating the loop.
        // The result of handle_liquid is ignored, the player *has* to handle all the liquid.
        handle_liquid( liquid, nullptr, radius );
    }
}

bool game::consume_liquid( item &liquid, const int radius )
{
    const auto original_charges = liquid.charges;
    while( liquid.charges > 0 && handle_liquid( liquid, nullptr, radius ) ) {
        // try again with the remaining charges
    }
    return original_charges != liquid.charges;
}

bool game::handle_liquid_from_ground( std::list<item>::iterator on_ground, const tripoint &pos, const int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    handle_liquid( *on_ground, nullptr, radius, &pos );
    if( on_ground->charges > 0 ) {
        return false;
    }
    m.i_at( pos ).erase( on_ground );
    return true;
}

bool game::handle_liquid_from_container( std::list<item>::iterator in_container, item &container, int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    const long old_charges = in_container->charges;
    handle_liquid( *in_container, &container, radius );
    if( in_container->charges != old_charges ) {
        container.on_contents_changed();
    }

    if( in_container->charges > 0 ) {
        return false;
    }
    container.contents.erase( in_container );
    return true;
}

bool game::handle_liquid_from_container( item &container, int radius )
{
    return handle_liquid_from_container( container.contents.begin(), container, radius );
}

extern void serialize_liquid_source( player_activity &act, const vehicle &veh, const itype_id &ftype );
extern void serialize_liquid_source( player_activity &act, const tripoint &pos, const item &liquid );

extern void serialize_liquid_target( player_activity &act, const vehicle &veh );
extern void serialize_liquid_target( player_activity &act, int container_item_pos );
extern void serialize_liquid_target( player_activity &act, const tripoint &pos );

bool game::handle_liquid( item &liquid, item * const source, const int radius,
                          const tripoint * const source_pos,
                          const vehicle * const source_veh )
{
    if( !liquid.made_of(LIQUID) ) {
        dbg(D_ERROR) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg("Tried to handle_liquid a non-liquid!");
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    const auto create_activity = [&]() {
        if( source_veh != nullptr ) {
            u.assign_activity( ACT_FILL_LIQUID, INT_MAX );
            serialize_liquid_source( u.activity, *source_veh, liquid.typeId() );
            return true;
        } else if( source_pos != nullptr ) {
            u.assign_activity( ACT_FILL_LIQUID, INT_MAX );
            serialize_liquid_source( u.activity, *source_pos, liquid );
            return true;
        } else {
            return false;
        }
    };

    const std::string liquid_name = liquid.has_infinite_charges() ? liquid.tname() : liquid.display_name( liquid.charges );

    uimenu menu;
    menu.return_invalid = true;
    if( source_pos != nullptr ) {
        menu.text = string_format( _( "What to do with the %s from %s?" ), liquid_name.c_str(), m.name( *source_pos ).c_str() );
    } else if( source_veh != nullptr ) {
        menu.text = string_format( _( "What to do with the %s from the %s?" ), liquid_name.c_str(), source_veh->name.c_str() );
    } else {
        menu.text = string_format( _( "What to do with the %s?" ), liquid_name.c_str() );
    }
    std::vector<std::function<void()>> actions;

    if( liquid.is_food( &u ) ) {
        menu.addentry( -1, true, 'e', _( "Consume it" ) );
        actions.emplace_back( [&]() {
            // consume_item already consumes moves.
            u.consume_item( liquid );
        } );
    }

    menu.addentry( -1, true, 'c', _( "Pour into a container" ) );
    actions.emplace_back( [&]() {
        const std::string text = string_format( _( "Container for %s" ), liquid_name.c_str() );
        item * const cont = inv_map_for_liquid( liquid, text, radius );

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        if( cont == source && source != nullptr ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        const int item_index = u.get_item_position( cont );
        // Currently activities can only store item position in the players inventory,
        // not on ground or similar. TODO: implement storing arbitrary container locations.
        if( item_index != INT_MIN && create_activity() ) {
            serialize_liquid_target( u.activity, item_index );
        } else if( u.pour_into( *cont, liquid ) ) {
            u.mod_moves( -100 );
        }
    } );

    std::set<vehicle *> opts;
    for( const auto &e : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        auto veh = g->m.veh_at( e );
        if( veh && std::any_of( veh->parts.begin(), veh->parts.end(), [&liquid]( const vehicle_part &pt ) {
            // cannot refill using active liquids (those that rot) due to #18570
            return !liquid.active && pt.can_reload( liquid.typeId() );
        } ) ) {
            opts.insert( veh );
        }
    }
    for( auto veh : opts ) {
        if( veh == source_veh ) {
            continue;
        }
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill nearby vehicle %s" ), veh->name.c_str() );
        actions.emplace_back( [&, veh]() {
            if( create_activity() ) {
                serialize_liquid_target( u.activity, *veh );
            } else if( u.pour_into( *veh, liquid ) ) {
                u.mod_moves( -1000 ); // consistent with veh_interact::do_refill activity
            }
        } );
    }

    for( auto &target_pos : m.points_in_radius( u.pos(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        if( source_pos != nullptr && *source_pos == target_pos ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( u.pos(), target_pos ) );
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Pour into an adjacent keg (%s)" ), dir.c_str() );
        actions.emplace_back( [&, target_pos]() {
            if( create_activity() ) {
                serialize_liquid_target( u.activity, target_pos );
            } else {
                iexamine::pour_into_keg( target_pos, liquid );
                u.mod_moves( -100 );
            }
        } );
    }

    menu.addentry( -1, true, 'g', _( "Pour on the ground" ) );
    actions.emplace_back( [&]() {
        // From infinite source to the ground somewhere else. The target has
        // infinite space and the liquid can not be used from there anyway.
        if( liquid.has_infinite_charges() && source_pos != nullptr ) {
            add_msg( m_info, _( "Clearing out the %s would take forever." ), m.name( *source_pos ).c_str() );
            return;
        }

        tripoint target_pos = u.pos();
        const std::string liqstr = string_format( _( "Pour %s where?" ), liquid_name.c_str() );

        refresh_all();
        if( !choose_adjacent( liqstr, target_pos ) ) {
            return;
        }

        if( source_pos != nullptr && *source_pos == target_pos ) {
            add_msg( m_info, _( "That's where you took it from!" ) );
            return;
        }
        if( !m.can_put_items_ter_furn( target_pos ) ) {
            add_msg( m_info, _( "You can't pour there!" ) );
            return;
        }

        if( create_activity() ) {
            serialize_liquid_target( u.activity, target_pos );
        } else {
            m.add_item_or_charges( target_pos, liquid, 1 );
            liquid.charges = 0;
            u.mod_moves( -100 );
        }
    } );
    if( liquid.rotten() ) {
        // Pre-select this one as it is the most likely one for rotten liquids
        menu.selected = menu.entries.size() - 1;
    }

    if( menu.entries.empty() ) {
        return false;
    }

    menu.query();
    refresh_all();
    const size_t chosen = static_cast<size_t>( menu.ret );
    if( chosen >= actions.size() ) {
        add_msg( _( "Never mind." ) );
        // Explicitly canceled all options (container, drink, pour).
        return false;
    }

    actions[chosen]();
    return true;
}

void game::drop( int pos, const tripoint &where )
{
    if( pos != INT_MIN ) {
        u.drop( pos, where );
    } else {
        u.drop( multidrop(), where );
    }
}

void game::drop_in_direction()
{
    tripoint dirp;

    if( choose_adjacent( _( "Drop where?" ), dirp ) ) {
        refresh_all();
        drop( INT_MIN, dirp );
    }
}

void game::reassign_item( int pos )
{
    if( pos == INT_MIN ) {
        pos = inv_for_all( _( "Reassign item:" ) );
    }
    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item &change_from = u.i_at( pos );
    if( change_from.is_null() ) {
        return;
    }
    long newch = popup_getkey( _( "%s; enter new letter (press SPACE for none, ESCAPE to cancel)." ),
                               change_from.tname().c_str() );
    if( newch == KEY_ESCAPE ) {
        add_msg( m_neutral, _( "Never mind." ) );
        return;
    }
    if( newch == ' ' ) {
        newch = 0;
    } else if( !inv_chars.valid( newch ) ) {
        add_msg( m_info, _("Invalid inventory letter. Only those characters are valid:\n\n%s"),
                 inv_chars.get_allowed_chars().c_str() );
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
    if (u.has_active_mutation("SHELL2")) {
        add_msg(m_info, _("You can't effectively throw while you're in your shell."));
        return;
    }

    if (pos == INT_MIN) {
        pos = inv_for_all( _( "Throw item:" ), _( "You don't have any items to throw." ) );
        refresh_all();
    }

    item thrown = u.i_at(pos);
    int range = u.throw_range( thrown );
    if (range < 0) {
        add_msg(m_info, _("You don't have that item."));
        return;
    } else if (range == 0) {
        add_msg(m_info, _("That is too heavy to throw."));
        return;
    }

    if (pos == -1 && thrown.has_flag("NO_UNWIELD")) {
        // pos == -1 is the weapon, NO_UNWIELD is used for bio_claws_weapon
        add_msg(m_info, _("That's part of your body, you can't throw that!"));
        return;
    }

    if (u.has_effect( effect_relax_gas)) {
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
    m.draw( w_terrain, u.pos() );

    // pl_target_ui() sets x and y, or returns empty vector if we canceled (by pressing Esc)
    std::vector<tripoint> trajectory = pl_target_ui( TARGET_MODE_THROW, &thrown, range );
    if (trajectory.empty()) {
        return;
    }

    if (u.is_worn(u.i_at(pos))) {
        thrown.on_takeoff(u);
    }

    // Throw a single charge of a stacking object.
    if( thrown.count_by_charges() && thrown.charges > 1 ) {
        u.i_at( pos ).charges--;
        thrown.charges = 1;
    } else {
        u.i_rem( pos );
    }

    u.throw_item( trajectory.back(), thrown );
    reenter_fullscreen();
}

bool game::plfire()
{
    if( u.has_effect( effect_relax_gas) ) {
        if( one_in(5) ) {
            add_msg(m_good, _("Your eyes steel, and you raise your weapon!"));
        } else {
            u.moves -= rng(2, 5) * 10;
            add_msg(m_bad, _("You can't fire your weapon, it's too heavy..."));
            return false;
        }
    }

    if( u.weapon.is_gunmod() ) {
        add_msg( m_info, _( "The %s must be attached to a gun, it can not be fired separately." ), u.weapon.tname().c_str() );
        return false;
    }

    auto gun = u.weapon.gun_current_mode();

    if( !( gun && u.can_use( *gun ) ) ) {
        return false; // check a valid mode was returned and we are able to use it
    }

    vehicle *veh = m.veh_at(u.pos());
    if( veh != nullptr && veh->player_in_control( u ) && gun->is_two_handed( u ) ) {
        add_msg(m_info, _("You need a free arm to drive!"));
        return false;
    }

    int reload_time = 0;
    if( !gun.melee() ) {
        if( gun->has_flag( "FIRE_TWOHAND" ) && ( !u.has_two_arms() || u.worn_with_flag( "RESTRICT_HANDS" ) ) ) {
            add_msg( m_info, _( "You need two free hands to fire your %s." ), gun->tname().c_str() );
            return false;
        }

        if( gun->has_flag( "RELOAD_AND_SHOOT" ) && !gun->ammo_remaining() ) {
            item::reload_option opt = u.select_ammo( *gun );
            if( !opt ) {
                return false; // menu cancelled
            }

            reload_time += opt.moves();
            if( !gun->reload( u, std::move( opt.ammo ), 1 ) ) {
                return false; // unable to reload
            }

            // Burn 2x the strength required to fire in stamina.
            u.mod_stat( "stamina", gun->type->min_str * -2 );

            // At low stamina levels, firing starts getting slow.
            int sta_percent = (100 * u.stamina) / u.get_stamina_max();
            reload_time += ( sta_percent < 25 ) ? ( ( 25 - sta_percent ) * 2 ) : 0;

            refresh_all();
        }

        if( !gun->ammo_sufficient() && !gun->has_flag("RELOAD_AND_SHOOT") ) {
            if( !gun->ammo_remaining() ) {
                add_msg(m_info, _("You need to reload!"));
            } else {
                add_msg( m_info, _( "Your %s needs %i charges to fire!" ), gun->tname().c_str(), gun->ammo_required() );
            }
            return false;
        }

        if( gun->get_gun_ups_drain() > 0 ) {
            const int ups_drain       = gun->get_gun_ups_drain();
            const int adv_ups_drain   = std::max( 1, ups_drain * 3 / 5 );

            if( !( u.has_charges( "UPS_off", ups_drain ) ||
                   u.has_charges( "adv_UPS_off", adv_ups_drain ) ||
                   (u.has_active_bionic( "bio_ups" ) && u.power_level >= ups_drain ) ) ) {
                add_msg( m_info,
                         _("You need a UPS with at least %d charges or an advanced UPS with at least %d charges to fire that!"),
                         ups_drain, adv_ups_drain );
                return false;
            }
        }

        if( gun->has_flag( "MOUNTED_GUN" ) ) {
            int vpart = -1;
            vehicle *veh = m.veh_at( u.pos(), vpart );
            if( !m.has_flag_ter_or_furn( "MOUNTABLE", u.pos() ) &&
                (veh == NULL || veh->part_with_feature(vpart, "MOUNTABLE") < 0)) {
                add_msg(m_info,
                        _("You need to be standing near acceptable terrain or furniture to use this weapon. A table, a mound of dirt, a broken window, etc."));
                return false;
            }
        }
    }

    int range = gun.melee() ? gun.qty : u.gun_engagement_range( *gun, player::engagement::maximum );

    temp_exit_fullscreen();
    m.draw( w_terrain, u.pos() );

    target_mode tmode = gun.melee() ? TARGET_MODE_REACH : TARGET_MODE_FIRE;
    std::vector<tripoint> trajectory = pl_target_ui( tmode, &u.weapon, range );

    if (trajectory.empty()) {
        if( gun->has_flag( "RELOAD_AND_SHOOT" ) && u.activity.type != ACT_AIM ) {
            const auto previous_moves = u.moves;
            unload( *gun );
            // Give back time for unloading as essentially nothing has been done at all.
            // Note that reload_time has not been applied either.
            u.moves = previous_moves;
        }
        reenter_fullscreen();
        return false;
    }
    draw_ter(); // Recenter our view

    bool res = false;

    if( gun.melee() ) {
        u.reach_attack( trajectory.back() );
        res = true;

    } else {
        u.moves -= reload_time;
        // @todo add check for TRIGGERHAPPY
        res = u.fire_gun( trajectory.back(), gun.qty, *gun );
    }

    reenter_fullscreen();
    return res;
}

// Helper for game::butcher
void add_corpses_to_menu( uimenu &kmenu, map_stack &items,
    const std::vector<int> &indices, size_t &menu_index,
    bool salvage )
{
    for( size_t index : indices ) {
        const item &it = items[index];
        int hotkey = -1;
        // First entry gets a hotkey matching the butcher command.
        if( menu_index == 0 ) {
            const long butcher_key = inp_mngr.get_previously_pressed_key();
            if( butcher_key != 0 ) {
                hotkey = butcher_key;
            }
        }
        if( it.is_corpse() ) {
            kmenu.addentry( menu_index++, true, hotkey, it.get_mtype()->nname() );
        } else if( !salvage ) {
            kmenu.addentry( menu_index++, true, hotkey, it.tname());
        } else {
            std::stringstream ss;
            ss << _("Cut up") << " " << it.tname();
            kmenu.addentry( menu_index++, true, hotkey, ss.str() );
        }
    }
}

void game::butcher()
{
    const static std::string salvage_string = "salvage";
    if (u.controlling_vehicle) {
        add_msg(m_info, _("You can't butcher while driving!"));
        return;
    }

    const int factor = u.max_quality( quality_id( "BUTCHER" ) );
    static const char *no_knife_msg = _( "You don't have a butchering tool." );
    static const char *no_corpse_msg = _( "There are no corpses here to butcher." );

    //You can't butcher on sealed terrain- you have to smash/shovel/etc it open first
    if( m.has_flag( "SEALED", u.pos() ) ) {
        if( m.sees_some_items( u.pos(), u ) ) {
            add_msg( m_info, _( "You can't access the items here." ) );
        } else if( factor > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }
        return;
    }

    const item *first_item_without_tools = nullptr;
    // Indices of relevant items
    std::vector<int> corpses;
    std::vector<int> disassembles;
    std::vector<int> salvageables;
    auto items = m.i_at(u.pos());
    const inventory &crafting_inv = u.crafting_inventory();

    // TODO: Properly handle different material whitelists
    // TODO: Improve quality of this section
    auto salvage_filter = []( item it ) {
        const auto usable = it.get_usable_item( salvage_string );
        return usable != nullptr;
    };

    std::vector< item * > salvage_tools = u.items_with( salvage_filter );
    int salvage_tool_index = INT_MIN;
    item *salvage_tool = nullptr;
    const salvage_actor *salvage_iuse = nullptr;
    if( !salvage_tools.empty() ) {
        salvage_tool = salvage_tools.front();
        salvage_tool_index = u.get_item_position( salvage_tool );
        item *usable = salvage_tool->get_usable_item( salvage_string );
        salvage_iuse = dynamic_cast<const salvage_actor*>(
            usable->get_use( salvage_string )->get_actor_ptr() );
    }


    // check if we have a butchering tool
    if( factor > INT_MIN ) {
        // get corpses
        for (size_t i = 0; i < items.size(); i++) {
            if( items[i].is_corpse() ) {
                corpses.push_back(i);
            }
        }
    }
    // Then get items to disassemble
    for( size_t i = 0; i < items.size(); i++ ) {
        if( items[i].is_corpse() ) {
            continue;
        }

        if( u.can_disassemble( items[i], crafting_inv ) ) {
            disassembles.push_back(i);
        } else if( first_item_without_tools == nullptr ) {
            first_item_without_tools = &items[i];
        }
    }
    // Now salvageable items
    if( salvage_tool_index != INT_MIN ) {
        for( size_t i = 0; i < items.size(); i++ ) {
            if( !items[i].is_corpse() &&
                salvage_iuse->valid_to_cut_up( &items[i] ) ) {
                    salvageables.push_back(i);
            }
        }
    }

    if( corpses.empty() && disassembles.empty() && salvageables.empty() ) {
        if( factor > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }

        if( first_item_without_tools != nullptr ) {
            add_msg( m_info, _("You don't have the necessary tools to disassemble any items here.") );
            // Just for the "You need x to disassemble y" messages
            u.can_disassemble( *first_item_without_tools, crafting_inv, true );
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

    // Magic indices for special butcher options
    enum : int {
        MULTISALVAGE =  MAX_ITEM_IN_SQUARE + 1,
        MULTIBUTCHER,
        MULTIDISASSEMBLE_ONE,
        MULTIDISASSEMBLE_ALL,
        CANCEL
    };
    // What are we butchering (ie. which vector to pick indices from)
    enum {
        BUTCHER_CORPSE,
        BUTCHER_DISASSEMBLE,
        BUTCHER_SALVAGE,
        BUTCHER_OTHER // For multisalvage etc.
    } butcher_type = BUTCHER_CORPSE;
    // Index to std::vector of indices...
    int indexer_index = 0;
    // Always ask before cutting up/disassembly, but not before butchery
    if( corpses.size() > 1 || !disassembles.empty() || !salvageables.empty() ) {
        uimenu kmenu;
        kmenu.text = _("Choose corpse to butcher / item to disassemble");

        kmenu.selected = 0;
        size_t i = 0;
        add_corpses_to_menu( kmenu, items, corpses, i, false );
        add_corpses_to_menu( kmenu, items, disassembles, i, false );
        add_corpses_to_menu( kmenu, items, salvageables, i, true );

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b',
                _("Butcher everything") );
        }
        if( disassembles.size() > 1 ) {
            kmenu.addentry( MULTIDISASSEMBLE_ONE, true, 'D',
                _("Disassemble everything once") );
            kmenu.addentry( MULTIDISASSEMBLE_ALL, true, 'd',
                _("Disassemble everything") );
        }
        if( salvageables.size() > 1 ) {
            kmenu.addentry( MULTISALVAGE, true, 'z', _("Cut up all you can") );
        }

        kmenu.addentry( CANCEL, true, 'q', _("Cancel"));
        kmenu.return_invalid = true;
        kmenu.query();
        if( kmenu.ret < 0 || kmenu.ret >= CANCEL ) {
            return;
        }

        size_t ret = (size_t)kmenu.ret;
        if( ret >= MULTISALVAGE && ret < CANCEL ) {
            butcher_type = BUTCHER_OTHER;
            indexer_index = ret;
        } else if( ret < corpses.size() ) {
            butcher_type = BUTCHER_CORPSE;
            indexer_index = ret;
        } else if( ret < corpses.size() + disassembles.size() ) {
            butcher_type = BUTCHER_DISASSEMBLE;
            indexer_index = ret - corpses.size();
        } else if( ret < corpses.size() + disassembles.size() + salvageables.size() ) {
            butcher_type = BUTCHER_SALVAGE;
            indexer_index = ret - corpses.size() - disassembles.size();
        } else {
            debugmsg( "Invalid butchery index: %d", ret );
            return;
        }
    }

    switch( butcher_type ) {
    case BUTCHER_OTHER:
        switch( indexer_index ) {
        case MULTISALVAGE:
            u.assign_activity( ACT_LONGSALVAGE, 0, salvage_tool_index );
            break;
        case MULTIBUTCHER:
            u.assign_activity( ACT_BUTCHER, 0, -1 );
            for( int i : corpses ) {
                u.activity.values.push_back( i );
            }
            break;
        case MULTIDISASSEMBLE_ONE:
            u.disassemble_all( true );
            break;
        case MULTIDISASSEMBLE_ALL:
            u.disassemble_all( false );
            break;
        default:
            debugmsg("Invalid butchery type: %d", indexer_index );
            return;
        }
        break;
    case BUTCHER_CORPSE:
        {
            draw_ter();
            int index = corpses[indexer_index];
            u.assign_activity( ACT_BUTCHER, 0, -1 );
            u.activity.values.push_back( index );
        }
        break;
    case BUTCHER_DISASSEMBLE:
        {
            size_t index = disassembles[indexer_index];
            u.disassemble( items[index], index, true );
        }
        break;
    case BUTCHER_SALVAGE:
        {
            size_t index = salvageables[indexer_index];
            salvage_iuse->cut_up( &u, salvage_tool, &items[index] );
        }
        break;
    }
}

void game::eat(int pos)
{
    if ((u.has_active_mutation("RUMINANT") || u.has_active_mutation("GRAZER")) &&
        m.ter(u.pos()) == t_underbrush && query_yn(_("Eat underbrush?"))) {
        u.moves -= 400;
        u.mod_hunger(-10);
        m.ter_set(u.pos(), t_grass);
        add_msg(_("You eat the underbrush."));
        return;
    }
    if (u.has_active_mutation("GRAZER") && m.ter(u.pos()) == t_grass &&
        query_yn(_("Graze?"))) {
        u.moves -= 400;
        ///\EFFECT_INT increases chance of leaving some grass growing when grazing
        if ((u.get_hunger() < 10) || one_in(20 - u.int_cur)) {
            add_msg(_("You eat some of the taller grass, careful to leave some growing."));
            u.mod_hunger(-2);
        } else {
            add_msg(_("You eat the grass."));
            u.mod_hunger(-5);
            m.ter_set(u.pos(), t_dirt);
        }
        return;
    }

    if( pos != INT_MIN ) {
        u.consume(pos);
        return;
    }

    // Can consume items from inventory or within one tile (including in vehicles)
    auto item_loc = inv_map_splice( [&]( const item &it ) {
        if( it.typeId() == "1st_aid" ) {
            return false; // temporary fix for #12991
        }
        return it.made_of( SOLID ) && (it.is_food( &u ) || it.is_food_container( &u ) );
    }, _( "Consume item:" ), 1, _( "You have nothing to consume." ) );

    item *it = item_loc.get_item();
    if( !it ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    pos = u.get_item_position( it );
    if( pos != INT_MIN ) {
        u.consume( pos );

    } else if( u.consume_item( *it ) ) {
        if( it->is_food_container() ) {
            it->contents.erase( it->contents.begin() );
            add_msg( _("You leave the empty %s."), it->tname().c_str() );
        } else {
            item_loc.remove_item();
        }
    }
}

void game::wear(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_for_unequipped( _( "Wear item:" ) );
    }

    u.wear(pos);
}

void game::takeoff(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_for_equipped( _( "Take off item:" ) );
    }
    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }
    u.takeoff( pos );
}

void game::change_side(int pos)
{
    if (pos == INT_MIN) {
        pos = inv_for_filter( _( "Change side for item:" ), [&]( const item &it ) {
            return u.is_worn(it) && it.is_sided();
        }, _( "You don't have sided items worn." ) );
    }
    if (pos == INT_MIN) {
        add_msg(_("Never mind."));
        return;
    }
    u.change_side(pos);
}

void game::reload( int pos, bool prompt )
{
    item *it = &u.i_at( pos );

    // bows etc do not need to reload.
    if( it->has_flag( "RELOAD_AND_SHOOT" ) ) {
        add_msg( m_info, _( "The %s does not need to be reloaded, it reloads and fires in a single motion." ),
                 it->tname().c_str() );
        return;
    }

    // for holsters and ammo pouches try to reload any contained item
    if( it->type->can_use( "holster" ) && !it->contents.empty() ) {
        it = &it->contents.front();
    }

    switch( u.rate_action_reload( *it ) ) {
        case HINT_IFFY:
            if( it->ammo_remaining() > 0 && it->ammo_remaining() == it->ammo_capacity() ) {
                add_msg( m_info, _( "The %s is already fully loaded!" ), it->tname().c_str() );
                return;
            }
            if( it->is_ammo_belt() ) {
                auto linkage = it->type->magazine->linkage;
                if( linkage != "NULL" && !g->u.has_charges( linkage, 1 ) ) {
                    add_msg( m_info, _( "You need at least one %s to reload the %s!" ),
                                     item::nname( linkage, 1 ).c_str(), it->tname().c_str() );
                    return;
                }
            }
            // intentional fall-through

        case HINT_CANT:
            add_msg( m_info, _( "You can't reload a %s!." ), it->tname().c_str() );
            return;

        case HINT_GOOD:
            break;
    }

    // for bandoliers we currently defer to iuse_actor methods
    if( it->is_bandolier() ) {
        auto ptr = dynamic_cast<const bandolier_actor *>( it->type->get_use( "bandolier" )->get_actor_ptr() );
        ptr->reload( u, *it );
        return;
    }

    item::reload_option opt = u.select_ammo( *it, prompt );
    if( opt ) {
        u.assign_activity( ACT_RELOAD, opt.moves(), opt.qty() );
        u.activity.targets.emplace_back( u, const_cast<item *>( opt.target ) );
        u.activity.targets.push_back( std::move( opt.ammo ) );
    }

    refresh_all();
}

void game::reload()
{
    if( !u.is_armed() ) {

        vehicle *veh = m.veh_at( u.pos() );
        turret_data turret;
        if( veh && ( turret = veh->turret_query( u.pos() ) ) && turret.can_reload() ) {
            item::reload_option opt = g->u.select_ammo( *turret.base(), true );
            if( opt ) {
                g->u.assign_activity( ACT_RELOAD, opt.moves(), opt.qty() );
                g->u.activity.targets.emplace_back( turret.base() );
                g->u.activity.targets.push_back( std::move( opt.ammo ) );
            }
            return;
        }

        add_msg(m_info, _( "You're not wielding anything." ) );
    } else {
        reload( -1 );
    }
}

// Unload a container, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload(int pos)
{
    item *it = nullptr;

    if( pos == INT_MIN ) {
        it = inv_map_splice( [&]( const item &it ) {
            return u.rate_action_unload( it ) == HINT_GOOD;
        }, _( "Unload item:" ), 1, _( "You have nothing to unload." ) ).get_item();

        if( it == nullptr ) {
            add_msg( _("Never mind.") );
            return;
        }
    } else {
        it = &u.i_at( pos );
        if( it->is_null() ) {
            debugmsg( "Tried to unload non-existent item" );
            return;
        }
    }

    if( unload( *it ) ) {
        if( it->has_flag( "MAG_DESTROY" ) && it->ammo_remaining() == 0 ) {
            u.remove_item( *it );
        }
    }
}

void game::mend( int pos )
{
    if( pos == INT_MIN ) {
        if( u.is_armed() ) {
            pos = -1;
        } else {
            add_msg(m_info, _( "You're not wielding anything." ) );
        }
    }

    item& obj = g->u.i_at( pos );
    if( g->u.has_item( obj ) ) {
        g->u.mend_item( item_location( g->u, &obj ) );
    }
}

bool add_or_drop_with_msg( player &u, item &it )
{
    if( it.made_of( LIQUID ) ) {
        g->consume_liquid( it, 1 );
        return it.charges <= 0;
    }
    if( !u.can_pickVolume( it ) ) {
        add_msg( _( "There's no room in your inventory for the %s, so you drop it." ),
                 it.tname().c_str() );
        g->m.add_item_or_charges( u.pos(), it );
    } else if( !u.can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
        add_msg( _( "The %s is too heavy to carry, so you drop it." ), it.tname().c_str() );
        g->m.add_item_or_charges( u.pos(), it );
    } else {
        auto &ni = u.i_add( it );
        add_msg( _( "You put the %s in your inventory." ), ni.tname().c_str() );
        add_msg( m_info, "%c - %s", ni.invlet == 0 ? ' ' : ni.invlet, ni.tname().c_str() );
    }
    return true;
}

bool game::unload( item &it )
{
    // Unload a container consuming moves per item successfully removed
    if( it.is_container() || it.is_bandolier() ) {
        if( it.contents.empty() ) {
            add_msg( m_info, _( "The %s is already empty!" ), it.tname().c_str() );
            return false;
        }

        it.contents.erase( std::remove_if( it.contents.begin(), it.contents.end(), [this]( item& e ) {
            int mv = u.item_handling_cost( e );
            if( !add_or_drop_with_msg( u, e ) ) {
                return false;
            }
            u.moves -= mv;
            return true;
        } ), it.contents.end() );
        it.on_contents_changed();
        return true;
    }

    // If item can be unloaded more than once (currently only guns) prompt user to choose
    std::vector<std::string> msgs( 1, it.tname() );
    std::vector<item *> opts( 1, &it );

    for( auto e : it.gunmods() ) {
        if( e->is_gun() && !e->has_flag( "NO_UNLOAD" ) &&
            ( e->magazine_current() || e->ammo_remaining() > 0 ) ) {
            msgs.emplace_back( e->tname() );
            opts.emplace_back( e );
        }
    }

    item *target = opts.size() > 1 ? opts[ ( uimenu( false, _("Unload what?"), msgs ) ) - 1 ] : &it;

    // Next check for any reasons why the item cannot be unloaded
    if( !target->ammo_type() || target->ammo_capacity() <= 0 ) {
        add_msg( m_info, _("You can't unload a %s!"), target->tname().c_str() );
        return false;
    }

    if( target->has_flag( "NO_UNLOAD" ) ) {
        if( target->has_flag( "RECHARGE" ) || target->has_flag( "USE_UPS" )) {
            add_msg( m_info, _( "You can't unload a rechargeable %s!" ), target->tname().c_str() );
        } else {
            add_msg( m_info, _( "You can't unload a %s!" ), target->tname().c_str() );
        }
        return false;
    }

    if( !target->magazine_current() && target->ammo_remaining() <= 0 && target->casings_count() <= 0 ) {
        if( target->is_tool() ) {
            add_msg( m_info, _( "Your %s isn't charged." ), target->tname().c_str() );
        } else {
            add_msg( m_info, _( "Your %s isn't loaded." ), target->tname().c_str() );
        }
        return false;
    }

    target->casings_handle( [&]( item &e ) {
        return u.i_add_or_drop( e );
    } );

    if( target->is_magazine() ) {
        // Remove all contained ammo consuming half as much time as required to load the magazine
        long qty = 0;
        target->contents.erase( std::remove_if( target->contents.begin(), target->contents.end(), [&]( item& e ) {
            int mv = u.item_reload_cost( *target, e, e.charges ) / 2;
            if( !add_or_drop_with_msg( u, e ) ) {
                return false;
            }
            qty += e.charges;
            u.moves -= mv;
            return true;
        } ), target->contents.end() );

        if( target->is_ammo_belt() ) {
            if( target->type->magazine->linkage != "NULL" ) {
                item link( target->type->magazine->linkage, calendar::turn, qty );
                add_or_drop_with_msg( u, link );
            }
            add_msg( _( "You disassemble your %s."), target->tname().c_str() );
        } else {
            add_msg( _( "You unload your %s." ), target->tname().c_str() );
        }
        return true;

    } else if( target->magazine_current() ) {
        if( !add_or_drop_with_msg( u, *target->magazine_current() ) ) {
            return false;
        }
        // Eject magazine consuming half as much time as required to insert it
        u.moves -= u.item_reload_cost( *target, *target->magazine_current(), -1 ) / 2;

        target->contents.erase( std::remove_if( target->contents.begin(), target->contents.end(), [&target]( const item& e ) {
            return target->magazine_current() == &e;
        } ) );

    } else if( target->ammo_remaining() ) {
        long qty = target->ammo_remaining();

        if( target->ammo_type() == ammotype( "plutonium" ) ) {
            qty = target->ammo_remaining() / PLUTONIUM_CHARGES;
            if( qty > 0 ) {
                add_msg( _( "You recover %i unused plutonium." ), qty );
            } else {
                add_msg( m_info, _( "You can't remove partially depleted plutonium!" ) );
                return false;
            }
        }

        // Construct a new ammo item and try to drop it
        item ammo( target->ammo_current(), calendar::turn, qty );

        if( ammo.made_of( LIQUID ) ) {
            if( !add_or_drop_with_msg( u, ammo ) ) {
                qty -= ammo.charges; // only handled part (or none) of the liquid
            }
            if( qty <= 0 ) {
                return false; // no liquid was moved
            }

        } else if( !add_or_drop_with_msg( u, ammo ) ) {
            return false;
        }

        // If successful remove appropriate qty of ammo consuming half as much time as required to load it
        u.moves -= u.item_reload_cost( *target, ammo, qty ) / 2;

        if( target->ammo_type() == ammotype( "plutonium" ) ) {
            qty *= PLUTONIUM_CHARGES;
        }

        target->ammo_set( target->ammo_current(), target->ammo_remaining() - qty );
    }

    // Turn off any active tools
    if( target->is_tool() && target->active && target->ammo_remaining() == 0 ) {
        target->type->invoke( &u, target, u.pos() );
    }

    add_msg( _( "You unload your %s." ), target->tname().c_str() );
    return true;
}

void game::wield( int pos )
{
    if( u.weapon.has_flag( "NO_UNWIELD" ) ) {
        // Bionics can't be unwielded
        add_msg( m_info, _( "You cannot unwield your %s." ), u.weapon.tname().c_str() );
        return;
    }
    if( pos == INT_MIN ) {
        pos = inv_for_all( _( "Wield item:" ), _( "You have nothing to wield." ) );
    }

    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    // Weapons need invlets to access, give one if not already assigned.
    item &it = u.i_at( pos );
    if( !it.is_null() && it.invlet == 0 ) {
        u.inv.assign_empty_invlet( it, true );
    }

    // If called for the current weapon then try unwielding it
    if( u.wield( pos == -1 ? u.ret_null : it ) ) {
        u.recoil = MIN_RECOIL;
    }
}

void game::read()
{
    // Can read items from inventory or within one tile (including in vehicles)
    auto loc = inv_map_splice( []( const item &it ) { return it.is_book(); }, _( "Read:" ), 1,
                                _( "You have nothing to read." ) );

    item *book = loc.get_item();
    if( !book ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    int pos = u.get_item_position( book );

    // Book is already in the inventory so attempt reading.
    if( pos != INT_MIN ) {
        u.read( pos );
        return;
    }

    if( u.volume_carried() + book->volume() > u.volume_capacity() ) {
        add_msg( _( "No space in inventory for %s" ), book->tname().c_str() );
        return;
    }

    // Add book to inventory and attempt to read it.
    pos = u.get_item_position( &u.i_add( *book ) );
    // At this point there is one copy of the book on the ground and one in the inventory.
    if( u.read( pos ) ) {
        // We started reading so remove book from map or vehicle tile.
        loc.remove_item();
    } else {
        // We failed so undo adding the book to the inventory.
        u.i_rem( pos );
    }
}

void game::chat()
{
    std::vector<npc *> available;
    for( auto &elem : active_npc ) {
        if( u.sees( elem->pos() ) &&
            rl_dist( u.pos(), elem->pos() ) <= 24 ) {
            available.push_back( elem );
        }
    }

    uimenu nmenu;
    nmenu.text = std::string( _("Who do you want to talk to?") );

    int i = 0;
    for( auto &elem : available ) {
        nmenu.addentry( i++, true, MENU_AUTOASSIGN, ( elem )->name );
    }

    nmenu.return_invalid = true;
    nmenu.addentry( i++, true, 'a', _( "Yell" ) );
    nmenu.addentry( i++, true, 'q', _( "Cancel" ) );

    nmenu.query();
    if( nmenu.ret < 0 || nmenu.ret > (int)available.size() ) {
        return;
    } else if( nmenu.ret == (int)available.size() ) {
        u.shout();
    } else {
        available[nmenu.ret]->talk_to_u();
    }

    u.moves -= 100;
    refresh_all();
}

void game::pldrive(int x, int y)
{
    if( !check_safe_mode_allowed() ) {
        return;
    }
    vehicle *veh = remoteveh();
    bool remote = true;
    int part = -1;
    if( !veh ) {
        veh = m.veh_at(u.pos(), part);
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
        if ( veh->all_parts_with_feature( "REMOTE_CONTROLS", true ).empty() ) {
            add_msg(m_info, _("Can't drive this vehicle remotely. It has no working controls."));
            return;
        }
    }

    int turn_delta = 15 * x;
    const float handling_diff = veh->handling_difficulty();
    if( turn_delta != 0 ) {
        float eff = veh->steering_effectiveness();
        if( eff < 0 ) {
            add_msg( m_info, _("This vehicle has no steering system installed, you can't turn it.") );
            return;
        }

        if( eff == 0 ) {
            add_msg( m_bad, _("The steering is completely broken!") );
            return;
        }

        // If you've got more moves than speed, it's most likely time stop
        // Let's get rid of that
        u.moves = std::min( u.moves, u.get_speed() );

        ///\EFFECT_DEX reduces chance of losing control of vehicle when turning

        ///\EFFECT_PER reduces chance of losing control of vehicle when turning

        ///\EFFECT_DRIVING reduces chance of losing control of vehicle when turning
        float skill = std::min( 10.0f, u.get_skill_level( skill_driving ) + ( u.get_dex() + u.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;
        int cost;
        if( penalty > 0.0f ) {
            // At 10 penalty (rather hard to get), we're taking 4 turns per turn
            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {
            // At 10 skill, with a perfect vehicle, we could turn up to 3 times per turn
            cost = std::max( u.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }

        if( penalty > skill || cost > 400 ) {
            add_msg( m_warning, _("You fumble with the %s's controls."), veh->name.c_str() );
            // Anything from a wasted attempt to 2 turns in the intended direction
            turn_delta *= rng( 0, 2 );
            // Also wastes next turn
            cost = std::max( cost, u.moves + 100 );
        } else if( one_in( 10 ) ) {
            // Don't warn all the time or it gets spammy
            if( cost >= u.get_speed() * 2 ) {
                add_msg( m_warning, _( "It takes you a very long time to steer that vehicle!" ) );
            } else if( cost >= u.get_speed() * 1.5f ) {
                add_msg( m_warning, _( "It takes you a long time to steer that vehicle!" ) );
            }
        }

        veh->turn( turn_delta );

        // At most 3 turns per turn, because otherwise it looks really weird and jumpy
        u.moves -= std::max( cost, u.get_speed() / 3 + 1 );
    }

    if( y != 0 ) {
        int thr_amount = 10 * 100;
        if( veh->cruise_on ) {
            veh->cruise_thrust( -y * thr_amount );
        } else {
            veh->thrust( -y );
            u.moves = std::min( u.moves, 0 );
        }
    }

    // @todo Actually check if we're on land on water (or disable water-skidding)
    if( veh->skidding && ( veh->valid_wheel_config( false ) || veh->valid_wheel_config( true ) ) ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) < u.dex_cur + u.get_skill_level( skill_driving ) * 2 ) {
            add_msg(_("You regain control of the %s."), veh->name.c_str());
            u.practice( skill_driving, veh->velocity / 5 );
            veh->velocity = int(veh->forward_velocity());
            veh->skidding = false;
            veh->move.init(veh->turn_dir);
        }
    }
}

bool game::check_safe_mode_allowed( bool repeat_safe_mode_warnings )
{
    if ( !repeat_safe_mode_warnings && safe_mode_warning_logged ) {
        // Already warned player since safe_mode_warning_logged is set.
        return false;
    }

    std::string msg_ignore = press_x(ACTION_IGNORE_ENEMY);
    if (!msg_ignore.empty()) {
        msg_ignore[0] = tolower(msg_ignore[0]); // TODO this probably isn't localization friendly
    }

    if (u.has_effect( effect_laserlocked)) {
        // Automatic and mandatory safemode.  Make BLOODY sure the player notices!
        add_msg(m_warning, _("You are being laser-targeted, %s to ignore."),
                msg_ignore.c_str());
        safe_mode_warning_logged = true;
        return false;
    }
    if( safe_mode != SAFE_MODE_STOP ) {
        return true;
    }
    // Currently driving around, ignore the monster, they have no chance against a proper car anyway (-:
    if( u.controlling_vehicle && !get_option<bool>( "SAFEMODEVEH" ) ) {
        return true;
    }
    // Monsters around and we don't wanna run
    std::string spotted_creature_name;
    if( new_seen_mon.empty() ) {
        // naming consistent with code in game::mon_info
        spotted_creature_name = _( "a survivor" );
        get_safemode().lastmon_whitelist = get_safemode().npc_type_name();
    } else {
        spotted_creature_name = zombie( new_seen_mon.back() ).name();
        get_safemode().lastmon_whitelist = spotted_creature_name;
    }

    std::string whitelist = "";
    if ( !get_safemode().empty() ) {
        whitelist = string_format( _( " or %s to whitelist the monster" ), press_x( ACTION_WHITELIST_ENEMY ).c_str() );
    }

    std::string const msg_safe_mode = press_x(ACTION_TOGGLE_SAFEMODE);
    add_msg( m_warning,
             _( "Spotted %s--safe mode is on! (%s to turn it off, %s to ignore monster%s)" ),
             spotted_creature_name.c_str(), msg_safe_mode.c_str(), msg_ignore.c_str(), whitelist.c_str() );
    safe_mode_warning_logged = true;
    return false;
}

void game::set_safe_mode( safe_mode_type mode )
{
    safe_mode = mode;
    safe_mode_warning_logged = false;
}

bool game::disable_robot( const tripoint &p )
{
    const int mondex = mon_at( p );
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
        if (!critter.has_flag(MF_INTERIOR_AMMO)) {
            for (auto & ammodef : critter.ammo) {
                if (ammodef.second > 0) {
                    m.spawn_item(p.x, p.y, ammodef.first, 1, ammodef.second, calendar::turn);
                }
            }
        }
        remove_zombie( mondex );
        return true;
    }
    // Manhacks are special, they have their own menu here.
    if( mid == mon_manhack ) {
        int choice = 0;
        if( critter.has_effect( effect_docile ) ) {
            choice = menu( true, _( "Reprogram the manhack?" ), _( "Engage targets." ), _( "Cancel" ), NULL );
        } else {
            choice = menu( true, _( "Reprogram the manhack?" ), _( "Follow me." ), _( "Cancel" ), NULL );
        }
        switch( choice ) {
            case 1:
                if( critter.has_effect( effect_docile ) ) {
                    critter.remove_effect( effect_docile );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s hovers momentarily as it surveys the area." ),
                                 critter.name().c_str() );
                    }
                } else {
                    critter.add_effect( effect_docile, 1, num_bp, true );
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

bool game::prompt_dangerous_tile( const tripoint &dest_loc ) const
{
    std::vector<std::string> harmful_stuff;
    const auto fields_here = m.field_at( u.pos() );
    for( const auto& e : m.field_at( dest_loc ) ) {
        // warn before moving into a dangerous field except when already standing within a similar field
        if( u.is_dangerous_field( e.second ) && fields_here.findField( e.first ) == nullptr ) {
            harmful_stuff.push_back( e.second.name() );
        }
    }

    if( !u.is_blind() ) {
        const trap &tr = m.tr_at(dest_loc);
        int vpart;
        const vehicle *const veh = m.veh_at( dest_loc, vpart );
        const bool boardable = veh && veh->part_with_feature( vpart, "BOARDABLE" ) >= 0;
        // Hack for now, later ledge should stop being a trap
        if( tr.can_see(dest_loc, u) && !tr.is_benign() &&
            m.has_floor( dest_loc ) && !boardable ) {
            harmful_stuff.push_back( tr.name.c_str() );
        }

        static const std::set< body_part > sharp_bps = {
            bp_eyes, bp_mouth, bp_head, bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r, bp_arm_l, bp_arm_r,
            bp_hand_l, bp_hand_r, bp_torso
        };

        static const auto sharp_bp_check = [this]( body_part bp ) {
            return u.immune_to( bp, { DT_CUT, 10 } );
        };

        if( m.has_flag( "ROUGH", dest_loc ) && !m.has_flag( "ROUGH", u.pos() ) && !boardable &&
            ( u.get_armor_bash( bp_foot_l ) < 5 || u.get_armor_bash( bp_foot_r ) < 5 ) ) {
            harmful_stuff.push_back( m.name( dest_loc ).c_str() );
        } else if( m.has_flag( "SHARP", dest_loc ) && !m.has_flag( "SHARP", u.pos() ) && !boardable &&
                   u.dex_cur < 78 && !std::all_of( sharp_bps.begin(), sharp_bps.end(), sharp_bp_check ) ) {
            harmful_stuff.push_back( m.name( dest_loc ).c_str() );
        }

    }

    if( !harmful_stuff.empty() &&
        !query_yn( _("Really step into %s?"), enumerate_as_string( harmful_stuff ).c_str() ) ) {
        return false;
    }
    return true;
}

bool game::plmove(int dx, int dy, int dz)
{
    if( (!check_safe_mode_allowed()) || u.has_active_mutation("SHELL2") ) {
        if ( u.has_active_mutation("SHELL2")) {
            add_msg(m_warning, _("You can't move while in your shell.  Deactivate it to go mobile."));
        }

        return false;
    }

    tripoint dest_loc;
    if( dz == 0 && u.has_effect( effect_stunned ) ) {
        dest_loc.x = rng(u.posx() - 1, u.posx() + 1);
        dest_loc.y = rng(u.posy() - 1, u.posy() + 1);
        dest_loc.z = u.posz();
    } else {
        if( tile_iso && use_tiles && !u.has_destination() ) {
            rotate_direction_cw(dx,dy);
        }
        dest_loc.x = u.posx() + dx;
        dest_loc.y = u.posy() + dy;
        dest_loc.z = u.posz() + dz;
    }

    if( dest_loc == u.pos() ) {
        // Well that sure was easy
        return true;
    }

    if( dz == 0 && ramp_move( dest_loc ) ) {
        // TODO: Make it work nice with automove (if it doesn't do so already?)
        return false;
    }

    if( u.has_effect( effect_amigara ) ) {
        int curdist = INT_MAX;
        int newdist = INT_MAX;
        const tripoint minp = tripoint( 0, 0, u.posz() );
        const tripoint maxp = tripoint( SEEX * MAPSIZE, SEEY * MAPSIZE, u.posz() );
        for( const tripoint &pt : m.points_in_rectangle( minp, maxp ) ) {
            if( m.ter( pt ) == t_fault ) {
                int dist = rl_dist( pt, u.pos() );
                if( dist < curdist ) {
                    curdist = dist;
                }
                dist = rl_dist( pt, dest_loc );
                if( dist < newdist ) {
                    newdist = dist;
                }
            }
        }
        if( newdist > curdist ) {
            add_msg( m_info, _( "You cannot pull yourself away from the faultline..." ) );
            return false;
        }
    }

    dbg(D_PEDANTIC_INFO) << "game:plmove: From (" <<
                         u.posx() << "," << u.posy() << "," << u.posz() << ") to (" <<
                         dest_loc.x << "," << dest_loc.y << "," << dest_loc.z << ")";

    if( disable_robot( dest_loc ) ) {
        return false;
    }

    // Check if our movement is actually an attack on a monster or npc
    int mondex = mon_at( dest_loc, true );
    int npcdex = npc_at( dest_loc );
    // Are we displacing a monster?

    bool attacking = false;
    if( mondex != -1 || npcdex != -1 ){
        attacking = true;
    }

    if( !u.move_effects( attacking ) ) {
        u.moves -= 100;
        return false;
    }

    if( mondex != -1 ) {
        monster &critter = zombie(mondex);
        if( critter.friendly == 0 &&
            !critter.has_effect( effect_pet) ) {
            if (u.has_destination()) {
                add_msg(m_warning, _("Monster in the way. Auto-move canceled."));
                add_msg(m_info, _("Click directly on monster to attack."));
                u.clear_destination();
                return false;
            }
            if (u.has_effect( effect_relax_gas)) {
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
            if( critter.is_hallucination() ) {
                critter.die( &u );
            }
            draw_hit_mon(dest_loc, critter, critter.is_dead());
            return false;
        } else if( critter.has_flag( MF_IMMOBILE ) ) {
            add_msg( m_info, _( "You can't displace your %s." ), critter.name().c_str() );
            return false;
        }
        // Successful displacing is handled (much) later
    }
    // If not a monster, maybe there's an NPC there
    if( npcdex != -1 ) {
        npc &np = *active_npc[npcdex];
        if( u.has_destination() ) {
            add_msg(_("NPC in the way, Auto-move canceled."));
            add_msg(m_info, _("Click directly on NPC to attack."));
            u.clear_destination();
            return false;
        }

        if( !np.is_enemy() ) {
            npc_menu( np );
            return false;
        }

        u.melee_attack( np, true );
        np.make_angry();
        return false;
    }

    // GRAB: pre-action checking.
    int vpart0 = -1, vpart1 = -1, dpart = -1;
    vehicle *veh0 = m.veh_at( u.pos(), vpart0 );
    vehicle *veh1 = m.veh_at( dest_loc, vpart1 );

    bool veh_closed_door = false;
    bool outside_vehicle = ( veh0 == nullptr || veh0 != veh1 );
    if( veh1 != nullptr ) {
        dpart = veh1->next_part_to_open(vpart1, outside_vehicle);
        veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
    }

    if( veh0 != nullptr && abs(veh0->velocity) > 100 ) {
        if( veh1 == nullptr ) {
            if (query_yn(_("Dive from moving vehicle?"))) {
                moving_vehicle_dismount( dest_loc );
            }
            return false;
        } else if( veh1 != veh0 ) {
            add_msg(m_info, _("There is another vehicle in the way."));
            return false;
        } else if( veh1->part_with_feature(vpart1, "BOARDABLE") < 0 ) {
            add_msg(m_info, _("That part of the vehicle is currently unsafe."));
            return false;
        }
    }

    bool toSwimmable = m.has_flag("SWIMMABLE", dest_loc);
    bool toDeepWater = m.has_flag(TFLAG_DEEP_WATER, dest_loc);
    bool fromSwimmable = m.has_flag("SWIMMABLE", u.pos());
    bool fromDeepWater = m.has_flag(TFLAG_DEEP_WATER, u.pos());
    bool fromBoat = veh0 != nullptr && veh0->all_parts_with_feature(VPFLAG_FLOATS).size() > 0;
    bool toBoat = veh1 != nullptr && veh1->all_parts_with_feature(VPFLAG_FLOATS).size() > 0;

    if( toSwimmable && toDeepWater && !toBoat ) { // Dive into water!
        // Requires confirmation if we were on dry land previously
        if ((fromSwimmable && fromDeepWater && !fromBoat) || query_yn(_("Dive into the water?"))) {
            if ((!fromDeepWater || fromBoat) && u.swim_speed() < 500) {
                add_msg(_("You start swimming."));
                add_msg(m_info, _("%s to dive underwater."),
                        press_x(ACTION_MOVE_DOWN).c_str());
            }
            plswim( dest_loc );
        }

        on_move_effects();
        return true;
    }

    if( walk_move( dest_loc ) ) {
        return true;
    }

    if( phasing_move( dest_loc ) ) {
        return true;
    }

    if( veh_closed_door ) {
        if( outside_vehicle ) {
            veh1->open_all_at( dpart );
        } else {
            veh1->open(dpart);
            add_msg(_("You open the %1$s's %2$s."), veh1->name.c_str(),
                    veh1->part_info( dpart ).name().c_str() );
        }

        u.moves -= 100;
        on_move_effects();
        return true;
    }

    if( m.furn(dest_loc) != f_safe_c && m.open_door( dest_loc, !m.is_outside( u.pos() ) ) ) {
        u.moves -= 100;
        return false;
    }

    // Invalid move
    const bool waste_moves = u.is_blind() || u.has_effect( effect_stunned );
    if( waste_moves || dest_loc.z != u.posz() ) {
        add_msg( _( "You bump into the %s!" ), m.obstacle_name( dest_loc ).c_str() );
        // Only lose movement if we're blind
        if( waste_moves ) {
            u.moves -= 100;
        }
    } else if( m.ter(dest_loc) == t_door_locked || m.ter(dest_loc) == t_door_locked_peep ||
               m.ter(dest_loc) == t_door_locked_alarm || m.ter(dest_loc) == t_door_locked_interior) {
        // Don't drain move points for learning something you could learn just by looking
        add_msg(_("That door is locked!"));
    } else if (m.ter(dest_loc) == t_door_bar_locked) {
        add_msg(_("You rattle the bars but the door is locked!"));
    }
    return false;
}

bool game::ramp_move( const tripoint &dest_loc )
{
    if( dest_loc.z != u.posz() ) {
        // No recursive ramp_moves
        return false;
    }

    // We're moving onto a tile with no support, check if it has a ramp below
    if( !m.has_floor_or_support( dest_loc ) ) {
        tripoint below( dest_loc.x, dest_loc.y, dest_loc.z - 1 );
        if( m.has_flag( TFLAG_RAMP, below ) ) {
            // But we're moving onto one from above
            const tripoint dp = dest_loc - u.pos();
            plmove( dp.x, dp.y, -1 );
            // No penalty for misaligned stairs here
            // Also cheaper than climbing up
            return true;
        }

        return false;
    }

    if( !m.has_flag( TFLAG_RAMP, u.pos() ) ||
        m.passable( dest_loc ) ) {
        return false;
    }

    // Try to find an aligned end of the ramp that will make our climb faster
    // Basically, finish walking on the stairs instead of pulling self up by hand
    bool aligned_ramps = false;
    for( const tripoint &pt : m.points_in_radius( u.pos(), 1 ) ) {
        if( rl_dist( pt, dest_loc ) <= 1.5f && m.has_flag( "RAMP_END", pt ) ) {
            aligned_ramps = true;
            break;
        }
    }

    const tripoint above_u( u.posx(), u.posy(), u.posz() + 1 );
    const tripoint above_dest( dest_loc.x, dest_loc.y, dest_loc.z + 1 );
    if( m.has_floor_or_support( above_u ) ) {
        add_msg( m_warning, _("You can't climb here - there's a ceiling above.") );
        return false;
    }

    const tripoint dp = dest_loc - u.pos();
    const tripoint old_pos = u.pos();
    plmove( dp.x, dp.y, 1 );
    // We can't just take the result of the above function here
    if( u.pos() != old_pos ) {
        u.moves -= 50 + (aligned_ramps ? 0 : 50 );
    }

    return true;
}

bool game::walk_move( const tripoint &dest_loc )
{
    int vpart_here, vpart_there;
    const vehicle *const veh_here = m.veh_at( u.pos(), vpart_here );
    const vehicle *const veh_there = m.veh_at( dest_loc, vpart_there );

    bool pushing = false;  // moving -into- grabbed tile; skip check for move_cost > 0
    bool pulling = false;  // moving -away- from grabbed tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0

    bool grabbed = u.grab_type != OBJECT_NONE;
    if( grabbed ) {
        const tripoint dp = dest_loc - u.pos();
        pushing = dp ==  u.grab_point;
        pulling = dp == -u.grab_point;
    }

    if( grabbed && dest_loc.z != u.posz() ) {
        add_msg( m_warning, _("You let go of the grabbed object") );
        grabbed = false;
        u.grab_type = OBJECT_NONE;
        u.grab_point = tripoint_zero;
    }

    // Now make sure we're actually holding something
    const vehicle *grabbed_vehicle = nullptr;
    if( grabbed && u.grab_type == OBJECT_FURNITURE ) {
        // We only care about shifting, because it's the only one that can change our destination
        if( m.has_furn( u.pos() + u.grab_point ) ) {
            shifting_furniture = !pushing && !pulling;
        } else {
            // We were grabbing a furniture that isn't there
            grabbed = false;
        }
    } else if( grabbed && u.grab_type == OBJECT_VEHICLE ) {
        grabbed_vehicle = m.veh_at( u.pos() + u.grab_point );
        if( grabbed_vehicle == nullptr ) {
            // We were grabbing a vehicle that isn't there anymore
            grabbed = false;
        }
    } else if( grabbed ) {
        // We were grabbing something WEIRD, let's pretend we weren't
        grabbed = false;
    }

    if( u.grab_point != tripoint_zero && !grabbed ) {
        add_msg( m_warning, _("Can't find grabbed object.") );
        u.grab_type = OBJECT_NONE;
        u.grab_point = tripoint_zero;
    }

    if( m.impassable( dest_loc ) && !pushing && !shifting_furniture ) {
        return false;
    }
    u.set_underwater(false);

    if( !shifting_furniture && !prompt_dangerous_tile( dest_loc ) ) {
        return true;
    }

    // Used to decide whether to print a 'moving is slow message
    const int mcost_from = m.move_cost( u.pos() ); //calculate this _before_ calling grabbed_move

    int modifier = 0;
    if( grabbed && u.grab_type == OBJECT_FURNITURE && u.pos() + u.grab_point == dest_loc ) {
        modifier = -m.furn( dest_loc ).obj().movecost;
    }

    const int mcost = m.combined_movecost( u.pos(), dest_loc, grabbed_vehicle, modifier );
    if( grabbed_move( dest_loc - u.pos() ) ) {
        return true;
    } else if( mcost == 0 ) {
        return false;
    }

    bool diag = trigdist && u.posx() != dest_loc.x && u.posy() != dest_loc.y;
    const int previous_moves = u.moves;
    u.moves -= u.run_cost( mcost, diag );

    u.burn_move_stamina( previous_moves - u.moves );

    // Adjust recoil down
    ///\EFFECT_STR increases recoil recovery speed

    ///\EFFECT_GUN inreases recoil recovery speed
    u.recoil -= int(u.str_cur / 2) + u.get_skill_level( skill_id( "gun" ) );
    u.recoil = std::max( MIN_RECOIL * 2, u.recoil );
    u.recoil = int(u.recoil / 2);

    // Print a message if movement is slow
    const int mcost_to = m.move_cost( dest_loc ); //calculate this _after_ calling grabbed_move
    const bool slowed = ( !u.has_trait( "PARKOUR" ) && ( mcost_to > 2 || mcost_from > 2 ) ) ||
                  mcost_to > 4 || mcost_from > 4;
    if( slowed ) {
        // Unless u.pos() has a higher movecost than dest_loc, state that dest_loc is the cause
        if( mcost_to >= mcost_from ) {
            if( veh_there ) {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ),
                         veh_there->part_info( vpart_there ).name().c_str() );
            } else {
                add_msg( m_warning, _( "Moving onto this %s is slow!"), m.name( dest_loc ).c_str() );
            }
        } else {
            if( veh_here ) {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ),
                         veh_here->part_info( vpart_here ).name().c_str() );
            } else {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ), m.name( u.pos() ).c_str() );
            }
        }
        sfx::play_variant_sound( "plmove", "clear_obstacle", sfx::get_heard_volume(u.pos()) );
    }

    place_player( dest_loc );
    on_move_effects();
    return true;
}

void game::place_player( const tripoint &dest_loc )
{
    int vpart1;
    const vehicle *veh1 = m.veh_at( dest_loc, vpart1 );
    if( veh1 != nullptr ) {
        const vehicle_part *part = &(veh1->parts[vpart1]);
        std::string label = veh1->get_label(part->mount.x, part->mount.y);
        if (label != "") {
            add_msg(m_info, _("Label here: %s"), label.c_str());
        }
    }

    std::string signage = m.get_signage( dest_loc );
    if (signage.size()) {
        add_msg(m_info, _("The sign says: %s"), signage.c_str());
    }
    if( m.has_graffiti_at( dest_loc ) ) {
        add_msg(_("Written here: %s"), utf8_truncate(m.graffiti_at( dest_loc ), 40).c_str());
    }
    // TODO: Move the stuff below to a Character method so that NPCs can reuse it
    if (m.has_flag("ROUGH", dest_loc) && (!u.in_vehicle)) {
        if (one_in(5) && u.get_armor_bash(bp_foot_l) < rng(2, 5)) {
            add_msg(m_bad, _("You hurt your left foot on the %s!"),
                    m.has_flag_ter( "ROUGH", dest_loc) ? m.tername(dest_loc).c_str() : m.furnname(dest_loc).c_str() );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
        if (one_in(5) && u.get_armor_bash(bp_foot_r) < rng(2, 5)) {
            add_msg(m_bad, _("You hurt your right foot on the %s!"),
                    m.has_flag_ter( "ROUGH", dest_loc) ? m.tername(dest_loc).c_str() : m.furnname(dest_loc).c_str() );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
    }
    ///\EFFECT_DEX increases chance of avoiding cuts on sharp terrain
    if( m.has_flag("SHARP", dest_loc) && !one_in(3) && !x_in_y(1+u.dex_cur/2, 40) &&
        (!u.in_vehicle) && (!u.has_trait("PARKOUR") || one_in(4)) ) {
        body_part bp = random_body_part();
        if(u.deal_damage( nullptr, bp, damage_instance( DT_CUT, rng( 1, 10 ) ) ).total_damage() > 0) {
            //~ 1$s - bodypart name in accusative, 2$s is terrain name.
            add_msg(m_bad, _("You cut your %1$s on the %2$s!"),
                    body_part_name_accusative(bp).c_str(),
                    m.has_flag_ter( "SHARP", dest_loc ) ? m.tername(dest_loc).c_str() : m.furnname(dest_loc).c_str() );
            if ((u.has_trait("INFRESIST")) && (one_in(1024))) {
            u.add_effect( effect_tetanus, 1, num_bp, true);
            } else if ((!u.has_trait("INFIMMUNE") || !u.has_trait("INFRESIST")) && (one_in(256))) {
              u.add_effect( effect_tetanus, 1, num_bp, true);
             }
        }
    }
    if (m.has_flag("UNSTABLE", dest_loc)) {
        u.add_effect( effect_bouldering, 1, num_bp, true);
    } else if (u.has_effect( effect_bouldering)) {
        u.remove_effect( effect_bouldering);
    }
    if (u.has_trait("LEG_TENT_BRACE") && (!u.footwear_factor() ||
                                             (u.footwear_factor() == .5 && one_in(2)))) {
        // DX and IN are long suits for Cephalopods,
        // so this shouldn't cause too much hardship
        // Presumed that if it's swimmable, they're
        // swimming and won't stick
        ///\EFFECT_DEX decreases chance of tentacles getting stuck to the ground

        ///\EFFECT_INT decreases chance of tentacles getting stuck to the ground
        if ((!(m.has_flag("SWIMMABLE", dest_loc)) && (one_in(80 + u.dex_cur + u.int_cur)))) {
            add_msg(_("Your tentacles stick to the ground, but you pull them free."));
            u.mod_fatigue(1);
        }
    }
    if (!u.has_artifact_with(AEP_STEALTH) && !u.has_trait("LEG_TENTACLES") &&
        !u.has_trait("DEBUG_SILENT")) {
        if (u.has_trait("LIGHTSTEP") || u.is_wearing("rm13_armor_on")) {
            sounds::sound(dest_loc, 2, "", true, "none", "none");    // Sound of footsteps may awaken nearby monsters
            sfx::do_footstep();
        } else if (u.has_trait("CLUMSY")) {
            sounds::sound(dest_loc, 10, "", true, "none", "none");
            sfx::do_footstep();
        } else if (u.has_bionic("bio_ankles")) {
            sounds::sound(dest_loc, 12, "", true, "none", "none");
            sfx::do_footstep();
        } else {
            sounds::sound(dest_loc, 6, "", true, "none");
            sfx::do_footstep();
        }
    }
    if (one_in(20) && u.has_artifact_with(AEP_MOVEMENT_NOISE)) {
        sounds::sound(u.pos(), 40, _("You emit a rattling sound."));
    }
    // If we moved out of the nonant, we need update our map data
    if (m.has_flag("SWIMMABLE", dest_loc) && u.has_effect( effect_onfire)) {
        add_msg(_("The water puts out the flames!"));
        u.remove_effect( effect_onfire);
    }

    const int mondex = mon_at( dest_loc );
    if( mondex != -1 ) {
        // We displaced a monster. It's probably a bug if it wasn't a friendly mon...
        // Immobile monsters can't be displaced.
        monster &critter = zombie( mondex );
        critter.move_to( u.pos(), true ); // Force the movement even though the player is there right now.
        add_msg(_("You displace the %s."), critter.name().c_str());
    }

    // If the player is in a vehicle, unboard them from the current part
    if( u.in_vehicle ) {
        m.unboard_vehicle( u.pos() );
    }

    if( dest_loc != u.pos() ) {
        u.lifetime_stats()->squares_walked++;
    }

    // Move the player
    // Start with z-level, to make make it less likely that old functions (2D ones) freak out
    if( m.has_zlevels() && dest_loc.z != get_levz() ) {
        vertical_shift( dest_loc.z );
    }

    u.setpos( dest_loc );
    update_map( u );
    // Important: don't use dest_loc after this line. `update_map` may have shifted the map
    // and dest_loc was not adjusted and therefor is still in the un-shifted system and probably wrong.

    //Autopickup
    if (get_option<bool>( "AUTO_PICKUP" ) && (!get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) || mostseen == 0) &&
        ( m.has_items( u.pos() ) || get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
        Pickup::pick_up(u.pos(), -1);
    }

    // If the new tile is a boardable part, board it
    if( veh1 != nullptr && veh1->part_with_feature(vpart1, "BOARDABLE") >= 0 ) {
        m.board_vehicle( u.pos(), &u );
    }

    // Traps!
    // Try to detect.
    u.search_surroundings();
    m.creature_on_trap( u );

    // apply martial art move bonuses
    u.ma_onmove_effects();

    // Drench the player if swimmable
    if( m.has_flag( "SWIMMABLE", u.pos() ) ) {
        u.drench( 40, mfb(bp_foot_l) | mfb(bp_foot_r) | mfb(bp_leg_l) | mfb(bp_leg_r), false );
    }

    // List items here
    if( !m.has_flag( "SEALED", u.pos() ) ) {
        if( u.is_blind() && !m.i_at( u.pos() ).empty() ) {
            add_msg(_("There's something here, but you can't see what it is."));
        } else if( m.has_items(u.pos()) ) {
            std::vector<std::string> names;
            std::vector<size_t> counts;
            std::vector<item> items;
            for( auto &tmpitem : m.i_at( u.pos() ) ) {

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

    if( veh1 != nullptr && veh1->part_with_feature(vpart1, "CONTROLS") >= 0 && u.in_vehicle ) {
        add_msg(_("There are vehicle controls here."));
        add_msg(m_info, _("%s to drive."),
                press_x(ACTION_CONTROL_VEHICLE).c_str());
    }
}

bool game::phasing_move( const tripoint &dest_loc )
{
    if( !u.has_active_bionic("bio_probability_travel") || u.power_level < 250 ) {
        return false;
    }

    if( dest_loc.z != u.posz() ) {
        // No vertical phasing yet
        return false;
    }

    //probability travel through walls but not water
    tripoint dest = dest_loc;
    // tile is impassable
    int tunneldist = 0;
    const int dx = sgn( dest.x - u.posx() );
    const int dy = sgn( dest.y - u.posy() );
    while( m.impassable( dest ) ||
           ( critter_at( dest ) != nullptr && tunneldist > 0 ) ) {
        //add 1 to tunnel distance for each impassable tile in the line
        tunneldist += 1;
        if( tunneldist * 250 > u.power_level ) { //oops, not enough energy! Tunneling costs 250 bionic power per impassable tile
            add_msg(_("You try to quantum tunnel through the barrier but are reflected! Try again with more energy!"));
            u.charge_power(-250);
            return false;
        }

        if( tunneldist > 24 ) {
            add_msg(m_info, _("It's too dangerous to tunnel that far!"));
            u.charge_power(-250);
            return false;
        }

        dest.x += dx;
        dest.y += dy;
    }

    if( tunneldist != 0 ) {
        if( u.in_vehicle ) {
            m.unboard_vehicle( u.pos() );
        }

        add_msg(_("You quantum tunnel through the %d-tile wide barrier!"), tunneldist);
        u.charge_power(-(tunneldist * 250)); //tunneling costs 250 bionic power per impassable tile
        u.moves -= 100; //tunneling costs 100 moves
        u.setpos( dest );

        int vpart;
        const vehicle *veh = m.veh_at( u.pos(), vpart );
        if( veh != nullptr &&
            veh->part_with_feature(vpart, "BOARDABLE") >= 0) {
            m.board_vehicle( u.pos(), &u );
        }

        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
        on_move_effects();
        return true;
    }

    return false;
}

bool game::grabbed_veh_move( const tripoint &dp )
{
    int grabbed_part = 0;
    vehicle *grabbed_vehicle = m.veh_at( u.pos() + u.grab_point, grabbed_part );
    if( nullptr == grabbed_vehicle ) {
        add_msg(m_info, _("No vehicle at grabbed point."));
        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
        return false;
    }

    const vehicle *veh_under_player = m.veh_at( u.pos() );
    if( grabbed_vehicle == veh_under_player ) {
        u.grab_point = -dp;
        return false;
    }

    tripoint dp_veh = -u.grab_point;
    tripoint prev_grab = u.grab_point;

    // We are not moving around the veh
    if ((dp_veh.x + dp.x) == 0 && (dp_veh.y + dp.y) == 0) {
        // We are pushing in the direction of veh
        dp_veh = dp;
    } else {
        u.grab_point = -dp;
    }

    if( (abs(dp.x + dp_veh.x) == 0 || abs(dp.y + dp_veh.y) == 0) &&
        u.grab_point.x != 0 && u.grab_point.y != 0 ) {
        // We are moving diagonal while veh is diagonal too and one direction is 0
        dp_veh.x = ((dp.x + dp_veh.x) == 0) ? 0 : dp_veh.x;
        dp_veh.y = ((dp.y + dp_veh.y) == 0) ? 0 : dp_veh.y;

        u.grab_point = -dp_veh;
    }

    if( abs(dp.x + dp_veh.x) != 2 && abs(dp.y + dp_veh.y) != 2 &&
        ((dp_veh.x + dp.x) == 0 || (dp_veh.y + dp.y) == 0) ) {
        // Not actually moving the vehicle, don't do the checks
        u.grab_point = -(dp + dp_veh);
        return false;
    }

    // Make sure the mass and pivot point are correct
    grabbed_vehicle->invalidate_mass();

    //vehicle movement: strength check
    int mc = 0;
    int str_req = (grabbed_vehicle->total_mass() / 25); //strengh reqired to move vehicle.

    //if vehicle is rollable we modify str_req based on a function of movecost per wheel.

    // Veh just too big to grab & move; 41-45 lets folks have a bit of a window
    // (Roughly 1.1K kg = danger zone; cube vans are about the max)
    if (str_req > 45) {
        add_msg(m_info, _("The %s is too bulky for you to move by hand."),
                grabbed_vehicle->name.c_str() );
        u.moves -= 100;
        return true; // No shoving around an RV.
    }

    const auto &wheel_indices = grabbed_vehicle->wheelcache;
    // If vehicle weighs too much, wheels don't provide a bonus.
    if( grabbed_vehicle->valid_wheel_config( false ) && str_req <= 40 ) {
        //determine movecost for terrain touching wheels
        const tripoint vehpos = grabbed_vehicle->global_pos3();
        for( int p : wheel_indices ) {
            const tripoint wheel_pos = vehpos + grabbed_vehicle->parts[p].precalc[0];
            const int mapcost = m.move_cost( wheel_pos, grabbed_vehicle );
            mc += (str_req / wheel_indices.size()) * mapcost;
        }
        //set strength check threshold
        //if vehicle has many or only one wheel (shopping cart), it is as if it had four.
        if(wheel_indices.size() > 4 || wheel_indices.size() == 1) {
            str_req = mc / 4 + 1;
        } else {
            str_req = mc / wheel_indices.size() + 1;
        }
    } else {
        str_req++;
        //if vehicle has no wheels str_req make a noise.
        if (str_req <= u.get_str() ) {
            sounds::sound( grabbed_vehicle->global_pos3(), str_req * 2,
                _("a scraping noise."));
        }
    }

    //final strength check and outcomes
    ///\EFFECT_STR determines ability to drag vehicles
    if (str_req <= u.get_str() ) {
        //calculate exertion factor and movement penalty
        ///\EFFECT_STR increases speed of dragging vehicles
        u.moves -= 100 * str_req / std::max( 1, u.get_str() );
        int ex = dice(1, 3) - 1 + str_req;
        if (ex > u.get_str() ) {
            add_msg(m_bad, _("You strain yourself to move the %s!"), grabbed_vehicle->name.c_str() );
            u.moves -= 200;
            u.mod_pain(1);
        } else if (ex == u.get_str() ) {
            u.moves -= 200;
            add_msg( _("It takes some time to move the %s."), grabbed_vehicle->name.c_str());
        }
    } else {
        u.moves -= 100;
        add_msg( m_bad, _("You lack the strength to move the %s"), grabbed_vehicle->name.c_str() );
        return true;
    }

    tileray mdir;

    mdir.init(dp_veh.x, dp_veh.y);
    grabbed_vehicle->turn(mdir.dir() - grabbed_vehicle->face.dir());
    grabbed_vehicle->face = grabbed_vehicle->turn_dir;
    grabbed_vehicle->precalc_mounts(1, mdir.dir(), grabbed_vehicle->pivot_point());

    // Grabbed part has to stay at distance 1 to the player
    // and in roughly the same direction.
    const tripoint new_part_pos = grabbed_vehicle->global_pos3() +
                                  grabbed_vehicle->parts[ grabbed_part ].precalc[ 1 ];
    const tripoint expected_pos = u.pos() + dp + u.grab_point;
    dp_veh = expected_pos - new_part_pos;

    std::vector<veh_collision> colls;
    // Set player location to illegal value so it can't collide with vehicle.
    const tripoint player_prev = u.pos();
    u.setpos( tripoint_zero );
    if( grabbed_vehicle->collision( colls, dp_veh, true ) ) {
        add_msg( _("The %s collides with %s."), grabbed_vehicle->name.c_str(), colls[0].target_name.c_str() );
        u.moves -= 10;
        u.setpos( player_prev );
        u.grab_point = prev_grab;
        return true;
    }

    u.setpos( player_prev );

    tripoint gp = grabbed_vehicle->global_pos3();
    grabbed_vehicle = m.displace_vehicle( gp, dp_veh );

    if( grabbed_vehicle == nullptr ) {
        debugmsg( "Grabbed vehicle disappeared" );
        return false;
    }

    for( int p : wheel_indices ) {
        if( one_in(2) ) {
            tripoint wheel_p = grabbed_vehicle->global_part_pos3( grabbed_part );
            grabbed_vehicle->handle_trap( wheel_p, p );
        }
    }

    return false;

}

bool game::grabbed_furn_move( const tripoint &dp )
{
    // Furniture: pull, push, or standing still and nudging object around.
    // Can push furniture out of reach.
    tripoint fpos = u.pos() + u.grab_point;
    // supposed position of grabbed furniture
    if( !m.has_furn( fpos ) ) {
        // where'd it go? We're grabbing thin air so reset.
        add_msg(m_info, _("No furniture at grabbed point.") );
        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
        return false;
    }

    const bool pushing_furniture = dp ==  u.grab_point;
    const bool pulling_furniture = dp == -u.grab_point;
    const bool shifting_furniture = !pushing_furniture && !pulling_furniture;

    tripoint fdest = fpos + dp; // intended destination of furniture.
    // Check floor: floorless tiles don't need to be flat and have no traps
    const bool has_floor = m.has_floor( fdest );
    // Unfortunately, game::is_empty fails for tiles we're standing on,
    // which will forbid pulling, so:
    const bool canmove = (
        m.passable(fdest) &&
        npc_at(fdest) == -1 &&
        mon_at(fdest) == -1 &&
        ( !pulling_furniture || is_empty( u.pos() + dp ) ) &&
        ( !has_floor || m.has_flag( "FLAT", fdest ) ) &&
        !m.has_furn( fdest ) &&
        m.veh_at( fdest ) == nullptr &&
        ( !has_floor || m.tr_at( fdest ).is_null() )
        );

    const furn_t furntype = m.furn(fpos).obj();
    const int src_items = m.i_at(fpos).size();
    const int dst_items = m.i_at(fdest).size();
    bool dst_item_ok = ( !m.has_flag("NOITEM", fdest) &&
                         !m.has_flag("SWIMMABLE", fdest) &&
                         !m.has_flag("DESTROY_ITEM", fdest) );
    bool src_item_ok = ( m.furn(fpos).obj().has_flag("CONTAINER") ||
                         m.furn(fpos).obj().has_flag("SEALED") );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    int furniture_contents_weight = 0;
    for( auto contained_item : m.i_at( fpos ) ) {
        furniture_contents_weight += contained_item.weight();
    }
    str_req += furniture_contents_weight / 4000;

    if( !canmove ) {
        // TODO: What is something?
        add_msg( _("The %s collides with something."), furntype.name.c_str() );
        u.moves -= 50;
        return true;
    ///\EFFECT_STR determins ability to drag furniture
    } else if ( str_req > u.get_str() &&
                one_in(std::max(20 - str_req - u.get_str(), 2)) ) {
        add_msg(m_bad, _("You strain yourself trying to move the heavy %s!"),
                furntype.name.c_str() );
        u.moves -= 100;
        u.mod_pain(1); // Hurt ourself.
        return true; // furniture and or obstacle wins.
    } else if ( !src_item_ok && dst_items > 0 ) {
        add_msg( _("There's stuff in the way.") );
        u.moves -= 50;
        return true;
    }

    u.moves -= str_req * 10;
    // Additional penalty if we can't comfortably move it.
    if( str_req > u.get_str() ) {
        int move_penalty = std::pow(str_req, 2.0) + 100.0;
        if( move_penalty <= 1000 ) {
            u.moves -= 100;
            add_msg( m_bad, _("The %s is too heavy for you to budge."),
                     furntype.name.c_str() );
            return true;
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
    sounds::sound(fdest, furntype.move_str_req * 2, _("a scraping noise."));

    // Actually move the furniture
    m.furn_set( fdest, m.furn( fpos ) );
    m.furn_set( fpos, f_null );

    if ( src_items > 0 ) {  // and the stuff inside.
        if ( dst_item_ok && src_item_ok ) {
            // Assume contents of both cells are legal, so we can just swap contents.
            std::list<item> temp;
            std::move( m.i_at(fpos).begin(), m.i_at(fpos).end(),
                       std::back_inserter(temp) );
            m.i_clear(fpos);
            for( auto item_iter = m.i_at(fdest).begin();
                 item_iter != m.i_at(fdest).end(); ++item_iter ) {
                m.i_at(fpos).push_back( *item_iter );
            }
            m.i_clear(fdest);
            for( auto item_iter = temp.begin(); item_iter != temp.end(); ++item_iter ) {
                m.i_at(fdest).push_back( *item_iter );
            }
        } else {
            add_msg(_("Stuff spills from the %s!"), furntype.name.c_str() );
        }
    }

    if ( shifting_furniture ) {
        // We didn't move
        tripoint d_sum = u.grab_point + dp;
        if( abs( d_sum.x ) < 2 && abs( d_sum.y ) < 2 ) {
            u.grab_point = d_sum; // furniture moved relative to us
        } else { // we pushed furniture out of reach
            add_msg( _("You let go of the %s"), furntype.name.c_str() );
            u.grab_point = tripoint_zero;
            u.grab_type = OBJECT_NONE;
        }
        return true; // We moved furniture but stayed still.
    }

    if( pushing_furniture && m.impassable( fpos ) ) {
        // Not sure how that chair got into a wall, but don't let player follow.
        add_msg( _("You let go of the %1$s as it slides past %2$s"),
                 furntype.name.c_str(), m.tername( fdest ).c_str() );
        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
        return true;
    }

    return false;
}

bool game::grabbed_move( const tripoint &dp )
{
    if( u.grab_type == OBJECT_NONE ) {
        return false;
    }

    if( dp.z != 0 ) {
        // No dragging stuff up/down stairs yet!
        return false;
    }

    // vehicle: pulling, pushing, or moving around the grabbed object.
    if( u.grab_type == OBJECT_VEHICLE ) {
        return grabbed_veh_move( dp );
    }

    if( u.grab_type == OBJECT_FURNITURE ) {
        return grabbed_furn_move( dp );
    }

    add_msg(m_info, _("Nothing at grabbed point %d,%d,%d or bad grabbed object type."),
        u.grab_point.x, u.grab_point.y, u.grab_point.z );
    u.grab_point = { 0, 0, 0 };
    u.grab_type = OBJECT_NONE;
    return false;
}

void game::on_move_effects()
{
    // TODO: Move this to a character method
    if (moveCount % 2 == 0) {
        if (u.has_bionic("bio_torsionratchet")) {
            u.charge_power(1);
        }
    }

    if( u.move_mode == "run" ) {
        if( u.stamina <= 0 ) {
            u.toggle_move_mode();
        }
        if( one_in( u.stamina ) ) {
            u.add_effect( effect_winded, 3);
        }
    }

    sfx::do_ambient();
}

void game::plswim( const tripoint &p )
{
    if (!m.has_flag("SWIMMABLE", p)) {
        dbg(D_ERROR) << "game:plswim: Tried to swim in "
                     << m.tername(p).c_str() << "!";
        debugmsg("Tried to swim in %s!", m.tername(p).c_str());
        return;
    }
    if (u.has_effect( effect_onfire)) {
        add_msg(_("The water puts out the flames!"));
        u.remove_effect( effect_onfire);
    }
    if (u.has_effect( effect_glowing)) {
        add_msg(_("The water washes off the glowing goo!"));
        u.remove_effect( effect_glowing);
    }
    int movecost = u.swim_speed();
    u.practice( skill_id( "swimming" ), u.is_underwater() ? 2 : 1);
    if (movecost >= 500) {
        if (!u.is_underwater() && !(u.shoe_type_count("swim_fins") == 2 ||
                                    (u.shoe_type_count("swim_fins") == 1 && one_in(2)))) {
            add_msg(m_bad, _("You sink like a rock!"));
            u.set_underwater(true);
            ///\EFFECT_STR increases breath-holding capacity while sinking
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
    bool diagonal = (p.x != u.posx() && p.y != u.posy());
    if( u.in_vehicle ) {
        m.unboard_vehicle( u.pos() );
    }
    u.setpos( p );
    update_map( u );
    {
        int part;
        const auto veh = m.veh_at( u.pos(), part );
        if( veh != nullptr && veh->part_with_feature( part, VPFLAG_BOARDABLE ) >= 0 ) {
            m.board_vehicle( u.pos(), &u );
        }
    }
    u.moves -= (movecost > 200 ? 200 : movecost)  * (trigdist && diagonal ? 1.41 : 1);
    u.inv.rust_iron_items();

    u.burn_move_stamina( movecost );

    int drenchFlags = mfb(bp_leg_l) | mfb(bp_leg_r) | mfb(bp_torso) | mfb(bp_arm_l) |
        mfb(bp_arm_r) | mfb(bp_foot_l) | mfb(bp_foot_r) | mfb(bp_hand_l) | mfb(bp_hand_r);

    if (u.is_underwater()) {
        drenchFlags |= mfb(bp_head) | mfb(bp_eyes) | mfb(bp_mouth) | mfb(bp_hand_l) | mfb(bp_hand_r);
    }
    u.drench( 100, drenchFlags, true );
}

float rate_critter( const Creature &c )
{
    const npc *np = dynamic_cast<const npc *>( &c );
    if( np != nullptr ) {
        return np->weapon_value( np->weapon );
    }

    const monster *m = dynamic_cast<const monster *>( &c );
    return m->type->difficulty;
}

void game::autoattack()
{
    int reach = u.weapon.reach_range( u );
    auto critters = u.get_hostile_creatures( reach );
    if( critters.empty() ) {
        add_msg( m_info, _( "No hostile creature in reach." ) );
        return;
    }

    Creature &best = **std::max_element( critters.begin(), critters.end(),
        []( const Creature *l, const Creature *r ) {
        return rate_critter( *l ) > rate_critter( *r );
    } );

    const tripoint diff = best.pos() - u.pos();
    if( abs( diff.x ) <= 1 && abs( diff.y ) <= 1 && diff.z == 0 ) {
        plmove( diff.x, diff.y );
        return;
    }

    u.reach_attack( best.pos() );
}

void game::fling_creature(Creature *c, const int &dir, float flvel, bool controlled)
{
    if( c == nullptr ) {
        debugmsg( "game::fling_creature invoked on null target" );
        return;
    }

    if( c->is_dead_state() ) {
        // Flinging a corpse causes problems, don't enable without testing
        return;
    }

    int steps = 0;
    const bool is_u = (c == &u);
    // Don't animate critters getting bashed if animations are off
    const bool animate = is_u || get_option<bool>( "ANIMATIONS" );

    player *p = dynamic_cast<player*>(c);

    tileray tdir(dir);
    int range = flvel / 10;
    tripoint pt = c->pos();
    while( range > 0 ) {
        c->underwater = false;
        // TODO: Check whenever it is actually in the viewport
        // or maybe even just redraw the changed tiles
        bool seen = is_u || u.sees( *c ); // To avoid redrawing when not seen
        tdir.advance();
        pt.x = c->posx() + tdir.dx();
        pt.y = c->posy() + tdir.dy();
        bool thru = true;
        int mondex = mon_at( pt );
        float force = 0;

        if( mondex >= 0 ) {
            monster &critter = zombie( mondex );
            // Approximate critter's "stopping power" with its max hp
            force = std::min<float>( 1.5f * critter.type->hp, flvel );
            const int damage = rng( force, force * 2.0f ) / 6;
            c->impact( damage, pt );
            // Multiply zed damage by 6 because no body parts
            const int zed_damage = std::max( 0, ( damage - critter.get_armor_bash( bp_torso ) ) * 6 );
            // TODO: Pass the "flinger" here - it's not the flung critter that deals damage
            critter.apply_damage( c, bp_torso, zed_damage );
            critter.check_dead_state();
            if( !critter.is_dead() ) {
                thru = false;
            }
        } else if( m.impassable( pt ) ) {
            int part = -1;
            vehicle *veh = m.veh_at( pt, part );
            if( veh != nullptr ) {
                part = veh->obstacle_at_part( part );
                if( part == -1 ) {
                    force = std::min<float>( m.bash_strength( pt ), flvel );
                } else {
                    // No good way of limiting force here
                    // Keep it 1 less than maximum to make the impact hurt
                    // but to keep the target flying after it
                    force = flvel - 1;
                }
            } else {
                force = std::min<float>( m.bash_strength( pt ), flvel );
            }
            const int damage = rng( force, force * 2.0f ) / 9;
            c->impact( damage, pt );
            if( m.is_bashable( pt ) ) {
                // Only go through if we successfully make the tile passable
                m.bash( pt, flvel );
                thru = m.passable( pt );
            } else {
                thru = false;
            }
        }

        // If the critter dies during flinging, moving it around causes debugmsgs
        if( c->is_dead_state() ) {
            return;
        }

        flvel -= force;
        if( thru ) {
            if( p != nullptr ) {
                if( p->in_vehicle ) {
                    m.unboard_vehicle( p->pos() );
                }
                // If we're flinging the player around, make sure the map stays centered on them.
                if( is_u ) {
                    update_map( pt.x, pt.y );
                } else {
                    p->setpos( pt );
                }
            } else if( mon_at( pt ) < 0 ) {
                // Dying monster doesn't always leave an empty tile (blob spawning etc.)
                // Just don't setpos if it happens - next iteration will do so
                // or the monster will stop a tile before the unpassable one
                c->setpos( pt );
            }
        } else {
            // Don't zero flvel - count this as slamming both the obstacle and the ground
            // although at lower velocity
            break;
        }
        range--;
        steps++;
        if( animate && (seen || u.sees( *c )) ) {
            draw();
        }
    }

    // Fall down to the ground - always on the last reached tile
    if( !m.has_flag( "SWIMMABLE", c->pos() ) ) {
        // Fall on ground
        int force = rng( flvel, flvel * 2 ) / 9;
        if( controlled ) {
            force = std::max( force / 2 - 5, 0 );
        }
        if( force > 0 ) {
            int dmg = c->impact( force, c->pos() );
            // TODO: Make landing damage the floor
            m.bash( c->pos(), dmg / 4, false, false, false );
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

tripoint point_selection_menu( const std::vector<tripoint> &pts )
{
    if( pts.empty() ) {
        debugmsg( "point_selection_menu called with empty point set" );
        return tripoint_min;
    }

    if( pts.size() == 1 ) {
        return pts[0];
    }

    const tripoint &upos = g->u.pos();
    uimenu pmenu;
    pmenu.title = _("Climb where?");
    int num = 0;
    for( const tripoint &pt : pts ) {
        // TODO: Sort the menu so that it can be used with numpad directions
        const std::string &direction = direction_name( direction_from( upos.x, upos.y, pt.x, pt.y ) );
        // TODO: Inform player what is on said tile
        // But don't just print terrain name (in many cases it will be "open air")
        pmenu.addentry( num++, true, MENU_AUTOASSIGN, _("Climb %s"), direction.c_str() );
    }

    pmenu.addentry( num, true, 'q', "%s", _("Cancel") );

    pmenu.selected = num;
    pmenu.query();
    const int ret = pmenu.ret;
    if( ret < 0 || ret >= num ) {
        return tripoint_min;
    }

    return pts[ret];
}

void game::vertical_move(int movez, bool force)
{
    // Check if there are monsters are using the stairs.
    bool slippedpast = false;
    if( !m.has_zlevels() && !coming_to_stairs.empty() && !force ) {
        // TODO: Allow travel if zombie couldn't reach stairs, but spawn him when we go up.
        add_msg(m_warning, _("You try to use the stairs. Suddenly you are blocked by a %s!"),
                coming_to_stairs[0].name().c_str());
        // Roll.
        ///\EFFECT_DEX increases chance of moving past monsters on stairs

        ///\EFFECT_DODGE increases chance of moving past monsters on stairs
        int dexroll = dice(6, u.dex_cur + u.get_skill_level( skill_dodge ) * 2);
        ///\EFFECT_STR increases chance of moving past monsters on stairs

        ///\EFFECT_MELEE increases chance of moving past monsters on stairs
        int strroll = dice(3, u.str_cur + u.get_skill_level( skill_melee ) * 1.5);
        if (coming_to_stairs.size() > 4) {
            add_msg(_("The are a lot of them on the %s!"), m.tername(u.pos()).c_str());
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
    if( m.has_flag( "SWIMMABLE", u.pos() ) && m.has_flag( TFLAG_DEEP_WATER, u.pos() ) ) {
        if (movez == -1) {
            if (u.is_underwater()) {
                add_msg(m_info, _("You are already underwater!"));
                return;
            }
            if (u.worn_with_flag("FLOTATION")) {
                add_msg(m_info, _("You can't dive while wearing a flotation device."));
                return;
            }
            u.set_underwater(true);
            ///\EFFECT_STR increases breath-holding capacity while diving
            u.oxygen = 30 + 2 * u.str_cur;
            add_msg(_("You dive underwater!"));
        } else {
            if( u.swim_speed() < 500 || u.shoe_type_count("swim_fins") ) {
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
    bool climbing = false;
    int move_cost = 100;
    tripoint stairs( u.posx(), u.posy(), u.posz() + movez );
    if( m.has_zlevels() && !force && movez == 1 && !m.has_flag( "GOES_UP", u.pos() ) ) {
        // Climbing
        if( m.has_floor_or_support( stairs ) ) {
            add_msg( m_info, _("You can't climb here - there's a ceiling above your head") );
            return;
        }

        const int cost = u.climbing_cost( u.pos(), stairs );
        if( cost == 0 ) {
            add_msg( m_info, _("You can't climb here - you need walls and/or furniture to brace against") );
            return;
        }

        std::vector<tripoint> pts;
        for( const auto &pt : m.points_in_radius( stairs, 1 ) ) {
            if( m.passable( pt ) &&
                m.has_floor_or_support( pt ) ) {
                pts.push_back( pt );
            }
        }

        if( cost <= 0 || pts.empty() ) {
            add_msg( m_info, _("You can't climb here - there is no terrain above you that would support your weight") );
            return;
        } else if( cost > 0 && !pts.empty() ) {
            // TODO: Make it an extended action
            climbing = true;
            move_cost = cost;

            stairs = point_selection_menu( pts );
            if( stairs == tripoint_min ) {
                return;
            }
        }
    }

    if( !force && movez == -1 && !m.has_flag( "GOES_DOWN", u.pos() ) ) {
        add_msg(m_info, _("You can't go down here!"));
        return;
    } else if( !climbing && !force && movez == 1 && !m.has_flag( "GOES_UP", u.pos() ) ) {
        add_msg(m_info, _("You can't go up here!"));
        return;
    }

    if( force ) {
        // Let go of a grabbed cart.
        u.grab_point = tripoint_zero;
        u.grab_type = OBJECT_NONE;
    } else if( u.grab_point != tripoint_zero ) {
        add_msg(m_info, _("You can't drag things up and down stairs."));
        return;
    }

    // Becase get_levz takes z-value from the map, it will change when vertical_shift (m.has_zlevels() == true)
    // is called or when the map is loaded on new z-level (== false).
    // This caches the z-level we start the movement on (current) and the level we're want to end.
    const int z_before = get_levz();
    const int z_after = get_levz() + movez;
    if( z_after < -OVERMAP_DEPTH || z_after > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to move outside allowed range of z-levels" );
        return;
    }

    // Shift the map up or down

    std::unique_ptr<map> tmp_map_ptr;
    if( !m.has_zlevels() ) {
        tmp_map_ptr.reset( new map() );
    }

    map &maybetmp = m.has_zlevels() ? m : *( tmp_map_ptr.get() );
    if( m.has_zlevels() ) {
        // We no longer need to shift the map here! What joy
    } else {
        maybetmp.load(get_levx(), get_levy(), z_after, false);
    }

    // Find the corresponding staircase
    bool rope_ladder = false;
    bool actually_moved = true;
    // TODO: Remove the stairfinding, make the mapgen gen aligned maps
    if( !force && !climbing ) {
        stairs = find_or_make_stairs( maybetmp, z_after, rope_ladder );
        actually_moved = stairs != tripoint_min;
    }

    if( !actually_moved ) {
        return;
    }

    if( !force ) {
        monstairz = z_before;
    }
    // Save all monsters that can reach the stairs, remove them from the tracker,
    // then despawn the remaining monsters. Because it's a vertical shift, all
    // monsters are out of the bounds of the map and will despawn.
    if( !m.has_zlevels() ) {
        const int to_x = u.posx();
        const int to_y = u.posy();
        for( unsigned int i = 0; i < num_zombies(); ) {
            monster &critter = zombie(i);
            int turns = critter.turns_to_reach( to_x, to_y );
            if( turns < 10 && coming_to_stairs.size() < 8 && critter.will_reach( to_x, to_y )
                && !slippedpast) {
                critter.staircount = 10 + turns;
                critter.on_unload();
                coming_to_stairs.push_back(critter);
                remove_zombie( i );
            } else {
                i++;
            }
        }

        shift_monsters( 0, 0, movez );
    }

    std::vector<npc*> npcs_to_bring;
    if( !m.has_zlevels() && abs( movez ) == 1 ) {
        std::copy_if( active_npc.begin(), active_npc.end(), back_inserter( npcs_to_bring ),
                      [this]( npc *np ) {
            return np->is_friend() && rl_dist( np->pos(), u.pos() ) < 2;
        } );
    }

    u.moves -= move_cost;

    vertical_shift( z_after );
    if( !force ) {
        update_map( stairs.x, stairs.y );
    }

    if( !npcs_to_bring.empty() ) {
        // Would look nicer randomly scrambled
        auto candidates = closest_tripoints_first( 1, u.pos() );
        std::remove_if( candidates.begin(), candidates.end(), [this]( const tripoint &c ) {
            return !is_empty( c );
        } );

        for( npc *np : npcs_to_bring ) {
            const auto found = std::find_if( candidates.begin(), candidates.end(),
                [this, np]( const tripoint &c ) {
                return !np->is_dangerous_fields( m.field_at( c ) ) && m.tr_at( c ).is_benign();
            } );

            if( found != candidates.end() ) {
                // @todo De-uglify
                np->setpos( *found );
                np->place_on_map();
                np->setpos( *found );
                candidates.erase( found );
            }

            if( candidates.empty() ) {
                break;
            }
        }

        reload_npcs();
    }

    if( rope_ladder ) {
        m.ter_set( u.pos(), t_rope_up );
    }

    if( m.ter( stairs ) == t_manhole_cover ) {
        m.spawn_item( stairs + point( rng(-1, 1), rng(-1, 1) ), "manhole_cover" );
        m.ter_set( stairs, t_manhole );
    }

    refresh_all();
    // Upon force movement, traps can not be avoided.
    m.creature_on_trap( u, !force );
}

tripoint game::find_or_make_stairs( map &mp, const int z_after, bool &rope_ladder )
{
    const int omtilesz = SEEX * 2;
    real_coords rc( m.getabs(u.posx(), u.posy()) );
    tripoint omtile_align_start( m.getlocal(rc.begin_om_pos()), z_after );
    tripoint omtile_align_end( omtile_align_start.x + omtilesz - 1, omtile_align_start.y + omtilesz - 1, omtile_align_start.z );

    // Try to find the stairs.
    tripoint stairs = tripoint_min;
    int best = INT_MAX;
    const int movez = z_after - get_levz();
    for( const tripoint &dest : m.points_in_rectangle( omtile_align_start, omtile_align_end ) ) {
        if( rl_dist( u.pos(), dest ) <= best &&
            ((movez == -1 && mp.has_flag("GOES_UP", dest)) ||
             (movez == 1 && (mp.has_flag("GOES_DOWN", dest) ||
                             mp.ter(dest) == t_manhole_cover)) ||
             ((movez == 2 || movez == -2) && mp.ter(dest) == t_elevator))) {
            stairs = dest;
            best = rl_dist( u.pos(), dest );
        }
    }

    if( stairs != tripoint_min ) {
        // Stairs found
        return stairs;
    }

    // No stairs found! Try to make some
    rope_ladder = false;
    stairs = u.pos();
    stairs.z = z_after;
    // Check the destination area for lava.
    if( mp.ter(stairs) == t_lava ) {
        if( movez < 0 &&
            !query_yn(_("There is a LOT of heat coming out of there.  Descend anyway?")) ) {
            return tripoint_min;
        } else if( movez > 0 && !query_yn(_("There is a LOT of heat coming out of there.  Ascend anyway?")) ){
            return tripoint_min;
        }

        return stairs;
    }

    if( movez > 0 ) {
        // Manhole covers need this to work
        // Maybe require manhole cover here and fail otherwise?
        return stairs;
    }

    if( mp.impassable( stairs ) ) {
        popup(_("Halfway down, the way down becomes blocked off."));
        return tripoint_min;
    }

    if( u.has_trait( "WEB_RAPPEL" ) ) {
        if (query_yn(_("There is a sheer drop halfway down. Web-descend?"))) {
            rope_ladder = true;
            if ((rng(4, 8)) < u.get_skill_level( skill_dodge )) {
                add_msg(_("You attach a web and dive down headfirst, flipping upright and landing on your feet."));
            } else {
                add_msg(_("You securely web up and work your way down, lowering yourself safely."));
            }
        } else {
            return tripoint_min;
        }
    } else if (u.has_trait("VINES2") || u.has_trait("VINES3")) {
        if (query_yn(_("There is a sheer drop halfway down.  Use your vines to descend?"))) {
            if (u.has_trait("VINES2")) {
                if (query_yn(_("Detach a vine?  It'll hurt, but you'll be able to climb back up..."))) {
                    rope_ladder = true;
                    add_msg(m_bad, _("You descend on your vines, though leaving a part of you behind stings."));
                    u.mod_pain(5);
                    u.apply_damage( nullptr, bp_torso, 5 );
                    u.mod_hunger(10);
                    u.mod_thirst(10);
                } else {
                    add_msg(_("You gingerly descend using your vines."));
                }
            } else {
                add_msg(_("You effortlessly lower yourself and leave a vine rooted for future use."));
                rope_ladder = true;
                u.mod_hunger(10);
                u.mod_thirst(10);
            }
        } else {
            return tripoint_min;
        }
    } else if (u.has_amount("grapnel", 1)) {
        if (query_yn(_("There is a sheer drop halfway down. Climb your grappling hook down?"))) {
            rope_ladder = true;
            u.use_amount("grapnel", 1);
        } else {
            return tripoint_min;
        }
    } else if (u.has_amount("rope_30", 1)) {
        if (query_yn(_("There is a sheer drop halfway down. Climb your rope down?"))) {
            rope_ladder = true;
            u.use_amount("rope_30", 1);
        } else {
            return tripoint_min;
        }
    } else if( !query_yn( _("There is a sheer drop halfway down.  Jump?") ) ) {
        return tripoint_min;
    }

    return stairs;
}

void game::vertical_shift( const int z_after )
{
    if( z_after < -OVERMAP_DEPTH || z_after > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  z_after, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return;
    }

    // TODO: Implement dragging stuff up/down
    u.grab_point = tripoint_zero;
    u.grab_type = OBJECT_NONE;

    scent.reset();

    u.setz( z_after );
    const int z_before = get_levz();
    if( !m.has_zlevels() ) {
        m.clear_vehicle_cache( z_before );
        m.access_cache( z_before ).vehicle_list.clear();
        m.set_transparency_cache_dirty( z_before );
        m.set_outside_cache_dirty( z_before );
        m.load( get_levx(), get_levy(), z_after, true );
        shift_monsters( 0, 0, z_after - z_before );
        reload_npcs();
    } else {
        // Shift the map itself
        m.vertical_shift( z_after );
    }

    m.spawn_monsters( true );

    vertical_notes( z_before, z_after );
}

void game::vertical_notes( int z_before, int z_after )
{
    if( z_before == z_after || !get_option<bool>( "AUTO_NOTES" ) ) {
        return;
    }

    if( !m.inbounds_z( z_before ) || !m.inbounds_z( z_after ) ) {
        debugmsg( "game::vertical_notes invalid arguments: z_before == %d, z_after == %d",
                  z_before, z_after );
        return;
    }
    // Figure out where we know there are up/down connectors
    // Fill in all the tiles we know about (e.g. subway stations)
    static const int REVEAL_RADIUS = 40;
    const tripoint gpos = u.global_omt_location();
    for (int x = -REVEAL_RADIUS; x <= REVEAL_RADIUS; x++) {
        for (int y = -REVEAL_RADIUS; y <= REVEAL_RADIUS; y++) {
            const int cursx = gpos.x + x;
            const int cursy = gpos.y + y;
            if( !overmap_buffer.seen( cursx, cursy, z_before ) ) {
                continue;
            }
            if( overmap_buffer.has_note( cursx, cursy, z_after ) ) {
                // Already has a note -> never add an AUTO-note
                continue;
            }
            const oter_id &ter = overmap_buffer.ter(cursx, cursy, z_before);
            const oter_id &ter2 = overmap_buffer.ter(cursx, cursy, z_after);
            if( z_after > z_before && ter->has_flag(known_up) &&
                !ter2->has_flag(known_down) ) {
                overmap_buffer.set_seen(cursx, cursy, z_after, true);
                overmap_buffer.add_note(cursx, cursy, z_after, _(">:W;AUTO: goes down"));
            } else if ( z_after < z_before && ter->has_flag(known_down) &&
                !ter2->has_flag(known_up) ) {
                overmap_buffer.set_seen(cursx, cursy, z_after, true);
                overmap_buffer.add_note(cursx, cursy, z_after, _("<:W;AUTO: goes up"));
            }
        }
    }
}

void game::update_map( player &p )
{
    int x = p.posx();
    int y = p.posy();
    update_map( x, y );
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

    if( shiftx == 0 && shifty == 0 ) {
        // adjust player position
        u.setpos( tripoint(x, y, get_levz()) );
        // Not actually shifting the submaps, all the stuff below would do nothing
        return;
    }

    // this handles loading/unloading submaps that have scrolled on or off the viewport
    m.shift( shiftx, shifty );

    // Shift monsters
    shift_monsters( shiftx, shifty, 0 );
    u.shift_destination(-shiftx * SEEX, -shifty * SEEY);

    // Shift NPCs
    for (std::vector<npc *>::iterator it = active_npc.begin();
         it != active_npc.end();) {
        (*it)->shift(shiftx, shifty);
        if( (*it)->posx() < 0 - SEEX * 2 || (*it)->posy() < 0 - SEEX * 2 ||
            (*it)->posx() > SEEX * (MAPSIZE + 2) || (*it)->posy() > SEEY * (MAPSIZE + 2) ) {
            //Remove the npc from the active list. It remains in the overmap list.
            (*it)->on_unload();
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

    scent.shift( shiftx * SEEX, shifty * SEEY );

    // Make sure map cache is consistent since it may have shifted.
    m.build_map_cache( get_levz() );

    // Also ensure the player is on current z-level
    // get_levz() should later be removed, when there is no longer such a thing
    // as "current z-level"
    u.setpos( tripoint(x, y, get_levz()) );

    // Update what parts of the world map we can see
    update_overmap_seen();
}

void game::update_overmap_seen()
{
    const tripoint ompos = u.global_omt_location();
    const int dist = u.overmap_sight_range(light_level( u.posz() ));
    // We can always see where we're standing
    overmap_buffer.set_seen(ompos.x, ompos.y, ompos.z, true);
    for (int x = ompos.x - dist; x <= ompos.x + dist; x++) {
        for (int y = ompos.y - dist; y <= ompos.y + dist; y++) {
            const std::vector<point> line = line_to(ompos.x, ompos.y, x, y, 0);
            int sight_points = dist;
            for (std::vector<point>::const_iterator it = line.begin();
                 it != line.end() && sight_points >= 0; ++it) {
                const oter_id &ter = overmap_buffer.ter(it->x, it->y, ompos.z);
                sight_points -= int( ter->see_cost );
            }
            if (sight_points >= 0) {
                overmap_buffer.set_seen(x, y, ompos.z, true);
            }
        }
    }
}

void game::replace_stair_monsters()
{
    for( auto &elem : coming_to_stairs ) {
        elem.staircount = 0;
        tripoint spawn_point( elem.posx(), elem.posy(), get_levz() );
        // Find some better spots if current is occupied
        // If we can't, just destroy the poor monster
        for( size_t i = 0; i < 10; i++ ) {
            if( is_empty( spawn_point ) && elem.can_move_to( spawn_point ) ) {
                elem.spawn( spawn_point );
                add_zombie( elem );
                break;
            }

            spawn_point.x = elem.posx() + rng( -10, 10 );
            spawn_point.y = elem.posy() + rng( -10, 10 );
        }
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

    const bool from_below = monstairz < get_levz();

    if( coming_to_stairs.empty() ) {
        return;
    }

    if( m.has_zlevels() ) {
        debugmsg( "%d monsters coming to stairs on a map with z-levels",
                  coming_to_stairs.size() );
        coming_to_stairs.clear();
    }

    for (int x = 0; x < SEEX * MAPSIZE; x++) {
        for (int y = 0; y < SEEY * MAPSIZE; y++) {
            tripoint dest( x, y, u.posz() );
            if( ( from_below && m.has_flag( "GOES_DOWN", dest ) ) ||
                ( !from_below && m.has_flag( "GOES_UP", dest ) ) ) {
                stairx.push_back(x);
                stairy.push_back(y);
                stairdist.push_back(rl_dist(dest, u.pos()));
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
        const tripoint dest{mposx, mposy, g->get_levz()};

        // We might be not be visible.
        if( (critter.posx() < 0 - (SEEX * MAPSIZE) / 6 ||
             critter.posy() < 0 - (SEEY * MAPSIZE) / 6 ||
             critter.posx() > (SEEX * MAPSIZE * 7) / 6 ||
             critter.posy() > (SEEY * MAPSIZE * 7) / 6) ) {
            continue;
        }

        critter.staircount -= 4;
        // Let the player know zombies are trying to come.
        if( u.sees( dest ) ) {
            std::stringstream dump;
            if( critter.staircount > 4 ) {
                dump << string_format(_("You see a %s on the stairs"), critter.name().c_str());
            } else {
                if( critter.staircount > 0 ) {
                    dump << (from_below ?
                             //~ The <monster> is almost at the <bottom/top> of the <terrain type>!
                             string_format(_("The %1$s is almost at the top of the %2$s!"),
                                           critter.name().c_str(),
                                           m.tername(dest).c_str()) :
                             string_format(_("The %1$s is almost at the bottom of the %2$s!"),
                                           critter.name().c_str(),
                                           m.tername(dest).c_str()));
                }
            }

            add_msg(m_warning, dump.str().c_str());
        } else {
            sounds::sound(dest, 5, _("a sound nearby from the stairs!"));
        }

        if( critter.staircount > 0 ) {
            continue;
        }

        if( is_empty(dest) ) {
            critter.spawn( dest );
            critter.staircount = 0;
            add_zombie(critter);
            if (u.sees(dest)) {
                if (!from_below) {
                    add_msg(m_warning, _("The %1$s comes down the %2$s!"),
                            critter.name().c_str(),
                            m.tername(dest).c_str());
                } else {
                    add_msg(m_warning, _("The %1$s comes up the %2$s!"),
                            critter.name().c_str(),
                            m.tername(dest).c_str());
                }
            }
            coming_to_stairs.erase(coming_to_stairs.begin() + i);
            continue;
        } else if( u.pos() == dest ) {
            // Monster attempts to push player of stairs
            int pushx = -1;
            int pushy = -1;
            int tries = 0;

            // the critter is now right on top of you and will attack unless
            // it can find a square to push you into with one of his tries.
            const int creature_push_attempts = 9;
            const int player_throw_resist_chance = 3;

            critter.spawn( dest );
            while (tries < creature_push_attempts) {
                tries++;
                pushx = rng(-1, 1), pushy = rng(-1, 1);
                int iposx = mposx + pushx;
                int iposy = mposy + pushy;
                tripoint pos( iposx, iposy, get_levz() );
                if ((pushx != 0 || pushy != 0) && (mon_at(pos) == -1) &&
                    critter.can_move_to( pos )) {
                    bool resiststhrow = (u.is_throw_immune()) ||
                                        (u.has_trait("LEG_TENT_BRACE"));
                    if (resiststhrow && one_in(player_throw_resist_chance)) {
                        u.moves -= 25; // small charge for avoiding the push altogether
                        add_msg(_("The %s fails to push you back!"),
                                critter.name().c_str());
                        return; //judo or leg brace prevent you from getting pushed at all
                    }
                    // Not accounting for tentacles latching on, so..
                    // Something is about to happen, lets charge half a move
                    u.moves -= 50;
                    if (resiststhrow && (u.is_throw_immune())) {
                        //we have a judoka who isn't getting pushed but counterattacking now.
                        mattack::thrown_by_judo(&critter);
                        return;
                    }
                    std::string msg = "";
                    ///\EFFECT_DODGE reduces chance of being downed when pushed off the stairs
                    if (!(resiststhrow) && (u.get_dodge() + rng(0, 3) < 12)) {
                        // dodge 12 - never get downed
                        // 11.. avoid 75%; 10.. avoid 50%; 9.. avoid 25%
                        u.add_effect( effect_downed, 2);
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
            u.moves -= 50;
            return;
        } else if( mon_at( dest ) != -1) {
            // Monster attempts to displace a monster from the stairs
            monster &other = zombie( mon_at( dest ) );
            critter.spawn( dest );

            // the critter is now right on top of another and will push it
            // if it can find a square to push it into inside of his tries.
            const int creature_push_attempts = 9;
            const int creature_throw_resist = 4;

            int tries = 0;
            int pushx = 0;
            int pushy = 0;
            while( tries < creature_push_attempts ) {
                tries++;
                pushx = rng(-1, 1);
                pushy = rng(-1, 1);
                int iposx = mposx + pushx;
                int iposy = mposy + pushy;
                tripoint pos( iposx, iposy, get_levz() );
                if ((pushx == 0 && pushy == 0) || ((iposx == u.posx()) && (iposy == u.posy()))) {
                    continue;
                }
                if ((mon_at(pos) == -1) && other.can_move_to(pos)) {
                    other.setpos( tripoint(iposx, iposy, get_levz()) );
                    other.moves -= 50;
                    std::string msg = "";
                    if (one_in(creature_throw_resist)) {
                        other.add_effect( effect_downed, 2);
                        msg = _("The %1$s pushed the %2$s hard.");
                    } else {
                        msg = _("The %1$s pushed the %2$s.");
                    };
                    add_msg(msg.c_str(), critter.name().c_str(), other.name().c_str());
                    return;
                }
            }
            return;
        }
    }
}

void game::despawn_monster(int mondex)
{
    monster &critter = zombie( mondex );
    if( !critter.is_hallucination() ) {
        // hallucinations aren't stored, they come and go as they like,
        overmap_buffer.despawn_monster( critter );
    }

    critter.on_unload();
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

        if( m.inbounds( critter.pos() ) && ( shiftz == 0 || m.has_zlevels() ) ) {
            i++;
            // We're inbounds, so don't despawn after all.
            // No need to shift z coords, they are absolute
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
    if( !get_world_option<bool>( "RANDOM_NPC" ) ) {
        return;
    }

    float density = get_world_option<float>( "NPC_DENSITY" );
    const int npc_num = get_cur_om().npcs.size();
    if( npc_num > 0 ) {
        // 100%, 80%, 64%, 52%, 41%, 33%...
        density *= powf( 0.8f, npc_num );
    }

    if( x_in_y( density, 100 ) ) {
        npc *tmp = new npc();
        tmp->normalize();
        tmp->randomize();
        //tmp->stock_missions();
        // Create the NPC in one of the outermost submaps,
        // hopefully far away to be invisible to the player,
        // to prevent NPCs appearing out of thin air.
        // This can be changed to let the NPC spawn further away,
        // so it does not became active immediately.
        int msx = get_levx();
        int msy = get_levy();
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
        tmp->spawn_at( msx, msy, get_levz() );
        tmp->form_opinion( u );
        tmp->mission = NPC_MISSION_NULL;
        tmp->add_new_mission( mission::reserve_random(ORIGIN_ANY_NPC, tmp->global_omt_location(), tmp->getID()) );
        // This will make the new NPC active
        load_npcs();
    }
}

void game::wait()
{
    std::map<int, int> durations;
    uimenu as_m;

    const bool has_watch = u.has_watch();
    const auto add_menu_item = [ &as_m, &durations, has_watch ]
        ( int retval, int hotkey, const std::string &caption = "", int duration = calendar::INDEFINITELY_LONG ) {

        std::string text( caption );

        if( has_watch && duration != calendar::INDEFINITELY_LONG ) {
            const std::string dur_str( calendar::print_duration( duration ) );
            text += ( text.empty() ? dur_str : string_format( " (%s)", dur_str.c_str() ) );
        }
        as_m.addentry( retval, true, hotkey, text );
        durations[retval] = duration;
    };

    add_menu_item( 1, '1', !has_watch ? _( "Wait 300 heartbeats" ) : "", MINUTES( 5 ) );
    add_menu_item( 2, '2', !has_watch ? _( "Wait 1800 heartbeats" ) : "", MINUTES( 30 ) );

    if( has_watch ) {
        add_menu_item( 3, '3', "", HOURS( 1 ) );
        add_menu_item( 4, '4', "", HOURS( 2 ) );
        add_menu_item( 5, '5', "", HOURS( 3 ) );
        add_menu_item( 6, '6', "", HOURS( 6 ) );
    }

    add_menu_item( 7,  'd', _( "Wait till dawn" ),     calendar::turn.diurnal_time_before( calendar::turn.sunrise() ) );
    add_menu_item( 8,  'n', _( "Wait till noon" ),     calendar::turn.diurnal_time_before( HOURS( 12 ) ) );
    add_menu_item( 9,  'k', _( "Wait till dusk" ),     calendar::turn.diurnal_time_before( calendar::turn.sunset() ) );
    add_menu_item( 10, 'm', _( "Wait till midnight" ), calendar::turn.diurnal_time_before( HOURS( 0 ) ) );
    add_menu_item( 11, 'w', _( "Wait till weather changes" ) );
    add_menu_item( 12, 'q', _( "Exit" ) );

    as_m.text = ( has_watch ) ? string_format( _( "It's %s now. " ), calendar::turn.print_time().c_str() ) : "";
    as_m.text += _( "Wait for how long?" );
    as_m.return_invalid = true;
    as_m.query(); /* calculate key and window variables, generate window, and loop until we get a valid answer */

    if( as_m.ret == 12 || durations.count( as_m.ret ) == 0 ) {
        return;
    }

    activity_type actType = ( as_m.ret == 11 ) ? ACT_WAIT_WEATHER : ACT_WAIT;

    player_activity new_act( actType, 100 * ( durations[as_m.ret] - 1 ), 0 );

    u.assign_activity( new_act, false );
    u.rooted_message();
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
    int tries = 0;
    tripoint new_pos = p->pos();
    bool is_u = (p == &u);

    if (add_teleglow) {
        p->add_effect( effect_teleglow, 300);
    }
    do {
        new_pos.x = p->posx() + rng(0, SEEX * 2) - SEEX;
        new_pos.y = p->posy() + rng(0, SEEY * 2) - SEEY;
        tries++;
    } while ( tries < 15 && m.impassable( new_pos ) );
    bool can_see = ( is_u || u.sees( new_pos ) );
    if (p->in_vehicle) {
        m.unboard_vehicle(p->pos());
    }
    p->setx( new_pos.x );
    p->sety( new_pos.y );
    if( m.impassable( new_pos ) ) { //Teleported into a wall
        if (can_see) {
            if (is_u) {
                add_msg(_("You teleport into the middle of a %s!"),
                        m.obstacle_name( new_pos ).c_str() );
                p->add_memorial_log(pgettext("memorial_male", "Teleported into a %s."),
                                    pgettext("memorial_female", "Teleported into a %s."),
                                    m.obstacle_name( new_pos ).c_str() );
            } else {
                add_msg(_("%1$s teleports into the middle of a %2$s!"),
                        p->name.c_str(), m.obstacle_name( new_pos ).c_str() );
            }
        }
        p->apply_damage( nullptr, bp_torso, 500 );
        p->check_dead_state();
    } else if (can_see) {
        const int i = mon_at( new_pos );
        if (i != -1) {
            monster &critter = zombie(i);
            if (is_u) {
                add_msg(_("You teleport into the middle of a %s!"),
                        critter.name().c_str());
                u.add_memorial_log(pgettext("memorial_male", "Telefragged a %s."),
                                   pgettext("memorial_female", "Telefragged a %s."),
                                   critter.name().c_str());
            } else {
                add_msg(_("%1$s teleports into the middle of a %2$s!"),
                        p->name.c_str(), critter.name().c_str());
            }
            critter.die_in_explosion( p );
        }
    }
    if( is_u ) {
        update_map( *p );
    }
}

void game::nuke( const tripoint &p )
{
    // TODO: nukes hit above surface, not critter = 0
    // TODO: Z
    int x = p.x;
    int y = p.y;
    tinymap tmpmap;
    tmpmap.load( x * 2, y * 2, 0, false);
    tripoint dest( 0, 0, p.z );
    int &i = dest.x;
    int &j = dest.y;
    for( i = 0; i < SEEX * 2; i++ ) {
        for( j = 0; j < SEEY * 2; j++ ) {
            if (!one_in(10)) {
                tmpmap.make_rubble( dest, f_rubble_rock, true, t_dirt, true);
            }
            if (one_in(3)) {
                tmpmap.add_field( dest, fd_nuke_gas, 3, 0 );
            }
            tmpmap.adjust_radiation( dest, rng(20, 80));
        }
    }
    tmpmap.save();
    overmap_buffer.ter(x, y, 0) = oter_id( "crater" );
    // Kill any npcs on that omap location.
    std::vector<npc *> npcs = overmap_buffer.get_npcs_near_omt(x, y, 0, 0);
    for( auto &npc : npcs ) {
        npc->marked_for_death = true;
    }
}

bool game::spread_fungus( const tripoint &p )
{
    int growth = 1;
    for( int i = p.x - 1; i <= p.x + 1; i++ ) {
        for( int j = p.y - 1; j <= p.y + 1; j++ ) {
            if (i == p.x && j == p.y) {
                continue;
            }
            if( m.has_flag("FUNGUS", tripoint( i, j, p.z ) ) ) {
                growth += 1;
            }
        }
    }

    bool converted = false;
    if (!m.has_flag_ter("FUNGUS", p)) {
        // Terrain conversion
        if (m.has_flag_ter("DIGGABLE", p)) {
            if (x_in_y(growth * 10, 100)) {
                m.ter_set(p, t_fungus);
                converted = true;
            }
        } else if (m.has_flag("FLAT", p)) {
            if( m.has_flag( TFLAG_INDOORS, p ) ) {
                if (x_in_y(growth * 10, 500)) {
                    m.ter_set(p, t_fungus_floor_in);
                    converted = true;
                }
            } else if( m.has_flag( TFLAG_SUPPORTS_ROOF, p ) ) {
                if (x_in_y(growth * 10, 1000)) {
                    m.ter_set(p, t_fungus_floor_sup);
                    converted = true;
                }
            } else {
                if (x_in_y(growth * 10, 2500)) {
                    m.ter_set(p, t_fungus_floor_out);
                    converted = true;
                }
            }
        } else if (m.has_flag("SHRUB", p)) {
            if (x_in_y(growth * 10, 200)) {
                m.ter_set(p, t_shrub_fungal);
                converted = true;
            } else if (x_in_y(growth, 1000)) {
                m.ter_set(p, t_marloss);
                converted = true;
            }
        } else if (m.has_flag("THIN_OBSTACLE", p)) {
            if (x_in_y(growth * 10, 150)) {
                m.ter_set(p, t_fungus_mound);
                converted = true;
            }
        } else if (m.has_flag("YOUNG", p)) {
            if (x_in_y(growth * 10, 500)) {
                m.ter_set(p, t_tree_fungal_young);
                converted = true;
            }
        } else if (m.has_flag("WALL", p)) {
            if (x_in_y(growth * 10, 5000)) {
                converted = true;
                m.ter_set(p, t_fungus_wall);
            }
        }
        // Furniture conversion
        if (converted) {
            if (m.has_flag("FLOWER", p)) {
                m.furn_set(p, f_flower_fungal);
            } else if (m.has_flag("ORGANIC", p)) {
                if (m.furn(p).obj().movecost == -10) {
                    m.furn_set(p, f_fungal_mass);
                } else {
                    m.furn_set(p, f_fungal_clump);
                }
            } else if (m.has_flag("PLANT", p)) {
                // Replace the (already existing) seed
                m.i_at( p )[0] = item( "fungal_seeds", calendar::turn );
            }
        }
        return true;
    } else {
        // Everything is already fungus
        if (growth == 9) {
            return false;
        }
        for (int i = p.x - 1; i <= p.x + 1; i++) {
            for (int j = p.y - 1; j <= p.y + 1; j++) {
                tripoint dest( i, j, p.z );
                // One spread on average
                if (!m.has_flag("FUNGUS", dest) && one_in(9 - growth)) {
                    //growth chance is 100 in X simplified
                    if (m.has_flag("DIGGABLE", dest)) {
                        m.ter_set(dest, t_fungus);
                        converted = true;
                    } else if (m.has_flag("FLAT", dest)) {
                        if( m.has_flag( TFLAG_INDOORS, dest ) ) {
                            if (one_in(5)) {
                                m.ter_set(dest, t_fungus_floor_in);
                                converted = true;
                            }
                        } else if( m.has_flag( TFLAG_SUPPORTS_ROOF, dest ) ) {
                            if (one_in(10)) {
                                m.ter_set(dest, t_fungus_floor_sup);
                                converted = true;
                            }
                        } else {
                            if (one_in(25)) {
                                m.ter_set(dest, t_fungus_floor_out);
                                converted = true;
                            }
                        }
                    } else if (m.has_flag("SHRUB", dest)) {
                        if (one_in(2)) {
                            m.ter_set(dest, t_shrub_fungal);
                            converted = true;
                        } else if (one_in(25)) {
                            m.ter_set(dest, t_marloss);
                            converted = true;
                        }
                    } else if (m.has_flag("THIN_OBSTACLE", dest)) {
                        if (x_in_y(10, 15)) {
                            m.ter_set(dest, t_fungus_mound);
                            converted = true;
                        }
                    } else if (m.has_flag("YOUNG", dest)) {
                        if (one_in(5)) {
                            if( m.get_field_strength( p, fd_fungal_haze ) != 0 ) {
                                if (one_in(8)) { // young trees are vulnerable
                                    m.ter_set(dest, t_fungus);
                                    summon_mon( mon_fungal_blossom, p );
                                    if (u.sees(p)) {
                                        add_msg(m_warning, _("The young tree blooms forth into a fungal blossom!"));
                                    }
                                } else if (one_in(4)) {
                                    m.ter_set(dest, t_marloss_tree);
                                }
                            } else {
                                m.ter_set(dest, t_tree_fungal_young);
                            }
                            converted = true;
                        }
                    } else if (m.has_flag("TREE", dest)) {
                        if (one_in(10)) {
                            if( m.get_field_strength( p, fd_fungal_haze ) != 0) {
                                if (one_in(10)) {
                                    m.ter_set(dest, t_fungus);
                                    summon_mon( mon_fungal_blossom, p );
                                    if (u.sees(p)) {
                                        add_msg(m_warning, _("The tree blooms forth into a fungal blossom!"));
                                    }
                                } else if (one_in(6)) {
                                    m.ter_set(dest, t_marloss_tree);
                                }
                            } else {
                                m.ter_set(dest, t_tree_fungal);
                            }
                            converted = true;
                        }
                    } else if (m.has_flag("WALL", dest)) {
                        if (one_in(50)) {
                            converted = true;
                            m.ter_set(dest, t_fungus_wall);
                        }
                    }

                    if (converted) {
                        if (m.has_flag("FLOWER", dest)) {
                            m.furn_set(dest, f_flower_fungal);
                        } else if (m.has_flag("ORGANIC", dest)) {
                            if (m.furn(dest).obj().movecost == -10) {
                                m.furn_set(dest, f_fungal_mass);
                            } else {
                                m.furn_set(dest, f_fungal_clump);
                            }
                        } else if (m.has_flag("PLANT", dest)) {
                            // Replace the (already existing) seed
                            m.i_at( p )[0] = item( "fungal_seeds", calendar::turn );
                        }
                    }
                }
            }
        }
        return false;
    }
}

std::vector<faction *> game::factions_at( const tripoint &p )
{
    // TODO: Factions with z-levels
    std::vector<faction *> ret;
    for( auto &elem : factions ) {
        if( trig_dist( p.x, p.y, elem.mapx, elem.mapy ) <= elem.size ) {
            ret.push_back( &( elem ) );
        }
    }
    return ret;
}

void game::display_scent()
{
    int div;
    bool got_value = query_int( div, _("Set the Scent Map sensitivity to (0 to cancel)?") );
    if ( !got_value || div < 1 ) {
        add_msg(_("Never mind."));
        return;
    };
    draw_ter();
    scent.draw( w_terrain, div * 2, u.pos() );
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
    popup_nowait(_("Saving game, this may take a while"));

    time_t now = time(NULL);    //timestamp for start of saving procedure

    //perform save
    save();
    //Pull all of the mission_npc's back out of the world map where they are saved
    mission_npc.clear();
    load_mission_npcs();
    //Now reset counters for autosaving, so we don't immediately autosave after a quicksave or autosave.
    moves_since_last_save = 0;
    last_save_timestamp = now;
}

void game::quickload()
{
    const WORLDPTR active_world = world_generator->active_world;
    if ( active_world == nullptr ) {
        return;
    }

    const std::string &save_name = base64_encode(u.name);
    if( active_world->save_exists( save_name ) ) {
        if( moves_since_last_save != 0 ) { // See if we need to reload anything
            MAPBUFFER.reset();
            overmap_buffer.clear();
            try {
                setup();
            } catch( const std::exception &err ) {
                debugmsg( "Error: %s", err.what() );
            }
            load( active_world->world_name, save_name );
        }
    } else {
        popup_getkey( _( "No saves for %s yet." ), u.name.c_str() );
    }
}

void game::autosave()
{
    //Don't autosave if the min-autosave interval has not passed since the last autosave/quicksave.
    if (time(NULL) < last_save_timestamp + 60 * get_option<int>( "AUTOSAVE_MINUTES" ) ) {
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

#if !(defined _WIN32 || defined WINDOWS || defined TILES)
    // Check whether LC_CTYPE supports the UTF-8 encoding
    // and show a warning if it doesn't
    if (std::strcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        const char *unicode_error_msg =
            _("You don't seem to have a valid Unicode locale. You may see some weird "
              "characters (e.g. empty boxes or question marks). You have been warned.");
        fold_and_print(tmp, 0, 0, maxx, c_white, unicode_error_msg, minWidth, minHeight, maxx, maxy);
        wrefresh(tmp);
        wgetch(tmp);
        werase(tmp);
    }
#endif

    wrefresh(tmp);
    delwin(tmp);
    erase();
}

void game::process_artifact(item *it, player *p)
{
    const bool worn = p->is_worn( *it );
    const bool wielded = ( it == &p->weapon );
    std::vector<art_effect_passive> effects;
    effects = it->type->artifact->effects_carried;
    if( worn ) {
        auto &ew = it->type->artifact->effects_worn;
        effects.insert( effects.end(), ew.begin(), ew.end() );
    }
    if( wielded ) {
        auto &ew = it->type->artifact->effects_wielded;
        effects.insert( effects.end(), ew.begin(), ew.end() );
    }
    if( it->is_tool() ) {
        // Recharge it if necessary
        if( it->ammo_remaining() < it->ammo_capacity() ) {
            switch (it->type->artifact->charge_type) {
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
                    is_in_sunlight(p->pos())) {
                    it->charges++;
                }
                break;
            // Artifacts can inflict pain even on Deadened folks.
            // Some weird Lovecraftian thing.  ;P
            // (So DON'T route them through mod_pain!)
            case ARTC_PAIN:
                if (calendar::turn.seconds() == 0) {
                    add_msg(m_bad, _("You suddenly feel sharp pain for no reason."));
                    p->mod_pain_noresist( 3 * rng(1, 3) );
                    it->charges++;
                }
                break;
            case ARTC_HP:
                if (calendar::turn.seconds() == 0) {
                    add_msg(m_bad, _("You feel your body decaying."));
                    p->hurtall(1, nullptr);
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

        case AEP_PBLUE:
            if (p->radiation > 0) {
                p->radiation--;
            }
            break;

        case AEP_SMOKE:
            if (one_in(10)) {
                tripoint pt( p->posx() + rng(-1, 1),
                             p->posy() + rng(-1, 1),
                             p->posz() );
                if( m.add_field( pt, fd_smoke, rng(1, 3), 0 ) ) {
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
                    m.adjust_field_age( tripoint( x, y, p->posz() ), fd_fire, -1);
                }
            }

        case AEP_HUNGER:
            if (one_in(100)) {
                p->mod_hunger(1);
            }
            break;

        case AEP_THIRST:
            if (one_in(120)) {
                p->mod_thirst(1);
            }
            break;

        case AEP_EVIL:
            if (one_in(150)) { // Once every 15 minutes, on average
                p->add_effect( effect_evil, 300);
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
    calendar::start = HOURS(get_world_option<int>( "INITIAL_TIME" ) );
    if( scen->has_flag("SPR_START") || scen->has_flag("SUM_START") ||
        scen->has_flag("AUT_START") || scen->has_flag("WIN_START") ||
        scen->has_flag("SUM_ADV_START") ) {
        if( scen->has_flag("SPR_START") ) {
            ; // Do nothing;
        } else if( scen->has_flag("SUM_START") ) {
            calendar::start += DAYS( calendar::season_length() );
        } else if( scen->has_flag("AUT_START") ) {
            calendar::start += DAYS( calendar::season_length() * 2 );
        } else if( scen->has_flag("WIN_START") ) {
            calendar::start += DAYS( calendar::season_length() * 3 );
        } else if( scen->has_flag("SUM_ADV_START") ) {
            calendar::start += DAYS( calendar::season_length() * 5 );
        } else {
            debugmsg("The Unicorn");
        }
    } else {
        if( get_world_option<std::string>( "INITIAL_SEASON" ) == "spring" ) {
            calendar::initial_season = SPRING;
            ; // Do nothing.
        } else if( get_world_option<std::string>( "INITIAL_SEASON" ) == "summer") {
            calendar::initial_season = SUMMER;
            calendar::start += DAYS( calendar::season_length() );
        } else if( get_world_option<std::string>( "INITIAL_SEASON" ) == "autumn" ) {
            calendar::initial_season = AUTUMN;
            calendar::start += DAYS( calendar::season_length() * 2 );
        } else {
            calendar::initial_season = WINTER;
            calendar::start += DAYS( calendar::season_length() * 3 );
        }
    }
    calendar::turn = calendar::start;

    if ( get_world_option<bool>( "ETERNAL_SEASON" ) ) {
      calendar::eternal_season = true;
    }
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

        case AEP_PBLUE:
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

    if (!stat_info.empty()) {
        add_msg(stat_info.c_str());
    }


    if (net_speed != 0) {
        add_msg(m_info, _("Speed %s%d! "), (net_speed > 0 ? "+" : ""), net_speed);
    }
}

int game::get_levx() const
{
    return m.get_abs_sub().x;
}

int game::get_levy() const
{
    return m.get_abs_sub().y;
}

int game::get_levz() const
{
    return m.get_abs_sub().z;
}

overmap &game::get_cur_om() const
{
    // The player is located in the middle submap of the map.
    const tripoint sm = m.get_abs_sub() + tripoint( MAPSIZE / 2, MAPSIZE / 2, 0 );
    const tripoint pos_om = sm_to_om_copy( sm );
    return overmap_buffer.get( pos_om.x, pos_om.y );
}
