#include "mod_manager.h"
#include "file_finder.h"
#include "debug.h"
#include "output.h"

#include <math.h>
#include <queue>
#include <iostream>
#include <fstream>
// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

#define MOD_SEARCH_PATH "./data"
#define MOD_SEARCH_FILE "modinfo.json"

mod_manager::mod_manager()
{
}

mod_manager::~mod_manager()
{
    clear();
}

dependency_tree &mod_manager::get_tree() {
    return tree;
}

void mod_manager::clear()
{
    tree.clear();
    for(t_mod_map::iterator a = mod_map.begin(); a != mod_map.end(); ++a) {
        delete a->second;
    }
    mod_map.clear();
}

void mod_manager::show_ui()
{
    refresh_mod_list();

    mod_ui *UI = new mod_ui(this);
    UI->show_layering_ui();

    delete UI;
}

void mod_manager::refresh_mod_list()
{
    clear();

    std::map<std::string, std::vector<std::string> > mod_dependency_map;
    load_mods_from(MOD_SEARCH_PATH);
    for(t_mod_map::iterator a = mod_map.begin(); a != mod_map.end(); ++a) {
        mod_dependency_map[a->second->ident] = a->second->dependencies;
    }
    tree.init(mod_dependency_map);
}

bool mod_manager::has_mod(const std::string &ident) const
{
    return mod_map.count(ident) > 0;
}

void mod_manager::load_mods_from(std::string path)
{
    std::vector<std::string> mod_files = file_finder::get_files_from_path(MOD_SEARCH_FILE, path, true);
    for (int i = 0; i < mod_files.size(); ++i) {
        load_mod_info(mod_files[i]);
    }
}

void mod_manager::load_modfile(JsonObject &jo, const std::string &main_path)
{
    std::string m_ident = jo.get_string("ident");
    if (has_mod(m_ident)) {
        // TODO: change this to make uniqe ident for the mod
        // (instead of discarding it?)
        debugmsg("there is already a mod with ident %s", m_ident.c_str());
        return;
    }
    std::string t_type = jo.get_string("type", "SUPPLEMENTAL");
    std::string m_author = jo.get_string("author", "Unknown Author");
    std::string m_name = jo.get_string("name", "No Name");
    std::string m_desc = jo.get_string("description", "No Description");
    std::string m_path;
    if (jo.has_string("path")) {
        m_path = jo.get_string("path");
        if (m_path.empty()) {
            // If an empty path is given, use only the
            // folder of the modinfo.json
            m_path = main_path;
        } else {
            // prefix the folder of modinfo.json
            m_path = main_path + "/" + m_path;
        }
    } else {
        // Default if no path is given:
        // "<folder-of-modinfo.json>/data"
        m_path = main_path + "/data";
    }
    std::vector<std::string> m_dependencies;

    if (jo.has_member("dependencies") && jo.has_array("dependencies")) {
        JsonArray jarr = jo.get_array("dependencies");
        while(jarr.has_more()) {
            const std::string dep = jarr.next_string();
            if (dep == m_ident) {
                debugmsg("mod %s has itself as dependency", m_ident.c_str());
                continue;
            }
            if (std::find(m_dependencies.begin(), m_dependencies.end(), dep) != m_dependencies.end()) {
                // Some dependency listed twice, ignore it, what else can be done?
                continue;
            }
            m_dependencies.push_back(dep);
        }
    }

    mod_type m_type;
    if (t_type == "CORE") {
        m_type = MT_CORE;
    } else if (t_type == "SUPPLEMENTAL") {
        m_type = MT_SUPPLEMENTAL;
    } else {
        throw std::string("Invalid mod type: ") + t_type + " for mod " + m_ident;
    }

    MOD_INFORMATION *modfile = new MOD_INFORMATION;
    modfile->ident = m_ident;
    modfile->_type = m_type;
    modfile->author = m_author;
    modfile->name = m_name;
    modfile->description = m_desc;
    modfile->dependencies = m_dependencies;
    modfile->path = m_path;

    mod_map[modfile->ident] = modfile;
}

