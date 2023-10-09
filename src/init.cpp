#include "init.h"

#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "achievement.h"
#include "activity_type.h"
#include "ammo.h"
#include "ammo_effect.h"
#include "anatomy.h"
#include "ascii_art.h"
#include "behavior.h"
#include "bionics.h"
#include "bodygraph.h"
#include "bodypart.h"
#include "butchery_requirements.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "character_modifier.h"
#include "city.h"
#include "climbing.h"
#include "clothing_mod.h"
#include "clzones.h"
#include "condition.h"
#include "construction.h"
#include "construction_category.h"
#include "construction_group.h"
#include "crafting_gui.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "disease.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "emit.h"
#include "event_statistics.h"
#include "faction.h"
#include "fault.h"
#include "field_type.h"
#include "filesystem.h"
#include "flag.h"
#include "gates.h"
#include "harvest.h"
#include "item_action.h"
#include "item_category.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "loading_ui.h"
#include "lru_cache.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "magic_ter_furn_transform.h"
#include "map_extras.h"
#include "mapdata.h"
#include "mapgen.h"
#include "martialarts.h"
#include "material.h"
#include "mission.h"
#include "math_parser_jmath.h"
#include "mod_tileset.h"
#include "monfaction.h"
#include "mongroup.h"
#include "monstergenerator.h"
#include "mood_face.h"
#include "morale_types.h"
#include "move_mode.h"
#include "mutation.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "overlay_ordering.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "overmap_location.h"
#include "path_info.h"
#include "profession.h"
#include "profession_group.h"
#include "proficiency.h"
#include "recipe_dictionary.h"
#include "recipe_groups.h"
#include "regional_settings.h"
#include "relic.h"
#include "requirements.h"
#include "rotatable_symbols.h"
#include "scenario.h"
#include "scent_map.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "skill.h"
#include "skill_boost.h"
#include "sounds.h"
#include "speech.h"
#include "speed_description.h"
#include "start_location.h"
#include "string_formatter.h"
#include "test_data.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle_group.h"
#include "vitamin.h"
#include "weakpoint.h"
#include "weather_type.h"
#include "widget.h"
#include "worldfactory.h"

DynamicDataLoader::DynamicDataLoader()
{
    initialize();
}

DynamicDataLoader::~DynamicDataLoader() = default;

DynamicDataLoader &DynamicDataLoader::get_instance()
{
    static DynamicDataLoader theDynamicDataLoader;
    return theDynamicDataLoader;
}

void DynamicDataLoader::load_object( const JsonObject &jo, const std::string &src,
                                     const cata_path &base_path,
                                     const cata_path &full_path )
{
    const std::string type = jo.get_string( "type" );
    const t_type_function_map::iterator it = type_function_map.find( type );
    if( it == type_function_map.end() ) {
        jo.throw_error_at( "type", "unrecognized JSON object" );
    }
    it->second( jo, src, base_path, full_path );
}

struct DynamicDataLoader::cached_streams {
    lru_cache<std::string, shared_ptr_fast<std::istringstream>> cache;
};

shared_ptr_fast<std::istream> DynamicDataLoader::get_cached_stream( const std::string &path )
{
    cata_assert( !finalized &&
                 "Cannot open data file after finalization." );
    cata_assert( stream_cache &&
                 "Stream cache is only available during finalization" );
    shared_ptr_fast<std::istringstream> cached = stream_cache->cache.get( path, nullptr );
    // Create a new stream if the file is not opened yet, or if some code is still
    // using the previous stream (in such case, `cached` and `stream_cache` have
    // two references to the stream, hence the test for > 2).
    if( !cached ) {
        cached = make_shared_fast<std::istringstream>( read_entire_file( path ) );
    } else if( cached.use_count() > 2 ) {
        cached = make_shared_fast<std::istringstream>( cached->str() );
    }
    stream_cache->cache.insert( 8, path, cached );
    return cached;
}

