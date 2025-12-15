/**
* game-scale core saving/loading only.
* See savegame.cpp for game component IO
*/

#include "game.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <ratio>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "achievement.h"
#include "auto_note.h"
#include "auto_pickup.h"
#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_path.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "cata_variant.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "char_validity_check.h"
#include "clzones.h"
#include "coordinates.h"
#include "debug.h"
#include "dependency_tree.h"
#include "diary.h"
#include "effect_on_condition.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "gamemode.h"
#include "get_version.h"
#include "hash_utils.h"
#include "init.h"
#include "input.h"
#include "item.h"
#include "json.h"
#include "loading_ui.h"
#include "magic_enchantment.h"
#include "main_menu.h"
#include "map.h"
#include "mapbuffer.h"
#include "memorial_logger.h"
#include "messages.h"
#include "mod_manager.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "perf.h"
#include "pimpl.h"
#include "popup.h"
#include "safemode_ui.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "uistate.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "worldfactory.h"
#include "zzip.h"

#if defined(_WIN32)
#if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#   include "platform_win.h"
#endif
#   include <tchar.h>
#endif

static const mod_id MOD_INFORMATION_dda( "dda" );

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

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

void game::load_map( const tripoint_abs_sm &pos_sm,
                     const bool pump_events )
{
    map &here = get_map();

    here.load( pos_sm, true, pump_events );
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
                        std::optional<zzip> z = zzip::load( ( save_file_path + zzip_suffix ).get_unrelative_path() );
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
                                            zzip_suffix ).get_unrelative_path();
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

bool game::save_external_options_record()
{
    const cata_path externals = PATH_INFO::world_base_save_path() / PATH_INFO::external_options();
    const bool saved_externals = write_to_file( externals, [&]( std::ostream & fout ) {
        JsonOut jout( fout );
        fout << "# This is saved for debugging purposes only.  Nothing in this file is ever read by the game."
             << std::endl;

        fout << "# This includes the values of all options, including externals, at the exact time of saving.  The purpose of this is to store a record of the game state as it existed."
             << std::endl;

        fout << "# Externals 'forced on' by mods or user edits to data/core/external_options.json would not normally be visible in the save."
             << std::endl;

        jout.start_array();

        const options_manager::options_container &options = get_options().get_raw_options();

        for( const auto &elem : options ) {
            jout.start_object();

            jout.member( "info", elem.second.getTooltip() );
            jout.member( "default", elem.second.getDefaultText( false ) );
            jout.member( "name", elem.first );
            jout.member( "value", elem.second.getValue( true ) );

            jout.end_object();
        }

        jout.end_array();
    }, _( "external options record" ) );

    return saved_externals;
}

bool game::save()
{
    if( save_is_dirty ) {
        popup( _( "The game is in an unsupported state after using debug tools and cannot be saved." ) );
        return false;
    }
    std::chrono::seconds time_since_load =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - time_of_last_load );
    std::chrono::seconds total_time_played = time_played_at_last_load + time_since_load;
    events().send<event_type::game_save>( time_since_load, total_time_played );
    try {
        if( !save_player_data() ||
            !save_achievements() ||
            !save_factions_missions_npcs() ||
            !save_external_options_record() ||
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
