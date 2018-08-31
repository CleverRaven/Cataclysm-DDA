#pragma once
#ifndef WORLDFACTORY_H
#define WORLDFACTORY_H

#include "options.h"
#include "pimpl.h"
#include "string_id.h"

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iosfwd>

class JsonIn;
class JsonObject;
enum special_game_id : int;
namespace catacurses
{
class window;
} // namespace catacurses
struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;

class save_t
{
    private:
        std::string name;

        save_t( const std::string &name );

    public:
        std::string player_name() const;
        std::string base_path() const;

        static save_t from_player_name( const std::string &name );
        static save_t from_base_path( const std::string &base_path );

        bool operator==( const save_t &rhs ) const {
            return name == rhs.name;
        }
        bool operator!=( const save_t &rhs ) const {
            return !operator==( rhs );
        }
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

        std::string world_name;
        options_manager::options_container WORLD_OPTIONS;
        std::vector<save_t> world_saves;
        /**
         * A (possibly empty) list of (idents of) mods that
         * should be loaded for this world.
         */
        std::vector<mod_id> active_mod_order;

        WORLD();

        bool save_exists( const save_t &name ) const;
        void add_save( const save_t &name );

        void load_options( JsonIn &jsin );
        void load_legacy_options( std::istream &fin );
};

class mod_manager;
class mod_ui;
class input_context;

typedef WORLD *WORLDPTR;

class worldfactory
{
    public:
        worldfactory();
        ~worldfactory();

        // Generate a world
        WORLDPTR make_new_world( bool show_prompt = true );
        WORLDPTR make_new_world( special_game_id special_type );
        // Used for unit tests - does NOT verify if the mods can be loaded
        WORLDPTR make_new_world( const std::vector<mod_id> &mods );
        WORLDPTR convert_to_world( const std::string &origin_path );
        /// Returns the *existing* world of given name.
        WORLDPTR get_world( const std::string &name );
        bool has_world( const std::string &name ) const;

        void set_active_world( WORLDPTR world );
        bool save_world( WORLDPTR world = NULL, bool is_conversion = false );

        void init();

        WORLDPTR pick_world( bool show_prompt = true );

        WORLDPTR active_world;

        std::vector<std::string> all_worldnames() const;

        mod_manager &get_mod_manager();

        void remove_world( const std::string &worldname );
        bool valid_worldname( const std::string &name, bool automated = false );

        /**
         * World need CDDA build with Lua support
         * @param world_name World name to test
         * @return True if world can't be loaded without Lua support. False otherwise. (When LUA is defined it's always false).
         */
        bool world_need_lua_build( std::string world_name );
        /**
         * @param delete_folder If true: delete all the files and directories  of the given
         * world folder. Else just avoid deleting the config files and the directory
         * itself.
         */
        void delete_world( const std::string &worldname, bool delete_folder );

        static void draw_worldgen_tabs( const catacurses::window &win, size_t current );
        void show_active_world_mods( const std::vector<mod_id> &world_mods );

    protected:
    private:
        std::map<std::string, WORLDPTR> all_worlds;

        std::string pick_random_name();
        int show_worldgen_tab_options( const catacurses::window &win, WORLDPTR world );
        int show_worldgen_tab_modselection( const catacurses::window &win, WORLDPTR world );
        int show_worldgen_tab_confirm( const catacurses::window &win, WORLDPTR world );

        void draw_modselection_borders( const catacurses::window &win, const input_context &ctxtp );
        void draw_mod_list( const catacurses::window &w, int &start, size_t cursor,
                            const std::vector<mod_id> &mods, bool is_active_list, const std::string &text_if_empty,
                            const catacurses::window &w_shift );

        bool load_world_options( WORLDPTR &world );

        WORLDPTR add_world( WORLDPTR world );

        pimpl<mod_manager> mman;
        pimpl<mod_ui> mman_ui;

        typedef std::function<int( const catacurses::window &, WORLDPTR )> worldgen_display;

        std::vector<worldgen_display> tabs;
};

void load_world_option( JsonObject &jo );

//load external option from json
void load_external_option( JsonObject &jo );

extern std::unique_ptr<worldfactory> world_generator;

#endif
