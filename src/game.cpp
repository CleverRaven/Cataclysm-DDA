#include "game.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <csignal>
#include <ctime>
#include <iterator>
#include <locale>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "action.h"
#include "activity_handlers.h"
#include "artifact.h"
#include "auto_pickup.h"
#include "bionics.h"
#include "bodypart.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "catalua.h"
#include "clzones.h"
#include "compatibility.h"
#include "computer.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "debug.h"
#include "debug_menu.h"
#include "dependency_tree.h"
#include "editmap.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "faction.h"
#include "filesystem.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "gamemode.h"
#include "gates.h"
#include "gun_mode.h"
#include "help.h"
#include "iexamine.h"
#include "init.h"
#include "input.h"
#include "item_category.h"
#include "item_group.h"
#include "item_location.h"
#include "iuse_actor.h"
#include "json.h"
#include "lightmap.h"
#include "line.h"
#include "live_view.h"
#include "loading_ui.h"
#include "lua_console.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "map_extras.h"
#include "mapsharing.h"
#include "martialarts.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "monattack.h"
#include "monexamine.h"
#include "monfaction.h"
#include "mongroup.h"
#include "monstergenerator.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "pickup.h"
#include "popup.h"
#include "projectile.h"
#include "ranged.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "scent_map.h"
#include "sidebar.h"
#include "sounds.h"
#include "start_location.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "submap.h"
#include "trait_group.h"
#include "translations.h"
#include "trap.h"
#include "uistate.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "vpart_reference.h"
#include "weather.h"
#include "weather_gen.h"
#include "worldfactory.h"
#include "map_selector.h"

#ifdef TILES
#include "cata_tiles.h"
#endif // TILES

#if (defined TILES || defined _WIN32 || defined WINDOWS)
#include "cursesport.h"
#endif

#if !(defined _WIN32 || defined WINDOWS || defined TILES)
#include <langinfo.h>
#include <cstring>
#endif

#if (defined _WIN32 || defined __WIN32__)
#if 1 // Hack to prevent reordering of #include "platform_win.h" by IWYU
#   include "platform_win.h"
#endif
#   include <tchar.h>
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

const int core_version = 6;
static constexpr int DANGEROUS_PROXIMITY = 5;

/** Will be set to true when running unit tests */
bool test_mode = false;

const mtype_id mon_manhack( "mon_manhack" );

const skill_id skill_melee( "melee" );
const skill_id skill_dodge( "dodge" );
const skill_id skill_driving( "driving" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_survival( "survival" );

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
const efftype_id effect_emp( "emp" );
const efftype_id effect_evil( "evil" );
const efftype_id effect_flu( "flu" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_has_bag( "has_bag" );
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_laserlocked( "laserlocked" );
const efftype_id effect_no_sight( "no_sight" );
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

static const bionic_id bio_remote( "bio_remote" );

static const trait_id trait_GRAZER( "GRAZER" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_RUMINANT( "RUMINANT" );
static const trait_id trait_SHELL2( "SHELL2" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );
static const trait_id trait_BURROW( "BURROW" );

void intro();

#ifdef __ANDROID__
extern std::map<std::string, std::list<input_event>> quick_shortcuts_map;
extern bool add_best_key_for_action_to_quick_shortcuts( action_id action,
        const std::string &category, bool back );
extern bool add_key_to_quick_shortcuts( long key, const std::string &category, bool back );
#endif

//The one and only game instance
std::unique_ptr<game> g;
#ifdef TILES
extern std::unique_ptr<cata_tiles> tilecontext;
extern void toggle_fullscreen_window();
#endif // TILES

uistatedata uistate;

bool is_valid_in_w_terrain( int x, int y )
{
    return x >= 0 && x < TERRAIN_WINDOW_WIDTH && y >= 0 && y < TERRAIN_WINDOW_HEIGHT;
}

// This is the main game set-up process.
game::game() :
    liveview( *liveview_ptr ),
    scent_ptr( *this ),
    new_game( false ),
    uquit( QUIT_NO ),
    m( *map_ptr ),
    u( *u_ptr ),
    scent( *scent_ptr ),
    events( *event_manager_ptr ),
    weather( WEATHER_CLEAR ),
    lightning_active( false ),
    u_shared_ptr( &u, null_deleter{} ),
    pixel_minimap_option( 0 ),
    safe_mode( SAFE_MODE_ON ),
    safe_mode_warning_logged( false ),
    mostseen( 0 ),
    nextweather( calendar::before_time_starts ),
    next_npc_id( 1 ),
    next_mission_id( 1 ),
    remoteveh_cache_time( calendar::before_time_starts ),
    user_action_counter( 0 ),
    tileset_zoom( 16 ),
    wind_direction_override(),
    windspeed_override(),
    weather_override( WEATHER_NULL )

{
    temperature = 0;
    player_was_sleeping = false;
    reset_light_level();
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
    narrow_sidebar = get_option<std::string>( "SIDEBAR_STYLE" ) == "narrow";
    right_sidebar = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    fullscreen = false;
    was_fullscreen = false;

    // These functions do not load stuff from json.
    // The content they load/initialize is hardcoded into the program.
    // Therefore they can be loaded here.
    // If this changes (if they load data from json), they have to
    // be moved to game::load_mod or game::load_core_data

    get_auto_pickup().load_global();
    get_safemode().load_global();
}

bool game::check_mod_data( const std::vector<mod_id> &opts, loading_ui &ui )
{
    auto &tree = world_generator->get_mod_manager().get_tree();

    // deduplicated list of mods to check
    std::set<mod_id> check( opts.begin(), opts.end() );

    // if no specific mods specified check all non-obsolete mods
    if( check.empty() ) {
        for( const mod_id &e : world_generator->get_mod_manager().all_mods() ) {
            if( !e->obsolete ) {
                check.emplace( e );
            }
        }
    }

    if( check.empty() ) {
        // if no loadable mods then test core data only
        try {
            load_core_data( ui );
            DynamicDataLoader::get_instance().finalize_loaded_data( ui );
        } catch( const std::exception &err ) {
            std::cerr << "Error loading data from json: " << err.what() << std::endl;
        }
    }

    for( const auto &e : check ) {
        if( !e.is_valid() ) {
            std::cerr << "Unknown mod: " << e.str() << std::endl;
            return false;
        }

        const MOD_INFORMATION &mod = *e;

        if( !tree.is_available( mod.ident ) ) {
            std::cerr << "Missing dependencies: " << mod.name() << "\n"
                      << tree.get_node( mod.ident )->s_errors() << std::endl;
            return false;
        }

        std::cout << "Checking mod " << mod.name() << " [" << mod.ident.str() << "]" << std::endl;

        try {
            load_core_data( ui );

            // Load any dependencies
            for( auto &dep : tree.get_dependencies_of_X_as_strings( mod.ident ) ) {
                load_data_from_dir( dep->path, dep->ident.str(), ui );
            }

            // Load mod itself
            load_data_from_dir( mod.path, mod.ident.str(), ui );
            DynamicDataLoader::get_instance().finalize_loaded_data( ui );
        } catch( const std::exception &err ) {
            std::cerr << "Error loading data: " << err.what() << std::endl;
        }
    }

    return true;
}

bool game::is_core_data_loaded() const
{
    return DynamicDataLoader::get_instance().is_data_finalized();
}

void game::load_core_data( loading_ui &ui )
{
    // core data can be loaded only once and must be first
    // anyway.
    DynamicDataLoader::get_instance().unload_data();

    init_lua();
    load_data_from_dir( FILENAMES[ "jsondir" ], "core", ui );
}

void game::load_data_from_dir( const std::string &path, const std::string &src, loading_ui &ui )
{
    // Process a preload file before the .json files,
    // so that custom IUSE's can be defined before
    // the items that need them are parsed
    lua_loadmod( path, "preload.lua" );

    DynamicDataLoader::get_instance().load_data_from_path( path, src, ui );

    // main.lua will be executed after JSON, allowing to
    // work with items defined by mod's JSON
    lua_loadmod( path, "main.lua" );
}

game::~game()
{
    MAPBUFFER.reset();
}

// Fixed window sizes
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7

#if (defined TILES)
// defined in sdltiles.cpp
void to_map_font_dimension( int &w, int &h );
void from_map_font_dimension( int &w, int &h );
void to_overmap_font_dimension( int &w, int &h );
void reinitialize_framebuffer();
#else
// unchanged, nothing to be translated without tiles
void to_map_font_dimension( int &, int & ) { }
void from_map_font_dimension( int &, int & ) { }
void to_overmap_font_dimension( int &, int & ) { }
//in pure curses, the framebuffer won't need reinitializing
void reinitialize_framebuffer() { }
#endif

void game::init_ui( const bool resized )
{
    // clear the screen
    static bool first_init = true;

    if( first_init ) {
        catacurses::clear();

        // print an intro screen, making sure the terminal is the correct size

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

    if( resized ) {
        get_options().get_option( "TERMINAL_X" ).setValue( TERMX * get_scaling_factor() );
        get_options().get_option( "TERMINAL_Y" ).setValue( TERMY * get_scaling_factor() );
        get_options().save();
    }
#else
    ( void ) resized;
    intro();

    TERMY = getmaxy( catacurses::stdscr );
    TERMX = getmaxx( catacurses::stdscr );

    // try to make FULL_SCREEN_HEIGHT symmetric according to TERMY
    if( TERMY % 2 ) {
        FULL_SCREEN_HEIGHT = 25;
    } else {
        FULL_SCREEN_HEIGHT = 24;
    }

    // now that TERMX and TERMY are set,
    // check if sidebar style needs to be overridden
    sidebarWidth = use_narrow_sidebar() ? 45 : 55;
    if( fullscreen ) {
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
    to_map_font_dimension( TERRAIN_WINDOW_WIDTH, TERRAIN_WINDOW_HEIGHT );

    VIEW_OFFSET_X = 0;
    VIEW_OFFSET_Y = 0;

    // VIEW_OFFSET_* are in standard font dimension.
    from_map_font_dimension( VIEW_OFFSET_X, VIEW_OFFSET_Y );

    // Position of the player in the terrain window, it is always in the center
    POSX = TERRAIN_WINDOW_WIDTH / 2;
    POSY = TERRAIN_WINDOW_HEIGHT / 2;

    // Set up the main UI windows.
    w_terrain = w_terrain_ptr = catacurses::newwin( TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH,
                                VIEW_OFFSET_Y, right_sidebar ? VIEW_OFFSET_X :
                                VIEW_OFFSET_X + sidebarWidth );
    werase( w_terrain );

    /**
     * Doing the same thing as above for the overmap
     */
    static const int OVERMAP_LEGEND_WIDTH = 28;
    OVERMAP_WINDOW_HEIGHT = TERMY;
    OVERMAP_WINDOW_WIDTH = TERMX - OVERMAP_LEGEND_WIDTH;
    to_overmap_font_dimension( OVERMAP_WINDOW_WIDTH, OVERMAP_WINDOW_HEIGHT );

    //Bring the framebuffer to the maximum required dimensions
    //Otherwise it segfaults when the overmap needs a bigger buffer size than it provides
    reinitialize_framebuffer();

    // minimapX x minimapY is always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
    int minimapX = 0;
    int minimapY = 0;
    int hpX = 0;
    int hpY = 0;
    int hpW = 0;
    int hpH = 0;
    int messX = 0;
    int messY = 0;
    int messW = 0;
    int messHshort = 0;
    int messHlong = 0;
    int locX = 0;
    int locY = 0;
    int locW = 0;
    int locH = 0;
    int statX = 0;
    int statY = 0;
    int statW = 0;
    int statH = 0;
    int stat2X = 0;
    int stat2Y = 0;
    int stat2W = 0;
    int stat2H = 0;
    int pixelminimapW = 0;
    int pixelminimapH = 0;
    int pixelminimapX = 0;
    int pixelminimapY = 0;

    bool pixel_minimap_custom_height = false;

#ifdef TILES
    pixel_minimap_custom_height = get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) > 0;
#endif // TILES

    if( use_narrow_sidebar() ) {
        // First, figure out how large each element will be.
        hpH = 7;
        hpW = 14;
        statH = 7;
        statW = sidebarWidth - MINIMAP_WIDTH - hpW;
        locH = 3;
        locW = sidebarWidth;
        stat2H = 2;
        stat2W = sidebarWidth;
        pixelminimapW = sidebarWidth;
        pixelminimapH = ( pixelminimapW / 2 );
        if( pixel_minimap_custom_height && pixelminimapH > get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) ) {
            pixelminimapH = get_option<int>( "PIXEL_MINIMAP_HEIGHT" );
        }
        messHshort = TERRAIN_WINDOW_TERM_HEIGHT - ( statH + locH + stat2H + pixelminimapH );
        messW = sidebarWidth;
        if( messHshort < 9 ) {
            pixelminimapH -= 9 - messHshort;
            messHshort = 9;
        }
        messHlong = TERRAIN_WINDOW_TERM_HEIGHT - ( statH + locH + stat2H );

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
        locH = 3;
        statX = 0;
        statH = 4;
        minimapX = 0;
        minimapY = 0;
        messX = MINIMAP_WIDTH;
        messY = 0;
        messW = sidebarWidth - messX;
        pixelminimapW = messW;
        pixelminimapH = ( pixelminimapW / 2 );
        if( pixel_minimap_custom_height && pixelminimapH > get_option<int>( "PIXEL_MINIMAP_HEIGHT" ) ) {
            pixelminimapH = get_option<int>( "PIXEL_MINIMAP_HEIGHT" );
        }
        messHshort = TERRAIN_WINDOW_TERM_HEIGHT - ( locH + statH +
                     pixelminimapH ); // 3 for w_location + 4 for w_stat, w_messages starts at 0
        if( messHshort < 9 ) {
            pixelminimapH -= 9 - messHshort;
            messHshort = 9;
        }
        messHlong = TERRAIN_WINDOW_TERM_HEIGHT - ( locH + statH );
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

    w_minimap = w_minimap_ptr = catacurses::newwin( MINIMAP_HEIGHT, MINIMAP_WIDTH, _y + minimapY,
                                _x + minimapX );
    werase( w_minimap );

    w_HP = w_HP_ptr = catacurses::newwin( hpH, hpW, _y + hpY, _x + hpX );
    werase( w_HP );

    w_messages_short = w_messages_short_ptr = catacurses::newwin( messHshort, messW, _y + messY,
                       _x + messX );
    werase( w_messages_short );

    w_messages_long = w_messages_long_ptr = catacurses::newwin( messHlong, messW, _y + messY,
                                            _x + messX );
    werase( w_messages_long );

    w_pixel_minimap = w_pixel_minimap_ptr = catacurses::newwin( pixelminimapH, pixelminimapW,
                                            _y + pixelminimapY, _x + pixelminimapX );
    werase( w_pixel_minimap );

    w_messages = w_messages_short;
    if( !pixel_minimap_option ) {
        w_messages = w_messages_long;
    }

    w_location_wider = w_location_wider_ptr = catacurses::newwin( locH, locW + 10, _y + locY,
                       _x + locX - 7 );
    werase( w_location_wider );

    w_location = w_location_ptr = catacurses::newwin( locH, locW, _y + locY, _x + locX );
    werase( w_location );

    w_status = w_status_ptr = catacurses::newwin( statH, statW, _y + statY, _x + statX );
    werase( w_status );

    w_status2 = w_status2_ptr = catacurses::newwin( stat2H, stat2W, _y + stat2Y, _x + stat2X );
    werase( w_status2 );

    liveview.init();

    // Only refresh if we are in-game, otherwise all resources are not initialized
    // and this refresh will crash the game.
    if( resized && u.getID() != -1 ) {
        refresh_all();
    }
}

void game::toggle_sidebar_style()
{
    narrow_sidebar = !narrow_sidebar;
#ifdef TILES
    tilecontext->reinit_minimap();
#endif // TILES
    init_ui();
    refresh_all();
}

void game::toggle_fullscreen()
{
#ifndef TILES
    fullscreen = !fullscreen;
    init_ui();
    refresh_all();
#else
    toggle_fullscreen_window();
    refresh_all();
#endif
}

void game::toggle_pixel_minimap()
{
#ifdef TILES
    if( pixel_minimap_option ) {
        clear_window_area( w_pixel_minimap );
    }
    pixel_minimap_option = !pixel_minimap_option;
    init_ui();
    refresh_all();
#endif // TILES
}

void game::reload_tileset()
{
#ifdef TILES
    try {
        tilecontext->reinit();
        tilecontext->load_tileset( get_option<std::string>( "TILES" ), false, true );
        tilecontext->do_tile_loading_report();
    } catch( const std::exception &err ) {
        popup( _( "Loading the tileset failed: %s" ), err.what() );
    }
    g->reset_zoom();
    g->refresh_all();
#endif // TILES
}

// temporarily switch out of fullscreen for functions that rely
// on displaying some part of the sidebar
void game::temp_exit_fullscreen()
{
    if( fullscreen ) {
        was_fullscreen = true;
        toggle_fullscreen();
    } else {
        was_fullscreen = false;
    }
}

void game::reenter_fullscreen()
{
    if( was_fullscreen ) {
        if( !fullscreen ) {
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
    loading_ui ui( true );
    load_core_data( ui );

    load_world_modfiles( ui );

    m =  map( get_option<bool>( "ZLEVELS" ) );

    next_npc_id = 1;
    next_mission_id = 1;
    new_game = true;
    uquit = QUIT_NO;   // We haven't quit the game
    bVMonsterLookFire = true;

    weather = WEATHER_CLEAR; // Start with some nice weather...
    // Weather shift in 30
    //@todo: shouldn't that use calendar::start instead of INITIAL_TIME?
    nextweather = calendar::time_of_cataclysm + time_duration::from_hours(
                      get_option<int>( "INITIAL_TIME" ) ) + 30_minutes;

    turnssincelastmon = 0; //Auto safe mode init

    sounds::reset_sounds();
    clear_zombies();
    coming_to_stairs.clear();
    active_npc.clear();
    faction_manager_ptr->clear();
    mission::clear_all();
    Messages::clear_messages();
    events = event_manager();

    SCT.vSCT.clear(); //Delete pending messages

    // reset kill counts
    kills.clear();
    npc_kills.clear();
    scent.reset();

    remoteveh_cache_time = calendar::before_time_starts;
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

void game::load_map( const tripoint &pos_sm )
{
    m.load( pos_sm.x, pos_sm.y, pos_sm.z, true );
}

// Set up all default values for a new game
bool game::start_game()
{
    if( !gamemode ) {
        gamemode.reset( new special_game() );
    }

    seed = rand();
    new_game = true;
    start_calendar();
    nextweather = calendar::turn;
    safe_mode = ( get_option<bool>( "SAFEMODE" ) ? SAFE_MODE_ON : SAFE_MODE_OFF );
    mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.
    get_safemode().load_global();

    init_autosave();

    catacurses::clear();
    catacurses::refresh();
    popup_nowait( _( "Please wait as we build your world" ) );
    load_master();
    u.setID( assign_npc_id() ); // should be as soon as possible, but *after* load_master

    const start_location &start_loc = u.start_location.obj();
    const tripoint omtstart = start_loc.find_player_initial_location();
    if( omtstart == overmap::invalid_tripoint ) {
        return false;
    }
    start_loc.prepare_map( omtstart );

    if( scen->has_map_special() ) {
        // Specials can add monster spawn points and similar and should be done before the main
        // map is loaded.
        start_loc.add_map_special( omtstart, scen->get_map_special() );
    }
    tripoint lev = omt_to_sm_copy( omtstart );
    // The player is centered in the map, but lev[xyz] refers to the top left point of the map
    lev.x -= HALF_MAPSIZE;
    lev.y -= HALF_MAPSIZE;
    load_map( lev );

    m.build_map_cache( get_levz() );
    // Do this after the map cache has been built!
    start_loc.place_player( u );
    // ...but then rebuild it, because we want visibility cache to avoid spawning monsters in sight
    m.build_map_cache( get_levz() );
    // Start the overmap with out immediate neighborhood visible, this needs to be after place_player
    overmap_buffer.reveal( point( u.global_omt_location().x, u.global_omt_location().y ),
                           get_option<int>( "DISTANCE_INITIAL_VISIBILITY" ), 0 );

    u.moves = 0;
    u.process_turn(); // process_turn adds the initial move points
    u.stamina = u.get_stamina_max();
    temperature = SPRING_TEMPERATURE;
    update_weather();
    u.next_climate_control_check = calendar::before_time_starts; // Force recheck at startup
    u.last_climate_control_ret = false;

    //Reset character safe mode/pickup rules
    get_auto_pickup().clear_character_rules();
    get_safemode().clear_character_rules();

    //Put some NPCs in there!
    if( get_option<std::string>( "STARTING_NPC" ) == "always" ||
        ( get_option<std::string>( "STARTING_NPC" ) == "scenario" &&
          !g->scen->has_flag( "LONE_START" ) ) ) {
        create_starting_npcs();
    }
    //Load NPCs. Set nearby npcs to active.
    load_npcs();
    // Spawn the monsters
    const bool spawn_near =
        get_option<bool>( "BLACK_ROAD" ) || g->scen->has_flag( "SUR_START" );
    // Surrounded start ones
    if( spawn_near ) {
        start_loc.surround_with_monsters( omtstart, mongroup_id( "GROUP_ZOMBIE" ), 70 );
    }

    m.spawn_monsters( !spawn_near ); // Static monsters

    // Make sure that no monsters are near the player
    // This can happen in lab starts
    if( !spawn_near ) {
        for( monster &critter : all_monsters() ) {
            if( rl_dist( critter.pos(), u.pos() ) <= 5 ||
                m.clear_path( critter.pos(), u.pos(), 40, 1, 100 ) ) {
                remove_zombie( critter );
            }
        }
    }

    //Create mutation_category_level
    u.set_highest_cat_level();
    //Calculate mutation drench protection stats
    u.drench_mut_calc();
    if( scen->has_flag( "FIRE_START" ) ) {
        start_loc.burn( omtstart, 3, 3 );
    }
    if( scen->has_flag( "INFECTED" ) ) {
        u.add_effect( effect_infected, 1_turns, random_body_part(), true );
    }
    if( scen->has_flag( "BAD_DAY" ) ) {
        u.add_effect( effect_flu, 1000_minutes );
        u.add_effect( effect_drunk, 270_minutes );
        u.add_morale( MORALE_FEELING_BAD, -100, -100, 50_minutes, 50_minutes );
    }
    if( scen->has_flag( "HELI_CRASH" ) ) {
        start_loc.handle_heli_crash( u );
        bool success = false;
        for( auto v : m.get_vehicles() ) {
            std::string name = v.v->type.str();
            std::string search = std::string( "helicopter" );
            if( name.find( search ) != std::string::npos ) {
                for( const vpart_reference &vp : v.v->get_any_parts( VPFLAG_CONTROLS ) ) {
                    const tripoint pos = vp.pos();
                    u.setpos( pos );

                    // Delete the items that would have spawned here from a "corpse"
                    for( auto sp : v.v->parts_at_relative( vp.mount(), true ) ) {
                        vehicle_stack here = v.v->get_items( sp );

                        for( auto iter = here.begin(); iter != here.end(); ) {
                            iter = here.erase( iter );
                        }
                    }

                    auto mons = critter_tracker->find( pos );
                    if( mons != nullptr ) {
                        critter_tracker->remove( *mons );
                    }

                    success = true;
                    break;
                }
                if( success ) {
                    v.v->name = "Bird Wreckage";
                    break;
                }
            }
        }
    }

    // Now that we're done handling coordinates, ensure the player's submap is in the center of the map
    update_map( u );

    //~ %s is player name
    u.add_memorial_log( pgettext( "memorial_male", "%s began their journey into the Cataclysm." ),
                        pgettext( "memorial_female", "%s began their journey into the Cataclysm." ),
                        u.name.c_str() );
    CallbackArgumentContainer lua_callback_args_info;
    lua_callback_args_info.emplace_back( u.getID() );
    lua_callback( "on_new_player_created", lua_callback_args_info );

    return true;
}

//Make any nearby overmap npcs active, and put them in the right location.
void game::load_npcs()
{
    const int radius = HALF_MAPSIZE - 1;
    // uses submap coordinates
    std::vector<std::shared_ptr<npc>> just_added;
    for( const auto &temp : overmap_buffer.get_npcs_near_player( radius ) ) {
        if( temp->is_active() ) {
            continue;
        }
        if( temp->has_companion_mission() ) {
            continue;
        }

        const tripoint sm_loc = temp->global_sm_location();
        // NPCs who are out of bounds before placement would be pushed into bounds
        // This can cause NPCs to teleport around, so we don't want that
        if( sm_loc.x < get_levx() || sm_loc.x >= get_levx() + MAPSIZE ||
            sm_loc.y < get_levy() || sm_loc.y >= get_levy() + MAPSIZE ||
            ( sm_loc.z != get_levz() && !m.has_zlevels() ) ) {
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
        if( temp->marked_for_death ) {
            temp->die( nullptr );
        } else {
            if( temp->my_fac != nullptr ) {
                temp->my_fac->known_by_u = true;
            }
            active_npc.push_back( temp );
            just_added.push_back( temp );
        }
    }

    for( const auto &npc : just_added ) {
        npc->on_load();
    }

    npcs_dirty = false;
}

void game::unload_npcs()
{
    for( const auto &npc : active_npc ) {
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

void game::create_starting_npcs()
{
    if( !get_option<bool>( "STATIC_NPC" ) ||
        get_option<std::string>( "STARTING_NPC" ) == "never" ) {
        return; //Do not generate a starting npc.
    }

    //We don't want more than one starting npc per starting location
    const int radius = 1;
    if( !overmap_buffer.get_npcs_near_player( radius ).empty() ) {
        return; //There is already an NPC in this starting location
    }

    std::shared_ptr<npc> tmp = std::make_shared<npc>();
    tmp->normalize();
    tmp->randomize( one_in( 2 ) ? NC_DOCTOR : NC_NONE );
    tmp->spawn_at_precise( { get_levx(), get_levy() }, u.pos() - point( 1, 1 ) );
    overmap_buffer.insert_npc( tmp );
    tmp->form_opinion( u );
    tmp->set_attitude( NPCATT_NULL );
    //This sets the NPC mission. This NPC remains in the starting location.
    tmp->mission = NPC_MISSION_SHELTER;
    tmp->chatbin.first_topic = "TALK_SHELTER";
    tmp->toggle_trait( trait_id( "NPC_STARTING_NPC" ) );
    //One random starting NPC mission
    tmp->add_new_mission( mission::reserve_random( ORIGIN_OPENER_NPC, tmp->global_omt_location(),
                          tmp->getID() ) );
}

bool game::cleanup_at_end()
{
    draw_sidebar();
    if( uquit == QUIT_DIED || uquit == QUIT_SUICIDE ) {
        // Put (non-hallucinations) into the overmap so they are not lost.
        for( monster &critter : all_monsters() ) {
            despawn_monster( critter );
        }
        // Save the factions', missions and set the NPC's overmap coordinates
        // Npcs are saved in the overmap.
        save_factions_missions_npcs(); //missions need to be saved as they are global for all saves.

        // save artifacts.
        save_artifacts();

        // and the overmap, and the local map.
        save_maps(); //Omap also contains the npcs who need to be saved.
    }

    if( uquit == QUIT_DIED || uquit == QUIT_SUICIDE ) {
        std::vector<std::string> vRip;

        int iMaxWidth = 0;
        int iNameLine = 0;
        int iInfoLine = 0;

        if( u.has_amount( "holybook_bible1", 1 ) || u.has_amount( "holybook_bible2", 1 ) ||
            u.has_amount( "holybook_bible3", 1 ) ) {
            if( !( u.has_trait( trait_id( "CANNIBAL" ) ) || u.has_trait( trait_id( "PSYCHOPATH" ) ) ) ) {
                vRip.emplace_back( "               _______  ___" );
                vRip.emplace_back( "              <       `/   |" );
                vRip.emplace_back( "               >  _     _ (" );
                vRip.emplace_back( "              |  |_) | |_) |" );
                vRip.emplace_back( "              |  | \\ | |   |" );
                vRip.emplace_back( "   ______.__%_|            |_________  __" );
                vRip.emplace_back( " _/                                  \\|  |" );
                iNameLine = vRip.size();
                vRip.emplace_back( "|                                        <" );
                vRip.emplace_back( "|                                        |" );
                iMaxWidth = vRip[vRip.size() - 1].length();
                vRip.emplace_back( "|                                        |" );
                vRip.emplace_back( "|_____.-._____              __/|_________|" );
                vRip.emplace_back( "              |            |" );
                iInfoLine = vRip.size();
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |           <" );
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |   _        |" );
                vRip.emplace_back( "              |__/         |" );
                vRip.emplace_back( "             % / `--.      |%" );
                vRip.emplace_back( "         * .%%|          -< @%%%" );
                vRip.emplace_back( "         `\\%`@|            |@@%@%%" );
                vRip.emplace_back( "       .%%%@@@|%     `   % @@@%%@%%%%" );
                vRip.emplace_back( "  _.%%%%%%@@@@@@%%%__/\\%@@%%@@@@@@@%%%%%%" );

            } else {
                vRip.emplace_back( "               _______  ___" );
                vRip.emplace_back( "              |       \\/   |" );
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |            |" );
                iInfoLine = vRip.size();
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |            |" );
                vRip.emplace_back( "              |           <" );
                vRip.emplace_back( "              |   _        |" );
                vRip.emplace_back( "              |__/         |" );
                vRip.emplace_back( "   ______.__%_|            |__________  _" );
                vRip.emplace_back( " _/                                   \\| \\" );
                iNameLine = vRip.size();
                vRip.emplace_back( "|                                         <" );
                vRip.emplace_back( "|                                         |" );
                iMaxWidth = vRip[vRip.size() - 1].length();
                vRip.emplace_back( "|                                         |" );
                vRip.emplace_back( "|_____.-._______            __/|__________|" );
                vRip.emplace_back( "             % / `_-.   _  |%" );
                vRip.emplace_back( "         * .%%|  |_) | |_)< @%%%" );
                vRip.emplace_back( "         `\\%`@|  | \\ | |   |@@%@%%" );
                vRip.emplace_back( "       .%%%@@@|%     `   % @@@%%@%%%%" );
                vRip.emplace_back( "  _.%%%%%%@@@@@@%%%__/\\%@@%%@@@@@@@%%%%%%" );
            }
        } else {
            vRip.emplace_back( "           _________  ____           " );
            vRip.emplace_back( "         _/         `/    \\_         " );
            vRip.emplace_back( "       _/      _     _      \\_.      " );
            vRip.emplace_back( "     _%\\      |_) | |_)       \\_     " );
            vRip.emplace_back( "   _/ \\/      | \\ | |           \\_   " );
            vRip.emplace_back( " _/                               \\_ " );
            vRip.emplace_back( "|                                   |" );
            iNameLine = vRip.size();
            vRip.emplace_back( " )                                 < " );
            vRip.emplace_back( "|                                   |" );
            vRip.emplace_back( "|                                   |" );
            vRip.emplace_back( "|   _                               |" );
            vRip.emplace_back( "|__/                                |" );
            iMaxWidth = vRip[vRip.size() - 1].length();
            vRip.emplace_back( " / `--.                             |" );
            vRip.emplace_back( "|                                  ( " );
            iInfoLine = vRip.size();
            vRip.emplace_back( "|                                   |" );
            vRip.emplace_back( "|                                   |" );
            vRip.emplace_back( "|     %                         .   |" );
            vRip.emplace_back( "|  @`                            %% |" );
            vRip.emplace_back( "| %@%@%\\                *      %`%@%|" );
            vRip.emplace_back( "%%@@@.%@%\\%%           `\\ %%.%%@@%@" );
            vRip.emplace_back( "@%@@%%%%%@@@@@@%%%%%%%%@@%%@@@%%%@%%@" );
        }

        const int iOffsetX = ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        const int iOffsetY = ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

        catacurses::window w_rip = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY,
                                   iOffsetX );
        draw_border( w_rip );

        sfx::do_player_death_hurt( g->u, true );
        sfx::fade_audio_group( 1, 2000 );
        sfx::fade_audio_group( 2, 2000 );
        sfx::fade_audio_group( 3, 2000 );
        sfx::fade_audio_group( 4, 2000 );

        for( unsigned int iY = 0; iY < vRip.size(); ++iY ) {
            for( unsigned int iX = 0; iX < vRip[iY].length(); ++iX ) {
                char cTemp = vRip[iY][iX];
                if( cTemp != ' ' ) {
                    nc_color ncColor = c_light_gray;

                    if( cTemp == '%' ) {
                        ncColor = c_green;

                    } else if( cTemp == '_' || cTemp == '|' ) {
                        ncColor = c_white;

                    } else if( cTemp == '@' ) {
                        ncColor = c_brown;

                    } else if( cTemp == '*' ) {
                        ncColor = c_red;
                    }

                    mvwputch( w_rip, iY + 1, iX + ( FULL_SCREEN_WIDTH / 2 ) - ( iMaxWidth / 2 ), ncColor,
                              vRip[iY][iX] );
                }
            }
        }

        std::string sTemp;
        std::stringstream ssTemp;

        center_print( w_rip, iInfoLine++, c_white, _( "Survived:" ) );

        int turns = calendar::turn - calendar::start;
        int minutes = ( turns / MINUTES( 1 ) ) % 60;
        int hours = ( turns / HOURS( 1 ) ) % 24;
        int days = turns / DAYS( 1 );

        if( days > 0 ) {
            sTemp = string_format( "%dd %dh %dm", days, hours, minutes );
        } else if( hours > 0 ) {
            sTemp = string_format( "%dh %dm", hours, minutes );
        } else {
            sTemp = string_format( "%dm", minutes );
        }

        center_print( w_rip, iInfoLine++, c_white, sTemp );

        int iTotalKills = 0;

        for( const auto &type : MonsterGenerator::generator().get_all_mtypes() ) {
            if( kill_count( type.id ) > 0 ) {
                iTotalKills += kill_count( type.id );
            }
        }

        ssTemp << iTotalKills;

        sTemp = _( "Kills:" );
        mvwprintz( w_rip, 1 + iInfoLine++, ( FULL_SCREEN_WIDTH / 2 ) - 5, c_light_gray,
                   ( sTemp + " " ).c_str() );
        wprintz( w_rip, c_magenta, ssTemp.str().c_str() );

        sTemp = _( "In memory of:" );
        mvwprintz( w_rip, iNameLine++, ( FULL_SCREEN_WIDTH / 2 ) - ( sTemp.length() / 2 ), c_light_gray,
                   sTemp.c_str() );

        sTemp = u.name;
        mvwprintz( w_rip, iNameLine++, ( FULL_SCREEN_WIDTH / 2 ) - ( sTemp.length() / 2 ), c_white,
                   sTemp.c_str() );

        sTemp = _( "Last Words:" );
        mvwprintz( w_rip, iNameLine++, ( FULL_SCREEN_WIDTH / 2 ) - ( sTemp.length() / 2 ), c_light_gray,
                   sTemp.c_str() );

        int iStartX = ( FULL_SCREEN_WIDTH / 2 ) - ( ( iMaxWidth - 4 ) / 2 );
        std::string sLastWords = string_input_popup()
                                 .window( w_rip, iStartX, iNameLine, iStartX + iMaxWidth - 4 - 1 )
                                 .max_length( iMaxWidth - 4 - 1 )
                                 .query_string();
        death_screen();
        if( uquit == QUIT_SUICIDE ) {
            u.add_memorial_log( pgettext( "memorial_male", "%s committed suicide." ),
                                pgettext( "memorial_female", "%s committed suicide." ),
                                u.name.c_str() );
        } else {
            u.add_memorial_log( pgettext( "memorial_male", "%s was killed." ),
                                pgettext( "memorial_female", "%s was killed." ),
                                u.name.c_str() );
        }
        if( !sLastWords.empty() ) {
            u.add_memorial_log( pgettext( "memorial_male", "Last words: %s" ),
                                pgettext( "memorial_female", "Last words: %s" ),
                                sLastWords.c_str() );
        }
        // Struck the save_player_data here to forestall Weirdness
        move_save_to_graveyard();
        write_memorial_file( sLastWords );
        u.memorial_log.clear();
        std::vector<std::string> characters = list_active_characters();
        // remove current player from the active characters list, as they are dead
        std::vector<std::string>::iterator curchar = std::find( characters.begin(),
                characters.end(), u.name );
        if( curchar != characters.end() ) {
            characters.erase( curchar );
        }

        if( characters.empty() ) {
            bool queryDelete = false;
            bool queryReset = false;

            if( get_option<std::string>( "WORLD_END" ) == "query" ) {
                uilist smenu;
                smenu.allow_cancel = false;
                smenu.addentry( 0, true, 'k', "%s", _( "Keep world" ) );
                smenu.addentry( 1, true, 'r', "%s", _( "Reset world" ) );
                smenu.addentry( 2, true, 'd', "%s", _( "Delete world" ) );
                smenu.query();

                switch( smenu.ret ) {
                    case 0:
                        break;
                    case 1:
                        queryReset = true;
                        break;
                    case 2:
                        queryDelete = true;
                        break;
                }
            }

            if( queryDelete || get_option<std::string>( "WORLD_END" ) == "delete" ) {
                world_generator->delete_world( world_generator->active_world->world_name, true );

            } else if( queryReset || get_option<std::string>( "WORLD_END" ) == "reset" ) {
                world_generator->delete_world( world_generator->active_world->world_name, false );
            }
        } else if( get_option<std::string>( "WORLD_END" ) != "keep" ) {
            std::stringstream message;
            std::string tmpmessage;
            for( auto &character : characters ) {
                tmpmessage += "\n  ";
                tmpmessage += character;
            }
            message << string_format( _( "World retained. Characters remaining:%s" ), tmpmessage.c_str() );
            popup( message.str(), PF_NONE );
        }
        if( gamemode ) {
            gamemode.reset( new special_game() ); // null gamemode or something..
        }
    }

    //clear all sound channels
    sfx::fade_audio_channel( -1, 300 );
    sfx::fade_audio_group( 1, 300 );
    sfx::fade_audio_group( 2, 300 );
    sfx::fade_audio_group( 3, 300 );
    sfx::fade_audio_group( 4, 300 );

    MAPBUFFER.reset();
    overmap_buffer.clear();

#ifdef __ANDROID__
    quick_shortcuts_map.clear();
#endif
    return true;
}

static int veh_lumi( vehicle &veh )
{
    float veh_luminance = 0.0;
    float iteration = 1.0;
    auto lights = veh.lights( true );

    for( const auto pt : lights ) {
        const auto &vp = pt->info();
        if( vp.has_flag( VPFLAG_CONE_LIGHT ) ||
            vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
            veh_luminance += vp.bonus / iteration;
            iteration = iteration * 1.1;
        }
    }
    // Calculation: see lightmap.cpp
    return LIGHT_RANGE( ( veh_luminance * 3 ) );
}

void game::calc_driving_offset( vehicle *veh )
{
    if( veh == nullptr || !get_option<bool>( "DRIVING_VIEW_OFFSET" ) ) {
        set_driving_view_offset( point_zero );
        return;
    }
    const int g_light_level = static_cast<int>( light_level( u.posz() ) );
    const int light_sight_range = u.sight_range( g_light_level );
    int sight = std::max( veh_lumi( *veh ), light_sight_range );

    // The maximal offset will leave at least this many tiles
    // between the PC and the edge of the main window.
    static const int border_range = 2;
    point max_offset( ( getmaxx( w_terrain ) + 1 ) / 2 - border_range - 1,
                      ( getmaxy( w_terrain ) + 1 ) / 2 - border_range - 1 );

    // velocity at or below this results in no offset at all
    static const float min_offset_vel = 1 * vehicles::vmiph_per_tile;
    // velocity at or above this results in maximal offset
    static const float max_offset_vel = std::min( max_offset.y, max_offset.x ) *
                                        vehicles::vmiph_per_tile;
    float velocity = veh->velocity;
    rl_vec2d offset = veh->move_vec();
    if( !veh->skidding && veh->player_in_control( u ) &&
        std::abs( veh->cruise_velocity - veh->velocity ) < 7 * vehicles::vmiph_per_tile ) {
        // Use the cruise controlled velocity, but only if
        // it is not too different from the actual velocity.
        // The actual velocity changes too often (see above slowdown).
        // Using it makes would make the offset change far too often.
        offset = veh->face_vec();
        velocity = veh->cruise_velocity;
    }
    float rel_offset;
    if( std::fabs( velocity ) < min_offset_vel ) {
        rel_offset = 0;
    } else if( std::fabs( velocity ) > max_offset_vel ) {
        rel_offset = ( velocity > 0 ) ? 1 : -1;
    } else {
        rel_offset = ( velocity - min_offset_vel ) / ( max_offset_vel - min_offset_vel );
    }
    // Squeeze into the corners, by making the offset vector longer,
    // the PC is still in view as long as both offset.x and
    // offset.y are <= 1
    if( std::fabs( offset.x ) > std::fabs( offset.y ) && std::fabs( offset.x ) > 0.2 ) {
        offset.y /= std::fabs( offset.x );
        offset.x = ( offset.x > 0 ) ? +1 : -1;
    } else if( std::fabs( offset.y ) > 0.2 ) {
        offset.x /= std::fabs( offset.y );
        offset.y = offset.y > 0 ? +1 : -1;
    }
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
    const point maxoff( ( sight * 2 + 1 - getmaxx( w_terrain ) ) / 2,
                        ( sight * 2 + 1 - getmaxy( w_terrain ) ) / 2 );
    if( maxoff.x <= 0 ) {
        offset.x = 0;
    } else if( offset.x > 0 && offset.x > maxoff.x ) {
        offset.x = maxoff.x;
    } else if( offset.x < 0 && -offset.x > maxoff.x ) {
        offset.x = -maxoff.x;
    }
    if( maxoff.y <= 0 ) {
        offset.y = 0;
    } else if( offset.y > 0 && offset.y > maxoff.y ) {
        offset.y = maxoff.y;
    } else if( offset.y < 0 && -offset.y > maxoff.y ) {
        offset.y = -maxoff.y;
    }

    // Turn the offset into a vector that increments the offset toward the desired position
    // instead of setting it there instantly, should smooth out jerkiness.
    const point offset_difference( offset.x - driving_view_offset.x,
                                   offset.y - driving_view_offset.y );

    const point offset_sign( ( offset_difference.x < 0 ) ? -1 : 1,
                             ( offset_difference.y < 0 ) ? -1 : 1 );
    // Shift the current offset in the direction of the calculated offset by one tile
    // per draw event, but snap to calculated offset if we're close enough to avoid jitter.
    offset.x = ( std::abs( offset_difference.x ) > 1 ) ?
               ( driving_view_offset.x + offset_sign.x ) : offset.x;
    offset.y = ( std::abs( offset_difference.y ) > 1 ) ?
               ( driving_view_offset.y + offset_sign.y ) : offset.y;

    set_driving_view_offset( point( offset.x, offset.y ) );
}

// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool game::do_turn()
{
    if( is_game_over() ) {
        return cleanup_at_end();
    }
    // Actual stuff
    if( new_game ) {
        new_game = false;
    } else {
        gamemode->per_turn();
        calendar::turn.increment();
    }

    // starting a new turn, clear out temperature cache
    temperature_cache.clear();

    if( npcs_dirty ) {
        load_npcs();
    }

    events.process();
    mission::process_all();

    if( calendar::once_every( 1_days ) ) {
        overmap_buffer.process_mongroups();
        if( calendar::turn.day_of_year() == 0 ) {
            lua_callback( "on_year_passed" );
        }
        lua_callback( "on_day_passed" );
    }

    if( calendar::once_every( 1_hours ) ) {
        lua_callback( "on_hour_passed" );
    }

    if( calendar::once_every( 1_minutes ) ) {
        lua_callback( "on_minute_passed" );
    }

    lua_callback( "on_turn_passed" );

    // Move hordes every 2.5 min
    if( calendar::once_every( time_duration::from_minutes( 2.5 ) ) ) {
        overmap_buffer.move_hordes();
        // Hordes that reached the reality bubble need to spawn,
        // make them spawn in invisible areas only.
        m.spawn_monsters( false );
    }

    u.update_body();

    // Auto-save if autosave is enabled
    if( get_option<bool>( "AUTOSAVE" ) &&
        calendar::once_every( 1_turns * get_option<int>( "AUTOSAVE_TURNS" ) ) &&
        !u.is_dead_state() ) {
        autosave();
    }

    update_weather();
    reset_light_level();

    perhaps_add_random_npc();

    // Process NPC sound events before they move or they hear themselves talking
    for( npc &guy : all_npcs() ) {
        if( rl_dist( guy.pos(), u.pos() ) < MAX_VIEW_DISTANCE ) {
            sounds::process_sound_markers( &guy );
        }
    }

    process_activity();

    // Process sound events into sound markers for display to the player.
    sounds::process_sound_markers( &u );

    if( u.is_deaf() ) {
        sfx::do_hearing_loss();
    }

    if( !u.has_effect( efftype_id( "sleep" ) ) ) {
        if( u.moves > 0 || uquit == QUIT_WATCH ) {
            while( u.moves > 0 || uquit == QUIT_WATCH ) {
                cleanup_dead();
                // Process any new sounds the player caused during their turn.
                sounds::process_sound_markers( &u );
                if( !u.activity && uquit != QUIT_WATCH ) {
                    draw();
                }

                if( handle_action() ) {
                    ++moves_since_last_save;
                    u.action_taken();
                }

                if( is_game_over() ) {
                    return cleanup_at_end();
                }

                if( uquit == QUIT_WATCH ) {
                    break;
                }
                if( u.activity ) {
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
        vehicle *veh = veh_pointer_or_null( m.veh_at( u.pos() ) );
        calc_driving_offset( veh );
    }

    // No-scent debug mutation has to be processed here or else it takes time to start working
    if( !u.has_active_bionic( bionic_id( "bio_scent_mask" ) ) &&
        !u.has_trait( trait_id( "DEBUG_NOSCENT" ) ) ) {
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
        point sm_topleft = sm_to_ms_copy( sm_loc.x, sm_loc.y );
        point in_reality = m.getlocal( sm_topleft );

        submap *sm = elem.second;

        const bool in_bubble_z = m.has_zlevels() || sm_loc.z == get_levz();
        for( auto &veh : sm->vehicles ) {
            veh->idle( in_bubble_z && m.inbounds( in_reality ) );
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
    if( u.moves < 0 && get_option<bool>( "FORCE_REDRAW" ) ) {
        draw();
        refresh_display();
    }
    u.process_active_items();

    if( get_levz() >= 0 && !u.is_underwater() ) {
        weather_data( weather ).effect();
    }

    const bool player_is_sleeping = u.has_effect( effect_sleep );

    if( player_is_sleeping ) {
        if( calendar::once_every( 30_minutes ) || !player_was_sleeping ) {
            draw();
            //Putting this in here to save on checking
            if( calendar::once_every( 1_hours ) ) {
                add_artifact_dreams( );
            }
        }

        if( calendar::once_every( 1_minutes ) ) {
            query_popup()
            .wait_message( "%s", _( "Wait till you wake up..." ) )
            .on_top( true )
            .show();

            catacurses::refresh();
            refresh_display();
        }
    }

    player_was_sleeping = player_is_sleeping;

    u.update_bodytemp();
    u.update_body_wetness( *weather_precise );
    u.apply_wetness_morale( temperature );
    u.do_skill_rust();

    if( calendar::once_every( 1_minutes ) ) {
        u.update_morale();
    }

    if( calendar::once_every( 9_turns ) ) {
        u.check_and_recover_morale();
    }

    if( !u.is_deaf() ) {
        sfx::remove_hearing_loss();
    }
    sfx::do_danger_music();
    sfx::do_fatigue();

    return false;
}

void game::set_driving_view_offset( const point &p )
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

void game::process_activity()
{
    if( !u.activity ) {
        return;
    }

    if( calendar::once_every( 5_minutes ) ) {
        draw();
        refresh_display();
    }

    while( u.moves > 0 && u.activity ) {
        u.activity.do_turn( u );
    }
}

void game::catch_a_monster( std::vector<monster *> &catchables, const tripoint &pos, player *p,
                            const time_duration &catch_duration ) // catching function
{
    monster *const fish = random_entry_removed( catchables );
    //spawn the corpse, rotten by a part of the duration
    m.add_item_or_charges( pos, item::make_corpse( fish->type->id, calendar::turn + rng( 0_turns,
                           catch_duration ) ) );
    u.add_msg_if_player( m_good, _( "You caught a %s." ), fish->type->nname().c_str() );
    //quietly kill the caught
    fish->no_corpse_quiet = true;
    fish->die( p );
}

void game::cancel_activity()
{
    u.cancel_activity();
}

bool cancel_auto_move( player &p, const std::string &text )
{
    if( p.has_destination() ) {
        add_msg( m_warning, _( "%s. Auto-move canceled" ), text.c_str() );
        p.clear_destination();
        return true;
    }
    return false;
}

bool game::cancel_activity_or_ignore_query( const distraction_type type, const std::string &text )
{
    if( cancel_auto_move( u, text ) || !u.activity || u.activity.is_distraction_ignored( type ) ) {
        return false;
    }

    bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );

    const auto allow_key = [force_uc]( const input_event & evt ) {
        return !force_uc || evt.type != CATA_INPUT_KEYBOARD ||
               // std::lower is undefined outside unsigned char range
               evt.get_first_input() < 'a' || evt.get_first_input() > 'z';
    };

    const auto &action = query_popup()
                         .context( "CANCEL_ACTIVITY_OR_IGNORE_QUERY" )
                         .message( force_uc ?
                                   pgettext( "cancel_activity_or_ignore_query",
                                           "<color_light_red>%s %s (Case Sensitive)</color>" ) :
                                   pgettext( "cancel_activity_or_ignore_query",
                                           "<color_light_red>%s %s</color>" ),
                                   text, u.activity.get_stop_phrase() )
                         .option( "YES", allow_key )
                         .option( "NO", allow_key )
                         .option( "IGNORE", allow_key )
                         .query()
                         .action;

    if( action == "YES" ) {
        u.cancel_activity();
        return true;
    }
    if( action == "IGNORE" ) {
        u.activity.ignore_distraction( type );
        for( auto &activity : u.backlog ) {
            activity.ignore_distraction( type );
        }
    }
    return false;
}

bool game::cancel_activity_query( const std::string &text )
{
    if( cancel_auto_move( u, text ) || !u.activity ) {
        return false;
    }

    if( query_yn( "%s %s", text.c_str(), u.activity.get_stop_phrase().c_str() ) ) {
        u.cancel_activity();
        u.resume_backlog_activity();
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

void game::set_critter_died()
{
    critter_died = true;
}

void game::update_weather()
{
    w_point &w = *weather_precise;
    winddirection = wind_direction_override ? *wind_direction_override : w.winddirection;
    windspeed = windspeed_override ? *windspeed_override : w.windpower;
    if( weather == WEATHER_NULL || calendar::turn >= nextweather ) {
        const weather_generator &weather_gen = get_cur_weather_gen();
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
        // Check weather every few turns, instead of every turn.
        //@todo: predict when the weather changes and use that time.
        nextweather = calendar::turn + 50_turns;
        if( weather != old_weather ) {
            CallbackArgumentContainer lua_callback_args_info;
            lua_callback_args_info.emplace_back( weather_data( weather ).name );
            lua_callback_args_info.emplace_back( weather_data( old_weather ).name );
            lua_callback( "on_weather_changed", lua_callback_args_info );
        }
        if( weather != old_weather && weather_data( weather ).dangerous &&
            get_levz() >= 0 && m.is_outside( u.pos() )
            && !u.has_activity( activity_id( "ACT_WAIT_WEATHER" ) ) ) {
            cancel_activity_or_ignore_query( distraction_type::weather_change,
                                             string_format( _( "The weather changed to %s!" ), weather_data( weather ).name.c_str() ) );
        }

        if( weather != old_weather && u.has_activity( activity_id( "ACT_WAIT_WEATHER" ) ) ) {
            u.assign_activity( activity_id( "ACT_WAIT_WEATHER" ), 0, 0 );
        }

        if( weather_data( weather ).sight_penalty !=
            weather_data( old_weather ).sight_penalty ) {
            for( int i = -OVERMAP_DEPTH; i <= OVERMAP_HEIGHT; i++ ) {
                m.set_transparency_cache_dirty( i );
            }
        }
    }
}

int get_heat_radiation( const tripoint &location, bool direct )
{
    // Direct heat from fire sources
    // Cache fires to avoid scanning the map around us bp times
    // Stored as intensity-distance pairs
    int temp_mod = 0;
    std::vector<std::pair<int, int>> fires;
    fires.reserve( 13 * 13 );
    int best_fire = 0;
    for( const tripoint &dest : g->m.points_in_radius( location, 6 ) ) {
        int heat_intensity = 0;

        int ffire = g->m.get_field_strength( dest, fd_fire );
        if( ffire > 0 ) {
            heat_intensity = ffire;
        } else if( g->m.tr_at( dest ).loadid == tr_lava ) {
            heat_intensity = 3;
        }
        if( heat_intensity == 0 || !g->m.sees( location, dest, -1 ) ) {
            // No heat source here
            continue;
        }
        // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
        const int fire_dist = std::max( 1, square_dist( dest, location ) );
        fires.emplace_back( std::make_pair( heat_intensity, fire_dist ) );
        if( fire_dist <= 1 ) {
            // Extend limbs/lean over a single adjacent fire to warm up
            best_fire = std::max( best_fire, heat_intensity );
        }
    }

    for( const auto &intensity_dist : fires ) {
        const int intensity = intensity_dist.first;
        const int distance = intensity_dist.second;
        temp_mod += 6 * intensity * intensity / distance;
    }
    if( direct ) {
        return best_fire;
    }
    return temp_mod;
}

int get_convection_temperature( const tripoint &location )
{
    // Heat from hot air (fields)
    int temp_mod = 0;
    const trap &trap_at_pos = g->m.tr_at( location );
    // directly on fire/lava tiles
    int tile_strength = g->m.get_field_strength( location, fd_fire );
    if( tile_strength > 0 || trap_at_pos.loadid == tr_lava ) {
        temp_mod += 300;
    }
    // hot air of a fire/lava
    auto tile_strength_mod = []( const tripoint & loc, field_id fld, int case_1, int case_2,
    int case_3 ) {
        int strength = g->m.get_field_strength( loc, fld );
        int cases[3] = { case_1, case_2, case_3 };
        return ( strength > 0 && strength < 4 ) ? cases[ strength - 1 ] : 0;
    };

    temp_mod += tile_strength_mod( location, fd_hot_air1,  2,   6,  10 );
    temp_mod += tile_strength_mod( location, fd_hot_air2,  6,  16,  20 );
    temp_mod += tile_strength_mod( location, fd_hot_air3, 16,  40,  70 );
    temp_mod += tile_strength_mod( location, fd_hot_air4, 70, 100, 160 );

    return temp_mod;
}

int game::get_temperature( const tripoint &location )
{
    const auto &cached = temperature_cache.find( location );
    if( cached != temperature_cache.end() ) {
        return cached->second;
    }

    int temp_mod = 0; // local modifier

    if( !new_game ) {
        temp_mod += get_heat_radiation( location, false );
        temp_mod += get_convection_temperature( location );
    }
    //underground temperature = average New England temperature = 43F/6C rounded to int
    const int temp = ( location.z < 0 ? AVERAGE_ANNUAL_TEMPERATURE : temperature ) +
                     ( new_game ? 0 : ( m.temperature( location ) + temp_mod ) );

    temperature_cache.emplace( std::make_pair( location, temp ) );
    return temp;
}

int game::assign_mission_id()
{
    int ret = next_mission_id;
    next_mission_id++;
    return ret;
}

npc *game::find_npc( int id )
{
    return overmap_buffer.find_npc( id ).get();
}

int game::kill_count( const mtype_id &mon )
{
    if( kills.find( mon ) != kills.end() ) {
        return kills[mon];
    }
    return 0;
}

int game::kill_count( const species_id &spec )
{
    int result = 0;
    for( const auto &pair : kills ) {
        if( pair.first->in_species( spec ) ) {
            result += pair.second;
        }
    }
    return result;
}

void game::increase_kill_count( const mtype_id &id )
{
    kills[id]++;
}

void game::record_npc_kill( const npc &p )
{
    npc_kills.push_back( p.get_name() );
}

std::list<std::string> game::get_npc_kill()
{
    return npc_kills;
}

void game::handle_key_blocking_activity()
{
    // If player is performing a task and a monster is dangerously close, warn them
    // regardless of previous safemode warnings
    if( u.activity && !u.has_activity( activity_id( "ACT_AIM" ) ) &&
        u.activity.moves_left > 0 &&
        !u.activity.is_distraction_ignored( distraction_type::hostile_spotted ) ) {
        Creature *hostile_critter = is_hostile_very_close();
        if( hostile_critter != nullptr ) {
            if( cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                 string_format( _( "You see %s approaching!" ),
                                                         hostile_critter->disp_name().c_str() ) ) ) {
                return;
            }
        }
    }

    if( u.activity && u.activity.moves_left > 0 ) {
        input_context ctxt = get_default_mode_input_context();
        const std::string action = ctxt.handle_input( 0 );
        if( action == "pause" ) {
            cancel_activity_query( _( "Confirm:" ) );
        } else if( action == "player_data" ) {
            u.disp_info();
            refresh_all();
        } else if( action == "messages" ) {
            Messages::display_messages();
            refresh_all();
        } else if( action == "help" ) {
            get_help().display_help();
            refresh_all();
        }
    }
}

/* item submenu for 'i' and '/'
* It use draw_item_info to draw item info and action menu
*
* @param pos position of item in inventory
* @param iStartX Left coordinate of the item info window
* @param iWidth width of the item info window (height = height of terminal)
* @return getch
*/
int game::inventory_item_menu( int pos, int iStartX, int iWidth,
                               const inventory_item_menu_positon position )
{
    int cMenu = static_cast<int>( '+' );

    item &oThisItem = u.i_at( pos );
    if( u.has_item( oThisItem ) ) {
#ifdef __ANDROID__
        if( get_option<bool>( "ANDROID_INVENTORY_AUTOADD" ) ) {
            add_key_to_quick_shortcuts( oThisItem.invlet, "INVENTORY", false );
        }
#endif

        std::vector<iteminfo> vThisItem;
        std::vector<iteminfo> vDummy;

        const bool bHPR = get_auto_pickup().has_rule( &oThisItem );
        const hint_rating rate_drop_item = u.weapon.has_flag( "NO_UNWIELD" ) ? HINT_CANT : HINT_GOOD;

        int max_text_length = 0;
        uilist action_menu;
        action_menu.allow_anykey = true;
        const auto addentry = [&]( const char key, const std::string & text, const hint_rating hint ) {
            // The char is used as retval from the uilist *and* as hotkey.
            action_menu.addentry( key, true, key, text );
            auto &entry = action_menu.entries.back();
            switch( hint ) {
                case HINT_CANT:
                    entry.text_color = c_light_gray;
                    break;
                case HINT_IFFY:
                    entry.text_color = c_light_red;
                    break;
                case HINT_GOOD:
                    entry.text_color = c_light_green;
                    break;
            }
            max_text_length = std::max( max_text_length, utf8_width( text ) );
        };
        addentry( 'a', pgettext( "action", "activate" ), u.rate_action_use( oThisItem ) );
        addentry( 'R', pgettext( "action", "read" ), u.rate_action_read( oThisItem ) );
        addentry( 'E', pgettext( "action", "eat" ), u.rate_action_eat( oThisItem ) );
        addentry( 'W', pgettext( "action", "wear" ), u.rate_action_wear( oThisItem ) );
        addentry( 'w', pgettext( "action", "wield" ), HINT_GOOD );
        addentry( 't', pgettext( "action", "throw" ), HINT_GOOD );
        addentry( 'c', pgettext( "action", "change side" ), u.rate_action_change_side( oThisItem ) );
        addentry( 'T', pgettext( "action", "take off" ), u.rate_action_takeoff( oThisItem ) );
        addentry( 'd', pgettext( "action", "drop" ), rate_drop_item );
        addentry( 'U', pgettext( "action", "unload" ), u.rate_action_unload( oThisItem ) );
        addentry( 'r', pgettext( "action", "reload" ), u.rate_action_reload( oThisItem ) );
        addentry( 'p', pgettext( "action", "part reload" ), u.rate_action_reload( oThisItem ) );
        addentry( 'm', pgettext( "action", "mend" ), u.rate_action_mend( oThisItem ) );
        addentry( 'D', pgettext( "action", "disassemble" ), u.rate_action_disassemble( oThisItem ) );
        addentry( '=', pgettext( "action", "reassign" ), HINT_GOOD );
        if( bHPR ) {
            addentry( '-', _( "Autopickup" ), HINT_IFFY );
        } else {
            addentry( '+', _( "Autopickup" ), HINT_GOOD );
        }

        int iScrollPos = 0;
        oThisItem.info( true, vThisItem );

        // +2+2 for border and adjacent spaces, +2 for '<hotkey><space>'
        int popup_width = max_text_length + 2 + 2 + 2;
        int popup_x = 0;
        switch( position ) {
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

        // TODO: Ideally the setup of uilist would be split into calculate variables (size, width...),
        // and actual window creation. This would allow us to let uilist calculate the width, we can
        // use that to adjust its location afterwards.
        action_menu.w_y = VIEW_OFFSET_Y;
        action_menu.w_x = popup_x + VIEW_OFFSET_X;
        action_menu.w_width = popup_width;
        // Filtering isn't needed, the number of entries is manageable.
        action_menu.filtering = false;
        // Default menu border color is different, this matches the border of the item info window.
        action_menu.border_color = BORDER_COLOR;

        do {
            draw_item_info( iStartX, iWidth, VIEW_OFFSET_X, TERMY - VIEW_OFFSET_Y * 2, oThisItem.tname(),
                            oThisItem.type_name(), vThisItem, vDummy,
                            iScrollPos, true, false, false );
            const int prev_selected = action_menu.selected;
            action_menu.query( false );
            if( action_menu.ret >= 0 ) {
                cMenu = action_menu.ret; /* Remember: hotkey == retval, see addentry above. */
            } else if( action_menu.ret == UILIST_UNBOUND && action_menu.keypress == KEY_RIGHT ) {
                // Simulate KEY_RIGHT == '\n' (confirm currently selected entry) for compatibility with old version.
                // TODO: ideally this should be done in the uilist, maybe via a callback.
                cMenu = action_menu.ret = action_menu.entries[action_menu.selected].retval;
            } else if( action_menu.keypress == KEY_PPAGE || action_menu.keypress == KEY_NPAGE ) {
                cMenu = action_menu.keypress;
                // Prevent the menu from scrolling with this key. TODO: Ideally the menu
                // could be instructed to ignore these two keys instead of scrolling.
                action_menu.selected = prev_selected;
                action_menu.fselected = prev_selected;
                action_menu.vshift = 0;
            } else {
                cMenu = 0;
            }

            switch( cMenu ) {
                case 'a':
                    use_item( pos );
                    break;
                case 'E':
                    eat( pos );
                    break;
                case 'W':
                    u.wear( u.i_at( pos ) );
                    break;
                case 'w':
                    wield( pos );
                    break;
                case 't':
                    plthrow( pos );
                    break;
                case 'c':
                    change_side( pos );
                    break;
                case 'T':
                    u.takeoff( u.i_at( pos ) );
                    break;
                case 'd':
                    u.drop( pos, u.pos() );
                    break;
                case 'U':
                    unload( pos );
                    break;
                case 'r':
                    reload( pos );
                    break;
                case 'p':
                    reload( pos, true );
                    break;
                case 'm':
                    mend( pos );
                    break;
                case 'R':
                    u.read( pos );
                    break;
                case 'D':
                    u.disassemble( pos );
                    break;
                case '=':
                    game_menus::inv::reassign_letter( u, u.i_at( pos ) );
                    break;
                case KEY_PPAGE:
                    iScrollPos--;
                    break;
                case KEY_NPAGE:
                    iScrollPos++;
                    break;
                case '+':
                    if( !bHPR ) {
                        get_auto_pickup().add_rule( &oThisItem );
                        add_msg( m_info, _( "'%s' added to character pickup rules." ), oThisItem.tname( 1,
                                 false ).c_str() );
                    }
                    break;
                case '-':
                    if( bHPR ) {
                        get_auto_pickup().remove_rule( &oThisItem );
                        add_msg( m_info, _( "'%s' removed from character pickup rules." ), oThisItem.tname( 1,
                                 false ).c_str() );
                    }
                    break;
                default:
                    break;
            }
        } while( action_menu.ret == UILIST_WAIT_INPUT || action_menu.ret == UILIST_UNBOUND );
    }
    return cMenu;
}

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview( input_context &ctxt, std::string &action )
{
    cata::optional<tripoint> liveview_pos;
    do {
        action = ctxt.handle_input();
        if( action == "MOUSE_MOVE" ) {
            const cata::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain );
            if( mouse_pos && ( !liveview_pos || *mouse_pos != *liveview_pos ) ) {
                liveview_pos = mouse_pos;
                liveview.show( *liveview_pos );
                draw_sidebar_messages();
            } else if( !mouse_pos ) {
                liveview_pos.reset();
                liveview.hide();
                draw_sidebar_messages();
            }
        }
    } while( action == "MOUSE_MOVE" ); // Freeze animation when moving the mouse

    if( action != "TIMEOUT" ) {
        // Keyboard event, break out of animation loop
        liveview.hide();
        return false;
    }

    // Mouse movement or un-handled key
    return true;
}

#ifdef TILES
void rescale_tileset( int size );
#endif

input_context get_default_mode_input_context()
{
    input_context ctxt( "DEFAULTMODE" );
    // Because those keys move the character, they don't pan, as their original name says
    ctxt.set_iso( true );
    ctxt.register_action( "UP", _( "Move North" ) );
    ctxt.register_action( "RIGHTUP", _( "Move Northeast" ) );
    ctxt.register_action( "RIGHT", _( "Move East" ) );
    ctxt.register_action( "RIGHTDOWN", _( "Move Southeast" ) );
    ctxt.register_action( "DOWN", _( "Move South" ) );
    ctxt.register_action( "LEFTDOWN", _( "Move Southwest" ) );
    ctxt.register_action( "LEFT", _( "Move West" ) );
    ctxt.register_action( "LEFTUP", _( "Move Northwest" ) );
    ctxt.register_action( "pause" );
    ctxt.register_action( "LEVEL_DOWN", _( "Descend Stairs" ) );
    ctxt.register_action( "LEVEL_UP", _( "Ascend Stairs" ) );
    ctxt.register_action( "toggle_map_memory" );
    ctxt.register_action( "center" );
    ctxt.register_action( "shift_n" );
    ctxt.register_action( "shift_ne" );
    ctxt.register_action( "shift_e" );
    ctxt.register_action( "shift_se" );
    ctxt.register_action( "shift_s" );
    ctxt.register_action( "shift_sw" );
    ctxt.register_action( "shift_w" );
    ctxt.register_action( "shift_nw" );
    ctxt.register_action( "toggle_move" );
    ctxt.register_action( "open" );
    ctxt.register_action( "close" );
    ctxt.register_action( "smash" );
    ctxt.register_action( "loot" );
    ctxt.register_action( "examine" );
    ctxt.register_action( "advinv" );
    ctxt.register_action( "pickup" );
    ctxt.register_action( "grab" );
    ctxt.register_action( "haul" );
    ctxt.register_action( "butcher" );
    ctxt.register_action( "chat" );
    ctxt.register_action( "look" );
    ctxt.register_action( "peek" );
    ctxt.register_action( "listitems" );
    ctxt.register_action( "zones" );
    ctxt.register_action( "inventory" );
    ctxt.register_action( "compare" );
    ctxt.register_action( "organize" );
    ctxt.register_action( "apply" );
    ctxt.register_action( "apply_wielded" );
    ctxt.register_action( "wear" );
    ctxt.register_action( "take_off" );
    ctxt.register_action( "eat" );
    ctxt.register_action( "read" );
    ctxt.register_action( "wield" );
    ctxt.register_action( "pick_style" );
    ctxt.register_action( "reload" );
    ctxt.register_action( "unload" );
    ctxt.register_action( "throw" );
    ctxt.register_action( "fire" );
    ctxt.register_action( "fire_burst" );
    ctxt.register_action( "select_fire_mode" );
    ctxt.register_action( "drop" );
    ctxt.register_action( "drop_adj" );
    ctxt.register_action( "bionics" );
    ctxt.register_action( "mutations" );
    ctxt.register_action( "sort_armor" );
    ctxt.register_action( "wait" );
    ctxt.register_action( "craft" );
    ctxt.register_action( "recraft" );
    ctxt.register_action( "long_craft" );
    ctxt.register_action( "construct" );
    ctxt.register_action( "disassemble" );
    ctxt.register_action( "sleep" );
    ctxt.register_action( "control_vehicle" );
    ctxt.register_action( "safemode" );
    ctxt.register_action( "autosafe" );
    ctxt.register_action( "autoattack" );
    ctxt.register_action( "ignore_enemy" );
    ctxt.register_action( "whitelist_enemy" );
    ctxt.register_action( "save" );
    ctxt.register_action( "quicksave" );
#ifndef RELEASE
    ctxt.register_action( "quickload" );
#endif
    ctxt.register_action( "quit" );
    ctxt.register_action( "player_data" );
    ctxt.register_action( "map" );
    ctxt.register_action( "sky" );
    ctxt.register_action( "missions" );
    ctxt.register_action( "factions" );
    ctxt.register_action( "kills" );
    ctxt.register_action( "morale" );
    ctxt.register_action( "messages" );
    ctxt.register_action( "help" );
    ctxt.register_action( "open_keybindings" );
    ctxt.register_action( "open_options" );
    ctxt.register_action( "open_autopickup" );
    ctxt.register_action( "open_safemode" );
    ctxt.register_action( "open_color" );
    ctxt.register_action( "open_world_mods" );
    ctxt.register_action( "debug" );
    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "debug_mode" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "toggle_sidebar_style" );
#ifndef __ANDROID__
    ctxt.register_action( "toggle_fullscreen" );
#endif
    ctxt.register_action( "toggle_pixel_minimap" );
    ctxt.register_action( "reload_tileset" );
    ctxt.register_action( "toggle_auto_features" );
    ctxt.register_action( "toggle_auto_pulp_butcher" );
    ctxt.register_action( "toggle_auto_mining" );
    ctxt.register_action( "toggle_auto_foraging" );
    ctxt.register_action( "action_menu" );
    ctxt.register_action( "main_menu" );
    ctxt.register_action( "item_action_menu" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "SEC_SELECT" );
    return ctxt;
}

vehicle *game::remoteveh()
{
    if( calendar::turn == remoteveh_cache_time ) {
        return remoteveh_cache;
    }
    remoteveh_cache_time = calendar::turn;
    std::stringstream remote_veh_string( u.get_value( "remote_controlling_vehicle" ) );
    if( remote_veh_string.str().empty() ||
        ( !u.has_active_bionic( bio_remote ) && !u.has_active_item( "remotevehcontrol" ) ) ) {
        remoteveh_cache = nullptr;
    } else {
        tripoint vp;
        remote_veh_string >> vp.x >> vp.y >> vp.z;
        vehicle *veh = veh_pointer_or_null( m.veh_at( vp ) );
        if( veh && veh->fuel_left( "battery", true ) > 0 ) {
            remoteveh_cache = veh;
        } else {
            remoteveh_cache = nullptr;
        }
    }
    return remoteveh_cache;
}

void game::setremoteveh( vehicle *veh )
{
    remoteveh_cache_time = calendar::turn;
    remoteveh_cache = veh;
    if( veh != nullptr && !u.has_active_bionic( bio_remote ) &&
        !u.has_active_item( "remotevehcontrol" ) ) {
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

bool game::try_get_left_click_action( action_id &act, const tripoint &mouse_target )
{
    bool new_destination = true;
    if( !destination_preview.empty() ) {
        auto &final_destination = destination_preview.back();
        if( final_destination.x == mouse_target.x && final_destination.y == mouse_target.y ) {
            // Second click
            new_destination = false;
            u.set_destination( destination_preview );
            destination_preview.clear();
            act = u.get_next_auto_move_direction();
            if( act == ACTION_NULL ) {
                // Something went wrong
                u.clear_destination();
                return false;
            }
        }
    }

    if( new_destination ) {
        destination_preview = m.route( u.pos(), mouse_target, u.get_pathfinding_settings(),
                                       u.get_path_avoid() );
        return false;
    }

    return true;
}

bool game::try_get_right_click_action( action_id &act, const tripoint &mouse_target )
{
    const bool cleared_destination = !destination_preview.empty();
    u.clear_destination();
    destination_preview.clear();

    if( cleared_destination ) {
        // Produce no-op if auto-move had just been cleared on this action
        // e.g. from a previous single left mouse click. This has the effect
        // of right-click cancelling an auto-move before it is initiated.
        return false;
    }

    const bool is_adjacent = square_dist( mouse_target.x, mouse_target.y, u.posx(), u.posy() ) <= 1;
    const bool is_self = square_dist( mouse_target.x, mouse_target.y, u.posx(), u.posy() ) <= 0;
    if( const monster *const mon = critter_at<monster>( mouse_target ) ) {
        if( !u.sees( *mon ) ) {
            add_msg( _( "Nothing relevant here." ) );
            return false;
        }

        if( !u.weapon.is_gun() ) {
            add_msg( m_info, _( "You are not wielding a ranged weapon." ) );
            return false;
        }

        //TODO: Add weapon range check. This requires weapon to be reloaded.

        act = ACTION_FIRE;
    } else if( is_adjacent &&
               m.close_door( tripoint( mouse_target.x, mouse_target.y, u.posz() ), !m.is_outside( u.pos() ),
                             true ) ) {
        act = ACTION_CLOSE;
    } else if( is_self ) {
        act = ACTION_PICKUP;
    } else if( is_adjacent ) {
        act = ACTION_EXAMINE;
    } else {
        add_msg( _( "Nothing relevant here." ) );
        return false;
    }

    return true;
}

bool game::is_game_over()
{
    if( uquit == QUIT_WATCH ) {
        // deny player movement and dodging
        u.moves = 0;
        // prevent pain from updating
        u.set_pain( 0 );
        // prevent dodging
        u.dodges_left = 0;
        return false;
    }
    if( uquit == QUIT_DIED ) {
        if( u.in_vehicle ) {
            m.unboard_vehicle( u.pos() );
        }
        u.place_corpse();
        return true;
    }
    if( uquit == QUIT_SUICIDE ) {
        if( u.in_vehicle ) {
            m.unboard_vehicle( u.pos() );
        }
        return true;
    }
    if( uquit != QUIT_NO ) {
        return true;
    }
    // is_dead_state() already checks hp_torso && hp_head, no need to for loop it
    if( u.is_dead_state() ) {
        Messages::deactivate();
        if( get_option<std::string>( "DEATHCAM" ) == "always" ) {
            uquit = QUIT_WATCH;
        } else if( get_option<std::string>( "DEATHCAM" ) == "ask" ) {
            uquit = query_yn( _( "Watch the last moments of your life...?" ) ) ?
                    QUIT_WATCH : QUIT_DIED;
        } else if( get_option<std::string>( "DEATHCAM" ) == "never" ) {
            uquit = QUIT_DIED;
        } else {
            // Something funky happened here, just die.
            dbg( D_ERROR ) << "no deathcam option given to options, defaulting to QUIT_DIED";
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
    const std::string &save_dir      = get_world_base_save_path();
    const std::string &graveyard_dir = FILENAMES["graveyarddir"];
    const std::string &prefix        = base64_encode( u.name ) + ".";

    if( !assure_dir_exist( graveyard_dir ) ) {
        debugmsg( "could not create graveyard path '%s'", graveyard_dir.c_str() );
    }

    const auto save_files = get_files_from_path( prefix, save_dir );
    if( save_files.empty() ) {
        debugmsg( "could not find save files in '%s'", save_dir.c_str() );
    }

    for( const auto &src_path : save_files ) {
        const std::string dst_path = graveyard_dir +
                                     src_path.substr( src_path.rfind( '/' ), std::string::npos );

        if( rename_file( src_path, dst_path ) ) {
            continue;
        }

        debugmsg( "could not rename file '%s' to '%s'", src_path.c_str(), dst_path.c_str() );

        if( remove_file( src_path ) ) {
            continue;
        }

        debugmsg( "could not remove file '%s'", src_path.c_str() );
    }
}

void game::load_master()
{
    using namespace std::placeholders;
    const auto datafile = get_world_base_save_path() + "/master.gsav";
    read_from_file_optional( datafile, std::bind( &game::unserialize_master, this, _1 ) );
    faction_manager_ptr->create_if_needed();
}

bool game::load( const std::string &world )
{
    world_generator->init();
    const WORLDPTR wptr = world_generator->get_world( world );
    if( !wptr ) {
        return false;
    }
    if( wptr->world_saves.empty() ) {
        debugmsg( "world '%s' contains no saves", world.c_str() );
        return false;
    }

    try {
        world_generator->set_active_world( wptr );
        g->setup();
        g->load( wptr->world_saves.front() );
    } catch( const std::exception &err ) {
        debugmsg( "cannot load world '%s': %s", world.c_str(), err.what() );
        return false;
    }

    return true;
}

void game::load( const save_t &name )
{
    using namespace std::placeholders;

    const std::string worldpath = get_world_base_save_path() + "/";
    const std::string playerpath = worldpath + name.base_path();

    // Now load up the master game data; factions (and more?)
    load_master();
    u = player();
    u.name = name.player_name();
    // This should be initialized more globally (in player/Character constructor)
    u.weapon = item( "null", 0 );
    if( !read_from_file( playerpath + ".sav", std::bind( &game::unserialize, this, _1 ) ) ) {
        return;
    }

    read_from_file_optional_json( playerpath + ".mm", [&]( JsonIn & jsin ) {
        u.deserialize_map_memory( jsin );
    } );

    read_from_file_optional( worldpath + name.base_path() + ".weather", std::bind( &game::load_weather,
                             this, _1 ) );
    nextweather = calendar::turn;

    read_from_file_optional( worldpath + name.base_path() + ".log",
                             std::bind( &player::load_memorial_file, &u, _1 ) );

#ifdef __ANDROID__
    read_from_file_optional( worldpath + name.base_path() + ".shortcuts",
                             std::bind( &game::load_shortcuts, this, _1 ) );
#endif

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
    read_from_file_optional( get_world_base_save_path() + "/uistate.json", []( std::istream & stream ) {
        JsonIn jsin( stream );
        uistate.deserialize( jsin );
    } );

    reload_npcs();
    update_map( u );

    // legacy, needs to be here as we access the map.
    if( u.getID() == 0 || u.getID() == -1 ) {
        // player does not have a real id, so assign a new one,
        u.setID( assign_npc_id() );
        // The vehicle stores the IDs of the boarded players, so update it, too.
        if( u.in_vehicle ) {
            if( const cata::optional<vpart_reference> vp = m.veh_at(
                        u.pos() ).part_with_feature( "BOARDABLE", true ) ) {
                vp->part().passenger_id = u.getID();
            }
        }
    }

    u.reset();
    draw();

    lua_callback( "on_savegame_loaded" );
}

void game::load_world_modfiles( loading_ui &ui )
{
    catacurses::erase();
    catacurses::refresh();

    auto &mods = world_generator->active_world->active_mod_order;

    // remove any duplicates whilst preserving order (fixes #19385)
    std::set<mod_id> found;
    mods.erase( std::remove_if( mods.begin(), mods.end(), [&found]( const mod_id & e ) {
        if( found.count( e ) ) {
            return true;
        } else {
            found.insert( e );
            return false;
        }
    } ), mods.end() );

    // require at least one core mod (saves before version 6 may implicitly require dda pack)
    if( std::none_of( mods.begin(), mods.end(), []( const mod_id & e ) {
    return e->core;
} ) ) {
        mods.insert( mods.begin(), mod_id( "dda" ) );
    }

    load_artifacts( get_world_base_save_path() + "/artifacts.gsav" );
    // this code does not care about mod dependencies,
    // it assumes that those dependencies are static and
    // are resolved during the creation of the world.
    // That means world->active_mod_order contains a list
    // of mods in the correct order.
    load_packs( _( "Loading files" ), mods, ui );

    // Load additional mods from that world-specific folder
    load_data_from_dir( get_world_base_save_path() + "/mods", "custom", ui );

    catacurses::erase();
    catacurses::refresh();

    DynamicDataLoader::get_instance().finalize_loaded_data( ui );
}

bool game::load_packs( const std::string &msg, const std::vector<mod_id> &packs, loading_ui &ui )
{
    ui.new_context( msg );
    std::vector<mod_id> missing;
    std::vector<mod_id> available;

    for( const mod_id &e : packs ) {
        if( e.is_valid() ) {
            available.emplace_back( e );
            ui.add_entry( e->name() );
        } else {
            missing.push_back( e );
        }
    }

    ui.show();
    for( const auto &e : available ) {
        const MOD_INFORMATION &mod = *e;
        load_data_from_dir( mod.path, mod.ident.str(), ui );

        // if mod specifies legacy migrations load any that are required
        if( !mod.legacy.empty() ) {
            for( int i = get_option<int>( "CORE_VERSION" ); i < core_version; ++i ) {
                popup_status( msg.c_str(), _( "Applying legacy migration (%s %i/%i)" ),
                              e.c_str(), i, core_version - 1 );
                load_data_from_dir( string_format( "%s/%i", mod.legacy.c_str(), i ), mod.ident.str(), ui );
            }
        }

        ui.proceed();
    }

    for( const auto &e : missing ) {
        debugmsg( "unknown content %s", e.c_str() );
    }

    return missing.empty();
}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    std::string masterfile = get_world_base_save_path() + "/master.gsav";
    return write_to_file_exclusive( masterfile, [&]( std::ostream & fout ) {
        serialize_master( fout );
    }, _( "factions data" ) );
}

bool game::save_artifacts()
{
    std::string artfilename = get_world_base_save_path() + "/artifacts.gsav";
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

bool game::save_player_data()
{
    const std::string playerfile = get_player_base_save_path();

    const bool saved_data = write_to_file( playerfile + ".sav", [&]( std::ostream & fout ) {
        serialize( fout );
    }, _( "player data" ) );
    const bool saved_map_memory = write_to_file( playerfile + ".mm", [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
        u.serialize_map_memory( jsout );
    }, _( "player map memory" ) );
    const bool saved_weather = write_to_file( playerfile + ".weather", [&]( std::ostream & fout ) {
        save_weather( fout );
    }, _( "weather state" ) );
    const bool saved_log = write_to_file( playerfile + ".log", [&]( std::ostream & fout ) {
        fout << u.dump_memorial();
    }, _( "player memorial" ) );
#ifdef __ANDROID__
    const bool saved_shortcuts = write_to_file( playerfile + ".shortcuts", [&]( std::ostream & fout ) {
        save_shortcuts( fout );
    }, _( "quick shortcuts" ) );
#endif

    return saved_data && saved_map_memory && saved_weather && saved_log
#ifdef __ANDROID__
           && saved_shortcuts
#endif
           ;
}

bool game::save()
{
    try {
        if( !save_player_data() ||
            !save_factions_missions_npcs() ||
            !save_artifacts() ||
            !save_maps() ||
            !get_auto_pickup().save_character() ||
            !get_safemode().save_character() ||
        !write_to_file_exclusive( get_world_base_save_path() + "/uistate.json", [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
            uistate.serialize( jsout );
        }, _( "uistate data" ) ) ) {
            return false;
        } else {
            world_generator->active_world->add_save( save_t::from_player_name( u.name ) );
            return true;
        }
    } catch( std::ios::failure &err ) {
        popup( _( "Failed to save game data" ) );
        return false;
    }
}

std::vector<std::string> game::list_active_characters()
{
    std::vector<std::string> saves;
    for( auto &worldsave : world_generator->active_world->world_saves ) {
        saves.push_back( worldsave.player_name() );
    }
    return saves;
}

/**
 * Writes information about the character out to a text file timestamped with
 * the time of the file was made. This serves as a record of the character's
 * state at the time the memorial was made (usually upon death) and
 * accomplishments in a human-readable format.
 */
void game::write_memorial_file( std::string sLastWords )
{
    const std::string &memorial_dir = FILENAMES["memorialdir"];
    const std::string &memorial_active_world_dir = memorial_dir + utf8_to_native(
                world_generator->active_world->world_name ) + "/";

    //Check if both dirs exist. Nested assure_dir_exist fails if the first dir of the nested dir does not exist.
    if( !assure_dir_exist( memorial_dir ) ) {
        dbg( D_ERROR ) << "game:write_memorial_file: Unable to make memorial directory.";
        debugmsg( "Could not make '%s' directory", memorial_dir.c_str() );
        return;
    }

    if( !assure_dir_exist( memorial_active_world_dir ) ) {
        dbg( D_ERROR ) <<
                       "game:write_memorial_file: Unable to make active world directory in memorial directory.";
        debugmsg( "Could not make '%s' directory", memorial_active_world_dir.c_str() );
        return;
    }

    // <name>-YYYY-MM-DD-HH-MM-SS.txt
    //       123456789012345678901234 ~> 24 chars + a null
    constexpr size_t suffix_len   = 24 + 1;
    constexpr size_t max_name_len = FILENAME_MAX - suffix_len;

    const size_t name_len = u.name.size();
    // Here -1 leaves space for the ~
    const size_t truncated_name_len = ( name_len >= max_name_len ) ? ( max_name_len - 1 ) : name_len;

    std::ostringstream memorial_file_path;
    memorial_file_path << memorial_active_world_dir;

    if( get_options().has_option( "ENCODING_CONV" ) && !get_option<bool>( "ENCODING_CONV" ) ) {
        // Use the default locale to replace non-printable characters with _ in the player name.
        std::locale locale {"C"};
        std::replace_copy_if( std::begin( u.name ), std::begin( u.name ) + truncated_name_len,
                              std::ostream_iterator<char>( memorial_file_path ),
        [&]( const char c ) {
            return !std::isgraph( c, locale );
        }, '_' );
    } else {
        memorial_file_path << utf8_to_native( u.name );
    }

    // Add a ~ if the player name was actually truncated.
    memorial_file_path << ( ( truncated_name_len != name_len ) ? "~-" : "-" );

    // Add a timestamp for uniqueness.
    char buffer[suffix_len] {};
    std::time_t t = std::time( nullptr );
    std::strftime( buffer, suffix_len, "%Y-%m-%d-%H-%M-%S", std::localtime( &t ) );
    memorial_file_path << buffer;

    memorial_file_path << ".txt";

    const std::string path_string = memorial_file_path.str();
    write_to_file( memorial_file_path.str(), [&]( std::ostream & fout ) {
        u.memorial( fout, sLastWords );
    }, _( "player memorial" ) );
}

void game::debug()
{
    int action = uilist( _( "Debug Functions - Using these is CHEATING!" ), {
        _( "Wish for an item" ),                // 0
        _( "Teleport - Short Range" ),          // 1
        _( "Teleport - Long Range" ),           // 2
        _( "Reveal map" ),                      // 3
        _( "Spawn NPC" ),                       // 4
        _( "Spawn Monster" ),                   // 5
        _( "Check game state..." ),             // 6
        _( "Kill NPCs" ),                       // 7
        _( "Mutate" ),                          // 8
        _( "Spawn a vehicle" ),                 // 9
        _( "Change all skills" ),               // 10
        _( "Learn all melee styles" ),          // 11
        _( "Unlock all recipes" ),              // 12
        _( "Edit player/NPC" ),                 // 13
        _( "Spawn Artifact" ),                  // 14
        _( "Spawn Clairvoyance Artifact" ),     // 15
        _( "Map editor" ),                      // 16
        _( "Change weather" ),                  // 17
        _( "Change wind direction" ),           // 18
        _( "Change wind speed" ),               // 19
        _( "Kill all monsters" ),               // 20
        _( "Display hordes" ),                  // 21
        _( "Test Item Group" ),                 // 22
        _( "Damage Self" ),                     // 23
        _( "Show Sound Clustering" ),           // 24
        _( "Lua Console" ),                     // 25
        _( "Display weather" ),                 // 26
        _( "Display overmap scents" ),          // 27
        _( "Change time" ),                     // 28
        _( "Set automove route" ),              // 29
        _( "Show mutation category levels" ),   // 30
        _( "Overmap editor" ),                  // 31
        _( "Draw benchmark (X seconds)" ),      // 32
        _( "Teleport - Adjacent overmap" ),     // 33
        _( "Test trait group" ),                // 34
        _( "Show debug message" ),              // 35
        _( "Crash game (test crash handling)" ),// 36
        _( "Spawn Map Extra" ),                 // 37
        _( "Quit to Main Menu" ),               // 38
    } );
    refresh_all();
    switch( action ) {
        case 0:
            debug_menu::wishitem( &u );
            break;

        case 1:
            debug_menu::teleport_short();
            break;

        case 2:
            debug_menu::teleport_long();
            break;

        case 3: {
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

        case 4: {
            std::shared_ptr<npc> temp = std::make_shared<npc>();
            temp->normalize();
            temp->randomize();
            temp->spawn_at_precise( { get_levx(), get_levy() }, u.pos() + point( -4, -4 ) );
            overmap_buffer.insert_npc( temp );
            temp->form_opinion( u );
            temp->mission = NPC_MISSION_NULL;
            temp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, temp->global_omt_location(),
                                   temp->getID() ) );
            load_npcs();
        }
        break;

        case 5:
            debug_menu::wishmonster( cata::nullopt );
            break;

        case 6: {
            std::string s = _( "Location %d:%d in %d:%d, %s\n" );
            s += _( "Current turn: %d.\n%s\n" );
            s += ngettext( "%d creature exists.\n", "%d creatures exist.\n", num_creatures() );
            popup_top(
                s.c_str(),
                u.posx(), u.posy(), get_levx(), get_levy(),
                overmap_buffer.ter( u.global_omt_location() )->get_name().c_str(),
                int( calendar::turn ),
                ( get_option<bool>( "RANDOM_NPC" ) ? _( "NPCs are going to spawn." ) :
                  _( "NPCs are NOT going to spawn." ) ),
                num_creatures() );
            for( const npc &guy : all_npcs() ) {
                tripoint t = guy.global_sm_location();
                add_msg( m_info, _( "%s: map ( %d:%d ) pos ( %d:%d )" ), guy.name.c_str(), t.x,
                         t.y, guy.posx(), guy.posy() );
            }

            add_msg( m_info, _( "(you: %d:%d)" ), u.posx(), u.posy() );

            disp_NPCs();
            break;
        }
        case 7:
            for( npc &guy : all_npcs() ) {
                add_msg( _( "%s's head implodes!" ), guy.name.c_str() );
                guy.hp_cur[bp_head] = 0;
            }
            break;

        case 8:
            debug_menu::wishmutate( &u );
            break;

        case 9:
            if( m.veh_at( u.pos() ) ) {
                dbg( D_ERROR ) << "game:load: There's already vehicle here";
                debugmsg( "There's already vehicle here" );
            } else {
                std::vector<vproto_id> veh_strings;
                uilist veh_menu;
                veh_menu.text = _( "Choose vehicle to spawn" );
                int menu_ind = 0;
                for( auto &elem : vehicle_prototype::get_all() ) {
                    if( elem != vproto_id( "custom" ) ) {
                        const vehicle_prototype &proto = elem.obj();
                        veh_strings.push_back( elem );
                        //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
                        veh_menu.addentry( menu_ind, true, MENU_AUTOASSIGN, _( "%1$s (%2$s)" ), _( proto.name.c_str() ),
                                           elem.c_str() );
                        ++menu_ind;
                    }
                }
                veh_menu.query();
                if( veh_menu.ret >= 0 && veh_menu.ret < static_cast<int>( veh_strings.size() ) ) {
                    //Didn't cancel
                    const vproto_id &selected_opt = veh_strings[veh_menu.ret];
                    tripoint dest = u.pos(); // TODO: Allow picking this when add_vehicle has 3d argument
                    vehicle *veh = m.add_vehicle( selected_opt, dest.x, dest.y, -90, 100, 0 );
                    if( veh != nullptr ) {
                        m.board_vehicle( u.pos(), &u );
                    }
                }
            }
            break;

        case 10: {
            debug_menu::wishskill( &u );
        }
        break;

        case 11:
            add_msg( m_info, _( "Martial arts debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( auto &style : all_martialart_types() ) {
                if( style != matype_id( "style_none" ) ) {
                    u.add_martialart( style );
                }
            }
            add_msg( m_good, _( "You now know a lot more than just 10 styles of kung fu." ) );
            break;

        case 12: {
            add_msg( m_info, _( "Recipe debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( const auto &e : recipe_dict ) {
                u.learn_recipe( &e.second );
            }
            add_msg( m_good, _( "You know how to craft that now." ) );
        }
        break;

        case 13:
            debug_menu::character_edit_menu();
            break;

        case 14:
            if( const cata::optional<tripoint> center = look_around() ) {
                artifact_natural_property prop = artifact_natural_property( rng( ARTPROP_NULL + 1,
                                                 ARTPROP_MAX - 1 ) );
                m.create_anomaly( *center, prop );
                m.spawn_natural_artifact( *center, prop );
            }
            break;

        case 15:
            u.i_add( item( architects_cube(), calendar::turn ) );
            break;

        case 16: {
            look_debug();
        }
        break;

        case 17: {
            uilist weather_menu;
            weather_menu.text = _( "Select new weather pattern:" );
            weather_menu.addentry( 0, true, MENU_AUTOASSIGN, weather_override == WEATHER_NULL ?
                                   _( "Keep normal weather patterns" ) : _( "Disable weather forcing" ) );
            for( int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++ ) {
                weather_menu.addentry( weather_id, true, MENU_AUTOASSIGN,
                                       weather_data( static_cast<weather_type>( weather_id ) ).name );
            }

            weather_menu.query();

            if( weather_menu.ret >= 0 && weather_menu.ret < NUM_WEATHER_TYPES ) {
                weather_type selected_weather = static_cast<weather_type>( weather_menu.ret );
                weather_override = selected_weather;
                nextweather = calendar::turn;
                update_weather();
            }
        }
        break;

        case 18: {
            uilist wind_direction_menu;
            wind_direction_menu.text = _( "Select new wind direction:" );
            wind_direction_menu.addentry( 0, true, MENU_AUTOASSIGN, wind_direction_override ?
                                          _( "Disable direction forcing" ) : _( "Keep normal wind direction" ) );
            int count = 1;
            for( int angle = 0; angle <= 315; angle += 45 ) {
                wind_direction_menu.addentry( count, true, MENU_AUTOASSIGN, get_wind_arrow( angle ) );
                count += 1;
            }
            wind_direction_menu.query();
            if( wind_direction_menu.ret == 0 ) {
                wind_direction_override = cata::nullopt;
            } else if( wind_direction_menu.ret >= 0 && wind_direction_menu.ret < 9 ) {
                wind_direction_override = ( wind_direction_menu.ret - 1 ) * 45;
                nextweather = calendar::turn;
                update_weather();
            }
        }
        break;

        case 19: {
            uilist wind_speed_menu;
            wind_speed_menu.text = _( "Select new wind speed:" );
            wind_speed_menu.addentry( 0, true, MENU_AUTOASSIGN, wind_direction_override ?
                                      _( "Disable speed forcing" ) : _( "Keep normal wind speed" ) );
            int count = 1;
            for( int speed = 0; speed <= 100; speed += 10 ) {
                std::string speedstring = std::to_string( speed ) + " " + velocity_units( VU_WIND );
                wind_speed_menu.addentry( count, true, MENU_AUTOASSIGN, speedstring );
                count += 1;
            }
            wind_speed_menu.query();
            if( wind_speed_menu.ret == 0 ) {
                windspeed_override = cata::nullopt;
            } else if( wind_speed_menu.ret >= 0 && wind_speed_menu.ret < 12 ) {
                int selected_wind_speed = ( wind_speed_menu.ret - 1 ) * 10;
                windspeed_override = selected_wind_speed;
                nextweather = calendar::turn;
                update_weather();
            }
        }
        break;

        case 20: {
            for( monster &critter : all_monsters() ) {
                // Use the normal death functions, useful for testing death
                // and for getting a corpse.
                critter.die( nullptr );
            }
            cleanup_dead();
        }
        break;
        case 21:
            ui::omap::display_hordes();
            break;
        case 22: {
            item_group::debug_spawn();
        }
        break;

        // Damage Self
        case 23: {
            int dbg_damage;
            if( query_int( dbg_damage, _( "Damage self for how much? hp: %d" ), u.hp_cur[hp_torso] ) ) {
                u.hp_cur[hp_torso] -= dbg_damage;
                u.die( nullptr );
                draw_sidebar();
            }
        }
        break;

        case 24: {
#ifdef TILES
            const point offset {
                POSX - u.posx() + u.view_offset.x,
                POSY - u.posy() + u.view_offset.y
            };
            draw_ter();
            auto sounds_to_draw = sounds::get_monster_sounds();
            for( const auto &sound : sounds_to_draw.first ) {
                mvwputch( w_terrain, offset.y + sound.y, offset.x + sound.x, c_yellow, '?' );
            }
            for( const auto &sound : sounds_to_draw.second ) {
                mvwputch( w_terrain, offset.y + sound.y, offset.x + sound.x, c_red, '?' );
            }
            wrefresh( w_terrain );
            inp_mngr.wait_for_any_key();
#else
            popup( _( "This binary was not compiled with tiles support." ) );
#endif
        }
        break;

        case 25: {
            lua_console console;
            console.run();
        }
        break;
        case 26:
            ui::omap::display_weather();
            break;
        case 27:
            ui::omap::display_scents();
            break;
        case 28: {
            auto set_turn = [&]( const int initial, const int factor, const char *const msg ) {
                const auto text = string_input_popup()
                                  .title( msg )
                                  .width( 20 )
                                  .text( to_string( initial ) )
                                  .only_digits( true )
                                  .query_string();
                if( text.empty() ) {
                    return;
                }
                const int new_value = ( std::atoi( text.c_str() ) - initial ) * factor;
                calendar::turn += std::max( std::min( INT_MAX / 2 - calendar::turn, new_value ),
                                            -calendar::turn );
            };

            uilist smenu;
            do {
                const int iSel = smenu.ret;
                smenu.reset();
                smenu.addentry( 0, true, 'y', "%s: %d", _( "year" ), calendar::turn.years() );
                smenu.addentry( 1, !calendar::eternal_season(), 's', "%s: %d",
                                _( "season" ), int( season_of_year( calendar::turn ) ) );
                smenu.addentry( 2, true, 'd', "%s: %d", _( "day" ), day_of_season<int>( calendar::turn ) );
                smenu.addentry( 3, true, 'h', "%s: %d", _( "hour" ), hour_of_day<int>( calendar::turn ) );
                smenu.addentry( 4, true, 'm', "%s: %d", _( "minute" ), minute_of_hour<int>( calendar::turn ) );
                smenu.addentry( 5, true, 't', "%s: %d", _( "turn" ), static_cast<int>( calendar::turn ) );
                smenu.selected = iSel;
                smenu.query();

                switch( smenu.ret ) {
                    case 0:
                        set_turn( calendar::turn.years(), to_turns<int>( calendar::year_length() ), _( "Set year to?" ) );
                        break;
                    case 1:
                        set_turn( int( season_of_year( calendar::turn ) ),
                                  to_turns<int>( calendar::turn.season_length() ),
                                  _( "Set season to? (0 = spring)" ) );
                        break;
                    case 2:
                        set_turn( day_of_season<int>( calendar::turn ), DAYS( 1 ), _( "Set days to?" ) );
                        break;
                    case 3:
                        set_turn( hour_of_day<int>( calendar::turn ), HOURS( 1 ), _( "Set hour to?" ) );
                        break;
                    case 4:
                        set_turn( minute_of_hour<int>( calendar::turn ), MINUTES( 1 ), _( "Set minute to?" ) );
                        break;
                    case 5:
                        set_turn( calendar::turn, 1,
                                  string_format( _( "Set turn to? (One day is %i turns)" ), int( DAYS( 1 ) ) ).c_str() );
                        break;
                    default:
                        break;
                }
            } while( smenu.ret != UILIST_CANCEL );
        }
        break;
        case 29: {
            const cata::optional<tripoint> dest = look_around();
            if( !dest || *dest == u.pos() ) {
                break;
            }

            auto rt = m.route( u.pos(), *dest, u.get_pathfinding_settings(), u.get_path_avoid() );
            if( !rt.empty() ) {
                u.set_destination( rt );
            } else {
                popup( "Couldn't find path" );
            }
        }
        break;
        case 30:
            for( const auto &elem : u.mutation_category_level ) {
                add_msg( "%s: %d", elem.first.c_str(), elem.second );
            }
            break;

        case 31:
            ui::omap::display_editor();
            break;

        case 32: {
            const int ms = string_input_popup()
                           .title( _( "Enter benchmark length (in milliseconds):" ) )
                           .width( 20 )
                           .text( "5000" )
                           .query_int();
            debug_menu::draw_benchmark( ms );
        }
        break;

        case 33:
            debug_menu::teleport_overmap();
            break;
        case 34:
            trait_group::debug_spawn();
            break;
        case 35:
            debugmsg( "Test debugmsg" );
            break;
        case 36:
            std::raise( SIGSEGV );
            break;
        case 37: {
            oter_id terrain_type = overmap_buffer.ter( g->u.global_omt_location() );

            map_extras ex = region_settings_map["default"].region_extras[terrain_type->get_extras()];
            uilist mx_menu;
            std::vector<std::string> mx_str;
            for( auto &extra : ex.values ) {
                mx_menu.addentry( -1, true, -1, extra.obj );
                mx_str.push_back( extra.obj );
            }
            mx_menu.query( false );
            int mx_choice = mx_menu.ret;
            if( mx_choice >= 0 && mx_choice < static_cast<int>( mx_str.size() ) ) {
                auto func = MapExtras::get_function( mx_str[ mx_choice ] );
                if( func != nullptr ) {
                    const tripoint where( ui::omap::choose_point() );
                    if( where != overmap::invalid_tripoint ) {
                        tinymap mx_map;
                        mx_map.load( where.x * 2, where.y * 2, where.z, false );
                        func( mx_map, where );
                    }
                }
            }
            break;
        }
        case 38:
            if( query_yn(
                    _( "Quit without saving? This may cause issues such as duplicated or missing items and vehicles!" ) ) ) {
                u.moves = 0;
                uquit = QUIT_NOSAVED;
            }
            break;
    }
    catacurses::erase();
    refresh_all();
}

void game::disp_kills()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ),
                           std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ) );

    std::vector<std::string> data;
    int totalkills = 0;
    const int colum_width = ( getmaxx( w ) - 2 ) / 3; // minus border

    std::map<std::tuple<std::string, std::string, std::string>, int> kill_counts;

    // map <name, sym, color> to kill count
    for( auto &elem : kills ) {
        const mtype &m = elem.first.obj();
        kill_counts[std::tuple<std::string, std::string, std::string>(
                                                m.nname(),
                                                m.sym,
                                                string_from_color( m.color )
                                            )] += elem.second;
        totalkills += elem.second;
    }

    for( const auto &entry : kill_counts ) {
        std::ostringstream buffer;
        buffer << "<color_" << std::get<2>( entry.first ) << ">";
        buffer << std::get<1>( entry.first ) << "</color>" << " ";
        buffer << "<color_light_gray>" << std::get<0>( entry.first ) << "</color>";
        const int w = colum_width - utf8_width( std::get<0>( entry.first ) );
        buffer.width( w - 3 ); // gap between cols, monster sym, space
        buffer.fill( ' ' );
        buffer << entry.second;
        buffer.width( 0 );
        data.push_back( buffer.str() );
    }
    for( const auto &npc_name : npc_kills ) {
        totalkills += 1;
        std::ostringstream buffer;
        buffer << "<color_magenta>@ " << npc_name << "</color>";
        const int w = colum_width - utf8_width( npc_name );
        buffer.width( w - 3 ); // gap between cols, monster sym, space
        buffer.fill( ' ' );
        buffer << "1";
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

    refresh_all();
}

void game::disp_NPC_epilogues()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ),
                           std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ) );
    epilogue epi;
    // @todo: This search needs to be expanded to all NPCs
    for( const npc &guy : all_npcs() ) {
        if( guy.is_friend() ) {
            epi.random_by_group( guy.male ? "male" : "female" );
            std::vector<std::string> txt;
            txt.emplace_back( epi.text );
            draw_border( w, BORDER_COLOR, guy.name, c_black_white );
            multipage( w, txt, "", 2 );
        }
    }

    refresh_all();
}

void game::disp_faction_ends()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ),
                           std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ) );
    std::vector<std::string> data;

    for( const faction &elem : faction_manager_ptr->all() ) {
        if( elem.known_by_u ) {
            if( elem.name == "Your Followers" ) {
                data.emplace_back( _( "       You are forgotten among the billions lost in the cataclysm..." ) );
                display_table( w, "", 1, data );
            } else if( elem.name == "The Old Guard" && elem.power != 100 ) {
                if( elem.power < 150 ) {
                    data.emplace_back(
                        _( "    Locked in an endless battle, the Old Guard was forced to consolidate their \
resources in a handful of fortified bases along the coast.  Without the men \
or material to rebuild, the soldiers that remained lost all hope..." ) );
                } else {
                    data.emplace_back( _( "    The steadfastness of individual survivors after the cataclysm impressed \
the tattered remains of the once glorious union.  Spurred on by small \
successes, a number of operations to re-secure facilities met with limited \
success.  Forced to eventually consolidate to large bases, the Old Guard left \
these facilities in the hands of the few survivors that remained.  As the \
years past, little materialized from the hopes of rebuilding civilization..." ) );
                }
                display_table( w, _( "The Old Guard" ), 1, data );
            } else if( elem.name == "The Free Merchants" && elem.power != 100 ) {
                if( elem.power < 150 ) {
                    data.emplace_back( _( "    Life in the refugee shelter deteriorated as food shortages and disease \
destroyed any hope of maintaining a civilized enclave.  The merchants and \
craftsmen dispersed to found new colonies but most became victims of \
marauding bandits.  Those who survived never found a place to call home..." ) );
                } else {
                    data.emplace_back( _( "    The Free Merchants struggled for years to keep themselves fed but their \
once profitable trade routes were plundered by bandits and thugs.  In squalor \
and filth the first generations born after the cataclysm are told stories of \
the old days when food was abundant and the children were allowed to play in \
the sun..." ) );
                }
                display_table( w, _( "The Free Merchants" ), 1, data );
            } else if( elem.name == "The Tacoma Commune" && elem.power != 100 ) {
                if( elem.power < 150 ) {
                    data.emplace_back( _( "    The fledgling outpost was abandoned a few months later.  The external \
threats combined with low crop yields caused the Free Merchants to withdraw \
their support.  When the exhausted migrants returned to the refugee center \
they were turned away to face the world on their own." ) );
                } else {
                    data.emplace_back(
                        _( "    The commune continued to grow rapidly through the years despite constant \
external threat.  While maintaining a reputation as a haven for all law-\
abiding citizens, the commune's leadership remained loyal to the interests of \
the Free Merchants.  Hard labor for little reward remained the price to be \
paid for those who sought the safety of the community." ) );
                }
                display_table( w, _( "The Tacoma Commune" ), 1, data );
            } else if( elem.name == "The Wasteland Scavengers" && elem.power != 100 ) {
                if( elem.power < 150 ) {
                    data.emplace_back(
                        _( "    The lone bands of survivors who wandered the now alien world dwindled in \
number through the years.  Unable to compete with the growing number of \
monstrosities that had adapted to live in their world, those who did survive \
lived in dejected poverty and hopelessness..." ) );
                } else {
                    data.emplace_back(
                        _( "    The scavengers who flourished in the opening days of the cataclysm found \
an ever increasing challenge in finding and maintaining equipment from the \
old world.  Enormous hordes made cities impossible to enter while new \
eldritch horrors appeared mysteriously near old research labs.  But on the \
fringes of where civilization once ended, bands of hunter-gatherers began to \
adopt agrarian lifestyles in fortified enclaves..." ) );
                }
                display_table( w, _( "The Wasteland Scavengers" ), 1, data );
            } else if( elem.name == "Hell's Raiders" && elem.power != 100 ) {
                if( elem.power < 150 ) {
                    data.emplace_back( _( "    The raiders grew more powerful than any other faction as attrition \
destroyed the Old Guard.  The ruthless men and women who banded together to \
rob refugees and pillage settlements soon found themselves without enough \
victims to survive.  The Hell's Raiders were eventually destroyed when \
infighting erupted into civil war but there were few survivors left to \
celebrate their destruction." ) );
                } else {
                    data.emplace_back( _( "    Fueled by drugs and rage, the Hell's Raiders fought tooth and nail to \
overthrow the last strongholds of the Old Guard.  The costly victories \
brought the warlords abundant territory and slaves but little in the way of \
stability.  Within weeks, infighting led to civil war as tribes vied for \
leadership of the faction.  When only one warlord finally secured control, \
there was nothing left to fight for... just endless cities full of the dead." ) );
                }
                display_table( w, _( "Hell's Raiders" ), 1, data );
            }

        }
        data.clear();
    }

    refresh_all();
}

struct npc_dist_to_player {
    const tripoint ppos;
    npc_dist_to_player() : ppos( g->u.global_omt_location() ) { }
    // Operator overload required to leverage sort API.
    bool operator()( const std::shared_ptr<npc> &a, const std::shared_ptr<npc> &b ) const {
        const tripoint apos = a->global_omt_location();
        const tripoint bpos = b->global_omt_location();
        return square_dist( ppos.x, ppos.y, apos.x, apos.y ) <
               square_dist( ppos.x, ppos.y, bpos.x, bpos.y );
    }
};

void game::disp_NPCs()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0,
                           ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 );

    const tripoint ppos = u.global_omt_location();
    const tripoint &lpos = u.pos();
    mvwprintz( w, 0, 0, c_white, _( "Your overmap position: %d, %d, %d" ), ppos.x, ppos.y, ppos.z );
    mvwprintz( w, 1, 0, c_white, _( "Your local position: %d, %d, %d" ), lpos.x, lpos.y, lpos.z );
    std::vector<std::shared_ptr<npc>> npcs = overmap_buffer.get_npcs_near_player( 100 );
    std::sort( npcs.begin(), npcs.end(), npc_dist_to_player() );
    size_t i;
    for( i = 0; i < 20 && i < npcs.size(); i++ ) {
        const tripoint apos = npcs[i]->global_omt_location();
        mvwprintz( w, i + 3, 0, c_white, "%s: %d, %d, %d", npcs[i]->name.c_str(),
                   apos.x, apos.y, apos.z );
    }
    for( const monster &m : all_monsters() ) {
        mvwprintz( w, i + 3, 0, c_white, "%s: %d, %d, %d", m.name().c_str(),
                   m.posx(), m.posy(), m.posz() );
        ++i;
    }
    wrefresh( w );
    inp_mngr.wait_for_any_key();
}

// A little helper to draw footstep glyphs.
static void draw_footsteps( const catacurses::window &window, const tripoint &offset )
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
    if( test_mode ) {
        return;
    }

    //temporary fix for updating visibility for minimap
    ter_view_z = ( u.pos() + u.view_offset ).z;
    m.build_map_cache( ter_view_z );
    m.update_visibility_cache( ter_view_z );

    draw_sidebar();

    werase( w_terrain );
    draw_ter();
    wrefresh( w_terrain );
}

void game::draw_pixel_minimap()
{
    // Force a refresh of the pixel minimap.
    // only do so if it is in use
    if( pixel_minimap_option && w_pixel_minimap ) {
        werase( w_pixel_minimap );
        //trick window into rendering
        mvwputch( w_pixel_minimap, 0, 0, c_black, ' ' );
        wrefresh( w_pixel_minimap );
    }
}

void game::draw_sidebar()
{
    if( fullscreen ) {
        return;
    }

    // w_status2 is not used with the wide sidebar (wide == !narrow)
    // Don't draw anything on it (no werase, wrefresh) in this case to avoid flickering
    // (it overlays other windows)
    const bool sideStyle = use_narrow_sidebar();

    // Draw Status
    draw_HP( u, w_HP );
    werase( w_status );
    if( sideStyle ) {
        werase( w_status2 );
    }

    // sidestyle ? narrow = 1 : wider = 0
    const catacurses::window &time_window = sideStyle ? w_status2 : w_status;
    const catacurses::window &s_window = sideStyle ?  w_location : w_location_wider;
    werase( s_window );
    u.disp_status( w_status, w_status2 );
    wmove( time_window, 1, sideStyle ? 15 : 43 );
    if( u.has_watch() ) {
        wprintz( time_window, c_white, to_string_time_of_day( calendar::turn ) );
    } else if( get_levz() >= 0 ) {
        std::vector<std::pair<char, nc_color> > vGlyphs;
        vGlyphs.push_back( std::make_pair( '_', c_red ) );
        vGlyphs.push_back( std::make_pair( '_', c_cyan ) );
        vGlyphs.push_back( std::make_pair( '.', c_brown ) );
        vGlyphs.push_back( std::make_pair( ',', c_blue ) );
        vGlyphs.push_back( std::make_pair( '+', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'c', c_light_blue ) );
        vGlyphs.push_back( std::make_pair( '*', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'C', c_white ) );
        vGlyphs.push_back( std::make_pair( '+', c_yellow ) );
        vGlyphs.push_back( std::make_pair( 'c', c_light_blue ) );
        vGlyphs.push_back( std::make_pair( '.', c_brown ) );
        vGlyphs.push_back( std::make_pair( ',', c_blue ) );
        vGlyphs.push_back( std::make_pair( '_', c_red ) );
        vGlyphs.push_back( std::make_pair( '_', c_cyan ) );

        const int iHour = hour_of_day<int>( calendar::turn );
        wprintz( time_window, c_white, "[" );
        bool bAddTrail = false;

        for( int i = 0; i < 14; i += 2 ) {
            if( iHour >= 8 + i && iHour <= 13 + ( i / 2 ) ) {
                wputch( time_window, hilite( c_white ), ' ' );

            } else if( iHour >= 6 + i && iHour <= 7 + i ) {
                wputch( time_window, hilite( vGlyphs[i].second ), vGlyphs[i].first );
                bAddTrail = true;

            } else if( iHour >= ( 18 + i ) % 24 && iHour <= ( 19 + i ) % 24 ) {
                wputch( time_window, vGlyphs[i + 1].second, vGlyphs[i + 1].first );

            } else if( bAddTrail && iHour >= 6 + ( i / 2 ) ) {
                wputch( time_window, hilite( c_white ), ' ' );

            } else {
                wputch( time_window, c_white, ' ' );
            }
        }

        wprintz( time_window, c_white, "]" );
    } else {
        wprintz( time_window, c_white, _( "Time: ???" ) );
    }

    const oter_id &cur_ter = overmap_buffer.ter( u.global_omt_location() );
    wrefresh( s_window );
    mvwprintz( s_window, 0, 0, c_light_gray, _( "Location: " ) );
    wprintz( s_window, c_white, utf8_truncate( cur_ter->get_name(), getmaxx( s_window ) ) );

    if( get_levz() < 0 ) {
        mvwprintz( s_window, 1, 0, c_light_gray, _( "Underground" ) );
    } else {
        mvwprintz( s_window, 1, 0, c_light_gray, _( "Weather :" ) );
        wprintz( s_window, weather_data( weather ).color, " %s", weather_data( weather ).name.c_str() );
    }

    if( u.has_item_with_flag( "THERMOMETER" ) || u.has_bionic( bionic_id( "bio_meteorologist" ) ) ) {
        mvwprintz( sideStyle ? w_status2 : s_window,
                   sideStyle ? 1 : 2, sideStyle ? 32 : 43, c_light_gray, _( "Temp :" ) );
        wprintz( sideStyle ? w_status2 : s_window,
                 c_white, " %s", print_temperature( get_temperature( u.pos() ) ).c_str() );
    }

    //moon phase display
    static std::vector<std::string> vMoonPhase = {"(   )", "(  ))", "( | )", "((  )"};

    const int iPhase = static_cast<int>( get_moon_phase( calendar::turn ) );
    std::string sPhase = vMoonPhase[iPhase % 4];

    if( iPhase > 0 ) {
        sPhase.insert( 5 - ( ( iPhase > 4 ) ? iPhase % 4 : 0 ), "</color>" );
        sPhase.insert( 5 - ( ( iPhase < 4 ) ? iPhase + 1 : 5 ),
                       "<color_" + string_from_color( i_black ) + ">" );
    }
    int x = sideStyle ?  32 : 43;
    int y = sideStyle ?  1 : 0;
    mvwprintz( s_window, y, x, c_light_gray, _( "Moon : " ) );
    trim_and_print( s_window, y, x + 7, 11, c_white, sPhase.c_str() );

    const auto ll = get_light_level( g->u.fine_detail_vision_mod() );
    mvwprintz( s_window, 2, 0, c_light_gray, "%s ", _( "Lighting:" ) );
    wprintz( s_window, ll.second, ll.first.c_str() );

    // display player noise in sidebar
    x = sideStyle ?  32 : 43;
    y = sideStyle ?  2 : 1;
    if( u.is_deaf() ) {
        mvwprintz( s_window, y, x, c_red, _( "Deaf!" ) );
    } else {
        mvwprintz( s_window, y, x, c_light_gray, "%s ", _( "Sound: " ) );
        mvwprintz( s_window, y, x + 7, c_yellow,  std::to_string( u.volume ) );
        wrefresh( s_window );
    }
    u.volume = 0;

    //Safemode coloring
    catacurses::window day_window = sideStyle ? w_status2 : w_status;
    mvwprintz( time_window, 1, 0, c_white, _( "%s, day %d" ),
               calendar::name_season( season_of_year( calendar::turn ) ),
               day_of_season<int>( calendar::turn ) + 1 );

    // don't display SAFE mode in vehicle, doesn't apply.
    if( !u.in_vehicle ) {
        if( safe_mode != SAFE_MODE_OFF || get_option<bool>( "AUTOSAFEMODE" ) ) {
            int iPercent = turnssincelastmon * 100 / get_option<int>( "AUTOSAFEMODETURNS" );
            wmove( sideStyle ? w_status : w_HP, sideStyle ? 5 : 23, sideStyle ? getmaxx( w_status ) - 4 : 0 );
            const std::array<std::string, 4> letters = {{ "S", "A", "F", "E" }};
            for( int i = 0; i < 4; i++ ) {
                nc_color c = ( safe_mode == SAFE_MODE_OFF && iPercent < ( i + 1 ) * 25 ) ? c_red : c_green;
                wprintz( sideStyle ? w_status : w_HP, c, letters[i].c_str() );
            }
        }
    }
    wrefresh( w_status );
    if( sideStyle ) {
        wrefresh( w_status2 );
    }
    wrefresh( w_HP );
    wrefresh( s_window );
    draw_minimap();
    draw_pixel_minimap();
    draw_sidebar_messages();
}

void game::draw_sidebar_messages()
{
    if( fullscreen ) {
        return;
    }

    werase( w_messages );

    // Print liveview or monster info and start log messages output below it.
    int topline = liveview.draw( w_messages, getmaxy( w_messages ) );
    if( topline == 0 ) {
        topline = mon_info( w_messages ) + 2;
    }
    int line = getmaxy( w_messages ) - 1;
    int maxlength = getmaxx( w_messages );
    Messages::display_messages( w_messages, 0, topline, maxlength, line );
    wrefresh( w_messages );
}

void game::draw_critter( const Creature &critter, const tripoint &center )
{
    const int my = POSY + ( critter.posy() - center.y );
    const int mx = POSX + ( critter.posx() - center.x );
    if( !is_valid_in_w_terrain( mx, my ) ) {
        return;
    }
    if( critter.posz() != center.z && m.has_zlevels() ) {
        static constexpr tripoint up_tripoint( 0, 0, 1 );
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

bool game::is_in_viewport( const tripoint &p, int margin ) const
{
    const tripoint diff( u.pos() + u.view_offset - p );

    return ( std::abs( diff.x ) <= getmaxx( w_terrain ) / 2 - margin ) &&
           ( std::abs( diff.y ) <= getmaxy( w_terrain ) / 2 - margin );
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

    for( Creature &critter : all_creatures() ) {
        draw_critter( critter, center );
    }

    if( u.has_active_bionic( bionic_id( "bio_scent_vision" ) ) && u.view_offset.z == 0 ) {
        tripoint tmp = center;
        int &realx = tmp.x;
        int &realy = tmp.y;
        for( realx = posx - POSX; realx <= posx + POSX; realx++ ) {
            for( realy = posy - POSY; realy <= posy + POSY; realy++ ) {
                if( scent.get( tmp ) != 0 ) {
                    int tempx = posx - realx;
                    int tempy = posy - realy;
                    if( !( isBetween( tempx, -2, 2 ) &&
                           isBetween( tempy, -2, 2 ) ) ) {
                        if( critter_at( tmp ) ) {
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
        mvwputch( w_terrain, POSY + ( final_destination.y - ( u.posy() + u.view_offset.y ) ),
                  POSX + ( final_destination.x - ( u.posx() + u.view_offset.x ) ), c_white, 'X' );
    }

    if( u.controlling_vehicle && !looking ) {
        draw_veh_dir_indicator( false );
        draw_veh_dir_indicator( true );
    }

    // Place the cursor over the player as is expected by screen readers.
    wmove( w_terrain, POSY + g->u.pos().y - center.y, POSX + g->u.pos().x - center.x );
}

cata::optional<tripoint> game::get_veh_dir_indicator_location( bool next ) const
{
    if( !get_option<bool>( "VEHICLE_DIR_INDICATOR" ) ) {
        return cata::nullopt;
    }
    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( !vp ) {
        return cata::nullopt;
    }
    vehicle *const veh = &vp->vehicle();
    rl_vec2d face = next ? veh->dir_vec() : veh->face_vec();
    float r = 10.0;
    return tripoint( static_cast<int>( r * face.x ), static_cast<int>( r * face.y ), u.pos().z );
}

void game::draw_veh_dir_indicator( bool next )
{
    if( const cata::optional<tripoint> indicator_offset = get_veh_dir_indicator_location( next ) ) {
        auto col = next ? c_white : c_dark_gray;
        mvwputch( w_terrain, POSY + indicator_offset->y - u.view_offset.y,
                  POSX + indicator_offset->x - u.view_offset.x, col, 'X' );
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
    catacurses::refresh();
}

void game::draw_minimap()
{
    // Draw the box
    werase( w_minimap );
    draw_border( w_minimap );

    const tripoint curs = u.global_omt_location();
    const int cursx = curs.x;
    const int cursy = curs.y;
    const tripoint targ = u.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;

    for( int i = -2; i <= 2; i++ ) {
        for( int j = -2; j <= 2; j++ ) {
            const int omx = cursx + i;
            const int omy = cursy + j;
            nc_color ter_color;
            long ter_sym;
            const bool seen = overmap_buffer.seen( omx, omy, get_levz() );
            const bool vehicle_here = overmap_buffer.has_vehicle( omx, omy, get_levz() );
            if( overmap_buffer.has_note( omx, omy, get_levz() ) ) {

                const std::string &note_text = overmap_buffer.note( omx, omy, get_levz() );

                ter_color = c_yellow;
                ter_sym = 'N';

                int symbolIndex = note_text.find( ':' );
                int colorIndex = note_text.find( ';' );

                bool symbolFirst = symbolIndex < colorIndex;

                if( colorIndex > -1 && symbolIndex > -1 ) {
                    if( symbolFirst ) {
                        if( colorIndex > 4 ) {
                            colorIndex = -1;
                        }
                        if( symbolIndex > 1 ) {
                            symbolIndex = -1;
                            colorIndex = -1;
                        }
                    } else {
                        if( symbolIndex > 4 ) {
                            symbolIndex = -1;
                        }
                        if( colorIndex > 2 ) {
                            colorIndex = -1;
                        }
                    }
                } else if( colorIndex > 2 ) {
                    colorIndex = -1;
                } else if( symbolIndex > 1 ) {
                    symbolIndex = -1;
                }

                if( symbolIndex > -1 ) {
                    int symbolStart = 0;
                    if( colorIndex > -1 && !symbolFirst ) {
                        symbolStart = colorIndex + 1;
                    }
                    ter_sym = note_text.substr( symbolStart, symbolIndex - symbolStart ).c_str()[0];
                }

                if( colorIndex > -1 ) {

                    int colorStart = 0;

                    if( symbolIndex > -1 && symbolFirst ) {
                        colorStart = symbolIndex + 1;
                    }

                    std::string sym = note_text.substr( colorStart, colorIndex - colorStart );

                    if( sym.length() == 2 ) {
                        if( sym == "br" ) {
                            ter_color = c_brown;
                        } else if( sym == "lg" ) {
                            ter_color = c_light_gray;
                        } else if( sym == "dg" ) {
                            ter_color = c_dark_gray;
                        }
                    } else {
                        char colorID = sym.c_str()[0];
                        if( colorID == 'r' ) {
                            ter_color = c_light_red;
                        } else if( colorID == 'R' ) {
                            ter_color = c_red;
                        } else if( colorID == 'g' ) {
                            ter_color = c_light_green;
                        } else if( colorID == 'G' ) {
                            ter_color = c_green;
                        } else if( colorID == 'b' ) {
                            ter_color = c_light_blue;
                        } else if( colorID == 'B' ) {
                            ter_color = c_blue;
                        } else if( colorID == 'W' ) {
                            ter_color = c_white;
                        } else if( colorID == 'C' ) {
                            ter_color = c_cyan;
                        } else if( colorID == 'c' ) {
                            ter_color = c_light_cyan;
                        } else if( colorID == 'P' ) {
                            ter_color = c_pink;
                        } else if( colorID == 'm' ) {
                            ter_color = c_magenta;
                        }
                    }
                }
            } else if( !seen ) {
                ter_sym = ' ';
                ter_color = c_black;
            } else if( vehicle_here ) {
                ter_color = c_cyan;
                ter_sym = 'c';
            } else {
                const oter_id &cur_ter = overmap_buffer.ter( omx, omy, get_levz() );
                ter_sym = cur_ter->get_sym();
                if( overmap_buffer.is_explored( omx, omy, get_levz() ) ) {
                    ter_color = c_dark_gray;
                } else {
                    ter_color = cur_ter->get_color();
                }
            }
            if( !drew_mission && targ.x == omx && targ.y == omy ) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
                drew_mission = true;
                if( i != 0 || j != 0 ) {
                    ter_color = red_background( ter_color );
                }
            }
            if( i == 0 && j == 0 ) {
                mvwputch_hi( w_minimap, 3, 3, ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, 3 + j, 3 + i, ter_color, ter_sym );
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
            int arrowx = -1;
            int arrowy = -1;
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

    const int sight_points = g->u.overmap_sight_range( g->light_level( g->u.posz() ) );
    for( int i = -3; i <= 3; i++ ) {
        for( int j = -3; j <= 3; j++ ) {
            if( i > -3 && i < 3 && j > -3 && j < 3 ) {
                continue; // only do hordes on the border, skip inner map
            }
            const int omx = cursx + i;
            const int omy = cursy + j;
            if( overmap_buffer.get_horde_size( omx, omy, get_levz() ) >= HORDE_VISIBILITY_SIZE ) {
                const tripoint cur_pos {
                    omx, omy, get_levz()
                };
                if( overmap_buffer.seen( omx, omy, get_levz() )
                    && g->u.overmap_los( cur_pos, sight_points ) ) {
                    mvwputch( w_minimap, j + 3, i + 3, c_green,
                              overmap_buffer.get_horde_size( omx, omy, get_levz() ) > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
                }
            }
        }
    }

    wrefresh( w_minimap );
}

float game::natural_light_level( const int zlev ) const
{
    // ignore while underground or above limits
    if( zlev > OVERMAP_HEIGHT || zlev < 0 ) {
        return LIGHT_AMBIENT_MINIMAL;
    }

    if( latest_lightlevels[zlev] > -std::numeric_limits<float>::max() ) {
        // Already found the light level for now?
        return latest_lightlevels[zlev];
    }

    float ret = LIGHT_AMBIENT_MINIMAL;

    // Sunlight/moonlight related stuff
    if( !lightning_active ) {
        ret = calendar::turn.sunlight();
    } else {
        // Recent lightning strike has lit the area
        ret = DAYLIGHT_LEVEL;
    }

    ret += weather_data( weather ).light_modifier;

    // Artifact light level changes here. Even though some of these only have an effect
    // aboveground it is cheaper performance wise to simply iterate through the entire
    // list once instead of twice.
    float mod_ret = -1;
    // Each artifact change does std::max(mod_ret, new val) since a brighter end value
    // will trump a lower one.
    if( const event *e = events.get( EVENT_DIM ) ) {
        // EVENT_DIM slowly dims the natural sky level, then relights it.
        const time_duration left = e->when - calendar::turn;
        // EVENT_DIM has an occurrence date of turn + 50, so the first 25 dim it,
        if( left > 25_turns ) {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( left - 25_turns ) ) / 25_turns );
            // and the last 25 scale back towards normal.
        } else {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( 25_turns - left ) ) / 25_turns );
        }
    }
    if( events.queued( EVENT_ARTIFACT_LIGHT ) ) {
        // EVENT_ARTIFACT_LIGHT causes everywhere to become as bright as day.
        mod_ret = std::max<float>( ret, DAYLIGHT_LEVEL );
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
    return LIGHT_RANGE( light );
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

Creature *game::is_hostile_nearby()
{
    int distance = ( get_option<int>( "SAFEMODEPROXIMITY" ) <= 0 ) ? MAX_VIEW_DISTANCE :
                   get_option<int>( "SAFEMODEPROXIMITY" );
    return is_hostile_within( distance );
}

Creature *game::is_hostile_very_close()
{
    return is_hostile_within( DANGEROUS_PROXIMITY );
}

Creature *game::is_hostile_within( int distance )
{
    for( auto &critter : u.get_visible_creatures( distance ) ) {
        if( u.attitude_to( *critter ) == Creature::A_HOSTILE ) {
            return critter;
        }
    }

    return nullptr;
}

//get the fishable critters around and return these
std::vector<monster *> game::get_fishable( int distance, const tripoint &fish_pos )
{
    // We're going to get the contiguous fishable terrain starting at
    // the provided fishing location (e.g. where a line was cast or a fish
    // trap was set), and then check whether or not fishable monsters are
    // actually in those locations. This will help us ensure that we're
    // getting our fish from the location that we're ACTUALLY fishing,
    // rather than just somewhere in the vicinity.

    std::unordered_set<tripoint> visited;

    const tripoint fishing_boundary_min( fish_pos.x - distance, fish_pos.y - distance, fish_pos.z );
    const tripoint fishing_boundary_max( fish_pos.x + distance, fish_pos.y + distance, fish_pos.z );

    const box fishing_boundaries( fishing_boundary_min, fishing_boundary_max );

    const auto get_fishable_terrain = [&]( tripoint starting_point,
    std::unordered_set<tripoint> &fishable_terrain ) {
        std::queue<tripoint> to_check;
        to_check.push( starting_point );
        while( !to_check.empty() ) {
            const tripoint current_point = to_check.front();
            to_check.pop();

            // We've been here before, so bail.
            if( visited.find( current_point ) != visited.end() ) {
                continue;
            }

            // This point is out of bounds, so bail.
            if( !generic_inbounds( current_point, fishing_boundaries ) ) {
                continue;
            }

            // Mark this point as visited.
            visited.emplace( current_point );

            if( m.has_flag( "FISHABLE", current_point ) ) {
                fishable_terrain.emplace( current_point );
                to_check.push( tripoint( current_point.x, current_point.y + 1, current_point.z ) );
                to_check.push( tripoint( current_point.x, current_point.y - 1, current_point.z ) );
                to_check.push( tripoint( current_point.x + 1, current_point.y, current_point.z ) );
                to_check.push( tripoint( current_point.x - 1, current_point.y, current_point.z ) );
            }
        }
        return;
    };

    // Starting at the provided location, get our fishable terrain
    // and populate a set with those locations which we'll then use
    // to determine if any fishable monsters are in those locations.
    std::unordered_set<tripoint> fishable_points;
    get_fishable_terrain( fish_pos, fishable_points );

    std::vector<monster *> unique_fish;
    for( monster &critter : all_monsters() ) {
        // If it is fishable...
        if( critter.has_flag( MF_FISHABLE ) ) {
            const tripoint critter_pos = critter.pos();
            // ...and it is in a fishable location.
            if( fishable_points.find( critter_pos ) != fishable_points.end() ) {
                unique_fish.push_back( &critter );
            }
        }
    }

    return unique_fish;
}

// Print monster info to the given window, and return the lowest row (0-indexed)
// to which we printed. This is used to share a window with the message log and
// make optimal use of space.
int game::mon_info( const catacurses::window &w )
{
    const int width = getmaxx( w );
    const int maxheight = 12;
    const int startrow = use_narrow_sidebar() ? 1 : 0;

    int newseen = 0;
    const int iProxyDist = ( get_option<int>( "SAFEMODEPROXIMITY" ) <= 0 ) ? MAX_VIEW_DISTANCE :
                           get_option<int>( "SAFEMODEPROXIMITY" );
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    std::vector<npc *> unique_types[9];
    std::vector<const mtype *> unique_mons[9];
    // dangerous_types tracks whether we should print in red to warn the player
    bool dangerous[8];
    for( auto &dangerou : dangerous ) {
        dangerou = false;
    }

    tripoint view = u.pos() + u.view_offset;
    new_seen_mon.clear();

    const int current_turn = calendar::turn;
    const int sm_ignored_turns = get_option<int>( "SAFEMODEIGNORETURNS" );

    for( auto &c : u.get_visible_creatures( MAPSIZE_X ) ) {
        const auto m = dynamic_cast<monster *>( c );
        const auto p = dynamic_cast<npc *>( c );
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

            const monster_attitude matt = critter.attitude( &u );
            const int mon_dist = rl_dist( u.pos(), critter.pos() );
            safemode_state = get_safemode().check_monster( critter.name(), critter.attitude_to( u ), mon_dist );

            if( ( !safemode_empty && safemode_state == RULE_BLACKLISTED ) || ( safemode_empty &&
                    ( MATT_ATTACK == matt || MATT_FOLLOW == matt ) ) ) {
                if( index < 8 && critter.sees( g->u ) ) {
                    dangerous[index] = true;
                }

                if( !safemode_empty || mon_dist <= iProxyDist ) {
                    bool passmon = false;
                    if( critter.ignoring > 0 ) {
                        if( safe_mode != SAFE_MODE_ON ) {
                            critter.ignoring = 0;
                        } else if( ( sm_ignored_turns == 0 || ( critter.lastseen_turn &&
                                                                to_turn<int>( *critter.lastseen_turn ) > current_turn - sm_ignored_turns ) ) &&
                                   ( mon_dist > critter.ignoring / 2 || mon_dist < 6 ) ) {
                            passmon = true;
                        }
                        critter.lastseen_turn = current_turn;
                    }

                    if( !passmon ) {
                        newseen++;
                        new_seen_mon.push_back( shared_from( critter ) );
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
            safemode_state = get_safemode().check_monster( get_safemode().npc_type_name(), p->attitude_to( u ),
                             npc_dist );

            if( ( !safemode_empty && safemode_state == RULE_BLACKLISTED ) || ( safemode_empty &&
                    p->get_attitude() == NPCATT_KILL ) ) {
                if( !safemode_empty || npc_dist <= iProxyDist ) {
                    newseen++;
                }
            }
            unique_types[index].push_back( p );
        }
    }

    if( newseen > mostseen ) {
        if( newseen - mostseen == 1 ) {
            if( !new_seen_mon.empty() ) {
                monster &critter = *new_seen_mon.back();
                cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                 string_format( _( "%s spotted!" ), critter.name().c_str() ) );
                if( u.has_trait( trait_id( "M_DEFENDER" ) ) && critter.type->in_species( PLANT ) ) {
                    add_msg( m_warning, _( "We have detected a %s - an enemy of the Mycus!" ), critter.name().c_str() );
                    if( !u.has_effect( effect_adrenaline_mycus ) ) {
                        u.add_effect( effect_adrenaline_mycus, 30_minutes );
                    } else if( u.get_effect_int( effect_adrenaline_mycus ) == 1 ) {
                        // Triffids present.  We ain't got TIME to adrenaline comedown!
                        u.add_effect( effect_adrenaline_mycus, 15_minutes );
                        u.mod_pain( 3 ); // Does take it out of you, though
                        add_msg( m_info, _( "Our fibers strain with renewed wrath!" ) );
                    }
                }
            } else {
                //Hostile NPC
                cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                 _( "Hostile survivor spotted!" ) );
            }
        } else {
            cancel_activity_or_ignore_query( distraction_type::hostile_spotted, _( "Monsters spotted!" ) );
        }
        turnssincelastmon = 0;
        if( safe_mode == SAFE_MODE_ON ) {
            set_safe_mode( SAFE_MODE_STOP );
        }
    } else if( get_option<bool>( "AUTOSAFEMODE" ) && newseen == 0 ) { // Auto-safe mode
        turnssincelastmon++;
        if( turnssincelastmon >= get_option<int>( "AUTOSAFEMODETURNS" ) && safe_mode == SAFE_MODE_OFF ) {
            set_safe_mode( SAFE_MODE_ON );
        }
    }

    if( newseen == 0 && safe_mode == SAFE_MODE_STOP ) {
        set_safe_mode( SAFE_MODE_ON );
    }

    mostseen = newseen;

    // Print the direction headings
    // Reminder:
    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)

    const std::array<std::string, 8> dir_labels = {{
            _( "North:" ), _( "NE:" ), _( "East:" ), _( "SE:" ),
            _( "South:" ), _( "SW:" ), _( "West:" ), _( "NW:" )
        }
    };
    std::array<int, 8> widths;
    for( int i = 0; i < 8; i++ ) {
        widths[i] = utf8_width( dir_labels[i].c_str() );
    }
    std::array<int, 8> xcoords;
    const std::array<int, 8> ycoords = {{ 0, 0, 1, 2, 2, 2, 1, 0 }};
    xcoords[0] = xcoords[4] = width / 3;
    xcoords[1] = xcoords[3] = xcoords[2] = ( width / 3 ) * 2;
    xcoords[5] = xcoords[6] = xcoords[7] = 0;
    //for the alignment of the 1,2,3 rows on the right edge
    xcoords[2] -= utf8_width( _( "East:" ) ) - utf8_width( _( "NE:" ) );
    for( int i = 0; i < 8; i++ ) {
        nc_color c = unique_types[i].empty() && unique_mons[i].empty() ? c_dark_gray
                     : ( dangerous[i] ? c_light_red : c_light_gray );
        mvwprintz( w, ycoords[i] + startrow, xcoords[i], c, dir_labels[i].c_str() );
    }

    // Print the symbols of all monsters in all directions.
    for( int i = 0; i < 8; i++ ) {
        point pr( xcoords[i] + widths[i] + 1, ycoords[i] + startrow );

        // The list of symbols needs a space on each end.
        int symroom = ( width / 3 ) - widths[i] - 2;
        const int typeshere_npc = unique_types[i].size();
        const int typeshere_mon = unique_mons[i].size();
        const int typeshere = typeshere_mon + typeshere_npc;
        for( int j = 0; j < typeshere && j < symroom; j++ ) {
            nc_color c;
            std::string sym;
            if( symroom < typeshere && j == symroom - 1 ) {
                // We've run out of room!
                c = c_white;
                sym = "+";
            } else if( j < typeshere_npc ) {
                switch( unique_types[i][j]->get_attitude() ) {
                    case NPCATT_KILL:
                        c = c_red;
                        break;
                    case NPCATT_FOLLOW:
                        c = c_light_green;
                        break;
                    default:
                        c = c_pink;
                        break;
                }
                sym = "@";
            } else {
                const mtype &mt = *unique_mons[i][j - typeshere_npc];
                c = mt.color;
                sym = mt.sym;
            }
            mvwprintz( w, pr.y, pr.x, c, sym );

            pr.x++;
        }
    } // for (int i = 0; i < 8; i++)

    // Now we print their full names!

    std::set<const mtype *> listed_mons;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr( 0, 4 + startrow );

    int lastrowprinted = 2 + startrow;

    // Print monster names, starting with those at location 8 (nearby).
    for( int j = 8; j >= 0 && pr.y < maxheight; j-- ) {
        // Separate names by some number of spaces (more for local monsters).
        int namesep = ( j == 8 ? 2 : 1 );
        for( const mtype *type : unique_mons[j] ) {
            if( pr.y >= maxheight ) {
                // no space to print to anyway
                break;
            }
            if( listed_mons.count( type ) > 0 ) {
                // this type is already printed.
                continue;
            }
            listed_mons.insert( type );

            const mtype &mt = *type;
            const std::string name = mt.nname();

            // Move to the next row if necessary. (The +2 is for the "Z ").
            if( pr.x + 2 + utf8_width( name ) >= width ) {
                pr.y++;
                pr.x = 0;
            }

            if( pr.y < maxheight ) { // Don't print if we've overflowed
                lastrowprinted = pr.y;
                mvwprintz( w, pr.y, pr.x, mt.color, mt.sym );
                pr.x += 2; // symbol and space
                nc_color danger = c_dark_gray;
                if( mt.difficulty >= 30 ) {
                    danger = c_red;
                } else if( mt.difficulty >= 16 ) {
                    danger = c_light_red;
                } else if( mt.difficulty >= 8 ) {
                    danger = c_white;
                } else if( mt.agro > 0 ) {
                    danger = c_light_gray;
                }
                mvwprintz( w, pr.y, pr.x, danger, name );
                pr.x += utf8_width( name ) + namesep;
            }
        }
    }

    return lastrowprinted;
}

void game::cleanup_dead()
{
    // Dead monsters need to stay in the tracker until everything else that needs to die does so
    // This is because dying monsters can still interact with other dying monsters (@ref Creature::killer)
    bool monster_is_dead = critter_tracker->kill_marked_for_death();

    bool npc_is_dead = false;
    // can't use all_npcs as that does not include dead ones
    for( const auto &n : active_npc ) {
        if( n->is_dead() ) {
            n->die( nullptr ); // make sure this has been called to create corpses etc.
            npc_is_dead = true;
        }
    }

    if( monster_is_dead ) {
        // From here on, pointers to creatures get invalidated as dead creatures get removed.
        critter_tracker->remove_dead();
    }

    if( npc_is_dead ) {
        for( auto it = active_npc.begin(); it != active_npc.end(); ) {
            if( ( *it )->is_dead() ) {
                overmap_buffer.remove_npc( ( *it )->getID() );
                it = active_npc.erase( it );
            } else {
                it++;
            }
        }
    }

    critter_died = false;
}

void game::monmove()
{
    cleanup_dead();

    // Make sure these don't match the first time around.
    tripoint cached_lev = m.get_abs_sub() + tripoint( 1, 0, 0 );

    mfactions monster_factions;
    const auto &playerfaction = mfaction_str_id( "player" );
    for( monster &critter : all_monsters() ) {
        // The first time through, and any time the map has been shifted,
        // recalculate monster factions.
        if( cached_lev != m.get_abs_sub() ) {
            // monster::plan() needs to know about all monsters on the same team as the monster.
            monster_factions.clear();
            for( monster &critter : all_monsters() ) {
                if( critter.friendly == 0 ) {
                    // Only 1 faction per mon at the moment.
                    monster_factions[ critter.faction ].insert( &critter );
                } else {
                    monster_factions[ playerfaction ].insert( &critter );
                }
            }
            cached_lev = m.get_abs_sub();
        }

        // Critters in impassable tiles get pushed away, unless it's not impassable for them
        if( !critter.is_dead() && m.impassable( critter.pos() ) && !critter.can_move_to( critter.pos() ) ) {
            dbg( D_ERROR ) << "game:monmove: " << critter.name().c_str()
                           << " can't move to its location! (" << critter.posx()
                           << ":" << critter.posy() << ":" << critter.posz() << "), "
                           << m.tername( critter.pos() ).c_str();
            add_msg( m_debug, "%s can't move to its location! (%d,%d,%d), %s", critter.name().c_str(),
                     critter.posx(), critter.posy(), critter.posz(), m.tername( critter.pos() ).c_str() );
            bool okay = false;
            for( const tripoint &dest : m.points_in_radius( critter.pos(), 3 ) ) {
                if( critter.can_move_to( dest ) && is_empty( dest ) ) {
                    critter.setpos( dest );
                    okay = true;
                    break;
                }
            }
            if( !okay ) {
                // die of "natural" cause (overpopulation is natural)
                critter.die( nullptr );
            }
        }

        if( !critter.is_dead() ) {
            critter.process_turn();
        }

        m.creature_in_field( critter );

        while( critter.moves > 0 && !critter.is_dead() ) {
            critter.made_footstep = false;
            // Controlled critters don't make their own plans
            if( !critter.has_effect( effect_controlled ) ) {
                // Formulate a path to follow
                critter.plan( monster_factions );
            }
            critter.move(); // Move one square, possibly hit u
            critter.process_triggers();
            m.creature_in_field( critter );
        }

        if( !critter.is_dead() &&
            u.has_active_bionic( bionic_id( "bio_alarm" ) ) &&
            u.power_level >= 25 &&
            rl_dist( u.pos(), critter.pos() ) <= 5 &&
            !critter.is_hallucination() ) {
            u.charge_power( -25 );
            add_msg( m_warning, _( "Your motion alarm goes off!" ) );
            cancel_activity_or_ignore_query( distraction_type::motion_alarm,
                                             _( "Your motion alarm goes off!" ) );
            if( u.has_effect( efftype_id( "sleep" ) ) ) {
                u.wake_up();
            }
        }
    }

    cleanup_dead();

    // The remaining monsters are all alive, but may be outside of the reality bubble.
    // If so, despawn them. This is not the same as dying, they will be stored for later and the
    // monster::die function is not called.
    for( monster &critter : all_monsters() ) {
        if( critter.posx() < 0 - ( MAPSIZE_X ) / 6 ||
            critter.posy() < 0 - ( MAPSIZE_Y ) / 6 ||
            critter.posx() > ( MAPSIZE_X * 7 ) / 6 ||
            critter.posy() > ( MAPSIZE_Y * 7 ) / 6 ) {
            despawn_monster( critter );
        }
    }

    // Now, do active NPCs.
    for( npc &guy : g->all_npcs() ) {
        int turns = 0;
        m.creature_in_field( guy );
        guy.process_turn();
        while( !guy.is_dead() && !guy.in_sleep_state() && guy.moves > 0 && turns < 10 ) {
            int moves = guy.moves;
            guy.move();
            if( moves == guy.moves ) {
                // Count every time we exit npc::move() without spending any moves.
                turns++;
            }

            // Turn on debug mode when in infinite loop
            // It has to be done before the last turn, otherwise
            // there will be no meaningful debug output.
            if( turns == 9 ) {
                debugmsg( "NPC %s entered infinite loop. Turning on debug mode",
                          guy.name.c_str() );
                debug_mode = true;
            }
        }

        // If we spun too long trying to decide what to do (without spending moves),
        // Invoke cranial detonation to prevent an infinite loop.
        if( turns == 10 ) {
            add_msg( _( "%s's brain explodes!" ), guy.name.c_str() );
            guy.die( nullptr );
        }

        if( !guy.is_dead() ) {
            guy.process_active_items();
            guy.update_body();
        }
    }
    cleanup_dead();
}

void game::flashbang( const tripoint &p, bool player_immune )
{
    draw_explosion( p, 8, c_white );
    int dist = rl_dist( u.pos(), p );
    if( dist <= 8 && !player_immune ) {
        if( !u.has_bionic( bionic_id( "bio_ears" ) ) && !u.is_wearing( "rm13_armor_on" ) ) {
            u.add_effect( effect_deaf, time_duration::from_turns( 40 - dist * 4 ) );
        }
        if( m.sees( u.pos(), p, 8 ) ) {
            int flash_mod = 0;
            if( u.has_trait( trait_PER_SLIME ) ) {
                if( one_in( 2 ) ) {
                    flash_mod = 3; // Yay, you weren't looking!
                }
            } else if( u.has_trait( trait_PER_SLIME_OK ) ) {
                flash_mod = 8; // Just retract those and extrude fresh eyes
            } else if( u.has_bionic( bionic_id( "bio_sunglasses" ) ) || u.is_wearing( "rm13_armor_on" ) ) {
                flash_mod = 6;
            } else if( u.worn_with_flag( "BLIND" ) || u.worn_with_flag( "FLASH_PROTECTION" ) ) {
                flash_mod = 3; // Not really proper flash protection, but better than nothing
            }
            u.add_env_effect( effect_blind, bp_eyes, ( 12 - flash_mod - dist ) / 2,
                              time_duration::from_turns( 10 - dist ) );
        }
    }
    for( monster &critter : all_monsters() ) {
        //@todo: can the following code be called for all types of creatures
        dist = rl_dist( critter.pos(), p );
        if( dist <= 8 ) {
            if( dist <= 4 ) {
                critter.add_effect( effect_stunned, time_duration::from_turns( 10 - dist ) );
            }
            if( critter.has_flag( MF_SEES ) && m.sees( critter.pos(), p, 8 ) ) {
                critter.add_effect( effect_blind, time_duration::from_turns( 18 - dist ) );
            }
            if( critter.has_flag( MF_HEARS ) ) {
                critter.add_effect( effect_deaf, time_duration::from_turns( 60 - dist * 4 ) );
            }
        }
    }
    sounds::sound( p, 12, sounds::sound_t::combat, _( "a huge boom!" ) );
    // TODO: Blind/deafen NPC
}

void game::shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult,
                      bool ignore_player )
{
    draw_explosion( p, radius, c_blue );

    sounds::sound( p, force * force * dam_mult / 2, sounds::sound_t::combat, _( "Crack!" ) );

    for( monster &critter : all_monsters() ) {
        if( rl_dist( critter.pos(), p ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), critter.name().c_str() );
            knockback( p, critter.pos(), force, stun, dam_mult );
        }
    }
    //@todo: combine the two loops and the case for g->u using all_creatures()
    for( npc &guy : all_npcs() ) {
        if( rl_dist( guy.pos(), p ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), guy.name.c_str() );
            knockback( p, guy.pos(), force, stun, dam_mult );
        }
    }
    if( rl_dist( u.pos(), p ) <= radius && !ignore_player &&
        ( !u.has_trait( trait_LEG_TENT_BRACE ) || u.footwear_factor() == 1 ||
          ( u.footwear_factor() == .5 && one_in( 2 ) ) ) ) {
        add_msg( m_bad, _( "You're caught in the shockwave!" ) );
        knockback( p, u.pos(), force, stun, dam_mult );
    }
}

/* Knockback target at t by force number of tiles in direction from s to t
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( const tripoint &s, const tripoint &t, int force, int stun, int dam_mult )
{
    std::vector<tripoint> traj;
    traj.clear();
    traj = line_to( s, t, 0, 0 );
    traj.insert( traj.begin(), s ); // how annoying, line_to() doesn't include the originating point!
    traj = continue_line( traj, force );
    traj.insert( traj.begin(), t ); // how annoying, continue_line() doesn't either!

    knockback( traj, force, stun, dam_mult );
}

/* Knockback target at traj.front() along line traj; traj should already have considered knockback distance.
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( std::vector<tripoint> &traj, int force, int stun, int dam_mult )
{
    ( void )force; //FIXME: unused but header says it should do something
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    tripoint tp = traj.front();
    if( !critter_at( tp ) ) {
        debugmsg( _( "Nothing at (%d,%d,%d) to knockback!" ), tp.x, tp.y, tp.z );
        return;
    }
    int force_remaining = 0;
    if( monster *const targ = critter_at<monster>( tp, true ) ) {
        if( stun > 0 ) {
            targ->add_effect( effect_stunned, 1_turns * stun );
            add_msg( _( "%s was stunned!" ), targ->name().c_str() );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i].x, traj[i].y ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    add_msg( _( "%s was stunned!" ), targ->name().c_str() );
                    add_msg( _( "%s slammed into an obstacle!" ), targ->name().c_str() );
                    targ->apply_damage( nullptr, bp_torso, dam_mult * force_remaining );
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( critter_at( traj[i] ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    add_msg( _( "%s was stunned!" ), targ->name().c_str() );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                if( critter_at<monster>( traj.front() ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->name().c_str() );
                } else if( npc *const guy = critter_at<npc>( traj.front() ) ) {
                    if( guy->male ) {
                        add_msg( _( "%s collided with someone else and sent him flying!" ),
                                 targ->name().c_str() );
                    } else {
                        add_msg( _( "%s collided with someone else and sent her flying!" ),
                                 targ->name().c_str() );
                    }
                } else if( u.pos() == traj.front() ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->name().c_str() );
                }
                knockback( traj, force_remaining, stun, dam_mult );
                break;
            }
            targ->setpos( traj[i] );
            if( m.has_flag( "LIQUID", targ->pos() ) && !targ->can_drown() && !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s drowns!" ), targ->name().c_str() );
                }
            }
            if( !m.has_flag( "LIQUID", targ->pos() ) && targ->has_flag( MF_AQUATIC ) &&
                !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s flops around and dies!" ), targ->name().c_str() );
                }
            }
        }
    } else if( npc *const targ = critter_at<npc>( tp ) ) {
        if( stun > 0 ) {
            targ->add_effect( effect_stunned, 1_turns * stun );
            add_msg( _( "%s was stunned!" ), targ->name.c_str() );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i].x, traj[i].y ) ) { // oops, we hit a wall!
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    if( targ->has_effect( effect_stunned ) ) {
                        add_msg( _( "%s was stunned!" ), targ->name.c_str() );
                    }

                    std::array<body_part, 8> bps = {{
                            bp_head,
                            bp_arm_l, bp_arm_r,
                            bp_hand_l, bp_hand_r,
                            bp_torso,
                            bp_leg_l, bp_leg_r
                        }
                    };
                    for( auto &bp : bps ) {
                        if( one_in( 2 ) ) {
                            targ->deal_damage( nullptr, bp, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                        }
                    }
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( critter_at( traj[i] ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    add_msg( _( "%s was stunned!" ), targ->name.c_str() );
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                const tripoint &traj_front = traj.front();
                if( critter_at<monster>( traj_front ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->name.c_str() );
                } else if( npc *const guy = critter_at<npc>( traj_front ) ) {
                    if( guy->male ) {
                        add_msg( _( "%s collided with someone else and sent him flying!" ),
                                 targ->name.c_str() );
                    } else {
                        add_msg( _( "%s collided with someone else and sent her flying!" ),
                                 targ->name.c_str() );
                    }
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y &&
                           ( u.has_trait( trait_LEG_TENT_BRACE ) && ( !u.footwear_factor() ||
                                   ( u.footwear_factor() == .5 && one_in( 2 ) ) ) ) ) {
                    add_msg( _( "%s collided with you, and barely dislodges your tentacles!" ), targ->name.c_str() );
                    force_remaining = 1;
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->name.c_str() );
                }
                knockback( traj, force_remaining, stun, dam_mult );
                break;
            }
            targ->setpos( traj[i] );
        }
    } else if( u.pos() == tp ) {
        if( stun > 0 ) {
            u.add_effect( effect_stunned, 1_turns * stun );
            add_msg( m_bad, ngettext( "You were stunned for %d turn!",
                                      "You were stunned for %d turns!",
                                      stun ),
                     stun );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i] ) ) { // oops, we hit a wall!
                u.setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    if( u.has_effect( effect_stunned ) ) {
                        add_msg( m_bad, ngettext( "You were stunned AGAIN for %d turn!",
                                                  "You were stunned AGAIN for %d turns!",
                                                  force_remaining ),
                                 force_remaining );
                    } else {
                        add_msg( m_bad, ngettext( "You were stunned for %d turn!",
                                                  "You were stunned for %d turns!",
                                                  force_remaining ),
                                 force_remaining );
                    }
                    u.add_effect( effect_stunned, 1_turns * force_remaining );
                    std::array<body_part, 8> bps = {{
                            bp_head,
                            bp_arm_l, bp_arm_r,
                            bp_hand_l, bp_hand_r,
                            bp_torso,
                            bp_leg_l, bp_leg_r
                        }
                    };
                    for( auto &bp : bps ) {
                        if( one_in( 2 ) ) {
                            u.deal_damage( nullptr, bp, damage_instance( DT_BASH, force_remaining * dam_mult ) );
                        }
                    }
                    u.check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( critter_at( traj[i] ) ) {
                u.setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    if( u.has_effect( effect_stunned ) ) {
                        add_msg( m_bad, ngettext( "You were stunned AGAIN for %d turn!",
                                                  "You were stunned AGAIN for %d turns!",
                                                  force_remaining ),
                                 force_remaining );
                    } else {
                        add_msg( m_bad, ngettext( "You were stunned for %d turn!",
                                                  "You were stunned for %d turns!",
                                                  force_remaining ),
                                 force_remaining );
                    }
                    u.add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                if( critter_at<monster>( traj.front() ) ) {
                    add_msg( _( "You collided with something and sent it flying!" ) );
                } else if( npc *const guy = critter_at<npc>( traj.front() ) ) {
                    if( guy->male ) {
                        add_msg( _( "You collided with someone and sent him flying!" ) );
                    } else {
                        add_msg( _( "You collided with someone and sent her flying!" ) );
                    }
                }
                knockback( traj, force_remaining, stun, dam_mult );
                break;
            }
            if( m.has_flag( "LIQUID", u.pos() ) && force_remaining < 1 ) {
                plswim( u.pos() );
            } else {
                u.setpos( traj[i] );
            }
        }
    }
}

void game::use_computer( const tripoint &p )
{
    if( u.has_trait( trait_id( "ILLITERATE" ) ) ) {
        add_msg( m_info, _( "You can not read a computer screen!" ) );
        return;
    }
    if( u.is_blind() ) {
        // we don't have screen readers in game
        add_msg( m_info, _( "You can not see a computer screen!" ) );
        return;
    }
    if( u.has_trait( trait_id( "HYPEROPIC" ) ) && !u.worn_with_flag( "FIX_FARSIGHT" ) &&
        !u.has_effect( effect_contacts ) ) {
        add_msg( m_info, _( "You'll need to put on reading glasses before you can see the screen." ) );
        return;
    }

    computer *used = m.computer_at( p );

    if( used == nullptr ) {
        if( m.has_flag( "CONSOLE", p ) ) { //Console without map data
            add_msg( m_bad, _( "The console doesn't display anything coherent." ) );
        } else {
            dbg( D_ERROR ) << "game:use_computer: Tried to use computer at (" <<
                           p.x << ", " << p.y << ", " << p.z << ") - none there";
            debugmsg( "Tried to use computer at (%d, %d, %d) - none there", p.x, p.y, p.z );
        }
        return;
    }

    used->use();

    refresh_all();
}

void game::resonance_cascade( const tripoint &p )
{
    const time_duration maxglow = time_duration::from_turns( 100 - 5 * trig_dist( p, u.pos() ) );
    const time_duration minglow = std::max( 0_turns, time_duration::from_turns( 60 - 5 * trig_dist( p,
                                            u.pos() ) ) );
    MonsterGroupResult spawn_details;
    monster invader;
    if( maxglow > 0_turns ) {
        u.add_effect( effect_teleglow, rng( minglow, maxglow ) * 100 );
    }
    int startx = ( p.x < 8 ? 0 : p.x - 8 ), endx = ( p.x + 8 >= SEEX * 3 ? SEEX * 3 - 1 : p.x + 8 );
    int starty = ( p.y < 8 ? 0 : p.y - 8 ), endy = ( p.y + 8 >= SEEY * 3 ? SEEY * 3 - 1 : p.y + 8 );
    tripoint dest( startx, starty, p.z );
    for( int &i = dest.x; i <= endx; i++ ) {
        for( int &j = dest.y; j <= endy; j++ ) {
            switch( rng( 1, 80 ) ) {
                case 1:
                case 2:
                    emp_blast( dest );
                    break;
                case 3:
                case 4:
                case 5:
                    for( int k = i - 1; k <= i + 1; k++ ) {
                        for( int l = j - 1; l <= j + 1; l++ ) {
                            field_id type = fd_null;
                            switch( rng( 1, 7 ) ) {
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
                            if( !one_in( 3 ) ) {
                                m.add_field( {k, l, p.z}, type, 3 );
                            }
                        }
                    }
                    break;
                case  6:
                case  7:
                case  8:
                case  9:
                case 10:
                    m.trap_set( dest, tr_portal );
                    break;
                case 11:
                case 12:
                    m.trap_set( dest, tr_goo );
                    break;
                case 13:
                case 14:
                case 15:
                    spawn_details = MonsterGroupManager::GetResultFromGroup( mongroup_id( "GROUP_NETHER" ) );
                    invader = monster( spawn_details.name, dest );
                    add_zombie( invader );
                    break;
                case 16:
                case 17:
                case 18:
                    m.destroy( dest );
                    break;
                case 19:
                    explosion( dest, rng( 1, 10 ), rng( 0, 1 ) * rng( 0, 6 ), one_in( 4 ) );
                    break;
                default:
                    break;
            }
        }
    }
}

void game::scrambler_blast( const tripoint &p )
{
    if( monster *const mon_ptr = critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( critter.has_flag( MF_ELECTRONIC ) ) {
            critter.make_friendly();
        }
        add_msg( m_warning, _( "The %s sparks and begins searching for a target!" ),
                 critter.name().c_str() );
    }
}

void game::emp_blast( const tripoint &p )
{
    // TODO: Implement z part
    int x = p.x;
    int y = p.y;
    const bool sight = g->u.sees( p );
    if( m.has_flag( "CONSOLE", x, y ) ) {
        if( sight ) {
            add_msg( _( "The %s is rendered non-functional!" ), m.tername( x, y ).c_str() );
        }
        m.ter_set( x, y, t_console_broken );
        return;
    }
    // TODO: More terrain effects.
    if( m.ter( x, y ) == t_card_science || m.ter( x, y ) == t_card_military ) {
        int rn = rng( 1, 100 );
        if( rn > 92 || rn < 40 ) {
            if( sight ) {
                add_msg( _( "The card reader is rendered non-functional." ) );
            }
            m.ter_set( x, y, t_card_reader_broken );
        }
        if( rn > 80 ) {
            if( sight ) {
                add_msg( _( "The nearby doors slide open!" ) );
            }
            for( int i = -3; i <= 3; i++ ) {
                for( int j = -3; j <= 3; j++ ) {
                    if( m.ter( x + i, y + j ) == t_door_metal_locked ) {
                        m.ter_set( x + i, y + j, t_floor );
                    }
                }
            }
        }
        if( rn >= 40 && rn <= 80 ) {
            if( sight ) {
                add_msg( _( "Nothing happens." ) );
            }
        }
    }
    if( monster *const mon_ptr = critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( critter.has_flag( MF_ELECTRONIC ) ) {
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
                if( sight ) {
                    add_msg( _( "The %s beeps erratically and deactivates!" ), critter.name().c_str() );
                }
                m.add_item_or_charges( x, y, critter.to_item() );
                for( auto &ammodef : critter.ammo ) {
                    if( ammodef.second > 0 ) {
                        m.spawn_item( x, y, ammodef.first, 1, ammodef.second, calendar::turn );
                    }
                }
                remove_zombie( critter );
            } else {
                if( sight ) {
                    add_msg( _( "The EMP blast fries the %s!" ), critter.name().c_str() );
                }
                int dam = dice( 10, 10 );
                critter.apply_damage( nullptr, bp_torso, dam );
                critter.check_dead_state();
                if( !critter.is_dead() && one_in( 6 ) ) {
                    critter.make_friendly();
                }
            }
        } else if( critter.has_flag( MF_ELECTRIC_FIELD ) ) {
            if( sight && !critter.has_effect( effect_emp ) ) {
                add_msg( m_good, _( "The %s's electrical field momentarily goes out!" ), critter.name().c_str() );
                critter.add_effect( effect_emp, 3_minutes );
            } else if( sight && critter.has_effect( effect_emp ) ) {
                int dam = dice( 3, 5 );
                add_msg( m_good, _( "The %s's disabled electrical field reverses polarity!" ),
                         critter.name().c_str() );
                add_msg( m_good, _( "It takes %d damage." ), dam );
                critter.add_effect( effect_emp, 1_minutes );
                critter.apply_damage( nullptr, bp_torso, dam );
                critter.check_dead_state();
            }
        } else if( sight ) {
            add_msg( _( "The %s is unaffected by the EMP blast." ), critter.name().c_str() );
        }
    }
    if( u.posx() == x && u.posy() == y ) {
        if( u.power_level > 0 ) {
            add_msg( m_bad, _( "The EMP blast drains your power." ) );
            int max_drain = ( u.power_level > 1000 ? 1000 : u.power_level );
            u.charge_power( -rng( 1 + max_drain / 3, max_drain ) );
        }
        // TODO: More effects?
        //e-handcuffs effects
        if( u.weapon.typeId() == "e_handcuffs" && u.weapon.charges > 0 ) {
            u.weapon.item_tags.erase( "NO_UNWIELD" );
            u.weapon.charges = 0;
            u.weapon.active = false;
            add_msg( m_good, _( "The %s on your wrists spark briefly, then release your hands!" ),
                     u.weapon.tname().c_str() );
        }
    }
    // Drain any items of their battery charge
    for( auto &it : m.i_at( x, y ) ) {
        if( it.is_tool() && it.ammo_type() == ammotype( "battery" ) ) {
            it.charges = 0;
        }
    }
    // TODO: Drain NPC energy reserves
}

template<typename T>
T *game::critter_at( const tripoint &p, bool allow_hallucination )
{
    if( const std::shared_ptr<monster> mon_ptr = critter_tracker->find( p ) ) {
        if( !allow_hallucination && mon_ptr->is_hallucination() ) {
            return nullptr;
        }
        return dynamic_cast<T *>( mon_ptr.get() );
    }
    if( p == u.pos() ) {
        return dynamic_cast<T *>( &u );
    }
    for( auto &cur_npc : active_npc ) {
        if( cur_npc->pos() == p && !cur_npc->is_dead() ) {
            return dynamic_cast<T *>( cur_npc.get() );
        }
    }
    return nullptr;
}

template<typename T>
const T *game::critter_at( const tripoint &p, bool allow_hallucination ) const
{
    return const_cast<game *>( this )->critter_at<T>( p, allow_hallucination );
}

template const monster *game::critter_at<monster>( const tripoint &, bool ) const;
template const npc *game::critter_at<npc>( const tripoint &, bool ) const;
template const player *game::critter_at<player>( const tripoint &, bool ) const;
template const Character *game::critter_at<Character>( const tripoint &, bool ) const;
template Character *game::critter_at<Character>( const tripoint &, bool );
template const Creature *game::critter_at<Creature>( const tripoint &, bool ) const;

template<typename T>
std::shared_ptr<T> game::shared_from( const T &critter )
{
    if( const std::shared_ptr<monster> mon_ptr = critter_tracker->find( critter.pos() ) ) {
        return std::dynamic_pointer_cast<T>( mon_ptr );
    }
    if( static_cast<const Creature *>( &critter ) == static_cast<const Creature *>( &u ) ) {
        // u is not stored in a shared_ptr, but it won't go out of scope anyway
        return std::dynamic_pointer_cast<T>( u_shared_ptr );
    }
    for( auto &cur_npc : active_npc ) {
        if( static_cast<const Creature *>( cur_npc.get() ) == static_cast<const Creature *>( &critter ) ) {
            return std::dynamic_pointer_cast<T>( cur_npc );
        }
    }
    return nullptr;
}

template std::shared_ptr<Creature> game::shared_from<Creature>( const Creature & );
template std::shared_ptr<Character> game::shared_from<Character>( const Character & );
template std::shared_ptr<player> game::shared_from<player>( const player & );
template std::shared_ptr<monster> game::shared_from<monster>( const monster & );
template std::shared_ptr<npc> game::shared_from<npc>( const npc & );

template<typename T>
T *game::critter_by_id( const int id )
{
    if( id == u.getID() ) {
        // player is always alive, therefore no is-dead check
        return dynamic_cast<T *>( &u );
    }
    for( auto &cur_npc : active_npc ) {
        if( cur_npc->getID() == id && !cur_npc->is_dead() ) {
            return dynamic_cast<T *>( cur_npc.get() );
        }
    }
    return nullptr;
}

// monsters don't have ids
template player *game::critter_by_id<player>( int );
template npc *game::critter_by_id<npc>( int );
template Creature *game::critter_by_id<Creature>( int );

monster *game::summon_mon( const mtype_id &id, const tripoint &p )
{
    monster mon( id );
    mon.spawn( p );
    return add_zombie( mon, true ) ? critter_at<monster>( p ) : nullptr;
}

// By default don't pin upgrades to current day
bool game::add_zombie( monster &critter )
{
    return add_zombie( critter, false );
}

bool game::add_zombie( monster &critter, bool pin_upgrade )
{
    if( !m.inbounds( critter.pos() ) ) {
        dbg( D_ERROR ) << "added a critter with out-of-bounds position: "
                       << critter.posx() << "," << critter.posy() << ","  << critter.posz()
                       << " - " << critter.disp_name();
    }

    critter.try_upgrade( pin_upgrade );
    critter.try_reproduce();
    critter.try_biosignature();
    if( !pin_upgrade ) {
        critter.on_load();
    }

    critter.last_updated = calendar::turn;
    critter.last_baby = calendar::turn;
    critter.last_biosig = calendar::turn;
    return critter_tracker->add( critter );
}

size_t game::num_creatures() const
{
    return critter_tracker->size() + active_npc.size() + 1; // 1 == g->u
}

bool game::update_zombie_pos( const monster &critter, const tripoint &pos )
{
    return critter_tracker->update_pos( critter, pos );
}

void game::remove_zombie( const monster &critter )
{
    critter_tracker->remove( critter );
}

void game::clear_zombies()
{
    critter_tracker->clear();
}

/**
 * Attempts to spawn a hallucination at given location.
 * Returns false if the hallucination couldn't be spawned for whatever reason, such as
 * a monster already in the target square.
 * @return Whether or not a hallucination was successfully spawned.
 */
bool game::spawn_hallucination( const tripoint &p )
{
    monster phantasm( MonsterGenerator::generator().get_valid_hallucination() );
    phantasm.hallucination = true;
    phantasm.spawn( p );

    //Don't attempt to place phantasms inside of other creatures
    if( !critter_at( phantasm.pos(), true ) ) {
        return critter_tracker->add( phantasm );
    } else {
        return false;
    }
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
    if( critter_at( a.pos() ) != &a ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  b.disp_name().c_str(), critter_at( a.pos() )->disp_name().c_str() );
        return false;
    }
    if( critter_at( b.pos() ) != &b ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  a.disp_name().c_str(), critter_at( b.pos() )->disp_name().c_str() );
        return false;
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
        monster *m1 = dynamic_cast< monster * >( &first );
        monster *m2 = dynamic_cast< monster * >( &second );
        if( m1 == nullptr || m2 == nullptr || m1 == m2 ) {
            debugmsg( "Couldn't swap two monsters" );
            return false;
        }

        critter_tracker->swap_positions( *m1, *m2 );
        return true;
    }

    player *u_or_npc = dynamic_cast< player * >( &first );
    player *other_npc = dynamic_cast< player * >( &second );

    if( u_or_npc->in_vehicle ) {
        g->m.unboard_vehicle( u_or_npc->pos() );
    }

    if( other_npc->in_vehicle ) {
        g->m.unboard_vehicle( other_npc->pos() );
    }

    tripoint temp = second.pos();
    second.setpos( first.pos() );
    first.setpos( temp );

    if( g->m.veh_at( u_or_npc->pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        g->m.board_vehicle( u_or_npc->pos(), u_or_npc );
    }

    if( g->m.veh_at( other_npc->pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
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
    return ( m.is_outside( p ) && light_level( p.z ) >= 40 &&
             ( weather == WEATHER_CLEAR || weather == WEATHER_SUNNY ) );
}

bool game::is_sheltered( const tripoint &p )
{
    const optional_vpart_position vp = m.veh_at( p );

    return ( !m.is_outside( p ) ||
             p.z < 0 ||
             ( vp && vp->is_inside() ) );
}

bool game::revive_corpse( const tripoint &p, item &it )
{
    if( !it.is_corpse() ) {
        debugmsg( "Tried to revive a non-corpse." );
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
    if( it.has_flag( "FIELD_DRESS" ) || it.has_flag( "FIELD_DRESS_FAILED" ) ||
        it.has_flag( "QUARTERED" ) ) {
        // Failed reanimation due to corpse being butchered
        it.active = false;
        return false;
    }

    critter.no_extra_death_drops = true;
    critter.add_effect( effect_downed, 5_turns, num_bp, true );

    if( it.get_var( "zlave" ) == "zlave" ) {
        critter.add_effect( effect_pacified, 1_turns, num_bp, true );
        critter.add_effect( effect_pet, 1_turns, num_bp, true );
    }

    if( it.get_var( "no_ammo" ) == "no_ammo" ) {
        for( auto &ammo : critter.ammo ) {
            ammo.second = 0;
        }
    }

    const bool ret = add_zombie( critter );
    if( !ret ) {
        debugmsg( "Couldn't add a revived monster" );
    }
    if( ret && !critter_at<monster>( p ) ) {
        debugmsg( "Revived monster is not where it's supposed to be. Prepare for crash." );
    }
    return ret;
}

static void make_active( item_location loc )
{
    switch( loc.where() ) {
        case item_location::type::map:
            g->m.make_active( loc );
            break;
        case item_location::type::vehicle:
            g->m.veh_at( loc.position() )->vehicle().make_active( loc );
            break;
        default:
            break;
    }
}

static void update_lum( item_location loc, bool add )
{
    switch( loc.where() ) {
        case item_location::type::map:
            g->m.update_lum( loc, add );
            break;
        default:
            break;
    }
}

void game::use_item( int pos )
{
    bool use_loc = false;
    item_location loc;

    if( pos == INT_MIN ) {
        loc = game_menus::inv::use( u );

        if( !loc ) {
            add_msg( _( "Never mind." ) );
            return;
        }

        const item &it = *loc.get_item();
        if( it.has_flag( "ALLOWS_REMOTE_USE" ) ) {
            use_loc = true;
        } else {
            int obtain_cost = loc.obtain_cost( u );
            pos = loc.obtain( u );
            // This method only handles items in te inventory, so refund the obtain cost.
            u.moves += obtain_cost;
        }
    }

    refresh_all();

    if( use_loc ) {
        update_lum( loc.clone(), false );
        u.use( loc.clone() );
        update_lum( loc.clone(), true );

        make_active( loc.clone() );
    } else {
        u.use( pos );
    }

    u.invalidate_crafting_inventory();
}

void game::exam_vehicle( vehicle &veh, int cx, int cy )
{
    auto act = veh_interact::run( veh, cx, cy );
    if( act ) {
        u.moves = 0;
        u.assign_activity( act );
    }
}

bool game::forced_door_closing( const tripoint &p, const ter_id &door_type, int bash_dmg )
{
    // TODO: Z
    const int &x = p.x;
    const int &y = p.y;
    const std::string &door_name = door_type.obj().name();
    int kbx = x; // Used when player/monsters are knocked back
    int kby = y; // and when moving items out of the way
    for( int i = 0; i < 20; i++ ) {
        const int x_ = x + rng( -1, +1 );
        const int y_ = y + rng( -1, +1 );
        if( is_empty( {x_, y_, get_levz()} ) ) {
            // invert direction, as game::knockback needs
            // the source of the force that knocks back
            kbx = -x_ + x + x;
            kby = -y_ + y + y;
            break;
        }
    }
    const tripoint kbp( kbx, kby, p.z );
    const bool can_see = u.sees( tripoint( x, y, p.z ) );
    player *npc_or_player = critter_at<player>( tripoint( x, y, p.z ) );
    if( npc_or_player != nullptr ) {
        if( bash_dmg <= 0 ) {
            return false;
        }
        if( npc_or_player->is_npc() && can_see ) {
            add_msg( _( "The %1$s hits the %2$s." ), door_name.c_str(), npc_or_player->name.c_str() );
        } else if( npc_or_player->is_player() ) {
            add_msg( m_bad, _( "The %s hits you." ), door_name.c_str() );
        }
        if( npc_or_player->activity ) {
            npc_or_player->cancel_activity();
        }
        // TODO: make the npc angry?
        npc_or_player->hitall( bash_dmg, 0, nullptr );
        knockback( kbp, p, std::max( 1, bash_dmg / 10 ), -1, 1 );
        // TODO: perhaps damage/destroy the gate
        // if the npc was really big?
    }
    if( monster *const mon_ptr = critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( bash_dmg <= 0 ) {
            return false;
        }
        if( can_see ) {
            add_msg( _( "The %1$s hits the %2$s." ), door_name.c_str(), critter.name().c_str() );
        }
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
            knockback( kbp, p, std::max( 1, bash_dmg / 10 ), -1, 1 );
            if( critter_at( p ) ) {
                return false;
            }
        }
    }
    if( const optional_vpart_position vp = m.veh_at( p ) ) {
        if( bash_dmg <= 0 ) {
            return false;
        }
        vp->vehicle().damage( vp->part_index(), bash_dmg );
        if( m.veh_at( p ) ) {
            // Check again in case all parts at the door tile
            // have been destroyed, if there is still a vehicle
            // there, the door can not be closed
            return false;
        }
    }
    if( bash_dmg < 0 && !m.i_at( x, y ).empty() ) {
        return false;
    }
    if( bash_dmg == 0 ) {
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
    if( m.has_flag( "NOITEM", x, y ) ) {
        auto items = m.i_at( x, y );
        while( !items.empty() ) {
            if( items[0].made_of( LIQUID ) ) {
                m.i_rem( x, y, 0 );
                continue;
            }
            if( items[0].made_of( material_id( "glass" ) ) && one_in( 2 ) ) {
                if( can_see ) {
                    add_msg( m_warning, _( "A %s shatters!" ), items[0].tname().c_str() );
                } else {
                    add_msg( m_warning, _( "Something shatters!" ) );
                }
                m.i_rem( x, y, 0 );
                continue;
            }
            m.add_item_or_charges( kbx, kby, items[0] );
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
    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( !vp ) {
        debugmsg( "Tried to exit non-existent vehicle." );
        return;
    }
    vehicle *const veh = &vp->vehicle();
    if( u.pos() == dest_loc ) {
        debugmsg( "Need somewhere to dismount towards." );
        return;
    }
    tileray ray( dest_loc.x - u.posx(), dest_loc.y - u.posy() );
    const int d = ray.dir(); // TODO:: make dir() const correct!
    add_msg( _( "You dive from the %s." ), veh->name.c_str() );
    m.unboard_vehicle( u.pos() );
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_creature( &u, d, 30, true );
    // Hit the ground according to vehicle speed
    if( !m.has_flag( "SWIMMABLE", u.pos() ) ) {
        if( veh->velocity > 0 ) {
            fling_creature( &u, veh->face.dir(), veh->velocity / static_cast<float>( 100 ) );
        } else {
            fling_creature( &u, veh->face.dir() + 180, -( veh->velocity ) / static_cast<float>( 100 ) );
        }
    }
}

void game::control_vehicle()
{
    int veh_part = -1;
    vehicle *veh = remoteveh();
    if( veh == nullptr ) {
        if( const optional_vpart_position vp = m.veh_at( u.pos() ) ) {
            veh = &vp->vehicle();
            veh_part = vp->part_index();
        }
    }

    if( veh != nullptr && veh->player_in_control( u ) ) {
        veh->use_controls( u.pos() );
    } else if( veh && veh->avail_part_with_feature( veh_part, "CONTROLS", true ) >= 0 &&
               u.in_vehicle ) {
        if( !veh->interact_vehicle_locked() ) {
            return;
        }
        if( veh->engine_on ) {
            u.controlling_vehicle = true;
            add_msg( _( "You take control of the %s." ), veh->name.c_str() );
        } else {
            veh->start_engines( true );
        }
    } else {
        const cata::optional<tripoint> examp_ = choose_adjacent( _( "Control vehicle where?" ) );
        if( !examp_ ) {
            return;
        }
        const optional_vpart_position vp = m.veh_at( *examp_ );
        if( !vp ) {
            add_msg( _( "No vehicle there." ) );
            return;
        }
        veh = &vp->vehicle();
        veh_part = vp->part_index();
        if( veh->avail_part_with_feature( veh_part, "CONTROLS", true ) >= 0 ) {
            veh->use_controls( *examp_ );
        }
    }
    if( veh ) {
        // If we reached here, we gained control of a vehicle.
        // Clear the map memory for the area covered by the vehicle to eliminate ghost vehicles.
        for( const tripoint &target : veh->get_points() ) {
            u.clear_memorized_tile( m.getabs( target ) );
        }
    }
}

bool pet_menu( monster *z )
{
    enum choices {
        swap_pos = 0,
        push_zlave,
        rename,
        attach_bag,
        drop_all,
        give_items,
        play_with_pet,
        pheromone,
        milk,
        rope
    };

    uilist amenu;
    std::string pet_name = z->get_name();
    if( z->type->in_species( ZOMBIE ) ) {
        pet_name = _( "zombie slave" );
    }

    amenu.text = string_format( _( "What to do with your %s?" ), pet_name.c_str() );

    amenu.addentry( swap_pos, true, 's', _( "Swap positions" ) );
    amenu.addentry( push_zlave, true, 'p', _( "Push %s" ), pet_name.c_str() );
    amenu.addentry( rename, true, 'e', _( "Rename" ) );

    if( z->has_effect( effect_has_bag ) ) {
        amenu.addentry( give_items, true, 'g', _( "Place items into bag" ) );
        amenu.addentry( drop_all, true, 'd', _( "Drop all items" ) );
    } else {
        amenu.addentry( attach_bag, true, 'b', _( "Attach bag" ) );
    }

    if( z->has_flag( MF_BIRDFOOD ) || z->has_flag( MF_CATFOOD ) || z->has_flag( MF_DOGFOOD ) ) {
        amenu.addentry( play_with_pet, true, 'y', _( "Play with %s" ), pet_name.c_str() );
    }

    if( z->has_effect( effect_tied ) ) {
        amenu.addentry( rope, true, 'r', _( "Untie" ) );
    } else {
        if( g->u.has_amount( "rope_6", 1 ) ) {
            amenu.addentry( rope, true, 'r', _( "Tie" ) );
        } else {
            amenu.addentry( rope, false, 'r', _( "You need a short rope" ) );
        }
    }

    if( z->type->in_species( ZOMBIE ) ) {
        amenu.addentry( pheromone, true, 't', _( "Tear out pheromone ball" ) );
    }

    if( z->has_flag( MF_MILKABLE ) ) {
        amenu.addentry( milk, true, 'm', _( "Milk %s" ), pet_name.c_str() );
    }

    amenu.query();
    int choice = amenu.ret;

    if( swap_pos == choice ) {
        g->u.moves -= 150;

        ///\EFFECT_STR increases chance to successfully swap positions with your pet

        ///\EFFECT_DEX increases chance to successfully swap positions with your pet
        if( !one_in( ( g->u.str_cur + g->u.dex_cur ) / 6 ) ) {

            bool t = z->has_effect( effect_tied );
            if( t ) {
                z->remove_effect( effect_tied );
            }

            tripoint zp = z->pos();
            z->move_to( g->u.pos(), true );
            g->u.setpos( zp );

            if( t ) {
                z->add_effect( effect_tied, 1_turns, num_bp, true );
            }

            add_msg( _( "You swap positions with your %s." ), pet_name.c_str() );

            return true;
        } else {
            add_msg( _( "You fail to budge your %s!" ), pet_name.c_str() );

            return true;
        }
    }

    if( push_zlave == choice ) {

        g->u.moves -= 30;

        ///\EFFECT_STR increases chance to successfully push your pet
        if( !one_in( g->u.str_cur ) ) {
            add_msg( _( "You pushed the %s." ), pet_name.c_str() );
        } else {
            add_msg( _( "You pushed the %s, but it resisted." ), pet_name.c_str() );
            return true;
        }

        int deltax = z->posx() - g->u.posx(), deltay = z->posy() - g->u.posy();

        z->move_to( tripoint( z->posx() + deltax, z->posy() + deltay, z->posz() ) );

        return true;
    }

    if( rename == choice ) {
        std::string unique_name = string_input_popup()
                                  .title( _( "Enter new pet name:" ) )
                                  .width( 20 )
                                  .query_string();
        if( unique_name.length() > 0 ) {
            z->unique_name = unique_name;
        }
        return true;
    }

    if( attach_bag == choice ) {
        int pos = g->inv_for_filter( _( "Bag item" ), []( const item & it ) {
            return it.is_armor() && it.get_storage() > 0_ml;
        } );

        if( pos == INT_MIN ) {
            add_msg( _( "Never mind." ) );
            return true;
        }

        item &it = g->u.i_at( pos );

        z->add_item( it );

        add_msg( _( "You mount the %1$s on your %2$s, ready to store gear." ),
                 it.display_name().c_str(), pet_name.c_str() );

        g->u.i_rem( pos );

        z->add_effect( effect_has_bag, 1_turns, num_bp, true );

        g->u.moves -= 200;

        return true;
    }

    if( drop_all == choice ) {
        for( auto &it : z->inv ) {
            g->m.add_item_or_charges( z->pos(), it );
        }

        z->inv.clear();

        z->remove_effect( effect_has_bag );

        add_msg( _( "You dump the contents of the %s's bag on the ground." ), pet_name.c_str() );

        g->u.moves -= 200;
        return true;
    }

    if( give_items == choice ) {

        if( z->inv.empty() ) {
            add_msg( _( "There is no container on your %s to put things in!" ), pet_name.c_str() );
            return true;
        }

        item &it = z->inv[0];

        if( !it.is_armor() ) {
            add_msg( _( "There is no container on your %s to put things in!" ), pet_name.c_str() );
            return true;
        }

        units::volume max_cap = it.get_storage();
        units::mass max_weight = z->weight_capacity() - it.weight();

        if( z->inv.size() > 1 ) {
            for( auto &i : z->inv ) {
                max_cap -= i.volume();
                max_weight -= i.weight();
            }
        }

        if( max_weight <= 0_gram ) {
            add_msg( _( "%1$s is overburdened. You can't transfer your %2$s." ),
                     pet_name.c_str(), it.tname( 1 ).c_str() );
            return true;
        }
        if( max_cap <= 0_ml ) {
            add_msg( _( "There's no room in your %1$s's %2$s for that, it's too bulky!" ),
                     pet_name.c_str(), it.tname( 1 ).c_str() );
            return true;
        }

        const auto items_to_stash = game_menus::inv::multidrop( g->u );
        if( !items_to_stash.empty() ) {
            g->u.drop( items_to_stash, z->pos(), true );
            z->add_effect( effect_controlled, 5_turns );
            return true;
        }

        return false;
    }

    if( play_with_pet == choice &&
        query_yn( _( "Spend a few minutes to play with your %s?" ), pet_name.c_str() ) ) {
        g->u.assign_activity( activity_id( "ACT_PLAY_WITH_PET" ), rng( 50, 125 ) * 100 );
        g->u.activity.str_values.push_back( pet_name );

    }

    if( pheromone == choice && query_yn( _( "Really kill the zombie slave?" ) ) ) {

        z->apply_damage( &g->u, bp_torso, 100 ); // damage the monster (and its corpse)
        z->die( &g->u ); // and make sure it's really dead

        g->u.moves -= 150;

        if( !one_in( 3 ) ) {
            g->u.add_msg_if_player( _( "You tear out the pheromone ball from the zombie slave." ) );

            item ball( "pheromone", 0 );
            iuse pheromone;
            pheromone.pheromone( &( g->u ), &ball, true, g->u.pos() );
        }

    }

    if( rope == choice ) {
        if( z->has_effect( effect_tied ) ) {
            z->remove_effect( effect_tied );
            item rope_6( "rope_6", 0 );
            g->u.i_add( rope_6 );
        } else {
            z->add_effect( effect_tied, 1_turns, num_bp, true );
            g->u.use_amount( "rope_6", 1 );
        }

        return true;
    }

    if( milk == choice ) {
        // pin the cow in place if it isn't already
        bool temp_tie = !z->has_effect( effect_tied );
        if( temp_tie ) {
            z->add_effect( effect_tied, 1_turns, num_bp, true );
        }

        monexamine::milk_source( *z );

        if( temp_tie ) {
            z->remove_effect( effect_tied );
        }

        return true;
    }

    return true;
}

bool game::npc_menu( npc &who )
{
    enum choices : int {
        talk = 0,
        swap_pos,
        push,
        examine_wounds,
        use_item,
        sort_armor,
        attack,
        disarm,
        steal
    };

    const bool obeys = debug_mode || ( who.is_friend() && !who.in_sleep_state() );

    uilist amenu;

    amenu.text = string_format( _( "What to do with %s?" ), who.disp_name().c_str() );
    amenu.addentry( talk, true, 't', _( "Talk" ) );
    amenu.addentry( swap_pos, obeys, 's', _( "Swap positions" ) );
    amenu.addentry( push, obeys, 'p', _( "Push away" ) );
    amenu.addentry( examine_wounds, true, 'w', _( "Examine wounds" ) );
    amenu.addentry( use_item, true, 'i', _( "Use item on" ) );
    amenu.addentry( sort_armor, true, 'r', _( "Sort armor" ) );
    amenu.addentry( attack, true, 'a', _( "Attack" ) );
    if( !who.is_friend() ) {
        amenu.addentry( disarm, who.is_armed(), 'd', _( "Disarm" ) );
        amenu.addentry( steal, !who.is_enemy(), 'S', _( "Steal" ) );
    }

    amenu.query();

    const int choice = amenu.ret;
    if( choice == talk ) {
        who.talk_to_u();
    } else if( choice == swap_pos ) {
        if( !prompt_dangerous_tile( who.pos() ) ) {
            return true;
        }
        // TODO: Make NPCs protest when displaced onto dangerous crap
        add_msg( _( "You swap places with %s." ), who.name.c_str() );
        swap_critters( u, who );
        // TODO: Make that depend on stuff
        u.mod_moves( -200 );
    } else if( choice == push ) {
        // TODO: Make NPCs protest when displaced onto dangerous crap
        tripoint oldpos = who.pos();
        who.move_away_from( u.pos(), true );
        u.mod_moves( -20 );
        if( oldpos != who.pos() ) {
            add_msg( _( "%s moves out of the way." ), who.name.c_str() );
        } else {
            add_msg( m_warning, _( "%s has nowhere to go!" ), who.name.c_str() );
        }
    } else if( choice == examine_wounds ) {
        ///\EFFECT_PER slightly increases precision when examining NPCs' wounds

        ///\EFFECT_FIRSTAID increases precision when examining NPCs' wounds
        const bool precise = u.get_skill_level( skill_firstaid ) * 4 + u.per_cur >= 20;
        who.body_window( _( "Limbs of: " ) + who.disp_name(), true, precise, 0, 0, 0, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f );
    } else if( choice == use_item ) {
        static const std::string heal_string( "heal" );
        const auto will_accept = []( const item & it ) {
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
        const int pos = inv_for_filter( _( "Use which item?" ), will_accept );

        if( pos == INT_MIN ) {
            add_msg( _( "Never mind" ) );
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
        if( who.is_enemy() || query_yn( _( "You may be attacked! Proceed?" ) ) ) {
            u.melee_attack( who, true );
            who.on_attacked( u );
        }
    } else if( choice == disarm ) {
        if( who.is_enemy() || query_yn( _( "You may be attacked! Proceed?" ) ) ) {
            u.disarm( who );
        }
    } else if( choice == steal && query_yn( _( "You may be attacked! Proceed?" ) ) ) {
        u.steal( who );
    }

    return true;
}

void game::examine()
{
    // if we are driving a vehicle, examine the
    // current tile without asking.
    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( vp && vp->vehicle().player_in_control( u ) ) {
        examine( u.pos() );
        return;
    }

    const cata::optional<tripoint> examp_ = choose_adjacent_highlight( _( "Examine where?" ),
                                            ACTION_EXAMINE );
    if( !examp_ ) {
        return;
    }
    // redraw terrain to erase 'examine' window
    draw_ter();
    wrefresh( w_terrain );

    examine( *examp_ );
}

const std::string get_fire_fuel_string( const tripoint &examp )
{
    if( g->m.has_flag( TFLAG_FIRE_CONTAINER, examp ) ) {
        field_entry *fire = g->m.get_field( examp, fd_fire );
        if( fire ) {
            std::stringstream ss;
            ss << string_format( _( "There is a fire here." ) ) << " ";
            if( fire->getFieldDensity() > 1 ) {
                ss << string_format( _( "It's too big and unpredictable to evaluate how long it will last." ) );
                return ss.str();
            }
            time_duration fire_age = fire->getFieldAge();
            // half-life inclusion
            int mod = 5 - g->u.get_skill_level( skill_survival );
            mod = std::max( mod, 0 );
            if( fire_age >= 0_turns ) {
                if( mod >= 4 ) { // = survival level 0-1
                    ss << string_format( _( "It's going to go out soon without extra fuel." ) );
                    return ss.str();
                } else {
                    fire_age = 30_minutes - fire_age;
                    fire_age = rng( fire_age - fire_age * mod / 5, fire_age + fire_age * mod / 5 );
                    ss << string_format(
                           _( "Without extra fuel it might burn yet for %s, but might also go out sooner." ),
                           to_string_approx( fire_age ) );
                    return ss.str();
                }
            } else {
                fire_age = fire_age * -1 + 30_minutes;
                if( mod >= 4 ) { // = survival level 0-1
                    if( fire_age <= 1_hours ) {
                        ss << string_format(
                               _( "It's quite decent and looks like it'll burn for a bit without extra fuel." ) );
                        return ss.str();
                    } else if( fire_age <= 3_hours ) {
                        ss << string_format( _( "It looks solid, and will burn for a few hours without extra fuel." ) );
                        return ss.str();
                    } else {
                        ss << string_format(
                               _( "It's very well supplied and even without extra fuel might burn for at least a part of a day." ) );
                        return ss.str();
                    }
                } else {
                    fire_age = rng( fire_age - fire_age * mod / 5, fire_age + fire_age * mod / 5 );
                    ss << string_format( _( "Without extra fuel it will burn for %s." ), to_string_approx( fire_age ) );
                    return ss.str();
                }
            }
        }
    }
    static const std::string empty_string;
    return empty_string;
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

        npc *np = dynamic_cast<npc *>( c );
        if( np != nullptr ) {
            if( npc_menu( *np ) ) {
                return;
            }
        }
    }

    const optional_vpart_position vp = m.veh_at( examp );
    if( vp ) {
        if( u.controlling_vehicle ) {
            add_msg( m_info, _( "You can't do that while driving." ) );
        } else if( abs( vp->vehicle().velocity ) > 0 ) {
            add_msg( m_info, _( "You can't do that on a moving vehicle." ) );
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
        wrefresh( w_terrain );
    }

    // In case of teleport trap or somesuch
    if( player_pos != u.pos() ) {
        return;
    }

    // Feedback for fire lasting time
    const std::string fire_fuel = get_fire_fuel_string( examp );
    if( !fire_fuel.empty() ) {
        add_msg( fire_fuel );
    }

    if( m.has_flag( "SEALED", examp ) ) {
        if( none ) {
            if( m.has_flag( "UNSTABLE", examp ) ) {
                add_msg( _( "The %s is too unstable to remove anything." ), m.name( examp ).c_str() );
            } else {
                add_msg( _( "The %s is firmly sealed." ), m.name( examp ).c_str() );
            }
        }
    } else {
        //examp has no traps, is a container and doesn't have a special examination function
        if( m.tr_at( examp ).is_null() && m.i_at( examp ).empty() &&
            m.has_flag( "CONTAINER", examp ) && none ) {
            add_msg( _( "It is empty." ) );
        } else {
            draw_sidebar_messages();
            sounds::process_sound_markers( &u );
            Pickup::pick_up( examp, 0 );
        }
    }
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carefully peeking around a corner, hence the large move cost.
void game::peek()
{
    const cata::optional<tripoint> p = choose_direction( _( "Peek where?" ), true );
    if( !p ) {
        refresh_all();
        return;
    }

    if( p->z != 0 ) {
        const tripoint old_pos = u.pos();
        vertical_move( p->z, false );

        if( old_pos != u.pos() ) {
            look_around();
            vertical_move( p->z * -1, false );
            draw_ter();
        }
        wrefresh( w_terrain );
        return;
    }

    if( m.impassable( u.pos() + *p ) ) {
        return;
    }

    peek( u.pos() + *p );
}

void game::peek( const tripoint &p )
{
    u.moves -= 200;
    tripoint prev = u.pos();
    u.setpos( p );
    tripoint center = p;
    const look_around_result result = look_around( catacurses::window(), center, center, false, false,
                                      true );
    u.setpos( prev );

    if( result.peek_action && *result.peek_action == PA_BLIND_THROW ) {
        plthrow( INT_MIN, p );
    }

    draw_ter();
    wrefresh( w_terrain );
}
////////////////////////////////////////////////////////////////////////////////////////////
cata::optional<tripoint> game::look_debug()
{
    editmap edit;
    return edit.edit();
}
////////////////////////////////////////////////////////////////////////////////////////////

void game::print_all_tile_info( const tripoint &lp, const catacurses::window &w_look,
                                const std::string &area_name, int column,
                                int &line,
                                const int last_line, bool draw_terrain_indicators,
                                const visibility_variables &cache )
{
    visibility_type visibility = VIS_HIDDEN;
    const bool inbounds = m.inbounds( lp );
    if( inbounds ) {
        visibility = m.get_visibility( m.apparent_light_at( lp, cache ), cache );
    }
    switch( visibility ) {
        case VIS_CLEAR: {
            const optional_vpart_position vp = m.veh_at( lp );
            const Creature *creature = critter_at( lp, true );
            print_terrain_info( lp, w_look, area_name, column, line );
            print_fields_info( lp, w_look, column, line );
            print_trap_info( lp, w_look, column, line );
            print_creature_info( creature, w_look, column, line );
            print_vehicle_info( veh_pointer_or_null( vp ), vp ? vp->part_index() : -1, w_look, column, line,
                                last_line );
            print_items_info( lp, w_look, column, line, last_line );
            print_graffiti_info( lp, w_look, column, line, last_line );

            if( draw_terrain_indicators ) {
                if( creature != nullptr && u.sees( *creature ) ) {
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

            if( draw_terrain_indicators ) {
                print_visibility_indicator( visibility );
            }
            break;
    }
    if( !inbounds ) {
        return;
    }
    auto this_sound = sounds::sound_at( lp );
    if( !this_sound.empty() ) {
        mvwprintw( w_look, ++line, 1, _( "You heard %s from here." ), this_sound.c_str() );
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
                           _( "You heard %s from above." ) : _( "You heard %s from below." ),
                           zlev_sound.c_str() );
            }
        }
    }
}

void game::print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                  visibility_type visibility )
{
    const char *visibility_message = nullptr;
    switch( visibility ) {
        case VIS_CLEAR:
            visibility_message = _( "Clearly visible." );
            break;
        case VIS_BOOMER:
            visibility_message = _( "A bright pink blur." );
            break;
        case VIS_BOOMER_DARK:
            visibility_message = _( "A pink blur." );
            break;
        case VIS_DARK:
            visibility_message = _( "Darkness." );
            break;
        case VIS_LIT:
            visibility_message = _( "Bright light." );
            break;
        case VIS_HIDDEN:
            visibility_message = _( "Unseen." );
            break;
    }

    mvwprintw( w_look, column, line, visibility_message );
    line += 2;
}

void game::print_terrain_info( const tripoint &lp, const catacurses::window &w_look,
                               const std::string &area_name, int column,
                               int &line )
{
    const int max_width = getmaxx( w_look ) - column - 1;
    int lines;
    std::string tile = m.tername( lp );
    tile = "(" + area_name + ") " + tile;
    if( m.has_furn( lp ) ) {
        tile += "; " + m.furnname( lp );
    }

    if( m.impassable( lp ) ) {
        lines = fold_and_print( w_look, line, column, max_width, c_light_gray, _( "%s; Impassable" ),
                                tile.c_str() );
    } else {
        lines = fold_and_print( w_look, line, column, max_width, c_light_gray, _( "%s; Movement cost %d" ),
                                tile.c_str(), m.move_cost( lp ) * 50 );

        const auto ll = get_light_level( std::max( 1.0,
                                         LIGHT_AMBIENT_LIT - m.ambient_light_at( lp ) + 1.0 ) );
        mvwprintw( w_look, ++lines, column, _( "Lighting: " ) );
        wprintz( w_look, ll.second, ll.first.c_str() );
    }

    std::string signage = m.get_signage( lp );
    if( !signage.empty() ) {
        trim_and_print( w_look, ++lines, column, max_width, c_dark_gray,
                        u.has_trait( trait_ILLITERATE ) ? _( "Sign: ???" ) : _( "Sign: %s" ), signage.c_str() );
    }

    if( m.has_zlevels() && lp.z > -OVERMAP_DEPTH && !m.has_floor( lp ) ) {
        // Print info about stuff below
        tripoint below( lp.x, lp.y, lp.z - 1 );
        std::string tile_below = m.tername( below );
        if( m.has_furn( below ) ) {
            tile_below += "; " + m.furnname( below );
        }

        if( !m.has_floor_or_support( lp ) ) {
            fold_and_print( w_look, ++lines, column, max_width, c_dark_gray, _( "Below: %s; No support" ),
                            tile_below.c_str() );
        } else {
            fold_and_print( w_look, ++lines, column, max_width, c_dark_gray, _( "Below: %s; Walkable" ),
                            tile_below.c_str() );
        }
    }

    int map_features = fold_and_print( w_look, ++lines, column, max_width, c_dark_gray,
                                       m.features( lp ) );
    if( line < lines ) {
        line = lines + map_features - 1;
    }
}

void game::print_fields_info( const tripoint &lp, const catacurses::window &w_look, int column,
                              int &line )
{
    const field &tmpfield = m.field_at( lp );
    for( auto &fld : tmpfield ) {
        const field_entry &cur = fld.second;
        if( fld.first == fd_fire && ( m.has_flag( TFLAG_FIRE_CONTAINER, lp ) ||
                                      m.ter( lp ) == t_pit_shallow || m.ter( lp ) == t_pit ) ) {
            const int max_width = getmaxx( w_look ) - column - 2;
            int lines = fold_and_print( w_look, ++line, column, max_width, cur.color(),
                                        get_fire_fuel_string( lp ) ) - 1;
            line += lines;
        } else {
            mvwprintz( w_look, ++line, column, cur.color(), cur.name() );
        }
    }
}

void game::print_trap_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                            int &line )
{
    const trap &tr = m.tr_at( lp );
    if( tr.can_see( lp, u ) ) {
        mvwprintz( w_look, ++line, column, tr.color, tr.name() );
    }
}

void game::print_creature_info( const Creature *creature, const catacurses::window &w_look,
                                const int column, int &line )
{
    if( creature != nullptr && ( u.sees( *creature ) || creature == &u ) ) {
        line = creature->print_info( w_look, ++line, 6, column );
    }
}

void game::print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                               const int column, int &line, const int last_line )
{
    if( veh ) {
        mvwprintw( w_look, ++line, column, _( "There is a %s there. Parts:" ), veh->name.c_str() );
        line = veh->print_part_list( w_look, ++line, last_line, getmaxx( w_look ), veh_part );
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
            visibility_indicator_color = c_dark_gray;
            break;
        case VIS_LIT:
            visibility_indicator = '#';
            visibility_indicator_color = c_light_gray;
            break;
        case VIS_HIDDEN:
            visibility_indicator = 'x';
            visibility_indicator_color = c_white;
            break;
    }

    mvwputch( w_terrain, POSY, POSX, visibility_indicator_color, visibility_indicator );
}

void game::print_items_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                             int &line,
                             const int last_line )
{
    if( !m.sees_some_items( lp, u ) ) {
        return;
    } else if( m.has_flag( "CONTAINER", lp ) && !m.could_see_items( lp, u ) ) {
        mvwprintw( w_look, ++line, column, _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( "BLIND" ) ) {
        mvwprintz( w_look, ++line, column, c_yellow,
                   _( "There's something there, but you can't see what it is." ) );
        return;
    } else {
        std::map<std::string, int> item_names;
        for( auto &item : m.i_at( lp ) ) {
            ++item_names[item.tname()];
        }

        const int max_width = getmaxx( w_look ) - column - 1;
        for( const auto &it : item_names ) {
            if( line >= last_line - 2 ) {
                mvwprintz( w_look, ++line, column, c_yellow, _( "More items here..." ) );
                break;
            }

            if( it.second > 1 ) {
                trim_and_print( w_look, ++line, column, max_width, c_white,
                                pgettext( "%s is the name of the item. %d is the quantity of that item.", "%s [%d]" ),
                                it.first.c_str(), it.second );
            } else {
                trim_and_print( w_look, ++line, column, max_width, c_white, it.first );
            }
        }
    }
}

void game::print_graffiti_info( const tripoint &lp, const catacurses::window &w_look,
                                const int column, int &line,
                                const int last_line )
{
    if( line > last_line ) {
        return;
    }

    const int max_width = getmaxx( w_look ) - column - 2;
    if( m.has_graffiti_at( lp ) ) {
        fold_and_print( w_look, ++line, column, max_width, c_light_gray, _( "Graffiti: %s" ),
                        m.graffiti_at( lp ).c_str() );
    }
}

void game::get_lookaround_dimensions( int &lookWidth, int &begin_y, int &begin_x ) const
{
    lookWidth = getmaxx( w_messages );
    begin_y = TERMY - getmaxy( w_messages ) + 1;
    if( getbegy( w_messages ) < begin_y ) {
        begin_y = getbegy( w_messages );
    }
    begin_x = getbegx( w_messages );
}

bool game::check_zone( const zone_type_id &type, const tripoint &where ) const
{
    return zone_manager::get_manager().has( type, m.getabs( where ) );
}

bool game::check_near_zone( const zone_type_id &type, const tripoint &where ) const
{
    return zone_manager::get_manager().has_near( type, m.getabs( where ) );
}

bool game::is_zones_manager_open() const
{
    return zones_manager_open;
}

static void zones_manager_shortcuts( const catacurses::window &w_info )
{
    werase( w_info );

    int tmpx = 1;
    tmpx += shortcut_print( w_info, 1, tmpx, c_white, c_light_green, _( "<A>dd" ) ) + 2;
    tmpx += shortcut_print( w_info, 1, tmpx, c_white, c_light_green, _( "<R>emove" ) ) + 2;
    tmpx += shortcut_print( w_info, 1, tmpx, c_white, c_light_green, _( "<E>nable" ) ) + 2;
    tmpx += shortcut_print( w_info, 1, tmpx, c_white, c_light_green, _( "<D>isable" ) ) + 2;

    tmpx = 1;
    tmpx += shortcut_print( w_info, 2, tmpx, c_white, c_light_green, _( "<+-> Move up/down" ) ) + 2;
    tmpx += shortcut_print( w_info, 2, tmpx, c_white, c_light_green, _( "<Enter>-Edit" ) ) + 2;

    tmpx = 1;
    tmpx += shortcut_print( w_info, 3, tmpx, c_white, c_light_green,
                            _( "<S>how all / hide distant" ) ) + 2;
    tmpx += shortcut_print( w_info, 3, tmpx, c_white, c_light_green, _( "<M>ap" ) ) + 2;

    wrefresh( w_info );
}

static void zones_manager_draw_borders( const catacurses::window &w_border,
                                        const catacurses::window &w_info_border,
                                        const int iInfoHeight, const int width )
{
    for( int i = 1; i < TERMX; ++i ) {
        if( i < width ) {
            mvwputch( w_border, 0, i, c_light_gray, LINE_OXOX ); // -
            mvwputch( w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, c_light_gray, LINE_OXOX ); // -
        }

        if( i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2 ) {
            mvwputch( w_border, i, 0, c_light_gray, LINE_XOXO ); // |
            mvwputch( w_border, i, width - 1, c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( w_border, 0, 0, c_light_gray, LINE_OXXO ); // |^
    mvwputch( w_border, 0, width - 1, c_light_gray, LINE_OOXX ); // ^|

    mvwputch( w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, c_light_gray,
              LINE_XXXO ); // |-
    mvwputch( w_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( w_border, 0, 2, c_white, _( "Zones manager" ) );
    wrefresh( w_border );

    for( int j = 0; j < iInfoHeight - 1; ++j ) {
        mvwputch( w_info_border, j, 0, c_light_gray, LINE_XOXO );
        mvwputch( w_info_border, j, width - 1, c_light_gray, LINE_XOXO );
    }

    for( int j = 0; j < width - 1; ++j ) {
        mvwputch( w_info_border, iInfoHeight - 1, j, c_light_gray, LINE_OXOX );
    }

    mvwputch( w_info_border, iInfoHeight - 1, 0, c_light_gray, LINE_XXOO );
    mvwputch( w_info_border, iInfoHeight - 1, width - 1, c_light_gray, LINE_XOOX );
    wrefresh( w_info_border );
}

void game::zones_manager()
{
    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    const int offset_x = ( u.posx() + u.view_offset.x ) - getmaxx( w_terrain ) / 2;
    const int offset_y = ( u.posy() + u.view_offset.y ) - getmaxy( w_terrain ) / 2;

    draw_ter();

    int stored_pm_opt = pixel_minimap_option;
    pixel_minimap_option = 0;

    int zone_ui_height = 12;
    int zone_options_height = 7;
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width :
                        VIEW_OFFSET_X;

    catacurses::window w_zones = catacurses::newwin( TERMY - 2 - zone_ui_height - VIEW_OFFSET_Y * 2,
                                 width - 2,
                                 VIEW_OFFSET_Y + 1, offsetX + 1 );
    catacurses::window w_zones_border = catacurses::newwin( TERMY - zone_ui_height - VIEW_OFFSET_Y * 2,
                                        width,
                                        VIEW_OFFSET_Y, offsetX );
    catacurses::window w_zones_info = catacurses::newwin( zone_ui_height - zone_options_height - 1,
                                      width - 2,
                                      TERMY - zone_ui_height - VIEW_OFFSET_Y, offsetX + 1 );
    catacurses::window w_zones_info_border = catacurses::newwin( zone_ui_height, width,
            TERMY - zone_ui_height - VIEW_OFFSET_Y, offsetX );
    catacurses::window w_zones_options = catacurses::newwin( zone_options_height - 1, width - 2,
                                         TERMY - zone_options_height - VIEW_OFFSET_Y, offsetX + 1 );

    zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
    zones_manager_shortcuts( w_zones_info );

    std::string action;
    input_context ctxt( "ZONES_MANAGER" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ADD_ZONE" );
    ctxt.register_action( "REMOVE_ZONE" );
    ctxt.register_action( "MOVE_ZONE_UP" );
    ctxt.register_action( "MOVE_ZONE_DOWN" );
    ctxt.register_action( "SHOW_ZONE_ON_MAP" );
    ctxt.register_action( "ENABLE_ZONE" );
    ctxt.register_action( "DISABLE_ZONE" );
    ctxt.register_action( "SHOW_ALL_ZONES" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    auto &mgr = zone_manager::get_manager();
    const int max_rows = TERMY - zone_ui_height - 2 - VIEW_OFFSET_Y * 2;
    int start_index = 0;
    int active_index = 0;
    bool blink = false;
    bool redraw_info = true;
    bool stuff_changed = false;
    bool show_all_zones = false;
    int zone_cnt = 0;

    // get zones on the same z-level, with distance between player and
    // zone center point <= 50 or all zones, if show_all_zones is true
    auto get_zones = [&]() {
        auto zones = mgr.get_zones();

        if( !show_all_zones ) {
            zones.erase(
                std::remove_if(
                    zones.begin(),
                    zones.end(),
            [&]( zone_manager::ref_zone_data ref ) -> bool {
                const auto &zone = ref.get();
                const auto &a = m.getabs( u.pos() );
                const auto &b = zone.get_center_point();
                return a.z != b.z || rl_dist( a, b ) > 50;
            }
                ),
            zones.end()
            );
        }

        zone_cnt = static_cast<int>( zones.size() );
        return zones;
    };

    auto zones = get_zones();

    auto zones_manager_options = [&]() {
        werase( w_zones_options );

        if( zone_cnt > 0 ) {
            const auto &zone = zones[active_index].get();

            if( zone.has_options() ) {
                const auto &descriptions = zone.get_options().get_descriptions();

                mvwprintz( w_zones_options, 0, 1, c_white, _( "Options" ) );

                int y = 1;
                for( const auto &desc : descriptions ) {
                    mvwprintz( w_zones_options, y, 3, c_white, desc.first );
                    mvwprintz( w_zones_options, y, 20, c_white, desc.second );
                    y++;
                }
            }
        }

        wrefresh( w_zones_options );
    };

    auto query_position = [this, w_zones_info]() -> cata::optional<std::pair<tripoint, tripoint>> {
        werase( w_zones_info );
        mvwprintz( w_zones_info, 3, 2, c_white, _( "Select first point." ) );
        wrefresh( w_zones_info );

        tripoint center = u.pos() + u.view_offset;

        const look_around_result first = look_around( w_zones_info, center, center, false, true, false );
        if( first.position )
        {
            mvwprintz( w_zones_info, 3, 2, c_white, _( "Select second point." ) );
            wrefresh( w_zones_info );

            const look_around_result second = look_around( w_zones_info, center, *first.position, true, true,
                    false );
            if( second.position ) {
                werase( w_zones_info );
                wrefresh( w_zones_info );

                tripoint first_abs = m.getabs( tripoint( std::min( first.position->x, second.position->x ),
                                               std::min( first.position->y, second.position->y ),
                                               std::min( first.position->z, second.position->z ) ) );
                tripoint second_abs = m.getabs( tripoint( std::max( first.position->x, second.position->x ),
                                                std::max( first.position->y, second.position->y ),
                                                std::max( first.position->z, second.position->z ) ) );

                return std::pair<tripoint, tripoint>( first_abs, second_abs );
            }
        }

        return cata::nullopt;
    };

    zones_manager_open = true;
    do {
        if( action == "ADD_ZONE" ) {
            zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );

            do { // not a loop, just for quick bailing out if canceled
                const auto maybe_id = mgr.query_type();
                if( !maybe_id.has_value() ) {
                    break;
                }

                const zone_type_id &id = maybe_id.value();
                auto options = zone_options::create( id );

                if( !options->query_at_creation() ) {
                    break;
                }

                auto default_name = options->get_zone_name_suggestion();
                if( default_name.empty() ) {
                    default_name = mgr.get_name_from_type( id );
                }
                const auto maybe_name = mgr.query_name( default_name );
                if( !maybe_name.has_value() ) {
                    break;
                }
                const itype_id &name = maybe_name.value();

                const auto position = query_position();
                if( !position ) {
                    break;
                }

                mgr.add( name, id, false, true, position->first, position->second, options );

                zones = get_zones();
                active_index = zone_cnt - 1;

                stuff_changed = true;
            } while( false );

            draw_ter();
            blink = false;
            redraw_info = true;

            zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
            zones_manager_shortcuts( w_zones_info );

        } else if( action == "SHOW_ALL_ZONES" ) {
            show_all_zones = !show_all_zones;
            zones = get_zones();
            active_index = 0;
            draw_ter();
            redraw_info = true;

        } else if( zone_cnt > 0 ) {
            if( action == "UP" ) {
                active_index--;
                if( active_index < 0 ) {
                    active_index = zone_cnt - 1;
                }
                draw_ter();
                blink = false;
                redraw_info = true;

            } else if( action == "DOWN" ) {
                active_index++;
                if( active_index >= zone_cnt ) {
                    active_index = 0;
                }
                draw_ter();
                blink = false;
                redraw_info = true;

            } else if( action == "REMOVE_ZONE" ) {
                if( active_index < zone_cnt ) {
                    mgr.remove( zones[active_index] );
                    zones = get_zones();
                    active_index--;

                    if( active_index < 0 ) {
                        active_index = 0;
                    }

                    draw_ter();
                    wrefresh( w_terrain );
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

            } else if( action == "CONFIRM" ) {
                auto &zone = zones[active_index].get();

                uilist as_m;
                as_m.text = _( "What do you want to change:" );
                as_m.entries.emplace_back( 1, true, '1', _( "Edit name" ) );
                as_m.entries.emplace_back( 2, true, '2', _( "Edit type" ) );
                as_m.entries.emplace_back( 3, zone.get_options().has_options(), '3',
                                           _( "Edit options" ) );
                as_m.entries.emplace_back( 4, !zone.get_is_vehicle(), '4', _( "Edit position" ) );
                as_m.query();

                switch( as_m.ret ) {
                    case 1:
                        if( zone.set_name() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 2:
                        if( zone.set_type() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 3:
                        if( zone.get_options().query() ) {
                            stuff_changed = true;
                        }
                        break;
                    case 4: {
                        const auto pos = query_position();
                        if( pos && ( pos->first != zone.get_start_point() || pos->second != zone.get_end_point() ) ) {

                            zone.set_position( *pos );
                            stuff_changed = true;
                        }
                    }
                    break;
                    default:
                        break;
                }

                draw_ter();

                blink = false;
                redraw_info = true;

                zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
                zones_manager_shortcuts( w_zones_info );

            } else if( action == "MOVE_ZONE_UP" && zone_cnt > 1 ) {
                if( active_index < zone_cnt - 1 ) {
                    mgr.swap( zones[active_index], zones[active_index + 1] );
                    zones = get_zones();
                    active_index++;
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

            } else if( action == "MOVE_ZONE_DOWN" && zone_cnt > 1 ) {
                if( active_index > 0 ) {
                    mgr.swap( zones[active_index], zones[active_index - 1] );
                    zones = get_zones();
                    active_index--;
                }
                blink = false;
                redraw_info = true;
                stuff_changed = true;

            } else if( action == "SHOW_ZONE_ON_MAP" ) {
                //show zone position on overmap;
                tripoint player_overmap_position = ms_to_omt_copy( m.getabs( u.pos() ) );
                tripoint zone_overmap = ms_to_omt_copy( zones[active_index].get().get_center_point() );

                ui::omap::display_zones( player_overmap_position, zone_overmap, active_index );

                zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
                zones_manager_shortcuts( w_zones_info );

                draw_ter();

                redraw_info = true;

            } else if( action == "ENABLE_ZONE" ) {
                zones[active_index].get().set_enabled( true );

                redraw_info = true;
                stuff_changed = true;

            } else if( action == "DISABLE_ZONE" ) {
                zones[active_index].get().set_enabled( false );

                redraw_info = true;
                stuff_changed = true;
            }
        }

        if( zone_cnt == 0 ) {
            werase( w_zones );
            wrefresh( w_zones_border );
            mvwprintz( w_zones, 5, 2, c_white, _( "No Zones defined." ) );

        } else if( redraw_info ) {
            redraw_info = false;
            werase( w_zones );

            calcStartPos( start_index, active_index, max_rows, zone_cnt );

            draw_scrollbar( w_zones_border, active_index, max_rows, zone_cnt, 1 );
            wrefresh( w_zones_border );

            int iNum = 0;

            tripoint player_absolute_pos = m.getabs( u.pos() );

            //Display saved zones
            for( auto &i : zones ) {
                if( iNum >= start_index &&
                    iNum < start_index + ( ( max_rows > zone_cnt ) ? zone_cnt : max_rows ) ) {
                    const auto &zone = i.get();

                    nc_color colorLine = ( zone.get_enabled() ) ? c_white : c_light_gray;

                    if( iNum == active_index ) {
                        mvwprintz( w_zones, iNum - start_index, 0, c_yellow, "%s", ">>" );
                        colorLine = ( zone.get_enabled() ) ? c_light_green : c_green;
                    }

                    //Draw Zone name
                    mvwprintz( w_zones, iNum - start_index, 3, colorLine,
                               zone.get_name() );

                    //Draw Type name
                    mvwprintz( w_zones, iNum - start_index, 20, colorLine,
                               mgr.get_name_from_type( zone.get_type() ) );

                    tripoint center = zone.get_center_point();

                    //Draw direction + distance
                    mvwprintz( w_zones, iNum - start_index, 32, colorLine, "%*d %s",
                               5, static_cast<int>( trig_dist( player_absolute_pos, center ) ),
                               direction_name_short( direction_from( player_absolute_pos, center ) ).c_str() );

                    //Draw Vehicle Indicator
                    mvwprintz( w_zones, iNum - start_index, 41, colorLine,
                               zone.get_is_vehicle() ? "*" : "" );
                }
                iNum++;
            }

            // Display zone options
            zones_manager_options();
        }

        if( zone_cnt > 0 ) {
            const auto &zone = zones[active_index].get();

            blink = !blink;

            tripoint start = m.getlocal( zone.get_start_point() );
            tripoint end = m.getlocal( zone.get_end_point() );

            if( blink ) {
                //draw marked area
                tripoint offset = tripoint( offset_x, offset_y, 0 ); //ASCII
#ifdef TILES
                if( use_tiles ) {
                    offset = tripoint_zero; //TILES
                } else {
                    offset = tripoint( -offset_x, -offset_y, 0 ); //SDL
                }
#endif

                draw_zones( start, end, offset );
            } else {
                //clear marked area
#ifdef TILES
                if( !use_tiles ) {
#endif
                    for( int iY = start.y; iY <= end.y; ++iY ) {
                        for( int iX = start.x; iX <= end.x; ++iX ) {
                            if( u.sees( tripoint( iX, iY, u.posz() ) ) ) {
                                m.drawsq( w_terrain, u,
                                          tripoint( iX, iY, u.posz() + u.view_offset.z ),
                                          false,
                                          false,
                                          u.pos() + u.view_offset );
                            } else {
                                if( u.has_effect( effect_boomered ) ) {
                                    mvwputch( w_terrain, iY - offset_y, iX - offset_x, c_magenta, '#' );

                                } else {
                                    mvwputch( w_terrain, iY - offset_y, iX - offset_x, c_black, ' ' );
                                }
                            }
                        }
                    }
#ifdef TILES
                }
#endif
            }

            ctxt.set_timeout( BLINK_SPEED );
        } else {
            ctxt.reset_timeout();
        }

        wrefresh( w_terrain );
        wrefresh( w_zones );
        wrefresh( w_zones_border );

        //Wait for input
        action = ctxt.handle_input();
    } while( action != "QUIT" );
    zones_manager_open = false;
    ctxt.reset_timeout();

    if( stuff_changed ) {
        auto &zones = zone_manager::get_manager();
        if( query_yn( _( "Save changes?" ) ) ) {
            zones.save_zones();
        } else {
            zones.load_zones();
        }

        zones.cache_data();
    }

    u.view_offset = stored_view_offset;
    pixel_minimap_option = stored_pm_opt;

    refresh_all();
}

void game::pre_print_all_tile_info( const tripoint &lp, const catacurses::window &w_info,
                                    int &first_line, const int last_line,
                                    const visibility_variables &cache )
{
    // get global area info according to look_around caret position
    const oter_id &cur_ter_m = overmap_buffer.ter( ms_to_omt_copy( g->m.getabs( lp ) ) );
    // we only need the area name and then pass it to print_all_tile_info() function below
    const std::string area_name = cur_ter_m->get_name();
    print_all_tile_info( lp, w_info, area_name, 1, first_line, last_line, !is_draw_tiles_mode(),
                         cache );
}

cata::optional<tripoint> game::look_around()
{
    tripoint center = u.pos() + u.view_offset;
    look_around_result result = look_around( catacurses::window(), center, center, false, false,
                                false );
    return result.position;
}

look_around_result game::look_around( catacurses::window w_info, tripoint &center,
                                      const tripoint &start_point, bool has_first_point, bool select_zone, bool peeking )
{
    bVMonsterLookFire = false;
    // TODO: Make this `true`
    const bool allow_zlev_move = m.has_zlevels() &&
                                 ( debug_mode || fov_3d || u.has_trait( trait_id( "DEBUG_NIGHTVISION" ) ) );

    temp_exit_fullscreen();

    const int offset_x = ( u.posx() + u.view_offset.x ) - getmaxx( w_terrain ) / 2;
    const int offset_y = ( u.posy() + u.view_offset.y ) - getmaxy( w_terrain ) / 2;

    tripoint lp = start_point; // cursor
    int &lx = lp.x;
    int &ly = lp.y;
    int &lz = lp.z;

    draw_ter( center );
    wrefresh( w_terrain );

    draw_pixel_minimap();

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
    bool fast_scroll = false;

    int lookWidth = 0;
    int lookY = 0;
    int lookX = 0;
    get_lookaround_dimensions( lookWidth, lookY, lookX );

    bool bNewWindow = false;
    if( !w_info ) {
        w_info = catacurses::newwin( getmaxy( w_messages ), lookWidth, lookY, lookX );
        bNewWindow = true;
    }

    dbg( D_PEDANTIC_INFO ) << ": calling handle_input()";

    std::string action;
    input_context ctxt( "LOOK" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    ctxt.register_action( "TOGGLE_FAST_SCROLL" );
    ctxt.register_action( "EXTENDED_DESCRIPTION" );
    ctxt.register_action( "SELECT" );
    if( peeking ) {
        ctxt.register_action( "throw_blind" );
    }
    if( !select_zone ) {
        ctxt.register_action( "TRAVEL_TO" );
        ctxt.register_action( "LIST_ITEMS" );
    }
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "CENTER" );

    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    const int old_levz = get_levz();

    m.update_visibility_cache( old_levz );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    bool blink = true;
    bool action_unprocessed = false;
    bool redraw = true;
    look_around_result result;
    do {
        if( redraw ) {
            if( bNewWindow ) {
                werase( w_info );
                draw_border( w_info );

                static const char *title_prefix = "< ";
                static const char *title = _( "Look Around" );
                static const char *title_suffix = " >";
                static const std::string full_title = string_format( "%s%s%s", title_prefix, title, title_suffix );
                const int start_pos = center_text_pos( full_title.c_str(), 0, getmaxx( w_info ) - 1 );
                mvwprintz( w_info, 0, start_pos, c_white, title_prefix );
                wprintz( w_info, c_green, title );
                wprintz( w_info, c_white, title_suffix );

                nc_color clr = c_white;
                std::string colored_key = string_format( "<color_light_green>%s</color>",
                                          ctxt.get_desc( "EXTENDED_DESCRIPTION", 1 ).c_str() );
                print_colored_text( w_info, getmaxy( w_info ) - 2, 2, clr, clr,
                                    string_format( _( "Press %s to view extended description" ),
                                                   colored_key.c_str() ) );
                colored_key = string_format( "<color_light_green>%s</color>",
                                             ctxt.get_desc( "LIST_ITEMS", 1 ).c_str() );
                print_colored_text( w_info, getmaxy( w_info ) - 1, 2, clr, clr,
                                    string_format( _( "Press %s to list items and monsters" ),
                                                   colored_key.c_str() ) );
                if( peeking ) {
                    colored_key = string_format( "<color_light_green>%s</color>", ctxt.get_desc( "throw_blind",
                                                 1 ).c_str() );
                    print_colored_text( w_info, getmaxy( w_info ) - 3, 2, clr, clr,
                                        string_format( _( "Press %s to blind throw" ), colored_key.c_str() ) );
                }

                int first_line = 1;
                const int last_line = getmaxy( w_messages ) - 2;
                pre_print_all_tile_info( lp, w_info, first_line, last_line, cache );
                if( fast_scroll ) {
                    //~ "Fast Scroll" mark below the top right corner of the info window
                    right_print( w_info, 1, 0, c_light_green, _( "F" ) );
                }

                wrefresh( w_info );
            }

            draw_ter( center, true );

            if( select_zone && has_first_point ) {
                if( blink ) {
                    const int dx = start_point.x - offset_x + u.posx() - lx;
                    const int dy = start_point.y - offset_y + u.posy() - ly;

                    const tripoint start = tripoint( std::min( dx, POSX ), std::min( dy, POSY ), lz );
                    const tripoint end = tripoint( std::max( dx, POSX ), std::max( dy, POSY ), lz );

                    tripoint offset = tripoint_zero; //ASCII/SDL
#ifdef TILES
                    if( use_tiles ) {
                        offset = tripoint( offset_x + lx - u.posx(), offset_y + ly - u.posy(), 0 ); //TILES
                    }
#endif
                    draw_zones( start, end, offset );
                }

                //Draw first point
                g->draw_cursor( start_point );
            }

            //Draw select cursor
            g->draw_cursor( lp );

            wrefresh( w_terrain );
        }

        if( select_zone && has_first_point ) {
            ctxt.set_timeout( BLINK_SPEED );
        }

        redraw = true;
        //Wait for input
        if( action_unprocessed ) {
            // There was an action unprocessed after the mouse event consumption loop, process it now
            action_unprocessed = false;
        } else {
            action = ctxt.handle_input();
        }
        if( action == "LIST_ITEMS" ) {
            list_items_monsters();
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
            if( !allow_zlev_move ) {
                continue;
            }

            const int dz = ( action == "LEVEL_UP" ? 1 : -1 );
            lz = clamp( lz + dz, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
            center.z = clamp( center.z + dz, -OVERMAP_DEPTH, OVERMAP_HEIGHT );

            add_msg( m_debug, "levx: %d, levy: %d, levz :%d", get_levx(), get_levy(), center.z );
            u.view_offset.z = center.z - u.posz();
            refresh_all();
            if( select_zone && has_first_point ) { // is blinking
                blink = true; // Always draw blink symbols when moving cursor
            }
        } else if( action == "TRAVEL_TO" ) {
            if( !u.sees( lp ) ) {
                add_msg( _( "You can't see that destination." ) );
                continue;
            }

            auto route = m.route( u.pos(), lp, u.get_pathfinding_settings(), u.get_path_avoid() );
            if( route.size() > 1 ) {
                route.pop_back();
                u.set_destination( route );
            } else {
                add_msg( m_info, _( "You can't travel there." ) );
                continue;
            }
        } else if( action == "debug_scent" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_scent();
            }
        } else if( action == "EXTENDED_DESCRIPTION" ) {
            extended_description( lp );
            draw_sidebar();
        } else if( action == "CENTER" ) {
            center = u.pos();
            lp = u.pos();
        } else if( action == "MOUSE_MOVE" ) {
            const tripoint old_lp = lp;
            // Maximum mouse events before a forced graphics update
            int max_consume = 10;
            do {
                const cata::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain );
                if( mouse_pos ) {
                    lx = mouse_pos->x;
                    ly = mouse_pos->y;
                }
                if( --max_consume == 0 ) {
                    break;
                }
                // Consume all consecutive mouse movements. This lowers CPU consumption
                // by graphics updates when user moves the mouse continuously.
                action = ctxt.handle_input( 10 );
            } while( action == "MOUSE_MOVE" );
            if( action != "MOUSE_MOVE" && action != "TIMEOUT" ) {
                // The last event is not a mouse event or timeout, it needs to be processed in the next loop.
                action_unprocessed = true;
            }
            lx = clamp( lx, 0, MAPSIZE_X );
            ly = clamp( ly, 0, MAPSIZE_Y );
            if( select_zone && has_first_point ) { // is blinking
                if( blink && lp == old_lp ) { // blink symbols drawn (blink == true) and cursor not changed
                    redraw = false; // no need to redraw, so don't redraw to save CPU
                } else {
                    blink = true; // Always draw blink symbols when moving cursor
                }
            } else if( lp == old_lp ) { // not blinking and cursor not changed
                redraw = false; // no need to redraw, so don't redraw to save CPU
            }
        } else if( cata::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            if( fast_scroll ) {
                vec->x *= soffset;
                vec->y *= soffset;
            }

            lx = lx + vec->x;
            ly = ly + vec->y;
            center.x = center.x + vec->x;
            center.y = center.y + vec->y;
            if( select_zone && has_first_point ) { // is blinking
                blink = true; // Always draw blink symbols when moving cursor
            }
        } else if( action == "TIMEOUT" ) {
            blink = !blink;
        } else if( action == "throw_blind" ) {
            result.peek_action = PA_BLIND_THROW;
        }
    } while( action != "QUIT" && action != "CONFIRM" && action != "SELECT" && action != "TRAVEL_TO" &&
             action != "throw_blind" );

    if( m.has_zlevels() && center.z != old_levz ) {
        m.build_map_cache( old_levz );
        u.view_offset.z = 0;
    }

    ctxt.reset_timeout();

    if( bNewWindow ) {
        w_info = catacurses::window();
    }
    reenter_fullscreen();
    bVMonsterLookFire = true;

    if( action == "CONFIRM" || action == "SELECT" ) {
        result.position = lp;
    }

    return result;
}

std::vector<map_item_stack> game::find_nearby_items( int iRadius )
{
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> item_order;

    if( u.is_blind() ) {
        return ret;
    }

    std::vector<tripoint> points = closest_tripoints_first( iRadius, u.pos() );

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
    if( bDrawX ) {
        char sym = 'X';
        if( t.z > 0 ) {
            sym = '^';
        } else if( t.z < 0 ) {
            sym = 'v';
        }
        if( pts.empty() ) {
            mvwputch( w_terrain, POSY, POSX, c_white, sym );
        } else {
            mvwputch( w_terrain, POSY + ( pts[pts.size() - 1].y - ( u.posy() + u.view_offset.y ) ),
                      POSX + ( pts[pts.size() - 1].x - ( u.posx() + u.view_offset.x ) ),
                      c_white, sym );
        }
    }

    wrefresh( w_terrain );
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state( const catacurses::window &window, int height, bool bRadiusSort )
{
    const int width = use_narrow_sidebar() ? 45 : 55;
    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( window, 0, i, c_light_gray, LINE_OXOX ); // -
            mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, i, c_light_gray, LINE_OXOX ); // -
        }

        if( i < TERMY - height - VIEW_OFFSET_Y * 2 ) {
            mvwputch( window, i, 0, c_light_gray, LINE_XOXO ); // |
            mvwputch( window, i, width - 1, c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( window, 0, 0, c_light_gray, LINE_OXXO ); // |^
    mvwputch( window, 0, width - 1, c_light_gray, LINE_OOXX ); // ^|

    mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, 0, c_light_gray, LINE_XXXO ); // |-
    mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, width - 1, c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( window, 0, 2, c_light_green, "<Tab> " );
    wprintz( window, c_white, _( "Items" ) );

    std::string sSort;
    if( bRadiusSort ) {
        //~ Sort type: distance.
        sSort = _( "<s>ort: dist" );
    } else {
        //~ Sort type: category.
        sSort = _( "<s>ort: cat" );
    }

    int letters = utf8_width( sSort );

    shortcut_print( window, 0, getmaxx( window ) - letters, c_white, c_light_green, sSort );

    std::vector<std::string> tokens;
    if( !sFilter.empty() ) {
        tokens.emplace_back( _( "<R>eset" ) );
    }

    tokens.emplace_back( _( "<E>xamine" ) );
    tokens.emplace_back( _( "<C>ompare" ) );
    tokens.emplace_back( _( "<F>ilter" ) );
    tokens.emplace_back( _( "<+/->Priority" ) );

    int gaps = tokens.size() + 1;
    letters = 0;
    int n = tokens.size();
    for( int i = 0; i < n; i++ ) {
        letters += utf8_width( tokens[i] ) - 2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = ( width - usedwidth ) / gaps;
    usedwidth += gap_spaces * gaps;
    int xpos = gap_spaces + ( width - usedwidth ) / 2;
    const int ypos = TERMY - height - 1 - VIEW_OFFSET_Y * 2;

    for( int i = 0; i < n; i++ ) {
        xpos += shortcut_print( window, ypos, xpos, c_white, c_light_green, tokens[i] ) + gap_spaces;
    }

    refresh_all();
}

void centerlistview( const tripoint &active_item_position )
{
    player &u = g->u;
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z = active_item_position.z;
        int xpos = POSX + active_item_position.x;
        int ypos = POSY + active_item_position.y;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            int xOffset = TERRAIN_WINDOW_WIDTH / 2;
            int yOffset = TERRAIN_WINDOW_HEIGHT / 2;
            if( !is_valid_in_w_terrain( xpos, ypos ) ) {
                if( xpos < 0 ) {
                    u.view_offset.x = xpos - xOffset;
                } else {
                    u.view_offset.x = xpos - ( TERRAIN_WINDOW_WIDTH - 1 ) + xOffset;
                }

                if( xpos < 0 ) {
                    u.view_offset.y = ypos - yOffset;
                } else {
                    u.view_offset.y = ypos - ( TERRAIN_WINDOW_HEIGHT - 1 ) + yOffset;
                }
            } else {
                u.view_offset.x = 0;
                u.view_offset.y = 0;
            }
        } else {
            if( xpos < 0 ) {
                u.view_offset.x = xpos;
            } else if( xpos >= TERRAIN_WINDOW_WIDTH ) {
                u.view_offset.x = xpos - ( TERRAIN_WINDOW_WIDTH - 1 );
            } else {
                u.view_offset.x = 0;
            }

            if( ypos < 0 ) {
                u.view_offset.y = ypos;
            } else if( ypos >= TERRAIN_WINDOW_HEIGHT ) {
                u.view_offset.y = ypos - ( TERRAIN_WINDOW_HEIGHT - 1 );
            } else {
                u.view_offset.y = 0;
            }
        }
    }

}

#define MAXIMUM_ZOOM_LEVEL 4
void game::zoom_out()
{
#ifdef TILES
    if( tileset_zoom > MAXIMUM_ZOOM_LEVEL ) {
        tileset_zoom = tileset_zoom / 2;
    } else {
        tileset_zoom = 64;
    }
    rescale_tileset( tileset_zoom );
#endif
}

void game::zoom_in()
{
#ifdef TILES
    if( tileset_zoom == 64 ) {
        tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        tileset_zoom = tileset_zoom * 2;
    }
    rescale_tileset( tileset_zoom );
#endif
}

void game::reset_zoom()
{
#ifdef TILES
    tileset_zoom = 16;
    rescale_tileset( tileset_zoom );
#endif // TILES
}

int game::get_moves_since_last_save() const
{
    return moves_since_last_save;
}

int game::get_user_action_counter() const
{
    return user_action_counter;
}

void game::list_items_monsters()
{
    std::vector<Creature *> mons = u.get_visible_creatures( DAYLIGHT_LEVEL );
    ///\EFFECT_PER increases range of interacting with items on the ground from a list
    const std::vector<map_item_stack> items = find_nearby_items( 2 * u.per_cur + 12 );

    if( mons.empty() && items.empty() ) {
        add_msg( m_info, _( "You don't see any items or monsters around you!" ) );
        return;
    }

    std::sort( mons.begin(), mons.end(), [&]( const Creature * lhs, const Creature * rhs ) {
        const auto att_lhs = lhs->attitude_to( u );
        const auto att_rhs = rhs->attitude_to( u );

        return att_lhs < att_rhs || ( att_lhs == att_rhs
                                      && rl_dist( u.pos(), lhs->pos() ) < rl_dist( u.pos(), rhs->pos() ) );
    } );

    // If the current list is empty, switch to the non-empty list
    if( uistate.vmenu_show_items ) {
        if( items.empty() ) {
            uistate.vmenu_show_items = false;
        }
    } else if( mons.empty() ) {
        uistate.vmenu_show_items = true;
    }

    temp_exit_fullscreen();
    game::vmenu_ret ret;
    while( true ) {
        ret = uistate.vmenu_show_items ? list_items( items ) : list_monsters( mons );
        if( ret == game::vmenu_ret::CHANGE_TAB ) {
            uistate.vmenu_show_items = !uistate.vmenu_show_items;
        } else {
            break;
        }
    }

    refresh_all();
    if( ret == game::vmenu_ret::FIRE ) {
        plfire( u.weapon );
    }
    reenter_fullscreen();
}

game::vmenu_ret game::list_items( const std::vector<map_item_stack> &item_list )
{
    int iInfoHeight = std::min( 25, TERMY / 2 );
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width : VIEW_OFFSET_X;

    catacurses::window w_items = catacurses::newwin( TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2,
                                 width - 2, VIEW_OFFSET_Y + 1, offsetX + 1 );
    catacurses::window w_items_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                        width, VIEW_OFFSET_Y, offsetX );
    catacurses::window w_item_info = catacurses::newwin( iInfoHeight, width,
                                     TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX );

    // use previously selected sorting method
    bool sort_radius = uistate.list_item_sort != 2;
    bool addcategory = !sort_radius;

    // reload filter/priority settings on the first invocation, if they were active
    if( !uistate.list_item_init ) {
        if( uistate.list_item_filter_active ) {
            sFilter = uistate.list_item_filter;
        }
        if( uistate.list_item_downvote_active ) {
            list_item_downvote = uistate.list_item_downvote;
        }
        if( uistate.list_item_priority_active ) {
            list_item_upvote = uistate.list_item_priority;
        }
        uistate.list_item_init = true;
    }

    std::vector<map_item_stack> ground_items = item_list;
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items =
        !sFilter.empty() ? filter_item_stacks( ground_items, sFilter ) : ground_items;
    int highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
    int lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
    int iItemNum = ground_items.size();

    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2;
    int iStartPos = 0;
    tripoint active_pos;
    cata::optional<tripoint> iLastActive;
    bool reset = true;
    bool refilter = true;
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    map_item_stack *activeItem = nullptr;
    std::map<int, std::string> mSortCategory;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "LEFT", _( "Previous item" ) );
    ctxt.register_action( "RIGHT", _( "Next item" ) );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    do {
        if( action == "COMPARE" ) {
            game_menus::inv::compare( u, active_pos );
            reset = true;
            refresh_all();
        } else if( action == "FILTER" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::FILTER );
            string_input_popup()
            .title( _( "Filter:" ) )
            .width( 55 )
            .description( _( "UP: history, CTRL-U: clear line, ESC: abort, ENTER: save" ) )
            .identifier( "item_filter" )
            .max_length( 256 )
            .edit( sFilter );
            reset = true;
            refilter = true;
            addcategory = !sort_radius;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            iLastActive.reset();
            reset = true;
            refilter = true;
            uistate.list_item_filter_active = false;
            addcategory = !sort_radius;
        } else if( action == "EXAMINE" && !filtered_items.empty() ) {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            int dummy = 0; // draw_item_info needs an int &
            activeItem->example->info( true, vThisItem );
            draw_item_info( 0, width - 5, 0, TERMY - VIEW_OFFSET_Y * 2,
                            activeItem->example->tname(), activeItem->example->type_name(), vThisItem, vDummy, dummy,
                            false, false, true );
            // wait until the user presses a key to wipe the screen
            iLastActive.reset();
            reset = true;
        } else if( action == "PRIORITY_INCREASE" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::HIGH_PRIORITY );
            list_item_upvote = string_input_popup()
                               .title( _( "High Priority:" ) )
                               .width( 55 )
                               .text( list_item_upvote )
                               .description( _( "UP: history, CTRL-U clear line, ESC: abort, ENTER: save" ) )
                               .identifier( "list_item_priority" )
                               .max_length( 256 )
                               .query_string();
            refilter = true;
            reset = true;
            addcategory = !sort_radius;
            uistate.list_item_priority_active = !list_item_upvote.empty();
        } else if( action == "PRIORITY_DECREASE" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::LOW_PRIORITY );
            list_item_downvote = string_input_popup()
                                 .title( _( "Low Priority:" ) )
                                 .width( 55 )
                                 .text( list_item_downvote )
                                 .description( _( "UP: history, CTRL-U clear line, ESC: abort, ENTER: save" ) )
                                 .identifier( "list_item_downvote" )
                                 .max_length( 256 )
                                 .query_string();
            refilter = true;
            reset = true;
            addcategory = !sort_radius;
            uistate.list_item_downvote_active = !list_item_downvote.empty();
        } else if( action == "SORT" ) {
            if( sort_radius ) {
                sort_radius = false;
                addcategory = true;
                uistate.list_item_sort = 2; // list is sorted by category
            } else {
                sort_radius = true;
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
                add_msg( _( "You can't see that destination." ) );
            }
            auto route = m.route( u.pos(), u.pos() + active_pos, u.get_pathfinding_settings(),
                                  u.get_path_avoid() );
            if( route.size() > 1 ) {
                route.pop_back();
                u.set_destination( route );
                break;
            } else {
                add_msg( m_info, _( "You can't travel there." ) );
            }
        }
        if( uistate.list_item_sort == 1 ) {
            ground_items = item_list;
        } else if( uistate.list_item_sort == 2 ) {
            std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort );
        }

        if( refilter ) {
            refilter = false;
            filtered_items = filter_item_stacks( ground_items, sFilter );
            highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
            lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
            iActive = 0;
            page_num = 0;
            iLastActive.reset();
            iItemNum = filtered_items.size();
        }

        if( addcategory ) {
            addcategory = false;
            iCatSortNum = 0;
            mSortCategory.clear();
            if( highPEnd > 0 ) {
                mSortCategory[0] = _( "HIGH PRIORITY" );
                iCatSortNum++;
            }
            std::string last_cat_name;
            for( int i = std::max( 0, highPEnd );
                 i < std::min( lowPStart, static_cast<int>( filtered_items.size() ) ); i++ ) {
                const std::string &cat_name = filtered_items[i].example->get_category().name();
                if( cat_name != last_cat_name ) {
                    mSortCategory[i + iCatSortNum++] = cat_name;
                    last_cat_name = cat_name;
                }
            }
            if( lowPStart < static_cast<int>( filtered_items.size() ) ) {
                mSortCategory[lowPStart + iCatSortNum++] = _( "LOW PRIORITY" );
            }
            if( !mSortCategory[0].empty() ) {
                iActive++;
            }
            iItemNum = int( filtered_items.size() ) + iCatSortNum;
        }

        if( reset ) {
            reset_item_list_state( w_items_border, iInfoHeight, sort_radius );
            reset = false;
            iScrollPos = 0;
        }

        if( action == "HELP_KEYBINDINGS" ) {
            game::draw_ter();
            wrefresh( w_terrain );
        } else if( action == "UP" ) {
            do {
                iActive--;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive < 0 ) {
                iActive = iItemNum - 1;
            }
        } else if( action == "DOWN" ) {
            do {
                iActive++;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive >= iItemNum ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            }
        } else if( action == "RIGHT" ) {
            if( !filtered_items.empty() && ++page_num >= static_cast<int>( activeItem->vIG.size() ) ) {
                page_num = activeItem->vIG.size() - 1;
            }
        } else if( action == "LEFT" ) {
            page_num = std::max( 0, page_num - 1 );
        } else if( action == "PAGE_UP" ) {
            iScrollPos--;
        } else if( action == "PAGE_DOWN" ) {
            iScrollPos++;
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        }

        if( ground_items.empty() ) {
            reset_item_list_state( w_items_border, iInfoHeight, sort_radius );
            wrefresh( w_items_border );
            mvwprintz( w_items, 10, 2, c_white, _( "You don't see any items around you!" ) );
        } else {
            werase( w_items );
            calcStartPos( iStartPos, iActive, iMaxRows, iItemNum );
            int iNum = 0;
            active_pos = tripoint_zero;
            bool high = false;
            bool low = false;
            int index = 0;
            int iCatSortOffset = 0;

            for( int i = 0; i < iStartPos; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            for( auto iter = filtered_items.begin(); iter != filtered_items.end(); ++index ) {
                if( highPEnd > 0 && index < highPEnd + iCatSortOffset ) {
                    high = true;
                    low = false;
                } else if( index >= lowPStart + iCatSortOffset ) {
                    high = false;
                    low = true;
                } else {
                    high = false;
                    low = false;
                }

                if( iNum >= iStartPos && iNum < iStartPos + ( iMaxRows > iItemNum ? iItemNum : iMaxRows ) ) {
                    int iThisPage = 0;
                    if( !mSortCategory[iNum].empty() ) {
                        iCatSortOffset++;
                        mvwprintz( w_items, iNum - iStartPos, 1, c_magenta, mSortCategory[iNum] );
                    } else {
                        if( iNum == iActive ) {
                            iThisPage = page_num;
                            active_pos = iter->vIG[iThisPage].pos;
                            activeItem = &( *iter );
                        }
                        std::stringstream sText;
                        if( iter->vIG.size() > 1 ) {
                            sText << "[" << iThisPage + 1 << "/" << iter->vIG.size() << "] (" << iter->totalcount << ") ";
                        }
                        sText << iter->example->tname();
                        if( iter->vIG[iThisPage].count > 1 ) {
                            sText << " [" << iter->vIG[iThisPage].count << "]";
                        }

                        nc_color col = c_light_green;
                        if( iNum != iActive ) {
                            if( high ) {
                                col = c_yellow;
                            } else if( low ) {
                                col = c_red;
                            } else {
                                col = iter->example->color_in_inventory();
                            }
                        }
                        trim_and_print( w_items, iNum - iStartPos, 1, width - 9, col, sText.str() );
                        const int numw = iItemNum > 9 ? 2 : 1;
                        const int x = iter->vIG[iThisPage].pos.x;
                        const int y = iter->vIG[iThisPage].pos.y;
                        mvwprintz( w_items, iNum - iStartPos, width - 6 - numw,
                                   iNum == iActive ? c_light_green : c_light_gray,
                                   "%*d %s", numw, rl_dist( 0, 0, x, y ),
                                   direction_name_short( direction_from( 0, 0, x, y ) ).c_str() );
                        ++iter;
                    }
                } else {
                    ++iter;
                }
                iNum++;
            }
            iNum = 0;
            for( int i = 0; i < iActive; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            mvwprintz( w_items_border, 0, ( width - 9 ) / 2 + ( iItemNum > 9 ? 0 : 1 ),
                       c_light_green, " %*d", iItemNum > 9 ? 2 : 1, iItemNum > 0 ? iActive - iNum + 1 : 0 );
            wprintz( w_items_border, c_white, " / %*d ", iItemNum > 9 ? 2 : 1, iItemNum - iCatSortNum );
            werase( w_item_info );

            if( iItemNum > 0 ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                activeItem->example->info( true, vThisItem );
                draw_item_info( w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true );
                // Only redraw trail/terrain if x/y position changed or if keybinding menu erased it
                if( ( !iLastActive || active_pos != *iLastActive ) || action == "HELP_KEYBINDINGS" ) {
                    iLastActive.emplace( active_pos );
                    centerlistview( active_pos );
                    draw_trail_to_square( active_pos, true );
                }
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, 1 );
            wrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty();
        draw_custom_border( w_item_info, bDrawLeft, 1, 0, 1, LINE_XOXO, LINE_XOXO, 1, 1 );
        wrefresh( w_items );
        wrefresh( w_item_info );
        catacurses::refresh();
        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;
    return game::vmenu_ret::QUIT;
}

game::vmenu_ret game::list_monsters( const std::vector<Creature *> &monster_list )
{
    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = right_sidebar ? TERMX - VIEW_OFFSET_X - width :
                        VIEW_OFFSET_X;

    catacurses::window w_monsters = catacurses::newwin( TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2,
                                    width - 2, VIEW_OFFSET_Y + 1, offsetX + 1 );
    catacurses::window w_monsters_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                           width, VIEW_OFFSET_Y, offsetX );
    catacurses::window w_monster_info = catacurses::newwin( iInfoHeight - 1, width - 2,
                                        TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX + 1 );
    catacurses::window w_monster_info_border = catacurses::newwin( iInfoHeight, width,
            TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX );

    const int max_gun_range = u.weapon.gun_range( &u );

    const tripoint stored_view_offset = u.view_offset;
    u.view_offset = tripoint_zero;

    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2 - 1;
    int iStartPos = 0;
    cata::optional<tripoint> iLastActivePos;
    Creature *cCurMon = nullptr;

    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( w_monsters_border, 0, i, BORDER_COLOR, LINE_OXOX ); // -
            mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, BORDER_COLOR,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2 ) {
            mvwputch( w_monsters_border, i, 0, BORDER_COLOR, LINE_XOXO ); // |
            mvwputch( w_monsters_border, i, width - 1, BORDER_COLOR, LINE_XOXO ); // |
        }
    }

    mvwputch( w_monsters_border, 0, 0, BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w_monsters_border, 0, width - 1, BORDER_COLOR, LINE_OOXX ); // ^|

    mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, BORDER_COLOR,
              LINE_XXXO ); // |-
    mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, BORDER_COLOR,
              LINE_XOXX ); // -|

    mvwprintz( w_monsters_border, 0, 2, c_light_green, "<Tab> " );
    wprintz( w_monsters_border, c_white, _( "Monsters" ) );

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_ADD" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_REMOVE" );
    ctxt.register_action( "QUIT" );
    if( bVMonsterLookFire ) {
        ctxt.register_action( "look" );
        ctxt.register_action( "fire" );
    }
    ctxt.register_action( "HELP_KEYBINDINGS" );

    // first integer is the row the attitude category string is printed in the menu
    std::map<int, Creature::Attitude> mSortCategory;

    for( int i = 0, last_attitude = -1; i < static_cast<int>( monster_list.size() ); i++ ) {
        const auto attitude = monster_list[i]->attitude_to( u );
        if( attitude != last_attitude ) {
            mSortCategory[i + mSortCategory.size()] = attitude;
            last_attitude = attitude;
        }
    }

    do {
        if( action == "HELP_KEYBINDINGS" ) {
            game::draw_ter();
            wrefresh( w_terrain );
        } else if( action == "UP" ) {
            iActive--;
            if( iActive < 0 ) {
                iActive = int( monster_list.size() ) - 1;
            }
        } else if( action == "DOWN" ) {
            iActive++;
            if( iActive >= int( monster_list.size() ) ) {
                iActive = 0;
            }
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        } else if( action == "SAFEMODE_BLACKLIST_REMOVE" ) {
            const auto m = dynamic_cast<monster *>( cCurMon );
            const std::string monName = ( m != nullptr ) ? m->name() : "human";

            if( get_safemode().has_rule( monName, Creature::A_ANY ) ) {
                get_safemode().remove_rule( monName, Creature::A_ANY );
            }
        } else if( action == "SAFEMODE_BLACKLIST_ADD" ) {
            if( !get_safemode().empty() ) {
                const auto m = dynamic_cast<monster *>( cCurMon );
                const std::string monName = ( m != nullptr ) ? m->name() : "human";

                get_safemode().add_rule( monName, Creature::A_ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                                         RULE_BLACKLISTED );
            }
        } else if( action == "look" ) {
            iLastActivePos = look_around();
        } else if( action == "fire" ) {
            if( cCurMon != nullptr && rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                u.last_target = shared_from( *cCurMon );
                u.view_offset = stored_view_offset;
                return game::vmenu_ret::FIRE;
            }
        }

        if( monster_list.empty() ) {
            wrefresh( w_monsters_border );
            mvwprintz( w_monsters, 10, 2, c_white, _( "You don't see any monsters around you!" ) );
        } else {
            werase( w_monsters );
            const int iNumMonster = monster_list.size();
            const int iMenuSize = monster_list.size() + mSortCategory.size();

            const int numw = iNumMonster > 999 ? 4 :
                             iNumMonster > 99  ? 3 :
                             iNumMonster > 9   ? 2 : 1;

            // given the currently selected monster iActive. get the selected row
            int iSelPos = iActive;
            for( auto &ia : mSortCategory ) {
                int index = ia.first;
                if( index <= iSelPos ) {
                    ++iSelPos;
                } else {
                    break;
                }
            }
            // use selected row get the start row
            calcStartPos( iStartPos, iSelPos, iMaxRows, iMenuSize );

            // get first visible monster and category
            int iCurMon = iStartPos;
            auto CatSortIter = mSortCategory.cbegin();
            while( CatSortIter != mSortCategory.cend() && CatSortIter->first < iStartPos ) {
                ++CatSortIter;
                --iCurMon;
            }

            const auto endY = std::min<int>( iMaxRows, iMenuSize );
            for( int y = 0; y < endY; ++y ) {
                if( CatSortIter != mSortCategory.cend() ) {
                    const int iCurPos = iStartPos + y;
                    const int iCatPos = CatSortIter->first;
                    if( iCurPos == iCatPos ) {
                        const std::string &cat_name = Creature::get_attitude_ui_data( CatSortIter->second ).first;
                        mvwprintz( w_monsters, y, 1, c_magenta, cat_name );
                        ++CatSortIter;
                        continue;
                    }
                }
                // select current monster
                const auto critter = monster_list[iCurMon];
                const bool selected = iCurMon == iActive;
                ++iCurMon;
                if( critter->sees( g->u ) ) {
                    mvwprintz( w_monsters, y, 0, c_yellow, "!" );
                }
                bool is_npc = false;
                const monster *m = dynamic_cast<monster *>( critter );
                const npc     *p = dynamic_cast<npc *>( critter );

                if( m != nullptr ) {
                    mvwprintz( w_monsters, y, 1, selected ? c_light_green : c_white, m->name() );
                } else {
                    mvwprintz( w_monsters, y, 1, selected ? c_light_green : c_white, critter->disp_name() );
                    is_npc = true;
                }

                if( selected && !get_safemode().empty() ) {
                    for( int i = 1; i < width - 2; i++ ) {
                        mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2,
                                  i, BORDER_COLOR, LINE_OXOX ); // -
                    }
                    const std::string monName = is_npc ? get_safemode().npc_type_name() : m->name();

                    std::string sSafemode;
                    if( get_safemode().has_rule( monName, Creature::A_ANY ) ) {
                        sSafemode = _( "<R>emove from safemode Blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safemode Blacklist" );
                    }

                    shortcut_print( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 3,
                                    c_white, c_light_green, sSafemode );
                }

                nc_color color = c_white;
                std::string sText;

                if( m != nullptr ) {
                    m->get_HP_Bar( color, sText );
                } else {
                    std::tie( sText, color ) =
                        ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
                }
                mvwprintz( w_monsters, y, 22, color, sText );

                if( m != nullptr ) {
                    const auto att = m->get_attitude();
                    sText = att.first;
                    color = att.second;
                } else if( p != nullptr ) {
                    sText = npc_attitude_name( p->get_attitude() );
                    color = p->symbol_color();
                }
                mvwprintz( w_monsters, y, 28, color, sText );

                mvwprintz( w_monsters, y, width - ( 6 + numw ), ( selected ? c_light_green : c_light_gray ),
                           "%*d %s",
                           numw, rl_dist( u.pos(), critter->pos() ),
                           direction_name_short( direction_from( u.pos(), critter->pos() ) ).c_str() );
            }

            mvwprintz( w_monsters_border, 0, ( width / 2 ) - numw - 2, c_light_green, " %*d", numw,
                       iActive + 1 );
            wprintz( w_monsters_border, c_white, " / %*d ", numw, int( monster_list.size() ) );

            cCurMon = monster_list[iActive];

            werase( w_monster_info );
            cCurMon->print_info( w_monster_info, 1, 11, 1 );

            if( bVMonsterLookFire ) {
                mvwprintz( w_monsters, getmaxy( w_monsters ) - 1, 1, c_light_green, ctxt.press_x( "look" ) );
                wprintz( w_monsters, c_light_gray, " %s", _( "to look around" ) );

                if( rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                    wprintz( w_monsters, c_light_gray, "%s", " " );
                    wprintz( w_monsters, c_light_green, ctxt.press_x( "fire" ) );
                    wprintz( w_monsters, c_light_gray, " %s", _( "to shoot" ) );
                }
            }

            // Only redraw trail/terrain if x/y position changed or if keybinding menu erased it
            tripoint iActivePos = cCurMon->pos() - u.pos();
            if( ( !iLastActivePos || iActivePos != *iLastActivePos ) || action == "HELP_KEYBINDINGS" ) {
                iLastActivePos.emplace( iActivePos );
                centerlistview( iActivePos );
                draw_trail_to_square( iActivePos, false );
            }

            draw_scrollbar( w_monsters_border, iActive, iMaxRows, int( monster_list.size() ), 1 );
            wrefresh( w_monsters_border );
        }

        for( int j = 0; j < iInfoHeight - 1; j++ ) {
            mvwputch( w_monster_info_border, j, 0, c_light_gray, LINE_XOXO );
            mvwputch( w_monster_info_border, j, width - 1, c_light_gray, LINE_XOXO );
        }

        for( int j = 0; j < width - 1; j++ ) {
            mvwputch( w_monster_info_border, iInfoHeight - 1, j, c_light_gray, LINE_OXOX );
        }

        mvwputch( w_monster_info_border, iInfoHeight - 1, 0, c_light_gray, LINE_XXOO );
        mvwputch( w_monster_info_border, iInfoHeight - 1, width - 1, c_light_gray, LINE_XOOX );

        wrefresh( w_monsters );
        wrefresh( w_monster_info_border );
        wrefresh( w_monster_info );

        catacurses::refresh();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return game::vmenu_ret::QUIT;
}

std::vector<vehicle *> nearby_vehicles_for( const itype_id &ft )
{
    std::vector<vehicle *> result;
    for( auto &&p : g->m.points_in_radius( g->u.pos(), 1 ) ) { // *NOPAD*
        vehicle *const veh = veh_pointer_or_null( g->m.veh_at( p ) );
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

bool game::handle_liquid_from_ground( std::list<item>::iterator on_ground, const tripoint &pos,
                                      const int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    handle_liquid( *on_ground, nullptr, radius, &pos );
    if( on_ground->charges > 0 ) {
        return false;
    }
    m.i_at( pos ).erase( on_ground );
    return true;
}

bool game::handle_liquid_from_container( std::list<item>::iterator in_container, item &container,
        int radius )
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

extern void serialize_liquid_source( player_activity &act, const vehicle &veh, const int part_num,
                                     const item &liquid );
extern void serialize_liquid_source( player_activity &act, const tripoint &pos,
                                     const item &liquid );
extern void serialize_liquid_source( player_activity &act, const monster &mon, const item &liquid );

extern void serialize_liquid_target( player_activity &act, const vehicle &veh );
extern void serialize_liquid_target( player_activity &act, int container_item_pos );
extern void serialize_liquid_target( player_activity &act, const tripoint &pos );
extern void serialize_liquid_target( player_activity &act, const monster &mon );

bool game::get_liquid_target( item &liquid, item *const source, const int radius,
                              const tripoint *const source_pos,
                              const vehicle *const source_veh,
                              const monster *const source_mon,
                              liquid_dest_opt &target )
{
    if( !liquid.made_of_from_type( LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    uilist menu;

    const std::string liquid_name = liquid.display_name( liquid.charges );
    if( source_pos != nullptr ) {
        menu.text = string_format( _( "What to do with the %s from %s?" ), liquid_name.c_str(),
                                   m.name( *source_pos ).c_str() );
    } else if( source_veh != nullptr ) {
        menu.text = string_format( _( "What to do with the %s from the %s?" ), liquid_name.c_str(),
                                   source_veh->name.c_str() );
    } else if( source_mon != nullptr ) {
        menu.text = string_format( _( "What to do with the %s from the %s?" ), liquid_name.c_str(),
                                   source_mon->get_name().c_str() );
    } else {
        menu.text = string_format( _( "What to do with the %s?" ), liquid_name.c_str() );
    }
    std::vector<std::function<void()>> actions;

    if( u.can_consume( liquid ) && !source_mon ) {
        menu.addentry( -1, true, 'e', _( "Consume it" ) );
        actions.emplace_back( [ & ]() {
            target.dest_opt = LD_CONSUME;
        } );
    }
    // This handles containers found anywhere near the player, including on the map and in vehicle storage.
    menu.addentry( -1, true, 'c', _( "Pour into a container" ) );
    actions.emplace_back( [ & ]() {
        target.item_loc = game_menus::inv::container_for( u, liquid, radius );
        item *const cont = target.item_loc.get_item();

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        if( source != nullptr && cont == source ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        target.dest_opt = LD_ITEM;
    } );
    // This handles liquids stored in vehicle parts directly (e.g. tanks).
    std::set<vehicle *> opts;
    for( const auto &e : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        auto veh = veh_pointer_or_null( g->m.veh_at( e ) );
        if( veh && std::any_of( veh->parts.begin(), veh->parts.end(), [&liquid]( const vehicle_part & pt ) {
        return pt.can_reload( liquid );
        } ) ) {
            opts.insert( veh );
        }
    }
    for( auto veh : opts ) {
        if( veh == source_veh ) {
            continue;
        }
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill nearby vehicle %s" ), veh->name.c_str() );
        actions.emplace_back( [ &, veh]() {
            target.veh = veh;
            target.dest_opt = LD_VEH;
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
        actions.emplace_back( [ &, target_pos ]() {
            target.pos = target_pos;
            target.dest_opt = LD_KEG;
        } );
    }

    menu.addentry( -1, true, 'g', _( "Pour on the ground" ) );
    actions.emplace_back( [ & ]() {
        // From infinite source to the ground somewhere else. The target has
        // infinite space and the liquid can not be used from there anyway.
        if( liquid.has_infinite_charges() && source_pos != nullptr ) {
            add_msg( m_info, _( "Clearing out the %s would take forever." ), m.name( *source_pos ).c_str() );
            return;
        }

        const std::string liqstr = string_format( _( "Pour %s where?" ), liquid_name.c_str() );

        refresh_all();
        const cata::optional<tripoint> target_pos_ = choose_adjacent( liqstr );
        if( !target_pos_ ) {
            return;
        }
        target.pos = *target_pos_;

        if( source_pos != nullptr && *source_pos == target.pos ) {
            add_msg( m_info, _( "That's where you took it from!" ) );
            return;
        }
        if( !m.can_put_items_ter_furn( target.pos ) ) {
            add_msg( m_info, _( "You can't pour there!" ) );
            return;
        }
        target.dest_opt = LD_GROUND;
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
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= actions.size() ) {
        add_msg( _( "Never mind." ) );
        // Explicitly canceled all options (container, drink, pour).
        return false;
    }

    actions[menu.ret]();
    return true;
}

bool game::perform_liquid_transfer( item &liquid, const tripoint *const source_pos,
                                    const vehicle *const source_veh, const int part_num,
                                    const monster *const source_mon, liquid_dest_opt &target )
{
    bool transfer_ok = false;
    if( !liquid.made_of_from_type( LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return transfer_ok;
    }

    const auto create_activity = [&]() {
        if( source_veh != nullptr ) {
            u.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( u.activity, *source_veh, part_num, liquid );
            return true;
        } else if( source_pos != nullptr ) {
            u.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( u.activity, *source_pos, liquid );
            return true;
        } else if( source_mon != nullptr ) {
            return false;
        } else {
            return false;
        }
    };

    switch( target.dest_opt ) {
        case LD_CONSUME:
            u.consume_item( liquid );
            transfer_ok = true;
            break;
        case LD_ITEM: {
            item *const cont = target.item_loc.get_item();
            const int item_index = u.get_item_position( cont );
            // Currently activities can only store item position in the players inventory,
            // not on ground or similar. TODO: implement storing arbitrary container locations.
            if( item_index != INT_MIN && create_activity() ) {
                serialize_liquid_target( u.activity, item_index );
            } else if( u.pour_into( *cont, liquid ) ) {
                if( cont->needs_processing() ) {
                    // Polymorphism fail, have to introspect into the type to set the target container as active.
                    switch( target.item_loc.where() ) {
                        case item_location::type::map:
                            m.make_active( target.item_loc );
                            break;
                        case item_location::type::vehicle:
                            m.veh_at( target.item_loc.position() )->vehicle().make_active( target.item_loc );
                            break;
                        case item_location::type::character:
                        case item_location::type::invalid:
                            break;
                    }
                }
                u.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        }
        case LD_VEH:
            if( target.veh == nullptr ) {
                break;
            }
            if( create_activity() ) {
                serialize_liquid_target( u.activity, *target.veh );
            } else if( u.pour_into( *target.veh, liquid ) ) {
                u.mod_moves( -1000 ); // consistent with veh_interact::do_refill activity
            }
            transfer_ok = true;
            break;
        case LD_KEG:
        case LD_GROUND:
            if( create_activity() ) {
                serialize_liquid_target( u.activity, target.pos );
            } else {
                if( target.dest_opt == LD_KEG ) {
                    iexamine::pour_into_keg( target.pos, liquid );
                } else {
                    m.add_item_or_charges( target.pos, liquid );
                    liquid.charges = 0;
                }
                u.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        case LD_NULL:
        default:
            break;
    }
    return transfer_ok;
}

bool game::handle_liquid( item &liquid, item *const source, const int radius,
                          const tripoint *const source_pos,
                          const vehicle *const source_veh, const int part_num,
                          const monster *const source_mon )
{
    if( liquid.made_of_from_type( SOLID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }
    if( !liquid.made_of( LIQUID ) ) {
        add_msg( _( "The %s froze solid before you could finish." ), liquid.tname().c_str() );
        return false;
    }
    struct liquid_dest_opt liquid_target;
    if( get_liquid_target( liquid, source, radius, source_pos, source_veh, source_mon,
                           liquid_target ) ) {
        return perform_liquid_transfer( liquid, source_pos, source_veh, part_num, source_mon,
                                        liquid_target );
    }
    return false;
}

void game::drop()
{
    u.drop( game_menus::inv::multidrop( u ), u.pos() );
}

void game::drop_in_direction()
{
    if( const cata::optional<tripoint> pnt = choose_adjacent( _( "Drop where?" ) ) ) {
        refresh_all();
        u.drop( game_menus::inv::multidrop( u ), *pnt );
    }
}

void game::plthrow( int pos, const cata::optional<tripoint> &blind_throw_from_pos )
{
    if( u.has_active_mutation( trait_SHELL2 ) ) {
        add_msg( m_info, _( "You can't effectively throw while you're in your shell." ) );
        return;
    }

    if( pos == INT_MIN ) {
        pos = inv_for_all( _( "Throw item" ), _( "You don't have any items to throw." ) );
        refresh_all();
    }

    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item thrown = u.i_at( pos );
    int range = u.throw_range( thrown );
    if( range < 0 ) {
        add_msg( m_info, _( "You don't have that item." ) );
        return;
    } else if( range == 0 ) {
        add_msg( m_info, _( "That is too heavy to throw." ) );
        return;
    }

    if( pos == -1 && thrown.has_flag( "NO_UNWIELD" ) ) {
        // pos == -1 is the weapon, NO_UNWIELD is used for bio_claws_weapon
        add_msg( m_info, _( "That's part of your body, you can't throw that!" ) );
        return;
    }

    if( u.has_effect( effect_relax_gas ) ) {
        if( one_in( 5 ) ) {
            add_msg( m_good, _( "You concentrate mightily, and your body obeys!" ) );
        } else {
            u.moves -= rng( 2, 5 ) * 10;
            add_msg( m_bad, _( "You can't muster up the effort to throw anything..." ) );
            return;
        }
    }

    // you must wield the item to throw it
    if( pos != -1 ) {
        u.i_rem( pos );
        if( !u.wield( thrown ) ) {
            // We have to remove the item before checking for wield because it
            // can invalidate our pos index.  Which means we have to add it
            // back if the player changed their mind about unwielding their
            // current item
            u.i_add( thrown );
            return;
        }
    }

    // Shift our position to our "peeking" position, so that the UI
    // for picking a throw point lets us target the location we couldn't
    // otherwise see.
    const tripoint original_player_position = u.pos();
    if( blind_throw_from_pos ) {
        u.setpos( *blind_throw_from_pos );
        draw_ter();
    }

    temp_exit_fullscreen();
    m.draw( w_terrain, u.pos() );

    const target_mode throwing_target_mode = blind_throw_from_pos ? TARGET_MODE_THROW_BLIND :
            TARGET_MODE_THROW;
    // target_ui() sets x and y, or returns empty vector if we canceled (by pressing Esc)
    std::vector<tripoint> trajectory = target_handler().target_ui( u, throwing_target_mode, &thrown,
                                       range );

    // If we previously shifted our position, put ourselves back now that we've picked our target.
    if( blind_throw_from_pos ) {
        u.setpos( original_player_position );
    }

    if( trajectory.empty() ) {
        return;
    }

    if( thrown.count_by_charges() && thrown.charges > 1 )  {
        u.i_at( -1 ).charges--;
        thrown.charges = 1;
    } else {
        u.i_rem( -1 );
    }
    u.throw_item( trajectory.back(), thrown, blind_throw_from_pos );
    reenter_fullscreen();
}

// @todo: Move data/functions related to targeting out of game class
bool game::plfire_check( const targeting_data &args )
{
    // @todo: Make this check not needed
    if( args.relevant == nullptr ) {
        debugmsg( "Can't plfire_check a null" );
        return false;
    }

    if( u.has_effect( effect_relax_gas ) ) {
        if( one_in( 5 ) ) {
            add_msg( m_good, _( "Your eyes steel, and you raise your weapon!" ) );
        } else {
            u.moves -= rng( 2, 5 ) * 10;
            add_msg( m_bad, _( "You can't fire your weapon, it's too heavy..." ) );
            // break a possible loop when aiming
            if( u.activity ) {
                u.cancel_activity();
            }

            return false;
        }
    }

    item &weapon = *args.relevant;
    if( weapon.is_gunmod() ) {
        add_msg( m_info,
                 _( "The %s must be attached to a gun, it can not be fired separately." ),
                 weapon.tname().c_str() );
        return false;
    }

    auto gun = weapon.gun_current_mode();
    // check that a valid mode was returned and we are able to use it
    if( !( gun && u.can_use( *gun ) ) ) {
        add_msg( m_info, _( "You can no longer fire." ) );
        return false;
    }

    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( vp && vp->vehicle().player_in_control( u ) && gun->is_two_handed( u ) ) {
        add_msg( m_info, _( "You need a free arm to drive!" ) );
        return false;
    }

    if( !weapon.is_gun() ) {
        // The weapon itself isn't a gun, this weapon is not fireable.
        return false;
    }

    if( gun->has_flag( "FIRE_TWOHAND" ) && ( !u.has_two_arms() ||
            u.worn_with_flag( "RESTRICT_HANDS" ) ) ) {
        add_msg( m_info, _( "You need two free hands to fire your %s." ), gun->tname().c_str() );
        return false;
    }

    // Skip certain checks if we are directly firing a vehicle turret
    if( args.mode != TARGET_MODE_TURRET_MANUAL ) {
        if( !gun->ammo_sufficient() && !gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
            if( !gun->ammo_remaining() ) {
                add_msg( m_info, _( "You need to reload!" ) );
            } else {
                add_msg( m_info, _( "Your %s needs %i charges to fire!" ),
                         gun->tname().c_str(), gun->ammo_required() );
            }
            return false;
        }

        if( gun->get_gun_ups_drain() > 0 ) {
            const int ups_drain = gun->get_gun_ups_drain();
            const int adv_ups_drain = std::max( 1, ups_drain * 3 / 5 );

            if( !( u.has_charges( "UPS_off", ups_drain ) ||
                   u.has_charges( "adv_UPS_off", adv_ups_drain ) ||
                   ( u.has_active_bionic( bionic_id( "bio_ups" ) ) && u.power_level >= ups_drain ) ) ) {
                add_msg( m_info,
                         _( "You need a UPS with at least %d charges or an advanced UPS with at least %d charges to fire that!" ),
                         ups_drain, adv_ups_drain );
                return false;
            }
        }

        if( gun->has_flag( "MOUNTED_GUN" ) ) {
            const bool v_mountable = static_cast<bool>( m.veh_at( u.pos() ).part_with_feature( "MOUNTABLE",
                                     true ) );
            bool t_mountable = m.has_flag_ter_or_furn( "MOUNTABLE", u.pos() );
            if( !t_mountable && !v_mountable ) {
                add_msg( m_info,
                         _( "You must stand near acceptable terrain or furniture to use this weapon. A table, a mound of dirt, a broken window, etc." ) );
                return false;
            }
        }
    }

    return true;
}

bool game::plfire()
{
    targeting_data args = u.get_targeting_data();
    if( !args.relevant ) {
        // args missing a valid weapon, this shouldn't happen.
        debugmsg( "Player tried to fire a null weapon." );
        return false;
    }
    // If we were wielding this weapon when we started aiming, make sure we still are.
    bool lost_weapon = ( args.held && &u.weapon != args.relevant );
    bool failed_check = !plfire_check( args );
    if( lost_weapon || failed_check ) {
        u.cancel_activity();
        return false;
    }

    int reload_time = 0;
    gun_mode gun = args.relevant->gun_current_mode();

    // @todo: move handling "RELOAD_AND_SHOOT" flagged guns to a separate function.
    if( gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
        if( !gun->ammo_remaining() ) {
            item::reload_option opt = u.ammo_location &&
                                      gun->can_reload_with( u.ammo_location->typeId() ) ? item::reload_option( &u, args.relevant,
                                              args.relevant, u.ammo_location.clone() ) : u.select_ammo( *gun );
            if( !opt ) {
                // Menu canceled
                return false;
            }
            reload_time += opt.moves();
            if( !gun->reload( u, std::move( opt.ammo ), 1 ) ) {
                // Reload not allowed
                return false;
            }

            // Burn 2x the strength required to fire in stamina.
            u.mod_stat( "stamina", gun->get_min_str() * -2 );
            // At low stamina levels, firing starts getting slow.
            int sta_percent = ( 100 * u.stamina ) / u.get_stamina_max();
            reload_time += ( sta_percent < 25 ) ? ( ( 25 - sta_percent ) * 2 ) : 0;

            // Update targeting data to include ammo's range bonus
            args.range = gun.target->gun_range( &u );
            args.ammo = gun->ammo_data();
            u.set_targeting_data( args );

            refresh_all();
        }
    }

    temp_exit_fullscreen();
    m.draw( w_terrain, u.pos() );
    std::vector<tripoint> trajectory = target_handler().target_ui( u, args );

    if( trajectory.empty() ) {
        bool not_aiming = u.activity.id() != activity_id( "ACT_AIM" );
        if( not_aiming && gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
            const auto previous_moves = u.moves;
            unload( *gun );
            // Give back time for unloading as essentially nothing has been done.
            // Note that reload_time has not been applied either.
            u.moves = previous_moves;
        }
        reenter_fullscreen();
        return false;
    }
    draw_ter(); // Recenter our view
    wrefresh( w_terrain );

    int shots = 0;

    u.moves -= reload_time;
    // @todo: add check for TRIGGERHAPPY
    if( args.pre_fire ) {
        args.pre_fire( shots );
    }
    shots = u.fire_gun( trajectory.back(), gun.qty, *gun );
    if( args.post_fire ) {
        args.post_fire( shots );
    }

    if( shots && args.power_cost ) {
        u.charge_power( -args.power_cost * shots );
    }
    reenter_fullscreen();
    return shots != 0;
}

bool game::plfire( item &weapon, int bp_cost )
{
    // @todo: bionic power cost of firing should be derived from a value of the relevant weapon.
    gun_mode gun = weapon.gun_current_mode();
    // gun can be null if the item is an unattached gunmod
    if( !gun ) {
        add_msg( m_info, _( "The %s can't be fired in its current state." ), weapon.tname().c_str() );
        return false;
    }

    targeting_data args = {
        TARGET_MODE_FIRE, &weapon, gun.target->gun_range( &u ),
        bp_cost, &u.weapon == &weapon, gun->ammo_data(),
        target_callback(), target_callback(),
        firing_callback(), firing_callback()
    };
    u.set_targeting_data( args );
    return plfire();
}

// Used to set up the first Hotkey in the display set
int get_initial_hotkey( const size_t menu_index )
{
    int hotkey = -1;
    if( menu_index == 0 ) {
        const long butcher_key = inp_mngr.get_previously_pressed_key();
        if( butcher_key != 0 ) {
            hotkey = butcher_key;
        }
    }
    return hotkey;
}

// Returns a vector of pairs.
//    Pair.first is the first items index with a unique tname.
//    Pair.second is the number of equivalent items per unique tname
// There are options for optimization here, but the function is hit infrequently
// enough that optimizing now is not a useful time expenditure.
const std::vector<std::pair<int, int>> generate_butcher_stack_display(
                                        map_stack &items, const std::vector<int> &indices )
{
    std::vector<std::pair<int, int>> result;
    std::vector<std::string> result_strings;
    result.reserve( indices.size() );
    result_strings.reserve( indices.size() );

    for( const size_t ndx : indices ) {
        const item &it = items[ ndx ];

        const std::string tname = it.tname();
        size_t s = 0;
        // Search for the index with a string equivalent to tname
        for( ; s < result_strings.size(); ++s ) {
            if( result_strings[s] == tname ) {
                break;
            }
        }
        // If none is found, this is a unique tname so we need to add
        // the tname to string vector, and make an empty result pair.
        // Has the side effect of making 's' a valid index
        if( s == result_strings.size() ) {
            // make a new entry
            result.push_back( std::make_pair<int, int>( static_cast<int>( ndx ), 0 ) );
            // Also push new entry string
            result_strings.push_back( tname );
        }
        // Increase count result pair at index s
        ++result[s].second;
    }

    return result;
}

// Corpses are always individual items
// Just add them individually to the menu
void add_corpses( uilist &menu, map_stack &items,
                  const std::vector<int> &indices, size_t &menu_index )
{
    int hotkey = get_initial_hotkey( menu_index );

    for( const auto index : indices ) {
        const item &it = items[index];
        menu.addentry( menu_index++, true, hotkey, it.get_mtype()->nname() );
        hotkey = -1;
    }
}

// Salvagables stack so we need to pass in a stack vector rather than an item index vector
void add_salvagables( uilist &menu, map_stack &items,
                      const std::vector<std::pair<int, int>> &stacks, size_t &menu_index,
                      const salvage_actor &salvage_iuse )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = items[ stack.first ];

            //~ Name and number of items listed for cutting up
            const auto &msg = string_format( pgettext( "butchery menu", "Cut up %s (%d)" ),
                                             it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( salvage_iuse.time_to_cut_up( it ) / 100 ) ) );
            hotkey = -1;
        }
    }
}

// Disassemblables stack so we need to pass in a stack vector rather than an item index vector
void add_disassemblables( uilist &menu, map_stack &items,
                          const std::vector<std::pair<int, int>> &stacks, size_t &menu_index )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = items[ stack.first ];

            //~ Name, number of items and time to complete disassembling
            const auto &msg = string_format( pgettext( "butchery menu", "%s (%d)" ),
                                             it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( recipe_dictionary::get_uncraft(
                                       it.typeId() ).time / 100 ) ) );
            hotkey = -1;
        }
    }
}

// Butchery sub-menu and time calculation
void butcher_submenu( map_stack &items, const std::vector<int> &corpses, int corpse = -1 )
{
    auto cut_time = [&]( enum butcher_type bt ) {
        int time_to_cut = 0;
        if( corpse != -1 ) {
            time_to_cut = butcher_time_to_cut( g->u, items[corpses[corpse]], bt );
        } else {
            for( int i : corpses ) {
                time_to_cut += butcher_time_to_cut( g->u, items[i], bt );
            }
        }
        return to_string_clipped( time_duration::from_turns( time_to_cut / 100 ) );
    };
    uilist smenu;
    smenu.desc_enabled = true;
    smenu.text = _( "Choose type of butchery:" );
    smenu.addentry_col( BUTCHER, true, 'B', _( "Quick butchery" ), cut_time( BUTCHER ),
                        _( "This technique is used when you are in a hurry, but still want to harvest something from the corpse.  Yields are lower as you don't try to be precise, but it's useful if you don't want to set up a workshop.  Prevents zombies from raising." ) );
    smenu.addentry_col( BUTCHER_FULL, true, 'b', _( "Full butchery" ), cut_time( BUTCHER_FULL ),
                        _( "This technique is used to properly butcher a corpse, and requires a rope & a tree or a butchering rack, a flat surface (for ex. a table, a leather tarp, etc.) and good tools.  Yields are plentiful and varied, but it is time consuming." ) );
    smenu.addentry_col( F_DRESS, true, 'f', _( "Field dress corpse" ), cut_time( F_DRESS ),
                        _( "Technique that involves removing internal organs and viscera to protect the corpse from rotting from inside. Yields internal organs. Carcass will be lighter and will stay fresh longer.  Can be combined with other methods for better effects." ) );
    smenu.addentry_col( SKIN, true, 's', ( "Skin corpse" ), cut_time( SKIN ),
                        _( "Skinning a corpse is an involved and careful process that usually takes some time.  You need skill and an appropriately sharp and precise knife to do a good job.  Some corpses are too small to yield a full-sized hide and will instead produce scraps that can be used in other ways." ) );
    smenu.addentry_col( QUARTER, true, 'k', _( "Quarter corpse" ), cut_time( QUARTER ),
                        _( "By quartering a previously field dressed corpse you will acquire four parts with reduced weight and volume.  It may help in transporting large game.  This action destroys skin, hide, pelt, etc., so don't use it if you want to harvest them later." ) );
    smenu.addentry_col( DISMEMBER, true, 'm', _( "Dismember corpse" ), cut_time( DISMEMBER ),
                        _( "If you're aiming to just destroy a body outright, and don't care about harvesting it, dismembering it will hack it apart in a very short amount of time, but yield little to no usable flesh." ) );
    smenu.addentry_col( DISSECT, true, 'd', _( "Dissect corpse" ), cut_time( DISSECT ),
                        _( "By careful dissection of the corpse, you will examine it for possible bionic implants, and harvest them if possible.  Requires scalpel-grade cutting tools, ruins corpse, and consumes a lot of time.  Your medical knowledge is most useful here." ) );
    smenu.query();
    switch( smenu.ret ) {
        case BUTCHER:
            g->u.assign_activity( activity_id( "ACT_BUTCHER" ), 0, -1 );
            break;
        case BUTCHER_FULL:
            g->u.assign_activity( activity_id( "ACT_BUTCHER_FULL" ), 0, -1 );
            break;
        case F_DRESS:
            g->u.assign_activity( activity_id( "ACT_FIELD_DRESS" ), 0, -1 );
            break;
        case SKIN:
            g->u.assign_activity( activity_id( "ACT_SKIN" ), 0, -1 );
            break;
        case QUARTER:
            g->u.assign_activity( activity_id( "ACT_QUARTER" ), 0, -1 );
            break;
        case DISMEMBER:
            g->u.assign_activity( activity_id( "ACT_DISMEMBER" ), 0, -1 );
            break;
        case DISSECT:
            g->u.assign_activity( activity_id( "ACT_DISSECT" ), 0, -1 );
            break;
        default:
            return;
    }
}

void game::butcher()
{
    static const std::string salvage_string = "salvage";
    if( u.controlling_vehicle ) {
        add_msg( m_info, _( "You can't butcher while driving!" ) );
        return;
    }

    const int factor = u.max_quality( quality_id( "BUTCHER" ) );
    const int factorD = u.max_quality( quality_id( "CUT_FINE" ) );
    static const char *no_knife_msg = _( "You don't have a butchering tool." );
    static const char *no_corpse_msg = _( "There are no corpses here to butcher." );

    //You can't butcher on sealed terrain- you have to smash/shovel/etc it open first
    if( m.has_flag( "SEALED", u.pos() ) ) {
        if( m.sees_some_items( u.pos(), u ) ) {
            add_msg( m_info, _( "You can't access the items here." ) );
        } else if( factor > INT_MIN || factorD > INT_MIN ) {
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
    auto items = m.i_at( u.pos() );
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
        salvage_iuse = dynamic_cast<const salvage_actor *>(
                           usable->get_use( salvage_string )->get_actor_ptr() );
    }

    // Reserve capacity for each to hold entire item set if necessary to prevent
    // reallocations later on
    corpses.reserve( items.size() );
    salvageables.reserve( items.size() );
    disassembles.reserve( items.size() );

    // Split into corpses, disassemble-able, and salvageable items
    // It's not much additional work to just generate a corpse list and
    // clear it later, but does make the splitting process nicer.
    for( size_t i = 0; i < items.size(); ++i ) {
        if( items[i].is_corpse() ) {
            corpses.push_back( i );
        } else {
            if( ( salvage_tool_index != INT_MIN ) && salvage_iuse->valid_to_cut_up( items[i] ) ) {
                salvageables.push_back( i );
            }
            if( u.can_disassemble( items[i], crafting_inv ).success() ) {
                disassembles.push_back( i );
            } else if( first_item_without_tools == nullptr ) {
                first_item_without_tools = &items[i];
            }
        }
    }

    // Clear corpses if butcher and dissect factors are INT_MIN
    if( factor == INT_MIN && factorD == INT_MIN ) {
        corpses.clear();
    }

    if( corpses.empty() && disassembles.empty() && salvageables.empty() ) {
        if( factor > INT_MIN || factorD > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }

        if( first_item_without_tools != nullptr ) {
            add_msg( m_info, _( "You don't have the necessary tools to disassemble any items here." ) );
            // Just for the "You need x to disassemble y" messages
            const auto ret = u.can_disassemble( *first_item_without_tools, crafting_inv );
            if( !ret.success() ) {
                add_msg( m_info, "%s", ret.c_str() );
            }
        }
        return;
    }

    Creature *hostile_critter = is_hostile_very_close();
    if( hostile_critter != nullptr ) {
        if( !query_yn( _( "You see %s nearby! Start butchering anyway?" ),
                       hostile_critter->disp_name().c_str() ) ) {
            return;
        }
    }

    // Magic indices for special butcher options
    enum : int {
        MULTISALVAGE =  MAX_ITEM_IN_SQUARE + 1,
        MULTIBUTCHER,
        MULTIDISASSEMBLE_ONE,
        MULTIDISASSEMBLE_ALL,
        NUM_BUTCHER_ACTIONS
    };
    // What are we butchering (ie. which vector to pick indices from)
    enum {
        BUTCHER_CORPSE,
        BUTCHER_DISASSEMBLE,
        BUTCHER_SALVAGE,
        BUTCHER_OTHER // For multisalvage etc.
    } butcher_select = BUTCHER_CORPSE;
    // Index to std::vector of indices...
    int indexer_index = 0;

    // Generate the indexed stacks so we can display them nicely
    const auto disassembly_stacks = generate_butcher_stack_display( items, disassembles );
    const auto salvage_stacks = generate_butcher_stack_display( items, salvageables );
    // Always ask before cutting up/disassembly, but not before butchery
    size_t ret = 0;
    if( corpses.size() > 1 || !disassembles.empty() || !salvageables.empty() ) {
        uilist kmenu;
        kmenu.text = _( "Choose corpse to butcher / item to disassemble" );

        size_t i = 0;
        // Add corpses, disassembleables, and salvagables to the UI
        add_corpses( kmenu, items, corpses, i );
        add_disassemblables( kmenu, items, disassembly_stacks, i );
        add_salvagables( kmenu, items, salvage_stacks, i, *salvage_iuse );

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b', _( "Butcher everything" ) );
        }
        if( disassembles.size() > 1 ) {
            int time_to_disassemble = 0;
            int time_to_disassemble_all = 0;
            for( const auto &stack : disassembly_stacks ) {
                const item &it = items[ stack.first ];
                const int time = recipe_dictionary::get_uncraft( it.typeId() ).time;
                time_to_disassemble += time;
                time_to_disassemble_all += time * stack.second;
            }

            kmenu.addentry_col( MULTIDISASSEMBLE_ONE, true, 'D', _( "Disassemble everything once" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble / 100 ) ) );
            kmenu.addentry_col( MULTIDISASSEMBLE_ALL, true, 'd', _( "Disassemble everything" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble_all / 100 ) ) );
        }
        if( salvageables.size() > 1 ) {
            int time_to_salvage = 0;
            for( const auto &stack : salvage_stacks ) {
                const item &it = items[ stack.first ];
                time_to_salvage += salvage_iuse->time_to_cut_up( it ) * stack.second;
            }

            kmenu.addentry_col( MULTISALVAGE, true, 'z', _( "Cut up everything" ),
                                to_string_clipped( time_duration::from_turns( time_to_salvage / 100 ) ) );
        }

        kmenu.query();

        if( kmenu.ret < 0 || kmenu.ret >= NUM_BUTCHER_ACTIONS ) {
            return;
        }

        ret = static_cast<size_t>( kmenu.ret );
        if( ret >= MULTISALVAGE && ret < NUM_BUTCHER_ACTIONS ) {
            butcher_select = BUTCHER_OTHER;
            indexer_index = ret;
        } else if( ret < corpses.size() ) {
            butcher_select = BUTCHER_CORPSE;
            indexer_index = ret;
        } else if( ret < corpses.size() + disassembly_stacks.size() ) {
            butcher_select = BUTCHER_DISASSEMBLE;
            indexer_index = ret - corpses.size();
        } else if( ret < corpses.size() + disassembly_stacks.size() + salvage_stacks.size() ) {
            butcher_select = BUTCHER_SALVAGE;
            indexer_index = ret - corpses.size() - disassembly_stacks.size();
        } else {
            debugmsg( "Invalid butchery index: %d", ret );
            return;
        }
    }

    if( !u.has_morale_to_craft() ) {
        if( butcher_select == BUTCHER_CORPSE || indexer_index == MULTIBUTCHER ) {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of guts and blood on your hands convinces you to turn away." ) );
        } else {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of work stops you before you begin." ) );
        }
        return;
    }
    const auto helpers = u.get_crafting_helpers();
    for( const npc *np : helpers ) {
        add_msg( m_info, _( "%s helps with this task..." ), np->name.c_str() );
        break;
    }
    switch( butcher_select ) {
        case BUTCHER_OTHER:
            switch( indexer_index ) {
                case MULTISALVAGE:
                    u.assign_activity( activity_id( "ACT_LONGSALVAGE" ), 0, salvage_tool_index );
                    break;
                case MULTIBUTCHER:
                    butcher_submenu( items, corpses );
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
                    debugmsg( "Invalid butchery type: %d", indexer_index );
                    return;
            }
            break;
        case BUTCHER_CORPSE: {
            butcher_submenu( items, corpses, indexer_index );
            draw_ter();
            wrefresh( w_terrain );
            u.activity.values.push_back( corpses[indexer_index] );
        }
        break;
        case BUTCHER_DISASSEMBLE: {
            // Pick index of first item in the disassembly stack
            size_t index = disassembly_stacks[indexer_index].first;
            u.disassemble( items[index], index, true );
        }
        break;
        case BUTCHER_SALVAGE: {
            // Pick index of first item in the salvage stack
            size_t index = salvage_stacks[indexer_index].first;
            item_location item_loc( map_cursor( u.pos() ), &items[index] );
            salvage_iuse->cut_up( u, *salvage_tool, item_loc );
        }
        break;
    }
}

void game::eat( int pos )
{
    if( ( u.has_active_mutation( trait_RUMINANT ) || u.has_active_mutation( trait_GRAZER ) ) &&
        ( m.ter( u.pos() ) == t_underbrush || m.ter( u.pos() ) == t_shrub ) ) {
        if( u.get_hunger() < 20 ) {
            add_msg( _( "You're too full to eat the leaves from the %s." ), m.ter( u.pos() )->name() );
            return;
        } else {
            u.moves -= 400;
            u.mod_hunger( -20 );
            add_msg( _( "You eat the leaves from the %s." ), m.ter( u.pos() )->name() );
            m.ter_set( u.pos(), t_grass );
            return;
        }
    }
    if( u.has_active_mutation( trait_GRAZER ) && ( m.ter( u.pos() ) == t_grass ||
            m.ter( u.pos() ) == t_grass_long || m.ter( u.pos() ) == t_grass_tall ) ) {
        if( u.get_hunger() < 8 ) {
            add_msg( _( "You're too full to graze." ) );
            return;
        } else {
            u.moves -= 400;
            add_msg( _( "You graze on the %s." ), m.ter( u.pos() )->name() );
            u.mod_hunger( -8 );
            if( m.ter( u.pos() ) == t_grass_tall ) {
                m.ter_set( u.pos(), t_grass_long );
            } else if( m.ter( u.pos() ) == t_grass_long ) {
                m.ter_set( u.pos(), t_grass );
            } else {
                m.ter_set( u.pos(), t_dirt );
            }
            return;
        }
    }
    if( u.has_active_mutation( trait_GRAZER ) ) {
        if( m.ter( u.pos() ) == t_grass_golf ) {
            add_msg( _( "This grass is too short to graze." ) );
            return;
        } else if( m.ter( u.pos() ) == t_grass_dead ) {
            add_msg( _( "This grass is dead and too mangled for you to graze." ) );
            return;
        } else if( m.ter( u.pos() ) == t_grass_white ) {
            add_msg( _( "This grass is tainted with paint and thus inedible." ) );
            return;
        }
    }

    if( pos != INT_MIN ) {
        u.consume( pos );
        return;
    }

    auto item_loc = game_menus::inv::consume( u );
    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item *it = item_loc.get_item();
    pos = u.get_item_position( it );
    if( pos != INT_MIN ) {
        u.consume( pos );

    } else if( u.consume_item( *it ) ) {
        if( it->is_food_container() ) {
            it->contents.erase( it->contents.begin() );
            add_msg( _( "You leave the empty %s." ), it->tname().c_str() );
        } else {
            item_loc.remove_item();
        }
    }
}

void game::change_side( int pos )
{
    if( pos == INT_MIN ) {
        pos = inv_for_filter( _( "Change side for item" ), [&]( const item & it ) {
            return u.is_worn( it ) && it.is_sided();
        }, _( "You don't have sided items worn." ) );
    }
    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    u.change_side( pos );
}

void game::reload( int pos, bool prompt )
{
    item_location loc( u, &u.i_at( pos ) );
    reload( loc, prompt );
}

void game::reload( item_location &loc, bool prompt )
{
    item *it = loc.get_item();
    bool use_loc = true;
    if( !it->has_flag( "ALLOWS_REMOTE_USE" ) ) {
        it = &u.i_at( loc.obtain( u ) );
        use_loc = false;
    }

    // bows etc do not need to reload. select favorite ammo for them instead
    if( it->has_flag( "RELOAD_AND_SHOOT" ) ) {
        item::reload_option opt = u.select_ammo( *it, prompt );
        if( !opt ) {
            return;
        } else if( u.ammo_location && opt.ammo == u.ammo_location ) {
            u.ammo_location = item_location();
        } else {
            u.ammo_location = opt.ammo.clone();
        }
        return;
    }

    // for holsters and ammo pouches try to reload any contained item
    if( it->type->can_use( "holster" ) && !it->contents.empty() ) {
        it = &it->contents.front();
    }

    switch( u.rate_action_reload( *it ) ) {
        case HINT_IFFY:
            if( ( it->is_ammo_container() || it->is_magazine() ) && it->ammo_remaining() > 0 &&
                it->ammo_remaining() == it->ammo_capacity() ) {
                add_msg( m_info, _( "The %s is already fully loaded!" ), it->tname().c_str() );
                return;
            }
            if( it->is_ammo_belt() ) {
                const auto &linkage = it->type->magazine->linkage;
                if( linkage && !u.has_charges( *linkage, 1 ) ) {
                    add_msg( m_info, _( "You need at least one %s to reload the %s!" ),
                             item::nname( *linkage, 1 ), it->tname() );
                    return;
                }
            }
            if( it->is_watertight_container() && it->is_container_full() ) {
                add_msg( m_info, _( "The %s is already full!" ), it->tname().c_str() );
                return;
            }

        // intentional fall-through

        case HINT_CANT:
            add_msg( m_info, _( "You can't reload a %s!" ), it->tname().c_str() );
            return;

        case HINT_GOOD:
            break;
    }

    // for bandoliers we currently defer to iuse_actor methods
    if( it->is_bandolier() ) {
        auto ptr = dynamic_cast<const bandolier_actor *>
                   ( it->type->get_use( "bandolier" )->get_actor_ptr() );
        ptr->reload( u, *it );
        return;
    }

    item::reload_option opt = u.ammo_location && it->can_reload_with( u.ammo_location->typeId() ) ?
                              item::reload_option( &u, it, it, u.ammo_location.clone() ) :
                              u.select_ammo( *it, prompt );

    if( opt ) {
        u.assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
        if( use_loc ) {
            u.activity.targets.emplace_back( loc.clone() );
        } else {
            u.activity.targets.emplace_back( u, const_cast<item *>( opt.target ) );
        }
        u.activity.targets.push_back( std::move( opt.ammo ) );
    }

    refresh_all();
}

void game::reload()
{
    // general reload item menu will popup if:
    // - user is unarmed;
    // - weapon wielded can't be reloaded (bows can, they just reload before shooting automatically)
    // - weapon wielded reloads before shooting (like bows)
    if( !u.is_armed() || !u.can_reload( u.weapon ) || u.weapon.has_flag( "RELOAD_AND_SHOOT" ) ) {
        vehicle *veh = veh_pointer_or_null( m.veh_at( u.pos() ) );
        turret_data turret;
        if( veh && ( turret = veh->turret_query( u.pos() ) ) && turret.can_reload() ) {
            item::reload_option opt = g->u.select_ammo( *turret.base(), true );
            if( opt ) {
                g->u.assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
                g->u.activity.targets.emplace_back( turret.base() );
                g->u.activity.targets.push_back( std::move( opt.ammo ) );
            }
            return;
        }

        item_location item_loc = inv_map_splice( [&]( const item & it ) {
            return u.rate_action_reload( it ) == HINT_GOOD;
        }, _( "Reload item" ), 1, _( "You have nothing to reload." ) );

        if( !item_loc ) {
            add_msg( _( "Never mind." ) );
            return;
        }

        reload( item_loc );

    } else {
        reload( -1 );
    }
}

// Unload a container, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload( int pos )
{
    item *it = nullptr;
    item_location item_loc;

    if( pos == INT_MIN ) {
        item_loc = inv_map_splice( [&]( const item & it ) {
            return u.rate_action_unload( it ) == HINT_GOOD;
        }, _( "Unload item" ), 1, _( "You have nothing to unload." ) );
        it = item_loc.get_item();

        if( it == nullptr ) {
            add_msg( _( "Never mind." ) );
            return;
        }
    } else {
        it = &u.i_at( pos );
        if( it->is_null() ) {
            debugmsg( "Tried to unload non-existent item" );
            return;
        }
        item_loc = item_location( u, it );
    }

    if( u.unload( *it ) ) {
        if( it->has_flag( "MAG_DESTROY" ) && it->ammo_remaining() == 0 ) {
            item_loc.remove_item();
        }
    }
}

void game::mend( int pos )
{
    if( pos == INT_MIN ) {
        if( u.is_armed() ) {
            pos = -1;
        } else {
            add_msg( m_info, _( "You're not wielding anything." ) );
        }
    }

    item &obj = g->u.i_at( pos );
    if( g->u.has_item( obj ) ) {
        g->u.mend_item( item_location( g->u, &obj ) );
    }
}

bool game::unload( item &it )
{
    return u.unload( it );
}

void game::wield( int pos )
{
    item_location loc( u, &u.i_at( pos ) );
    wield( loc );
}

void game::wield( item_location &loc )
{
    if( u.is_armed() ) {
        const bool is_unwielding = u.is_wielding( *loc );
        const auto ret = u.can_unwield( *loc );

        if( !ret.success() ) {
            add_msg( m_info, "%s", ret.c_str() );
        }

        u.unwield();

        if( is_unwielding ) {
            return;
        }
    }

    const auto ret = u.can_wield( *loc );
    if( !ret.success() ) {
        add_msg( m_info, "%s", ret.c_str() );
    }

    // Need to do this here because holster_actor::use() checks if/where the item is worn
    item &target = *loc.get_item();
    if( target.get_use( "holster" ) && !target.contents.empty() ) {
        if( query_yn( _( "Draw %s from %s?" ), target.get_contained().tname(), target.tname() ) ) {
            u.invoke_item( &target );
            return;
        }
    }

    // Can't use loc.obtain() here because that would cause things to spill.
    item to_wield = *loc.get_item();
    item_location::type location_type = loc.where();
    tripoint pos = loc.position();
    int worn_index = INT_MIN;
    if( u.is_worn( *loc.get_item() ) ) {
        int item_pos = u.get_item_position( loc.get_item() );
        if( item_pos != INT_MIN ) {
            worn_index = Character::worn_position_to_index( item_pos );
        }
    }
    int move_cost = loc.obtain_cost( u );
    loc.remove_item();
    if( !u.wield( to_wield ) ) {
        switch( location_type ) {
            case item_location::type::character:
                if( worn_index != INT_MIN ) {
                    auto it = u.worn.begin();
                    std::advance( it, worn_index );
                    u.worn.insert( it, to_wield );
                } else {
                    u.i_add( to_wield );
                }
                break;
            case item_location::type::map:
                m.add_item( pos, to_wield );
                break;
            case item_location::type::vehicle: {
                const cata::optional<vpart_reference> vp = g->m.veh_at( pos ).part_with_feature( "CARGO", false );
                // If we fail to return the item to the vehicle for some reason, add it to the map instead.
                if( !vp || !( vp->vehicle().add_item( vp->part_index(), to_wield ) ) ) {
                    m.add_item( pos, to_wield );
                }
                break;
            }
            case item_location::type::invalid:
                debugmsg( "Failed wield from invalid item location" );
                break;
        }
        return;
    }

    u.mod_moves( -move_cost );
}

void game::wield()
{
    item_location loc = game_menus::inv::wield( u );

    if( loc ) {
        wield( loc );
    } else {
        add_msg( _( "Never mind." ) );
    }
}

bool game::check_safe_mode_allowed( bool repeat_safe_mode_warnings )
{
    if( !repeat_safe_mode_warnings && safe_mode_warning_logged ) {
        // Already warned player since safe_mode_warning_logged is set.
        return false;
    }

    std::string msg_ignore = press_x( ACTION_IGNORE_ENEMY );
    if( !msg_ignore.empty() ) {
        std::wstring msg_ignore_wide = utf8_to_wstr( msg_ignore );
        // Operate on a wide-char basis to prevent corrupted multi-byte string
        msg_ignore_wide[0] = towlower( msg_ignore_wide[0] );
        msg_ignore = wstr_to_utf8( msg_ignore_wide );
    }

    if( u.has_effect( effect_laserlocked ) ) {
        // Automatic and mandatory safemode.  Make BLOODY sure the player notices!
        add_msg( m_warning, _( "You are being laser-targeted, %s to ignore." ),
                 msg_ignore.c_str() );
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
    // Monsters around and we don't want to run
    std::string spotted_creature_name;
    if( new_seen_mon.empty() ) {
        // naming consistent with code in game::mon_info
        spotted_creature_name = _( "a survivor" );
        get_safemode().lastmon_whitelist = get_safemode().npc_type_name();
    } else {
        spotted_creature_name = new_seen_mon.back()->name();
        get_safemode().lastmon_whitelist = spotted_creature_name;
    }

    std::string whitelist;
    if( !get_safemode().empty() ) {
        whitelist = string_format( _( " or %s to whitelist the monster" ),
                                   press_x( ACTION_WHITELIST_ENEMY ).c_str() );
    }

    const std::string msg_safe_mode = press_x( ACTION_TOGGLE_SAFEMODE );
    add_msg( m_warning,
             _( "Spotted %1$s--safe mode is on! (%2$s to turn it off, %3$s to ignore monster%4$s)" ),
             spotted_creature_name.c_str(), msg_safe_mode.c_str(),
             msg_ignore.c_str(), whitelist.c_str() );
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
    monster *const mon_ptr = critter_at<monster>( p );
    if( !mon_ptr ) {
        return false;
    }
    monster &critter = *mon_ptr;
    if( critter.friendly == 0 ) {
        // Can only disable / reprogram friendly monsters
        return false;
    }
    const auto mid = critter.type->id;
    const auto mon_item_id = critter.type->revert_to_itype;
    if( !mon_item_id.empty() &&
        query_yn( _( "Deactivate the %s?" ), critter.name() ) ) {

        u.moves -= 100;
        m.add_item_or_charges( p.x, p.y, critter.to_item() );
        if( !critter.has_flag( MF_INTERIOR_AMMO ) ) {
            for( auto &ammodef : critter.ammo ) {
                if( ammodef.second > 0 ) {
                    m.spawn_item( p.x, p.y, ammodef.first, 1, ammodef.second, calendar::turn );
                }
            }
        }
        remove_zombie( critter );
        return true;
    }
    // Manhacks are special, they have their own menu here.
    if( mid == mon_manhack ) {
        int choice = UILIST_CANCEL;
        if( critter.has_effect( effect_docile ) ) {
            choice = uilist( _( "Reprogram the manhack?" ), { _( "Engage targets." ) } );
        } else {
            choice = uilist( _( "Reprogram the manhack?" ), { _( "Follow me." ) } );
        }
        switch( choice ) {
            case 0:
                if( critter.has_effect( effect_docile ) ) {
                    critter.remove_effect( effect_docile );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s hovers momentarily as it surveys the area." ),
                                 critter.name().c_str() );
                    }
                } else {
                    critter.add_effect( effect_docile, 1_turns, num_bp, true );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s lets out a whirring noise and starts to follow you." ),
                                 critter.name().c_str() );
                    }
                }
                u.moves -= 100;
                return true;
            default:
                break;
        }
    }
    return false;
}

bool game::prompt_dangerous_tile( const tripoint &dest_loc ) const
{
    std::vector<std::string> harmful_stuff;
    const auto fields_here = m.field_at( u.pos() );
    for( const auto &e : m.field_at( dest_loc ) ) {
        // warn before moving into a dangerous field except when already standing within a similar field
        if( u.is_dangerous_field( e.second ) && fields_here.findField( e.first ) == nullptr ) {
            harmful_stuff.push_back( e.second.name() );
        }
    }

    if( !u.is_blind() ) {
        const trap &tr = m.tr_at( dest_loc );
        const bool boardable = static_cast<bool>( m.veh_at( dest_loc ).part_with_feature( "BOARDABLE",
                               true ) );
        // Hack for now, later ledge should stop being a trap
        // Note: in non-z-level mode, ledges obey different rules and so should be handled as regular traps
        if( tr.loadid == tr_ledge && m.has_zlevels() ) {
            if( !boardable ) {
                harmful_stuff.emplace_back( tr.name().c_str() );
            }
        } else if( tr.can_see( dest_loc, u ) && !tr.is_benign() ) {
            harmful_stuff.emplace_back( tr.name().c_str() );
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
            harmful_stuff.emplace_back( m.name( dest_loc ).c_str() );
        } else if( m.has_flag( "SHARP", dest_loc ) && !m.has_flag( "SHARP", u.pos() ) && !boardable &&
                   u.dex_cur < 78 && !std::all_of( sharp_bps.begin(), sharp_bps.end(), sharp_bp_check ) ) {
            harmful_stuff.emplace_back( m.name( dest_loc ).c_str() );
        }

    }

    if( !harmful_stuff.empty() &&
        !query_yn( _( "Really step into %s?" ), enumerate_as_string( harmful_stuff ).c_str() ) ) {
        return false;
    }
    return true;
}

bool game::plmove( int dx, int dy, int dz )
{
    if( ( !check_safe_mode_allowed() ) || u.has_active_mutation( trait_SHELL2 ) ) {
        if( u.has_active_mutation( trait_SHELL2 ) ) {
            add_msg( m_warning, _( "You can't move while in your shell.  Deactivate it to go mobile." ) );
        }
        return false;
    }

    tripoint dest_loc;
    if( dz == 0 && u.has_effect( effect_stunned ) ) {
        dest_loc.x = rng( u.posx() - 1, u.posx() + 1 );
        dest_loc.y = rng( u.posy() - 1, u.posy() + 1 );
        dest_loc.z = u.posz();
    } else {
        if( tile_iso && use_tiles && !u.has_destination() ) {
            rotate_direction_cw( dx, dy );
        }
        dest_loc.x = u.posx() + dx;
        dest_loc.y = u.posy() + dy;
        dest_loc.z = u.posz() + dz;
    }

    if( dest_loc == u.pos() ) {
        // Well that sure was easy
        return true;
    }

    if( !u.has_effect( effect_stunned ) && !u.is_underwater() ) {
        int turns;
        if( get_option<bool>( "AUTO_FEATURES" ) && mostseen == 0 && get_option<bool>( "AUTO_MINING" ) &&
            u.weapon.has_flag( "DIG_TOOL" ) && m.has_flag( "MINEABLE", dest_loc ) && !m.veh_at( dest_loc ) ) {
            if( u.weapon.has_flag( "POWERED" ) ) {
                if( u.weapon.ammo_sufficient() ) {
                    turns = MINUTES( 30 );
                    u.weapon.ammo_consume( u.weapon.ammo_required(), u.pos() );
                    u.assign_activity( activity_id( "ACT_JACKHAMMER" ), turns * 100, -1,
                                       u.get_item_position( &u.weapon ) );
                    u.activity.placement = dest_loc;
                    add_msg( _( "You start breaking the %1$s with your %2$s." ),
                             m.tername( dest_loc ).c_str(), u.weapon.tname().c_str() );
                    u.defer_move( dest_loc ); // don't move into the tile until done mining
                    return true;
                } else {
                    add_msg( _( "Your %s doesn't turn on." ), u.weapon.tname().c_str() );
                }
            } else {
                if( m.move_cost( dest_loc ) == 2 ) {
                    // breaking up some flat surface, like pavement
                    turns = MINUTES( 20 );
                } else {
                    turns = ( ( MAX_STAT + 4 ) - std::min( u.str_cur, MAX_STAT ) ) * MINUTES( 5 );
                }
                u.assign_activity( activity_id( "ACT_PICKAXE" ), turns * 100, -1,
                                   u.get_item_position( &u.weapon ) );
                u.activity.placement = dest_loc;
                add_msg( _( "You start breaking the %1$s with your %2$s." ),
                         m.tername( dest_loc ).c_str(), u.weapon.tname().c_str() );
                u.defer_move( dest_loc ); // don't move into the tile until done mining
                return true;
            }
        } else if( u.has_active_mutation( trait_BURROW ) ) {
            if( m.move_cost( dest_loc ) == 2 ) {
                turns = MINUTES( 10 );
            } else {
                turns = MINUTES( 30 );
            }
            u.assign_activity( activity_id( "ACT_BURROW" ), turns * 100, -1, 0 );
            u.activity.placement = dest_loc;
            add_msg( _( "You start tearing into the %s with your teeth and claws." ),
                     m.tername( dest_loc ).c_str() );
        }
    }

    if( dz == 0 && ramp_move( dest_loc ) ) {
        // TODO: Make it work nice with automove (if it doesn't do so already?)
        return false;
    }

    if( u.has_effect( effect_amigara ) ) {
        int curdist = INT_MAX;
        int newdist = INT_MAX;
        const tripoint minp = tripoint( 0, 0, u.posz() );
        const tripoint maxp = tripoint( MAPSIZE_X, MAPSIZE_Y, u.posz() );
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

    dbg( D_PEDANTIC_INFO ) << "game:plmove: From (" <<
                           u.posx() << "," << u.posy() << "," << u.posz() << ") to (" <<
                           dest_loc.x << "," << dest_loc.y << "," << dest_loc.z << ")";

    if( disable_robot( dest_loc ) ) {
        return false;
    }

    // Check if our movement is actually an attack on a monster or npc
    // Are we displacing a monster?

    bool attacking = false;
    if( critter_at( dest_loc ) ) {
        attacking = true;
    }

    if( !u.move_effects( attacking ) ) {
        u.moves -= 100;
        return false;
    }

    if( monster *const mon_ptr = critter_at<monster>( dest_loc, true ) ) {
        monster &critter = *mon_ptr;
        if( critter.friendly == 0 &&
            !critter.has_effect( effect_pet ) ) {
            if( u.has_destination() ) {
                add_msg( m_warning, _( "Monster in the way. Auto-move canceled." ) );
                add_msg( m_info, _( "Click directly on monster to attack." ) );
                u.clear_destination();
                return false;
            }
            if( u.has_effect( effect_relax_gas ) ) {
                if( one_in( 8 ) ) {
                    add_msg( m_good, _( "Your willpower asserts itself, and so do you!" ) );
                } else {
                    u.moves -= rng( 2, 8 ) * 10;
                    add_msg( m_bad, _( "You're too pacified to strike anything..." ) );
                    return false;
                }
            }
            u.melee_attack( critter, true );
            if( critter.is_hallucination() ) {
                critter.die( &u );
            }
            draw_hit_mon( dest_loc, critter, critter.is_dead() );
            return false;
        } else if( critter.has_flag( MF_IMMOBILE ) ) {
            add_msg( m_info, _( "You can't displace your %s." ), critter.name().c_str() );
            return false;
        }
        // Successful displacing is handled (much) later
    }
    // If not a monster, maybe there's an NPC there
    if( npc *const np_ = critter_at<npc>( dest_loc ) ) {
        npc &np = *np_;
        if( u.has_destination() ) {
            add_msg( _( "NPC in the way, Auto-move canceled." ) );
            add_msg( m_info, _( "Click directly on NPC to attack." ) );
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
    int dpart = -1;
    const optional_vpart_position vp0 = m.veh_at( u.pos() );
    vehicle *const veh0 = veh_pointer_or_null( vp0 );
    const optional_vpart_position vp1 = m.veh_at( dest_loc );
    vehicle *const veh1 = veh_pointer_or_null( vp1 );

    bool veh_closed_door = false;
    bool outside_vehicle = ( veh0 == nullptr || veh0 != veh1 );
    if( veh1 != nullptr ) {
        dpart = veh1->next_part_to_open( vp1->part_index(), outside_vehicle );
        veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
    }

    if( veh0 != nullptr && abs( veh0->velocity ) > 100 ) {
        if( veh1 == nullptr ) {
            if( query_yn( _( "Dive from moving vehicle?" ) ) ) {
                moving_vehicle_dismount( dest_loc );
            }
            return false;
        } else if( veh1 != veh0 ) {
            add_msg( m_info, _( "There is another vehicle in the way." ) );
            return false;
        } else if( !vp1.part_with_feature( "BOARDABLE", true ) ) {
            add_msg( m_info, _( "That part of the vehicle is currently unsafe." ) );
            return false;
        }
    }

    bool toSwimmable = m.has_flag( "SWIMMABLE", dest_loc );
    bool toDeepWater = m.has_flag( TFLAG_DEEP_WATER, dest_loc );
    bool fromSwimmable = m.has_flag( "SWIMMABLE", u.pos() );
    bool fromDeepWater = m.has_flag( TFLAG_DEEP_WATER, u.pos() );
    bool fromBoat = veh0 != nullptr && veh0->is_in_water();
    bool toBoat = veh1 != nullptr && veh1->is_in_water();

    if( toSwimmable && toDeepWater && !toBoat ) { // Dive into water!
        // Requires confirmation if we were on dry land previously
        if( ( fromSwimmable && fromDeepWater && !fromBoat ) || query_yn( _( "Dive into the water?" ) ) ) {
            if( ( !fromDeepWater || fromBoat ) && u.swim_speed() < 500 ) {
                add_msg( _( "You start swimming." ) );
                add_msg( m_info, _( "%s to dive underwater." ),
                         press_x( ACTION_MOVE_DOWN ).c_str() );
            }
            plswim( dest_loc );
        }

        on_move_effects();
        return true;
    }

    //Wooden Fence Gate (or equivalently walkable doors):
    // open it if we are walking
    // vault over it if we are running
    if( m.passable_ter_furn( dest_loc )
        && u.move_mode == "walk"
        && m.open_door( dest_loc, !m.is_outside( u.pos() ) ) ) {
        u.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( u.has_destination() ) {
            u.defer_move( dest_loc );
        }
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
            veh1->open( dpart );
            add_msg( _( "You open the %1$s's %2$s." ), veh1->name.c_str(),
                     veh1->part_info( dpart ).name().c_str() );
        }
        u.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( u.has_destination() ) {
            u.defer_move( dest_loc );
        }
        return true;
    }

    if( m.furn( dest_loc ) != f_safe_c && m.open_door( dest_loc, !m.is_outside( u.pos() ) ) ) {
        u.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( u.has_destination() ) {
            u.defer_move( dest_loc );
        }
        return true;
    }

    // Invalid move
    const bool waste_moves = u.is_blind() || u.has_effect( effect_stunned );
    if( waste_moves || dest_loc.z != u.posz() ) {
        add_msg( _( "You bump into the %s!" ), m.obstacle_name( dest_loc ).c_str() );
        // Only lose movement if we're blind
        if( waste_moves ) {
            u.moves -= 100;
        }
    } else if( m.ter( dest_loc ) == t_door_locked || m.ter( dest_loc ) == t_door_locked_peep ||
               m.ter( dest_loc ) == t_door_locked_alarm || m.ter( dest_loc ) == t_door_locked_interior ) {
        // Don't drain move points for learning something you could learn just by looking
        add_msg( _( "That door is locked!" ) );
    } else if( m.ter( dest_loc ) == t_door_bar_locked ) {
        add_msg( _( "You rattle the bars but the door is locked!" ) );
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
        if( rl_dist( pt, dest_loc ) < 2 && m.has_flag( "RAMP_END", pt ) ) {
            aligned_ramps = true;
            break;
        }
    }

    const tripoint above_u( u.posx(), u.posy(), u.posz() + 1 );
    if( m.has_floor_or_support( above_u ) ) {
        add_msg( m_warning, _( "You can't climb here - there's a ceiling above." ) );
        return false;
    }

    const tripoint dp = dest_loc - u.pos();
    const tripoint old_pos = u.pos();
    plmove( dp.x, dp.y, 1 );
    // We can't just take the result of the above function here
    if( u.pos() != old_pos ) {
        u.moves -= 50 + ( aligned_ramps ? 0 : 50 );
    }

    return true;
}

bool game::walk_move( const tripoint &dest_loc )
{
    const optional_vpart_position vp_here = m.veh_at( u.pos() );
    const optional_vpart_position vp_there = m.veh_at( dest_loc );

    bool pushing = false; // moving -into- grabbed tile; skip check for move_cost > 0
    bool pulling = false; // moving -away- from grabbed tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0

    bool grabbed = u.get_grab_type() != OBJECT_NONE;
    if( grabbed ) {
        const tripoint dp = dest_loc - u.pos();
        pushing = dp ==  u.grab_point;
        pulling = dp == -u.grab_point;
    }

    if( grabbed && dest_loc.z != u.posz() ) {
        add_msg( m_warning, _( "You let go of the grabbed object" ) );
        grabbed = false;
        u.grab( OBJECT_NONE );
    }

    // Now make sure we're actually holding something
    const vehicle *grabbed_vehicle = nullptr;
    if( grabbed && u.get_grab_type() == OBJECT_FURNITURE ) {
        // We only care about shifting, because it's the only one that can change our destination
        if( m.has_furn( u.pos() + u.grab_point ) ) {
            shifting_furniture = !pushing && !pulling;
        } else {
            // We were grabbing a furniture that isn't there
            grabbed = false;
        }
    } else if( grabbed && u.get_grab_type() == OBJECT_VEHICLE ) {
        grabbed_vehicle = veh_pointer_or_null( m.veh_at( u.pos() + u.grab_point ) );
        if( grabbed_vehicle == nullptr ) {
            // We were grabbing a vehicle that isn't there anymore
            grabbed = false;
        }
    } else if( grabbed ) {
        // We were grabbing something WEIRD, let's pretend we weren't
        grabbed = false;
    }

    if( u.grab_point != tripoint_zero && !grabbed ) {
        add_msg( m_warning, _( "Can't find grabbed object." ) );
        u.grab( OBJECT_NONE );
    }

    if( m.impassable( dest_loc ) && !pushing && !shifting_furniture ) {
        return false;
    }
    u.set_underwater( false );

    if( !shifting_furniture && !prompt_dangerous_tile( dest_loc ) ) {
        return true;
    }

    // Used to decide whether to print a 'moving is slow message
    const int mcost_from = m.move_cost( u.pos() ); //calculate this _before_ calling grabbed_move

    int modifier = 0;
    if( grabbed && u.get_grab_type() == OBJECT_FURNITURE && u.pos() + u.grab_point == dest_loc ) {
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

    // Max out recoil
    u.recoil = MAX_RECOIL;

    // Print a message if movement is slow
    const int mcost_to = m.move_cost( dest_loc ); //calculate this _after_ calling grabbed_move
    const bool fungus = m.has_flag_ter_or_furn( "FUNGUS", u.pos() ) ||
                        m.has_flag_ter_or_furn( "FUNGUS",
                                dest_loc ); //fungal furniture has no slowing effect on mycus characters
    const bool slowed = ( ( !u.has_trait( trait_PARKOUR ) && ( mcost_to > 2 || mcost_from > 2 ) ) ||
                          mcost_to > 4 || mcost_from > 4 ) &&
                        !( u.has_trait( trait_M_IMMUNE ) && fungus );
    if( slowed ) {
        // Unless u.pos() has a higher movecost than dest_loc, state that dest_loc is the cause
        if( mcost_to >= mcost_from ) {
            if( auto displayed_part = vp_there.part_displayed() ) {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ),
                         displayed_part->part().name() );
            } else {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ), m.name( dest_loc ).c_str() );
            }
        } else {
            if( auto displayed_part = vp_here.part_displayed() ) {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ),
                         displayed_part->part().name() );
            } else {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ), m.name( u.pos() ).c_str() );
            }
        }
        sfx::play_variant_sound( "plmove", "clear_obstacle", sfx::get_heard_volume( u.pos() ) );
    }

    if( u.has_trait( trait_id( "LEG_TENT_BRACE" ) ) && ( !u.footwear_factor() ||
            ( u.footwear_factor() == .5 && one_in( 2 ) ) ) ) {
        // DX and IN are long suits for Cephalopods,
        // so this shouldn't cause too much hardship
        // Presumed that if it's swimmable, they're
        // swimming and won't stick
        ///\EFFECT_DEX decreases chance of tentacles getting stuck to the ground

        ///\EFFECT_INT decreases chance of tentacles getting stuck to the ground
        if( !m.has_flag( "SWIMMABLE", dest_loc ) && one_in( 80 + u.dex_cur + u.int_cur ) ) {
            add_msg( _( "Your tentacles stick to the ground, but you pull them free." ) );
            u.mod_fatigue( 1 );
        }
    }

    if( !u.has_artifact_with( AEP_STEALTH ) && !u.has_trait( trait_id( "DEBUG_SILENT" ) ) ) {
        int volume = 6;
        volume *= u.mutation_value( "noise_modifier" );
        if( volume > 0 ) {
            if( u.is_wearing( "rm13_armor_on" ) ) {
                volume = 2;
            } else if( u.has_bionic( bionic_id( "bio_ankles" ) ) ) {
                volume = 12;
            }
            sounds::sound( dest_loc, volume, sounds::sound_t::movement, _( "footsteps" ), true,
                           "none", "none" );    // Sound of footsteps may awaken nearby monsters
            sfx::do_footstep();
        }

        if( one_in( 20 ) && u.has_artifact_with( AEP_MOVEMENT_NOISE ) ) {
            sounds::sound( u.pos(), 40, sounds::sound_t::movement, _( "a rattling sound." ) );
        }
    }

    if( u.is_hauling() ) {
        u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
        // Whether the source is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Whether the destination is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Source relative to the player
        u.activity.placement = u.pos() - dest_loc;
        // Destination relative to the player
        u.activity.coords.push_back( tripoint_zero );
        map_stack items = m.i_at( u.pos() );
        if( items.empty() ) {
            u.stop_hauling();
        }
        int index = 0;
        for( auto it = items.begin(); it != items.end(); ++index, ++it ) {
            int amount = it->count();
            u.activity.values.push_back( index );
            u.activity.values.push_back( amount );
        }
    }
    if( m.has_flag_ter_or_furn( TFLAG_HIDE_PLACE, dest_loc ) ) {
        add_msg( m_good, _( "You are hiding in the %s." ), m.name( dest_loc ).c_str() );
    }

    if( dest_loc != u.pos() ) {
        u.lifetime_stats.squares_walked++;
    }

    place_player( dest_loc );
    on_move_effects();

    return true;
}

void game::place_player( const tripoint &dest_loc )
{
    const optional_vpart_position vp1 = m.veh_at( dest_loc );
    if( const cata::optional<std::string> label = vp1.get_label() ) {
        add_msg( m_info, _( "Label here: %s" ), *label );
    }
    std::string signage = m.get_signage( dest_loc );
    if( !signage.empty() ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "The sign says: %s" ), signage.c_str() );
        } else {
            add_msg( m_info, _( "There is a sign here, but you are unable to read it." ) );
        }
    }
    if( m.has_graffiti_at( dest_loc ) ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "Written here: %s" ), m.graffiti_at( dest_loc ).c_str() );
        } else {
            add_msg( m_info, _( "Something is written here, but you are unable to read it." ) );
        }
    }
    // TODO: Move the stuff below to a Character method so that NPCs can reuse it
    if( m.has_flag( "ROUGH", dest_loc ) && ( !u.in_vehicle ) ) {
        if( one_in( 5 ) && u.get_armor_bash( bp_foot_l ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your left foot on the %s!" ),
                     m.has_flag_ter( "ROUGH", dest_loc ) ? m.tername( dest_loc ).c_str() : m.furnname(
                         dest_loc ).c_str() );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
        if( one_in( 5 ) && u.get_armor_bash( bp_foot_r ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your right foot on the %s!" ),
                     m.has_flag_ter( "ROUGH", dest_loc ) ? m.tername( dest_loc ).c_str() : m.furnname(
                         dest_loc ).c_str() );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
    }
    ///\EFFECT_DEX increases chance of avoiding cuts on sharp terrain
    if( m.has_flag( "SHARP", dest_loc ) && !one_in( 3 ) && !x_in_y( 1 + u.dex_cur / 2, 40 ) &&
        ( !u.in_vehicle ) && ( !u.has_trait( trait_PARKOUR ) || one_in( 4 ) ) ) {
        body_part bp = random_body_part();
        if( u.deal_damage( nullptr, bp, damage_instance( DT_CUT, rng( 1, 10 ) ) ).total_damage() > 0 ) {
            //~ 1$s - bodypart name in accusative, 2$s is terrain name.
            add_msg( m_bad, _( "You cut your %1$s on the %2$s!" ),
                     body_part_name_accusative( bp ).c_str(),
                     m.has_flag_ter( "SHARP", dest_loc ) ? m.tername( dest_loc ).c_str() : m.furnname(
                         dest_loc ).c_str() );
            if( ( u.has_trait( trait_INFRESIST ) ) && ( one_in( 1024 ) ) ) {
                u.add_effect( effect_tetanus, 1_turns, num_bp, true );
            } else if( ( !u.has_trait( trait_INFIMMUNE ) || !u.has_trait( trait_INFRESIST ) ) &&
                       ( one_in( 256 ) ) ) {
                u.add_effect( effect_tetanus, 1_turns, num_bp, true );
            }
        }
    }
    if( m.has_flag( "UNSTABLE", dest_loc ) ) {
        u.add_effect( effect_bouldering, 1_turns, num_bp, true );
    } else if( u.has_effect( effect_bouldering ) ) {
        u.remove_effect( effect_bouldering );
    }
    if( m.has_flag_ter_or_furn( TFLAG_NO_SIGHT, dest_loc ) ) {
        u.add_effect( effect_no_sight, 1_turns, num_bp, true );
    } else if( u.has_effect( effect_no_sight ) ) {
        u.remove_effect( effect_no_sight );
    }


    // If we moved out of the nonant, we need update our map data
    if( m.has_flag( "SWIMMABLE", dest_loc ) && u.has_effect( effect_onfire ) ) {
        add_msg( _( "The water puts out the flames!" ) );
        u.remove_effect( effect_onfire );
    }

    if( monster *const mon_ptr = critter_at<monster>( dest_loc ) ) {
        // We displaced a monster. It's probably a bug if it wasn't a friendly mon...
        // Immobile monsters can't be displaced.
        monster &critter = *mon_ptr;
        critter.move_to( u.pos(), true ); // Force the movement even though the player is there right now.
        add_msg( _( "You displace the %s." ), critter.name().c_str() );
    }

    // If the player is in a vehicle, unboard them from the current part
    if( u.in_vehicle ) {
        m.unboard_vehicle( u.pos() );
    }
    // Move the player
    // Start with z-level, to make it less likely that old functions (2D ones) freak out
    if( m.has_zlevels() && dest_loc.z != get_levz() ) {
        vertical_shift( dest_loc.z );
    }

    if( u.is_hauling() && ( !m.can_put_items( dest_loc ) ||
                            m.has_flag( TFLAG_DEEP_WATER, dest_loc ) ||
                            vp1 ) ) {
        u.stop_hauling();
    }

    u.setpos( dest_loc );
    update_map( u );
    // Important: don't use dest_loc after this line. `update_map` may have shifted the map
    // and dest_loc was not adjusted and therefore is still in the un-shifted system and probably wrong.

    //Auto pulp or butcher and Auto foraging
    if( get_option<bool>( "AUTO_FEATURES" ) && mostseen == 0 ) {
        static const direction adjacentDir[8] = { NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST };

        const std::string forage_type = get_option<std::string>( "AUTO_FORAGING" );
        if( forage_type != "off" ) {
            static const auto forage = [&]( const tripoint & pos ) {
                const auto &xter_t = m.ter( pos ).obj().examine;
                const bool forage_bushes = forage_type == "both" || forage_type == "bushes";
                const bool forage_trees = forage_type == "both" || forage_type == "trees";
                if( xter_t == &iexamine::none ) {
                    return;
                } else if( ( forage_bushes && xter_t == &iexamine::shrub_marloss ) ||
                           ( forage_bushes && xter_t == &iexamine::shrub_wildveggies ) ||
                           ( forage_trees && xter_t == &iexamine::tree_marloss ) ||
                           ( forage_trees && xter_t == &iexamine::harvest_ter )
                         ) {
                    xter_t( u, pos );
                }
            };

            for( auto &elem : adjacentDir ) {
                forage( u.pos() + direction_XY( elem ) );
            }
        }

        const std::string pulp_butcher = get_option<std::string>( "AUTO_PULP_BUTCHER" );
        if( pulp_butcher == "butcher" && u.max_quality( quality_id( "BUTCHER" ) ) > INT_MIN ) {
            std::vector<int> corpses;
            auto items = m.i_at( u.pos() );

            for( size_t i = 0; i < items.size(); i++ ) {
                if( items[i].is_corpse() ) {
                    corpses.push_back( i );
                }
            }

            if( !corpses.empty() ) {
                u.assign_activity( activity_id( "ACT_BUTCHER" ), 0, -1 );
                for( int i : corpses ) {
                    u.activity.values.push_back( i );
                }
            }
        } else if( pulp_butcher == "pulp" || pulp_butcher == "pulp_adjacent" ) {
            static const auto pulp = [&]( const tripoint & pos ) {
                for( const auto &maybe_corpse : m.i_at( pos ) ) {
                    if( maybe_corpse.is_corpse() && maybe_corpse.can_revive() &&
                        maybe_corpse.get_mtype()->bloodType() != fd_acid ) {
                        u.assign_activity( activity_id( "ACT_PULP" ), calendar::INDEFINITELY_LONG, 0 );
                        u.activity.placement = pos;
                        u.activity.auto_resume = true;
                        u.activity.str_values.push_back( "auto_pulp_no_acid" );
                        return;
                    }
                }
            };

            if( pulp_butcher == "pulp_adjacent" ) {
                for( auto &elem : adjacentDir ) {
                    pulp( u.pos() + direction_XY( elem ) );
                }
            } else {
                pulp( u.pos() );
            }
        }
    }

    //Autopickup
    if( get_option<bool>( "AUTO_PICKUP" ) && ( !get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) ||
            mostseen == 0 ) &&
        ( m.has_items( u.pos() ) || get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
        Pickup::pick_up( u.pos(), -1 );
    }

    // If the new tile is a boardable part, board it
    if( vp1.part_with_feature( "BOARDABLE", true ) ) {
        m.board_vehicle( u.pos(), &u );
    }

    // Traps!
    // Try to detect.
    u.search_surroundings();
    m.creature_on_trap( u );
    // Drench the player if swimmable
    if( m.has_flag( "SWIMMABLE", u.pos() ) ) {
        u.drench( 40, { { bp_foot_l, bp_foot_r, bp_leg_l, bp_leg_r } }, false );
    }

    // List items here
    if( !m.has_flag( "SEALED", u.pos() ) ) {
        if( get_option<bool>( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS" ) ||
            !g->check_zone( zone_type_id( "NO_AUTO_PICKUP" ), u.pos() ) ) {
            if( u.is_blind() && !m.i_at( u.pos() ).empty() ) {
                add_msg( _( "There's something here, but you can't see what it is." ) );
            } else if( m.has_items( u.pos() ) ) {
                std::vector<std::string> names;
                std::vector<size_t> counts;
                std::vector<item> items;
                for( auto &tmpitem : m.i_at( u.pos() ) ) {

                    std::string next_tname = tmpitem.tname();
                    std::string next_dname = tmpitem.display_name();
                    bool by_charges = tmpitem.count_by_charges();
                    bool got_it = false;
                    for( size_t i = 0; i < names.size(); ++i ) {
                        if( by_charges && next_tname == names[i] ) {
                            counts[i] += tmpitem.charges;
                            got_it = true;
                            break;
                        } else if( next_dname == names[i] ) {
                            counts[i] += 1;
                            got_it = true;
                            break;
                        }
                    }
                    if( !got_it ) {
                        if( by_charges ) {
                            names.push_back( tmpitem.tname( tmpitem.charges ) );
                            counts.push_back( tmpitem.charges );
                        } else {
                            names.push_back( tmpitem.display_name( 1 ) );
                            counts.push_back( 1 );
                        }
                        items.push_back( tmpitem );
                    }
                    if( names.size() > 10 ) {
                        break;
                    }
                }
                for( size_t i = 0; i < names.size(); ++i ) {
                    if( !items[i].count_by_charges() ) {
                        names[i] = items[i].display_name( counts[i] );
                    } else {
                        names[i] = items[i].tname( counts[i] );
                    }
                }
                int and_the_rest = 0;
                for( size_t i = 0; i < names.size(); ++i ) {
                    //~ number of items: "<number> <item>"
                    std::string fmt = ngettext( "%1$d %2$s", "%1$d %2$s", counts[i] );
                    names[i] = string_format( fmt, counts[i], names[i].c_str() );
                    // Skip the first two.
                    if( i > 1 ) {
                        and_the_rest += counts[i];
                    }
                }
                if( names.size() == 1 ) {
                    add_msg( _( "You see here %s." ), names[0].c_str() );
                } else if( names.size() == 2 ) {
                    add_msg( _( "You see here %s and %s." ),
                             names[0].c_str(), names[1].c_str() );
                } else if( names.size() == 3 ) {
                    add_msg( _( "You see here %s, %s, and %s." ), names[0].c_str(),
                             names[1].c_str(), names[2].c_str() );
                } else if( and_the_rest < 7 ) {
                    add_msg( ngettext( "You see here %s, %s and %d more item.",
                                       "You see here %s, %s and %d more items.",
                                       and_the_rest ),
                             names[0].c_str(), names[1].c_str(), and_the_rest );
                } else {
                    add_msg( _( "You see here %s and many more items." ),
                             names[0].c_str() );
                }
            }
        }
    }

    if( vp1.part_with_feature( "CONTROLS", true ) && u.in_vehicle ) {
        add_msg( _( "There are vehicle controls here." ) );
        add_msg( m_info, _( "%s to drive." ),
                 press_x( ACTION_CONTROL_VEHICLE ).c_str() );
    }
}

void game::place_player_overmap( const tripoint &om_dest )
{
    //First offload the active npcs.
    unload_npcs();
    for( monster &critter : all_monsters() ) {
        despawn_monster( critter );
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
    const tripoint map_om_pos( om_dest.x * 2 - HALF_MAPSIZE, om_dest.y * 2 - HALF_MAPSIZE, om_dest.z );
    const tripoint player_pos( u.pos().x, u.pos().y, map_om_pos.z );

    load_map( map_om_pos );
    load_npcs();
    m.spawn_monsters( true ); // Static monsters
    update_overmap_seen();
    // update weather now as it could be different on the new location
    nextweather = calendar::turn;
    place_player( player_pos );
}

bool game::phasing_move( const tripoint &dest_loc )
{
    if( !u.has_active_bionic( bionic_id( "bio_probability_travel" ) ) || u.power_level < 250 ) {
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
        if( tunneldist * 250 >
            u.power_level ) { //oops, not enough energy! Tunneling costs 250 bionic power per impassable tile
            add_msg( _( "You try to quantum tunnel through the barrier but are reflected! Try again with more energy!" ) );
            u.charge_power( -250 );
            return false;
        }

        if( tunneldist > 24 ) {
            add_msg( m_info, _( "It's too dangerous to tunnel that far!" ) );
            u.charge_power( -250 );
            return false;
        }

        dest.x += dx;
        dest.y += dy;
    }

    if( tunneldist != 0 ) {
        if( u.in_vehicle ) {
            m.unboard_vehicle( u.pos() );
        }

        add_msg( _( "You quantum tunnel through the %d-tile wide barrier!" ), tunneldist );
        u.charge_power( -( tunneldist * 250 ) ); //tunneling costs 250 bionic power per impassable tile
        u.moves -= 100; //tunneling costs 100 moves
        u.setpos( dest );

        if( m.veh_at( u.pos() ).part_with_feature( "BOARDABLE", true ) ) {
            m.board_vehicle( u.pos(), &u );
        }

        u.grab( OBJECT_NONE );
        on_move_effects();
        return true;
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
        // Where did it go? We're grabbing thin air so reset.
        add_msg( m_info, _( "No furniture at grabbed point." ) );
        u.grab( OBJECT_NONE );
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
                             m.passable( fdest ) &&
                             critter_at<npc>( fdest ) == nullptr &&
                             critter_at<monster>( fdest ) == nullptr &&
                             ( !pulling_furniture || is_empty( u.pos() + dp ) ) &&
                             ( !has_floor || m.has_flag( "FLAT", fdest ) ) &&
                             !m.has_furn( fdest ) &&
                             !m.veh_at( fdest ) &&
                             ( !has_floor || m.tr_at( fdest ).is_null() )
                         );

    const furn_t furntype = m.furn( fpos ).obj();
    const int src_items = m.i_at( fpos ).size();
    const int dst_items = m.i_at( fdest ).size();
    bool dst_item_ok = ( !m.has_flag( "NOITEM", fdest ) &&
                         !m.has_flag( "SWIMMABLE", fdest ) &&
                         !m.has_flag( "DESTROY_ITEM", fdest ) );
    bool src_item_ok = ( m.furn( fpos ).obj().has_flag( "CONTAINER" ) ||
                         m.furn( fpos ).obj().has_flag( "SEALED" ) );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( auto &contained_item : m.i_at( fpos ) ) {
        furniture_contents_weight += contained_item.weight();
    }
    str_req += furniture_contents_weight / 4_kilogram;

    if( !canmove ) {
        // TODO: What is something?
        add_msg( _( "The %s collides with something." ), furntype.name().c_str() );
        u.moves -= 50;
        return true;
        ///\EFFECT_STR determines ability to drag furniture
    } else if( str_req > u.get_str() &&
               one_in( std::max( 20 - str_req - u.get_str(), 2 ) ) ) {
        add_msg( m_bad, _( "You strain yourself trying to move the heavy %s!" ),
                 furntype.name().c_str() );
        u.moves -= 100;
        u.mod_pain( 1 ); // Hurt ourselves.
        return true; // furniture and or obstacle wins.
    } else if( !src_item_ok && dst_items > 0 ) {
        add_msg( _( "There's stuff in the way." ) );
        u.moves -= 50;
        return true;
    }

    u.moves -= str_req * 10;
    // Additional penalty if we can't comfortably move it.
    if( str_req > u.get_str() ) {
        int move_penalty = std::pow( str_req, 2.0 ) + 100.0;
        if( move_penalty <= 1000 ) {
            u.moves -= 100;
            add_msg( m_bad, _( "The %s is too heavy for you to budge." ),
                     furntype.name().c_str() );
            return true;
        }
        u.moves -= move_penalty;
        if( move_penalty > 500 ) {
            add_msg( _( "Moving the heavy %s is taking a lot of time!" ),
                     furntype.name().c_str() );
        } else if( move_penalty > 200 ) {
            if( one_in( 3 ) ) { // Nag only occasionally.
                add_msg( _( "It takes some time to move the heavy %s." ),
                         furntype.name().c_str() );
            }
        }
    }
    sounds::sound( fdest, furntype.move_str_req * 2, sounds::sound_t::movement,
                   _( "a scraping noise." ) );

    // Actually move the furniture
    m.furn_set( fdest, m.furn( fpos ) );
    m.furn_set( fpos, f_null );

    if( src_items > 0 ) { // and the stuff inside.
        if( dst_item_ok && src_item_ok ) {
            // Assume contents of both cells are legal, so we can just swap contents.
            std::list<item> temp;
            std::move( m.i_at( fpos ).begin(), m.i_at( fpos ).end(),
                       std::back_inserter( temp ) );
            m.i_clear( fpos );
            for( auto item_iter = m.i_at( fdest ).begin();
                 item_iter != m.i_at( fdest ).end(); ++item_iter ) {
                m.i_at( fpos ).push_back( *item_iter );
            }
            m.i_clear( fdest );
            for( auto &cur_item : temp ) {
                m.i_at( fdest ).push_back( cur_item );
            }
        } else {
            add_msg( _( "Stuff spills from the %s!" ), furntype.name().c_str() );
        }
    }

    if( shifting_furniture ) {
        // We didn't move
        tripoint d_sum = u.grab_point + dp;
        if( abs( d_sum.x ) < 2 && abs( d_sum.y ) < 2 ) {
            u.grab_point = d_sum; // furniture moved relative to us
        } else { // we pushed furniture out of reach
            add_msg( _( "You let go of the %s" ), furntype.name().c_str() );
            u.grab( OBJECT_NONE );
        }
        return true; // We moved furniture but stayed still.
    }

    if( pushing_furniture && m.impassable( fpos ) ) {
        // Not sure how that chair got into a wall, but don't let player follow.
        add_msg( _( "You let go of the %1$s as it slides past %2$s" ),
                 furntype.name().c_str(), m.tername( fdest ).c_str() );
        u.grab( OBJECT_NONE );
        return true;
    }

    return false;
}

bool game::grabbed_move( const tripoint &dp )
{
    if( u.get_grab_type() == OBJECT_NONE ) {
        return false;
    }

    if( dp.z != 0 ) {
        // No dragging stuff up/down stairs yet!
        return false;
    }

    // vehicle: pulling, pushing, or moving around the grabbed object.
    if( u.get_grab_type() == OBJECT_VEHICLE ) {
        return grabbed_veh_move( dp );
    }

    if( u.get_grab_type() == OBJECT_FURNITURE ) {
        return grabbed_furn_move( dp );
    }

    add_msg( m_info, _( "Nothing at grabbed point %d,%d,%d or bad grabbed object type." ),
             u.grab_point.x, u.grab_point.y, u.grab_point.z );
    u.grab( OBJECT_NONE );
    return false;
}

void game::on_move_effects()
{
    // TODO: Move this to a character method
    if( u.lifetime_stats.squares_walked % 8 == 0 ) {
        if( u.has_active_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            u.charge_power( 1 );
        }
    }
    if( u.lifetime_stats.squares_walked % 160 == 0 ) {
        if( u.has_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            u.charge_power( 1 );
        }
    }

    if( u.move_mode == "run" ) {
        if( u.stamina <= 0 ) {
            u.toggle_move_mode();
        }
        if( u.stamina < u.get_stamina_max() / 2 && one_in( u.stamina ) ) {
            u.add_effect( effect_winded, 3_turns );
        }
    }

    // apply martial art move bonuses
    u.ma_onmove_effects();

    sfx::do_ambient();
}

void game::plswim( const tripoint &p )
{
    if( !m.has_flag( "SWIMMABLE", p ) ) {
        dbg( D_ERROR ) << "game:plswim: Tried to swim in "
                       << m.tername( p ).c_str() << "!";
        debugmsg( "Tried to swim in %s!", m.tername( p ).c_str() );
        return;
    }
    if( u.has_effect( effect_onfire ) ) {
        add_msg( _( "The water puts out the flames!" ) );
        u.remove_effect( effect_onfire );
    }
    if( u.has_effect( effect_glowing ) ) {
        add_msg( _( "The water washes off the glowing goo!" ) );
        u.remove_effect( effect_glowing );
    }
    int movecost = u.swim_speed();
    u.practice( skill_id( "swimming" ), u.is_underwater() ? 2 : 1 );
    if( movecost >= 500 ) {
        if( !u.is_underwater() && !( u.shoe_type_count( "swim_fins" ) == 2 ||
                                     ( u.shoe_type_count( "swim_fins" ) == 1 && one_in( 2 ) ) ) ) {
            add_msg( m_bad, _( "You sink like a rock!" ) );
            u.set_underwater( true );
            ///\EFFECT_STR increases breath-holding capacity while sinking
            u.oxygen = 30 + 2 * u.str_cur;
        }
    }
    if( u.oxygen <= 5 && u.is_underwater() ) {
        if( movecost < 500 )
            popup( _( "You need to breathe! (%s to surface.)" ),
                   press_x( ACTION_MOVE_UP ).c_str() );
        else {
            popup( _( "You need to breathe but you can't swim!  Get to dry land, quick!" ) );
        }
    }
    bool diagonal = ( p.x != u.posx() && p.y != u.posy() );
    if( u.in_vehicle ) {
        m.unboard_vehicle( u.pos() );
    }
    u.setpos( p );
    update_map( u );
    if( m.veh_at( u.pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        m.board_vehicle( u.pos(), &u );
    }
    u.moves -= ( movecost > 200 ? 200 : movecost )  * ( trigdist && diagonal ? 1.41 : 1 );
    u.inv.rust_iron_items();

    u.burn_move_stamina( movecost );

    body_part_set drenchFlags{ {
            bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
            bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
        }
    };

    if( u.is_underwater() ) {
        drenchFlags |= { { bp_head, bp_eyes, bp_mouth, bp_hand_l, bp_hand_r } };
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
        add_msg( m_info, _( "No hostile creature in reach. Waiting a turn." ) );
        if( check_safe_mode_allowed() ) {
            u.pause();
        }
        return;
    }

    Creature &best = **std::max_element( critters.begin(), critters.end(),
    []( const Creature * l, const Creature * r ) {
        return rate_critter( *l ) > rate_critter( *r );
    } );

    const tripoint diff = best.pos() - u.pos();
    if( abs( diff.x ) <= 1 && abs( diff.y ) <= 1 && diff.z == 0 ) {
        plmove( diff.x, diff.y );
        return;
    }

    u.reach_attack( best.pos() );
}

void game::fling_creature( Creature *c, const int &dir, float flvel, bool controlled )
{
    if( c == nullptr ) {
        debugmsg( "game::fling_creature invoked on null target" );
        return;
    }

    if( c->is_dead_state() ) {
        // Flinging a corpse causes problems, don't enable without testing
        return;
    }

    if( c->is_hallucination() ) {
        // Don't fling hallucinations
        return;
    }

    int steps = 0;
    bool thru = true;
    const bool is_u = ( c == &u );
    // Don't animate critters getting bashed if animations are off
    const bool animate = is_u || get_option<bool>( "ANIMATIONS" );

    player *p = dynamic_cast<player *>( c );

    tileray tdir( dir );
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
        float force = 0;

        if( monster *const mon_ptr = critter_at<monster>( pt ) ) {
            monster &critter = *mon_ptr;
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
            if( !m.veh_at( pt ).obstacle_at_part() ) {
                force = std::min<float>( m.bash_strength( pt ), flvel );
            } else {
                // No good way of limiting force here
                // Keep it 1 less than maximum to make the impact hurt
                // but to keep the target flying after it
                force = flvel - 1;
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
            } else if( !critter_at( pt ) ) {
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
        if( animate && ( seen || u.sees( *c ) ) ) {
            draw();
        }
    }

    // Fall down to the ground - always on the last reached tile
    if( !m.has_flag( "SWIMMABLE", c->pos() ) ) {
        const trap_id trap_under_creature = m.tr_at( c->pos() ).loadid;
        // Didn't smash into a wall or a floor so only take the fall damage
        if( thru && trap_under_creature == tr_ledge ) {
            m.creature_on_trap( *c, false );
        } else {
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
            // Always apply traps to creature i.e. bear traps, tele traps etc.
            m.creature_on_trap( *c, false );
        }
    } else {
        c->underwater = true;
        if( is_u ) {
            if( controlled ) {
                add_msg( _( "You dive into water." ) );
            } else {
                add_msg( m_warning, _( "You fall into water." ) );
            }
        }
    }
}

cata::optional<tripoint> point_selection_menu( const std::vector<tripoint> &pts )
{
    if( pts.empty() ) {
        debugmsg( "point_selection_menu called with empty point set" );
        return cata::nullopt;
    }

    if( pts.size() == 1 ) {
        return pts[0];
    }

    const tripoint &upos = g->u.pos();
    uilist pmenu;
    pmenu.title = _( "Climb where?" );
    int num = 0;
    for( const tripoint &pt : pts ) {
        // TODO: Sort the menu so that it can be used with numpad directions
        const std::string &direction = direction_name( direction_from( upos.x, upos.y, pt.x, pt.y ) );
        // TODO: Inform player what is on said tile
        // But don't just print terrain name (in many cases it will be "open air")
        pmenu.addentry( num++, true, MENU_AUTOASSIGN, _( "Climb %s" ), direction.c_str() );
    }

    pmenu.query();
    const int ret = pmenu.ret;
    if( ret < 0 || ret >= num ) {
        return cata::nullopt;
    }

    return pts[ret];
}

void game::vertical_move( int movez, bool force )
{
    // Check if there are monsters are using the stairs.
    bool slippedpast = false;
    if( !m.has_zlevels() && !coming_to_stairs.empty() && !force ) {
        // TODO: Allow travel if zombie couldn't reach stairs, but spawn him when we go up.
        add_msg( m_warning, _( "You try to use the stairs. Suddenly you are blocked by a %s!" ),
                 coming_to_stairs[0].name().c_str() );
        // Roll.
        ///\EFFECT_DEX increases chance of moving past monsters on stairs

        ///\EFFECT_DODGE increases chance of moving past monsters on stairs
        int dexroll = dice( 6, u.dex_cur + u.get_skill_level( skill_dodge ) * 2 );
        ///\EFFECT_STR increases chance of moving past monsters on stairs

        ///\EFFECT_MELEE increases chance of moving past monsters on stairs
        int strroll = dice( 3, u.str_cur + u.get_skill_level( skill_melee ) * 1.5 );
        if( coming_to_stairs.size() > 4 ) {
            add_msg( _( "The are a lot of them on the %s!" ), m.tername( u.pos() ).c_str() );
            dexroll /= 4;
            strroll /= 2;
        } else if( coming_to_stairs.size() > 1 ) {
            add_msg( m_warning, _( "There's something else behind it!" ) );
            dexroll /= 2;
        }

        if( dexroll < 14 || strroll < 12 ) {
            update_stair_monsters();
            u.moves -= 100;
            return;
        }

        if( dexroll >= 14 ) {
            add_msg( _( "You manage to slip past!" ) );
        } else if( strroll >= 12 ) {
            add_msg( _( "You manage to push past!" ) );
        }
        slippedpast = true;
        u.moves -= 100;
    }

    // > and < are used for diving underwater.
    if( m.has_flag( "SWIMMABLE", u.pos() ) && m.has_flag( TFLAG_DEEP_WATER, u.pos() ) ) {
        if( movez == -1 ) {
            if( u.is_underwater() ) {
                add_msg( m_info, _( "You are already underwater!" ) );
                return;
            }
            if( u.worn_with_flag( "FLOTATION" ) ) {
                add_msg( m_info, _( "You can't dive while wearing a flotation device." ) );
                return;
            }
            u.set_underwater( true );
            ///\EFFECT_STR increases breath-holding capacity while diving
            u.oxygen = 30 + 2 * u.str_cur;
            add_msg( _( "You dive underwater!" ) );
        } else {
            if( u.swim_speed() < 500 || u.shoe_type_count( "swim_fins" ) ) {
                u.set_underwater( false );
                add_msg( _( "You surface." ) );
            } else {
                add_msg( m_info, _( "You try to surface but can't!" ) );
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
            add_msg( m_info, _( "You can't climb here - there's a ceiling above your head" ) );
            return;
        }

        const int cost = u.climbing_cost( u.pos(), stairs );
        if( cost == 0 ) {
            add_msg( m_info, _( "You can't climb here - you need walls and/or furniture to brace against" ) );
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
            add_msg( m_info,
                     _( "You can't climb here - there is no terrain above you that would support your weight" ) );
            return;
        } else {
            // TODO: Make it an extended action
            climbing = true;
            move_cost = cost;

            const cata::optional<tripoint> pnt = point_selection_menu( pts );
            if( !pnt ) {
                return;
            }
            stairs = *pnt;
        }
    }

    if( !force && movez == -1 && !m.has_flag( "GOES_DOWN", u.pos() ) ) {
        add_msg( m_info, _( "You can't go down here!" ) );
        return;
    } else if( !climbing && !force && movez == 1 && !m.has_flag( "GOES_UP", u.pos() ) ) {
        add_msg( m_info, _( "You can't go up here!" ) );
        return;
    }

    if( force ) {
        // Let go of a grabbed cart.
        u.grab( OBJECT_NONE );
    } else if( u.grab_point != tripoint_zero ) {
        add_msg( m_info, _( "You can't drag things up and down stairs." ) );
        return;
    }

    // Because get_levz takes z-value from the map, it will change when vertical_shift (m.has_zlevels() == true)
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
        maybetmp.load( get_levx(), get_levy(), z_after, false );
    }

    // Find the corresponding staircase
    bool rope_ladder = false;
    // TODO: Remove the stairfinding, make the mapgen gen aligned maps
    if( !force && !climbing ) {
        const cata::optional<tripoint> pnt = find_or_make_stairs( maybetmp, z_after, rope_ladder );
        if( !pnt ) {
            return;
        }
        stairs = *pnt;
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
        for( monster &critter : all_monsters() ) {
            int turns = critter.turns_to_reach( to_x, to_y );
            if( turns < 10 && coming_to_stairs.size() < 8 && critter.will_reach( to_x, to_y )
                && !slippedpast ) {
                critter.staircount = 10 + turns;
                critter.on_unload();
                coming_to_stairs.push_back( critter );
                remove_zombie( critter );
            }
        }

        shift_monsters( 0, 0, movez );
    }

    std::vector<std::shared_ptr<npc>> npcs_to_bring;
    std::vector<monster *> monsters_following;
    if( !m.has_zlevels() && abs( movez ) == 1 ) {
        std::copy_if( active_npc.begin(), active_npc.end(), back_inserter( npcs_to_bring ),
        [this]( const std::shared_ptr<npc> &np ) {
            return np->is_friend() && rl_dist( np->pos(), u.pos() ) < 2;
        } );
    }

    if( m.has_zlevels() && abs( movez ) == 1 ) {
        for( monster &critter : all_monsters() ) {
            if( critter.attack_target() == &g->u ) {
                monsters_following.push_back( &critter );
            }
        }
    }

    u.moves -= move_cost;

    const tripoint old_pos = g->u.pos();
    point submap_shift = point_zero;
    vertical_shift( z_after );
    if( !force ) {
        submap_shift = update_map( stairs.x, stairs.y );
    }
    const tripoint adjusted_pos( old_pos.x - submap_shift.x * SEEX, old_pos.y - submap_shift.y * SEEY,
                                 old_pos.z );

    if( !npcs_to_bring.empty() ) {
        // Would look nicer randomly scrambled
        auto candidates = closest_tripoints_first( 1, u.pos() );
        std::remove_if( candidates.begin(), candidates.end(), [this]( const tripoint & c ) {
            return !is_empty( c );
        } );

        for( const auto &np : npcs_to_bring ) {
            const auto found = std::find_if( candidates.begin(), candidates.end(),
            [this, np]( const tripoint & c ) {
                return !np->is_dangerous_fields( m.field_at( c ) ) && m.tr_at( c ).is_benign();
            } );

            if( found != candidates.end() ) {
                // @todo: De-uglify
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

    // This ugly check is here because of stair teleport bullshit
    // @todo: Remove stair teleport bullshit
    if( rl_dist( g->u.pos(), old_pos ) <= 1 ) {
        for( monster *m : monsters_following ) {
            m->set_dest( g->u.pos() );
        }
    }

    if( rope_ladder ) {
        m.ter_set( u.pos(), t_rope_up );
    }

    if( m.ter( stairs ) == t_manhole_cover ) {
        m.spawn_item( stairs + point( rng( -1, 1 ), rng( -1, 1 ) ), "manhole_cover" );
        m.ter_set( stairs, t_manhole );
    }

    // Wouldn't work and may do strange things
    if( u.is_hauling() && !m.has_zlevels() ) {
        add_msg( _( "You cannot haul items here." ) );
        u.stop_hauling();
    }

    if( u.is_hauling() ) {
        u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
        // Whether the source is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Whether the destination is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Source relative to the player
        u.activity.placement = adjusted_pos - u.pos();
        // Destination relative to the player
        u.activity.coords.push_back( tripoint_zero );
        map_stack items = m.i_at( adjusted_pos );
        if( items.empty() ) {
            std::cout << "no items" << std::endl;
            u.stop_hauling();
        }
        int index = 0;
        for( auto it = items.begin(); it != items.end(); ++index, ++it ) {
            int amount = it->count();
            u.activity.values.push_back( index );
            u.activity.values.push_back( amount );
        }
    }

    refresh_all();
    // Upon force movement, traps can not be avoided.
    m.creature_on_trap( u, !force );
}

cata::optional<tripoint> game::find_or_make_stairs( map &mp, const int z_after, bool &rope_ladder )
{
    const int omtilesz = SEEX * 2;
    real_coords rc( m.getabs( u.posx(), u.posy() ) );
    tripoint omtile_align_start( m.getlocal( rc.begin_om_pos() ), z_after );
    tripoint omtile_align_end( omtile_align_start.x + omtilesz - 1, omtile_align_start.y + omtilesz - 1,
                               omtile_align_start.z );

    // Try to find the stairs.
    cata::optional<tripoint> stairs;
    int best = INT_MAX;
    const int movez = z_after - get_levz();
    Creature *blocking_creature = nullptr;
    for( const tripoint &dest : m.points_in_rectangle( omtile_align_start, omtile_align_end ) ) {
        if( rl_dist( u.pos(), dest ) <= best &&
            ( ( movez == -1 && mp.has_flag( "GOES_UP", dest ) ) ||
              ( movez == 1 && ( mp.has_flag( "GOES_DOWN", dest ) ||
                                mp.ter( dest ) == t_manhole_cover ) ) ||
              ( ( movez == 2 || movez == -2 ) && mp.ter( dest ) == t_elevator ) ) ) {
            if( mp.has_zlevels() && critter_at( dest ) ) {
                blocking_creature = critter_at( dest );
                continue;
            }
            stairs.emplace( dest );
            best = rl_dist( u.pos(), dest );
        }
    }

    if( stairs ) {
        // Stairs found
        return stairs;
    }

    if( blocking_creature ) {
        add_msg( _( "There's a %s in the way!" ), blocking_creature->disp_name().c_str() );
        return cata::nullopt;
    }
    // No stairs found! Try to make some
    rope_ladder = false;
    stairs.emplace( u.pos() );
    stairs->z = z_after;
    // Check the destination area for lava.
    if( mp.ter( *stairs ) == t_lava ) {
        if( movez < 0 &&
            !query_yn(
                _( "There is a LOT of heat coming out of there, even the stairs have melted away.  Jump down?  You won't be able to get back up." ) ) ) {
            return cata::nullopt;
        } else if( movez > 0 &&
                   !query_yn(
                       _( "There is a LOT of heat coming out of there.  Push through the half-molten rocks and ascend?  You will not be able to get back down." ) ) ) {
            return cata::nullopt;
        }

        return stairs;
    }

    if( movez > 0 ) {
        if( !mp.has_flag( "GOES_DOWN", *stairs ) ) {
            if( !query_yn( _( "You may be unable to return back down these stairs.  Continue up?" ) ) ) {
                return cata::nullopt;
            }
        }
        // Manhole covers need this to work
        // Maybe require manhole cover here and fail otherwise?
        return stairs;
    }

    if( mp.impassable( *stairs ) ) {
        popup( _( "Halfway down, the way down becomes blocked off." ) );
        return cata::nullopt;
    }

    if( u.has_trait( trait_id( "WEB_RAPPEL" ) ) ) {
        if( query_yn( _( "There is a sheer drop halfway down. Web-descend?" ) ) ) {
            rope_ladder = true;
            if( ( rng( 4, 8 ) ) < u.get_skill_level( skill_dodge ) ) {
                add_msg( _( "You attach a web and dive down headfirst, flipping upright and landing on your feet." ) );
            } else {
                add_msg( _( "You securely web up and work your way down, lowering yourself safely." ) );
            }
        } else {
            return cata::nullopt;
        }
    } else if( u.has_trait( trait_VINES2 ) || u.has_trait( trait_VINES3 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down.  Use your vines to descend?" ) ) ) {
            if( u.has_trait( trait_VINES2 ) ) {
                if( query_yn( _( "Detach a vine?  It'll hurt, but you'll be able to climb back up..." ) ) ) {
                    rope_ladder = true;
                    add_msg( m_bad, _( "You descend on your vines, though leaving a part of you behind stings." ) );
                    u.mod_pain( 5 );
                    u.apply_damage( nullptr, bp_torso, 5 );
                    u.mod_hunger( 10 );
                    u.mod_thirst( 10 );
                } else {
                    add_msg( _( "You gingerly descend using your vines." ) );
                }
            } else {
                add_msg( _( "You effortlessly lower yourself and leave a vine rooted for future use." ) );
                rope_ladder = true;
                u.mod_hunger( 10 );
                u.mod_thirst( 10 );
            }
        } else {
            return cata::nullopt;
        }
    } else if( u.has_amount( "grapnel", 1 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down. Climb your grappling hook down?" ) ) ) {
            rope_ladder = true;
            u.use_amount( "grapnel", 1 );
        } else {
            return cata::nullopt;
        }
    } else if( u.has_amount( "rope_30", 1 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down. Climb your rope down?" ) ) ) {
            rope_ladder = true;
            u.use_amount( "rope_30", 1 );
        } else {
            return cata::nullopt;
        }
    } else if( !query_yn( _( "There is a sheer drop halfway down.  Jump?" ) ) ) {
        return cata::nullopt;
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
    u.grab( OBJECT_NONE );

    scent.reset();

    u.setz( z_after );
    const int z_before = get_levz();
    if( !m.has_zlevels() ) {
        m.clear_vehicle_cache( z_before );
        m.access_cache( z_before ).vehicle_list.clear();
        m.access_cache( z_before ).zone_vehicles.clear();
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
    for( int x = -REVEAL_RADIUS; x <= REVEAL_RADIUS; x++ ) {
        for( int y = -REVEAL_RADIUS; y <= REVEAL_RADIUS; y++ ) {
            const int cursx = gpos.x + x;
            const int cursy = gpos.y + y;
            if( !overmap_buffer.seen( cursx, cursy, z_before ) ) {
                continue;
            }
            if( overmap_buffer.has_note( cursx, cursy, z_after ) ) {
                // Already has a note -> never add an AUTO-note
                continue;
            }
            const oter_id &ter = overmap_buffer.ter( cursx, cursy, z_before );
            const oter_id &ter2 = overmap_buffer.ter( cursx, cursy, z_after );
            if( z_after > z_before && ter->has_flag( known_up ) &&
                !ter2->has_flag( known_down ) ) {
                overmap_buffer.set_seen( cursx, cursy, z_after, true );
                overmap_buffer.add_note( cursx, cursy, z_after, string_format( ">:W;%s", _( "AUTO: goes down" ) ) );
            } else if( z_after < z_before && ter->has_flag( known_down ) &&
                       !ter2->has_flag( known_up ) ) {
                overmap_buffer.set_seen( cursx, cursy, z_after, true );
                overmap_buffer.add_note( cursx, cursy, z_after, string_format( "<:W;%s", _( "AUTO: goes up" ) ) );
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

point game::update_map( int &x, int &y )
{
    int shiftx = 0;
    int shifty = 0;

    while( x < HALF_MAPSIZE_X ) {
        x += SEEX;
        shiftx--;
    }
    while( x >= HALF_MAPSIZE_X + SEEX ) {
        x -= SEEX;
        shiftx++;
    }
    while( y < HALF_MAPSIZE_Y ) {
        y += SEEY;
        shifty--;
    }
    while( y >= HALF_MAPSIZE_Y + SEEY ) {
        y -= SEEY;
        shifty++;
    }

    if( shiftx == 0 && shifty == 0 ) {
        // adjust player position
        u.setpos( tripoint( x, y, get_levz() ) );
        // Not actually shifting the submaps, all the stuff below would do nothing
        return point_zero;
    }

    // this handles loading/unloading submaps that have scrolled on or off the viewport
    m.shift( shiftx, shifty );

    // Shift monsters
    shift_monsters( shiftx, shifty, 0 );
    u.shift_destination( -shiftx * SEEX, -shifty * SEEY );

    // Shift NPCs
    for( auto it = active_npc.begin(); it != active_npc.end(); ) {
        ( *it )->shift( shiftx, shifty );
        if( ( *it )->posx() < 0 - SEEX * 2 || ( *it )->posy() < 0 - SEEX * 2 ||
            ( *it )->posx() > SEEX * ( MAPSIZE + 2 ) || ( *it )->posy() > SEEY * ( MAPSIZE + 2 ) ) {
            //Remove the npc from the active list. It remains in the overmap list.
            ( *it )->on_unload();
            it = active_npc.erase( it );
        } else {
            it++;
        }
    }

    scent.shift( shiftx * SEEX, shifty * SEEY );

    // Also ensure the player is on current z-level
    // get_levz() should later be removed, when there is no longer such a thing
    // as "current z-level"
    u.setpos( tripoint( x, y, get_levz() ) );

    // Only do the loading after all coordinates have been shifted.

    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();

    // Make sure map cache is consistent since it may have shifted.
    m.build_map_cache( get_levz() );

    // Spawn monsters if appropriate
    // This call will generate new monsters in addition to loading, so it's placed after NPC loading
    m.spawn_monsters( false ); // Static monsters

    // Update what parts of the world map we can see
    update_overmap_seen();

    return point( shiftx, shifty );
}

void game::update_overmap_seen()
{
    const tripoint ompos = u.global_omt_location();
    const int dist = u.overmap_sight_range( light_level( u.posz() ) );
    // We can always see where we're standing
    overmap_buffer.set_seen( ompos.x, ompos.y, ompos.z, true );
    for( int x = ompos.x - dist; x <= ompos.x + dist; x++ ) {
        for( int y = ompos.y - dist; y <= ompos.y + dist; y++ ) {
            const std::vector<point> line = line_to( ompos.x, ompos.y, x, y, 0 );
            int sight_points = dist;
            for( auto it = line.begin();
                 it != line.end() && sight_points >= 0; ++it ) {
                const oter_id &ter = overmap_buffer.ter( it->x, it->y, ompos.z );
                sight_points -= int( ter->get_see_cost() );
            }
            if( sight_points >= 0 ) {
                overmap_buffer.set_seen( x, y, ompos.z, true );
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
    std::vector<int> stairx;
    std::vector<int> stairy;
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

    for( int x = 0; x < MAPSIZE_X; x++ ) {
        for( int y = 0; y < MAPSIZE_Y; y++ ) {
            tripoint dest( x, y, u.posz() );
            if( ( from_below && m.has_flag( "GOES_DOWN", dest ) ) ||
                ( !from_below && m.has_flag( "GOES_UP", dest ) ) ) {
                stairx.push_back( x );
                stairy.push_back( y );
                stairdist.push_back( rl_dist( dest, u.pos() ) );
            }
        }
    }
    if( stairdist.empty() ) {
        return;         // Found no stairs?
    }

    // Find closest stairs.
    size_t si = 0;
    for( size_t i = 0; i < stairdist.size(); i++ ) {
        if( stairdist[i] < stairdist[si] ) {
            si = i;
        }
    }

    // Find up to 4 stairs for distance stairdist[si] +1
    std::vector<int> nearest;
    nearest.push_back( si );
    for( size_t i = 0; i < stairdist.size() && nearest.size() < 4; i++ ) {
        if( ( i != si ) && ( stairdist[i] <= stairdist[si] + 1 ) ) {
            nearest.push_back( i );
        }
    }
    // Randomize the stair choice
    si = random_entry_ref( nearest );

    // Attempt to spawn zombies.
    for( size_t i = 0; i < coming_to_stairs.size(); i++ ) {
        int mposx = stairx[si];
        int mposy = stairy[si];
        monster &critter = coming_to_stairs[i];
        const tripoint dest {
            mposx, mposy, g->get_levz()
        };

        // We might be not be visible.
        if( ( critter.posx() < 0 - ( MAPSIZE_X ) / 6 ||
              critter.posy() < 0 - ( MAPSIZE_Y ) / 6 ||
              critter.posx() > ( MAPSIZE_X * 7 ) / 6 ||
              critter.posy() > ( MAPSIZE_Y * 7 ) / 6 ) ) {
            continue;
        }

        critter.staircount -= 4;
        // Let the player know zombies are trying to come.
        if( u.sees( dest ) ) {
            std::stringstream dump;
            if( critter.staircount > 4 ) {
                dump << string_format( _( "You see a %s on the stairs" ), critter.name().c_str() );
            } else {
                if( critter.staircount > 0 ) {
                    dump << ( from_below ?
                              //~ The <monster> is almost at the <bottom/top> of the <terrain type>!
                              string_format( _( "The %1$s is almost at the top of the %2$s!" ),
                                             critter.name().c_str(),
                                             m.tername( dest ).c_str() ) :
                              string_format( _( "The %1$s is almost at the bottom of the %2$s!" ),
                                             critter.name().c_str(),
                                             m.tername( dest ).c_str() ) );
                }
            }

            add_msg( m_warning, dump.str().c_str() );
        } else {
            sounds::sound( dest, 5, sounds::sound_t::movement,
                           _( "a sound nearby from the stairs!" ) );
        }

        if( critter.staircount > 0 ) {
            continue;
        }

        if( is_empty( dest ) ) {
            critter.spawn( dest );
            critter.staircount = 0;
            add_zombie( critter );
            if( u.sees( dest ) ) {
                if( !from_below ) {
                    add_msg( m_warning, _( "The %1$s comes down the %2$s!" ),
                             critter.name().c_str(),
                             m.tername( dest ).c_str() );
                } else {
                    add_msg( m_warning, _( "The %1$s comes up the %2$s!" ),
                             critter.name().c_str(),
                             m.tername( dest ).c_str() );
                }
            }
            coming_to_stairs.erase( coming_to_stairs.begin() + i );
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
            while( tries < creature_push_attempts ) {
                tries++;
                pushx = rng( -1, 1 );
                pushy = rng( -1, 1 );
                int iposx = mposx + pushx;
                int iposy = mposy + pushy;
                tripoint pos( iposx, iposy, get_levz() );
                if( ( pushx != 0 || pushy != 0 ) && !critter_at( pos ) &&
                    critter.can_move_to( pos ) ) {
                    bool resiststhrow = ( u.is_throw_immune() ) ||
                                        ( u.has_trait( trait_LEG_TENT_BRACE ) );
                    if( resiststhrow && one_in( player_throw_resist_chance ) ) {
                        u.moves -= 25; // small charge for avoiding the push altogether
                        add_msg( _( "The %s fails to push you back!" ),
                                 critter.name().c_str() );
                        return; //judo or leg brace prevent you from getting pushed at all
                    }
                    // Not accounting for tentacles latching on, so..
                    // Something is about to happen, lets charge half a move
                    u.moves -= 50;
                    if( resiststhrow && ( u.is_throw_immune() ) ) {
                        //we have a judoka who isn't getting pushed but counterattacking now.
                        mattack::thrown_by_judo( &critter );
                        return;
                    }
                    std::string msg;
                    ///\EFFECT_DODGE reduces chance of being downed when pushed off the stairs
                    if( !( resiststhrow ) && ( u.get_dodge() + rng( 0, 3 ) < 12 ) ) {
                        // dodge 12 - never get downed
                        // 11.. avoid 75%; 10.. avoid 50%; 9.. avoid 25%
                        u.add_effect( effect_downed, 2_turns );
                        msg = _( "The %s pushed you back hard!" );
                    } else {
                        msg = _( "The %s pushed you back!" );
                    }
                    add_msg( m_warning, msg.c_str(), critter.name().c_str() );
                    u.setx( u.posx() + pushx );
                    u.sety( u.posy() + pushy );
                    return;
                }
            }
            add_msg( m_warning,
                     _( "The %s tried to push you back but failed! It attacks you!" ),
                     critter.name().c_str() );
            critter.melee_attack( u );
            u.moves -= 50;
            return;
        } else if( monster *const mon_ptr = critter_at<monster>( dest ) ) {
            // Monster attempts to displace a monster from the stairs
            monster &other = *mon_ptr;
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
                pushx = rng( -1, 1 );
                pushy = rng( -1, 1 );
                int iposx = mposx + pushx;
                int iposy = mposy + pushy;
                tripoint pos( iposx, iposy, get_levz() );
                if( ( pushx == 0 && pushy == 0 ) || ( ( iposx == u.posx() ) && ( iposy == u.posy() ) ) ) {
                    continue;
                }
                if( !critter_at( pos ) && other.can_move_to( pos ) ) {
                    other.setpos( tripoint( iposx, iposy, get_levz() ) );
                    other.moves -= 50;
                    std::string msg;
                    if( one_in( creature_throw_resist ) ) {
                        other.add_effect( effect_downed, 2_turns );
                        msg = _( "The %1$s pushed the %2$s hard." );
                    } else {
                        msg = _( "The %1$s pushed the %2$s." );
                    }
                    add_msg( msg.c_str(), critter.name().c_str(), other.name().c_str() );
                    return;
                }
            }
            return;
        }
    }
}

void game::despawn_monster( monster &critter )
{
    if( !critter.is_hallucination() ) {
        // hallucinations aren't stored, they come and go as they like,
        overmap_buffer.despawn_monster( critter );
    }

    critter.on_unload();
    remove_zombie( critter );
}

void game::shift_monsters( const int shiftx, const int shifty, const int shiftz )
{
    // If either shift argument is non-zero, we're shifting.
    if( shiftx == 0 && shifty == 0 && shiftz == 0 ) {
        return;
    }
    for( monster &critter : all_monsters() ) {
        if( shiftx != 0 || shifty != 0 ) {
            critter.shift( shiftx, shifty );
        }

        if( m.inbounds( critter.pos() ) && ( shiftz == 0 || m.has_zlevels() ) ) {
            // We're inbounds, so don't despawn after all.
            // No need to shift Z-coordinates, they are absolute
            continue;
        }
        // Either a vertical shift or the critter is now outside of the reality bubble,
        // anyway: it must be saved and removed.
        despawn_monster( critter );
    }
    // The order in which zombies are shifted may cause zombies to briefly exist on
    // the same square. This messes up the mon_at cache, so we need to rebuild it.
    rebuild_mon_at_cache();
}

void game::perhaps_add_random_npc()
{
    if( !calendar::once_every( 1_hours ) ) {
        return;
    }
    // Create a new NPC?
    // Only allow NPCs on 0 z-level, otherwise they can bug out due to lack of spots
    if( !get_option<bool>( "RANDOM_NPC" ) || ( !m.has_zlevels() && get_levz() != 0 ) ) {
        return;
    }

    float density = get_option<float>( "NPC_DENSITY" );
    //@todo This is inaccurate when the player is near a overmap border, and it will
    //immediately spawn new npcs upon entering a new overmap. Rather use number of npcs *nearby*.
    const int npc_num = get_cur_om().get_npcs().size();
    if( npc_num > 0 ) {
        // 100%, 80%, 64%, 52%, 41%, 33%...
        density *= powf( 0.8f, npc_num );
    }

    if( !x_in_y( density, 100 ) ) {
        return;
    }

    std::shared_ptr<npc> tmp = std::make_shared<npc>();
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
    switch( rng( 0, 4 ) ) { // on which side of the map to spawn
        case 0:
            msy += rng( 0, MAPSIZE - 1 );
            break;
        case 1:
            msx += MAPSIZE - 1;
            msy += rng( 0, MAPSIZE - 1 );
            break;
        case 2:
            msx += rng( 0, MAPSIZE - 1 );
            break;
        case 3:
            msy += MAPSIZE - 1;
            msx += rng( 0, MAPSIZE - 1 );
            break;
        default:
            break;
    }
    // adds the npc to the correct overmap.
    tmp->spawn_at_sm( msx, msy, 0 );
    overmap_buffer.insert_npc( tmp );
    tmp->form_opinion( u );
    tmp->mission = NPC_MISSION_NULL;
    tmp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, tmp->global_omt_location(),
                          tmp->getID() ) );
    // This will make the new NPC active
    load_npcs();
}

void game::teleport( player *p, bool add_teleglow )
{
    if( p == nullptr ) {
        p = &u;
    }
    int tries = 0;
    tripoint new_pos = p->pos();
    bool is_u = ( p == &u );

    if( add_teleglow ) {
        p->add_effect( effect_teleglow, 30_minutes );
    }
    do {
        new_pos.x = p->posx() + rng( 0, SEEX * 2 ) - SEEX;
        new_pos.y = p->posy() + rng( 0, SEEY * 2 ) - SEEY;
        tries++;
    } while( tries < 15 && m.impassable( new_pos ) );
    bool can_see = ( is_u || u.sees( new_pos ) );
    if( p->in_vehicle ) {
        m.unboard_vehicle( p->pos() );
    }
    p->setx( new_pos.x );
    p->sety( new_pos.y );
    if( m.impassable( new_pos ) ) { //Teleported into a wall
        if( can_see ) {
            if( is_u ) {
                add_msg( _( "You teleport into the middle of a %s!" ),
                         m.obstacle_name( new_pos ).c_str() );
                p->add_memorial_log( pgettext( "memorial_male", "Teleported into a %s." ),
                                     pgettext( "memorial_female", "Teleported into a %s." ),
                                     m.obstacle_name( new_pos ).c_str() );
            } else {
                add_msg( _( "%1$s teleports into the middle of a %2$s!" ),
                         p->name.c_str(), m.obstacle_name( new_pos ).c_str() );
            }
        }
        p->apply_damage( nullptr, bp_torso, 500 );
        p->check_dead_state();
    } else if( can_see ) {
        if( monster *const mon_ptr = critter_at<monster>( new_pos ) ) {
            monster &critter = *mon_ptr;
            if( is_u ) {
                add_msg( _( "You teleport into the middle of a %s!" ),
                         critter.name().c_str() );
                u.add_memorial_log( pgettext( "memorial_male", "Telefragged a %s." ),
                                    pgettext( "memorial_female", "Telefragged a %s." ),
                                    critter.name().c_str() );
            } else {
                add_msg( _( "%1$s teleports into the middle of a %2$s!" ),
                         p->name.c_str(), critter.name().c_str() );
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
    tmpmap.load( x * 2, y * 2, 0, false );
    tripoint dest( 0, 0, p.z );
    int &i = dest.x;
    int &j = dest.y;
    for( i = 0; i < SEEX * 2; i++ ) {
        for( j = 0; j < SEEY * 2; j++ ) {
            if( !one_in( 10 ) ) {
                tmpmap.make_rubble( dest, f_rubble_rock, true, t_dirt, true );
            }
            if( one_in( 3 ) ) {
                tmpmap.add_field( dest, fd_nuke_gas, 3 );
            }
            tmpmap.adjust_radiation( dest, rng( 20, 80 ) );
        }
    }
    tmpmap.save();
    overmap_buffer.ter( x, y, 0 ) = oter_id( "crater" );
    // Kill any npcs on that omap location.
    for( const auto &npc : overmap_buffer.get_npcs_near_omt( x, y, 0, 0 ) ) {
        npc->marked_for_death = true;
    }
}

void game::display_scent()
{
    int div;
    bool got_value = query_int( div, _( "Set the Scent Map sensitivity to (0 to cancel)?" ) );
    if( !got_value || div < 1 ) {
        add_msg( _( "Never mind." ) );
        return;
    }
    draw_ter();
    scent.draw( w_terrain, div * 2, u.pos() + u.view_offset );
    wrefresh( w_terrain );
    inp_mngr.wait_for_any_key();
}

void game::init_autosave()
{
    moves_since_last_save = 0;
    last_save_timestamp = time( nullptr );
}

void game::quicksave()
{
    //Don't autosave if the player hasn't done anything since the last autosave/quicksave,
    if( !moves_since_last_save ) {
        return;
    }
    add_msg( m_info, _( "Saving game, this may take a while" ) );
    popup_nowait( _( "Saving game, this may take a while" ) );

    time_t now = time( nullptr ); //timestamp for start of saving procedure

    //perform save
    save();
    //Now reset counters for autosaving, so we don't immediately autosave after a quicksave or autosave.
    moves_since_last_save = 0;
    last_save_timestamp = now;
}

void game::quickload()
{
    const WORLDPTR active_world = world_generator->active_world;
    if( active_world == nullptr ) {
        return;
    }

    if( active_world->save_exists( save_t::from_player_name( u.name ) ) ) {
        if( moves_since_last_save != 0 ) { // See if we need to reload anything
            MAPBUFFER.reset();
            overmap_buffer.clear();
            try {
                setup();
            } catch( const std::exception &err ) {
                debugmsg( "Error: %s", err.what() );
            }
            load( save_t::from_player_name( u.name ) );
        }
    } else {
        popup_getkey( _( "No saves for %s yet." ), u.name.c_str() );
    }
}

void game::autosave()
{
    //Don't autosave if the min-autosave interval has not passed since the last autosave/quicksave.
    if( time( nullptr ) < last_save_timestamp + 60 * get_option<int>( "AUTOSAVE_MINUTES" ) ) {
        return;
    }
    quicksave();    //Driving checks are handled by quicksave()
}

void intro()
{
    int maxy = getmaxy( catacurses::stdscr );
    int maxx = getmaxx( catacurses::stdscr );
    const int minHeight = FULL_SCREEN_HEIGHT;
    const int minWidth = FULL_SCREEN_WIDTH;
    catacurses::window tmp = catacurses::newwin( minHeight, minWidth, 0, 0 );

    while( maxy < minHeight || maxx < minWidth ) {
        werase( tmp );
        if( maxy < minHeight && maxx < minWidth ) {
            fold_and_print( tmp, 0, 0, maxx, c_white,
                            _( "Whoa! Your terminal is tiny! This game requires a minimum terminal size of "
                               "%dx%d to work properly. %dx%d just won't do. Maybe a smaller font would help?" ),
                            minWidth, minHeight, maxx, maxy );
        } else if( maxx < minWidth ) {
            fold_and_print( tmp, 0, 0, maxx, c_white,
                            _( "Oh! Hey, look at that. Your terminal is just a little too narrow. This game "
                               "requires a minimum terminal size of %dx%d to function. It just won't work "
                               "with only %dx%d. Can you stretch it out sideways a bit?" ),
                            minWidth, minHeight, maxx, maxy );
        } else {
            fold_and_print( tmp, 0, 0, maxx, c_white,
                            _( "Woah, woah, we're just a little short on space here. The game requires a "
                               "minimum terminal size of %dx%d to run. %dx%d isn't quite enough! Can you "
                               "make the terminal just a smidgen taller?" ),
                            minWidth, minHeight, maxx, maxy );
        }
        wrefresh( tmp );
        inp_mngr.wait_for_any_key();
        maxy = getmaxy( catacurses::stdscr );
        maxx = getmaxx( catacurses::stdscr );
    }
    werase( tmp );

#if !(defined _WIN32 || defined WINDOWS || defined TILES)
    // Check whether LC_CTYPE supports the UTF-8 encoding
    // and show a warning if it doesn't
    if( std::strcmp( nl_langinfo( CODESET ), "UTF-8" ) != 0 ) {
        const char *unicode_error_msg =
            _( "You don't seem to have a valid Unicode locale. You may see some weird "
               "characters (e.g. empty boxes or question marks). You have been warned." );
        fold_and_print( tmp, 0, 0, maxx, c_white, unicode_error_msg, minWidth, minHeight, maxx, maxy );
        wrefresh( tmp );
        inp_mngr.wait_for_any_key();
        werase( tmp );
    }
#endif

    wrefresh( tmp );
    catacurses::erase();
}

void game::process_artifact( item &it, player &p )
{
    const bool worn = p.is_worn( it );
    const bool wielded = ( &it == &p.weapon );
    std::vector<art_effect_passive> effects = it.type->artifact->effects_carried;
    if( worn ) {
        auto &ew = it.type->artifact->effects_worn;
        effects.insert( effects.end(), ew.begin(), ew.end() );
    }
    if( wielded ) {
        auto &ew = it.type->artifact->effects_wielded;
        effects.insert( effects.end(), ew.begin(), ew.end() );
    }
    if( it.is_tool() ) {
        // Recharge it if necessary
        if( it.ammo_remaining() < it.ammo_capacity() && calendar::once_every( 1_minutes ) ) {
            //Before incrementing charge, check that any extra requirements are met
            if( check_art_charge_req( it ) ) {
                switch( it.type->artifact->charge_type ) {
                    case ARTC_NULL:
                    case NUM_ARTCS:
                        break; // dummy entries
                    case ARTC_TIME:
                        // Once per hour
                        if( calendar::once_every( 1_hours ) ) {
                            it.charges++;
                        }
                        break;
                    case ARTC_SOLAR:
                        if( calendar::once_every( 10_minutes ) &&
                            is_in_sunlight( p.pos() ) ) {
                            it.charges++;
                        }
                        break;
                    // Artifacts can inflict pain even on Deadened folks.
                    // Some weird Lovecraftian thing.  ;P
                    // (So DON'T route them through mod_pain!)
                    case ARTC_PAIN:
                        if( calendar::once_every( 1_minutes ) ) {
                            add_msg( m_bad, _( "You suddenly feel sharp pain for no reason." ) );
                            p.mod_pain_noresist( 3 * rng( 1, 3 ) );
                            it.charges++;
                        }
                        break;
                    case ARTC_HP:
                        if( calendar::once_every( 1_minutes ) ) {
                            add_msg( m_bad, _( "You feel your body decaying." ) );
                            p.hurtall( 1, nullptr );
                            it.charges++;
                        }
                        break;
                    case ARTC_FATIGUE:
                        if( calendar::once_every( 1_minutes ) ) {
                            add_msg( m_bad, _( "You feel fatigue seeping into your body." ) );
                            u.mod_fatigue( 3 * rng( 1, 3 ) );
                            u.mod_stat( "stamina", -9 * rng( 1, 3 ) * rng( 1, 3 ) * rng( 2, 3 ) );
                            it.charges++;
                        }
                        break;
                    // Portals are energetic enough to charge the item.
                    // Tears in reality are consumed too, but can't charge it.
                    case ARTC_PORTAL:
                        for( const tripoint &dest : m.points_in_radius( p.pos(), 1 ) ) {
                            m.remove_field( dest, fd_fatigue );
                            if( m.tr_at( dest ).loadid == tr_portal ) {
                                add_msg( m_good, _( "The portal collapses!" ) );
                                m.remove_trap( dest );
                                it.charges++;
                                break;
                            }
                        }
                        break;
                }
            }
        }
    }

    for( auto &i : effects ) {
        switch( i ) {
            case AEP_STR_UP:
                p.mod_str_bonus( +4 );
                break;
            case AEP_DEX_UP:
                p.mod_dex_bonus( +4 );
                break;
            case AEP_PER_UP:
                p.mod_per_bonus( +4 );
                break;
            case AEP_INT_UP:
                p.mod_int_bonus( +4 );
                break;
            case AEP_ALL_UP:
                p.mod_str_bonus( +2 );
                p.mod_dex_bonus( +2 );
                p.mod_per_bonus( +2 );
                p.mod_int_bonus( +2 );
                break;
            case AEP_SPEED_UP: // Handled in player::current_speed()
                break;

            case AEP_PBLUE:
                if( p.radiation > 0 ) {
                    p.radiation--;
                }
                break;

            case AEP_SMOKE:
                if( one_in( 10 ) ) {
                    tripoint pt( p.posx() + rng( -1, 1 ),
                                 p.posy() + rng( -1, 1 ),
                                 p.posz() );
                    m.add_field( pt, fd_smoke, rng( 1, 3 ) );
                }
                break;

            case AEP_SNAKES:
                break; // Handled in player::hit()

            case AEP_EXTINGUISH:
                for( const tripoint &dest : m.points_in_radius( p.pos(), 1 ) ) {
                    m.adjust_field_age( dest, fd_fire, -1_turns );
                }
                break;

            case AEP_FUN:
                //Bonus fluctuates, wavering between 0 and 30-ish - usually around 12
                p.add_morale( MORALE_FEELING_GOOD, rng( 1, 2 ) * rng( 2, 3 ), 0, 3_turns, 0_turns, false );
                break;

            case AEP_HUNGER:
                if( one_in( 100 ) ) {
                    p.mod_hunger( 1 );
                }
                break;

            case AEP_THIRST:
                if( one_in( 120 ) ) {
                    p.mod_thirst( 1 );
                }
                break;

            case AEP_EVIL:
                if( one_in( 150 ) ) { // Once every 15 minutes, on average
                    p.add_effect( effect_evil, 30_minutes );
                    if( it.is_armor() ) {
                        if( !worn ) {
                            add_msg( _( "You have an urge to wear the %s." ),
                                     it.tname().c_str() );
                        }
                    } else if( !wielded ) {
                        add_msg( _( "You have an urge to wield the %s." ),
                                 it.tname().c_str() );
                    }
                }
                break;

            case AEP_SCHIZO:
                break; // Handled in player::suffer()

            case AEP_RADIOACTIVE:
                if( one_in( 4 ) ) {
                    p.irradiate( 1.0f );
                }
                break;

            case AEP_STR_DOWN:
                p.mod_str_bonus( -3 );
                break;

            case AEP_DEX_DOWN:
                p.mod_dex_bonus( -3 );
                break;

            case AEP_PER_DOWN:
                p.mod_per_bonus( -3 );
                break;

            case AEP_INT_DOWN:
                p.mod_int_bonus( -3 );
                break;

            case AEP_ALL_DOWN:
                p.mod_str_bonus( -2 );
                p.mod_dex_bonus( -2 );
                p.mod_per_bonus( -2 );
                p.mod_int_bonus( -2 );
                break;

            case AEP_SPEED_DOWN:
                break; // Handled in player::current_speed()

            default:
                //Suppress warnings
                break;
        }
    }
    // Recalculate, as it might have changed (by mod_*_bonus above)
    p.str_cur = p.get_str();
    p.int_cur = p.get_int();
    p.dex_cur = p.get_dex();
    p.per_cur = p.get_per();
}
//Check if an artifact's extra charge requirements are currently met
bool check_art_charge_req( item &it )
{
    player &p = g->u;
    bool reqsmet = true;
    const bool worn = p.is_worn( it );
    const bool wielded = ( &it == &p.weapon );
    const bool heldweapon = ( wielded && !it.is_armor() ); //don't charge wielded clothes
    switch( it.type->artifact->charge_req ) {
        case( ACR_NULL ):
        case( NUM_ACRS ):
            break;
        case( ACR_EQUIP ):
            //Generated artifacts won't both be wearable and have charges, but nice for mods
            reqsmet = ( worn || heldweapon );
            break;
        case( ACR_SKIN ):
            //As ACR_EQUIP, but also requires nothing worn on bodypart wielding or wearing item
            if( !worn && !heldweapon ) {
                reqsmet = false;
                break;
            }
            for( const body_part bp : all_body_parts ) {
                if( it.covers( bp ) || ( heldweapon && ( bp == bp_hand_r || bp == bp_hand_l ) ) ) {
                    reqsmet = true;
                    for( auto &i : p.worn ) {
                        if( i.covers( bp ) && ( &it != &i ) && i.get_coverage() > 50 ) {
                            reqsmet = false;
                            break; //This one's no good, check the next body part
                        }
                    }
                    if( reqsmet ) {
                        break;    //Only need skin contact on one bodypart
                    }
                }
            }
            break;
        case( ACR_SLEEP ):
            reqsmet = p.has_effect( effect_sleep );
            break;
        case( ACR_RAD ):
            reqsmet = ( ( g->m.get_radiation( p.pos() ) > 0 ) || ( p.radiation > 0 ) );
            break;
        case( ACR_WET ):
            reqsmet = std::any_of( p.body_wetness.begin(), p.body_wetness.end(),
            []( const int w ) {
                return w != 0;
            } );
            if( !reqsmet && sum_conditions( calendar::turn - 1_turns, calendar::turn, p.pos() ).rain_amount > 0
                && !( p.in_vehicle && g->m.veh_at( p.pos() )->is_inside() ) ) {
                reqsmet = true;
            }
            break;
        case( ACR_SKY ):
            reqsmet = ( p.posz() > 0 );
            break;
    }
    return reqsmet;
}

void game::start_calendar()
{
    calendar::start = HOURS( get_option<int>( "INITIAL_TIME" ) );
    const bool scen_season = scen->has_flag( "SPR_START" ) || scen->has_flag( "SUM_START" ) ||
                             scen->has_flag( "AUT_START" ) || scen->has_flag( "WIN_START" ) ||
                             scen->has_flag( "SUM_ADV_START" );
    const std::string nonscen_season = get_option<std::string>( "INITIAL_SEASON" );
    if( scen->has_flag( "SPR_START" ) || ( !scen_season && nonscen_season == "spring" ) ) {
        calendar::initial_season = SPRING;
    } else if( scen->has_flag( "SUM_START" ) || ( !scen_season && nonscen_season == "summer" ) ) {
        calendar::initial_season = SUMMER;
        calendar::start += to_turns<int>( calendar::season_length() );
    } else if( scen->has_flag( "AUT_START" ) || ( !scen_season && nonscen_season == "autumn" ) ) {
        calendar::initial_season = AUTUMN;
        calendar::start += to_turns<int>( calendar::season_length() * 2 );
    } else if( scen->has_flag( "WIN_START" ) || ( !scen_season && nonscen_season == "winter" ) ) {
        calendar::initial_season = WINTER;
        calendar::start += to_turns<int>( calendar::season_length() * 3 );
    } else if( scen->has_flag( "SUM_ADV_START" ) ) {
        calendar::initial_season = SUMMER;
        calendar::start += to_turns<int>( calendar::season_length() * 5 );
    } else {
        debugmsg( "The Unicorn" );
    }
    calendar::turn = calendar::start;
}

void game::add_artifact_messages( const std::vector<art_effect_passive> &effects )
{
    int net_str = 0;
    int net_dex = 0;
    int net_per = 0;
    int net_int = 0;
    int net_speed = 0;

    for( auto &i : effects ) {
        switch( i ) {
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
                add_msg( m_warning, _( "Your skin feels slithery." ) );
                break;

            case AEP_INVISIBLE:
                add_msg( m_good, _( "You fade into invisibility!" ) );
                break;

            case AEP_CLAIRVOYANCE:
                add_msg( m_good, _( "You can see through walls!" ) );
                break;

            case AEP_CLAIRVOYANCE_PLUS:
                add_msg( m_good, _( "You can see through walls!" ) );
                break;

            case AEP_SUPER_CLAIRVOYANCE:
                add_msg( m_good, _( "You can see through everything!" ) );
                break;

            case AEP_STEALTH:
                add_msg( m_good, _( "Your steps stop making noise." ) );
                break;

            case AEP_GLOW:
                add_msg( _( "A glow of light forms around you." ) );
                break;

            case AEP_PSYSHIELD:
                add_msg( m_good, _( "Your mental state feels protected." ) );
                break;

            case AEP_RESIST_ELECTRICITY:
                add_msg( m_good, _( "You feel insulated." ) );
                break;

            case AEP_CARRY_MORE:
                add_msg( m_good, _( "Your back feels strengthened." ) );
                break;

            case AEP_FUN:
                add_msg( m_good, _( "You feel a pleasant tingle." ) );
                break;

            case AEP_HUNGER:
                add_msg( m_warning, _( "You feel hungry." ) );
                break;

            case AEP_THIRST:
                add_msg( m_warning, _( "You feel thirsty." ) );
                break;

            case AEP_EVIL:
                add_msg( m_warning, _( "You feel an evil presence..." ) );
                break;

            case AEP_SCHIZO:
                add_msg( m_bad, _( "You feel a tickle of insanity." ) );
                break;

            case AEP_RADIOACTIVE:
                add_msg( m_warning, _( "Your skin prickles with radiation." ) );
                break;

            case AEP_MUTAGENIC:
                add_msg( m_bad, _( "You feel your genetic makeup degrading." ) );
                break;

            case AEP_ATTENTION:
                add_msg( m_warning, _( "You feel an otherworldly attention upon you..." ) );
                break;

            case AEP_FORCE_TELEPORT:
                add_msg( m_bad, _( "You feel a force pulling you inwards." ) );
                break;

            case AEP_MOVEMENT_NOISE:
                add_msg( m_warning, _( "You hear a rattling noise coming from inside yourself." ) );
                break;

            case AEP_BAD_WEATHER:
                add_msg( m_warning, _( "You feel storms coming." ) );
                break;

            case AEP_SICK:
                add_msg( m_bad, _( "You feel unwell." ) );
                break;

            case AEP_SMOKE:
                add_msg( m_warning, _( "A cloud of smoke appears." ) );
                break;
            default:
                //Suppress warnings
                break;
        }
    }

    std::string stat_info;
    if( net_str != 0 ) {
        stat_info += string_format( _( "Str %s%d! " ),
                                    ( net_str > 0 ? "+" : "" ), net_str );
    }
    if( net_dex != 0 ) {
        stat_info += string_format( _( "Dex %s%d! " ),
                                    ( net_dex > 0 ? "+" : "" ), net_dex );
    }
    if( net_int != 0 ) {
        stat_info += string_format( _( "Int %s%d! " ),
                                    ( net_int > 0 ? "+" : "" ), net_int );
    }
    if( net_per != 0 ) {
        stat_info += string_format( _( "Per %s%d! " ),
                                    ( net_per > 0 ? "+" : "" ), net_per );
    }

    if( !stat_info.empty() ) {
        add_msg( stat_info.c_str() );
    }

    if( net_speed != 0 ) {
        add_msg( m_info, _( "Speed %s%d! " ), ( net_speed > 0 ? "+" : "" ), net_speed );
    }
}

void game::add_artifact_dreams( )
{
    //If player is sleeping, get a dream from a carried artifact
    //Don't need to check that player is sleeping here, that's done before calling
    std::list<item *> art_items = g->u.get_artifact_items();
    std::vector<item *>      valid_arts;
    std::vector<std::vector<std::string>>
                                       valid_dreams; // Tracking separately so we only need to check its req once
    //Pull the list of dreams
    add_msg( m_debug, "Checking %s carried artifacts", art_items.size() );
    for( auto &it : art_items ) {
        //Pick only the ones with an applicable dream
        auto art = it->type->artifact;
        if( art->charge_req != ACR_NULL && ( it->ammo_remaining() < it->ammo_capacity() ||
                                             it->ammo_capacity() == 0 ) ) { //or max 0 in case of wacky mod shenanigans
            add_msg( m_debug, "Checking artifact %s", it->tname() );
            if( check_art_charge_req( *it ) ) {
                add_msg( m_debug, "   Has freq %s,%s", art->dream_freq_met, art->dream_freq_unmet );
                if( art->dream_freq_met   > 0 && x_in_y( art->dream_freq_met,   100 ) ) {
                    add_msg( m_debug, "Adding met dream from %s", it->tname() );
                    valid_arts.push_back( it );
                    valid_dreams.push_back( art->dream_msg_met );
                }
            } else {
                add_msg( m_debug, "   Has freq %s,%s", art->dream_freq_met, art->dream_freq_unmet );
                if( art->dream_freq_unmet > 0 && x_in_y( art->dream_freq_unmet, 100 ) ) {
                    add_msg( m_debug, "Adding unmet dream from %s", it->tname() );
                    valid_arts.push_back( it );
                    valid_dreams.push_back( art->dream_msg_unmet );
                }
            }
        }
    }
    if( !valid_dreams.empty() ) {
        add_msg( m_debug, "Found %s valid artifact dreams", valid_dreams.size() );
        const int selected = rng( 0, valid_arts.size() - 1 );
        auto it = valid_arts[selected];
        auto msg = random_entry( valid_dreams[selected] );
        const std::string &dream = string_format( _( msg.c_str() ), it->tname().c_str() );
        add_msg( dream );
    } else {
        add_msg( m_debug, "Didn't have any dreams, sorry" );
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
    const tripoint sm = m.get_abs_sub() + tripoint( HALF_MAPSIZE, HALF_MAPSIZE, 0 );
    const tripoint pos_om = sm_to_om_copy( sm );
    return overmap_buffer.get( pos_om.x, pos_om.y );
}

std::vector<npc *> game::allies()
{
    return get_npcs_if( [&]( const npc & guy ) {
        return guy.is_friend();
    } );
}

std::vector<Creature *> game::get_creatures_if( const std::function<bool( const Creature & )>
        &pred )
{
    std::vector<Creature *> result;
    for( Creature &critter : all_creatures() ) {
        if( pred( critter ) ) {
            result.push_back( &critter );
        }
    }
    return result;
}

std::vector<npc *> game::get_npcs_if( const std::function<bool( const npc & )> &pred )
{
    std::vector<npc *> result;
    for( npc &guy : all_npcs() ) {
        if( pred( guy ) ) {
            result.push_back( &guy );
        }
    }
    return result;
}

template<>
bool game::non_dead_range<monster>::iterator::valid()
{
    current = iter->lock();
    return current && !current->is_dead();
}

template<>
bool game::non_dead_range<npc>::iterator::valid()
{
    current = iter->lock();
    return current && !current->is_dead();
}

template<>
bool game::non_dead_range<Creature>::iterator::valid()
{
    current = iter->lock();
    // There is no Creature::is_dead function, so we can't write
    // return current && !current->is_dead();
    if( !current ) {
        return false;
    }
    if( const monster *const ptr = dynamic_cast<monster *>( current.get() ) ) {
        return !ptr->is_dead();
    }
    if( const npc *const ptr = dynamic_cast<npc *>( current.get() ) ) {
        return !ptr->is_dead();
    }
    return true; // must be g->u
}

game::monster_range::monster_range( game &g )
{
    const auto &monsters = g.critter_tracker->get_monsters_list();
    items.insert( items.end(), monsters.begin(), monsters.end() );
}

game::Creature_range::Creature_range( game &g ) : u( &g.u, []( player * ) { } )
{
    const auto &monsters = g.critter_tracker->get_monsters_list();
    items.insert( items.end(), monsters.begin(), monsters.end() );
    items.insert( items.end(), g.active_npc.begin(), g.active_npc.end() );
    items.push_back( u );
}

game::npc_range::npc_range( game &g )
{
    items.insert( items.end(), g.active_npc.begin(), g.active_npc.end() );
}

game::Creature_range game::all_creatures()
{
    return Creature_range( *this );
}

game::monster_range game::all_monsters()
{
    return monster_range( *this );
}

game::npc_range game::all_npcs()
{
    return npc_range( *this );
}

Creature *game::get_creature_if( const std::function<bool( const Creature & )> &pred )
{
    for( Creature &critter : all_creatures() ) {
        if( pred( critter ) ) {
            return &critter;
        }
    }
    return nullptr;
}

std::string game::get_player_base_save_path() const
{
    return get_world_base_save_path() + "/" + base64_encode( u.name );
}

std::string game::get_world_base_save_path() const
{
    return world_generator->active_world->folder_path();
}
