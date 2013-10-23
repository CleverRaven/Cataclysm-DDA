#include "mod_manager.h"
#include "file_finder.h"
#include "debug.h"

#define MOD_SEARCH_PATH "."
#define MOD_SEARCH_FILE "modinfo.json"

MOD_INFORMATION::~MOD_INFORMATION()
{
    DebugLog() << "~MOD_INFORMATION\n";
}

mod_manager::mod_manager()
{
    tree = new dependency_tree();
    clear();
}

mod_manager::~mod_manager()
{
    //dtor
    clear();
    if (tree){
        delete tree;
    }
}

void mod_manager::clear()
{
    mod_map.clear();
    tree->clear();
    if (!mods.empty()){
        for (int i = 0; i < mods.size(); ++i){
            delete mods[i];
        }
        mods.clear();
    }
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
    if (!load_mods_from(MOD_SEARCH_PATH)){
        // no mods loaded! oh noes!
    }
    for (int i = 0; i < mods.size(); ++i){
        mod_dependency_map[mods[i]->ident] = mods[i]->dependencies;
        mod_map[mods[i]->ident] = i;
    }
    tree->init(mod_dependency_map);
}

bool mod_manager::load_mods_from(std::string path)
{
    std::vector<std::string> mod_files = file_finder::get_files_from_path(MOD_SEARCH_FILE, path, true);
    if (mod_files.empty()){
        return false;
    }
    unsigned num_loaded = 0;
    const std::string mod_file_input = std::string("/") + MOD_SEARCH_FILE;
    for (int i = 0; i < mod_files.size(); ++i){
        int path_index = mod_files[i].find(mod_file_input);

        MOD_INFORMATION *modinfo = new MOD_INFORMATION;

        if (!load_mod_info(modinfo, mod_files[i])){
            delete modinfo;
            continue;
        }
        modinfo->path = mod_files[i].substr(0, path_index);
        ++num_loaded;
    }
    return num_loaded; // false if 0 loaded, true otherwise
}

MOD_INFORMATION *mod_manager::load_modfile(JsonObject &jo)
{
    if (!jo.has_member("ident")){
        DebugLog() << "mod_manager:: loading modfile failed, no ident found\n";
        return NULL;
    }

    MOD_INFORMATION *modfile = new MOD_INFORMATION;
    std::string m_ident = jo.get_string("ident");
    std::string t_type = jo.get_string("type", "UNKNOWN");
    std::string m_author = jo.get_string("author", "Unknown Author");
    std::string m_name = jo.get_string("name", "No Name");
    std::string m_desc = jo.get_string("description", "No Description");
    std::vector<std::string> m_dependencies;

    if (jo.has_member("dependencies") && jo.has_array("dependencies")){
        JsonArray jarr = jo.get_array("dependencies");

        for (int i = 0; i < jarr.size(); ++i){
            if (jarr.has_string(i)){
                m_dependencies.push_back(jarr.get_string(i));
            }
        }
    }

    mod_type m_type;
    if (t_type == "CORE") { m_type = MT_CORE;}
    else if (t_type == "SUPPLEMENTAL") {m_type = MT_SUPPLEMENTAL;}
    else { m_type = MT_UNKNOWN;}

    modfile->ident = m_ident;
    modfile->_type = m_type;
    modfile->author = m_author;
    modfile->name = m_name;
    modfile->description = m_desc;
    modfile->dependencies = m_dependencies;

    mods.push_back(modfile);

    return modfile;
}

bool mod_manager::load_mod_info(MOD_INFORMATION *mod, std::string info_file_path)
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
    try{
        JsonIn jsin(&iss);
        char ch;
        // Manually load the modinfo object because the json handler cannot be used to deal with this.
        jsin.eat_whitespace();
        ch = jsin.peek();
        if( ch == '{' ) {
            JsonObject jo = jsin.get_object();
            mod = load_modfile(jo);
            jo.finish();
        } else {
            // not an object?
            std::stringstream err;
            err << jsin.line_number() << ": ";
            err << "expected object or array, but found '" << ch << "'";
            throw err.str();
        }
    }
    catch(std::string e){
        return false;
    }
    return true;
}
