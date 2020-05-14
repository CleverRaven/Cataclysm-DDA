#include "mod_manager.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <queue>

#include "assign.h"
#include "cata_utility.h"
#include "debug.h"
#include "dependency_tree.h"
#include "filesystem.h"
#include "json.h"
#include "path_info.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "worldfactory.h"

static const std::string MOD_SEARCH_FILE( "modinfo.json" );

template<>
const MOD_INFORMATION &string_id<MOD_INFORMATION>::obj() const
{
    const auto &map = world_generator->get_mod_manager().mod_map;
    const auto iter = map.find( *this );
    if( iter == map.end() ) {
        debugmsg( "Invalid mod %s requested", str() );
        static const MOD_INFORMATION dummy{};
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<MOD_INFORMATION>::is_valid() const
{
    return world_generator->get_mod_manager().mod_map.count( *this ) > 0;
}

std::string MOD_INFORMATION::name() const
{
    if( name_.empty() ) {
        // "No name" gets confusing if many mods have no name
        //~ name of a mod that has no name entry, (%s is the mods identifier)
        return string_format( _( "No name (%s)" ), ident.c_str() );
    } else {
        return _( name_ );
    }
}

// These accessors are to delay the initialization of the strings in the respective containers until after gettext is initialized.
const std::vector<std::pair<std::string, std::string> > &get_mod_list_categories()
{
    static const std::vector<std::pair<std::string, std::string> > mod_list_categories = {
        {"content", translate_marker( "CORE CONTENT PACKS" )},
        {"items", translate_marker( "ITEM ADDITION MODS" )},
        {"creatures", translate_marker( "CREATURE MODS" )},
        {"misc_additions", translate_marker( "MISC ADDITIONS" )},
        {"buildings", translate_marker( "BUILDINGS MODS" )},
        {"vehicles", translate_marker( "VEHICLE MODS" )},
        {"rebalance", translate_marker( "REBALANCING MODS" )},
        {"magical", translate_marker( "MAGICAL MODS" )},
        {"item_exclude", translate_marker( "ITEM EXCLUSION MODS" )},
        {"monster_exclude", translate_marker( "MONSTER EXCLUSION MODS" )},
        {"graphical", translate_marker( "GRAPHICAL MODS" )},
        {"", translate_marker( "NO CATEGORY" )}
    };

    return mod_list_categories;
}

const std::vector<std::pair<std::string, std::string> > &get_mod_list_tabs()
{
    static const std::vector<std::pair<std::string, std::string> > mod_list_tabs = {
        {"tab_default", translate_marker( "Default" )},
        {"tab_blacklist", translate_marker( "Blacklist" )},
        {"tab_balance", translate_marker( "Balance" )}
    };

    return mod_list_tabs;
}

const std::map<std::string, std::string> &get_mod_list_cat_tab()
{
    static const std::map<std::string, std::string> mod_list_cat_tab = {
        {"item_exclude", "tab_blacklist"},
        {"monster_exclude", "tab_blacklist"},
        {"rebalance", "tab_balance"}
    };

    return mod_list_cat_tab;
}

void mod_manager::load_replacement_mods( const std::string &path )
{
    read_from_file_optional_json( path, [&]( JsonIn & jsin ) {
        jsin.start_array();
        while( !jsin.end_array() ) {
            auto arr = jsin.get_array();
            mod_replacements.emplace( mod_id( arr.get_string( 0 ) ),
                                      mod_id( arr.size() > 1 ? arr.get_string( 1 ) : "" ) );
        }
    } );
}

mod_manager::mod_manager()
{
    load_replacement_mods( PATH_INFO::mods_replacements() );
    refresh_mod_list();
    set_usable_mods();
}

mod_manager::~mod_manager() = default;

std::vector<mod_id> mod_manager::all_mods() const
{
    std::vector<mod_id> result;
    std::transform( mod_map.begin(), mod_map.end(),
    std::back_inserter( result ), []( const decltype( mod_manager::mod_map )::value_type & pair ) {
        return pair.first;
    } );
    return result;
}

dependency_tree &mod_manager::get_tree()
{
    return *tree;
}

void mod_manager::clear()
{
    tree->clear();
    mod_map.clear();
    default_mods.clear();
}

void mod_manager::refresh_mod_list()
{
    clear();

    std::map<mod_id, std::vector<mod_id>> mod_dependency_map;
    load_mods_from( PATH_INFO::moddir() );
    load_mods_from( PATH_INFO::user_moddir() );

    if( file_exist( PATH_INFO::mods_dev_default() ) ) {
        load_mod_info( PATH_INFO::mods_dev_default() );
    }
    if( file_exist( PATH_INFO::mods_user_default() ) ) {
        load_mod_info( PATH_INFO::mods_user_default() );
    }

    if( !set_default_mods( mod_id( "user:default" ) ) ) {
        set_default_mods( mod_id( "dev:default" ) );
    }
    // remove these mods from the list, so they do not appear to the user
    remove_mod( mod_id( "user:default" ) );
    remove_mod( mod_id( "dev:default" ) );
    for( auto &elem : mod_map ) {
        const auto &deps = elem.second.dependencies;
        mod_dependency_map[elem.second.ident] = std::vector<mod_id>( deps.begin(), deps.end() );
    }
    tree->init( mod_dependency_map );
}

void mod_manager::remove_mod( const mod_id &ident )
{
    const auto a = mod_map.find( ident );
    if( a != mod_map.end() ) {
        mod_map.erase( a );
    }
}

void mod_manager::remove_invalid_mods( std::vector<mod_id> &mods ) const
{
    mods.erase( std::remove_if( mods.begin(), mods.end(), [this]( const mod_id & mod ) {
        return mod_map.count( mod ) == 0;
    } ), mods.end() );
}

bool mod_manager::set_default_mods( const mod_id &ident )
{
    // can't use string_id::is_valid as the global mod_manger instance does not exist yet
    const auto iter = mod_map.find( ident );
    if( iter == mod_map.end() ) {
        return false;
    }
    const MOD_INFORMATION &mod = iter->second;
    auto deps = std::vector<mod_id>( mod.dependencies.begin(), mod.dependencies.end() );
    remove_invalid_mods( deps );
    default_mods = deps;
    return true;
}

void mod_manager::load_mods_from( const std::string &path )
{
    for( auto &mod_file : get_files_from_path( MOD_SEARCH_FILE, path, true ) ) {
        load_mod_info( mod_file );
    }
}

void mod_manager::load_modfile( const JsonObject &jo, const std::string &path )
{
    if( !jo.has_string( "type" ) || jo.get_string( "type" ) != "MOD_INFO" ) {
        // Ignore anything that is not a mod-info
        jo.allow_omitted_members();
        return;
    }

    const mod_id m_ident( jo.get_string( "ident" ) );
    // can't use string_id::is_valid as the global mod_manger instance does not exist yet
    if( mod_map.count( m_ident ) > 0 ) {
        // TODO: change this to make unique ident for the mod
        // (instead of discarding it?)
        debugmsg( "there is already a mod with ident %s", m_ident.c_str() );
        return;
    }

    const std::string m_name = jo.get_string( "name", "" );

    std::string m_cat = jo.get_string( "category", "" );
    std::pair<int, std::string> p_cat = {-1, ""};
    bool bCatFound = false;

    do {
        for( size_t i = 0; i < get_mod_list_categories().size(); ++i ) {
            if( get_mod_list_categories()[i].first == m_cat ) {
                p_cat = { static_cast<int>( i ), get_mod_list_categories()[i].second };
                bCatFound = true;
                break;
            }
        }

        if( !bCatFound && !m_cat.empty() ) {
            m_cat.clear();
        } else {
            break;
        }
    } while( !bCatFound );

    MOD_INFORMATION modfile;
    modfile.ident = m_ident;
    modfile.name_ = m_name;
    modfile.category = p_cat;

    if( assign( jo, "path", modfile.path ) ) {
        modfile.path = path + "/" + modfile.path;
    } else {
        modfile.path = path;
    }
    if( assign( jo, "legacy", modfile.legacy ) ) {
        modfile.legacy = path + "/" + modfile.legacy;
    }

    assign( jo, "authors", modfile.authors );
    assign( jo, "maintainers", modfile.maintainers );
    assign( jo, "description", modfile.description );
    assign( jo, "version", modfile.version );
    assign( jo, "dependencies", modfile.dependencies );
    assign( jo, "core", modfile.core );
    assign( jo, "obsolete", modfile.obsolete );

    if( std::find( modfile.dependencies.begin(), modfile.dependencies.end(),
                   modfile.ident ) != modfile.dependencies.end() ) {
        jo.throw_error( "mod specifies self as a dependency", "dependencies" );
    }

    mod_map[modfile.ident] = std::move( modfile );
}

bool mod_manager::set_default_mods( const t_mod_list &mods )
{
    default_mods = mods;
    return write_to_file( PATH_INFO::mods_user_default(), [&]( std::ostream & fout ) {
        JsonOut json( fout, true ); // pretty-print
        json.start_object();
        json.member( "type", "MOD_INFO" );
        json.member( "ident", "user:default" );
        json.member( "dependencies" );
        json.write( mods );
        json.end_object();
    }, _( "list of default mods" ) );
}

bool mod_manager::copy_mod_contents( const t_mod_list &mods_to_copy,
                                     const std::string &output_base_path )
{
    if( mods_to_copy.empty() ) {
        // nothing to copy, so technically we succeeded already!
        return true;
    }
    std::vector<std::string> search_extensions;
    search_extensions.push_back( ".json" );

    DebugLog( D_INFO, DC_ALL ) << "Copying mod contents into directory: " << output_base_path;

    if( !assure_dir_exist( output_base_path ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" << output_base_path <<
                                    "] for saving";
        return false;
    }

    for( size_t i = 0; i < mods_to_copy.size(); ++i ) {
        const MOD_INFORMATION &mod = *mods_to_copy[i];
        size_t start_index = mod.path.size();

        // now to get all of the json files inside of the mod and get them ready to copy
        auto input_files = get_files_from_path( ".json", mod.path, true, true );
        auto input_dirs  = get_directories_with( search_extensions, mod.path, true );

        if( input_files.empty() && mod.path.find( MOD_SEARCH_FILE ) != std::string::npos ) {
            // Self contained mod, all data is inside the modinfo.json file
            input_files.push_back( mod.path );
            start_index = mod.path.find_last_of( "/\\" );
            if( start_index == std::string::npos ) {
                start_index = 0;
            }
        }

        if( input_files.empty() ) {
            continue;
        }

        // create needed directories
        const std::string cur_mod_dir = string_format( "%s/mod_%05d", output_base_path, i + 1 );

        std::queue<std::string> dir_to_make;
        dir_to_make.push( cur_mod_dir );
        for( auto &input_dir : input_dirs ) {
            dir_to_make.push( cur_mod_dir + "/" + input_dir.substr( start_index ) );
        }

        while( !dir_to_make.empty() ) {
            if( !assure_dir_exist( dir_to_make.front() ) ) {
                DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" <<
                                            dir_to_make.front() << "] for saving";
            }

            dir_to_make.pop();
        }

        // trim file paths from full length down to just /data forward
        for( auto &input_file : input_files ) {
            std::string output_path = input_file;
            output_path = cur_mod_dir + output_path.substr( start_index );
            copy_file( input_file, output_path );
        }
    }
    return true;
}

void mod_manager::load_mod_info( const std::string &info_file_path )
{
    const std::string main_path = info_file_path.substr( 0, info_file_path.find_last_of( "/\\" ) );
    read_from_file_optional_json( info_file_path, [&]( JsonIn & jsin ) {
        if( jsin.test_object() ) {
            // find type and dispatch single object
            JsonObject jo = jsin.get_object();
            load_modfile( jo, main_path );
            jo.finish();
        } else if( jsin.test_array() ) {
            jsin.start_array();
            // find type and dispatch each object until array close
            while( !jsin.end_array() ) {
                JsonObject jo = jsin.get_object();
                load_modfile( jo, main_path );
                jo.finish();
            }
        } else {
            // not an object or an array?
            jsin.error( "expected array or object" );
        }
    } );
}

std::string mod_manager::get_mods_list_file( const WORLDPTR world )
{
    return world->folder_path() + "/mods.json";
}

void mod_manager::save_mods_list( WORLDPTR world ) const
{
    if( world == nullptr ) {
        return;
    }
    const std::string path = get_mods_list_file( world );
    if( world->active_mod_order.empty() ) {
        // If we were called from load_mods_list to prune the list,
        // and it's empty now, delete the file.
        remove_file( path );
        return;
    }
    write_to_file( path, [&]( std::ostream & fout ) {
        JsonOut json( fout, true ); // pretty-print
        json.write( world->active_mod_order );
    }, _( "list of mods" ) );
}

void mod_manager::load_mods_list( WORLDPTR world ) const
{
    if( world == nullptr ) {
        return;
    }
    std::vector<mod_id> &amo = world->active_mod_order;
    amo.clear();
    bool obsolete_mod_found = false;
    read_from_file_optional_json( get_mods_list_file( world ), [&]( JsonIn & jsin ) {
        for( const std::string line : jsin.get_array() ) {
            const mod_id mod( line );
            if( std::find( amo.begin(), amo.end(), mod ) != amo.end() ) {
                continue;
            }
            const auto iter = mod_replacements.find( mod );
            if( iter != mod_replacements.end() ) {
                if( !iter->second.is_empty() ) {
                    amo.push_back( iter->second );
                }
                obsolete_mod_found = true;
                continue;
            }
            amo.push_back( mod );
        }
    } );
    if( obsolete_mod_found ) {
        // If we found an obsolete mod, overwrite the mod list without the obsolete one.
        save_mods_list( world );
    }
}

const mod_manager::t_mod_list &mod_manager::get_default_mods() const
{
    return default_mods;
}

inline bool compare_mod_by_name_and_category( const MOD_INFORMATION *const a,
        const MOD_INFORMATION *const b )
{
    return localized_compare( std::make_pair( a->category, a->name() ),
                              std::make_pair( b->category, b->name() ) );
}

void mod_manager::set_usable_mods()
{
    std::vector<mod_id> available_cores, available_supplementals;
    std::vector<mod_id> ordered_mods;

    std::vector<const MOD_INFORMATION *> mods;
    for( const auto &pair : mod_map ) {
        if( !pair.second.obsolete ) {
            mods.push_back( &pair.second );
        }
    }
    std::sort( mods.begin(), mods.end(), &compare_mod_by_name_and_category );

    for( const MOD_INFORMATION *const modinfo : mods ) {
        if( modinfo->core ) {
            available_cores.push_back( modinfo->ident );
        } else {
            available_supplementals.push_back( modinfo->ident );
        }
    }
    ordered_mods.insert( ordered_mods.begin(), available_supplementals.begin(),
                         available_supplementals.end() );
    ordered_mods.insert( ordered_mods.begin(), available_cores.begin(), available_cores.end() );

    usable_mods = ordered_mods;
}
