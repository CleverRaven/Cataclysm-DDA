#include "init.h"

#include "json.h"
#include "filesystem.h"

// can load from json
#include "effect.h"
#include "vitamin.h"
#include "fault.h"
#include "material.h"
#include "bionics.h"
#include "profession.h"
#include "skill.h"
#include "mutation.h"
#include "text_snippets.h"
#include "item_factory.h"
#include "vehicle_group.h"
#include "crafting.h"
#include "crafting_gui.h"
#include "mapdata.h"
#include "color.h"
#include "trap.h"
#include "mission.h"
#include "monstergenerator.h"
#include "inventory.h"
#include "tutorial.h"
#include "overmap.h"
#include "artifact.h"
#include "mapgen.h"
#include "speech.h"
#include "construction.h"
#include "name.h"
#include "ammo.h"
#include "debug.h"
#include "path_info.h"
#include "start_location.h"
#include "scenario.h"
#include "omdata.h"
#include "options.h"
#include "game.h"
#include "faction.h"
#include "npc.h"
#include "item_action.h"
#include "dialogue.h"
#include "mongroup.h"
#include "monfaction.h"
#include "martialarts.h"
#include "veh_type.h"
#include "clzones.h"
#include "sounds.h"
#include "gates.h"
#include "overlay_ordering.h"
#include "worldfactory.h"
#include "npc_class.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <locale> // for loading names

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

void DynamicDataLoader::load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type");
    t_type_function_map::iterator it = type_function_map.find(type);
    if (it == type_function_map.end()) {
        jo.throw_error( "unrecognized JSON object", "type" );
    }
    it->second(jo);
}

void load_ingored_type(JsonObject &jo)
{
    // This does nothing!
    // This function is used for types that are to be ignored
    // (for example for testing or for unimplemented types)
    (void) jo;
}

void DynamicDataLoader::add( const std::string &type, std::function<void(JsonObject&)> f )
{
    const auto pair = type_function_map.insert( std::make_pair( type, std::move( f ) ) );
    if( !pair.second ) {
        debugmsg( "tried to insert a second handler for type %s into the DynamicDataLoader", type.c_str() );
    }
}

