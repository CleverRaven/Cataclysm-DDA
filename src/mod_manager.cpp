#include "mod_manager.h"
#include "filesystem.h"
#include "debug.h"
#include "output.h"
#include "worldfactory.h"
#include "cata_utility.h"
#include "path_info.h"
#include "translations.h"

#include <math.h>
#include <queue>
#include <iostream>
#include <fstream>
#include <unordered_set>

#include "json.h"
#include "generic_factory.h"

#define MOD_SEARCH_FILE "modinfo.json"

/** Second field is optional replacement mod */
static std::map<std::string, std::string> mod_replacements;

// These accessors are to delay the initialization of the strings in the respective containers until after gettext is initialized.
const std::vector<std::pair<std::string, std::string> > &get_mod_list_categories() {
    static const std::vector<std::pair<std::string, std::string> > mod_list_categories = {
        {"content", _("CORE CONTENT PACKS")},
        {"items", _("ITEM ADDITION MODS")},
        {"creatures", _("CREATURE MODS")},
        {"misc_additions", _("MISC ADDITIONS")},
        {"buildings", _("BUILDINGS MODS")},
        {"vehicles", _("VEHICLE MODS")},
        {"rebalance", _("REBALANCING MODS")},
        {"magical", _("MAGICAL MODS")},
        {"item_exclude", _("ITEM EXCLUSION MODS")},
        {"monster_exclude", _("MONSTER EXCLUSION MODS")},
        {"", _("NO CATEGORY")}
    };

    return mod_list_categories;
}

const std::vector<std::pair<std::string, std::string> > &get_mod_list_tabs() {
    static const std::vector<std::pair<std::string, std::string> > mod_list_tabs = {
        {"tab_default", _("Default")},
        {"tab_blacklist", _("Blacklist")},
        {"tab_balance", _("Balance")}
    };

    return mod_list_tabs;
}

const std::map<std::string, std::string> &get_mod_list_cat_tab() {
    static const std::map<std::string, std::string> mod_list_cat_tab = {
        {"item_exclude", "tab_blacklist"},
        {"monster_exclude", "tab_blacklist"},
        {"rebalance", "tab_balance"}
    };

    return mod_list_cat_tab;
}

static void load_replacement_mods( const std::string path )
{
    read_from_file_optional_json( path, [&]( JsonIn &jsin ) {
        jsin.start_array();
        while (!jsin.end_array()) {
            auto arr = jsin.get_array();
            mod_replacements.emplace( arr.get_string( 0 ), arr.size() > 1 ? arr.get_string( 1 ) : "" );
        }
    } );
}

bool MOD_INFORMATION::need_lua() const
{
    return file_exist( path + "/main.lua" ) || file_exist( path + "/preload.lua" );
}

mod_manager::mod_manager()
{
    // Insure mod_replacements is initialized.
    if( mod_replacements.empty() && file_exist(FILENAMES["mods-replacements"]) ) {
        load_replacement_mods(FILENAMES["mods-replacements"]);
    }
}

mod_manager::~mod_manager() = default;

dependency_tree &mod_manager::get_tree()
{
    return tree;
}

void mod_manager::clear()
{
    tree.clear();
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
        const auto &deps = elem.second->dependencies;
        mod_dependency_map[elem.second->ident] = std::vector<std::string>( deps.begin(), deps.end() );
    }
    tree.init(mod_dependency_map);
}

void mod_manager::remove_mod(const std::string &ident)
{
    t_mod_map::iterator a = mod_map.find(ident);
    if (a != mod_map.end()) {
        mod_map.erase(a);
    }
}

void mod_manager::remove_invalid_mods( std::vector<std::string> &m ) const
{
    m.erase( std::remove_if( m.begin(), m.end(), [this]( const std::string &mod ) {
        return !has_mod( mod );
    } ), m.end() );
}

