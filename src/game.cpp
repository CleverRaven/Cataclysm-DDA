#include "game.h"

#include <wctype.h>
#include <assert.h>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iterator>
#include <locale>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <unordered_map>

#include "action.h"
#include "activity_handlers.h"
#include "artifact.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "bionics.h"
#include "bodypart.h"
#include "cata_utility.h"
#include "auto_note.h"
#include "catacharset.h"
#include "clzones.h"
#include "computer.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cursesport.h"
#include "debug.h"
#include "dependency_tree.h"
#include "editmap.h"
#include "enums.h"
#include "faction.h"
#include "filesystem.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "game_ui.h"
#include "gamemode.h"
#include "gates.h"
#include "harvest.h"
#include "help.h"
#include "iexamine.h"
#include "init.h"
#include "input.h"
#include "item_category.h"
#include "item_location.h"
#include "iuse_actor.h"
#include "json.h"
#include "kill_tracker.h"
#include "lightmap.h"
#include "line.h"
#include "live_view.h"
#include "loading_ui.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "monattack.h"
#include "monexamine.h"
#include "monstergenerator.h"
#include "morale_types.h"
#include "mtype.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "panels.h"
#include "path_info.h"
#include "pickup.h"
#include "popup.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "scent_map.h"
#include "scores_ui.h"
#include "sdltiles.h"
#include "sounds.h"
#include "start_location.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "submap.h"
#include "timed_event.h"
#include "translations.h"
#include "trap.h"
#include "uistate.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "worldfactory.h"
#include "map_selector.h"
#include "basecamp.h"
#include "character.h"
#include "color.h"
#include "damage.h"
#include "field.h"
#include "item_stack.h"
#include "itype.h"
#include "iuse.h"
#include "player.h"
#include "player_activity.h"
#include "recipe.h"
#include "ret_val.h"
#include "tileray.h"
#include "ui.h"
#include "units.h"
#include "int_id.h"
#include "string_id.h"
#include "colony.h"
#include "item.h"

class inventory;

#if defined(TILES)
#include "cata_tiles.h"
#endif // TILES

#if !(defined(_WIN32) || defined(TILES))
#include <langinfo.h>
#include <cstring>
#endif

#if defined(_WIN32)
#if 1 // Hack to prevent reordering of #include "platform_win.h" by IWYU
#   include "platform_win.h"
#endif
#   include <tchar.h>
#endif

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

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
const skill_id skill_electronics( "electronics" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_computer( "computer" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id PLANT( "PLANT" );

const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
const efftype_id effect_alarm_clock( "alarm_clock" );
const efftype_id effect_amigara( "amigara" );
const efftype_id effect_assisted( "assisted" );
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
const efftype_id effect_hot( "hot" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_laserlocked( "laserlocked" );
const efftype_id effect_no_sight( "no_sight" );
const efftype_id effect_onfire( "onfire" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_paid( "paid" );
const efftype_id effect_pet( "pet" );
const efftype_id effect_relax_gas( "relax_gas" );
const efftype_id effect_sleep( "sleep" );
const efftype_id effect_stunned( "stunned" );
const efftype_id effect_teleglow( "teleglow" );
const efftype_id effect_tetanus( "tetanus" );
const efftype_id effect_visuals( "visuals" );
const efftype_id effect_winded( "winded" );
const efftype_id effect_ridden( "ridden" );
const efftype_id effect_riding( "riding" );
const efftype_id effect_has_bag( "has_bag" );
const efftype_id effect_harnessed( "harnessed" );

static const bionic_id bio_remote( "bio_remote" );

static const trait_id trait_GRAZER( "GRAZER" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_PARKOUR( "PARKOUR" );
static const trait_id trait_RUMINANT( "RUMINANT" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );

const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

static const faction_id your_followers( "your_followers" );

void intro();

#if defined(__ANDROID__)
extern std::map<std::string, std::list<input_event>> quick_shortcuts_map;
extern bool add_best_key_for_action_to_quick_shortcuts( action_id action,
        const std::string &category, bool back );
extern bool add_key_to_quick_shortcuts( int key, const std::string &category, bool back );
#endif

//The one and only game instance
std::unique_ptr<game> g;

//The one and only uistate instance
uistatedata uistate;

bool is_valid_in_w_terrain( const point &p )
{
    return p.x >= 0 && p.x < TERRAIN_WINDOW_WIDTH && p.y >= 0 && p.y < TERRAIN_WINDOW_HEIGHT;
}

// This is the main game set-up process.
game::game() :
    liveview( *liveview_ptr ),
    scent_ptr( *this ),
    m( *map_ptr ),
    u( *u_ptr ),
    scent( *scent_ptr ),
    timed_events( *timed_event_manager_ptr ),
    uquit( QUIT_NO ),
    new_game( false ),
    displaying_scent( false ),
    displaying_temperature( false ),
    displaying_visibility( false ),
    displaying_radiation( false ),
    safe_mode( SAFE_MODE_ON ),
    pixel_minimap_option( 0 ),
    mostseen( 0 ),
    u_shared_ptr( &u, null_deleter{} ),
    safe_mode_warning_logged( false ),
    next_npc_id( 1 ),
    next_mission_id( 1 ),
    remoteveh_cache_time( calendar::before_time_starts ),
    user_action_counter( 0 ),
    tileset_zoom( DEFAULT_TILESET_ZOOM ),
    last_mouse_edge_scroll( std::chrono::steady_clock::now() )
{
    player_was_sleeping = false;
    reset_light_level();
    events().subscribe( &*stats_tracker_ptr );
    events().subscribe( &*kill_tracker_ptr );
    events().subscribe( &*memorial_logger_ptr );
    events().subscribe( &*spell_events_ptr );
    world_generator = std::make_unique<worldfactory>();
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
    fullscreen = false;
    was_fullscreen = false;
    show_panel_adm = false;
    panel_manager::get_manager().init();

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
        world_generator->set_active_world( nullptr );
        world_generator->init();
        const std::vector<mod_id> mods_empty;
        WORLDPTR test_world = world_generator->make_new_world( mods_empty );
        world_generator->set_active_world( test_world );

        // if no loadable mods then test core data only
        try {
            load_core_data( ui );
            DynamicDataLoader::get_instance().finalize_loaded_data( ui );
        } catch( const std::exception &err ) {
            std::cerr << "Error loading data from json: " << err.what() << std::endl;
        }

        std::string world_name = world_generator->active_world->world_name;
        world_generator->delete_world( world_name, true );

        MAPBUFFER.reset();
        overmap_buffer.clear();
    }

    for( const auto &e : check ) {
        world_generator->set_active_world( nullptr );
        world_generator->init();
        const std::vector<mod_id> mods_empty;
        WORLDPTR test_world = world_generator->make_new_world( mods_empty );
        world_generator->set_active_world( test_world );

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

        std::string world_name = world_generator->active_world->world_name;
        world_generator->delete_world( world_name, true );

        MAPBUFFER.reset();
        overmap_buffer.clear();
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

    load_data_from_dir( FILENAMES[ "jsondir" ], "core", ui );
}

void game::load_data_from_dir( const std::string &path, const std::string &src, loading_ui &ui )
{
    DynamicDataLoader::get_instance().load_data_from_path( path, src, ui );
}

game::~game()
{
    MAPBUFFER.reset();
}

// Fixed window sizes
#define MINIMAP_HEIGHT 7
#define MINIMAP_WIDTH 7

void game::init_ui( const bool resized )
{
    // clear the screen
    static bool first_init = true;

    if( first_init ) {
        catacurses::clear();

        // print an intro screen, making sure the terminal is the correct size

        first_init = false;

#if defined(TILES)
        //class variable to track the option being active
        //only set once, toggle action is used to change during game
        pixel_minimap_option = get_option<bool>( "PIXEL_MINIMAP" );
#endif // TILES
    }

    // First get TERMX, TERMY
#if defined(TILES) || defined(_WIN32)
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

#endif
    // remove some space for the sidebar, this is the maximal space
    // (using standard font) that the terrain window can have
    int sidebar_left = panel_manager::get_manager().get_width_left();
    int sidebar_right = panel_manager::get_manager().get_width_right();

    TERRAIN_WINDOW_HEIGHT = TERMY;
    TERRAIN_WINDOW_WIDTH = TERMX - ( sidebar_left + sidebar_right );
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

    w_terrain = w_terrain_ptr = catacurses::newwin( TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH,
                                point( VIEW_OFFSET_X + sidebar_left, VIEW_OFFSET_Y ) );
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

    // minimap is always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
    int _y = VIEW_OFFSET_Y;
    int _x = VIEW_OFFSET_X;

    w_minimap = w_minimap_ptr = catacurses::newwin( MINIMAP_HEIGHT, MINIMAP_WIDTH, point( _x, _y ) );
    werase( w_minimap );

    w_panel_adm = w_panel_adm_ptr = catacurses::newwin( 20, 75, point( ( TERMX / 2 ) - 38,
                                    ( TERMY / 2 ) - 10 ) );
    werase( w_panel_adm );
    // need to init in order to avoid crash. gets updated by the panel code.
    w_pixel_minimap = catacurses::newwin( 1, 1, point_zero );
    liveview.init();

    // Only refresh if we are in-game, otherwise all resources are not initialized
    // and this refresh will crash the game.
    if( resized && u.getID().is_valid() ) {
        refresh_all();
    }
}

void game::toggle_fullscreen()
{
#if !defined(TILES)
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
#if defined(TILES)
    if( pixel_minimap_option ) {
        clear_window_area( w_pixel_minimap );
    }
    pixel_minimap_option = !pixel_minimap_option;
    init_ui();
    refresh_all();
#endif // TILES
}

void game::toggle_panel_adm()
{
    show_panel_adm = !show_panel_adm;
}

void game::reload_tileset()
{
#if defined(TILES)
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

    m = map( get_option<bool>( "ZLEVELS" ) );

    next_npc_id = character_id( 1 );
    next_mission_id = 1;
    new_game = true;
    uquit = QUIT_NO;   // We haven't quit the game
    bVMonsterLookFire = true;

    // invalidate calendar caches in case we were previously playing
    // a different world
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

    weather.weather = WEATHER_CLEAR; // Start with some nice weather...
    // Weather shift in 30
    weather.nextweather = calendar::start_of_cataclysm + time_duration::from_hours(
                              get_option<int>( "INITIAL_TIME" ) ) + 30_minutes;

    turnssincelastmon = 0; //Auto safe mode init

    sounds::reset_sounds();
    clear_zombies();
    coming_to_stairs.clear();
    active_npc.clear();
    faction_manager_ptr->clear();
    mission::clear_all();
    Messages::clear_messages();
    timed_events = timed_event_manager();

    SCT.vSCT.clear(); //Delete pending messages

    stats().clear();
    // reset kill counts
    kill_tracker_ptr->clear();
    // reset follower list
    follower_ids.clear();
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
    m.load( pos_sm, true );
}

// Set up all default values for a new game
bool game::start_game()
{
    if( !gamemode ) {
        gamemode = std::make_unique<special_game>();
    }

    seed = rng_bits();
    new_game = true;
    start_calendar();
    weather.nextweather = calendar::turn;
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

    if( scen->has_map_extra() ) {
        // Map extras can add monster spawn points and similar and should be done before the main
        // map is loaded.
        start_loc.add_map_extra( omtstart, scen->get_map_extra() );
    }

    tripoint lev = omt_to_sm_copy( omtstart );
    // The player is centered in the map, but lev[xyz] refers to the top left point of the map
    lev.x -= HALF_MAPSIZE;
    lev.y -= HALF_MAPSIZE;
    load_map( lev );

    m.invalidate_map_cache( get_levz() );
    m.build_map_cache( get_levz() );
    // Do this after the map cache has been built!
    start_loc.place_player( u );
    // ...but then rebuild it, because we want visibility cache to avoid spawning monsters in sight
    m.invalidate_map_cache( get_levz() );
    m.build_map_cache( get_levz() );
    // Start the overmap with out immediate neighborhood visible, this needs to be after place_player
    overmap_buffer.reveal( u.global_omt_location().xy(),
                           get_option<int>( "DISTANCE_INITIAL_VISIBILITY" ), 0 );

    u.moves = 0;
    u.process_turn(); // process_turn adds the initial move points
    u.stamina = u.get_stamina_max();
    weather.temperature = SPRING_TEMPERATURE;
    weather.update_weather();
    u.next_climate_control_check = calendar::before_time_starts; // Force recheck at startup
    u.last_climate_control_ret = false;

    //Reset character safe mode/pickup rules
    get_auto_pickup().clear_character_rules();
    get_safemode().clear_character_rules();
    get_auto_notes_settings().clear();
    get_auto_notes_settings().default_initialize();

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
    for( auto &e : u.inv_dump() ) {
        e->set_owner( g->u.get_faction() );
    }
    // Now that we're done handling coordinates, ensure the player's submap is in the center of the map
    update_map( u );
    // Profession pets
    for( const mtype_id &elem : u.starting_pets ) {
        if( monster *const mon = place_critter_around( elem, u.pos(), 5 ) ) {
            mon->friendly = -1;
            mon->add_effect( effect_pet, 1_turns, num_bp, true );
        } else {
            add_msg( m_debug, "cannot place starting pet, no space!" );
        }
    }
    // Assign all of this scenario's missions to the player.
    for( const mission_type_id &m : scen->missions() ) {
        const auto mission = mission::reserve_new( m, character_id() );
        mission->assign( u );
    }

    g->events().send<event_type::game_start>( u.getID() );
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
        if( ( temp->has_companion_mission() ) && ( temp->mission != NPC_MISSION_TRAVELLING ) ) {
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

const kill_tracker &game::get_kill_tracker() const
{
    return *kill_tracker_ptr;
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
    tmp->spawn_at_precise( { get_levx(), get_levy() }, u.pos() - point_south_east );
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
    if( uquit == QUIT_DIED || uquit == QUIT_SUICIDE ) {
        // Put (non-hallucinations) into the overmap so they are not lost.
        for( monster &critter : all_monsters() ) {
            despawn_monster( critter );
        }
        // Reset NPC factions and disposition
        reset_npc_dispositions();
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
            vRip.emplace_back( R"(           _________  ____           )" );
            vRip.emplace_back( R"(         _/         `/    \_         )" );
            vRip.emplace_back( R"(       _/      _     _      \_.      )" );
            vRip.emplace_back( R"(     _%\      |_) | |_)       \_     )" );
            vRip.emplace_back( R"(   _/ \/      | \ | |           \_   )" );
            vRip.emplace_back( R"( _/                               \_ )" );
            vRip.emplace_back( R"(|                                   |)" );
            iNameLine = vRip.size();
            vRip.emplace_back( R"( )                                 < )" );
            vRip.emplace_back( R"(|                                   |)" );
            vRip.emplace_back( R"(|                                   |)" );
            vRip.emplace_back( R"(|   _                               |)" );
            vRip.emplace_back( R"(|__/                                |)" );
            iMaxWidth = vRip[vRip.size() - 1].length();
            vRip.emplace_back( R"( / `--.                             |)" );
            vRip.emplace_back( R"(|                                  ( )" );
            iInfoLine = vRip.size();
            vRip.emplace_back( R"(|                                   |)" );
            vRip.emplace_back( R"(|                                   |)" );
            vRip.emplace_back( R"(|     %                         .   |)" );
            vRip.emplace_back( R"(|  @`                            %% |)" );
            vRip.emplace_back( R"(| %@%@%\                *      %`%@%|)" );
            vRip.emplace_back( R"(%%@@@.%@%\%%            `\  %%.%%@@%@)" );
            vRip.emplace_back( R"(@%@@%%%%%@@@@@@%%%%%%%%@@%%@@@%%%@%%@)" );
        }

        const int iOffsetX = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;
        const int iOffsetY = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;

        catacurses::window w_rip = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                   point( iOffsetX, iOffsetY ) );
        draw_border( w_rip );

        sfx::do_player_death_hurt( g->u, true );
        sfx::fade_audio_group( sfx::group::weather, 2000 );
        sfx::fade_audio_group( sfx::group::time_of_day, 2000 );
        sfx::fade_audio_group( sfx::group::context_themes, 2000 );
        sfx::fade_audio_group( sfx::group::fatigue, 2000 );

        for( size_t iY = 0; iY < vRip.size(); ++iY ) {
            for( size_t iX = 0; iX < vRip[iY].length(); ++iX ) {
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

                    mvwputch( w_rip, point( iX + FULL_SCREEN_WIDTH / 2 - ( iMaxWidth / 2 ), iY + 1 ), ncColor,
                              vRip[iY][iX] );
                }
            }
        }

        std::string sTemp;

        center_print( w_rip, iInfoLine++, c_white, _( "Survived:" ) );

        const time_duration survived = calendar::turn - calendar::start_of_cataclysm;
        const int minutes = to_minutes<int>( survived ) % 60;
        const int hours = to_hours<int>( survived ) % 24;
        const int days = to_days<int>( survived );

        if( days > 0 ) {
            sTemp = string_format( "%dd %dh %dm", days, hours, minutes );
        } else if( hours > 0 ) {
            sTemp = string_format( "%dh %dm", hours, minutes );
        } else {
            sTemp = string_format( "%dm", minutes );
        }

        center_print( w_rip, iInfoLine++, c_white, sTemp );

        const int iTotalKills = get_kill_tracker().monster_kill_count();

        sTemp = _( "Kills:" );
        mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - 5, 1 + iInfoLine++ ), c_light_gray,
                   ( sTemp + " " ) );
        wprintz( w_rip, c_magenta, "%d", iTotalKills );

        sTemp = _( "In memory of:" );
        mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ),
                   c_light_gray,
                   sTemp );

        sTemp = u.name;
        mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ), c_white,
                   sTemp );

        sTemp = _( "Last Words:" );
        mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ),
                   c_light_gray,
                   sTemp );

        int iStartX = FULL_SCREEN_WIDTH / 2 - ( ( iMaxWidth - 4 ) / 2 );
        std::string sLastWords = string_input_popup()
                                 .window( w_rip, iStartX, iNameLine, iStartX + iMaxWidth - 4 - 1 )
                                 .max_length( iMaxWidth - 4 - 1 )
                                 .query_string();
        death_screen();
        const bool is_suicide = uquit == QUIT_SUICIDE;
        events().send<event_type::game_over>( is_suicide, sLastWords );
        // Struck the save_player_data here to forestall Weirdness
        move_save_to_graveyard();
        write_memorial_file( sLastWords );
        memorial().clear();
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
            std::string tmpmessage;
            for( auto &character : characters ) {
                tmpmessage += "\n  ";
                tmpmessage += character;
            }
            popup( _( "World retained. Characters remaining:%s" ), tmpmessage );
        }
        if( gamemode ) {
            gamemode = std::make_unique<special_game>(); // null gamemode or something..
        }
    }

    //clear all sound channels
    sfx::fade_audio_channel( sfx::channel::any, 300 );
    sfx::fade_audio_group( sfx::group::weather, 300 );
    sfx::fade_audio_group( sfx::group::time_of_day, 300 );
    sfx::fade_audio_group( sfx::group::context_themes, 300 );
    sfx::fade_audio_group( sfx::group::fatigue, 300 );

    MAPBUFFER.reset();
    overmap_buffer.clear();