void DynamicDataLoader::initialize()
{
    // all of the applicable types that can be loaded, along with their loading functions
    // Add to this as needed with new StaticFunctionAccessors or new ClassFunctionAccessors for new applicable types
    // Static Function Access
    add( "fault", &fault::load_fault );
    add( "vitamin", &vitamin::load_vitamin );
    add( "material", &material_type::load_material );
    add( "bionic", &load_bionic );
    add( "profession", &profession::load_profession );
    add( "skill", &Skill::load_skill );
    add( "dream", &load_dream );
    add( "mutation_category", &load_mutation_category );
    add( "mutation", &mutation_branch::load );
    add( "furniture", &load_furniture );
    add( "terrain", &load_terrain );
    add( "monstergroup", &MonsterGroupManager::LoadMonsterGroup );
    add( "MONSTER_BLACKLIST", &MonsterGroupManager::LoadMonsterBlacklist );
    add( "MONSTER_WHITELIST", &MonsterGroupManager::LoadMonsterWhitelist );
    add( "speech", &load_speech );
    add( "ammunition_type", &ammunition_type::load_ammunition_type );
    add( "scenario", &scenario::load_scenario );
    add( "start_location", &start_location::load_location );

    // json/colors.json would be listed here, but it's loaded before the others (see curses_start_color())
    // Non Static Function Access
    add( "snippet", []( JsonObject &jo ) { SNIPPET.load_snippet( jo ); } );
    add( "item_group", []( JsonObject &jo ) { item_controller->load_item_group( jo ); } );
    add( "item_action", []( JsonObject &jo ) { item_action_generator::generator().load_item_action( jo ); } );

    add( "vehicle_part",  &vpart_info::load );
    add( "vehicle",  &vehicle_prototype::load );
    add( "vehicle_group",  &VehicleGroup::load );
    add( "vehicle_placement",  &VehiclePlacement::load );
    add( "vehicle_spawn",  &VehicleSpawn::load );

    add( "trap", &trap::load );
    add( "AMMO", []( JsonObject &jo ) { item_controller->load_ammo( jo ); } );
    add( "GUN", []( JsonObject &jo ) { item_controller->load_gun( jo ); } );
    add( "ARMOR", []( JsonObject &jo ) { item_controller->load_armor( jo ); } );
    add( "TOOL", []( JsonObject &jo ) { item_controller->load_tool( jo ); } );
    add( "TOOL_ARMOR", []( JsonObject &jo ) { item_controller->load_tool_armor( jo ); } );
    add( "BOOK", []( JsonObject &jo ) { item_controller->load_book( jo ); } );
    add( "COMESTIBLE", []( JsonObject &jo ) { item_controller->load_comestible( jo ); } );
    add( "CONTAINER", []( JsonObject &jo ) { item_controller->load_container( jo ); } );
    add( "ENGINE", []( JsonObject &jo ) { item_controller->load_engine( jo ); } );
    add( "WHEEL", []( JsonObject &jo ) { item_controller->load_wheel( jo ); } );
    add( "GUNMOD", []( JsonObject &jo ) { item_controller->load_gunmod( jo ); } );
    add( "MAGAZINE", []( JsonObject &jo ) { item_controller->load_magazine( jo ); } );
    add( "GENERIC", []( JsonObject &jo ) { item_controller->load_generic( jo ); } );
    add( "BIONIC_ITEM", []( JsonObject &jo ) { item_controller->load_bionic( jo ); } );
    add( "ITEM_CATEGORY", []( JsonObject &jo ) { item_controller->load_item_category( jo ); } );
    add( "MIGRATION", []( JsonObject &jo ) { item_controller->load_migration( jo ); } );

    add( "MONSTER", []( JsonObject &jo ) { MonsterGenerator::generator().load_monster( jo ); } );
    add( "SPECIES", []( JsonObject &jo ) { MonsterGenerator::generator().load_species( jo ); } );

    add( "recipe_category", &load_recipe_category );
    add( "recipe", &load_recipe );
    add( "tool_quality", &quality::load_static );
    add( "technique", &load_technique );
    add( "martial_art", &load_martial_art );
    add( "effect_type", &load_effect_type );
    add( "tutorial_messages", &load_tutorial_messages );
    add( "overmap_terrain", &load_overmap_terrain );
    add( "construction", &load_construction );
    add( "mapgen", &load_mapgen );
    add( "overmap_special", &load_overmap_specials );

    add( "region_settings", &load_region_settings );
    add( "region_overlay", &load_region_overlay );
    add( "ITEM_BLACKLIST", []( JsonObject &jo ) { item_controller->load_item_blacklist( jo ); } );
    add( "ITEM_WHITELIST", []( JsonObject &jo ) { item_controller->load_item_whitelist( jo ); } );
    add( "WORLD_OPTION", &load_world_option );

    // loaded earlier.
    add( "colordef", &load_ingored_type );
    // mod information, ignored, handled by the mod manager
    add( "MOD_INFO", &load_ingored_type );

    add( "faction", &faction::load_faction );
    add( "npc", &npc::load_npc );
    add( "npc_class", &npc_class::load_npc_class );
    add( "talk_topic", &load_talk_topic );
    add( "epilogue", &epilogue::load_epilogue );

    add( "MONSTER_FACTION", &monfactions::load_monster_faction );

    add( "sound_effect", &sfx::load_sound_effects );
    add( "playlist", &sfx::load_playlist );

    add( "gate", &gates::load_gates );
    add( "overlay_order", &load_overlay_ordering );
}

void DynamicDataLoader::load_data_from_path(const std::string &path)
{
    // We assume that each folder is consistent in itself,
    // and all the previously loaded folders.
    // E.g. the core might provide a vpart "frame-x"
    // the first loaded mode might provide a vehicle that uses that frame
    // But not the other way round.

    // get a list of all files in the directory
    str_vec files = get_files_from_path(".json", path, true, true);
    if (files.empty()) {
        std::ifstream tmp(path.c_str(), std::ios::in);
        if (tmp) {
            // path is actually a file, don't checking the extension,
            // assume we want to load this file anyway
            files.push_back(path);
        }
    }
    // iterate over each file
    for( auto &files_i : files ) {
        const std::string &file = files_i;
        // open the file as a stream
        std::ifstream infile(file.c_str(), std::ifstream::in | std::ifstream::binary);
        // and stuff it into ram
        std::istringstream iss(
            std::string(
                (std::istreambuf_iterator<char>(infile)),
                std::istreambuf_iterator<char>()
            )
        );
        try {
            // parse it
            JsonIn jsin(iss);
            load_all_from_json(jsin);
        } catch( const JsonError &err ) {
            throw std::runtime_error( file + ": " + err.what() );
        }
    }
}

