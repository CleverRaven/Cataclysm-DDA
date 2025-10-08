#include "game.h"
#include "map_memory.h"

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
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
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
#include "ascii_art.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "avatar_action.h"
#include "basecamp.h"
#include "bionics.h"
#include "body_part_set.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "butchery_requirements.h"
#include "butchery.h"
#include "cached_options.h"
#include "cata_imgui.h"
#include "cata_path.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "catacharset.h"
#include "character.h"
#include "character_attire.h"
#include "character_martial_arts.h"
#include "char_validity_check.h"
#include "city.h"
#include "climbing.h"
#include "clzones.h"
#include "colony.h"
#include "color.h"
#include "condition.h"
#include "computer.h"
#include "computer_session.h"
#include "construction.h"
#include "construction_group.h"
#include "contents_change_handler.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "current_map.h"
#include "cursesport.h" // IWYU pragma: keep
#include "damage.h"
#include "debug.h"
#include "debug_menu.h"
#include "dependency_tree.h"
#include "dialogue.h"
#include "dialogue_chatbin.h"
#include "diary.h"
#include "distraction_manager.h"
#include "editmap.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "end_screen.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "fault.h"
#include "field.h"
#include "field_type.h"
#include "filesystem.h"
#include "flag.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "flood_fill.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "game_ui.h"
#include "gamemode.h"
#include "gates.h"
#include "get_version.h"
#include "harvest.h"
#include "iexamine.h"
#include "imgui/imgui_stdlib.h"
#include "init.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "inventory.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
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
#include "magic.h"
#include "magic_enchantment.h"
#include "main_menu.h"
#include "map.h"
#include "map_item_stack.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapsharing.h"
#include "maptile_fwd.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "monexamine.h"
#include "mongroup.h"
#include "monster.h"
#include "monstergenerator.h"
#include "move_mode.h"
#include "mtype.h"
#include "npc.h"
#include "npctrade.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "panels.h"
#include "past_achievements_info.h"
#include "path_info.h"
#include "pathfinding.h"
#include "perf.h"
#include "pickup.h"
#include "player_activity.h"
#include "popup.h"
#include "profession.h"
#include "proficiency.h"
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
#include "translation.h"
#include "translation_cache.h"
#include "translations.h"
#include "trap.h"
#include "ui_helpers.h"
#include "ui_extended_description.h"
#include "ui_manager.h"
#include "uilist.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_appliance.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "viewer.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "wcwidth.h"
#include "weakpoint.h"
#include "weather.h"
#include "weather_type.h"
#include "worldfactory.h"
#include "zzip.h"

#if defined(TILES)
#include "sdl_utils.h"
#endif // TILES