#if defined(__ANDROID__)
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
    const point offset_difference( -driving_view_offset + point( offset.x, offset.y ) );

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
        calendar::turn += 1_turns;
    }

    // starting a new turn, clear out temperature cache
    weather.temperature_cache.clear();

    if( npcs_dirty ) {
        load_npcs();
    }

    timed_events.process();
    mission::process_all();
    // If controlling a vehicle that is owned by someone else
    if( u.in_vehicle && u.controlling_vehicle ) {
        vehicle *veh = veh_pointer_or_null( m.veh_at( u.pos() ) );
        if( veh && !veh->handle_potential_theft( dynamic_cast<player &>( u ), true ) ) {
            veh->handle_potential_theft( dynamic_cast<player &>( u ), false, false );
        }
    }
    if( calendar::once_every( 1_days ) ) {
        overmap_buffer.process_mongroups();
    }

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

    weather.update_weather();
    reset_light_level();

    perhaps_add_random_npc();
    process_activity();
    // Process NPC sound events before they move or they hear themselves talking
    for( npc &guy : all_npcs() ) {
        if( rl_dist( guy.pos(), u.pos() ) < MAX_VIEW_DISTANCE ) {
            sounds::process_sound_markers( &guy );
        }
    }

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
                for( npc &guy : all_npcs() ) {
                    if( rl_dist( guy.pos(), u.pos() ) < MAX_VIEW_DISTANCE ) {
                        sounds::process_sound_markers( &guy );
                    }
                }
                sounds::process_sound_markers( &u );
                if( !u.activity && !u.has_distant_destination() && uquit != QUIT_WATCH ) {
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
            // Rate limit key polling to 10 times a second.
            static auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now() );
            const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now() );
            if( ( now - start ).count() > 100 ) {
                handle_key_blocking_activity();
                start = now;
            }

            // If player is performing a task and a monster is dangerously close, warn them
            // regardless of previous safemode warnings
            if( u.activity && !u.has_activity( activity_id( "ACT_AIM" ) ) &&
                u.activity.moves_left > 0 &&
                !u.activity.is_distraction_ignored( distraction_type::hostile_spotted ) ) {
                Creature *hostile_critter = is_hostile_very_close();
                if( hostile_critter != nullptr ) {
                    cancel_activity_or_ignore_query( distraction_type::hostile_spotted,
                                                     string_format( _( "The %s is dangerously close!" ),
                                                             hostile_critter->get_name() ) );
                }
            }

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
    following_vehicles();
    m.vehmove();

    // Process power and fuel consumption for all vehicles, including off-map ones.
    // m.vehmove used to do this, but now it only give them moves instead.
    for( auto &elem : MAPBUFFER ) {
        tripoint sm_loc = elem.first;
        point sm_topleft = sm_to_ms_copy( sm_loc.xy() );
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
    if( calendar::once_every( 3_minutes ) ) {
        overmap_npc_move();
    }
    if( calendar::once_every( 10_seconds ) ) {
        for( const tripoint elem : m.get_furn_field_locations() ) {
            const auto &furn = m.furn( elem ).obj();
            for( const emit_id &e : furn.emissions ) {
                g->m.emit_field( elem, e );
            }
        }
    }
    update_stair_monsters();
    u.process_turn();
    if( u.moves < 0 && get_option<bool>( "FORCE_REDRAW" ) ) {
        draw();
        refresh_display();
    }
    u.process_active_items();

    if( get_levz() >= 0 && !u.is_underwater() ) {
        weather::effect( weather.weather )();
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
    u.update_body_wetness( *weather.weather_precise );
    u.apply_wetness_morale( weather.temperature );
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
    sfx::do_vehicle_engine_sfx();
    sfx::do_vehicle_exterior_engine_sfx();
    sfx::do_fatigue();

    // reset player noise
    u.volume = 0;

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

void game::following_vehicles()
{
    for( auto &veh : m.get_vehicles() ) {
        auto &v = veh.v;
        if( v->is_following ) {
            v->drive_to_local_target( u.pos(), true );
        }
    }
}

void game::catch_a_monster( monster *fish, const tripoint &pos, player *p,
                            const time_duration &catch_duration ) // catching function
{
    //spawn the corpse, rotten by a part of the duration
    m.add_item_or_charges( pos, item::make_corpse( fish->type->id, calendar::turn + rng( 0_turns,
                           catch_duration ) ) );
    u.add_msg_if_player( m_good, _( "You caught a %s." ), fish->type->nname() );
    //quietly kill the caught
    fish->no_corpse_quiet = true;
    fish->die( p );
}

static bool cancel_auto_move( player &p, const std::string &text )
{
    if( p.has_destination() && query_yn( "%s, cancel Auto-move?", text ) ) {
        add_msg( m_warning, _( "%s. Auto-move canceled" ), text );
        if( !p.omt_path.empty() ) {
            p.omt_path.clear();
        }
        p.clear_destination();
        return true;
    }
    return false;
}

bool game::cancel_activity_or_ignore_query( const distraction_type type, const std::string &text )
{
    if( u.has_distant_destination() ) {
        if( cancel_auto_move( u, text ) ) {
            return true;
        } else {
            u.set_destination( u.get_auto_move_route(), player_activity( activity_id( "ACT_TRAVELLING" ) ) );
            return false;
        }
    }
    if( !u.activity || u.activity.is_distraction_ignored( type ) ) {
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
    if( u.has_distant_destination() ) {
        if( cancel_auto_move( u, text ) ) {
            return true;
        } else {
            u.set_destination( u.get_auto_move_route(), player_activity( activity_id( "ACT_TRAVELLING" ) ) );
            return false;
        }
    }
    if( !u.activity ) {
        return false;
    }
    if( query_yn( "%s %s", text, u.activity.get_stop_phrase() ) ) {
        u.cancel_activity();
        u.resume_backlog_activity();
        return true;
    }
    return false;
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

static int maptile_field_intensity( maptile &mt, field_type_id fld )
{
    auto field_ptr = mt.find_field( fld );

    return field_ptr == nullptr ? 0 : field_ptr->get_field_intensity();
}

static bool maptile_trap_eq( maptile &mt, const trap_id &id )
{
    return mt.get_trap() == id;
}

int get_heat_radiation( const tripoint &location, bool direct )
{
    // Direct heat from fire sources
    // Cache fires to avoid scanning the map around us bp times
    // Stored as intensity-distance pairs
    int temp_mod = 0;
    int best_fire = 0;
    for( const tripoint &dest : g->m.points_in_radius( location, 6 ) ) {
        int heat_intensity = 0;

        maptile mt = g->m.maptile_at( dest );

        int ffire = maptile_field_intensity( mt, fd_fire );
        if( ffire > 0 ) {
            heat_intensity = ffire;
        } else if( maptile_trap_eq( mt, tr_lava ) ) {
            heat_intensity = 3;
        }
        if( heat_intensity == 0 ) {
            // No heat source here
            continue;
        }
        if( g->u.pos() == location ) {
            if( !g->m.pl_line_of_sight( dest, -1 ) ) {
                continue;
            }
        } else if( !g->m.sees( location, dest, -1 ) ) {
            continue;
        }
        // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
        const int fire_dist = std::max( 1, square_dist( dest, location ) );
        temp_mod += 6 * heat_intensity * heat_intensity / fire_dist;
        if( fire_dist <= 1 ) {
            // Extend limbs/lean over a single adjacent fire to warm up
            best_fire = std::max( best_fire, heat_intensity );
        }
    }
    if( direct ) {
        return best_fire;
    }
    return temp_mod;
}

int get_convection_temperature( const tripoint &location )
{
    int temp_mod = 0;
    // Directly on lava tiles
    maptile mt = g->m.maptile_at( location );
    int lava_mod = maptile_trap_eq( mt, tr_lava ) ? fd_fire.obj().get_convection_temperature_mod() : 0;
    // Modifier from fields
    for( auto fd : g->m.field_at( location ) ) {
        // Nullify lava modifier when there is open fire
        if( fd.first.obj().has_fire ) {
            lava_mod = 0;
        }
        temp_mod += fd.second.convection_temperature_mod();
    }
    return temp_mod + lava_mod;
}

int game::assign_mission_id()
{
    int ret = next_mission_id;
    next_mission_id++;
    return ret;
}

npc *game::find_npc( character_id id )
{
    return overmap_buffer.find_npc( id ).get();
}

void game::add_npc_follower( const character_id &id )
{
    follower_ids.insert( id );
    u.follower_ids.insert( id );
}

void game::remove_npc_follower( const character_id &id )
{
    follower_ids.erase( id );
    u.follower_ids.erase( id );
}

static void update_faction_api( npc *guy )
{
    if( guy->get_faction_ver() < 2 ) {
        guy->set_fac( your_followers );
        guy->set_faction_ver( 2 );
    }
}

void game::validate_mounted_npcs()
{
    for( monster &m : all_monsters() ) {
        if( m.has_effect( effect_ridden ) && m.mounted_player_id.is_valid() ) {
            player *mounted_pl = g->critter_by_id<player>( m.mounted_player_id );
            if( !mounted_pl ) {
                // Target no longer valid.
                m.mounted_player_id = character_id();
                m.remove_effect( effect_ridden );
                continue;
            }
            mounted_pl->mounted_creature = shared_from( m );
            mounted_pl->setpos( m.pos() );
            mounted_pl->add_effect( effect_riding, 1_turns, num_bp, true );
            m.mounted_player = mounted_pl;
        }
    }
}

void game::validate_npc_followers()
{
    // Make sure visible followers are in the list.
    const std::vector<npc *> visible_followers = get_npcs_if( [&]( const npc & guy ) {
        return guy.is_player_ally();
    } );
    for( npc *guy : visible_followers ) {
        update_faction_api( guy );
        add_npc_follower( guy->getID() );
    }
    // Make sure overmapbuffered NPC followers are in the list.
    for( const auto &temp_guy : overmap_buffer.get_npcs_near_player( 300 ) ) {
        npc *guy = temp_guy.get();
        if( guy->is_player_ally() ) {
            update_faction_api( guy );
            add_npc_follower( guy->getID() );
        }
    }
    // Make sure that serialized player followers sync up with game list
    for( const auto &temp_id : u.follower_ids ) {
        add_npc_follower( temp_id );
    }
}

void game::validate_camps()
{
    basecamp camp = m.hoist_submap_camp( u.pos() );
    if( camp.is_valid() ) {
        overmap_buffer.add_camp( camp );
        m.remove_submap_camp( u.pos() );
    } else if( camp.camp_omt_pos() != tripoint_zero ) {
        std::string camp_name = _( "Faction Camp" );
        camp.set_name( camp_name );
        overmap_buffer.add_camp( camp );
        m.remove_submap_camp( u.pos() );
    }
}

std::set<character_id> game::get_follower_list()
{
    return follower_ids;
}

void game::handle_key_blocking_activity()
{
    if( ( u.activity && u.activity.moves_left > 0 ) || ( u.has_destination() &&
            !u.omt_path.empty() ) ) {
        input_context ctxt = get_default_mode_input_context();
        const std::string action = ctxt.handle_input( 0 );
        if( action == "pause" ) {
            cancel_activity_query( _( "Confirm:" ) );
        } else if( action == "player_data" ) {
            u.disp_info();
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
#if defined(__ANDROID__)
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

        if( oThisItem.is_favorite ) {
            addentry( 'f', pgettext( "action", "unfavorite" ), HINT_GOOD );
        } else {
            addentry( 'f', pgettext( "action", "favorite" ), HINT_GOOD );
        }

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
                    avatar_action::plthrow( u, pos );
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
                    u.disassemble( item_location( u, &u.i_at( pos ) ), false );
                    break;
                case 'f':
                    oThisItem.is_favorite = !oThisItem.is_favorite;
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
                                 false ) );
                    }
                    break;
                case '-':
                    if( bHPR ) {
                        get_auto_pickup().remove_rule( &oThisItem );
                        add_msg( m_info, _( "'%s' removed from character pickup rules." ), oThisItem.tname( 1,
                                 false ) );
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
            } else if( !mouse_pos ) {
                liveview_pos.reset();
                liveview.hide();
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

std::pair<tripoint, tripoint> game::mouse_edge_scrolling( input_context ctxt, const int speed,
        const tripoint &last, bool iso )
{
    const int rate = get_option<int>( "EDGE_SCROLL" );
    auto ret = std::make_pair( tripoint_zero, last );
    if( rate == -1 ) {
        // Fast return when the option is disabled.
        return ret;
    }
    // Ensure the parameters are used even if the #if below is false
    ( void ) ctxt;
    ( void ) speed;
    ( void ) iso;
#if (defined TILES || defined _WIN32 || defined WINDOWS)
    auto now = std::chrono::steady_clock::now();
    if( now < last_mouse_edge_scroll + std::chrono::milliseconds( rate ) ) {
        return ret;
    } else {
        last_mouse_edge_scroll = now;
    }
    const input_event event = ctxt.get_raw_input();
    if( event.type == CATA_INPUT_MOUSE ) {
        const int threshold_x = projected_window_width() / 100;
        const int threshold_y = projected_window_height() / 100;
        if( event.mouse_pos.x <= threshold_x ) {
            ret.first.x -= speed;
            if( iso ) {
                ret.first.y -= speed;
            }
        } else if( event.mouse_pos.x >= projected_window_width() - threshold_x ) {
            ret.first.x += speed;
            if( iso ) {
                ret.first.y += speed;
            }
        }
        if( event.mouse_pos.y <= threshold_y ) {
            ret.first.y -= speed;
            if( iso ) {
                ret.first.x += speed;
            }
        } else if( event.mouse_pos.y >= projected_window_height() - threshold_y ) {
            ret.first.y += speed;
            if( iso ) {
                ret.first.x -= speed;
            }
        }
        ret.second = ret.first;
    } else if( event.type == CATA_INPUT_TIMEOUT ) {
        ret.first = ret.second;
    }
#endif
    return ret;
}

tripoint game::mouse_edge_scrolling_terrain( input_context &ctxt )
{
    auto ret = mouse_edge_scrolling( ctxt, std::max( DEFAULT_TILESET_ZOOM / tileset_zoom, 1 ),
                                     last_mouse_edge_scroll_vector_terrain, tile_iso );
    last_mouse_edge_scroll_vector_terrain = ret.second;
    last_mouse_edge_scroll_vector_overmap = tripoint_zero;
    return ret.first;
}

tripoint game::mouse_edge_scrolling_overmap( input_context &ctxt )
{
    // overmap has no iso mode
    auto ret = mouse_edge_scrolling( ctxt, 2, last_mouse_edge_scroll_vector_overmap, false );
    last_mouse_edge_scroll_vector_overmap = ret.second;
    last_mouse_edge_scroll_vector_terrain = tripoint_zero;
    return ret.first;
}

input_context get_default_mode_input_context()
{
    input_context ctxt( "DEFAULTMODE" );
    // Because those keys move the character, they don't pan, as their original name says
    ctxt.set_iso( true );
    ctxt.register_action( "UP", translate_marker( "Move North" ) );
    ctxt.register_action( "RIGHTUP", translate_marker( "Move Northeast" ) );
    ctxt.register_action( "RIGHT", translate_marker( "Move East" ) );
    ctxt.register_action( "RIGHTDOWN", translate_marker( "Move Southeast" ) );
    ctxt.register_action( "DOWN", translate_marker( "Move South" ) );
    ctxt.register_action( "LEFTDOWN", translate_marker( "Move Southwest" ) );
    ctxt.register_action( "LEFT", translate_marker( "Move West" ) );
    ctxt.register_action( "LEFTUP", translate_marker( "Move Northwest" ) );
    ctxt.register_action( "pause" );
    ctxt.register_action( "LEVEL_DOWN", translate_marker( "Descend Stairs" ) );
    ctxt.register_action( "LEVEL_UP", translate_marker( "Ascend Stairs" ) );
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
    ctxt.register_action( "cycle_move" );
    ctxt.register_action( "reset_move" );
    ctxt.register_action( "toggle_run" );
    ctxt.register_action( "toggle_crouch" );
    ctxt.register_action( "open_movement" );
    ctxt.register_action( "open" );
    ctxt.register_action( "close" );
    ctxt.register_action( "smash" );
    ctxt.register_action( "loot" );
    ctxt.register_action( "examine" );
    ctxt.register_action( "advinv" );
    ctxt.register_action( "pickup" );
    ctxt.register_action( "pickup_feet" );
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
    ctxt.register_action( "open_consume" );
    ctxt.register_action( "read" );
    ctxt.register_action( "wield" );
    ctxt.register_action( "pick_style" );
    ctxt.register_action( "reload_item" );
    ctxt.register_action( "reload_weapon" );
    ctxt.register_action( "unload" );
    ctxt.register_action( "throw" );
    ctxt.register_action( "fire" );
    ctxt.register_action( "cast_spell" );
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
    ctxt.register_action( "auto_travel_mode" );
    ctxt.register_action( "safemode" );
    ctxt.register_action( "autosafe" );
    ctxt.register_action( "autoattack" );
    ctxt.register_action( "ignore_enemy" );
    ctxt.register_action( "whitelist_enemy" );
    ctxt.register_action( "save" );
    ctxt.register_action( "quicksave" );
#if !defined(RELEASE)
    ctxt.register_action( "quickload" );
#endif
    ctxt.register_action( "quit" );
    ctxt.register_action( "player_data" );
    ctxt.register_action( "map" );
    ctxt.register_action( "sky" );
    ctxt.register_action( "missions" );
    ctxt.register_action( "factions" );
    ctxt.register_action( "scores" );
    ctxt.register_action( "morale" );
    ctxt.register_action( "messages" );
    ctxt.register_action( "help" );
    ctxt.register_action( "open_keybindings" );
    ctxt.register_action( "open_options" );
    ctxt.register_action( "open_autopickup" );
    ctxt.register_action( "open_autonotes" );
    ctxt.register_action( "open_safemode" );
    ctxt.register_action( "open_color" );
    ctxt.register_action( "open_world_mods" );
    ctxt.register_action( "debug" );
    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "debug_temp" );
    ctxt.register_action( "debug_visibility" );
    ctxt.register_action( "debug_radiation" );
    ctxt.register_action( "debug_mode" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
#if !defined(__ANDROID__)
    ctxt.register_action( "toggle_fullscreen" );
#endif
    ctxt.register_action( "toggle_pixel_minimap" );
    ctxt.register_action( "toggle_panel_adm" );
    ctxt.register_action( "reload_tileset" );
    ctxt.register_action( "toggle_auto_features" );
    ctxt.register_action( "toggle_auto_pulp_butcher" );
    ctxt.register_action( "toggle_auto_mining" );
    ctxt.register_action( "toggle_auto_foraging" );
    ctxt.register_action( "toggle_auto_pickup" );
    ctxt.register_action( "toggle_thief_mode" );
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

    const bool is_adjacent = square_dist( mouse_target.xy(), point( u.posx(), u.posy() ) ) <= 1;
    const bool is_self = square_dist( mouse_target.xy(), point( u.posx(), u.posy() ) ) <= 0;
    if( const monster *const mon = critter_at<monster>( mouse_target ) ) {
        if( !u.sees( *mon ) ) {
            add_msg( _( "Nothing relevant here." ) );
            return false;
        }

        if( !u.weapon.is_gun() ) {
            add_msg( m_info, _( "You are not wielding a ranged weapon." ) );
            return false;
        }

        // TODO: Add weapon range check. This requires weapon to be reloaded.

        act = ACTION_FIRE;
    } else if( is_adjacent &&
               m.close_door( tripoint( mouse_target.xy(), u.posz() ), !m.is_outside( u.pos() ),
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
    show_scores_ui( stats(), get_kill_tracker() );
    disp_NPC_epilogues();
    follower_ids.clear();
    disp_faction_ends();
}

void game::move_save_to_graveyard()
{
    const std::string &save_dir      = get_world_base_save_path();
    const std::string &graveyard_dir = FILENAMES["graveyarddir"];
    const std::string &prefix        = base64_encode( u.name ) + ".";

    if( !assure_dir_exist( graveyard_dir ) ) {
        debugmsg( "could not create graveyard path '%s'", graveyard_dir );
    }

    const auto save_files = get_files_from_path( prefix, save_dir );
    if( save_files.empty() ) {
        debugmsg( "could not find save files in '%s'", save_dir );
    }

    for( const auto &src_path : save_files ) {
        const std::string dst_path = graveyard_dir +
                                     src_path.substr( src_path.rfind( '/' ), std::string::npos );

        if( rename_file( src_path, dst_path ) ) {
            continue;
        }

        debugmsg( "could not rename file '%s' to '%s'", src_path, dst_path );

        if( remove_file( src_path ) ) {
            continue;
        }

        debugmsg( "could not remove file '%s'", src_path );
    }
}

void game::load_master()
{
    using namespace std::placeholders;
    const auto datafile = get_world_base_save_path() + "/master.gsav";
    read_from_file_optional( datafile, std::bind( &game::unserialize_master, this, _1 ) );
}

bool game::load( const std::string &world )
{
    world_generator->init();
    const WORLDPTR wptr = world_generator->get_world( world );
    if( !wptr ) {
        return false;
    }
    if( wptr->world_saves.empty() ) {
        debugmsg( "world '%s' contains no saves", world );
        return false;
    }

    try {
        world_generator->set_active_world( wptr );
        g->setup();
        g->load( wptr->world_saves.front() );
    } catch( const std::exception &err ) {
        debugmsg( "cannot load world '%s': %s", world, err.what() );
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
    u = avatar();
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
    weather.nextweather = calendar::turn;

    read_from_file_optional( worldpath + name.base_path() + ".log",
                             std::bind( &memorial_logger::load, &memorial(), _1 ) );

#if defined(__ANDROID__)
    read_from_file_optional( worldpath + name.base_path() + ".shortcuts",
                             std::bind( &game::load_shortcuts, this, _1 ) );
#endif

    // Now that the player's worn items are updated, their sight limits need to be
    // recalculated. (This would be cleaner if u.worn were private.)
    u.recalc_sight_limits();

    if( !gamemode ) {
        gamemode = std::make_unique<special_game>();
    }

    safe_mode = get_option<bool>( "SAFEMODE" ) ? SAFE_MODE_ON : SAFE_MODE_OFF;
    mostseen = 0; // ...and mostseen is 0, we haven't seen any monsters yet.

    init_autosave();
    get_auto_pickup().load_character(); // Load character auto pickup rules
    get_auto_notes_settings().load();   // Load character auto notes settings
    get_safemode().load_character(); // Load character safemode rules
    zone_manager::get_manager().load_zones(); // Load character world zones
    read_from_file_optional( get_world_base_save_path() + "/uistate.json", []( std::istream & stream ) {
        JsonIn jsin( stream );
        uistate.deserialize( jsin );
    } );
    reload_npcs();
    validate_npc_followers();
    validate_mounted_npcs();
    validate_camps();
    update_map( u );
    for( auto &e : u.inv_dump() ) {
        e->set_owner( g->u.get_faction() );
    }
    // legacy, needs to be here as we access the map.
    if( !u.getID().is_valid() ) {
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

    // populate calendar caches now, after active world is set, but before we do
    // anything else, to ensure they pick up the correct value from the save's
    // worldoptions
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

    u.reset();
    draw();
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

void game::reset_npc_dispositions()
{
    for( auto elem : follower_ids ) {
        std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
        if( !npc_to_get )  {
            continue;
        }
        npc *npc_to_add = npc_to_get.get();
        npc_to_add->chatbin.missions.clear();
        npc_to_add->chatbin.missions_assigned.clear();
        npc_to_add->mission = NPC_MISSION_NULL;
        npc_to_add->chatbin.mission_selected = nullptr;
        npc_to_add->set_attitude( NPCATT_NULL );
        npc_to_add->op_of_u.anger = 0;
        npc_to_add->op_of_u.fear = 0;
        npc_to_add->op_of_u.trust = 0;
        npc_to_add->op_of_u.value = 0;
        npc_to_add->op_of_u.owed = 0;
        npc_to_add->set_fac( faction_id( "no_faction" ) );
        npc_to_add->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC,
                                     npc_to_add->global_omt_location(),
                                     npc_to_add->getID() ) );

    }

}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    std::string masterfile = get_world_base_save_path() + "/master.gsav";
    return write_to_file( masterfile, [&]( std::ostream & fout ) {
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
        fout << memorial().dump();
    }, _( "player memorial" ) );
#if defined(__ANDROID__)
    const bool saved_shortcuts = write_to_file( playerfile + ".shortcuts", [&]( std::ostream & fout ) {
        save_shortcuts( fout );
    }, _( "quick shortcuts" ) );
#endif

    return saved_data && saved_map_memory && saved_weather && saved_log
#if defined(__ANDROID__)
           && saved_shortcuts
#endif
           ;
}

event_bus &game::events()
{
    return *event_bus_ptr;
}

stats_tracker &game::stats()
{
    return *stats_tracker_ptr;
}

memorial_logger &game::memorial()
{
    return *memorial_logger_ptr;
}

spell_events &game::spell_events_subscriber()
{
    return *spell_events_ptr;
}

bool game::save()
{
    try {
        if( !save_player_data() ||
            !save_factions_missions_npcs() ||
            !save_artifacts() ||
            !save_maps() ||
            !get_auto_pickup().save_character() ||
            !get_auto_notes_settings().save() ||
            !get_safemode().save_character() ||
        !write_to_file( get_world_base_save_path() + "/uistate.json", [&]( std::ostream & fout ) {
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
        debugmsg( "Could not make '%s' directory", memorial_dir );
        return;
    }

    if( !assure_dir_exist( memorial_active_world_dir ) ) {
        dbg( D_ERROR ) <<
                       "game:write_memorial_file: Unable to make active world directory in memorial directory.";
        debugmsg( "Could not make '%s' directory", memorial_active_world_dir );
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
        memorial().write( fout, sLastWords );
    }, _( "player memorial" ) );
}

void game::disp_NPC_epilogues()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ), std::max( 0,
                                   ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
    epilogue epi;
    // TODO: This search needs to be expanded to all NPCs
    for( auto elem : follower_ids ) {
        std::shared_ptr<npc> npc_to_get = overmap_buffer.find_npc( elem );
        if( !npc_to_get ) {
            continue;
        }
        npc *guy = npc_to_get.get();
        epi.random_by_group( guy->male ? "male" : "female" );
        std::vector<std::string> txt;
        txt.emplace_back( epi.text );
        draw_border( w, BORDER_COLOR, guy->name, c_black_white );
        multipage( w, txt, "", 2 );
    }

    refresh_all();
}

void game::disp_faction_ends()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ),
                                  std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
    std::vector<std::string> data;

    for( const auto &elem : faction_manager_ptr->all() ) {
        if( elem.second.known_by_u ) {
            if( elem.second.name == "Your Followers" ) {
                data.emplace_back( _( "       You are forgotten among the billions lost in the cataclysm..." ) );
                display_table( w, "", 1, data );
            } else if( elem.second.name == "The Old Guard" && elem.second.power != 100 ) {
                if( elem.second.power < 150 ) {
                    data.emplace_back(
                        _( "    Locked in an endless battle, the Old Guard was forced to consolidate their "
                           "resources in a handful of fortified bases along the coast.  Without the men "
                           "or material to rebuild, the soldiers that remained lost all hope..." ) );
                } else {
                    data.emplace_back( _( "    The steadfastness of individual survivors after the cataclysm impressed "
                                          "the tattered remains of the once glorious union.  Spurred on by small "
                                          "successes, a number of operations to re-secure facilities met with limited "
                                          "success.  Forced to eventually consolidate to large bases, the Old Guard left "
                                          "these facilities in the hands of the few survivors that remained.  As the "
                                          "years past, little materialized from the hopes of rebuilding civilization..." ) );
                }
                display_table( w, _( "The Old Guard" ), 1, data );
            } else if( elem.second.name == "The Free Merchants" && elem.second.power != 100 ) {
                if( elem.second.power < 150 ) {
                    data.emplace_back( _( "    Life in the refugee shelter deteriorated as food shortages and disease "
                                          "destroyed any hope of maintaining a civilized enclave.  The merchants and "
                                          "craftsmen dispersed to found new colonies but most became victims of "
                                          "marauding bandits.  Those who survived never found a place to call home..." ) );
                } else {
                    data.emplace_back( _( "    The Free Merchants struggled for years to keep themselves fed but their "
                                          "once profitable trade routes were plundered by bandits and thugs.  In squalor "
                                          "and filth the first generations born after the cataclysm are told stories of "
                                          "the old days when food was abundant and the children were allowed to play in "
                                          "the sun..." ) );
                }
                display_table( w, _( "The Free Merchants" ), 1, data );
            } else if( elem.second.name == "The Tacoma Commune" && elem.second.power != 100 ) {
                if( elem.second.power < 150 ) {
                    data.emplace_back( _( "    The fledgling outpost was abandoned a few months later.  The external "
                                          "threats combined with low crop yields caused the Free Merchants to withdraw "
                                          "their support.  When the exhausted migrants returned to the refugee center "
                                          "they were turned away to face the world on their own." ) );
                } else {
                    data.emplace_back(
                        _( "    The commune continued to grow rapidly through the years despite constant "
                           "external threat.  While maintaining a reputation as a haven for all law-"
                           "abiding citizens, the commune's leadership remained loyal to the interests of "
                           "the Free Merchants.  Hard labor for little reward remained the price to be "
                           "paid for those who sought the safety of the community." ) );
                }
                display_table( w, _( "The Tacoma Commune" ), 1, data );
            } else if( elem.second.name == "The Wasteland Scavengers" && elem.second.power != 100 ) {
                if( elem.second.power < 150 ) {
                    data.emplace_back(
                        _( "    The lone bands of survivors who wandered the now alien world dwindled in "
                           "number through the years.  Unable to compete with the growing number of "
                           "monstrosities that had adapted to live in their world, those who did survive "
                           "lived in dejected poverty and hopelessness..." ) );
                } else {
                    data.emplace_back(
                        _( "    The scavengers who flourished in the opening days of the cataclysm found "
                           "an ever increasing challenge in finding and maintaining equipment from the "
                           "old world.  Enormous hordes made cities impossible to enter while new "
                           "eldritch horrors appeared mysteriously near old research labs.  But on the "
                           "fringes of where civilization once ended, bands of hunter-gatherers began to "
                           "adopt agrarian lifestyles in fortified enclaves..." ) );
                }
                display_table( w, _( "The Wasteland Scavengers" ), 1, data );
            } else if( elem.second.name == "Hell's Raiders" && elem.second.power != 100 ) {
                if( elem.second.power < 150 ) {
                    data.emplace_back( _( "    The raiders grew more powerful than any other faction as attrition "
                                          "destroyed the Old Guard.  The ruthless men and women who banded together to "
                                          "rob refugees and pillage settlements soon found themselves without enough "
                                          "victims to survive.  The Hell's Raiders were eventually destroyed when "
                                          "infighting erupted into civil war but there were few survivors left to "
                                          "celebrate their destruction." ) );
                } else {
                    data.emplace_back( _( "    Fueled by drugs and rage, the Hell's Raiders fought tooth and nail to "
                                          "overthrow the last strongholds of the Old Guard.  The costly victories "
                                          "brought the warlords abundant territory and slaves but little in the way of "
                                          "stability.  Within weeks, infighting led to civil war as tribes vied for "
                                          "leadership of the faction.  When only one warlord finally secured control, "
                                          "there was nothing left to fight for... just endless cities full of the dead." ) );
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
        return square_dist( ppos.xy(), apos.xy() ) <
               square_dist( ppos.xy(), bpos.xy() );
    }
};

void game::disp_NPCs()
{
    catacurses::window w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                           point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                  TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );

    const tripoint ppos = u.global_omt_location();
    const tripoint &lpos = u.pos();
    mvwprintz( w, point_zero, c_white, _( "Your overmap position: %d, %d, %d" ), ppos.x, ppos.y,
               ppos.z );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    mvwprintz( w, point( 0, 1 ), c_white, _( "Your local position: %d, %d, %d" ), lpos.x, lpos.y,
               lpos.z );
    std::vector<std::shared_ptr<npc>> npcs = overmap_buffer.get_npcs_near_player( 100 );
    std::sort( npcs.begin(), npcs.end(), npc_dist_to_player() );
    size_t i;
    for( i = 0; i < 20 && i < npcs.size(); i++ ) {
        const tripoint apos = npcs[i]->global_omt_location();
        mvwprintz( w, point( 0, i + 3 ), c_white, "%s: %d, %d, %d", npcs[i]->name,
                   apos.x, apos.y, apos.z );
    }
    for( const monster &m : all_monsters() ) {
        mvwprintz( w, point( 0, i + 3 ), c_white, "%s: %d, %d, %d", m.name(),
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

        mvwputch( window, footstep.xy() + offset.xy(), c_yellow, glyph );
    }
}

void game::draw()
{
    if( test_mode ) {
        return;
    }

    //temporary fix for updating visibility for minimap
    ter_view_p.z = ( u.pos() + u.view_offset ).z;
    m.build_map_cache( ter_view_p.z );
    m.update_visibility_cache( ter_view_p.z );

    werase( w_terrain );
    draw_ter();
    wrefresh( w_terrain );

    draw_panels( true );
}

void game::draw_panels( bool force_draw )
{
    draw_panels( 0, 1, force_draw );
}

void game::draw_panels( size_t column, size_t index, bool force_draw )
{
    static int previous_turn = -1;
    const int current_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    const bool draw_this_turn = current_turn > previous_turn || force_draw;
    auto &mgr = panel_manager::get_manager();
    int y = 0;
    const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    int spacer = get_option<bool>( "SIDEBAR_SPACERS" ) ? 1 : 0;
    int log_height = 0;
    for( const window_panel &panel : mgr.get_current_layout() ) {
        if( panel.get_height() != -2 && panel.toggle && panel.render() ) {
            log_height += panel.get_height() + spacer;
        }
    }
    log_height = std::max( TERMY - log_height, 3 );
    for( const window_panel &panel : mgr.get_current_layout() ) {
        if( panel.render() ) {
            // height clamped to window height.
            int h = std::min( panel.get_height(), TERMY - y );
            if( h == -2 ) {
                h = log_height;
            }
            h += spacer;
            if( panel.toggle && panel.render() && h > 0 ) {
                if( panel.always_draw || draw_this_turn ) {
                    panel.draw( u, catacurses::newwin( h, panel.get_width(),
                                                       point( sidebar_right ? TERMX - panel.get_width() : 0, y ) ) );
                }
                if( show_panel_adm ) {
                    const std::string panel_name = _( panel.get_name() );
                    const int panel_name_width = utf8_width( panel_name );
                    auto label = catacurses::newwin( 1, panel_name_width, point( sidebar_right ?
                                                     TERMX - panel.get_width() - panel_name_width - 1 : panel.get_width() + 1, y ) );
                    werase( label );
                    mvwprintz( label, point_zero, c_light_red, panel_name );
                    wrefresh( label );
                    label = catacurses::newwin( h, 1,
                                                point( sidebar_right ? TERMX - panel.get_width() - 1 : panel.get_width(), y ) );
                    werase( label );
                    if( h == 1 ) {
                        mvwputch( label, point_zero, c_light_red, LINE_OXOX );
                    } else {
                        mvwputch( label, point_zero, c_light_red, LINE_OXXX );
                        for( int i = 1; i < h - 1; i++ ) {
                            mvwputch( label, point( 0, i ), c_light_red, LINE_XOXO );
                        }
                        mvwputch( label, point( 0, h - 1 ), c_light_red, sidebar_right ? LINE_XXOO : LINE_XOOX );
                    }
                    wrefresh( label );
                }
                y += h;
            }
        }
    }
    if( show_panel_adm ) {
        mgr.draw_adm( w_panel_adm, column, index );
    }
    previous_turn = current_turn;
}

void game::draw_pixel_minimap( const catacurses::window &w )
{
    w_pixel_minimap = w;
}

void game::draw_critter( const Creature &critter, const tripoint &center )
{
    const int my = POSY + ( critter.posy() - center.y );
    const int mx = POSX + ( critter.posx() - center.x );
    if( !is_valid_in_w_terrain( point( mx, my ) ) ) {
        return;
    }
    if( critter.posz() != center.z && m.has_zlevels() ) {
        static constexpr tripoint up_tripoint( tripoint_above );
        if( critter.posz() == center.z - 1 &&
            ( debug_mode || u.sees( critter ) ) &&
            m.valid_move( critter.pos(), critter.pos() + up_tripoint, false, true ) ) {
            // Monster is below
            // TODO: Make this show something more informative than just green 'v'
            // TODO: Allow looking at this mon with look command
            // TODO: Redraw this after weather etc. animations
            mvwputch( w_terrain, point( mx, my ), c_green_cyan, 'v' );
        }
        return;
    }
    if( u.sees( critter ) || &critter == &u ) {
        critter.draw( w_terrain, center.xy(), false );
        return;
    }

    if( u.sees_with_infrared( critter ) ) {
        mvwputch( w_terrain, point( mx, my ), c_red, '?' );
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
    draw_ter( u.pos() + u.view_offset, false,
              draw_sounds );
}

void game::draw_ter( const tripoint &center, const bool looking, const bool draw_sounds )
{
    ter_view_p = center;
    const int posx = center.x;
    const int posy = center.y;

    // TODO: Make it not rebuild the cache all the time (cache point+moves?)
    if( !looking ) {
        // If we're looking, the cache is built at start (entering looking mode)
        m.build_map_cache( center.z );
    }

    m.draw( w_terrain, center );

    if( draw_sounds ) {
        draw_footsteps( w_terrain, tripoint( -center.x, -center.y, center.z ) + point( POSX, POSY ) );
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
                            mvwputch( w_terrain, point( realx + POSX - posx, realy + POSY - posy ), c_white, '?' );
                        } else {
                            mvwputch( w_terrain, point( realx + POSX - posx, realy + POSY - posy ), c_magenta, '#' );
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
        mvwputch( w_terrain, final_destination.xy() - u.view_offset.xy() + point( POSX - u.posx(),
                  POSY - u.posy() ), c_white, 'X' );
    }

    if( u.controlling_vehicle && !looking ) {
        draw_veh_dir_indicator( false );
        draw_veh_dir_indicator( true );
    }
    // Place the cursor over the player as is expected by screen readers.
    wmove( w_terrain, -center.xy() + g->u.pos().xy() + point( POSX, POSY ) );
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
        mvwputch( w_terrain, indicator_offset->xy() - u.view_offset.xy() + point( POSX, POSY ), col, 'X' );
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
            tripoint omp( omx, omy, get_levz() );
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omp );
            const bool vehicle_here = overmap_buffer.has_vehicle( omp );
            if( overmap_buffer.has_note( omp ) ) {

                const std::string &note_text = overmap_buffer.note( omp );

                ter_color = c_yellow;
                ter_sym = "N";

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
                ter_sym = " ";
                ter_color = c_black;
            } else if( vehicle_here ) {
                ter_color = c_cyan;
                ter_sym = "c";
            } else {
                const oter_id &cur_ter = overmap_buffer.ter( omp );
                ter_sym = cur_ter->get_symbol();
                if( overmap_buffer.is_explored( omp ) ) {
                    ter_color = c_dark_gray;
                } else {
                    ter_color = cur_ter->get_color();
                }
            }
            if( !drew_mission && targ.xy() == omp.xy() ) {
                // If there is a mission target, and it's not on the same
                // overmap terrain as the player character, mark it.
                // TODO: Inform player if the mission is above or below
                drew_mission = true;
                if( i != 0 || j != 0 ) {
                    ter_color = red_background( ter_color );
                }
            }
            if( i == 0 && j == 0 ) {
                mvwputch_hi( w_minimap, point( 3, 3 ), ter_color, ter_sym );
            } else {
                mvwputch( w_minimap, point( 3 + i, 3 + j ), ter_color, ter_sym );
            }
        }
    }

    // Print arrow to mission if we have one!
    if( !drew_mission ) {
        double slope = ( cursx != targ.x ) ? static_cast<double>( targ.y - cursy ) / static_cast<double>
                       ( targ.x - cursx ) : 4;

        if( cursx == targ.x || fabs( slope ) > 3.5 ) { // Vertical slope
            if( targ.y > cursy ) {
                mvwputch( w_minimap, point( 3, 6 ), c_red, "*" );
            } else {
                mvwputch( w_minimap, point( 3, 0 ), c_red, "*" );
            }
        } else {
            int arrowx = -1;
            int arrowy = -1;
            if( fabs( slope ) >= 1. ) { // y diff is bigger!
                arrowy = ( targ.y > cursy ? 6 : 0 );
                arrowx = static_cast<int>( 3 + 3 * ( targ.y > cursy ? slope : ( 0 - slope ) ) );
                if( arrowx < 0 ) {
                    arrowx = 0;
                }
                if( arrowx > 6 ) {
                    arrowx = 6;
                }
            } else {
                arrowx = ( targ.x > cursx ? 6 : 0 );
                arrowy = static_cast<int>( 3 + 3 * ( targ.x > cursx ? slope : ( 0 - slope ) ) );
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

            mvwputch( w_minimap, point( arrowx, arrowy ), c_red, glyph );
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
            tripoint omp( omx, omy, get_levz() );
            if( overmap_buffer.get_horde_size( omp ) >= HORDE_VISIBILITY_SIZE ) {
                const tripoint cur_pos {
                    omx, omy, get_levz()
                };
                if( overmap_buffer.seen( omp )
                    && g->u.overmap_los( cur_pos, sight_points ) ) {
                    mvwputch( w_minimap, point( i + 3, j + 3 ), c_green,
                              overmap_buffer.get_horde_size( omp ) > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
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
    if( !weather.lightning_active ) {
        ret = sunlight( calendar::turn );
    } else {
        // Recent lightning strike has lit the area
        ret = default_daylight_level();
    }

    ret += weather::light_modifier( weather.weather );

    // Artifact light level changes here. Even though some of these only have an effect
    // aboveground it is cheaper performance wise to simply iterate through the entire
    // list once instead of twice.
    float mod_ret = -1;
    // Each artifact change does std::max(mod_ret, new val) since a brighter end value
    // will trump a lower one.
    if( const timed_event *e = timed_events.get( TIMED_EVENT_DIM ) ) {
        // TIMED_EVENT_DIM slowly dims the natural sky level, then relights it.
        const time_duration left = e->when - calendar::turn;
        // TIMED_EVENT_DIM has an occurrence date of turn + 50, so the first 25 dim it,
        if( left > 25_turns ) {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( left - 25_turns ) ) / 25_turns );
            // and the last 25 scale back towards normal.
        } else {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( 25_turns - left ) ) / 25_turns );
        }
    }
    if( timed_events.queued( TIMED_EVENT_ARTIFACT_LIGHT ) ) {
        // TIMED_EVENT_ARTIFACT_LIGHT causes everywhere to become as bright as day.
        mod_ret = std::max<float>( ret, default_daylight_level() );
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
character_id game::assign_npc_id()
{
    character_id ret = next_npc_id;
    ++next_npc_id;
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

std::unordered_set<tripoint> game::get_fishable_locations( int distance, const tripoint &fish_pos )
{
    // We're going to get the contiguous fishable terrain starting at
    // the provided fishing location (e.g. where a line was cast or a fish
    // trap was set), and then check whether or not fishable monsters are
    // actually in those locations. This will help us ensure that we're
    // getting our fish from the location that we're ACTUALLY fishing,
    // rather than just somewhere in the vicinity.

    std::unordered_set<tripoint> visited;

    const tripoint fishing_boundary_min( fish_pos + point( -distance, -distance ) );
    const tripoint fishing_boundary_max( fish_pos + point( distance, distance ) );

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
            if( !fishing_boundaries.contains_inclusive( current_point ) ) {
                continue;
            }

            // Mark this point as visited.
            visited.emplace( current_point );

            if( m.has_flag( "FISHABLE", current_point ) ) {
                fishable_terrain.emplace( current_point );
                to_check.push( current_point + point_south );
                to_check.push( current_point + point_north );
                to_check.push( current_point + point_east );
                to_check.push( current_point + point_west );
            }
        }
        return;
    };

    // Starting at the provided location, get our fishable terrain
    // and populate a set with those locations which we'll then use
    // to determine if any fishable monsters are in those locations.
    std::unordered_set<tripoint> fishable_points;
    get_fishable_terrain( fish_pos, fishable_points );

    return fishable_points;
}

std::vector<monster *> game::get_fishable_monsters( std::unordered_set<tripoint>
        &fishable_locations )
{
    std::vector<monster *> unique_fish;
    for( monster &critter : all_monsters() ) {
        // If it is fishable...
        if( critter.has_flag( MF_FISHABLE ) ) {
            const tripoint critter_pos = critter.pos();
            // ...and it is in a fishable location.
            if( fishable_locations.find( critter_pos ) != fishable_locations.end() ) {
                unique_fish.push_back( &critter );
            }
        }
    }

    return unique_fish;
}

// Print monster info to the given window
void game::mon_info( const catacurses::window &w, int hor_padding )
{
    const int width = getmaxx( w ) - 2 * hor_padding;
    const int maxheight = getmaxy( w );

    const int startrow = 0;

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

    static int previous_turn = 0;
    // @todo change current_turn to time_point
    const int current_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    const int sm_ignored_turns = get_option<int>( "SAFEMODEIGNORETURNS" );

    for( auto &c : u.get_visible_creatures( MAPSIZE_X ) ) {
        const auto m = dynamic_cast<monster *>( c );
        const auto p = dynamic_cast<npc *>( c );
        const auto dir_to_mon = direction_from( view.xy(), point( c->posx(), c->posy() ) );
        const int mx = POSX + ( c->posx() - view.x );
        const int my = POSY + ( c->posy() - view.y );
        int index = 8;
        if( !is_valid_in_w_terrain( point( mx, my ) ) ) {
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
                                                 string_format( _( "%s spotted!" ), critter.name() ) );
                if( u.has_trait( trait_id( "M_DEFENDER" ) ) && critter.type->in_species( PLANT ) ) {
                    add_msg( m_warning, _( "We have detected a %s - an enemy of the Mycus!" ), critter.name() );
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
    } else if( current_turn > previous_turn && get_option<bool>( "AUTOSAFEMODE" ) &&
               newseen == 0 ) { // Auto-safe mode, but only if it's a new turn
        turnssincelastmon += current_turn - previous_turn;
        if( turnssincelastmon >= get_option<int>( "AUTOSAFEMODETURNS" ) && safe_mode == SAFE_MODE_OFF ) {
            set_safe_mode( SAFE_MODE_ON );
            add_msg( m_info, _( "Safe mode ON!" ) );
        }
    }

    if( newseen == 0 && safe_mode == SAFE_MODE_STOP ) {
        set_safe_mode( SAFE_MODE_ON );
    }

    previous_turn = current_turn;
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
        widths[i] = utf8_width( dir_labels[i] );
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
        mvwprintz( w, point( xcoords[i] + hor_padding, ycoords[i] + startrow ), c, dir_labels[i] );
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
            mvwprintz( w, pr, c, sym );

            pr.x++;
        }
    }

    // Now we print their full names!

    std::set<const mtype *> listed_mons;

    // Start printing monster names on row 4. Rows 0-2 are for labels, and row 3
    // is blank.
    point pr( hor_padding, 4 + startrow );

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
                pr.x = hor_padding;
            }

            if( pr.y < maxheight ) { // Don't print if we've overflowed
                mvwprintz( w, pr, mt.color, mt.sym );
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
                mvwprintz( w, pr, danger, name );
                pr.x += utf8_width( name ) + namesep;
            }
        }
    }
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
                remove_npc_follower( ( *it )->getID() );
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

    for( monster &critter : all_monsters() ) {
        // Critters in impassable tiles get pushed away, unless it's not impassable for them
        if( !critter.is_dead() && m.impassable( critter.pos() ) && !critter.can_move_to( critter.pos() ) ) {
            dbg( D_ERROR ) << "game:monmove: " << critter.name()
                           << " can't move to its location! (" << critter.posx()
                           << ":" << critter.posy() << ":" << critter.posz() << "), "
                           << m.tername( critter.pos() );
            add_msg( m_debug, "%s can't move to its location! (%d,%d,%d), %s", critter.name(),
                     critter.posx(), critter.posy(), critter.posz(), m.tername( critter.pos() ) );
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

        while( critter.moves > 0 && !critter.is_dead() && !critter.has_effect( effect_ridden ) ) {
            critter.made_footstep = false;
            // Controlled critters don't make their own plans
            if( !critter.has_effect( effect_controlled ) ) {
                // Formulate a path to follow
                critter.plan();
            } else {
                critter.moves = 0;
                break;
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
        while( !guy.is_dead() && ( !guy.in_sleep_state() || guy.activity.id() == "ACT_OPERATION" ) &&
               guy.moves > 0 && turns < 10 ) {
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
                          guy.name );
                debug_mode = true;
            }
        }

        // If we spun too long trying to decide what to do (without spending moves),
        // Invoke cranial detonation to prevent an infinite loop.
        if( turns == 10 ) {
            add_msg( _( "%s's brain explodes!" ), guy.name );
            guy.die( nullptr );
        }

        if( !guy.is_dead() ) {
            guy.process_active_items();
            guy.npc_update_body();
        }
    }
    cleanup_dead();
}

void game::overmap_npc_move()
{
    std::vector<npc *> travelling_npcs;
    for( auto &elem : overmap_buffer.get_npcs_near_player( 75 ) ) {
        if( !elem ) {
            continue;
        }
        npc *npc_to_add = elem.get();
        if( ( !npc_to_add->is_active() || rl_dist( u.pos(), npc_to_add->pos() ) > SEEX * 2 ) &&
            npc_to_add->mission == NPC_MISSION_TRAVELLING ) {
            travelling_npcs.push_back( npc_to_add );
        }
    }
    for( auto &elem : travelling_npcs ) {
        if( elem->has_omt_destination() ) {
            if( elem->omt_path.empty() ) {
                elem->omt_path = overmap_buffer.get_npc_path( elem->global_omt_location(), elem->goal );
            } else {
                if( elem->omt_path.back() == elem->global_omt_location() ) {
                    elem->omt_path.pop_back();
                }
                elem->travel_overmap( omt_to_sm_copy( elem->omt_path.back() ) );
            }
            reload_npcs();
        }
    }
    return;
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
    ( void )force; // FIXME: unused but header says it should do something
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
            add_msg( _( "%s was stunned!" ), targ->name() );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i].xy() ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    add_msg( _( "%s was stunned!" ), targ->name() );
                    add_msg( _( "%s slammed into an obstacle!" ), targ->name() );
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
                    add_msg( _( "%s was stunned!" ), targ->name() );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                if( critter_at<monster>( traj.front() ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->name() );
                } else if( npc *const guy = critter_at<npc>( traj.front() ) ) {
                    if( guy->male ) {
                        add_msg( _( "%s collided with someone else and sent him flying!" ),
                                 targ->name() );
                    } else {
                        add_msg( _( "%s collided with someone else and sent her flying!" ),
                                 targ->name() );
                    }
                } else if( u.pos() == traj.front() ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->name() );
                }
                knockback( traj, force_remaining, stun, dam_mult );
                break;
            }
            targ->setpos( traj[i] );
            if( m.has_flag( "LIQUID", targ->pos() ) && !targ->can_drown() && !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s drowns!" ), targ->name() );
                }
            }
            if( !m.has_flag( "LIQUID", targ->pos() ) && targ->has_flag( MF_AQUATIC ) &&
                !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s flops around and dies!" ), targ->name() );
                }
            }
        }
    } else if( npc *const targ = critter_at<npc>( tp ) ) {
        if( stun > 0 ) {
            targ->add_effect( effect_stunned, 1_turns * stun );
            add_msg( _( "%s was stunned!" ), targ->name );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i].xy() ) ) { // oops, we hit a wall!
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    if( targ->has_effect( effect_stunned ) ) {
                        add_msg( _( "%s was stunned!" ), targ->name );
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
                    add_msg( _( "%s was stunned!" ), targ->name );
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                const tripoint &traj_front = traj.front();
                if( critter_at<monster>( traj_front ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->name );
                } else if( npc *const guy = critter_at<npc>( traj_front ) ) {
                    if( guy->male ) {
                        add_msg( _( "%s collided with someone else and sent him flying!" ),
                                 targ->name );
                    } else {
                        add_msg( _( "%s collided with someone else and sent her flying!" ),
                                 targ->name );
                    }
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y &&
                           ( u.has_trait( trait_LEG_TENT_BRACE ) && ( !u.footwear_factor() ||
                                   ( u.footwear_factor() == .5 && one_in( 2 ) ) ) ) ) {
                    add_msg( _( "%s collided with you, and barely dislodges your tentacles!" ), targ->name );
                    force_remaining = 1;
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->name );
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
                avatar_action::swim( m, u, u.pos() );
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
        !u.has_effect( effect_contacts ) && !u.has_bionic( bionic_id( "bio_eye_optic" ) ) ) {
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

template<typename T>
T *game::critter_at( const tripoint &p, bool allow_hallucination )
{
    if( const std::shared_ptr<monster> mon_ptr = critter_tracker->find( p ) ) {
        if( !allow_hallucination && mon_ptr->is_hallucination() ) {
            return nullptr;
        }
        // if we wanted to check for an NPC / player / avatar,
        // there is sometimes a monster AND an NPC/player there at the same time.
        // because the NPC/player etc may be riding that monster.
        // so only return the monster if we were actually looking for a monster.
        // otherwise, keep looking for the rider.
        // critter_at<creature> or critter_at() with no template will still default to returning monster first,
        // which is ok for the occasions where that happens.
        if( !mon_ptr->has_effect( effect_ridden ) || ( std::is_same<T, monster>::value ||
                std::is_same<T, Creature>::value || std::is_same<T, const monster>::value ||
                std::is_same<T, const Creature>::value ) ) {
            return dynamic_cast<T *>( mon_ptr.get() );
        }
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
template const avatar *game::critter_at<avatar>( const tripoint &, bool ) const;
template avatar *game::critter_at<avatar>( const tripoint &, bool );
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
template std::shared_ptr<avatar> game::shared_from<avatar>( const avatar & );
template std::shared_ptr<monster> game::shared_from<monster>( const monster & );
template std::shared_ptr<npc> game::shared_from<npc>( const npc & );

template<typename T>
T *game::critter_by_id( const character_id id )
{
    if( id == u.getID() ) {
        // player is always alive, therefore no is-dead check
        return dynamic_cast<T *>( &u );
    }
    return find_npc( id );
}

// monsters don't have ids
template player *game::critter_by_id<player>( character_id );
template npc *game::critter_by_id<npc>( character_id );
template Creature *game::critter_by_id<Creature>( character_id );

static bool can_place_monster( game &g, const monster &mon, const tripoint &p )
{
    if( const monster *const critter = g.critter_at<monster>( p ) ) {
        // Creature_tracker handles this. The hallucination monster will simply vanish
        if( !critter->is_hallucination() ) {
            return false;
        }
    }
    // Although monsters can sometimes exist on the same place as a Character (e.g. ridden horse),
    // it is usually wrong. So don't allow it.
    if( g.critter_at<Character>( p ) ) {
        return false;
    }
    return mon.can_move_to( p );
}

static cata::optional<tripoint> choose_where_to_place_monster( game &g, const monster &mon,
        const tripoint_range &range )
{
    return random_point( range, [&]( const tripoint & p ) {
        return can_place_monster( g, mon, p );
    } );
}

monster *game::place_critter_at( const mtype_id &id, const tripoint &p )
{
    return place_critter_around( id, p, 0 );
}

monster *game::place_critter_at( const std::shared_ptr<monster> mon, const tripoint &p )
{
    return place_critter_around( mon, p, 0 );
}

monster *game::place_critter_around( const mtype_id &id, const tripoint &center, const int radius )
{
    // @todo change this into an assert, it must never happen.
    if( id.is_null() ) {
        return nullptr;
    }
    return place_critter_around( std::make_shared<monster>( id ), center, radius );
}

monster *game::place_critter_around( const std::shared_ptr<monster> mon, const tripoint &center,
                                     const int radius )
{
    cata::optional<tripoint> where;
    if( can_place_monster( *this, *mon, center ) ) {
        where = center;
    }

    // This loop ensures the monster is placed as close to the center as possible,
    // but all places that equally far from the center have the same probability.
    for( int r = 1; r <= radius && !where; ++r ) {
        where = choose_where_to_place_monster( *this, *mon, m.points_in_radius( center, r ) );
    }

    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    return critter_tracker->add( mon ) ? mon.get() : nullptr;
}

monster *game::place_critter_within( const mtype_id &id, const tripoint_range &range )
{
    // @todo change this into an assert, it must never happen.
    if( id.is_null() ) {
        return nullptr;
    }
    return place_critter_within( std::make_shared<monster>( id ), range );
}

monster *game::place_critter_within( const std::shared_ptr<monster> mon,
                                     const tripoint_range &range )
{
    const cata::optional<tripoint> where = choose_where_to_place_monster( *this, *mon, range );
    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    return critter_tracker->add( mon ) ? mon.get() : nullptr;
}

// By default don't pin upgrades to current day
bool game::add_zombie( const std::shared_ptr<monster> critter_ptr )
{
    return add_zombie( critter_ptr, false );
}

bool game::add_zombie( const std::shared_ptr<monster> critter_ptr, const bool pin_upgrade )
{
    assert( critter_ptr );
    monster &critter = *critter_ptr;

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
    return critter_tracker->add( critter_ptr );
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
    if( one_in( 100 ) ) {
        std::shared_ptr<npc> tmp = std::make_shared<npc>();
        tmp->normalize();
        tmp->randomize( NC_HALLU );
        tmp->spawn_at_precise( { get_levx(), get_levy() }, p );
        if( !critter_at( p, true ) ) {
            overmap_buffer.insert_npc( tmp );
            load_npcs();
            return true;
        } else {
            return false;
        }
    }

    const mtype_id &mt = MonsterGenerator::generator().get_valid_hallucination();
    const std::shared_ptr<monster> phantasm = std::make_shared<monster>( mt );
    phantasm->hallucination = true;
    phantasm->spawn( p );

    //Don't attempt to place phantasms inside of other creatures
    if( !critter_at( phantasm->pos(), true ) ) {
        return critter_tracker->add( phantasm );
    } else {
        return false;
    }
}

bool game::swap_critters( Creature &a, Creature &b )
{
    if( &a == &b ) {
        // No need to do anything, but print a debugmsg anyway
        debugmsg( "Tried to swap %s with itself", a.disp_name() );
        return true;
    }
    if( critter_at( a.pos() ) != &a ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  b.disp_name(), critter_at( a.pos() )->disp_name() );
        return false;
    }
    if( critter_at( b.pos() ) != &b ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  a.disp_name(), critter_at( b.pos() )->disp_name() );
        return false;
    }
    // Simplify by "sorting" the arguments
    // Only the first argument can be u
    // If swapping player/npc with a monster, monster is second
    bool a_first = a.is_player() ||
                   ( a.is_npc() && !b.is_player() );
    Creature &first  = a_first ? a : b;
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
             ( weather.weather == WEATHER_CLEAR || weather.weather == WEATHER_SUNNY ) );
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
    std::shared_ptr<monster> newmon_ptr = std::make_shared<monster>( it.get_mtype()->id );
    monster &critter = *newmon_ptr;
    critter.init_from_item( it );
    if( critter.get_hp() < 1 ) {
        // Failed reanimation due to corpse being too burned
        return false;
    }
    if( it.has_flag( "FIELD_DRESS" ) || it.has_flag( "FIELD_DRESS_FAILED" ) ||
        it.has_flag( "QUARTERED" ) ) {
        // Failed reanimation due to corpse being butchered
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

    return place_critter_at( newmon_ptr, p );
}

void game::save_cyborg( item *cyborg, const tripoint &couch_pos, player &installer )
{
    int assist_bonus = installer.get_effect_int( effect_assisted );

    float adjusted_skill = installer.bionics_adjusted_skill( skill_firstaid,
                           skill_computer,
                           skill_electronics,
                           -1 );

    int damage = cyborg->damage();
    int dmg_lvl = cyborg->damage_level( 4 );
    int difficulty = 12;

    if( damage != 0 ) {

        popup( _( "WARNING: Patient's body is damaged.  Difficulty of the procedure is increased by %s." ),
               dmg_lvl );

        // Damage of the cyborg increases difficulty
        difficulty += dmg_lvl;
    }

    int chance_of_success = bionic_manip_cos( adjusted_skill + assist_bonus, true, difficulty );
    int success = chance_of_success - rng( 1, 100 ) ;

    if( !g->u.query_yn(
            _( "WARNING: %i percent chance of SEVERE damage to all body parts!  Continue anyway?" ),
            100 - static_cast<int>( chance_of_success ) ) ) {
        return;
    }

    if( success > 0 ) {
        add_msg( m_good, _( "Successfully removed Personality override." ) );
        add_msg( m_bad, _( "Autodoc immediately destroys the CBM upon removal." ) );

        m.i_rem( couch_pos, cyborg );

        const string_id<npc_template> npc_cyborg( "cyborg_rescued" );
        std::shared_ptr<npc> tmp = std::make_shared<npc>();
        tmp->normalize();
        tmp->load_npc_template( npc_cyborg );
        tmp->spawn_at_precise( { get_levx(), get_levy() }, couch_pos );
        overmap_buffer.insert_npc( tmp );
        tmp->hurtall( dmg_lvl * 10, nullptr );
        tmp->add_effect( effect_downed, rng( 1_turns, 4_turns ), num_bp, false, 0, true );
        load_npcs();

    } else {
        const int failure_level = static_cast<int>( sqrt( abs( success ) * 4.0 * difficulty /
                                  adjusted_skill ) );
        const int fail_type = std::min( 5, failure_level );
        switch( fail_type ) {
            case 1:
            case 2:
                add_msg( m_info, _( "The removal fails." ) );
                add_msg( m_bad, _( "The body is damaged." ) );
                cyborg->set_damage( damage + 1000 );
                break;
            case 3:
            case 4:
                add_msg( m_info, _( "The removal fails badly." ) );
                add_msg( m_bad, _( "The body is badly damaged!" ) );
                cyborg->set_damage( damage + 2000 );
                break;
            case 5:
                add_msg( m_info, _( "The removal is a catastrophe." ) );
                add_msg( m_bad, _( "The body is destroyed!" ) );
                m.i_rem( couch_pos, cyborg );
                break;
            default:
                break;
        }

    }

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
        update_lum( loc, false );
        u.use( loc );
        update_lum( loc, true );

        make_active( loc );
    } else {
        u.use( pos );
    }

    u.invalidate_crafting_inventory();
}

void game::exam_vehicle( vehicle &veh, const point &c )
{
    auto act = veh_interact::run( veh, c );
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
    player *npc_or_player = critter_at<player>( tripoint( x, y, p.z ), false );
    if( npc_or_player != nullptr ) {
        if( bash_dmg <= 0 ) {
            return false;
        }
        if( npc_or_player->is_npc() && can_see ) {
            add_msg( _( "The %1$s hits the %2$s." ), door_name, npc_or_player->name );
        } else if( npc_or_player->is_player() ) {
            add_msg( m_bad, _( "The %s hits you." ), door_name );
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
            add_msg( _( "The %1$s hits the %2$s." ), door_name, critter.name() );
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
    if( bash_dmg < 0 && !m.i_at( point( x, y ) ).empty() ) {
        return false;
    }
    if( bash_dmg == 0 ) {
        for( auto &elem : m.i_at( point( x, y ) ) ) {
            if( elem.made_of( LIQUID ) ) {
                // Liquids are OK, will be destroyed later
                continue;
            } else if( elem.volume() < 250_ml ) {
                // Dito for small items, will be moved away
                continue;
            }
            // Everything else prevents the door from closing
            return false;
        }
    }

    m.ter_set( point( x, y ), door_type );
    if( m.has_flag( "NOITEM", point( x, y ) ) ) {
        map_stack items = m.i_at( point( x, y ) );
        for( map_stack::iterator it = items.begin(); it != items.end(); ) {
            if( it->made_of( LIQUID ) ) {
                it = items.erase( it );
                continue;
            }
            if( it->made_of( material_id( "glass" ) ) && one_in( 2 ) ) {
                if( can_see ) {
                    add_msg( m_warning, _( "A %s shatters!" ), it->tname() );
                } else {
                    add_msg( m_warning, _( "Something shatters!" ) );
                }
                it = items.erase( it );
                continue;
            }
            m.add_item_or_charges( point( kbx, kby ), *it );
            it = items.erase( it );
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
    tileray ray( dest_loc.xy() + point( -u.posx(), -u.posy() ) );
    const int d = ray.dir(); // TODO:: make dir() const correct!
    add_msg( _( "You dive from the %s." ), veh->name );
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
            veh->handle_potential_theft( dynamic_cast<player &>( u ) );
            return;
        }
        if( veh->engine_on ) {
            if( !veh->handle_potential_theft( dynamic_cast<player &>( u ) ) ) {
                return;
            }
            u.controlling_vehicle = true;
            add_msg( _( "You take control of the %s." ), veh->name );
        } else {
            if( !veh->handle_potential_theft( dynamic_cast<player &>( u ) ) ) {
                return;
            }
            veh->start_engines( true );
        }
    } else {    // Start looking for nearby vehicle controls.
        int num_valid_controls = 0;
        cata::optional<tripoint> vehicle_position;
        cata::optional<vpart_reference> vehicle_controls;
        for( const tripoint elem : m.points_in_radius( g->u.pos(), 1 ) ) {
            if( const optional_vpart_position vp = m.veh_at( elem ) ) {
                const cata::optional<vpart_reference> controls = vp.value().part_with_feature( "CONTROLS", true );
                if( controls ) {
                    num_valid_controls++;
                    vehicle_position = elem;
                    vehicle_controls = controls;
                }
            }
        }
        if( num_valid_controls < 1 ) {
            add_msg( _( "No vehicle controls found." ) );
            return;
        } else if( num_valid_controls > 1 ) {
            vehicle_position = choose_adjacent( _( "Control vehicle where?" ) );
            if( !vehicle_position ) {
                return;
            }
            const optional_vpart_position vp = m.veh_at( *vehicle_position );
            if( vp ) {
                vehicle_controls = vp.value().part_with_feature( "CONTROLS", true );
                if( !vehicle_controls ) {
                    add_msg( _( "The vehicle doesn't have controls there." ) );
                    return;
                }
            } else {
                add_msg( _( "No vehicle there." ) );
                return;
            }
        }
        // If we hit neither of those, there's only one set of vehicle controls, which should already have been found.
        if( vehicle_controls ) {
            veh = &vehicle_controls->vehicle();
            if( !veh->handle_potential_theft( dynamic_cast<player &>( u ) ) ) {
                return;
            }
            veh->use_controls( *vehicle_position );
            //May be folded up (destroyed), so need to re-get it
            veh = g->remoteveh();
        }
    }
    if( veh ) {
        // If we reached here, we gained control of a vehicle.
        // Clear the map memory for the area covered by the vehicle to eliminate ghost vehicles.
        for( const tripoint &target : veh->get_points() ) {
            u.clear_memorized_tile( m.getabs( target ) );
        }
        veh->is_following = false;
    }
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

    const bool obeys = debug_mode || ( who.is_player_ally() && !who.in_sleep_state() );

    uilist amenu;

    amenu.text = string_format( _( "What to do with %s?" ), who.disp_name() );
    amenu.addentry( talk, true, 't', _( "Talk" ) );
    amenu.addentry( swap_pos, obeys && !who.is_mounted() &&
                    !u.is_mounted(), 's', _( "Swap positions" ) );
    amenu.addentry( push, obeys && !who.is_mounted(), 'p', _( "Push away" ) );
    amenu.addentry( examine_wounds, true, 'w', _( "Examine wounds" ) );
    amenu.addentry( use_item, true, 'i', _( "Use item on" ) );
    amenu.addentry( sort_armor, true, 'r', _( "Sort armor" ) );
    amenu.addentry( attack, true, 'a', _( "Attack" ) );
    if( !who.is_player_ally() ) {
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
        add_msg( _( "You swap places with %s." ), who.name );
        swap_critters( u, who );
        // TODO: Make that depend on stuff
        u.mod_moves( -200 );
    } else if( choice == push ) {
        // TODO: Make NPCs protest when displaced onto dangerous crap
        tripoint oldpos = who.pos();
        who.move_away_from( u.pos(), true );
        u.mod_moves( -20 );
        if( oldpos != who.pos() ) {
            add_msg( _( "%s moves out of the way." ), who.name );
        } else {
            add_msg( m_warning, _( "%s has nowhere to go!" ), who.name );
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
            // fighting is hard work!
            u.increase_activity_level( EXTRA_EXERCISE );
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
    u.manual_examine = true;
    // redraw terrain to erase 'examine' window
    draw_ter();
    wrefresh( w_terrain );
    draw_panels( true );
    examine( *examp_ );
    u.manual_examine = false;
}

static std::string get_fire_fuel_string( const tripoint &examp )
{
    if( g->m.has_flag( TFLAG_FIRE_CONTAINER, examp ) ) {
        field_entry *fire = g->m.get_field( examp, fd_fire );
        if( fire ) {
            std::stringstream ss;
            ss << _( "There is a fire here." ) << " ";
            if( fire->get_field_intensity() > 1 ) {
                ss << _( "It's too big and unpredictable to evaluate how long it will last." );
                return ss.str();
            }
            time_duration fire_age = fire->get_field_age();
            // half-life inclusion
            int mod = 5 - g->u.get_skill_level( skill_survival );
            mod = std::max( mod, 0 );
            if( fire_age >= 0_turns ) {
                if( mod >= 4 ) { // = survival level 0-1
                    ss << _( "It's going to go out soon without extra fuel." );
                    return ss.str();
                } else {
                    fire_age = 30_minutes - fire_age;
                    if( to_string_approx( fire_age - fire_age * mod / 5 ) == to_string_approx(
                            fire_age + fire_age * mod / 5 ) ) {
                        ss << string_format(
                               _( "Without extra fuel it might burn yet for maybe %s, but might also go out sooner." ),
                               to_string_approx( fire_age - fire_age * mod / 5 ) );
                    } else {
                        ss << string_format(
                               _( "Without extra fuel it might burn yet for between %s to %s, but might also go out sooner." ),
                               to_string_approx( fire_age - fire_age * mod / 5 ),
                               to_string_approx( fire_age + fire_age * mod / 5 ) );
                    }
                    return ss.str();
                }
            } else {
                fire_age = fire_age * -1 + 30_minutes;
                if( mod >= 4 ) { // = survival level 0-1
                    if( fire_age <= 1_hours ) {
                        ss << _( "It's quite decent and looks like it'll burn for a bit without extra fuel." );
                        return ss.str();
                    } else if( fire_age <= 3_hours ) {
                        ss << _( "It looks solid, and will burn for a few hours without extra fuel." );
                        return ss.str();
                    } else {
                        ss << _( "It's very well supplied and even without extra fuel might burn for at least a part of a day." );
                        return ss.str();
                    }
                } else {
                    if( to_string_approx( fire_age - fire_age * mod / 5 ) == to_string_approx(
                            fire_age + fire_age * mod / 5 ) ) {
                        ss << string_format( _( "Without extra fuel it will burn for about %s." ),
                                             to_string_approx( fire_age - fire_age * mod / 5 ) );
                    } else {
                        ss << string_format( _( "Without extra fuel it will burn for between %s to %s." ),
                                             to_string_approx( fire_age - fire_age * mod / 5 ),
                                             to_string_approx( fire_age + fire_age * mod / 5 ) );
                    }
                    return ss.str();
                }
            }
        }
    }
    return {};
}

void game::examine( const tripoint &examp )
{
    Creature *c = critter_at( examp );
    if( c != nullptr ) {
        monster *mon = dynamic_cast<monster *>( c );
        if( mon != nullptr && mon->has_effect( effect_pet ) && !u.is_mounted() ) {
            if( monexamine::pet_menu( *mon ) ) {
                return;
            }
        } else if( mon && mon->has_flag( MF_RIDEABLE_MECH ) && !mon->has_effect( effect_pet ) ) {
            if( monexamine::mech_hack( *mon ) ) {
                return;
            }
        } else if( mon && mon->has_flag( MF_PAY_BOT ) ) {
            if( monexamine::pay_bot( *mon ) ) {
                return;
            }
        } else if( u.is_mounted() ) {
            add_msg( m_warning, _( "You cannot do that while mounted." ) );
        }
        npc *np = dynamic_cast<npc *>( c );
        if( np != nullptr && !u.is_mounted() ) {
            if( npc_menu( *np ) ) {
                return;
            }
        } else if( np != nullptr && u.is_mounted() ) {
            add_msg( m_warning, _( "You cannot do that while mounted." ) );
        }
    }

    const optional_vpart_position vp = m.veh_at( examp );
    if( vp && u.is_mounted() ) {
        if( !u.mounted_creature->has_flag( MF_RIDEABLE_MECH ) ) {
            add_msg( m_warning, _( "You cannot interact with a vehicle while mounted." ) );
        } else {
            vp->vehicle().interact_with( examp, vp->part_index() );
            return;
        }
    } else if( vp && !u.is_mounted() ) {
        vp->vehicle().interact_with( examp, vp->part_index() );
        return;
    }

    if( m.has_flag( "CONSOLE", examp ) && !u.is_mounted() ) {
        use_computer( examp );
        return;
    } else if( m.has_flag( "CONSOLE", examp ) && u.is_mounted() ) {
        add_msg( m_warning, _( "You cannot use a console while mounted." ) );
    }
    const furn_t &xfurn_t = m.furn( examp ).obj();
    const ter_t &xter_t = m.ter( examp ).obj();

    const tripoint player_pos = u.pos();

    if( m.has_furn( examp ) && !u.is_mounted() ) {
        xfurn_t.examine( u, examp );
    } else if( m.has_furn( examp ) && u.is_mounted() ) {
        add_msg( m_warning, _( "You cannot do that while mounted." ) );
    } else {
        if( !u.is_mounted() ) {
            xter_t.examine( u, examp );
        } else if( u.is_mounted() && xter_t.examine == &iexamine::none ) {
            xter_t.examine( u, examp );
        } else {
            add_msg( m_warning, _( "You cannot do that while mounted." ) );
        }
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

    if( !m.tr_at( examp ).is_null() && !u.is_mounted() ) {
        iexamine::trap( u, examp );
        draw_ter();
        wrefresh( w_terrain );
        draw_panels();
    } else if( !m.tr_at( examp ).is_null() && u.is_mounted() ) {
        add_msg( m_warning, _( "You cannot do that while mounted." ) );
    }

    // In case of teleport trap or somesuch
    if( player_pos != u.pos() ) {
        return;
    }

    // Feedback for fire lasting time, this can be judged while mounted
    const std::string fire_fuel = get_fire_fuel_string( examp );
    if( !fire_fuel.empty() ) {
        add_msg( fire_fuel );
    }

    if( m.has_flag( "SEALED", examp ) ) {
        if( none ) {
            if( m.has_flag( "UNSTABLE", examp ) ) {
                add_msg( _( "The %s is too unstable to remove anything." ), m.name( examp ) );
            } else {
                add_msg( _( "The %s is firmly sealed." ), m.name( examp ) );
            }
        }
    } else {
        //examp has no traps, is a container and doesn't have a special examination function
        if( m.tr_at( examp ).is_null() && m.i_at( examp ).empty() &&
            m.has_flag( "CONTAINER", examp ) && none ) {
            add_msg( _( "It is empty." ) );
        } else if( ( m.has_flag( TFLAG_FIRE_CONTAINER, examp ) &&
                     xfurn_t.examine == &iexamine::fireplace ) ||
                   xfurn_t.examine == &iexamine::workbench ) {
            return;
        } else {
            sounds::process_sound_markers( &u );
            Pickup::pick_up( examp, 0 );
        }
    }
}

void game::pickup()
{
    // First check if there is no/only one option for pickup
    int num_tiles_with_items = 0;
    tripoint tile_with_items = u.pos();
    for( const tripoint &p : m.points_in_radius( u.pos(), 1 ) ) {
        bool veh_has_items = false;
        const optional_vpart_position vp = m.veh_at( p );
        if( vp ) {
            const int cargo_part = vp->vehicle().part_with_feature( vp->part_index(), "CARGO", false );
            veh_has_items = cargo_part >= 0 && !vp->vehicle().get_items( cargo_part ).empty();
        }
        if( m.has_items( p ) || veh_has_items ) {
            ++num_tiles_with_items;
            tile_with_items = p;
        }
    }
    if( num_tiles_with_items == 0 ) {
        add_msg( _( "There's nothing to pick up there" ) );
        return;
    } else if( num_tiles_with_items == 1 ) {
        pickup( tile_with_items );
        return;
    }

    const cata::optional<tripoint> examp_ = choose_adjacent_highlight( _( "Pickup where?" ),
                                            ACTION_PICKUP );
    if( !examp_ ) {
        return;
    }
    // redraw terrain to erase 'pickup' window
    draw_ter();
    // wrefresh is called in pickup( const tripoint & )
    pickup( *examp_ );
}

void game::pickup( const tripoint &p )
{
    // Highlight target
    g->m.drawsq( w_terrain, u, p, true, true, u.pos() + u.view_offset );
    wrefresh( w_terrain );

    Pickup::pick_up( p, 0 );
}

void game::pickup_feet()
{
    Pickup::pick_up( u.pos(), 1 );
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
        draw_panels();
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
        avatar_action::plthrow( u, INT_MIN, p );
    }
    m.invalidate_map_cache( p.z );

    draw_ter();
    wrefresh( w_terrain );
    draw_panels();
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
            print_creature_info( creature, w_look, column, line, last_line );
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
        mvwprintw( w_look, point( 1, ++line ), _( "You heard %s from here." ), this_sound );
    } else {
        // Check other z-levels
        tripoint tmp = lp;
        for( tmp.z = -OVERMAP_DEPTH; tmp.z <= OVERMAP_HEIGHT; tmp.z++ ) {
            if( tmp.z == lp.z ) {
                continue;
            }

            auto zlev_sound = sounds::sound_at( tmp );
            if( !zlev_sound.empty() ) {
                mvwprintw( w_look, point( 1, ++line ), tmp.z > lp.z ?
                           _( "You heard %s from above." ) : _( "You heard %s from below." ), zlev_sound );
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

    mvwprintw( w_look, point( line, column ), visibility_message );
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
        lines = fold_and_print( w_look, point( column, line ), max_width, c_light_gray,
                                _( "%s; Impassable" ),
                                tile );
    } else {
        lines = fold_and_print( w_look, point( column, line ), max_width, c_light_gray,
                                _( "%s; Movement cost %d" ),
                                tile, m.move_cost( lp ) * 50 );

        const auto ll = get_light_level( std::max( 1.0,
                                         LIGHT_AMBIENT_LIT - m.ambient_light_at( lp ) + 1.0 ) );
        mvwprintw( w_look, point( column, ++lines ), _( "Lighting: " ) );
        wprintz( w_look, ll.second, ll.first );
    }

    std::string signage = m.get_signage( lp );
    if( !signage.empty() ) {
        trim_and_print( w_look, point( column, ++lines ), max_width, c_dark_gray,
                        u.has_trait( trait_ILLITERATE ) ? _( "Sign: ???" ) : _( "Sign: %s" ), signage );
    }

    if( m.has_zlevels() && lp.z > -OVERMAP_DEPTH && !m.has_floor( lp ) ) {
        // Print info about stuff below
        tripoint below( lp.xy(), lp.z - 1 );
        std::string tile_below = m.tername( below );
        if( m.has_furn( below ) ) {
            tile_below += "; " + m.furnname( below );
        }

        if( !m.has_floor_or_support( lp ) ) {
            fold_and_print( w_look, point( column, ++lines ), max_width, c_dark_gray,
                            _( "Below: %s; No support" ),
                            tile_below );
        } else {
            fold_and_print( w_look, point( column, ++lines ), max_width, c_dark_gray,
                            _( "Below: %s; Walkable" ),
                            tile_below );
        }
    }

    int map_features = fold_and_print( w_look, point( column, ++lines ), max_width, c_dark_gray,
                                       m.features( lp ) );
    fold_and_print( w_look, point( column, ++lines ), max_width, c_light_gray, _( "Coverage: %d%%" ),
                    m.coverage( lp ) );
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
        if( fld.first.obj().has_fire && ( m.has_flag( TFLAG_FIRE_CONTAINER, lp ) ||
                                          m.ter( lp ) == t_pit_shallow || m.ter( lp ) == t_pit ) ) {
            const int max_width = getmaxx( w_look ) - column - 2;
            int lines = fold_and_print( w_look, point( column, ++line ), max_width, cur.color(),
                                        get_fire_fuel_string( lp ) ) - 1;
            line += lines;
        } else {
            mvwprintz( w_look, point( column, ++line ), cur.color(), cur.name() );
        }
    }
}

void game::print_trap_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                            int &line )
{
    const trap &tr = m.tr_at( lp );
    if( tr.can_see( lp, u ) ) {
        partial_con *pc = g->m.partial_con_at( lp );
        std::string tr_name;
        if( pc && tr.loadid == tr_unfinished_construction ) {
            const std::vector<construction> &list_constructions = get_constructions();
            const construction &built = list_constructions[pc->id];
            tr_name = string_format( _( "Unfinished task: %s, %d%% complete" ), built.description,
                                     pc->counter / 100000 );
        } else {
            tr_name = tr.name();
        }

        mvwprintz( w_look, point( column, ++line ), tr.color, tr_name );
    }
}

void game::print_creature_info( const Creature *creature, const catacurses::window &w_look,
                                const int column, int &line, const int last_line )
{
    int vLines = last_line - line;
    if( creature != nullptr && ( u.sees( *creature ) || creature == &u ) ) {
        line = creature->print_info( w_look, ++line, vLines, column );
    }
}

void game::print_vehicle_info( const vehicle *veh, int veh_part, const catacurses::window &w_look,
                               const int column, int &line, const int last_line )
{
    if( veh ) {
        mvwprintw( w_look, point( column, ++line ), _( "There is a %s there. Parts:" ), veh->name );
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

    mvwputch( w_terrain, point( POSX, POSY ), visibility_indicator_color, visibility_indicator );
}

void game::print_items_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                             int &line,
                             const int last_line )
{
    if( !m.sees_some_items( lp, u ) ) {
        return;
    } else if( m.has_flag( "CONTAINER", lp ) && !m.could_see_items( lp, u ) ) {
        mvwprintw( w_look, point( column, ++line ), _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( "BLIND" ) ) {
        mvwprintz( w_look, point( column, ++line ), c_yellow,
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
                mvwprintz( w_look, point( column, ++line ), c_yellow, _( "More items here..." ) );
                break;
            }

            if( it.second > 1 ) {
                trim_and_print( w_look, point( column, ++line ), max_width, c_white,
                                pgettext( "%s is the name of the item. %d is the quantity of that item.", "%s [%d]" ),
                                it.first.c_str(), it.second );
            } else {
                trim_and_print( w_look, point( column, ++line ), max_width, c_white, it.first );
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
        fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                        m.ter( lp ) == t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                        m.graffiti_at( lp ) );
    }
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
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<A>dd" ) ) + 2;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<R>emove" ) ) + 2;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<D>isable" ) ) + 2;

    tmpx = 1;
    tmpx += shortcut_print( w_info, point( tmpx, 2 ), c_white, c_light_green,
                            _( "<+-> Move up/down" ) ) + 2;
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    tmpx += shortcut_print( w_info, point( tmpx, 2 ), c_white, c_light_green, _( "<Enter>-Edit" ) ) + 2;

    tmpx = 1;
    tmpx += shortcut_print( w_info, point( tmpx, 3 ), c_white, c_light_green,
                            _( "<S>how all / hide distant" ) ) + 2;
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    tmpx += shortcut_print( w_info, point( tmpx, 3 ), c_white, c_light_green, _( "<M>ap" ) ) + 2;

    wrefresh( w_info );
}

static void zones_manager_draw_borders( const catacurses::window &w_border,
                                        const catacurses::window &w_info_border,
                                        const int iInfoHeight, const int width )
{
    for( int i = 1; i < TERMX; ++i ) {
        if( i < width ) {
            mvwputch( w_border, point( i, 0 ), c_light_gray, LINE_OXOX ); // -
            mvwputch( w_border, point( i, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2 ) {
            mvwputch( w_border, point( 0, i ), c_light_gray, LINE_XOXO ); // |
            mvwputch( w_border, point( width - 1, i ), c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( w_border, point_zero, c_light_gray, LINE_OXXO ); // |^
    mvwputch( w_border, point( width - 1, 0 ), c_light_gray, LINE_OOXX ); // ^|

    mvwputch( w_border, point( 0, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
              LINE_XXXO ); // |-
    mvwputch( w_border, point( width - 1, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( w_border, point( 2, 0 ), c_white, _( "Zones manager" ) );
    wrefresh( w_border );

    for( int j = 0; j < iInfoHeight - 1; ++j ) {
        mvwputch( w_info_border, point( 0, j ), c_light_gray, LINE_XOXO );
        mvwputch( w_info_border, point( width - 1, j ), c_light_gray, LINE_XOXO );
    }

    for( int j = 0; j < width - 1; ++j ) {
        mvwputch( w_info_border, point( j, iInfoHeight - 1 ), c_light_gray, LINE_OXOX );
    }

    mvwputch( w_info_border, point( 0, iInfoHeight - 1 ), c_light_gray, LINE_XXOO );
    mvwputch( w_info_border, point( width - 1, iInfoHeight - 1 ), c_light_gray, LINE_XOOX );
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

    const int width = 45;
    const int offsetX = get_option<std::string>( "SIDEBAR_POSITION" ) == "left" ?
                        TERMX + VIEW_OFFSET_X - width : VIEW_OFFSET_X;
    int w_zone_height = TERMY - zone_ui_height - VIEW_OFFSET_Y * 2;
    catacurses::window w_zones = catacurses::newwin( w_zone_height - 2, width - 2,
                                 point( offsetX + 1, VIEW_OFFSET_Y + 1 ) );
    catacurses::window w_zones_border = catacurses::newwin( w_zone_height, width,
                                        point( offsetX, VIEW_OFFSET_Y ) );
    catacurses::window w_zones_info = catacurses::newwin( zone_ui_height - zone_options_height - 1,
                                      width - 2, point( offsetX + 1, w_zone_height + VIEW_OFFSET_Y ) );
    catacurses::window w_zones_info_border = catacurses::newwin( zone_ui_height, width,
            point( offsetX, w_zone_height + VIEW_OFFSET_Y ) );
    catacurses::window w_zones_options = catacurses::newwin( zone_options_height - 1, width - 2,
                                         point( offsetX + 1, TERMY - zone_options_height - VIEW_OFFSET_Y ) );

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
    const int max_rows = w_zone_height - 2;
    int start_index = 0;
    int active_index = 0;
    bool blink = false;
    bool stuff_changed = false;
    bool show_all_zones = false;
    int zone_cnt = 0;

    // get zones on the same z-level, with distance between player and
    // zone center point <= 50 or all zones, if show_all_zones is true
    auto get_zones = [&]() {
        std::vector<zone_manager::ref_zone_data> zones;
        if( show_all_zones ) {
            zones = mgr.get_zones();
        } else {
            const tripoint &u_abs_pos = m.getabs( u.pos() );
            for( zone_manager::ref_zone_data &ref : mgr.get_zones() ) {
                const tripoint &zone_abs_pos = ref.get().get_center_point();
                if( u_abs_pos.z == zone_abs_pos.z && rl_dist( u_abs_pos, zone_abs_pos ) <= 50 ) {
                    zones.emplace_back( ref );
                }
            }
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

                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_zones_options, point( 1, 0 ), c_white, _( "Options" ) );

                int y = 1;
                for( const auto &desc : descriptions ) {
                    mvwprintz( w_zones_options, point( 3, y ), c_white, desc.first );
                    mvwprintz( w_zones_options, point( 20, y ), c_white, desc.second );
                    y++;
                }
            }
        }

        wrefresh( w_zones_options );
    };

    auto query_position = [this, w_zones_info]() -> cata::optional<std::pair<tripoint, tripoint>> {
        werase( w_zones_info );
        mvwprintz( w_zones_info, point( 2, 3 ), c_white, _( "Select first point." ) );
        wrefresh( w_zones_info );

        tripoint center = u.pos() + u.view_offset;

        const look_around_result first = look_around( w_zones_info, center, center, false, true,
                false );
        if( first.position )
        {
            mvwprintz( w_zones_info, point( 2, 3 ), c_white, _( "Select second point." ) );
            wrefresh( w_zones_info );

            const look_around_result second = look_around( w_zones_info, center, *first.position,
                    true, true, false );
            if( second.position ) {
                werase( w_zones_info );
                wrefresh( w_zones_info );

                tripoint first_abs = m.getabs( tripoint( std::min( first.position->x,
                                               second.position->x ),
                                               std::min( first.position->y, second.position->y ),
                                               std::min( first.position->z,
                                                       second.position->z ) ) );
                tripoint second_abs = m.getabs( tripoint( std::max( first.position->x,
                                                second.position->x ),
                                                std::max( first.position->y, second.position->y ),
                                                std::max( first.position->z,
                                                        second.position->z ) ) );
                return std::pair<tripoint, tripoint>( first_abs, second_abs );
            }
        }

        return cata::nullopt;
    };

    zones_manager_open = true;
    do {
        if( action == "ADD_ZONE" ) {
            zones_manager_draw_borders( w_zones_border, w_zones_info_border,
                                        zone_ui_height, width );

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

                mgr.add( name, id, g->u.get_faction()->id, false, true, position->first,
                         position->second, options );

                zones = get_zones();
                active_index = zone_cnt - 1;

                stuff_changed = true;
            } while( false );

            draw_ter();
            blink = false;

            zones_manager_draw_borders( w_zones_border, w_zones_info_border,
                                        zone_ui_height, width );
            zones_manager_shortcuts( w_zones_info );

        } else if( action == "SHOW_ALL_ZONES" ) {
            show_all_zones = !show_all_zones;
            zones = get_zones();
            active_index = 0;
            draw_ter();

        } else if( zone_cnt > 0 ) {
            if( action == "UP" ) {
                active_index--;
                if( active_index < 0 ) {
                    active_index = zone_cnt - 1;
                }
                draw_ter();
                blink = false;

            } else if( action == "DOWN" ) {
                active_index++;
                if( active_index >= zone_cnt ) {
                    active_index = 0;
                }
                draw_ter();
                blink = false;

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
                    draw_panels( true );
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "CONFIRM" ) {
                auto &zone = zones[active_index].get();

                uilist as_m;
                as_m.text = _( "What do you want to change:" );
                as_m.entries.emplace_back( 1, true, '1', _( "Edit name" ) );
                as_m.entries.emplace_back( 2, true, '2', _( "Edit type" ) );
                as_m.entries.emplace_back( 3, zone.get_options().has_options(), '3',
                                           zone.get_type() == zone_type_id( "LOOT_CUSTOM" ) ? _( "Edit filter" ) : _( "Edit options" ) );
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
                        if( pos && ( pos->first != zone.get_start_point() ||
                                     pos->second != zone.get_end_point() ) ) {
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

                zones_manager_draw_borders( w_zones_border, w_zones_info_border,
                                            zone_ui_height, width );
                zones_manager_shortcuts( w_zones_info );

            } else if( action == "MOVE_ZONE_UP" && zone_cnt > 1 ) {
                if( active_index < zone_cnt - 1 ) {
                    mgr.swap( zones[active_index], zones[active_index + 1] );
                    zones = get_zones();
                    active_index++;
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "MOVE_ZONE_DOWN" && zone_cnt > 1 ) {
                if( active_index > 0 ) {
                    mgr.swap( zones[active_index], zones[active_index - 1] );
                    zones = get_zones();
                    active_index--;
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "SHOW_ZONE_ON_MAP" ) {
                //show zone position on overmap;
                tripoint player_overmap_position = ms_to_omt_copy( m.getabs( u.pos() ) );
                tripoint zone_overmap = ms_to_omt_copy( zones[active_index].get().get_center_point() );

                ui::omap::display_zones( player_overmap_position, zone_overmap, active_index );

                zones_manager_draw_borders( w_zones_border, w_zones_info_border,
                                            zone_ui_height, width );
                zones_manager_shortcuts( w_zones_info );

                draw_ter();

            } else if( action == "ENABLE_ZONE" ) {
                zones[active_index].get().set_enabled( true );

                stuff_changed = true;

            } else if( action == "DISABLE_ZONE" ) {
                zones[active_index].get().set_enabled( false );

                stuff_changed = true;
            }
        }

        if( zone_cnt == 0 ) {
            werase( w_zones );
            wrefresh( w_zones_border );
            mvwprintz( w_zones, point( 2, 5 ), c_white, _( "No Zones defined." ) );

        } else {
            werase( w_zones );

            calcStartPos( start_index, active_index, max_rows, zone_cnt );

            draw_scrollbar( w_zones_border, active_index, max_rows, zone_cnt, point_south );
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
                        mvwprintz( w_zones, point( 0, iNum - start_index ), c_yellow, "%s", ">>" );
                        colorLine = ( zone.get_enabled() ) ? c_light_green : c_green;
                    }

                    //Draw Zone name
                    mvwprintz( w_zones, point( 3, iNum - start_index ), colorLine,
                               zone.get_name() );

                    //Draw Type name
                    mvwprintz( w_zones, point( 20, iNum - start_index ), colorLine,
                               mgr.get_name_from_type( zone.get_type() ) );

                    tripoint center = zone.get_center_point();

                    //Draw direction + distance
                    mvwprintz( w_zones, point( 32, iNum - start_index ), colorLine, "%*d %s",
                               5, static_cast<int>( trig_dist( player_absolute_pos, center ) ),
                               direction_name_short( direction_from( player_absolute_pos,
                                                     center ) ) );

                    //Draw Vehicle Indicator
                    mvwprintz( w_zones, point( 41, iNum - start_index ), colorLine,
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
#if defined(TILES)
                if( use_tiles ) {
                    offset = tripoint_zero; //TILES
                } else {
                    offset = tripoint( -offset_x, -offset_y, 0 ); //SDL
                }
#endif

                draw_zones( start, end, offset );
            } else {
                //clear marked area
#if defined(TILES)
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
                                    mvwputch( w_terrain, point( iX - offset_x, iY - offset_y ),
                                              c_magenta, '#' );
                                } else {
                                    mvwputch( w_terrain, point( iX - offset_x, iY - offset_y ),
                                              c_black, ' ' );
                                }
                            }
                        }
                    }
#if defined(TILES)
                }
#endif
            }

            ctxt.set_timeout( BLINK_SPEED );
        } else {
            ctxt.reset_timeout();
        }

        wrefresh( w_terrain );
        zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
        zones_manager_shortcuts( w_zones_info );
        draw_panels();
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
    const bool allow_zlev_move = m.has_zlevels();

    temp_exit_fullscreen();

    const int offset_x = ( u.posx() + u.view_offset.x ) - getmaxx( w_terrain ) / 2;
    const int offset_y = ( u.posy() + u.view_offset.y ) - getmaxy( w_terrain ) / 2;

    tripoint lp = start_point; // cursor
    if( !has_first_point ) {
        lp = start_point;
    }
    int &lx = lp.x;
    int &ly = lp.y;
    int &lz = lp.z;

    draw_ter( center );
    wrefresh( w_terrain );
    draw_panels();

    int soffset = get_option<int>( "FAST_SCROLL_OFFSET" );
    bool fast_scroll = false;

    bool bNewWindow = false;
    if( !w_info ) {
        int panel_width = panel_manager::get_manager().get_current_layout().begin()->get_width();

        // Set the examine window to a bit smaller than the current minimap size, with a bit less to show some above it.
        // Hopefully the player has the minimap at or close to the bottom.
        int height = TERMY - ( catacurses::getmaxy( w_pixel_minimap ) + 11 );

        // If particularly small, base height on panel width irrespective of other elements.
        // Value here is attempting to get a square-ish result assuming 1x2 proportioned font.
        if( height < panel_width / 2 ) {
            height = panel_width / 2;
        }

        int la_y = 0;
        int la_x = TERMX - panel_width;
        int la_h = height;
        int la_w = panel_width;
        w_info = catacurses::newwin( la_h, la_w, point( la_x, la_y ) );
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
    ctxt.register_action( "debug_temp" );
    ctxt.register_action( "debug_visibility" );
    ctxt.register_action( "debug_radiation" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );

    const int old_levz = get_levz();
    const int min_levz = std::max( old_levz - fov_3d_z_range, -OVERMAP_DEPTH );
    const int max_levz = std::min( old_levz + fov_3d_z_range, OVERMAP_HEIGHT );

    m.update_visibility_cache( old_levz );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    bool blink = true;
    bool redraw = true;
    look_around_result result;
    do {
        if( redraw ) {
            if( bNewWindow ) {
                werase( w_info );
                draw_border( w_info );

                static const std::string title_prefix = "< ";
                const std::string title = _( "Look Around" );
                static const std::string title_suffix = " >";
                static const std::string full_title = title_prefix + title + title_suffix;
                const int start_pos = center_text_pos( full_title, 0, getmaxx( w_info ) - 1 );
                mvwprintz( w_info, point( start_pos, 0 ), c_white, title_prefix );
                wprintz( w_info, c_green, title );
                wprintz( w_info, c_white, title_suffix );

                std::string fast_scroll_text = string_format( _( "%s - %s" ),
                                               ctxt.get_desc( "TOGGLE_FAST_SCROLL" ),
                                               ctxt.get_action_name( "TOGGLE_FAST_SCROLL" ) );
                center_print( w_info, getmaxy( w_info ) - 1, fast_scroll ? c_light_green : c_green,
                              fast_scroll_text );

                int first_line = 1;
                const int last_line = getmaxy( w_info ) - 2;
                pre_print_all_tile_info( lp, w_info, first_line, last_line, cache );
            }

            draw_ter( center, true );

            if( select_zone && has_first_point ) {
                if( blink ) {
                    const int dx = start_point.x - offset_x + u.posx() - lx;
                    const int dy = start_point.y - offset_y + u.posy() - ly;

                    const tripoint start = tripoint( std::min( dx, POSX ), std::min( dy, POSY ), lz );
                    const tripoint end = tripoint( std::max( dx, POSX ), std::max( dy, POSY ), lz );

                    tripoint offset; //ASCII/SDL
#if defined(TILES)
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

            // redraw order: terrain, panels, look_around panel
            wrefresh( w_terrain );
            draw_panels();
            wrefresh( w_info );

        }

        if( select_zone && has_first_point ) {
            ctxt.set_timeout( BLINK_SPEED );
        }

        redraw = true;
        //Wait for input
        // only specify a timeout here if "EDGE_SCROLL" is enabled
        // otherwise use the previously set timeout
        int scroll_timeout = get_option<int>( "EDGE_SCROLL" );
        if( scroll_timeout >= 0 ) {
            action = ctxt.handle_input( scroll_timeout );
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
            lz = clamp( lz + dz, min_levz, max_levz );
            center.z = clamp( center.z + dz, min_levz, max_levz );

            add_msg( m_debug, "levx: %d, levy: %d, levz :%d", get_levx(), get_levy(), center.z );
            u.view_offset.z = center.z - u.posz();
            m.invalidate_map_cache( center.z );
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
        } else if( action == "debug_temp" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_temperature();
            }
        } else if( action == "debug_radiation" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_radiation();
            }
        } else if( action == "EXTENDED_DESCRIPTION" ) {
            extended_description( lp );
        } else if( action == "CENTER" ) {
            center = u.pos();
            lp = u.pos();
            u.view_offset.z = 0;
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            // This block is structured this way so that edge scroll can work
            // whether the mouse is moving at the edge or simply stationary
            // at the edge. But even if edge scroll isn't in play, there's
            // other things for us to do here.

            if( action == "TIMEOUT" ) {
                blink = !blink;
            }
            tripoint edge_scroll = mouse_edge_scrolling_terrain( ctxt );
            if( edge_scroll != tripoint_zero ) {
                if( action == "MOUSE_MOVE" ) {
                    edge_scroll *= 2;
                }
                center += edge_scroll;
            } else if( action == "MOUSE_MOVE" ) {

                const tripoint old_lp = lp;
                const tripoint old_center = center;

                const cata::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain );
                if( mouse_pos ) {
                    lx = mouse_pos->x;
                    ly = mouse_pos->y;
                }
                if( select_zone && has_first_point ) { // is blinking
                    if( blink && lp == old_lp ) { // blink symbols drawn (blink == true) and cursor not changed
                        redraw = false; // no need to redraw, so don't redraw to save CPU
                    } else {
                        blink = true; // Always draw blink symbols when moving cursor
                    }
                } else if( lp == old_lp && center == old_center ) { // not blinking and cursor not changed
                    redraw = false; // no need to redraw, so don't redraw to save CPU
                }
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
        } else if( action == "throw_blind" ) {
            result.peek_action = PA_BLIND_THROW;
        } else if( action == "zoom_in" ) {
            center.x = lp.x;
            center.y = lp.y;
            zoom_in();
        } else if( action == "zoom_out" ) {
            center.x = lp.x;
            center.y = lp.y;
            zoom_out();
        }
    } while( action != "QUIT" && action != "CONFIRM" && action != "SELECT" && action != "TRAVEL_TO" &&
             action != "throw_blind" );

    if( m.has_zlevels() && center.z != old_levz ) {
        m.invalidate_map_cache( old_levz );
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
            mvwputch( w_terrain, point( POSX, POSY ), c_white, sym );
        } else {
            mvwputch( w_terrain, pts[pts.size() - 1].xy() - u.view_offset.xy() + point( POSX - u.posx(),
                      POSY - u.posy() ),
                      c_white, sym );
        }
    }

    wrefresh( w_terrain );
}

static void centerlistview( const tripoint &active_item_position, int ui_width )
{
    player &u = g->u;
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z = active_item_position.z;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            u.view_offset.x = active_item_position.x;
            u.view_offset.y = active_item_position.y;
        } else {
            int xpos = POSX + active_item_position.x;
            int ypos = POSY + active_item_position.y;

            // item/monster list UI is on the right, so get the difference between its width
            // and the width of the sidebar on the right (if any)
            int sidebar_right_adjusted = ui_width - panel_manager::get_manager().get_width_right();
            // if and only if that difference is greater than zero, use that as offset
            int right_offset = sidebar_right_adjusted > 0 ? sidebar_right_adjusted : 0;

            // Convert offset to tile counts, calculate adjusted terrain window width
            // This lets us account for possible differences in terrain width between
            // the normal sidebar and the list-all-whatever display.
            to_map_font_dim_width( right_offset );
            int terrain_width = TERRAIN_WINDOW_WIDTH - right_offset;

            if( xpos < 0 ) {
                u.view_offset.x = xpos;
            } else if( xpos >= terrain_width ) {
                u.view_offset.x = xpos - ( terrain_width - 1 );
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
#if defined(TILES)
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
#if defined(TILES)
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
#if defined(TILES)
    tileset_zoom = DEFAULT_TILESET_ZOOM;
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

bool game::take_screenshot( const std::string &path ) const
{
#if defined(TILES)
    return save_screenshot( path );
#else
    ( void )path; // unused
    return false;
#endif
}

//helper method so we can keep list_items shorter
void game::reset_item_list_state( const catacurses::window &window, int height, bool bRadiusSort )
{
    const int width = 44;
    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( window, point( i, 0 ), c_light_gray, LINE_OXOX ); // -
            mvwputch( window, point( i, TERMY - height - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - height - VIEW_OFFSET_Y * 2 ) {
            mvwputch( window, point( 0, i ), c_light_gray, LINE_XOXO ); // |
            mvwputch( window, point( width - 1, i ), c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( window, point_zero, c_light_gray, LINE_OXXO ); // |^
    mvwputch( window, point( width - 1, 0 ), c_light_gray, LINE_OOXX ); // ^|

    mvwputch( window, point( 0, TERMY - height - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
              LINE_XXXO ); // |-
    mvwputch( window, point( width - 1, TERMY - height - 1 - VIEW_OFFSET_Y * 2 ), c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( window, point( 2, 0 ), c_light_green, "<Tab> " );
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

    shortcut_print( window, point( getmaxx( window ) - letters, 0 ), c_white, c_light_green, sSort );

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
        xpos += shortcut_print( window, point( xpos, ypos ), c_white, c_light_green,
                                tokens[i] ) + gap_spaces;
    }

    refresh_all();
}

void game::list_items_monsters()
{
    std::vector<Creature *> mons = u.get_visible_creatures( current_daylight_level( calendar::turn ) );
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
        avatar_action::fire( u, m, u.weapon );
    }
    reenter_fullscreen();
}

game::vmenu_ret game::list_items( const std::vector<map_item_stack> &item_list )
{
    int iInfoHeight = std::min( 25, TERMY / 2 );
    const int width = 44;
    const int offsetX = TERMX - VIEW_OFFSET_X - width;

    catacurses::window w_items = catacurses::newwin( TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2,
                                 width - 2, point( offsetX + 1, VIEW_OFFSET_Y + 1 ) );
    catacurses::window w_items_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                        width, point( offsetX, VIEW_OFFSET_Y ) );
    catacurses::window w_item_info = catacurses::newwin( iInfoHeight, width,
                                     point( offsetX, TERMY - iInfoHeight - VIEW_OFFSET_Y ) );

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
    bool refilter = true;
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    map_item_stack *activeItem = nullptr;
    std::map<int, std::string> mSortCategory;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", translate_marker( "Move cursor up" ) );
    ctxt.register_action( "DOWN", translate_marker( "Move cursor down" ) );
    ctxt.register_action( "LEFT", translate_marker( "Previous item" ) );
    ctxt.register_action( "RIGHT", translate_marker( "Next item" ) );
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
            refilter = true;
            addcategory = !sort_radius;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            iLastActive.reset();
            refilter = true;
            uistate.list_item_filter_active = false;
            addcategory = !sort_radius;
        } else if( action == "EXAMINE" && !filtered_items.empty() ) {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            int dummy = 0; // draw_item_info needs an int &
            assert( activeItem ); // To appease static analysis
            activeItem->example->info( true, vThisItem );
            draw_item_info( 0, width - 5, 0, TERMY - VIEW_OFFSET_Y * 2,
                            activeItem->example->tname(), activeItem->example->type_name(), vThisItem, vDummy, dummy,
                            false, false, true );
            // wait until the user presses a key to wipe the screen
            iLastActive.reset();
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
            iItemNum = static_cast<int>( filtered_items.size() ) + iCatSortNum;
        }

        reset_item_list_state( w_items_border, iInfoHeight, sort_radius );

        if( action == "HELP_KEYBINDINGS" ) {
            draw_ter();
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
            if( !filtered_items.empty() ) {
                assert( activeItem ); // To appease static analysis
                if( ++page_num >= static_cast<int>( activeItem->vIG.size() ) ) {
                    page_num = activeItem->vIG.size() - 1;
                }
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
            mvwprintz( w_items, point( 2, 10 ), c_white, _( "You don't see any items around you!" ) );
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
                        mvwprintz( w_items, point( 1, iNum - iStartPos ), c_magenta, mSortCategory[iNum] );
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
                        trim_and_print( w_items, point( 1, iNum - iStartPos ), width - 9, col, sText.str() );
                        const int numw = iItemNum > 9 ? 2 : 1;
                        const int x = iter->vIG[iThisPage].pos.x;
                        const int y = iter->vIG[iThisPage].pos.y;
                        mvwprintz( w_items, point( width - 6 - numw, iNum - iStartPos ),
                                   iNum == iActive ? c_light_green : c_light_gray,
                                   "%*d %s", numw, rl_dist( point_zero, point( x, y ) ),
                                   direction_name_short( direction_from( point_zero, point( x, y ) ) ) );
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
            mvwprintz( w_items_border, point( ( width - 9 ) / 2 + ( iItemNum > 9 ? 0 : 1 ), 0 ),
                       c_light_green, " %*d", iItemNum > 9 ? 2 : 1, iItemNum > 0 ? iActive - iNum + 1 : 0 );
            wprintz( w_items_border, c_white, " / %*d ", iItemNum > 9 ? 2 : 1, iItemNum - iCatSortNum );
            werase( w_item_info );

            if( iItemNum > 0 ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                activeItem->example->info( true, vThisItem );
                draw_item_info( w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true );

                iLastActive.emplace( active_pos );
                centerlistview( active_pos, width );
                draw_trail_to_square( active_pos, true );
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, point_south );
            wrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty();

        draw_custom_border( w_item_info, bDrawLeft, true, false, true, LINE_XOXO, LINE_XOXO, true, true );
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
    int iInfoHeight = 14;
    const int width = 45;
    const int offsetX = TERMX - VIEW_OFFSET_X - width; //VIEW_OFFSET_X;
    catacurses::window w_monsters = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                    width - 2, point( offsetX + 1, VIEW_OFFSET_Y + 1 ) );
    catacurses::window w_monsters_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                           width, point( offsetX, VIEW_OFFSET_Y ) );
    catacurses::window w_monster_info = catacurses::newwin( iInfoHeight - 1, width - 2,
                                        point( offsetX + 1, TERMY - iInfoHeight - VIEW_OFFSET_Y ) );
    catacurses::window w_monster_info_border = catacurses::newwin( iInfoHeight, width + 1,
            point( offsetX, TERMY - iInfoHeight - VIEW_OFFSET_Y ) );

    const int max_gun_range = u.weapon.gun_range( &u );

    const tripoint stored_view_offset = u.view_offset;
    u.view_offset = tripoint_zero;

    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2 - 1;
    int iStartPos = 0;
    cata::optional<tripoint> iLastActivePos;
    Creature *cCurMon = nullptr;

    for( int j = 0; j < iInfoHeight - 1; j++ ) {
        mvwputch( w_monster_info_border, point( 0, j ), c_light_gray, LINE_XOXO );
        mvwputch( w_monster_info_border, point( width - 1, j ), c_light_gray, LINE_XOXO );
    }

    for( int j = 0; j < width - 1; j++ ) {
        mvwputch( w_monster_info_border, point( j, iInfoHeight - 1 ), c_light_gray, LINE_OXOX );
    }

    mvwputch( w_monsters_border, point_zero, BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w_monsters_border, point( width - 1, 0 ), BORDER_COLOR, LINE_OOXX ); // ^|

    mvwputch( w_monsters_border, point( 0, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ), BORDER_COLOR,
              LINE_XXXO ); // |-
    mvwputch( w_monsters_border, point( width - 1, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ),
              BORDER_COLOR,
              LINE_XOXX ); // -|

    for( int i = 1; i < getmaxy( w_monsters ) - 1; i++ ) {
        mvwputch( w_monsters_border, point( 0, i ), BORDER_COLOR, LINE_XOXO ); // |
        mvwputch( w_monsters_border, point( width - 1, i ), BORDER_COLOR, LINE_XOXO ); // |
    }

    mvwprintz( w_monsters_border, point( 2, 0 ), c_light_green, "<Tab> " );
    wprintz( w_monsters_border, c_white, _( "Monsters" ) );

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_action( "UP", translate_marker( "Move cursor up" ) );
    ctxt.register_action( "DOWN", translate_marker( "Move cursor down" ) );
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
            draw_ter();
            wrefresh( w_terrain );
        } else if( action == "UP" ) {
            iActive--;
            if( iActive < 0 ) {
                iActive = static_cast<int>( monster_list.size() ) - 1;
            }
        } else if( action == "DOWN" ) {
            iActive++;
            if( iActive >= static_cast<int>( monster_list.size() ) ) {
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
            mvwprintz( w_monsters, point( 2, 10 ), c_white, _( "You don't see any monsters around you!" ) );
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
                        const std::string cat_name = Creature::get_attitude_ui_data(
                                                         CatSortIter->second ).first.translated();
                        mvwprintz( w_monsters, point( 1, y ), c_magenta, cat_name );
                        ++CatSortIter;
                        continue;
                    }
                }
                // select current monster
                const auto critter = monster_list[iCurMon];
                const bool selected = iCurMon == iActive;
                ++iCurMon;
                if( critter->sees( g->u ) ) {
                    mvwprintz( w_monsters, point( 0, y ), c_yellow, "!" );
                }
                bool is_npc = false;
                const monster *m = dynamic_cast<monster *>( critter );
                const npc     *p = dynamic_cast<npc *>( critter );

                if( m != nullptr ) {
                    mvwprintz( w_monsters, point( 1, y ), selected ? c_light_green : c_white, m->name() );
                } else {
                    mvwprintz( w_monsters, point( 1, y ), selected ? c_light_green : c_white, critter->disp_name() );
                    is_npc = true;
                }

                if( selected && !get_safemode().empty() ) {
                    for( int i = 1; i < width - 2; i++ ) {
                        mvwputch( w_monsters_border, point( i, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ), BORDER_COLOR,
                                  LINE_OXOX ); // -
                    }
                    const std::string monName = is_npc ? get_safemode().npc_type_name() : m->name();

                    std::string sSafemode;
                    if( get_safemode().has_rule( monName, Creature::A_ANY ) ) {
                        sSafemode = _( "<R>emove from safemode Blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safemode Blacklist" );
                    }

                    shortcut_print( w_monsters_border, point( 3, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2 ),
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
                mvwprintz( w_monsters, point( 22, y ), color, sText );

                if( m != nullptr ) {
                    const auto att = m->get_attitude();
                    sText = att.first;
                    color = att.second;
                } else if( p != nullptr ) {
                    sText = npc_attitude_name( p->get_attitude() );
                    color = p->symbol_color();
                }
                mvwprintz( w_monsters, point( 28, y ), color, sText );

                mvwprintz( w_monsters, point( width - ( 6 + numw ), y ),
                           ( selected ? c_light_green : c_light_gray ),
                           "%*d %s",
                           numw, rl_dist( u.pos(), critter->pos() ),
                           direction_name_short( direction_from( u.pos(), critter->pos() ) ) );
            }

            mvwprintz( w_monsters_border, point( ( width / 2 ) - numw - 2, 0 ), c_light_green, " %*d", numw,
                       iActive + 1 );
            wprintz( w_monsters_border, c_white, " / %*d ", numw, static_cast<int>( monster_list.size() ) );

            cCurMon = monster_list[iActive];

            werase( w_monster_info );
            cCurMon->print_info( w_monster_info, 1, 11, 1 );

            if( bVMonsterLookFire ) {
                mvwprintz( w_monsters, point( 1, getmaxy( w_monsters ) - 3 ), c_light_green,
                           ctxt.press_x( "look" ) );
                wprintz( w_monsters, c_light_gray, " %s", _( "to look around" ) );

                if( rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                    wprintz( w_monsters, c_light_gray, "%s", " " );
                    mvwprintz( w_monsters, point( 24, getmaxy( w_monsters ) - 3 ), c_light_green,
                               ctxt.press_x( "fire" ) );
                    wprintz( w_monsters, c_light_gray, " %s", _( "to shoot" ) );
                }
            }

            // Only redraw trail/terrain if x/y position changed or if keybinding menu erased it
            tripoint iActivePos = cCurMon->pos() - u.pos();
            iLastActivePos.emplace( iActivePos );
            centerlistview( iActivePos, width );
            draw_trail_to_square( iActivePos, false );

            draw_scrollbar( w_monsters_border, iActive, iMaxRows, static_cast<int>( monster_list.size() ),
                            point_south );
            wrefresh( w_monsters_border );
        }

        // repairing the damage caused by refreshing the whole screen for w_terrain
        // the previous situation was only refreshing the screen minus sidebar width.
        for( int j = 0; j < iInfoHeight - 1; j++ ) {
            mvwputch( w_monster_info_border, point( 0, j ), c_light_gray, LINE_XOXO );
            mvwputch( w_monster_info_border, point( width - 1, j ), c_light_gray, LINE_XOXO );
        }

        for( int j = 0; j < width - 1; j++ ) {
            mvwputch( w_monster_info_border, point( j, iInfoHeight - 1 ), c_light_gray, LINE_OXOX );
        }

        mvwhline( w_monsters, point( 0, getmaxy( w_monsters ) - 2 ), 0, 45 );

        for( int i = 1; i < getmaxy( w_monsters ) - 1; i++ ) {
            mvwputch( w_monsters_border, point( 0, i ), BORDER_COLOR, LINE_XOXO ); // |
            mvwputch( w_monsters_border, point( width - 1, i ), BORDER_COLOR, LINE_XOXO ); // |
        }

        mvwputch( w_monster_info_border, point( 0, getmaxy( w_monster_info_border ) - 1 ), BORDER_COLOR,
                  LINE_XXOO );  // |_
        mvwputch( w_monster_info_border, point( width - 1, getmaxy( w_monster_info_border ) - 1 ),
                  BORDER_COLOR, LINE_XOOX ); // _|

        wrefresh( w_monsters_border );
        wrefresh( w_monster_info_border );
        wrefresh( w_monsters );
        wrefresh( w_monster_info );
        catacurses::refresh();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return game::vmenu_ret::QUIT;
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

// Used to set up the first Hotkey in the display set
static int get_initial_hotkey( const size_t menu_index )
{
    int hotkey = -1;
    if( menu_index == 0 ) {
        const int butcher_key = inp_mngr.get_previously_pressed_key();
        if( butcher_key != 0 ) {
            hotkey = butcher_key;
        }
    }
    return hotkey;
}

// Returns a vector of pairs.
//    Pair.first is the iterator to the first item with a unique tname.
//    Pair.second is the number of equivalent items per unique tname
// There are options for optimization here, but the function is hit infrequently
// enough that optimizing now is not a useful time expenditure.
static std::vector<std::pair<map_stack::iterator, int>> generate_butcher_stack_display(
            std::vector<map_stack::iterator> &its )
{
    std::vector<std::pair<map_stack::iterator, int>> result;
    std::vector<std::string> result_strings;
    result.reserve( its.size() );
    result_strings.reserve( its.size() );

    for( map_stack::iterator &it : its ) {
        const std::string tname = it->tname();
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
            result.emplace_back( it, 0 );
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
static void add_corpses( uilist &menu, const std::vector<map_stack::iterator> &its,
                         size_t &menu_index )
{
    int hotkey = get_initial_hotkey( menu_index );

    for( const map_stack::iterator &it : its ) {
        menu.addentry( menu_index++, true, hotkey, it->get_mtype()->nname() );
        hotkey = -1;
    }
}

// Salvagables stack so we need to pass in a stack vector rather than an item index vector
static void add_salvagables( uilist &menu,
                             const std::vector<std::pair<map_stack::iterator, int>> &stacks,
                             size_t &menu_index, const salvage_actor &salvage_iuse )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

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
static void add_disassemblables( uilist &menu,
                                 const std::vector<std::pair<map_stack::iterator, int>> &stacks, size_t &menu_index )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

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
static void butcher_submenu( const std::vector<map_stack::iterator> &corpses, int corpse = -1 )
{
    auto cut_time = [&]( enum butcher_type bt ) {
        int time_to_cut = 0;
        if( corpse != -1 ) {
            time_to_cut = butcher_time_to_cut( g->u, *corpses[corpse], bt );
        } else {
            for( const map_stack::iterator &it : corpses ) {
                time_to_cut += butcher_time_to_cut( g->u, *it, bt );
            }
        }
        return to_string_clipped( time_duration::from_turns( time_to_cut / 100 ) );
    };
    const bool enough_light = g->u.fine_detail_vision_mod() <= 4;

    const int factor = g->u.max_quality( quality_id( "BUTCHER" ) );
    const std::string msgFactor = factor > INT_MIN
                                  ? string_format( _( "Your best tool has %d butchering." ), factor )
                                  :  _( "You have no butchering tool." );

    const int factorD = g->u.max_quality( quality_id( "CUT_FINE" ) );
    const std::string msgFactorD = factorD > INT_MIN
                                   ? string_format( _( "Your best tool has %d fine cutting." ), factorD )
                                   :  _( "You have no fine cutting tool." );

    bool has_skin = false;
    bool has_organs = false;

    if( corpse != -1 ) {
        const mtype *dead_mon = corpses[corpse]->get_mtype();
        if( dead_mon ) {
            for( const harvest_entry &entry : dead_mon->harvest.obj() ) {
                if( entry.type == "skin" ) {
                    has_skin = true;
                }
                if( entry.type == "offal" ) {
                    has_organs = true;
                }
            }
        }
    }

    uilist smenu;
    smenu.desc_enabled = true;
    smenu.text = _( "Choose type of butchery:" );

    const std::string cannot_see = colorize( _( "can't see!" ), c_red );

    smenu.addentry_col( BUTCHER, enough_light, 'B', _( "Quick butchery" ),
                        enough_light ? cut_time( BUTCHER ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "This technique is used when you are in a hurry, "
                                          "but still want to harvest something from the corpse. "
                                          " Yields are lower as you don't try to be precise, "
                                          "but it's useful if you don't want to set up a workshop.  "
                                          "Prevents zombies from raising." ),
                                       msgFactor ) );
    smenu.addentry_col( BUTCHER_FULL, enough_light, 'b', _( "Full butchery" ),
                        enough_light ? cut_time( BUTCHER_FULL ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "This technique is used to properly butcher a corpse, "
                                          "and requires a rope & a tree or a butchering rack, "
                                          "a flat surface (for ex. a table, a leather tarp, etc.) "
                                          "and good tools.  Yields are plentiful and varied, "
                                          "but it is time consuming." ),
                                       msgFactor ) );
    smenu.addentry_col( F_DRESS, enough_light &&
                        has_organs, 'f', _( "Field dress corpse" ),
                        enough_light ? ( has_organs ? cut_time( F_DRESS ) : colorize( _( "has no organs" ),
                                         c_red ) ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "Technique that involves removing internal organs and "
                                          "viscera to protect the corpse from rotting from inside.  "
                                          "Yields internal organs.  Carcass will be lighter and will "
                                          "stay fresh longer.  Can be combined with other methods for "
                                          "better effects." ),
                                       msgFactor ) );
    smenu.addentry_col( SKIN, enough_light &&
                        has_skin, 's', _( "Skin corpse" ),
                        enough_light ? ( has_skin ? cut_time( SKIN ) : colorize( _( "has no skin" ), c_red ) ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "Skinning a corpse is an involved and careful process that "
                                          "usually takes some time.  You need skill and an appropriately "
                                          "sharp and precise knife to do a good job.  Some corpses are "
                                          "too small to yield a full-sized hide and will instead produce "
                                          "scraps that can be used in other ways." ),
                                       msgFactor ) );
    smenu.addentry_col( QUARTER, enough_light, 'k', _( "Quarter corpse" ),
                        enough_light ? cut_time( QUARTER ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "By quartering a previously field dressed corpse you will "
                                          "acquire four parts with reduced weight and volume.  It "
                                          "may help in transporting large game.  This action destroys "
                                          "skin, hide, pelt, etc., so don't use it if you want to "
                                          "harvest them later." ),
                                       msgFactor ) );
    smenu.addentry_col( DISMEMBER, true, 'm', _( "Dismember corpse" ), cut_time( DISMEMBER ),
                        string_format( "%s  %s",
                                       _( "If you're aiming to just destroy a body outright and don't "
                                          "care about harvesting it, dismembering it will hack it apart "
                                          "in a very short amount of time but yields little to no usable flesh." ),
                                       msgFactor ) );
    smenu.addentry_col( DISSECT, enough_light, 'd', _( "Dissect corpse" ),
                        enough_light ? cut_time( DISSECT ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "By careful dissection of the corpse, you will examine it for "
                                          "possible bionic implants, or discrete organs and harvest them "
                                          "if possible.  Requires scalpel-grade cutting tools, ruins "
                                          "corpse, and consumes a lot of time.  Your medical knowledge "
                                          "is most useful here." ),
                                       msgFactorD ) );
    smenu.query();
    switch( smenu.ret ) {
        case BUTCHER:
            g->u.assign_activity( activity_id( "ACT_BUTCHER" ), 0, true );
            break;
        case BUTCHER_FULL:
            g->u.assign_activity( activity_id( "ACT_BUTCHER_FULL" ), 0, true );
            break;
        case F_DRESS:
            g->u.assign_activity( activity_id( "ACT_FIELD_DRESS" ), 0, true );
            break;
        case SKIN:
            g->u.assign_activity( activity_id( "ACT_SKIN" ), 0, true );
            break;
        case QUARTER:
            g->u.assign_activity( activity_id( "ACT_QUARTER" ), 0, true );
            break;
        case DISMEMBER:
            g->u.assign_activity( activity_id( "ACT_DISMEMBER" ), 0, true );
            break;
        case DISSECT:
            g->u.assign_activity( activity_id( "ACT_DISSECT" ), 0, true );
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
    const std::string no_knife_msg = _( "You don't have a butchering tool." );
    const std::string no_corpse_msg = _( "There are no corpses here to butcher." );

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
    std::vector<map_stack::iterator> corpses;
    std::vector<map_stack::iterator> disassembles;
    std::vector<map_stack::iterator> salvageables;
    map_stack items = m.i_at( u.pos() );
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
    for( map_stack::iterator it = items.begin(); it != items.end(); ++it ) {
        if( it->is_corpse() ) {
            corpses.push_back( it );
        } else {
            if( ( salvage_tool_index != INT_MIN ) && salvage_iuse->valid_to_cut_up( *it ) ) {
                salvageables.push_back( it );
            }
            if( u.can_disassemble( *it, crafting_inv ).success() ) {
                disassembles.push_back( it );
            } else if( !first_item_without_tools ) {
                first_item_without_tools = &*it;
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

        if( first_item_without_tools ) {
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
                       hostile_critter->disp_name() ) ) {
            return;
        }
    }

    // Magic indices for special butcher options
    enum : int {
        MULTISALVAGE = MAX_ITEM_IN_SQUARE + 1,
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
    // Index to std::vector of iterators...
    int indexer_index = 0;

    // Generate the indexed stacks so we can display them nicely
    const auto disassembly_stacks = generate_butcher_stack_display( disassembles );
    const auto salvage_stacks = generate_butcher_stack_display( salvageables );
    // Always ask before cutting up/disassembly, but not before butchery
    size_t ret = 0;
    if( !corpses.empty() || !disassembles.empty() || !salvageables.empty() ) {
        uilist kmenu;
        kmenu.text = _( "Choose corpse to butcher / item to disassemble" );

        size_t i = 0;
        // Add corpses, disassembleables, and salvagables to the UI
        add_corpses( kmenu, corpses, i );
        add_disassemblables( kmenu, disassembly_stacks, i );
        if( !salvageables.empty() ) {
            assert( salvage_iuse ); // To appease static analysis
            add_salvagables( kmenu, salvage_stacks, i, *salvage_iuse );
        }

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b', _( "Butcher everything" ) );
        }
        if( disassembles.size() > 1 ) {
            int time_to_disassemble = 0;
            int time_to_disassemble_all = 0;
            for( const auto &stack : disassembly_stacks ) {
                const int time = recipe_dictionary::get_uncraft( stack.first->typeId() ).time;
                time_to_disassemble += time;
                time_to_disassemble_all += time * stack.second;
            }

            kmenu.addentry_col( MULTIDISASSEMBLE_ONE, true, 'D', _( "Disassemble everything once" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble / 100 ) ) );
            kmenu.addentry_col( MULTIDISASSEMBLE_ALL, true, 'd', _( "Disassemble everything" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble_all / 100 ) ) );
        }
        if( salvageables.size() > 1 ) {
            assert( salvage_iuse ); // To appease static analysis
            int time_to_salvage = 0;
            for( const auto &stack : salvage_stacks ) {
                time_to_salvage += salvage_iuse->time_to_cut_up( *stack.first ) * stack.second;
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
        add_msg( m_info, _( "%s helps with this task..." ), np->name );
        break;
    }
    switch( butcher_select ) {
        case BUTCHER_OTHER:
            switch( indexer_index ) {
                case MULTISALVAGE:
                    u.assign_activity( activity_id( "ACT_LONGSALVAGE" ), 0, salvage_tool_index );
                    break;
                case MULTIBUTCHER:
                    butcher_submenu( corpses );
                    for( map_stack::iterator &it : corpses ) {
                        u.activity.targets.emplace_back( map_cursor( u.pos() ), &*it );
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
            butcher_submenu( corpses, indexer_index );
            draw_ter();
            wrefresh( w_terrain );
            draw_panels( true );
            u.activity.targets.emplace_back( map_cursor( u.pos() ), &*corpses[indexer_index] );
        }
        break;
        case BUTCHER_DISASSEMBLE: {
            // Pick index of first item in the disassembly stack
            item *const target = &*disassembly_stacks[indexer_index].first;
            u.disassemble( item_location( map_cursor( u.pos() ), target ), true );
        }
        break;
        case BUTCHER_SALVAGE: {
            // Pick index of first item in the salvage stack
            item *const target = &*salvage_stacks[indexer_index].first;
            item_location item_loc( map_cursor( u.pos() ), target );
            salvage_iuse->cut_up( u, *salvage_tool, item_loc );
        }
        break;
    }
}

void game::eat()
{
    eat( game_menus::inv::consume, INT_MIN );
}

void game::eat( int pos )
{
    eat( game_menus::inv::consume, pos );
}

void game::eat( item_location( *menu )( player &p ) )
{
    eat( menu, INT_MIN );
}

void game::eat( item_location( *menu )( player &p ), int pos )
{
    if( ( u.has_active_mutation( trait_RUMINANT ) || u.has_active_mutation( trait_GRAZER ) ) &&
        ( m.ter( u.pos() ) == t_underbrush || m.ter( u.pos() ) == t_shrub ) ) {
        if( u.get_hunger() < 20 ) {
            add_msg( _( "You're too full to eat the leaves from the %s." ), m.ter( u.pos() )->name() );
            return;
        } else {
            u.moves -= 400;
            m.ter_set( u.pos(), t_grass );
            add_msg( _( "You eat the underbrush." ) );
            item food( "underbrush", calendar::turn, 1 );
            u.eat( food );
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
            add_msg( _( "You eat the grass." ) );
            item food( item( "grass", calendar::turn, 1 ) );
            u.eat( food );
            m.ter_set( u.pos(), t_dirt );
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

    auto item_loc = menu( u );
    if( !item_loc ) {
        u.cancel_activity();
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
            add_msg( _( "You leave the empty %s." ), it->tname() );
        } else {
            item_loc.remove_item();
        }
    }
    if( g->u.get_value( "THIEF_MODE_KEEP" ) != "YES" ) {
        g->u.set_value( "THIEF_MODE", "THIEF_ASK" );
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

void game::reload( item_location &loc, bool prompt, bool empty )
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
            u.ammo_location = opt.ammo;
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
                add_msg( m_info, _( "The %s is already fully loaded!" ), it->tname() );
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
                add_msg( m_info, _( "The %s is already full!" ), it->tname() );
                return;
            }

        // intentional fall-through

        case HINT_CANT:
            add_msg( m_info, _( "You can't reload a %s!" ), it->tname() );
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
                              item::reload_option( &u, it, it, u.ammo_location ) :
                              u.select_ammo( *it, prompt, empty );

    if( opt.ammo.get_item() != nullptr && opt.ammo.get_item()->is_frozen_liquid() ) {
        if( !u.crush_frozen_liquid( opt.ammo ) ) {
            return;
        }
    }

    if( opt ) {
        u.assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
        if( use_loc ) {
            u.activity.targets.emplace_back( loc );
        } else {
            u.activity.targets.emplace_back( u, const_cast<item *>( opt.target ) );
        }
        u.activity.targets.push_back( std::move( opt.ammo ) );
    }

    refresh_all();
}

// Reload something.
void game::reload_item()
{
    item_location item_loc = inv_map_splice( [&]( const item & it ) {
        return u.rate_action_reload( it ) == HINT_GOOD;
    }, _( "Reload item" ), 1, _( "You have nothing to reload." ) );

    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    reload( item_loc );
}

void game::reload_weapon( bool try_everything )
{
    // As a special streamlined activity, hitting reload repeatedly should:
    // Reload wielded gun
    // First reload a magazine if necessary.
    // Then load said magazine into gun.
    // Reload magazines that are compatible with the current gun.
    // Reload other guns in inventory.
    // Reload misc magazines in inventory.
    std::vector<item_location> reloadables = u.find_reloadables();
    std::sort( reloadables.begin(), reloadables.end(),
    [this]( const item_location & a, const item_location & b ) {
        const item *ap = a.get_item();
        const item *bp = b.get_item();
        // Current wielded weapon comes first.
        if( this->u.is_wielding( *bp ) ) {
            return false;
        }
        if( this->u.is_wielding( *ap ) ) {
            return true;
        }
        // Second sort by afiliation with wielded gun
        const std::set<itype_id> compatible_magazines = this->u.weapon.magazine_compatible();
        const bool mag_ap = compatible_magazines.count( ap->typeId() ) > 0;
        const bool mag_bp = compatible_magazines.count( bp->typeId() ) > 0;
        if( mag_ap != mag_bp ) {
            return mag_ap;
        }
        // Third sort by gun vs magazine,
        if( ap->is_gun() != bp->is_gun() ) {
            return ap->is_gun();
        }
        // Finally sort by speed to reload.
        return ( ap->get_reload_time() * ( ap->ammo_capacity() - ap->ammo_remaining() ) ) <
               ( bp->get_reload_time() * ( bp->ammo_capacity() - bp->ammo_remaining() ) );
    } );
    for( item_location &candidate : reloadables ) {
        std::vector<item::reload_option> ammo_list;
        u.list_ammo( *candidate.get_item(), ammo_list, false );
        if( !ammo_list.empty() ) {
            reload( candidate, false, false );
            return;
        }
    }
    // Just for testing, bail out here to avoid unwanted side effects.
    if( !try_everything ) {
        return;
    }
    // If we make it here and haven't found anything to reload, start looking elsewhere.
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

    reload_item();
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
            if( u.style_selected != matype_id( "style_none" ) ) {
                u.martialart_use_message();
            }
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
        //~ %1$s: weapon name, %2$s: holster name
        if( query_yn( pgettext( "holster", "Draw %1$s from %2$s?" ), target.get_contained().tname(),
                      target.tname() ) ) {
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
        auto ret = u.can_takeoff( *loc.get_item() );
        if( !ret.success() ) {
            add_msg( m_info, "%s", ret.c_str() );
            return;
        }
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
        if( u.get_int_base() < 5 || u.has_trait( trait_id( "PROF_CHURL" ) ) ) {
            add_msg( game_message_params{ m_warning, gmf_bypass_cooldown },
                     _( "There's an angry red dot on your body, %s to brush it off." ), msg_ignore );
        } else {
            add_msg( game_message_params{ m_warning, gmf_bypass_cooldown },
                     _( "You are being laser-targeted, %s to ignore." ), msg_ignore );
        }
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
                                   press_x( ACTION_WHITELIST_ENEMY ) );
    }

    const std::string msg_safe_mode = press_x( ACTION_TOGGLE_SAFEMODE );
    add_msg( game_message_params{ m_warning, gmf_bypass_cooldown },
             _( "Spotted %1$s--safe mode is on! (%2$s to turn it off, %3$s to ignore monster%4$s)" ),
             spotted_creature_name, msg_safe_mode, msg_ignore, whitelist );
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
    if( critter.friendly == 0 || critter.has_flag( MF_RIDEABLE_MECH ) ||
        ( critter.has_flag( MF_PAY_BOT ) && critter.has_effect( effect_paid ) ) ) {
        // Can only disable / reprogram friendly monsters
        return false;
    }
    const auto mid = critter.type->id;
    const auto mon_item_id = critter.type->revert_to_itype;
    if( !mon_item_id.empty() &&
        query_yn( _( "Deactivate the %s?" ), critter.name() ) ) {

        u.moves -= 100;
        m.add_item_or_charges( p.xy(), critter.to_item() );
        if( !critter.has_flag( MF_INTERIOR_AMMO ) ) {
            for( auto &ammodef : critter.ammo ) {
                if( ammodef.second > 0 ) {
                    m.spawn_item( p.xy(), ammodef.first, 1, ammodef.second, calendar::turn );
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
                                 critter.name() );
                    }
                } else {
                    critter.add_effect( effect_docile, 1_turns, num_bp, true );
                    if( one_in( 3 ) ) {
                        add_msg( _( "The %s lets out a whirring noise and starts to follow you." ),
                                 critter.name() );
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
        if( u.is_dangerous_field( e.second ) && fields_here.find_field( e.first ) == nullptr ) {
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
                harmful_stuff.emplace_back( tr.name() );
            }
        } else if( tr.can_see( dest_loc, u ) && !tr.is_benign() && !boardable ) {
            harmful_stuff.emplace_back( tr.name() );
        }

        static const std::set< body_part > sharp_bps = {
            bp_eyes, bp_mouth, bp_head, bp_leg_l, bp_leg_r, bp_foot_l, bp_foot_r, bp_arm_l, bp_arm_r,
            bp_hand_l, bp_hand_r, bp_torso
        };

        const auto sharp_bp_check = [this]( body_part bp ) {
            return u.immune_to( bp, { DT_CUT, 10 } );
        };

        if( m.has_flag( "ROUGH", dest_loc ) && !m.has_flag( "ROUGH", u.pos() ) && !boardable &&
            ( u.get_armor_bash( bp_foot_l ) < 5 || u.get_armor_bash( bp_foot_r ) < 5 ) ) {
            harmful_stuff.emplace_back( m.name( dest_loc ) );
        } else if( m.has_flag( "SHARP", dest_loc ) && !m.has_flag( "SHARP", u.pos() ) && !( u.in_vehicle ||
                   g->m.veh_at( dest_loc ) ) &&
                   u.dex_cur < 78 && !std::all_of( sharp_bps.begin(), sharp_bps.end(), sharp_bp_check ) ) {
            harmful_stuff.emplace_back( m.name( dest_loc ) );
        }

    }

    if( !harmful_stuff.empty() &&
        !query_yn( _( "Really step into %s?" ), enumerate_as_string( harmful_stuff ) ) ) {
        return false;
    }
    if( !harmful_stuff.empty() && u.is_mounted() &&
        m.tr_at( dest_loc ).loadid == tr_ledge ) {
        add_msg( m_warning, _( "Your %s refuses to move over that ledge!" ),
                 u.mounted_creature->get_name() );
        return false;
    }
    return true;
}

bool game::walk_move( const tripoint &dest_loc )
{
    if( u.is_mounted() ) {
        auto mons = u.mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return false;
            }
        }
    }
    const optional_vpart_position vp_here = m.veh_at( u.pos() );
    const optional_vpart_position vp_there = m.veh_at( dest_loc );

    bool pushing = false; // moving -into- grabbed tile; skip check for move_cost > 0
    bool pulling = false; // moving -away- from grabbed tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0

    const tripoint furn_pos = u.pos() + u.grab_point;
    const tripoint furn_dest = dest_loc + u.grab_point;

    bool grabbed = u.get_grab_type() != OBJECT_NONE;
    if( grabbed ) {
        const tripoint dp = dest_loc - u.pos();
        pushing = dp ==  u.grab_point;
        pulling = dp == -u.grab_point;
    }
    if( grabbed && dest_loc.z != u.posz() ) {
        add_msg( m_warning, _( "You let go of the grabbed object." ) );
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
        if( vp_there && u.mounted_creature && u.mounted_creature->has_flag( MF_RIDEABLE_MECH ) &&
            vp_there->vehicle().handle_potential_theft( dynamic_cast<player &>( u ) ) ) {
            tripoint diff = dest_loc - u.pos();
            if( diff.x < 0 ) {
                diff.x -= 2;
            } else if( diff.x > 0 ) {
                diff.x += 2;
            }
            if( diff.y < 0 ) {
                diff.y -= 2;
            } else if( diff.y > 0 ) {
                diff.y += 2;
            }
            u.mounted_creature->shove_vehicle( dest_loc + diff.xy(),
                                               dest_loc );
        }
        return false;
    }
    if( vp_there && !vp_there->vehicle().handle_potential_theft( dynamic_cast<player &>( u ) ) ) {
        return false;
    }
    if( u.is_mounted() && !pushing && vp_there ) {
        add_msg( m_warning, _( "You cannot board a vehicle whilst riding." ) );
        return false;
    }
    u.set_underwater( false );

    if( !shifting_furniture && !pushing && !prompt_dangerous_tile( dest_loc ) ) {
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
    if( u.is_mounted() ) {
        auto crit = u.mounted_creature.get();
        if( !crit->has_flag( MF_RIDEABLE_MECH ) &&
            ( m.has_flag_ter_or_furn( "MOUNTABLE", dest_loc ) ||
              m.has_flag_ter_or_furn( "BARRICADABLE_DOOR", dest_loc ) ||
              m.has_flag_ter_or_furn( "OPENCLOSE_INSIDE", dest_loc ) ||
              m.has_flag_ter_or_furn( "BARRICADABLE_DOOR_DAMAGED", dest_loc ) ||
              m.has_flag_ter_or_furn( "BARRICADABLE_DOOR_REINFORCED", dest_loc ) ) ) {
            add_msg( m_warning, _( "You cannot pass obstacles whilst mounted." ) );
            return false;
        }
        const double base_moves = u.run_cost( mcost, diag ) * 100.0 / crit->get_speed();
        const double encumb_moves = u.get_weight() / 4800.0_gram;
        u.moves -= static_cast<int>( ceil( base_moves + encumb_moves ) );
        if( u.movement_mode_is( PMM_WALK ) ) {
            crit->use_mech_power( -2 );
        } else if( u.movement_mode_is( PMM_CROUCH ) ) {
            crit->use_mech_power( -1 );
        } else if( u.movement_mode_is( PMM_RUN ) ) {
            crit->use_mech_power( -3 );
        }
    } else {
        u.moves -= u.run_cost( mcost, diag );
        /**
        TODO:
        This should really use the mounted creatures stamina, if mounted.
        Monsters don't currently have stamina however.
        For the time being just don't burn players stamina when mounted.
        */
        u.burn_move_stamina( previous_moves - u.moves );
    }
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
    if( slowed && !u.is_mounted() ) {
        // Unless u.pos() has a higher movecost than dest_loc, state that dest_loc is the cause
        if( mcost_to >= mcost_from ) {
            if( auto displayed_part = vp_there.part_displayed() ) {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ),
                         displayed_part->part().name() );
                sfx::do_obstacle( displayed_part->part().info().get_id().str() );
            } else {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ), m.name( dest_loc ) );
                sfx::do_obstacle( m.ter( dest_loc ).id().str() );
            }
        } else {
            if( auto displayed_part = vp_here.part_displayed() ) {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ),
                         displayed_part->part().name() );
                sfx::do_obstacle( displayed_part->part().info().get_id().str() );
            } else {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ), m.name( u.pos() ) );
                sfx::do_obstacle( m.ter( u.pos() ).id().str() );
            }
        }
    }
    if( !u.is_mounted() && u.has_trait( trait_id( "LEG_TENT_BRACE" ) ) &&
        ( !u.footwear_factor() ||
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
        int volume = u.is_stealthy() ? 3 : 6;
        volume *= u.mutation_value( "noise_modifier" );
        if( volume > 0 ) {
            if( u.is_wearing( "rm13_armor_on" ) ) {
                volume = 2;
            } else if( u.has_bionic( bionic_id( "bio_ankles" ) ) ) {
                volume = 12;
            }
            if( u.movement_mode_is( PMM_RUN ) ) {
                volume *= 1.5;
            } else if( u.movement_mode_is( PMM_CROUCH ) ) {
                volume /= 2;
            }
            if( u.is_mounted() ) {
                auto mons = u.mounted_creature.get();
                switch( mons->get_size() ) {
                    case MS_TINY:
                        volume = 0; // No sound for the tinies
                        break;
                    case MS_SMALL:
                        volume /= 3;
                        break;
                    case MS_MEDIUM:
                        break;
                    case MS_LARGE:
                        volume *= 1.5;
                        break;
                    case MS_HUGE:
                        volume *= 2;
                        break;
                    default:
                        break;
                }
                if( mons->has_flag( MF_LOUDMOVES ) ) {
                    volume += 6;
                }
                sounds::sound( dest_loc, volume, sounds::sound_t::movement, mons->type->get_footsteps(), false,
                               "none", "none" );
            } else {
                sounds::sound( dest_loc, volume, sounds::sound_t::movement, _( "footsteps" ), true,
                               "none", "none" );    // Sound of footsteps may awaken nearby monsters
            }
            sfx::do_footstep();
        }

        if( one_in( 20 ) && u.has_artifact_with( AEP_MOVEMENT_NOISE ) ) {
            sounds::sound( u.pos(), 40, sounds::sound_t::movement, _( "a rattling sound." ), true,
                           "misc", "rattling" );
        }
    }

    if( m.has_flag_ter_or_furn( TFLAG_HIDE_PLACE, dest_loc ) ) {
        add_msg( m_good, _( "You are hiding in the %s." ), m.name( dest_loc ) );
    }

    if( dest_loc != u.pos() ) {
        mtype_id mount_type;
        if( u.is_mounted() ) {
            mount_type = u.mounted_creature->type->id;
        }
        g->events().send<event_type::avatar_moves>( mount_type );
    }

    tripoint oldpos = u.pos();
    point submap_shift = place_player( dest_loc );
    point ms_shift = sm_to_ms_copy( submap_shift );
    oldpos = oldpos - ms_shift;

    if( pulling ) {
        const tripoint shifted_furn_pos = furn_pos - ms_shift;
        const tripoint shifted_furn_dest = furn_dest - ms_shift;
        const time_duration fire_age = m.get_field_age( shifted_furn_pos, fd_fire );
        const int fire_intensity = m.get_field_intensity( shifted_furn_pos, fd_fire );
        m.remove_field( shifted_furn_pos, fd_fire );
        m.set_field_intensity( shifted_furn_dest, fd_fire, fire_intensity );
        m.set_field_age( shifted_furn_dest, fd_fire, fire_age );
    }

    if( u.is_hauling() ) {
        u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
        // Whether the destination is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Destination relative to the player
        u.activity.coords.push_back( tripoint_zero );
        map_stack items = m.i_at( oldpos );
        if( items.empty() ) {
            u.stop_hauling();
        }
        for( item &it : items ) {
            u.activity.targets.emplace_back( map_cursor( oldpos ), &it );
            // Quantity of 0 means move all
            u.activity.values.push_back( 0 );
        }
    }

    on_move_effects();

    return true;
}

point game::place_player( const tripoint &dest_loc )
{
    const optional_vpart_position vp1 = m.veh_at( dest_loc );
    if( const cata::optional<std::string> label = vp1.get_label() ) {
        add_msg( m_info, _( "Label here: %s" ), *label );
    }
    std::string signage = m.get_signage( dest_loc );
    if( !signage.empty() ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "The sign says: %s" ), signage );
        } else {
            add_msg( m_info, _( "There is a sign here, but you are unable to read it." ) );
        }
    }
    if( m.has_graffiti_at( dest_loc ) ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "Written here: %s" ), m.graffiti_at( dest_loc ) );
        } else {
            add_msg( m_info, _( "Something is written here, but you are unable to read it." ) );
        }
    }
    // TODO: Move the stuff below to a Character method so that NPCs can reuse it
    if( m.has_flag( "ROUGH", dest_loc ) && ( !u.in_vehicle ) && ( !u.is_mounted() ) ) {
        if( one_in( 5 ) && u.get_armor_bash( bp_foot_l ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your left foot on the %s!" ),
                     m.has_flag_ter( "ROUGH", dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
        if( one_in( 5 ) && u.get_armor_bash( bp_foot_r ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your right foot on the %s!" ),
                     m.has_flag_ter( "ROUGH", dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 1 ) );
        }
    }
    ///\EFFECT_DEX increases chance of avoiding cuts on sharp terrain
    if( m.has_flag( "SHARP", dest_loc ) && !one_in( 3 ) && !x_in_y( 1 + u.dex_cur / 2.0, 40 ) &&
        ( !u.in_vehicle && !g->m.veh_at( dest_loc ) ) && ( !u.has_trait( trait_PARKOUR ) ||
                one_in( 4 ) ) ) {
        if( u.is_mounted() ) {
            add_msg( _( "Your %s gets cut!" ), u.mounted_creature->get_name() );
            u.mounted_creature->apply_damage( nullptr, bp_torso, rng( 1, 10 ) );
        } else {
            body_part bp = random_body_part();
            if( u.deal_damage( nullptr, bp, damage_instance( DT_CUT, rng( 1, 10 ) ) ).total_damage() > 0 ) {
                //~ 1$s - bodypart name in accusative, 2$s is terrain name.
                add_msg( m_bad, _( "You cut your %1$s on the %2$s!" ),
                         body_part_name_accusative( bp ),
                         m.has_flag_ter( "SHARP", dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                             dest_loc ) );
                if( ( u.has_trait( trait_INFRESIST ) ) && ( one_in( 1024 ) ) ) {
                    u.add_effect( effect_tetanus, 1_turns, num_bp, true );
                } else if( ( !u.has_trait( trait_INFIMMUNE ) || !u.has_trait( trait_INFRESIST ) ) &&
                           ( one_in( 256 ) ) ) {
                    u.add_effect( effect_tetanus, 1_turns, num_bp, true );
                }
            }
        }
    }
    if( m.has_flag( "UNSTABLE", dest_loc ) && !u.is_mounted() ) {
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
        if( u.is_mounted() ) {
            monster *mon = u.mounted_creature.get();
            if( mon->has_effect( effect_onfire ) ) {
                mon->remove_effect( effect_onfire );
            }
        }
    }

    if( monster *const mon_ptr = critter_at<monster>( dest_loc ) ) {
        // We displaced a monster. It's probably a bug if it wasn't a friendly mon...
        // Immobile monsters can't be displaced.
        monster &critter = *mon_ptr;
        // TODO handling for ridden creatures other than players mount.
        if( !critter.has_effect( effect_ridden ) ) {
            if( u.is_mounted() ) {
                std::vector<tripoint> valid;
                for( const tripoint &jk : g->m.points_in_radius( critter.pos(), 1 ) ) {
                    if( is_empty( jk ) ) {
                        valid.push_back( jk );
                    }
                }
                if( !valid.empty() ) {
                    critter.move_to( random_entry( valid ) );
                    add_msg( _( "You push the %s out of the way." ), critter.name() );
                } else {
                    add_msg( _( "There is no room to push the %s out of the way." ), critter.name() );
                    return u.pos().xy();
                }
            } else {
                critter.move_to( u.pos(), true ); // Force the movement even though the player is there right now.
                add_msg( _( "You displace the %s." ), critter.name() );
            }
        } else if( !u.has_effect( effect_riding ) ) {
            add_msg( _( "You cannot move the %s out of the way." ), critter.name() );
            return u.pos().xy();
        }
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
    if( u.is_mounted() ) {
        monster *mon = u.mounted_creature.get();
        mon->setpos( dest_loc );
        mon->process_triggers();
        m.creature_in_field( *mon );
    }
    point submap_shift = update_map( u );
    // Important: don't use dest_loc after this line. `update_map` may have shifted the map
    // and dest_loc was not adjusted and therefore is still in the un-shifted system and probably wrong.
    // If you must use it you can calculate the position in the new, shifted system with
    // adjusted_pos = ( old_pos.x - submap_shift.x * SEEX, old_pos.y - submap_shift.y * SEEY, old_pos.z )

    //Auto pulp or butcher and Auto foraging
    if( get_option<bool>( "AUTO_FEATURES" ) && mostseen == 0  && !u.is_mounted() ) {
        static const direction adjacentDir[8] = { NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST };

        const std::string forage_type = get_option<std::string>( "AUTO_FORAGING" );
        if( forage_type != "off" ) {
            const auto forage = [&]( const tripoint & pos ) {
                const auto &xter_t = m.ter( pos ).obj().examine;
                const auto &xfurn_t = m.furn( pos ).obj().examine;
                const bool forage_everything = forage_type == "both";
                const bool forage_bushes = forage_everything || forage_type == "bushes";
                const bool forage_trees = forage_everything || forage_type == "trees";
                if( xter_t == &iexamine::none ) {
                    return;
                } else if( ( forage_bushes && xter_t == &iexamine::shrub_marloss ) ||
                           ( forage_bushes && xter_t == &iexamine::shrub_wildveggies ) ||
                           ( forage_bushes && xter_t == &iexamine::harvest_ter_nectar ) ||
                           ( forage_trees && xter_t == &iexamine::tree_marloss ) ||
                           ( forage_trees && xter_t == &iexamine::harvest_ter ) ||
                           ( forage_trees && xter_t == &iexamine::harvest_ter_nectar )
                         ) {
                    xter_t( u, pos );
                } else if( ( forage_everything && xfurn_t == &iexamine::harvest_furn ) ||
                           ( forage_everything && xfurn_t == &iexamine::harvest_furn_nectar )
                         ) {
                    xfurn_t( u, pos );
                }
            };

            for( auto &elem : adjacentDir ) {
                forage( u.pos() + direction_XY( elem ) );
            }
        }

        const std::string pulp_butcher = get_option<std::string>( "AUTO_PULP_BUTCHER" );
        if( pulp_butcher == "butcher" && u.max_quality( quality_id( "BUTCHER" ) ) > INT_MIN ) {
            std::vector<item *> corpses;

            for( item &it : m.i_at( u.pos() ) ) {
                corpses.push_back( &it );
            }

            if( !corpses.empty() ) {
                u.assign_activity( activity_id( "ACT_BUTCHER" ), 0, true );
                for( item *it : corpses ) {
                    u.activity.targets.emplace_back( map_cursor( u.pos() ), it );
                }
            }
        } else if( pulp_butcher == "pulp" || pulp_butcher == "pulp_adjacent" ) {
            const auto pulp = [&]( const tripoint & pos ) {
                for( const auto &maybe_corpse : m.i_at( pos ) ) {
                    if( maybe_corpse.is_corpse() && maybe_corpse.can_revive() &&
                        !maybe_corpse.get_mtype()->bloodType().obj().has_acid ) {
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
    if( !u.is_mounted() && get_option<bool>( "AUTO_PICKUP" ) && !u.is_hauling() &&
        ( !get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) || mostseen == 0 ) &&
        ( m.has_items( u.pos() ) || get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
        Pickup::pick_up( u.pos(), -1 );
    }

    // If the new tile is a boardable part, board it
    if( vp1.part_with_feature( "BOARDABLE", true ) && !u.is_mounted() ) {
        m.board_vehicle( u.pos(), &u );
    }

    // Traps!
    // Try to detect.
    u.search_surroundings();
    if( u.is_mounted() ) {
        m.creature_on_trap( *u.mounted_creature );
    } else {
        m.creature_on_trap( u );
    }
    // Drench the player if swimmable
    if( m.has_flag( "SWIMMABLE", u.pos() ) && !u.is_mounted() ) {
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
                    names[i] = string_format( fmt, counts[i], names[i] );
                    // Skip the first two.
                    if( i > 1 ) {
                        and_the_rest += counts[i];
                    }
                }
                if( names.size() == 1 ) {
                    add_msg( _( "You see here %s." ), names[0] );
                } else if( names.size() == 2 ) {
                    add_msg( _( "You see here %s and %s." ), names[0], names[1] );
                } else if( names.size() == 3 ) {
                    add_msg( _( "You see here %s, %s, and %s." ), names[0], names[1], names[2] );
                } else if( and_the_rest < 7 ) {
                    add_msg( ngettext( "You see here %s, %s and %d more item.",
                                       "You see here %s, %s and %d more items.",
                                       and_the_rest ),
                             names[0], names[1], and_the_rest );
                } else {
                    add_msg( _( "You see here %s and many more items." ), names[0] );
                }
            }
        }
    }

    if( vp1.part_with_feature( "CONTROLS", true ) && u.in_vehicle && !u.is_mounted() ) {
        add_msg( _( "There are vehicle controls here." ) );
        add_msg( m_info, _( "%s to drive." ), press_x( ACTION_CONTROL_VEHICLE ) );
    } else if( vp1.part_with_feature( "CONTROLS", true ) && u.in_vehicle &&
               u.is_mounted() ) {
        add_msg( _( "There are vehicle controls here but you cannot reach them whilst mounted." ) );
    }
    return submap_shift;
}

void game::place_player_overmap( const tripoint &om_dest )
{
    // if player is teleporting around, they dont bring their horse with them
    if( u.is_mounted() ) {
        u.remove_effect( effect_riding );
        u.mounted_creature->remove_effect( effect_ridden );
        u.mounted_creature = nullptr;
    }
    // offload the active npcs.
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
    const tripoint map_om_pos( tripoint( 2 * om_dest.x, 2 * om_dest.y,
                                         om_dest.z ) + point( -HALF_MAPSIZE, -HALF_MAPSIZE ) );
    const tripoint player_pos( u.pos().xy(), map_om_pos.z );
    load_map( map_om_pos );
    load_npcs();
    m.spawn_monsters( true ); // Static monsters
    update_overmap_seen();
    // update weather now as it could be different on the new location
    weather.nextweather = calendar::turn;
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

    const bool only_liquid_items = std::all_of( m.i_at( fdest ).begin(), m.i_at( fdest ).end(),
    [&]( item & liquid_item ) {
        return liquid_item.made_of_from_type( LIQUID );
    } );

    const bool dst_item_ok = !m.has_flag( "NOITEM", fdest ) &&
                             !m.has_flag( "SWIMMABLE", fdest ) &&
                             !m.has_flag( "DESTROY_ITEM", fdest ) &&
                             only_liquid_items;
    const bool src_item_ok = m.furn( fpos ).obj().has_flag( "CONTAINER" ) ||
                             m.furn( fpos ).obj().has_flag( "FIRE_CONTAINER" ) ||
                             m.furn( fpos ).obj().has_flag( "SEALED" );

    const int fire_intensity = m.get_field_intensity( fpos, fd_fire );
    time_duration fire_age = m.get_field_age( fpos, fd_fire );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( auto &contained_item : m.i_at( fpos ) ) {
        furniture_contents_weight += contained_item.weight();
    }
    str_req += furniture_contents_weight / 4_kilogram;
    if( canmove ) {
        u.increase_activity_level( ACTIVE_EXERCISE );
    }
    if( !canmove ) {
        // TODO: What is something?
        add_msg( _( "The %s collides with something." ), furntype.name() );
        u.moves -= 50;
        return true;
        ///\EFFECT_STR determines ability to drag furniture
    } else if( str_req > u.get_str() &&
               one_in( std::max( 20 - str_req - u.get_str(), 2 ) ) ) {
        add_msg( m_bad, _( "You strain yourself trying to move the heavy %s!" ),
                 furntype.name() );
        u.moves -= 100;
        u.mod_pain( 1 ); // Hurt ourselves.
        return true; // furniture and or obstacle wins.
    } else if( !src_item_ok && !dst_item_ok && dst_items > 0 ) {
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
                     furntype.name() );
            return true;
        }
        u.moves -= move_penalty;
        if( move_penalty > 500 ) {
            add_msg( _( "Moving the heavy %s is taking a lot of time!" ),
                     furntype.name() );
        } else if( move_penalty > 200 ) {
            if( one_in( 3 ) ) { // Nag only occasionally.
                add_msg( _( "It takes some time to move the heavy %s." ),
                         furntype.name() );
            }
        }
    }
    sounds::sound( fdest, furntype.move_str_req * 2, sounds::sound_t::movement,
                   _( "a scraping noise." ), true, "misc", "scraping" );

    // Actually move the furniture.
    m.furn_set( fdest, m.furn( fpos ) );
    m.furn_set( fpos, f_null );

    if( fire_intensity == 1 && !pulling_furniture ) {
        m.remove_field( fpos, fd_fire );
        m.set_field_intensity( fdest, fd_fire, fire_intensity );
        m.set_field_age( fdest, fd_fire, fire_age );
    }

    // Is there is only liquids on the ground, remove them after moving furniture.
    if( dst_items > 0 && only_liquid_items ) {
        m.i_clear( fdest );
    }

    if( src_items > 0 ) { // Move the stuff inside.
        if( dst_item_ok && src_item_ok ) {
            // Assume contents of both cells are legal, so we can just swap contents.
            std::list<item> temp;
            std::move( m.i_at( fpos ).begin(), m.i_at( fpos ).end(),
                       std::back_inserter( temp ) );
            m.i_clear( fpos );
            for( auto item_iter = m.i_at( fdest ).begin();
                 item_iter != m.i_at( fdest ).end(); ++item_iter ) {
                m.i_at( fpos ).insert( *item_iter );
            }
            m.i_clear( fdest );
            for( auto &cur_item : temp ) {
                m.i_at( fdest ).insert( cur_item );
            }
        } else {
            add_msg( _( "Stuff spills from the %s!" ), furntype.name() );
        }
    }

    if( shifting_furniture ) {
        // We didn't move
        tripoint d_sum = u.grab_point + dp;
        if( abs( d_sum.x ) < 2 && abs( d_sum.y ) < 2 ) {
            u.grab_point = d_sum; // furniture moved relative to us
        } else { // we pushed furniture out of reach
            add_msg( _( "You let go of the %s." ), furntype.name() );
            u.grab( OBJECT_NONE );
        }
        return true; // We moved furniture but stayed still.
    }

    if( pushing_furniture && m.impassable( fpos ) ) {
        // Not sure how that chair got into a wall, but don't let player follow.
        add_msg( _( "You let go of the %1$s as it slides past %2$s." ),
                 furntype.name(), m.tername( fdest ) );
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
    if( !u.is_mounted() ) {
        const item muscle( "muscle" );
        if( one_in( 8 ) ) {// active power gen
            if( u.has_active_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
                u.charge_power( 1 );
            }
            for( const bionic_id &bid : u.get_bionic_fueled_with( muscle ) ) {
                if( u.has_active_bionic( bid ) ) {
                    u.charge_power( muscle.fuel_energy() * bid->fuel_efficiency );
                }
            }
        }
        if( one_in( 160 ) ) {//  passive power gen
            if( u.has_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
                u.charge_power( 1 );
            }
            for( const bionic_id &bid : u.get_bionic_fueled_with( muscle ) ) {
                if( u.has_bionic( bid ) ) {
                    u.charge_power( muscle.fuel_energy() * bid->fuel_efficiency );
                }
            }
        }
        if( u.has_active_bionic( bionic_id( "bio_jointservo" ) ) ) {
            if( u.movement_mode_is( PMM_RUN ) ) {
                u.charge_power( -20 );
            } else {
                u.charge_power( -10 );
            }
        }
    }

    if( u.movement_mode_is( PMM_RUN ) ) {
        if( u.stamina <= 0 ) {
            u.toggle_run_mode();
        }
        if( u.stamina < u.get_stamina_max() / 5 && one_in( u.stamina ) ) {
            u.add_effect( effect_winded, 10_turns );
        }
    }

    // apply martial art move bonuses
    u.ma_onmove_effects();

    sfx::do_ambient();
}

void game::on_options_changed()
{
#if defined(TILES)
    tilecontext->on_options_changed();
#endif
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

static cata::optional<tripoint> point_selection_menu( const std::vector<tripoint> &pts )
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
        const std::string &direction = direction_name( direction_from( upos.xy(), pt.xy() ) );
        // TODO: Inform player what is on said tile
        // But don't just print terrain name (in many cases it will be "open air")
        pmenu.addentry( num++, true, MENU_AUTOASSIGN, _( "Climb %s" ), direction );
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
    if( u.is_mounted() ) {
        auto mons = u.mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return;
            }
        }
    }
    // Check if there are monsters are using the stairs.
    bool slippedpast = false;
    if( !m.has_zlevels() && !coming_to_stairs.empty() && !force ) {
        // TODO: Allow travel if zombie couldn't reach stairs, but spawn him when we go up.
        add_msg( m_warning, _( "You try to use the stairs. Suddenly you are blocked by a %s!" ),
                 coming_to_stairs[0].name() );
        // Roll.
        ///\EFFECT_DEX increases chance of moving past monsters on stairs

        ///\EFFECT_DODGE increases chance of moving past monsters on stairs
        int dexroll = dice( 6, u.dex_cur + u.get_skill_level( skill_dodge ) * 2 );
        ///\EFFECT_STR increases chance of moving past monsters on stairs

        ///\EFFECT_MELEE increases chance of moving past monsters on stairs
        int strroll = dice( 3, u.str_cur + u.get_skill_level( skill_melee ) * 1.5 );
        if( coming_to_stairs.size() > 4 ) {
            add_msg( _( "The are a lot of them on the %s!" ), m.tername( u.pos() ) );
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
        tmp_map_ptr = std::make_unique<map>();
    }

    map &maybetmp = m.has_zlevels() ? m : *( tmp_map_ptr.get() );
    if( m.has_zlevels() ) {
        // We no longer need to shift the map here! What joy
    } else {
        maybetmp.load( tripoint( get_levx(), get_levy(), z_after ), false );
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
    std::shared_ptr<monster> stored_mount;
    if( u.is_mounted() && !m.has_zlevels() ) {
        // Store a *copy* of the mount, so we can remove the original monster instance
        // from the tracker before the map shifts.
        // Map shifting would otherwise just despawn the mount and would later respawn it.
        stored_mount = std::make_shared<monster>( *u.mounted_creature );
        critter_tracker->remove( *u.mounted_creature );
    }
    if( !m.has_zlevels() ) {
        const tripoint to = u.pos();
        for( monster &critter : all_monsters() ) {
            if( critter.has_effect( effect_ridden ) ) {
                continue;
            }
            int turns = critter.turns_to_reach( to.xy() );
            if( turns < 10 && coming_to_stairs.size() < 8 && critter.will_reach( to.xy() )
                && !slippedpast ) {
                critter.staircount = 10 + turns;
                critter.on_unload();
                coming_to_stairs.push_back( critter );
                remove_zombie( critter );
            }
        }
        auto mons = critter_tracker->find( g->u.pos() );
        if( mons != nullptr ) {
            critter_tracker->remove( *mons );
        }
        shift_monsters( tripoint( 0, 0, movez ) );
    }

    std::vector<std::shared_ptr<npc>> npcs_to_bring;
    std::vector<monster *> monsters_following;
    if( !m.has_zlevels() && abs( movez ) == 1 ) {
        std::copy_if( active_npc.begin(), active_npc.end(), back_inserter( npcs_to_bring ),
        [this]( const std::shared_ptr<npc> &np ) {
            return np->is_walking_with() && !np->is_mounted() && rl_dist( np->pos(), u.pos() ) < 2;
        } );
    }

    if( m.has_zlevels() && abs( movez ) == 1 ) {
        for( monster &critter : all_monsters() ) {
            if( critter.attack_target() == &g->u || ( !critter.has_effect( effect_ridden ) &&
                    critter.has_effect( effect_pet ) && critter.friendly == -1 ) ) {
                monsters_following.push_back( &critter );
            }
        }
    }

    if( u.is_mounted() ) {
        monster *crit = u.mounted_creature.get();
        if( crit->has_flag( MF_RIDEABLE_MECH ) ) {
            crit->use_mech_power( -1 );
            if( u.movement_mode_is( PMM_WALK ) ) {
                crit->use_mech_power( -2 );
            } else if( u.movement_mode_is( PMM_CROUCH ) ) {
                crit->use_mech_power( -1 );
            } else if( u.movement_mode_is( PMM_RUN ) ) {
                crit->use_mech_power( -3 );
            }
        }
    } else {
        u.moves -= move_cost;
    }
    const tripoint old_pos = g->u.pos();
    point submap_shift;
    vertical_shift( z_after );
    if( !force ) {
        submap_shift = update_map( stairs.x, stairs.y );
    }
    if( u.is_mounted() ) {
        if( stored_mount ) {
            assert( !m.has_zlevels() );
            stored_mount->spawn( g->u.pos() );
            if( critter_tracker->add( stored_mount ) ) {
                u.mounted_creature = stored_mount;
            }
        } else {
            u.mounted_creature->setpos( g->u.pos() );
        }
    }

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
                // TODO: De-uglify
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
    // TODO: Remove stair teleport bullshit
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
        const tripoint adjusted_pos = old_pos - sm_to_ms_copy( submap_shift );
        u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
        // Whether the destination is inside a vehicle (not supported)
        u.activity.values.push_back( 0 );
        // Destination relative to the player
        u.activity.coords.push_back( tripoint_zero );
        map_stack items = m.i_at( adjusted_pos );
        if( items.empty() ) {
            u.stop_hauling();
        }
        for( item &it : items ) {
            u.activity.targets.emplace_back( map_cursor( adjusted_pos ), &it );
            // Quantitu of 0 means move all
            u.activity.values.push_back( 0 );
        }
    }

    m.invalidate_map_cache( g->get_levz() );
    refresh_all();
    // Upon force movement, traps can not be avoided.
    m.creature_on_trap( u, !force );
}

cata::optional<tripoint> game::find_or_make_stairs( map &mp, const int z_after, bool &rope_ladder )
{
    const int omtilesz = SEEX * 2;
    real_coords rc( m.getabs( point( u.posx(), u.posy() ) ) );
    tripoint omtile_align_start( m.getlocal( rc.begin_om_pos() ), z_after );
    tripoint omtile_align_end( omtile_align_start + point( -1 + omtilesz, -1 + omtilesz ) );

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
        add_msg( _( "There's a %s in the way!" ), blocking_creature->disp_name() );
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
                    u.mod_stored_nutr( 10 );
                    u.mod_thirst( 10 );
                } else {
                    add_msg( _( "You gingerly descend using your vines." ) );
                }
            } else {
                add_msg( _( "You effortlessly lower yourself and leave a vine rooted for future use." ) );
                rope_ladder = true;
                u.mod_stored_nutr( 10 );
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
        m.load( tripoint( get_levx(), get_levy(), z_after ), true );
        shift_monsters( tripoint( 0, 0, z_after - z_before ) );
        reload_npcs();
    } else {
        // Shift the map itself
        m.vertical_shift( z_after );
    }

    m.spawn_monsters( true );
    // this may be required after a vertical shift if z-levels are not enabled
    // the critter is unloaded/loaded, and it needs to reconstruct its rider data after being reloaded.
    validate_mounted_npcs();
    vertical_notes( z_before, z_after );
}

void game::vertical_notes( int z_before, int z_after )
{
    if( z_before == z_after || !get_option<bool>( "AUTO_NOTES" ) ||
        !get_option<bool>( "AUTO_NOTES_STAIRS" ) ) {
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
    for( const tripoint &p : points_in_radius( u.global_omt_location(), REVEAL_RADIUS ) ) {
        const tripoint cursp_before( p.xy(), z_before );
        const tripoint cursp_after( p.xy(), z_after );

        if( !overmap_buffer.seen( cursp_before ) ) {
            continue;
        }
        if( overmap_buffer.has_note( cursp_after ) ) {
            // Already has a note -> never add an AUTO-note
            continue;
        }
        const oter_id &ter = overmap_buffer.ter( cursp_before );
        const oter_id &ter2 = overmap_buffer.ter( cursp_after );
        if( z_after > z_before && ter->has_flag( known_up ) &&
            !ter2->has_flag( known_down ) ) {
            overmap_buffer.set_seen( cursp_after, true );
            overmap_buffer.add_note( cursp_after, string_format( ">:W;%s", _( "AUTO: goes down" ) ) );
        } else if( z_after < z_before && ter->has_flag( known_down ) &&
                   !ter2->has_flag( known_up ) ) {
            overmap_buffer.set_seen( cursp_after, true );
            overmap_buffer.add_note( cursp_after, string_format( "<:W;%s", _( "AUTO: goes up" ) ) );
        }
    }
}

point game::update_map( player &p )
{
    int x = p.posx();
    int y = p.posy();
    return update_map( x, y );
}

point game::update_map( int &x, int &y )
{
    point shift;

    while( x < HALF_MAPSIZE_X ) {
        x += SEEX;
        shift.x--;
    }
    while( x >= HALF_MAPSIZE_X + SEEX ) {
        x -= SEEX;
        shift.x++;
    }
    while( y < HALF_MAPSIZE_Y ) {
        y += SEEY;
        shift.y--;
    }
    while( y >= HALF_MAPSIZE_Y + SEEY ) {
        y -= SEEY;
        shift.y++;
    }

    if( shift == point_zero ) {
        // adjust player position
        u.setpos( tripoint( x, y, get_levz() ) );
        // Not actually shifting the submaps, all the stuff below would do nothing
        return point_zero;
    }

    // this handles loading/unloading submaps that have scrolled on or off the viewport
    m.shift( shift );

    // Shift monsters
    shift_monsters( tripoint( shift, 0 ) );
    const point shift_ms = sm_to_ms_copy( shift );
    u.shift_destination( -shift_ms );

    // Shift NPCs
    for( auto it = active_npc.begin(); it != active_npc.end(); ) {
        ( *it )->shift( shift.x, shift.y );
        if( ( *it )->posx() < 0 - SEEX * 2 || ( *it )->posy() < 0 - SEEX * 2 ||
            ( *it )->posx() > SEEX * ( MAPSIZE + 2 ) || ( *it )->posy() > SEEY * ( MAPSIZE + 2 ) ) {
            //Remove the npc from the active list. It remains in the overmap list.
            ( *it )->on_unload();
            it = active_npc.erase( it );
        } else {
            it++;
        }
    }

    scent.shift( shift_ms.x, shift_ms.y );

    // Also ensure the player is on current z-level
    // get_levz() should later be removed, when there is no longer such a thing
    // as "current z-level"
    u.setpos( tripoint( x, y, get_levz() ) );

    // Only do the loading after all coordinates have been shifted.

    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();

    // Make sure map cache is consistent since it may have shifted.
    if( m.has_zlevels() ) {
        for( int zlev = -OVERMAP_DEPTH; zlev <= OVERMAP_HEIGHT; ++zlev ) {
            m.invalidate_map_cache( zlev );
        }
    } else {
        m.invalidate_map_cache( get_levz() );
    }
    m.build_map_cache( get_levz() );

    // Spawn monsters if appropriate
    // This call will generate new monsters in addition to loading, so it's placed after NPC loading
    m.spawn_monsters( false ); // Static monsters

    // Update what parts of the world map we can see
    update_overmap_seen();

    return shift;
}

void game::update_overmap_seen()
{
    const tripoint ompos = u.global_omt_location();
    const int dist = u.overmap_sight_range( light_level( u.posz() ) );
    const int dist_squared = dist * dist;
    // We can always see where we're standing
    overmap_buffer.set_seen( ompos, true );
    for( const tripoint &p : points_in_radius( ompos, dist ) ) {
        const point delta = p.xy() - ompos.xy();
        const int h_squared = delta.x * delta.x + delta.y * delta.y;
        if( trigdist && h_squared > dist_squared ) {
            continue;
        }
        // If circular distances are enabled, scale overmap distances by the diagonality of the sight line.
        const float multiplier = trigdist ? std::sqrt( h_squared ) / std::max( std::abs( delta.x ),
                                 std::abs( delta.y ) ) : 1;
        const std::vector<tripoint> line = line_to( ompos, p, 0 );
        float sight_points = dist;
        for( auto it = line.begin();
             it != line.end() && sight_points >= 0; ++it ) {
            const oter_id &ter = overmap_buffer.ter( *it );
            sight_points -= static_cast<int>( ter->get_see_cost() ) * multiplier;
        }
        if( sight_points >= 0 ) {
            tripoint seen( p );
            do {
                overmap_buffer.set_seen( seen, true );
                --seen.z;
            } while( seen.z >= 0 );
        }
    }
}

void game::replace_stair_monsters()
{
    for( auto &elem : coming_to_stairs ) {
        elem.staircount = 0;
        const tripoint pnt( elem.pos().xy(), get_levz() );
        place_critter_around( std::make_shared<monster>( elem ), pnt, 10 );
    }

    coming_to_stairs.clear();
}

// TODO: abstract out the location checking code
// TODO: refactor so zombies can follow up and down stairs instead of this mess
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

    for( const tripoint &dest : m.points_on_zlevel( u.posz() ) ) {
        if( ( from_below && m.has_flag( "GOES_DOWN", dest ) ) ||
            ( !from_below && m.has_flag( "GOES_UP", dest ) ) ) {
            stairx.push_back( dest.x );
            stairy.push_back( dest.y );
            stairdist.push_back( rl_dist( dest, u.pos() ) );
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
                dump << string_format( _( "You see a %s on the stairs" ), critter.name() );
            } else {
                if( critter.staircount > 0 ) {
                    dump << ( from_below ?
                              //~ The <monster> is almost at the <bottom/top> of the <terrain type>!
                              string_format( _( "The %1$s is almost at the top of the %2$s!" ),
                                             critter.name(),
                                             m.tername( dest ) ) :
                              string_format( _( "The %1$s is almost at the bottom of the %2$s!" ),
                                             critter.name(),
                                             m.tername( dest ) ) );
                }
            }

            add_msg( m_warning, dump.str() );
        } else {
            sounds::sound( dest, 5, sounds::sound_t::movement,
                           _( "a sound nearby from the stairs!" ), true, "misc", "stairs_movement" );
        }

        if( critter.staircount > 0 ) {
            continue;
        }

        if( is_empty( dest ) ) {
            critter.spawn( dest );
            critter.staircount = 0;
            place_critter_at( std::make_shared<monster>( critter ), dest );
            if( u.sees( dest ) ) {
                if( !from_below ) {
                    add_msg( m_warning, _( "The %1$s comes down the %2$s!" ),
                             critter.name(),
                             m.tername( dest ) );
                } else {
                    add_msg( m_warning, _( "The %1$s comes up the %2$s!" ),
                             critter.name(),
                             m.tername( dest ) );
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
                                 critter.name() );
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
                    add_msg( m_warning, msg.c_str(), critter.name() );
                    u.setx( u.posx() + pushx );
                    u.sety( u.posy() + pushy );
                    return;
                }
            }
            add_msg( m_warning,
                     _( "The %s tried to push you back but failed! It attacks you!" ),
                     critter.name() );
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
                    add_msg( m_neutral, msg, critter.name(), other.name() );
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
    // simulate it being dead so further processing of it (e.g. in monmove) will yield
    critter.set_hp( 0 );
}

void game::shift_monsters( const tripoint &shift )
{
    // If either shift argument is non-zero, we're shifting.
    if( shift == tripoint_zero ) {
        return;
    }
    for( monster &critter : all_monsters() ) {
        if( shift.xy() != point_zero ) {
            critter.shift( shift.xy() );
        }

        if( m.inbounds( critter.pos() ) && ( shift.z == 0 || m.has_zlevels() ) ) {
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
    critter_tracker->rebuild_cache();
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
    // TODO: This is inaccurate when the player is near a overmap border, and it will
    //immediately spawn new npcs upon entering a new overmap. Rather use number of npcs *nearby*.
    const int npc_num = get_cur_om().get_npcs().size();
    if( npc_num > 0 ) {
        // 100%, 80%, 64%, 52%, 41%, 33%...
        density *= powf( 0.8f, npc_num );
    }

    if( !x_in_y( density, 100 ) ) {
        return;
    }

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
    tripoint omt_pos = sm_to_omt_copy( tripoint( msx, msy, 0 ) );
    const auto oter = overmap_buffer.ter( omt_pos );
    // shouldnt spawn on lakes or rivers.
    if( is_river_or_lake( oter ) ) {
        return;
    }
    std::shared_ptr<npc> tmp = std::make_shared<npc>();
    tmp->normalize();
    tmp->randomize();
    std::string new_fac_id = "solo_";
    new_fac_id += tmp->name;
    // create a new "lone wolf" faction for this one NPC
    faction *new_solo_fac = faction_manager_ptr->add_new_faction( tmp->name, faction_id( new_fac_id ),
                            faction_id( "no_faction" ) );
    tmp->set_fac( new_solo_fac ? new_solo_fac->id : faction_id( "no_faction" ) );
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

void game::display_scent()
{
    if( use_tiles ) {
        displaying_temperature = false;
        displaying_visibility = false;
        displaying_radiation = false;
        displaying_scent = !displaying_scent;
    } else {
        int div;
        bool got_value = query_int( div, _( "Set the Scent Map sensitivity to (0 to cancel)?" ) );
        if( !got_value || div < 1 ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        draw_ter();
        scent.draw( w_terrain, div * 2, u.pos() + u.view_offset );
        wrefresh( w_terrain );
        draw_panels();
        inp_mngr.wait_for_any_key();
    }
}

void game::display_temperature()
{
    if( use_tiles ) {
        displaying_scent = false;
        displaying_visibility = false;
        displaying_radiation = false;
        displaying_temperature = !displaying_temperature;
    }
}

void game::display_visibility()
{
    if( use_tiles ) {
        displaying_scent = false;
        displaying_temperature = false;
        displaying_radiation = false;
        displaying_visibility = !displaying_visibility;
    }
}

void game::display_radiation()
{
    if( use_tiles ) {
        displaying_scent = false;
        displaying_visibility = false;
        displaying_temperature = false;
        displaying_radiation = !displaying_radiation;
    }
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
        popup_getkey( _( "No saves for %s yet." ), u.name );
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
    catacurses::window tmp = catacurses::newwin( minHeight, minWidth, point_zero );

    while( maxy < minHeight || maxx < minWidth ) {
        werase( tmp );
        if( maxy < minHeight && maxx < minWidth ) {
            fold_and_print( tmp, point_zero, maxx, c_white,
                            _( "Whoa! Your terminal is tiny! This game requires a minimum terminal size of "
                               "%dx%d to work properly. %dx%d just won't do. Maybe a smaller font would help?" ),
                            minWidth, minHeight, maxx, maxy );
        } else if( maxx < minWidth ) {
            fold_and_print( tmp, point_zero, maxx, c_white,
                            _( "Oh! Hey, look at that. Your terminal is just a little too narrow. This game "
                               "requires a minimum terminal size of %dx%d to function. It just won't work "
                               "with only %dx%d. Can you stretch it out sideways a bit?" ),
                            minWidth, minHeight, maxx, maxy );
        } else {
            fold_and_print( tmp, point_zero, maxx, c_white,
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

#if !(defined(_WIN32) || defined(TILES))
    // Check whether LC_CTYPE supports the UTF-8 encoding
    // and show a warning if it doesn't
    if( std::strcmp( nl_langinfo( CODESET ), "UTF-8" ) != 0 ) {
        const char *unicode_error_msg =
            _( "You don't seem to have a valid Unicode locale. You may see some weird "
               "characters (e.g. empty boxes or question marks). You have been warned." );
        fold_and_print( tmp, point_zero, maxx, c_white, unicode_error_msg, minWidth, minHeight,
                        maxx, maxy );
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
        const std::vector<art_effect_passive> &ew = it.type->artifact->effects_worn;
        effects.insert( effects.end(), ew.begin(), ew.end() );
    }
    if( wielded ) {
        const std::vector<art_effect_passive> &ew = it.type->artifact->effects_wielded;
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
                            u.mod_stat( "stamina", -90 * rng( 1, 3 ) * rng( 1, 3 ) * rng( 2, 3 ) );
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

    for( const art_effect_passive &i : effects ) {
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
                    m.mod_field_age( dest, fd_fire, -1_turns );
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
                                     it.tname() );
                        }
                    } else if( !wielded ) {
                        add_msg( _( "You have an urge to wield the %s." ),
                                 it.tname() );
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
    const bool scen_season = scen->has_flag( "SPR_START" ) || scen->has_flag( "SUM_START" ) ||
                             scen->has_flag( "AUT_START" ) || scen->has_flag( "WIN_START" ) ||
                             scen->has_flag( "SUM_ADV_START" );

    if( scen_season ) {
        // Configured starting date overridden by scenario, calendar::start is left as Spring 1
        calendar::start_of_cataclysm = calendar::turn_zero + 1_hours * get_option<int>( "INITIAL_TIME" );
        calendar::turn = calendar::turn_zero + 1_hours * get_option<int>( "INITIAL_TIME" );
        if( scen->has_flag( "SPR_START" ) ) {
            calendar::initial_season = SPRING;
        } else if( scen->has_flag( "SUM_START" ) ) {
            calendar::initial_season = SUMMER;
            calendar::turn += calendar::season_length();
        } else if( scen->has_flag( "AUT_START" ) ) {
            calendar::initial_season = AUTUMN;
            calendar::turn += calendar::season_length() * 2;
        } else if( scen->has_flag( "WIN_START" ) ) {
            calendar::initial_season = WINTER;
            calendar::turn += calendar::season_length() * 3;
        } else if( scen->has_flag( "SUM_ADV_START" ) ) {
            calendar::initial_season = SUMMER;
            calendar::turn += calendar::season_length() * 5;
        } else {
            debugmsg( "The Unicorn" );
        }
    } else {
        // No scenario, so use the starting date+time configured in world options
        const int initial_days = get_option<int>( "INITIAL_DAY" );
        calendar::start_of_cataclysm = calendar::turn_zero + 1_days * initial_days;

        // Determine the season based off how long the seasons are set to be
        // First mod by length of season to get number of seasons elapsed, then mod by 4 to force a 0-3 range of values
        const int season_number = ( initial_days % get_option<int>( "SEASON_LENGTH" ) ) % 4;
        if( season_number == 0 ) {
            calendar::initial_season = SPRING;
        } else if( season_number == 1 ) {
            calendar::initial_season = SUMMER;
        } else if( season_number == 2 ) {
            calendar::initial_season = AUTUMN;
        } else {
            calendar::initial_season = WINTER;
        }

        calendar::turn = calendar::start_of_cataclysm
                         + 1_hours * get_option<int>( "INITIAL_TIME" )
                         + 1_days * get_option<int>( "SPAWN_DELAY" );
    }

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
        add_msg( m_neutral, stat_info );
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
        if( art.has_value() && art->charge_req != ACR_NULL &&
            ( it->ammo_remaining() < it->ammo_capacity() ||
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
        const std::string &dream = string_format( _( msg ), it->tname() );
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
    return overmap_buffer.get( pos_om.xy() );
}

std::vector<npc *> game::allies()
{
    return get_npcs_if( [&]( const npc & guy ) {
        if( !guy.is_hallucination() ) {
            return guy.is_ally( g->u );
        } else {
            return false;
        }
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
    const Creature *const critter = current.get();
    if( critter->is_monster() ) {
        return !static_cast<const monster *>( critter )->is_dead();
    }
    if( critter->is_npc() ) {
        return !static_cast<const npc *>( critter )->is_dead();
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
    if( world_generator->active_world == nullptr ) {
        return FILENAMES["savedir"];
    }
    return world_generator->active_world->folder_path();
}
