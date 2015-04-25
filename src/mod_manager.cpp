#include "mod_manager.h"
#include "filesystem.h"
#include "debug.h"
#include "output.h"
#include "worldfactory.h"
#include "path_info.h"

#include <math.h>
#include <queue>
#include <iostream>
#include <fstream>
#include <unordered_set>

#include "json.h"
#include <fstream>

#define MOD_SEARCH_FILE "modinfo.json"

static std::unordered_set<std::string> obsolete_mod_list;

static void load_obsolete_mods( const std::string path )
{
    // info_file_path is the fully qualified path to the information file for this mod
    std::ifstream infile( path.c_str(), std::ifstream::in | std::ifstream::binary );
    if( !infile ) {
        // fail silently?
        return;
    }
    try {
        JsonIn jsin( infile );
        jsin.eat_whitespace();
        char ch = jsin.peek();
        if( ch == '[' ) {
            jsin.start_array();
            // find type and dispatch each object until array close
            while (!jsin.end_array()) {
                obsolete_mod_list.insert( jsin.get_string() );
            }
        } else {
            // not an object or an array?
            std::stringstream err;
            err << jsin.line_number() << ": ";
            err << "expected array, but found '" << ch << "'";
            throw err.str();
        }
    } catch(std::string e) {
        debugmsg("%s", e.c_str());
    }
}

mod_manager::mod_manager()
{
    // Insure obsolete_mod_list is initialized.
    if( obsolete_mod_list.empty() && file_exist(FILENAMES["obsolete-mods"]) ) {
        load_obsolete_mods(FILENAMES["obsolete-mods"]);
    }
}

mod_manager::~mod_manager()
{
    clear();
}

dependency_tree &mod_manager::get_tree()
{
    return tree;
}

void mod_manager::clear()
{
    tree.clear();
    for( auto &elem : mod_map ) {
        delete elem.second;
    }
    mod_map.clear();
    default_mods.clear();
}

void mod_manager::refresh_mod_list()
{
    clear();

    std::map<std::string, std::vector<std::string> > mod_dependency_map;
    load_mods_from(FILENAMES["moddir"]);
    if (set_default_mods("user:default")) {
    } else if(set_default_mods("dev:default")) {
    }
    // remove these mods from the list, so they do not appear to the user
    remove_mod("user:default");
    remove_mod("dev:default");
    for( auto &elem : mod_map ) {
        mod_dependency_map[elem.second->ident] = elem.second->dependencies;
    }
    tree.init(mod_dependency_map);
}

void mod_manager::remove_mod(const std::string &ident)
{
    t_mod_map::iterator a = mod_map.find(ident);
    if (a != mod_map.end()) {
        delete a->second;
        mod_map.erase(a);
    }
}

bool mod_manager::set_default_mods(const std::string &ident)
{
    if (!has_mod(ident)) {
        return false;
    }
    MOD_INFORMATION *mod = mod_map[ident];
    default_mods = mod->dependencies;
    return true;
}

bool mod_manager::has_mod(const std::string &ident) const
{
    return mod_map.count(ident) > 0;
}

void mod_manager::load_mods_from(std::string path)
{
    for( auto &mod_file : get_files_from_path(MOD_SEARCH_FILE, path, true) ) {
        load_mod_info( mod_file );
    }
    if (file_exist(FILENAMES["mods-dev-default"])) {
        load_mod_info(FILENAMES["mods-dev-default"]);
    }
    if (file_exist(FILENAMES["mods-user-default"])) {
        load_mod_info(FILENAMES["mods-user-default"]);
    }
}

void mod_manager::load_modfile(JsonObject &jo, const std::string &main_path)
{
    if (!jo.has_string("type") || jo.get_string("type") != "MOD_INFO") {
        // Ignore anything that is not a mod-info
        return;
    }
    std::string m_ident = jo.get_string("ident");
    if (has_mod(m_ident)) {
        // TODO: change this to make unique ident for the mod
        // (instead of discarding it?)
        debugmsg("there is already a mod with ident %s", m_ident.c_str());
        return;
    }

    std::string t_type = jo.get_string("mod-type", "SUPPLEMENTAL");
    std::vector<std::string> m_authors;
    if (jo.has_array("authors")) {
        m_authors = jo.get_string_array("authors");
    } else {
        if(jo.has_string("author")) {
            m_authors.push_back(jo.get_string("author"));
        }
    }
    std::string m_name = jo.get_string("name", "");
    if (m_name.empty()) {
        // "No name" gets confusing if many mods have no name
        //~ name of a mod that has no name entry, (%s is the mods identifier)
        m_name = string_format(_("No name (%s)"), m_ident.c_str());
    } else {
        m_name = _(m_name.c_str());
    }
    std::string m_desc = jo.get_string("description", "");
    if (m_desc.empty()) {
        m_desc = _("No description");
    } else {
        m_desc = _(m_desc.c_str());
    }
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
    modfile->authors = m_authors;
    modfile->name = m_name;
    modfile->description = m_desc;
    modfile->dependencies = m_dependencies;
    modfile->path = m_path;

    mod_map[modfile->ident] = modfile;
}

