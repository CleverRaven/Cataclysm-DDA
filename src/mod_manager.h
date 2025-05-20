#pragma once
#ifndef CATA_SRC_MOD_MANAGER_H
#define CATA_SRC_MOD_MANAGER_H

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_path.h"
#include "output.h"
#include "pimpl.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class dependency_tree;
class mod_manager;
struct WORLD;

const std::vector<std::pair<std::string, translation>> &get_mod_list_categories();
const std::vector<std::pair<std::string, translation>> &get_mod_list_tabs();
const std::map<std::string, std::string> &get_mod_list_cat_tab();

mod_id get_mod_base_id_from_src( mod_id src );

struct MOD_INFORMATION {
    private:
        friend mod_manager;
        translation name_;

    public:
        std::string name() const;

        mod_id ident;

        /** Directory to load JSON from relative to directory containing modinfo.json */
        cata_path path;

        /** All authors who have added content to the mod (excluding maintenance changes) */
        std::set<std::string> authors;

        /**
         *  Assigned maintainers responsible for maintaining compatibility with core
         *  @note these should be verbatim GH account names
         */
        std::set<std::string> maintainers;

        /** Full filenames (including extension) of any loading screens this mod may have */
        std::set<std::string> loading_images;

        translation description;
        std::string version;

        /** What other mods must be loaded prior to this one? */
        std::vector<mod_id> dependencies;

        /** What other mods are incompatible with this one? */
        std::vector<mod_id> conflicts;

        /** Core mods are loaded before any other mods */
        bool core = false;

        /** Obsolete mods are loaded for legacy saves but cannot be used when starting new worlds */
        bool obsolete = false;

        std::pair<int, translation> category = { -1, translation() };
};

// Enumerates and formats the mod origin
template<typename src_id>
std::string get_origin( const std::vector<std::pair<src_id, mod_id>> &src )
{
    std::string origin_str = enumerate_as_string( src.begin(),
    src.end(), []( const std::pair<src_id, mod_id> &source ) {
        return string_format( "'%s'", source.second->name() );
    }, enumeration_conjunction::arrow );
    return string_format( _( "Origin: %s" ), origin_str );
}

class mod_manager
{
    public:
        using t_mod_list = std::vector<mod_id>;

        mod_manager();
        ~mod_manager();
        /**
         * Reload the map of available mods (@ref mod_map).
         * This also reloads the dependency tree.
         */
        void refresh_mod_list();

        std::vector<mod_id> all_mods() const;

        /**
         * Returns the dependency tree for the loaded mods.
         * @returns @ref dependency_tree
         */
        dependency_tree &get_tree();
        /**
         * Clear @ref mod_map and delete @ref dependency_tree.
         */
        void clear();
        /**
         * Copy the json files of the listed mods into the
         * given folder (output_base_path)
         * @param mods_to_copy A list (of idents) of mods whose
         * json data should be copied.
         * @param output_base_path The folder where to put the copies.
         * The function creates a sub folder for each mod and
         * puts the files there. The original folder structure is
         * retained.
         */
        bool copy_mod_contents( const t_mod_list &mods_to_copy, const cata_path &output_base_path );

        /**
         * Save list of mods that are active in that world to
         * the world folder.
         */
        void save_mods_list( const WORLD *world ) const;
        /**
         * Load list of mods that should be active in that
         * world.
         */
        void load_mods_list( WORLD *world ) const;
        const t_mod_list &get_default_mods() const;
        bool set_default_mods( const t_mod_list &mods );
        const std::vector<mod_id> &get_usable_mods() const {
            return usable_mods;
        }

    private:
        // Make this accessible for now
        friend class mod_ui;
        friend class worldfactory;
        friend mod_id;
        /**
         * @returns path of a file in the world folder that contains
         * the list of mods that should be loaded for this world.
         */
        static cata_path get_mods_list_file( const WORLD *world );
        /**
         * Load all modinfo.json files (recursively) from the
         * given root.
         * @param path The root folder from which the modinfo
         * files are searched.
         */
        void load_mods_from( const cata_path &path );
        /**
         * Load all mod information from a json file.
         * (@see load_modfile)
         */
        void load_mod_info( const cata_path &info_file_path );
        /**
         * Load mod info from a json object. Put the loaded modinfo
         * directly into @ref mod_map.
         * @throws JsonError on all kind of errors.
         */
        void load_modfile( const JsonObject &jo, const cata_path &path );

        bool set_default_mods( const mod_id &ident );
        void remove_mod( const mod_id &ident );
        void remove_invalid_mods( std::vector<mod_id> &mods ) const;
        void load_replacement_mods( const cata_path &path );

        pimpl<dependency_tree> tree;

        /**
         * The map of known mods, key is the mod ident.
         */
        std::map<mod_id, MOD_INFORMATION> mod_map;
        t_mod_list default_mods;
        /** Second field is optional replacement mod */
        std::map<mod_id, mod_id> mod_replacements;

        std::vector<mod_id> usable_mods;

        void set_usable_mods();
};

class mod_ui
{
    public:
        explicit mod_ui( mod_manager &mman );

        std::string get_information( const MOD_INFORMATION *mod );
        mod_manager &active_manager;
        dependency_tree &mm_tree;

        void try_add( const mod_id &mod_to_add,
                      std::vector<mod_id> &active_list );
        void try_rem( size_t selection, std::vector<mod_id> &active_list );
        void try_shift( char direction, size_t &selection, std::vector<mod_id> &active_list );

        bool can_shift_up( size_t selection, const std::vector<mod_id> &active_list );
        bool can_shift_down( size_t selection, const std::vector<mod_id> &active_list );
};

#endif // CATA_SRC_MOD_MANAGER_H
