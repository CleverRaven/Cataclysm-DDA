#include "init.h"

#include "json.h"
#include "filesystem.h"

// can load from json
#include "effect.h"
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
#include "computer.h"
#include "help.h"
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

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <locale> // for loading names

DynamicDataLoader::DynamicDataLoader()
{
}

DynamicDataLoader::~DynamicDataLoader()
{
    reset();
}

DynamicDataLoader &DynamicDataLoader::get_instance()
{
    static DynamicDataLoader theDynamicDataLoader;
    if (theDynamicDataLoader.type_function_map.empty()) {
        theDynamicDataLoader.initialize();
    }
    return theDynamicDataLoader;
}

void DynamicDataLoader::load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type");
    t_type_function_map::iterator it = type_function_map.find(type);
    if (it == type_function_map.end()) {
        jo.throw_error( "unrecognized JSON object", "type" );
    }
    (*it->second)(jo);
}

void load_ingored_type(JsonObject &jo)
{
    // This does nothing!
    // This function is used for types that are to be ignored
    // (for example for testing or for unimplemented types)
    (void) jo;
}

void DynamicDataLoader::initialize()
{
    reset();
    // all of the applicable types that can be loaded, along with their loading functions
    // Add to this as needed with new StaticFunctionAccessors or new ClassFunctionAccessors for new applicable types
    // Static Function Access
    type_function_map["material"] = new StaticFunctionAccessor(&material_type::load_material);
    type_function_map["bionic"] = new StaticFunctionAccessor(&load_bionic);
    type_function_map["profession"] = new StaticFunctionAccessor(&profession::load_profession);
    type_function_map["skill"] = new StaticFunctionAccessor(&Skill::load_skill);
    type_function_map["dream"] = new StaticFunctionAccessor(&load_dream);
    type_function_map["mutation_category"] = new StaticFunctionAccessor(&load_mutation_category);
    type_function_map["mutation"] = new StaticFunctionAccessor(&mutation_branch::load);
    type_function_map["lab_note"] = new StaticFunctionAccessor(&computer::load_lab_note);
    type_function_map["hint"] = new StaticFunctionAccessor(&load_hint);
    type_function_map["furniture"] = new StaticFunctionAccessor(&load_furniture);
    type_function_map["terrain"] = new StaticFunctionAccessor(&load_terrain);
    type_function_map["monstergroup"] = new StaticFunctionAccessor(
        &MonsterGroupManager::LoadMonsterGroup);
    type_function_map["MONSTER_BLACKLIST"] = new StaticFunctionAccessor(
        &MonsterGroupManager::LoadMonsterBlacklist);
    type_function_map["MONSTER_WHITELIST"] = new StaticFunctionAccessor(
        &MonsterGroupManager::LoadMonsterWhitelist);
    type_function_map["speech"] = new StaticFunctionAccessor(&load_speech);
    type_function_map["ammunition_type"] = new StaticFunctionAccessor(
        &ammunition_type::load_ammunition_type);
    type_function_map["scenario"] = new StaticFunctionAccessor(&scenario::load_scenario);
    type_function_map["start_location"] = new StaticFunctionAccessor(&start_location::load_location);

    // json/colors.json would be listed here, but it's loaded before the others (see curses_start_color())
    // Non Static Function Access
    type_function_map["snippet"] = new ClassFunctionAccessor<snippet_library>(&SNIPPET,
            &snippet_library::load_snippet);
    type_function_map["item_group"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_group);
    type_function_map["item_action"] = new ClassFunctionAccessor<item_action_generator>
    ( &item_action_generator::generator(), &item_action_generator::load_item_action );

    type_function_map["vehicle_part"] = new StaticFunctionAccessor( &vpart_info::load );
    type_function_map["vehicle"] = new StaticFunctionAccessor( &vehicle_prototype::load );
    type_function_map["vehicle_group"] = new StaticFunctionAccessor( &VehicleGroup::load );
    type_function_map["vehicle_placement"] = new StaticFunctionAccessor( &VehiclePlacement::load );
    type_function_map["vehicle_spawn"] = new StaticFunctionAccessor( &VehicleSpawn::load );

    type_function_map["trap"] = new StaticFunctionAccessor(&trap::load);
    type_function_map["AMMO"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_ammo);
    type_function_map["GUN"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_gun);
    type_function_map["ARMOR"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_armor);
    type_function_map["TOOL"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_tool);
    type_function_map["TOOL_ARMOR"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_tool_armor);
    type_function_map["BOOK"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_book);
    type_function_map["COMESTIBLE"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_comestible);
    type_function_map["CONTAINER"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_container);
    type_function_map["GUNMOD"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_gunmod);
    type_function_map["GENERIC"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_generic);
    type_function_map["BIONIC_ITEM"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_bionic);
    type_function_map["VAR_VEH_PART"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_veh_part);
    type_function_map["ITEM_CATEGORY"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_category);

    type_function_map["MONSTER"] = new ClassFunctionAccessor<MonsterGenerator>
    (&MonsterGenerator::generator(), &MonsterGenerator::load_monster);
    type_function_map["SPECIES"] = new ClassFunctionAccessor<MonsterGenerator>
    (&MonsterGenerator::generator(), &MonsterGenerator::load_species);

    type_function_map["recipe_category"] = new StaticFunctionAccessor(&load_recipe_category);
    type_function_map["recipe"] = new StaticFunctionAccessor(&load_recipe);
    type_function_map["tool_quality"] = new StaticFunctionAccessor(&quality::load);
    type_function_map["technique"] = new StaticFunctionAccessor(&load_technique);
    type_function_map["martial_art"] = new StaticFunctionAccessor(&load_martial_art);
    type_function_map["effect_type"] = new StaticFunctionAccessor(&load_effect_type);
    type_function_map["tutorial_messages"] =
        new StaticFunctionAccessor(&load_tutorial_messages);
    type_function_map["overmap_terrain"] =
        new StaticFunctionAccessor(&load_overmap_terrain);
    type_function_map["construction"] =
        new StaticFunctionAccessor(&load_construction);
    type_function_map["mapgen"] =
        new StaticFunctionAccessor(&load_mapgen);
    type_function_map["overmap_special"] =
        new StaticFunctionAccessor(&load_overmap_specials);

    type_function_map["region_settings"] = new StaticFunctionAccessor(&load_region_settings);
    type_function_map["region_overlay"] = new StaticFunctionAccessor(&load_region_overlay);
    type_function_map["ITEM_BLACKLIST"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_blacklist);
    type_function_map["ITEM_WHITELIST"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_whitelist);

    // ...unimplemented?
    type_function_map["INSTRUMENT"] = new StaticFunctionAccessor(&load_ingored_type);
    // loaded earlier.
    type_function_map["colordef"] = new StaticFunctionAccessor(&load_ingored_type);
    // mod information, ignored, handled by the mod manager
    type_function_map["MOD_INFO"] = new StaticFunctionAccessor(&load_ingored_type);

    type_function_map["faction"] = new StaticFunctionAccessor(
        &faction::load_faction);
    type_function_map["npc"] = new StaticFunctionAccessor(
        &npc::load_npc);
    type_function_map["talk_topic"] = new StaticFunctionAccessor(
        &load_talk_topic);
    type_function_map["epilogue"] = new StaticFunctionAccessor(
        &epilogue::load_epilogue);

    type_function_map["MONSTER_FACTION"] =
        new StaticFunctionAccessor(&monfactions::load_monster_faction);
    type_function_map["sound_effect"] = new StaticFunctionAccessor(&sfx::load_sound_effects);
    type_function_map["playlist"] = new StaticFunctionAccessor(&sfx::load_playlist);

}

void DynamicDataLoader::reset()
{
    for( auto &elem : type_function_map ) {
        delete elem.second;
    }
    type_function_map.clear();
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
    material_type::reset();
    profession::reset();
    Skill::reset();
    computer::clear_lab_notes();
    dreams.clear();
    clear_hints();
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

    // TODO:
    //    NameGenerator::generator().clear_names();
}

extern void calculate_mapgen_weights();
void DynamicDataLoader::finalize_loaded_data()
{
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
    item_controller->finialize_item_blacklist();
    finalize_recipes();
    finialize_martial_arts();
    check_consistency();
}

void DynamicDataLoader::check_consistency()
{
    item_controller->check_definitions();
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
}