void DynamicDataLoader::load_deferred( deferred_json &data )
{
    while( !data.empty() ) {
        const size_t n = data.size();
        auto it = data.begin();
        for( size_t idx = 0; idx != n; ++idx ) {
            try {
                const JsonObject &jo = it->first;
                load_object( jo, it->second );
            } catch( const JsonError &err ) {
                debugmsg( "(json-error)\n%s", err.what() );
            }
            ++it;
            inp_mngr.pump_events();
        }
        data.erase( data.begin(), it );
        if( data.size() == n ) {
            for( const auto &elem : data ) {
                try {
                    elem.first.throw_error( "JSON contains circular dependency, this object is discarded" );
                } catch( const JsonError &err ) {
                    debugmsg( "(json-error)\n%s", err.what() );
                }
                inp_mngr.pump_events();
            }
            data.clear();
            return; // made no progress on this cycle so abort
        }
    }
}

static void load_ignored_type( const JsonObject &jo )
{
    // This does nothing!
    // This function is used for types that are to be ignored
    // (for example for testing or for unimplemented types)
    jo.allow_omitted_members();
}

void DynamicDataLoader::add( const std::string &type,
                             const std::function<void( const JsonObject &, const std::string &, const cata_path &, const cata_path & )>
                             &f )
{
    const auto pair = type_function_map.emplace( type, f );
    if( !pair.second ) {
        debugmsg( "tried to insert a second handler for type %s into the DynamicDataLoader", type.c_str() );
    }
}

void DynamicDataLoader::add( const std::string &type,
                             const std::function<void( const JsonObject &, const std::string &, const std::string &, const std::string & )>
                             &f )
{
    add( type, [f]( const JsonObject & obj, const std::string & src, const cata_path & base_path,
    const cata_path & full_path ) {
        f( obj, src, base_path.generic_u8string(), full_path.generic_u8string() );
    } );
}

void DynamicDataLoader::add( const std::string &type,
                             const std::function<void( const JsonObject &, const std::string & )> &f )
{
    add( type, [f]( const JsonObject & obj, const std::string & src, const cata_path &,
    const cata_path & ) {
        f( obj, src );
    } );
}

void DynamicDataLoader::add( const std::string &type,
                             const std::function<void( const JsonObject & )> &f )
{
    add( type, [f]( const JsonObject & obj, const std::string_view,  const cata_path &,
    const cata_path & ) {
        f( obj );
    } );
}