bool mod_manager::set_default_mods(const std::string &ident)
{
    if (!has_mod(ident)) {
        return false;
    }
    MOD_INFORMATION &mod = *mod_map[ident];
    auto deps = std::vector<std::string>( mod.dependencies.begin(), mod.dependencies.end() );
    remove_invalid_mods( deps );
    default_mods = deps;
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

void mod_manager::load_modfile( JsonObject &jo, const std::string &path )
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

    std::string m_name = jo.get_string("name", "");
    if (m_name.empty()) {
        // "No name" gets confusing if many mods have no name
        //~ name of a mod that has no name entry, (%s is the mods identifier)
        m_name = string_format(_("No name (%s)"), m_ident.c_str());
    } else {
        m_name = _(m_name.c_str());
    }

    std::string m_cat = jo.get_string("category", "");
    std::pair<int, std::string> p_cat = {-1, ""};
    bool bCatFound = false;

    do {
        for( size_t i = 0; i < get_mod_list_categories().size(); ++i ) {
            if( get_mod_list_categories()[i].first == m_cat ) {
                p_cat = { int(i), get_mod_list_categories()[i].second };
                bCatFound = true;
                break;
            }
        }

        if( !bCatFound && m_cat != "" ) {
            m_cat = "";
        } else {
            break;
        }
    } while( !bCatFound );

    std::unique_ptr<MOD_INFORMATION> modfile( new MOD_INFORMATION );
    modfile->ident = m_ident;
    modfile->name = m_name;
    modfile->category = p_cat;

    if( assign( jo, "path", modfile->path ) ) {
        modfile->path = path + "/" + modfile->path;
    } else {
        modfile->path = path;
    }
    if( assign( jo, "legacy", modfile->legacy ) ) {
        modfile->legacy = path + "/" + modfile->legacy;
    }

    assign( jo, "authors", modfile->authors );
    assign( jo, "maintainers", modfile->maintainers );
    assign( jo, "description", modfile->description );
    assign( jo, "dependencies", modfile->dependencies );
    assign( jo, "core", modfile->core );
    assign( jo, "obsolete", modfile->obsolete );

    if( modfile->dependencies.count( modfile->ident ) ) {
        jo.throw_error( "mod specifies self as a dependency", "dependencies" );
    }

    mod_map[modfile->ident] = std::move( modfile );
}

bool mod_manager::set_default_mods(const t_mod_list &mods)
{
    default_mods = mods;
    return write_to_file( FILENAMES["mods-user-default"], [&]( std::ostream &fout ) {
        JsonOut json( fout, true ); // pretty-print
        json.start_object();
        json.member("type", "MOD_INFO");
        json.member("ident", "user:default");
        json.member("dependencies");
        json.write(mods);
        json.end_object();
    }, _( "list of default mods" ) );
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
        MOD_INFORMATION &mod = *mod_map[mods_to_copy[i]];
        size_t start_index = mod.path.size();

        // now to get all of the json files inside of the mod and get them ready to copy
        auto input_files = get_files_from_path(".json", mod.path, true, true);
        auto input_dirs  = get_directories_with(search_extensions, mod.path, true);

        if (input_files.empty() && mod.path.find(MOD_SEARCH_FILE) != std::string::npos) {
            // Self contained mod, all data is inside the modinfo.json file
            input_files.push_back(mod.path);
            start_index = mod.path.find_last_of("/\\");
            if (start_index == std::string::npos) {
                start_index = 0;
            }
        }

        if (input_files.empty()) {
            continue;
        }

        // create needed directories
        std::ostringstream cur_mod_dir;
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
    const std::string main_path = info_file_path.substr(0, info_file_path.find_last_of("/\\"));
    read_from_file_optional_json( info_file_path, [&]( JsonIn &jsin ) {
        if( jsin.test_object() ) {
            // find type and dispatch single object
            JsonObject jo = jsin.get_object();
            load_modfile(jo, main_path);
            jo.finish();
        } else if( jsin.test_array() ) {
            jsin.start_array();
            // find type and dispatch each object until array close
            while (!jsin.end_array()) {
                JsonObject jo = jsin.get_object();
                load_modfile(jo, main_path);
                jo.finish();
            }
        } else {
            // not an object or an array?
            jsin.error( "expected array or object" );
        }
    } );
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
        remove_file(path);
        return;
    }
    write_to_file( path, [&]( std::ostream &fout ) {
        JsonOut json( fout, true ); // pretty-print
        json.write(world->active_mod_order);
    }, _( "list of mods" ) );
}

void mod_manager::load_mods_list(WORLDPTR world) const
{
    if (world == NULL) {
        return;
    }
    std::vector<std::string> &amo = world->active_mod_order;
    amo.clear();
    bool obsolete_mod_found = false;
    read_from_file_optional_json( get_mods_list_file( world ), [&]( JsonIn &jsin ) {
        JsonArray ja = jsin.get_array();
        while (ja.has_more()) {
            const std::string mod = ja.next_string();
            if( mod.empty() || std::find(amo.begin(), amo.end(), mod) != amo.end() ) {
                continue;
            }
            if( mod_replacements.count( mod ) ) {
                amo.push_back( mod_replacements[ mod ] );
                obsolete_mod_found = true;
            } else {
                amo.push_back(mod);
            }
        }
    } );
    if( obsolete_mod_found ) {
        // If we found an obsolete mod, overwrite the mod list without the obsolete one.
        save_mods_list(world);
    }
}

const mod_manager::t_mod_list &mod_manager::get_default_mods() const
{
    return default_mods;
}
