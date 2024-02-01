#include "game.h"

#include <functional>
#include <algorithm>
#include <bitset>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

#include "achievement.h"
#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_type.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "basecamp.h"
#include "bionics.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "butchery_requirements.h"
#include "cached_options.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "catacharset.h"
#include "character.h"
#include "character_martial_arts.h"
#include "city.h"
#include "climbing.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "computer_session.h"
#include "construction.h"
#include "construction_group.h"
#include "contents_change_handler.h"
#include "coordinate_conversions.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "cursesport.h" // IWYU pragma: keep
#include "damage.h"
#include "debug.h"
#include "dependency_tree.h"
#include "dialogue_chatbin.h"
#include "diary.h"
#include "distraction_manager.h"
#include "editmap.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "filesystem.h"
#include "flag.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "game_ui.h"
#include "gamemode.h"
#include "gates.h"
#include "get_version.h"
#include "harvest.h"
#include "help.h"
#include "iexamine.h"
#include "init.h"
#include "input.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "item_category.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_search.h"
#include "item_stack.h"
#include "iteminfo_query.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "json.h"
#include "kill_tracker.h"
#include "level_cache.h"
#include "lightmap.h"
#include "line.h"
#include "live_view.h"
#include "loading_ui.h"
#include "main_menu.h"
#include "magic.h"
#include "make_static.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "monexamine.h"
#include "monstergenerator.h"
#include "move_mode.h"
#include "mtype.h"
#include "npc.h"
#include "npctrade.h"
#include "npc_class.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "panels.h"
#include "past_games_info.h"
#include "past_achievements_info.h"
#include "path_info.h"
#include "pathfinding.h"
#include "pickup.h"
#include "player_activity.h"
#include "popup.h"
#include "profession.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "rng.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "scent_map.h"
#include "scores_ui.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "sounds.h"
#include "start_location.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "talker.h"
#include "text_snippets.h"
#include "tileray.h"
#include "timed_event.h"
#include "translations.h"
#include "trap.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_appliance.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"
#include "weather_type.h"
#include "worldfactory.h"

#if defined(TILES)
#include "sdl_utils.h"
#endif // TILES

static const activity_id ACT_BLEED( "ACT_BLEED" );
static const activity_id ACT_BUTCHER( "ACT_BUTCHER" );
static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_DISMEMBER( "ACT_DISMEMBER" );
static const activity_id ACT_DISSECT( "ACT_DISSECT" );
static const activity_id ACT_FIELD_DRESS( "ACT_FIELD_DRESS" );
static const activity_id ACT_PULP( "ACT_PULP" );
static const activity_id ACT_QUARTER( "ACT_QUARTER" );
static const activity_id ACT_SKIN( "ACT_SKIN" );
static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_TRAIN_TEACHER( "ACT_TRAIN_TEACHER" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );
static const activity_id ACT_VIEW_RECIPE( "ACT_VIEW_RECIPE" );

static const bionic_id bio_jointservo( "bio_jointservo" );
static const bionic_id bio_probability_travel( "bio_probability_travel" );
static const bionic_id bio_remote( "bio_remote" );

static const character_modifier_id character_modifier_slip_prevent_mod( "slip_prevent_mod" );

static const climbing_aid_id climbing_aid_ability_WALL_CLING( "ability_WALL_CLING" );
static const climbing_aid_id climbing_aid_default( "default" );
static const climbing_aid_id climbing_aid_furn_CLIMBABLE( "furn_CLIMBABLE" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
static const efftype_id effect_asked_to_train( "asked_to_train" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_fake_common_cold( "fake_common_cold" );
static const efftype_id effect_fake_flu( "fake_flu" );
static const efftype_id effect_gliding( "gliding" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_led_by_leash( "led_by_leash" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_psi_stunned( "psi_stunned" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_tetanus( "tetanus" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_took_xanax( "took_xanax" );
static const efftype_id effect_transition_contacts( "transition_contacts" );
static const efftype_id effect_winded( "winded" );

static const faction_id faction_no_faction( "no_faction" );
static const faction_id faction_your_followers( "your_followers" );

static const flag_id json_flag_CONVECTS_TEMPERATURE( "CONVECTS_TEMPERATURE" );
static const flag_id json_flag_LEVITATION( "LEVITATION" );
static const flag_id json_flag_SPLINT( "SPLINT" );

static const furn_str_id furn_f_rope_up( "f_rope_up" );
static const furn_str_id furn_f_web_up( "f_web_up" );

static const harvest_drop_type_id harvest_drop_blood( "blood" );
static const harvest_drop_type_id harvest_drop_offal( "offal" );
static const harvest_drop_type_id harvest_drop_skin( "skin" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_grapnel( "grapnel" );
static const itype_id itype_holybook_bible1( "holybook_bible1" );
static const itype_id itype_holybook_bible2( "holybook_bible2" );
static const itype_id itype_holybook_bible3( "holybook_bible3" );
static const itype_id itype_manhole_cover( "manhole_cover" );
static const itype_id itype_remotevehcontrol( "remotevehcontrol" );
static const itype_id itype_rope_30( "rope_30" );
static const itype_id itype_swim_fins( "swim_fins" );
static const itype_id itype_towel( "towel" );
static const itype_id itype_towel_wet( "towel_wet" );

static const json_character_flag json_flag_CLIMB_NO_LADDER( "CLIMB_NO_LADDER" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_INFECTION_IMMUNE( "INFECTION_IMMUNE" );
static const json_character_flag json_flag_NYCTOPHOBIA( "NYCTOPHOBIA" );
static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );
static const json_character_flag json_flag_WEB_RAPPEL( "WEB_RAPPEL" );

static const material_id material_glass( "glass" );

static const mod_id MOD_INFORMATION_dda( "dda" );

static const mongroup_id GROUP_BLACK_ROAD( "GROUP_BLACK_ROAD" );

static const mtype_id mon_manhack( "mon_manhack" );

static const overmap_special_id overmap_special_world( "world" );

static const proficiency_id proficiency_prof_parkour( "prof_parkour" );
static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static const skill_id skill_dodge( "dodge" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_survival( "survival" );

static const species_id species_PLANT( "PLANT" );

static const string_id<npc_template> npc_template_cyborg_rescued( "cyborg_rescued" );

static const trait_id trait_BADKNEES( "BADKNEES" );
static const trait_id trait_CANNIBAL( "CANNIBAL" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INATTENTIVE( "INATTENTIVE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_M_DEFENDER( "M_DEFENDER" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_NPC_STARTING_NPC( "NPC_STARTING_NPC" );
static const trait_id trait_NPC_STATIC_NPC( "NPC_STATIC_NPC" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );
static const trait_id trait_THICKSKIN( "THICKSKIN" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );
static const trait_id trait_WAYFARER( "WAYFARER" );

static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_NO_AUTO_PICKUP( "NO_AUTO_PICKUP" );

#if defined(TILES)
#include "cata_tiles.h"
#endif // TILES

#if defined(_WIN32)
#if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#   include "platform_win.h"
#endif
#   include <tchar.h>
#endif

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int DANGEROUS_PROXIMITY = 5;

#if defined(__ANDROID__)
extern bool add_key_to_quick_shortcuts( int key, const std::string &category, bool back ); // NOLINT
#endif

//The one and only game instance
std::unique_ptr<game> g;

//The one and only uistate instance
uistatedata uistate;

bool is_valid_in_w_terrain( const point &p )
{
    return p.x >= 0 && p.x < TERRAIN_WINDOW_WIDTH && p.y >= 0 && p.y < TERRAIN_WINDOW_HEIGHT;
}

static void achievement_attained( const achievement *a, bool achievements_enabled )
{
    if( achievements_enabled ) {
        add_msg( m_good, _( "You completed the achievement \"%s\"." ),
                 a->name() );
        std::string popup_option = get_option<std::string>( "ACHIEVEMENT_COMPLETED_POPUP" );
        bool show_popup;
        if( test_mode || popup_option == "never" ) {
            show_popup = false;
        } else if( popup_option == "always" ) {
            show_popup = true;
        } else if( popup_option == "first" ) {
            const achievement_completion_info *past_info = get_past_games().achievement( a->id );
            show_popup = !past_info || past_info->games_completed.empty();
        } else {
            debugmsg( "Unexpected ACHIEVEMENT_COMPLETED_POPUP option value %s", popup_option );
            show_popup = false;
        }

        if( show_popup ) {
            std::string message = colorize( _( "Achievement completed!" ), c_light_green );
            message += "\n\n";
            message += get_achievements().ui_text_for( a );
            message += "\n";
            message += colorize( _( "Achievement completion popups can be\nconfigured via the "
                                    "Interface options" ), c_dark_gray );
            popup( message );
        }
    }
    get_event_bus().send<event_type::player_gets_achievement>( a->id, achievements_enabled );
}

static void achievement_failed( const achievement *a, bool achievements_enabled )
{
    if( !a->is_conduct() ) {
        return;
    }
    if( achievements_enabled ) {
        add_msg( m_bad, _( "You lost the conduct \"%s\"." ), a->name() );
    }
    get_event_bus().send<event_type::player_fails_conduct>( a->id, achievements_enabled );
}

// This is the main game set-up process.
game::game() :
    liveview( *liveview_ptr ),
    scent_ptr( *this ),
    achievements_tracker_ptr( *stats_tracker_ptr, achievement_attained, achievement_failed, true ),
    m( *map_ptr ),
    u( *u_ptr ),
    scent( *scent_ptr ),
    timed_events( *timed_event_manager_ptr ),
    uquit( QUIT_NO ),
    safe_mode( SAFE_MODE_ON ),
    u_shared_ptr( &u, null_deleter{} ),
    next_npc_id( 1 ),
    next_mission_id( 1 ),
    remoteveh_cache_time( calendar::before_time_starts ),
    tileset_zoom( DEFAULT_TILESET_ZOOM ),
    last_mouse_edge_scroll( std::chrono::steady_clock::now() )
{
    first_redraw_since_waiting_started = true;
    reset_light_level();
    events().subscribe( &*stats_tracker_ptr );
    events().subscribe( &*kill_tracker_ptr );
    events().subscribe( &*memorial_logger_ptr );
    events().subscribe( &*achievements_tracker_ptr );
    events().subscribe( &*spell_events_ptr );
    events().subscribe( &*eoc_events_ptr );
    world_generator = std::make_unique<worldfactory>();
    // do nothing, everything that was in here is moved to init_data() which is called immediately after g = new game; in main.cpp
    // The reason for this move is so that g is not uninitialized when it gets to installing the parts into vehicles.
}

game::~game() = default;

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

    // These functions do not load stuff from json.
    // The content they load/initialize is hardcoded into the program.
    // Therefore they can be loaded here.
    // If this changes (if they load data from json), they have to
    // be moved to game::load_mod or game::load_core_data

    get_auto_pickup().load_global();
    get_auto_notes_settings().load( false );
    get_safemode().load_global();
}

bool game::check_mod_data( const std::vector<mod_id> &opts, loading_ui &ui )
{
    dependency_tree &tree = world_generator->get_mod_manager().get_tree();

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
        WORLD *test_world = world_generator->make_new_world( mods_empty );
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

        MAPBUFFER.clear();
        overmap_buffer.clear();
    }

    for( const auto &e : check ) {
        world_generator->set_active_world( nullptr );
        world_generator->init();
        const std::vector<mod_id> mods_empty;
        WORLD *test_world = world_generator->make_new_world( mods_empty );
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

            // Load any dependencies and de-duplicate them
            std::vector<mod_id> dep_vector = tree.get_dependencies_of_X_as_strings( mod.ident );
            std::set<mod_id> dep_set( dep_vector.begin(), dep_vector.end() );
            for( const auto &dep : dep_set ) {
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

        MAPBUFFER.clear();
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

    load_data_from_dir( PATH_INFO::jsondir(), "core", ui );
}

void game::load_data_from_dir( const cata_path &path, const std::string &src, loading_ui &ui )
{
    DynamicDataLoader::get_instance().load_data_from_path( path, src, ui );
}

#if !(defined(_WIN32) || defined(TILES))
// in ncurses_def.cpp
extern void check_encoding(); // NOLINT
extern void ensure_term_size(); // NOLINT
#endif

void game_ui::init_ui()
{
    // clear the screen
    static bool first_init = true;

    if( first_init ) {
#if !(defined(_WIN32) || defined(TILES))
        check_encoding();
#endif

        first_init = false;

#if defined(TILES)
        //class variable to track the option being active
        //only set once, toggle action is used to change during game
        pixel_minimap_option = get_option<bool>( "PIXEL_MINIMAP" );
        if( get_option<std::string>( "PIXEL_MINIMAP_BG" ) == "theme" ) {
            SDL_Color pixel_minimap_color = curses_color_to_SDL( c_black );
            pixel_minimap_r = pixel_minimap_color.r;
            pixel_minimap_g = pixel_minimap_color.g;
            pixel_minimap_b = pixel_minimap_color.b;
            pixel_minimap_a = pixel_minimap_color.a;
        } else {
            pixel_minimap_r = 0x00;
            pixel_minimap_g = 0x00;
            pixel_minimap_b = 0x00;
            pixel_minimap_a = 0xFF;
        }
#endif // TILES
    }

    // First get TERMX, TERMY
#if defined(TILES) || defined(_WIN32)
    TERMX = get_terminal_width();
    TERMY = get_terminal_height();

    get_options().get_option( "TERMINAL_X" ).setValue( TERMX * get_scaling_factor() );
    get_options().get_option( "TERMINAL_Y" ).setValue( TERMY * get_scaling_factor() );
    get_options().save();
#else
    TERMY = getmaxy( catacurses::stdscr );
    TERMX = getmaxx( catacurses::stdscr );

    ensure_term_size();

    // try to make FULL_SCREEN_HEIGHT symmetric according to TERMY
    if( TERMY % 2 ) {
        FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT + 1;
    } else {
        FULL_SCREEN_HEIGHT = EVEN_MINIMUM_TERM_HEIGHT;
    }
#endif
}

void game::toggle_fullscreen()
{
#if !defined(TILES)
    fullscreen = !fullscreen;
    mark_main_ui_adaptor_resize();
#else
    toggle_fullscreen_window();
#endif
}

void game::toggle_pixel_minimap() const
{
#if defined(TILES)
    if( pixel_minimap_option ) {
        clear_window_area( w_pixel_minimap );
    }
    pixel_minimap_option = !pixel_minimap_option;
    mark_main_ui_adaptor_resize();
#endif // TILES
}

void game::toggle_language_to_en()
{
    // No-op if we aren't complied with localization
#if defined(LOCALIZE)
    const std::string english = "en" ;
    static std::string secondary_lang = english;
    std::string current_lang = TranslationManager::GetInstance().GetCurrentLanguage();
    secondary_lang = current_lang != english ? current_lang : secondary_lang;
    std::string new_lang = current_lang != english ? english : secondary_lang;
    set_language( new_lang );
#endif
}

bool game::is_tileset_isometric() const
{
#if defined(TILES)
    return use_tiles && tilecontext && tilecontext->is_isometric();
#else
    return false;
#endif
}

void game::reload_tileset()
{
#if defined(TILES)
    // Disable UIs below to avoid accessing the tile context during loading.
    ui_adaptor ui( ui_adaptor::disable_uis_below {} );
    try {
        closetilecontext->reinit();
        closetilecontext->load_tileset( get_option<std::string>( "TILES" ),
                                        /*precheck=*/false, /*force=*/true,
                                        /*pump_events=*/true );
        closetilecontext->do_tile_loading_report();

        tilecontext = closetilecontext;
    } catch( const std::exception &err ) {
        popup( _( "Loading the tileset failed: %s" ), err.what() );
    }
    if( use_far_tiles ) {
        try {
            if( fartilecontext->is_valid() ) {
                fartilecontext->reinit();
            }
            fartilecontext->load_tileset( get_option<std::string>( "DISTANT_TILES" ),
                                          /*precheck=*/false, /*force=*/true,
                                          /*pump_events=*/true );
            fartilecontext->do_tile_loading_report();
        } catch( const std::exception &err ) {
            popup( _( "Loading the zoomed out tileset failed: %s" ), err.what() );
        }
    }
    try {
        overmap_tilecontext->reinit();
        overmap_tilecontext->load_tileset( get_option<std::string>( "OVERMAP_TILES" ),
                                           /*precheck=*/false, /*force=*/true,
                                           /*pump_events=*/true );
        overmap_tilecontext->do_tile_loading_report();
    } catch( const std::exception &err ) {
        popup( _( "Loading the overmap tileset failed: %s" ), err.what() );
    }
    g->reset_zoom();
    g->mark_main_ui_adaptor_resize();
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
    loading_ui ui( true );
    {
        background_pane background;
        static_popup popup;
        popup.message( "%s", _( "Please wait while the world data loads…\nLoading core data" ) );
        ui_manager::redraw();
        refresh_display();

        load_core_data( ui );
    }
    load_world_modfiles( ui );
    // Panel manager needs JSON data to be loaded before init
    panel_manager::get_manager().init();

    m = map();

    next_npc_id = character_id( 1 );
    next_mission_id = 1;
    new_game = true;
    uquit = QUIT_NO;   // We haven't quit the game
    bVMonsterLookFire = true;

    // invalidate calendar caches in case we were previously playing
    // a different world
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

    calendar::set_eternal_night( ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "night" );
    calendar::set_eternal_day( ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "day" );

    calendar::set_location( ::get_option<float>( "LATITUDE" ), ::get_option<float>( "LONGITUDE" ) );

    weather.weather_id = WEATHER_CLEAR;
    // Weather shift in 30
    weather.nextweather = calendar::start_of_game + 30_minutes;

    turnssincelastmon = 0_turns; //Auto safe mode init

    sounds::reset_sounds();
    clear_zombies();
    critter_tracker->clear_npcs();
    faction_manager_ptr->clear();
    mission::clear_all();
    Messages::clear_messages();
    timed_events = timed_event_manager();

    SCT.vSCT.clear(); //Delete pending messages

    stats().clear();
    // reset kill counts
    kill_tracker_ptr->clear();
    achievements_tracker_ptr->clear();
    eoc_events_ptr->clear();
    // reset follower list
    follower_ids.clear();
    scent.reset();
    effect_on_conditions::clear( u );
    u.character_mood_face( true );
    remoteveh_cache_time = calendar::before_time_starts;
    remoteveh_cache = nullptr;
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    unique_npcs.clear();
    get_weather().weather_override = WEATHER_NULL;
    // back to menu for save loading, new game etc
}

bool game::has_gametype() const
{
    return gamemode && gamemode->id() != special_game_type::NONE;
}

special_game_type game::gametype() const
{
    return gamemode ? gamemode->id() : special_game_type::NONE;
}

void game::load_map( const tripoint_abs_sm &pos_sm,
                     const bool pump_events )
{
    m.load( pos_sm, true, pump_events );
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

    background_pane background;
    static_popup popup;
    popup.message( "%s", _( "Please wait as we build your world" ) );
    ui_manager::redraw();
    refresh_display();

    load_master();
    u.setID( assign_npc_id() ); // should be as soon as possible, but *after* load_master

    // Make sure the items are added after the calendar is started
    u.add_profession_items();
    // Move items from the inventory. eventually the inventory should not contain items at all.
    u.migrate_items_to_storage( true );

    const start_location &start_loc = u.random_start_location ? scen->random_start_location().obj() :
                                      u.start_location.obj();
    tripoint_abs_omt omtstart = overmap::invalid_tripoint;
    const bool select_starting_city = get_option<bool>( "SELECT_STARTING_CITY" );
    do {
        if( select_starting_city ) {
            if( !u.starting_city.has_value() ) {
                u.starting_city = random_entry( city::get_all() );
                u.world_origin = u.starting_city->pos_om;
            }
            omtstart = start_loc.find_player_initial_location( u.starting_city.value() );
        } else {
            omtstart = start_loc.find_player_initial_location( u.world_origin.value_or( point_abs_om() ) );
        }
        if( omtstart == overmap::invalid_tripoint ) {

            MAPBUFFER.clear();
            overmap_buffer.clear();

            if( !query_yn(
                    _( "Try again?\n\nIt may require several attempts until the game finds a valid starting location." ) ) ) {
                return false;
            }
        }
    } while( omtstart == overmap::invalid_tripoint );

    start_loc.prepare_map( omtstart );

    if( scen->has_map_extra() ) {
        // Map extras can add monster spawn points and similar and should be done before the main
        // map is loaded.
        start_loc.add_map_extra( omtstart, scen->get_map_extra() );
    }

    tripoint_abs_sm lev = project_to<coords::sm>( omtstart );
    // The player is centered in the map, but lev[xyz] refers to the top left point of the map
    lev -= point( HALF_MAPSIZE, HALF_MAPSIZE );
    load_map( lev, /*pump_events=*/true );

    int level = m.get_abs_sub().z();
    u.setpos( m.getlocal( project_to<coords::ms>( omtstart ) ) );
    m.invalidate_map_cache( level );
    m.build_map_cache( level );
    // Do this after the map cache has been built!
    start_loc.place_player( u, omtstart );
    // ...but then rebuild it, because we want visibility cache to avoid spawning monsters in sight
    m.invalidate_map_cache( level );
    m.build_map_cache( level );
    // Start the overmap with out immediate neighborhood visible, this needs to be after place_player
    overmap_buffer.reveal( u.global_omt_location().xy(),
                           get_option<int>( "DISTANCE_INITIAL_VISIBILITY" ), 0 );

    const int city_size = get_option<int>( "CITY_SIZE" );
    if( get_scenario()->get_reveal_locale() && city_size > 0 ) {
        city_reference nearest_city = overmap_buffer.closest_city( m.get_abs_sub() );
        const tripoint_abs_omt city_center_omt = project_to<coords::omt>( nearest_city.abs_sm_pos );
        // Very necessary little hack: We look for roads around our start, and path from the closest. Because the most common start(evac shelter) cannot be pathed through...
        const tripoint_abs_omt nearest_road = overmap_buffer.find_closest( omtstart, "road", 3, false );
        // Reveal route to closest city and a 3 tile radius around the route
        overmap_buffer.reveal_route( nearest_road, city_center_omt, 3 );
        // Reveal destination city (scaling with city size setting)
        overmap_buffer.reveal( city_center_omt, city_size );
    }

    u.moves = 0;
    u.process_turn(); // process_turn adds the initial move points
    u.set_stamina( u.get_stamina_max() );
    weather.temperature = SPRING_TEMPERATURE;
    weather.update_weather();
    u.next_climate_control_check = calendar::before_time_starts; // Force recheck at startup
    u.last_climate_control_ret = false;

    //Reset character safe mode/pickup rules
    get_auto_pickup().clear_character_rules();
    get_safemode().clear_character_rules();
    get_auto_notes_settings().clear();
    get_auto_notes_settings().default_initialize();

    // spawn the starting NPC, assuming it's not disallowed by the scenario
    if( !get_scenario()->has_flag( "LONE_START" ) ) {
        create_starting_npcs();
    }
    //Load NPCs. Set nearby npcs to active.
    load_npcs();
    // Spawn the monsters
    // Surrounded start ones
    std::vector<std::pair<mongroup_id, float>> surround_groups = get_scenario()->surround_groups();
    const bool surrounded_start_scenario = !surround_groups.empty();
    const bool surrounded_start_options = get_option<bool>( "BLACK_ROAD" );
    if( surrounded_start_options && !surrounded_start_scenario ) {
        surround_groups.emplace_back( GROUP_BLACK_ROAD, 70.0f );
    }
    const bool spawn_near = surrounded_start_options || surrounded_start_scenario;
    if( spawn_near ) {
        for( const std::pair<mongroup_id, float> &sg : surround_groups ) {
            start_loc.surround_with_monsters( omtstart, sg.first, sg.second );
        }
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
    u.calc_mutation_levels();
    //Calculate mutation drench protection stats
    u.drench_mut_calc();
    if( scen->has_flag( "FIRE_START" ) ) {
        start_loc.burn( omtstart, 3, 3 );
    }
    if( scen->has_flag( "HELI_CRASH" ) ) {
        start_loc.handle_heli_crash( u );
        bool success = false;
        for( wrapped_vehicle v : m.get_vehicles() ) {
            std::string name = v.v->type.str();
            std::string search = std::string( "helicopter" );
            if( name.find( search ) != std::string::npos ) {
                for( const vpart_reference &vp : v.v->get_any_parts( VPFLAG_CONTROLS ) ) {
                    const tripoint pos = vp.pos();
                    u.setpos( pos );

                    // Delete the items that would have spawned here from a "corpse"
                    for( const int sp : v.v->parts_at_relative( vp.mount(), true ) ) {
                        vpart_reference( *v.v, sp ).items().clear();
                    }

                    auto mons = critter_tracker->find( u.get_location() );
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
    if( scen->has_flag( "BORDERED" ) ) {
        const point_abs_omt p_player = get_player_character().global_omt_location().xy();
        point_abs_om om;
        point_om_omt omt;
        std::tie( om, omt ) = project_remain<coords::om>( p_player );

        // The Wall is supposed to be a 100x100 rectangle centered on player,
        // but if player is spawned next to edges of the overmap we need to make it smaller so it doesn't overlap the overmap's borders
        int left_x = omt.x() < 50 ? 1 : omt.x() - 50;
        int right_x = omt.x() > 129 ? 179 : omt.x() + 50;
        int up_y = omt.y() < 50 ? 1 : omt.y() - 50;
        int down_y = omt.y() > 129 ? 179 : omt.y() + 50;

        overmap &starting_om = get_cur_om();

        for( int x = left_x; x <= right_x; x++ ) {
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                starting_om.place_special_forced( overmap_special_world, { x, up_y, z },
                                                  om_direction::type::north );
            }
        }

        for( int x = left_x; x <= right_x; x++ ) {
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                starting_om.place_special_forced( overmap_special_world, { x, down_y, z },
                                                  om_direction::type::north );
            }
        }

        for( int y = up_y; y <= down_y; y++ ) {
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                starting_om.place_special_forced( overmap_special_world, { left_x, y, z },
                                                  om_direction::type::north );
            }
        }

        for( int y = up_y; y <= down_y; y++ ) {
            for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
                starting_om.place_special_forced( overmap_special_world, { right_x, y, z },
                                                  om_direction::type::north );
            }
        }
    }
    for( item *&e : u.inv_dump() ) {
        e->set_owner( get_player_character() );
    }
    // Now that we're done handling coordinates, ensure the player's submap is in the center of the map
    update_map( u );
    // Profession pets
    for( const mtype_id &elem : u.starting_pets ) {
        if( monster *const mon = place_critter_around( elem, u.pos(), 5 ) ) {
            mon->friendly = -1;
            mon->add_effect( effect_pet, 1_turns, true );
        } else {
            add_msg_debug( debugmode::DF_GAME, "cannot place starting pet, no space!" );
        }
    }
    if( u.starting_vehicle &&
        !place_vehicle_nearby( u.starting_vehicle, u.global_omt_location().xy(), 1, 30,
                               std::vector<std::string> {} ) ) {
        debugmsg( "could not place starting vehicle" );
    }
    // Assign all of this scenario's missions to the player.
    for( const mission_type_id &m : scen->missions() ) {
        mission *new_mission = mission::reserve_new( m, character_id() );
        new_mission->assign( u );
    }

    // Same for profession missions
    if( !!u.prof ) {
        for( const mission_type_id &m : u.prof->missions() ) {
            mission *new_mission = mission::reserve_new( m, character_id() );
            new_mission->assign( u );
        }
    }
    // ... and for hobbies
    for( const profession *hby : u.hobbies ) {
        if( !!hby ) {
            for( const mission_type_id &m : hby->missions() ) {
                mission *new_mission = mission::reserve_new( m, character_id() );
                new_mission->assign( u );
            }
        }
    }

    get_event_bus().send<event_type::game_start>( getVersionString() );
    get_event_bus().send<event_type::game_avatar_new>( /*is_new_game=*/true, /*is_debug=*/false,
            u.getID(), u.name, u.male, u.prof->ident(), u.custom_profession );
    time_played_at_last_load = std::chrono::seconds( 0 );
    time_of_last_load = std::chrono::steady_clock::now();
    tripoint_abs_omt abs_omt = u.global_omt_location();
    const oter_id &cur_ter = overmap_buffer.ter( abs_omt );
    get_event_bus().send<event_type::avatar_enters_omt>( abs_omt.raw(), cur_ter );

    effect_on_conditions::load_new_character( u );
    return true;
}

vehicle *game::place_vehicle_nearby(
    const vproto_id &id, const point_abs_omt &origin, int min_distance,
    int max_distance, const std::vector<std::string> &omt_search_types )
{
    std::vector<std::string> search_types = omt_search_types;
    if( search_types.empty() ) {
        const vehicle &veh = *id->blueprint;
        if( veh.max_ground_velocity() == 0 && veh.can_float() ) {
            search_types.emplace_back( "river" );
            search_types.emplace_back( "lake" );
            search_types.emplace_back( "ocean" );
        } else {
            search_types.emplace_back( "road" );
            search_types.emplace_back( "field" );
        }
    }
    for( const std::string &search_type : search_types ) {
        omt_find_params find_params;
        find_params.types.emplace_back( search_type, ot_match_type::type );
        // find nearest road
        find_params.min_distance = min_distance;
        find_params.search_range = max_distance;
        // if player spawns underground, park their car on the surface.
        const tripoint_abs_omt omt_origin( origin, 0 );
        for( const tripoint_abs_omt &goal : overmap_buffer.find_all( omt_origin, find_params ) ) {
            // try place vehicle there.
            tinymap target_map;
            target_map.load( project_to<coords::sm>( goal ), false );
            const tripoint tinymap_center( SEEX, SEEY, goal.z() );
            static constexpr std::array<units::angle, 4> angles = {{
                    0_degrees, 90_degrees, 180_degrees, 270_degrees
                }
            };
            vehicle *veh = target_map.add_vehicle( id, tinymap_center, random_entry( angles ),
                                                   rng( 50, 80 ), 0, false );
            if( veh ) {
                tripoint abs_local = m.getlocal( target_map.getabs( tinymap_center ) );
                veh->sm_pos =  ms_to_sm_remain( abs_local );
                veh->pos = abs_local.xy();

                veh->unlock();          // always spawn unlocked
                veh->toggle_tracking(); // always spawn tracked

                target_map.save();
                return veh;
            }
        }
    }
    return nullptr;
}

//Make any nearby overmap npcs active, and put them in the right location.
void game::load_npcs()
{
    const int radius = HALF_MAPSIZE - 1;
    const tripoint_abs_sm abs_sub( get_map().get_abs_sub() );
    const half_open_rectangle<point_abs_sm> map_bounds( abs_sub.xy(), abs_sub.xy() + point( MAPSIZE,
            MAPSIZE ) );
    // uses submap coordinates
    std::vector<shared_ptr_fast<npc>> just_added;
    for( const auto &temp : overmap_buffer.get_npcs_near_player( radius ) ) {
        const character_id &id = temp->getID();
        const auto found = std::find_if( critter_tracker->active_npc.begin(),
                                         critter_tracker->active_npc.end(),
        [id]( const shared_ptr_fast<npc> &n ) {
            return n->getID() == id;
        } );
        if( found != critter_tracker->active_npc.end() ) {
            continue;
        }
        if( temp->is_active() ) {
            continue;
        }
        if( temp->has_companion_mission() ) {
            continue;
        }

        const tripoint_abs_sm sm_loc = temp->global_sm_location();
        // NPCs who are out of bounds before placement would be pushed into bounds
        // This can cause NPCs to teleport around, so we don't want that
        if( !map_bounds.contains( sm_loc.xy() ) ) {
            continue;
        }

        add_msg_debug( debugmode::DF_NPC, "game::load_npcs: Spawning static NPC, %s %s",
                       abs_sub.to_string_writable(), sm_loc.to_string_writable() );
        temp->place_on_map();
        if( !m.inbounds( temp->pos() ) ) {
            continue;
        }
        // In the rare case the npc was marked for death while
        // it was on the overmap. Kill it.
        if( temp->marked_for_death ) {
            temp->die( nullptr );
        } else {
            critter_tracker->active_npc.push_back( temp );
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
    for( const auto &npc : critter_tracker->active_npc ) {
        npc->on_unload();
    }

    critter_tracker->clear_npcs();
}

void game::remove_npc( character_id const &id )
{
    std::list<shared_ptr_fast<npc>> &active_npc = critter_tracker->active_npc;
    auto const it = std::find_if( active_npc.begin(), active_npc.end(),
    [id]( shared_ptr_fast<npc> const & n ) {
        return n->getID() == id;
    } );
    if( it != active_npc.end() ) {
        active_npc.erase( it );
    }
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
    //We don't want more than one starting npc per starting location
    const int radius = 1;
    if( !overmap_buffer.get_npcs_near_player( radius ).empty() ) {
        return; //There is already an NPC in this starting location
    }

    shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
    tmp->normalize();
    tmp->randomize( one_in( 2 ) ? NC_DOCTOR : NC_NONE );
    // hardcoded, consistent NPC position
    // start_loc::place_player relies on this and must be updated if this is changed
    tmp->spawn_at_precise( u.get_location() + point_north_west );
    overmap_buffer.insert_npc( tmp );
    tmp->form_opinion( u );
    tmp->set_attitude( NPCATT_NULL );
    //This sets the NPC mission. This NPC remains in the starting location.
    tmp->mission = NPC_MISSION_SHELTER;
    tmp->chatbin.first_topic = "TALK_SHELTER";
    tmp->toggle_trait( trait_NPC_STARTING_NPC );
    tmp->set_fac( faction_no_faction );
    //One random starting NPC mission
    tmp->add_new_mission( mission::reserve_random( ORIGIN_OPENER_NPC, tmp->global_omt_location(),
                          tmp->getID() ) );
}

static int veh_lumi( vehicle &veh )
{
    float veh_luminance = 0.0f;
    float iteration = 1.0f;
    auto lights = veh.lights();

    for( const vehicle_part *pt : lights ) {
        const vpart_info &vp = pt->info();
        if( vp.has_flag( VPFLAG_CONE_LIGHT ) ||
            vp.has_flag( VPFLAG_WIDE_CONE_LIGHT ) ) {
            veh_luminance += vp.bonus / iteration;
            iteration = iteration * 1.1f;
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
    const float max_offset_vel = std::min( max_offset.y, max_offset.x ) *
                                 vehicles::vmiph_per_tile;
    float velocity = veh->velocity;
    rl_vec2d offset = veh->move_vec();
    if( !veh->skidding && veh->player_in_control( u ) &&
        std::abs( veh->cruise_velocity - veh->velocity ) < 7 * vehicles::vmiph_per_tile ) {
        // Use cruise_velocity, but only if
        // it is not too different from the actual velocity.
        // The actual velocity changes too often (see above slowdown).
        // Using it makes would make the offset change far too often.
        offset = veh->face_vec();
        velocity = veh->cruise_velocity;
    }
    const float rel_offset = inverse_lerp( min_offset_vel, max_offset_vel, velocity );
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

void game::catch_a_monster( monster *fish, const tripoint &pos, Character *p,
                            const time_duration &catch_duration ) // catching function
{
    //spawn the corpse, rotten by a part of the duration
    m.add_item_or_charges( pos, item::make_corpse( fish->type->id, calendar::turn + rng( 0_turns,
                           catch_duration ) ) );
    if( u.sees( pos ) ) {
        u.add_msg_if_player( m_good, _( "You caught a %s." ), fish->type->nname() );
    }
    //quietly kill the caught
    fish->no_corpse_quiet = true;
    fish->die( p );
}

static bool cancel_auto_move( Character &you, const std::string &text )
{
    if( !you.has_destination() ) {
        return false;
    }
    g->invalidate_main_ui_adaptor();
    if( query_yn( _( "%s Cancel auto move?" ), text ) )  {
        add_msg( m_warning, _( "%s Auto move canceled." ), text );
        if( !you.omt_path.empty() ) {
            you.omt_path.clear();
        }
        you.clear_destination();
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
            u.set_destination( u.get_auto_move_route(), player_activity( ACT_TRAVELLING ) );
            return false;
        }
    }
    if( !u.activity || u.activity.is_distraction_ignored( type ) ) {
        return false;
    }
    const bool force_uc = get_option<bool>( "FORCE_CAPITAL_YN" );
    const auto &allow_key = force_uc ? input_context::disallow_lower_case_or_non_modified_letters
                            : input_context::allow_all_keys;

    const std::string &action = query_popup()
                                .preferred_keyboard_mode( keyboard_mode::keycode )
                                .context( "CANCEL_ACTIVITY_OR_IGNORE_QUERY" )
                                .message( force_uc && !is_keycode_mode_supported() ?
                                          pgettext( "cancel_activity_or_ignore_query",
                                                  "<color_light_red>%s %s (Case Sensitive)</color>" ) :
                                          pgettext( "cancel_activity_or_ignore_query",
                                                  "<color_light_red>%s %s</color>" ),
                                          text, u.activity.get_stop_phrase() )
                                .option( "YES", allow_key )
                                .option( "NO", allow_key )
                                .option( "MANAGER", allow_key )
                                .option( "IGNORE", allow_key )
                                .query()
                                .action;

    if( action == "YES" ) {
        u.cancel_activity();
        return true;
    }
    if( action == "IGNORE" ) {
        u.activity.ignore_distraction( type );
        for( player_activity &activity : u.backlog ) {
            activity.ignore_distraction( type );
        }
    }
    if( action == "MANAGER" ) {
        u.cancel_activity();
        get_distraction_manager().show();
        return true;
    }

    ui_manager::redraw();
    refresh_display();

    return false;
}

bool game::portal_storm_query( const distraction_type type, const std::string &text )
{
    if( u.has_distant_destination() ) {
        if( cancel_auto_move( u, text ) ) {
            return true;
        } else {
            u.set_destination( u.get_auto_move_route(), player_activity( ACT_TRAVELLING ) );
            return false;
        }
    }
    if( !u.activity || u.activity.is_distraction_ignored( type ) ) {
        return false;
    }

    static const std::vector<nc_color> color_list = {
        c_light_red, c_red, c_green, c_light_green,
        c_blue, c_light_blue, c_yellow
    };
    const nc_color color = random_entry( color_list );

    query_popup()
    .context( "YES_QUERY" )
    .message( "%s", text )
    .option( "YES0" )
    .option( "YES1" )
    .option( "YES2" )
    .default_color( color )
    .query();

    // ensure it never happens again during this activity - shouldn't be an issue anyway
    u.activity.ignore_distraction( type );
    for( player_activity &activity : u.backlog ) {
        activity.ignore_distraction( type );
    }

    ui_manager::redraw();
    refresh_display();

    return false;
}

bool game::cancel_activity_query( const std::string &text )
{
    if( u.has_destination() ) {
        if( cancel_auto_move( u, text ) ) {
            return true;
        } else if( u.has_distant_destination() ) {
            u.set_destination( u.get_auto_move_route(), player_activity( ACT_TRAVELLING ) );
            return false;
        }
    }
    if( !u.activity ) {
        return false;
    }
    g->invalidate_main_ui_adaptor();
    if( query_yn( "%s %s", text, u.activity.get_stop_phrase() ) ) {
        if( u.activity.id() == ACT_TRAIN_TEACHER ) {
            for( npc &n : all_npcs() ) {
                // Also cancel activities for students
                for( const int st_id : u.activity.values ) {
                    if( n.getID().get_value() == st_id ) {
                        n.cancel_activity();
                    }
                }
            }
            u.remove_effect( effect_asked_to_train );
        } else if( u.activity.id() == ACT_TRAIN ) {
            for( npc &n : all_npcs() ) {
                // If the player is the only student, cancel the teacher's activity
                if( n.getID().get_value() == u.activity.index && n.activity.values.size() == 1 ) {
                    n.cancel_activity();
                }
            }
        }
        u.cancel_activity();
        u.clear_destination();
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
    const field_entry *field_ptr = mt.find_field( fld );

    return field_ptr == nullptr ? 0 : field_ptr->get_field_intensity();
}

units::temperature_delta get_heat_radiation( const tripoint &location )
{
    units::temperature_delta temp_mod = units::from_kelvin_delta( 0 );
    Character &player_character = get_player_character();
    map &here = get_map();
    // Convert it to an int id once, instead of 139 times per turn
    const field_type_id fd_fire_int = fd_fire.id();
    for( const tripoint &dest : here.points_in_radius( location, 6 ) ) {
        int heat_intensity = 0;

        maptile mt = here.maptile_at( dest );

        int ffire = maptile_field_intensity( mt, fd_fire_int );
        if( ffire > 0 ) {
            heat_intensity = ffire;
        } else  {
            heat_intensity = mt.get_ter()->heat_radiation;
        }
        if( heat_intensity == 0 ) {
            // No heat source here
            continue;
        }
        if( player_character.pos() == location ) {
            if( !here.clear_path( dest, location, -1, 1, 100 ) ) {
                continue;
            }
        } else if( !here.sees( location, dest, -1 ) ) {
            continue;
        }
        // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
        const int fire_dist = std::max( 1, square_dist( dest, location ) );
        temp_mod += units::from_fahrenheit_delta( 6.f * heat_intensity * heat_intensity / fire_dist );
    }
    return temp_mod;
}

int get_best_fire( const tripoint &location )
{
    int best_fire = 0;
    Character &player_character = get_player_character();
    map &here = get_map();
    // Convert it to an int id once, instead of 139 times per turn
    const field_type_id fd_fire_int = fd_fire.id();
    for( const tripoint &dest : here.points_in_radius( location, 6 ) ) {
        int heat_intensity = 0;

        maptile mt = here.maptile_at( dest );

        int ffire = maptile_field_intensity( mt, fd_fire_int );
        if( ffire > 0 ) {
            heat_intensity = ffire;
        } else  {
            heat_intensity = mt.get_ter()->heat_radiation;
        }
        if( heat_intensity == 0 ) {
            // No heat source here
            continue;
        }
        if( player_character.pos() == location ) {
            if( !here.clear_path( dest, location, -1, 1, 100 ) ) {
                continue;
            }
        } else if( !here.sees( location, dest, -1 ) ) {
            continue;
        }
        if( square_dist( dest, location ) <= 1 ) {
            // Extend limbs/lean over a single adjacent fire to warm up
            best_fire = std::max( best_fire, heat_intensity );
        }
    }
    return best_fire;
}

units::temperature_delta get_convection_temperature( const tripoint &location )
{
    units::temperature_delta temp_mod = units::from_kelvin_delta( 0 );
    map &here = get_map();
    // Directly on lava tiles
    units::temperature_delta lava_mod = here.tr_at( location ).has_flag(
                                            json_flag_CONVECTS_TEMPERATURE ) ?
                                        units::from_fahrenheit_delta( fd_fire->get_intensity_level().convection_temperature_mod ) :
                                        units::from_kelvin_delta( 0 );
    // Modifier from fields
    for( auto fd : here.field_at( location ) ) {
        // Nullify lava modifier when there is open fire
        if( fd.first.obj().has_fire ) {
            lava_mod = units::from_kelvin_delta( 0 );
        }
        temp_mod += units::from_fahrenheit_delta(
                        fd.second.get_intensity_level().convection_temperature_mod );
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

npc *game::find_npc_by_unique_id( const std::string &unique_id )
{
    return overmap_buffer.find_npc_by_unique_id( unique_id ).get();
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
        guy->set_fac( faction_your_followers );
        guy->set_faction_ver( 2 );
    }
}

void game::validate_linked_vehicles()
{
    for( wrapped_vehicle &veh : m.get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->tow_data.other_towing_point != tripoint_zero ) {
            vehicle *other_v = veh_pointer_or_null( m.veh_at( v->tow_data.other_towing_point ) );
            if( other_v ) {
                // the other vehicle is towing us.
                v->tow_data.set_towing( other_v, v );
                v->tow_data.other_towing_point = tripoint_zero;
            }
        }
    }
}

void game::validate_mounted_npcs()
{
    for( monster &m : all_monsters() ) {
        if( m.has_effect( effect_ridden ) && m.mounted_player_id.is_valid() ) {
            Character *mounted_pl = g->critter_by_id<Character>( m.mounted_player_id );
            if( !mounted_pl ) {
                // Target no longer valid.
                m.mounted_player_id = character_id();
                m.remove_effect( effect_ridden );
                continue;
            }
            mounted_pl->mounted_creature = shared_from( m );
            mounted_pl->setpos( m.pos() );
            mounted_pl->add_effect( effect_riding, 1_turns, true );
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
    for( const character_id &temp_id : u.follower_ids ) {
        add_npc_follower( temp_id );
    }
}

void game::validate_camps()
{
    basecamp camp = m.hoist_submap_camp( u.pos() );
    if( camp.is_valid() ) {
        overmap_buffer.add_camp( camp );
        m.remove_submap_camp( u.pos() );
    } else if( camp.camp_omt_pos() != tripoint_abs_omt() ) {
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

static hint_rating rate_action_change_side( const avatar &you, const item &it )
{
    if( !it.is_sided() ) {
        return hint_rating::cant;
    }

    return you.is_worn( it ) ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_disassemble( avatar &you, const item &it )
{
    if( you.can_disassemble( it, you.crafting_inventory() ).success() ) {
        // Possible right now
        return hint_rating::good;
    } else if( it.is_disassemblable() ) {
        // Potentially possible, but we currently lack requirements
        return hint_rating::iffy;
    } else {
        // Never possible
        return hint_rating::cant;
    }
}

static hint_rating rate_action_view_recipe( avatar &you, const item &it )
{
    const inventory &inven = you.crafting_inventory();
    const std::vector<Character *> helpers = you.get_crafting_group();
    if( it.is_craft() ) {
        const recipe &craft_recipe = it.get_making();
        if( craft_recipe.is_null() || !craft_recipe.ident().is_valid() ) {
            return hint_rating::cant;
        } else if( you.get_available_recipes( inven, &helpers ).contains( &craft_recipe ) ) {
            return hint_rating::good;
        }
    } else {
        itype_id item = it.typeId();
        bool is_byproduct = false;  // product or byproduct
        bool can_craft = false;
        // Does a recipe for the item exist?
        for( const auto& [_, r] : recipe_dict ) {
            if( !r.obsolete && ( item == r.result() || r.in_byproducts( item ) ) ) {
                is_byproduct = true;
                // If if exists, do I know it?
                if( you.get_available_recipes( inven, &helpers ).contains( &r ) ) {
                    can_craft = true;
                    break;
                }
            }
        }
        if( !is_byproduct ) {
            return hint_rating::cant;
        } else if( can_craft ) {
            return hint_rating::good;
        }
    }
    return hint_rating::iffy;
}

static hint_rating rate_action_eat( const avatar &you, const item &it )
{
    if( it.is_container() ) {
        hint_rating best_rate = hint_rating::cant;
        it.visit_items( [&you, &best_rate]( item * node, item * ) {
            if( you.can_consume_as_is( *node ) )  {
                ret_val<edible_rating> rate = you.will_eat( *node );
                if( rate.success() ) {
                    best_rate = hint_rating::good;
                    return VisitResponse::ABORT;
                } else if( rate.value() != INEDIBLE || rate.value() != INEDIBLE_MUTATION ) {
                    best_rate = hint_rating::iffy;
                }
            }
            return VisitResponse::NEXT;
        } );
        return best_rate;
    }

    if( !you.can_consume_as_is( it ) ) {
        return hint_rating::cant;
    }

    const auto rating = you.will_eat( it );
    if( rating.success() ) {
        return hint_rating::good;
    } else if( rating.value() == INEDIBLE || rating.value() == INEDIBLE_MUTATION ) {
        return hint_rating::cant;
    }

    return hint_rating::iffy;
}

static hint_rating rate_action_collapse( const item &it )
{
    for( const item_pocket *pocket : it.get_all_standard_pockets() ) {
        if( !pocket->settings.is_collapsed() ) {
            return hint_rating::good;
        }
    }
    return hint_rating::cant;
}

static hint_rating rate_action_expand( const item &it )
{
    for( const item_pocket *pocket : it.get_all_standard_pockets() ) {
        if( pocket->settings.is_collapsed() ) {
            return hint_rating::good;
        }
    }
    return hint_rating::cant;
}

static hint_rating rate_action_mend( const avatar &, const item &it )
{
    // TODO: check also if item damage could be repaired via a tool
    if( !it.faults.empty() ) {
        return hint_rating::good;
    }
    return it.faults_potential().empty() ? hint_rating::cant : hint_rating::iffy;
}

static hint_rating rate_action_read( const avatar &you, const item &it )
{
    if( !it.is_book() ) {
        return hint_rating::cant;
    }

    std::vector<std::string> dummy;
    return you.get_book_reader( it, dummy ) ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_take_off( const avatar &you, const item &it )
{
    if( !it.is_armor() || it.has_flag( flag_NO_TAKEOFF ) || it.has_flag( flag_INTEGRATED ) ) {
        return hint_rating::cant;
    }

    if( you.is_worn( it ) ) {
        return hint_rating::good;
    }

    return hint_rating::iffy;
}

static hint_rating rate_action_use( const avatar &you, const item &it )
{
    if( it.is_container() && it.item_has_uses_recursive() ) {
        return hint_rating::good;
    }
    if( it.is_broken() ) {
        return hint_rating::iffy;
    } else if( it.is_tool() ) {
        return it.ammo_sufficient( &you ) ? hint_rating::good : hint_rating::iffy;
    } else if( it.is_gunmod() ) {
        /** @EFFECT_GUN >0 allows rating estimates for gun modifications */
        if( static_cast<int>( you.get_skill_level( skill_gun ) ) == 0 ) {
            return hint_rating::iffy;
        } else {
            return hint_rating::good;
        }
    } else if( it.is_food() || it.is_medication() || it.is_book() || it.is_armor() ) {
        if( it.is_medication() && !you.can_use_heal_item( it ) ) {
            return hint_rating::cant;
        }
        if( it.is_comestible() && it.is_frozen_liquid() ) {
            return hint_rating::cant;
        }
        // The rating is subjective, could be argued as hint_rating::cant or hint_rating::good as well
        return hint_rating::iffy;
    } else if( it.type->has_use() ) {
        return hint_rating::good;
    }

    return hint_rating::cant;
}

static hint_rating rate_action_wear( const avatar &you, const item &it )
{
    if( !it.is_armor() ) {
        return hint_rating::cant;
    }

    if( you.is_worn( it ) ) {
        return hint_rating::iffy;
    }

    return you.can_wear( it ).success() ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_wield( const avatar &you, const item &it )
{
    return you.can_wield( it ).success() ? hint_rating::good : hint_rating::iffy;
}

static hint_rating rate_action_insert( const avatar &you, const item_location &loc )
{
    if( loc->will_spill_if_unsealed()
        && loc.where() != item_location::type::map
        && !you.is_wielding( *loc ) ) {

        return hint_rating::cant;
    }
    return hint_rating::good;
}

/* item submenu for 'i' and '/'
* It use draw_item_info to draw item info and action menu
*
* @param locThisItem the item
* @param iStartX Left coordinate of the item info window
* @param iWidth width of the item info window (height = height of terminal)
* @return getch
*/
int game::inventory_item_menu( item_location locThisItem,
                               const std::function<int()> &iStartX,
                               const std::function<int()> &iWidth,
                               const inventory_item_menu_position position )
{
    int cMenu = static_cast<int>( '+' );

    item &oThisItem = *locThisItem;
    if( u.has_item( oThisItem ) ) {
#if defined(__ANDROID__)
        if( get_option<bool>( "ANDROID_INVENTORY_AUTOADD" ) ) {
            add_key_to_quick_shortcuts( oThisItem.invlet, "INVENTORY", false );
        }
#endif
        const bool bHPR = get_auto_pickup().has_rule( &oThisItem );
        std::vector<iteminfo> vThisItem;
        std::vector<iteminfo> vDummy;
        item_info_data data;
        int iScrollPos = 0;
        int iScrollHeight = 0;
        uilist action_menu;
        std::unique_ptr<ui_adaptor> ui;

        bool exit = false;
        bool first_execution = true;
        static int lang_version = detail::get_current_language_version();
        catacurses::window w_info;
        do {
            //lang check here is needed to redraw the menu when using "Toggle language to English" option
            if( first_execution || lang_version != detail::get_current_language_version() ) {

                const hint_rating rate_drop_item = u.get_wielded_item() &&
                                                   u.get_wielded_item()->has_flag( flag_NO_UNWIELD ) ?
                                                   hint_rating::cant : hint_rating::good;
                action_menu.reset();
                action_menu.allow_anykey = true;
                const auto addentry = [&]( const char key, const std::string & text, const hint_rating hint ) {
                    // The char is used as retval from the uilist *and* as hotkey.
                    action_menu.addentry( key, true, key, text );
                    auto &entry = action_menu.entries.back();
                    switch( hint ) {
                        case hint_rating::cant:
                            entry.text_color = c_light_gray;
                            break;
                        case hint_rating::iffy:
                            entry.text_color = c_light_red;
                            break;
                        case hint_rating::good:
                            entry.text_color = c_light_green;
                            break;
                    }
                };
                addentry( 'a', pgettext( "action", "activate" ), rate_action_use( u, oThisItem ) );
                addentry( 'R', pgettext( "action", "read" ), rate_action_read( u, oThisItem ) );
                addentry( 'E', pgettext( "action", "eat" ), rate_action_eat( u, oThisItem ) );
                addentry( 'W', pgettext( "action", "wear" ), rate_action_wear( u, oThisItem ) );
                addentry( 'w', pgettext( "action", "wield" ), rate_action_wield( u, oThisItem ) );
                addentry( 't', pgettext( "action", "throw" ), rate_action_wield( u, oThisItem ) );
                addentry( 'c', pgettext( "action", "change side" ), rate_action_change_side( u, oThisItem ) );
                addentry( 'T', pgettext( "action", "take off" ), rate_action_take_off( u, oThisItem ) );
                addentry( 'd', pgettext( "action", "drop" ), rate_drop_item );
                addentry( 'U', pgettext( "action", "unload" ), u.rate_action_unload( oThisItem ) );
                addentry( 'r', pgettext( "action", "reload" ), u.rate_action_reload( oThisItem ) );
                addentry( 'p', pgettext( "action", "part reload" ), u.rate_action_reload( oThisItem ) );
                addentry( 'm', pgettext( "action", "mend" ), rate_action_mend( u, oThisItem ) );
                addentry( 'D', pgettext( "action", "disassemble" ), rate_action_disassemble( u, oThisItem ) );
                if( oThisItem.is_container() && !oThisItem.is_corpse() ) {
                    addentry( 'i', pgettext( "action", "insert" ), rate_action_insert( u, locThisItem ) );
                    if( oThisItem.num_item_stacks() > 0 ) {
                        addentry( 'o', pgettext( "action", "open" ), hint_rating::good );
                    }
                    addentry( 'v', pgettext( "action", "pocket settings" ), hint_rating::good );
                }

                if( oThisItem.is_favorite ) {
                    addentry( 'f', pgettext( "action", "unfavorite" ), hint_rating::good );
                } else {
                    addentry( 'f', pgettext( "action", "favorite" ), hint_rating::good );
                }

                addentry( 'V', pgettext( "action", "view recipe" ), rate_action_view_recipe( u, oThisItem ) );
                addentry( '>', pgettext( "action", "hide contents" ), rate_action_collapse( oThisItem ) );
                addentry( '<', pgettext( "action", "show contents" ), rate_action_expand( oThisItem ) );
                addentry( '=', pgettext( "action", "reassign" ), hint_rating::good );

                if( bHPR ) {
                    addentry( '-', _( "Auto pickup" ), hint_rating::iffy );
                } else {
                    addentry( '+', _( "Auto pickup" ), hint_rating::good );
                }

                oThisItem.info( true, vThisItem );

                action_menu.w_y_setup = 0;
                action_menu.w_x_setup = [&]( const int popup_width ) -> int {
                    switch( position )
                    {
                        default:
                        case RIGHT_TERMINAL_EDGE:
                            return 0;
                        case LEFT_OF_INFO:
                            return iStartX() - popup_width;
                        case RIGHT_OF_INFO:
                            return iStartX() + iWidth();
                        case LEFT_TERMINAL_EDGE:
                            return TERMX - popup_width;
                    }
                };
                // Filtering isn't needed, the number of entries is manageable.
                action_menu.filtering = false;
                // Default menu border color is different, this matches the border of the item info window.
                action_menu.border_color = BORDER_COLOR;

                data = item_info_data( oThisItem.tname(), oThisItem.type_name(), vThisItem, vDummy, iScrollPos );
                data.without_getch = true;

                ui = std::make_unique<ui_adaptor>();
                ui->on_screen_resize( [&]( ui_adaptor & ui ) {
                    w_info = catacurses::newwin( TERMY, iWidth(), point( iStartX(), 0 ) );
                    iScrollHeight = TERMY - 2;
                    ui.position_from_window( w_info );
                } );
                ui->mark_resize();

                ui->on_redraw( [&]( const ui_adaptor & ) {
                    draw_item_info( w_info, data );
                } );

                action_menu.additional_actions = {
                    { "RIGHT", translation() }
                };

                lang_version = detail::get_current_language_version();
                first_execution = false;
            }

            const int prev_selected = action_menu.selected;
            action_menu.query( false );
            if( action_menu.ret >= 0 ) {
                cMenu = action_menu.ret; /* Remember: hotkey == retval, see addentry above. */
            } else if( action_menu.ret == UILIST_UNBOUND && action_menu.ret_act == "RIGHT" ) {
                // Simulate KEY_RIGHT == '\n' (confirm currently selected entry) for compatibility with old version.
                // TODO: ideally this should be done in the uilist, maybe via a callback.
                cMenu = action_menu.ret = action_menu.entries[action_menu.selected].retval;
            } else if( action_menu.ret_act == "PAGE_UP" || action_menu.ret_act == "PAGE_DOWN" ) {
                cMenu = action_menu.ret_act == "PAGE_UP" ? KEY_PPAGE : KEY_NPAGE;
                // Prevent the menu from scrolling with this key. TODO: Ideally the menu
                // could be instructed to ignore these two keys instead of scrolling.
                action_menu.selected = prev_selected;
                action_menu.fselected = prev_selected;
                action_menu.vshift = 0;
            } else {
                cMenu = 0;
            }

            if( action_menu.ret != UILIST_WAIT_INPUT && action_menu.ret != UILIST_UNBOUND ) {
                exit = true;
                ui = nullptr;
            }

            switch( cMenu ) {
                case 'a': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    if( locThisItem.get_item()->type->has_use() &&
                        !locThisItem.get_item()->item_has_uses_recursive( true ) ) { // NOLINT(bugprone-branch-clone)
                        // Item has uses and none of its contents (if any) has uses.
                        avatar_action::use_item( u, locThisItem );
                    } else if( locThisItem.get_item()->item_has_uses_recursive() ) {
                        game::item_action_menu( locThisItem );
                    } else if( locThisItem.get_item()->has_relic_activation() ) {
                        avatar_action::use_item( u, locThisItem );
                    } else {
                        add_msg( m_info, _( "You can't use a %s there." ), locThisItem->tname() );
                        break;
                    }
                    handler.handle_by( u );
                    break;
                }
                case 'E':
                    if( !locThisItem.get_item()->is_container() ) {
                        avatar_action::eat( u, locThisItem );
                    } else {
                        avatar_action::eat_or_use( u, game_menus::inv::consume( u, locThisItem ) );
                    }
                    break;
                case 'W': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    u.wear( locThisItem );
                    handler.handle_by( u );
                    break;
                }
                case 'w':
                    if( u.can_wield( *locThisItem ).success() ) {
                        contents_change_handler handler;
                        handler.unseal_pocket_containing( locThisItem );
                        wield( locThisItem );
                        handler.handle_by( u );
                    } else {
                        add_msg( m_info, "%s", u.can_wield( *locThisItem ).c_str() );
                    }
                    break;
                case 't': {
                    contents_change_handler handler;
                    handler.unseal_pocket_containing( locThisItem );
                    avatar_action::plthrow( u, locThisItem );
                    handler.handle_by( u );
                    break;
                }
                case 'c':
                    u.change_side( locThisItem );
                    break;
                case 'T':
                    u.takeoff( locThisItem );
                    break;
                case 'd':
                    u.drop( locThisItem, u.pos() );
                    break;
                case 'U':
                    u.unload( locThisItem );
                    break;
                case 'r':
                    reload( locThisItem );
                    break;
                case 'p':
                    reload( locThisItem, true );
                    break;
                case 'm':
                    avatar_action::mend( u, locThisItem );
                    break;
                case 'R':
                    u.read( locThisItem );
                    break;
                case 'D':
                    u.disassemble( locThisItem, false );
                    break;
                case 'f':
                    oThisItem.is_favorite = !oThisItem.is_favorite;
                    if( locThisItem.has_parent() ) {
                        item_location parent = locThisItem.parent_item();
                        item_pocket *const pocket = locThisItem.parent_pocket();
                        if( pocket ) {
                            pocket->restack();
                        } else {
                            debugmsg( "parent container does not contain item" );
                        }
                    }
                    break;
                case 'v':
                    if( oThisItem.is_container() ) {
                        oThisItem.favorite_settings_menu();
                    }
                    break;
                case 'V': {
                    int is_recipe = 0;
                    std::string this_itype = oThisItem.typeId().str();
                    if( oThisItem.is_craft() ) {
                        this_itype = oThisItem.get_making().ident().str();
                        is_recipe = 1;
                    }
                    player_activity recipe_act = player_activity( ACT_VIEW_RECIPE, 0, is_recipe, 0, this_itype );
                    u.assign_activity( recipe_act );
                    break;
                }
                case 'i':
                    if( oThisItem.is_container() ) {
                        game_menus::inv::insert_items( u, locThisItem );
                    }
                    break;
                case 'o':
                    if( oThisItem.is_container() && oThisItem.num_item_stacks() > 0 ) {
                        game_menus::inv::common( locThisItem, u );
                    }
                    break;
                case '=':
                    game_menus::inv::reassign_letter( u, oThisItem );
                    break;
                case KEY_PPAGE:
                    iScrollPos -= iScrollHeight;
                    if( ui ) {
                        ui->invalidate_ui();
                    }
                    break;
                case KEY_NPAGE:
                    iScrollPos += iScrollHeight;
                    if( ui ) {
                        ui->invalidate_ui();
                    }
                    break;
                case '+':
                    if( !bHPR ) {
                        get_auto_pickup().add_rule( &oThisItem, true );
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
                case '<':
                case '>':
                    for( item_pocket *pocket : oThisItem.get_all_standard_pockets() ) {
                        pocket->settings.set_collapse( cMenu == '>' );
                    }
                    break;
                default:
                    break;
            }
        } while( !exit );
    }
    return cMenu;
}

// Checks input to see if mouse was moved and handles the mouse view box accordingly.
// Returns true if input requires breaking out into a game action.
bool game::handle_mouseview( input_context &ctxt, std::string &action )
{
    std::optional<tripoint> liveview_pos;

    do {
        action = ctxt.handle_input();
        if( action == "MOUSE_MOVE" ) {
            const std::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain, ter_view_p.xy(), true );
            if( mouse_pos && ( !liveview_pos || *mouse_pos != *liveview_pos ) ) {
                liveview_pos = mouse_pos;
                liveview.show( *liveview_pos );
            } else if( !mouse_pos ) {
                liveview_pos.reset();
                liveview.hide();
            }
            ui_manager::redraw();
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

std::pair<tripoint, tripoint> game::mouse_edge_scrolling( input_context &ctxt, const int speed,
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
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if( now < last_mouse_edge_scroll + std::chrono::milliseconds( rate ) ) {
        return ret;
    } else {
        last_mouse_edge_scroll = now;
    }
    const input_event event = ctxt.get_raw_input();
    if( event.type == input_event_t::mouse ) {
        const point threshold( projected_window_width() / 100, projected_window_height() / 100 );
        if( event.mouse_pos.x <= threshold.x ) {
            ret.first.x -= speed;
            if( iso ) {
                ret.first.y -= speed;
            }
        } else if( event.mouse_pos.x >= projected_window_width() - threshold.x ) {
            ret.first.x += speed;
            if( iso ) {
                ret.first.y += speed;
            }
        }
        if( event.mouse_pos.y <= threshold.y ) {
            ret.first.y -= speed;
            if( iso ) {
                ret.first.x += speed;
            }
        } else if( event.mouse_pos.y >= projected_window_height() - threshold.y ) {
            ret.first.y += speed;
            if( iso ) {
                ret.first.x -= speed;
            }
        }
        ret.second = ret.first;
    } else if( event.type == input_event_t::timeout ) {
        ret.first = ret.second;
    }
#endif
    return ret;
}

tripoint game::mouse_edge_scrolling_terrain( input_context &ctxt )
{
    auto ret = mouse_edge_scrolling( ctxt, std::max( DEFAULT_TILESET_ZOOM / tileset_zoom, 1 ),
                                     last_mouse_edge_scroll_vector_terrain, g->is_tileset_isometric() );
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
    input_context ctxt( "DEFAULTMODE", keyboard_mode::keycode );
    // Because those keys move the character, they don't pan, as their original name says
    ctxt.set_iso( true );
    ctxt.register_action( "UP", to_translation( "Move north" ) );
    ctxt.register_action( "RIGHTUP", to_translation( "Move northeast" ) );
    ctxt.register_action( "RIGHT", to_translation( "Move east" ) );
    ctxt.register_action( "RIGHTDOWN", to_translation( "Move southeast" ) );
    ctxt.register_action( "DOWN", to_translation( "Move south" ) );
    ctxt.register_action( "LEFTDOWN", to_translation( "Move southwest" ) );
    ctxt.register_action( "LEFT", to_translation( "Move west" ) );
    ctxt.register_action( "LEFTUP", to_translation( "Move northwest" ) );
    ctxt.register_action( "pause" );
    ctxt.register_action( "LEVEL_DOWN", to_translation( "Descend stairs" ) );
    ctxt.register_action( "LEVEL_UP", to_translation( "Ascend stairs" ) );
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
    ctxt.register_action( "cycle_move_reverse" );
    ctxt.register_action( "reset_move" );
    ctxt.register_action( "toggle_run" );
    ctxt.register_action( "toggle_crouch" );
    ctxt.register_action( "toggle_prone" );
    ctxt.register_action( "open_movement" );
    ctxt.register_action( "open" );
    ctxt.register_action( "close" );
    ctxt.register_action( "smash" );
    ctxt.register_action( "loot" );
    ctxt.register_action( "examine" );
    ctxt.register_action( "examine_and_pickup" );
    ctxt.register_action( "advinv" );
    ctxt.register_action( "pickup" );
    ctxt.register_action( "pickup_all" );
    ctxt.register_action( "grab" );
    ctxt.register_action( "haul" );
    ctxt.register_action( "haul_toggle" );
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
    ctxt.register_action( "reload_wielded" );
    ctxt.register_action( "insert" );
    ctxt.register_action( "unload" );
    ctxt.register_action( "throw" );
    ctxt.register_action( "fire" );
    ctxt.register_action( "cast_spell" );
    ctxt.register_action( "fire_burst" );
    ctxt.register_action( "select_fire_mode" );
    ctxt.register_action( "drop" );
    ctxt.register_action( "unload_container" );
    ctxt.register_action( "drop_adj" );
    ctxt.register_action( "bionics" );
    ctxt.register_action( "mutations" );
    ctxt.register_action( "medical" );
    ctxt.register_action( "bodystatus" );
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
    ctxt.register_action( "workout" );
    ctxt.register_action( "save" );
    ctxt.register_action( "quicksave" );
#if !defined(RELEASE)
    ctxt.register_action( "quickload" );
#endif
    ctxt.register_action( "SUICIDE" );
    ctxt.register_action( "player_data" );
    ctxt.register_action( "map" );
    ctxt.register_action( "sky" );
    ctxt.register_action( "missions" );
    ctxt.register_action( "factions" );
    ctxt.register_action( "morale" );
    ctxt.register_action( "messages" );
    ctxt.register_action( "help" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "open_options" );
    ctxt.register_action( "open_autopickup" );
    ctxt.register_action( "open_autonotes" );
    ctxt.register_action( "open_safemode" );
    ctxt.register_action( "open_distraction_manager" );
    ctxt.register_action( "open_color" );
    ctxt.register_action( "open_world_mods" );
    ctxt.register_action( "debug" );
    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "debug_scent_type" );
    ctxt.register_action( "debug_temp" );
    ctxt.register_action( "debug_visibility" );
    ctxt.register_action( "debug_lighting" );
    ctxt.register_action( "debug_radiation" );
    ctxt.register_action( "debug_hour_timer" );
    ctxt.register_action( "debug_mode" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
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
    ctxt.register_action( "toggle_prevent_occlusion" );
    ctxt.register_action( "diary" );
    ctxt.register_action( "action_menu" );
    ctxt.register_action( "main_menu" );
    ctxt.register_action( "item_action_menu" );
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "CLICK_AND_DRAG" );
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
        ( !u.has_active_bionic( bio_remote ) && !u.has_active_item( itype_remotevehcontrol ) ) ) {
        remoteveh_cache = nullptr;
    } else {
        tripoint vp;
        remote_veh_string >> vp.x >> vp.y >> vp.z;
        vehicle *veh = veh_pointer_or_null( m.veh_at( vp ) );
        if( veh && veh->fuel_left( itype_battery ) > 0 ) {
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
        !u.has_active_item( itype_remotevehcontrol ) ) {
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

bool game::try_get_left_click_action( action_id &act, const tripoint_bub_ms &mouse_target )
{
    bool new_destination = true;
    if( !destination_preview.empty() ) {
        auto &final_destination = destination_preview.back();
        if( final_destination.xy() == mouse_target.xy() ) {
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
        const std::optional<std::vector<tripoint_bub_ms>> try_route =
        safe_route_to( u, mouse_target, 0, []( const std::string & msg ) {
            add_msg( msg );
        } );
        if( try_route.has_value() ) {
            destination_preview = *try_route;
            return true;
        }
        return false;
    }

    return true;
}

bool game::try_get_right_click_action( action_id &act, const tripoint_bub_ms &mouse_target )
{
    const bool cleared_destination = !destination_preview.empty();
    u.clear_destination();
    destination_preview.clear();

    if( cleared_destination ) {
        // Produce no-op if auto move had just been cleared on this action
        // e.g. from a previous single left mouse click. This has the effect
        // of right-click canceling an auto move before it is initiated.
        return false;
    }

    const bool is_adjacent = square_dist( mouse_target.xy(), u.pos_bub().xy() ) <= 1;
    const bool is_self = square_dist( mouse_target.xy(), u.pos_bub().xy() ) <= 0;
    if( const monster *const mon = get_creature_tracker().creature_at<monster>( mouse_target ) ) {
        if( !u.sees( *mon ) ) {
            add_msg( _( "Nothing relevant here." ) );
            return false;
        }

        if( !u.get_wielded_item() || !u.get_wielded_item()->is_gun() ) {
            add_msg( m_info, _( "You are not wielding a ranged weapon." ) );
            return false;
        }

        // TODO: Add weapon range check. This requires weapon to be reloaded.

        act = ACTION_FIRE;
    } else if( is_adjacent &&
               m.close_door( tripoint_bub_ms( mouse_target.xy(), u.posz() ),
                             !m.is_outside( u.pos() ), true ) ) {
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
    if( uquit == QUIT_DIED || uquit == QUIT_WATCH ) {
        events().send<event_type::avatar_dies>();
    }
    if( uquit == QUIT_WATCH ) {
        // deny player movement and dodging
        u.moves = 0;
        // prevent pain from updating
        u.set_pain( 0 );
        // prevent dodging
        u.set_dodges_left( 0 );
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
        bury_screen();
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
        effect_on_conditions::prevent_death();
        if( !u.is_dead_state() ) {
            return false;
        }
        bury_screen();
        effect_on_conditions::avatar_death();
        if( !u.is_dead_state() ) {
            return false;
        }
        Messages::deactivate();
        if( get_option<std::string>( "DEATHCAM" ) == "always" ) {
            uquit = QUIT_WATCH;
        } else if( get_option<std::string>( "DEATHCAM" ) == "ask" ) {
            uquit = query_yn( _( "Watch the last moments of your life…?" ) ) ?
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

void game::bury_screen() const
{
    avatar &u = get_avatar();

    std::vector<std::string> vRip;

    int iMaxWidth = 0;
    int iNameLine = 0;
    int iInfoLine = 0;

    if( u.has_amount( itype_holybook_bible1, 1 ) || u.has_amount( itype_holybook_bible2, 1 ) ||
        u.has_amount( itype_holybook_bible3, 1 ) ) {
        if( !( u.has_trait( trait_CANNIBAL ) || u.has_trait( trait_PSYCHOPATH ) ) ) {
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
            iMaxWidth = utf8_width( vRip.back() );
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
            vRip.emplace_back( "         * .%%|          -< @%%%" ); // NOLINT(cata-text-style)
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
            iMaxWidth = utf8_width( vRip.back() );
            vRip.emplace_back( "|                                         |" );
            vRip.emplace_back( "|_____.-._______            __/|__________|" );
            vRip.emplace_back( "             % / `_-.   _  |%" );
            vRip.emplace_back( "         * .%%|  |_) | |_)< @%%%" ); // NOLINT(cata-text-style)
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
        iMaxWidth = utf8_width( vRip.back() );
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

    const point iOffset( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                         TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 );

    catacurses::window w_rip = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                               iOffset );
    draw_border( w_rip );

    sfx::do_player_death_hurt( get_player_character(), true );
    sfx::fade_audio_group( sfx::group::weather, 2000 );
    sfx::fade_audio_group( sfx::group::time_of_day, 2000 );
    sfx::fade_audio_group( sfx::group::context_themes, 2000 );
    sfx::fade_audio_group( sfx::group::fatigue, 2000 );

    for( size_t iY = 0; iY < vRip.size(); ++iY ) {
        size_t iX = 0;
        const char *str = vRip[iY].data();
        for( int slen = vRip[iY].size(); slen > 0; ) {
            const uint32_t cTemp = UTF8_getch( &str, &slen );
            if( cTemp != U' ' ) {
                nc_color ncColor = c_light_gray;

                if( cTemp == U'%' ) {
                    ncColor = c_green;

                } else if( cTemp == U'_' || cTemp == U'|' ) {
                    ncColor = c_white;

                } else if( cTemp == U'@' ) {
                    ncColor = c_brown;

                } else if( cTemp == U'*' ) {
                    ncColor = c_red;
                }

                mvwputch( w_rip, point( iX + FULL_SCREEN_WIDTH / 2 - ( iMaxWidth / 2 ), iY + 1 ), ncColor,
                          cTemp );
            }
            iX += mk_wcwidth( cTemp );
        }
    }

    std::string sTemp;

    center_print( w_rip, iInfoLine++, c_white, _( "Survived:" ) );

    const time_duration survived = calendar::turn - calendar::start_of_game;
    const int minutes = to_minutes<int>( survived ) % 60;
    const int hours = to_hours<int>( survived ) % 24;
    const int days = to_days<int>( survived );

    if( days > 0 ) {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        sTemp = string_format( "%dd %dh %dm", days, hours, minutes );
    } else if( hours > 0 ) {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        sTemp = string_format( "%dh %dm", hours, minutes );
    } else {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        sTemp = string_format( "%dm", minutes );
    }

    center_print( w_rip, iInfoLine++, c_white, sTemp );

    const int iTotalKills = g->get_kill_tracker().monster_kill_count();

    sTemp = _( "Kills:" );
    mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - 5, 1 + iInfoLine++ ), c_light_gray,
               ( sTemp + " " ) );
    wprintz( w_rip, c_magenta, "%d", iTotalKills );

    sTemp = _( "In memory of:" );
    mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ),
               c_light_gray,
               sTemp );

    sTemp = u.get_name();
    mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ), c_white,
               sTemp );

    sTemp = _( "Last Words:" );
    mvwprintz( w_rip, point( FULL_SCREEN_WIDTH / 2 - utf8_width( sTemp ) / 2, iNameLine++ ),
               c_light_gray,
               sTemp );

    int iStartX = FULL_SCREEN_WIDTH / 2 - ( ( iMaxWidth - 4 ) / 2 );
    std::string sLastWords = string_input_popup()
                             .window( w_rip, point( iStartX, iNameLine ), iStartX + iMaxWidth - 4 - 1 )
                             .max_length( iMaxWidth - 4 - 1 )
                             .query_string();

    const bool is_suicide = uquit == QUIT_SUICIDE;
    get_event_bus().send<event_type::game_avatar_death>( u.getID(), u.name, u.male, is_suicide,
            sLastWords );
}

void game::death_screen()
{
    gamemode->game_over();
    Messages::display_messages();
    u.get_avatar_diary()->death_entry();
    show_scores_ui( *achievements_tracker_ptr, stats(), get_kill_tracker() );
    disp_NPC_epilogues();
    follower_ids.clear();
    display_faction_epilogues();
}

// A timestamp that can be used to make unique file names
// Date format is a somewhat ISO-8601 compliant local time date (except we use '-' instead of ':' probably because of Windows file name rules).
// XXX: removed the timezone suffix due to an mxe bug
// See: https://github.com/mxe/mxe/issues/2749
static std::string timestamp_now()
{
    std::time_t time = std::time( nullptr );
    std::stringstream date_buffer;
    date_buffer << std::put_time( std::localtime( &time ), "%Y-%m-%dT%H-%M-%S" );
    return date_buffer.str();
}

void game::move_save_to_graveyard()
{
    const std::string &save_dir      = PATH_INFO::world_base_save_path();
    const std::string graveyard_dir = PATH_INFO::graveyarddir() + "/" + timestamp_now() + "/";
    const std::string &prefix        = base64_encode( u.get_save_id() ) + ".";

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
    const cata_path datafile = PATH_INFO::world_base_save_path_path() / SAVE_MASTER;
    read_from_file_optional( datafile, [this, &datafile]( std::istream & is ) {
        unserialize_master( datafile, is );
    } );
}

bool game::load( const std::string &world )
{
    world_generator->init();
    WORLD *const wptr = world_generator->get_world( world );
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

bool game::load( const save_t &name )
{
    loading_ui ui( true );
    ui.new_context( _( "Loading the save…" ) );

    const cata_path worldpath = PATH_INFO::world_base_save_path_path();
    const cata_path save_file_path = PATH_INFO::world_base_save_path_path() /
                                     ( name.base_path() + SAVE_EXTENSION );

    bool abort = false;

    using named_entry = std::pair<std::string, std::function<void()>>;
    const std::vector<named_entry> entries = {{
            {
                _( "Master save" ), [&]()
                {
                    // Now load up the master game data; factions (and more?)
                    load_master();
                }
            },
            {
                _( "Character save" ), [&]()
                {

                    u = avatar();
                    u.set_save_id( name.decoded_name() );
                    if( !read_from_file(
                            save_file_path,
                    [this, &save_file_path]( std::istream & is ) {
                    unserialize( is, save_file_path );
                    } ) ) {
                        abort = true;
                    }
                }
            },
            {
                _( "Map memory" ), [&]()
                {
                    u.load_map_memory();
                }
            },
            {
                _( "Diary" ), [&]()
                {
                    u.get_avatar_diary()->load();
                }
            },
            {
                _( "Memorial" ), [&]()
                {
                    const cata_path log_filename =
                    worldpath / ( name.base_path() + SAVE_EXTENSION_LOG );
                    read_from_file_optional(
                        log_filename.get_unrelative_path(),
                    [this]( std::istream & is ) {
                        memorial().load( is );
                    } );
                }
            },
            {
                _( "Finalizing" ), [&]()
                {

#if defined(__ANDROID__)
                    const cata_path shortcuts_filename =
                    worldpath / ( name.base_path() + SAVE_EXTENSION_SHORTCUTS );
                    if( file_exist( shortcuts_filename ) ) {
                        load_shortcuts( shortcuts_filename );
                    }
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
                    get_auto_notes_settings().load( true ); // Load character auto notes settings
                    get_safemode().load_character(); // Load character safemode rules
                    zone_manager::get_manager().load_zones(); // Load character world zones
                    read_from_file_optional_json(
                        PATH_INFO::world_base_save_path_path() / "uistate.json",
                    []( const JsonValue & jsin ) {
                        uistate.deserialize( jsin.get_object() );
                    } );
                    reload_npcs();
                    validate_npc_followers();
                    validate_mounted_npcs();
                    validate_camps();
                    validate_linked_vehicles();
                    update_map( u );
                    for( item *&e : u.inv_dump() ) {
                        e->set_owner( get_player_character() );
                    }
                    // legacy, needs to be here as we access the map.
                    if( !u.getID().is_valid() ) {
                        // player does not have a real id, so assign a new one,
                        u.setID( assign_npc_id() );
                        // The vehicle stores the IDs of the boarded players, so update it, too.
                        if( u.in_vehicle ) {
                            if( const std::optional<vpart_reference> vp = m.veh_at(
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

                    calendar::set_eternal_night(
                        ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "night" );
                    calendar::set_eternal_day(
                        ::get_option<std::string>( "ETERNAL_TIME_OF_DAY" ) == "day" );

                    u.reset();
                    u.recalculate_enchantment_cache();
                    u.enchantment_cache->activate_passive( u );
                    events().send<event_type::game_load>( getVersionString() );
                    time_of_last_load = std::chrono::steady_clock::now();
                    time_played_at_last_load = std::chrono::seconds( 0 );
                    std::optional<event_multiset::summaries_type::value_type> last_save =
                        stats().get_events( event_type::game_save ).last();
                    if( last_save ) {
                        auto time_played_it = last_save->first.find( "total_time_played" );
                        if( time_played_it != last_save->first.end() &&
                            time_played_it->second.type() == cata_variant_type::chrono_seconds ) {
                            time_played_at_last_load =
                                time_played_it->second.get<std::chrono::seconds>();
                        }
                    }

                    effect_on_conditions::load_existing_character( u );
                    // recalculate light level for correctly resuming crafting and disassembly
                    m.build_map_cache( m.get_abs_sub().z() );
                }
            },
        }
    };

    for( const named_entry &e : entries ) {
        ui.add_entry( e.first );
    }

    ui.show();
    for( const named_entry &e : entries ) {
        e.second();
        if( abort ) {
            return false;
        }
        ui.proceed();
    }

    return true;
}

void game::load_world_modfiles( loading_ui &ui )
{
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
        mods.insert( mods.begin(), MOD_INFORMATION_dda );
    }

    // this code does not care about mod dependencies,
    // it assumes that those dependencies are static and
    // are resolved during the creation of the world.
    // That means world->active_mod_order contains a list
    // of mods in the correct order.
    load_packs( _( "Loading files" ), mods, ui );

    // Load additional mods from that world-specific folder
    load_data_from_dir( PATH_INFO::world_base_save_path_path() / "mods", "custom", ui );

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
        restore_on_out_of_scope<check_plural_t> restore_check_plural( check_plural );
        if( mod.ident.str() == "test_data" ) {
            check_plural = check_plural_t::none;
        }
        load_data_from_dir( mod.path, mod.ident.str(), ui );

        ui.proceed();
    }

    for( const auto &e : missing ) {
        debugmsg( "unknown content %s", e.c_str() );
    }

    return missing.empty();
}

void game::reset_npc_dispositions()
{
    for( character_id elem : follower_ids ) {
        shared_ptr_fast<npc> npc_to_get = overmap_buffer.find_npc( elem );
        if( !npc_to_get )  {
            continue;
        }
        npc *npc_to_add = npc_to_get.get();
        npc_to_add->chatbin.clear_all();
        npc_to_add->mission = NPC_MISSION_NULL;
        npc_to_add->set_attitude( NPCATT_NULL );
        npc_to_add->op_of_u = npc_opinion();
        npc_to_add->set_fac( faction_no_faction );
        npc_to_add->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC,
                                     npc_to_add->global_omt_location(),
                                     npc_to_add->getID() ) );

    }

}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    std::string masterfile = PATH_INFO::world_base_save_path() + "/" + SAVE_MASTER;
    return write_to_file( masterfile, [&]( std::ostream & fout ) {
        serialize_master( fout );
    }, _( "factions data" ) );
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
    const std::string playerfile = PATH_INFO::player_base_save_path();

    const bool saved_data = write_to_file( playerfile + SAVE_EXTENSION, [&]( std::ostream & fout ) {
        serialize( fout );
    }, _( "player data" ) );
    const bool saved_map_memory = u.save_map_memory();
    const bool saved_log = write_to_file( playerfile + SAVE_EXTENSION_LOG, [&](
    std::ostream & fout ) {
        memorial().save( fout );
    }, _( "player memorial" ) );
#if defined(__ANDROID__)
    const bool saved_shortcuts = write_to_file( playerfile + SAVE_EXTENSION_SHORTCUTS, [&](
    std::ostream & fout ) {
        save_shortcuts( fout );
    }, _( "quick shortcuts" ) );
#endif
    const bool saved_diary = u.get_avatar_diary()->store();
    return saved_data && saved_map_memory && saved_log && saved_diary
#if defined(__ANDROID__)
           && saved_shortcuts
#endif
           ;
}


bool game::save_achievements()
{
    const std::string &achievement_dir = PATH_INFO::achievementdir();

    //Check if achievement dir exists
    if( !assure_dir_exist( achievement_dir ) ) {
        dbg( D_ERROR ) << "game:save_achievements: Unable to make achievement directory.";
        debugmsg( "Could not make '%s' directory", achievement_dir );
        return false;
    }

    // This sets the maximum length for the filename
    constexpr size_t suffix_len = 24 + 1;
    constexpr size_t max_name_len = FILENAME_MAX - suffix_len;

    const size_t name_len = u.name.size();
    // Here -1 leaves space for the ~
    const size_t truncated_name_len = ( name_len >= max_name_len ) ? ( max_name_len - 1 ) : name_len;

    std::ostringstream achievement_file_path;

    achievement_file_path << achievement_dir;

    if( get_options().has_option( "ENCODING_CONV" ) && !get_option<bool>( "ENCODING_CONV" ) ) {
        // Use the default locale to replace non-printable characters with _ in the player name.
        std::locale locale{ "C" };
        std::replace_copy_if( std::begin( u.name ), std::begin( u.name ) + truncated_name_len,
                              std::ostream_iterator<char>( achievement_file_path ),
        [&]( const char c ) {
            return !std::isgraph( c, locale );
        }, '_' );
    } else {
        achievement_file_path << u.name;
    }

    // Add a ~ if the player name was actually truncated.
    achievement_file_path << ( ( truncated_name_len != name_len ) ? "~-" : "-" );
    const int character_id = get_player_character().getID().get_value();
    const std::string json_path_string = achievement_file_path.str() + std::to_string(
            character_id ) + ".json";

    // Clear past achievements so that it will be reloaded
    clear_past_achievements();

    return write_to_file( json_path_string, [&]( std::ostream & fout ) {
        get_achievements().write_json_achievements( fout );
    }, _( "player achievements" ) );

}

event_bus &game::events()
{
    return *event_bus_ptr;
}

stats_tracker &game::stats()
{
    return *stats_tracker_ptr;
}

achievements_tracker &game::achievements()
{
    return *achievements_tracker_ptr;
}

memorial_logger &game::memorial()
{
    return *memorial_logger_ptr;
}

void game::update_unique_npc_location( const std::string &id, point_abs_om loc )
{
    unique_npcs[id] = loc;
}

point_abs_om game::get_unique_npc_location( const std::string &id )
{
    if( unique_npc_exists( id ) ) {
        return unique_npcs[id];
    } else {
        debugmsg( "Tried to find npc %s which doesn't exist.", id );
        return point_abs_om();
    }
}

bool game::unique_npc_exists( const std::string &id )
{
    return unique_npcs.count( id ) > 0;
}

void game::unique_npc_despawn( const std::string &id )
{
    unique_npcs.erase( id );
}

spell_events &game::spell_events_subscriber()
{
    return *spell_events_ptr;
}

bool game::save()
{
    std::chrono::seconds time_since_load =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - time_of_last_load );
    std::chrono::seconds total_time_played = time_played_at_last_load + time_since_load;
    events().send<event_type::game_save>( time_since_load, total_time_played );
    try {
        if( !save_player_data() ||
            !save_achievements() ||
            !save_factions_missions_npcs() ||
            !save_maps() ||
            !get_auto_pickup().save_character() ||
            !get_auto_notes_settings().save( true ) ||
            !get_safemode().save_character() ||
            !zone_manager::get_manager().save_zones() ||
        !write_to_file( PATH_INFO::world_base_save_path() + "/uistate.json", [&]( std::ostream & fout ) {
        JsonOut jsout( fout );
            uistate.serialize( jsout );
        }, _( "uistate data" ) ) ) {
            debugmsg( "game not saved" );
            return false;
        } else {
            world_generator->last_world_name = world_generator->active_world->world_name;
            world_generator->last_character_name = u.name;
            world_generator->save_last_world_info();
            world_generator->active_world->add_save( save_t::from_save_id( u.get_save_id() ) );
            write_to_file( PATH_INFO::world_base_save_path_path() / ( base64_encode(
            u.get_save_id() ) + ".pt" ), [&total_time_played]( std::ostream & fout ) {
                fout.imbue( std::locale::classic() );
                fout << total_time_played.count();
            } );
#if defined(EMSCRIPTEN)
            // This will allow the window to be closed without a prompt, until do_turn()
            // is called.
            EM_ASM( window.game_unsaved = false; );
#endif
            return true;
        }
    } catch( std::ios::failure & ) {
        popup( _( "Failed to save game data" ) );
        return false;
    }
}

std::vector<std::string> game::list_active_saves()
{
    std::vector<std::string> saves;
    for( save_t &worldsave : world_generator->active_world->world_saves ) {
        saves.push_back( worldsave.decoded_name() );
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
    const std::string &memorial_dir = PATH_INFO::memorialdir();
    const std::string &memorial_active_world_dir = memorial_dir +
            world_generator->active_world->world_name + "/";

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
        memorial_file_path << u.name;
    }

    // Add a ~ if the player name was actually truncated.
    memorial_file_path << ( ( truncated_name_len != name_len ) ? "~-" : "-" );

    // Add a timestamp for uniqueness.

#if defined(_WIN32)
    SYSTEMTIME current_time;
    GetLocalTime( &current_time );
    memorial_file_path << string_format( "%d-%02d-%02d-%02d-%02d-%02d",
                                         current_time.wYear, current_time.wMonth, current_time.wDay,
                                         current_time.wHour, current_time.wMinute, current_time.wSecond );
#else
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char buffer[suffix_len] {};
    std::time_t t = std::time( nullptr );
    tm current_time;
    localtime_r( &t, &current_time );
    size_t result = std::strftime( buffer, suffix_len, "%Y-%m-%d-%H-%M-%S", &current_time );
    if( result == 0 ) {
        cata_fatal( "Could not construct filename" );
    }
    memorial_file_path << buffer;
#endif

    const std::string text_path_string = memorial_file_path.str() + ".txt";
    const std::string json_path_string = memorial_file_path.str() + ".json";

    write_to_file( text_path_string, [&]( std::ostream & fout ) {
        memorial().write_text_memorial( fout, sLastWords );
    }, _( "player memorial" ) );

    write_to_file( json_path_string, [&]( std::ostream & fout ) {
        memorial().write_json_memorial( fout );
    }, _( "player memorial" ) );
}

void game::disp_NPC_epilogues()
{
    // TODO: This search needs to be expanded to all NPCs
    for( character_id elem : follower_ids ) {
        shared_ptr_fast<npc> guy = overmap_buffer.find_npc( elem );
        if( !guy ) {
            continue;
        }
        const auto new_win = []() {
            return catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                       point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ),
                                              std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
        };
        scrollable_text( new_win, guy->disp_name(), guy->get_epilogue() );
    }
}

void game::display_faction_epilogues()
{
    for( const auto &elem : faction_manager_ptr->all() ) {
        if( elem.second.known_by_u ) {
            const std::vector<std::string> epilogue = elem.second.epilogue();
            if( !epilogue.empty() ) {
                const auto new_win = []() {
                    return catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                               point( std::max( 0, ( TERMX - FULL_SCREEN_WIDTH ) / 2 ),
                                                      std::max( 0, ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) ) );
                };
                scrollable_text( new_win, elem.second.name,
                                 std::accumulate( epilogue.begin() + 1, epilogue.end(), epilogue.front(),
                []( std::string lhs, const std::string & rhs ) -> std::string {
                    return std::move( lhs ) + "\n" + rhs;
                } ) );
            }
        }
    }
}

struct npc_dist_to_player {
    const tripoint_abs_omt ppos{};
    npc_dist_to_player() : ppos( get_player_character().global_omt_location() ) { }
    // Operator overload required to leverage sort API.
    bool operator()( const shared_ptr_fast<npc> &a,
                     const shared_ptr_fast<npc> &b ) const {
        const tripoint_abs_omt apos = a->global_omt_location();
        const tripoint_abs_omt bpos = b->global_omt_location();
        return square_dist( ppos.xy(), apos.xy() ) <
               square_dist( ppos.xy(), bpos.xy() );
    }
};

void game::disp_NPCs()
{
    const tripoint_abs_omt ppos = u.global_omt_location();
    const tripoint lpos = u.pos();
    const int scan_range = 120;
    std::vector<shared_ptr_fast<npc>> npcs = overmap_buffer.get_npcs_near_player( scan_range );
    std::sort( npcs.begin(), npcs.end(), npc_dist_to_player() );

    catacurses::window w;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        w = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                       TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );
        ui.position_from_window( w );
    } );
    ui.mark_resize();
    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w );
        mvwprintz( w, point_zero, c_white, _( "Your overmap position: %s" ), ppos.to_string() );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w, point( 0, 1 ), c_white, _( "Your local position: %s" ), lpos.to_string() );
        size_t i;
        int static_npc_count = 0;
        for( i = 0; i < npcs.size(); i++ ) {
            if(
                npcs[i]->has_trait( trait_NPC_STARTING_NPC ) || npcs[i]->has_trait( trait_NPC_STATIC_NPC ) ) {
                static_npc_count++;
            }
        }
        mvwprintz( w, point( 0, 2 ), c_white, _( "Total NPCs within %d OMTs: %d.  %d are static NPCs." ),
                   scan_range, npcs.size(), static_npc_count );
        for( i = 0; i < 20 && i < npcs.size(); i++ ) {
            const tripoint_abs_omt apos = npcs[i]->global_omt_location();
            mvwprintz( w, point( 0, i + 4 ), c_white, "%s: %s", npcs[i]->get_name(),
                       apos.to_string() );
        }
        for( const monster &m : all_monsters() ) {
            mvwprintz( w, point( 0, i + 4 ), c_white, "%s: %d, %d, %d", m.name(),
                       m.posx(), m.posy(), m.posz() );
            ++i;
        }
        wnoutrefresh( w );
    } );

    input_context ctxt( "DISP_NPCS" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    bool stop = false;
    while( !stop ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "CONFIRM" || action == "QUIT" ) {
            stop = true;
        }
    }
}

// A little helper to draw footstep glyphs.
static void draw_footsteps( const catacurses::window &window, const tripoint &offset )
{
    for( const tripoint &footstep : sounds::get_footstep_markers() ) {
        char glyph = '?';
        if( footstep.z != offset.z ) { // Here z isn't an offset, but a coordinate
            glyph = footstep.z > offset.z ? '^' : 'v';
        }

        mvwputch( window, footstep.xy() + offset.xy(), c_yellow, glyph );
    }
}

shared_ptr_fast<ui_adaptor> game::create_or_get_main_ui_adaptor()
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( !ui ) {
        main_ui_adaptor = ui = make_shared_fast<ui_adaptor>();
        ui->on_redraw( []( ui_adaptor & ui ) {
            g->draw( ui );
        } );
        ui->on_screen_resize( [this]( ui_adaptor & ui ) {
            // remove some space for the sidebar, this is the maximal space
            // (using standard font) that the terrain window can have
            const int sidebar_left = panel_manager::get_manager().get_width_left();
            const int sidebar_right = panel_manager::get_manager().get_width_right();

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
             * The code here calculates size available for w_terrain, caps it at
             * max_view_size (the maximal view range than any character can have at
             * any time).
             * It is stored in TERRAIN_WINDOW_*.
             */
            to_map_font_dimension( TERRAIN_WINDOW_WIDTH, TERRAIN_WINDOW_HEIGHT );

            // Position of the player in the terrain window, it is always in the center
            POSX = TERRAIN_WINDOW_WIDTH / 2;
            POSY = TERRAIN_WINDOW_HEIGHT / 2;

            w_terrain = w_terrain_ptr = catacurses::newwin( TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH,
                                        point( sidebar_left, 0 ) );

            // minimap is always MINIMAP_WIDTH x MINIMAP_HEIGHT in size
            w_minimap = w_minimap_ptr = catacurses::newwin( MINIMAP_HEIGHT, MINIMAP_WIDTH, point_zero );

            // need to init in order to avoid crash. gets updated by the panel code.
            w_pixel_minimap = catacurses::newwin( 1, 1, point_zero );

            ui.position_from_window( catacurses::stdscr );
        } );
        ui->mark_resize();
    }
    return ui;
}

void game::invalidate_main_ui_adaptor() const
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( ui ) {
        ui->invalidate_ui();
    }
}

void game::mark_main_ui_adaptor_resize() const
{
    shared_ptr_fast<ui_adaptor> ui = main_ui_adaptor.lock();
    if( ui ) {
        ui->mark_resize();
    }
}

game::draw_callback_t::draw_callback_t( const std::function<void()> &cb )
    : cb( cb )
{
}

game::draw_callback_t::~draw_callback_t()
{
    if( added ) {
        g->invalidate_main_ui_adaptor();
    }
}

void game::draw_callback_t::operator()()
{
    if( cb ) {
        cb();
    }
}

void game::add_draw_callback( const shared_ptr_fast<draw_callback_t> &cb )
{
    draw_callbacks.erase(
        std::remove_if( draw_callbacks.begin(), draw_callbacks.end(),
    []( const weak_ptr_fast<draw_callback_t> &cbw ) {
        return cbw.expired();
    } ),
    draw_callbacks.end()
    );
    draw_callbacks.emplace_back( cb );
    cb->added = true;
    invalidate_main_ui_adaptor();
}

static void draw_trail( const tripoint &start, const tripoint &end, bool bDrawX );

static shared_ptr_fast<game::draw_callback_t> create_zone_callback(
    const std::optional<tripoint> &zone_start,
    const std::optional<tripoint> &zone_end,
    const bool &zone_blink,
    const bool &zone_cursor,
    const bool &is_moving_zone = false
)
{
    return make_shared_fast<game::draw_callback_t>(
    [&]() {
        if( zone_cursor ) {
            if( is_moving_zone ) {
                g->draw_cursor( ( zone_start.value() + zone_end.value() ) / 2 );
            } else {
                if( zone_end ) {
                    g->draw_cursor( zone_end.value() );
                } else if( zone_start ) {
                    g->draw_cursor( zone_start.value() );
                }
            }
        }
        if( zone_blink && zone_start && zone_end ) {
            avatar &player_character = get_avatar();
            const point offset2( player_character.view_offset.xy() +
                                 point( player_character.posx() - getmaxx( g->w_terrain ) / 2,
                                        player_character.posy() - getmaxy( g->w_terrain ) / 2 ) );

            tripoint offset;
#if defined(TILES)
            if( use_tiles ) {
                offset = tripoint_zero; //TILES
            } else {
#endif
                offset = tripoint( offset2, 0 ); //CURSES
#if defined(TILES)
            }
#endif

            const tripoint start( std::min( zone_start->x, zone_end->x ),
                                  std::min( zone_start->y, zone_end->y ),
                                  zone_end->z );
            const tripoint end( std::max( zone_start->x, zone_end->x ),
                                std::max( zone_start->y, zone_end->y ),
                                zone_end->z );
            g->draw_zones( start, end, offset );
        }
    } );
}

static shared_ptr_fast<game::draw_callback_t> create_trail_callback(
    const std::optional<tripoint> &trail_start,
    const std::optional<tripoint> &trail_end,
    const bool &trail_end_x
)
{
    return make_shared_fast<game::draw_callback_t>(
    [&]() {
        if( trail_start && trail_end ) {
            draw_trail( trail_start.value(), trail_end.value(), trail_end_x );
        }
    } );
}

void game::init_draw_async_anim_curses( const tripoint &p, const std::string &ncstr,
                                        const nc_color &nccol )
{
    std::pair <std::string, nc_color> anim( ncstr, nccol );
    async_anim_layer_curses[p] = anim;
}

void game::draw_async_anim_curses()
{
    // game::draw_async_anim_curses can be called multiple times, storing each animation to be played in async_anim_layer_curses
    // Iterate through every animation in async_anim_layer
    for( const auto &anim : async_anim_layer_curses ) {
        const tripoint p = anim.first - u.view_offset + tripoint( POSX - u.posx(), POSY - u.posy(),
                           -u.posz() );
        const std::string ncstr = anim.second.first;
        const nc_color nccol = anim.second.second;

        mvwprintz( w_terrain, p.xy(), nccol, ncstr );
    }
}

void game::void_async_anim_curses()
{
    async_anim_layer_curses.clear();
}

void game::init_draw_blink_curses( const tripoint &p, const std::string &ncstr,
                                   const nc_color &nccol )
{
    std::pair <std::string, nc_color> anim( ncstr, nccol );
    blink_layer_curses[p] = anim;
}

void game::draw_blink_curses()
{
    // game::draw_blink_curses can be called multiple times, storing each animation to be played in blink_layer_curses
    // Iterate through every animation in async_anim_layer
    for( const auto &anim : blink_layer_curses ) {
        const tripoint p = anim.first - u.view_offset + tripoint( POSX - u.posx(), POSY - u.posy(),
                           -u.posz() );
        const std::string ncstr = anim.second.first;
        const nc_color nccol = anim.second.second;

        mvwprintz( w_terrain, p.xy(), nccol, ncstr );
    }
}

void game::void_blink_curses()
{
    blink_layer_curses.clear();
}

bool game::has_blink_curses()
{
    return !blink_layer_curses.empty();
}

void game::draw( ui_adaptor &ui )
{
    if( test_mode ) {
        return;
    }

    ter_view_p.z = ( u.pos() + u.view_offset ).z;
    m.build_map_cache( ter_view_p.z );
    m.update_visibility_cache( ter_view_p.z );

    werase( w_terrain );
    void_blink_curses();
    draw_ter();
    for( auto it = draw_callbacks.begin(); it != draw_callbacks.end(); ) {
        shared_ptr_fast<draw_callback_t> cb = it->lock();
        if( cb ) {
            ( *cb )();
            ++it;
        } else {
            it = draw_callbacks.erase( it );
        }
    }
    draw_async_anim_curses();
    // Only draw blinking symbols when in active phase
    if( blink_active_phase ) {
        draw_blink_curses();
    }
    wnoutrefresh( w_terrain );

    draw_panels( true );

    // Ensure that the cursor lands on the character when everything is drawn.
    // This allows screen readers to describe the area around the player, making it
    // much easier to play with them
    // (e.g. for blind players)
    ui.set_cursor( w_terrain, -u.view_offset.xy() + point( POSX, POSY ) );
}

void game::draw_panels( bool force_draw )
{
    static int previous_turn = -1;
    const int current_turn = to_turns<int>( calendar::turn - calendar::turn_zero );
    const bool draw_this_turn = current_turn > previous_turn || force_draw;
    panel_manager &mgr = panel_manager::get_manager();
    int y = 0;
    const bool sidebar_right = get_option<std::string>( "SIDEBAR_POSITION" ) == "right";
    int spacer = get_option<bool>( "SIDEBAR_SPACERS" ) ? 1 : 0;
    // Total up height used by all panels, and see what is left over for log
    int log_height = 0;
    for( const window_panel &panel : mgr.get_current_layout().panels() ) {
        // Skip height processing
        if( !panel.toggle || !panel.render() ) {
            continue;
        }
        // The panel with height -2 is the message log panel
        const int p_height = panel.get_height();
        if( p_height != -2 ) {
            log_height += p_height + spacer;
        }
    }
    log_height = std::max( TERMY - log_height, 3 );
    // Draw each panel having render() true
    for( const window_panel &panel : mgr.get_current_layout().panels() ) {
        if( panel.render() ) {
            // height clamped to window height.
            int h = std::min( panel.get_height(), TERMY - y );
            // The panel with height -2 is the message log panel
            if( h == -2 ) {
                h = log_height;
            }
            h += spacer;
            if( panel.toggle && panel.render() && h > 0 ) {
                if( panel.always_draw || draw_this_turn ) {
                    catacurses::window w = catacurses::newwin( h, panel.get_width(),
                                           point( sidebar_right ? TERMX - panel.get_width() : 0, y ) );
                    int tmp_h = panel.draw( { u, w, panel.get_widget() } );
                    h += tmp_h;
                    // lines skipped for rendering -> reclaim space in the sidebar
                    if( tmp_h < 0 ) {
                        y += h;
                        log_height -= tmp_h;
                        continue;
                    }
                }
                if( show_panel_adm ) {
                    const std::string panel_name = panel.get_name();
                    const int panel_name_width = utf8_width( panel_name );
                    catacurses::window label = catacurses::newwin( 1, panel_name_width, point( sidebar_right ?
                                               TERMX - panel.get_width() - panel_name_width - 1 : panel.get_width() + 1, y ) );
                    werase( label );
                    mvwprintz( label, point_zero, c_light_red, panel_name );
                    wnoutrefresh( label );
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
                    wnoutrefresh( label );
                }
                y += h;
            }
        }
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
    if( critter.posz() != center.z ) {
        static constexpr tripoint up_tripoint( tripoint_above );
        if( critter.posz() == center.z - 1 &&
            ( debug_mode || u.sees( critter ) ) &&
            m.valid_move( critter.pos(), critter.pos() + up_tripoint, false, true ) ) {
            // Monster is below
            // TODO: Make this show something more informative than just green 'v'
            // TODO: Allow looking at this mon with look command
            init_draw_blink_curses( tripoint( critter.pos().xy(), center.z ), "v", c_green_cyan );
        }
        if( critter.posz() == center.z + 1 &&
            ( debug_mode || u.sees( critter ) ) &&
            m.valid_move( critter.pos(), critter.pos() + tripoint_below, false, true ) ) {
            // Monster is above
            init_draw_blink_curses( tripoint( critter.pos().xy(), center.z ), "^", c_green_cyan );
        }
        return;
    }
    if( u.sees( critter ) || &critter == &u ) {
        critter.draw( w_terrain, center.xy(), false );
        return;
    }

    if( u.sees_with_infrared( critter ) || u.sees_with_specials( critter ) ) {
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
    draw_ter( u.pos() + u.view_offset, is_looking,
              draw_sounds );
}

void game::draw_ter( const tripoint &center, const bool looking, const bool draw_sounds )
{
    ter_view_p = center;

    m.draw( w_terrain, center );

    if( draw_sounds ) {
        draw_footsteps( w_terrain, tripoint( -center.x, -center.y, center.z ) + point( POSX, POSY ) );
    }

    for( Creature &critter : all_creatures() ) {
        draw_critter( critter, center );
    }

    if( !destination_preview.empty() && u.view_offset.z == 0 ) {
        // Draw auto move preview trail
        const tripoint_bub_ms &final_destination = destination_preview.back();
        tripoint_bub_ms line_center = u.pos_bub() + u.view_offset;
        draw_line( final_destination, line_center, destination_preview, true );
        // TODO: fix point types
        mvwputch( w_terrain,
                  final_destination.xy().raw() - u.view_offset.xy() +
                  point( POSX - u.posx(), POSY - u.posy() ), c_white, 'X' );
    }

    if( u.controlling_vehicle && !looking ) {
        draw_veh_dir_indicator( false );
        draw_veh_dir_indicator( true );
    }
}

std::optional<tripoint> game::get_veh_dir_indicator_location( bool next ) const
{
    if( !get_option<bool>( "VEHICLE_DIR_INDICATOR" ) ) {
        return std::nullopt;
    }
    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( !vp ) {
        return std::nullopt;
    }
    vehicle *const veh = &vp->vehicle();
    rl_vec2d face = next ? veh->dir_vec() : veh->face_vec();
    float r = 10.0f;
    return tripoint( static_cast<int>( r * face.x ), static_cast<int>( r * face.y ), u.pos().z );
}

void game::draw_veh_dir_indicator( bool next )
{
    if( const std::optional<tripoint> indicator_offset = get_veh_dir_indicator_location( next ) ) {
        nc_color col = next ? c_white : c_dark_gray;
        mvwputch( w_terrain, indicator_offset->xy() - u.view_offset.xy() + point( POSX, POSY ), col, 'X' );
    }
}

void game::draw_minimap()
{

    // Draw the box
    werase( w_minimap );
    draw_border( w_minimap );

    const tripoint_abs_omt curs = u.global_omt_location();
    const point_abs_omt curs2( curs.xy() );
    const tripoint_abs_omt targ = u.get_active_mission_target();
    bool drew_mission = targ == overmap::invalid_tripoint;

    const int levz = m.get_abs_sub().z();
    for( int i = -2; i <= 2; i++ ) {
        for( int j = -2; j <= 2; j++ ) {
            const point_abs_omt om( curs2 + point( i, j ) );
            nc_color ter_color;
            tripoint_abs_omt omp( om, levz );
            std::string ter_sym;
            const bool seen = overmap_buffer.seen( omp );
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
            } else if( overmap_buffer.has_vehicle( omp ) ) {
                ter_color = c_cyan;
                ter_sym = overmap_buffer.get_vehicle_ter_sym( omp );
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
        double slope = curs2.x() != targ.x() ?
                       static_cast<double>( targ.y() - curs2.y() ) / ( targ.x() - curs2.x() ) : 4;

        if( curs2.x() == targ.x() || std::fabs( slope ) > 3.5 ) { // Vertical slope
            if( targ.y() > curs2.y() ) {
                mvwputch( w_minimap, point( 3, 6 ), c_red, "*" );
            } else {
                mvwputch( w_minimap, point( 3, 0 ), c_red, "*" );
            }
        } else {
            point arrow( point_north_west );
            if( std::fabs( slope ) >= 1. ) { // y diff is bigger!
                arrow.y = targ.y() > curs2.y() ? 6 : 0;
                arrow.x =
                    static_cast<int>( 3 + 3 * ( targ.y() > curs2.y() ? slope : ( 0 - slope ) ) );
                if( arrow.x < 0 ) {
                    arrow.x = 0;
                }
                if( arrow.x > 6 ) {
                    arrow.x = 6;
                }
            } else {
                arrow.x = targ.x() > curs2.x() ? 6 : 0;
                arrow.y = static_cast<int>( 3 + 3 * ( targ.x() > curs2.x() ? slope : -slope ) );
                if( arrow.y < 0 ) {
                    arrow.y = 0;
                }
                if( arrow.y > 6 ) {
                    arrow.y = 6;
                }
            }
            char glyph = '*';
            if( targ.z() > u.posz() ) {
                glyph = '^';
            } else if( targ.z() < u.posz() ) {
                glyph = 'v';
            }

            mvwputch( w_minimap, arrow, c_red, glyph );
        }
    }

    Character &player_character = get_player_character();
    const int sight_points = player_character.overmap_sight_range( g->light_level(
                                 player_character.posz() ) );
    for( int i = -3; i <= 3; i++ ) {
        for( int j = -3; j <= 3; j++ ) {
            if( i > -3 && i < 3 && j > -3 && j < 3 ) {
                continue; // only do hordes on the border, skip inner map
            }
            const tripoint_abs_omt omp( curs2 + point( i, j ), levz );
            if( overmap_buffer.get_horde_size( omp ) >= HORDE_VISIBILITY_SIZE ) {
                if( overmap_buffer.seen( omp )
                    && player_character.overmap_los( omp, sight_points ) ) {
                    mvwputch( w_minimap, point( i + 3, j + 3 ), c_green,
                              overmap_buffer.get_horde_size( omp ) > HORDE_VISIBILITY_SIZE * 2 ? 'Z' : 'z' );
                }
            }
        }
    }

    wnoutrefresh( w_minimap );
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
    ret = sun_moon_light_at( calendar::turn );
    ret += get_weather().weather_id->light_modifier;

    // Artifact light level changes here. Even though some of these only have an effect
    // aboveground it is cheaper performance wise to simply iterate through the entire
    // list once instead of twice.
    float mod_ret = -1;
    // Each artifact change does std::max(mod_ret, new val) since a brighter end value
    // will trump a lower one.
    if( const timed_event *e = timed_events.get( timed_event_type::DIM ) ) {
        // timed_event_type::DIM slowly dims the natural sky level, then relights it.
        const time_duration left = e->when - calendar::turn;
        // timed_event_type::DIM has an occurrence date of turn + 50, so the first 25 dim it,
        if( left > 25_turns ) {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( left - 25_turns ) ) / 25_turns );
            // and the last 25 scale back towards normal.
        } else {
            mod_ret = std::max( static_cast<double>( mod_ret ), ( ret * ( 25_turns - left ) ) / 25_turns );
        }
    }
    if( timed_events.queued( timed_event_type::ARTIFACT_LIGHT ) ) {
        // timed_event_type::ARTIFACT_LIGHT causes everywhere to become as bright as day.
        mod_ret = std::max<float>( ret, default_daylight_level() );
    }
    if( const timed_event *e = timed_events.get( timed_event_type::CUSTOM_LIGHT_LEVEL ) ) {
        mod_ret = e->strength;
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

Creature *game::is_hostile_very_close( bool dangerous )
{
    return is_hostile_within( DANGEROUS_PROXIMITY, dangerous );
}

Creature *game::is_hostile_within( int distance, bool dangerous )
{
    for( Creature *&critter : u.get_visible_creatures( distance ) ) {
        if( u.attitude_to( *critter ) == Creature::Attitude::HOSTILE ) {
            if( dangerous ) {
                if( critter->is_ranged_attacker() ) {
                    return critter;
                }

                const pathfinding_settings pf_settings = pathfinding_settings{ 8, distance, distance * 2, 4, true, true, false, true, false, false };
                static const std::unordered_set<tripoint> path_avoid = {};

                if( !get_map().route( u.pos(), critter->pos(), pf_settings, path_avoid ).empty() ) {
                    return critter;
                }
                continue;
            }
            return critter;
        }
    }

    return nullptr;
}

field_entry *game::is_in_dangerous_field()
{
    map &here = get_map();
    for( std::pair<const field_type_id, field_entry> &field : here.field_at( u.pos() ) ) {
        if( u.is_dangerous_field( field.second ) ) {
            return &field.second;
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

    const inclusive_cuboid<tripoint> fishing_boundaries(
        fishing_boundary_min, fishing_boundary_max );

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
            if( !fishing_boundaries.contains( current_point ) ) {
                continue;
            }

            // Mark this point as visited.
            visited.emplace( current_point );

            if( m.has_flag( ter_furn_flag::TFLAG_FISHABLE, current_point ) ) {
                fishable_terrain.emplace( current_point );
                to_check.push( current_point + point_south );
                to_check.push( current_point + point_north );
                to_check.push( current_point + point_east );
                to_check.push( current_point + point_west );
            }
        }
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
        if( critter.has_flag( mon_flag_FISHABLE ) ) {
            const tripoint critter_pos = critter.pos();
            // ...and it is in a fishable location.
            if( fishable_locations.find( critter_pos ) != fishable_locations.end() ) {
                unique_fish.push_back( &critter );
            }
        }
    }

    return unique_fish;
}

void game::mon_info_update( )
{
    int newseen = 0;
    const int safe_proxy_dist = get_option<int>( "SAFEMODEPROXIMITY" );
    const int iProxyDist = ( safe_proxy_dist <= 0 ) ? MAX_VIEW_DISTANCE :
                           safe_proxy_dist;

    monster_visible_info &mon_visible = u.get_mon_visible();
    auto &new_seen_mon = mon_visible.new_seen_mon;
    auto &unique_types = mon_visible.unique_types;
    auto &unique_mons = mon_visible.unique_mons;
    auto &dangerous = mon_visible.dangerous;
    mon_visible.has_dangerous_creature_in_proximity = false;

    // 7 0 1    unique_types uses these indices;
    // 6 8 2    0-7 are provide by direction_from()
    // 5 4 3    8 is used for local monsters (for when we explain them below)
    for( auto &t : unique_types ) {
        t.clear();
    }
    for( auto &m : unique_mons ) {
        m.clear();
    }
    std::fill( dangerous.begin(), dangerous.end(), false );

    const tripoint view = u.pos() + u.view_offset;
    new_seen_mon.clear();

    static time_point previous_turn = calendar::turn_zero;
    const time_duration sm_ignored_turns =
        time_duration::from_turns( get_option<int>( "SAFEMODEIGNORETURNS" ) );

    for( Creature *c : u.get_visible_creatures( MAPSIZE_X ) ) {
        monster *m = dynamic_cast<monster *>( c );
        npc *p = dynamic_cast<npc *>( c );
        const direction dir_to_mon = direction_from( view.xy(), point( c->posx(), c->posy() ) );
        const point m2( -view.xy() + point( POSX + c->posx(), POSY + c->posy() ) );
        int index = 8;
        if( !is_valid_in_w_terrain( m2 ) ) {
            // for compatibility with old code, see diagram below, it explains the values for index,
            // also might need revisiting one z-levels are in.
            switch( dir_to_mon ) {
                case direction::ABOVENORTHWEST:
                case direction::NORTHWEST:
                case direction::BELOWNORTHWEST:
                    index = 7;
                    break;
                case direction::ABOVENORTH:
                case direction::NORTH:
                case direction::BELOWNORTH:
                    index = 0;
                    break;
                case direction::ABOVENORTHEAST:
                case direction::NORTHEAST:
                case direction::BELOWNORTHEAST:
                    index = 1;
                    break;
                case direction::ABOVEWEST:
                case direction::WEST:
                case direction::BELOWWEST:
                    index = 6;
                    break;
                case direction::ABOVECENTER:
                case direction::CENTER:
                case direction::BELOWCENTER:
                    index = 8;
                    break;
                case direction::ABOVEEAST:
                case direction::EAST:
                case direction::BELOWEAST:
                    index = 2;
                    break;
                case direction::ABOVESOUTHWEST:
                case direction::SOUTHWEST:
                case direction::BELOWSOUTHWEST:
                    index = 5;
                    break;
                case direction::ABOVESOUTH:
                case direction::SOUTH:
                case direction::BELOWSOUTH:
                    index = 4;
                    break;
                case direction::ABOVESOUTHEAST:
                case direction::SOUTHEAST:
                case direction::BELOWSOUTHEAST:
                    index = 3;
                    break;
                case direction::last:
                    cata_fatal( "invalid direction" );
            }
        }

        bool need_processing = false;
        const bool safemode_empty = get_safemode().empty();

        if( m != nullptr ) {
            //Safemode monster check
            monster &critter = *m;

            const monster_attitude matt = critter.attitude( &u );
            const int mon_dist = rl_dist( u.pos(), critter.pos() );
            if( !safemode_empty ) {
                need_processing = get_safemode().check_monster(
                                      critter.name(),
                                      critter.attitude_to( u ),
                                      mon_dist,
                                      u.controlling_vehicle ) == rule_state::BLACKLISTED;
            } else {
                need_processing =  MATT_ATTACK == matt || MATT_FOLLOW == matt;
            }
            if( need_processing ) {
                if( index < 8 && critter.sees( get_player_character() ) ) {
                    dangerous[index] = true;
                }

                if( !safemode_empty || mon_dist <= iProxyDist ) {
                    bool passmon = false;
                    mon_visible.has_dangerous_creature_in_proximity = true;
                    if( critter.ignoring > 0 ) {
                        if( safe_mode != SAFE_MODE_ON ) {
                            critter.ignoring = 0;
                        } else if( ( sm_ignored_turns == time_duration() ||
                                     ( critter.lastseen_turn &&
                                       *critter.lastseen_turn > calendar::turn - sm_ignored_turns ) ) &&
                                   ( mon_dist > critter.ignoring / 2 || mon_dist < 6 ) ) {
                            passmon = true;
                        }
                        critter.lastseen_turn = calendar::turn;
                    }

                    if( !passmon ) {
                        newseen++;
                        new_seen_mon.push_back( shared_from( critter ) );
                    }
                }
            }

            std::vector<std::pair<const mtype *, int>> &vec = unique_mons[index];
            const auto mon_it = std::find_if( vec.begin(), vec.end(),
            [&]( const std::pair<const mtype *, int> &elem ) {
                return elem.first == critter.type;
            } );
            if( mon_it == vec.end() ) {
                vec.emplace_back( critter.type, 1 );
            } else {
                mon_it->second++;
            }
        } else if( p != nullptr ) {
            //Safe mode NPC check

            const int npc_dist = rl_dist( u.pos(), p->pos() );
            if( !safemode_empty ) {
                need_processing = get_safemode().check_monster(
                                      get_safemode().npc_type_name(),
                                      p->attitude_to( u ),
                                      npc_dist,
                                      u.controlling_vehicle ) == rule_state::BLACKLISTED ;
            } else {
                need_processing = npc_dist <= iProxyDist &&
                                  p->get_attitude() == NPCATT_KILL;
            }
            if( need_processing ) {
                mon_visible.has_dangerous_creature_in_proximity = true;
                newseen++;
            }
            unique_types[index].push_back( p );
        }
    }

    if( uistate.distraction_hostile_spotted && newseen > mostseen ) {
        if( newseen - mostseen == 1 ) {
            if( !new_seen_mon.empty() ) {
                monster &critter = *new_seen_mon.back();
                cancel_activity_or_ignore_query( distraction_type::hostile_spotted_far,
                                                 string_format( _( "%s spotted!" ), critter.name() ) );
                if( u.has_trait( trait_M_DEFENDER ) && critter.type->in_species( species_PLANT ) ) {
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
                cancel_activity_or_ignore_query( distraction_type::hostile_spotted_far,
                                                 _( "Hostile survivor spotted!" ) );
            }
        } else {
            cancel_activity_or_ignore_query( distraction_type::hostile_spotted_far, _( "Monsters spotted!" ) );
        }
        turnssincelastmon = 0_turns;
        if( safe_mode == SAFE_MODE_ON ) {
            set_safe_mode( SAFE_MODE_STOP );
        }
    } else if( calendar::turn > previous_turn && get_option<bool>( "AUTOSAFEMODE" ) &&
               newseen == 0 ) { // Auto safe mode, but only if it's a new turn
        turnssincelastmon += calendar::turn - previous_turn;
        time_duration auto_safe_mode =
            time_duration::from_turns( get_option<int>( "AUTOSAFEMODETURNS" ) );
        if( turnssincelastmon >= auto_safe_mode && safe_mode == SAFE_MODE_OFF ) {
            set_safe_mode( SAFE_MODE_ON );
            add_msg( m_info, _( "Safe mode ON!" ) );
        }
    }

    if( newseen == 0 && safe_mode == SAFE_MODE_STOP ) {
        set_safe_mode( SAFE_MODE_ON );
    }

    previous_turn = calendar::turn;
    mostseen = newseen;
}

void game::cleanup_dead()
{
    // Dead monsters need to stay in the tracker until everything else that needs to die does so
    // This is because dying monsters can still interact with other dying monsters (@ref Creature::killer)
    bool monster_is_dead = critter_tracker->kill_marked_for_death();

    bool npc_is_dead = false;
    // can't use all_npcs as that does not include dead ones
    for( const auto &n : critter_tracker->active_npc ) {
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
        for( auto it = critter_tracker->active_npc.begin(); it != critter_tracker->active_npc.end(); ) {
            if( ( *it )->is_dead() ) {
                get_avatar().get_mon_visible().remove_npc( ( *it ).get() );
                remove_npc_follower( ( *it )->getID() );
                overmap_buffer.remove_npc( ( *it )->getID() );
                it = critter_tracker->active_npc.erase( it );
            } else {
                it++;
            }
        }
    }

    critter_died = false;
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

    knockback( traj, stun, dam_mult );
}

/* Knockback target at traj.front() along line traj; traj should already have considered knockback distance.
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( std::vector<tripoint> &traj, int stun, int dam_mult )
{
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    tripoint tp = traj.front();
    creature_tracker &creatures = get_creature_tracker();
    if( !creatures.creature_at( tp ) ) {
        debugmsg( _( "Nothing at (%d,%d,%d) to knockback!" ), tp.x, tp.y, tp.z );
        return;
    }
    std::size_t force_remaining = traj.size();
    if( monster *const targ = creatures.creature_at<monster>( tp, true ) ) {
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
                    targ->apply_damage( nullptr, bodypart_id( "torso" ), dam_mult * force_remaining );
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    add_msg( _( "%s was stunned!" ), targ->name() );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                if( creatures.creature_at<monster>( traj.front() ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->name() );
                } else if( npc *const guy = creatures.creature_at<npc>( traj.front() ) ) {
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
                knockback( traj, stun, dam_mult );
                break;
            }
            targ->setpos( traj[i] );
            if( m.has_flag( ter_furn_flag::TFLAG_LIQUID, targ->pos() ) && !targ->can_drown() &&
                !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s drowns!" ), targ->name() );
                }
            }
            if( !m.has_flag( ter_furn_flag::TFLAG_LIQUID, targ->pos() ) && targ->has_flag( mon_flag_AQUATIC ) &&
                !targ->is_dead() ) {
                targ->die( nullptr );
                if( u.sees( *targ ) ) {
                    add_msg( _( "The %s flops around and dies!" ), targ->name() );
                }
            }
        }
    } else if( npc *const targ = creatures.creature_at<npc>( tp ) ) {
        if( stun > 0 ) {
            targ->add_effect( effect_stunned, 1_turns * stun );
            add_msg( _( "%s was stunned!" ), targ->get_name() );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( m.impassable( traj[i].xy() ) ) { // oops, we hit a wall!
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    if( targ->has_effect( effect_stunned ) ) {
                        add_msg( _( "%s was stunned!" ), targ->get_name() );
                    }

                    std::array<bodypart_id, 8> bps = {{
                            bodypart_id( "head" ),
                            bodypart_id( "arm_l" ), bodypart_id( "arm_r" ),
                            bodypart_id( "hand_l" ), bodypart_id( "hand_r" ),
                            bodypart_id( "torso" ),
                            bodypart_id( "leg_l" ), bodypart_id( "leg_r" )
                        }
                    };
                    for( const bodypart_id &bp : bps ) {
                        if( one_in( 2 ) ) {
                            targ->deal_damage( nullptr, bp, damage_instance( damage_bash, force_remaining * dam_mult ) );
                        }
                    }
                    targ->check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                targ->setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    add_msg( _( "%s was stunned!" ), targ->get_name() );
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                const tripoint &traj_front = traj.front();
                if( creatures.creature_at<monster>( traj_front ) ) {
                    add_msg( _( "%s collided with something else and sent it flying!" ),
                             targ->get_name() );
                } else if( npc *const guy = creatures.creature_at<npc>( traj_front ) ) {
                    if( guy->male ) {
                        add_msg( _( "%s collided with someone else and sent him flying!" ),
                                 targ->get_name() );
                    } else {
                        add_msg( _( "%s collided with someone else and sent her flying!" ),
                                 targ->get_name() );
                    }
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y &&
                           u.has_trait( trait_LEG_TENT_BRACE ) && u.is_barefoot() ) {
                    add_msg( _( "%s collided with you, and barely dislodges your tentacles!" ), targ->get_name() );
                } else if( u.posx() == traj_front.x && u.posy() == traj_front.y ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->get_name() );
                }
                knockback( traj, stun, dam_mult );
                break;
            }
            targ->setpos( traj[i] );
        }
    } else if( u.pos() == tp ) {
        if( stun > 0 ) {
            u.add_effect( effect_stunned, 1_turns * stun );
            add_msg( m_bad, n_gettext( "You were stunned for %d turn!",
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
                        add_msg( m_bad, n_gettext( "You were stunned AGAIN for %d turn!",
                                                   "You were stunned AGAIN for %d turns!",
                                                   force_remaining ),
                                 force_remaining );
                    } else {
                        add_msg( m_bad, n_gettext( "You were stunned for %d turn!",
                                                   "You were stunned for %d turns!",
                                                   force_remaining ),
                                 force_remaining );
                    }
                    u.add_effect( effect_stunned, 1_turns * force_remaining );
                    std::array<bodypart_id, 8> bps = {{
                            bodypart_id( "head" ),
                            bodypart_id( "arm_l" ), bodypart_id( "arm_r" ),
                            bodypart_id( "hand_l" ), bodypart_id( "hand_r" ),
                            bodypart_id( "torso" ),
                            bodypart_id( "leg_l" ), bodypart_id( "leg_r" )
                        }
                    };
                    for( const bodypart_id &bp : bps ) {
                        if( one_in( 2 ) ) {
                            u.deal_damage( nullptr, bp, damage_instance( damage_bash, force_remaining * dam_mult ) );
                        }
                    }
                    u.check_dead_state();
                }
                m.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                u.setpos( traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    if( u.has_effect( effect_stunned ) ) {
                        add_msg( m_bad, n_gettext( "You were stunned AGAIN for %d turn!",
                                                   "You were stunned AGAIN for %d turns!",
                                                   force_remaining ),
                                 force_remaining );
                    } else {
                        add_msg( m_bad, n_gettext( "You were stunned for %d turn!",
                                                   "You were stunned for %d turns!",
                                                   force_remaining ),
                                 force_remaining );
                    }
                    u.add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                if( creatures.creature_at<monster>( traj.front() ) ) {
                    add_msg( _( "You collided with something and sent it flying!" ) );
                } else if( npc *const guy = creatures.creature_at<npc>( traj.front() ) ) {
                    if( guy->male ) {
                        add_msg( _( "You collided with someone and sent him flying!" ) );
                    } else {
                        add_msg( _( "You collided with someone and sent her flying!" ) );
                    }
                }
                knockback( traj, stun, dam_mult );
                break;
            }
            if( m.has_flag( ter_furn_flag::TFLAG_LIQUID, u.pos() ) && force_remaining == 0 ) {
                avatar_action::swim( m, u, u.pos() );
            } else {
                u.setpos( traj[i] );
            }
        }
    }
}

void game::use_computer( const tripoint &p )
{
    if( u.has_trait( trait_ILLITERATE ) ) {
        add_msg( m_info, _( "You can not read a computer screen!" ) );
        return;
    }
    if( u.is_blind() ) {
        // we don't have screen readers in game
        add_msg( m_info, _( "You can not see a computer screen!" ) );
        return;
    }
    if( u.has_flag( json_flag_HYPEROPIC ) && !u.worn_with_flag( flag_FIX_FARSIGHT ) &&
        !u.has_effect( effect_contacts ) && !u.has_effect( effect_transition_contacts ) &&
        !u.has_flag( STATIC( json_character_flag( "ENHANCED_VISION" ) ) ) ) {
        add_msg( m_info, _( "You'll need to put on reading glasses before you can see the screen." ) );
        return;
    }

    computer *used = m.computer_at( p );

    if( used == nullptr ) {
        if( m.has_flag( ter_furn_flag::TFLAG_CONSOLE, p ) ) { //Console without map data
            add_msg( m_bad, _( "The console doesn't display anything coherent." ) );
        } else {
            dbg( D_ERROR ) << "game:use_computer: Tried to use computer at (" <<
                           p.x << ", " << p.y << ", " << p.z << ") - none there";
            debugmsg( "Tried to use computer at (%d, %d, %d) - none there", p.x, p.y, p.z );
        }
        return;
    }
    if( used->eocs.empty() ) {
        computer_session( *used ).use();
    } else {
        dialogue d( get_talker_for( get_avatar() ), get_talker_for( used ) );
        for( const effect_on_condition_id &eoc : used->eocs ) {
            eoc->activate( d );
        }
    }
}

template<typename T>
shared_ptr_fast<T> game::shared_from( const T &critter )
{
    if( static_cast<const Creature *>( &critter ) == static_cast<const Creature *>( &u ) ) {
        // u is not stored in a shared_ptr, but it won't go out of scope anyway
        return std::dynamic_pointer_cast<T>( u_shared_ptr );
    }
    if( critter.is_monster() ) {
        if( const shared_ptr_fast<monster> mon_ptr = critter_tracker->find( critter.get_location() ) ) {
            if( static_cast<const Creature *>( mon_ptr.get() ) == static_cast<const Creature *>( &critter ) ) {
                return std::dynamic_pointer_cast<T>( mon_ptr );
            }
        }
    }
    if( critter.is_npc() ) {
        for( auto &cur_npc : critter_tracker->active_npc ) {
            if( static_cast<const Creature *>( cur_npc.get() ) == static_cast<const Creature *>( &critter ) ) {
                return std::dynamic_pointer_cast<T>( cur_npc );
            }
        }
    }
    return nullptr;
}

template shared_ptr_fast<Creature> game::shared_from<Creature>( const Creature & );
template shared_ptr_fast<Character> game::shared_from<Character>( const Character & );
template shared_ptr_fast<avatar> game::shared_from<avatar>( const avatar & );
template shared_ptr_fast<monster> game::shared_from<monster>( const monster & );
template shared_ptr_fast<npc> game::shared_from<npc>( const npc & );

template<typename T>
T *game::critter_by_id( const character_id &id )
{
    if( id == u.getID() ) {
        // player is always alive, therefore no is-dead check
        return dynamic_cast<T *>( &u );
    }
    return find_npc( id );
}

// monsters don't have ids
template Character *game::critter_by_id<Character>( const character_id & );
template npc *game::critter_by_id<npc>( const character_id & );
template Creature *game::critter_by_id<Creature>( const character_id & );

static bool can_place_monster( const monster &mon, const tripoint &p )
{
    creature_tracker &creatures = get_creature_tracker();
    if( const monster *const critter = creatures.creature_at<monster>( p ) ) {
        // creature_tracker handles this. The hallucination monster will simply vanish
        if( !critter->is_hallucination() ) {
            return false;
        }
    }
    // Although monsters can sometimes exist on the same place as a Character (e.g. ridden horse),
    // it is usually wrong. So don't allow it.
    if( creatures.creature_at<Character>( p ) ) {
        return false;
    }
    return mon.will_move_to( p ) && mon.know_danger_at( p );
}

static bool can_place_npc( const tripoint &p )
{
    creature_tracker &creatures = get_creature_tracker();
    if( const monster *const critter = creatures.creature_at<monster>( p ) ) {
        // creature_tracker handles this. The hallucination monster will simply vanish
        if( !critter->is_hallucination() ) {
            return false;
        }
    }
    if( creatures.creature_at<Character>( p ) ) {
        return false;
    }
    return !g->is_dangerous_tile( p );
}

static std::optional<tripoint> choose_where_to_place_monster( const monster &mon,
        const tripoint_range<tripoint> &range )
{
    return random_point( range, [&]( const tripoint & p ) {
        return can_place_monster( mon, p );
    } );
}

monster *game::place_critter_at( const mtype_id &id, const tripoint &p )
{
    return place_critter_around( id, p, 0 );
}

monster *game::place_critter_at( const shared_ptr_fast<monster> &mon, const tripoint &p )
{
    return place_critter_around( mon, p, 0 );
}

monster *game::place_critter_around( const mtype_id &id, const tripoint &center, const int radius )
{
    // TODO: change this into an assert, it must never happen.
    if( id.is_null() ) {
        return nullptr;
    }
    shared_ptr_fast<monster> mon = make_shared_fast<monster>( id );
    mon->ammo = mon->type->starting_ammo;
    return place_critter_around( mon, center, radius );
}

monster *game::place_critter_around( const shared_ptr_fast<monster> &mon,
                                     const tripoint &center,
                                     const int radius,
                                     bool forced )
{
    std::optional<tripoint> where;
    if( forced || can_place_monster( *mon, center ) ) {
        where = center;
    }

    // This loop ensures the monster is placed as close to the center as possible,
    // but all places that equally far from the center have the same probability.
    for( int r = 1; r <= radius && !where; ++r ) {
        where = choose_where_to_place_monster( *mon, m.points_in_radius( center, r ) );
    }

    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    return critter_tracker->add( mon ) ? mon.get() : nullptr;
}

monster *game::place_critter_within( const mtype_id &id, const tripoint_range<tripoint> &range )
{
    // TODO: change this into an assert, it must never happen.
    if( id.is_null() ) {
        return nullptr;
    }
    return place_critter_within( make_shared_fast<monster>( id ), range );
}

monster *game::place_critter_within( const shared_ptr_fast<monster> &mon,
                                     const tripoint_range<tripoint> &range )
{
    const std::optional<tripoint> where = choose_where_to_place_monster( *mon, range );
    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    return critter_tracker->add( mon ) ? mon.get() : nullptr;
}

size_t game::num_creatures() const
{
    // Plus one for the player.
    return critter_tracker->size() + critter_tracker->active_npc.size() + 1;
}

bool game::update_zombie_pos( const monster &critter, const tripoint_abs_ms &old_pos,
                              const tripoint_abs_ms &new_pos )
{
    return critter_tracker->update_pos( critter, old_pos, new_pos );
}

void game::remove_zombie( const monster &critter )
{
    critter_tracker->remove( critter );
}

void game::clear_zombies()
{
    critter_tracker->clear();
}

bool game::find_nearby_spawn_point( const tripoint &target, const mtype_id &mt, int min_radius,
                                    int max_radius, tripoint &point, bool outdoor_only, bool indoor_only, bool open_air_allowed )
{
    tripoint target_point;
    //find a legal outdoor place to spawn based on the specified radius,
    //we just try a bunch of random points and use the first one that works, it none do then no spawn
    for( int attempts = 0; attempts < 75; attempts++ ) {
        target_point = target + tripoint( rng( -max_radius, max_radius ),
                                          rng( -max_radius, max_radius ), 0 );
        if( can_place_monster( monster( mt->id ), target_point ) &&
            ( open_air_allowed || get_map().has_floor_or_water( target_point ) ) &&
            ( !outdoor_only || get_map().is_outside( target_point ) ) &&
            ( !indoor_only || !get_map().is_outside( target_point ) ) &&
            rl_dist( target_point, target ) >= min_radius ) {
            point = target_point;
            return true;
        }
    }
    return false;
}

bool game::find_nearby_spawn_point( const tripoint &target, int min_radius,
                                    int max_radius, tripoint &point, bool outdoor_only, bool indoor_only, bool open_air_allowed )
{
    tripoint target_point;
    //find a legal outdoor place to spawn based on the specified radius,
    //we just try a bunch of random points and use the first one that works, it none do then no spawn
    for( int attempts = 0; attempts < 75; attempts++ ) {
        target_point = target + tripoint( rng( -max_radius, max_radius ),
                                          rng( -max_radius, max_radius ), 0 );
        if( can_place_npc( target_point ) &&
            ( open_air_allowed || get_map().has_floor_or_water( target_point ) ) &&
            ( !outdoor_only || get_map().is_outside( target_point ) ) &&
            ( !indoor_only || !get_map().is_outside( target_point ) ) &&
            rl_dist( target_point, target ) >= min_radius ) {
            point = target_point;
            return true;
        }
    }
    return false;
}

/**
 * Attempts to spawn a hallucination at given location.
 * Returns false if the hallucination couldn't be spawned for whatever reason, such as
 * a monster already in the target square.
 * @return Whether or not a hallucination was successfully spawned.
 */
bool game::spawn_hallucination( const tripoint &p )
{
    //Don't spawn hallucinations on open air
    if( get_map().has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) {
        return false;
    }

    if( one_in( 100 ) ) {
        shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
        tmp->normalize();
        tmp->randomize( NC_HALLU );
        tmp->spawn_at_precise( tripoint_abs_ms( get_map().getabs( p ) ) );
        if( !get_creature_tracker().creature_at( p, true ) ) {
            overmap_buffer.insert_npc( tmp );
            load_npcs();
            return true;
        } else {
            return false;
        }
    }

    mtype_id hallu = MonsterGenerator::generator().get_valid_hallucination();

    // If there's 'spawns' exist for player's current location, then
    // spawn random hallucination monster from 'spawns'-dependent monster group (90% chance)
    // or a completely random hallucination monster (10% chance)
    const oter_id &terrain_type = overmap_buffer.ter( get_player_character().global_omt_location() );
    const overmap_static_spawns &spawns = terrain_type->get_static_spawns();
    if( !spawns.group.is_null() && !one_in( 9 ) ) {
        hallu = MonsterGroupManager::GetRandomMonsterFromGroup( spawns.group );
    }

    return spawn_hallucination( p, hallu, std::nullopt );
}
/**
 * Attempts to spawn a hallucination at given location.
 * Returns false if the hallucination couldn't be spawned for whatever reason, such as
 * a monster already in the target square.
 * @return Whether or not a hallucination was successfully spawned.
 */
bool game::spawn_hallucination( const tripoint &p, const mtype_id &mt,
                                std::optional<time_duration> lifespan )
{
    //Don't spawn hallucinations on open air
    if( get_map().has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) {
        return false;
    }

    const shared_ptr_fast<monster> phantasm = make_shared_fast<monster>( mt );
    phantasm->hallucination = true;
    phantasm->spawn( p );
    if( lifespan.has_value() ) {
        phantasm->set_summon_time( lifespan.value() );
    }
    //Don't attempt to place phantasms inside of other creatures
    if( !get_creature_tracker().creature_at( phantasm->pos(), true ) ) {
        return critter_tracker->add( phantasm );
    } else {
        return false;
    }
}

bool game::spawn_npc( const tripoint &p, const string_id<npc_template> &npc_class,
                      std::string &unique_id,
                      std::vector<trait_id> &traits, std::optional<time_duration> lifespan )
{
    if( !unique_id.empty() && g->unique_npc_exists( unique_id ) ) {
        add_msg_debug( debugmode::DF_NPC, "NPC with unique id %s already exists.", unique_id );
        return false;
    }
    shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
    tmp->normalize();
    tmp->load_npc_template( npc_class );
    tmp->spawn_at_precise( tripoint_abs_ms( get_map().getabs( p ) ) );
    if( !get_creature_tracker().creature_at( p, true ) ) {
        overmap_buffer.insert_npc( tmp );
        for( const trait_id &new_trait : traits ) {
            tmp->set_mutation( new_trait );
        }
        if( !unique_id.empty() ) {
            tmp->set_unique_id( unique_id );
        }
        if( lifespan.has_value() ) {
            tmp->set_summon_time( lifespan.value() );
        }
        load_npcs();
        return true;
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
    creature_tracker &creatures = get_creature_tracker();
    if( creatures.creature_at( a.pos() ) != &a ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  b.disp_name(), creatures.creature_at( a.pos() )->disp_name() );
        return false;
    }
    if( creatures.creature_at( b.pos() ) != &b ) {
        debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                  a.disp_name(), creatures.creature_at( b.pos() )->disp_name() );
        return false;
    }
    // Simplify by "sorting" the arguments
    // Only the first argument can be u
    // If swapping player/npc with a monster, monster is second
    bool a_first = a.is_avatar() ||
                   ( a.is_npc() && !b.is_avatar() );
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

    Character *u_or_npc = dynamic_cast< Character * >( &first );
    Character *other_npc = dynamic_cast< Character * >( &second );

    if( u_or_npc->in_vehicle ) {
        m.unboard_vehicle( u_or_npc->pos() );
    }

    if( other_npc && other_npc->in_vehicle ) {
        m.unboard_vehicle( other_npc->pos() );
    }

    tripoint temp = second.pos();
    second.setpos( first.pos() );

    if( first.is_avatar() ) {
        walk_move( temp );
    } else {
        first.setpos( temp );
        if( m.veh_at( u_or_npc->pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            m.board_vehicle( u_or_npc->pos(), u_or_npc );
        }
    }

    if( other_npc && m.veh_at( other_npc->pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        m.board_vehicle( other_npc->pos(), other_npc );
    }
    return true;
}

bool game::is_empty( const tripoint &p )
{
    return ( m.passable( p ) || m.has_flag( ter_furn_flag::TFLAG_LIQUID, p ) ) &&
           get_creature_tracker().creature_at( p ) == nullptr;
}

bool game::is_empty( const tripoint_bub_ms &p )
{
    return is_empty( p.raw() );
}

bool game::is_in_sunlight( const tripoint &p )
{
    return !is_sheltered( p ) &&
           incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::minimal;
}

bool game::is_sheltered( const tripoint &p )
{
    const optional_vpart_position vp = m.veh_at( p );
    bool is_inside = vp && vp->is_inside();

    return !m.is_outside( p ) ||
           p.z < 0 ||
           is_inside;
}

bool game::revive_corpse( const tripoint &p, item &it )
{
    return revive_corpse( p, it, 1 );
}

bool game::revive_corpse( const tripoint &p, item &it, int radius )
{
    if( !it.is_corpse() ) {
        debugmsg( "Tried to revive a non-corpse." );
        return false;
    }
    // If this is not here, the game may attempt to spawn a monster before the map exists,
    // leading to it querying for furniture, and crashing.
    if( g->new_game ) {
        return false;
    }
    if( it.has_flag( flag_FIELD_DRESS ) || it.has_flag( flag_FIELD_DRESS_FAILED ) ||
        it.has_flag( flag_QUARTERED ) ) {
        // Failed reanimation due to corpse being butchered
        return false;
    }
    shared_ptr_fast<monster> newmon_ptr;
    if( it.has_var( "zombie_form" ) ) {
        // the monster was not a zombie but turns into one when its corpse is revived
        newmon_ptr = make_shared_fast<monster>( mtype_id( it.get_var( "zombie_form" ) ) );
    } else {
        newmon_ptr = make_shared_fast<monster>( it.get_mtype()->id );
    }
    monster &critter = *newmon_ptr;
    critter.init_from_item( it );
    if( critter.get_hp() < 1 ) {
        // Failed reanimation due to corpse being too burned
        return false;
    }

    critter.no_extra_death_drops = true;
    critter.add_effect( effect_downed, 5_turns, true );

    if( it.get_var( "no_ammo" ) == "no_ammo" ) {
        for( auto &ammo : critter.ammo ) {
            ammo.second = 0;
        }
    }

    return place_critter_around( newmon_ptr, p, radius );
}

void game::save_cyborg( item *cyborg, const tripoint &couch_pos, Character &installer )
{
    int damage = cyborg->damage();
    int dmg_lvl = cyborg->damage_level();
    int difficulty = 12;

    if( damage != 0 ) {

        popup( _( "WARNING: Patient's body is damaged.  Difficulty of the procedure is increased by %s." ),
               dmg_lvl );

        // Damage of the cyborg increases difficulty
        difficulty += dmg_lvl;
    }

    int chance_of_success = bionic_success_chance( true, -1, difficulty, installer );
    int success = chance_of_success - rng( 1, 100 );

    if( !get_avatar().query_yn(
            _( "WARNING: %i percent chance of SEVERE damage to all body parts!  Continue anyway?" ),
            100 - static_cast<int>( chance_of_success ) ) ) {
        return;
    }

    if( success > 0 ) {
        add_msg( m_good, _( "Successfully removed Personality override." ) );
        add_msg( m_bad, _( "Autodoc immediately destroys the CBM upon removal." ) );

        m.i_rem( couch_pos, cyborg );

        shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
        tmp->normalize();
        tmp->load_npc_template( npc_template_cyborg_rescued );
        tmp->spawn_at_precise( tripoint_abs_ms( get_map().getabs( couch_pos ) ) );
        overmap_buffer.insert_npc( tmp );
        tmp->hurtall( dmg_lvl * 10, nullptr );
        tmp->add_effect( effect_downed, rng( 1_turns, 4_turns ), false, 0, true );
        load_npcs();

    } else {
        const int failure_level = static_cast<int>( std::sqrt( std::abs( success ) * 4.0 * difficulty /
                                  installer.bionics_adjusted_skill( true, 12 ) ) );
        const int fail_type = std::min( 5, failure_level );
        switch( fail_type ) {
            case 1:
            case 2:
                add_msg( m_info, _( "The removal fails." ) );
                add_msg( m_bad, _( "The body is damaged." ) );
                cyborg->mod_damage( 1000 );
                break;
            case 3:
            case 4:
                add_msg( m_info, _( "The removal fails badly." ) );
                add_msg( m_bad, _( "The body is badly damaged!" ) );
                cyborg->mod_damage( 2000 );
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

void game::exam_appliance( vehicle &veh, const point &c )
{
    player_activity act = veh_app_interact::run( veh, c );
    if( act ) {
        u.moves = 0;
        u.assign_activity( act );
    }
}

void game::exam_vehicle( vehicle &veh, const point &c )
{
    if( veh.magic ) {
        add_msg( m_info, _( "This is your %s" ), veh.name );
        return;
    }
    player_activity act = veh_interact::run( veh, c );
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
    point kb( x, y ); // and when moving items out of the way
    const auto valid_location = [&]( const tripoint & p ) {
        return g->is_empty( p );
    };
    if( const std::optional<tripoint> pos = random_point( m.points_in_radius( p, 2 ),
                                            valid_location ) ) {
        kb.x = -pos->x + x + x;
        kb.y = -pos->y + y + y;
    }
    const tripoint kbp( kb, p.z );
    if( kbp == p ) {
        // can't pushback any creatures anywhere, that means the door can't close.
        return false;
    }
    const bool can_see = u.sees( tripoint( x, y, p.z ) );
    creature_tracker &creatures = get_creature_tracker();
    Character *npc_or_player = creatures.creature_at<Character>( tripoint( x, y, p.z ), false );
    if( npc_or_player != nullptr ) {
        if( bash_dmg <= 0 ) {
            return false;
        }
        if( npc_or_player->is_npc() && can_see ) {
            add_msg( _( "The %1$s hits the %2$s." ), door_name, npc_or_player->get_name() );
        } else if( npc_or_player->is_avatar() ) {
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
    if( monster *const mon_ptr = creatures.creature_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( bash_dmg <= 0 ) {
            return false;
        }
        if( can_see ) {
            add_msg( _( "The %1$s hits the %2$s." ), door_name, critter.name() );
        }
        if( critter.type->size <= creature_size::small ) {
            critter.die_in_explosion( nullptr );
        } else {
            critter.apply_damage( nullptr, bodypart_id( "torso" ), bash_dmg );
            critter.check_dead_state();
        }
        if( !critter.is_dead() && critter.type->size >= creature_size::huge ) {
            // big critters simply prevent the gate from closing
            // TODO: perhaps damage/destroy the gate
            // if the critter was really big?
            return false;
        }
        if( !critter.is_dead() ) {
            // Still alive? Move the critter away so the door can close
            knockback( kbp, p, std::max( 1, bash_dmg / 10 ), -1, 1 );
            if( creatures.creature_at( p ) ) {
                return false;
            }
        }
    }
    if( const optional_vpart_position vp = m.veh_at( p ) ) {
        if( bash_dmg <= 0 ) {
            return false;
        }
        vp->vehicle().damage( m, vp->part_index(), bash_dmg );
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
        for( item &elem : m.i_at( point( x, y ) ) ) {
            if( elem.made_of( phase_id::LIQUID ) ) {
                // Liquids are OK, will be destroyed later
                continue;
            }
            if( elem.volume() < 250_ml ) {
                // Dito for small items, will be moved away
                continue;
            }
            // Everything else prevents the door from closing
            return false;
        }
    }

    m.ter_set( point( x, y ), door_type );
    if( m.has_flag( ter_furn_flag::TFLAG_NOITEM, point( x, y ) ) ) {
        map_stack items = m.i_at( point( x, y ) );
        for( map_stack::iterator it = items.begin(); it != items.end(); ) {
            if( it->made_of( phase_id::LIQUID ) ) {
                it = items.erase( it );
                continue;
            }
            const int glass_portion = it->made_of( material_glass );
            const float glass_fraction = glass_portion / static_cast<float>( it->type->mat_portion_total );
            if( glass_portion && rng_float( 0.0f, 1.0f ) < glass_fraction * 0.5f ) {
                if( can_see ) {
                    add_msg( m_warning, _( "A %s shatters!" ), it->tname() );
                } else {
                    add_msg( m_warning, _( "Something shatters!" ) );
                }
                it = items.erase( it );
                continue;
            }
            m.add_item_or_charges( kbp, *it );
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
    // TODO:: make dir() const correct!
    const units::angle d = ray.dir();
    add_msg( _( "You dive from the %s." ), veh->name );
    m.unboard_vehicle( u.pos() );
    u.moves -= 200;
    // Dive three tiles in the direction of tox and toy
    fling_creature( &u, d, 30, true, true );
    // Hit the ground according to vehicle speed
    if( !m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, u.pos() ) ) {
        if( veh->velocity > 0 ) {
            fling_creature( &u, veh->face.dir(), veh->velocity / static_cast<float>( 100 ), false, true );
        } else {
            fling_creature( &u, veh->face.dir() + 180_degrees,
                            -( veh->velocity ) / static_cast<float>( 100 ), false, true );
        }
    }
}

void game::control_vehicle()
{
    if( vehicle *remote_veh = remoteveh() ) { // remote controls have priority
        for( const vpart_reference &vpr : remote_veh->get_avail_parts( "REMOTE_CONTROLS" ) ) {
            remote_veh->interact_with( vpr.pos() );
            return;
        }
    }
    vehicle *veh = nullptr;
    if( const optional_vpart_position vp = m.veh_at( u.pos() ) ) {
        veh = &vp->vehicle();
        const int controls_idx = veh->avail_part_with_feature( vp->mount(), "CONTROLS" );
        const int reins_idx = veh->avail_part_with_feature( vp->mount(), "CONTROL_ANIMAL" );
        const bool controls_ok = controls_idx >= 0; // controls available to "drive"
        const bool reins_ok = reins_idx >= 0 // reins + animal available to "drive"
                              && veh->has_engine_type( fuel_type_animal, false )
                              && veh->get_harnessed_animal();
        if( veh->player_in_control( u ) ) {
            // player already "driving" - offer ways to leave
            if( controls_ok ) {
                veh->interact_with( u.pos() );
            } else if( reins_idx >= 0 ) {
                u.controlling_vehicle = false;
                add_msg( m_info, _( "You let go of the reins." ) );
            }
        } else if( u.in_vehicle && ( controls_ok || reins_ok ) ) {
            // player not driving but has controls or reins on tile
            if( veh->is_locked ) {
                veh->interact_with( u.pos() );
                return; // interact_with offers to hotwire
            }
            if( !veh->handle_potential_theft( u ) ) {
                return; // player not owner and refused to steal
            }
            if( veh->engine_on ) {
                u.controlling_vehicle = true;
                add_msg( _( "You take control of the %s." ), veh->name );
            } else {
                veh->start_engines( true );
            }
        }
    }
    if( !veh ) { // no controls or animal reins under player position, search nearby
        int num_valid_controls = 0;
        std::optional<tripoint> vehicle_position;
        std::optional<vpart_reference> vehicle_controls;
        for( const tripoint &elem : m.points_in_radius( get_player_character().pos(), 1 ) ) {
            if( const optional_vpart_position vp = m.veh_at( elem ) ) {
                const std::optional<vpart_reference> controls = vp.value().part_with_feature( "CONTROLS", true );
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
            if( !veh->handle_potential_theft( u ) ) {
                return;
            }
            veh->interact_with( *vehicle_position );
        }
    }
    if( veh ) {
        // If we reached here, we gained control of a vehicle.
        // Clear the map memory for the area covered by the vehicle to eliminate ghost vehicles.
        for( const tripoint &target : veh->get_points() ) {
            u.memorize_clear_decoration( m.getglobal( target ), "vp_" );
            m.memory_cache_dec_set_dirty( target, true );
        }
        veh->is_following = false;
        veh->is_patrolling = false;
        veh->autopilot_on = false;
        veh->is_autodriving = false;
    }
}

bool game::npc_menu( npc &who )
{
    enum choices : int {
        talk = 0,
        swap_pos,
        push,
        examine_wounds,
        examine_status,
        use_item,
        sort_armor,
        attack,
        disarm,
        steal,
        trade
    };

    const bool obeys = debug_mode || ( who.is_friendly( u ) && !who.in_sleep_state() );

    uilist amenu;

    amenu.text = string_format( _( "What to do with %s?" ), who.disp_name() );
    amenu.addentry( talk, true, 't', _( "Talk" ) );
    amenu.addentry( swap_pos, obeys && !who.is_mounted() &&
                    !u.is_mounted(), 's', _( "Swap positions" ) );
    amenu.addentry( push, obeys && !who.is_mounted(), 'p', _( "Push away" ) );
    amenu.addentry( examine_wounds, true, 'w', _( "Examine wounds" ) );
    amenu.addentry( examine_status, true, 'e', _( "Examine status" ) );
    amenu.addentry( use_item, true, 'i', _( "Use item on" ) );
    amenu.addentry( sort_armor, true, 'r', _( "Sort armor" ) );
    amenu.addentry( attack, true, 'a', _( "Attack" ) );
    if( !who.is_player_ally() ) {
        amenu.addentry( disarm, who.is_armed(), 'd', _( "Disarm" ) );
        amenu.addentry( steal, !who.is_enemy(), 'S', _( "Steal" ) );
    } else {
        amenu.addentry( trade, true, 'b', _( "Trade" ) );
    }

    amenu.query();

    const int choice = amenu.ret;
    if( choice == talk ) {
        u.talk_to( get_talker_for( who ) );
    } else if( choice == swap_pos ) {
        if( !prompt_dangerous_tile( who.pos() ) ) {
            return true;
        }
        if( u.get_grab_type() == object_type::NONE ) {
            // TODO: Make NPCs protest when displaced onto dangerous crap
            add_msg( _( "You swap places with %s." ), who.get_name() );
            swap_critters( u, who );
            // TODO: Make that depend on stuff
            u.mod_moves( -200 );
        } else {
            add_msg( _( "You cannot swap places while grabbing something." ) );
        }
    } else if( choice == push ) {
        // TODO: Make NPCs protest when displaced onto dangerous crap
        tripoint oldpos = who.pos();
        who.move_away_from( u.pos(), true );
        u.mod_moves( -20 );
        if( oldpos != who.pos() ) {
            add_msg( _( "%s moves out of the way." ), who.get_name() );
        } else {
            add_msg( m_warning, _( "%s has nowhere to go!" ), who.get_name() );
        }
    } else if( choice == examine_wounds ) {
        ///\EFFECT_PER slightly increases precision when examining NPCs' wounds
        ///\EFFECT_FIRSTAID increases precision when examining NPCs' wounds
        float prof_bonus = u.get_skill_level( skill_firstaid );
        prof_bonus = u.has_proficiency( proficiency_prof_wound_care ) ? prof_bonus + 1 : prof_bonus;
        prof_bonus = u.has_proficiency( proficiency_prof_wound_care_expert ) ? prof_bonus + 2 : prof_bonus;
        const bool precise = prof_bonus * 4 + u.per_cur >= 20;
        who.body_window( _( "Limbs of: " ) + who.disp_name(), true, precise, 0, 0, 0, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f );
    } else if( choice == examine_status ) {
        if( debug_mode || ( who.is_npc() && ( who.as_npc()->op_of_u.trust >= 5 ||
                                              who.is_friendly( u ) ) ) ||
            who.in_sleep_state() ) {
            display_bodygraph( who );
        } else {
            who.say( SNIPPET.random_from_category( "<no>" ).value_or( translation() ).translated() );
        }
    } else if( choice == use_item ) {
        static const std::string heal_string( "heal" );
        const auto will_accept = [&who]( const item & it ) {
            if( it.has_flag( json_flag_SPLINT ) && who.can_wear( it ).success() ) {
                return true;
            }
            const use_function *use_fun = it.get_use( heal_string );
            if( use_fun == nullptr ) {
                return false;
            }

            const auto *actor = dynamic_cast<const heal_actor *>( use_fun->get_actor_ptr() );

            return actor != nullptr &&
                   actor->limb_power >= 0 &&
                   actor->head_power >= 0 &&
                   actor->torso_power >= 0;
        };
        item_location loc = game_menus::inv::titled_filter_menu( will_accept, u, _( "Use which item?" ) );

        if( !loc ) {
            add_msg( _( "Never mind" ) );
            return false;
        }
        item &used = *loc;
        if( used.has_flag( json_flag_SPLINT ) ) {
            std::string reason = _( "Nope." );
            if( who.wear_if_wanted( used, reason ) ) {
                u.i_rem( &used );
            }
        } else {
            bool did_use = u.invoke_item( &used, heal_string, who.pos() );
            if( did_use ) {
                // Note: exiting a body part selection menu counts as use here
                u.mod_moves( -300 );
            }
        }
    } else if( choice == sort_armor ) {
        if( who.is_hallucination() ) {
            who.say( SNIPPET.random_from_category( "<no>" ).value_or( translation() ).translated() );
        } else {
            who.worn.sort_armor( who );
            u.mod_moves( -100 );
        }
    } else if( choice == attack ) {
        if( who.is_enemy() || query_yn( _( "You may be attacked!  Proceed?" ) ) ) {
            u.melee_attack( who, true );
            who.on_attacked( u );
        }
    } else if( choice == disarm ) {
        if( who.is_enemy() || query_yn( _( "You may be attacked!  Proceed?" ) ) ) {
            u.disarm( who );
        }
    } else if( choice == steal && query_yn( _( "You may be attacked!  Proceed?" ) ) ) {
        u.steal( who );
    } else if( choice == trade ) {
        if( who.is_hallucination() ) {
            who.say( SNIPPET.random_from_category( "<hallu_dont_trade>" ).value_or(
                         translation() ).translated() );
        } else {
            npc_trading::trade( who, 0, _( "Trade" ) );
        }
    }

    return true;
}

void game::examine( bool with_pickup )
{
    // if we are driving a vehicle, examine the
    // current tile without asking.
    const optional_vpart_position vp = m.veh_at( u.pos() );
    if( vp && vp->vehicle().player_in_control( u ) ) {
        examine( u.pos(), with_pickup );
        return;
    }

    std::optional<tripoint> examp;
    if( with_pickup ) {
        // Examine and/or pick up items
        examp = choose_adjacent_highlight( _( "Examine terrain, furniture, or items where?" ),
                                           _( "There is nothing that can be examined nearby." ),
                                           ACTION_EXAMINE_AND_PICKUP, false );
    } else {
        // Examine but do not pick up items
        examp = choose_adjacent_highlight( _( "Examine terrain or furniture where?" ),
                                           _( "There is nothing that can be examined nearby." ),
                                           ACTION_EXAMINE, false );
    }

    if( !examp ) {
        return;
    }
    u.manual_examine = true;
    examine( *examp, with_pickup );
    u.manual_examine = false;
}

static std::string get_fire_fuel_string( const tripoint &examp )
{
    map &here = get_map();
    if( here.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, examp ) ) {
        field_entry *fire = here.get_field( examp, fd_fire );
        if( fire ) {
            std::string ss;
            ss += _( "There is a fire here." );
            ss += " ";
            if( fire->get_field_intensity() > 1 ) {
                ss += _( "It's too big and unpredictable to evaluate how long it will last." );
                return ss;
            }
            time_duration fire_age = fire->get_field_age();
            // half-life inclusion
            int mod = 5 - get_player_character().get_skill_level( skill_survival );
            mod = std::max( mod, 0 );
            if( fire_age >= 0_turns ) {
                if( mod >= 4 ) { // = survival level 0-1
                    ss += _( "It's going to go out soon without extra fuel." );
                    return ss;
                } else {
                    fire_age = 30_minutes - fire_age;
                    if( to_string_approx( fire_age - fire_age * mod / 5 ) == to_string_approx(
                            fire_age + fire_age * mod / 5 ) ) {
                        ss += string_format(
                                  _( "Without extra fuel it might burn yet for maybe %s, but might also go out sooner." ),
                                  to_string_approx( fire_age - fire_age * mod / 5 ) );
                    } else {
                        ss += string_format(
                                  _( "Without extra fuel it might burn yet for between %s to %s, but might also go out sooner." ),
                                  to_string_approx( fire_age - fire_age * mod / 5 ),
                                  to_string_approx( fire_age + fire_age * mod / 5 ) );
                    }
                    return ss;
                }
            } else {
                fire_age = fire_age * -1 + 30_minutes;
                if( mod >= 4 ) { // = survival level 0-1
                    if( fire_age <= 1_hours ) {
                        ss += _( "It's quite decent and looks like it'll burn for a bit without extra fuel." );
                        return ss;
                    } else if( fire_age <= 3_hours ) {
                        ss += _( "It looks solid, and will burn for a few hours without extra fuel." );
                        return ss;
                    } else {
                        ss += _( "It's very well supplied and even without extra fuel might burn for at least a part of a day." );
                        return ss;
                    }
                } else {
                    if( to_string_approx( fire_age - fire_age * mod / 5 ) == to_string_approx(
                            fire_age + fire_age * mod / 5 ) ) {
                        ss += string_format( _( "Without extra fuel it will burn for %s." ),
                                             to_string_approx( fire_age - fire_age * mod / 5 ) );
                    } else {
                        ss += string_format( _( "Without extra fuel it will burn for between %s to %s." ),
                                             to_string_approx( fire_age - fire_age * mod / 5 ),
                                             to_string_approx( fire_age + fire_age * mod / 5 ) );
                    }
                    return ss;
                }
            }
        }
    }
    return {};
}

void game::examine( const tripoint &examp, bool with_pickup )
{
    if( disable_robot( examp ) ) {
        return;
    }

    Creature *c = get_creature_tracker().creature_at( examp );
    if( c != nullptr ) {
        monster *mon = dynamic_cast<monster *>( c );
        if( mon != nullptr ) {
            add_msg( _( "There is a %s." ), mon->get_name() );
            if( mon->has_effect( effect_pet ) && !u.is_mounted() ) {
                if( monexamine::pet_menu( *mon ) ) {
                    return;
                }
            } else if( mon->has_flag( mon_flag_RIDEABLE_MECH ) && !mon->has_effect( effect_pet ) ) {
                if( monexamine::mech_hack( *mon ) ) {
                    return;
                }
            } else if( mon->has_flag( mon_flag_PAY_BOT ) ) {
                if( monexamine::pay_bot( *mon ) ) {
                    return;
                }
            } else if( mon->attitude_to( u ) == Creature::Attitude::FRIENDLY && !u.is_mounted() ) {
                if( monexamine::mfriend_menu( *mon ) ) {
                    return;
                }
            } else if( mon->has_flag( mon_flag_CONVERSATION ) && !mon->type->chat_topics.empty() ) {
                get_avatar().talk_to( get_talker_for( mon ) );
            }
        } else {
            u.cant_do_mounted();
        }
        npc *np = dynamic_cast<npc *>( c );
        if( np != nullptr && !u.cant_do_mounted() ) {
            if( npc_menu( *np ) ) {
                return;
            }
        }
    }

    const optional_vpart_position vp = m.veh_at( examp );
    if( vp ) {
        if( !u.is_mounted() || u.mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !vp->vehicle().is_appliance() ) {
                vp->vehicle().interact_with( examp, with_pickup );
            } else {
                g->exam_appliance( vp->vehicle(), vp->mount() );
            }
            return;
        } else {
            add_msg( m_warning, _( "You cannot interact with a vehicle while mounted." ) );
        }
    }

    iexamine::part_con( get_avatar(), examp );
    // trap::iexamine will handle the invisible traps.
    m.tr_at( examp ).examine( examp );

    if( m.has_flag( ter_furn_flag::TFLAG_CONSOLE, examp ) && !u.is_mounted() ) {
        use_computer( examp );
        return;
    } else if( m.has_flag( ter_furn_flag::TFLAG_CONSOLE, examp ) && u.is_mounted() ) {
        add_msg( m_warning, _( "You cannot use a console while mounted." ) );
    }
    const furn_t &xfurn_t = m.furn( examp ).obj();
    const ter_t &xter_t = m.ter( examp ).obj();

    const tripoint player_pos = u.pos();

    if( m.has_furn( examp ) ) {
        if( !u.cant_do_mounted() ) {
            xfurn_t.examine( u, examp );
        }
    } else {
        if( xter_t.can_examine( examp ) && !u.is_mounted() ) {
            xter_t.examine( u, examp );
        }
    }

    // Did the player get moved? Bail out if so; our examp probably
    // isn't valid anymore.
    if( player_pos != u.pos() ) {
        return;
    }

    bool none = true;
    if( xter_t.can_examine( examp ) || xfurn_t.can_examine( examp ) ) {
        none = false;
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

    if( m.has_flag( ter_furn_flag::TFLAG_SEALED, examp ) ) {
        if( none ) {
            if( m.has_flag( ter_furn_flag::TFLAG_UNSTABLE, examp ) ) {
                add_msg( _( "The %s is too unstable to remove anything." ), m.name( examp ) );
            } else {
                add_msg( _( "The %s is firmly sealed." ), m.name( examp ) );
            }
        }
    } else {
        //examp has no traps, is a container and doesn't have a special examination function
        if( m.tr_at( examp ).is_null() && m.i_at( examp ).empty() &&
            m.has_flag( ter_furn_flag::TFLAG_CONTAINER, examp ) && none ) {
            add_msg( _( "It is empty." ) );
        } else if( ( m.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, examp ) &&
                     xfurn_t.has_examine( iexamine::fireplace ) ) ||
                   xfurn_t.has_examine( iexamine::workbench ) ) {
            return;
        } else {
            sounds::process_sound_markers( &u );
            // Pick up items, if there are any, unless there is reason to not to
            if( with_pickup && m.has_items( examp ) && !u.is_mounted() &&
                !m.has_flag( ter_furn_flag::TFLAG_NO_PICKUP_ON_EXAMINE, examp ) &&
                !m.only_liquid_in_liquidcont( examp ) ) {
                pickup( examp );
            }
        }
    }
}

void game::pickup()
{
    // Prompt for which adjacent/current tile to pick up items from
    const std::optional<tripoint> where_ = choose_adjacent_highlight( _( "Pick up items where?" ),
                                           _( "There is nothing to pick up nearby." ),
                                           ACTION_PICKUP, false );
    if( !where_ ) {
        return;
    }
    // Pick up items only from the selected tile
    u.pick_up( game_menus::inv::pickup( u, *where_ ) );
}

void game::pickup_all()
{
    // Pick up items from current and all adjacent tiles
    u.pick_up( game_menus::inv::pickup( u ) );
}

void game::pickup( const tripoint &p )
{
    // Highlight target
    shared_ptr_fast<game::draw_callback_t> hilite_cb = make_shared_fast<game::draw_callback_t>( [&]() {
        m.drawsq( w_terrain, p, drawsq_params().highlight( true ) );
    } );
    add_draw_callback( hilite_cb );

    // Pick up items only from the selected tile
    u.pick_up( game_menus::inv::pickup( u, p ) );
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carefully peeking around a corner, hence the large move cost.
void game::peek()
{
    const std::optional<tripoint> p = choose_direction( _( "Peek where?" ), true );
    if( !p ) {
        return;
    }
    tripoint new_pos = u.pos() + *p;
    if( p->z != 0 ) {
        // Character might peek to a different submap; ensures return location is accurate.
        const tripoint_abs_ms old_loc = u.get_location();
        vertical_move( p->z, false, true );

        if( old_loc != u.get_location() ) {
            new_pos = u.pos();
            u.move_to( old_loc );
            m.vertical_shift( old_loc.z() );
        } else {
            return;
        }
    }

    if( m.impassable( new_pos ) ) {
        return;
    }

    peek( new_pos );
}

void game::peek( const tripoint &p )
{
    u.moves -= 200;
    tripoint prev = u.pos();
    u.setpos( p );
    const bool is_same_pos = u.pos() == prev;
    const bool is_standup_peek = is_same_pos && u.is_crouching();
    tripoint center = p;
    m.build_map_cache( p.z );
    m.update_visibility_cache( p.z );

    look_around_result result;
    const look_around_params looka_params = { true, center, center, false, false, true, true };
    if( is_standup_peek ) {   // Non moving peek from crouch is a standup peek
        u.reset_move_mode();
        result = look_around( looka_params );
        u.activate_crouch_mode();
    } else {                // Else is normal peek
        result = look_around( looka_params );
        u.setpos( prev );
    }

    if( result.peek_action && *result.peek_action == PA_BLIND_THROW ) {
        item_location loc;
        avatar_action::plthrow( u, loc, p );
    }
    m.invalidate_map_cache( p.z );
    m.invalidate_visibility_cache();
}

////////////////////////////////////////////////////////////////////////////////////////////
std::optional<tripoint> game::look_debug()
{
    editmap edit;
    return edit.edit();
}
////////////////////////////////////////////////////////////////////////////////////////////

void game::draw_look_around_cursor( const tripoint &lp, const visibility_variables &cache )
{
    if( !liveview.is_enabled() ) {
#if defined( TILES )
        if( is_draw_tiles_mode() ) {
            draw_cursor( lp );
            return;
        }
#endif
        const tripoint view_center = u.pos() + u.view_offset;
        visibility_type visibility = visibility_type::HIDDEN;
        const bool inbounds = m.inbounds( lp );
        if( inbounds ) {
            visibility = m.get_visibility( m.apparent_light_at( lp, cache ), cache );
        }
        if( visibility == visibility_type::CLEAR ) {
            const Creature *const creature = get_creature_tracker().creature_at( lp, true );
            if( creature != nullptr && u.sees( *creature ) ) {
                creature->draw( w_terrain, view_center, true );
            } else {
                m.drawsq( w_terrain, lp, drawsq_params().highlight( true ).center( view_center ) );
            }
        } else {
            std::string visibility_indicator;
            nc_color visibility_indicator_color = c_white;
            switch( visibility ) {
                case visibility_type::CLEAR:
                    // Already handled by the outer if statement
                    break;
                case visibility_type::BOOMER:
                case visibility_type::BOOMER_DARK:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_pink;
                    break;
                case visibility_type::DARK:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_dark_gray;
                    break;
                case visibility_type::LIT:
                    visibility_indicator = '#';
                    visibility_indicator_color = c_light_gray;
                    break;
                case visibility_type::HIDDEN:
                    visibility_indicator = 'x';
                    visibility_indicator_color = c_white;
                    break;
            }

            const tripoint screen_pos = point( POSX, POSY ) + lp - view_center;
            mvwputch( w_terrain, screen_pos.xy(), visibility_indicator_color, visibility_indicator );
        }
    }
}

void game::print_all_tile_info( const tripoint &lp, const catacurses::window &w_look,
                                const std::string &area_name, int column,
                                int &line,
                                const int last_line,
                                const visibility_variables &cache )
{
    visibility_type visibility = visibility_type::HIDDEN;
    const bool inbounds = m.inbounds( lp );
    if( inbounds ) {
        visibility = m.get_visibility( m.apparent_light_at( lp, cache ), cache );
    }
    const Creature *creature = get_creature_tracker().creature_at( lp, true );
    switch( visibility ) {
        case visibility_type::CLEAR: {
            const optional_vpart_position vp = m.veh_at( lp );
            print_terrain_info( lp, w_look, area_name, column, line );
            print_fields_info( lp, w_look, column, line );
            print_trap_info( lp, w_look, column, line );
            print_part_con_info( lp, w_look, column, line );
            print_creature_info( creature, w_look, column, line, last_line );
            print_vehicle_info( veh_pointer_or_null( vp ), vp ? vp->part_index() : -1, w_look, column, line,
                                last_line );
            print_items_info( lp, w_look, column, line, last_line );
            print_graffiti_info( lp, w_look, column, line, last_line );
        }
        break;
        case visibility_type::BOOMER:
        case visibility_type::BOOMER_DARK:
        case visibility_type::DARK:
        case visibility_type::LIT:
        case visibility_type::HIDDEN:
            print_visibility_info( w_look, column, line, visibility );

            if( creature != nullptr ) {
                std::vector<std::string> buf;
                if( u.sees_with_infrared( *creature ) ) {
                    creature->describe_infrared( buf );
                } else if( u.sees_with_specials( *creature ) ) {
                    creature->describe_specials( buf );
                }
                for( const std::string &s : buf ) {
                    mvwprintw( w_look, point( 1, ++line ), s );
                }
            }
            break;
    }
    if( !inbounds ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    std::string this_sound = sounds::sound_at( lp );
    if( !this_sound.empty() ) {
        const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                          _( "From here you heard %s" ),
                                          this_sound );
        line += lines - 1;
    } else {
        // Check other z-levels
        tripoint tmp = lp;
        for( tmp.z = -OVERMAP_DEPTH; tmp.z <= OVERMAP_HEIGHT; tmp.z++ ) {
            if( tmp.z == lp.z ) {
                continue;
            }

            std::string zlev_sound = sounds::sound_at( tmp );
            if( !zlev_sound.empty() ) {
                const int lines = fold_and_print( w_look, point( 1, ++line ), max_width, c_light_gray,
                                                  tmp.z > lp.z ?
                                                  _( "From above you heard %s" ) : _( "From below you heard %s" ), zlev_sound );
                line += lines - 1;
            }
        }
    }
}

void game::print_visibility_info( const catacurses::window &w_look, int column, int &line,
                                  visibility_type visibility )
{
    const char *visibility_message = nullptr;
    switch( visibility ) {
        case visibility_type::CLEAR:
            visibility_message = _( "Clearly visible." );
            break;
        case visibility_type::BOOMER:
            visibility_message = _( "A bright pink blur." );
            break;
        case visibility_type::BOOMER_DARK:
            visibility_message = _( "A pink blur." );
            break;
        case visibility_type::DARK:
            visibility_message = _( "Darkness." );
            break;
        case visibility_type::LIT:
            visibility_message = _( "Bright light." );
            break;
        case visibility_type::HIDDEN:
            visibility_message = _( "Unseen." );
            break;
    }

    mvwprintz( w_look, point( line, column ), c_light_gray, visibility_message );
    line += 2;
}

void game::print_terrain_info( const tripoint &lp, const catacurses::window &w_look,
                               const std::string &area_name, int column, int &line )
{
    const int max_width = getmaxx( w_look ) - column - 1;

    // Print OMT type and terrain type on first two lines
    // if can't fit in one line.
    std::string tile = uppercase_first_letter( m.tername( lp ) );
    std::string area = uppercase_first_letter( area_name );
    if( const timed_event *e = get_timed_events().get( timed_event_type::OVERRIDE_PLACE ) ) {
        area = _( e->string_id );
    }
    mvwprintz( w_look, point( column, line++ ), c_yellow, area );
    mvwprintz( w_look, point( column, line++ ), c_white, tile );
    std::string desc = string_format( m.ter( lp ).obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    for( int i = 0; i < numlines; i++ ) {
        mvwprintz( w_look, point( column, line++ ), c_light_gray, lines[i] );
    }

    // Furniture, if any
    print_furniture_info( lp, w_look, column, line );

    // Cover percentage from terrain and furniture next.
    fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray, _( "Cover: %d%%" ),
                    m.coverage( lp ) );

    if( m.has_flag( ter_furn_flag::TFLAG_TREE, lp ) ) {
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Can be <color_green>cut down</color> with the right tools." ) );
        line += lines - 1;
    }

    // Terrain and furniture flags next. These can be several lines for some combinations of
    // furnitures and terrains.
    lines = foldstring( m.features( lp ), max_width );
    numlines = lines.size();
    for( int i = 0; i < numlines; i++ ) {
        mvwprintz( w_look, point( column, ++line ), c_light_gray, lines[i] );
    }

    // Move cost from terrain and furniture and vehicle parts.
    // Vehicle part information is printed in a different function.
    if( m.impassable( lp ) ) {
        mvwprintz( w_look, point( column, ++line ), c_light_red, _( "Impassable" ) );
    } else {
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Move cost: %d" ),
                   m.move_cost( lp ) * 50 );
    }

    // Next print the string on any SIGN flagged furniture if any.
    std::string signage = m.get_signage( lp );
    if( !signage.empty() ) {
        std::string sign_string = u.has_trait( trait_ILLITERATE ) ? "???" : signage;
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          _( "Sign: %s" ), sign_string );
        line += lines - 1;
    }

    // Print light level on the selected tile.
    std::pair<std::string, nc_color> ll = get_light_level( std::max( 1.0,
                                          LIGHT_AMBIENT_LIT - m.ambient_light_at( lp ) + 1.0 ) );
    mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Lighting: " ) );
    mvwprintz( w_look, point( column + utf8_width( _( "Lighting: " ) ), line ), ll.second, ll.first );

    // Print the terrain and any furntiure on the tile below and whether it is walkable.
    if( lp.z > -OVERMAP_DEPTH && !m.has_floor( lp ) ) {
        tripoint below( lp.xy(), lp.z - 1 );
        std::string tile_below = m.tername( below );
        if( m.has_furn( below ) ) {
            tile_below += ", " + m.furnname( below );
        }

        if( !m.has_floor_or_support( lp ) ) {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray,
                            _( "Below: %s; No support" ), tile_below );
        } else {
            fold_and_print( w_look, point( column, ++line ), max_width, c_dark_gray, _( "Below: %s; Walkable" ),
                            tile_below );
        }
    }

    ++line;
}

void game::print_furniture_info( const tripoint &lp, const catacurses::window &w_look, int column,
                                 int &line )
{
    // Do nothing if there is no furniture here
    if( !m.has_furn( lp ) ) {
        return;
    }
    const int max_width = getmaxx( w_look ) - column - 1;

    // Print furniture name in white
    std::string desc = uppercase_first_letter( m.furnname( lp ) );
    mvwprintz( w_look, point( column, line++ ), c_white, desc );

    // Print each line of furniture description in gray
    desc = string_format( m.furn( lp ).obj().description );
    std::vector<std::string> lines = foldstring( desc, max_width );
    int numlines = lines.size();
    for( int i = 0; i < numlines; i++ ) {
        mvwprintz( w_look, point( column, line++ ), c_light_gray, lines[i] );
    }

    // If this furniture has a crafting pseudo item, check for tool qualities and print them
    if( !m.furn( lp )->crafting_pseudo_item.is_empty() ) {
        // Make a pseudo item instance so we can use qualities_info later
        const item pseudo( m.furn( lp )->crafting_pseudo_item );
        // Set up iteminfo query to show qualities
        std::vector<iteminfo_parts> quality_part = { iteminfo_parts::QUALITIES };
        const iteminfo_query quality_query( quality_part );
        // Render info into info_vec
        std::vector<iteminfo> info_vec;
        pseudo.qualities_info( info_vec, &quality_query, 1, false );
        // Get a newline-separated string of quality info, then parse and print each line
        std::string quality_string = format_item_info( info_vec, {} );
        size_t strpos = 0;
        while( ( strpos = quality_string.find( '\n' ) ) != std::string::npos ) {
            trim_and_print( w_look, point( column, line++ ), max_width, c_light_gray,
                            quality_string.substr( 0, strpos + 1 ) );
            // Delete used token
            quality_string.erase( 0, strpos + 1 );
        }
    }
}

void game::print_fields_info( const tripoint &lp, const catacurses::window &w_look, int column,
                              int &line )
{
    const field &tmpfield = m.field_at( lp );
    for( const auto &fld : tmpfield ) {
        const field_entry &cur = fld.second;
        if( fld.first.obj().has_fire && ( m.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, lp ) ||
                                          m.ter( lp ) == t_pit_shallow || m.ter( lp ) == t_pit ) ) {
            const int max_width = getmaxx( w_look ) - column - 2;
            int lines = fold_and_print( w_look, point( column, ++line ), max_width, cur.color(),
                                        get_fire_fuel_string( lp ) ) - 1;
            line += lines;
        } else {
            mvwprintz( w_look, point( column, ++line ), cur.color(), cur.name() );
        }
    }

    int size = std::distance( tmpfield.begin(), tmpfield.end() );
    if( size > 0 ) {
        mvwprintz( w_look, point( column, ++line ), c_white, "\n" );
    }
}

void game::print_trap_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                            int &line )
{
    const trap &tr = m.tr_at( lp );
    if( tr.can_see( lp, u ) ) {
        std::string tr_name = tr.name();
        mvwprintz( w_look, point( column, ++line ), tr.color, tr_name );
    }

    ++line;
}

void game::print_part_con_info( const tripoint &lp, const catacurses::window &w_look,
                                const int column,
                                int &line )
{
    // TODO: fix point types
    partial_con *pc = m.partial_con_at( tripoint_bub_ms( lp ) );
    std::string tr_name;
    if( pc != nullptr ) {
        const construction &built = pc->id.obj();
        tr_name = string_format( _( "Unfinished task: %s, %d%% complete" ), built.group->name(),
                                 pc->counter / 100000 );

        int const width = getmaxx( w_look ) - column - 2;
        fold_and_print( w_look, point( column, ++line ), width, c_white, tr_name );

        ++line;
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
        // Print the name of the vehicle.
        mvwprintz( w_look, point( column, ++line ), c_light_gray, _( "Vehicle: " ) );
        mvwprintz( w_look, point( column + utf8_width( _( "Vehicle: " ) ), line ), c_white, "%s",
                   veh->name );
        // Then the list of parts on that tile.
        line = veh->print_part_list( w_look, ++line, last_line, getmaxx( w_look ), veh_part );
    }
}

static void add_visible_items_recursive( std::map<std::string, std::pair<int, nc_color>>
        &item_names, const item &it )
{
    ++item_names[it.tname()].first;
    item_names[it.tname()].second = it.color_in_inventory();

    for( const item *content : it.all_known_contents() ) {
        add_visible_items_recursive( item_names, *content );
    }
}

void game::print_items_info( const tripoint &lp, const catacurses::window &w_look, const int column,
                             int &line,
                             const int last_line )
{
    if( !m.sees_some_items( lp, u ) ) {
        return;
    } else if( m.has_flag( ter_furn_flag::TFLAG_CONTAINER, lp ) && !m.could_see_items( lp, u ) ) {
        mvwprintw( w_look, point( column, ++line ), _( "You cannot see what is inside of it." ) );
    } else if( u.has_effect( effect_blind ) || u.worn_with_flag( flag_BLIND ) ) {
        mvwprintz( w_look, point( column, ++line ), c_yellow,
                   _( "There's something there, but you can't see what it is." ) );
        return;
    } else {
        std::map<std::string, std::pair<int, nc_color>> item_names;
        for( const item &it : m.i_at( lp ) ) {
            add_visible_items_recursive( item_names, it );
        }

        const int max_width = getmaxx( w_look ) - column - 1;
        for( auto it = item_names.begin(); it != item_names.end(); ++it ) {
            // last line but not last item
            if( line + 1 >= last_line && std::next( it ) != item_names.end() ) {
                mvwprintz( w_look, point( column, ++line ), c_yellow, _( "More items here…" ) );
                break;
            }

            if( it->second.first > 1 ) {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second,
                                pgettext( "%s is the name of the item.  %d is the quantity of that item.", "%s [%d]" ),
                                it->first.c_str(), it->second.first );
            } else {
                trim_and_print( w_look, point( column, ++line ), max_width, it->second.second, it->first );
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
        const int lines = fold_and_print( w_look, point( column, ++line ), max_width, c_light_gray,
                                          m.ter( lp ) == t_grave_new ? _( "Graffiti: %s" ) : _( "Inscription: %s" ),
                                          m.graffiti_at( lp ) );
        line += lines - 1;
    }
}

bool game::check_zone( const zone_type_id &type, const tripoint &where ) const
{
    return zone_manager::get_manager().has( type, m.getglobal( where ) );
}

bool game::check_near_zone( const zone_type_id &type, const tripoint &where ) const
{
    return zone_manager::get_manager().has_near( type, m.getglobal( where ) );
}

bool game::is_zones_manager_open() const
{
    return zones_manager_open;
}

static void zones_manager_shortcuts( const catacurses::window &w_info, faction_id const &faction )
{
    werase( w_info );

    int tmpx = 1;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<A>dd" ) ) + 2;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<P>ersonal" ) ) + 2;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<R>emove" ) ) + 2;
    tmpx += shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<E>nable" ) ) + 2;
    shortcut_print( w_info, point( tmpx, 1 ), c_white, c_light_green, _( "<D>isable" ) );

    tmpx = 1;
    shortcut_print( w_info, point( tmpx, 2 ), c_white, c_light_green,
                    _( "<T>-Toggle zone display" ) );

    tmpx += shortcut_print( w_info, point( tmpx, 3 ), c_white, c_light_green,
                            _( "<Z>-Enable personal" ) ) + 2;
    shortcut_print( w_info, point( tmpx, 3 ), c_white, c_light_green,
                    _( "<X>-Disable personal" ) );

    tmpx = 1;
    tmpx += shortcut_print( w_info, point( tmpx, 4 ), c_white, c_light_green,
                            _( "<+-> Move up/down" ) ) + 2;
    shortcut_print( w_info, point( tmpx, 4 ), c_white, c_light_green, _( "<Enter>-Edit" ) );

    tmpx = 1;
    tmpx += shortcut_print( w_info, point( tmpx, 5 ), c_white, c_light_green,
                            _( "<S>how all / hide distant" ) ) + 2;
    shortcut_print( w_info, point( tmpx, 5 ), c_white, c_light_green, _( "<M>ap" ) );

    if( debug_mode ) {
        shortcut_print( w_info, point( 1, 6 ), c_light_red, c_light_green,
                        string_format( _( "Shown <F>action: %s" ), faction.str() ) );
    }

    wnoutrefresh( w_info );
}

static void zones_manager_draw_borders( const catacurses::window &w_border,
                                        const catacurses::window &w_info_border,
                                        const int iInfoHeight, const int width )
{
    for( int i = 1; i < TERMX; ++i ) {
        if( i < width ) {
            mvwputch( w_border, point( i, 0 ), c_light_gray, LINE_OXOX ); // -
            mvwputch( w_border, point( i, TERMY - iInfoHeight - 1 ), c_light_gray,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - iInfoHeight ) {
            mvwputch( w_border, point( 0, i ), c_light_gray, LINE_XOXO ); // |
            mvwputch( w_border, point( width - 1, i ), c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( w_border, point_zero, c_light_gray, LINE_OXXO ); // |^
    mvwputch( w_border, point( width - 1, 0 ), c_light_gray, LINE_OOXX ); // ^|

    mvwputch( w_border, point( 0, TERMY - iInfoHeight - 1 ), c_light_gray,
              LINE_XXXO ); // |-
    mvwputch( w_border, point( width - 1, TERMY - iInfoHeight - 1 ), c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( w_border, point( 2, 0 ), c_white, _( "Zones manager" ) );
    wnoutrefresh( w_border );

    for( int j = 0; j < iInfoHeight - 1; ++j ) {
        mvwputch( w_info_border, point( 0, j ), c_light_gray, LINE_XOXO );
        mvwputch( w_info_border, point( width - 1, j ), c_light_gray, LINE_XOXO );
    }

    for( int j = 0; j < width - 1; ++j ) {
        mvwputch( w_info_border, point( j, iInfoHeight - 1 ), c_light_gray, LINE_OXOX );
    }

    mvwputch( w_info_border, point( 0, iInfoHeight - 1 ), c_light_gray, LINE_XXOO );
    mvwputch( w_info_border, point( width - 1, iInfoHeight - 1 ), c_light_gray, LINE_XOOX );
    wnoutrefresh( w_info_border );
}

void game::zones_manager()
{
    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    const int zone_ui_height = 14;
    const int zone_options_height = debug_mode ? 6 : 7;

    const int width = 45;

    int offsetX = 0;
    int max_rows = 0;

    catacurses::window w_zones;
    catacurses::window w_zones_border;
    catacurses::window w_zones_info;
    catacurses::window w_zones_info_border;
    catacurses::window w_zones_options;

    bool show = true;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( !show ) {
            ui.position( point_zero, point_zero );
            return;
        }
        offsetX = get_option<std::string>( "SIDEBAR_POSITION" ) != "left" ?
                  TERMX - width : 0;
        const int w_zone_height = TERMY - zone_ui_height;
        max_rows = w_zone_height - 2;
        w_zones = catacurses::newwin( w_zone_height - 2, width - 2,
                                      point( offsetX + 1, 1 ) );
        w_zones_border = catacurses::newwin( w_zone_height, width,
                                             point( offsetX, 0 ) );
        w_zones_info = catacurses::newwin( zone_ui_height - zone_options_height - 1,
                                           width - 2, point( offsetX + 1, w_zone_height ) );
        w_zones_info_border = catacurses::newwin( zone_ui_height, width,
                              point( offsetX, w_zone_height ) );
        w_zones_options = catacurses::newwin( zone_options_height - 1, width - 2,
                                              point( offsetX + 1, TERMY - zone_options_height ) );

        ui.position( point( offsetX, 0 ), point( width, TERMY ) );
    } );
    ui.mark_resize();

    input_context ctxt( "ZONES_MANAGER" );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ADD_ZONE" );
    ctxt.register_action( "ADD_PERSONAL_ZONE" );
    ctxt.register_action( "REMOVE_ZONE" );
    ctxt.register_action( "MOVE_ZONE_UP" );
    ctxt.register_action( "MOVE_ZONE_DOWN" );
    ctxt.register_action( "SHOW_ZONE_ON_MAP" );
    ctxt.register_action( "ENABLE_ZONE" );
    ctxt.register_action( "DISABLE_ZONE" );
    ctxt.register_action( "TOGGLE_ZONE_DISPLAY" );
    ctxt.register_action( "ENABLE_PERSONAL_ZONES" );
    ctxt.register_action( "DISABLE_PERSONAL_ZONES" );
    ctxt.register_action( "SHOW_ALL_ZONES" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( debug_mode ) {
        ctxt.register_action( "CHANGE_FACTION" );
    }

    zone_manager &mgr = zone_manager::get_manager();
    int start_index = 0;
    int active_index = 0;
    bool blink = false;
    bool stuff_changed = false;
    bool show_all_zones = false;
    int zone_cnt = 0;
    faction_id zones_faction( your_fac );

    // reset any zones that were temporarily disabled for an activity
    mgr.reset_disabled();

    // cache the players location for person zones
    if( mgr.has_personal_zones() ) {
        mgr.cache_avatar_location();
    }

    // get zones with distance between player and
    // zone center point <= 60 or all zones, if show_all_zones is true
    auto get_zones = [&]() {
        std::vector<zone_manager::ref_zone_data> zones;
        if( show_all_zones ) {
            zones = mgr.get_zones( zones_faction );
        } else {
            const tripoint_abs_ms u_abs_pos = u.get_location();
            for( zone_manager::ref_zone_data &ref : mgr.get_zones( zones_faction ) ) {
                const tripoint_abs_ms &zone_abs_pos = ref.get().get_center_point();
                if( rl_dist( u_abs_pos, zone_abs_pos ) <= ACTIVITY_SEARCH_DISTANCE ) {
                    zones.emplace_back( ref );
                }
            }
        }
        zones.erase( std::remove_if( zones.begin(), zones.end(),
        []( zone_manager::ref_zone_data const & it ) {
            zone_type_id const type = it.get().get_type();
            return !debug_mode && type.is_valid() && type->hidden;
        } ),
        zones.end() );
        zone_cnt = static_cast<int>( zones.size() );
        return zones;
    };

    auto zones = get_zones();

    auto zones_manager_options = [&]() {
        werase( w_zones_options );

        if( zone_cnt > 0 ) {
            const zone_data &zone = zones[active_index].get();

            // NOLINTNEXTLINE(cata-use-named-point-constants)
            mvwprintz( w_zones_options, point( 1, 0 ), c_white, mgr.get_name_from_type( zone.get_type() ) );

            if( zone.has_options() ) {
                const auto &descriptions = zone.get_options().get_descriptions();

                // NOLINTNEXTLINE(cata-use-named-point-constants)
                mvwprintz( w_zones_options, point( 1, 1 ), c_white, _( "Options" ) );

                int y = 2;
                for( const auto &desc : descriptions ) {
                    mvwprintz( w_zones_options, point( 3, y ), c_white, desc.first );
                    mvwprintz( w_zones_options, point( 20, y ), c_white, desc.second );
                    y++;
                }
            }
        }

        wnoutrefresh( w_zones_options );
    };

    std::optional<tripoint> zone_start;
    std::optional<tripoint> zone_end;
    bool zone_blink = false;
    bool zone_cursor = false;
    shared_ptr_fast<draw_callback_t> zone_cb = create_zone_callback(
                zone_start, zone_end, zone_blink, zone_cursor );
    add_draw_callback( zone_cb );

    // This lambda returns either absolute coordinates or relative-to-player
    // coordinates, depending on whether personal is false or true respectively.
    // In C++20 we could have the return type depend on the parameter using
    // if constexpr( personal ) but for now it will just return tripoints.
    auto query_position =
    [&]( bool personal = false ) -> std::optional<std::pair<tripoint, tripoint>> {
        on_out_of_scope invalidate_current_ui( [&]()
        {
            ui.mark_resize();
        } );
        restore_on_out_of_scope<bool> show_prev( show );
        restore_on_out_of_scope<std::optional<tripoint>> zone_start_prev( zone_start );
        restore_on_out_of_scope<std::optional<tripoint>> zone_end_prev( zone_end );
        show = false;
        zone_start = std::nullopt;
        zone_end = std::nullopt;
        ui.mark_resize();

        static_popup popup;
        popup.on_top( true );
        popup.message( "%s", _( "Select first point." ) );

        tripoint center = u.pos() + u.view_offset;

        const look_around_result first =
        look_around( /*show_window=*/false, center, center, false, true, false );
        if( first.position )
        {
            popup.message( "%s", _( "Select second point." ) );

            const look_around_result second = look_around( /*show_window=*/false, center, *first.position,
                    true, true, false );
            if( second.position ) {
                if( personal ) {
                    tripoint first_rel(
                        std::min( first.position->x - u.posx(), second.position->x - u.posx() ),
                        std::min( first.position->y - u.posy(), second.position->y - u.posy() ),
                        std::min( first.position->z - u.posz(), second.position->z - u.posz() ) ) ;
                    tripoint second_rel(
                        std::max( first.position->x - u.posx(), second.position->x - u.posx() ),
                        std::max( first.position->y - u.posy(), second.position->y - u.posy() ),
                        std::max( first.position->z - u.posz(), second.position->z - u.posz() ) ) ;
                    return { { first_rel, second_rel } };
                }
                tripoint_abs_ms first_abs =
                    m.getglobal(
                        tripoint(
                            std::min( first.position->x, second.position->x ),
                            std::min( first.position->y, second.position->y ),
                            std::min( first.position->z, second.position->z ) ) );
                tripoint_abs_ms second_abs =
                    m.getglobal(
                        tripoint(
                            std::max( first.position->x, second.position->x ),
                            std::max( first.position->y, second.position->y ),
                            std::max( first.position->z, second.position->z ) ) );

                return { { first_abs.raw(), second_abs.raw() } };
            }
        }

        return std::nullopt;
    };

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( !show ) {
            return;
        }
        zones_manager_draw_borders( w_zones_border, w_zones_info_border, zone_ui_height, width );
        zones_manager_shortcuts( w_zones_info, zones_faction );

        if( zone_cnt == 0 ) {
            werase( w_zones );
            mvwprintz( w_zones, point( 2, 5 ), c_white, _( "No Zones defined." ) );

        } else {
            werase( w_zones );

            calcStartPos( start_index, active_index, max_rows, zone_cnt );

            draw_scrollbar( w_zones_border, active_index, max_rows, zone_cnt, point_south );
            wnoutrefresh( w_zones_border );

            int iNum = 0;

            tripoint_abs_ms player_absolute_pos = u.get_location();

            //Display saved zones
            for( auto &i : zones ) {
                if( iNum >= start_index &&
                    iNum < start_index + ( ( max_rows > zone_cnt ) ? zone_cnt : max_rows ) ) {
                    const zone_data &zone = i.get();

                    nc_color colorLine = zone.get_enabled() ? c_white : c_light_gray;

                    if( iNum == active_index ) {
                        mvwprintz( w_zones, point( 0, iNum - start_index ), c_yellow, "%s", ">>" );
                        colorLine = zone.get_enabled() ? c_light_green : c_green;
                    }

                    //Draw Zone name
                    mvwprintz( w_zones, point( 3, iNum - start_index ), colorLine,
                               //~ "P: <Zone Name>" represents a personal zone
                               trim_by_length( ( zone.get_is_personal() ? _( "P: " ) : "" ) + zone.get_name(), 28 ) );

                    tripoint_abs_ms center = zone.get_center_point();

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

        wnoutrefresh( w_zones );
    } );

    const int scroll_rate = zone_cnt > 20 ? 10 : 3;
    bool quit = false;
    bool save = false;
    zones_manager_open = true;
    zone_manager::get_manager().save_zones( "zmgr-temp" );
    while( !quit ) {
        if( zone_cnt > 0 ) {
            blink = !blink;
            const zone_data &zone = zones[active_index].get();
            zone_start = m.getlocal( zone.get_start_point() );
            zone_end = m.getlocal( zone.get_end_point() );
            ctxt.set_timeout( get_option<int>( "BLINK_SPEED" ) );
        } else {
            blink = false;
            zone_start = zone_end = std::nullopt;
            ctxt.reset_timeout();
        }

        // Actually accessed from the terrain overlay callback `zone_cb` in the
        // call to `ui_manager::redraw`.
        //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        zone_blink = blink;
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        //Wait for input
        const std::string action = ctxt.handle_input();

        if( action == "ADD_ZONE" ) {
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

                std::string default_name = options->get_zone_name_suggestion();
                if( default_name.empty() ) {
                    default_name = mgr.get_name_from_type( id );
                }
                const auto maybe_name = mgr.query_name( default_name );
                if( !maybe_name.has_value() ) {
                    break;
                }
                const std::string &name = maybe_name.value();

                const auto position = query_position();
                if( !position ) {
                    break;
                }

                int vehicle_zones_pre = 0;
                for( zone_manager::ref_zone_data zone : get_zones() ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones_pre++;
                    }
                }

                // TODO: fix point types
                mgr.add( name, id, get_player_character().get_faction()->id, false, true,
                         position->first, position->second, options, false );

                zones = get_zones();
                active_index = zone_cnt - 1;

                int vehicle_zones_post = 0;
                for( zone_manager::ref_zone_data zone : zones ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones_post++;
                    }
                }

                if( vehicle_zones_post == vehicle_zones_pre ) {
                    active_index -= vehicle_zones_post;
                }

                stuff_changed = true;
            } while( false );

            blink = false;
        } else if( action == "ADD_PERSONAL_ZONE" ) {
            do { // not a loop, just for quick bailing out if canceled
                const auto maybe_id = mgr.query_type( true );
                if( !maybe_id.has_value() ) {
                    break;
                }

                const zone_type_id &id = maybe_id.value();
                auto options = zone_options::create( id );

                if( !options->query_at_creation() ) {
                    break;
                }

                std::string default_name = options->get_zone_name_suggestion();
                if( default_name.empty() ) {
                    default_name = mgr.get_name_from_type( id );
                }
                const auto maybe_name = mgr.query_name( default_name );
                if( !maybe_name.has_value() ) {
                    break;
                }
                const std::string &name = maybe_name.value();

                const auto position = query_position( true );
                if( !position ) {
                    break;
                }

                //add a zone that is relative to the avatar position
                // TODO: fix point types
                mgr.add( name, id, get_player_character().get_faction()->id, false, true,
                         position->first, position->second, options, true );
                zones = get_zones();
                active_index = zone_cnt - 1;

                int vehicle_zones = 0;
                for( zone_manager::ref_zone_data zone : zones ) {
                    if( zone.get().get_is_vehicle() ) {
                        vehicle_zones++;
                    }
                }

                active_index -= vehicle_zones;

                stuff_changed = true;
            } while( false );

            blink = false;
        } else if( action == "SHOW_ALL_ZONES" ) {
            show_all_zones = !show_all_zones;
            zones = get_zones();
            active_index = 0;
        } else if( action == "CHANGE_FACTION" ) {
            ui.invalidate_ui();
            std::string facname = zones_faction.str();
            string_input_popup()
            .description( _( "Show zones for faction:" ) )
            .width( 55 )
            .max_length( 256 )
            .edit( facname );
            zones_faction = faction_id( facname );
            zones = get_zones();
        } else if( action == "QUIT" ) {
            if( stuff_changed ) {
                const query_ynq_result res = query_ynq( _( "Save changes?" ) );
                switch( res ) {
                    case query_ynq_result::quit:
                        break;
                    case query_ynq_result::no:
                        save = false;
                        quit = true;
                        break;
                    case query_ynq_result::yes:
                        save = true;
                        quit = true;
                        break;
                }
            } else {
                save = false;
                quit = true;
            }
        } else if( zone_cnt > 0 ) {
            if( navigate_ui_list( action, active_index, scroll_rate, zone_cnt, true ) ) {
                blink = false;
            } else if( action == "REMOVE_ZONE" ) {
                if( active_index < zone_cnt ) {
                    mgr.remove( zones[active_index] );
                    zones = get_zones();
                    active_index--;

                    if( active_index < 0 ) {
                        active_index = 0;
                    }
                }
                blink = false;
                stuff_changed = true;

            } else if( action == "CONFIRM" ) {
                zone_data &zone = zones[active_index].get();

                uilist as_m;
                as_m.text = _( "What do you want to change:" );
                as_m.entries.emplace_back( 1, true, '1', _( "Edit name" ) );
                as_m.entries.emplace_back( 2, true, '2', _( "Edit type" ) );
                as_m.entries.emplace_back( 3, zone.get_options().has_options(), '3',
                                           zone.get_type() == zone_type_LOOT_CUSTOM ? _( "Edit filter" ) : _( "Edit options" ) );
                as_m.entries.emplace_back( 4, !zone.get_is_vehicle(), '4', _( "Edit position" ) );
                as_m.entries.emplace_back( 5, !zone.get_is_vehicle(), '5', _( "Move position" ) );
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
                        const auto pos = query_position( zone.get_is_personal() );
                        // FIXME: this comparison is nonsensival in the
                        // personal zone case because it's between different
                        // coordinate systems.
                        if( pos && ( pos->first != zone.get_start_point().raw() ||
                                     pos->second != zone.get_end_point().raw() ) ) {
                            zone.set_position( *pos );
                            stuff_changed = true;
                        }
                        break;
                    }
                    case 5: {
                        on_out_of_scope invalidate_current_ui( [&]() {
                            ui.mark_resize();
                        } );
                        restore_on_out_of_scope<bool> show_prev( show );
                        restore_on_out_of_scope<std::optional<tripoint>> zone_start_prev( zone_start );
                        restore_on_out_of_scope<std::optional<tripoint>> zone_end_prev( zone_end );
                        show = false;
                        zone_start = std::nullopt;
                        zone_end = std::nullopt;
                        ui.mark_resize();
                        static_popup message_pop;
                        message_pop.on_top( true );
                        message_pop.message( "%s", _( "Moving zone." ) );
                        const tripoint zone_local_start_point = m.getlocal( zone.get_start_point() );
                        const tripoint zone_local_end_point = m.getlocal( zone.get_end_point() );
                        // local position of the zone center, used to calculate the u.view_offset,
                        // could center the screen to the position it represents
                        tripoint view_center = m.getlocal( zone.get_center_point() );
                        const look_around_result result_local = look_around( false, view_center,
                                                                zone_local_start_point, false, false,
                                                                false, true, zone_local_end_point );
                        if( result_local.position ) {
                            const tripoint_abs_ms new_start_point = m.getglobal( *result_local.position );
                            const tripoint_abs_ms new_end_point = zone.get_end_point() - zone.get_start_point() +
                                                                  new_start_point;
                            if( new_start_point == zone.get_start_point() ) {
                                break; // Nothing changed, don't save
                            }
                            if( zone.get_is_personal() ) {
                                const tripoint_rel_ms new_start_point_rl = new_start_point - u.get_location();
                                const tripoint_rel_ms new_end_point_rl = new_end_point - u.get_location();
                                zone.set_position( std::make_pair( new_start_point_rl.raw(), new_end_point_rl.raw() ) );
                            } else {
                                zone.set_position( std::make_pair( new_start_point.raw(), new_end_point.raw() ) );
                            }
                            stuff_changed = true;
                        }
                    }
                    break;
                    default:
                        break;
                }

                blink = false;
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
                tripoint_abs_omt player_overmap_position = u.global_omt_location();
                tripoint_abs_omt zone_overmap =
                    project_to<coords::omt>( zones[active_index].get().get_center_point() );

                ui::omap::display_zones( player_overmap_position, zone_overmap, active_index );
            } else if( action == "ENABLE_ZONE" ) {
                zones[active_index].get().set_enabled( true );

                stuff_changed = true;

            } else if( action == "DISABLE_ZONE" ) {
                zones[active_index].get().set_enabled( false );

                stuff_changed = true;

            } else if( action == "TOGGLE_ZONE_DISPLAY" ) {
                zones[active_index].get().toggle_display();
                stuff_changed = true;

            } else if( action == "ENABLE_PERSONAL_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_personal() ) {
                        zone.set_enabled( true );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            } else if( action == "DISABLE_PERSONAL_ZONES" ) {
                bool zones_changed = false;

                for( const auto &i : zones ) {
                    zone_data &zone = i.get();
                    if( !zone.get_enabled() ) {
                        continue;
                    }
                    if( zone.get_is_personal() ) {
                        zone.set_enabled( false );
                        zones_changed = true;
                    }
                }

                stuff_changed = zones_changed;
            }
        }
    }
    zones_manager_open = false;
    ctxt.reset_timeout();
    zone_cb = nullptr;

    if( stuff_changed ) {
        zone_manager &zones = zone_manager::get_manager();
        if( !save ) {
            zones.load_zones( "zmgr-temp" );
        }

        zones.cache_data();
    }

    u.view_offset = stored_view_offset;
}

void game::pre_print_all_tile_info( const tripoint &lp, const catacurses::window &w_info,
                                    int &first_line, const int last_line,
                                    const visibility_variables &cache )
{
    // get global area info according to look_around caret position
    // TODO: fix point types
    const oter_id &cur_ter_m = overmap_buffer.ter( tripoint_abs_omt( ms_to_omt_copy( m.getabs(
                                   lp ) ) ) );
    // we only need the area name and then pass it to print_all_tile_info() function below
    const std::string area_name = cur_ter_m->get_name();
    print_all_tile_info( lp, w_info, area_name, 1, first_line, last_line, cache );
}

std::optional<std::vector<tripoint_bub_ms>> game::safe_route_to( Character &who,
        const tripoint_bub_ms &target, int threshold,
        const std::function<void( const std::string &msg )> &report ) const
{
    map &here = get_map();
    if( !who.sees( target ) ) {
        report( _( "You can't see the destination." ) );
        return std::nullopt;
    }
    if( rl_dist( who.pos_bub(), target ) <= threshold ) {
        return {}; // no need to move anywhere, already near destination
    }
    using route_t = std::vector<tripoint_bub_ms>;
    const auto get_shorter_route = [&who]( const route_t &a, const route_t &b ) {
        if( a.empty() ) {
            return b; // if route is empty return the non-empty one
        } else if( b.empty() ) {
            return a; // if route is empty return the non-empty one
        } else if( a.size() != b.size() ) {
            return a.size() < b.size() ? a : b; // if both non empty return shortest length in tiles
        } else {
            // if both are equal length in tiles return the one closest to target
            const float dist_a = rl_dist_exact( who.pos_bub().raw(), a.back().raw() );
            const float dist_b = rl_dist_exact( who.pos_bub().raw(), b.back().raw() );
            return dist_a < dist_b ? a : b;
        }
    };
    route_t shortest_route;
    std::unordered_set<tripoint> path_avoid;
    for( const tripoint_bub_ms &p : points_in_radius( who.pos_bub(), 60 ) ) {
        if( is_dangerous_tile( p.raw() ) ) {
            path_avoid.insert( p.raw() );
        }
    }
    for( const tripoint_bub_ms &p : here.points_in_radius( target, threshold, 0 ) ) {
        if( path_avoid.count( p.raw() ) > 0 ) {
            continue; // dont route to dangerous tiles
        }
        const route_t route = here.route( who.pos_bub(), p, who.get_pathfinding_settings(), path_avoid );
        if( route.empty() ) {
            continue; // no route
        }
        shortest_route = get_shorter_route( shortest_route, route );
    }
    if( shortest_route.empty() ) {
        report( _( "You can't travel there." ) );
        return std::nullopt;
    }
    return shortest_route;
}

std::optional<tripoint> game::look_around()
{
    tripoint center = u.pos() + u.view_offset;
    look_around_result result = look_around( /*show_window=*/true, center, center, false, false,
                                false );
    return result.position;
}

//look_around_result game::look_around( const bool show_window, tripoint &center,
//                                      const tripoint &start_point, bool has_first_point, bool select_zone, bool peeking )
look_around_result game::look_around(
    const bool show_window, tripoint &center, const tripoint &start_point, bool has_first_point,
    bool select_zone, bool peeking, bool is_moving_zone, const tripoint &end_point, bool change_lv )
{
    bVMonsterLookFire = false;

    temp_exit_fullscreen();

    // TODO: fix point types
    tripoint_bub_ms lp( is_moving_zone ? ( start_point + end_point ) / 2 : start_point ); // cursor
    int &lx = lp.x();
    int &ly = lp.y();
    int &lz = lp.z();

    int soffset = get_option<int>( "FAST_SCROLL_OFFSET" );
    static bool fast_scroll = false;

    std::unique_ptr<ui_adaptor> ui;
    catacurses::window w_info;
    if( show_window ) {
        ui = std::make_unique<ui_adaptor>();
        ui->on_screen_resize( [&]( ui_adaptor & ui ) {
            int panel_width = panel_manager::get_manager().get_current_layout().panels().begin()->get_width();
            int height = pixel_minimap_option ? TERMY - getmaxy( w_pixel_minimap ) : TERMY;

            // If particularly small, base height on panel width irrespective of other elements.
            // Value here is attempting to get a square-ish result assuming 1x2 proportioned font.
            if( height < panel_width / 2 ) {
                height = panel_width / 2;
            }

            int la_y = 0;
            int la_x = TERMX - panel_width;
            std::string position = get_option<std::string>( "LOOKAROUND_POSITION" );
            if( position == "left" ) {
                if( get_option<std::string>( "SIDEBAR_POSITION" ) == "right" ) {
                    la_x = panel_manager::get_manager().get_width_left();
                } else {
                    la_x = panel_manager::get_manager().get_width_left() - panel_width;
                }
            }
            int la_h = height;
            int la_w = panel_width;
            w_info = catacurses::newwin( la_h, la_w, point( la_x, la_y ) );

            ui.position_from_window( w_info );
        } );
        ui->mark_resize();
    }

    dbg( D_PEDANTIC_INFO ) << ": calling handle_input()";

    std::string action;
    input_context ctxt( "LOOK" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    if( change_lv && fov_3d_z_range > 0 ) {
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
    }
    ctxt.register_action( "TOGGLE_FAST_SCROLL" );
    ctxt.register_action( "CHANGE_MONSTER_NAME" );
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
    ctxt.register_action( "debug_scent_type" );
    ctxt.register_action( "debug_temp" );
    ctxt.register_action( "debug_visibility" );
    ctxt.register_action( "debug_lighting" );
    ctxt.register_action( "debug_radiation" );
    ctxt.register_action( "debug_hour_timer" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "zoom_out" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "toggle_pixel_minimap" );

    const int old_levz = m.get_abs_sub().z();
    const int min_levz = std::max( old_levz - fov_3d_z_range, -OVERMAP_DEPTH );
    const int max_levz = std::min( old_levz + fov_3d_z_range, OVERMAP_HEIGHT - 1 );

    m.update_visibility_cache( old_levz );
    const visibility_variables &cache = m.get_visibility_variables_cache();

    bool blink = true;
    look_around_result result;

    shared_ptr_fast<draw_callback_t> ter_indicator_cb;

    if( show_window && ui ) {
        ui->on_redraw( [&]( const ui_adaptor & ) {
            werase( w_info );
            draw_border( w_info );

            center_print( w_info, 0, c_white, string_format( _( "< <color_green>Look around</color> >" ) ) );

            creature_tracker &creatures = get_creature_tracker();
            monster *const mon = creatures.creature_at<monster>( lp, true );
            if( mon && u.sees( *mon ) ) {
                std::string mon_name_text = string_format( _( "%s - %s" ),
                                            ctxt.get_desc( "CHANGE_MONSTER_NAME" ),
                                            ctxt.get_action_name( "CHANGE_MONSTER_NAME" ) );
                mvwprintz( w_info, point( 1, getmaxy( w_info ) - 2 ), c_red, mon_name_text );
            }

            std::string fast_scroll_text = string_format( _( "%s - %s" ),
                                           ctxt.get_desc( "TOGGLE_FAST_SCROLL" ),
                                           ctxt.get_action_name( "TOGGLE_FAST_SCROLL" ) );
            mvwprintz( w_info, point( 1, getmaxy( w_info ) - 1 ), fast_scroll ? c_light_green : c_green,
                       fast_scroll_text );

            if( !ctxt.keys_bound_to( "toggle_pixel_minimap" ).empty() ) {
                std::string pixel_minimap_text = string_format( _( "%s - %s" ),
                                                 ctxt.get_desc( "toggle_pixel_minimap" ),
                                                 ctxt.get_action_name( "toggle_pixel_minimap" ) );
                right_print( w_info, getmaxy( w_info ) - 1, 1, pixel_minimap_option ? c_light_green : c_green,
                             pixel_minimap_text );
            }

            int first_line = 1;
            const int last_line = getmaxy( w_info ) - 2;
            // TODO: fix point types
            pre_print_all_tile_info( lp.raw(), w_info, first_line, last_line, cache );

            wnoutrefresh( w_info );
        } );
        ter_indicator_cb = make_shared_fast<draw_callback_t>( [&]() {
            // TODO: fix point types
            draw_look_around_cursor( lp.raw(), cache );
        } );
        add_draw_callback( ter_indicator_cb );
    }

    std::optional<tripoint> zone_start;
    std::optional<tripoint> zone_end;
    bool zone_blink = false;
    bool zone_cursor = true;
    shared_ptr_fast<draw_callback_t> zone_cb = create_zone_callback( zone_start, zone_end, zone_blink,
            zone_cursor, is_moving_zone );
    add_draw_callback( zone_cb );

    is_looking = true;
    const tripoint prev_offset = u.view_offset;
#if defined(TILES)
    const int prev_tileset_zoom = tileset_zoom;
    while( is_moving_zone && square_dist( start_point, end_point ) > 256 / get_zoom() &&
           get_zoom() != 4 ) {
        zoom_out();
    }
    mark_main_ui_adaptor_resize();
#endif
    do {
        u.view_offset = center - u.pos();
        if( select_zone ) {
            if( has_first_point ) {
                zone_start = start_point;
                // TODO: fix point types
                zone_end = lp.raw();
            } else {
                // TODO: fix point types
                zone_start = lp.raw();
                zone_end = std::nullopt;
            }
            // Actually accessed from the terrain overlay callback `zone_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            zone_blink = blink;
        }

        if( is_moving_zone ) {
            // TODO: fix point types
            zone_start = lp.raw() - ( start_point + end_point ) / 2 + start_point;
            // TODO: fix point types
            zone_end = lp.raw() - ( start_point + end_point ) / 2 + end_point;
            // Actually accessed from the terrain overlay callback `zone_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            zone_blink = blink;
        }
#if defined(TILES)
        // Mark cata_tiles draw caches as dirty
        tilecontext->set_draw_cache_dirty();
#endif
        invalidate_main_ui_adaptor();
        ui_manager::redraw();

        if( ( select_zone && has_first_point ) || is_moving_zone ) {
            ctxt.set_timeout( get_option<int>( "BLINK_SPEED" ) );
        }

        //Wait for input
        // only specify a timeout here if "EDGE_SCROLL" is enabled
        // otherwise use the previously set timeout
        const tripoint edge_scroll = mouse_edge_scrolling_terrain( ctxt );
        const int scroll_timeout = get_option<int>( "EDGE_SCROLL" );
        const bool edge_scrolling = edge_scroll != tripoint_zero && scroll_timeout >= 0;
        if( edge_scrolling ) {
            action = ctxt.handle_input( scroll_timeout );
        } else {
            action = ctxt.handle_input();
        }
        if( ( action == "LEVEL_UP" || action == "LEVEL_DOWN" || action == "MOUSE_MOVE" ||
              ctxt.get_direction( action ) ) && ( ( select_zone && has_first_point ) || is_moving_zone ) ) {
            blink = true; // Always draw blink symbols when moving cursor
        } else if( action == "TIMEOUT" ) {
            blink = !blink;
        }
        if( action == "LIST_ITEMS" ) {
            list_items_monsters();
        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "toggle_pixel_minimap" ) {
            toggle_pixel_minimap();

            if( show_window && ui ) {
                ui->mark_resize();
            }
        } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
            const int dz = action == "LEVEL_UP" ? 1 : -1;
            lz = clamp( lz + dz, min_levz, max_levz );
            center.z = clamp( center.z + dz, min_levz, max_levz );

            add_msg_debug( debugmode::DF_GAME, "levx: %d, levy: %d, levz: %d",
                           get_map().get_abs_sub().x(), get_map().get_abs_sub().y(), center.z );
            u.view_offset.z = center.z - u.posz();
            m.invalidate_map_cache( center.z );
            // Fix player character not visible from above
            m.build_map_cache( u.posz() );
            m.invalidate_visibility_cache();
        } else if( action == "TRAVEL_TO" ) {
            const std::optional<std::vector<tripoint_bub_ms>> try_route = safe_route_to( u, lp,
            0,  []( const std::string & msg ) {
                add_msg( msg );
            } );
            if( try_route.has_value() ) {
                u.set_destination( *try_route );
                continue;
            }
        } else if( action == "debug_scent" || action == "debug_scent_type" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_scent();
            }
        } else if( action == "debug_temp" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_temperature();
            }
        } else if( action == "debug_lighting" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_lighting();
            }
        } else if( action == "debug_transparency" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_transparency();
            }
        } else if( action == "debug_radiation" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                display_radiation();
            }
        } else if( action == "debug_hour_timer" ) {
            toggle_debug_hour_timer();
        } else if( action == "EXTENDED_DESCRIPTION" ) {
            // TODO: fix point types
            extended_description( lp.raw() );
        } else if( action == "CHANGE_MONSTER_NAME" ) {
            creature_tracker &creatures = get_creature_tracker();
            monster *const mon = creatures.creature_at<monster>( lp, true );
            if( mon ) {
                string_input_popup popup;
                popup
                .title( _( "Nickname:" ) )
                .width( 85 )
                .edit( mon->nickname );
            }
        } else if( action == "CENTER" ) {
            center = u.pos();
            lp = u.pos_bub();
            u.view_offset.z = 0;
        } else if( action == "MOUSE_MOVE" || action == "TIMEOUT" ) {
            // This block is structured this way so that edge scroll can work
            // whether the mouse is moving at the edge or simply stationary
            // at the edge. But even if edge scroll isn't in play, there's
            // other things for us to do here.

            if( edge_scrolling ) {
                center += action == "MOUSE_MOVE" ? edge_scroll * 2 : edge_scroll;
            } else if( action == "MOUSE_MOVE" ) {
                const std::optional<tripoint> mouse_pos = ctxt.get_coordinates( w_terrain, ter_view_p.xy(), true );
                if( mouse_pos ) {
                    lx = mouse_pos->x;
                    ly = mouse_pos->y;
                }
            }
        } else if( std::optional<tripoint> vec = ctxt.get_direction( action ) ) {
            if( fast_scroll ) {
                vec->x *= soffset;
                vec->y *= soffset;
            }

            lx = lx + vec->x;
            ly = ly + vec->y;
            center.x = center.x + vec->x;
            center.y = center.y + vec->y;
        } else if( action == "throw_blind" ) {
            result.peek_action = PA_BLIND_THROW;
        } else if( action == "zoom_in" ) {
            // TODO: fix point types
            center.x = lp.x();
            center.y = lp.y();
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            // TODO: fix point types
            center.x = lp.x();
            center.y = lp.y();
            zoom_out();
            mark_main_ui_adaptor_resize();
        }
    } while( action != "QUIT" && action != "CONFIRM" && action != "SELECT" && action != "TRAVEL_TO" &&
             action != "throw_blind" );

    if( center.z != old_levz ) {
        m.invalidate_map_cache( old_levz );
        m.build_map_cache( old_levz );
        u.view_offset.z = 0;
    }

    ctxt.reset_timeout();
    u.view_offset = prev_offset;
    zone_cb = nullptr;
    is_looking = false;

    reenter_fullscreen();
    bVMonsterLookFire = true;

    if( action == "CONFIRM" || action == "SELECT" ) {
        // TODO: fix point types
        result.position = is_moving_zone ? zone_start : lp.raw();
    }

#if defined(TILES)
    if( is_moving_zone && get_zoom() != prev_tileset_zoom ) {
        // Reset the tileset zoom to the previous value
        set_zoom( prev_tileset_zoom );
        mark_main_ui_adaptor_resize();
    }
#endif

    return result;
}

look_around_result game::look_around( look_around_params looka_params )
{
    return look_around( looka_params.show_window, looka_params.center, looka_params.start_point,
                        looka_params.has_first_point, looka_params.select_zone, looka_params.peeking, false, tripoint_zero,
                        looka_params.change_lv );
}

static void add_item_recursive( std::vector<std::string> &item_order,
                                std::map<std::string, map_item_stack> &temp_items, const item *it, const tripoint &relative_pos )
{
    const std::string name = it->tname();

    if( std::find( item_order.begin(), item_order.end(), name ) == item_order.end() ) {
        item_order.push_back( name );
        temp_items[name] = map_item_stack( it, relative_pos );
    } else {
        temp_items[name].add_at_pos( it, relative_pos );
    }

    for( const item *content : it->all_known_contents() ) {
        add_item_recursive( item_order, temp_items, content, relative_pos );
    }
}

std::vector<map_item_stack> game::find_nearby_items( int iRadius )
{
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> item_order;

    if( u.is_blind() ) {
        return ret;
    }

    for( tripoint &points_p_it : closest_points_first( u.pos(), iRadius ) ) {
        if( points_p_it.y >= u.posy() - iRadius && points_p_it.y <= u.posy() + iRadius &&
            u.sees( points_p_it ) &&
            m.sees_some_items( points_p_it, u ) ) {

            for( item &elem : m.i_at( points_p_it ) ) {
                const tripoint relative_pos = points_p_it - u.pos();

                add_item_recursive( item_order, temp_items, &elem, relative_pos );
            }
        }
    }

    for( auto &elem : item_order ) {
        ret.push_back( temp_items[elem] );
    }

    return ret;
}

void draw_trail( const tripoint &start, const tripoint &end, const bool bDrawX )
{
    std::vector<tripoint> pts;
    avatar &player_character = get_avatar();
    tripoint center = player_character.pos() + player_character.view_offset;
    if( start != end ) {
        //Draw trail
        pts = line_to( start, end, 0, 0 );
    } else {
        //Draw point
        pts.emplace_back( start );
    }

    g->draw_line( end, center, pts );
    if( bDrawX ) {
        char sym = 'X';
        if( end.z > center.z ) {
            sym = '^';
        } else if( end.z < center.z ) {
            sym = 'v';
        }
        if( pts.empty() ) {
            mvwputch( g->w_terrain, point( POSX, POSY ), c_white, sym );
        } else {
            mvwputch( g->w_terrain, pts.back().xy() - player_character.view_offset.xy() +
                      point( POSX - player_character.posx(), POSY - player_character.posy() ),
                      c_white, sym );
        }
    }
}

void game::draw_trail_to_square( const tripoint &t, bool bDrawX )
{
    ::draw_trail( u.pos(), u.pos() + t, bDrawX );
}

static void centerlistview( const tripoint &active_item_position, int ui_width )
{
    Character &u = get_avatar();
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z = active_item_position.z;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            u.view_offset.x = active_item_position.x;
            u.view_offset.y = active_item_position.y;
        } else {
            point pos( active_item_position.xy() + point( POSX, POSY ) );

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

            if( pos.x < 0 ) {
                u.view_offset.x = pos.x;
            } else if( pos.x >= terrain_width ) {
                u.view_offset.x = pos.x - ( terrain_width - 1 );
            } else {
                u.view_offset.x = 0;
            }

            if( pos.y < 0 ) {
                u.view_offset.y = pos.y;
            } else if( pos.y >= TERRAIN_WINDOW_HEIGHT ) {
                u.view_offset.y = pos.y - ( TERRAIN_WINDOW_HEIGHT - 1 );
            } else {
                u.view_offset.y = 0;
            }
        }
    }

}

#if defined(TILES)
static constexpr int MAXIMUM_ZOOM_LEVEL = 4;
#endif
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

void game::zoom_out_overmap()
{
#if defined(TILES)
    if( overmap_tileset_zoom > MAXIMUM_ZOOM_LEVEL ) {
        overmap_tileset_zoom /= 2;
    } else {
        overmap_tileset_zoom = 64;
    }
    overmap_tilecontext->set_draw_scale( overmap_tileset_zoom );
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

void game::zoom_in_overmap()
{
#if defined(TILES)
    if( overmap_tileset_zoom == 64 ) {
        overmap_tileset_zoom = MAXIMUM_ZOOM_LEVEL;
    } else {
        overmap_tileset_zoom *= 2;
    }
    overmap_tilecontext->set_draw_scale( overmap_tileset_zoom );
#endif
}

void game::reset_zoom()
{
#if defined(TILES)
    tileset_zoom = DEFAULT_TILESET_ZOOM;
    rescale_tileset( tileset_zoom );
#endif // TILES
}

void game::set_zoom( const int level )
{
#if defined(TILES)
    if( tileset_zoom != level ) {
        tileset_zoom = level;
        rescale_tileset( tileset_zoom );
    }
#else
    static_cast<void>( level );
#endif // TILES
}

int game::get_zoom() const
{
#if defined(TILES)
    return tileset_zoom;
#else
    return DEFAULT_TILESET_ZOOM;
#endif
}

int game::get_moves_since_last_save() const
{
    return moves_since_last_save;
}

int game::get_user_action_counter() const
{
    return user_action_counter;
}

#if defined(TILES)
bool game::take_screenshot( const std::string &path ) const
{
    return save_screenshot( path );
}

bool game::take_screenshot() const
{
    // check that the current '<world>/screenshots' directory exists
    std::stringstream map_directory;
    map_directory << PATH_INFO::world_base_save_path() << "/screenshots/";
    assure_dir_exist( map_directory.str() );

    // build file name: <map_dir>/screenshots/[<character_name>]_<date>.png
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string tmp_file_name = string_format( "[%s]_%s.png", get_player_character().get_name(),
                                      timestamp_now() );

    std::string file_name = ensure_valid_file_name( tmp_file_name );
    auto current_file_path = map_directory.str() + file_name;

    // Take a screenshot of the viewport.
    if( take_screenshot( current_file_path ) ) {
        popup( _( "Successfully saved your screenshot to: %s" ), map_directory.str() );
        return true;
    } else {
        popup( _( "An error occurred while trying to save the screenshot." ) );
        return false;
    }
}
#else
bool game::take_screenshot( const std::string &/*path*/ ) const
{
    return false;
}

bool game::take_screenshot() const
{
    popup( _( "This binary was not compiled with tiles support." ) );
    return false;
}
#endif

//helper method so we can keep list_items shorter
void game::reset_item_list_state( const catacurses::window &window, int height, bool bRadiusSort )
{
    const int width = getmaxx( window );
    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( window, point( i, 0 ), c_light_gray, LINE_OXOX ); // -
            mvwputch( window, point( i, TERMY - height - 1 ), c_light_gray,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - height ) {
            mvwputch( window, point( 0, i ), c_light_gray, LINE_XOXO ); // |
            mvwputch( window, point( width - 1, i ), c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( window, point_zero, c_light_gray, LINE_OXXO ); // |^
    mvwputch( window, point( width - 1, 0 ), c_light_gray, LINE_OOXX ); // ^|

    mvwputch( window, point( 0, TERMY - height - 1 ), c_light_gray,
              LINE_XXXO ); // |-
    mvwputch( window, point( width - 1, TERMY - height - 1 ), c_light_gray,
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
    point pos( gap_spaces + ( width - usedwidth ) / 2, TERMY - height - 1 );

    for( int i = 0; i < n; i++ ) {
        pos.x += shortcut_print( window, pos, c_white, c_light_green,
                                 tokens[i] ) + gap_spaces;
    }
}

void game::list_items_monsters()
{
    // Search whole reality bubble because each function internally verifies
    // the visibility of the items / monsters in question.
    std::vector<Creature *> mons = u.get_visible_creatures( 60 );
    const std::vector<map_item_stack> items = find_nearby_items( 60 );

    if( mons.empty() && items.empty() ) {
        add_msg( m_info, _( "You don't see any items or monsters around you!" ) );
        return;
    }

    std::sort( mons.begin(), mons.end(), [&]( const Creature * lhs, const Creature * rhs ) {
        if( !u.has_trait( trait_INATTENTIVE ) ) {
            const Creature::Attitude att_lhs = lhs->attitude_to( u );
            const Creature::Attitude att_rhs = rhs->attitude_to( u );

            return att_lhs < att_rhs || ( att_lhs == att_rhs
                                          && rl_dist( u.pos(), lhs->pos() ) < rl_dist( u.pos(), rhs->pos() ) );
        } else { // Sort just by ditance if player has inattentive trait
            return ( rl_dist( u.pos(), lhs->pos() ) < rl_dist( u.pos(), rhs->pos() ) );
        }

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

    if( ret == game::vmenu_ret::FIRE ) {
        avatar_action::fire_wielded_weapon( u );
    }
    reenter_fullscreen();
}

static std::string list_items_filter_history_help()
{
    return colorize( _( "UP: history, CTRL-U: clear line, ESC: abort, ENTER: save" ), c_green );
}

game::vmenu_ret game::list_items( const std::vector<map_item_stack> &item_list )
{
    std::vector<map_item_stack> ground_items = item_list;
    int iInfoHeight = 0;
    int iMaxRows = 0;
    int width = 0;
    int max_name_width = 0;

    //find max length of item name and resize window width
    for( const map_item_stack &cur_item : ground_items ) {
        const int item_len = utf8_width( remove_color_tags( cur_item.example->display_name() ) ) + 15;
        if( item_len > max_name_width ) {
            max_name_width = item_len;
        }
    }

    tripoint active_pos;
    map_item_stack *activeItem = nullptr;

    catacurses::window w_items;
    catacurses::window w_items_border;
    catacurses::window w_item_info;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        iInfoHeight = std::min( 25, TERMY / 2 );
        iMaxRows = TERMY - iInfoHeight - 2;

        width = clamp( max_name_width, 55, TERMX / 3 );

        const int offsetX = TERMX - width;

        w_items = catacurses::newwin( TERMY - 2 - iInfoHeight,
                                      width - 2, point( offsetX + 1, 1 ) );
        w_items_border = catacurses::newwin( TERMY - iInfoHeight,
                                             width, point( offsetX, 0 ) );
        w_item_info = catacurses::newwin( iInfoHeight, width,
                                          point( offsetX, TERMY - iInfoHeight ) );

        if( activeItem ) {
            centerlistview( active_pos, width );
        }

        ui.position( point( offsetX, 0 ), point( width, TERMY ) );
    } );
    ui.mark_resize();

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

    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items =
        !sFilter.empty() ? filter_item_stacks( ground_items, sFilter ) : ground_items;
    int highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
    int lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
    int iItemNum = ground_items.size();

    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    int iActive = 0; // Item index that we're looking at
    bool refilter = true;
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    std::map<int, std::string> mSortCategory;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "LEFT", to_translation( "Previous item" ) );
    ctxt.register_action( "RIGHT", to_translation( "Next item" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
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

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        reset_item_list_state( w_items_border, iInfoHeight, sort_radius );

        int iStartPos = 0;
        if( ground_items.empty() ) {
            wnoutrefresh( w_items_border );
            mvwprintz( w_items, point( 2, 10 ), c_white, _( "You don't see any items around you!" ) );
        } else {
            werase( w_items );
            calcStartPos( iStartPos, iActive, iMaxRows, iItemNum );
            int iNum = 0;
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

                if( iNum >= iStartPos && iNum < iStartPos + std::min( iMaxRows, iItemNum ) ) {
                    int iThisPage = 0;
                    if( !mSortCategory[iNum].empty() ) {
                        iCatSortOffset++;
                        mvwprintz( w_items, point( 1, iNum - iStartPos ), c_magenta, mSortCategory[iNum] );
                    } else {
                        if( iNum == iActive ) {
                            iThisPage = page_num;
                        }
                        std::string sText;
                        if( iter->vIG.size() > 1 ) {
                            sText += string_format( "[%d/%d] (%d) ", iThisPage + 1, iter->vIG.size(), iter->totalcount );
                        }
                        sText += iter->example->tname();
                        if( iter->vIG[iThisPage].count > 1 ) {
                            sText += string_format( "[%d]", iter->vIG[iThisPage].count );
                        }

                        nc_color col = c_light_gray;
                        if( iNum == iActive ) {
                            col = hilite( c_white );
                        } else if( high ) {
                            col = c_yellow;
                        } else if( low ) {
                            col = c_red;
                        } else {
                            col = iter->example->color_in_inventory();
                        }
                        trim_and_print( w_items, point( 1, iNum - iStartPos ), width - 9, col, sText );
                        const int numw = iItemNum > 9 ? 2 : 1;
                        const point p( iter->vIG[iThisPage].pos.xy() );
                        mvwprintz( w_items, point( width - 6 - numw, iNum - iStartPos ),
                                   iNum == iActive ? c_light_green : c_light_gray,
                                   "%*d %s", numw, rl_dist( point_zero, p ),
                                   direction_name_short( direction_from( point_zero, p ) ) );
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

            if( iItemNum > 0 && activeItem ) {
                std::vector<iteminfo> vThisItem;
                std::vector<iteminfo> vDummy;
                activeItem->vIG[page_num].it->info( true, vThisItem );

                item_info_data dummy( "", "", vThisItem, vDummy, iScrollPos );
                dummy.without_getch = true;
                dummy.without_border = true;

                draw_item_info( w_item_info, dummy );
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, point_south );
            wnoutrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty() || !activeItem;
        draw_custom_border( w_item_info, bDrawLeft, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

        if( iItemNum > 0 && activeItem ) {
            // print info window title: < item name >
            mvwprintw( w_item_info, point( 2, 0 ), "< " );
            trim_and_print( w_item_info, point( 4, 0 ), width - 8,
                            activeItem->vIG[page_num].it->color_in_inventory(),
                            activeItem->vIG[page_num].it->display_name() );
            wprintw( w_item_info, " >" );
            // move the cursor to the selected item (for screen readers)
            ui.set_cursor( w_items, point( 1, iActive - iStartPos ) );
        }

        wnoutrefresh( w_items );
        wnoutrefresh( w_item_info );
    } );

    std::optional<tripoint> trail_start;
    std::optional<tripoint> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );

    do {
        if( action == "COMPARE" && activeItem ) {
            game_menus::inv::compare( u, active_pos );
        } else if( action == "FILTER" ) {
            ui.invalidate_ui();
            string_input_popup()
            .title( _( "Filter:" ) )
            .width( 55 )
            .description( item_filter_rule_string( item_filter_type::FILTER ) + "\n\n"
                          + list_items_filter_history_help() )
            .desc_color( c_white )
            .identifier( "item_filter" )
            .max_length( 256 )
            .edit( sFilter );
            refilter = true;
            addcategory = !sort_radius;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            refilter = true;
            uistate.list_item_filter_active = false;
            addcategory = !sort_radius;
        } else if( action == "EXAMINE" && !filtered_items.empty() && activeItem ) {
            std::vector<iteminfo> vThisItem;
            std::vector<iteminfo> vDummy;
            activeItem->vIG[page_num].it->info( true, vThisItem );

            item_info_data info_data( activeItem->vIG[page_num].it->tname(),
                                      activeItem->vIG[page_num].it->type_name(), vThisItem,
                                      vDummy );
            info_data.handle_scrolling = true;

            draw_item_info( [&]() -> catacurses::window {
                return catacurses::newwin( TERMY, width - 5, point_zero );
            }, info_data );
        } else if( action == "PRIORITY_INCREASE" ) {
            ui.invalidate_ui();
            list_item_upvote = string_input_popup()
                               .title( _( "High Priority:" ) )
                               .width( 55 )
                               .text( list_item_upvote )
                               .description( item_filter_rule_string( item_filter_type::HIGH_PRIORITY ) + "\n\n"
                                             + list_items_filter_history_help() )
                               .desc_color( c_white )
                               .identifier( "list_item_priority" )
                               .max_length( 256 )
                               .query_string();
            refilter = true;
            addcategory = !sort_radius;
            uistate.list_item_priority_active = !list_item_upvote.empty();
        } else if( action == "PRIORITY_DECREASE" ) {
            ui.invalidate_ui();
            list_item_downvote = string_input_popup()
                                 .title( _( "Low Priority:" ) )
                                 .width( 55 )
                                 .text( list_item_downvote )
                                 .description( item_filter_rule_string( item_filter_type::LOW_PRIORITY ) + "\n\n"
                                               + list_items_filter_history_help() )
                                 .desc_color( c_white )
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
        } else if( action == "TRAVEL_TO" && activeItem ) {
            // try finding route to the tile, or one tile away from it
            const std::optional<std::vector<tripoint_bub_ms>> try_route =
            safe_route_to( u, u.pos_bub() + active_pos, 1, []( const std::string & msg ) {
                popup( msg );
            } );
            if( try_route.has_value() ) {
                u.set_destination( *try_route );
                break;
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
                const std::string &cat_name =
                    filtered_items[i].example->get_category_of_contents().name();
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

        const int item_info_scroll_lines = catacurses::getmaxy( w_item_info ) - 4;
        const int scroll_rate = iItemNum > 20 ? 10 : 3;

        if( action == "UP" ) {
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
        } else if( action == "PAGE_DOWN" ) {
            iScrollPos = 0;
            page_num = 0;
            if( iActive == iItemNum - 1 ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else if( iActive + scroll_rate >= iItemNum ) {
                iActive = iItemNum - 1;
            } else {
                iActive += +scroll_rate - 1;
                do {
                    iActive++;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "PAGE_UP" ) {
            iScrollPos = 0;
            page_num = 0;
            if( mSortCategory[0].empty() ? iActive == 0 : iActive == 1 ) {
                iActive = iItemNum - 1;
            } else if( iActive <= scroll_rate ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            } else {
                iActive += -scroll_rate + 1;
                do {
                    iActive--;
                } while( !mSortCategory[iActive].empty() );
            }
        } else if( action == "RIGHT" ) {
            if( !filtered_items.empty() && activeItem ) {
                if( ++page_num >= static_cast<int>( activeItem->vIG.size() ) ) {
                    page_num = activeItem->vIG.size() - 1;
                }
            }
        } else if( action == "LEFT" ) {
            page_num = std::max( 0, page_num - 1 );
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            iScrollPos -= item_info_scroll_lines;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            iScrollPos += item_info_scroll_lines;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        }

        active_pos = tripoint_zero;
        activeItem = nullptr;

        if( mSortCategory[iActive].empty() ) {
            auto iter = filtered_items.begin();
            for( int iNum = 0; iter != filtered_items.end() && iNum < iActive; iNum++ ) {
                if( mSortCategory[iNum].empty() ) {
                    ++iter;
                }
            }
            if( iter != filtered_items.end() ) {
                active_pos = iter->vIG[page_num].pos;
                activeItem = &( *iter );
            }
        }

        if( activeItem ) {
            centerlistview( active_pos, width );
            trail_start = u.pos();
            trail_end = u.pos() + active_pos;
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = true;
        } else {
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;
    return game::vmenu_ret::QUIT;
}

game::vmenu_ret game::list_monsters( const std::vector<Creature *> &monster_list )
{
    const int iInfoHeight = 15;
    const int width = 55;
    int offsetX = 0;
    int iMaxRows = 0;

    catacurses::window w_monsters;
    catacurses::window w_monsters_border;
    catacurses::window w_monster_info;
    catacurses::window w_monster_info_border;

    Creature *cCurMon = nullptr;
    tripoint iActivePos;

    bool hide_ui = false;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        if( hide_ui ) {
            ui.position( point_zero, point_zero );
        } else {
            offsetX = TERMX - width;
            iMaxRows = TERMY - iInfoHeight - 1;

            w_monsters = catacurses::newwin( iMaxRows, width - 2, point( offsetX + 1,
                                             1 ) );
            w_monsters_border = catacurses::newwin( iMaxRows + 1, width, point( offsetX,
                                                    0 ) );
            w_monster_info = catacurses::newwin( iInfoHeight - 2, width - 2,
                                                 point( offsetX + 1, TERMY - iInfoHeight + 1 ) );
            w_monster_info_border = catacurses::newwin( iInfoHeight, width, point( offsetX,
                                    TERMY - iInfoHeight ) );

            if( cCurMon ) {
                centerlistview( iActivePos, width );
            }

            ui.position( point( offsetX, 0 ), point( width, TERMY ) );
        }
    } );
    ui.mark_resize();

    int max_gun_range = 0;
    if( u.get_wielded_item() ) {
        max_gun_range = u.get_wielded_item()->gun_range( &u );
    }

    const tripoint stored_view_offset = u.view_offset;
    u.view_offset = tripoint_zero;

    int iActive = 0; // monster index that we're looking at

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_navigate_ui_list();
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "zoom_in" );
    ctxt.register_action( "zoom_out" );
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
    const bool player_knows = !u.has_trait( trait_INATTENTIVE );
    if( player_knows ) {
        for( int i = 0, last_attitude = -1; i < static_cast<int>( monster_list.size() ); i++ ) {
            const Creature::Attitude attitude = monster_list[i]->attitude_to( u );
            if( static_cast<int>( attitude ) != last_attitude ) {
                mSortCategory[i + mSortCategory.size()] = attitude;
                last_attitude = static_cast<int>( attitude );
            }
        }
    }

    ui.on_redraw( [&]( const ui_adaptor & ) {
        if( !hide_ui ) {
            draw_custom_border( w_monsters_border, 1, 1, 1, 1, 1, 1, LINE_XOXO, LINE_XOXO );
            draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

            mvwprintz( w_monsters_border, point( 2, 0 ), c_light_green, "<Tab> " );
            wprintz( w_monsters_border, c_white, _( "Monsters" ) );

            if( monster_list.empty() ) {
                werase( w_monsters );
                mvwprintz( w_monsters, point( 2, iMaxRows / 3 ), c_white,
                           _( "You don't see any monsters around you!" ) );
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
                int iStartPos = 0;
                // use selected row get the start row
                calcStartPos( iStartPos, iSelPos, iMaxRows - 1, iMenuSize );

                // get first visible monster and category
                int iCurMon = iStartPos;
                auto CatSortIter = mSortCategory.cbegin();
                while( CatSortIter != mSortCategory.cend() && CatSortIter->first < iStartPos ) {
                    ++CatSortIter;
                    --iCurMon;
                }

                std::string monNameSelected;
                std::string sSafemode;
                const int endY = std::min<int>( iMaxRows - 1, iMenuSize );
                for( int y = 0; y < endY; ++y ) {

                    if( player_knows && CatSortIter != mSortCategory.cend() ) {
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
                    Creature *critter = monster_list[iCurMon];
                    const bool selected = iCurMon == iActive;
                    ++iCurMon;
                    if( critter->sees( u ) && player_knows ) {
                        mvwprintz( w_monsters, point( 0, y ), c_yellow, "!" );
                    }
                    bool is_npc = false;
                    const monster *m = dynamic_cast<monster *>( critter );
                    const npc     *p = dynamic_cast<npc *>( critter );
                    nc_color name_color = critter->basic_symbol_color();

                    if( selected ) {
                        name_color = hilite( name_color );
                    }

                    if( m != nullptr ) {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, m->name() );
                    } else {
                        trim_and_print( w_monsters, point( 1, y ), width - 32, name_color, critter->disp_name() );
                        is_npc = true;
                    }

                    if( selected && !get_safemode().empty() ) {
                        monNameSelected = is_npc ? get_safemode().npc_type_name() : m->name();

                        if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                            sSafemode = _( "<R>emove from safe mode blacklist" );
                        } else {
                            sSafemode = _( "<A>dd to safe mode blacklist" );
                        }
                    }

                    nc_color color = c_white;
                    std::string sText;

                    if( m != nullptr ) {
                        m->get_HP_Bar( color, sText );
                    } else {
                        std::tie( sText, color ) =
                            ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
                    }
                    mvwprintz( w_monsters, point( width - 31, y ), color, sText );
                    const int bar_max_width = 5;
                    const int bar_width = utf8_width( sText );
                    for( int i = 0; i < bar_max_width - bar_width; ++i ) {
                        mvwprintz( w_monsters, point( width - 27 - i, y ), c_white, "." );
                    }

                    if( m != nullptr ) {
                        const auto att = m->get_attitude();
                        sText = att.first;
                        color = att.second;
                    } else if( p != nullptr ) {
                        sText = npc_attitude_name( p->get_attitude() );
                        color = p->symbol_color();
                    }
                    if( !player_knows ) {
                        sText = _( "Unknown" );
                        color = c_yellow;
                    }
                    mvwprintz( w_monsters, point( width - 19, y ), color, sText );

                    const int mon_dist = rl_dist( u.pos(), critter->pos() );
                    const int numd = mon_dist > 999 ? 4 :
                                     mon_dist > 99 ? 3 :
                                     mon_dist > 9 ? 2 : 1;

                    trim_and_print( w_monsters, point( width - ( 5 + numd ), y ), 6 + numd,
                                    selected ? c_light_green : c_light_gray,
                                    "%*d %s",
                                    numd, mon_dist,
                                    direction_name_short( direction_from( u.pos(), critter->pos() ) ) );
                }

                mvwprintz( w_monsters_border, point( ( width / 2 ) - numw - 2, 0 ), c_light_green, " %*d", numw,
                           iActive + 1 );
                wprintz( w_monsters_border, c_white, " / %*d ", numw, static_cast<int>( monster_list.size() ) );

                werase( w_monster_info );
                if( cCurMon ) {
                    cCurMon->print_info( w_monster_info, 1, iInfoHeight - 3, 1 );
                }

                draw_custom_border( w_monster_info_border, 1, 1, 1, 1, LINE_XXXO, LINE_XOXX, 1, 1 );

                if( bVMonsterLookFire ) {
                    mvwprintw( w_monster_info_border, point_east, "< " );
                    wprintz( w_monster_info_border, c_light_green, ctxt.press_x( "look" ) );
                    wprintz( w_monster_info_border, c_light_gray, " %s", _( "to look around" ) );

                    if( cCurMon && rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                        std::string press_to_fire_text = string_format( _( "%s %s" ),
                                                         ctxt.press_x( "fire" ),
                                                         string_format( _( "<color_light_gray>to shoot</color>" ) ) );
                        right_print( w_monster_info_border, 0, 3, c_light_green, press_to_fire_text );
                    }
                    wprintw( w_monster_info_border, " >" );
                }

                if( !get_safemode().empty() ) {
                    if( get_safemode().has_rule( monNameSelected, Creature::Attitude::ANY ) ) {
                        sSafemode = _( "<R>emove from safe mode blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safe mode blacklist" );
                    }

                    shortcut_print( w_monsters, point( 2, getmaxy( w_monsters ) - 1 ),
                                    c_white, c_light_green, sSafemode );
                }

                draw_scrollbar( w_monsters_border, iActive, iMaxRows, static_cast<int>( monster_list.size() ),
                                point_south );
            }

            wnoutrefresh( w_monsters_border );
            wnoutrefresh( w_monster_info_border );
            wnoutrefresh( w_monsters );
            wnoutrefresh( w_monster_info );
        }
    } );

    std::optional<tripoint> trail_start;
    std::optional<tripoint> trail_end;
    bool trail_end_x = false;
    shared_ptr_fast<draw_callback_t> trail_cb = create_trail_callback( trail_start, trail_end,
            trail_end_x );
    add_draw_callback( trail_cb );
    const int recmax = static_cast<int>( monster_list.size() );
    const int scroll_rate = recmax > 20 ? 10 : 3;

    do {
        if( navigate_ui_list( action, iActive, scroll_rate, recmax, true ) ) {
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return game::vmenu_ret::CHANGE_TAB;
        } else if( action == "zoom_in" ) {
            zoom_in();
            mark_main_ui_adaptor_resize();
        } else if( action == "zoom_out" ) {
            zoom_out();
            mark_main_ui_adaptor_resize();
        } else if( action == "SAFEMODE_BLACKLIST_REMOVE" ) {
            const monster *m = dynamic_cast<monster *>( cCurMon );
            const std::string monName = ( m != nullptr ) ? m->name() : "human";

            if( get_safemode().has_rule( monName, Creature::Attitude::ANY ) ) {
                get_safemode().remove_rule( monName, Creature::Attitude::ANY );
            }
        } else if( action == "SAFEMODE_BLACKLIST_ADD" ) {
            if( !get_safemode().empty() ) {
                const monster *m = dynamic_cast<monster *>( cCurMon );
                const std::string monName = ( m != nullptr ) ? m->name() : "human";

                get_safemode().add_rule( monName, Creature::Attitude::ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                                         rule_state::BLACKLISTED );
            }
        } else if( action == "look" ) {
            hide_ui = true;
            ui.mark_resize();
            look_around();
            hide_ui = false;
            ui.mark_resize();
        } else if( action == "fire" ) {
            if( cCurMon != nullptr && rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                u.last_target = shared_from( *cCurMon );
                u.recoil = MAX_RECOIL;
                u.view_offset = stored_view_offset;
                return game::vmenu_ret::FIRE;
            }
        }

        if( iActive >= 0 && static_cast<size_t>( iActive ) < monster_list.size() ) {
            cCurMon = monster_list[iActive];
            iActivePos = cCurMon->pos() - u.pos();
            centerlistview( iActivePos, width );
            trail_start = u.pos();
            trail_end = cCurMon->pos();
            // Actually accessed from the terrain overlay callback `trail_cb` in the
            // call to `ui_manager::redraw`.
            //NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
            trail_end_x = false;
        } else {
            cCurMon = nullptr;
            iActivePos = tripoint_zero;
            u.view_offset = stored_view_offset;
            trail_start = trail_end = std::nullopt;
        }
        invalidate_main_ui_adaptor();

        ui_manager::redraw();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return game::vmenu_ret::QUIT;
}

void game::insert_item( drop_locations &targets )
{
    if( targets.empty() || !targets.front().first ) {
        return;
    }
    std::string title = string_format( _( "%s: %s and %d items" ), _( "Insert item" ),
                                       targets.front().first->tname(), targets.size() - 1 );
    item_location item_loc = inv_map_splice( [ &, targets]( const item_location & it ) {
        if( targets.front().first.parent_item() == it ) {
            return false;
        }
        return it->is_container() && !it->is_corpse() && rate_action_insert( u, it ) == hint_rating::good;
    }, title, 1, _( "You have no container to insert items." ) );

    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    u.assign_activity( insert_item_activity_actor( item_loc, targets, true ) );
}

void game::insert_item()
{
    item_location item_loc = inv_map_splice( [&]( const item_location & it ) {
        return it->is_container() && !it->is_corpse() && rate_action_insert( u, it ) == hint_rating::good;
    }, _( "Insert item" ), 1, _( "You have no container to insert items." ) );

    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    game_menus::inv::insert_items( u, item_loc );
}

void game::unload_container()
{
    if( const std::optional<tripoint> pnt = choose_adjacent( _( "Unload where?" ) ) ) {
        u.drop( game_menus::inv::unload_container( u ), *pnt );
    }
}

void game::drop_in_direction( const tripoint &pnt )
{
    u.drop( game_menus::inv::multidrop( u ), pnt );
}

// Used to set up the first Hotkey in the display set
static std::optional<input_event> get_initial_hotkey( const size_t menu_index )
{
    return menu_index == 0
           ? hotkey_for_action( ACTION_BUTCHER, /*maximum_modifier_count=*/1 )
           : std::nullopt;
}

// Returns a vector of pairs.
//    Pair.first is the iterator to the first item with a unique tname.
//    Pair.second is the number of equivalent items per unique tname
// There are options for optimization here, but the function is hit infrequently
// enough that optimizing now is not a useful time expenditure.
static std::vector<std::pair<map_stack::iterator, int>> generate_butcher_stack_display(
            const std::vector<map_stack::iterator> &its )
{
    std::vector<std::pair<map_stack::iterator, int>> result;
    std::vector<std::string> result_strings;
    result.reserve( its.size() );
    result_strings.reserve( its.size() );

    for( const map_stack::iterator &it : its ) {
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
        result[s].second += it->count_by_charges() ? it->charges : 1;
    }

    return result;
}

// Corpses are always individual items
// Just add them individually to the menu
static void add_corpses( uilist &menu, const std::vector<map_stack::iterator> &its,
                         size_t &menu_index )
{
    std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

    for( const map_stack::iterator &it : its ) {
        menu.addentry( menu_index++, true, hotkey, it->get_mtype()->nname() );
        hotkey = std::nullopt;
    }
}

// Salvagables stack so we need to pass in a stack vector rather than an item index vector
static void add_salvagables( uilist &menu,
                             const std::vector<std::pair<map_stack::iterator, int>> &stacks,
                             size_t &menu_index, const salvage_actor &salvage_iuse )
{
    if( !stacks.empty() ) {
        std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

            //~ Name and number of items listed for cutting up
            const std::string &msg = string_format( pgettext( "butchery menu", "Cut up %s (%d)" ),
                                                    it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( salvage_iuse.time_to_cut_up( it ) / 100 ) ) );
            hotkey = std::nullopt;
        }
    }
}

// Disassemblables stack so we need to pass in a stack vector rather than an item index vector
static void add_disassemblables( uilist &menu,
                                 const std::vector<std::pair<map_stack::iterator, int>> &stacks, size_t &menu_index )
{
    if( !stacks.empty() ) {
        std::optional<input_event> hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = *stack.first;

            //~ Name, number of items and time to complete disassembling
            const std::string &msg = string_format( pgettext( "butchery menu", "Disassemble %s (%d)" ),
                                                    it.tname(), stack.second );
            recipe uncraft_recipe;
            if( it.typeId() == itype_disassembly ) {
                uncraft_recipe = it.get_making();
            } else {
                uncraft_recipe = recipe_dictionary::get_uncraft( it.typeId() );
            }
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( uncraft_recipe.time_to_craft( get_player_character(),
                                                  recipe_time_flag::ignore_proficiencies ) ) );
            hotkey = std::nullopt;
        }
    }
}

// Butchery sub-menu and time calculation
static void butcher_submenu( const std::vector<map_stack::iterator> &corpses, int index = -1 )
{
    avatar &player_character = get_avatar();
    auto cut_time = [&]( butcher_type bt ) {
        int time_to_cut = 0;
        if( index != -1 ) {
            const mtype &corpse = *corpses[index]->get_mtype();
            const float factor = corpse.harvest->get_butchery_requirements().get_fastest_requirements(
                                     player_character.crafting_inventory(),
                                     corpse.size, bt ).first;
            time_to_cut = butcher_time_to_cut( player_character, *corpses[index], bt ) * factor;
        } else {
            for( const map_stack::iterator &it : corpses ) {
                const mtype &corpse = *it->get_mtype();
                const float factor = corpse.harvest->get_butchery_requirements().get_fastest_requirements(
                                         player_character.crafting_inventory(),
                                         corpse.size, bt ).first;
                time_to_cut += butcher_time_to_cut( player_character, *it, bt ) * factor;
            }
        }
        return to_string_clipped( time_duration::from_moves( time_to_cut ) );
    };
    const bool enough_light = player_character.fine_detail_vision_mod() <= 4;

    const int factor = player_character.max_quality( qual_BUTCHER, PICKUP_RANGE );
    const std::string msgFactor = factor > INT_MIN
                                  ? string_format( _( "Your best tool has <color_cyan>%d butchering</color>." ), factor )
                                  :  _( "You have no butchering tool." );

    const int factorD = player_character.max_quality( qual_CUT_FINE, PICKUP_RANGE );
    const std::string msgFactorD = factorD > INT_MIN
                                   ? string_format( _( "Your best tool has <color_cyan>%d fine cutting</color>." ), factorD )
                                   :  _( "You have no fine cutting tool." );

    bool has_blood = false;
    bool has_skin = false;
    bool has_organs = false;
    std::string dissect_wp_hint; // dissection weakpoint proficiencies training hint
    int dissect_wp_hint_lines = 0; // track hint lines so menu width doesn't change

    if( index != -1 ) {
        const mtype *dead_mon = corpses[index]->get_mtype();
        if( dead_mon ) {
            for( const harvest_entry &entry : dead_mon->harvest.obj() ) {
                if( entry.type == harvest_drop_skin && !corpses[index]->has_flag( flag_SKINNED ) ) {
                    has_skin = true;
                }
                if( entry.type == harvest_drop_offal && !( corpses[index]->has_flag( flag_QUARTERED ) ||
                        corpses[index]->has_flag( flag_FIELD_DRESS ) ||
                        corpses[index]->has_flag( flag_FIELD_DRESS_FAILED ) ) ) {
                    has_organs = true;
                }
                if( entry.type == harvest_drop_blood && dead_mon->bleed_rate > 0 &&
                    !( corpses[index]->has_flag( flag_QUARTERED ) ||
                       corpses[index]->has_flag( flag_FIELD_DRESS ) ||
                       corpses[index]->has_flag( flag_FIELD_DRESS_FAILED ) || corpses[index]->has_flag( flag_BLED ) ) ) {
                    has_blood = true;
                }
            }
            if( !dead_mon->families.families.empty() ) {
                dissect_wp_hint += std::string( "\n\n" ) + _( "Dissecting may yield knowledge of:" );
                dissect_wp_hint_lines += 2;
                for( const weakpoint_family &wf : dead_mon->families.families ) {
                    std::string prof_status;
                    if( !player_character.has_prof_prereqs( wf.proficiency ) ) {
                        prof_status += colorize( string_format( " (%s)", _( "missing proficiencies" ) ), c_red );
                    } else if( player_character.has_proficiency( wf.proficiency ) ) {
                        prof_status += colorize( string_format( " (%s)", _( "already known" ) ), c_dark_gray );
                    }
                    dissect_wp_hint += string_format( "\n  %s%s", wf.proficiency->name(), prof_status );
                    dissect_wp_hint_lines++;
                }
            }
        }
    }

    uilist smenu;
    smenu.desc_enabled = true;
    smenu.desc_lines_hint += dissect_wp_hint_lines;
    smenu.text = _( "Choose type of butchery:" );

    const std::string cannot_see = colorize( _( "can't see!" ), c_red );

    smenu.addentry_col( static_cast<int>( butcher_type::QUICK ), enough_light,
                        'B', _( "Quick butchery" ),
                        enough_light ? cut_time( butcher_type::QUICK ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "This technique is used when you are in a hurry, "
                                          "but still want to harvest something from the corpse. "
                                          " Yields are lower as you don't try to be precise, "
                                          "but it's useful if you don't want to set up a workshop.  "
                                          "Prevents zombies from raising." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::FULL ), enough_light,
                        'b', _( "Full butchery" ),
                        enough_light ? cut_time( butcher_type::FULL ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "This technique is used to properly butcher a corpse, "
                                          "and requires a rope & a tree or a butchering rack, "
                                          "a flat surface (for ex. a table, a leather tarp, etc.) "
                                          "and good tools.  Yields are plentiful and varied, "
                                          "but it is time consuming." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::FIELD_DRESS ), enough_light && has_organs,
                        'f', _( "Field dress corpse" ),
                        enough_light ? ( has_organs ? cut_time( butcher_type::FIELD_DRESS ) :
                                         colorize( _( "has no organs" ), c_red ) ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "Technique that involves removing internal organs and "
                                          "viscera to protect the corpse from rotting from inside.  "
                                          "Yields internal organs.  Carcass will be lighter and will "
                                          "stay fresh longer.  Can be combined with other methods for "
                                          "better effects." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::SKIN ), enough_light && has_skin,
                        's', _( "Skin corpse" ),
                        enough_light ? ( has_skin ? cut_time( butcher_type::SKIN ) : colorize( _( "has no skin" ),
                                         c_red ) ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "Skinning a corpse is an involved and careful process that "
                                          "usually takes some time.  You need skill and an appropriately "
                                          "sharp and precise knife to do a good job.  Some corpses are "
                                          "too small to yield a full-sized hide and will instead produce "
                                          "scraps that can be used in other ways." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::BLEED ), enough_light && has_blood,
                        'l', _( "Bleed corpse" ),
                        enough_light ? ( has_blood ? cut_time( butcher_type::BLEED ) : colorize( _( "has no blood" ),
                                         c_red ) ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "Bleeding involves severing the carotid arteries and jugular "
                                          "veins, or the blood vessels from which they arise.  "
                                          "You need skill and an appropriately sharp and precise knife "
                                          "to do a good job." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::QUARTER ), enough_light,
                        'k', _( "Quarter corpse" ),
                        enough_light ? cut_time( butcher_type::QUARTER ) : cannot_see,
                        string_format( "%s  %s",
                                       _( "By quartering a previously field dressed corpse you will "
                                          "acquire four parts with reduced weight and volume.  It "
                                          "may help in transporting large game.  This action destroys "
                                          "skin, hide, pelt, etc., so don't use it if you want to "
                                          "harvest them later." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::DISMEMBER ), true,
                        'm', _( "Dismember corpse" ),
                        cut_time( butcher_type::DISMEMBER ),
                        string_format( "%s  %s",
                                       _( "If you're aiming to just destroy a body outright and don't "
                                          "care about harvesting it, dismembering it will hack it apart "
                                          "in a very short amount of time but yields little to no usable flesh." ),
                                       msgFactor ) );
    smenu.addentry_col( static_cast<int>( butcher_type::DISSECT ), enough_light,
                        'd', _( "Dissect corpse" ),
                        enough_light ? cut_time( butcher_type::DISSECT ) : cannot_see,
                        string_format( "%s  %s%s",
                                       _( "By careful dissection of the corpse, you will examine it for "
                                          "possible bionic implants, or discrete organs and harvest them "
                                          "if possible.  Requires scalpel-grade cutting tools, ruins "
                                          "corpse, and consumes a lot of time.  Your medical knowledge "
                                          "is most useful here." ),
                                       msgFactorD, dissect_wp_hint ) );
    smenu.query();
    switch( smenu.ret ) {
        case static_cast<int>( butcher_type::QUICK ):
            player_character.assign_activity( ACT_BUTCHER, 0, true );
            break;
        case static_cast<int>( butcher_type::FULL ):
            player_character.assign_activity( ACT_BUTCHER_FULL, 0, true );
            break;
        case static_cast<int>( butcher_type::FIELD_DRESS ):
            player_character.assign_activity( ACT_FIELD_DRESS, 0, true );
            break;
        case static_cast<int>( butcher_type::SKIN ):
            player_character.assign_activity( ACT_SKIN, 0, true );
            break;
        case static_cast<int>( butcher_type::BLEED ) :
            player_character.assign_activity( ACT_BLEED, 0, true );
            break;
        case static_cast<int>( butcher_type::QUARTER ):
            player_character.assign_activity( ACT_QUARTER, 0, true );
            break;
        case static_cast<int>( butcher_type::DISMEMBER ):
            player_character.assign_activity( ACT_DISMEMBER, 0, true );
            break;
        case static_cast<int>( butcher_type::DISSECT ):
            player_character.assign_activity( ACT_DISSECT, 0, true );
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

    const int factor = u.max_quality( qual_BUTCHER, PICKUP_RANGE );
    const int factorD = u.max_quality( qual_CUT_FINE, PICKUP_RANGE );
    const std::string no_knife_msg = _( "You don't have a butchering tool." );
    const std::string no_corpse_msg = _( "There are no corpses here to butcher." );

    //You can't butcher on sealed terrain- you have to smash/shovel/etc it open first
    if( m.has_flag( ter_furn_flag::TFLAG_SEALED, u.pos() ) ) {
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
    auto salvage_filter = []( const item & it ) {
        const item *usable = it.get_usable_item( salvage_string );
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
            if( ( salvage_tool_index != INT_MIN ) && salvage_iuse->valid_to_cut_up( nullptr, *it ) ) {
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

    Creature *hostile_critter = is_hostile_very_close( true );
    if( hostile_critter != nullptr ) {
        if( !query_yn( _( "You see %s nearby!  Start butchering anyway?" ),
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
    // What are we butchering (i.e.. which vector to pick indices from)
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
        if( salvage_iuse && !salvageables.empty() ) {
            add_salvagables( kmenu, salvage_stacks, i, *salvage_iuse );
        }

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b', _( "Butcher everything" ) );
        }

        if( disassembly_stacks.size() > 1 || ( disassembly_stacks.size() == 1 &&
                                               disassembly_stacks.front().second > 1 ) ) {
            int time_to_disassemble_once = 0;
            int time_to_disassemble_recursive = 0;
            for( const auto &stack : disassembly_stacks ) {
                recipe uncraft_recipe;
                if( stack.first->typeId() == itype_disassembly ) {
                    uncraft_recipe = stack.first->get_making();
                } else {
                    uncraft_recipe = recipe_dictionary::get_uncraft( stack.first->typeId() );
                }

                const int time = uncraft_recipe.time_to_craft_moves(
                                     get_player_character(), recipe_time_flag::ignore_proficiencies );
                time_to_disassemble_once += time * stack.second;
                if( stack.first->typeId() == itype_disassembly ) {
                    item test( uncraft_recipe.result(), calendar::turn, 1 );
                    time_to_disassemble_recursive += test.get_recursive_disassemble_moves(
                                                         get_player_character() ) * stack.second;
                } else {
                    time_to_disassemble_recursive += stack.first->get_recursive_disassemble_moves(
                                                         get_player_character() ) * stack.second;
                }

            }

            kmenu.addentry_col( MULTIDISASSEMBLE_ONE, true, 'D', _( "Disassemble everything once" ),
                                to_string_clipped( time_duration::from_moves( time_to_disassemble_once ) ) );
            kmenu.addentry_col( MULTIDISASSEMBLE_ALL, true, 'd', _( "Disassemble everything recursively" ),
                                to_string_clipped( time_duration::from_moves( time_to_disassemble_recursive ) ) );
        }
        if( salvage_iuse && salvageables.size() > 1 ) {
            int time_to_salvage = 0;
            for( const auto &stack : salvage_stacks ) {
                time_to_salvage += salvage_iuse->time_to_cut_up( *stack.first ) * stack.second;
            }

            kmenu.addentry_col( MULTISALVAGE, true, 'z', _( "Cut up everything" ),
                                to_string_clipped( time_duration::from_moves( time_to_salvage ) ) );
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
    const std::vector<Character *> helpers = u.get_crafting_helpers();
    for( std::size_t i = 0; i < helpers.size() && i < 3; i++ ) {
        add_msg( m_info, _( "%s helps with this task…" ), helpers[i]->get_name() );
    }
    switch( butcher_select ) {
        case BUTCHER_OTHER:
            switch( indexer_index ) {
                case MULTISALVAGE:
                    u.assign_activity( longsalvage_activity_actor( salvage_tool_index ) );
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
            if( !salvage_iuse || !salvage_tool ) {
                debugmsg( "null salvage_iuse or salvage_tool" );
            } else {
                // Pick index of first item in the salvage stack
                item *const target = &*salvage_stacks[indexer_index].first;
                item_location item_loc( map_cursor( u.pos() ), target );
                salvage_iuse->try_to_cut_up( u, *salvage_tool, item_loc );
            }
        }
        break;
    }
}

void game::reload( item_location &loc, bool prompt, bool empty )
{
    // bows etc. do not need to reload. select favorite ammo for them instead
    if( loc->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        item::reload_option opt = u.select_ammo( loc, prompt );
        if( !opt ) {
            return;
        } else if( u.ammo_location && opt.ammo == u.ammo_location ) {
            u.ammo_location = item_location();
        } else {
            u.ammo_location = opt.ammo;
        }
        return;
    }

    switch( u.rate_action_reload( *loc ) ) {
        case hint_rating::iffy:
            if( ( loc->is_ammo_container() || loc->is_magazine() ) && loc->ammo_remaining() > 0 &&
                loc->remaining_ammo_capacity() == 0 ) {
                add_msg( m_info, _( "The %s is already fully loaded!" ), loc->tname() );
                return;
            }
            if( loc->is_ammo_belt() ) {
                const auto &linkage = loc->type->magazine->linkage;
                if( linkage && !u.has_charges( *linkage, 1 ) ) {
                    add_msg( m_info, _( "You need at least one %s to reload the %s!" ),
                             item::nname( *linkage, 1 ), loc->tname() );
                    return;
                }
            }
            if( loc->is_watertight_container() && loc->is_container_full() ) {
                add_msg( m_info, _( "The %s is already full!" ), loc->tname() );
                return;
            }

        // intentional fall-through

        case hint_rating::cant:
            add_msg( m_info, _( "You can't reload a %s!" ), loc->tname() );
            return;

        case hint_rating::good:
            break;
    }

    if( !u.has_item( *loc ) && !loc->has_flag( flag_ALLOWS_REMOTE_USE ) ) {
        loc = loc.obtain( u );
        if( !loc ) {
            add_msg( _( "Never mind." ) );
            return;
        }
    }

    // for holsters and ammo pouches try to reload any contained item
    if( loc->type->can_use( "holster" ) && loc->num_item_stacks() == 1 ) {
        loc = item_location( loc, &loc->only_item() );
    }

    item::reload_option opt = u.ammo_location &&
                              loc->can_reload_with( *u.ammo_location.get_item(), false ) ?
                              item::reload_option( &u, loc, u.ammo_location ) :
                              u.select_ammo( loc, prompt, empty );

    if( opt.ammo.get_item() == nullptr || ( opt.ammo.get_item()->is_frozen_liquid() &&
                                            !u.crush_frozen_liquid( opt.ammo ) ) ) {
        return;
    }

    if( opt ) {
        const int extra_moves = loc->get_var( "dirt", 0 ) > 7800 ? 2500 : 0;
        if( extra_moves > 0 ) {
            add_msg( m_warning, _( "You struggle to reload the fouled %s." ), loc->tname() );
        }
        u.assign_activity( reload_activity_actor( std::move( opt ), extra_moves ) );
    }
}

// Reload something.
void game::reload_item()
{
    item_location item_loc = inv_map_splice( [&]( const item & it ) {
        return u.rate_action_reload( it ) == hint_rating::good;
    }, _( "Reload item" ), 1, _( "You have nothing to reload." ) );

    if( !item_loc ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    reload( item_loc );
}

void game::reload_wielded( bool prompt )
{
    item_location weapon = u.get_wielded_item();
    if( !weapon || !weapon->is_reloadable() ) {
        add_msg( _( "You aren't holding something you can reload." ) );
        return;
    }
    reload( weapon, prompt );
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
        // Non gun/magazines are sorted last and later ignored.
        if( !a->is_magazine() && !a->is_gun() ) {
            return false;
        }
        // Current wielded weapon comes first.
        if( this->u.is_wielding( *b ) ) {
            return false;
        }
        if( this->u.is_wielding( *a ) ) {
            return true;
        }
        // Second sort by affiliation with wielded gun
        item_location weapon = u.get_wielded_item();
        if( weapon ) {
            const bool mag_a = weapon->is_compatible( *a ).success();
            const bool mag_b = weapon->is_compatible( *b ).success();
            if( mag_a != mag_b ) {
                return mag_a;
            }
        }
        // Third sort by gun vs magazine,
        if( a->is_gun() != b->is_gun() ) {
            return a->is_gun();
        }
        // Finally sort by speed to reload.
        return ( a->get_reload_time() * a->remaining_ammo_capacity() ) <
               ( b->get_reload_time() * b->remaining_ammo_capacity() );
    } );
    for( item_location &candidate : reloadables ) {
        if( !candidate->is_magazine() && !candidate->is_gun() ) {
            continue;
        }
        std::vector<item::reload_option> ammo_list;
        u.list_ammo( candidate, ammo_list, false );
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
    const optional_vpart_position ovp = m.veh_at( u.pos() );
    if( ovp ) {
        const turret_data turret = ovp->vehicle().turret_query( ovp->pos() );
        if( turret.can_reload() ) {
            item::reload_option opt = u.select_ammo( turret.base(), true );
            if( opt ) {
                u.assign_activity( reload_activity_actor( std::move( opt ) ) );
            }
        }
        return;
    }

    reload_item();
}

void game::wield( item_location loc )
{
    if( !loc ) {
        debugmsg( "ERROR: tried to wield null item" );
        return;
    }
    const item_location weapon = u.get_wielded_item();
    if( weapon && weapon != loc && weapon->has_item( *loc ) ) {
        add_msg( m_info, _( "You need to put the bag away before trying to wield something from it." ) );
        return;
    }
    if( u.has_wield_conflicts( *loc ) ) {
        const bool is_unwielding = u.is_wielding( *loc );
        const auto ret = u.can_unwield( *loc );

        if( !ret.success() ) {
            add_msg( m_info, "%s", ret.c_str() );
        }

        if( !u.unwield() ) {
            return;
        }

        if( is_unwielding ) {
            if( !u.martial_arts_data->selected_is_none() ) {
                u.martial_arts_data->martialart_use_message( u );
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
    if( target.get_use( "holster" ) && !target.empty() ) {
        //~ %1$s: holster name
        if( query_yn( pgettext( "holster", "Draw from %1$s?" ),
                      target.tname() ) ) {
            u.invoke_item( &target );
            return;
        }
    }

    // Can't use loc.obtain() here because that would cause things to spill.
    item to_wield = *loc.get_item();
    item_location::type location_type = loc.where();
    tripoint pos = loc.position();
    const int obtain_cost = loc.obtain_cost( u );
    int worn_index = INT_MIN;

    // Need to account for case where we're trying to wield a weapon that belongs to someone else
    if( !avatar_action::check_stealing( u, *loc.get_item() ) ) {
        return;
    }

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
    loc.remove_item();
    if( !u.wield( to_wield, obtain_cost ) ) {
        switch( location_type ) {
            case item_location::type::container:
                // this will not cause things to spill, as it is inside another item
                loc = loc.obtain( u );
                wield( loc );
                break;
            case item_location::type::character:
                if( worn_index != INT_MIN ) {
                    u.worn.insert_item_at_index( to_wield, worn_index );
                } else {
                    u.i_add( to_wield, true, nullptr, loc.get_item() );
                }
                break;
            case item_location::type::map:
                m.add_item( pos, to_wield );
                break;
            case item_location::type::vehicle: {
                const std::optional<vpart_reference> ovp = m.veh_at( pos ).cargo();
                // If we fail to return the item to the vehicle for some reason, add it to the map instead.
                if( !ovp || !ovp->vehicle().add_item( ovp->part(), to_wield ) ) {
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
        if( u.get_int_base() < 5 || u.has_trait( trait_PROF_CHURL ) ) {
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
    std::string spotted_creature_text;
    const monster_visible_info &mon_visible = u.get_mon_visible();
    const std::vector<shared_ptr_fast<monster>> &new_seen_mon = mon_visible.new_seen_mon;

    const nc_color mon_color = c_red;
    const nc_color dir_color = c_light_blue;
    if( new_seen_mon.empty() ) {
        // naming consistent with code in game::mon_info
        spotted_creature_text = colorize( _( "a survivor" ), mon_color );
        get_safemode().lastmon_whitelist = get_safemode().npc_type_name();
    } else if( new_seen_mon.size() == 1 ) {
        const shared_ptr_fast<monster> &mon = new_seen_mon.back();
        const std::string dist_text = string_format( _( "%d tiles" ), rl_dist( u.pos(), mon->pos() ) );
        //~ %s: Cardinal/ordinal direction ("east")
        const std::string dir_text = string_format( _( "to the %s" ),
                                     colorize( direction_name( direction_from( u.pos(), mon->pos() ) ), dir_color ) );
        //~ %1$s: Name of monster spotted ("headless zombie")
        //~ %2$s: Distance to monster ("17 tiles")
        //~ %3$s: Description of where the monster is ("to the east")
        spotted_creature_text = string_format( pgettext( "monster description", "%1$s %2$s %3$s" ),
                                               colorize( mon->name(), mon_color ),
                                               dist_text,
                                               dir_text );
        get_safemode().lastmon_whitelist = mon->name();
    } else {
        // We've got multiple monsters to inform about.
        // Find the range of distances.
        int min_dist = INT_MAX;
        int max_dist = 0;
        // Find the most frequent type to call out by name.
        std::unordered_map<std::string, std::vector<const monster *>> mons_by_name;
        for( const shared_ptr_fast<monster> &mon : new_seen_mon ) {
            min_dist = std::min( min_dist, rl_dist( u.pos(), mon->pos() ) );
            max_dist = std::max( min_dist, rl_dist( u.pos(), mon->pos() ) );
            mons_by_name[mon->name()].push_back( mon.get() );
        }
        const std::vector<const monster *> &most_frequent_mon = std::max_element( mons_by_name.begin(),
        mons_by_name.end(), []( const auto & a, const auto & b ) {
            return a.second.size() < b.second.size();
        } )->second;

        const monster *const mon = most_frequent_mon.back();
        const std::string most_frequent_mon_text = colorize( most_frequent_mon.size() > 1 ?
                string_format( "%d %s",
                               most_frequent_mon.size(), mon->name( most_frequent_mon.size() ) ) : mon->name(), mon_color );

        const std::string dist_text = min_dist == max_dist ?
                                      //~ %d: Distance to all monsters ("7")
                                      string_format( _( "%d tiles" ), max_dist ) :
                                      //~ %d, %d: Minimum and Maximum distance to the monsters ("5"; "20")
                                      string_format( _( "%d-%d tiles" ), min_dist, max_dist );

        // If they're all in one or two directions, let's call that out.
        // Otherwise, we can say they're in "various directions", so it's clear they aren't clustered.
        std::set<direction> most_frequent_mon_dirs;
        std::transform( most_frequent_mon.begin(), most_frequent_mon.end(),
                        std::inserter( most_frequent_mon_dirs,
        most_frequent_mon_dirs.begin() ), [&]( const monster * const mon ) {
            return direction_from( u.pos(), mon->pos() );
        } );
        std::string dir_text;
        if( most_frequent_mon_dirs.size() == 1 ) {
            //~ %s: Cardinal/ordinal direction ("east")
            dir_text = string_format( _( "to the %s" ),
                                      colorize( direction_name( *most_frequent_mon_dirs.begin() ), dir_color ) );
        } else if( most_frequent_mon_dirs.size() == 2 ) {
            //~ %s, %s: Cardinal/ordinal directions ("east"; "southwest")
            dir_text = string_format( _( "to the %s and %s" ),
                                      colorize( direction_name( *most_frequent_mon_dirs.begin() ), dir_color ),
                                      colorize( direction_name( *std::next( most_frequent_mon_dirs.begin() ) ), dir_color ) );
        } else {
            dir_text = colorize( _( "in various directions" ), dir_color );
        }

        // Finally, let's mention that there are other monsters around that we didn't name explicitly.
        const size_t other_mon_count = new_seen_mon.size() - most_frequent_mon.size();
        std::string other_mon_text;
        if( other_mon_count > 0 ) {
            //~ %s: Description of other monster count ("4 others"), %d: How many other monsters there are
            other_mon_text = string_format( _( " and %s" ),
                                            colorize( string_format( n_gettext( _( "%d other" ),  _( "%d others" ),
                                                    other_mon_count ), other_mon_count ), mon_color ) );
        }

        //~ %1$s: Description of primary monster spotted ("3 fat zombies")
        //~ %2$s: Description of how far away the monsters are ("7 tiles" or "5-20 tiles")
        //~ %3$s: Description of where the primary monster is ("to the east and south")
        //~ %4$s: Description of any other monsters spotted (" and 4 others")
        spotted_creature_text = string_format( _( "%1$s %2$s %3$s%4$s" ),  most_frequent_mon_text,
                                               dist_text,
                                               dir_text,
                                               other_mon_text );
        get_safemode().lastmon_whitelist = mon->name();
    }

    std::string whitelist;
    if( !get_safemode().empty() ) {
        whitelist = string_format( _( ", or %s to whitelist the monster" ),
                                   press_x( ACTION_WHITELIST_ENEMY ) );
    }

    const std::string msg_safe_mode = press_x( ACTION_TOGGLE_SAFEMODE );
    add_msg( game_message_params{ m_warning, gmf_bypass_cooldown },
             _( "Spotted %1$s -- safe mode is on!  (%2$s to turn it off, %3$s to ignore monster%4$s)" ),
             spotted_creature_text, msg_safe_mode, msg_ignore, whitelist );
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
    monster *const mon_ptr = get_creature_tracker().creature_at<monster>( p );
    if( !mon_ptr ) {
        return false;
    }
    monster &critter = *mon_ptr;
    if( !disable_activity_actor::can_disable_or_reprogram( critter ) ) {
        return false;
    }

    const mtype_id mid = critter.type->id;
    const itype_id mon_item_id = critter.type->revert_to_itype;
    if( !mon_item_id.is_empty() &&
        query_yn( _( "Deactivate the %s?" ), critter.name() ) ) {
        const disable_activity_actor actor( p, disable_activity_actor::get_disable_turns(), false );
        u.assign_activity( actor );
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

        if( choice == 0 ) {
            u.assign_activity( disable_activity_actor( p, disable_activity_actor::get_disable_turns(), true ) );
        }
    }
    return false;
}

bool game::is_dangerous_tile( const tripoint &dest_loc ) const
{
    return !get_dangerous_tile( dest_loc ).empty();
}

bool game::prompt_dangerous_tile( const tripoint &dest_loc ) const
{
    if( u.has_effect( effect_stunned ) || u.has_effect( effect_psi_stunned ) ) {
        return true;
    }

    std::vector<std::string> harmful_stuff = get_dangerous_tile( dest_loc );

    if( !harmful_stuff.empty() &&
        !query_yn( _( "Really step into %s?" ), enumerate_as_string( harmful_stuff ) ) ) {
        return false;
    }
    if( !harmful_stuff.empty() && u.is_mounted() && m.tr_at( dest_loc ) == tr_ledge ) {
        add_msg( m_warning, _( "Your %s refuses to move over that ledge!" ),
                 u.mounted_creature->get_name() );
        return false;
    }
    return true;
}

std::vector<std::string> game::get_dangerous_tile( const tripoint &dest_loc ) const
{
    if( u.is_blind() ) {
        return {}; // blinded players don't see dangerous tiles
    }

    std::vector<std::string> harmful_stuff;
    const field fields_here = m.field_at( u.pos() );
    const auto veh_here = m.veh_at( u.pos() ).part_with_feature( "BOARDABLE", true );
    const auto veh_dest = m.veh_at( dest_loc ).part_with_feature( "BOARDABLE", true );
    const bool veh_here_inside = veh_here && veh_here->is_inside();
    const bool veh_dest_inside = veh_dest && veh_dest->is_inside();

    for( const std::pair<const field_type_id, field_entry> &e : m.field_at( dest_loc ) ) {
        if( !u.is_dangerous_field( e.second ) ) {
            continue;
        }

        const bool has_field_here = fields_here.find_field( e.first ) != nullptr;
        const bool empty_effects = e.second.field_effects().empty();

        // if the field is dangerous but has no effects apparently this
        // means effects are hardcoded in map_field.cpp so we should...
        bool danger_dest = empty_effects; // ... warn if effects are empty
        bool danger_here = has_field_here && empty_effects;
        for( const field_effect &fe : e.second.field_effects() ) {
            if( !danger_dest ) {
                danger_dest = true;
                if( fe.immune_in_vehicle && veh_dest ) { // NOLINT(bugprone-branch-clone)
                    danger_dest = false;
                } else if( fe.immune_inside_vehicle && veh_dest_inside ) {
                    danger_dest = false;
                } else if( fe.immune_outside_vehicle && !veh_dest_inside ) {
                    danger_dest = false;
                } else if( u.is_immune_effect( fe.id ) || u.check_immunity_data( fe.immunity_data ) ) {
                    danger_dest = false;
                }
            }
            if( has_field_here && !danger_here ) {
                danger_here = true;
                if( fe.immune_in_vehicle && veh_here ) { // NOLINT(bugprone-branch-clone)
                    danger_here = false;
                } else if( fe.immune_inside_vehicle && veh_here_inside ) {
                    danger_here = false;
                } else if( fe.immune_outside_vehicle && !veh_here_inside ) {
                    danger_here = false;
                } else if( u.is_immune_effect( fe.id ) || u.check_immunity_data( fe.immunity_data ) ) {
                    danger_here = false;
                }
            }
        }

        // don't warn if already in a field of the same type
        if( !danger_dest || danger_here ) {
            continue;
        }

        harmful_stuff.push_back( e.second.name() );
    }

    const trap &tr = m.tr_at( dest_loc );
    // HACK: Hack for now, later ledge should stop being a trap
    if( tr == tr_ledge ) {
        if( !veh_dest && !u.has_effect_with_flag( json_flag_LEVITATION ) ) {
            harmful_stuff.emplace_back( tr.name() );
        }
    } else if( tr.can_see( dest_loc, u ) && !tr.is_benign() && !veh_dest ) {
        harmful_stuff.emplace_back( tr.name() );
    }

    static const std::set< bodypart_str_id > sharp_bps = {
        body_part_eyes, body_part_mouth, body_part_head,
        body_part_leg_l, body_part_leg_r, body_part_foot_l,
        body_part_foot_r, body_part_arm_l, body_part_arm_r,
        body_part_hand_l, body_part_hand_r, body_part_torso
    };

    const auto sharp_bp_check = [this]( bodypart_id bp ) {
        return u.immune_to( bp, { damage_cut, 10 } );
    };

    if( m.has_flag( ter_furn_flag::TFLAG_ROUGH, dest_loc ) &&
        !m.has_flag( ter_furn_flag::TFLAG_ROUGH, u.pos() ) &&
        !veh_dest &&
        ( u.get_armor_type( damage_bash, bodypart_id( "foot_l" ) ) < 5 ||
          u.get_armor_type( damage_bash, bodypart_id( "foot_r" ) ) < 5 ) ) { // NOLINT(bugprone-branch-clone)
        harmful_stuff.emplace_back( m.name( dest_loc ) );
    } else if( m.has_flag( ter_furn_flag::TFLAG_SHARP, dest_loc ) &&
               !m.has_flag( ter_furn_flag::TFLAG_SHARP, u.pos() ) &&
               !( u.in_vehicle || m.veh_at( dest_loc ) ) &&
               u.dex_cur < 78 &&
               !( u.is_mounted() &&
                  u.mounted_creature->get_armor_type( damage_cut, bodypart_id( "torso" ) ) >= 10 ) &&
               !std::all_of( sharp_bps.begin(), sharp_bps.end(), sharp_bp_check ) ) {
        harmful_stuff.emplace_back( m.name( dest_loc ) );
    }

    return harmful_stuff;
}

bool game::walk_move( const tripoint &dest_loc, const bool via_ramp, const bool furniture_move )
{
    if( m.has_flag_ter( ter_furn_flag::TFLAG_SMALL_PASSAGE, dest_loc ) ) {
        if( u.get_size() > creature_size::medium ) {
            add_msg( m_warning, _( "You can't fit there." ) );
            return false; // character too large to fit through a tight passage
        }
        if( u.is_mounted() ) {
            monster *mount = u.mounted_creature.get();
            if( mount->get_size() > creature_size::medium ) {
                add_msg( m_warning, _( "Your mount can't fit there." ) );
                return false; // char's mount is too large for tight passages
            }
        }
    }

    const int ramp_adjust = via_ramp ? u.posz() : dest_loc.z;
    const float dest_light_level = get_map().ambient_light_at( tripoint( dest_loc.xy(), ramp_adjust ) );

    // Allow players with nyctophobia to move freely through cloudy and dark tiles
    const float nyctophobia_threshold = LIGHT_AMBIENT_LIT - 3.0f;

    // Forbid players from moving through very dark tiles, unless they are running or took xanax
    if( u.has_flag( json_flag_NYCTOPHOBIA ) && !u.has_effect( effect_took_xanax ) && !u.is_running() &&
        dest_light_level < nyctophobia_threshold ) {
        add_msg( m_bad,
                 _( "It's so dark and scary in there!  You can't force yourself to walk into this tile.  Switch to running movement mode to move there." ) );
        return false;
    }

    if( u.is_mounted() ) {
        monster *mons = u.mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return false;
            }
        }
        if( !mons->move_effects( false ) ) {
            add_msg( m_bad, _( "You cannot move as your %s isn't able to move." ), mons->get_name() );
            return false;
        }
    }
    const optional_vpart_position vp_here = m.veh_at( u.pos() );
    const optional_vpart_position vp_there = m.veh_at( dest_loc );

    bool pushing = false; // moving -into- grabbed tile; skip check for move_cost > 0
    bool pulling = false; // moving -away- from grabbed tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0

    const tripoint furn_pos = u.pos() + u.grab_point;
    const tripoint furn_dest = dest_loc + tripoint( u.grab_point.xy(), 0 );

    bool grabbed = u.get_grab_type() != object_type::NONE;
    if( grabbed ) {
        const tripoint dp = dest_loc - u.pos();
        pushing = dp.xy() ==  u.grab_point.xy();
        pulling = dp.xy() == -u.grab_point.xy();
    }

    // Now make sure we're actually holding something
    const vehicle *grabbed_vehicle = nullptr;
    const optional_vpart_position vp_grabbed = m.veh_at( u.pos() + u.grab_point );
    if( grabbed && u.get_grab_type() == object_type::FURNITURE ) {
        // We only care about shifting, because it's the only one that can change our destination
        if( m.has_furn( u.pos() + u.grab_point ) ) {
            shifting_furniture = !pushing && !pulling;
        } else {
            // We were grabbing a furniture that isn't there
            grabbed = false;
        }
    } else if( grabbed && u.get_grab_type() == object_type::VEHICLE ) {
        grabbed_vehicle = veh_pointer_or_null( m.veh_at( u.pos() + u.grab_point ) );
        if( grabbed_vehicle == nullptr ) {
            // We were grabbing a vehicle that isn't there anymore
            grabbed = false;
        }
    } else if( grabbed ) {
        // We were grabbing something WEIRD, let's pretend we weren't
        grabbed = false;
    }
    if( u.grab_point != tripoint_zero && !grabbed && !furniture_move ) {
        add_msg( m_warning, _( "Can't find grabbed object." ) );
        u.grab( object_type::NONE );
    }

    if( m.impassable( dest_loc ) && !pushing && !shifting_furniture ) {
        if( vp_there && u.mounted_creature && u.mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) &&
            vp_there->vehicle().handle_potential_theft( u ) ) {
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
    if( vp_there && !vp_there->vehicle().handle_potential_theft( u ) ) {
        return false;
    }
    if( u.is_mounted() && !pushing && vp_there ) {
        add_msg( m_warning, _( "You cannot board a vehicle whilst riding." ) );
        return false;
    }
    u.set_underwater( false );

    if( vp_there && !u.move_in_vehicle( static_cast<Creature *>( &u ), dest_loc ) ) {
        return false;
    }

    if( !shifting_furniture && !pushing && is_dangerous_tile( dest_loc ) ) {
        std::vector<std::string> harmful_stuff = get_dangerous_tile( dest_loc );
        if( harmful_stuff.size() == 1 && harmful_stuff[0] == "ledge" ) {
            iexamine::ledge( u, dest_loc );
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "ALWAYS" &&
                   !prompt_dangerous_tile( dest_loc ) ) {
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "RUNNING" &&
                   ( !u.is_running() || !prompt_dangerous_tile( dest_loc ) ) ) {
            add_msg( m_warning,
                     _( "Stepping into that %1$s looks risky.  Run into it if you wish to enter anyway." ),
                     enumerate_as_string( harmful_stuff ) );
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "CROUCHING" &&
                   ( !u.is_crouching() || !prompt_dangerous_tile( dest_loc ) ) ) {
            add_msg( m_warning,
                     _( "Stepping into that %1$s looks risky.  Crouch and move into it if you wish to enter anyway." ),
                     enumerate_as_string( harmful_stuff ) );
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "NEVER" &&
                   !u.is_running() ) {
            add_msg( m_warning,
                     _( "Stepping into that %1$s looks risky.  Run into it if you wish to enter anyway." ),
                     enumerate_as_string( harmful_stuff ) );
            return true;
        }
    }
    // Used to decide whether to print a 'moving is slow message
    const int mcost_from = m.move_cost( u.pos() ); //calculate this _before_ calling grabbed_move

    int modifier = 0;
    if( grabbed && u.get_grab_type() == object_type::FURNITURE && u.pos() + u.grab_point == dest_loc ) {
        modifier = -m.furn( dest_loc ).obj().movecost;
    }

    const int mcost = m.combined_movecost( u.pos(), dest_loc, grabbed_vehicle, modifier,
                                           via_ramp );

    if( !furniture_move && grabbed_move( dest_loc - u.pos(), via_ramp ) ) {
        return true;
    } else if( mcost == 0 ) {
        return false;
    }
    bool diag = trigdist && u.posx() != dest_loc.x && u.posy() != dest_loc.y;
    const int previous_moves = u.moves;
    if( u.is_mounted() ) {
        auto *crit = u.mounted_creature.get();
        if( !crit->has_flag( mon_flag_RIDEABLE_MECH ) &&
            ( m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_MOUNTABLE, dest_loc ) ||
              m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR, dest_loc ) ||
              m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, dest_loc ) ||
              m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR_DAMAGED, dest_loc ) ||
              m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR_REINFORCED, dest_loc ) ) ) {
            add_msg( m_warning, _( "You cannot pass obstacles whilst mounted." ) );
            return false;
        }
        const double base_moves = u.run_cost( mcost, diag ) * 100.0 / crit->get_speed();
        const double encumb_moves = u.get_weight() / 4800.0_gram;
        u.moves -= static_cast<int>( std::ceil( base_moves + encumb_moves ) );
        crit->use_mech_power( u.current_movement_mode()->mech_power_use() );
    } else {
        u.moves -= u.run_cost( mcost, diag );
        /**
        TODO:
        This should really use the mounted creatures stamina, if mounted.
        Monsters don't currently have stamina however.
        For the time being just don't burn players stamina when mounted.
        */
        if( grabbed_vehicle == nullptr || grabbed_vehicle->wheelcache.empty() ) {
            //Burn normal amount of stamina if no vehicle grabbed or vehicle lacks wheels
            u.burn_move_stamina( previous_moves - u.moves );
        } else {
            //Burn half as much stamina if vehicle has wheels, without changing move time
            u.burn_move_stamina( 0.50 * ( previous_moves - u.moves ) );
        }
    }
    // Max out recoil & reset aim point
    u.recoil = MAX_RECOIL;
    u.last_target_pos = std::nullopt;

    // Print a message if movement is slow
    const int mcost_to = m.move_cost( dest_loc ); //calculate this _after_ calling grabbed_move
    const bool fungus = m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, u.pos() ) ||
                        m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS,
                                dest_loc ); //fungal furniture has no slowing effect on Mycus characters
    const bool slowed = ( ( !u.has_proficiency( proficiency_prof_parkour ) && ( mcost_to > 2 ||
                            mcost_from > 2 ) ) ||
                          mcost_to > 4 || mcost_from > 4 ) ||
                        ( !u.has_trait( trait_M_IMMUNE ) && fungus );
    if( slowed && !u.is_mounted() ) {
        // Unless u.pos() has a higher movecost than dest_loc, state that dest_loc is the cause
        if( mcost_to >= mcost_from ) {
            if( auto displayed_part = vp_there.part_displayed() ) {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ),
                         displayed_part->part().name() );
                sfx::do_obstacle( displayed_part->part().info().id.str() );
            } else {
                add_msg( m_warning, _( "Moving onto this %s is slow!" ), m.name( dest_loc ) );
                if( m.has_furn( dest_loc ) ) {
                    sfx::do_obstacle( m.furn( dest_loc ).id().str() );
                } else {
                    sfx::do_obstacle( m.ter( dest_loc ).id().str() );
                }
            }
        } else {
            if( auto displayed_part = vp_here.part_displayed() ) {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ),
                         displayed_part->part().name() );
                sfx::do_obstacle( displayed_part->part().info().id.str() );
            } else {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ), m.name( u.pos() ) );
                if( m.has_furn( u.pos() ) ) {
                    sfx::do_obstacle( m.furn( u.pos() ).id().str() );
                } else {
                    sfx::do_obstacle( m.ter( u.pos() ).id().str() );
                }
            }
        }
    }
    if( !u.is_mounted() && u.has_trait( trait_LEG_TENT_BRACE ) &&
        u.is_barefoot() ) {
        // DX and IN are long suits for Cephalopods,
        // so this shouldn't cause too much hardship
        // Presumed that if it's swimmable, they're
        // swimming and won't stick
        ///\EFFECT_DEX decreases chance of tentacles getting stuck to the ground

        ///\EFFECT_INT decreases chance of tentacles getting stuck to the ground
        if( !m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, dest_loc ) &&
            one_in( 80 + u.dex_cur + u.int_cur ) ) {
            add_msg( _( "Your tentacles stick to the ground, but you pull them free." ) );
            u.mod_fatigue( 1 );
        }
    }

    u.make_footstep_noise();

    //only clatter items every so often based on activity level
    if( to_turns<int>( calendar::turn - u.last_pocket_noise ) > std::max( static_cast<int>
            ( 10 - u.activity_level() ), 1 ) )  {
        u.make_clatter_sound();
        u.last_pocket_noise = calendar::turn;
    }

    if( m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_HIDE_PLACE, dest_loc ) ) {
        add_msg( m_good, _( "You are hiding in the %s." ), m.name( dest_loc ) );
    }

    tripoint oldpos = u.pos();
    tripoint old_abs_pos = m.getabs( oldpos );

    bool moving = dest_loc != oldpos;

    point submap_shift = place_player( dest_loc );
    point ms_shift = sm_to_ms_copy( submap_shift );
    oldpos = oldpos - ms_shift;

    if( moving ) {
        cata_event_dispatch::avatar_moves( old_abs_pos, u, m );

        // Add trail animation when sprinting
        if( get_option<bool>( "ANIMATIONS" ) && u.is_running() ) {
            if( u.posy() < oldpos.y ) {
                if( u.posx() < oldpos.x ) {
                    draw_async_anim( oldpos, "run_nw", "\\", c_light_gray );
                } else if( u.posx() == oldpos.x ) {
                    draw_async_anim( oldpos, "run_n", "|", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_ne", "/", c_light_gray );
                }
            } else if( u.posy() == oldpos.y ) {
                if( u.posx() < oldpos.x ) {
                    draw_async_anim( oldpos, "run_w", "-", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_e", "-", c_light_gray );
                }
            } else {
                if( u.posx() < oldpos.x ) {
                    draw_async_anim( oldpos, "run_sw", "/", c_light_gray );
                } else if( u.posx() == oldpos.x ) {
                    draw_async_anim( oldpos, "run_s", "|", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_se", "\\", c_light_gray );
                }
            }
        }
    }

    if( furniture_move ) {
        // Adjust the grab_point if player has changed z level.
        u.grab_point.z -= u.posz() - oldpos.z;
    }

    if( grabbed_vehicle ) {
        // Vehicle might be at different z level than the grabbed part.
        u.grab_point.z = vp_grabbed->pos().z - u.posz();
    }

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
        start_hauling( oldpos );
    }

    on_move_effects();

    return true;
}

point game::place_player( const tripoint &dest_loc, bool quick )
{
    const optional_vpart_position vp1 = m.veh_at( dest_loc );
    if( const std::optional<std::string> label = vp1.get_label() ) {
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
    if( m.has_flag( ter_furn_flag::TFLAG_ROUGH, dest_loc ) && ( !u.in_vehicle ) &&
        ( !u.is_mounted() ) ) {
        if( one_in( 5 ) && u.get_armor_type( damage_bash, bodypart_id( "foot_l" ) ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your left foot on the %s!" ),
                     m.has_flag_ter( ter_furn_flag::TFLAG_ROUGH, dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bodypart_id( "foot_l" ), damage_instance( damage_cut, 1 ) );
        }
        if( one_in( 5 ) && u.get_armor_type( damage_bash, bodypart_id( "foot_r" ) ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your right foot on the %s!" ),
                     m.has_flag_ter( ter_furn_flag::TFLAG_ROUGH, dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bodypart_id( "foot_l" ), damage_instance( damage_cut, 1 ) );
        }
    }
    ///\EFFECT_DEX increases chance of avoiding cuts on sharp terrain
    if( m.has_flag( ter_furn_flag::TFLAG_SHARP, dest_loc ) && !one_in( 3 ) &&
        !x_in_y( 1 + u.dex_cur / 2.0, 40 ) &&
        ( !u.in_vehicle && !m.veh_at( dest_loc ) ) && ( !u.has_proficiency( proficiency_prof_parkour ) ||
                one_in( 4 ) ) && ( u.has_trait( trait_THICKSKIN ) ? !one_in( 8 ) : true ) ) {
        const int sharp_damage = rng( 1, 10 );
        if( u.is_mounted() ) {
            if( u.mounted_creature->get_armor_type( damage_cut, bodypart_id( "torso" ) ) < sharp_damage &&
                u.mounted_creature->get_hp() > sharp_damage ) {
                add_msg( _( "Your %s gets cut!" ), u.mounted_creature->get_name() );
                u.mounted_creature->apply_damage( nullptr, bodypart_id( "torso" ), sharp_damage );
            }
        } else {
            if( u.get_hp() > sharp_damage ) {
                const bodypart_id bp = u.get_random_body_part();
                if( u.deal_damage( nullptr, bp,
                                   damage_instance( damage_cut, sharp_damage ) ).total_damage() > 0 ) {
                    //~ 1$s - bodypart name in accusative, 2$s is terrain name.
                    add_msg( m_bad, _( "You cut your %1$s on the %2$s!" ),
                             body_part_name_accusative( bp ),
                             m.has_flag_ter( ter_furn_flag::TFLAG_SHARP, dest_loc ) ? m.tername( dest_loc ) : m.furnname(
                                 dest_loc ) );
                    if( !u.has_flag( json_flag_INFECTION_IMMUNE ) ) {
                        const int chance_in = u.has_trait( trait_INFRESIST ) ? 1024 : 256;
                        if( one_in( chance_in ) ) {
                            u.add_effect( effect_tetanus, 1_turns, true );
                        }
                    }
                }
            }
        }
    }
    if( m.has_flag( ter_furn_flag::TFLAG_UNSTABLE, dest_loc ) &&
        !u.is_mounted() && !m.has_vehicle_floor( dest_loc ) ) {
        u.add_effect( effect_bouldering, 1_turns, true );
    } else if( u.has_effect( effect_bouldering ) ) {
        u.remove_effect( effect_bouldering );
    }
    if( m.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_SIGHT, dest_loc ) ) {
        u.add_effect( effect_no_sight, 1_turns, true );
    } else if( u.has_effect( effect_no_sight ) ) {
        u.remove_effect( effect_no_sight );
    }

    // If we moved out of the nonant, we need update our map data
    if( m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, dest_loc ) && u.has_effect( effect_onfire ) ) {
        add_msg( _( "The water puts out the flames!" ) );
        u.remove_effect( effect_onfire );
        if( u.is_mounted() ) {
            monster *mon = u.mounted_creature.get();
            if( mon->has_effect( effect_onfire ) ) {
                mon->remove_effect( effect_onfire );
            }
        }
    }

    if( monster *const mon_ptr = get_creature_tracker().creature_at<monster>( dest_loc ) ) {
        // We displaced a monster. It's probably a bug if it wasn't a friendly mon...
        // Immobile monsters can't be displaced.
        monster &critter = *mon_ptr;
        // TODO: handling for ridden creatures other than players mount.
        if( !critter.has_effect( effect_ridden ) ) {
            if( u.is_mounted() ) {
                std::vector<tripoint> maybe_valid;
                for( const tripoint &jk : m.points_in_radius( critter.pos(), 1 ) ) {
                    if( is_empty( jk ) ) {
                        maybe_valid.push_back( jk );
                    }
                }
                bool moved = false;
                while( !maybe_valid.empty() ) {
                    if( critter.move_to( random_entry_removed( maybe_valid ) ) ) {
                        add_msg( _( "You push the %s out of the way." ), critter.name() );
                        moved = true;
                    }
                }
                if( !moved ) {
                    add_msg( _( "There is no room to push the %s out of the way." ), critter.name() );
                    return u.pos().xy();
                }
            } else {
                // Force the movement even though the player is there right now.
                const bool moved = critter.move_to( u.pos(), /*force=*/false, /*step_on_critter=*/true );
                if( moved ) {
                    add_msg( _( "You displace the %s." ), critter.name() );
                } else {
                    add_msg( _( "You cannot move the %s out of the way." ), critter.name() );
                    return u.pos().xy();
                }
            }
        } else if( !u.has_effect( effect_riding ) ) {
            add_msg( _( "You cannot move the %s out of the way." ), critter.name() );
            return u.pos().xy();
        }
    }

    // If the player is in a vehicle, unboard them from the current part
    bool was_in_control_same_pos = false;
    if( u.in_vehicle ) {
        if( u.pos() == dest_loc ) {
            was_in_control_same_pos = true;
        } else {
            m.unboard_vehicle( u.pos() );
        }
    }
    // Move the player
    // Start with z-level, to make it less likely that old functions (2D ones) freak out
    bool z_level_changed = false;
    if( dest_loc.z != m.get_abs_sub().z() ) {
        z_level_changed = vertical_shift( dest_loc.z );
    }

    if( u.is_hauling() && ( !m.can_put_items( dest_loc ) ||
                            m.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, dest_loc ) ||
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
    point submap_shift = update_map( u, z_level_changed );
    // Important: don't use dest_loc after this line. `update_map` may have shifted the map
    // and dest_loc was not adjusted and therefore is still in the un-shifted system and probably wrong.
    // If you must use it you can calculate the position in the new, shifted system with
    // adjusted_pos = ( old_pos.x - submap_shift.x * SEEX, old_pos.y - submap_shift.y * SEEY, old_pos.z )

    //Auto pulp or butcher and Auto foraging
    if( !quick && get_option<bool>( "AUTO_FEATURES" ) && mostseen == 0  && !u.is_mounted() ) {
        static constexpr std::array<direction, 8> adjacentDir = {
            direction::NORTH, direction::NORTHEAST, direction::EAST, direction::SOUTHEAST,
            direction::SOUTH, direction::SOUTHWEST, direction::WEST, direction::NORTHWEST
        };

        const std::string forage_type = get_option<std::string>( "AUTO_FORAGING" );
        if( forage_type != "off" ) {
            const auto forage = [&]( const tripoint & pos ) {
                const ter_t &xter_t = *m.ter( pos );
                const furn_t &xfurn_t = *m.furn( pos );
                const bool forage_everything = forage_type == "all";
                const bool forage_bushes = forage_everything || forage_type == "bushes";
                const bool forage_trees = forage_everything || forage_type == "trees";
                const bool forage_crops = forage_everything || forage_type == "crops";
                if( !xter_t.can_examine( pos ) && !xfurn_t.can_examine( pos ) ) {
                    return;
                } else if( ( forage_bushes && xter_t.has_examine( iexamine::shrub_marloss ) ) ||
                           ( forage_bushes && xter_t.has_examine( iexamine::shrub_wildveggies ) ) ||
                           ( forage_bushes && xter_t.has_examine( iexamine::harvest_ter_nectar ) ) ||
                           ( forage_trees && xter_t.has_examine( iexamine::tree_marloss ) ) ||
                           ( forage_trees && xter_t.has_examine( iexamine::harvest_ter ) ) ||
                           ( forage_trees && xter_t.has_examine( iexamine::harvest_ter_nectar ) ) ||
                           ( forage_crops && xter_t.has_examine( iexamine::harvest_plant_ex ) )
                         ) {
                    xter_t.examine( u, pos );
                } else if( ( forage_everything && xfurn_t.has_examine( iexamine::harvest_furn ) ) ||
                           ( forage_everything && xfurn_t.has_examine( iexamine::harvest_furn_nectar ) ) ||
                           ( forage_crops && xfurn_t.has_examine( iexamine::harvest_plant_ex ) )
                         ) {
                    xfurn_t.examine( u, pos );
                }
            };

            for( const direction &elem : adjacentDir ) {
                forage( u.pos() + displace_XY( elem ) );
            }
        }

        const std::string pulp_butcher = get_option<std::string>( "AUTO_PULP_BUTCHER" );
        if( pulp_butcher == "butcher" &&
            u.max_quality( qual_BUTCHER, PICKUP_RANGE ) > INT_MIN ) {
            std::vector<item *> corpses;

            for( item &it : m.i_at( u.pos() ) ) {
                corpses.push_back( &it );
            }

            if( !corpses.empty() ) {
                u.assign_activity( ACT_BUTCHER, 0, true );
                for( item *it : corpses ) {
                    u.activity.targets.emplace_back( map_cursor( u.pos() ), it );
                }
            }
        } else if( pulp_butcher == "pulp" || pulp_butcher == "pulp_adjacent" ||
                   pulp_butcher == "pulp_zombie_only" || pulp_butcher == "pulp_adjacent_zombie_only" ) {
            const bool acid_immune = u.is_immune_damage( damage_acid ) || u.is_immune_field( fd_acid );
            const auto pulp = [&]( const tripoint & pos ) {
                for( const item &maybe_corpse : m.i_at( pos ) ) {
                    if( maybe_corpse.is_corpse() && maybe_corpse.can_revive() &&
                        ( !maybe_corpse.get_mtype()->bloodType().obj().has_acid || acid_immune ) ) {

                        if( pulp_butcher == "pulp_zombie_only" || pulp_butcher == "pulp_adjacent_zombie_only" ) {
                            if( !maybe_corpse.get_mtype()->has_flag( mon_flag_REVIVES ) ) {
                                continue;
                            }
                        }

                        u.assign_activity( ACT_PULP, calendar::INDEFINITELY_LONG, 0 );
                        u.activity.placement = m.getglobal( pos );
                        u.activity.auto_resume = true;
                        u.activity.str_values.emplace_back( "auto_pulp_no_acid" );
                        return;
                    }
                }
            };

            if( pulp_butcher == "pulp_adjacent" || pulp_butcher == "pulp_adjacent_zombie_only" ) {
                for( const direction &elem : adjacentDir ) {
                    pulp( u.pos() + displace_XY( elem ) );
                }
            } else {
                pulp( u.pos() );
            }
        }
    }

    // Auto pickup
    if( !quick && !u.is_mounted() && get_option<bool>( "AUTO_PICKUP" ) && !u.is_hauling() &&
        ( !get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) ||
          !u.get_mon_visible().has_dangerous_creature_in_proximity ) &&
        ( m.has_items( u.pos() ) || get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
        Pickup::autopickup( u.pos() );
    }

    // If the new tile is a boardable part, board it
    if( !was_in_control_same_pos && vp1.part_with_feature( "BOARDABLE", true ) && !u.is_mounted() ) {
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
    if( m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, u.pos() ) &&
        !m.has_flag_furn( "BRIDGE", u.pos() ) &&
        !( u.is_mounted() || ( u.in_vehicle && vp1->vehicle().can_float() ) ) ) {
        u.drench( 80, u.get_drenching_body_parts( false, false ),
                  false );
    }

    // List items here
    if( !quick && !m.has_flag( ter_furn_flag::TFLAG_SEALED, u.pos() ) ) {
        if( get_option<bool>( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS" ) ||
            !check_zone( zone_type_NO_AUTO_PICKUP, u.pos() ) ) {
            if( u.is_blind() && !m.i_at( u.pos() ).empty() ) {
                add_msg( _( "There's something here, but you can't see what it is." ) );
            } else if( m.has_items( u.pos() ) ) {
                std::vector<std::string> names;
                std::vector<size_t> counts;
                std::vector<item> items;
                for( item &tmpitem : m.i_at( u.pos() ) ) {

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
                    std::string fmt = n_gettext( "%1$d %2$s", "%1$d %2$s", counts[i] );
                    names[i] = string_format( fmt, counts[i], names[i] );
                    // Skip the first two.
                    if( i > 1 ) {
                        and_the_rest += counts[i];
                    }
                }

                if( get_option<bool>( "LOG_ITEMS_ON_THE_GROUND" ) ) {
                    if( names.size() == 1 ) {
                        add_msg( _( "You see here %s." ), names[0] );
                    } else if( names.size() == 2 ) {
                        add_msg( _( "You see here %s and %s." ), names[0], names[1] );
                    } else if( names.size() == 3 ) {
                        add_msg( _( "You see here %s, %s, and %s." ), names[0], names[1], names[2] );
                    } else if( and_the_rest < 7 ) {
                        add_msg( n_gettext( "You see here %s, %s and %d more item.",
                                            "You see here %s, %s and %d more items.",
                                            and_the_rest ),
                                 names[0], names[1], and_the_rest );
                    } else {
                        add_msg( _( "You see here %s and many more items." ), names[0] );
                    }
                }
            }
        }
    }

    if( !was_in_control_same_pos &&
        ( vp1.part_with_feature( "CONTROL_ANIMAL", true ) ||
          vp1.part_with_feature( "CONTROLS", true ) ) && u.in_vehicle && !u.is_mounted() ) {
        add_msg( _( "There are vehicle controls here." ) );
        if( !u.has_trait( trait_WAYFARER ) ) {
            add_msg( m_info, _( "%s to drive." ), press_x( ACTION_CONTROL_VEHICLE ) );
        }
    } else if( vp1.part_with_feature( "CONTROLS", true ) && u.in_vehicle &&
               u.is_mounted() ) {
        add_msg( _( "There are vehicle controls here but you cannot reach them whilst mounted." ) );
    }
    return submap_shift;
}

void game::place_player_overmap( const tripoint_abs_omt &om_dest, bool move_player )
{
    // if player is teleporting around, they don't bring their horse with them
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
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        m.clear_vehicle_list( z );
    }
    m.rebuild_vehicle_level_caches();
    m.access_cache( m.get_abs_sub().z() ).map_memory_cache_dec.reset();
    m.access_cache( m.get_abs_sub().z() ).map_memory_cache_ter.reset();
    // offset because load_map expects the coordinates of the top left corner, but the
    // player will be centered in the middle of the map.
    const tripoint_abs_sm map_sm_pos =
        project_to<coords::sm>( om_dest ) - point( HALF_MAPSIZE, HALF_MAPSIZE );
    const tripoint player_pos( u.pos().xy(), map_sm_pos.z() );
    load_map( map_sm_pos );
    load_npcs();
    m.spawn_monsters( true ); // Static monsters
    update_overmap_seen();
    // update weather now as it could be different on the new location
    weather.nextweather = calendar::turn;
    if( move_player ) {
        place_player( player_pos );
    }
}

bool game::phasing_move( const tripoint &dest_loc, const bool via_ramp )
{
    const units::energy trigger_cost = bio_probability_travel->power_trigger;

    if( !u.has_active_bionic( bio_probability_travel ) ||
        u.get_power_level() < trigger_cost ) {
        return false;
    }

    if( dest_loc.z != u.posz() && !via_ramp ) {
        // No vertical phasing yet
        return false;
    }

    //probability travel through walls but not water
    tripoint dest = dest_loc;
    // tile is impassable
    int tunneldist = 0;
    const point d( sgn( dest.x - u.posx() ), sgn( dest.y - u.posy() ) );
    creature_tracker &creatures = get_creature_tracker();
    while( m.impassable( dest ) ||
           ( creatures.creature_at( dest ) != nullptr && tunneldist > 0 ) ) {
        //add 1 to tunnel distance for each impassable tile in the line
        tunneldist += 1;
        //Being dimensionally anchored prevents quantum shenanigans.
        if( u.worn_with_flag( flag_DIMENSIONAL_ANCHOR ) ||
            u.has_flag( flag_DIMENSIONAL_ANCHOR ) ) {
            u.add_msg_if_player( m_info, _( "You are repelled by the barrier!" ) );
            u.mod_power_level( -trigger_cost ); //cost of tunneling one tile.
            return false;
        }
        if( tunneldist * trigger_cost >
            u.get_power_level() ) { //oops, not enough energy! Tunneling costs 250 bionic power per impassable tile
            add_msg( _( "You try to quantum tunnel through the barrier but are reflected!  Try again with more energy!" ) );
            u.mod_power_level( -trigger_cost );
            return false;
        }
        if( tunneldist > 24 ) {
            add_msg( m_info, _( "It's too dangerous to tunnel that far!" ) );
            u.mod_power_level( -trigger_cost );
            return false;
        }

        dest.x += d.x;
        dest.y += d.y;
    }

    if( tunneldist != 0 ) {
        if( u.in_vehicle ) {
            m.unboard_vehicle( u.pos() );
        }

        add_msg( _( "You quantum tunnel through the %d-tile wide barrier!" ), tunneldist );
        //tunneling costs 250 bionic power per impassable tile
        u.mod_power_level( -( tunneldist * trigger_cost ) );
        u.moves -= 100; //tunneling costs 100 moves
        u.setpos( dest );

        if( m.veh_at( u.pos() ).part_with_feature( "BOARDABLE", true ) ) {
            m.board_vehicle( u.pos(), &u );
        }

        u.grab( object_type::NONE );
        on_move_effects();
        m.creature_on_trap( u );
        return true;
    }

    return false;
}

bool game::can_move_furniture( tripoint fdest, const tripoint &dp )
{
    const bool pulling_furniture = dp.xy() == -u.grab_point.xy();
    const bool has_floor = m.has_floor_or_water( fdest );
    creature_tracker &creatures = get_creature_tracker();
    bool is_ramp_or_road = m.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, fdest ) ||
                           m.has_flag( ter_furn_flag::TFLAG_RAMP_UP, fdest ) ||
                           m.has_flag( ter_furn_flag::TFLAG_ROAD, fdest );
    return  m.passable( fdest ) &&
            creatures.creature_at<npc>( fdest ) == nullptr &&
            creatures.creature_at<monster>( fdest ) == nullptr &&
            ( !pulling_furniture || is_empty( u.pos() + dp ) ) &&
            ( !has_floor || m.has_flag( ter_furn_flag::TFLAG_FLAT, fdest ) ||
              is_ramp_or_road ) &&
            !m.has_furn( fdest ) &&
            !m.veh_at( fdest ) &&
            ( !has_floor || m.tr_at( fdest ).is_null() );
}

int game::grabbed_furn_move_time( const tripoint &dp )
{
    // Furniture: pull, push, or standing still and nudging object around.
    // Can push furniture out of reach.
    tripoint fpos = u.pos() + u.grab_point;
    // supposed position of grabbed furniture
    if( !m.has_furn( fpos ) ) {
        return 0;
    }

    tripoint fdest = fpos + tripoint( dp.xy(), 0 ); // intended destination of furniture.

    const bool canmove = can_move_furniture( fdest, dp );
    const furn_t &furntype = m.furn( fpos ).obj();
    const int dst_items = m.i_at( fdest ).size();

    const bool only_liquid_items = std::all_of( m.i_at( fdest ).begin(), m.i_at( fdest ).end(),
    [&]( item & liquid_item ) {
        return liquid_item.made_of_from_type( phase_id::LIQUID );
    } );

    const bool dst_item_ok = !m.has_flag( ter_furn_flag::TFLAG_NOITEM, fdest ) &&
                             !m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, fdest ) &&
                             !m.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, fdest ) &&
                             only_liquid_items;
    const bool src_item_ok = m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_CONTAINER ) ||
                             m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER ) ||
                             m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_SEALED );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( item &contained_item : m.i_at( fpos ) ) {
        furniture_contents_weight += contained_item.weight();
    }
    str_req += furniture_contents_weight / 4_kilogram;
    //ARM_STR affects dragging furniture
    int str = u.get_arm_str();

    const float weary_mult = 1.0f / u.exertion_adjusted_move_multiplier();
    if( !canmove ) { // NOLINT(bugprone-branch-clone)
        return 50 * weary_mult;
    } else if( str_req > str &&
               one_in( std::max( 20 - ( str_req - str ), 2 ) ) ) {
        return 100 * weary_mult;
    } else if( !src_item_ok && !dst_item_ok && dst_items > 0 ) {
        return 50 * weary_mult;
    }
    int moves_total = 0;
    moves_total = str_req * 10;
    // Additional penalty if we can't comfortably move it.
    if( str_req > str ) {
        int move_penalty = std::pow( str_req, 2.0 ) + 100.0;
        if( move_penalty <= 1000 ) {
            if( str >= str_req - 3 ) {
                moves_total += std::max( 3000, move_penalty * 10 ) * weary_mult;
            } else {
                moves_total += 100 * weary_mult;
                return moves_total;
            }
        }
        moves_total += move_penalty;
    }
    return moves_total;
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
        u.grab( object_type::NONE );
        return false;
    }

    int ramp_offset = 0;
    // Furniture could be on a ramp at different time than player so adjust for that.
    if( m.has_flag( ter_furn_flag::TFLAG_RAMP_UP, fpos + tripoint( dp.xy(), 0 ) ) ) {
        ramp_offset = 1;
    } else if( m.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, fpos + tripoint( dp.xy(), 0 ) ) ) {
        ramp_offset = -1;
    }

    const bool pushing_furniture = dp.xy() ==  u.grab_point.xy();
    const bool pulling_furniture = dp.xy() == -u.grab_point.xy();
    const bool shifting_furniture = !pushing_furniture && !pulling_furniture;

    // Intended destination of furniture.
    const tripoint fdest = fpos + tripoint( dp.xy(), ramp_offset );

    // Unfortunately, game::is_empty fails for tiles we're standing on,
    // which will forbid pulling, so:
    const bool canmove = can_move_furniture( fdest, dp );
    // @TODO: it should be possible to move over invisible traps. This should probably
    // trigger the trap.
    // The current check (no move if trap) allows a player to detect invisible traps by
    // attempting to move stuff onto it.

    const furn_t furntype = m.furn( fpos ).obj();
    const int src_items = m.i_at( fpos ).size();
    const int dst_items = m.i_at( fdest ).size();

    const bool only_liquid_items = std::all_of( m.i_at( fdest ).begin(), m.i_at( fdest ).end(),
    [&]( item & liquid_item ) {
        return liquid_item.made_of_from_type( phase_id::LIQUID );
    } );

    const bool dst_item_ok = !m.has_flag( ter_furn_flag::TFLAG_NOITEM, fdest ) &&
                             !m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, fdest ) &&
                             !m.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, fdest );

    const bool src_item_ok = m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_CONTAINER ) ||
                             m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER ) ||
                             m.furn( fpos ).obj().has_flag( ter_furn_flag::TFLAG_SEALED );

    const int fire_intensity = m.get_field_intensity( fpos, fd_fire );
    time_duration fire_age = m.get_field_age( fpos, fd_fire );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( item &contained_item : m.i_at( fpos ) ) {
        furniture_contents_weight += contained_item.weight();
    }
    str_req += furniture_contents_weight / 4_kilogram;
    int str = u.get_arm_str();

    if( !canmove ) {
        // TODO: What is something?
        add_msg( _( "The %s collides with something." ), furntype.name() );
        return true;
    } else if( str_req > str && u.get_perceived_pain() > 40 &&
               !u.has_trait( trait_CENOBITE ) && !u.has_trait( trait_MASOCHIST ) &&
               !u.has_trait( trait_MASOCHIST_MED ) ) {
        add_msg( m_bad, _( "You are in too much pain to try moving the heavy %s!" ),
                 furntype.name() );
        return true;

    } else if( str_req > str && u.get_perceived_pain() > 50 &&
               ( u.has_trait( trait_MASOCHIST ) || u.has_trait( trait_MASOCHIST_MED ) ) ) {
        add_msg( m_bad,
                 _( "Even with your appetite for pain, you are in too much pain to try moving the heavy %s!" ),
                 furntype.name() );
        return true;

        ///\EFFECT_STR determines ability to drag furniture
    } else if( str_req > str &&
               one_in( std::max( 20 - str_req - str, 2 ) ) ) {
        add_msg( m_bad, _( "You strain yourself trying to move the heavy %s!" ),
                 furntype.name() );
        u.mod_pain( 1 ); // Hurt ourselves.
        return true; // furniture and or obstacle wins.
    } else if( !src_item_ok && !only_liquid_items && dst_items > 0 ) {
        add_msg( _( "There's stuff in the way." ) );
        return true;
    }

    // Additional penalty if we can't comfortably move it.
    if( str_req > str ) {
        int move_penalty = std::pow( str_req, 2.0 ) + 100.0;
        if( move_penalty <= 1000 ) {
            if( str >= str_req - 3 ) {
                add_msg( m_bad, _( "The %s is really heavy!" ), furntype.name() );
                if( one_in( 3 ) ) {
                    add_msg( m_bad, _( "You fail to move the %s." ), furntype.name() );
                    return true;
                }
            } else {
                add_msg( m_bad, _( "The %s is too heavy for you to budge." ), furntype.name() );
                return true;
            }
        }
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
    m.furn_set( fpos, f_null, true );

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
            for( item &cur_item : temp ) {
                m.i_at( fdest ).insert( cur_item );
            }
        } else {
            add_msg( _( "Stuff spills from the %s!" ), furntype.name() );
        }
    }

    if( !m.has_floor_or_water( fdest ) && !m.has_flag( ter_furn_flag::TFLAG_FLAT, fdest ) ) {
        std::string danger_tile = enumerate_as_string( get_dangerous_tile( fdest ) );
        add_msg( _( "You let go of the %1$s as it falls down the %2$s." ), furntype.name(), danger_tile );
        u.grab( object_type::NONE );
        m.drop_furniture( fdest );
        return true;
    }

    u.grab_point.z += ramp_offset;

    if( shifting_furniture ) {
        // We didn't move
        tripoint d_sum = u.grab_point + dp;
        if( std::abs( d_sum.x ) < 2 && std::abs( d_sum.y ) < 2 ) {
            u.grab_point = d_sum; // furniture moved relative to us
        } else { // we pushed furniture out of reach
            add_msg( _( "You let go of the %s." ), furntype.name() );
            u.grab( object_type::NONE );
        }
        return true; // We moved furniture but stayed still.
    }

    if( pushing_furniture && m.impassable( fpos ) ) {
        // Not sure how that chair got into a wall, but don't let player follow.
        add_msg( _( "You let go of the %1$s as it slides past %2$s." ),
                 furntype.name(), m.tername( fdest ) );
        u.grab( object_type::NONE );
        return true;
    }

    return false;
}

bool game::grabbed_move( const tripoint &dp, const bool via_ramp )
{
    if( u.get_grab_type() == object_type::NONE ) {
        return false;
    }

    // vehicle: pulling, pushing, or moving around the grabbed object.
    if( u.get_grab_type() == object_type::VEHICLE ) {
        return grabbed_veh_move( dp );
    }

    if( u.get_grab_type() == object_type::FURNITURE ) {
        u.assign_activity( move_furniture_activity_actor( dp, via_ramp ) );
        return true;
    }

    add_msg( m_info, _( "Nothing at grabbed point %d,%d,%d or bad grabbed object type." ),
             u.grab_point.x, u.grab_point.y, u.grab_point.z );
    u.grab( object_type::NONE );
    return false;
}

void game::on_move_effects()
{
    // TODO: Move this to a character method
    if( !u.is_mounted() ) {
        const item muscle( "muscle" );
        for( const bionic_id &bid : u.get_bionic_fueled_with_muscle() ) {
            if( u.has_active_bionic( bid ) ) {// active power gen
                u.mod_power_level( muscle.fuel_energy() * bid->fuel_efficiency );
            } else {// passive power gen
                u.mod_power_level( muscle.fuel_energy() * bid->passive_fuel_efficiency );
            }
        }
        if( u.has_active_bionic( bio_jointservo ) ) {
            if( u.is_running() ) {
                u.mod_power_level( -bio_jointservo->power_trigger * 1.55 );
            } else {
                u.mod_power_level( -bio_jointservo->power_trigger );
            }

            // If there's not enough power to run Joint Servo, forcefully turn it off
            if( u.get_power_level() <= 0_mJ ) {
                u.force_bionic_deactivation( **u.find_bionic_by_type( bio_jointservo ) );
                u.add_msg_if_player( m_bad, _( "Your Joint Servo bionic powers down." ) );
            }
        }
    }

    if( u.is_running() ) {
        if( !u.can_run() ) {
            u.toggle_run_mode();
        }
        if( u.get_stamina() <= 0 ) {
            u.add_effect( effect_winded, 10_turns );
        }
    }

    // apply martial art move bonuses
    u.martial_arts_data->ma_onmove_effects( u );

    sfx::do_ambient();
}

void game::on_options_changed()
{
#if defined(TILES)
    tilecontext->on_options_changed();
#endif
}

void game::water_affect_items( Character &ch ) const
{
    std::vector<item_location> dissolved;
    std::vector<item_location> destroyed;
    std::vector<item_location> wet;

    for( item_location &loc : ch.all_items_loc() ) {
        // check flag first because its cheaper
        if( loc->has_flag( flag_WATER_DISSOLVE ) && !loc.protected_from_liquids() ) {
            dissolved.emplace_back( loc );
        } else if( loc->has_flag( flag_WATER_BREAK ) && !loc->is_broken()
                   && !loc.protected_from_liquids() ) {
            destroyed.emplace_back( loc );
        } else if( loc->has_flag( flag_WATER_BREAK_ACTIVE ) && !loc->is_broken()
                   && !loc.protected_from_liquids() ) {
            wet.emplace_back( loc );
        } else if( loc->typeId() == itype_towel && !loc.protected_from_liquids() ) {
            loc->convert( itype_towel_wet, &ch ).active = true;
        }
    }

    if( dissolved.empty() && destroyed.empty() && wet.empty() ) {
        return;
    }

    for( item_location &it : dissolved ) {
        add_msg_if_player_sees( ch.pos(), m_bad, _( "%1$s %2$s dissolved in the water!" ),
                                ch.disp_name( true, true ), it->display_name() );
        it.remove_item();
    }

    for( item_location &it : destroyed ) {
        add_msg_if_player_sees( ch.pos(), m_bad, _( "The water destroyed %1$s %2$s!" ),
                                ch.disp_name( true ), it->display_name() );
        it->deactivate();
        it->set_flag( flag_ITEM_BROKEN );
    }

    for( item_location &it : wet ) {
        const int wetness_add = 5100 * std::log10( units::to_milliliter( it->volume() ) );
        it->wetness += wetness_add;
        it->wetness = std::min( it->wetness, 5 * wetness_add );
    }
}

bool game::fling_creature( Creature *c, const units::angle &dir, float flvel, bool controlled,
                           bool intentional )
{
    if( c == nullptr ) {
        debugmsg( "game::fling_creature invoked on null target" );
        return false;
    }

    if( c->is_dead_state() ) {
        // Flinging a corpse causes problems, don't enable without testing
        return false;
    }

    if( c->is_hallucination() ) {
        // Don't fling hallucinations
        return false;
    }

    // Target creature shouldn't be grabbed if thrown
    // It should also not be thrown if the throw is weaker than the grab
    for( const effect &eff : c->get_effects_with_flag( json_flag_GRAB ) ) {
        if( !x_in_y( flvel / 2, eff.get_intensity() ) ) {
            c->add_msg_player_or_npc( m_warning,
                                      _( "You're almost sent flying, but something holds you in place!" ),
                                      _( "<npcname> is almost sent flying, but something holds them in place!" ) );
            return false;
        } else {
            c->remove_effect( eff.get_id(), eff.get_bp() );
        }
    }

    bool thru = true;
    const bool is_u = c == &u;
    // Don't animate critters getting bashed if animations are off
    const bool animate = is_u || get_option<bool>( "ANIMATIONS" );

    Character *you = dynamic_cast<Character *>( c );

    // at this point we should check if knockback immune
    // assuming that the shove wasn't intentional
    if( !intentional && you != nullptr ) {

        double knockback_resist = 0.0;
        knockback_resist = you->calculate_by_enchantment( knockback_resist,
                           enchant_vals::mod::KNOCKBACK_RESIST );
        if( knockback_resist >= 1 ) {
            // you are immune
            return false;
        }

        // 1.0 knockback resist is immune, 0 is normal
        flvel *= 1 - knockback_resist;
    }

    tileray tdir( dir );
    int range = flvel / 10;
    tripoint pt = c->pos();
    creature_tracker &creatures = get_creature_tracker();
    while( range > 0 ) {
        c->underwater = false;
        // TODO: Check whenever it is actually in the viewport
        // or maybe even just redraw the changed tiles
        bool seen = is_u || u.sees( *c ); // To avoid redrawing when not seen
        tdir.advance();
        pt.x = c->posx() + tdir.dx();
        pt.y = c->posy() + tdir.dy();
        float force = 0.0f;

        if( monster *const mon_ptr = creatures.creature_at<monster>( pt ) ) {
            monster &critter = *mon_ptr;
            // Approximate critter's "stopping power" with its max hp
            force = std::min<float>( 1.5f * critter.type->hp, flvel );
            const int damage = rng( force, force * 2.0f ) / 6;
            c->impact( damage, pt );
            // Multiply zed damage by 6 because no body parts
            const int zed_damage = std::max( 0,
                                             ( damage - critter.get_armor_type( damage_bash, bodypart_id( "torso" ) ) ) * 6 );
            // TODO: Pass the "flinger" here - it's not the flung critter that deals damage
            critter.apply_damage( c, bodypart_id( "torso" ), zed_damage );
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
            return true;
        }

        flvel -= force;
        if( thru ) {
            if( you != nullptr ) {
                if( you->in_vehicle ) {
                    m.unboard_vehicle( you->pos() );
                }
                // If we're flinging the player around, make sure the map stays centered on them.
                if( is_u ) {
                    update_map( pt.x, pt.y );
                } else {
                    you->setpos( pt );
                }
            } else if( !creatures.creature_at( pt ) ) {
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
        if( animate && ( seen || u.sees( *c ) ) ) {
            invalidate_main_ui_adaptor();
            inp_mngr.pump_events();
            ui_manager::redraw_invalidated();
            refresh_display();
        }
    }

    // Fall down to the ground - always on the last reached tile
    if( !m.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, c->pos() ) ) {
        const trap &trap_under_creature = m.tr_at( c->pos() );
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

        if( you != nullptr ) {
            water_affect_items( *you );
        }

        if( is_u ) {
            if( controlled ) {
                add_msg( _( "You dive into water." ) );
            } else {
                add_msg( m_warning, _( "You fall into water." ) );
            }
        }
    }
    return true;
}

std::optional<tripoint> game::point_selection_menu( const std::vector<tripoint> &pts, bool up )
{
    if( pts.empty() ) {
        debugmsg( "point_selection_menu called with empty point set" );
        return std::nullopt;
    }

    if( pts.size() == 1 ) {
        return pts[0];
    }

    const tripoint upos = get_player_character().pos();
    uilist pmenu;
    pmenu.title = _( "Climb where?" );
    int num = 0;
    for( const tripoint &pt : pts ) {
        // TODO: Sort the menu so that it can be used with numpad directions
        const std::string &dir_arrow = direction_arrow( direction_from( upos.xy(), pt.xy() ) );
        const std::string &dir_name = direction_name( direction_from( upos.xy(), pt.xy() ) );
        const std::string &up_or_down = up ? _( "Climb up %s (%s)" ) : _( "Climb down %s (%s)" );
        // TODO: Inform player what is on said tile
        // But don't just print terrain name (in many cases it will be "open air")
        pmenu.addentry( num++, true, MENU_AUTOASSIGN, string_format( up_or_down, dir_name, dir_arrow ) );
    }

    pmenu.query();
    const int ret = pmenu.ret;
    if( ret < 0 || ret >= num ) {
        return std::nullopt;
    }

    return pts[ret];
}

static std::optional<tripoint> find_empty_spot_nearby( const tripoint &pos )
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &p : here.points_in_radius( pos, 1 ) ) {
        if( p == pos ) {
            continue;
        }
        if( here.impassable( p ) ) {
            continue;
        }
        if( creatures.creature_at( p ) ) {
            continue;
        }
        return p;
    }
    return std::nullopt;
}

void game::vertical_move( int movez, bool force, bool peeking )
{
    if( u.is_mounted() ) {
        monster *mons = u.mounted_creature.get();
        if( mons->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return;
            }
        }
    }

    map &here = get_map();

    // Force means we're going down, even if there's no staircase, etc.
    bool climbing = false;
    climbing_aid_id climbing_aid = climbing_aid_default;
    int move_cost = 100;
    tripoint stairs( u.posx(), u.posy(), u.posz() + movez );
    bool wall_cling = u.has_flag( json_flag_WALL_CLING );
    bool adjacent_climb = false;
    if( !force && movez == 1 && !here.has_flag( ter_furn_flag::TFLAG_GOES_UP, u.pos() ) &&
        !u.is_underwater() ) {
        // Climbing
        for( const tripoint &p : here.points_in_radius( u.pos(), 2 ) ) {
            if( here.has_flag( ter_furn_flag::TFLAG_CLIMB_ADJACENT, p ) ) {
                adjacent_climb = true;
            }
        }
        if( here.has_floor_or_support( stairs ) ) {
            add_msg( m_info, _( "You can't climb here - there's a ceiling above your head." ) );
            return;
        }

        if( u.get_working_arm_count() < 1 && !here.has_flag( ter_furn_flag::TFLAG_LADDER, u.pos() ) ) {
            add_msg( m_info, _( "You can't climb because your arms are too damaged or encumbered." ) );
            return;
        }

        const int cost = u.climbing_cost( u.pos(), stairs );
        add_msg_debug( debugmode::DF_GAME, "Climb cost %d", cost );
        const bool can_climb_here = cost > 0 ||
                                    u.has_flag( json_flag_CLIMB_NO_LADDER ) || wall_cling;
        if( !can_climb_here && !adjacent_climb ) {
            add_msg( m_info, _( "You can't climb here - you need walls and/or furniture to brace against." ) );
            return;
        }

        const item_location weapon = u.get_wielded_item();
        if( !here.has_flag( ter_furn_flag::TFLAG_LADDER, u.pos() ) && weapon &&
            weapon->is_two_handed( u ) ) {
            if( query_yn(
                    _( "You can't climb because you have to wield a %s with both hands.\n\nPut it away?" ),
                    weapon->tname() ) ) {
                if( !u.unwield() ) {
                    return;
                }
            } else {
                return;
            }
        }

        std::vector<tripoint> pts;
        for( const tripoint &pt : here.points_in_radius( stairs, 1 ) ) {
            if( here.passable( pt ) &&
                here.has_floor_or_support( pt ) ) {
                pts.push_back( pt );
            }
        }

        if( wall_cling && here.is_wall_adjacent( stairs ) ) {
            pts.push_back( stairs );
        }

        if( pts.empty() ) {
            add_msg( m_info,
                     _( "You can't climb here - there is no terrain above you that would support your weight." ) );
            return;
        } else {
            // TODO: Make it an extended action
            climbing = true;
            climbing_aid = climbing_aid_furn_CLIMBABLE;
            u.set_activity_level( EXTRA_EXERCISE );
            move_cost = cost == 0 ? 1000 : cost + 500;

            const std::optional<tripoint> pnt = point_selection_menu( pts );
            if( !pnt ) {
                return;
            }
            stairs = *pnt;
        }
    }

    if( !force && movez == -1 && !here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, u.pos() ) &&
        !u.is_underwater() && !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR_WATER, u.pos() ) &&
        !u.has_effect( effect_gliding ) ) {
        if( wall_cling && !here.has_floor_or_support( u.pos() ) ) {
            climbing = true;
            climbing_aid = climbing_aid_ability_WALL_CLING;
            u.set_activity_level( EXTRA_EXERCISE );
            u.mod_stamina( -750 );
            move_cost += 500;
        } else {
            add_msg( m_info, _( "You can't go down here!" ) );
            return;
        }
    } else if( !climbing && !force && movez == 1 &&
               !here.has_flag( ter_furn_flag::TFLAG_GOES_UP, u.pos() ) && !u.is_underwater() ) {
        add_msg( m_info, _( "You can't go up here!" ) );
        return;
    }

    if( force ) {
        // Let go of a grabbed cart.
        u.grab( object_type::NONE );
    } else if( u.grab_point != tripoint_zero ) {
        add_msg( m_info, _( "You can't drag things up and down stairs." ) );
        return;
    }

    // TODO: Use u.posz() instead of m.abs_sub
    const int z_after = m.get_abs_sub().z() + movez;
    if( z_after < -OVERMAP_DEPTH ) {
        add_msg( m_info, _( "Halfway down, the way down becomes blocked off." ) );
        return;
    } else if( z_after >= OVERMAP_HEIGHT ) {
        add_msg( m_info, _( "Halfway up, the way up becomes blocked off." ) );
        return;
    }

    if( !u.move_effects( false ) && !force ) {
        u.moves -= 100;
        return;
    }

    if( climbing &&
        slip_down( ( ( movez > 1 ) ? climb_maneuver::up : climb_maneuver::down ), climbing_aid ) ) {
        return;
    }

    bool swimming = false;
    bool surfacing = false;
    bool submerging = false;
    // > and < are used for diving underwater.
    if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, u.pos() ) ) {
        swimming = true;
        const ter_id &target_ter = here.ter( u.pos() + tripoint( 0, 0, movez ) );

        // If we're in a water tile that has both air above and deep enough water to submerge in...
        if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, u.pos() ) &&
            !here.has_flag( ter_furn_flag::TFLAG_WATER_CUBE, u.pos() ) ) {
            // ...and we're trying to swim down
            if( movez == -1 ) {
                // ...and we're already submerged
                if( u.is_underwater() ) {
                    // ...and there's more water beneath us.
                    if( target_ter->has_flag( ter_furn_flag::TFLAG_WATER_CUBE ) ) {
                        // Then go ahead and move down.
                        add_msg( _( "You swim down." ) );
                    } else {
                        // There's no more water beneath us.
                        add_msg( m_info,
                                 _( "You are already underwater and there is no more water beneath you to swim down!" ) );
                        return;
                    }
                }
                // ...and we're not already submerged.
                else {
                    // Check for a flotation device first before allowing us to submerge.
                    if( u.worn_with_flag( flag_FLOTATION ) ) {
                        add_msg( m_info, _( "You can't dive while wearing a flotation device." ) );
                        return;
                    }

                    // Then dive under the surface.
                    u.set_underwater( true );
                    add_msg( _( "You dive underwater!" ) );
                    submerging = true;
                }
            }
            // ...and we're trying to surface
            else if( movez == 1 ) {
                // ... and we're already submerged
                if( u.is_underwater() ) {
                    if( u.swim_speed() < 500 || u.shoe_type_count( itype_swim_fins ) ) {
                        u.set_underwater( false );
                        add_msg( _( "You surface." ) );
                        surfacing = true;
                    } else {
                        add_msg( m_info, _( "You try to surface but can't!" ) );
                        return;
                    }
                }
            }
        }
        // If we're in a water tile that is entirely water
        else if( here.has_flag( ter_furn_flag::TFLAG_WATER_CUBE, u.pos() ) ) {
            // If you're at this point, you should already be underwater, but force that to be the case.
            if( !u.is_underwater() ) {
                u.set_underwater( true );
            }

            // ...and we're trying to swim down
            if( movez == -1 ) {
                // ...and there's more water beneath us.
                if( target_ter->has_flag( ter_furn_flag::TFLAG_WATER_CUBE ) ) {
                    // Then go ahead and move down.
                    add_msg( _( "You swim down." ) );
                } else {
                    add_msg( m_info,
                             _( "You are already underwater and there is no more water beneath you to swim down!" ) );
                    return;
                }
            }
            // ...and we're trying to move up
            else if( movez == 1 ) {
                // ...and there's more water above us us.
                if( target_ter->has_flag( ter_furn_flag::TFLAG_WATER_CUBE ) ||
                    target_ter->has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ) {
                    // Then go ahead and move up.
                    add_msg( _( "You swim up." ) );
                } else {
                    add_msg( m_info, _( "You are already underwater and there is no water above you to swim up!" ) );
                    return;
                }
            }
        }
    }

    // Find the corresponding staircase
    bool rope_ladder = false;
    // TODO: Remove the stairfinding, make the mapgen gen aligned maps
    if( !force && !climbing && !swimming ) {
        const std::optional<tripoint> pnt = find_or_make_stairs( m, z_after, rope_ladder, peeking,
                                            u.pos() );
        if( !pnt ) {
            return;
        }
        stairs = *pnt;
    }

    std::vector<monster *> monsters_following;
    if( std::abs( movez ) == 1 ) {
        bool ladder = here.has_flag( ter_furn_flag::TFLAG_DIFFICULT_Z, u.pos() );
        for( monster &critter : all_monsters() ) {
            if( ladder && !critter.climbs() ) {
                continue;
            }
            // TODO: just check if it's going for the avatar's location, it's simpler
            Creature *target = critter.attack_target();
            if( ( target && target->is_avatar() ) || ( !critter.has_effect( effect_ridden ) &&
                    ( critter.is_pet_follow() || critter.has_effect( effect_led_by_leash ) ) &&
                    !critter.has_effect( effect_tied ) && critter.sees( u ) ) ) {
                monsters_following.push_back( &critter );
            }
        }
    }

    if( u.is_mounted() ) {
        monster *crit = u.mounted_creature.get();
        if( crit->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            crit->use_mech_power( u.current_movement_mode()->mech_power_use() + 1_kJ );
        }
    } else {
        u.moves -= move_cost;
        u.mod_stamina( -move_cost );
    }

    if( surfacing || submerging ) {
        // Surfacing and submerging don't actually move us anywhere, and just
        // toggle our underwater state in the same location.
        return;
    }

    const tripoint old_pos = u.pos();
    const tripoint old_abs_pos = here.getabs( old_pos );
    point submap_shift;
    const bool z_level_changed = vertical_shift( z_after );
    if( !force ) {
        submap_shift = update_map( stairs.x, stairs.y, z_level_changed );
    }

    // if an NPC or monster is on the stairs when player ascends/descends
    // they may end up merged on the same tile, do some displacement to resolve that.
    creature_tracker &creatures = get_creature_tracker();
    if( creatures.creature_at<npc>( u.pos(), true ) ||
        creatures.creature_at<monster>( u.pos(), true ) ) {
        std::string crit_name;
        bool player_displace = false;
        std::optional<tripoint> displace = find_empty_spot_nearby( u.pos() );
        if( !displace.has_value() ) {
            // They can always move to the previous location of the player.
            displace = old_pos;
        }
        npc *guy = creatures.creature_at<npc>( u.pos(), true );
        if( guy ) {
            crit_name = guy->get_name();
            tripoint old_pos = guy->pos();
            if( !guy->is_enemy() ) {
                guy->move_away_from( u.pos(), true );
                if( old_pos != guy->pos() ) {
                    add_msg( _( "%s moves out of the way for you." ), guy->get_name() );
                }
            } else {
                player_displace = true;
            }
        }
        monster *mon = creatures.creature_at<monster>( u.pos(), true );
        // if the monster is ridden by the player or an NPC:
        // Dont displace them. If they are mounted by a friendly NPC,
        // then the NPC will already have been displaced just above.
        // if they are ridden by the player, we want them to coexist on same tile
        if( mon && !mon->mounted_player ) {
            crit_name = mon->get_name();
            if( mon->friendly == -1 ) {
                mon->setpos( *displace );
                add_msg( _( "Your %s moves out of the way for you." ), mon->get_name() );
            } else {
                player_displace = true;
            }
        }
        if( player_displace ) {
            u.setpos( *displace );
            u.moves -= 20;
            add_msg( _( "You push past %s blocking the way." ), crit_name );
        }
    }

    // Now that we know the player's destination position, we can move their mount as well
    if( u.is_mounted() ) {
        u.mounted_creature->setpos( u.pos() );
    }

    // This ugly check is here because of stair teleport bullshit
    // TODO: Remove stair teleport bullshit
    if( rl_dist( u.pos(), old_pos ) <= 1 ) {
        for( monster *m : monsters_following ) {
            m->set_dest( u.get_location() );
        }
    }

    if( rope_ladder ) {
        if( u.has_flag( json_flag_WEB_RAPPEL ) ) {
            here.furn_set( u.pos(), furn_f_web_up );
        } else {
            here.furn_set( u.pos(), furn_f_rope_up );
        }
    }

    if( here.ter( stairs ) == t_manhole_cover ) {
        here.spawn_item( stairs + point( rng( -1, 1 ), rng( -1, 1 ) ), itype_manhole_cover );
        here.ter_set( stairs, t_manhole );
    }

    if( u.is_hauling() ) {
        const tripoint adjusted_pos = old_pos - sm_to_ms_copy( submap_shift );
        start_hauling( adjusted_pos );
    }

    here.invalidate_map_cache( here.get_abs_sub().z() );
    // Upon force movement, traps can not be avoided.
    if( !wall_cling && ( get_map().tr_at( u.pos() ) == tr_ledge &&
                         !u.has_effect( effect_gliding ) ) )  {
        here.creature_on_trap( u, !force );
    }

    u.recoil = MAX_RECOIL;

    cata_event_dispatch::avatar_moves( old_abs_pos, u, m );
}

void game::start_hauling( const tripoint &pos )
{
    std::vector<item_location> candidate_items = m.get_haulable_items( pos );
    // Find target items and quantities thereof for the new activity
    u.trim_haul_list( candidate_items );
    std::vector<item_location> target_items = u.haul_list;

    if( u.is_autohauling() && !u.suppress_autohaul ) {
        for( const item_location &item : u.haul_list ) {
            candidate_items.erase( std::remove( candidate_items.begin(), candidate_items.end(), item ),
                                   candidate_items.end() );
        }
        if( u.hauling_filter.empty() ) {
            target_items.insert( target_items.end(), candidate_items.begin(), candidate_items.end() );
        } else {
            std::function<bool( const item & )> filter = item_filter_from_string( u.hauling_filter );
            std::copy_if( candidate_items.begin(), candidate_items.end(), std::back_inserter( target_items ),
            [&filter]( const item_location & item ) {
                return filter( *item );
            } );
        }
    }

    u.suppress_autohaul = false;
    u.haul_list.clear();

    // Quantity of 0 means move all
    const std::vector<int> quantities( target_items.size(), 0 );

    if( target_items.empty() ) {
        // Nothing to haul
        if( !u.is_autohauling() ) {
            u.stop_hauling();
        }
        return;
    }

    // Whether the destination is inside a vehicle (not supported)
    const bool to_vehicle = false;
    // Destination relative to the player
    const tripoint relative_destination{};

    const move_items_activity_actor actor( target_items, quantities, to_vehicle, relative_destination,
                                           true );
    u.assign_activity( actor );
}

std::optional<tripoint> game::find_or_make_stairs( map &mp, const int z_after, bool &rope_ladder,
        bool peeking, const tripoint &pos )
{
    const bool is_avatar = u.pos() == pos;
    const int omtilesz = SEEX * 2;
    real_coords rc( mp.getabs( pos.xy() ) );
    tripoint omtile_align_start( mp.getlocal( rc.begin_om_pos() ), z_after );
    tripoint omtile_align_end( omtile_align_start + point( -1 + omtilesz, -1 + omtilesz ) );

    // Try to find the stairs.
    std::optional<tripoint> stairs;
    int best = INT_MAX;
    const int movez = z_after - pos.z;
    const bool going_down_1 = movez == -1;
    const bool going_up_1 = movez == 1;
    // If there are stairs on the same x and y as we currently are, use those
    if( going_down_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos + tripoint_below ) ) {
        stairs.emplace( pos + tripoint_below );
    }
    if( going_up_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos + tripoint_above ) ) {
        stairs.emplace( pos + tripoint_above );
    }
    // We did not find stairs directly above or below, so search the map for them
    // If there's empty space right below us, we can just go down that way.
    if( !stairs.has_value() && get_map().tr_at( u.pos() ) != tr_ledge ) {
        for( const tripoint &dest : mp.points_in_rectangle( omtile_align_start, omtile_align_end ) ) {
            if( rl_dist( u.pos(), dest ) <= best &&
                ( ( going_down_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_UP, dest ) ) ||
                  ( going_up_1 && ( mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, dest ) ||
                                    mp.ter( dest ) == t_manhole_cover ) ) ||
                  ( ( movez == 2 || movez == -2 ) && mp.ter( dest ) == t_elevator ) ) ) {
                stairs.emplace( dest );
                best = rl_dist( u.pos(), dest );
            }
        }
    }

    creature_tracker &creatures = get_creature_tracker();
    if( stairs.has_value() ) {
        if( !is_avatar ) {
            return stairs;
        }
        if( Creature *blocking_creature = creatures.creature_at( stairs.value() ) ) {
            npc *guy = dynamic_cast<npc *>( blocking_creature );
            monster *mon = dynamic_cast<monster *>( blocking_creature );
            bool would_move = ( guy && !guy->is_enemy() ) || ( mon && mon->friendly == -1 );
            std::string cr_name = blocking_creature->get_name();
            std::string msg;
            if( guy ) {
                //~ %s is the name of hostile NPC
                msg = string_format( _( "%s is in the way!" ), cr_name );
            } else {
                //~ %s is some monster
                msg = string_format( _( "There's a %s in the way!" ), cr_name );
            }

            if( ( peeking && !would_move )
                || ( !would_move && !query_yn(
                         //~ %s is a warning about monster/hostile NPC in the way, e.g. "There's a zombie in the way!"
                         _( "%s  Attempt to push past?  You may have to fight your way back up." ), msg ) ) ) {
                add_msg( msg );
                return std::nullopt;
            }
        }
        return stairs;
    }

    if( !is_avatar || peeking ) {
        return std::nullopt;
    }
    // No stairs found! Try to make some
    rope_ladder = false;
    stairs.emplace( pos );
    stairs->z = z_after;
    // Check the destination area for lava.
    if( mp.ter( *stairs ) == t_lava ) {
        if( movez < 0 &&
            !query_yn(
                _( "There is a LOT of heat coming out of there, even the stairs have melted away.  Jump down?  You won't be able to get back up." ) ) ) {
            return std::nullopt;
        }
        if( movez > 0 &&
            !query_yn(
                _( "There is a LOT of heat coming out of there.  Push through the half-molten rocks and ascend?  You will not be able to get back down." ) ) ) {
            return std::nullopt;
        }

        return stairs;
    }

    if( u.has_effect( effect_gliding ) && get_map().tr_at( u.pos() ) == tr_ledge ) {
        return stairs;
    }

    if( movez > 0 ) {
        if( !mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, *stairs ) ) {
            if( !query_yn( _( "You may be unable to return back down these stairs.  Continue up?" ) ) ) {
                return std::nullopt;
            }
        }
        // Manhole covers need this to work
        // Maybe require manhole cover here and fail otherwise?
        return stairs;
    }

    if( mp.impassable( *stairs ) ) {
        popup( _( "Halfway down, the way down becomes blocked off." ) );
        return std::nullopt;
    }

    if( u.has_flag( json_flag_WEB_RAPPEL ) ) {
        if( query_yn( _( "There is a sheer drop halfway down.  Web-descend?" ) ) ) {
            rope_ladder = true;
            if( rng( 4, 8 ) < u.get_skill_level( skill_dodge ) ) {
                add_msg( _( "You attach a web and dive down headfirst, flipping upright and landing on your feet." ) );
            } else {
                add_msg( _( "You securely web up and work your way down, lowering yourself safely." ) );
            }
        } else {
            return std::nullopt;
        }
    } else if( u.has_trait( trait_VINES2 ) || u.has_trait( trait_VINES3 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down.  Use your vines to descend?" ) ) ) {
            if( u.has_trait( trait_VINES2 ) ) {
                if( query_yn( _( "Detach a vine?  It'll hurt, but you'll be able to climb back up…" ) ) ) {
                    rope_ladder = true;
                    add_msg( m_bad, _( "You descend on your vines, though leaving a part of you behind stings." ) );
                    u.mod_pain( 5 );
                    u.apply_damage( nullptr, bodypart_id( "torso" ), 5 );
                    u.mod_stored_kcal( -87 );
                    u.mod_thirst( 10 );
                } else {
                    add_msg( _( "You gingerly descend using your vines." ) );
                }
            } else {
                add_msg( _( "You effortlessly lower yourself and leave a vine rooted for future use." ) );
                rope_ladder = true;
                u.mod_stored_kcal( -87 );
                u.mod_thirst( 10 );
            }
        } else {
            return std::nullopt;
        }
    } else if( u.has_amount( itype_grapnel, 1 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down.  Climb your grappling hook down?" ) ) ) {
            rope_ladder = true;
            for( item &used_item : u.use_amount( itype_grapnel, 1 ) ) {
                used_item.spill_contents( u );
            }
        } else {
            return std::nullopt;
        }
    } else if( u.has_amount( itype_rope_30, 1 ) ) {
        if( query_yn( _( "There is a sheer drop halfway down.  Climb your rope down?" ) ) ) {
            rope_ladder = true;
            for( item &used_item : u.use_amount( itype_rope_30, 1 ) ) {
                used_item.spill_contents( u );
            }
        } else {
            return std::nullopt;
        }
    } else if( !query_yn( _( "There is a sheer drop halfway down.  Jump?" ) ) ) {
        return std::nullopt;
    }

    return stairs;
}

bool game::vertical_shift( const int z_after )
{
    if( z_after < -OVERMAP_DEPTH || z_after > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  z_after, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return false;
    }

    scent.reset();

    const int z_before = u.posz();
    u.move_to( tripoint_abs_ms( u.get_location().xy(), z_after ) );

    // Shift the map itself
    m.vertical_shift( z_after );

    vertical_notes( z_before, z_after );

    return z_before != z_after;
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
    for( const tripoint_abs_omt &p : points_in_radius( u.global_omt_location(), REVEAL_RADIUS ) ) {
        const tripoint_abs_omt cursp_before( p.xy(), z_before );
        const tripoint_abs_omt cursp_after( p.xy(), z_after );

        if( !overmap_buffer.seen( cursp_before ) ) {
            continue;
        }
        if( overmap_buffer.has_note( cursp_after ) ) {
            // Already has a note -> never add an AUTO-note
            continue;
        }
        const oter_id &ter = overmap_buffer.ter( cursp_before );
        const oter_id &ter2 = overmap_buffer.ter( cursp_after );
        if( z_after > z_before && ter->has_flag( oter_flags::known_up ) &&
            !ter2->has_flag( oter_flags::known_down ) ) {
            overmap_buffer.set_seen( cursp_after, true );
            overmap_buffer.add_note( cursp_after, string_format( ">:W;%s", _( "AUTO: goes down" ) ) );
        } else if( z_after < z_before && ter->has_flag( oter_flags::known_down ) &&
                   !ter2->has_flag( oter_flags::known_up ) ) {
            overmap_buffer.set_seen( cursp_after, true );
            overmap_buffer.add_note( cursp_after, string_format( "<:W;%s", _( "AUTO: goes up" ) ) );
        }
    }
}

point game::update_map( Character &p, bool z_level_changed )
{
    point p2( p.posx(), p.posy() );
    return update_map( p2.x, p2.y, z_level_changed );
}

point game::update_map( int &x, int &y, bool z_level_changed )
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
        u.setpos( tripoint( x, y, m.get_abs_sub().z() ) );
        if( z_level_changed ) {
            // Update what parts of the world map we can see
            // We may be able to see farther now that the z-level has changed.
            update_overmap_seen();
        }
        // Not actually shifting the submaps, all the stuff below would do nothing
        return point_zero;
    }

    // this handles loading/unloading submaps that have scrolled on or off the viewport
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    inclusive_rectangle<point> size_1( point( -1, -1 ), point( 1, 1 ) );
    point remaining_shift = shift;
    while( remaining_shift != point_zero ) {
        point this_shift = clamp( remaining_shift, size_1 );
        m.shift( this_shift );
        remaining_shift -= this_shift;
    }

    // Shift monsters
    shift_monsters( tripoint( shift, 0 ) );
    const point shift_ms = sm_to_ms_copy( shift );
    u.shift_destination( -shift_ms );

    // Shift NPCs
    for( auto it = critter_tracker->active_npc.begin(); it != critter_tracker->active_npc.end(); ) {
        ( *it )->shift( shift );
        if( ( *it )->posx() < 0 || ( *it )->posx() >= MAPSIZE_X ||
            ( *it )->posy() < 0 || ( *it )->posy() >= MAPSIZE_Y ) {
            //Remove the npc from the active list. It remains in the overmap list.
            ( *it )->on_unload();
            it = critter_tracker->active_npc.erase( it );
        } else {
            it++;
        }
    }

    scent.shift( shift_ms );

    // Also ensure the player is on current z-level
    // m.get_abs_sub().z should later be removed, when there is no longer such a thing
    // as "current z-level"
    u.setpos( tripoint( x, y, m.get_abs_sub().z() ) );

    // Only do the loading after all coordinates have been shifted.

    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();

    // Make sure map cache is consistent since it may have shifted.
    for( int zlev = -OVERMAP_DEPTH; zlev <= OVERMAP_HEIGHT; ++zlev ) {
        m.invalidate_map_cache( zlev );
    }
    m.build_map_cache( m.get_abs_sub().z() );

    // Spawn monsters if appropriate
    // This call will generate new monsters in addition to loading, so it's placed after NPC loading
    m.spawn_monsters( false ); // Static monsters

    // Update what parts of the world map we can see
    update_overmap_seen();

    return shift;
}

void game::update_overmap_seen()
{
    const tripoint_abs_omt ompos = u.global_omt_location();
    const int dist = u.overmap_sight_range( light_level( u.posz() ) );
    const int dist_squared = dist * dist;
    // We can always see where we're standing
    overmap_buffer.set_seen( ompos, true );
    for( const tripoint_abs_omt &p : points_in_radius( ompos, dist ) ) {
        const point_rel_omt delta = p.xy() - ompos.xy();
        const int h_squared = delta.x() * delta.x() + delta.y() * delta.y();
        if( trigdist && h_squared > dist_squared ) {
            continue;
        }
        if( delta == point_rel_omt() ) {
            // 1. This case is already handled outside of the loop
            // 2. Calculating multiplier would cause division by zero
            continue;
        }
        // If circular distances are enabled, scale overmap distances by the diagonality of the sight line.
        point abs_delta = delta.raw().abs();
        int max_delta = std::max( abs_delta.x, abs_delta.y );
        const float multiplier = trigdist ? std::sqrt( h_squared ) / max_delta : 1;
        const std::vector<tripoint_abs_omt> line = line_to( ompos, p );
        float sight_points = dist;
        for( auto it = line.begin();
             it != line.end() && sight_points >= 0; ++it ) {
            const oter_id &ter = overmap_buffer.ter( *it );
            sight_points -= static_cast<int>( ter->get_see_cost() ) * multiplier;
        }
        if( sight_points >= 0 ) {
            tripoint_abs_omt seen( p );
            do {
                overmap_buffer.set_seen( seen, true );
                --seen.z();
            } while( seen.z() >= 0 );
        }
    }
}

void game::despawn_monster( monster &critter )
{
    critter.on_unload();
    // hallucinations aren't stored, they come and go as they like
    if( !critter.is_hallucination() ) {
        // despawn_monster saves a copy of the monster in the overmap, so
        // this must be called after on_unload (which updates state)
        overmap_buffer.despawn_monster( critter );
    }
    remove_zombie( critter );
    // simulate it being dead so further processing of it (e.g. in monmove) will yield
    critter.set_hp( 0 );
}

void game::despawn_nonlocal_monsters()
{
    for( monster &critter : g->all_monsters() ) {
        if( critter.posx() < 0 - MAPSIZE_X / 6 ||
            critter.posy() < 0 - MAPSIZE_Y / 6 ||
            critter.posx() > ( MAPSIZE_X * 7 ) / 6 ||
            critter.posy() > ( MAPSIZE_Y * 7 ) / 6 ) {
            g->despawn_monster( critter );
        }
    }
}

void game::shift_monsters( const tripoint &shift )
{
    if( shift == tripoint_zero ) {
        return;
    }
    for( monster &critter : all_monsters() ) {
        if( shift.xy() != point_zero ) {
            critter.shift( shift.xy() );
        }

        if( m.inbounds( critter.pos() ) ) {
            // We're inbounds, so don't despawn after all.
            continue;
        }
        // The critter is now outside of the reality bubble; it must be saved and removed.
        despawn_monster( critter );
    }
}

void game::perhaps_add_random_npc( bool ignore_spawn_timers_and_rates )
{
    if( !ignore_spawn_timers_and_rates && !calendar::once_every( 1_hours ) ) {
        return;
    }
    // Create a new NPC?

    double spawn_time = get_option<float>( "NPC_SPAWNTIME" );
    if( !ignore_spawn_timers_and_rates && spawn_time == 0.0 ) {
        return;
    }

    // spawn algorithm is a chance per hour, but the config is specified in average days
    // actual chance per hour is (100 / 24 ) / days
    static constexpr double days_to_rate_factor = 100.0 / 24;
    double spawn_rate = days_to_rate_factor / spawn_time;
    static constexpr int radius_spawn_range = 90;
    std::vector<shared_ptr_fast<npc>> npcs = overmap_buffer.get_npcs_near_player( radius_spawn_range );
    size_t npc_num = npcs.size();
    for( auto &npc : npcs ) {
        if( npc->has_trait( trait_NPC_STATIC_NPC ) || npc->has_trait( trait_NPC_STARTING_NPC ) ) {
            npc_num--;
        }
    }

    if( npc_num > 0 ) {
        // 100%, 80%, 64%, 52%, 41%, 33%...
        spawn_rate *= std::pow( 0.8f, npc_num );
    }

    if( !ignore_spawn_timers_and_rates && !x_in_y( spawn_rate, 100 ) ) {
        return;
    }
    bool spawn_allowed = false;
    tripoint_abs_omt spawn_point;
    int counter = 0;
    while( !spawn_allowed ) {
        if( counter >= 10 ) {
            return;
        }
        const tripoint_abs_omt u_omt = u.global_omt_location();
        spawn_point = u_omt + point( rng( -radius_spawn_range, radius_spawn_range ),
                                     rng( -radius_spawn_range, radius_spawn_range ) );
        // Only spawn random NPCs on z-level 0
        spawn_point.z() = 0;
        const oter_id oter = overmap_buffer.ter( spawn_point );
        // shouldn't spawn on bodies of water.
        if( !is_water_body( oter ) ) {
            spawn_allowed = true;
        }
        counter += 1;
    }
    shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
    tmp->normalize();
    tmp->randomize();

    if( one_in( 100 ) ) {
        // Same chances and duration of flu vs. cold as for the player.
        if( one_in( 6 ) ) {
            tmp->add_effect( effect_fake_flu, rng( 3_days, 10_days ) );
        } else {
            tmp->add_effect( effect_fake_common_cold, rng( 1_days, 14_days ) );
        }
    }

    std::string new_fac_id = "solo_";
    new_fac_id += tmp->name;
    // create a new "lone wolf" faction for this one NPC
    faction *new_solo_fac = faction_manager_ptr->add_new_faction( tmp->name, faction_id( new_fac_id ),
                            faction_no_faction );
    tmp->set_fac( new_solo_fac ? new_solo_fac->id : faction_no_faction );
    // adds the npc to the correct overmap.
    tmp->spawn_at_omt( spawn_point );
    overmap_buffer.insert_npc( tmp );
    tmp->form_opinion( u );
    tmp->mission = NPC_MISSION_NULL;
    tmp->long_term_goal_action();
    tmp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, tmp->global_omt_location(),
                          tmp->getID() ) );
    // This will make the new NPC active- if its nearby to the player
    load_npcs();
}

// Redraw window and show spinner, so cata window doesn't look frozen while pathfinding on overmap
void game::display_om_pathfinding_progress( size_t /* open_set */, size_t /* known_size */ )
{
    ui_adaptor dummy( ui_adaptor::disable_uis_below {} );
    static_popup pop;
    pop.on_top( true ).wait_message( "%s", _( "Hang on a bit…" ) );
    ui_manager::redraw();
    refresh_display();
    inp_mngr.pump_events();
}

bool game::display_overlay_state( const action_id action )
{
    return displaying_overlays && *displaying_overlays == action;
}

void game::display_toggle_overlay( const action_id action )
{
    if( display_overlay_state( action ) ) {
        displaying_overlays.reset();
    } else {
        displaying_overlays = action;
    }
}

void game::display_scent()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_SCENT );
    } else {
        int div;
        bool got_value = query_int( div, _( "Set the Scent Map sensitivity to (0 to cancel)?" ) );
        if( !got_value || div < 1 ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        shared_ptr_fast<game::draw_callback_t> scent_cb = make_shared_fast<game::draw_callback_t>( [&]() {
            scent.draw( w_terrain, div * 2, u.pos() + u.view_offset );
        } );
        g->add_draw_callback( scent_cb );

        ui_manager::redraw();
        inp_mngr.wait_for_any_key();
    }
}

void game::display_temperature()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_TEMPERATURE );
    }
}

void game::display_vehicle_ai()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_VEHICLE_AI );
    }
}

void game::display_visibility()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_VISIBILITY );
        if( display_overlay_state( ACTION_DISPLAY_VISIBILITY ) ) {
            std::vector< tripoint > locations;
            uilist creature_menu;
            int num_creatures = 0;
            creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
            locations.emplace_back( get_player_character().pos() ); // add player first.
            for( const Creature &critter : g->all_creatures() ) {
                if( critter.is_avatar() ) {
                    continue;
                }
                creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, critter.disp_name() );
                locations.emplace_back( critter.pos() );
            }

            pointmenu_cb callback( locations );
            creature_menu.callback = &callback;
            creature_menu.w_y_setup = 0;
            creature_menu.query();
            if( creature_menu.ret >= 0 && static_cast<size_t>( creature_menu.ret ) < locations.size() ) {
                Creature *creature = get_creature_tracker().creature_at<Creature>( locations[creature_menu.ret] );
                displaying_visibility_creature = creature;
            }
        } else {
            displaying_visibility_creature = nullptr;
        }
    }
}

void game::toggle_debug_hour_timer()
{
    debug_hour_timer.toggle();
}

void game::debug_hour_timer::toggle()
{
    enabled = !enabled;
    start_time = std::nullopt;
    add_msg( string_format( "debug timer %s", enabled ? "enabled" : "disabled" ) );
}

void game::debug_hour_timer::print_time()
{
    if( enabled ) {
        if( calendar::once_every( time_duration::from_hours( 1 ) ) ) {
            const IRLTimeMs now = std::chrono::time_point_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() );
            if( start_time ) {
                add_msg( "in-game hour took: %d ms", ( now - *start_time ).count() );
            } else {
                add_msg( "starting debug timer" );
            }
            start_time = now;
        }
    }
}

void game::display_lighting()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_LIGHTING );
        if( !g->display_overlay_state( ACTION_DISPLAY_LIGHTING ) ) {
            return;
        }
        uilist lighting_menu;
        std::vector<std::string> lighting_menu_strings{
            "Global lighting conditions"
        };

        int count = 0;
        for( const auto &menu_str : lighting_menu_strings ) {
            lighting_menu.addentry( count++, true, MENU_AUTOASSIGN, "%s", menu_str );
        }

        lighting_menu.w_y_setup = 0;
        lighting_menu.query();
        if( ( lighting_menu.ret >= 0 ) &&
            ( static_cast<size_t>( lighting_menu.ret ) < lighting_menu_strings.size() ) ) {
            g->displaying_lighting_condition = lighting_menu.ret;
        }
    }
}

void game::display_radiation()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_RADIATION );
    }
}

void game::display_transparency()
{
    if( use_tiles ) {
        display_toggle_overlay( ACTION_DISPLAY_TRANSPARENCY );
    }
}

void game::init_autosave()
{
    moves_since_last_save = 0;
    last_save_timestamp = std::time( nullptr );
}

void game::quicksave()
{
    //Don't autosave if the player hasn't done anything since the last autosave/quicksave,
    if( !moves_since_last_save ) {
        return;
    }
    add_msg( m_info, _( "Saving game, this may take a while." ) );

    static_popup popup;
    popup.message( "%s", _( "Saving game, this may take a while." ) );
    ui_manager::redraw();
    refresh_display();

    time_t now = std::time( nullptr ); //timestamp for start of saving procedure

    //perform save
    save();
    //Now reset counters for autosaving, so we don't immediately autosave after a quicksave or autosave.
    moves_since_last_save = 0;
    last_save_timestamp = now;
}

void game::quickload()
{
    const WORLD *active_world = world_generator->active_world;
    if( active_world == nullptr ) {
        return;
    }
    std::string const world_name = active_world->world_name;
    std::string const &save_id = u.get_save_id();

    if( active_world->save_exists( save_t::from_save_id( save_id ) ) ) {
        if( moves_since_last_save != 0 ) { // See if we need to reload anything
            moves_since_last_save = 0;
            last_save_timestamp = std::time( nullptr );

            u.moves = 0;
            uquit = QUIT_NOSAVED;

            main_menu::queued_world_to_load = world_name;
            main_menu::queued_save_id_to_load = save_id;

        }

    } else {
        popup_getkey( _( "No saves for current character yet." ) );
    }
}

void game::autosave()
{
    //Don't autosave if the min-autosave interval has not passed since the last autosave/quicksave.
    if( std::time( nullptr ) < last_save_timestamp + 60 * get_option<int>( "AUTOSAVE_MINUTES" ) ) {
        return;
    }
    quicksave();    //Driving checks are handled by quicksave()
}

void game::start_calendar()
{
    calendar::start_of_cataclysm = scen->start_of_cataclysm();
    calendar::start_of_game = scen->start_of_game();
    calendar::turn = calendar::start_of_game;
    calendar::initial_season = static_cast<season_type>( ( to_days<int>( calendar::start_of_game -
                               calendar::turn_zero ) / get_option<int>( "SEASON_LENGTH" ) ) % season_type::NUM_SEASONS );
}

overmap &game::get_cur_om() const
{
    // The player is located in the middle submap of the map.
    const tripoint_abs_sm sm = m.get_abs_sub() + tripoint( HALF_MAPSIZE, HALF_MAPSIZE, 0 );
    const tripoint_abs_om pos_om = project_to<coords::om>( sm );
    return overmap_buffer.get( pos_om.xy() );
}

std::vector<npc *> game::allies()
{
    return get_npcs_if( [&]( const npc & guy ) {
        if( !guy.is_hallucination() ) {
            return guy.is_ally( get_player_character() );
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

std::vector<Character *> game::get_characters_if( const std::function<bool( const Character & )>
        &pred )
{
    std::vector<Character *> result;
    avatar &a = get_avatar();
    if( pred( a ) ) {
        result.push_back( &a );
    }
    for( npc &guy : all_npcs() ) {
        if( pred( guy ) ) {
            result.push_back( &guy );
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
    return true; // must be the avatar
}

game::monster_range::monster_range( game &game_ref )
{
    const auto &monsters = game_ref.critter_tracker->get_monsters_list();
    items.insert( items.end(), monsters.begin(), monsters.end() );
}

game::Creature_range::Creature_range( game &game_ref ) : u( &game_ref.u, []( Character * ) { } )
{
    const auto &monsters = game_ref.critter_tracker->get_monsters_list();
    items.insert( items.end(), monsters.begin(), monsters.end() );
    items.insert( items.end(), game_ref.critter_tracker->active_npc.begin(),
                  game_ref.critter_tracker->active_npc.end() );
    items.emplace_back( u );
}

game::npc_range::npc_range( game &game_ref )
{
    items.insert( items.end(), game_ref.critter_tracker->active_npc.begin(),
                  game_ref.critter_tracker->active_npc.end() );
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

std::string PATH_INFO::player_base_save_path()
{
    return PATH_INFO::world_base_save_path() + "/" + base64_encode( get_avatar().get_save_id() );
}

cata_path PATH_INFO::player_base_save_path_path()
{
    return PATH_INFO::world_base_save_path_path() / base64_encode( get_avatar().get_save_id() );
}

std::string PATH_INFO::world_base_save_path()
{
    if( world_generator->active_world == nullptr ) {
        return PATH_INFO::savedir();
    }
    return world_generator->active_world->folder_path();
}

cata_path PATH_INFO::world_base_save_path_path()
{
    if( world_generator->active_world == nullptr ) {
        return PATH_INFO::savedir_path();
    }
    return world_generator->active_world->folder_path_path();
}

void game::shift_destination_preview( const point &delta )
{
    for( tripoint_bub_ms &p : destination_preview ) {
        p += delta;
    }
}

int game::slip_down_chance( climb_maneuver, climbing_aid_id aid_id,
                            bool show_chance_messages )
{
    if( aid_id.is_null() ) {
        // The NULL climbing aid ID may be passed as a default argument.
        aid_id = climbing_aid_default;
    }

    const climbing_aid &aid = aid_id.obj();

    int slip = 100;

    const bool parkour = u.has_proficiency( proficiency_prof_parkour );
    const bool badknees = u.has_trait( trait_BADKNEES );
    if( parkour && badknees ) {
        if( show_chance_messages ) {
            add_msg( m_info, _( "Your skill in parkour makes up for your bad knees while climbing." ) );
        }
    } else if( u.has_proficiency( proficiency_prof_parkour ) ) {
        slip /= 2;
        if( show_chance_messages ) {
            add_msg( m_info, _( "Your skill in parkour makes it easier to climb." ) );
        }
    } else if( u.has_trait( trait_BADKNEES ) ) {
        slip *= 2;
        if( show_chance_messages ) {
            add_msg( m_info, _( "Your bad knees make it difficult to climb." ) );
        }
    }

    add_msg_debug( debugmode::DF_GAME, "Slip chance after proficiency/trait modifiers %d%%", slip );

    // Climbing is difficult with wet hands and feet.
    float wet_penalty = 1.0f;
    bool wet_feet = false;
    bool wet_hands = false;

    for( const bodypart_id &bp : u.get_all_body_parts_of_type( body_part_type::type::foot,
            get_body_part_flags::primary_type ) ) {
        if( u.get_part_wetness( bp ) > 0 ) {
            add_msg_debug( debugmode::DF_GAME, "Foot %s %.1f wet", body_part_name( bp ),
                           u.get_part( bp )->get_wetness_percentage() );
            wet_feet = true;
            wet_penalty += u.get_part( bp )->get_wetness_percentage() / 2;
        }
    }

    for( const bodypart_id &bp : u.get_all_body_parts_of_type( body_part_type::type::hand,
            get_body_part_flags::primary_type ) ) {
        if( u.get_part_wetness( bp ) > 0 ) {
            add_msg_debug( debugmode::DF_GAME, "Hand %s %.1f wet", body_part_name( bp ),
                           u.get_part( bp )->get_wetness_percentage() );
            wet_hands = true;
            wet_penalty += u.get_part( bp )->get_wetness_percentage() / 2;
        }
    }
    if( show_chance_messages ) {
        if( wet_feet && wet_hands ) {
            add_msg( m_info, _( "Your wet hands and feet make it harder to climb." ) );
        } else if( wet_feet ) {
            add_msg( m_info, _( "Your wet feet make it harder to climb." ) );
        } else if( wet_hands ) {
            add_msg( m_info, _( "Your wet hands make it harder to climb." ) );
        }
    }

    // Apply wetness penalty
    slip *= wet_penalty;

    add_msg_debug( debugmode::DF_GAME, "Slip chance after wetness penalty %d%%", slip );

    ///\EFFECT_DEX decreases chances of slipping while climbing
    ///\EFFECT_STR decreases chances of slipping while climbing
    /// Not using arm strength since lifting score comes into play later
    slip /= std::max( 1, u.dex_cur + u.str_cur );

    add_msg_debug( debugmode::DF_GAME, "Slip chance after stat modifiers %d%%", slip );

    // Apply limb score penalties - grip, arm strength and footing are all relevant
    slip /= u.get_modifier( character_modifier_slip_prevent_mod );
    add_msg_debug( debugmode::DF_GAME, "Slipping chance after limb scores %d%%", slip );

    // Being weighed down makes it easier for you to slip.
    double weight_ratio = static_cast<double>( units::to_gram( u.weight_carried() ) ) / units::to_gram(
                              u.weight_capacity() );
    slip += roll_remainder( 8.0 * weight_ratio );
    add_msg_debug( debugmode::DF_GAME, "Weight ratio %.2f, slip chance %d%%", weight_ratio,
                   slip );

    // Decreasing stamina makes you slip up more often
    const float stamina_ratio = static_cast<float>( u.get_stamina() ) / u.get_stamina_max();
    if( stamina_ratio < 0.8 ) {
        slip /= std::max( stamina_ratio, .1f );

        if( show_chance_messages ) {
            if( stamina_ratio > 0.6 ) {
                add_msg( m_info, _( "You are winded, which makes climbing harder." ) );
            } else if( stamina_ratio > 0.4 ) {
                add_msg( m_info, _( "You are out of breath, which makes climbing much harder." ) );
            } else if( stamina_ratio > 0.2 ) {
                add_msg( m_info, _( "You can't catch your breath, which makes it much more difficult to climb." ) );
            } else {
                add_msg( m_info, _( "You feel faint and can't keep your balance." ) );
            }
        }
    }
    add_msg_debug( debugmode::DF_GAME, "Stamina ratio %.2f, slip chance %d%%",
                   stamina_ratio, slip );

    if( show_chance_messages ) {
        if( weight_ratio >= 1 ) {
            add_msg( m_info, _( "Your carried weight tries to drag you down." ) );
        } else if( weight_ratio > .75 ) {
            add_msg( m_info, _( "You strain to climb with the weight of your possessions." ) );
        } else if( weight_ratio > .5 ) {
            add_msg( m_info, _( "You feel the weight of your luggage makes it more difficult to climb." ) );
        } else if( weight_ratio > .25 ) {
            add_msg( m_info, _( "Your carried weight makes it a little harder to climb." ) );
        }
    }

    // Affordances (other than ledge) may reduce slip chance, even below zero.
    slip += aid.slip_chance_mod;
    if( show_chance_messages ) {
        if( aid.slip_chance_mod >= 0 ) {
            // TODO allow for a message specific to the climbing aid?
            add_msg( m_info, _( "There's nothing here to help you climb." ) );
        }
    }

    add_msg_debug( debugmode::DF_GAME, "After affordance modifier, final slip chance %d%%",
                   slip );

    return slip;
}

bool game::slip_down( climb_maneuver maneuver, climbing_aid_id aid_id,
                      bool show_chance_messages )
{
    int slip = slip_down_chance( maneuver, aid_id, show_chance_messages );

    if( x_in_y( slip, 100 ) ) {
        add_msg( m_bad, _( "You slip while climbing and fall down." ) );
        if( slip >= 100 ) {
            add_msg( m_bad, _( "Climbing is impossible in your current state." ) );
        }
        // Check for traps if climbing UP or DOWN.  Note that ledges (open air) count as traps.
        if( maneuver != climb_maneuver::over_obstacle ) {
            m.creature_on_trap( u );
        }
        return true;
    }
    return false;
}

// These helpers map climbing aid IDs to/from integers for use as return values in uilist.
//   The integers are offset by 4096 to avoid collision with other options (see iexamine::ledge)
static int climb_affordance_menu_encode( const climbing_aid_id &aid_id )
{
    return 0x1000 + int_id<climbing_aid>( aid_id ).to_i();
}
static bool climb_affordance_menu_decode( int retval, climbing_aid_id &aid_id )
{
    int_id< climbing_aid > as_int_id( retval - 0x1000 );
    if( as_int_id.is_valid() ) {
        aid_id = as_int_id.id();
        return true;
    }
    return false;
}

void game::climb_down_menu_gen( const tripoint &examp, uilist &cmenu )
{
    // NOTE this menu may be merged with the iexamine::ledge menu, manage keys carefully!

    map &here = get_map();
    Character &you = get_player_character();

    if( !here.valid_move( you.pos(), examp, false, true ) ) {
        // Can't move horizontally to the ledge
        return;
    }

    // Scan the height of the drop and what's in the way.
    const climbing_aid::fall_scan fall( examp );

    add_msg_debug( debugmode::DF_IEXAMINE, "Ledge height %d", fall.height );
    if( fall.height == 0 ) {
        you.add_msg_if_player( _( "You can't climb down there." ) );
        return;
    }

    // This is used to mention object names.  TODO make this more flexible.
    std::string target_disp_name = m.disp_name( fall.pos_furniture_or_floor() );

    climbing_aid::condition_list conditions = climbing_aid::detect_conditions( you, examp );

    climbing_aid::aid_list aids = climbing_aid::list( conditions );

    // Debug:
    {
        add_msg_debug( debugmode::DF_IEXAMINE, "Climbing aid conditions: %d", conditions.size() );
        for( climbing_aid::condition &cond : conditions ) {
            add_msg_debug( debugmode::DF_IEXAMINE, cond.category_string() + ": " + cond.flag );
        }
        add_msg_debug( debugmode::DF_IEXAMINE, "Climbing aids available: %d", aids.size() );
        for( const climbing_aid *aid : climbing_aid::list_all( conditions ) ) {
            add_msg_debug( debugmode::DF_IEXAMINE, "#%d %s: %s (%s)",
                           climb_affordance_menu_encode( aid->id ), aid->id.str(),
                           aid->down.menu_text.translated(), target_disp_name );
        }
        add_msg_debug( debugmode::DF_IEXAMINE, "%d-level drop; %d until furniture; %d until creature.",
                       fall.height, fall.height_until_furniture, fall.height_until_creature );
    }

    for( const climbing_aid *aid : aids ) {
        // Enable climbing aid unless it would deploy furniture in occupied tiles.
        bool enable_aid = true;
        if( aid->down.deploys_furniture() &&
            fall.height_until_furniture < std::min( fall.height, aid->down.max_height ) ) {
            // Can't deploy because it would overwrite existing furniture.
            enable_aid = false;
        }

        // Certain climbing aids can't be used for partial descent.
        if( !aid->down.allow_remaining_height && aid->down.max_height < fall.height ) {
            // TODO this check could block the safest non-deploying aid!
            enable_aid = false;
        }

        int hotkey = aid->down.menu_hotkey;
        if( hotkey == 0 ) {
            // Deployables require a hotkey def and we only show one non-deployable; use 'c' for it.
            hotkey = 'c';
        }

        std::string text_translated = enable_aid ?
                                      aid->down.menu_text.translated() : aid->down.menu_cant.translated();
        cmenu.addentry( climb_affordance_menu_encode( aid->id ), enable_aid, hotkey,
                        string_format( text_translated, target_disp_name ) );
    }
}

bool game::climb_down_menu_pick( const tripoint &examp, int retval )
{
    climbing_aid_id aid_id = climbing_aid_default;

    if( climb_affordance_menu_decode( retval, aid_id ) ) {
        climb_down_using( examp, aid_id );
        return true;
    } else {
        return false;
    }
}

void game::climb_down( const tripoint &examp )
{
    uilist cmenu;
    cmenu.text = _( "How would you prefer to climb down?" );

    climb_down_menu_gen( examp, cmenu );

    // If there would only be one choice, skip the menu.
    if( cmenu.entries.size() == 1 ) {
        climb_down_menu_pick( examp, cmenu.entries[0].retval );
    } else {
        cmenu.query();
        climb_down_menu_pick( examp, cmenu.ret );
    }
}

void game::climb_down_using( const tripoint &examp, climbing_aid_id aid_id, bool deploy_affordance )
{
    const climbing_aid &aid = aid_id.obj();

    map &here = get_map();
    Character &you = get_player_character();

    // If player is grabbed, trapped, or somehow otherwise movement-impeded, first try to break free
    if( !you.move_effects( false ) ) {
        you.moves -= 100;
        return;
    }

    if( !here.valid_move( you.pos(), examp, false, true ) ) {
        // Can't move horizontally to the ledge
        return;
    }

    // Scan the height of the drop and what's in the way.
    const climbing_aid::fall_scan fall( examp );

    int estimated_climb_cost = you.climbing_cost( fall.pos_bottom(), examp );
    const float fall_mod = you.fall_damage_mod();
    add_msg_debug( debugmode::DF_IEXAMINE, "Climb cost %d", estimated_climb_cost );
    add_msg_debug( debugmode::DF_IEXAMINE, "Fall damage modifier %.2f", fall_mod );

    std::string query;

    // The most common query reads as follows:
    //   It [seems somewhat risky] to climb down like this.
    //   Falling [would hurt].
    //   You [probably won't be able to climb back up].
    //   Climb down the rope?

    // Calculate chance of slipping.  Prints possible causes to log.
    int slip_chance = slip_down_chance( climb_maneuver::down, aid_id, true );

    // Roughly estimate damage if we should fall.
    int damage_estimate = 10 * fall.height;
    if( damage_estimate <= 30 ) {
        damage_estimate *= fall_mod;
    } else {
        damage_estimate *= std::pow( fall_mod, 30.f / damage_estimate );
    }

    // Rough messaging about safety.  "seems safe" can leave a 1-2% chance unlike "perfectly safe".
    bool seems_perfectly_safe = slip_chance < -5 && aid.down.max_height >= fall.height;
    if( seems_perfectly_safe ) {
        query = _( "It <color_green>seems perfectly safe</color> to climb down like this." );
    } else if( slip_chance < 3 ) {
        query = _( "It <color_green>seems safe</color> to climb down like this." );
    } else if( slip_chance < 8 ) {
        query = _( "It <color_yellow>seems a bit tricky</color> to climb down like this." );
    } else if( slip_chance < 20 ) {
        query = _( "It <color_yellow>seems somewhat risky</color> to climb down like this." );
    } else if( slip_chance < 50 ) {
        query = _( "It <color_red>seems very risky</color> to climb down like this." );
    } else if( slip_chance < 80 ) {
        query = _( "It <color_pink>looks like you'll slip</color> if you climb down like this." );
    } else {
        query = _( "It <color_pink>doesn't seem possible to climb down safely</color>." );
    }

    if( !seems_perfectly_safe ) {
        std::string hint_fall_damage;
        if( damage_estimate >= 100 ) {
            hint_fall_damage = _( "Falling <color_pink>would kill you</color>." );
        } else if( damage_estimate >= 60 ) {
            hint_fall_damage = _( "Falling <color_pink>could cripple or kill you</color>." );
        } else if( damage_estimate >= 30 ) {
            hint_fall_damage = _( "Falling <color_pink>would break bones.</color>." );
        } else if( damage_estimate >= 15 ) {
            hint_fall_damage = _( "Falling <color_red>would hurt badly</color>." );
        } else if( damage_estimate >= 5 ) {
            hint_fall_damage = _( "Falling <color_red>would hurt</color>." );
        } else {
            hint_fall_damage = _( "Falling <color_green>wouldn't hurt much</color>." );
        }
        query += "\n";
        query += hint_fall_damage;
    }

    if( fall.height > aid.down.max_height ) {
        // Warn the player that they will fall even after a successful climb
        int remaining_height = fall.height - aid.down.max_height;
        query += "\n";
        query += string_format( n_gettext(
                                    "Even if you climb down safely, you will fall <color_yellow>at least %d story</color>.",
                                    "Even if you climb down safely, you will fall <color_red>at least %d stories</color>.",
                                    remaining_height ), remaining_height );
    }

    // Certain climbing aids make it easy to climb back up, usually by making furniture.
    if( aid.down.easy_climb_back_up >= fall.height ) {
        estimated_climb_cost = 50;
    }

    // Note, this easy_climb_back_up can be set by factors other than the climbing aid.
    bool easy_climb_back_up = false;
    std::string hint_climb_back;
    if( estimated_climb_cost <= 0 ) {
        hint_climb_back = _( "You <color_red>probably won't be able to climb back up</color>." );
    } else if( estimated_climb_cost < 200 ) {
        hint_climb_back = _( "You <color_green>should be easily able to climb back up</color>." );
        easy_climb_back_up = true;
    } else {
        hint_climb_back = _( "You <color_yellow>may have problems trying to climb back up</color>." );
    }
    query += "\n";
    query += hint_climb_back;

    std::string query_prompt = _( "Climb down?" );
    if( !aid.down.confirm_text.empty() ) {
        query_prompt = aid.down.confirm_text.translated();
    }
    query += "\n\n";
    query += query_prompt;

    add_msg_debug( debugmode::DF_GAME, "Generated climb_down prompt for the player." );
    add_msg_debug( debugmode::DF_GAME, "Climbing aid: %s / deploy furniture %d", std::string( aid_id ),
                   int( deploy_affordance ) );
    add_msg_debug( debugmode::DF_GAME, "Slip chance %d / est damage %d", slip_chance, damage_estimate );
    add_msg_debug( debugmode::DF_GAME, "We can descend %d / total height %d", aid.down.max_height,
                   fall.height );

    if( !seems_perfectly_safe || !easy_climb_back_up ) {

        // This is used to mention object names.  TODO make this more flexible.
        std::string target_disp_name = m.disp_name( fall.pos_furniture_or_floor() );

        // Show the risk prompt.
        if( !query_yn( query.c_str(), target_disp_name ) ) {
            return;
        }
    }

    you.set_activity_level( ACTIVE_EXERCISE );
    float weary_mult = 1.0f / you.exertion_adjusted_move_multiplier( ACTIVE_EXERCISE );

    you.moves -= to_moves<int>( 1_seconds + 1_seconds * fall_mod ) * weary_mult;
    you.setpos( examp );

    // Pre-descent message.
    if( !aid.down.msg_before.empty() ) {
        you.add_msg_if_player( aid.down.msg_before.translated() );
    }

    // Descent: perform one slip check per level
    tripoint descent_pos = examp;
    for( int i = 0; i < fall.height && i < aid.down.max_height; ++i ) {
        if( g->slip_down( climb_maneuver::down, aid_id, false ) ) {
            // The player has slipped and probably fallen.
            return;
        } else {
            descent_pos.z--;
            if( aid.down.deploys_furniture() ) {
                here.furn_set( descent_pos, aid.down.deploy_furn );
            }
        }
    }

    int descended_levels = examp.z - descent_pos.z;
    add_msg_debug( debugmode::DF_IEXAMINE, "Safe movement down %d Z-levels", descended_levels );

    // Post-descent logic...

    // Use up items after successful climb
    if( aid.base_condition.cat == climbing_aid::category::item && aid.base_condition.uses_item > 0 ) {
        for( item &used_item : you.use_amount( itype_id( aid.base_condition.flag ),
                                               aid.base_condition.uses_item ) ) {
            used_item.spill_contents( you );
        }
    }

    // Pre-descent message.
    if( !aid.down.msg_after.empty() ) {
        you.add_msg_if_player( aid.down.msg_after.translated() );
    }

    // You ride the ride, you pay the tithe.
    if( aid.down.cost.damage > 0 ) {
        you.apply_damage( nullptr, bodypart_id( "torso" ), aid.down.cost.damage );
    }
    if( aid.down.cost.pain > 0 ) {
        you.mod_pain( aid.down.cost.pain );
    }
    if( aid.down.cost.kcal > 0 ) {
        you.mod_stored_kcal( -aid.down.cost.kcal );
    }
    if( aid.down.cost.thirst > 0 ) {
        you.mod_thirst( aid.down.cost.thirst );
    }

    // vertical_move with force=true triggers traps (ie, fall) at the end of the move.
    g->vertical_move( -descended_levels, true );

    if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, you.pos() ) ) {
        you.set_underwater( true );
        g->water_affect_items( you );
        you.add_msg_if_player( _( "You climb down and dive underwater." ) );
    }
}

namespace cata_event_dispatch
{
void avatar_moves( const tripoint &old_abs_pos, const avatar &u, const map &m )
{
    const tripoint new_pos = u.pos();
    const tripoint new_abs_pos = m.getabs( new_pos );
    mtype_id mount_type;
    if( u.is_mounted() ) {
        mount_type = u.mounted_creature->type->id;
    }
    get_event_bus().send<event_type::avatar_moves>( mount_type, m.ter( new_pos ).id(),
            u.current_movement_mode(), u.is_underwater(), new_pos.z );

    // TODO: fix point types
    const tripoint_abs_omt old_abs_omt( ms_to_omt_copy( old_abs_pos ) );
    const tripoint_abs_omt new_abs_omt( ms_to_omt_copy( new_abs_pos ) );
    if( old_abs_omt != new_abs_omt ) {
        const oter_id &cur_ter = overmap_buffer.ter( new_abs_omt );
        const oter_id &past_ter = overmap_buffer.ter( old_abs_omt );
        get_event_bus().send<event_type::avatar_enters_omt>( new_abs_omt.raw(), cur_ter );
        // if the player has moved omt then might trigger an EOC for that OMT
        effect_on_conditions::om_move();
        if( !past_ter->get_exit_EOC().is_null() ) {
            dialogue d( get_talker_for( get_avatar() ), nullptr );
            effect_on_condition_id eoc = cur_ter->get_exit_EOC();
            if( eoc->type == eoc_type::ACTIVATION ) {
                eoc->activate( d );
            } else {
                debugmsg( "Must use an activation eoc for OMT movement.  Otherwise, create a non-recurring effect_on_condition for this with its condition and effects, then have a recurring one queue it." );
            }
        }

        if( !cur_ter->get_entry_EOC().is_null() ) {
            dialogue d( get_talker_for( get_avatar() ), nullptr );
            effect_on_condition_id eoc = cur_ter->get_exit_EOC();
            if( eoc->type == eoc_type::ACTIVATION ) {
                eoc->activate( d );
            } else {
                debugmsg( "Must use an activation eoc for OMT movement.  Otherwise, create a non-recurring effect_on_condition for this with its condition and effects, then have a recurring one queue it." );
            }
        }

    }
}
} // namespace cata_event_dispatch

achievements_tracker &get_achievements()
{
    return g->achievements();
}

Character &get_player_character()
{
    return g->u;
}

viewer &get_player_view()
{
    return g->u;
}

avatar &get_avatar()
{
    return g->u;
}

map &get_map()
{
    return g->m;
}

creature_tracker &get_creature_tracker()
{
    return *g->critter_tracker;
}

event_bus &get_event_bus()
{
    return g->events();
}

memorial_logger &get_memorial()
{
    return g->memorial();
}

const scenario *get_scenario()
{
    return g->scen;
}
void set_scenario( const scenario *new_scenario )
{
    g->scen = new_scenario;
}

scent_map &get_scent()
{
    return g->scent;
}

stats_tracker &get_stats()
{
    return g->stats();
}

timed_event_manager &get_timed_events()
{
    return g->timed_events;
}

weather_manager &get_weather()
{
    return g->weather;
}

global_variables &get_globals()
{
    return g->global_variables_instance;
}
