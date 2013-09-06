#include "init.h"

#include "json.h"

#include "material.h"
#include "bionics.h"

#include "debug.h"

#include <string>
#include <vector>
#include <fstream>
#include <sstream> // for throwing errors

/* Currently just for loading JSON data from files in data/raw */

// TODO: make this actually load files from the named directory
std::vector<std::string> listfiles(std::string const &dirname)
{
    std::vector<std::string> ret;
    ret.push_back("data/raw/materials.json");
    ret.push_back("data/raw/bionics.json");
    return ret;
}

bool load_object_from_json(std::string const &type, Jsin &jsin) throw (std::string)
{
    if (!jsin.good()) {
        throw "bad stream. aborting object load.";
    }
    if (type == "material") {
        return material_type::load_material(jsin);
    } else if (type == "bionic") {
        return load_bionic(jsin);
    } else {
        // unknown type, skip it
        jsin.skip_object();
        return false;
    }
    return false;
}

void load_json_dir(std::string const &dirname)
{
    // get a list of all files in the directory
    std::vector<std::string> dir = listfiles(dirname);
    // iterate over each file
    std::vector<std::string>::iterator it;
    for (it = dir.begin(); it != dir.end(); it++) {
        // open the file as a stream
        std::ifstream infile(it->c_str());
        // parse it
        dout(D_INFO) << "Loading JSON from file: " + *(it);
        Jsin jsin(&infile);
        load_all_from_json(jsin);
    }
}

void load_all_from_json(Jsin &jsin) throw (std::string)
{
    char ch;
    std::string type = "";
    jsin.eat_whitespace();
    // examine first non-whitespace char
    ch = jsin.peek();
    if (ch == '{') {
        // find type and dispatch single object
        jsin.save_pos();
        type = jsin.fetch_string("type");
        jsin.load_pos();
        load_object_from_json(type, jsin);
    } else if (ch == '[') {
        jsin.start_array();
        // find type and dispatch each object until array close
        while (!jsin.end_array()) {
            jsin.eat_whitespace();
            ch = jsin.peek();
            if (ch != '{') {
                std::stringstream err;
                err << "expected array of objects but found '";
                err << ch << "', not '{'";
                throw err.str();
            }
            jsin.save_pos();
            type = jsin.fetch_string("type");
            jsin.load_pos();
            load_object_from_json(type, jsin);
        }
    } else {
        // not an object or an array?
        std::stringstream err;
        err << "expected object or array, but found '" << ch << "'";
        throw err.str();
    }
}


