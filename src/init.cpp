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

// need data initialized
#include "bodypart.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors
#include <locale> // for loading names

#include "savegame.h"
#include "file_finder.h"


typedef std::string type_string;
std::map<type_string, unsigned> type_counts;
std::map<type_string, TFunctor*> type_function_map;
std::map<type_string, int> type_delayed_order;
std::vector<std::vector<std::string> > type_delayed;
std::set<std::string> type_ignored;

std::map<int,int> reverse_legacy_ter_id;
std::map<int,int> reverse_legacy_furn_id;
/*
 * Populate optional ter_id and furn_id variables
 */
void init_data_mappings() {
    set_ter_ids();
    set_furn_ids();
    set_oter_ids();
    set_trap_ids();
    finalize_overmap_terrain();
    calculate_mapgen_weights();
// temporary (reliable) kludge until switch statements are rewritten
    std::map<std::string, int> legacy_lookup;
    for(int i=0; i< num_legacy_ter;i++) {
         legacy_lookup[ legacy_ter_id[i] ] = i;
    }
    reverse_legacy_ter_id.clear();
    for( int i=0; i < terlist.size(); i++ ) {
        if ( legacy_lookup.find(terlist[i].id) != legacy_lookup.end() ) {
             reverse_legacy_ter_id[ i ] = legacy_lookup[ terlist[i].id ];
        } else {
             reverse_legacy_ter_id[ i ] = 0;
        }
    }
    legacy_lookup.clear();
    for(int i=0; i< num_legacy_furn;i++) {
         legacy_lookup[ legacy_furn_id[i] ] = i;
    }
    reverse_legacy_furn_id.clear();
    for( int i=0; i < furnlist.size(); i++ ) {
        if ( legacy_lookup.find(furnlist[i].id) != legacy_lookup.end() ) {
             reverse_legacy_furn_id[ i ] = legacy_lookup[ furnlist[i].id ];
        } else {
             reverse_legacy_furn_id[ i ] = 0;
        }
    }
}

void load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type");

    type_counts[type]++;

    if (type_function_map.find(type) != type_function_map.end())
    {
        /*
        if ( initialrun && type_delayed_order.find(type) != type_delayed_order.end() ) {
            type_delayed[ type_delayed_order[type] ].push_back( jo.dump_input() );
            return;
        }
        */
        (*type_function_map[type])(jo);
    } else if ( type_ignored.count(type) == 0) {
        std::stringstream err;
        err << jo.line_number() << ": ";
        err << "unrecognized JSON object, type: \"" << type << "\"";
        throw err.str();
    }
}

void init_data_structures()
{
    type_ignored.insert("colordef");   // loaded earlier.
    type_ignored.insert("INSTRUMENT"); // ...unimplemented?

    const int delayed_queue_size = 3;      // for now this is only 1 depth, then mods + mods delayed (likely overkill);
    //type_delayed_order["vehicle"] = 0;     // after vehicle_parts
    //type_delayed_order["recipe"] = 0;      // after items
    //type_delayed_order["martial_art"] = 0; //

    type_delayed.resize(delayed_queue_size);
    for(int i = 0; i < delayed_queue_size; i++ ) {
        type_delayed[i].clear();
    }

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
    type_function_map["monstergroup"] = new StaticFunctionAccessor(&MonsterGroupManager::LoadMonsterGroup);
    type_function_map["speech"] = new StaticFunctionAccessor(&load_speech);

    //data/json/colors.json would be listed here, but it's loaded before the others (see curses_start_color())
    // Non Static Function Access
    type_function_map["snippet"] = new ClassFunctionAccessor<snippet_library>(&SNIPPET, &snippet_library::load_snippet);
    type_function_map["item_group"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_item_group);

    type_function_map["vehicle_part"] = new ClassFunctionAccessor<game>(g, &game::load_vehiclepart);
    type_function_map["vehicle"] = new ClassFunctionAccessor<game>(g, &game::load_vehicle);
    type_function_map["trap"] = new ClassFunctionAccessor<game>(g, &game::load_trap);
    type_function_map["AMMO"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_ammo);
    type_function_map["GUN"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_gun);
    type_function_map["ARMOR"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_armor);
    type_function_map["TOOL"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_tool);
    type_function_map["BOOK"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_book);
    type_function_map["COMESTIBLE"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_comestible);
    type_function_map["CONTAINER"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_container);
    type_function_map["GUNMOD"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_gunmod);
    type_function_map["GENERIC"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_generic);
    type_function_map["ITEM_CATEGORY"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_item_category);

    type_function_map["MONSTER"] = new ClassFunctionAccessor<MonsterGenerator>(&MonsterGenerator::generator(), &MonsterGenerator::load_monster);
    type_function_map["SPECIES"] = new ClassFunctionAccessor<MonsterGenerator>(&MonsterGenerator::generator(), &MonsterGenerator::load_species);

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

    mutations_category[""].clear();
}

