#pragma once
#ifndef CATA_SRC_WORLDFACTORY_H
#define CATA_SRC_WORLDFACTORY_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "cuboid_rectangle.h"
#include "options.h"
#include "pimpl.h"
#include "type_id.h"

enum class special_game_type;

class JsonArray;
class JsonObject;

namespace catacurses
{
class window;
} // namespace catacurses

class save_t
{
    private:
        std::string name;

        explicit save_t( const std::string &name );

    public:
        std::string decoded_name() const;
        std::string base_path() const;

        static save_t from_save_id( const std::string &save_id );
        static save_t from_base_path( const std::string &base_path );

        bool operator==( const save_t &rhs ) const {
            return name == rhs.name;
        }
        bool operator!=( const save_t &rhs ) const {
            return !operator==( rhs );
        }
        save_t( const save_t & ) = default;
        save_t &operator=( const save_t & ) = default;
};

struct WORLD {
    public:
        /**
         * @returns A path to a folder in the file system that should contain
         * all the world specific files. It depends on @ref world_name,
         * changing that will also change the result of this function.
         */
        std::string folder_path() const;
        cata_path folder_path_path() const;

        std::string world_name;
        options_manager::options_container WORLD_OPTIONS;
        std::vector<save_t> world_saves;
        /**
         * A (possibly empty) list of (idents of) mods that
         * should be loaded for this world.
         */
        std::vector<mod_id> active_mod_order;

        WORLD();
        explicit WORLD( const std::string &name );
        void COPY_WORLD( const WORLD *world_to_copy );

        bool save_exists( const save_t &name ) const;
        void add_save( const save_t &name );

        bool save( bool is_conversion = false ) const;

        void load_options( const JsonArray &options_json );
        bool load_options();
};

class mod_manager;
class mod_ui;
class input_context;

class worldfactory
{
    public:
        worldfactory();
        ~worldfactory();

        // Generate a world
        WORLD *make_new_world( bool show_prompt = true, const std::string &world_to_copy = "" );
        WORLD *make_new_world( special_game_type special_type );
        // Used for unit tests - does NOT verify if the mods can be loaded
        WORLD *make_new_world( const std::string &name, const std::vector<mod_id> &mods );
        WORLD *make_new_world( const std::vector<mod_id> &mods );
        /// Returns the *existing* world of given name.
        WORLD *get_world( const std::string &name );
        /// Returns the *existing* world's name from its index in the world list.
        std::string get_world_name( size_t index );
        /// Returns the *existing* world's index in the world list from its name.
        size_t get_world_index( const std::string &name );
        bool has_world( const std::string &name ) const;

        void set_active_world( WORLD *world );

        void init();

        WORLD *pick_world( bool show_prompt = true, bool empty_only = false );

        WORLD *active_world;

        std::vector<std::string> all_worldnames() const;

        std::string last_world_name;
        std::string last_character_name;

        void save_last_world_info() const;

        mod_manager &get_mod_manager();

        void remove_world( const std::string &worldname );
        bool valid_worldname( const std::string &name, bool automated = false ) const;

        /**
         * @param delete_folder If true: delete all the files and directories  of the given
         * world folder. Else just avoid deleting the config files and the directory
         * itself.
         */
        void delete_world( const std::string &worldname, bool delete_folder );

        static std::map<size_t, inclusive_rectangle<point>> draw_worldgen_tabs( const catacurses::window &w,
                size_t current );
        void show_active_world_mods( const std::vector<mod_id> &world_mods );

        const std::map<std::string, std::unique_ptr<WORLD>> &get_all_worlds() const;

    private:
        std::map<std::string, std::unique_ptr<WORLD>> all_worlds;

        void load_last_world_info();

        std::string pick_random_name();
        int show_worldgen_tab_options( const catacurses::window &win, WORLD *world, bool with_tabs );
        int show_worldgen_tab_modselection( const catacurses::window &win, WORLD *world, bool with_tabs );
        int show_worldgen_basic( WORLD *world );
        int show_worldgen_advanced( WORLD *world );

        void draw_modselection_borders( const catacurses::window &win, const input_context &ctxtp );
        std::map<int, inclusive_rectangle<point>> draw_mod_list( const catacurses::window &w, int &start,
                                               size_t cursor, const std::vector<mod_id> &mods,
                                               bool is_active_list, const std::string &text_if_empty,
                                               const catacurses::window &w_shift, bool recalc_start );

        WORLD *add_world( std::unique_ptr<WORLD> retworld );

        pimpl<mod_manager> mman;
        pimpl<mod_ui> mman_ui;

        using worldgen_display = std::function<int ( const catacurses::window &, WORLD *, bool )>;

        std::vector<worldgen_display> tabs;
};

void load_world_option( const JsonObject &jo );

//load external option from json
void load_external_option( const JsonObject &jo );

extern std::unique_ptr<worldfactory> world_generator;

#endif // CATA_SRC_WORLDFACTORY_H