void DynamicDataLoader::initialize()
{
    // Initialize loading data that must be in place before any loading functions are called
    init_mapdata();

    // all of the applicable types that can be loaded, along with their loading functions
    // Add to this as needed with new StaticFunctionAccessors or new ClassFunctionAccessors for new applicable types
    // Static Function Access
    add( "WORLD_OPTION", &load_world_option );
    add( "EXTERNAL_OPTION", &load_external_option );
    add( "option_slider", &option_slider::load_option_sliders );
    add( "json_flag", &json_flag::load_all );
    add( "jmath_function", &jmath_func::load_func );
    add( "var_migration", &global_variables::load_migrations );
    add( "connect_group", &connect_group::load );
    add( "fault", &fault::load );
    add( "fault_fix", &fault_fix::load );
    add( "relic_procgen_data", &relic_procgen_data::load_relic_procgen_data );
    add( "effect_on_condition", &effect_on_conditions::load );
    add( "field_type", &field_types::load );
    add( "weather_type", &weather_types::load );
    add( "ammo_effect", &ammo_effects::load );
    add( "emit", &emit::load_emit );
    add( "activity_type", &activity_type::load );
    add( "addiction_type", &add_type::load_add_types );
    add( "movement_mode", &move_mode::load_move_mode );
    add( "vitamin", &vitamin::load_vitamin );
    add( "material", &materials::load );
    add( "bionic", &bionic_data::load_bionic );
    add( "bionic_migration", &bionic_data::load_bionic_migration );
    add( "profession", &profession::load_profession );
    add( "profession_group", &profession_group::load_profession_group );
    add( "profession_item_substitutions", &profession::load_item_substitutions );
    add( "proficiency", &proficiency::load_proficiencies );
    add( "proficiency_category", &proficiency_category::load_proficiency_categories );
    add( "speed_description", &speed_description::load_speed_descriptions );
    add( "mood_face", &mood_face::load_mood_faces );
    add( "skill", &Skill::load_skill );
    add( "skill_display_type", &SkillDisplayType::load );
    add( "dream", &dream::load );
    add( "mutation_category", &mutation_category_trait::load );
    add( "mutation_type", &load_mutation_type );
    add( "mutation", &mutation_branch::load_trait );
    add( "furniture", &load_furniture );
    add( "terrain", &load_terrain );
    add( "monstergroup", &MonsterGroupManager::LoadMonsterGroup );
    add( "MONSTER_BLACKLIST", &MonsterGroupManager::LoadMonsterBlacklist );
    add( "MONSTER_WHITELIST", &MonsterGroupManager::LoadMonsterWhitelist );
    add( "speech", &load_speech );
    add( "ammunition_type", &ammunition_type::load_ammunition_type );
    add( "start_location", &start_locations::load );
    add( "scenario", &scenario::load_scenario );
    add( "SCENARIO_BLACKLIST", &scen_blacklist::load_scen_blacklist );
    add( "shopkeeper_blacklist", &shopkeeper_blacklist::load_blacklist );
    add( "shopkeeper_consumption_rates", &shopkeeper_cons_rates::load_rate );
    add( "skill_boost", &skill_boost::load_boost );
    add( "enchantment", &enchantment::load_enchantment );
    add( "hit_range", &Creature::load_hit_range );
    add( "scent_type", &scent_type::load_scent_type );
    add( "disease_type", &disease_type::load_disease_type );
    add( "ascii_art", &ascii_art::load_ascii_art );

    // json/colors.json would be listed here, but it's loaded before the others (see init_colors())
    // Non Static Function Access
    add( "snippet", []( const JsonObject & jo ) {
        SNIPPET.load_snippet( jo );
    } );
    add( "item_group", []( const JsonObject & jo ) {
        item_controller->load_item_group( jo );
    } );
    add( "trait_group", []( const JsonObject & jo ) {
        mutation_branch::load_trait_group( jo );
    } );
    add( "item_action", []( const JsonObject & jo ) {
        item_action_generator::generator().load_item_action( jo );
    } );

    add( "vehicle_part",  &vehicles::parts::load );
    add( "vehicle_part_category",  &vpart_category::load );
    add( "vehicle_part_migration", &vpart_migration::load );
    add( "vehicle", &vehicles::load_prototype );
    add( "vehicle_group",  &VehicleGroup::load );
    add( "vehicle_placement",  &VehiclePlacement::load );
    add( "vehicle_spawn",  &VehicleSpawn::load );

    add( "requirement", []( const JsonObject & jo ) {
        requirement_data::load_requirement( jo, string_id<requirement_data>::NULL_ID(), true );
    } );
    add( "trap", &trap::load_trap );

    add( "AMMO", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_ammo( jo, src );
    } );
    add( "GUN", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_gun( jo, src );
    } );
    add( "ARMOR", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_armor( jo, src );
    } );
    add( "PET_ARMOR", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_pet_armor( jo, src );
    } );
    add( "TOOL", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_tool( jo, src );
    } );
    add( "TOOLMOD", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_toolmod( jo, src );
    } );
    add( "TOOL_ARMOR", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_tool_armor( jo, src );
    } );
    add( "BOOK", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_book( jo, src );
    } );
    add( "COMESTIBLE", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_comestible( jo, src );
    } );
    add( "ENGINE", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_engine( jo, src );
    } );
    add( "WHEEL", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_wheel( jo, src );
    } );
    add( "GUNMOD", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_gunmod( jo, src );
    } );
    add( "MAGAZINE", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_magazine( jo, src );
    } );
    add( "BATTERY", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_battery( jo, src );
    } );
    add( "GENERIC", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_generic( jo, src );
    } );
    add( "BIONIC_ITEM", []( const JsonObject & jo, const std::string & src ) {
        item_controller->load_bionic( jo, src );
    } );

    add( "ITEM_CATEGORY", &item_category::load_item_cat );

    add( "MIGRATION", []( const JsonObject & jo ) {
        item_controller->load_migration( jo );
    } );

    add( "charge_removal_blacklist", load_charge_removal_blacklist );
    add( "temperature_removal_blacklist", load_temperature_removal_blacklist );
    add( "test_data", &test_data::load );

    add( "MONSTER", []( const JsonObject & jo, const std::string & src ) {
        MonsterGenerator::generator().load_monster( jo, src );
    } );
    add( "SPECIES", []( const JsonObject & jo, const std::string & src ) {
        MonsterGenerator::generator().load_species( jo, src );
    } );
    add( "monster_flag", &mon_flag::load_mon_flags );

    add( "LOOT_ZONE", &zone_type::load_zones );
    add( "monster_adjustment", &load_monster_adjustment );
    add( "recipe_category", &load_recipe_category );
    add( "recipe",  &recipe_dictionary::load_recipe );
    add( "uncraft", &recipe_dictionary::load_uncraft );
    add( "practice", &recipe_dictionary::load_practice );
    add( "nested_category", &recipe_dictionary::load_nested_category );
    add( "recipe_group",  &recipe_group::load );

    add( "tool_quality", &quality::load_static );
    add( "technique", &load_technique );
    add( "weapon_category", &weapon_category::load_weapon_categories );
    add( "martial_art", &load_martial_art );
    add( "climbing_aid", &climbing_aid::load_climbing_aid );
    add( "effect_type", &load_effect_type );
    add( "oter_id_migration", &overmap::load_oter_id_migration );
    add( "overmap_terrain", &overmap_terrains::load );
    add( "construction_category", &construction_categories::load );
    add( "construction_group", &construction_groups::load );
    add( "construction", &load_construction );
    add( "mapgen", &load_mapgen );
    add( "overmap_land_use_code", &overmap_land_use_codes::load );
    add( "overmap_connection", &overmap_connections::load );
    add( "overmap_location", &overmap_locations::load );
    add( "city", &city::load_city );
    add( "overmap_special", &overmap_specials::load );
    add( "overmap_special_migration", &overmap_special_migration::load_migrations );
    add( "city_building", &city_buildings::load );
    add( "map_extra", &MapExtras::load );

    add( "region_settings", &load_region_settings );
    add( "region_overlay", &load_region_overlay );
    add( "ITEM_BLACKLIST", []( const JsonObject & jo ) {
        item_controller->load_item_blacklist( jo );
    } );
    add( "TRAIT_BLACKLIST", []( const JsonObject & jo ) {
        mutation_branch::load_trait_blacklist( jo );
    } );

    add( "TRAIT_MIGRATION", []( const JsonObject & jo ) {
        mutation_branch::load_trait_migration( jo );
    } );

    // loaded earlier.
    add( "colordef", &load_ignored_type );
    // mod information, ignored, handled by the mod manager
    add( "MOD_INFO", &load_ignored_type );

    add( "faction", &faction_template::load );
    add( "npc", &npc_template::load );
    add( "npc_class", &npc_class::load_npc_class );
    add( "talk_topic", &load_talk_topic );
    add( "behavior", &behavior::load_behavior );

    add( "MONSTER_FACTION", &monfactions::load_monster_faction );

    add( "sound_effect", &sfx::load_sound_effects );
    add( "sound_effect_preload", &sfx::load_sound_effect_preload );
    add( "playlist", &sfx::load_playlist );

    add( "gate", &gates::load );
    add( "overlay_order", &load_overlay_ordering );
    add( "mission_definition", []( const JsonObject & jo, const std::string & src ) {
        mission_type::load_mission_type( jo, src );
    } );
    add( "butchery_requirement", &butchery_requirements::load_butchery_req );
    add( "harvest_drop_type", &harvest_drop_type::load_harvest_drop_types );
    add( "harvest", &harvest_list::load_harvest_list );
    add( "monster_attack", []( const JsonObject & jo, const std::string & src ) {
        MonsterGenerator::generator().load_monster_attack( jo, src );
    } );
    add( "palette", mapgen_palette::load );
    add( "rotatable_symbol", &rotatable_symbols::load );
    add( "limb_score", &limb_score::load_limb_scores );
    add( "character_mod", &character_modifier::load_character_modifiers );
    add( "body_part", &body_part_type::load_bp );
    add( "sub_body_part", &sub_body_part_type::load_bp );
    add( "body_graph", &bodygraph::load_bodygraphs );
    add( "anatomy", &anatomy::load_anatomy );
    add( "morale_type", &morale_type_data::load_type );
    add( "SPELL", &spell_type::load_spell );
    add( "clothing_mod", &clothing_mods::load );
    add( "ter_furn_transform", &ter_furn_transform::load_transform );
    add( "event_transformation", &event_transformation::load_transformation );
    add( "event_statistic", &event_statistic::load_statistic );
    add( "score", &score::load_score );
    add( "achievement", &achievement::load_achievement );
    add( "conduct", &achievement::load_achievement );
    add( "widget", &widget::load_widget );
    add( "weakpoint_set", &weakpoints::load_weakpoint_sets );
    add( "damage_type", &damage_type::load_damage_types );
    add( "damage_info_order", &damage_info_order::load_damage_info_orders );