void release_data_structures()
{
    std::map<type_string, TFunctor*>::iterator it;
    for (it = type_function_map.begin(); it != type_function_map.end(); it++) {
        if (it->second != NULL)
            delete it->second;
    }
    type_function_map.clear();

    std::stringstream counts;
    for (std::map<std::string, unsigned>::iterator it = type_counts.begin(); it != type_counts.end(); ++it){
        counts << it->first << " -- " << it->second << "\n";
    }
    DebugLog() << counts.str();
}

void load_json_dir(std::string const &dirname)
{
    //    load_overlay("data/overlay.json");
    // get a list of all files in the directory
    std::vector<std::string> dir =
        file_finder::get_files_from_path(".json", dirname, true, true);
    // iterate over each file
    std::vector<std::string>::iterator it;
    for (it = dir.begin(); it != dir.end(); it++) {
        // open the file as a stream
        std::ifstream infile(it->c_str(), std::ifstream::in | std::ifstream::binary);
        // and stuff it into ram
        std::istringstream iss(
            std::string(
                (std::istreambuf_iterator<char>(infile)),
                std::istreambuf_iterator<char>()
            )
        );
        infile.close();
        // parse it
        try {
            JsonIn jsin(iss);
            load_all_from_json(jsin);
        } catch (std::string e) {
            DebugLog() << *(it) << ": " << e << "\n";
            throw *(it) + ": " + e;
        }
    }

    for( int i = 0; i < type_delayed.size(); i++ ) {
        for ( int d = 0; d < type_delayed[i].size(); d++ ) {
            std::istringstream iss( type_delayed[i][d] );
            try {
                JsonIn jsin(iss);
                load_all_from_json(jsin);
            } catch (std::string e) {
                throw *(it) + ": " + e;
            }
        }
        type_delayed[i].clear();
    }

    if ( t_floor != termap["t_floor"].loadid ) {
        init_data_mappings(); // this should only kick off once
    }
}

void load_all_from_json(JsonIn &jsin)
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

void unload_active_json_data()
{
    material_type::reset();
    profession::reset();
    Skill::reset();
    computer::clear_lab_notes();
    dreams.clear();
    clear_hints();

    // clear bionics
    for (std::map<bionic_id, bionic_data*>::iterator bio = bionics.begin(); bio != bionics.end(); ++bio){
        delete bio->second;
    }
    bionics.clear();

    // clear mutations (traits, mutation categories and mutation data)
    mutations_category.clear();
    mutation_data.clear();
    traits.clear();
    // clear furniture
    furnlist.clear();
    furnmap.clear();
    // clear terrains
    terlist.clear();
    termap.clear();
    // clear monster groups
    MonsterGroupManager::ClearMonsterGroups();
    // clear snippits
    SNIPPET.clear_snippets();
    // clear item groups and items
    item_controller->clear_items_and_groups();

    // clear migo speech
    // FIXME
//    parrotVector.clear();
    // clear out names
//    NameGenerator::generator().clear_names();
    // clear out vehicles and vehicle parts
    vehicle_part_types.clear();
    for (std::map<std::string, vehicle*>::iterator veh = g->vtypes.begin(); veh != g->vtypes.end(); ++veh){
        delete veh->second;
    }
    g->vtypes.clear();
    // clear recipes, recipe categories, and tool qualities
    clear_recipes_categories_qualities();
    // clear techniques, martial arts, and ma buffs
    clear_techniques_and_martial_arts();
    // clear tutorial messages
    clear_tutorial_messages();
    g->mission_types.clear();

    item_controller->reinit();
}