static const activity_id ACT_TRAIN( "ACT_TRAIN" );
static const activity_id ACT_TRAIN_TEACHER( "ACT_TRAIN_TEACHER" );
static const activity_id ACT_TRAVELLING( "ACT_TRAVELLING" );

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
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_adrenaline_mycus( "adrenaline_mycus" );
static const efftype_id effect_asked_to_train( "asked_to_train" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_contacts( "contacts" );
static const efftype_id effect_cramped_space( "cramped_space" );
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

static const flag_id json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const flag_id json_flag_CONVECTS_TEMPERATURE( "CONVECTS_TEMPERATURE" );
static const flag_id json_flag_LEVITATION( "LEVITATION" );
static const flag_id json_flag_SPLINT( "SPLINT" );
static const flag_id json_flag_WATERWALKING( "WATERWALKING" );

static const furn_str_id furn_f_rope_up( "f_rope_up" );
static const furn_str_id furn_f_vine_up( "f_vine_up" );
static const furn_str_id furn_f_web_up( "f_web_up" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_grapnel( "grapnel" );
static const itype_id itype_manhole_cover( "manhole_cover" );
static const itype_id itype_remotevehcontrol( "remotevehcontrol" );
static const itype_id itype_rope_30( "rope_30" );
static const itype_id itype_swim_fins( "swim_fins" );
static const itype_id itype_towel( "towel" );
static const itype_id itype_towel_wet( "towel_wet" );

static const json_character_flag json_flag_ALL_TERRAIN_NAVIGATION( "ALL_TERRAIN_NAVIGATION" );
static const json_character_flag json_flag_CLIMB_FLYING( "CLIMB_FLYING" );
static const json_character_flag json_flag_CLIMB_NO_LADDER( "CLIMB_NO_LADDER" );
static const json_character_flag json_flag_ENHANCED_VISION( "ENHANCED_VISION" );
static const json_character_flag json_flag_GRAB( "GRAB" );
static const json_character_flag json_flag_HYPEROPIC( "HYPEROPIC" );
static const json_character_flag json_flag_INFECTION_IMMUNE( "INFECTION_IMMUNE" );
static const json_character_flag json_flag_ITEM_WATERPROOFING( "ITEM_WATERPROOFING" );
static const json_character_flag json_flag_NYCTOPHOBIA( "NYCTOPHOBIA" );
static const json_character_flag json_flag_PHASE_MOVEMENT( "PHASE_MOVEMENT" );
static const json_character_flag json_flag_VINE_RAPPEL( "VINE_RAPPEL" );
static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );
static const json_character_flag json_flag_WEB_RAPPEL( "WEB_RAPPEL" );

static const mod_id MOD_INFORMATION_dda( "dda" );

static const mongroup_id GROUP_BLACK_ROAD( "GROUP_BLACK_ROAD" );

static const mtype_id mon_manhack( "mon_manhack" );

static const npc_class_id NC_DOCTOR( "NC_DOCTOR" );
static const npc_class_id NC_HALLU( "NC_HALLU" );

static const overmap_special_id overmap_special_world( "world" );

static const proficiency_id proficiency_prof_parkour( "prof_parkour" );
static const proficiency_id proficiency_prof_wound_care( "prof_wound_care" );
static const proficiency_id proficiency_prof_wound_care_expert( "prof_wound_care_expert" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_PRY( "PRY" );

static const skill_id skill_dodge( "dodge" );
static const skill_id skill_firstaid( "firstaid" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_swimming( "swimming" );

static const species_id species_PLANT( "PLANT" );

static const string_id<npc_template> npc_template_cyborg_rescued( "cyborg_rescued" );

static const ter_str_id ter_t_elevator( "t_elevator" );
static const ter_str_id ter_t_lava( "t_lava" );
static const ter_str_id ter_t_manhole( "t_manhole" );
static const ter_str_id ter_t_manhole_cover( "t_manhole_cover" );

static const trait_id trait_BADKNEES( "BADKNEES" );
static const trait_id trait_CENOBITE( "CENOBITE" );
static const trait_id trait_ILLITERATE( "ILLITERATE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_MASOCHIST( "MASOCHIST" );
static const trait_id trait_MASOCHIST_MED( "MASOCHIST_MED" );
static const trait_id trait_M_DEFENDER( "M_DEFENDER" );
static const trait_id trait_M_IMMUNE( "M_IMMUNE" );
static const trait_id trait_NPC_STARTING_NPC( "NPC_STARTING_NPC" );
static const trait_id trait_NPC_STATIC_NPC( "NPC_STATIC_NPC" );
static const trait_id trait_PROF_CHURL( "PROF_CHURL" );
static const trait_id trait_THICKSKIN( "THICKSKIN" );
static const trait_id trait_VINES2( "VINES2" );
static const trait_id trait_VINES3( "VINES3" );
static const trait_id trait_WAYFARER( "WAYFARER" );

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

bool is_valid_in_w_terrain( const point_rel_ms &p )
{
    return p.x() >= 0 && p.x() < TERRAIN_WINDOW_WIDTH && p.y() >= 0 && p.y() < TERRAIN_WINDOW_HEIGHT;
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
            show_popup = !get_past_achievements().is_completed( a->id );
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
    current_map( *current_map_ptr ),
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
    current_map.set( &m );
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

bool game::check_mod_data( const std::vector<mod_id> &opts )
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
            load_core_data();
            DynamicDataLoader::get_instance().finalize_loaded_data();
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
            load_core_data();

            // Load any dependencies and de-duplicate them
            std::vector<mod_id> dep_vector = tree.get_dependencies_of_X_as_strings( mod.ident );
            std::set<mod_id> dep_set( dep_vector.begin(), dep_vector.end() );
            for( const auto &dep : dep_set ) {
                load_data_from_dir( dep->path, dep->ident.str() );
            }

            // Load mod itself
            load_data_from_dir( mod.path, mod.ident.str() );
            DynamicDataLoader::get_instance().finalize_loaded_data();
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

void game::load_core_data()
{
    // core data can be loaded only once and must be first
    // anyway.
    DynamicDataLoader::get_instance().unload_data();

    load_data_from_dir( PATH_INFO::jsondir(), "core" );
}

void game::load_data_from_dir( const cata_path &path, const std::string &src )
{
    DynamicDataLoader::get_instance().load_data_from_path( path, src );
}

void game::load_mod_data_from_dir( const cata_path &path, const std::string &src )
{
    DynamicDataLoader::get_instance().load_mod_data_from_path( path, src );
}
void game::load_mod_interaction_data_from_dir( const cata_path &path, const std::string &src )
{
    DynamicDataLoader::get_instance().load_mod_interaction_files_from_path( path, src );
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

/*
 * Initialize more stuff after mapbuffer is loaded.
 */
void game::setup()
{
    new_game = true;
    {
        static_popup popup;
        popup.message( "%s", _( "Please wait while the world data loads…\nLoading core data" ) );
        ui_manager::redraw();
        refresh_display();

        load_core_data();
    }

    // invalidate calendar caches in case we were previously playing
    // a different world
    calendar::set_eternal_season( ::get_option<bool>( "ETERNAL_SEASON" ) );
    // must be called before load_world_modfiles() #81904
    calendar::set_season_length( ::get_option<int>( "SEASON_LENGTH" ) );

    world_generator->get_mod_manager().check_mods_list( world_generator->active_world );
    load_world_modfiles();
    // Panel manager needs JSON data to be loaded before init
    panel_manager::get_manager().init();

    m = map();

    next_npc_id = character_id( 1 );
    next_mission_id = 1;
    uquit = QUIT_NO;   // We haven't quit the game
    bVMonsterLookFire = true;

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
    map &here = get_map();

    here.load( pos_sm, true, pump_events );
}

void game::legacy_migrate_npctalk_var_prefix( global_variables::impl_t &map_of_vars )
{
    // migrate existing variables with npctalk_var prefix to no prefix (npctalk_var_foo to just foo)
    // remove after 0.J

    if( savegame_loading_version >= 36 ) {
        return;
    }

    const std::string prefix = "npctalk_var_";
    for( auto i = map_of_vars.begin(); i != map_of_vars.end(); ) {
        if( i->first.rfind( prefix, 0 ) == 0 ) {
            auto extracted =  map_of_vars.extract( i++ );
            std::string new_key = extracted.key().substr( prefix.size() );
            extracted.key() = new_key;
            map_of_vars.insert( std::move( extracted ) );
        } else {
            ++i;
        }
    }
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
    overmap_buffer.current_region_type = "default";
    u.setID( assign_npc_id() ); // should be as soon as possible, but *after* load_master

    // Make sure the items are added after the calendar is started
    u.add_profession_items();
    // Move items from the raw inventory to item_location s. See header TODO.
    u.migrate_items_to_storage( true );

    const start_location &start_loc = u.random_start_location ? scen->random_start_location().obj() :
                                      u.start_location.obj();
    tripoint_abs_omt omtstart = tripoint_abs_omt::invalid;
    std::unordered_map<std::string, std::string> associated_parameters;
    const bool select_starting_city = get_option<bool>( "SELECT_STARTING_CITY" );
    do {
        if( select_starting_city ) {
            if( !u.starting_city.has_value() ) {
                u.starting_city = random_entry( city::get_all() );
                u.world_origin = u.starting_city->pos_om;
            }
            auto ret = start_loc.find_player_initial_location( u.starting_city.value() );
            omtstart = ret.first;
            associated_parameters = ret.second;
        } else {
            auto ret = start_loc.find_player_initial_location( u.world_origin.value_or( point_abs_om() ) );
            omtstart = ret.first;
            associated_parameters = ret.second;
        }
        if( omtstart.is_invalid() ) {

            MAPBUFFER.clear();
            overmap_buffer.clear();

            if( !query_yn(
                    _( "Try again?\n\nIt may require several attempts until the game finds a valid starting location." ) ) ) {
                return false;
            }
        }
    } while( omtstart.is_invalid() );

    // Set parameter(s) if specified in chosen start_loc
    start_loc.set_parameters( omtstart, associated_parameters );

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

    start_loc.place_player( u, omtstart );
    map &here = reality_bubble();
    int level = here.get_abs_sub().z();
    // Rebuild map cache because we want visibility cache to avoid spawning monsters in sight
    here.invalidate_map_cache( level );
    here.build_map_cache( level );
    // Start the overmap with out immediate neighborhood visible, this needs to be after place_player
    overmap_buffer.reveal( u.pos_abs_omt().xy(),
                           get_scenario()->get_distance_initial_visibility(), 0 );

    const int city_size = get_option<int>( "CITY_SIZE" );
    if( get_scenario()->get_reveal_locale() && city_size > 0 ) {
        city_reference nearest_city = overmap_buffer.closest_city( here.get_abs_sub() );
        const tripoint_abs_omt city_center_omt = project_to<coords::omt>( nearest_city.abs_sm_pos );
        // Very necessary little hack: We look for roads around our start, and path from the closest. Because the most common start(evac shelter) cannot be pathed through...
        const tripoint_abs_omt nearest_road = overmap_buffer.find_closest( omtstart, "road", 3, false );
        // Reveal route to closest city and a 3 tile radius around the route
        overmap_buffer.reveal_route( nearest_road, city_center_omt, 3 );
        // Reveal destination city (scaling with city size setting)
        overmap_buffer.reveal( city_center_omt, city_size );
    }

    u.set_moves( 0 );
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

    here.spawn_monsters( !spawn_near ); // Static monsters

    // Make sure that no monsters are near the player
    // This can happen in lab starts
    if( !spawn_near ) {
        for( monster &critter : all_monsters() ) {
            if( rl_dist( critter.pos_bub(), u.pos_bub() ) <= 5 ||
                here.clear_path( critter.pos_bub(), u.pos_bub(), 40, 1, 100 ) ) {
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
        for( wrapped_vehicle v : here.get_vehicles() ) {
            std::string name = v.v->type.str();
            std::string search = std::string( "helicopter" );
            if( name.find( search ) != std::string::npos ) {
                for( const vpart_reference &vp : v.v->get_any_parts( VPFLAG_CONTROLS ) ) {
                    const tripoint_abs_ms pos = vp.pos_abs();
                    u.setpos( pos );

                    // Delete the items that would have spawned here from a "corpse"
                    for( const int sp : v.v->parts_at_relative( vp.mount_pos(), true ) ) {
                        vpart_reference( *v.v, sp ).items().clear();
                    }

                    auto mons = critter_tracker->find( u.pos_abs() );
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
        const point_abs_omt p_player = get_player_character().pos_abs_omt().xy();
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
        if( monster *const mon = place_critter_around( elem, u.pos_bub(), 5 ) ) {
            mon->friendly = -1;
            mon->add_effect( effect_pet, 1_turns, true );
        } else {
            add_msg_debug( debugmode::DF_GAME, "cannot place starting pet, no space!" );
        }
    }
    if( u.starting_vehicle &&
        !place_vehicle_nearby( u.starting_vehicle, u.pos_abs_omt().xy(), 1, 30,
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
            u.getID(), u.name, u.custom_profession );
    time_played_at_last_load = std::chrono::seconds( 0 );
    time_of_last_load = std::chrono::steady_clock::now();
    tripoint_abs_omt abs_omt = u.pos_abs_omt();
    const oter_id &cur_ter = overmap_buffer.ter( abs_omt );
    get_event_bus().send<event_type::avatar_enters_omt>( abs_omt.raw(), cur_ter );

    effect_on_conditions::load_new_character( u );
    if( debug_menu::is_debug_character() ) {
        debug_menu::do_debug_quick_setup();
        add_msg( m_good, _( "Debug character detected.  Quick setup applied, good luck on the testing!" ) );
    }
    return true;
}

vehicle *game::place_vehicle_nearby(
    const vproto_id &id, const point_abs_omt &origin, int min_distance,
    int max_distance, const std::vector<std::string> &omt_search_types )
{
    map &here = get_map();

    std::vector<std::string> search_types = omt_search_types;
    if( search_types.empty() ) {
        const vehicle &veh = *id->blueprint;
        if( veh.max_ground_velocity( here ) == 0 && veh.can_float( here ) ) {
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
            target_map.load( goal, false );
            // Redundant as long as map operations aren't using get_map() in a transitive call chain. Added for future proofing.
            swap_map swap( *target_map.cast_to_map() );
            const tripoint_omt_ms tinymap_center( SEEX, SEEY, goal.z() );
            static constexpr std::array<units::angle, 4> angles = {{
                    0_degrees, 90_degrees, 180_degrees, 270_degrees
                }
            };
            vehicle *veh = target_map.add_vehicle( id, tinymap_center, random_entry( angles ),
                                                   rng( 50, 80 ), 0, false );
            if( veh ) {
                const tripoint_abs_ms abs_local = target_map.get_abs( tinymap_center );
                tripoint_abs_sm quotient;
                point_sm_ms remainder;
                std::tie( quotient, remainder ) = coords::project_remain<coords::sm>( abs_local );
                veh->sm_pos = quotient;
                veh->pos = remainder;

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
    map &here = get_map();
    const int radius = HALF_MAPSIZE - 1;
    const tripoint_abs_sm abs_sub( here.get_abs_sub() );
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

        const tripoint_abs_sm sm_loc = temp->pos_abs_sm();
        // NPCs who are out of bounds before placement would be pushed into bounds
        // This can cause NPCs to teleport around, so we don't want that
        if( !map_bounds.contains( sm_loc.xy() ) ) {
            continue;
        }

        add_msg_debug( debugmode::DF_NPC, "game::load_npcs: Spawning static NPC, %s %s",
                       abs_sub.to_string_writable(), sm_loc.to_string_writable() );
        temp->place_on_map( &here );
        if( !reality_bubble().inbounds( temp->pos_bub() ) ) {
            continue;
        }
        // In the rare case the npc was marked for death while
        // it was on the overmap. Kill it.
        if( temp->marked_for_death ) {
            temp->die( &here, nullptr );
        } else {
            critter_tracker->active_npc.push_back( temp );
            just_added.push_back( temp );
        }
    }

    for( const auto &npc : just_added ) {
        npc->on_load( &here );
    }

    npcs_dirty = false;
}

void game::load_npcs( map *here )
{
    const int mapsize = here->getmapsize();
    const int radius = mapsize / 2 - 1;
    const tripoint_abs_sm abs_sub( here->get_abs_sub() );
    const tripoint_abs_sm center = abs_sub + point_rel_sm{radius, radius};
    const half_open_rectangle<point_abs_sm> map_bounds( abs_sub.xy(), abs_sub.xy() + point( mapsize,
            mapsize ) );
    // uses submap coordinates
    std::vector<shared_ptr_fast<npc>> just_added;
    for( const auto &temp : overmap_buffer.get_npcs_near( center, radius ) ) {
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

        const tripoint_abs_sm sm_loc = temp->pos_abs_sm();
        // NPCs who are out of bounds before placement would be pushed into bounds
        // This can cause NPCs to teleport around, so we don't want that
        if( !map_bounds.contains( sm_loc.xy() ) ) {
            continue;
        }

        add_msg_debug( debugmode::DF_NPC, "game::load_npcs: Spawning static NPC, %s %s",
                       abs_sub.to_string_writable(), sm_loc.to_string_writable() );
        temp->place_on_map( here );
        if( !here->inbounds( temp->pos_abs() ) ) {
            continue;
        }
        // In the rare case the npc was marked for death while
        // it was on the overmap. Kill it.
        if( temp->marked_for_death ) {
            temp->die( here, nullptr );
        } else {
            critter_tracker->active_npc.push_back( temp );
            just_added.push_back( temp );
        }
    }

    for( const auto &npc : just_added ) {
        npc->on_load( here );
    }

    npcs_dirty = false;
}

void game::on_witness_theft( const item &target )
{
    const map &here = get_map();

    Character &p = get_player_character();
    std::vector<npc *> witnesses;
    for( npc &elem : g->all_npcs() ) {
        if( rl_dist( elem.pos_bub(), p.pos_bub() ) < MAX_VIEW_DISTANCE &&
            elem.sees( here, p.pos_bub( here ) ) &&
            target.is_owned_by( elem ) ) {
            witnesses.push_back( &elem );
        }
    }
    for( npc *elem : witnesses ) {
        elem->say( "<witnessed_thievery>", 7 );
    }
    if( !witnesses.empty() ) {
        if( p.add_faction_warning( target.get_owner() ) ||
            target.get_owner() == faction_no_faction ) {
            for( npc *elem : witnesses ) {
                elem->make_angry();
            }
        }
    }
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
    tmp->randomize( one_in( 2 ) ? NC_DOCTOR : npc_class_id::NULL_ID() );
    // hardcoded, consistent NPC position
    // start_loc::place_player relies on this and must be updated if this is changed
    tmp->spawn_at_precise( u.pos_abs() + point::north_west );
    overmap_buffer.insert_npc( tmp );
    tmp->form_opinion( u );
    tmp->set_attitude( NPCATT_NULL );
    //This sets the NPC mission. This NPC remains in the starting location.
    tmp->mission = NPC_MISSION_SHELTER;
    tmp->chatbin.first_topic = "TALK_SHELTER";
    tmp->toggle_trait( trait_NPC_STARTING_NPC );
    tmp->set_fac( faction_no_faction );
    //One random starting NPC mission
    tmp->add_new_mission( mission::reserve_random( ORIGIN_OPENER_NPC, tmp->pos_abs_omt(),
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
    map &here = get_map();

    if( veh == nullptr || !get_option<bool>( "DRIVING_VIEW_OFFSET" ) ) {
        set_driving_view_offset( point_rel_ms::zero );
        return;
    }
    const int g_light_level = static_cast<int>( light_level( u.posz() ) );
    const int light_sight_range = u.sight_range( g_light_level );
    int sight = std::max( veh_lumi( *veh ), light_sight_range );

    // The maximal offset will leave at least this many tiles
    // between the PC and the edge of the main window.
    static const int border_range = 2;
    point_rel_ms max_offset( ( getmaxx( w_terrain ) + 1 ) / 2 - border_range - 1,
                             ( getmaxy( w_terrain ) + 1 ) / 2 - border_range - 1 );

    // velocity at or below this results in no offset at all
    static const float min_offset_vel = 1 * vehicles::vmiph_per_tile;
    // velocity at or above this results in maximal offset
    const float max_offset_vel = std::min( max_offset.y(), max_offset.x() ) *
                                 vehicles::vmiph_per_tile;
    float velocity = veh->velocity;
    rl_vec2d offset = veh->move_vec();
    if( !veh->skidding && veh->player_in_control( here,  u ) &&
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
    offset.x *= max_offset.x();
    offset.y *= max_offset.y();
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
    const point_rel_ms maxoff( ( sight * 2 + 1 - getmaxx( w_terrain ) ) / 2,
                               ( sight * 2 + 1 - getmaxy( w_terrain ) ) / 2 );
    if( maxoff.x() <= 0 ) {
        offset.x = 0;
    } else if( offset.x > 0 && offset.x > maxoff.x() ) {
        offset.x = maxoff.x();
    } else if( offset.x < 0 && -offset.x > maxoff.x() ) {
        offset.x = -maxoff.x();
    }
    if( maxoff.y() <= 0 ) {
        offset.y = 0;
    } else if( offset.y > 0 && offset.y > maxoff.y() ) {
        offset.y = maxoff.y();
    } else if( offset.y < 0 && -offset.y > maxoff.y() ) {
        offset.y = -maxoff.y();
    }

    // Turn the offset into a vector that increments the offset toward the desired position
    // instead of setting it there instantly, should smooth out jerkiness.
    const point_rel_ms offset_difference( -driving_view_offset + point( offset.x, offset.y ) );

    const point_rel_ms offset_sign( ( offset_difference.x() < 0 ) ? -1 : 1,
                                    ( offset_difference.y() < 0 ) ? -1 : 1 );
    // Shift the current offset in the direction of the calculated offset by one tile
    // per draw event, but snap to calculated offset if we're close enough to avoid jitter.
    offset.x = ( std::abs( offset_difference.x() ) > 1 ) ?
               ( driving_view_offset.x() + offset_sign.x() ) : offset.x;
    offset.y = ( std::abs( offset_difference.y() ) > 1 ) ?
               ( driving_view_offset.y() + offset_sign.y() ) : offset.y;

    set_driving_view_offset( { offset.x, offset.y } );
}

void game::set_driving_view_offset( const point_rel_ms &p )
{
    // remove the previous driving offset,
    // store the new offset and apply the new offset.
    u.view_offset.raw() -=
        driving_view_offset.raw(); // TODO: Implement -= etc. for relative coordinates.
    driving_view_offset = p;
    u.view_offset.raw() +=
        driving_view_offset.raw(); // TODO: Implement -= etc. for relative coordinates.
}

void game::catch_a_monster( monster *fish, const tripoint_bub_ms &pos, Character *p,
                            const time_duration &catch_duration ) // catching function
{
    map &here = get_map();

    //spawn the corpse, rotten by a part of the duration
    here.add_item_or_charges( pos, item::make_corpse( fish->type->id, calendar::turn + rng( 0_turns,
                              catch_duration ) ) );
    if( u.sees( here, pos ) ) {
        u.add_msg_if_player( m_good, _( "You caught a %s." ), fish->type->nname() );
    }
    //quietly kill the caught
    fish->no_corpse_quiet = true;
    fish->die( &here, p );
}

static bool cancel_auto_move( Character &you, const std::string &text )
{
    if( !you.has_destination() ) {
        return false;
    }
    g->invalidate_main_ui_adaptor();
    if( query_yn( _( "%s Cancel auto move?" ), text ) )  {
        add_msg( m_warning, _( "%s Auto move canceled." ), text );
        you.abort_automove();
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
        u.abort_automove();
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

units::temperature_delta get_heat_radiation( const tripoint_bub_ms &location )
{
    units::temperature_delta temp_mod = units::from_kelvin_delta( 0 );
    Character &player_character = get_player_character();
    map &here = get_map();
    // Convert it to an int id once, instead of 139 times per turn
    const field_type_id fd_fire_int = fd_fire.id();
    for( const tripoint_bub_ms &dest : here.points_in_radius( location, 6 ) ) {
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
        if( player_character.pos_bub() == location ) {
            bool heat_can_spread = true;
            for( const tripoint_bub_ms &p : line_to( player_character.pos_bub(), dest ) ) {
                if( !here.has_flag( ter_furn_flag::TFLAG_PERMEABLE, p ) && here.impassable( p ) ) {
                    heat_can_spread = false;
                    break;
                }
            }
            if( !heat_can_spread ) {
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

int get_best_fire( const tripoint_bub_ms &location )
{
    int best_fire = 0;
    Character &player_character = get_player_character();
    map &here = get_map();
    // Convert it to an int id once, instead of 139 times per turn
    const field_type_id fd_fire_int = fd_fire.id();
    for( const tripoint_bub_ms &dest : here.points_in_radius( location, 6 ) ) {
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
        if( player_character.pos_bub() == location ) {
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

units::temperature_delta get_convection_temperature( const tripoint_bub_ms &location )
{
    units::temperature_delta temp_mod = units::from_kelvin_delta( 0 );
    map &here = get_map();
    // Directly on lava tiles
    units::temperature_delta lava_mod = here.tr_at( location ).has_flag(
                                            json_flag_CONVECTS_TEMPERATURE ) ?
                                        units::from_fahrenheit_delta( fd_fire->get_intensity_level().convection_temperature_mod ) :
                                        units::from_kelvin_delta( 0 );
    // Modifier from fields
    for( auto &fd : here.field_at( location ) ) {
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
    map &here = get_map();

    for( wrapped_vehicle &veh : here.get_vehicles() ) {
        vehicle *v = veh.v;
        if( v->tow_data.other_towing_point != tripoint_bub_ms::zero ) {
            vehicle *other_v = veh_pointer_or_null( here.veh_at( v->tow_data.other_towing_point ) );
            if( other_v ) {
                // the other vehicle is towing us.
                v->tow_data.set_towing( other_v, v );
                v->tow_data.other_towing_point = tripoint_bub_ms::zero;
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
            mounted_pl->setpos( m.pos_abs() );
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
    map &here = get_map();

    basecamp camp = here.hoist_submap_camp( u.pos_bub() );
    if( camp.is_valid() ) {
        overmap_buffer.add_camp( camp );
        here.remove_submap_camp( u.pos_bub() );
    } else if( camp.camp_omt_pos() != tripoint_abs_omt() ) {
        std::string camp_name = _( "Faction Camp" );
        camp.set_name( camp_name );
        overmap_buffer.add_camp( camp );
        here.remove_submap_camp( u.pos_bub() );
    }
}

std::set<character_id> game::get_follower_list()
{
    return follower_ids;
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
    ctxt.register_action( "throw_wielded" );
    ctxt.register_action( "fire" );
    ctxt.register_action( "cast_spell" );
    ctxt.register_action( "recast_spell" );
    ctxt.register_action( "fire_burst" );
    ctxt.register_action( "select_fire_mode" );
    ctxt.register_action( "select_default_ammo" );
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
    map &here = get_map();

    if( calendar::turn == remoteveh_cache_time ) {
        return remoteveh_cache;
    }
    remoteveh_cache_time = calendar::turn;
    diag_value const *remote_controlling_vehicle = u.maybe_get_value( "remote_controlling_vehicle" );
    if( !remote_controlling_vehicle ||
        ( !u.has_active_bionic( bio_remote ) && !u.has_active_item( itype_remotevehcontrol ) ) ) {
        remoteveh_cache = nullptr;
    } else {
        // FIXME: migrate to abs
        tripoint_bub_ms vp( remote_controlling_vehicle->tripoint().raw() );
        vehicle *veh = veh_pointer_or_null( here.veh_at( vp ) );
        if( veh && veh->fuel_left( here, itype_battery ) > 0 ) {
            remoteveh_cache = veh;
        } else {
            remoteveh_cache = nullptr;
        }
    }
    return remoteveh_cache;
}

void game::setremoteveh( vehicle *veh )
{
    map &here = get_map();

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

    // FIXME: migrate to abs
    u.set_value( "remote_controlling_vehicle", tripoint_abs_ms{ veh->pos_bub( here ).raw() } );
}

bool game::is_game_over()
{
    map &here = get_map();

    if( uquit == QUIT_DIED || uquit == QUIT_WATCH ) {
        Creature *player_killer = u.get_killer();
        if( player_killer && player_killer->as_character() ) {
            events().send<event_type::character_kills_character>(
                player_killer->as_character()->getID(), u.getID(), u.get_name() );
        }
        events().send<event_type::character_dies>( u.getID() );
        events().send<event_type::avatar_dies>();
    }
    if( uquit == QUIT_WATCH ) {
        // deny player movement and dodging
        u.set_moves( 0 );
        // prevent pain from updating
        u.set_pain( 0 );
        // prevent dodging
        u.set_dodges_left( 0 );
        return false;
    }
    if( uquit == QUIT_DIED ) {
        if( u.in_vehicle ) {
            here.unboard_vehicle( u.pos_bub() );
        }
        u.place_corpse( &here );
        return true;
    }
    if( uquit == QUIT_SUICIDE ) {
        bury_screen();
        if( u.in_vehicle ) {
            here.unboard_vehicle( u.pos_bub() );
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

// A timestamp that can be used to make unique file names
// Date format is a somewhat ISO-8601 compliant local time date (except we use '-' instead of ':' probably because of Windows file name rules).
// XXX: removed the timezone suffix due to an mxe bug
// See: https://github.com/mxe/mxe/issues/2749
std::string game::timestamp_now() const
{
    std::time_t time = std::time( nullptr );
    std::stringstream date_buffer;
    date_buffer << std::put_time( std::localtime( &time ), "%Y-%m-%dT%H-%M-%S" );
    return date_buffer.str();
}

void game::move_save_to_graveyard()
{
    const cata_path save_dir      = PATH_INFO::world_base_save_path();
    const cata_path graveyard_dir = PATH_INFO::graveyarddir_path() / timestamp_now();
    const std::string prefix      = base64_encode( u.get_save_id() ) + ".";

    if( !assure_dir_exist( graveyard_dir ) ) {
        debugmsg( "could not create graveyard path '%s'", graveyard_dir );
    }

    const auto save_files = get_files_from_path( prefix, save_dir );
    if( save_files.empty() ) {
        debugmsg( "could not find save files in '%s'", save_dir );
    }

    for( const cata_path &src_path : save_files ) {
        const cata_path dst_path = graveyard_dir / src_path.get_relative_path().filename();

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
    const cata_path datafile = PATH_INFO::world_base_save_path() / SAVE_MASTER;
    read_from_file_optional( datafile, [this, &datafile]( std::istream & is ) {
        unserialize_master( datafile, is );
    } );
}

bool game::load_dimension_data()
{
    const cata_path datafile = PATH_INFO::current_dimension_save_path() / SAVE_DIMENSION_DATA;
    // if for whatever reason the dimension data file doesn't have a set region_type, use the default one
    overmap_buffer.current_region_type = "default";
    // If dimension_data.gsav doesn't exist, return false
    return read_from_file_optional( datafile, [this, &datafile]( std::istream & is ) {
        unserialize_dimension_data( datafile, is );
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
    map &here = get_map();

    const cata_path worldpath = PATH_INFO::world_base_save_path();
    const cata_path save_file_path = PATH_INFO::world_base_save_path() /
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
                _( "Dimension data" ), [&]()
                {
                    // Load up dimension specific data (ie; weather, overmapstate)
                    load_dimension_data();
                }
            },
            {
                _( "Character save" ), [&]()
                {

                    u = avatar();
                    u.set_save_id( name.decoded_name() );

                    if( world_generator->active_world->has_compression_enabled() ) {
                        std::optional<zzip> z = zzip::load( ( save_file_path + ".zzip" ).get_unrelative_path() );
                        abort = !z.has_value() ||
                        !read_from_zzip_optional( z.value(), save_file_path.get_unrelative_path().filename(),
                        [this]( std::string_view sv ) {
                            unserialize( std::string{ sv } );
                        } );
                    } else {
                        abort = !read_from_file(
                                    save_file_path,
                        [this, &save_file_path]( std::istream & is ) {
                            unserialize( is, save_file_path );
                        } );
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
                        PATH_INFO::world_base_save_path() / "uistate.json",
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
                            if( const std::optional<vpart_reference> vp = here.veh_at(
                                        u.pos_bub() ).part_with_feature( "BOARDABLE", true ) ) {
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
                    here.build_map_cache( here.get_abs_sub().z() );
                }
            },
        }
    };

    for( const named_entry &e : entries ) {
        loading_ui::show( _( "Loading the save…" ), e.first );
        e.second();
        if( abort ) {
            loading_ui::done();
            return false;
        }
    }

    loading_ui::done();
    return true;
}

void game::load_world_modfiles()
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
    load_packs( _( "Loading files" ), mods );

    // Load additional mods from that world-specific folder
    load_mod_data_from_dir( PATH_INFO::world_base_save_path() / "mods", "custom" );
    load_mod_interaction_data_from_dir( PATH_INFO::world_base_save_path() / "mods" /
                                        "mod_interactions", "custom" );

    DynamicDataLoader::get_instance().finalize_loaded_data();
}

void game::load_packs( const std::string &msg, const std::vector<mod_id> &packs )
{
    for( const auto &mod : packs ) {
        // Suppress missing mods the player chose to leave in the modlist
        if( !mod.is_valid() ) {
            continue;
        }
        loading_ui::show( msg, mod->name() );
        restore_on_out_of_scope restore_check_plural( check_plural );
        if( mod.str() == "test_data" ) {
            check_plural = check_plural_t::none;
        }
        cata_timer pack_timer( string_format( "%s pack load time:", mod->name() ) );
        load_mod_data_from_dir( mod->path, mod.str() );
    }
    cata_timer::print_stats();

    for( const auto &mod : packs ) {
        if( !mod.is_valid() ) {
            continue;
        }
        loading_ui::show( msg, mod->name() );
        load_mod_interaction_data_from_dir( mod->path / "mod_interactions", mod.str() );
    }
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
                                     npc_to_add->pos_abs_omt(),
                                     npc_to_add->getID() ) );

    }

}

//Saves all factions and missions and npcs.
bool game::save_factions_missions_npcs()
{
    cata_path masterfile = PATH_INFO::world_base_save_path() / SAVE_MASTER;
    return write_to_file( masterfile, [&]( std::ostream & fout ) {
        serialize_master( fout );
    }, _( "factions data" ) );
}
//Saves per-dimension data like Weather and overmapbuffer state
bool game::save_dimension_data()
{
    cata_path data_file = PATH_INFO::current_dimension_save_path() / SAVE_DIMENSION_DATA;
    return write_to_file( data_file, [&]( std::ostream & fout ) {
        serialize_dimension_data( fout );
    }, _( "dimension data" ) );
}
bool game::save_maps()
{
    map &here = get_map();

    try {
        here.save();
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
    const cata_path playerfile = PATH_INFO::player_base_save_path();

    bool saved_data;
    if( world_generator->active_world->has_compression_enabled() ) {
        std::stringstream save;
        serialize_json( save );
        std::filesystem::path save_path = ( playerfile + SAVE_EXTENSION +
                                            ".zzip" ).get_unrelative_path();
        std::filesystem::path tmp_path = save_path;
        tmp_path.concat( ".tmp" ); // NOLINT(cata-u8-path)
        std::optional<zzip> z = zzip::load( save_path );
        saved_data = z->add_file( ( playerfile + SAVE_EXTENSION ).get_unrelative_path().filename(),
                                  save.str() );
        if( saved_data && z->compact_to( tmp_path, 2.0 ) ) {
            z.reset();
            saved_data = rename_file( tmp_path, save_path );
        }
    } else {
        saved_data = write_to_file( playerfile + SAVE_EXTENSION, [&]( std::ostream & fout ) {
            serialize_json( fout );
        }, _( "player data" ) );
    }
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
            return !std::isgraph( c, locale ) || !is_char_allowed( c );
        }, '_' );
    } else {
        std::replace_copy_if( std::begin( u.name ), std::end( u.name ),
                              std::ostream_iterator<char>( achievement_file_path ),
        [&]( const char c ) {
            return !is_char_allowed( c );
        }, '_' );
    }

    // Add a ~ if the player name was actually truncated.
    achievement_file_path << ( ( truncated_name_len != name_len ) ? "~-" : "-" );

    // Add world timestamp to distinguish characters from different worlds with the same name
    achievement_file_path << world_generator->active_world->timestamp << "-";

    const int character_id = get_player_character().getID().get_value();
    const std::string json_path_string = achievement_file_path.str() + std::to_string(
            character_id ) + ".json";

    return write_to_file( json_path_string, [&]( std::ostream & fout ) {
        get_achievements().write_json_achievements( fout, u.name );
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
            !save_dimension_data() ||
            !save_maps() ||
            !get_auto_pickup().save_character() ||
            !get_auto_notes_settings().save( true ) ||
            !get_safemode().save_character() ||
            !zone_manager::get_manager().save_zones() ||
            !write_to_file( PATH_INFO::world_base_save_path() / "uistate.json", [&](
        std::ostream & fout ) {
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
            write_to_file( PATH_INFO::world_base_save_path() / ( base64_encode(
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
            return !std::isgraph( c, locale ) || !is_char_allowed( c );
        }, '_' );
    } else {
        std::replace_copy_if( std::begin( u.name ), std::end( u.name ),
                              std::ostream_iterator<char>( memorial_file_path ),
        [&]( const char c ) {
            return !is_char_allowed( c );
        }, '_' );
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

struct npc_dist_to_player {
    const tripoint_abs_omt ppos{};
    npc_dist_to_player() : ppos( get_player_character().pos_abs_omt() ) { }
    // Operator overload required to leverage sort API.
    bool operator()( const shared_ptr_fast<npc> &a,
                     const shared_ptr_fast<npc> &b ) const {
        const tripoint_abs_omt apos = a->pos_abs_omt();
        const tripoint_abs_omt bpos = b->pos_abs_omt();
        return square_dist( ppos.xy(), apos.xy() ) <
               square_dist( ppos.xy(), bpos.xy() );
    }
};

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
    ret *= get_weather().weather_id->light_multiplier;
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

                const pathfinding_settings pf_settings = pathfinding_settings{ {{{ damage_bash, 8}}}, distance, distance * 2, 4, true, true, false, true, false, false };
                const pathfinding_target pf_t = pathfinding_target::point( critter->pos_bub() );
                if( !get_map().route( u.pos_bub(), pf_t, pf_settings ).empty() ) {
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
    for( std::pair<const field_type_id, field_entry> &field : here.field_at( u.pos_bub() ) ) {
        if( u.is_dangerous_field( field.second ) ) {
            if( u.in_vehicle ) {
                bool not_safe = false;
                for( const field_effect &fe : field.second.field_effects() ) {
                    not_safe |= !fe.immune_inside_vehicle;
                }
                if( !not_safe ) {
                    continue;
                }
            }
            return &field.second;
        }
    }

    return nullptr;
}

std::unordered_set<tripoint_abs_ms> game::get_fishable_locations_abs( int distance,
        const tripoint_bub_ms &fish_pos )
{
    const std::unordered_set<tripoint_bub_ms> temp = game::get_fishable_locations_bub( distance,
            fish_pos );
    std::unordered_set<tripoint_abs_ms> result;
    map &here = get_map();
    for( const tripoint_bub_ms pos : temp ) {
        result.insert( here.get_abs( pos ) );
    }

    return result;
}
std::unordered_set<tripoint_bub_ms> game::get_fishable_locations_bub( int distance,
        const tripoint_bub_ms &fish_pos )
{
    map &here = get_map();

    // We're going to get the contiguous fishable terrain starting at
    // the provided fishing location (e.g. where a line was cast or a fish
    // trap was set), and then check whether or not fishable monsters are
    // actually in those locations. This will help us ensure that we're
    // getting our fish from the location that we're ACTUALLY fishing,
    // rather than just somewhere in the vicinity.

    std::unordered_set<tripoint_bub_ms> visited;

    const tripoint_bub_ms fishing_boundary_min( fish_pos + point( -distance, -distance ) );
    const tripoint_bub_ms fishing_boundary_max( fish_pos + point( distance, distance ) );

    const inclusive_cuboid<tripoint_bub_ms> fishing_boundaries(
        fishing_boundary_min, fishing_boundary_max );

    // Starting at the provided location, get our fishable terrain
    // and populate a set with those locations which we'll then use
    // to determine if any fishable monsters are in those locations.
    return ff::point_flood_fill_4_connected<std::unordered_set>( fish_pos, visited, [&here,
    &fishing_boundaries]( const tripoint_bub_ms & p ) {
        return !fishing_boundaries.contains( p ) && here.has_flag( ter_furn_flag::TFLAG_FISHABLE, p );
    } );
}

std::vector<monster *> game::get_fishable_monsters( std::unordered_set<tripoint_abs_ms>
        &fishable_locations )
{
    const map &here = get_map();
    std::unordered_set<tripoint_bub_ms> temp;

    for( const tripoint_abs_ms pos : fishable_locations ) {
        temp.insert( here.get_bub( pos ) );
    }

    return game::get_fishable_monsters( temp );
}

std::vector<monster *> game::get_fishable_monsters( std::unordered_set<tripoint_bub_ms>
        &fishable_locations )
{
    std::vector<monster *> unique_fish;
    for( monster &critter : all_monsters() ) {
        // If it is fishable...
        if( critter.has_flag( mon_flag_FISHABLE ) ) {
            const tripoint_bub_ms critter_pos = critter.pos_bub();
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
    const map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

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

    const tripoint_bub_ms view = pos + u.view_offset;
    new_seen_mon.clear();

    static time_point previous_turn = calendar::turn_zero;
    const time_duration sm_ignored_turns =
        time_duration::from_turns( get_option<int>( "SAFEMODEIGNORETURNS" ) );

    for( Creature *c : u.get_visible_creatures( MAPSIZE_X ) ) {
        monster *m = dynamic_cast<monster *>( c );
        npc *p = dynamic_cast<npc *>( c );
        const tripoint_bub_ms c_pos = c->pos_bub( here );

        const direction dir_to_mon = direction_from( view.raw().xy(), c_pos.xy().raw() );
        const point_rel_ms m2( -view.raw().xy() + point( POSX + c_pos.x(), POSY + c_pos.y() ) );
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
            const int mon_dist = rl_dist( pos, critter.pos_bub( here ) );
            if( !safemode_empty ) {
                need_processing = get_safemode().check_monster(
                                      critter.name(),
                                      critter.attitude_to( u ),
                                      mon_dist,
                                      u.controlling_vehicle ) == rule_state::BLACKLISTED;
            } else {
                need_processing =  MATT_ATTACK == matt || ( MATT_FOLLOW == matt &&
                                   critter.get_dest() == u.pos_abs() );
            }
            if( need_processing ) {
                if( index < 8 && critter.sees( here, get_player_character() ) ) {
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

            const int npc_dist = rl_dist( u.pos_abs(), p->pos_abs() );
            if( !safemode_empty ) {
                need_processing = get_safemode().check_monster(
                                      get_safemode().npc_type_name(),
                                      p->attitude_to( u ),
                                      npc_dist,
                                      u.controlling_vehicle ) == rule_state::BLACKLISTED ;
            }
            if( !need_processing ) {
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
    map &here = get_map();

    // Dead monsters need to stay in the tracker until everything else that needs to die does so
    // This is because dying monsters can still interact with other dying monsters (@ref Creature::killer)
    bool monster_is_dead = critter_tracker->kill_marked_for_death();

    bool npc_is_dead = false;
    // can't use all_npcs as that does not include dead ones
    for( const auto &n : critter_tracker->active_npc ) {
        if( n->is_dead() ) {
            n->die( &here, nullptr ); // make sure this has been called to create corpses etc.
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
                ++it;
            }
        }
    }

    critter_died = false;
}

/* Knockback target at t by force number of tiles in direction from s to t
   stun > 0 indicates base stun duration, and causes impact stun; stun == -1 indicates only impact stun
   dam_mult multiplies impact damage, bash effect on impact, and sound level on impact */

void game::knockback( const tripoint_bub_ms &s, const tripoint_bub_ms &t, int force, int stun,
                      int dam_mult )
{
    std::vector<tripoint_bub_ms> traj;
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

void game::knockback( std::vector<tripoint_bub_ms> &traj, int stun, int dam_mult )
{
    // TODO: make the force parameter actually do something.
    // the header file says higher force causes more damage.
    // perhaps that is what it should do?
    tripoint_bub_ms tp = traj.front();
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    creature_tracker &creatures = get_creature_tracker();
    if( !creatures.creature_at( tp ) ) {
        debugmsg( _( "Nothing at (%d,%d,%d) to knockback!" ), tp.x(), tp.y(), tp.z() );
        return;
    }
    std::size_t force_remaining = traj.size();
    if( monster *const targ = creatures.creature_at<monster>( tp, true ) ) {
        const tripoint_bub_ms targ_pos = targ->pos_bub( here );

        if( stun > 0 ) {
            targ->add_effect( effect_stunned, 1_turns * stun );
            add_msg( _( "%s was stunned!" ), targ->name() );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( here.impassable( traj[i].xy() ) ) {
                targ->setpos( here, traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                    add_msg( _( "%s was stunned!" ), targ->name() );
                    add_msg( _( "%s slammed into an obstacle!" ), targ->name() );
                    targ->apply_damage( nullptr, bodypart_id( "torso" ), dam_mult * force_remaining );
                    targ->check_dead_state( &here );
                }
                here.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                targ->setpos( here, traj[i - 1] );
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
                } else if( pos == traj.front() ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->name() );
                }
                knockback( traj, stun, dam_mult );
                break;
            }
            targ->setpos( here, traj[i] );
            if( here.has_flag( ter_furn_flag::TFLAG_LIQUID, targ_pos ) && !targ->can_drown() &&
                !targ->is_dead() ) {
                targ->die( &here, nullptr );
                if( u.sees( here, *targ ) ) {
                    add_msg( _( "The %s drowns!" ), targ->name() );
                }
            }
            if( !here.has_flag( ter_furn_flag::TFLAG_LIQUID, targ_pos ) &&
                targ->has_flag( mon_flag_AQUATIC ) &&
                !targ->is_dead() ) {
                targ->die( &here, nullptr );
                if( u.sees( here, *targ ) ) {
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
            if( here.impassable( traj[i].xy() ) ) { // oops, we hit a wall!
                targ->setpos( here, traj[i - 1] );
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
                    targ->check_dead_state( &here );
                }
                here.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                targ->setpos( here, traj[i - 1] );
                force_remaining = traj.size() - i;
                if( stun != 0 ) {
                    add_msg( _( "%s was stunned!" ), targ->get_name() );
                    targ->add_effect( effect_stunned, 1_turns * force_remaining );
                }
                traj.erase( traj.begin(), traj.begin() + i );
                const tripoint_bub_ms &traj_front = traj.front();
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
                } else if( pos.x() == traj_front.x() && pos.y() == traj_front.y() &&
                           u.has_trait( trait_LEG_TENT_BRACE ) && u.is_barefoot() ) {
                    add_msg( _( "%s collided with you, and barely dislodges your tentacles!" ), targ->get_name() );
                } else if( pos.x() == traj_front.x() && pos.y() == traj_front.y() ) {
                    add_msg( m_bad, _( "%s collided with you and sent you flying!" ), targ->get_name() );
                }
                knockback( traj, stun, dam_mult );
                break;
            }
            targ->setpos( here, traj[i] );
        }
    } else if( pos == tp ) {
        if( stun > 0 ) {
            u.add_effect( effect_stunned, 1_turns * stun );
            add_msg( m_bad, n_gettext( "You were stunned for %d turn!",
                                       "You were stunned for %d turns!",
                                       stun ),
                     stun );
        }
        for( size_t i = 1; i < traj.size(); i++ ) {
            if( here.impassable( traj[i] ) ) { // oops, we hit a wall!
                u.setpos( here, traj[i - 1] );
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
                    u.check_dead_state( &here );
                }
                here.bash( traj[i], 2 * dam_mult * force_remaining );
                break;
            } else if( creatures.creature_at( traj[i] ) ) {
                u.setpos( here, traj[i - 1] );
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
            if( here.has_flag( ter_furn_flag::TFLAG_LIQUID, pos ) && force_remaining == 0 ) {
                avatar_action::swim( here, u, pos );
            } else {
                u.setpos( here,  traj[i] );
            }
        }
    }
}

void game::use_computer( const tripoint_bub_ms &p )
{
    map &here = get_map();

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
        !u.has_flag( json_flag_ENHANCED_VISION ) ) {
        add_msg( m_info, _( "You'll need to put on reading glasses before you can see the screen." ) );
        return;
    }

    computer *used = here.computer_at( p );

    if( used == nullptr ) {
        if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, p ) ) { //Console without map data
            add_msg( m_bad, _( "The console doesn't display anything coherent." ) );
        } else {
            dbg( D_ERROR ) << "game:use_computer: Tried to use computer at (" <<
                           p.x() << ", " << p.y() << ", " << p.z() << ") - none there";
            debugmsg( "Tried to use computer at (%d, %d, %d) - none there", p.x(), p.y(), p.z() );
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
        if( const shared_ptr_fast<monster> mon_ptr = critter_tracker->find( critter.pos_abs() ) ) {
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

static bool can_place_monster( const monster &mon, const tripoint_bub_ms &p )
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

static bool can_place_monster( const monster &mon, map *here, const tripoint_bub_ms &p )
{
    const tripoint_abs_ms pos = here->get_abs( p );
    creature_tracker &creatures = get_creature_tracker();
    if( const monster *const critter = creatures.creature_at<monster>( pos ) ) {
        // creature_tracker handles this. The hallucination monster will simply vanish
        if( !critter->is_hallucination() ) {
            return false;
        }
    }
    // Although monsters can sometimes exist on the same place as a Character (e.g. ridden horse),
    // it is usually wrong. So don't allow it.
    if( creatures.creature_at<Character>( pos ) ) {
        return false;
    }
    return mon.will_move_to( here, p ) && mon.know_danger_at( here, p );
}

static bool can_place_npc( const tripoint_bub_ms &p )
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

static std::optional<tripoint_bub_ms> choose_where_to_place_monster( const monster &mon,
        const tripoint_range<tripoint_bub_ms> &range )
{
    return random_point( range, [&]( const tripoint_bub_ms & p ) {
        return can_place_monster( mon, p );
    } );
}

static std::optional<tripoint_bub_ms> choose_where_to_place_monster( const monster &mon, map *here,
        const tripoint_range<tripoint_bub_ms> &range )
{
    return random_point( range, [&]( const tripoint_bub_ms & p ) {
        return can_place_monster( mon, here, p );
    } );
}

monster *game::place_critter_at( const mtype_id &id, const tripoint_bub_ms &p )
{
    return place_critter_around( id, p, 0 );
}

monster *game::place_critter_at( const shared_ptr_fast<monster> &mon, const tripoint_bub_ms &p )
{
    return place_critter_around( mon, p, 0 );
}

monster *game::place_critter_around( const mtype_id &id, const tripoint_bub_ms &center,
                                     const int radius )
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
                                     const tripoint_bub_ms &center,
                                     const int radius,
                                     bool forced )
{
    map &here = get_map();

    std::optional<tripoint_bub_ms> where;
    if( forced || can_place_monster( *mon, center ) ) {
        where = center;
    }

    // This loop ensures the monster is placed as close to the center as possible,
    // but all places that equally far from the center have the same probability.
    for( int r = 1; r <= radius && !where; ++r ) {
        where = choose_where_to_place_monster( *mon, here.points_in_radius( center, r ) );
    }

    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    if( critter_tracker->add( mon ) ) {
        mon->gravity_check();
        return mon.get();
    }
    return nullptr;
}

monster *game::place_critter_within( const mtype_id &id,
                                     const tripoint_range<tripoint_bub_ms> &range )
{
    // TODO: change this into an assert, it must never happen.
    if( id.is_null() ) {
        return nullptr;
    }
    shared_ptr_fast<monster> mon = make_shared_fast<monster>( id );
    mon->ammo = mon->type->starting_ammo;
    return place_critter_within( mon, range );
}

monster *game::place_critter_within( const shared_ptr_fast<monster> &mon,
                                     const tripoint_range<tripoint_bub_ms> &range )
{
    const std::optional<tripoint_bub_ms> where = choose_where_to_place_monster( *mon, range );
    if( !where ) {
        return nullptr;
    }
    mon->spawn( *where );
    if( critter_tracker->add( mon ) ) {
        mon->gravity_check();
        return mon.get();
    }
    return nullptr;
}

monster *game::place_critter_at_or_within( const shared_ptr_fast<monster> &mon, map *here,
        const tripoint_bub_ms &center, const tripoint_range<tripoint_bub_ms> &range )
{
    tripoint_range<tripoint_bub_ms> center_range = points_in_radius( center, 0 );

    std::optional<tripoint_bub_ms> where = choose_where_to_place_monster( *mon, here, center_range );

    if( !where ) {
        where = choose_where_to_place_monster( *mon, here, range );
    }

    if( !where ) {
        return nullptr;
    }
    mon->spawn( here->get_abs( where.value() ) );
    if( critter_tracker->add( mon ) ) {
        mon->gravity_check();
        return mon.get();
    }
    return nullptr;
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

bool game::find_nearby_spawn_point( const tripoint_bub_ms &target, const mtype_id &mt,
                                    int min_radius,
                                    int max_radius, tripoint_bub_ms &point, bool outdoor_only, bool indoor_only, bool open_air_allowed )
{
    map &here = get_map();

    tripoint_bub_ms target_point;
    //find a legal outdoor place to spawn based on the specified radius,
    //we just try a bunch of random points and use the first one that works, it none do then no spawn
    for( int attempts = 0; attempts < 75; attempts++ ) {
        target_point = target + tripoint( rng( -max_radius, max_radius ),
                                          rng( -max_radius, max_radius ), 0 );
        if( can_place_monster( monster( mt->id ), target_point ) &&
            ( open_air_allowed || here.has_floor_or_water( target_point ) ) &&
            ( !outdoor_only || here.is_outside( target_point ) ) &&
            ( !indoor_only || !here.is_outside( target_point ) ) &&
            rl_dist( target_point, target ) >= min_radius ) {
            point = target_point;
            return true;
        }
    }
    return false;
}

bool game::find_nearby_spawn_point( const tripoint_bub_ms &target, int min_radius,
                                    int max_radius, tripoint_bub_ms &point, bool outdoor_only, bool indoor_only, bool open_air_allowed )
{
    map &here = get_map();

    tripoint_bub_ms target_point;
    //find a legal outdoor place to spawn based on the specified radius,
    //we just try a bunch of random points and use the first one that works, it none do then no spawn
    for( int attempts = 0; attempts < 75; attempts++ ) {
        target_point = target + tripoint( rng( -max_radius, max_radius ),
                                          rng( -max_radius, max_radius ), 0 );
        if( can_place_npc( target_point ) &&
            ( open_air_allowed || here.has_floor_or_water( target_point ) ) &&
            ( !outdoor_only || here.is_outside( target_point ) ) &&
            ( !indoor_only || !here.is_outside( target_point ) ) &&
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
bool game::spawn_hallucination( const tripoint_bub_ms &p )
{
    map &here = get_map();

    //Don't spawn hallucinations on open air
    if( here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) {
        return false;
    }

    if( one_in( 100 ) ) {
        shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
        tmp->normalize();
        tmp->randomize( NC_HALLU );
        tmp->spawn_at_precise( here.get_abs( p ) );
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
    const oter_id &terrain_type = overmap_buffer.ter( get_player_character().pos_abs_omt() );
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
bool game::spawn_hallucination( const tripoint_bub_ms &p, const mtype_id &mt,
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
    if( !get_creature_tracker().creature_at( phantasm->pos_bub(), true ) ) {
        return critter_tracker->add( phantasm );
    } else {
        return false;
    }
}

bool game::spawn_npc( const tripoint_bub_ms &p, const string_id<npc_template> &npc_class,
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
    tmp->spawn_at_precise( get_map().get_abs( p ) );
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
    map &here = get_map();

    Creature *lhs = &a;
    Creature *rhs = &b;

    if( rhs->as_character() && !lhs->as_character() ) {
        std::swap( lhs, rhs ); // LHS will always be a character if either is a character
    }

    // Possible options:
    // LHS and RHS are both monsters
    // LHS is character, RHS is monster
    // LHS is character, RHS is character
    // This is simply to make it easier to follow the code.

    const tripoint_bub_ms lhs_cached_pos = lhs->pos_bub( );
    const tripoint_bub_ms rhs_cached_pos = rhs->pos_bub( );

    if( lhs == rhs ) {
        // No need to do anything, but print a debugmsg anyway
        debugmsg( "Tried to swap %s with itself", lhs->disp_name() );
        return true;
    }
    creature_tracker &creatures = get_creature_tracker();
    if( creatures.creature_at( lhs_cached_pos ) != &a ) {
        if( creatures.creature_at( lhs_cached_pos ) == nullptr ) {
            debugmsg( "Tried to swap %s and %s when the latter isn't present at its own location %s.",
                      rhs->disp_name(), lhs->disp_name(), lhs_cached_pos.to_string() );
        } else {
            debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                      rhs->disp_name(), creatures.creature_at( lhs_cached_pos )->disp_name() );
        }
        return false;
    }
    if( creatures.creature_at( rhs_cached_pos ) != &b ) {
        if( creatures.creature_at( rhs_cached_pos ) == nullptr ) {
            debugmsg( "Tried to swap %s and %s when the latter isn't present at its own location %s.",
                      lhs->disp_name(), rhs->disp_name(), rhs_cached_pos.to_string() );
        } else {
            debugmsg( "Tried to swap when it would cause a collision between %s and %s.",
                      lhs->disp_name(), creatures.creature_at( rhs_cached_pos )->disp_name() );
        }
        return false;
    }

    if( lhs->is_monster() ) { // then we have two monsters.
        monster *lhs_as_mon = dynamic_cast< monster * >( lhs );
        monster *rhs_as_mon = dynamic_cast< monster * >( rhs );
        if( lhs_as_mon == nullptr || rhs_as_mon == nullptr || lhs_as_mon == rhs_as_mon ) {
            debugmsg( "Couldn't swap two monsters" );
            return false;
        }

        critter_tracker->swap_positions( *lhs_as_mon, *rhs_as_mon );
        return true;
    }

    // If we've reached here, LHS must be a character. RHS *might* be a character.
    Character *lhs_as_char = static_cast< Character * >( lhs );

    // Case: character swaps with monster.
    if( rhs->as_monster() ) {
        if( lhs_as_char->in_vehicle ) { // unboard lhs always...
            here.unboard_vehicle( lhs_cached_pos );
        }
        // Go ahead and move rhs, so the destination is clear
        rhs->setpos( here, lhs_cached_pos );
        if( lhs->as_avatar() ) {
            walk_move( rhs_cached_pos ); // Does vehicle boarding! Do not double board! DON'T DO IT! >:(
            return true;
        } else {
            if( here.veh_at( rhs_cached_pos ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
                here.board_vehicle( rhs_cached_pos, lhs_as_char );
            }
            lhs->setpos( here, rhs_cached_pos );
            return true;
        }
    }

    // If we've reached here, both LHS and RHS are characters. So we need to do some real juggling...
    Character *rhs_as_char = static_cast< Character * >( rhs );

    // Make sure we're both unboarded.
    if( lhs_as_char->in_vehicle ) {
        here.unboard_vehicle( lhs_cached_pos );
    }
    if( rhs_as_char->in_vehicle ) {
        here.unboard_vehicle( rhs_cached_pos );
    }

    if( lhs->as_avatar() ) {
        if( here.veh_at( lhs_cached_pos ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            here.board_vehicle( lhs_cached_pos, rhs_as_char ); // sets position!
        }
        rhs->setpos( here, lhs_cached_pos ); // If board vehicle didn't move us, harmless otherwise.
        walk_move( rhs_cached_pos ); // boards if needed!
        return true;
    } else if( rhs->as_avatar() ) {
        if( here.veh_at( rhs_cached_pos ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            here.board_vehicle( rhs_cached_pos, lhs_as_char );
        }
        lhs->setpos( here, rhs_cached_pos );
        walk_move( lhs_cached_pos );
        return true;
    }
    // Two NPCs, oh boy! no walk_move() here.
    if( here.veh_at( rhs_cached_pos ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        here.board_vehicle( rhs_cached_pos, lhs_as_char );
    }
    if( here.veh_at( lhs_cached_pos ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        here.board_vehicle( lhs_cached_pos, rhs_as_char );
    }
    lhs->setpos( here, rhs_cached_pos );
    rhs->setpos( here, lhs_cached_pos );
    return true;
}

bool game::is_empty( const tripoint_bub_ms &p )
{
    map &here = get_map();

    return ( here.passable( p ) || here.has_flag( ter_furn_flag::TFLAG_LIQUID, p ) ) &&
           get_creature_tracker().creature_at( p ) == nullptr;
}

bool game::is_empty( map *here, const tripoint_abs_ms &p )
{
    const tripoint_bub_ms pos = here->get_bub( p );
    return ( here->passable( pos ) || here->has_flag( ter_furn_flag::TFLAG_LIQUID, pos ) ) &&
           get_creature_tracker().creature_at( p ) == nullptr;
}

bool game::is_in_sunlight( const tripoint_bub_ms &p )
{
    return !is_sheltered( p ) &&
           incident_sun_irradiance( get_weather().weather_id, calendar::turn ) > irradiance::minimal;
}

bool game::is_in_sunlight( map *here, const tripoint_bub_ms &p )
{
    return !is_sheltered( here, p ) &&
           incident_sun_irradiance( current_weather( here->get_abs( p ), calendar::turn ),
                                    calendar::turn ) > irradiance::minimal;
}

bool game::is_sheltered( const tripoint_bub_ms &p )
{
    map &here = get_map();

    return game::is_sheltered( &here, p );
}

bool game::is_sheltered( map *here, const tripoint_bub_ms &p )
{
    const optional_vpart_position vp = here->veh_at( p );
    bool is_inside = vp && vp->is_inside();

    return !here->is_outside( p ) ||
           p.z() < 0 ||
           is_inside;
}

bool game::revive_corpse( const tripoint_bub_ms &p, item &it )
{
    return revive_corpse( p, it, 1 );
}

bool game::revive_corpse( const tripoint_bub_ms &p, item &it, int radius )
{
    if( !it.is_corpse() ) {
        debugmsg( "Tried to revive a non-corpse." );
        return false;
    }
    // If this is not here, the game may attempt to spawn a monster before the map exists,
    // leading to it querying for furniture, and crashing.
    if( g->new_game || g->swapping_dimensions ) {
        return false;
    }
    if( it.has_flag( flag_FIELD_DRESS ) || it.has_flag( flag_FIELD_DRESS_FAILED ) ||
        it.has_flag( flag_QUARTERED ) ) {
        // Failed reanimation due to corpse being butchered
        return false;
    }

    assing_revive_form( it, p );

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

    if( it.get_var( "times_combatted", 0.0 ) > 0.0 ) {
        critter.times_combatted_player = it.get_var( "times_combatted", 0.0 );
    }

    return place_critter_around( newmon_ptr, tripoint_bub_ms( p ), radius );
}

void game::assing_revive_form( item &it, tripoint_bub_ms p )
{
    const mtype *montype = it.get_mtype();
    if( montype == nullptr ) {
        return;
    }
    dialogue d( nullptr, nullptr );
    write_var_value( var_type::context, "loc", &d, get_map().get_abs( p ) );
    write_var_value( var_type::context, "corpse_damage", &d, it.damage() );
    for( const revive_type &rev_type : montype->revive_types ) {
        if( rev_type.condition( d ) ) {
            if( !rev_type.revive_mon.is_null() ) {
                it.set_var( "zombie_form", rev_type.revive_mon.str() );
                return;
            }
            if( !rev_type.revive_monster_group.is_null() ) {
                mtype_id mon = MonsterGroupManager::GetRandomMonsterFromGroup( rev_type.revive_monster_group );
                it.set_var( "zombie_form", mon.str() );
            }
        }
    }
}

void game::save_cyborg( item *cyborg, const tripoint_bub_ms &couch_pos, Character &installer )
{
    map &here = get_map();

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

        here.i_rem( couch_pos, cyborg );

        shared_ptr_fast<npc> tmp = make_shared_fast<npc>();
        tmp->normalize();
        tmp->load_npc_template( npc_template_cyborg_rescued );
        tmp->spawn_at_precise( here.get_abs( couch_pos ) );
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
                here.i_rem( couch_pos, cyborg );
                break;
            default:
                break;
        }

    }

}

void game::exam_appliance( vehicle &veh, const point_rel_ms &c )
{
    map &here = get_map();

    player_activity act = veh_app_interact::run( here, veh, c );
    if( act ) {
        u.set_moves( 0 );
        u.assign_activity( act );
    }
}

void game::exam_vehicle( vehicle &veh, const point_rel_ms &c )
{
    map &here = get_map();

    if( veh.magic ) {
        add_msg( m_info, _( "This is your %s" ), veh.name );
        return;
    }
    player_activity act = veh_interact::run( here, veh, c );
    if( act ) {
        u.set_moves( 0 );
        u.assign_activity( act );
    }
}

void game::open_gate( const tripoint_bub_ms &p )
{
    gates::open_gate( p, u );
}

void game::moving_vehicle_dismount( const tripoint_bub_ms &dest_loc )
{
    map &here = get_map();

    const tripoint_bub_ms pos = u.pos_bub( );

    const optional_vpart_position vp = here.veh_at( pos );
    if( !vp ) {
        debugmsg( "Tried to exit non-existent vehicle." );
        return;
    }
    vehicle *const veh = &vp->vehicle();
    if( pos == dest_loc ) {
        debugmsg( "Need somewhere to dismount towards." );
        return;
    }
    tileray ray( dest_loc.xy() - pos.xy() );
    // TODO:: make dir() const correct!
    const units::angle d = ray.dir();
    add_msg( _( "You dive from the %s." ), veh->name );
    here.unboard_vehicle( pos );
    u.mod_moves( -to_moves<int>( 2_seconds ) );
    // Dive three tiles in the direction of tox and toy
    fling_creature( &u, d, 30, true, true );
    // Hit the ground according to vehicle speed
    if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos ) ) {
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
    map &here = get_map();

    if( vehicle *remote_veh = remoteveh() ) { // remote controls have priority
        for( const vpart_reference &vpr : remote_veh->get_avail_parts( "REMOTE_CONTROLS" ) ) {
            remote_veh->interact_with( &here, vpr.pos_bub( here ) );
            return;
        }
    }
    vehicle *veh = nullptr;
    bool controls_ok = false;
    bool reins_ok = false;
    if( const optional_vpart_position vp = here.veh_at( u.pos_bub() ) ) {
        veh = &vp->vehicle();
        const int controls_idx = veh->avail_part_with_feature( vp->mount_pos(), "CONTROLS" );
        const int reins_idx = veh->avail_part_with_feature( vp->mount_pos(), "CONTROL_ANIMAL" );
        controls_ok = controls_idx >= 0; // controls available to "drive"
        reins_ok = reins_idx >= 0 // reins + animal available to "drive"
                   && veh->has_engine_type( fuel_type_animal, false )
                   && veh->get_harnessed_animal( here );
        if( veh->player_in_control( here, u ) ) {
            // player already "driving" - offer ways to leave
            if( controls_ok ) {
                veh->interact_with( &here, u.pos_bub() );
            } else if( reins_idx >= 0 ) {
                u.controlling_vehicle = false;
                add_msg( m_info, _( "You let go of the reins." ) );
            }
        } else if( u.in_vehicle && ( controls_ok || reins_ok ) ) {
            // player not driving but has controls or reins on tile
            if( veh->is_locked ) {
                veh->interact_with( &here, u.pos_bub() );
                return; // interact_with offers to hotwire
            }
            if( !veh->handle_potential_theft( u ) ) {
                return; // player not owner and refused to steal
            }
            const item_location weapon = u.get_wielded_item();
            if( weapon ) {
                if( u.worn_with_flag( flag_RESTRICT_HANDS ) ) {
                    add_msg( m_info, _( "Something you are wearing hinders the use of both hands." ) );
                    return;
                }
                if( !u.has_two_arms_lifting() ) {
                    if( query_yn(
                            _( "You can't drive because you have to wield a %s.\n\nPut it away?" ),
                            weapon->tname() ) ) {
                        if( !u.unwield() ) {
                            return;
                        }
                    } else {
                        return;
                    }
                } else if( weapon->is_two_handed( u ) ) {
                    if( query_yn(
                            _( "You can't drive because you have to wield a %s with both hands.\n\nPut it away?" ),
                            weapon->tname() ) ) {
                        if( !u.unwield() ) {
                            return;
                        }
                    } else {
                        return;
                    }
                }
            }
            if( veh->engine_on ) {
                u.controlling_vehicle = true;
                add_msg( _( "You take control of the %s." ), veh->name );
            } else {
                veh->start_engines( here, &u, true );
            }
        }
    }
    if( !controls_ok && !reins_ok ) { // no controls or reins under player position, search nearby
        int num_valid_controls = 0;
        std::optional<tripoint_bub_ms> vehicle_position;
        std::optional<vpart_reference> vehicle_controls;
        for( const tripoint_bub_ms &elem : here.points_in_radius( get_player_character().pos_bub(), 1 ) ) {
            if( const optional_vpart_position vp = here.veh_at( elem ) ) {
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
            const std::optional<tripoint_bub_ms> temp = choose_adjacent( _( "Control vehicle where?" ) );
            if( !vehicle_position ) {
                return;
            } else {
                vehicle_position.value() = temp.value();
            }
            const optional_vpart_position vp = here.veh_at( *vehicle_position );
            if( vp ) {
                vehicle_controls = vp.value().part_with_feature( "CONTROLS", true );
                if( !vehicle_controls ) {
                    add_msg( _( "The vehicle doesn't have controls there." ) );
                    return;
                }
            } else {
                add_msg( _( "There's no vehicle there." ) );
                return;
            }
        }
        // If we hit neither of those, there's only one set of vehicle controls, which should already have been found.
        if( vehicle_controls ) {
            veh = &vehicle_controls->vehicle();
            if( !veh->handle_potential_theft( u ) ) {
                return;
            }
            veh->interact_with( &here, *vehicle_position );
        }
    }
    if( u.controlling_vehicle ) {
        // If we reached here, we gained control of a vehicle.
        // Clear the map memory for the area covered by the vehicle to eliminate ghost vehicles.
        for( const tripoint_abs_ms &target : veh->get_points() ) {
            u.memorize_clear_decoration( target, "vp_" );
            here.memory_cache_dec_set_dirty( here.get_bub( target ), true );
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
    amenu.addentry( push, ( debug_mode || ( !who.is_enemy() && !who.in_sleep_state() ) ) &&
                    !who.is_mounted(), 'p', _( "Push away" ) );
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
        if( !prompt_dangerous_tile( who.pos_bub() ) ) {
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
        if( !obeys ) {
            if( !query_yn( _( "%s may be upset by this.  Continue?" ), who.name ) ) {
                return true;
            }
            npc_opinion &attitude = who.op_of_u;
            attitude.anger += 3;
            attitude.trust -= 3;
            attitude.value -= 1;
            who.form_opinion( u );

        }
        // TODO: Make NPCs protest when displaced onto dangerous crap
        tripoint_bub_ms oldpos = who.pos_bub();
        who.move_away_from( u.pos_bub(), true );
        u.mod_moves( -20 );
        if( oldpos != who.pos_bub() ) {
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
            bool did_use = u.invoke_item( &used, heal_string, who.pos_bub() );
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
    map &here = get_map();

    // if we are driving a vehicle, examine the
    // current tile without asking.
    const optional_vpart_position vp = here.veh_at( u.pos_bub() );
    if( vp && vp->vehicle().player_in_control( here, u ) ) {
        examine( u.pos_bub(), with_pickup );
        return;
    }

    std::optional<tripoint_bub_ms> examp;
    if( with_pickup ) {
        // Examine and/or pick up items
        examp = choose_adjacent_highlight( here, _( "Examine terrain, furniture, or items where?" ),
                                           _( "There is nothing that can be examined nearby." ),
                                           ACTION_EXAMINE_AND_PICKUP, false );
    } else {
        // Examine but do not pick up items
        examp = choose_adjacent_highlight( here, _( "Examine terrain or furniture where?" ),
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

std::string game::get_fire_fuel_string( const tripoint_bub_ms &examp )
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

void game::examine( const tripoint_bub_ms &examp, bool with_pickup )
{
    map &here = get_map();

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

    const optional_vpart_position vp = here.veh_at( examp );
    if( vp ) {
        if( !u.is_mounted() || u.mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) ) {
            if( !vp->vehicle().is_appliance() ) {
                vp->vehicle().interact_with( &here, examp, with_pickup );
            } else {
                g->exam_appliance( vp->vehicle(), vp->mount_pos() );
            }
            return;
        } else {
            add_msg( m_warning, _( "You cannot interact with a vehicle while mounted." ) );
        }
    }

    iexamine::part_con( get_avatar(), examp );
    // trap::iexamine will handle the invisible traps.
    here.tr_at( examp ).examine( examp );

    if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, examp ) && !u.is_mounted() ) {
        use_computer( examp );
        return;
    } else if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, examp ) && u.is_mounted() ) {
        add_msg( m_warning, _( "You cannot use a console while mounted." ) );
    }
    const furn_t &xfurn_t = here.furn( examp ).obj();
    const ter_t &xter_t = here.ter( examp ).obj();

    const tripoint_bub_ms player_pos = u.pos_bub();

    if( here.has_furn( examp ) ) {
        if( !u.cant_do_mounted() ) {
            if( !here.has_flag( "FREE_TO_EXAMINE", examp ) &&
                !warn_player_maybe_anger_local_faction( false, true ) ) {
                return; // player declined to mess with faction's stuff
            }
            xfurn_t.examine( u, examp );
        }
    } else {
        if( xter_t.can_examine( examp ) && !u.is_mounted() ) {
            if( !here.has_flag( "FREE_TO_EXAMINE", examp ) &&
                !warn_player_maybe_anger_local_faction( false, true ) ) {
                return; // player declined to mess with faction's stuff
            }
            xter_t.examine( u, examp );
        }
    }

    // Did the player get moved? Bail out if so; our examp probably
    // isn't valid anymore.
    if( player_pos != u.pos_bub() ) {
        return;
    }

    bool none = true;
    if( xter_t.can_examine( examp ) || xfurn_t.can_examine( examp ) ) {
        none = false;
    }

    // In case of teleport trap or somesuch
    if( player_pos != u.pos_bub() ) {
        return;
    }

    // Feedback for fire lasting time, this can be judged while mounted
    const std::string fire_fuel = get_fire_fuel_string( examp );
    if( !fire_fuel.empty() ) {
        add_msg( fire_fuel );
    }

    if( here.has_flag( ter_furn_flag::TFLAG_SEALED, examp ) ) {
        if( none ) {
            if( here.has_flag( ter_furn_flag::TFLAG_UNSTABLE, examp ) ) {
                add_msg( _( "The %s is too unstable to remove anything." ), here.name( examp ) );
            } else {
                add_msg( _( "The %s is firmly sealed." ), here.name( examp ) );
            }
        }
    } else {
        //examp has no traps, is a container and doesn't have a special examination function
        if( here.tr_at( examp ).is_null() && here.i_at( examp ).empty() &&
            here.has_flag( ter_furn_flag::TFLAG_CONTAINER, examp ) && none ) {
            add_msg( _( "It is empty." ) );
        } else if( ( here.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER, examp ) &&
                     xfurn_t.has_examine( iexamine::fireplace ) ) ||
                   xfurn_t.has_examine( iexamine::workbench ) ) {
            return;
        } else {
            sounds::process_sound_markers( &u );
            // Pick up items, if there are any, unless there is reason to not to
            if( with_pickup && here.has_items( examp ) && !u.is_mounted() &&
                !here.has_flag( ter_furn_flag::TFLAG_NO_PICKUP_ON_EXAMINE, examp ) &&
                !here.only_liquid_in_liquidcont( examp ) ) {
                pickup( examp );
            }
        }
    }
}

bool game::warn_player_maybe_anger_local_faction( bool really_bad_offense,
        bool asking_for_public_goods )
{
    Character &player_character = get_player_character();
    std::optional<basecamp *> bcp = overmap_buffer.find_camp(
                                        player_character.pos_abs_omt().xy() );
    if( !bcp ) {
        return true; // Nobody to piss off
    }
    basecamp *actual_camp = *bcp;
    if( actual_camp->get_owner()->limited_area_claim &&
        player_character.pos_abs_omt() != actual_camp->camp_omt_pos() ) {
        return true; // outside of claimed area
    }
    if( actual_camp->allowed_access_by( player_character, asking_for_public_goods ) ) {
        return true; // You're allowed to do this anyway
    }

    // The threshold for guaranteed hostility. Don't bother query/modifying relationship if they already hate us
    // TODO: Make this magic number into a constant
    if( actual_camp->get_owner()->likes_u < -10 ) {
        return true;
    }

    // Else there's a camp, and we're doing something we're not supposed to! Time to warn the player.
    if( !query_yn(
            _( "You're in the territory of %s, they probably won't appreciate your actions.  Continue anyway?" ),
            actual_camp->get_owner()->name ) ) {
        return false;
    } else {
        faction *owner_fac_pointer = g->faction_manager_ptr->get( actual_camp->get_owner() );
        // really_bad_offense loses 20 points of fac rep, otherwise 5
        owner_fac_pointer->likes_u -= really_bad_offense ? 20 : 5;
        owner_fac_pointer->trusts_u -= really_bad_offense ? 20 : 5;
        owner_fac_pointer->respects_u -= really_bad_offense ? 20 : 5;
        return true;
    }
}

void game::pickup()
{
    map &here = get_map();

    // Prompt for which adjacent/current tile to pick up items from
    const std::optional<tripoint_bub_ms> where_ = choose_adjacent_highlight(
                here, _( "Pick up items where?" ),
                _( "There is nothing to pick up nearby." ),
                ACTION_PICKUP, false );
    if( !where_ ) {
        return;
    }
    // Pick up items only from the selected tile
    u.pick_up( game_menus::inv::pickup( {where_.value()} ) );
}

void game::pickup_all()
{
    // Pick up items from current and all adjacent tiles
    u.pick_up( game_menus::inv::pickup() );
}

void game::pickup( const tripoint_bub_ms &p )
{
    map &here = get_map();

    // Highlight target
    shared_ptr_fast<game::draw_callback_t> hilite_cb = make_shared_fast<game::draw_callback_t>( [&]() {
        here.drawsq( w_terrain, p, drawsq_params().highlight( true ) );
    } );
    add_draw_callback( hilite_cb );

    // Pick up items only from the selected tile
    u.pick_up( game_menus::inv::pickup( {p} ) );
}

//Shift player by one tile, look_around(), then restore previous position.
//represents carefully peeking around a corner, hence the large move cost.
void game::peek()
{
    map &here = get_map();

    const std::optional<tripoint_rel_ms> p = choose_direction( _( "Peek where?" ), true );
    if( !p ) {
        return;
    }
    tripoint_bub_ms new_pos = u.pos_bub() + *p;
    if( p->z() != 0 ) {
        // Character might peek to a different submap; ensures return location is accurate.
        const tripoint_abs_ms old_loc = u.pos_abs();
        vertical_move( p->z(), false, true );

        if( old_loc != u.pos_abs() ) {
            new_pos = u.pos_bub();
            u.move_to( old_loc );
            here.vertical_shift( old_loc.z() );
        } else {
            return;
        }
    }

    if( here.impassable( new_pos ) ) {
        return;
    }

    peek( new_pos );
}

void game::peek( const tripoint_bub_ms &p )
{
    map &here = get_map();

    u.mod_moves( -u.get_speed() * 2 );
    tripoint_bub_ms prev = u.pos_bub();
    u.setpos( here, p, false );
    const bool is_same_pos = u.pos_bub() == prev;
    const bool is_standup_peek = is_same_pos && u.is_crouching();
    tripoint_bub_ms center = p;
    here.build_map_cache( p.z() );
    here.update_visibility_cache( p.z() );

    look_around_result result;
    const look_around_params looka_params = { true, center, center, false, false, true, true};
    if( is_standup_peek ) {   // Non moving peek from crouch is a standup peek
        u.reset_move_mode();
        result = look_around( looka_params );
        u.activate_crouch_mode();
    } else {                // Else is normal peek
        result = look_around( looka_params );
        u.setpos( here, prev, false );
    }

    if( result.peek_action ) {
        if( *result.peek_action == PA_BLIND_THROW ) {
            item_location loc;
            avatar_action::plthrow( u, loc, p );
        } else if( *result.peek_action == PA_BLIND_THROW_WIELDED ) {
            avatar_action::plthrow_wielded( u, p );
        }
    }
    here.invalidate_map_cache( p.z() );
    here.invalidate_visibility_cache();
}

////////////////////////////////////////////////////////////////////////////////////////////
std::optional<tripoint_bub_ms> game::look_debug()
{
    editmap edit;
    return edit.edit();
}
////////////////////////////////////////////////////////////////////////////////////////////

bool game::check_zone( const zone_type_id &type, const tripoint_bub_ms &where ) const
{
    map &here = get_map();

    return zone_manager::get_manager().has( type, here.get_abs( where ) );
}

bool game::check_near_zone( const zone_type_id &type, const tripoint_bub_ms &where ) const
{
    map &here = get_map();

    return zone_manager::get_manager().has_near( type, here.get_abs( where ) );
}

bool game::is_zones_manager_open() const
{
    return zones_manager_open;
}

std::optional<std::vector<tripoint_bub_ms>> game::safe_route_to( Character &who,
        const tripoint_bub_ms &target, int threshold,
        const std::function<void( const std::string &msg )> &report ) const
{
    map &here = get_map();
    if( !who.sees( here, target ) ) {
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
    for( const tripoint_bub_ms &p : here.points_in_radius( target, threshold, 0 ) ) {
        if( is_dangerous_tile( p ) ) {
            continue;
        }
        const route_t route = here.route( who.pos_bub(), pathfinding_target::point( p ),
        who.get_pathfinding_settings(), [this]( const tripoint_bub_ms & p ) {
            return is_dangerous_tile( p );
        } );
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

static void add_item_recursive( std::vector<std::string> &item_order,
                                std::map<std::string, map_item_stack> &temp_items, const item *it,
                                const tripoint_rel_ms &relative_pos )
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
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> item_order;

    if( u.is_blind() ) {
        return ret;
    }

    for( tripoint_bub_ms &points_p_it : closest_points_first( pos, iRadius ) ) {
        if( points_p_it.y() >= pos.y() - iRadius && points_p_it.y() <= pos.y() + iRadius &&
            u.sees( here, points_p_it ) &&
            here.sees_some_items( points_p_it, u ) ) {

            for( item &elem : here.i_at( points_p_it ) ) {
                const tripoint_rel_ms relative_pos = points_p_it - pos;

                add_item_recursive( item_order, temp_items, &elem, relative_pos );
            }
        }
    }

    ret.reserve( item_order.size() );
    for( auto &elem : item_order ) {
        ret.push_back( temp_items[elem] );
    }

    return ret;
}

int game::get_moves_since_last_save() const
{
    return moves_since_last_save;
}

int game::get_user_action_counter() const
{
    return user_action_counter;
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

    game_menus::inv::insert_items( item_loc );
}

void game::unload_container()
{
    if( const std::optional<tripoint_bub_ms> pnt = choose_adjacent( _( "Unload where?" ) ) ) {
        u.drop( game_menus::inv::unload_container(), *pnt );
    }
}

void game::drop_in_direction( const tripoint_bub_ms &pnt )
{
    u.drop( game_menus::inv::multidrop( u ), pnt );
}

static item::reload_option favorite_ammo_or_select( avatar &u, item_location &loc, bool empty,
        bool prompt )
{
    if( u.ammo_location ) {
        std::vector<item::reload_option> ammo_list;
        if( u.list_ammo( loc, ammo_list, false ) ) {
            const auto is_favorite_and_compatible = [&loc, &u]( const item::reload_option & opt ) {
                return opt.ammo == u.ammo_location && loc.can_reload_with( u.ammo_location, false );
            };
            auto it = std::find_if( ammo_list.begin(), ammo_list.end(), is_favorite_and_compatible );
            if( it != ammo_list.end() ) {
                return *it;
            }
        }
    } else {
        const_cast<item_location &>( u.ammo_location ) = item_location();
    }
    return u.select_ammo( loc, prompt, empty );
}

void game::reload( item_location &loc, bool prompt, bool empty )
{
    // bows etc. do not need to reload. select favorite ammo for them instead
    if( loc->has_flag( flag_RELOAD_AND_SHOOT ) ) {
        item::reload_option opt = u.select_ammo( loc, prompt );
        if( !opt ) {
            return;
        } else if( u.ammo_location && opt.ammo == u.ammo_location ) {
            u.add_msg_if_player( _( "Cleared ammo preferences for %s." ), loc->tname() );
            u.ammo_location = item_location();
        } else if( u.has_item( *opt.ammo ) ) {
            u.add_msg_if_player( _( "Selected %s as default ammo for %s." ), opt.ammo->tname(), loc->tname() );
            u.ammo_location = opt.ammo;
        } else {
            u.add_msg_if_player( _( "You need to keep that ammo on you to select it as default ammo." ) );
        }
        return;
    }

    switch( u.rate_action_reload( *loc ) ) {
        case hint_rating::iffy:
            if( ( loc->is_ammo_container() || loc->is_magazine() ) && loc->ammo_remaining( ) > 0 &&
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

            [[fallthrough]];

        case hint_rating::cant:
            add_msg( m_info, _( "You can't reload a %s!" ), loc->tname() );
            return;

        case hint_rating::good:
            break;
    }

    if( !u.has_item( *loc ) && !loc->has_flag( flag_ALLOWS_REMOTE_USE ) ) {
        if( !avatar_action::check_stealing( get_player_character(), *loc.get_item() ) ) {
            return;
        }
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

    item::reload_option opt = favorite_ammo_or_select( u, loc, empty, prompt );

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
    map &here = get_map();

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
    const optional_vpart_position ovp = here.veh_at( u.pos_abs() );
    if( ovp ) {
        const turret_data turret = ovp->vehicle().turret_query( ovp->pos_abs( ) );
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
        const std::string dist_text = string_format( _( "%d tiles" ), rl_dist( u.pos_bub(),
                                      mon->pos_bub() ) );
        //~ %s: Cardinal/ordinal direction ("east")
        const std::string dir_text = string_format( _( "to the %s" ),
                                     colorize( direction_name( direction_from( u.pos_bub(), mon->pos_bub() ) ), dir_color ) );
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
            min_dist = std::min( min_dist, rl_dist( u.pos_bub(), mon->pos_bub() ) );
            max_dist = std::max( min_dist, rl_dist( u.pos_bub(), mon->pos_bub() ) );
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
            return direction_from( u.pos_bub(), mon->pos_bub() );
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

bool game::disable_robot( const tripoint_bub_ms &p )
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
            u.assign_activity( disable_activity_actor( p, disable_activity_actor::get_disable_turns(),
                               true ) );
        }
    }
    return false;
}

bool game::is_dangerous_tile( const tripoint_bub_ms &dest_loc ) const
{
    return !get_dangerous_tile( dest_loc, 1 ).empty();
}

bool game::prompt_dangerous_tile( const tripoint_bub_ms &dest_loc,
                                  std::vector<std::string> *harmful_stuff ) const
{
    map &here = get_map();

    if( u.has_effect( effect_stunned ) || u.has_effect( effect_psi_stunned ) ) {
        return true;
    }

    if( harmful_stuff == nullptr ) {
        auto dangerous_tile = get_dangerous_tile( dest_loc );
        harmful_stuff = &dangerous_tile;
    }

    if( !harmful_stuff->empty() &&
        !query_yn( _( "Really step into %s?" ), enumerate_as_string( *harmful_stuff ) ) ) {
        return false;
    }
    if( !harmful_stuff->empty() && u.is_mounted() && here.is_open_air( dest_loc ) ) {
        add_msg( m_warning, _( "Your %s refuses to move over that ledge!" ),
                 u.mounted_creature->get_name() );
        return false;
    }
    return true;
}

std::vector<std::string> game::get_dangerous_tile( const tripoint_bub_ms &dest_loc,
        const size_t max ) const
{
    map &here = get_map();

    if( u.is_blind() ) {
        return {}; // blinded players don't see dangerous tiles
    }

    std::vector<std::string> harmful_stuff;
    const field fields_here = here.field_at( u.pos_bub() );
    const auto veh_here = here.veh_at( u.pos_bub() ).part_with_feature( "BOARDABLE", true );
    const auto veh_dest = here.veh_at( dest_loc ).part_with_feature( "BOARDABLE", true );
    const bool veh_here_inside = veh_here && veh_here->is_inside();
    const bool veh_dest_inside = veh_dest && veh_dest->is_inside();

    for( const std::pair<const field_type_id, field_entry> &e : here.field_at( dest_loc ) ) {
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
        if( harmful_stuff.size() == max ) {
            return harmful_stuff;
        }
    }

    if( here.is_open_air( dest_loc ) ) {
        if( !veh_dest && !u.has_effect_with_flag( json_flag_LEVITATION ) ) {
            harmful_stuff.emplace_back( "ledge" );
            if( harmful_stuff.size() == max ) {
                return harmful_stuff;
            }
        }
    }

    const trap &tr = here.tr_at( dest_loc );

    if( tr.can_see( dest_loc, u ) && !tr.is_benign() && !veh_dest ) {
        harmful_stuff.push_back( tr.name() );
        if( harmful_stuff.size() == max ) {
            return harmful_stuff;
        }
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

    if( here.has_flag( ter_furn_flag::TFLAG_ROUGH, dest_loc ) &&
        !here.has_flag( ter_furn_flag::TFLAG_ROUGH, u.pos_bub() ) &&
        !u.has_flag( json_flag_ALL_TERRAIN_NAVIGATION ) &&
        !veh_dest &&
        ( u.get_armor_type( damage_bash, bodypart_id( "foot_l" ) ) < 5 ||
          u.get_armor_type( damage_bash, bodypart_id( "foot_r" ) ) < 5 ) ) { // NOLINT(bugprone-branch-clone)
        harmful_stuff.emplace_back( here.name( dest_loc ) );
    } else if( here.has_flag( ter_furn_flag::TFLAG_SHARP, dest_loc ) &&
               !here.has_flag( ter_furn_flag::TFLAG_SHARP, u.pos_bub() ) &&
               !u.has_flag( json_flag_ALL_TERRAIN_NAVIGATION ) &&
               !( u.in_vehicle || here.veh_at( dest_loc ) ) &&
               u.dex_cur < 78 &&
               !( u.is_mounted() &&
                  u.mounted_creature->get_armor_type( damage_cut, bodypart_id( "torso" ) ) >= 10 ) &&
               !std::all_of( sharp_bps.begin(), sharp_bps.end(), sharp_bp_check ) ) {
        harmful_stuff.push_back( here.name( dest_loc ) );
    }

    return harmful_stuff;
}

bool game::walk_move( const tripoint_bub_ms &dest_loc, const bool via_ramp,
                      const bool furniture_move )
{
    if( u.has_flag( json_flag_PHASE_MOVEMENT ) ) {
        place_player( dest_loc );
        return true; // debug trait immunity to gravity, walls etc
    }
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    const tripoint_abs_ms dest_loc_abs = here.get_abs( dest_loc );

    if( here.has_flag_ter( ter_furn_flag::TFLAG_SMALL_PASSAGE, dest_loc ) ) {
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

    const int ramp_adjust = via_ramp ? pos.z() : dest_loc.z();
    const float dest_light_level = here.ambient_light_at( tripoint_bub_ms( point_bub_ms(
                                       dest_loc.xy() ), ramp_adjust ) );

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
    const optional_vpart_position &vp_here = here.veh_at( pos );
    const optional_vpart_position &vp_there = here.veh_at( dest_loc );
    const optional_vpart_position &vp_grab = here.veh_at( pos + u.grab_point );
    const vehicle *grabbed_vehicle = veh_pointer_or_null( vp_grab );

    bool pushing = false; // moving -into- grabbed tile; skip check for move_cost > 0
    bool pulling = false; // moving -away- from grabbed tile; check for move_cost > 0
    bool shifting_furniture = false; // moving furniture and staying still; skip check for move_cost > 0

    const tripoint_bub_ms furn_pos = pos + u.grab_point;
    const tripoint_bub_ms furn_dest = dest_loc + u.grab_point.xy();

    bool grabbed = u.get_grab_type() != object_type::NONE;
    if( grabbed ) {
        const tripoint_rel_ms dp = dest_loc - pos;
        pushing = dp.xy() == u.grab_point.xy();
        pulling = dp.xy() == -u.grab_point.xy();
    }

    // Now make sure we're actually holding something
    if( grabbed && u.get_grab_type() == object_type::FURNITURE ) {
        // We only care about shifting, because it's the only one that can change our destination
        if( here.has_furn( pos + u.grab_point ) ) {
            shifting_furniture = !pushing && !pulling;
        } else {
            // We were grabbing a furniture that isn't there
            grabbed = false;
        }
    } else if( grabbed && u.get_grab_type() == object_type::VEHICLE ) {
        if( !vp_grab ) {
            // We were grabbing a vehicle that isn't there anymore
            grabbed = false;
        }
        //can't board vehicle with solid parts while grabbing it
        else if( vp_there && !pushing && !here.impassable( dest_loc ) &&
                 !empty( grabbed_vehicle->get_avail_parts( VPFLAG_OBSTACLE ) ) &&
                 &vp_there->vehicle() == grabbed_vehicle ) {
            add_msg( m_warning, _( "You move into the %s, releasing it." ), grabbed_vehicle->name );
            u.grab( object_type::NONE );
        }
    } else if( grabbed && u.get_grab_type() != object_type::FURNITURE_ON_VEHICLE ) {
        // We were grabbing something WEIRD, let's pretend we weren't
        grabbed = false;
    }
    if( u.grab_point != tripoint_rel_ms::zero && !grabbed && !furniture_move ) {
        add_msg( m_warning, _( "Can't find grabbed object." ) );
        u.grab( object_type::NONE );
    }

    const std::vector<field_type_id> impassable_field_ids = here.get_impassable_field_type_ids_at(
                dest_loc );

    if( ( !here.passable_skip_fields( dest_loc ) || ( !impassable_field_ids.empty() &&
            !u.is_immune_fields( impassable_field_ids ) ) ) && !pushing && !shifting_furniture ) {
        if( vp_there && u.mounted_creature && u.mounted_creature->has_flag( mon_flag_RIDEABLE_MECH ) &&
            vp_there->vehicle().handle_potential_theft( u ) ) {
            tripoint_rel_ms diff = dest_loc - pos;
            if( diff.x() < 0 ) {
                diff.x() -= 2;
            } else if( diff.x() > 0 ) {
                diff.x() += 2;
            }
            if( diff.y() < 0 ) {
                diff.y() -= 2;
            } else if( diff.y() > 0 ) {
                diff.y() += 2;
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

    std::vector<std::string> harmful_stuff = get_dangerous_tile( dest_loc );
    if( !shifting_furniture && !pushing && !harmful_stuff.empty() ) {
        if( harmful_stuff.size() == 1 && harmful_stuff[0] == "ledge" ) {
            iexamine::ledge( u, tripoint_bub_ms( dest_loc ) );
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "ALWAYS" &&
                   !prompt_dangerous_tile( dest_loc, &harmful_stuff ) ) {
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "RUNNING" &&
                   ( !u.is_running() || !prompt_dangerous_tile( dest_loc, &harmful_stuff ) ) ) {
            add_msg( m_warning,
                     _( "Stepping into that %1$s looks risky.  Run into it if you wish to enter anyway." ),
                     enumerate_as_string( harmful_stuff ) );
            return true;
        } else if( get_option<std::string>( "DANGEROUS_TERRAIN_WARNING_PROMPT" ) == "CROUCHING" &&
                   ( !u.is_crouching() || !prompt_dangerous_tile( dest_loc, &harmful_stuff ) ) ) {
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
    const int mcost_from = here.move_cost( pos ); //calculate this _before_ calling grabbed_move

    int modifier = 0;
    if( grabbed && u.get_grab_type() == object_type::FURNITURE &&
        ( pos + u.grab_point ) == dest_loc ) {
        modifier = -here.furn( dest_loc ).obj().movecost;
    }

    const int mcost = here.combined_movecost( pos, dest_loc, grabbed_vehicle,
                      modifier,
                      via_ramp, false, !impassable_field_ids.empty() && u.is_immune_fields( impassable_field_ids ) );

    if( !furniture_move && grabbed_move( dest_loc - pos, via_ramp ) ) {
        return true;
    } else if( mcost == 0 ) {
        return false;
    }
    bool diag = trigdist && pos.x() != dest_loc.x() && pos.y() != dest_loc.y();
    const int previous_moves = u.get_moves();
    if( u.is_mounted() ) {
        auto *crit = u.mounted_creature.get();
        if( !crit->has_flag( mon_flag_RIDEABLE_MECH ) &&
            ( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_MOUNTABLE, dest_loc ) ||
              here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR, dest_loc ) ||
              here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE, dest_loc ) ||
              here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR_DAMAGED, dest_loc ) ||
              here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_BARRICADABLE_DOOR_REINFORCED, dest_loc ) ) ) {
            add_msg( m_warning, _( "You cannot pass obstacles whilst mounted." ) );
            return false;
        }
        const double base_moves = u.run_cost( mcost, diag ) * 100.0 / crit->get_speed();
        const double encumb_moves = u.get_weight() / 4800.0_gram;
        u.mod_moves( -static_cast<int>( std::ceil( base_moves + encumb_moves ) ) );
        crit->use_mech_power( u.current_movement_mode()->mech_power_use() );
    } else {
        u.mod_moves( -u.run_cost( mcost, diag ) );
        /**
        TODO:
        This should really use the mounted creatures stamina, if mounted.
        Monsters don't currently have stamina however.
        For the time being just don't burn players stamina when mounted.
        */
        if( grabbed_vehicle == nullptr || grabbed_vehicle->wheelcache.empty() ) {
            //Burn normal amount of stamina if no vehicle grabbed or vehicle lacks wheels
            u.burn_move_stamina( previous_moves - u.get_moves() );
        } else {
            //Burn half as much stamina if vehicle has wheels, without changing move time
            u.burn_move_stamina( 0.50 * ( previous_moves - u.get_moves() ) );
        }
    }
    // Max out recoil & reset aim point
    u.recoil = MAX_RECOIL;
    u.last_target_pos = std::nullopt;

    // Print a message if movement is slow
    const int mcost_to = here.move_cost( dest_loc ); //calculate this _after_ calling grabbed_move
    const bool fungus = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS, pos ) ||
                        here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FUNGUS,
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
                add_msg( m_warning, _( "Moving onto this %s is slow!" ), here.name( dest_loc ) );
                if( here.has_furn( dest_loc ) ) {
                    sfx::do_obstacle( here.furn( dest_loc ).id().str() );
                } else {
                    sfx::do_obstacle( here.ter( dest_loc ).id().str() );
                }
            }
        } else {
            if( auto displayed_part = vp_here.part_displayed() ) {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ),
                         displayed_part->part().name() );
                sfx::do_obstacle( displayed_part->part().info().id.str() );
            } else {
                add_msg( m_warning, _( "Moving off of this %s is slow!" ), here.name( pos ) );
                if( here.has_furn( pos ) ) {
                    sfx::do_obstacle( here.furn( pos ).id().str() );
                } else {
                    sfx::do_obstacle( here.ter( pos ).id().str() );
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
        if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, dest_loc ) &&
            one_in( 80 + u.dex_cur + u.int_cur ) ) {
            add_msg( _( "Your tentacles stick to the ground, but you pull them free." ) );
            u.mod_sleepiness( 1 );
        }
    }

    u.make_footstep_noise();

    //only clatter items every so often based on activity level
    if( to_turns<int>( calendar::turn - u.last_pocket_noise ) > std::max( static_cast<int>
            ( 10 - u.activity_level() ), 1 ) ) {
        u.make_clatter_sound();
        u.last_pocket_noise = calendar::turn;
    }

    if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_HIDE_PLACE, dest_loc ) ) {
        add_msg( m_good, _( "You are hiding in the %s." ), here.name( dest_loc ) );
    }

    tripoint_bub_ms oldpos = pos;
    tripoint_abs_ms old_abs_pos = here.get_abs( oldpos );

    bool moving = dest_loc != oldpos;

    point_rel_sm submap_shift = place_player( dest_loc );
    point_rel_ms ms_shift = coords::project_to<coords::ms>( submap_shift );
    oldpos = oldpos - ms_shift.raw();

    if( moving ) {
        cata_event_dispatch::avatar_moves( old_abs_pos, u, here );

        // Add trail animation when sprinting
        if( get_option<bool>( "ANIMATIONS" ) && u.is_running() ) {
            if( pos.y() < oldpos.y() ) {
                if( pos.x() < oldpos.x() ) {
                    draw_async_anim( oldpos, "run_nw", "\\", c_light_gray );
                } else if( pos.x() == oldpos.x() ) {
                    draw_async_anim( oldpos, "run_n", "|", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_ne", "/", c_light_gray );
                }
            } else if( pos.y() == oldpos.y() ) {
                if( pos.x() < oldpos.x() ) {
                    draw_async_anim( oldpos, "run_w", "-", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_e", "-", c_light_gray );
                }
            } else {
                if( pos.x() < oldpos.x() ) {
                    draw_async_anim( oldpos, "run_sw", "/", c_light_gray );
                } else if( pos.x() == oldpos.x() ) {
                    draw_async_anim( oldpos, "run_s", "|", c_light_gray );
                } else {
                    draw_async_anim( oldpos, "run_se", "\\", c_light_gray );
                }
            }
        }
    }

    if( furniture_move ) {
        // Adjust the grab_point if player has changed z level.
        u.grab_point.z() -= pos.z() - oldpos.z();
    }

    if( grabbed_vehicle ) {
        // Vehicle might be at different z level than the grabbed part.
        u.grab_point.z() = vp_grab->pos_abs().z() - pos.z();
    }

    if( pulling ) {
        const tripoint_bub_ms shifted_furn_pos = furn_pos - ms_shift;
        const tripoint_bub_ms shifted_furn_dest = furn_dest - ms_shift;
        const time_duration fire_age = here.get_field_age( shifted_furn_pos, fd_fire );
        const int fire_intensity = here.get_field_intensity( shifted_furn_pos, fd_fire );
        here.remove_field( shifted_furn_pos, fd_fire );
        here.set_field_intensity( shifted_furn_dest, fd_fire, fire_intensity );
        here.set_field_age( shifted_furn_dest, fd_fire, fire_age );
    }

    if( u.is_hauling() ) {
        start_hauling( oldpos );
    }

    if( u.will_be_cramped_in_vehicle_tile( here, dest_loc_abs ) ) {
        if( u.get_size() == creature_size::huge ) {
            add_msg( m_warning, _( "You barely fit in this tiny human vehicle." ) );
        } else if( u.get_total_volume() > u.get_base_volume() ) {
            add_msg( m_warning,
                     _( "All the stuff you're carrying isn't making it any easier to move in here." ) );
        }
        u.add_effect( effect_cramped_space, 2_turns, true );
    }

    on_move_effects();

    return true;
}


static void autopulp_or_butcher( avatar &u )
{

    const std::string pulp_butcher = get_option<std::string>( "AUTO_PULP_BUTCHER" );

    if( pulp_butcher == "off" ) {
        return;
    }

    enum class AUTO_AREA : int {
        CENTER,
        ADJACENT
    };

    enum class AUTO_TYPE : int {
        BUTCHER_QUICK,
        PULP,
        PULP_OR_DISMEMBER
    };

    enum class AUTO_WHO : int {
        ALL,
        ZOMBIES
    };

    map &here = get_map();
    AUTO_AREA area{};
    AUTO_TYPE type{};
    AUTO_WHO who{};

    // probably replace this with enum
    if( pulp_butcher == "butcher" ) {
        area = AUTO_AREA::CENTER;
        type = AUTO_TYPE::BUTCHER_QUICK;
        who = AUTO_WHO::ALL;
    } else if( pulp_butcher == "pulp" ) {
        area = AUTO_AREA::CENTER;
        type = AUTO_TYPE::PULP;
        who = AUTO_WHO::ALL;
    } else if( pulp_butcher == "pulp_adjacent" ) {
        area = AUTO_AREA::ADJACENT;
        type = AUTO_TYPE::PULP;
        who = AUTO_WHO::ALL;
    } else if( pulp_butcher == "pulp_zombie_only" ) {
        area = AUTO_AREA::CENTER;
        type = AUTO_TYPE::PULP;
        who = AUTO_WHO::ZOMBIES;
    } else if( pulp_butcher == "pulp_adjacent_zombie_only" ) {
        area = AUTO_AREA::ADJACENT;
        type = AUTO_TYPE::PULP;
        who = AUTO_WHO::ZOMBIES;
    } else if( pulp_butcher == "pulp_or_dismember" ) {
        area = AUTO_AREA::CENTER;
        type = AUTO_TYPE::PULP_OR_DISMEMBER;
        who = AUTO_WHO::ALL;
    } else if( pulp_butcher == "pulp_or_dismember_adjacent" ) {
        area = AUTO_AREA::ADJACENT;
        type = AUTO_TYPE::PULP_OR_DISMEMBER;
        who = AUTO_WHO::ALL;
    } else if( pulp_butcher == "pulp_or_dismember_zombies_only" ) {
        area = AUTO_AREA::CENTER;
        type = AUTO_TYPE::PULP_OR_DISMEMBER;
        who = AUTO_WHO::ZOMBIES;
    } else if( pulp_butcher == "pulp_or_dismember_zombies_only_adjacent" ) {
        area = AUTO_AREA::ADJACENT;
        type = AUTO_TYPE::PULP_OR_DISMEMBER;
        who = AUTO_WHO::ZOMBIES;
    }

    std::vector<tripoint_bub_ms> target;
    switch( area ) {
        case AUTO_AREA::CENTER: {
            target.push_back( u.pos_bub() );
            break;
        }
        case AUTO_AREA::ADJACENT: {
            target.reserve( eight_horizontal_neighbors.size() );
            for( const tripoint &dir : eight_horizontal_neighbors ) {
                target.emplace_back( u.pos_bub() + dir );
            }
            break;
        }
    }

    std::vector<item_location> corpses;
    for( const tripoint_bub_ms &pos : target ) {
        for( item &it : here.i_at( pos ) ) {
            // skip not corpses or corpses that are already pulped
            if( !it.can_revive() ) {
                continue;
            }
            if( who == AUTO_WHO::ZOMBIES && !it.get_mtype()->has_flag( mon_flag_REVIVES ) ) {
                continue;
            }
            const item_location corpse_loc( map_cursor( pos ), &it );
            corpses.push_back( corpse_loc );
        }
    }

    if( corpses.empty() ) {
        return;
    }
    switch( type ) {
        case AUTO_TYPE::BUTCHER_QUICK: {
            std::vector<butchery_data> butchery_list;
            for( item_location corpse : corpses ) {
                butchery_data bd( corpse, butcher_type::QUICK );
                butchery_list.emplace_back( bd );
            }
            if( !butchery_list.empty() ) {
                u.assign_activity( butchery_activity_actor( butchery_list ) );
            }
            return;
        }
        case AUTO_TYPE::PULP: {
            if( !corpses.empty() ) {
                u.assign_activity( pulp_activity_actor( corpses ) );
            }
            return;
        }
        case AUTO_TYPE::PULP_OR_DISMEMBER: {

            std::vector<item_location> to_be_pulped;
            std::vector<butchery_data> to_be_butchered;
            to_be_pulped.reserve( corpses.size() );
            to_be_butchered.reserve( corpses.size() );
            for( item_location &loc : corpses ) {
                if( g->can_pulp_corpse( u, *loc.get_item()->get_mtype() ) ) {
                    to_be_pulped.emplace_back( loc );
                } else {
                    butchery_data bd( loc, butcher_type::DISMEMBER );
                    to_be_butchered.emplace_back( bd );
                }
            }

            // what happens right now is that whatever activity is assigned last is one that actually fires
            // it would be nice to make it to store activity in backlog and fire from it
            // (doesn't work because in this case activity starts from the middle, bypassing ::start() function)
            // or some another reasonable alternative
            // but this would suffice
            if( !to_be_butchered.empty() ) {
                u.assign_activity( player_activity( butchery_activity_actor( to_be_butchered ) ) );
            }
            if( !to_be_pulped.empty() ) {
                u.assign_activity( player_activity( pulp_activity_actor( to_be_pulped ) ) );
            }
            return;
        }
    }
}

point_rel_sm game::place_player( const tripoint_bub_ms &dest_loc, bool quick )
{
    map &here = get_map();
    const optional_vpart_position vp1 = here.veh_at( dest_loc );
    if( const std::optional<std::string> label = vp1.get_label() ) {
        add_msg( m_info, _( "Label here: %s" ), *label );
    }
    std::string signage = here.get_signage( dest_loc );
    if( !signage.empty() ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "The sign says: %s" ), signage );
        } else {
            add_msg( m_info, _( "There is a sign here, but you are unable to read it." ) );
        }
    }
    if( here.has_graffiti_at( dest_loc ) ) {
        if( !u.has_trait( trait_ILLITERATE ) ) {
            add_msg( m_info, _( "Written here: %s" ), here.graffiti_at( dest_loc ) );
        } else {
            add_msg( m_info, _( "Something is written here, but you are unable to read it." ) );
        }
    }
    // TODO: Move the stuff below to a Character method so that NPCs can reuse it
    if( here.has_flag( ter_furn_flag::TFLAG_ROUGH, dest_loc ) && ( !u.in_vehicle ) &&
        ( !u.is_mounted() ) && !u.has_flag( json_flag_ALL_TERRAIN_NAVIGATION ) ) {
        if( one_in( 5 ) && u.get_armor_type( damage_bash, bodypart_id( "foot_l" ) ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your left foot on the %s!" ),
                     here.has_flag_ter( ter_furn_flag::TFLAG_ROUGH,
                                        dest_loc ) ? here.tername( dest_loc ) : here.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bodypart_id( "foot_l" ), damage_instance( damage_cut, 1 ) );
        }
        if( one_in( 5 ) && u.get_armor_type( damage_bash, bodypart_id( "foot_r" ) ) < rng( 2, 5 ) ) {
            add_msg( m_bad, _( "You hurt your right foot on the %s!" ),
                     here.has_flag_ter( ter_furn_flag::TFLAG_ROUGH,
                                        dest_loc ) ? here.tername( dest_loc ) : here.furnname(
                         dest_loc ) );
            u.deal_damage( nullptr, bodypart_id( "foot_l" ), damage_instance( damage_cut, 1 ) );
        }
    }
    ///\EFFECT_DEX increases chance of avoiding cuts on sharp terrain
    if( here.has_flag( ter_furn_flag::TFLAG_SHARP, dest_loc ) && !one_in( 3 ) &&
        !u.has_flag( json_flag_ALL_TERRAIN_NAVIGATION ) &&
        !x_in_y( 1 + u.dex_cur / 2.0, 40 ) &&
        ( !u.in_vehicle && !here.veh_at( dest_loc ) ) && ( !u.has_proficiency( proficiency_prof_parkour ) ||
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
                             here.has_flag_ter( ter_furn_flag::TFLAG_SHARP,
                                                dest_loc ) ? here.tername( dest_loc ) : here.furnname(
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
    if( here.has_flag( ter_furn_flag::TFLAG_UNSTABLE, dest_loc ) &&
        !u.is_mounted() && !here.has_vehicle_floor( dest_loc ) ) {
        u.add_effect( effect_bouldering, 1_turns, true );
    } else if( u.has_effect( effect_bouldering ) ) {
        u.remove_effect( effect_bouldering );
    }
    if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_SIGHT, dest_loc ) ) {
        u.add_effect( effect_no_sight, 1_turns, true );
    } else if( u.has_effect( effect_no_sight ) ) {
        u.remove_effect( effect_no_sight );
    }

    // If we moved out of the nonant, we need update our map data
    if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, dest_loc ) && u.has_effect( effect_onfire ) ) {
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
                std::vector<tripoint_bub_ms> maybe_valid;
                for( const tripoint_bub_ms &jk : here.points_in_radius( critter.pos_bub(), 1 ) ) {
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
                    return point_rel_sm::zero;
                }
            } else {
                // Force the movement even though the player is there right now.
                const bool moved = critter.move_to( u.pos_bub(), /*force=*/false, /*step_on_critter=*/true );
                if( moved ) {
                    add_msg( _( "You displace the %s." ), critter.name() );
                } else {
                    add_msg( _( "You cannot move the %s out of the way." ), critter.name() );
                    return point_rel_sm::zero;
                }
            }
        } else if( !u.has_effect( effect_riding ) ) {
            add_msg( _( "You cannot move the %s out of the way." ), critter.name() );
            return point_rel_sm::zero;
        }
    }

    // If the player is in a vehicle, unboard them from the current part
    bool was_in_control_same_pos = false;
    if( u.in_vehicle ) {
        if( u.pos_bub() == dest_loc ) {
            was_in_control_same_pos = true;
        } else {
            here.unboard_vehicle( u.pos_bub() );
        }
    }
    // Move the player
    // Start with z-level, to make it less likely that old functions (2D ones) freak out
    bool z_level_changed = false;
    if( dest_loc.z() != here.get_abs_sub().z() ) {
        z_level_changed = vertical_shift( dest_loc.z() );
    }

    if( u.is_hauling() && ( !here.can_put_items( dest_loc ) ||
                            here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, dest_loc ) ||
                            vp1 ) ) {
        u.stop_hauling();
    }
    u.setpos( here, dest_loc );
    if( u.is_mounted() ) {
        monster *mon = u.mounted_creature.get();
        mon->setpos( here, dest_loc );
        mon->process_triggers();
        here.creature_in_field( *mon );
    }
    point_rel_sm submap_shift = update_map( u, z_level_changed );
    // Important: don't use dest_loc after this line. `update_map` may have shifted the map
    // and dest_loc was not adjusted and therefore is still in the un-shifted system and probably wrong.
    // If you must use it you can calculate the position in the new, shifted system with
    // adjusted_pos = ( old_pos.x - submap_shift.x * SEEX, old_pos.y - submap_shift.y * SEEY, old_pos.z )

    //Auto pulp or butcher and Auto foraging
    if( !quick && get_option<bool>( "AUTO_FEATURES" )  && mostseen == 0 && !u.is_mounted() ) {
        static constexpr std::array<direction, 8> adjacentDir = {
            direction::NORTH, direction::NORTHEAST, direction::EAST, direction::SOUTHEAST,
            direction::SOUTH, direction::SOUTHWEST, direction::WEST, direction::NORTHWEST
        };

        const std::string forage_type = get_option<std::string>( "AUTO_FORAGING" );
        if( forage_type != "off" ) {
            const auto forage = [&]( const tripoint_bub_ms & pos ) {
                const ter_t &xter_t = *here.ter( pos );
                const furn_t &xfurn_t = *here.furn( pos );
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
                forage( u.pos_bub() + displace_XY( elem ) );
            }
        }

        autopulp_or_butcher( u );
    }

    // Auto pickup
    if( !quick && !u.is_mounted() && get_option<bool>( "AUTO_PICKUP" ) && !u.is_hauling() &&
        ( !get_option<bool>( "AUTO_PICKUP_SAFEMODE" ) ||
          !u.get_mon_visible().has_dangerous_creature_in_proximity ) &&
        ( here.has_items( u.pos_bub() ) || get_option<bool>( "AUTO_PICKUP_ADJACENT" ) ) ) {
        Pickup::autopickup( u.pos_bub() );
    }

    // If the new tile is a boardable part, board it
    if( !was_in_control_same_pos && vp1.part_with_feature( "BOARDABLE", true ) && !u.is_mounted() ) {
        here.board_vehicle( u.pos_bub(), &u );
    }

    // Traps!
    // Try to detect.
    u.search_surroundings();
    if( u.is_mounted() ) {
        here.creature_on_trap( *u.mounted_creature );
    } else {
        here.creature_on_trap( u );
    }
    // Drench the player if swimmable
    if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, u.pos_bub() ) &&
        !here.has_flag_furn( "BRIDGE", u.pos_bub() ) &&
        !( u.is_mounted() || ( u.in_vehicle && vp1->vehicle().can_float( here ) ) ) &&
        !u.has_flag( json_flag_WATERWALKING ) ) {
        u.drench( 80, u.get_drenching_body_parts( false, false ),
                  false );
    }

    // List items here
    if( !quick && !here.has_flag( ter_furn_flag::TFLAG_SEALED, u.pos_bub() ) ) {
        if( get_option<bool>( "NO_AUTO_PICKUP_ZONES_LIST_ITEMS" ) ||
            !check_zone( zone_type_NO_AUTO_PICKUP, u.pos_bub() ) ) {
            const map_stack &ms = here.i_at( u.pos_bub() );
            if( u.is_blind() && !ms.empty() ) {
                add_msg( _( "There's something here, but you can't see what it is." ) );
            } else if( here.has_items( u.pos_bub() ) ) {
                std::vector<std::string> names;
                std::vector<size_t> counts;
                std::vector<item> items;
                for( const item &tmpitem : ms ) {

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
    map &here = get_map();

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
        here.unboard_vehicle( u.pos_bub() );
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        here.clear_vehicle_list( z );
    }
    here.rebuild_vehicle_level_caches();
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_dec.reset();
    here.access_cache( here.get_abs_sub().z() ).map_memory_cache_ter.reset();
    // offset because load_map expects the coordinates of the top left corner, but the
    // player will be centered in the middle of the map.
    const tripoint_abs_sm map_sm_pos =
        project_to<coords::sm>( om_dest ) - point( HALF_MAPSIZE, HALF_MAPSIZE );
    const tripoint_bub_ms player_pos( u.pos_bub().xy(), map_sm_pos.z() );
    load_map( map_sm_pos );
    load_npcs();
    here.spawn_monsters( true ); // Static monsters
    update_overmap_seen();
    // update weather now as it could be different on the new location
    weather.set_nextweather( calendar::turn );
    if( move_player ) {
        place_player( player_pos );
    }
}

bool game::phasing_move( const tripoint_bub_ms &dest_loc, const bool via_ramp )
{
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    const units::energy trigger_cost = bio_probability_travel->power_trigger;

    if( !u.has_active_bionic( bio_probability_travel ) ||
        u.get_power_level() < trigger_cost ) {
        return false;
    }

    if( dest_loc.z() != pos.z() && !via_ramp ) {
        // No vertical phasing yet
        return false;
    }

    //probability travel through walls but not water
    tripoint_bub_ms dest = dest_loc;
    // tile is impassable
    int tunneldist = 0;
    const point_rel_ms d( sgn( dest.x() - pos.x() ), sgn( dest.y() - pos.y() ) );
    creature_tracker &creatures = get_creature_tracker();
    while( here.impassable( dest ) ||
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
        dest.raw() += d.raw(); // TODO: Make += etc. work with relative coordinates.
    }

    if( tunneldist != 0 ) {
        if( u.in_vehicle ) {
            here.unboard_vehicle( pos );
        }

        add_msg( _( "You quantum tunnel through the %d-tile wide barrier!" ), tunneldist );
        //tunneling costs 250 bionic power per impassable tile
        u.mod_power_level( -( tunneldist * trigger_cost ) );
        u.mod_moves( -to_moves<int>( 1_seconds ) ); //tunneling takes exactly one second
        u.setpos( here, dest );

        if( here.veh_at( pos ).part_with_feature( "BOARDABLE", true ) ) {
            here.board_vehicle( pos, &u );
        }

        u.grab( object_type::NONE );
        on_move_effects();
        here.creature_on_trap( u );
        return true;
    }

    return false;
}

bool game::phasing_move_enchant( const tripoint_bub_ms &dest_loc, const int phase_distance )
{
    map &here = get_map();
    const tripoint_bub_ms pos = u.pos_bub( here );

    if( phase_distance < 1 ) {
        return false;
    }

    // phasing only applies to impassible tiles such as walls

    int tunneldist = 0;
    tripoint_bub_ms dest = dest_loc;
    const tripoint_rel_ms d( sgn( dest.x() - pos.x() ), sgn( dest.y() - pos.y() ),
                             sgn( dest.z() - pos.z() ) );
    creature_tracker &creatures = get_creature_tracker();

    while( here.impassable( dest ) ||
           ( creatures.creature_at( dest ) != nullptr && tunneldist > 0 ) ) {
        // add 1 to tunnel distance for each impassable tile in the line
        tunneldist += 1;
        if( tunneldist > phase_distance ) {
            return false;
        }
        if( tunneldist > 48 ) {
            return false;
        }
        dest += d;
    }

    // vertical handling for adjacent tiles
    if( d.z() != 0 && !here.impassable( dest_loc ) && tunneldist == 0 ) {
        tunneldist += 1;
    }

    if( tunneldist != 0 ) {
        if( u.in_vehicle ) {
            here.unboard_vehicle( pos );
        }

        if( dest.z() != pos.z() ) {
            // calling vertical_shift here doesn't actually move the character for some reason, but it does perform other necessary tasks for vertical movement
            vertical_shift( dest.z() );
        }

        u.setpos( here, dest );

        if( here.veh_at( pos ).part_with_feature( "BOARDABLE", true ) ) {
            here.board_vehicle( pos, &u );
        }

        u.grab( object_type::NONE );
        on_move_effects();
        here.creature_on_trap( u );
        return true;
    }

    return false;
}

bool game::can_move_furniture( tripoint_bub_ms fdest, const tripoint_rel_ms &dp )
{
    map &here = get_map();

    const bool pulling_furniture = dp.xy() == -u.grab_point.xy();
    const bool has_floor = here.has_floor_or_water( fdest );
    creature_tracker &creatures = get_creature_tracker();
    bool is_ramp_or_road = here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, fdest ) ||
                           here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, fdest ) ||
                           here.has_flag( ter_furn_flag::TFLAG_ROAD, fdest );
    if( !here.passable( fdest ) ) {
        return false;
    }
    if( creatures.creature_at<npc>( fdest ) != nullptr ||
        creatures.creature_at<monster>( fdest ) != nullptr ) {
        return false;
    }
    if( !( !pulling_furniture || is_empty( u.pos_bub() + dp ) ) &&
        ( !has_floor || here.has_flag( ter_furn_flag::TFLAG_FLAT, fdest ) ||
          is_ramp_or_road ) ) {
        return false;
    }
    if( here.has_furn( fdest ) ) {
        return false;
    }
    if( here.veh_at( fdest ) ) {
        if( !here.veh_at( fdest )->can_load_furniture() ) {
            return false;
        }
    }
    if( !( !has_floor || here.tr_at( fdest ).is_null() ) ) {
        return false;
    }
    return true;
}

int game::grabbed_furn_move_time( const tripoint_rel_ms &dp )
{
    map &here = get_map();

    // Furniture: pull, push, or standing still and nudging object around.
    // Can push furniture out of reach.
    tripoint_bub_ms fpos = u.pos_bub() + u.grab_point;
    // supposed position of grabbed furniture
    if( !here.has_furn( fpos ) ) {
        return 0;
    }

    tripoint_bub_ms fdest = fpos + tripoint_rel_ms( dp.xy(), 0 ); // intended destination of furniture.

    const bool canmove = can_move_furniture( fdest, dp );
    const furn_t &furntype = here.furn( fpos ).obj();
    const map_stack &ms = here.i_at( fdest );
    const int dst_items = ms.size();

    const bool only_liquid_items = std::all_of( ms.begin(), ms.end(),
    [&]( const item & liquid_item ) {
        return liquid_item.made_of_from_type( phase_id::LIQUID );
    } );

    const bool dst_item_ok = !here.has_flag( ter_furn_flag::TFLAG_NOITEM, fdest ) &&
                             !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, fdest ) &&
                             !here.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, fdest ) &&
                             only_liquid_items;
    const furn_t &fo = here.furn( fpos ).obj();
    const bool src_item_ok = fo.has_flag( ter_furn_flag::TFLAG_CONTAINER ) ||
                             fo.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER ) ||
                             fo.has_flag( ter_furn_flag::TFLAG_SEALED );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( item &contained_item : here.i_at( fpos ) ) {
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

bool game::grabbed_furn_move( const tripoint_rel_ms &dp )
{
    map &here = get_map();
    // Furniture: pull, push, or standing still and nudging object around.
    // Can push furniture out of reach.
    tripoint_bub_ms fpos = ( u.pos_bub() + u.grab_point );
    // supposed position of grabbed furniture
    if( !here.has_furn( fpos ) ) {
        // Where did it go? We're grabbing thin air so reset.
        add_msg( m_info, _( "No furniture at grabbed point." ) );
        u.grab( object_type::NONE );
        return false;
    }

    int ramp_z_offset = 0;
    // Furniture could be on a ramp at different time than player so adjust for that.
    if( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, fpos + dp.xy() ) ) {
        ramp_z_offset = 1;
    } else if( here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, fpos + dp.xy() ) ) {
        ramp_z_offset = -1;
    }

    const bool pushing_furniture = dp.xy() ==  u.grab_point.xy();
    const bool pulling_furniture = dp.xy() == -u.grab_point.xy();
    const bool shifting_furniture = !pushing_furniture && !pulling_furniture;

    // Intended destination of furniture.
    const tripoint_bub_ms fdest = fpos + tripoint_rel_ms( dp.xy(), ramp_z_offset );

    // Unfortunately, game::is_empty fails for tiles we're standing on,
    // which will forbid pulling, so:
    const bool canmove = can_move_furniture( fdest, dp );
    // @TODO: it should be possible to move over invisible traps. This should probably
    // trigger the trap.
    // The current check (no move if trap) allows a player to detect invisible traps by
    // attempting to move stuff onto it.

    const furn_t furntype = here.furn( fpos ).obj();
    const int src_items = here.i_at( fpos ).size();
    const map_stack &dst_ms = here.i_at( fdest );
    const int dst_items = dst_ms.size();

    const bool only_liquid_items = std::all_of( dst_ms.begin(), dst_ms.end(),
    [&]( const item & liquid_item ) {
        return liquid_item.made_of_from_type( phase_id::LIQUID );
    } );

    const bool dst_item_ok = !here.has_flag( ter_furn_flag::TFLAG_NOITEM, fdest ) &&
                             !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, fdest ) &&
                             !here.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, fdest );

    const furn_t &fo = here.furn( fpos ).obj();
    const bool src_item_ok = fo.has_flag( ter_furn_flag::TFLAG_CONTAINER ) ||
                             fo.has_flag( ter_furn_flag::TFLAG_FIRE_CONTAINER ) ||
                             fo.has_flag( ter_furn_flag::TFLAG_SEALED );

    const int fire_intensity = here.get_field_intensity( fpos, fd_fire );
    time_duration fire_age = here.get_field_age( fpos, fd_fire );

    int str_req = furntype.move_str_req;
    // Factor in weight of items contained in the furniture.
    units::mass furniture_contents_weight = 0_gram;
    for( item &contained_item : here.i_at( fpos ) ) {
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

    if( here.veh_at( fdest ) ) {
        u.grab( object_type::NONE );
        here.veh_at( fdest )->part_with_feature( "FURNITURE_TIEDOWN", true )->part().load_furniture( here,
                fpos );
        here.furn_set( fpos, furn_str_id::NULL_ID(), true );
        here.veh_at( fdest )->vehicle().invalidate_mass();
        add_msg( _( "You load the furniture onto the vehicle." ) );
        tripoint_rel_ms new_grab_pt( fdest - ( u.pos_bub() +
                                               ( shifting_furniture ? tripoint_rel_ms::zero : dp ) ) );
        if( std::abs( new_grab_pt.x() ) < 2 && std::abs( new_grab_pt.y() ) < 2 ) {
            u.grab( object_type::FURNITURE_ON_VEHICLE, new_grab_pt );
        }
        return shifting_furniture;
    } else {
        // Actually move the furniture.
        here.furn_set( fdest, here.furn( fpos ), false, false, true );
        here.furn_set( fpos, furn_str_id::NULL_ID(), true );
    }

    if( fire_intensity == 1 && !pulling_furniture ) {
        here.remove_field( fpos, fd_fire );
        here.set_field_intensity( fdest, fd_fire, fire_intensity );
        here.set_field_age( fdest, fd_fire, fire_age );
    }

    // Is there is only liquids on the ground, remove them after moving furniture.
    if( dst_items > 0 && only_liquid_items ) {
        here.i_clear( fdest );
    }

    if( src_items > 0 ) { // Move the stuff inside.
        if( dst_item_ok && src_item_ok ) {
            // Assume contents of both cells are legal, so we can just swap contents.
            std::list<item> temp;
            map_stack ms = here.i_at( fpos );
            std::move( ms.begin(), ms.end(),
                       std::back_inserter( temp ) );
            here.i_clear( fpos );
            for( auto item_iter = ms.begin();
                 item_iter != ms.end(); ++item_iter ) {
                ms.insert( *item_iter );
            }
            here.i_clear( fdest );
            for( item &cur_item : temp ) {
                ms.insert( cur_item );
            }
        } else {
            add_msg( _( "Stuff spills from the %s!" ), furntype.name() );
        }
    }

    if( !here.has_floor_or_water( fdest ) && !here.has_flag( ter_furn_flag::TFLAG_FLAT, fdest ) ) {
        std::string danger_tile = enumerate_as_string( get_dangerous_tile( fdest ) );
        add_msg( _( "You let go of the %1$s as it falls down the %2$s." ), furntype.name(), danger_tile );
        u.grab( object_type::NONE );
        return true;
    }

    u.grab_point.z() += ramp_z_offset;

    if( shifting_furniture ) {
        // We didn't move
        tripoint_rel_ms d_sum = u.grab_point + dp;
        if( std::abs( d_sum.x() ) < 2 && std::abs( d_sum.y() ) < 2 ) {
            u.grab_point = d_sum; // furniture moved relative to us
        } else { // we pushed furniture out of reach
            add_msg( _( "You let go of the %s." ), furntype.name() );
            u.grab( object_type::NONE );
        }
        return true; // We moved furniture but stayed still.
    }

    if( pushing_furniture && here.impassable( fpos ) ) {
        // Not sure how that chair got into a wall, but don't let player follow.
        add_msg( _( "You let go of the %1$s as it slides past %2$s." ),
                 furntype.name(), here.tername( fdest ) );
        u.grab( object_type::NONE );
        return true;
    }

    return false;
}

bool game::grabbed_move( const tripoint_rel_ms &dp, const bool via_ramp )
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

    if( u.get_grab_type() == object_type::FURNITURE_ON_VEHICLE ) {
        u.assign_activity( move_furniture_on_vehicle_activity_actor( dp, via_ramp ) );
        return true;
    }

    add_msg( m_info, _( "Nothing at grabbed point %d,%d,%d or bad grabbed object type." ),
             u.grab_point.x(), u.grab_point.y(), u.grab_point.z() );
    u.grab( object_type::NONE );
    return false;
}

void game::on_move_effects()
{
    // TODO: Move this to a character method
    if( !u.is_mounted() ) {
        const item muscle( fuel_type_muscle );
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
    bool gear_waterproofed = ch.has_flag( json_flag_ITEM_WATERPROOFING );

    for( item_location &loc : ch.all_items_loc() ) {
        if( gear_waterproofed ) {
            break;
        }
        // check flag first because its cheaper
        if( loc->has_flag( flag_WATER_DISSOLVE ) && !loc.protected_from_liquids() ) {
            add_msg_if_player_sees( ch.pos_bub(), m_bad, _( "%1$s %2$s dissolved in the water!" ),
                                    ch.disp_name( true, true ), loc->display_name() );
            loc.remove_item();
        } else if( loc->has_flag( flag_WATER_BREAK ) && !loc->is_broken()
                   && !loc.protected_from_liquids() ) {

            add_msg_if_player_sees( ch.pos_bub(), m_bad, _( "The water destroyed %1$s %2$s!" ),
                                    ch.disp_name( true ), loc->display_name() );
            loc->deactivate();
            // TODO: Maybe different types of wet faults? But I can't think of any.
            // This just means it's still too wet to use.
            // TODO2: add fault types to electronics, remove `force` argument from here
            loc->set_random_fault_of_type( "wet", true );
            // An electronic item in water is also shorted.
            if( loc->has_flag( flag_ELECTRONIC ) ) {
                loc->set_random_fault_of_type( "shorted", true );
            }
        } else if( loc->has_flag( flag_WATER_BREAK_ACTIVE ) && !loc->is_broken()
                   && !loc.protected_from_liquids() ) {
            const int wetness_add = 5100 * std::log10( units::to_milliliter( loc->volume() ) );
            loc->wetness += wetness_add;
            loc->wetness = std::min( loc->wetness, 5 * wetness_add );
        } else if( loc->typeId() == itype_towel && !loc.protected_from_liquids() ) {
            loc->convert( itype_towel_wet, &ch ).active = true;
        }
    }
}

bool game::fling_creature( Creature *c, const units::angle &dir, float flvel, bool controlled,
                           bool intentional )
{
    map &here = get_map();
    tripoint_bub_ms pos = c->pos_bub( here );

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

    if( c->has_flag( json_flag_CANNOT_MOVE ) ) {
        // cannot fling creatures that cannot move.
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
    tripoint_bub_ms pt = pos;
    creature_tracker &creatures = get_creature_tracker();
    while( range > 0 ) {
        c->underwater = false;
        // TODO: Check whenever it is actually in the viewport
        // or maybe even just redraw the changed tiles
        bool seen = is_u || u.sees( here, *c ); // To avoid redrawing when not seen
        tdir.advance();
        pt.x() = pos.x() + tdir.dx();
        pt.y() = pos.y() + tdir.dy();
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
            critter.check_dead_state( &here );
            if( !critter.is_dead() ) {
                thru = false;
            }
        } else if( here.impassable( pt ) ) {
            if( !here.veh_at( pt ).obstacle_at_part() ) {
                force = std::min<float>( here.bash_strength( pt ), flvel );
            } else {
                // No good way of limiting force here
                // Keep it 1 less than maximum to make the impact hurt
                // but to keep the target flying after it
                force = flvel - 1;
            }
            const int damage = rng( force, force * 2.0f ) / 9;
            c->impact( damage, pt );
            if( here.is_bashable( pt ) ) {
                // Only go through if we successfully make the tile passable
                here.bash( pt, flvel );
                thru = here.passable( pt );
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
                    here.unboard_vehicle( you->pos_bub( here ) );
                }
                // If we're flinging the player around, make sure the map stays centered on them.
                if( is_u ) {
                    update_map( pt.x(), pt.y() );
                    // update_map invalidates all bubble coordinates if it shifts the map.
                    pos = c->pos_bub( here );
                } else {
                    you->setpos( here, pt );
                }
            } else if( !creatures.creature_at( pt ) ) {
                // Dying monster doesn't always leave an empty tile (blob spawning etc.)
                // Just don't setpos if it happens - next iteration will do so
                // or the monster will stop a tile before the unpassable one
                c->setpos( here, pt );
            }
        } else {
            // Don't zero flvel - count this as slamming both the obstacle and the ground
            // although at lower velocity
            break;
        }
        range--;
        if( animate && ( seen || u.sees( here,  *c ) ) ) {
            invalidate_main_ui_adaptor();
            inp_mngr.pump_events();
            ui_manager::redraw_invalidated();
            refresh_display();
        }
    }

    // Fall down to the ground - always on the last reached tile
    if( !here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos ) ) {
        // Didn't smash into a wall or a floor so only take the fall damage
        if( thru && here.is_open_air( pos ) ) {
            here.creature_on_trap( *c, false );
        } else {
            // Fall on ground
            int force = rng( flvel, flvel * 2 ) / 9;
            if( controlled ) {
                force = std::max( force / 2 - 5, 0 );
            }
            if( force > 0 ) {
                int dmg = c->impact( force, pos );
                // TODO: Make landing damage the floor
                here.bash( pos, dmg / 4, false, false, false );
            }
            // Always apply traps to creature i.e. bear traps, tele traps etc.
            here.creature_on_trap( *c, false );
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

static std::optional<tripoint_bub_ms> find_empty_spot_nearby( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : here.points_in_radius( pos, 1 ) ) {
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
    if( u.has_flag( json_flag_CANNOT_MOVE ) ) {
        return;
    }

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
    tripoint_bub_ms pos = u.pos_bub( here );

    // Force means we're going down, even if there's no staircase, etc.
    bool climbing = false;
    climbing_aid_id climbing_aid = climbing_aid_default;
    int move_cost = 100;
    tripoint_bub_ms stairs( pos.xy(), pos.z() + movez );
    bool wall_cling = u.has_flag( json_flag_WALL_CLING );
    bool adjacent_climb = false;
    bool climb_flying = u.has_flag( json_flag_CLIMB_FLYING );
    if( !force && movez == 1 && !here.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos ) &&
        !u.is_underwater() ) {
        // Climbing

        for( const tripoint_bub_ms &p : here.points_in_radius( pos, 2 ) ) {
            if( here.has_flag( ter_furn_flag::TFLAG_CLIMB_ADJACENT, p ) ) {
                adjacent_climb = true;
            }
        }
        if( here.has_floor_or_support( stairs ) ) {
            tripoint_bub_ms dest_phase = pos;
            dest_phase.z() += 1;
            if( phasing_move_enchant( dest_phase, u.calculate_by_enchantment( 0,
                                      enchant_vals::mod::PHASE_DISTANCE ) ) ) {
                return;
            } else {
                add_msg( m_info, _( "You can't climb here - there's a ceiling above your head." ) );
                return;
            }
        }

        if( !climb_flying && ( u.get_working_arm_count() < 1 &&
                               !here.has_flag( ter_furn_flag::TFLAG_LADDER, pos ) ) ) {
            add_msg( m_info, _( "You can't climb because your arms are too damaged or encumbered." ) );
            return;
        }

        const int cost = u.climbing_cost( pos, stairs );
        add_msg_debug( debugmode::DF_GAME, "Climb cost %d", cost );
        const bool can_climb_here = cost > 0 ||
                                    u.has_flag( json_flag_CLIMB_NO_LADDER ) || wall_cling || climb_flying;
        if( !can_climb_here && !adjacent_climb ) {
            add_msg( m_info, _( "You can't climb here - you need walls and/or furniture to brace against." ) );
            return;
        }

        const item_location weapon = u.get_wielded_item();
        if( !climb_flying && !here.has_flag( ter_furn_flag::TFLAG_LADDER, pos ) && weapon &&
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

        std::vector<tripoint_bub_ms> pts;
        if( climb_flying ) {
            pts.push_back( stairs );
        } else {
            for( const tripoint_bub_ms &pt : here.points_in_radius( stairs, 1 ) ) {
                if( here.passable_through( pt ) ) {
                    pts.push_back( pt );
                }
            }
            if( wall_cling && here.is_wall_adjacent( stairs ) ) {
                pts.push_back( stairs );
            }
        }

        if( pts.empty() ) {
            add_msg( m_info,
                     _( "You can't climb here - there is no terrain above you that would support your weight." ) );
            return;
        } else {
            // TODO: Make it an extended action
            climbing = true;
            climbing_aid = climbing_aid_furn_CLIMBABLE;

            if( climb_flying ) {
                u.set_activity_level( MODERATE_EXERCISE );
                move_cost = 100;
            } else {
                u.set_activity_level( EXTRA_EXERCISE );
                move_cost = cost == 0 ? 1000 : cost + 500;
            }

            // climbing up, select which tile to climb to
            const std::optional<tripoint_bub_ms> pnt = point_selection_menu( pts );
            if( !pnt ) {
                return;
            }
            if( here.dangerous_field_at( *pnt ) &&
                !query_yn( _( "There appears to be a dangerous field at your destination.  Continue?" ) ) ) {
                return;
            }
            stairs = *pnt;
        }
    }

    if( !force && movez == -1 && !here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos ) &&
        !u.is_underwater() && !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR_WATER, pos ) &&
        !u.has_effect( effect_gliding ) ) {
        tripoint_bub_ms dest_phase = pos;
        dest_phase.z() -= 1;

        if( wall_cling && !here.has_floor_or_support( pos ) ) {
            climbing = true;
            climbing_aid = climbing_aid_ability_WALL_CLING;
            u.set_activity_level( EXTRA_EXERCISE );
            u.burn_energy_all( -750 );
            move_cost += 500;
        } else if( phasing_move_enchant( dest_phase, u.calculate_by_enchantment( 0,
                                         enchant_vals::mod::PHASE_DISTANCE ) ) ) {
            return;
        } else {
            add_msg( m_info, _( "You can't go down here!" ) );
            return;
        }
    } else if( !climbing && !force && movez == 1 &&
               !here.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos ) && !u.is_underwater() ) {
        add_msg( m_info, _( "You can't go up here!" ) );
        return;
    }

    if( force ) {
        // Let go of a grabbed cart.
        u.grab( object_type::NONE );
    } else if( u.grab_point != tripoint_rel_ms::zero ) {
        add_msg( m_info, _( "You can't drag things up and down stairs." ) );
        return;
    }

    // TODO: Use u.posz() instead of m.abs_sub
    const int z_after = here.get_abs_sub().z() + movez;
    if( z_after < -OVERMAP_DEPTH ) {
        add_msg( m_info, _( "Halfway down, the way down becomes blocked off." ) );
        return;
    } else if( z_after > OVERMAP_HEIGHT ) {
        add_msg( m_info, _( "Halfway up, the way up becomes blocked off." ) );
        return;
    }

    if( !u.move_effects( false ) && !force ) {
        // move_effects determined we could not move, waste all moves
        u.set_moves( 0 );
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
    if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, pos ) ) {
        if( !u.has_flag( json_flag_WATERWALKING ) ) {
            swimming = true;
        }
        const ter_id &target_ter = here.ter( pos + tripoint( 0, 0, movez ) );

        // If we're in a water tile that has both air above and deep enough water to submerge in...
        if( here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, pos ) &&
            !here.has_flag( ter_furn_flag::TFLAG_WATER_CUBE, pos ) ) {
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
                    // Check for a flotation device or the WATERWALKING flag first before allowing us to submerge.
                    if( u.worn_with_flag( flag_FLOTATION ) ) {
                        add_msg( m_info, _( "You can't dive while wearing a flotation device." ) );
                        return;
                    }
                    if( u.has_flag( json_flag_WATERWALKING ) ) {
                        add_msg( m_info, _( "You can't dive while walking on water." ) );
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
        else if( here.has_flag( ter_furn_flag::TFLAG_WATER_CUBE, pos ) ) {
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
        const std::optional<tripoint_bub_ms> pnt = find_or_make_stairs( here, z_after, rope_ladder, peeking,
                pos );
        if( !pnt ) {
            return;
        }
        stairs = *pnt;
    }

    if( !force && !climbing && here.dangerous_field_at( stairs ) &&
        !query_yn( _( "There appears to be a dangerous field at your destination.  Continue?" ) ) ) {
        return;
    }

    // LAST CHANCE TO ABORT! Everything past here assumes the movement is going ahead.

    std::vector<monster *> monsters_following;
    if( std::abs( movez ) == 1 ) {
        bool ladder = here.has_flag( ter_furn_flag::TFLAG_DIFFICULT_Z, pos );
        for( monster &critter : all_monsters() ) {
            if( ladder && !critter.climbs() ) {
                continue;
            }
            // TODO: just check if it's going for the avatar's location, it's simpler
            Creature *target = critter.attack_target();
            if( ( target && target->is_avatar() ) || ( !critter.has_effect( effect_ridden ) &&
                    ( critter.is_pet_follow() || critter.has_effect( effect_led_by_leash ) ) &&
                    !critter.has_effect( effect_tied ) && critter.sees( here, u ) ) ) {
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
        u.mod_moves( -move_cost );
        u.burn_energy_all( -move_cost );
    }

    if( surfacing || submerging ) {
        // Surfacing and submerging don't actually move us anywhere, and just
        // toggle our underwater state in the same location.
        return;
    }

    tripoint_bub_ms old_pos = pos;
    const tripoint_abs_ms old_abs_pos = here.get_abs( old_pos );
    const bool z_level_changed = vertical_shift( z_after );
    if( !force ) {
        const point_rel_sm submap_shift = update_map( stairs.x(), stairs.y(), z_level_changed );
        // Adjust all bubble coordinates used according to the submap shift.
        const point_rel_ms ms_shift = coords::project_to<coords::ms>( submap_shift );
        pos = pos - ms_shift;
        old_pos = old_pos - ms_shift;
        stairs = stairs - ms_shift;
    }

    // if an NPC or monster is on the stairs when player ascends/descends
    // they may end up merged on the same tile, do some displacement to resolve that.
    creature_tracker &creatures = get_creature_tracker();
    if( creatures.creature_at<npc>( pos, true ) ||
        creatures.creature_at<monster>( pos, true ) ) {
        std::string crit_name;
        bool player_displace = false;
        std::optional<tripoint_bub_ms> displace = find_empty_spot_nearby( pos );
        if( !displace.has_value() ) {
            // They can always move to the previous location of the player.
            displace = old_pos;
        }
        npc *guy = creatures.creature_at<npc>( pos, true );
        if( guy ) {
            crit_name = guy->get_name();
            tripoint_bub_ms old_pos = guy->pos_bub( here );
            if( !guy->is_enemy() ) {
                guy->move_away_from( pos, true );
                if( old_pos != guy->pos_bub( here ) ) {
                    add_msg( _( "%s moves out of the way for you." ), guy->get_name() );
                }
            } else {
                player_displace = true;
            }
        }
        monster *mon = creatures.creature_at<monster>( pos, true );
        // if the monster is ridden by the player or an NPC:
        // Dont displace them. If they are mounted by a friendly NPC,
        // then the NPC will already have been displaced just above.
        // if they are ridden by the player, we want them to coexist on same tile
        if( mon && !mon->mounted_player ) {
            crit_name = mon->get_name();
            if( mon->friendly == -1 ) {
                mon->setpos( here, *displace );
                add_msg( _( "Your %s moves out of the way for you." ), mon->get_name() );
            } else {
                player_displace = true;
            }
        }
        if( player_displace ) {
            u.setpos( here, *displace );
            u.mod_moves( -to_moves<int>( 1_seconds ) * 0.2 );
            add_msg( _( "You push past %s blocking the way." ), crit_name );
        }
        pos = u.pos_bub( here );
    }

    // Now that we know the player's destination position, we can move their mount as well
    if( u.is_mounted() ) {
        u.mounted_creature->setpos( here, pos );
    }

    // This ugly check is here because of stair teleport bullshit
    // TODO: Remove stair teleport bullshit
    if( rl_dist( pos, old_pos ) <= 1 ) {
        for( monster *m : monsters_following ) {
            m->set_dest( u.pos_abs() );
        }
    }

    if( rope_ladder ) {
        if( u.has_flag( json_flag_WEB_RAPPEL ) ) {
            here.furn_set( pos, furn_f_web_up );
        } else if( u.has_flag( json_flag_VINE_RAPPEL ) ) {
            here.furn_set( pos, furn_f_vine_up );
        } else {
            here.furn_set( pos, furn_f_rope_up );
        }
    }

    if( here.ter( stairs ) == ter_t_manhole_cover ) {
        here.spawn_item( stairs + point( rng( -1, 1 ), rng( -1, 1 ) ), itype_manhole_cover );
        here.ter_set( stairs, ter_t_manhole );
    }

    if( u.is_hauling() ) {
        start_hauling( old_pos );
    }

    here.invalidate_map_cache( here.get_abs_sub().z() );
    here.build_map_cache( here.get_abs_sub().z() );
    u.gravity_check();

    u.recoil = MAX_RECOIL;

    cata_event_dispatch::avatar_moves( old_abs_pos, u, here );
}

bool game::travel_to_dimension( const std::string &new_prefix,
                                const std::string &region_type,
                                const std::vector<npc *> &npc_travellers )
{
    map &here = get_map();
    avatar &player = get_avatar();
    if( !npc_travellers.empty() ) {
        int traveller_count = npc_travellers.size();
        for( auto it = critter_tracker->active_npc.begin(); it != critter_tracker->active_npc.end(); ) {
            // skip unloading a traveller
            bool skip = false;
            if( traveller_count > 0 ) {
                for( npc *guy : npc_travellers ) {
                    if( guy->getID() == ( *it )->getID() ) {
                        skip = true;
                        traveller_count--;
                        break;
                    }
                }
            }
            if( !skip ) {
                ( *it )->on_unload();
                it = critter_tracker->active_npc.erase( it );
            } else {
                it++;
            }
        }
    } else {
        unload_npcs();
    }
    for( monster &critter : all_monsters() ) {
        despawn_monster( critter );
    }
    if( player.in_vehicle ) {
        here.unboard_vehicle( player.pos_bub() );
    }
    // Make sure we don't mess up savedata if for some reason maps can't be saved
    if( !save_maps() || !save_dimension_data() ) {
        return false;
    }
    player.save_map_memory();
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT; z++ ) {
        here.clear_vehicle_list( z );
    }
    here.rebuild_vehicle_level_caches();
    // Inputting an empty string to the text input EOC fails
    // so i'm using 'default' as empty/main dimension
    if( new_prefix != "default" ) {
        dimension_prefix = new_prefix;
    } else {
        dimension_prefix.clear();
    }
    // Load in data specific to the dimension (like weather)
    if( !load_dimension_data() ) {
        // dimension data file not found/created yet

        // Only allow `region_type` input for new dimensions.
        overmap_buffer.current_region_type = region_type;
    }
    // Clear the immediate game area around the player
    MAPBUFFER.clear();
    // hack to prevent crashes from temperature checks
    // This returns to false in 'on_turn()' so it should be fine?
    swapping_dimensions = true;
    // Clear the overmap
    overmap_buffer.clear();
    // load/create new overmap
    overmap_buffer.get( point_abs_om{} );
    // clear map memory from the previous dimension
    player.clear_map_memory();
    // Load map memory in new dimension, if there is any
    player.load_map_memory();
    // Loads submaps and invalidate related caches
    here.load( tripoint_abs_sm( here.get_abs_sub() ), false );

    here.invalidate_visibility_cache();
    //without this vehicles only load in after walking around a bit
    here.reset_vehicles_sm_pos();
    load_npcs();
    // Handle static monsters
    here.spawn_monsters( true, true );
    // updates the weather, if the weather settings are different in the new world
    weather.weather_override = WEATHER_NULL;
    weather.set_nextweather( calendar::turn );
    update_overmap_seen();
    return true;
}

void game::start_hauling( const tripoint_bub_ms &pos )
{
    map &here = get_map();

    std::vector<item_location> candidate_items = here.get_haulable_items( pos );
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
    const tripoint_rel_ms relative_destination{};

    const move_items_activity_actor actor( target_items, quantities, to_vehicle, relative_destination,
                                           true );
    u.assign_activity( actor );
}

std::optional<tripoint_bub_ms> game::find_stairs( const map &mp, int z_after,
        const tripoint_bub_ms &pos )
{
    const int movez = z_after - pos.z();
    const bool going_down_1 = movez == -1;
    const bool going_up_1 = movez == 1;

    // If there are stairs on the same x and y as we currently are, use those
    if( going_down_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_UP, pos + tripoint::below ) ) {
        return pos + tripoint::below;
    }
    if( going_up_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, pos + tripoint::above ) ) {
        return pos + tripoint::above;
    }
    // We did not find stairs directly above or below, so search the map for them
    // If there's empty space right below us, we can just go down that way.
    int best = INT_MAX;
    std::optional<tripoint_bub_ms> stairs;
    const int omtilesz = SEEX * 2 - 1;
    const tripoint_abs_ms abs_omt_base( project_to<coords::ms>( project_to<coords::omt>( mp.get_abs(
                                            pos ) ) ) );

    tripoint_bub_ms omtile_align_start( mp.get_bub( tripoint_abs_ms( abs_omt_base.xy(),
                                        z_after ) ) );
    tripoint_bub_ms omtile_align_end( omtile_align_start + point( omtilesz, omtilesz ) );

    if( !mp.is_open_air( u.pos_bub( mp ) ) ) {
        for( const tripoint_bub_ms &dest : mp.points_in_rectangle( omtile_align_start,
                omtile_align_end ) ) {
            if( rl_dist( u.pos_bub( mp ), dest ) <= best &&
                ( ( going_down_1 && mp.has_flag( ter_furn_flag::TFLAG_GOES_UP, dest ) ) ||
                  ( going_up_1 && ( mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, dest ) ||
                                    mp.ter( dest ) == ter_t_manhole_cover ) ) ||
                  ( ( movez == 2 || movez == -2 ) && mp.ter( dest ) == ter_t_elevator ) ) ) {
                stairs.emplace( dest );
                best = rl_dist( u.pos_bub( mp ), dest );
            }
        }
    }

    return stairs;
}

std::optional<tripoint_bub_ms> game::find_or_make_stairs( map &mp, const int z_after,
        bool &rope_ladder,
        bool peeking, const tripoint_bub_ms &pos )
{
    const bool is_avatar = u.pos_bub( mp ) == pos;
    const int movez = z_after - pos.z();

    // Try to find the stairs.
    std::optional<tripoint_bub_ms> stairs = find_stairs( mp, z_after, pos );

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
    stairs->z() = z_after;
    // Check the destination area for lava.
    if( mp.ter( *stairs ) == ter_t_lava ) {
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

    if( u.has_effect( effect_gliding ) && mp.is_open_air( u.pos_bub( mp ) ) ) {
        return stairs;
    }

    if( movez > 0 ) {
        if( !mp.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, *stairs ) ) {
            if( mp.has_floor_or_support( *stairs ) ) {
                if( !query_yn( _( "You may be unable to return back down these stairs.  Continue up?" ) ) ) {
                    return std::nullopt;
                }
            } else {
                // query for target, same as climbing
                std::vector<tripoint_bub_ms> pts;

                for( const tripoint_bub_ms &pt : mp.points_in_radius( *stairs, 1 ) ) {
                    if( mp.passable_through( pt ) ) {
                        pts.push_back( pt );
                    }
                }

                if( pts.empty() ) {
                    add_msg( m_info,
                             _( "You can't climb here - there is no terrain above you that would support your weight." ) );
                    return std::nullopt;
                } else {
                    const std::optional<tripoint_bub_ms> pnt = point_selection_menu( pts );
                    if( !pnt ) {
                        return std::nullopt;
                    }
                    stairs = pnt;
                }
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
    map &here = get_map();

    if( z_after < -OVERMAP_DEPTH || z_after > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  z_after, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return false;
    }

    scent.reset();

    const int z_before = u.posz();
    u.move_to( tripoint_abs_ms( u.pos_abs().xy(), z_after ) );

    // Shift the map itself
    here.vertical_shift( z_after );

    vertical_notes( z_before, z_after );

    return z_before != z_after;
}

void game::vertical_notes( int z_before, int z_after )
{
    map &here = get_map();

    if( z_before == z_after || !get_option<bool>( "AUTO_NOTES" ) ||
        !get_option<bool>( "AUTO_NOTES_STAIRS" ) ) {
        return;
    }

    if( !here.inbounds_z( z_before ) || !here.inbounds_z( z_after ) ) {
        debugmsg( "game::vertical_notes invalid arguments: z_before == %d, z_after == %d",
                  z_before, z_after );
        return;
    }
    // Figure out where we know there are up/down connectors
    // Fill in all the tiles we know about (e.g. subway stations)
    static const int REVEAL_RADIUS = 40;
    for( const tripoint_abs_omt &p : points_in_radius( u.pos_abs_omt(), REVEAL_RADIUS ) ) {
        const tripoint_abs_omt cursp_before( p.xy(), z_before );
        const tripoint_abs_omt cursp_after( p.xy(), z_after );

        if( overmap_buffer.seen( cursp_before ) == om_vision_level::unseen ) {
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
            overmap_buffer.set_seen( cursp_after, om_vision_level::full );
            overmap_buffer.add_note( cursp_after, string_format( ">:W;%s", _( "AUTO: goes down" ) ) );
        } else if( z_after < z_before && ter->has_flag( oter_flags::known_down ) &&
                   !ter2->has_flag( oter_flags::known_up ) ) {
            overmap_buffer.set_seen( cursp_after, om_vision_level::full );
            overmap_buffer.add_note( cursp_after, string_format( "<:W;%s", _( "AUTO: goes up" ) ) );
        }
    }
}

point_rel_sm game::update_map( Character &p, bool z_level_changed )
{
    point_bub_ms p2( p.pos_bub().xy() );
    return update_map( p2.x(), p2.y(), z_level_changed );
}

point_rel_sm game::update_map( int &x, int &y, bool z_level_changed )
{
    map &here = get_map();

    point_rel_sm shift;

    while( x < HALF_MAPSIZE_X ) {
        x += SEEX;
        shift.x()--;
    }
    while( x >= HALF_MAPSIZE_X + SEEX ) {
        x -= SEEX;
        shift.x()++;
    }
    while( y < HALF_MAPSIZE_Y ) {
        y += SEEY;
        shift.y()--;
    }
    while( y >= HALF_MAPSIZE_Y + SEEY ) {
        y -= SEEY;
        shift.y()++;
    }

    if( shift == point_rel_sm::zero ) {
        // adjust player position
        u.setpos( here, tripoint_bub_ms( x, y, here.get_abs_sub().z() ) );
        if( z_level_changed ) {
            // Update what parts of the world map we can see
            // We may be able to see farther now that the z-level has changed.
            update_overmap_seen();
        }
        // Not actually shifting the submaps, all the stuff below would do nothing
        return point_rel_sm::zero;
    }

    // this handles loading/unloading submaps that have scrolled on or off the viewport
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    inclusive_rectangle<point_rel_sm> size_1( {-1, -1}, { 1, 1 } );
    point_rel_sm remaining_shift = shift;
    while( remaining_shift != point_rel_sm::zero ) {
        point_rel_sm this_shift = clamp( remaining_shift, size_1 );
        here.shift( point_rel_sm( this_shift ) );
        remaining_shift -= this_shift;
    }

    // Shift monsters
    shift_monsters( { shift, 0 } );
    const point_rel_ms shift_ms = coords::project_to<coords::ms>( shift );
    u.shift_destination( -shift_ms );

    // Shift NPCs
    for( auto it = critter_tracker->active_npc.begin(); it != critter_tracker->active_npc.end(); ) {
        ( *it )->shift( shift );
        const tripoint_bub_ms pos = ( *it )->pos_bub( here );

        // TODO: This could probably be replaced by an inbounds check.
        if( pos.x() < 0 || pos.x() >= MAPSIZE_X ||
            pos.y() < 0 || pos.y() >= MAPSIZE_Y ) {
            //Remove the npc from the active list. It remains in the overmap list.
            ( *it )->on_unload();
            it = critter_tracker->active_npc.erase( it );
        } else {
            ++it;
        }
    }

    scent.shift( shift_ms );

    // Also ensure the player is on current z-level
    // m.get_abs_sub().z should later be removed, when there is no longer such a thing
    // as "current z-level"
    u.setpos( here, tripoint_bub_ms( x, y, here.get_abs_sub().z() ) );

    // Only do the loading after all coordinates have been shifted.

    // Check for overmap saved npcs that should now come into view.
    // Put those in the active list.
    load_npcs();

    // Make sure map cache is consistent since it may have shifted.
    for( int zlev = -OVERMAP_DEPTH; zlev <= OVERMAP_HEIGHT; ++zlev ) {
        here.invalidate_map_cache( zlev );
    }
    here.build_map_cache( here.get_abs_sub().z() );

    // Spawn monsters if appropriate
    // This call will generate new monsters in addition to loading, so it's placed after NPC loading
    here.spawn_monsters( false ); // Static monsters

    // Update what parts of the world map we can see
    update_overmap_seen();

    return shift;
}

void game::update_overmap_seen()
{
    const tripoint_abs_omt ompos = u.pos_abs_omt();
    const int dist = u.overmap_modified_sight_range( light_level( u.posz() ) );
    const int dist_squared = dist * dist;
    // We can always see where we're standing
    overmap_buffer.set_seen( ompos, om_vision_level::full );
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
        point_rel_omt abs_delta = delta.abs();
        int max_delta = std::max( abs_delta.x(), abs_delta.y() );
        const float multiplier = trigdist ? std::sqrt( h_squared ) / max_delta : 1;
        const std::vector<tripoint_abs_omt> line = line_to( ompos, p );
        float sight_points = dist;
        for( auto it = line.begin();
             it != line.end() && sight_points >= 0; ++it ) {
            const oter_id &ter = overmap_buffer.ter( *it );
            sight_points -= static_cast<int>( ter->get_see_cost() ) * multiplier;
        }
        if( sight_points < 0 ) {
            continue;
        }
        const auto set_seen = []( const tripoint_abs_omt & p, om_vision_level level ) {
            tripoint_abs_omt seen( p );
            do {
                overmap_buffer.set_seen( seen, level );
                --seen.z();
            } while( seen.z() >= 0 );
        };
        int tiles_from = rl_dist( p, ompos );
        if( tiles_from < std::floor( dist / 2 ) ) {
            set_seen( p, om_vision_level::full );
        } else if( tiles_from < dist ) {
            set_seen( p, om_vision_level::details );
        } else if( tiles_from < dist * 2 ) {
            set_seen( p, om_vision_level::outlines );
        } else {
            set_seen( p, om_vision_level::vague );
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
    map &here = get_map();

    for( monster &critter : g->all_monsters() ) {
        const tripoint_bub_ms pos = critter.pos_bub( here );

        if( pos.x() < 0 - MAPSIZE_X / 6 ||
            pos.y() < 0 - MAPSIZE_Y / 6 ||
            pos.x() > ( MAPSIZE_X * 7 ) / 6 ||
            pos.y() > ( MAPSIZE_Y * 7 ) / 6 ) {
            g->despawn_monster( critter );
        }
    }
}

void game::shift_monsters( const tripoint_rel_sm &shift )
{
    if( shift == tripoint_rel_sm::zero ) {
        return;
    }
    for( monster &critter : all_monsters() ) {
        if( shift.xy() != point_rel_sm::zero ) {
            critter.shift( shift.xy() );
        }

        if( reality_bubble().inbounds( critter.pos_bub() ) ) {
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
        const tripoint_abs_omt u_omt = u.pos_abs_omt();
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
    new_fac_id += std::to_string( tmp->getID().get_value() );
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
    tmp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, tmp->pos_abs_omt(),
                          tmp->getID() ) );
    // This will make the new NPC active- if its nearby to the player
    load_npcs();
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

void game::init_autosave()
{
    moves_since_last_save = 0;
    last_save_timestamp = std::time( nullptr );
}

void game::quicksave()
{
    //Don't autosave if the player hasn't done anything since the last autosave/quicksave,
    if( !moves_since_last_save && !world_generator->active_world->world_saves.empty() ) {
        return;
    }
    add_msg( m_info, _( "Saving game, this may take a while." ) );

    static_popup popup;
    popup.message( "%s", _( "Saving game, this may take a while." ) );
    ui_manager::redraw();
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

            u.set_moves( 0 );
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
    map &here = get_map();

    // The player is located in the middle submap of the map.
    const tripoint_abs_sm sm = here.get_abs_sub() + tripoint( HALF_MAPSIZE, HALF_MAPSIZE, 0 );
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

std::vector<vehicle *> game::get_vehicles_if( const std::function<bool( const vehicle & )> &pred )
{
    std::vector<vehicle *> result;
    for( wrapped_vehicle &veh : get_map().get_vehicles() ) {
        if( pred( *veh.v ) ) {
            result.push_back( veh.v );
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

cata_path PATH_INFO::player_base_save_path()
{
    return PATH_INFO::world_base_save_path() / base64_encode( get_avatar().get_save_id() );
}

cata_path PATH_INFO::world_base_save_path()
{
    if( world_generator->active_world == nullptr ) {
        return PATH_INFO::savedir_path();
    }
    return world_generator->active_world->folder_path();
}

cata_path PATH_INFO::dimensions_save_path()
{
    return PATH_INFO::world_base_save_path() / "dimensions";
}

cata_path PATH_INFO::current_dimension_save_path()
{
    std::string dimension_prefix = g->get_dimension_prefix();
    if( !dimension_prefix.empty() ) {
        return PATH_INFO::dimensions_save_path() / dimension_prefix;
    }
    return PATH_INFO::world_base_save_path();
}


cata_path PATH_INFO::current_dimension_player_save_path()
{
    return PATH_INFO::current_dimension_save_path() / base64_encode( get_avatar().get_save_id() );
}

void game::shift_destination_preview( const point_rel_ms &delta )
{
    for( tripoint_bub_ms &p : destination_preview ) {
        p += delta.raw(); // TODO: Make += etc. work with corresponding relative coordinates
    }
}

float game::slip_down_chance( climb_maneuver, climbing_aid_id aid_id,
                              bool show_chance_messages )
{
    if( aid_id.is_null() ) {
        // The NULL climbing aid ID may be passed as a default argument.
        aid_id = climbing_aid_default;
    }

    const climbing_aid &aid = aid_id.obj();

    float slip = 100.0f;

    const bool parkour = u.has_proficiency( proficiency_prof_parkour );
    const bool badknees = u.has_trait( trait_BADKNEES );
    bool climb_flying = u.has_flag( json_flag_CLIMB_FLYING );

    // If you're levitating or flying, there's nothing to slip on
    if( climb_flying ) {
        slip = 0;
    }

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

    add_msg_debug( debugmode::DF_GAME, "Slip chance after proficiency/trait modifiers %.0f%%", slip );

    // Climbing is difficult with wet hands and feet.
    float wet_penalty = 1.0f;
    bool wet_feet = false;
    bool wet_hands = false;

    for( const bodypart_id &bp : u.get_all_body_parts_of_type( bp_type::foot,
            get_body_part_flags::primary_type ) ) {
        if( u.get_part_wetness( bp ) > 0 && !climb_flying ) {
            add_msg_debug( debugmode::DF_GAME, "Foot %s %.1f wet", body_part_name( bp ),
                           u.get_part( bp )->get_wetness_percentage() );
            wet_feet = true;
            wet_penalty += u.get_part( bp )->get_wetness_percentage() / 2;
        }
    }

    for( const bodypart_id &bp : u.get_all_body_parts_of_type( bp_type::hand,
            get_body_part_flags::primary_type ) ) {
        if( u.get_part_wetness( bp ) > 0 && !climb_flying ) {
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

    add_msg_debug( debugmode::DF_GAME, "Slip chance after wetness penalty %.0f%%", slip );

    ///\EFFECT_DEX decreases chances of slipping while climbing
    ///\EFFECT_STR decreases chances of slipping while climbing
    /// Not using arm strength since lifting score comes into play later
    slip /= std::max( 1, u.dex_cur + u.str_cur );

    add_msg_debug( debugmode::DF_GAME, "Slip chance after stat modifiers %.0f%%", slip );

    // Apply limb score penalties - grip, arm strength and footing are all relevant
    slip /= u.get_modifier( character_modifier_slip_prevent_mod );
    add_msg_debug( debugmode::DF_GAME, "Slipping chance after limb scores %.0f%%", slip );

    // Being weighed down makes it easier for you to slip.
    double weight_ratio = static_cast<double>( units::to_gram( u.weight_carried() ) ) / units::to_gram(
                              u.weight_capacity() );
    slip += roll_remainder( 8.0 * weight_ratio );
    add_msg_debug( debugmode::DF_GAME, "Weight ratio %.2f, slip chance %.0f%%", weight_ratio,
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
    add_msg_debug( debugmode::DF_GAME, "Stamina ratio %.2f, slip chance %.0f%%",
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

    add_msg_debug( debugmode::DF_GAME, "After affordance modifier, final slip chance %.0f%%",
                   slip );

    return slip;
}

bool game::slip_down( climb_maneuver maneuver, climbing_aid_id aid_id,
                      bool show_chance_messages )
{
    map &here = get_map();

    float slip = slip_down_chance( maneuver, aid_id, show_chance_messages );

    if( x_in_y( slip, 100 ) ) {
        add_msg( m_bad, _( "You slip while climbing and fall down." ) );
        if( slip >= 100 ) {
            add_msg( m_bad, _( "Climbing is impossible in your current state." ) );
        }
        // Check for traps and gravity if climbing up or down.
        if( maneuver != climb_maneuver::over_obstacle ) {
            here.creature_on_trap( u );
            u.gravity_check();
        }
        return true;
    }
    return false;
}

pulp_data game::calculate_character_ability_to_pulp( const Character &you )
{
    pulp_data pd{};

    double pulp_power_bash = 1;

    // the idea was that roll_damage() gonna handle melee skills, but it doesn't care for skill, apparently?
    const std::pair<float, item> pair_bash = you.get_best_weapon_by_damage_type( damage_bash );
    pulp_power_bash = pair_bash.first;
    pd.bash_tool = item::nname( pair_bash.second.typeId() );

    if( pair_bash.second.has_flag( flag_MESSY ) || pair_bash.first > 40 ) {
        pd.mess_radius = 2;
    }

    // bash is good, but the rest physical damages can be useful also
    damage_instance di;
    u.roll_damage( damage_cut, false, di, true, pair_bash.second, attack_vector_id::NULL_ID(),
                   sub_bodypart_str_id::NULL_ID(), 1.f );

    u.roll_damage( damage_stab, false, di, true, pair_bash.second, attack_vector_id::NULL_ID(),
                   sub_bodypart_str_id::NULL_ID(), 1.f );

    for( const damage_unit &du : di ) {
        // potentially move it to json, if someone find necrotic mace +3 should pulp faster for some reason
        if( du.type == damage_cut ) {
            pulp_power_bash += du.amount / 3;
        } else if( du.type == damage_stab ) {
            pulp_power_bash += du.amount / 6;
        }
    }

    const double weight_factor = units::to_kilogram( you.get_weight() ) / 10;
    const double athletic_factor = std::min( 9.0f, you.get_skill_level( skill_swimming ) + 3 );
    const int strength_factor = you.get_str() / 2;
    // stomp deliver about 5000-13000 N of force, beginner boxer deal 2500 N of force in it's punch
    // ballpark of 13 damage for untrained human vs 30+ for highly trained person of average weight
    const double pulp_power_stomps = athletic_factor + weight_factor + strength_factor;

    // if no tool is bashy enough, use only stomps
    // if stomps are too weak, use only bash
    // if roughly same, use both
    float bash_factor = 0.0;
    if( pulp_power_bash < pulp_power_stomps * 0.5 ) {
        bash_factor = pulp_power_stomps;
        pd.stomps_only = true;
    } else if( pulp_power_stomps < pulp_power_bash * 0.5 ) {
        bash_factor = pulp_power_bash;
        pd.weapon_only = true;
    } else {
        bash_factor = std::max( pulp_power_stomps, pulp_power_bash );
    }

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "you: %s\nbash weapon: %s\nfinal pulp_power_bash: %s\npulp_power_stomps: %s\n",
                   you.name.c_str(), pair_bash.second.type->id.c_str(), pair_bash.first, pulp_power_bash,
                   pulp_power_stomps, bash_factor );

    bash_factor = std::pow( bash_factor, 1.8f );

    const item &best_cut = you.best_item_with_quality( qual_BUTCHER );
    if( best_cut.get_quality( qual_BUTCHER ) > 5 ) {
        pd.can_cut_precisely = true;
        pd.cut_tool = item::nname( best_cut.typeId() );
    }

    const item &best_pry = you.best_item_with_quality( qual_PRY );
    if( best_pry.get_quality( qual_PRY ) > 0 ) {
        pd.can_pry_armor = true;
        pd.pry_tool = item::nname( best_pry.typeId() );
    }

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "final bash factor: %s, butcher tool name: %s, prying tool name (if used): %s",
                   bash_factor, pd.cut_tool, pd.pry_tool );

    const float skill_factor = std::sqrt( 2 + std::max(
            you.get_skill_level( skill_survival ),
            you.get_skill_level( skill_firstaid ) ) );

    pd.nominal_pulp_power = bash_factor * skill_factor * ( pd.can_cut_precisely ? 1 : 0.85 );

    add_msg_debug( debugmode::DF_ACTIVITY, "final pulp_power: %s", pd.nominal_pulp_power );

    // since we are trying to depict pulping as more involved than you Isaac Clarke the corpse 100% of time,
    // we would assume there is some time between attacks, where you try to reach some sweet spot,
    // cut the part with a knife, or plain rest because you are not athletic enough
    // REGEN_RATE + 1 purely to prevent you regening all of your stamina in the process + apply muscle strain when it be implemented
    pd.pulp_effort = get_option<float>( "PLAYER_BASE_STAMINA_REGEN_RATE" ) + 1;

    return pd;
}

pulp_data game::calculate_pulpability( const Character &you, const mtype &corpse_mtype )
{
    pulp_data pd = calculate_character_ability_to_pulp( you );

    return calculate_pulpability( you, corpse_mtype, pd );
}

pulp_data game::calculate_pulpability( const Character &you, const mtype &corpse_mtype,
                                       pulp_data pd )
{
    // potentially make a new var and stash all this calculations in corpse

    double pow_factor;
    if( corpse_mtype.size == creature_size::huge ) {
        pow_factor = 1.2;
    } else if( corpse_mtype.size == creature_size::large ) {
        pow_factor = 1.1;
    } else {
        pow_factor = 1;
    }

    float corpse_volume = units::to_liter( corpse_mtype.volume );
    // in seconds
    int time_to_pulp = std::max( 1.0, ( std::pow( corpse_volume,
                                        pow_factor ) * 1000 ) / pd.nominal_pulp_power );
    // in seconds also
    // 30 seconds for human body volume of 62.5L, scale from this
    int min_time_to_pulp = std::max( 1.0, 30 * corpse_volume / 62.5 );

    // you have a hard time pulling armor to reach important parts of this monster
    if( corpse_mtype.has_flag( mon_flag_PULP_PRYING ) ) {
        if( pd.can_pry_armor ) {
            time_to_pulp *= 1.25;
        } else {
            time_to_pulp *= 1.7;
        }
    }

    // Adjust pulp_power to match the time taken.
    pd.pulp_power = ( std::pow( corpse_volume, pow_factor ) * 1000 ) / time_to_pulp;

    // +25% to pulp time if char knows no weakpoints of monster
    // -25% if knows all of them
    if( !corpse_mtype.families.families.empty() ) {
        float wp_known = 0;
        for( const weakpoint_family &wf : corpse_mtype.families.families ) {
            if( you.has_proficiency( wf.proficiency ) ) {
                ++wp_known;
            } else if( !pd.unknown_prof.has_value() ) {
                pd.unknown_prof = wf.proficiency;
            }
        }
        // We're not adjusting pulp_power here, so no proficiency will result in more
        // splatter and full proficiency will result in less, as no needless bashes
        // resulting in splatter are made.
        time_to_pulp *= 1.25 - 0.5 * wp_known / corpse_mtype.families.families.size();
    }

    if( time_to_pulp < min_time_to_pulp ) {
        pd.time_to_pulp = min_time_to_pulp;
        // Reducing the power to match the time in order to get the same amount of splatter.
        pd.pulp_power = pd.time_to_pulp / ( std::pow( corpse_volume, pow_factor ) * 1000 );
    } else {
        pd.time_to_pulp = time_to_pulp;
    }

    return pd;
}

bool game::can_pulp_corpse( const Character &you, const mtype &corpse_mtype )
{
    pulp_data pd = calculate_pulpability( you, corpse_mtype );

    return can_pulp_corpse( pd );
}

bool game::can_pulp_corpse( const pulp_data &pd )
{
    // if pulping is longer than an hour, this is a hard no
    return pd.time_to_pulp < 3600;
}

bool game::can_pulp_acid_corpse( const Character &you, const mtype &corpse_mtype )
{
    const bool acid_immune = you.is_immune_damage( damage_acid ) ||
                             you.is_immune_field( fd_acid );
    // this corpse is acid, and you are not immune to it
    const bool acid_corpse = corpse_mtype.bloodType().obj().has_acid && !acid_immune;

    if( acid_corpse ) {
        return false;
    }

    return true;
}

namespace cata_event_dispatch
{
void avatar_moves( const tripoint_abs_ms &old_abs_pos, const avatar &u, const map &m )
{
    const tripoint_bub_ms new_pos = u.pos_bub();
    const tripoint_abs_ms new_abs_pos = m.get_abs( new_pos );
    mtype_id mount_type;
    if( u.is_mounted() ) {
        mount_type = u.mounted_creature->type->id;
    }
    get_event_bus().send<event_type::avatar_moves>( mount_type, m.ter( new_pos ).id(),
            u.current_movement_mode(), u.is_underwater(), new_pos.z() );

    const tripoint_abs_omt old_abs_omt( coords::project_to<coords::omt>(
                                            old_abs_pos ) );
    const tripoint_abs_omt new_abs_omt( coords::project_to<coords::omt>(
                                            new_abs_pos ) );
    if( old_abs_omt != new_abs_omt ) {
        const oter_id &cur_ter = overmap_buffer.ter( new_abs_omt );
        const oter_id &past_ter = overmap_buffer.ter( old_abs_omt );
        get_event_bus().send<event_type::avatar_enters_omt>( new_abs_omt.raw(), cur_ter );
        // if the player has moved omt then might trigger an EOC for that OMT
        if( !past_ter->get_exit_EOC().is_null() ) {
            dialogue d( get_talker_for( get_avatar() ), nullptr );
            effect_on_condition_id eoc = cur_ter->get_exit_EOC();
            eoc->activate_activation_only( d, "OMT movement" );
        }

        if( !cur_ter->get_entry_EOC().is_null() ) {
            dialogue d( get_talker_for( get_avatar() ), nullptr );
            effect_on_condition_id eoc = cur_ter->get_entry_EOC();
            eoc->activate_activation_only( d, "OMT movement" );
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
    return g->current_map.get_map();
}

map &reality_bubble()
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