#if defined(TILES)
    add( "mod_tileset", &load_mod_tileset );
#else
    // Dummy function
    add( "mod_tileset", load_ignored_type );
#endif
}

void DynamicDataLoader::load_data_from_path( const cata_path &path, const std::string &src,
        loading_ui &ui )
{
    cata_assert( !finalized &&
                 "Can't load additional data after finalization.  Must be unloaded first." );
    // We assume that each folder is consistent in itself,
    // and all the previously loaded folders.
    // E.g. the core might provide a vpart "frame-x"
    // the first loaded mode might provide a vehicle that uses that frame
    // But not the other way round.

    std::vector<cata_path> files;
    if( dir_exist( path.get_unrelative_path() ) ) {
        const std::vector<cata_path> dir_files = get_files_from_path( ".json", path, true, true );
        files.insert( files.end(), dir_files.begin(), dir_files.end() );
    } else if( file_exist( path.get_unrelative_path() ) ) {
        files.emplace_back( path );
    }

    // iterate over each file
    for( const cata_path &file : files ) {
        try {
            // parse it
            JsonValue jsin = json_loader::from_path( file );
            load_all_from_json( jsin, src, ui, path, file );
        } catch( const JsonError &err ) {
            throw std::runtime_error( err.what() );
        }
    }
}

