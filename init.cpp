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
    std::string type = jo.get_string("type");
/* -- Keep around temporarily until all necessary additions are made. Don't want to forget something!
    if (type == "material") { material_type::load_material(jo);}
    else if (type == "NAME"){NameGenerator::generator().load_name_from_json(jo);}
    else if (type == "recipe") { load_recipe(jo); }
    else if (type == "ITEM_COMESTIBLE"){item_controller->load_comestible(jo);}
    else if (type == "PARROT"){g->load_parrot_phrase(jo);}
    else if (type == "ITEM_TOOL"){item_controller->load_tool(jo);}
    else if (type == "ITEM_ARMOR"){item_controller->load_armor(jo);}
    else if (type == "mutation") { load_mutation(jo); }
    else if (type == "item_group") { item_controller->load_item_group(jo); }
    else if (type == "ITEM_GENERIC"){item_controller->load_generic(jo);}
    else if (type == "ITEM_AMMO") {item_controller->load_ammo(jo);}
    else if (type == "MONSTER"){monster_factory::factory().load_monster(jo);}
    else if (type == "ITEM_GUN"){item_controller->load_gun(jo);}
    else if (type == "hint") { load_hint(jo); }
    else if (type == "bionic") { load_bionic(jo); }
    else if (type == "vehicle_part"){g->load_vehicle_part(jo);}
    else if (type == "ITEM_BOOK"){item_controller->load_book(jo);}
	else if (type == "lab_note") { computer::load_lab_note(jo); }
    else if (type == "profession") { profession::load_profession(jo); }
    else if (type == "ITEM_GUNMOD"){item_controller->load_gunmod(jo);}
    else if (type == "dream") { load_dream(jo); }
    else if (type == "skill") { Skill::load_skill(jo); }
    else if (type == "vehicle"){ g->cache_vehicles(jo);} // for now don't load vehicles...
    else if (type == "monster_group") {MonsterGroupManager::load_monster_group(jo);}
    else if (type == "snippet") { SNIPPET.load_snippet(jo); }
    else if (type == "ITEM_CONTAINER"){item_controller->load_container(jo);}
    else if (type == "recipe_category") { load_recipe_category(jo); }
    else if (type == "MONSTER_SPECIES"){monster_factory::factory().load_species(jo);}
    else if (type == "ITEM_INSTRUMENT"){/* DO NOTHING!!! MWAHAHA */}
*/
	if (type_function_map.find(type) != type_function_map.end())
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

    // Non Static Function Access
    type_function_map["snippet"] = new ClassFunctionAccessor<snippet_library>(&SNIPPET, &snippet_library::load_snippet);
    type_function_map["item_group"] = new ClassFunctionAccessor<Item_factory>(item_controller, &Item_factory::load_item_group);

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


