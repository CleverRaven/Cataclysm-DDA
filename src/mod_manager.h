#ifndef MOD_MANAGER_H
#define MOD_MANAGER_H

#include "dependency_tree.h"
#include "json.h"
#include "translations.h"

#include "cursesdef.h"
#include <string>
#include <vector>
#include <map>

const std::vector<std::pair<std::string, std::string> > &get_mod_list_categories();

struct WORLD;
typedef WORLD *WORLDPTR;

class mod_ui;
class game;
class worldfactory;

const std::vector<std::pair<std::string, std::string> > &get_mod_list_categories();
const std::vector<std::pair<std::string, std::string> > &get_mod_list_tabs();
const std::map<std::string, std::string> &get_mod_list_cat_tab();

enum mod_type {
    MT_CORE,
    MT_SUPPLEMENTAL
};
struct MOD_INFORMATION {
    std::string path;
    std::string name;
    std::string ident;
    std::vector<std::string> authors;
    std::string description;
    mod_type _type;
    std::vector<std::string> dependencies;
    bool obsolete;
    /** Mod require Lua support **/
    bool need_lua;

    std::pair<int, std::string> category;

    MOD_INFORMATION() : _type( MT_SUPPLEMENTAL ), obsolete( false ), category( {
        -1, ""
    } ) {};

    std::string type() {
        switch( _type ) {
            case MT_CORE:
                return "CORE";
                break;
            case MT_SUPPLEMENTAL:
                return "SUPPLEMENTAL";
                break;
        }
    };
};

class mod_manager
{
    public:
        typedef std::vector<std::string> t_mod_list;

        mod_manager();
        virtual ~mod_manager();
        /**
         * Reload the map of available mods (@ref mod_map).
         * This also reloads the dependency tree.
         */
        void refresh_mod_list();

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
         * @returns true if the mod manager knows a mod with this ident.
         */
        bool has_mod( const std::string &ident ) const;
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
        bool copy_mod_contents( const t_mod_list &mods_to_copy, const std::string &output_base_path );

        /**
         * Save list of mods that are active in that world to
         * the world folder.
         */
        void save_mods_list( WORLDPTR world ) const;
        /**
         * Load list of mods that should be active in that
         * world.
         */
        void load_mods_list( WORLDPTR world ) const;
        const t_mod_list &get_default_mods() const;
        bool set_default_mods( const t_mod_list &mods );
    protected:
    private:
        // Make this accessible for now
        friend class mod_ui;
        friend class worldfactory;
        friend class game;
        /**
         * @returns path of a file in the world folder that contains
         * the list of mods that should be loaded for this world.
         */
        static std::string get_mods_list_file( const WORLDPTR world );
        /**
         * Load all modinfo.json files (recursively) from the
         * given root.
         * @param path The root folder from which the modinfo
         * files are searched.
         */
        void load_mods_from( std::string path );
        /**
         * Load all mod information from a json file.
         * (@see load_modfile)
         */
        void load_mod_info( std::string info_file_path );
        /**
         * Load mod info from a json object. Put the loaded modinfo
         * directly into @ref mod_map.
         * @throws std::string on all kind of errors. The string
         * contains the error message.
         */
        void load_modfile( JsonObject &jo, const std::string &main_path );

        bool set_default_mods( const std::string &ident );
        void remove_mod( const std::string &ident );
        void remove_invalid_mods( std::vector<std::string> &mods ) const;

        dependency_tree tree;

        typedef std::map<std::string, MOD_INFORMATION *> t_mod_map;
        /**
         * The map of known mod, key is the mod ident. Values are
         * never NULL.
         */
        t_mod_map mod_map;
        t_mod_list default_mods;
};

class mod_ui
{
    public:
        mod_ui( mod_manager *modman );
        virtual ~mod_ui();

        std::vector<std::string> usable_mods;
        std::string get_information( MOD_INFORMATION *mod );
        mod_manager *active_manager;
        dependency_tree *mm_tree;

        void try_add( const std::string &mod_to_add,
                      std::vector<std::string> &active_list );
        void try_rem( int selection, std::vector<std::string> &active_list );
        void try_shift( char direction, int &selection, std::vector<std::string> &active_list );

        bool can_shift_up( int selection, std::vector<std::string> active_list );
        bool can_shift_down( int selection, std::vector<std::string> active_list );

    private:
        void set_usable_mods();
};

#endif