void DynamicDataLoader::load_all_from_json( const JsonValue &jsin, const std::string &src,
        loading_ui &,
        const cata_path &base_path, const cata_path &full_path )
{
    if( jsin.test_object() ) {
        // find type and dispatch single object
        JsonObject jo = jsin.get_object();
        load_object( jo, src, base_path, full_path );
    } else if( jsin.test_array() ) {
        JsonArray ja = jsin.get_array();
        // find type and dispatch each object until array close
        for( JsonObject jo : ja ) {
            load_object( jo, src, base_path, full_path );
        }
    } else {
        // not an object or an array?
        jsin.throw_error( "expected object or array" );
    }
    inp_mngr.pump_events();
}

void DynamicDataLoader::unload_data()
{
    finalized = false;

    achievement::reset();
    activity_type::reset();
    add_type::reset();
    ammo_effects::reset();
    ammunition_type::reset();
    anatomy::reset();
    ascii_art::reset();
    behavior::reset();
    body_part_type::reset();
    butchery_requirements::reset();
    sub_body_part_type::reset();
    bodygraph::reset();
    climbing_aid::reset();
    weapon_category::reset();
    clear_techniques_and_martial_arts();
    character_modifier::reset();
    clothing_mods::reset();
    construction_categories::reset();
    construction_groups::reset();
    damage_type::reset();
    damage_info_order::reset();
    disease_type::reset();
    dreams.clear();
    emit::reset();
    enchantment::reset();
    event_statistic::reset();
    effect_on_conditions::reset();
    event_transformation::reset();
    faction_template::reset();
    fault_fix::reset();
    fault::reset();
    field_types::reset();
    gates::reset();
    harvest_drop_type::reset();
    harvest_list::reset();
    item_category::reset();
    item_controller->reset();
    jmath_func::reset();
    json_flag::reset();
    connect_group::reset();
    limb_score::reset();
    mapgen_palette::reset();
    materials::reset();
    mission_type::reset();
    move_mode::reset();
    monfactions::reset();
    mon_flag::reset();
    MonsterGenerator::generator().reset();
    MonsterGroupManager::ClearMonsterGroups();
    morale_type_data::reset();
    mutation_branch::reset_all();
    mutation_category_trait::reset();
    mutations_category.clear();
    npc_class::reset_npc_classes();
    npc_template::reset();
    option_slider::reset();
    overmap_connections::reset();
    overmap_land_use_codes::reset();
    overmap_locations::reset();
    city::reset();
    overmap_specials::reset();
    overmap_special_migration::reset();
    overmap_terrains::reset();
    overmap::reset_oter_id_migrations();
    profession::reset();
    proficiency::reset();
    proficiency_category::reset();
    mood_face::reset();
    speed_description::reset();
    quality::reset();
    reset_monster_adjustment();
    recipe_dictionary::reset();
    recipe_group::reset();
    relic_procgen_data::reset();
    requirement_data::reset();
    reset_bionics();
    reset_constructions();
    reset_effect_types();
    reset_furn_ter();
    reset_mapgens();
    MapExtras::clear();
    reset_mod_tileset();
    reset_overlay_ordering();
    reset_recipe_categories();
    reset_region_settings();
    reset_scenarios_blacklist();
    reset_speech();
    rotatable_symbols::reset();
    scenario::reset();
    scent_type::reset();
    score::reset();
    shopkeeper_blacklist::reset();
    shopkeeper_cons_rates::reset();
    Skill::reset();
    skill_boost::reset();
    SNIPPET.clear_snippets();
    spell_type::reset_all();
    start_locations::reset();
    ter_furn_transform::reset();
    trap::reset();
    unload_talk_topics();
    VehicleGroup::reset();
    VehiclePlacement::reset();
    VehicleSpawn::reset();
    vehicles::reset_prototypes();
    vitamin::reset();
    vehicles::parts::reset();
    vpart_category::reset();
    vpart_migration::reset();
    weakpoints::reset();
    weather_types::reset();
    widget::reset();
    zone_type::reset();
}