void DynamicDataLoader::load_all_from_json(JsonIn &jsin)
{
    char ch;
    jsin.eat_whitespace();
    // examine first non-whitespace char
    ch = jsin.peek();
    if (ch == '{') {
        // find type and dispatch single object
        JsonObject jo = jsin.get_object();
        load_object(jo);
        jo.finish();
        // if there's anything else in the file, it's an error.
        jsin.eat_whitespace();
        if (jsin.good()) {
            jsin.error( string_format( "expected single-object file but found '%c'", jsin.peek() ) );
        }
    } else if (ch == '[') {
        jsin.start_array();
        // find type and dispatch each object until array close
        while (!jsin.end_array()) {
            jsin.eat_whitespace();
            ch = jsin.peek();
            if (ch != '{') {
                jsin.error( string_format( "expected array of objects but found '%c', not '{'", ch ) );
            }
            JsonObject jo = jsin.get_object();
            load_object(jo);
            jo.finish();
        }
    } else {
        // not an object or an array?
        jsin.error( string_format( "expected object or array, but found '%c'", ch ) );
    }
}

void init_names()
{
    const std::string filename = PATH_INFO::find_translated_file( "namesdir",
                                 ".json", "names" );
    load_names_from_file(filename);
}

void DynamicDataLoader::unload_data()
{
    vitamin::reset();
    fault::reset();
    material_type::reset();
    profession::reset();
    Skill::reset();
    dreams.clear();
    // clear techniques, martial arts, and ma buffs
    clear_techniques_and_martial_arts();
    // Mission types are not loaded from json, but they depend on
    // the overmap terrain + items and that gets loaded from json.
    mission_type::reset();
    item_controller->reset();
    mutations_category.clear();
    mutation_category_traits.clear();
    mutation_branch::reset_all();
    reset_bionics();
    clear_tutorial_messages();
    reset_furn_ter();
    MonsterGroupManager::ClearMonsterGroups();
    SNIPPET.clear_snippets();
    vehicle_prototype::reset();
    vpart_info::reset();
    MonsterGenerator::generator().reset();
    reset_recipe_categories();
    reset_recipes();
    quality::reset();
    trap::reset();
    reset_constructions();
    reset_overmap_terrain();
    reset_region_settings();
    reset_mapgens();
    reset_effect_types();
    reset_speech();
    clear_overmap_specials();
    ammunition_type::reset();
    unload_talk_topics();
    start_location::reset();
    scenario::reset();
    gates::reset();
    reset_overlay_ordering();
    npc_class::reset_npc_classes();

    // TODO:
    //    NameGenerator::generator().clear_names();
}

extern void calculate_mapgen_weights();
void DynamicDataLoader::finalize_loaded_data()
{
    item_controller->finalize();
    vpart_info::finalize();
    mission_type::initialize(); // Needs overmap terrain.
    set_ter_ids();
    set_furn_ids();
    set_oter_ids();
    trap::finalize();
    finalize_overmap_terrain();
    vehicle_prototype::finalize();
    calculate_mapgen_weights();
    MonsterGenerator::generator().finalize_mtypes();
    MonsterGroupManager::FinalizeMonsterGroups();
    monfactions::finalize();
    finalize_recipes();
    finialize_martial_arts();
    finalize_constructions();
    check_consistency();
}

void DynamicDataLoader::check_consistency()
{
    vitamin::check_consistency();
    item_controller->check_definitions();
    fault::check_consistency();
    vpart_info::check();
    MonsterGenerator::generator().check_monster_definitions();
    MonsterGroupManager::check_group_definitions();
    check_recipe_definitions();
    check_furniture_and_terrain();
    check_constructions();
    profession::check_definitions();
    scenario::check_definitions();
    check_martialarts();
    mutation_branch::check_consistency();
    ammunition_type::check_consistency();
    trap::check_consistency();
    check_bionics();
    npc_class::check_consistency();
}
