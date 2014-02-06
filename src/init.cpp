#include "init.h"

#include "json.h"
#include "file_finder.h"

// can load from json
#include "effect.h"
#include "material.h"
#include "bionics.h"
#include "profession.h"
#include "skill.h"
#include "mutation.h"
#include "text_snippets.h"
#include "item_factory.h"
#include "crafting.h"
#include "computer.h"
#include "help.h"
#include "mapdata.h"
#include "color.h"
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

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <locale> // for loading names

#include "savegame.h"
#include "file_finder.h"

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

std::map<int, int> reverse_legacy_ter_id;
std::map<int, int> reverse_legacy_furn_id;
/*
 * Populate optional ter_id and furn_id variables
 */
void init_data_mappings()
{
    set_ter_ids();
    set_furn_ids();
    set_oter_ids();
    set_trap_ids();
    // temporary (reliable) kludge until switch statements are rewritten
    std::map<std::string, int> legacy_lookup;
    for(int i = 0; i < num_legacy_ter; i++) {
        legacy_lookup[ legacy_ter_id[i] ] = i;
    }
    reverse_legacy_ter_id.clear();
    for( int i = 0; i < terlist.size(); i++ ) {
        if ( legacy_lookup.find(terlist[i].id) != legacy_lookup.end() ) {
            reverse_legacy_ter_id[ i ] = legacy_lookup[ terlist[i].id ];
        } else {
            reverse_legacy_ter_id[ i ] = 0;
        }
    }
    legacy_lookup.clear();
    for(int i = 0; i < num_legacy_furn; i++) {
        legacy_lookup[ legacy_furn_id[i] ] = i;
    }
    reverse_legacy_furn_id.clear();
    for( int i = 0; i < furnlist.size(); i++ ) {
        if ( legacy_lookup.find(furnlist[i].id) != legacy_lookup.end() ) {
            reverse_legacy_furn_id[ i ] = legacy_lookup[ furnlist[i].id ];
        } else {
            reverse_legacy_furn_id[ i ] = 0;
        }
    }
}

void DynamicDataLoader::load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type");
    t_type_function_map::iterator it = type_function_map.find(type);
    if (it == type_function_map.end()) {
        std::stringstream err;
        err << jo.line_number() << ": ";
        err << "unrecognized JSON object, type: \"" << type << "\"";
        throw err.str();
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
    type_function_map["mutation"] = new StaticFunctionAccessor(&load_mutation);
    type_function_map["lab_note"] = new StaticFunctionAccessor(&computer::load_lab_note);
    type_function_map["hint"] = new StaticFunctionAccessor(&load_hint);
    type_function_map["furniture"] = new StaticFunctionAccessor(&load_furniture);
    type_function_map["terrain"] = new StaticFunctionAccessor(&load_terrain);
    type_function_map["monstergroup"] = new StaticFunctionAccessor(
        &MonsterGroupManager::LoadMonsterGroup);
    type_function_map["speech"] = new StaticFunctionAccessor(&load_speech);
    type_function_map["ammunition_type"] = new StaticFunctionAccessor(&ammunition_type::load_ammunition_type);

    //data/json/colors.json would be listed here, but it's loaded before the others (see curses_start_color())
    // Non Static Function Access
    type_function_map["snippet"] = new ClassFunctionAccessor<snippet_library>(&SNIPPET,
            &snippet_library::load_snippet);
    type_function_map["item_group"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_group);

    type_function_map["vehicle_part"] = new ClassFunctionAccessor<game>(g, &game::load_vehiclepart);
    type_function_map["vehicle"] = new ClassFunctionAccessor<game>(g, &game::load_vehicle);
    type_function_map["trap"] = new ClassFunctionAccessor<game>(g, &game::load_trap);
    type_function_map["AMMO"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_ammo);
    type_function_map["GUN"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_gun);
    type_function_map["ARMOR"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_armor);
    type_function_map["TOOL"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_tool);
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
    type_function_map["tool_quality"] = new StaticFunctionAccessor(&load_quality);
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

    type_function_map["monitems"] = new ClassFunctionAccessor<game>(g, &game::load_monitem);

    type_function_map["region_settings"] = new StaticFunctionAccessor(&load_region_settings);
    type_function_map["ITEM_BLACKLIST"] = new ClassFunctionAccessor<Item_factory>(item_controller,
            &Item_factory::load_item_blacklist);

    // ...unimplemented?
    type_function_map["INSTRUMENT"] = new StaticFunctionAccessor(&load_ingored_type);
    // loaded earlier.
    type_function_map["colordef"] = new StaticFunctionAccessor(&load_ingored_type);
    // mod information, ignored, handled by the mod manager
    type_function_map["MOD_INFO"] = new StaticFunctionAccessor(&load_ingored_type);

    // init maps used for loading json data
    init_martial_arts();
}