void DynamicDataLoader::finalize_loaded_data()
{
    // Create a dummy that will not display anything
    // TODO: Make it print to stdout?
    loading_ui ui( false );
    finalize_loaded_data( ui );
}

void DynamicDataLoader::finalize_loaded_data( loading_ui &ui )
{
    cata_assert( !finalized && "Can't finalize the data twice." );
    cata_assert( !stream_cache && "Expected stream cache to be null before finalization" );

    on_out_of_scope reset_stream_cache( [this]() {
        stream_cache.reset();
    } );
    stream_cache = std::make_unique<cached_streams>();

    ui.new_context( _( "Finalizing" ) );

    using named_entry = std::pair<std::string, std::function<void()>>;
    const std::vector<named_entry> entries = {{
            { _( "Flags" ), &json_flag::finalize_all },
            { _( "Option sliders" ), &option_slider::finalize_all },
            { _( "Body parts" ), &body_part_type::finalize_all },
            { _( "Sub body parts" ), &sub_body_part_type::finalize_all },
            { _( "Body graphs" ), &bodygraph::finalize_all },
            { _( "Bionics" ), &bionic_data::finalize_bionic },
            { _( "Weather types" ), &weather_types::finalize_all },
            { _( "Effect on conditions" ), &effect_on_conditions::finalize_all },
            { _( "Field types" ), &field_types::finalize_all },
            { _( "Ammo effects" ), &ammo_effects::finalize_all },
            { _( "Emissions" ), &emit::finalize },
            { _( "Materials" ), &material_type::finalize_all },
            {
                _( "Items" ), []()
                {
                    item_controller->finalize();
                }
            },
            {
                _( "Crafting requirements" ), []()
                {
                    requirement_data::finalize();
                }
            },
            { _( "Vehicle part categories" ), &vpart_category::finalize },
            { _( "Vehicle parts" ), &vehicles::parts::finalize },
            { _( "Traps" ), &trap::finalize },
            { _( "Terrain" ), &set_ter_ids },
            { _( "Furniture" ), &set_furn_ids },
            { _( "Overmap land use codes" ), &overmap_land_use_codes::finalize },
            { _( "Overmap terrain" ), &overmap_terrains::finalize },
            { _( "Overmap connections" ), &overmap_connections::finalize },
            { _( "Overmap specials" ), &overmap_specials::finalize },
            { _( "Overmap locations" ), &overmap_locations::finalize },
            { _( "Cities" ), &city::finalize },
            { _( "Math functions" ), &jmath_func::finalize },
            { _( "Math expressions" ), &finalize_conditions },
            { _( "Start locations" ), &start_locations::finalize_all },
            { _( "Vehicle part migrations" ), &vpart_migration::finalize },
            { _( "Vehicle prototypes" ), &vehicles::finalize_prototypes },
            { _( "Mapgen weights" ), &calculate_mapgen_weights },
            { _( "Mapgen parameters" ), &overmap_specials::finalize_mapgen_parameters },
            { _( "Behaviors" ), &behavior::finalize },
            {
                _( "Monster types" ), []()
                {
                    MonsterGenerator::generator().finalize_mtypes();
                }
            },
            { _( "Monster groups" ), &MonsterGroupManager::FinalizeMonsterGroups },
            { _( "Monster factions" ), &monfactions::finalize },
            { _( "Factions" ), &npc_factions::finalize },
            { _( "Move modes" ), &move_mode::finalize },
            { _( "Constructions" ), &finalize_constructions },
            { _( "Crafting recipes" ), &recipe_dictionary::finalize },
            { _( "Recipe groups" ), &recipe_group::check },
            { _( "Martial arts" ), &finalize_martial_arts },
            { _( "Climbing aids" ), &climbing_aid::finalize },
            { _( "NPC classes" ), &npc_class::finalize_all },
            { _( "Missions" ), &mission_type::finalize },
            { _( "Harvest lists" ), &harvest_list::finalize_all },
            { _( "Anatomies" ), &anatomy::finalize_all },
            { _( "Mutations" ), &mutation_branch::finalize_all },
            { _( "Achievements" ), &achievement::finalize },
            { _( "Damage info orders" ), &damage_info_order::finalize_all },
            { _( "Widgets" ), &widget::finalize },
            { _( "Fault fixes" ), &fault_fix::finalize },
#if defined(TILES)
            { _( "Tileset" ), &load_tileset },
#endif
        }
    };

    for( const named_entry &e : entries ) {
        ui.add_entry( e.first );
    }

    ui.show();
    for( const named_entry &e : entries ) {
        e.second();
        ui.proceed();
    }

    check_consistency( ui );
    finalized = true;
}