bool mod_manager::set_default_mods(const t_mod_list &mods)
{
    default_mods = mods;
    std::ofstream stream(FILENAMES["mods-user-default"].c_str(), std::ios::out | std::ios::binary);
    if(!stream) {
        popup(_("Can not open %s for writing"), FILENAMES["mods-user-default"].c_str());
        return false;
    }
    try {
        stream.exceptions(std::ios::failbit | std::ios::badbit);
        JsonOut json(stream, true); // pretty-print
        json.start_object();
        json.member("type", "MOD_INFO");
        json.member("ident", "user:default");
        json.member("dependencies");
        json.write(mods);
        json.end_object();
        return true;
    } catch(std::ios::failure &) {
        // this might happen and indicates an I/O-error
        popup(_("Failed to write default mods to %s"), FILENAMES["mods-user-default"].c_str());
    } catch(std::string e) {
        // this should not happen, it comes from json-serialization
        debugmsg("%s", e.c_str());
    }
    return false;
}

bool mod_manager::copy_mod_contents(const t_mod_list &mods_to_copy,
                                    const std::string &output_base_path)
{
    if (mods_to_copy.empty()) {
        // nothing to copy, so technically we succeeded already!
        return true;
    }
    std::vector<std::string> search_extensions;
    search_extensions.push_back(".json");

    DebugLog( D_INFO, DC_ALL ) << "Copying mod contents into directory: " << output_base_path;

    if (!assure_dir_exist(output_base_path)) {
        DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" << output_base_path <<
                                    "] for saving";
        return false;
    }

    std::ostringstream number_stream;
    for (size_t i = 0; i < mods_to_copy.size(); ++i) {
        number_stream.str(std::string());
        number_stream.width(5);
        number_stream.fill('0');
        number_stream << (i + 1);
        MOD_INFORMATION *mod = mod_map[mods_to_copy[i]];
        size_t start_index = mod->path.size();

        // now to get all of the json files inside of the mod and get them ready to copy
        auto input_files = get_files_from_path(".json", mod->path, true, true);
        auto input_dirs  = get_directories_with(search_extensions, mod->path, true);

        if (input_files.empty() && mod->path.find(MOD_SEARCH_FILE) != std::string::npos) {
            // Self contained mod, all data is inside the modinfo.json file
            input_files.push_back(mod->path);
            start_index = mod->path.find_last_of("/\\");
            if (start_index == std::string::npos) {
                start_index = 0;
            }
        }

        if (input_files.empty()) {
            continue;
        }

        // create needed directories
        std::stringstream cur_mod_dir;
        cur_mod_dir << output_base_path << "/mod_" << number_stream.str();

        std::queue<std::string> dir_to_make;
        dir_to_make.push(cur_mod_dir.str());
        for( auto &input_dir : input_dirs ) {
            dir_to_make.push( cur_mod_dir.str() + "/" + input_dir.substr( start_index ) );
        }

        while (!dir_to_make.empty()) {
            if (!assure_dir_exist(dir_to_make.front())) {
                DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" <<
                                            dir_to_make.front() << "] for saving";
            }

            dir_to_make.pop();
        }

        std::ofstream fout;
        // trim file paths from full length down to just /data forward
        for( auto &input_file : input_files ) {
            std::string output_path = input_file;
            output_path = cur_mod_dir.str() + output_path.substr(start_index);

            std::ifstream infile( input_file.c_str(), std::ifstream::in | std::ifstream::binary );
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
    if (!infile) {
        // fail silently?
        return;
    }
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

std::string mod_manager::get_mods_list_file(const WORLDPTR world)
{
    return world->world_path + "/mods.json";
}

void mod_manager::save_mods_list(WORLDPTR world) const
{
    if( world == NULL ) {
        return;
    }
    const std::string path = get_mods_list_file(world);
    if( world->active_mod_order.empty() ) {
        // If we were called from load_mods_list to prune the list,
        // and it's empty now, delete the file.
        if( file_exist(path) ) {
            remove_file(path);
        }
        return;
    }
    std::ofstream mods_list_file(path.c_str(), std::ios::out | std::ios::binary);
    if(!mods_list_file) {
        popup(_("Can not open %s for writing"), path.c_str());
    }
    try {
        mods_list_file.exceptions(std::ios::failbit | std::ios::badbit);
        JsonOut json(mods_list_file, true); // pretty-print
        json.write(world->active_mod_order);
    } catch(std::ios::failure &) {
        // this might happen and indicates an I/O-error
        popup(_("Failed to write to %s"), path.c_str());
    } catch (std::string e) {
        popup( _( "Failed to write list of mods to %s: %s" ), path.c_str(), e.c_str() );
    }
}

void mod_manager::load_mods_list(WORLDPTR world) const
{
    if (world == NULL) {
        return;
    }
    std::vector<std::string> &amo = world->active_mod_order;
    amo.clear();
    std::ifstream mods_list_file( get_mods_list_file(world).c_str(),
                                  std::ios::in | std::ios::binary );
    if (!mods_list_file) {
        return;
    }
    bool obsolete_mod_found = false;
    try {
        JsonIn jsin(mods_list_file);
        JsonArray ja = jsin.get_array();
        while (ja.has_more()) {
            const std::string mod = ja.next_string();
            if( mod.empty() || std::find(amo.begin(), amo.end(), mod) != amo.end() ) {
                continue;
            }
            if( obsolete_mod_list.count( mod ) ) {
                obsolete_mod_found = true;
                continue;
            }

            amo.push_back(mod);
        }
    } catch (std::string e) {
        DebugLog( D_ERROR, DC_ALL ) << "worldfactory: loading mods list failed: " << e;
    }
    if( obsolete_mod_found ) {
        // If we found an obsolete mod, overwrite the mod list without the obsolete one.
        save_mods_list(world);
    }
}

const mod_manager::t_mod_list &mod_manager::get_default_mods() const
{
    return default_mods;
}