void DynamicDataLoader::reset()
{
    for(t_type_function_map::iterator a = type_function_map.begin(); a != type_function_map.end();
        ++a) {
        delete a->second;
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
    str_vec files = file_finder::get_files_from_path(".json", path, true, true);
    if (files.empty()) {
        std::ifstream tmp(path.c_str(), std::ios::in);
        if (tmp) {
            // path is actually a file, don't checking the extension,
            // assume we want to load this file anyway
            files.push_back(path);
        }
    }
    // iterate over each file
    for (size_t i = 0; i < files.size(); i++) {
        const std::string &file = files[i];
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
        } catch (std::string e) {
            DebugLog() << file << ": " << e << "\n";
            throw file + ": " + e;
        }
    }
}

void DynamicDataLoader::load_all_from_json(JsonIn &jsin)
{
    char ch;
    std::string type = "";
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
            std::stringstream err;
            err << jsin.line_number() << ": ";
            err << "expected single-object file but found '";
            err << jsin.peek() << "'";
            throw err.str();
        }
    } else if (ch == '[') {
        jsin.start_array();
        // find type and dispatch each object until array close
        while (!jsin.end_array()) {
            jsin.eat_whitespace();
            ch = jsin.peek();
            if (ch != '{') {
                std::stringstream err;
                err << jsin.line_number() << ": ";
                err << "expected array of objects but found '";
                err << ch << "', not '{'";
                throw err.str();
            }
            JsonObject jo = jsin.get_object();
            load_object(jo);
            jo.finish();
        }
    } else {
        // not an object or an array?
        std::stringstream err;
        err << jsin.line_number() << ": ";
        err << "expected object or array, but found '" << ch << "'";
        throw err.str();
    }
}

#if defined LOCALIZE && ! defined __CYGWIN__
// load names depending on current locale
void init_names()
{
    std::locale loc("");
    std::string loc_name = loc.name();
    if (loc_name == "C") {
        loc_name = "en";
    }
    size_t dotpos = loc_name.find('.');
    if (dotpos != std::string::npos) {
        loc_name = loc_name.substr(0, dotpos);
    }
    // test if a local version exists
    std::string filename = "data/names/" + loc_name + ".json";
    std::ifstream fin(filename.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!fin.good()) {
        // if not, use "en.json"
        filename = "data/names/en.json";
    }
    fin.close();

    load_names_from_file(filename);
}
#else
void init_names()
{
    load_names_from_file("data/names/en.json");
}
#endif

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
    g->mission_types.clear();
    item_controller->clear_items_and_groups();
    mutations_category.clear();
    mutation_data.clear();
    traits.clear();
    reset_bionics();
    clear_tutorial_messages();
    furnlist.clear();
    furnmap.clear();
    terlist.clear();
    termap.clear();
    MonsterGroupManager::ClearMonsterGroups();
    SNIPPET.clear_snippets();
    g->reset_vehicles();
    g->reset_vehicleparts();
    MonsterGenerator::generator().reset();
    reset_recipe_categories();
    reset_recipes();
    reset_recipes_qualities();
    g->reset_monitems();
    g->release_traps();
    reset_constructions();
    reset_overmap_terrain();
    reset_region_settings();
    reset_mapgens();
    reset_effect_types();
    reset_speech();

    // artifacts are not loaded from json, but must be unloaded anyway.
    artifact_itype_ids.clear();
    // TODO:
    //    NameGenerator::generator().clear_names();
}

extern void calculate_mapgen_weights();
extern void init_data_mappings();
void DynamicDataLoader::finalize_loaded_data() {
    g->init_missions(); // Needs overmap terrain.
    init_data_mappings();
    finalize_overmap_terrain();
    calculate_mapgen_weights();
    MonsterGenerator::generator().finalize_mtypes();
    g->finalize_vehicles();
    item_controller->finialize_item_blacklist();
    finalize_recipes();
    check_consistency();
}

void DynamicDataLoader::check_consistency() {
    item_controller->check_itype_definitions();
    item_controller->check_items_of_groups_exist();
    MonsterGenerator::generator().check_monster_definitions();
    MonsterGroupManager::check_group_definitions();
    check_recipe_definitions();
}