void DynamicDataLoader::check_consistency( loading_ui &ui )
{
    ui.new_context( _( "Verifying" ) );

    using named_entry = std::pair<std::string, std::function<void()>>;
    const std::vector<named_entry> entries = {{
            { _( "Flags" ), &json_flag::check_consistency },
            { _( "Option sliders" ), &option_slider::check_consistency },
            {
                _( "Crafting requirements" ), []()
                {
                    requirement_data::check_consistency();
                }
            },
            { _( "Vitamins" ), &vitamin::check_consistency },
            { _( "Weather types" ), &weather_types::check_consistency },
            { _( "Effect on conditions" ), &effect_on_conditions::check_consistency },
            { _( "Field types" ), &field_types::check_consistency },
            { _( "Ammo effects" ), &ammo_effects::check_consistency },
            { _( "Emissions" ), &emit::check_consistency },
            { _( "Effect types" ), &effect_type::check_consistency },
            { _( "Activities" ), &activity_type::check_consistency },
            { _( "Addiction types" ), &add_type::check_add_types },
            {
                _( "Items" ), []()
                {
                    item_controller->check_definitions();
                }
            },
            { _( "Materials" ), &materials::check },
            { _( "Faults" ), &fault::check_consistency },
            { _( "Fault fixes" ), &fault_fix::check_consistency },
            { _( "Vehicle parts" ), &vehicles::parts::check },
            { _( "Vehicle part migrations" ), &vpart_migration::check },
            { _( "Mapgen definitions" ), &check_mapgen_definitions },
            { _( "Mapgen palettes" ), &mapgen_palette::check_definitions },
            {
                _( "Monster types" ), []()
                {
                    MonsterGenerator::generator().check_monster_definitions();
                }
            },
            { _( "Monster groups" ), &MonsterGroupManager::check_group_definitions },
            { _( "Furniture and terrain" ), &check_furniture_and_terrain },
            { _( "Constructions" ), &check_constructions },
            { _( "Crafting recipes" ), &recipe_dictionary::check_consistency },
            { _( "Professions" ), &profession::check_definitions },
            { _( "Profession groups" ), &profession_group::check_profession_group_consistency },
            { _( "Scenarios" ), &scenario::check_definitions },
            { _( "Martial arts" ), &check_martialarts },
            { _( "Climbing aid" ), &climbing_aid::check_consistency },
            { _( "Mutations" ), &mutation_branch::check_consistency },
            { _( "Mutation categories" ), &mutation_category_trait::check_consistency },
            { _( "Region settings" ), check_region_settings },
            { _( "Overmap land use codes" ), &overmap_land_use_codes::check_consistency },
            { _( "Overmap connections" ), &overmap_connections::check_consistency },
            { _( "Overmap terrain" ), &overmap_terrains::check_consistency },
            { _( "Overmap locations" ), &overmap_locations::check_consistency },
            { _( "Cities" ), &city::check_consistency },
            { _( "Overmap specials" ), &overmap_specials::check_consistency },
            { _( "Map extras" ), &MapExtras::check_consistency },
            { _( "Shop rates" ), &shopkeeper_cons_rates::check_all },
            { _( "Start locations" ), &start_locations::check_consistency },
            { _( "Ammunition types" ), &ammunition_type::check_consistency },
            { _( "Traps" ), &trap::check_consistency },
            { _( "Bionics" ), &bionic_data::check_bionic_consistency },
            { _( "Gates" ), &gates::check },
            { _( "NPC classes" ), &npc_class::check_consistency },
            { _( "Behaviors" ), &behavior::check_consistency },
            { _( "Mission types" ), &mission_type::check_consistency },
            {
                _( "Item actions" ), []()
                {
                    item_action_generator::generator().check_consistency();
                }
            },
            { _( "Harvest lists" ), &harvest_list::check_consistency },
            { _( "NPC templates" ), &npc_template::check_consistency },
            { _( "Body parts" ), &body_part_type::check_consistency },
            { _( "Body graphs" ), &bodygraph::check_all },
            { _( "Anatomies" ), &anatomy::check_consistency },
            { _( "Spells" ), &spell_type::check_consistency },
            { _( "Transformations" ), &event_transformation::check_consistency },
            { _( "Statistics" ), &event_statistic::check_consistency },
            { _( "Scent types" ), &scent_type::check_scent_consistency },
            { _( "Scores" ), &score::check_consistency },
            { _( "Achievements" ), &achievement::check_consistency },
            { _( "Disease types" ), &disease_type::check_disease_consistency },
            { _( "Factions" ), &faction_template::check_consistency },
            { _( "Damage types" ), &damage_type::check }
        }
    };

    for( const named_entry &e : entries ) {
        ui.add_entry( e.first );
    }

    ui.show();
    for( const named_entry &e : entries ) {
        e.second();
        ui.proceed();
    }
}
