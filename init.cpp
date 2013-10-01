#include "init.h"

#include "json.h"
#include "game.h" // because mi-go parrot requires it!
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
#include "mongroup.h"
#include "monster_factory.h"
#include "file_finder.h"
#include "mapdata.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors

#include "debug.h"


typedef std::string type_string;
std::map<type_string, TFunctor*> type_function_map;

/* Currently just for loading JSON data from files in data/raw */

// TODO: make this actually load files from the named directory
std::vector<std::string> listfiles(std::string const &dirname)
{
    std::vector<std::string> ret = file_finder::get_files_from_path(".json", dirname, true);

    return ret;
}

void load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type", "");

	if ((type_function_map.find(type) != type_function_map.end()))
    {
        (*type_function_map[type])(jo);
    }
    else {
        std::stringstream err;
        err << jo.line_number() << ": ";
        err << "unrecognized JSON object, type: \"" << type << "\"";
        throw err.str();
    }
}

void load_null(JsonObject &jo) {} // empty, just used as a route for unimplemented types

void init_data_structures()
{
    // all of the applicable types that can be loaded, along with their loading functions
    // Add to this as needed with new StaticFunctionAccessors or new ClassFunctionAccessors for new applicable types
    // Static Function Access
    type_function_map["material"] = new StaticFunctionAccessor(&material_type::load_material);
    type_function_map["bionic"] = new StaticFunctionAccessor(&load_bionic);
    type_function_map["profession"] = new StaticFunctionAccessor(&profession::load_profession);
    type_function_map["skill"] = new StaticFunctionAccessor(&Skill::load_skill);
    type_function_map["dream"] = new StaticFunctionAccessor(&load_dream);
    type_function_map["mutation"] = new StaticFunctionAccessor(&load_mutation);
    type_function_map["recipe_category"] = new StaticFunctionAccessor(&load_recipe_category);
    type_function_map["recipe"] = new StaticFunctionAccessor(&load_recipe);
    type_function_map["lab_note"] = new StaticFunctionAccessor(&computer::load_lab_note);
    type_function_map["hint"] = new StaticFunctionAccessor(&load_hint);
    type_function_map["furniture"] = new StaticFunctionAccessor(&load_furniture);
    type_function_map["terrain"] = new StaticFunctionAccessor(&load_terrain);
    type_function_map["monster_group"] = new StaticFunctionAccessor(&MonsterGroupManager::load_monster_group);


    // Non Static Function Access
    type_function_map["snippet"] = new ClassFunctionAccessor<snippet_library>(&SNIPPET, &snippet_library::load_snippet);
    type_function_map["item_group"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_item_group);
    type_function_map["NAME"] = new ClassFunctionAccessor<NameGenerator>(&NameGenerator::generator(), &NameGenerator::load_name_from_json);
    type_function_map["MONSTER_SPECIES"] = new ClassFunctionAccessor<monster_factory>(&monster_factory::factory(), &monster_factory::load_species);
    type_function_map["MONSTER"] = new ClassFunctionAccessor<monster_factory>(&monster_factory::factory(), &monster_factory::load_monster);
    type_function_map["PARROT"] = new ClassFunctionAccessor<game>(g, &game::load_parrot_phrase);
    type_function_map["vehicle"] = new ClassFunctionAccessor<game>(g, &game::cache_vehicles);
    type_function_map["vehicle_part"] = new ClassFunctionAccessor<game>(g, &game::load_vehicle_part);
    type_function_map["AMMO"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_ammo);
    type_function_map["GENERIC"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_generic);
    type_function_map["GUN"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_gun);
    type_function_map["ARMOR"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_armor);
    type_function_map["BOOK"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_book);
    type_function_map["TOOL"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_tool);
    type_function_map["COMESTIBLE"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_comestible);
    type_function_map["CONTAINER"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_container);
    type_function_map["GUNMOD"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_gunmod);

    // Not currently implemented types
    type_function_map["INSTRUMENT"] = new StaticFunctionAccessor(&load_null);

    mutations_category[""].clear();
    init_mutation_parts();
}

void load_json_dir(std::string const &dirname)
{
    // get a list of all files in the directory
    std::vector<std::string> dir = listfiles(dirname);
    // iterate over each file
    std::vector<std::string>::iterator it;
    for (it = dir.begin(); it != dir.end(); it++) {
        // open the file as a stream
        std::ifstream infile(it->c_str(), std::ifstream::in | std::ifstream::binary);
        // parse it
        try {
            JsonIn jsin(&infile);
            load_all_from_json(jsin);
        } catch (std::string e) {
            throw *(it) + ": " + e;
        }
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