// TODO: move this declaration into a header, but which?
extern bool assure_dir_exist(const std::string &path);

bool mod_manager::copy_mod_contents(std::vector<std::string> mods_to_copy, std::string output_base_path)
{
    if (mods_to_copy.size() == 0){
        // nothing to copy, so technically we succeeded already!
        return true;
    }
    std::vector<std::string> search_extensions;
    search_extensions.push_back(".json");

    DebugLog() << "Copying mod contents into directory: "<<output_base_path<<"\n";

    if (!assure_dir_exist(output_base_path)) {
        DebugLog() << "Unable to create or open mod directory at [" << output_base_path << "] for saving\n";
        return false;
    }

    std::ostringstream number_stream;
    for (int i = 0; i < mods_to_copy.size(); ++i){
        number_stream.str(std::string());
        number_stream.width(5);
        number_stream.fill('0');
        number_stream << (i + 1);
        MOD_INFORMATION *mod = mod_map[mods_to_copy[i]];

        // now to get all of the json files inside of the mod and get them ready to copy
        std::vector<std::string> input_files = file_finder::get_files_from_path(".json", mod->path, true, true);
        std::vector<std::string> input_dirs  = file_finder::get_directories_with(search_extensions, mod->path, true);

        if (input_files.size() == 0){
            continue;
        }

        // create needed directories
        std::stringstream cur_mod_dir;
        cur_mod_dir << output_base_path << "/mod_" << number_stream.str();

        std::queue<std::string> dir_to_make;
        dir_to_make.push(cur_mod_dir.str());
        size_t start_index = mod->path.size();
        for (int j = 0; j < input_dirs.size(); ++j){
            dir_to_make.push(cur_mod_dir.str() + "/" + input_dirs[j].substr(start_index));
        }

        while (!dir_to_make.empty()){
            if (!assure_dir_exist(dir_to_make.front())) {
                DebugLog() << "Unable to create or open mod directory at [" << dir_to_make.front() << "] for saving\n";
            }

            dir_to_make.pop();
        }

        std::ofstream fout;
        // trim file paths from full length down to just /data forward
        for (int j = 0; j < input_files.size(); ++j){
            std::string output_path = input_files[j];
            output_path = cur_mod_dir.str() + output_path.substr(start_index);

            std::ifstream infile(input_files[j].c_str(), std::ifstream::in | std::ifstream::binary);
            // and stuff it into ram
            std::istringstream iss(
                std::string(
                    (std::istreambuf_iterator<char>(infile)),
                    std::istreambuf_iterator<char>()
                )
            );
            infile.close();

            fout.open(output_path.c_str());
            fout << iss.str();
            fout.close();
        }
    }
    return true;
}

void mod_manager::load_mod_info(std::string info_file_path)
{
    // info_file_path is the fully qualified path to the information file for this mod
    std::ifstream infile(info_file_path.c_str(), std::ifstream::in | std::ifstream::binary);
    std::istringstream iss(
        std::string(
            (std::istreambuf_iterator<char>(infile)),
            std::istreambuf_iterator<char>()
        )
    );
    infile.close();
    const std::string main_path = info_file_path.substr(0, info_file_path.find_last_of("/\\"));
    try {
        JsonIn jsin(iss);
        jsin.eat_whitespace();
        char ch = jsin.peek();
        if (ch == '{') {
            // find type and dispatch single object
            JsonObject jo = jsin.get_object();
            load_modfile(jo, main_path);
            jo.finish();
        } else if (ch == '[') {
            jsin.start_array();
            // find type and dispatch each object until array close
            while (!jsin.end_array()) {
                jsin.eat_whitespace();
                JsonObject jo = jsin.get_object();
                load_modfile(jo, main_path);
                jo.finish();
            }
        } else {
            // not an object or an array?
            std::stringstream err;
            err << jsin.line_number() << ": ";
            err << "expected object or array, but found '" << ch << "'";
            throw err.str();
        }
    } catch(std::string e) {
        debugmsg("%s", e.c_str());
    }
}
