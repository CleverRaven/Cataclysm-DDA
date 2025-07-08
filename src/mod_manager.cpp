#include "mod_manager.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iterator>
#include <memory>
#include <ostream>
#include <queue>

#include "assign.h"
#include "cata_utility.h"
#include "debug.h"
#include "dependency_tree.h"
#include "filesystem.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "localized_comparator.h"
#include "output.h"
#include "path_info.h"
#include "string_formatter.h"
#include "worldfactory.h"

static const mod_id MOD_INFORMATION_dev_default( "dev:default" );
static const mod_id MOD_INFORMATION_user_default( "user:default" );

static const std::string MOD_SEARCH_FILE( "modinfo.json" );

mod_id get_mod_base_id_from_src( mod_id src )
{
    mod_id base_mod_id;
    size_t split_loc = src.str().find( '#' );
    if( split_loc == std::string::npos ) {
        return src;
    } else {
        return mod_id( src.str().substr( 0, split_loc ) );
    }
}

template<>
const MOD_INFORMATION &string_id<MOD_INFORMATION>::obj() const
{
    const auto &map = world_generator->get_mod_manager().mod_map;
    const auto iter = map.find( get_mod_base_id_from_src( *this ) );
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
        return name_.translated();
    }
}

// These accessors are to delay the initialization of the strings in the respective containers until after gettext is initialized.
const std::vector<std::pair<std::string, translation>> &get_mod_list_categories()
{
    static const std::vector<std::pair<std::string, translation>> mod_list_categories = {
        {"total_conversion", to_translation( "TOTAL CONVERSIONS" )},
        {"content", to_translation( "CORE CONTENT PACKS" )},
        {"items", to_translation( "ITEM ADDITION MODS" )},
        {"creatures", to_translation( "CREATURE MODS" )},
        {"misc_additions", to_translation( "MISC ADDITIONS" )},
        {"buildings", to_translation( "BUILDINGS MODS" )},
        {"vehicles", to_translation( "VEHICLE MODS" )},
        {"rebalance", to_translation( "REBALANCING MODS" )},
        {"magical", to_translation( "MAGICAL MODS" )},
        {"item_exclude", to_translation( "ITEM EXCLUSION MODS" )},
        {"monster_exclude", to_translation( "MONSTER EXCLUSION MODS" )},
        {"graphical", to_translation( "GRAPHICAL MODS" )},
        {"accessibility", to_translation( "ACCESSIBILITY MODS" )},
        {"", to_translation( "NO CATEGORY" )}
    };

    return mod_list_categories;
}

