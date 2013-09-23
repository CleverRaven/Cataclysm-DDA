#include "init.h"

#include "json.h"

#include "material.h"
#include "bionics.h"
#include "profession.h"
#include "skill.h"
#include "mutation.h"
#include "text_snippets.h"
#include "item_factory.h"
#include "crafting.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors

/* Currently just for loading JSON data from files in data/raw */

// TODO: make this actually load files from the named directory
std::vector<std::string> listfiles(std::string const &dirname)
{
    std::vector<std::string> ret;
    ret.push_back("data/json/materials.json");
    ret.push_back("data/json/bionics.json");
    ret.push_back("data/json/professions.json");
    ret.push_back("data/json/skills.json");
    ret.push_back("data/json/dreams.json");
    ret.push_back("data/json/mutations.json");
    ret.push_back("data/json/snippets.json");
    ret.push_back("data/json/item_groups.json");
    ret.push_back("data/json/recipes.json");
    return ret;
}

void load_object(JsonObject &jo)
{
    std::string type = jo.get_string("type");
    if (type == "material") { material_type::load_material(jo);}
    else if (type == "bionic") { load_bionic(jo); }
    else if (type == "profession") { profession::load_profession(jo); }
    else if (type == "skill") { Skill::load_skill(jo); }
    else if (type == "dream") { load_dream(jo); }
    else if (type == "mutation") { load_mutation(jo); }
    else if (type == "snippet") { SNIPPET.load_snippet(jo); }
    else if (type == "item_group") { item_controller->load_item_group(jo); }
    else if (type == "recipe_category") { load_recipe_category(jo); }
    else if (type == "recipe") { load_recipe(jo); }
    else {
        std::stringstream err;
        err << jo.line_number() << ": ";
        err << "unrecognized JSON object, type: \"" << type << "\"";
        throw err.str();
    }
}

void init_data_structures()
{
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