const std::vector<std::pair<std::string, translation>> &get_mod_list_tabs()
{
    static const std::vector<std::pair<std::string, translation>> mod_list_tabs = {
        {"tab_default", to_translation( "Default" )},
        {"tab_blacklist", to_translation( "Blacklist" )},
        {"tab_balance", to_translation( "Balance" )}
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

static std::map<mod_id, mod_id> migrated_mods;
static std::map<mod_id, translation> removed_mods;

void mod_migrations::load( const JsonObject &jo )
{
    const mod_id old_id( jo.get_string( "id" ) );
    if( jo.has_string( "new_id" ) ) {
        const mod_id new_id( jo.get_string( "new_id" ) );
        migrated_mods.insert( std::make_pair( old_id, new_id ) );
    } else {
        translation removal_reason;
        jo.read( "removal_reason", removal_reason );
        removed_mods.insert( std::make_pair( old_id, removal_reason ) );
    }
}

void mod_migrations::reset()
{
    migrated_mods.clear();
    removed_mods.clear();
}

void mod_migrations::check()
{
    for( const auto &migration : migrated_mods ) {
        if( !migration.second.is_valid() ) {
            debugmsg( "mod_migration from '%s' specifies invalid new_id '%s'", migration.first.c_str(),
                      migration.second.c_str() );
        }
    }
}

mod_manager::mod_manager()
{
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
    migrated_mods.clear();
    removed_mods.clear();
}

void mod_manager::refresh_mod_list()
{
    clear();

    if( !dir_exist( PATH_INFO::user_moddir() ) ) {
        assure_dir_exist( PATH_INFO::user_moddir() );
    }

    std::map<mod_id, std::vector<mod_id>> mod_dependency_map;
    load_mods_from( PATH_INFO::moddir() );
    load_mods_from( PATH_INFO::user_moddir_path() );

    if( file_exist( PATH_INFO::mods_dev_default() ) ) {
        load_mod_info( PATH_INFO::mods_dev_default() );
    }
    if( file_exist( PATH_INFO::mods_user_default() ) ) {
        load_mod_info( PATH_INFO::mods_user_default() );
    }

    if( !set_default_mods( MOD_INFORMATION_user_default ) ) {
        set_default_mods( MOD_INFORMATION_dev_default );
    }
    for( auto &elem : mod_map ) {
        const auto &deps = elem.second.dependencies;
        mod_dependency_map[elem.second.ident] = std::vector<mod_id>( deps.begin(), deps.end() );
    }
    tree->init( mod_dependency_map );
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
    default_mods = deps;
    return true;
}

void mod_manager::load_mods_from( const cata_path &path )
{
    if( !dir_exist( path.get_unrelative_path() ) ) {
        return; // don't try to enumerate non-existing directories
    }
    for( cata_path &mod_file : get_files_from_path( MOD_SEARCH_FILE, path, true ) ) {
        load_mod_info( mod_file );
    }
}

void mod_manager::load_modfile( const JsonObject &jo, const cata_path &path )
{
    if( !jo.has_string( "type" ) || jo.get_string( "type" ) != "MOD_INFO" ) {
        // Ignore anything that is not a mod-info
        jo.allow_omitted_members();
        return;
    }

    const mod_id m_ident( jo.get_string( "id" ) );
    // can't use string_id::is_valid as the global mod_manger instance does not exist yet
    if( mod_map.count( m_ident ) > 0 ) {
        // TODO: change this to make unique ident for the mod
        // (instead of discarding it?)
        debugmsg( "there is already a mod with ident %s", m_ident.c_str() );
        return;
    }
    if( m_ident.str().find( '#' ) != std::string::npos ) {
        debugmsg( "Mod id %s contains illegal '#' character.", m_ident.str() );
        return;
    }

    translation m_name;
    jo.read( "name", m_name );

    std::string m_cat = jo.get_string( "category", "" );
    std::pair<int, translation> p_cat = {-1, translation()};
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

    std::string mod_json_path;
    if( assign( jo, "path", mod_json_path ) ) {
        modfile.path = path / mod_json_path;
    } else {
        modfile.path = path;
    }

    assign( jo, "authors", modfile.authors );
    assign( jo, "maintainers", modfile.maintainers );
    assign( jo, "description", modfile.description );
    assign( jo, "version", modfile.version );
    assign( jo, "dependencies", modfile.dependencies );
    assign( jo, "conflicts", modfile.conflicts );
    assign( jo, "core", modfile.core );
    assign( jo, "obsolete", modfile.obsolete );
    assign( jo, "loading_images", modfile.loading_images );

    if( std::find( modfile.dependencies.begin(), modfile.dependencies.end(),
                   modfile.ident ) != modfile.dependencies.end() ) {
        jo.throw_error_at( "dependencies", "mod specifies self as a dependency" );
    }

    // TODO: Temporary migration, remove after 0.I stable
    if( !modfile.obsolete && modfile.ident.str() == "user:default" ) {
        modfile.obsolete = true;
        set_default_mods( modfile.dependencies );
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
        json.member( "id", "user:default" );
        json.member( "conflicts", std::vector<std::string>() );
        json.member( "dependencies" );
        json.write( mods );
        json.member( "//",
                     "Not really obsolete!  Marked as such to prevent it from showing in the main list" );
        json.member( "obsolete", true );
        json.end_object();
    }, _( "list of default mods" ) );
}

bool mod_manager::copy_mod_contents( const t_mod_list &mods_to_copy,
                                     const cata_path &output_base_path )
{
    if( mods_to_copy.empty() ) {
        // nothing to copy, so technically we succeeded already!
        return true;
    }
    std::vector<std::string> search_extensions;
    search_extensions.emplace_back( ".json" );

    DebugLog( D_INFO, DC_ALL ) << "Copying mod contents into directory: " << output_base_path;

    if( !assure_dir_exist( output_base_path ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" << output_base_path <<
                                    "] for saving";
        return false;
    }

    for( size_t i = 0; i < mods_to_copy.size(); ++i ) {
        const MOD_INFORMATION &mod = *mods_to_copy[i];

        // now to get all of the json files inside of the mod and get them ready to copy
        auto input_files = get_files_from_path( ".json", mod.path, true, true );
        auto input_dirs  = get_directories_with( search_extensions, mod.path, true );

        cata_path mod_base_path = mod.path;

        if( input_files.empty() && mod.path.get_relative_path().filename().u8string() == MOD_SEARCH_FILE ) {
            // Self contained mod, all data is inside the modinfo.json file
            input_files.push_back( mod.path );
            mod_base_path = mod.path.parent_path();
        }

        if( input_files.empty() ) {
            continue;
        }

        // create needed directories
        // NOLINTNEXTLINE(cata-translate-string-literal)
        const cata_path cur_mod_dir = output_base_path / string_format( "mod_%05d", i + 1 );

        std::queue<cata_path> dir_to_make;
        dir_to_make.push( cur_mod_dir );
        for( cata_path &input_dir : input_dirs ) {
            dir_to_make.push( cur_mod_dir / input_dir.get_relative_path().lexically_relative(
                                  mod_base_path.get_relative_path() ) );
        }

        while( !dir_to_make.empty() ) {
            if( !assure_dir_exist( dir_to_make.front() ) ) {
                DebugLog( D_ERROR, DC_ALL ) << "Unable to create or open mod directory at [" <<
                                            dir_to_make.front() << "] for saving";
            }

            dir_to_make.pop();
        }

        // trim file paths from full length down to just /data forward
        for( cata_path &input_file : input_files ) {
            cata_path output_path = cur_mod_dir / ( input_file.get_relative_path().lexically_relative(
                    mod_base_path.get_relative_path() ) );
            copy_file( input_file, output_path );
        }
    }
    return true;
}

void mod_manager::load_mod_info( const cata_path &info_file_path )
{
    const cata_path main_path = info_file_path.parent_path();
    read_from_file_optional_json( info_file_path, [&]( const JsonValue & jsin ) {
        if( jsin.test_object() ) {
            // find type and dispatch single object
            JsonObject jo = jsin.get_object();
            load_modfile( jo, main_path );
        } else if( jsin.test_array() ) {
            // find type and dispatch each object until array close
            for( JsonObject jo : jsin.get_array() ) {
                load_modfile( jo, main_path );
            }
        } else {
            // not an object or an array?
            jsin.throw_error( "expected array or object" );
        }
    } );
}

cata_path mod_manager::get_mods_list_file( const WORLD *world )
{
    return world->folder_path() / "mods.json";
}

void mod_manager::save_mods_list( const WORLD *world ) const
{
    if( world == nullptr ) {
        return;
    }
    const cata_path path = get_mods_list_file( world );
    if( world->active_mod_order.empty() ) {
        // If we were called from load_mods_list to prune the list,
        // and it's empty now, delete the file.
        remove_file( path.get_unrelative_path() );
        return;
    }
    write_to_file( path, [&]( std::ostream & fout ) {
        JsonOut json( fout, true ); // pretty-print
        json.write( world->active_mod_order );
    }, _( "list of mods" ) );
}

void mod_manager::load_mods_list( WORLD *world ) const
{
    if( world == nullptr ) {
        return;
    }
    std::vector<mod_id> &amo = world->active_mod_order;
    amo.clear();
    bool changed = false;
    read_from_file_optional_json( get_mods_list_file( world ), [&]( const JsonArray & jsin ) {
        for( const std::string line : jsin ) {
            const mod_id mod( line );
            if( std::find( amo.begin(), amo.end(), mod ) != amo.end() ) {
                continue;
            }
            if( mod.is_valid() ) {
                amo.push_back( mod );
            } else {
                if( const auto it = migrated_mods.find( mod ); it != migrated_mods.end() &&
                    std::find( amo.begin(), amo.end(), it->second ) == amo.end() ) {
                    amo.push_back( it->second );
                    changed = true;
                } else if( const auto it = removed_mods.find( mod ); it != removed_mods.end() ) {
                    if( !query_yn(
                            _( "Mod %s has been removed with reason: %s\nRemove it from this world's active mods?" ),
                            mod.c_str(), it->second.translated() ) ) {
                        amo.push_back( mod );
                    } else {
                        changed = true;
                    }
                } else {
                    if( !query_yn( _( "Mod %s not found in mods folder, remove it from this world's active mods?" ),
                                   mod.c_str() ) ) {
                        amo.push_back( mod );
                    } else {
                        changed = true;
                    }
                }
            }
        }
    } );
    // If we migrated or the player chose to remove a mod, overwrite the mod list.
    if( changed ) {
        save_mods_list( world );
    }
}

const mod_manager::t_mod_list &mod_manager::get_default_mods() const
{
    return default_mods;
}

static bool compare_mod_by_name_and_category( const MOD_INFORMATION *const a,
        const MOD_INFORMATION *const b )
{
    return localized_compare( std::make_pair( a->category, a->name() ),
                              std::make_pair( b->category, b->name() ) );
}

void mod_manager::set_usable_mods()
{
    std::vector<mod_id> available_cores;
    std::vector<mod_id> available_supplementals;
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
