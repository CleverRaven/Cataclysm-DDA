#pragma once
#ifndef WORLDFACTORY_H
#define WORLDFACTORY_H

#include "cursesdef.h"
#include "enums.h"
#include "json.h"
#include "options.h"

#include <functional>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <iosfwd>

class JsonIn;

struct WORLD {
    std::string world_path;
    std::string world_name;
    std::unordered_map<std::string, options_manager::cOpt> WORLD_OPTIONS;
    std::vector<std::string> world_saves;
    /**
     * A (possibly empty) list of (idents of) mods that
     * should be loaded for this world.
     */
    std::vector<std::string> active_mod_order;

    WORLD();

    bool save_exists( const std::string &name ) const;
    void add_save( const std::string &name );

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
        /** Default constructor */
        worldfactory();
        /** Default destructor */
        virtual ~worldfactory();

        // Generate a world
        WORLDPTR make_new_world( bool show_prompt = true );
        WORLDPTR make_new_world( special_game_id special_type );
        // Used for unit tests - does NOT verify if the mods can be loaded
        WORLDPTR make_new_world( const std::vector<std::string> &mods );
        WORLDPTR convert_to_world( std::string origin_path );

        void set_active_world( WORLDPTR world );
        bool save_world( WORLDPTR world = NULL, bool is_conversion = false );
        std::map<std::string, WORLDPTR> get_all_worlds();

        WORLDPTR pick_world( bool show_prompt = true );

        WORLDPTR active_world;

        std::map<std::string, WORLDPTR> all_worlds;
        std::vector<std::string> all_worldnames;

        mod_manager *get_mod_manager();

        void remove_world( std::string worldname );
        bool valid_worldname( std::string name, bool automated = false );

        /**
         * World need CDDA build with Lua support
         * @param world_name World name to test
         * @return True if world can't be loaded without Lua support. False otherwise. (When LUA is defined it's allways false).
         */
        bool world_need_lua_build( std::string world_name );
    protected:
    private:
        std::string pick_random_name();
        int show_worldgen_tab_options( WINDOW *win, WORLDPTR world );
        int show_worldgen_tab_modselection( WINDOW *win, WORLDPTR world );
        int show_worldgen_tab_confirm( WINDOW *win, WORLDPTR world );

        void draw_modselection_borders( WINDOW *win, input_context *ctxtp );
        void draw_worldgen_tabs( WINDOW *win, unsigned int current );
        void draw_mod_list( WINDOW *w, int &start, int &cursor, const std::vector<std::string> &mods,
                            bool is_active_list, const std::string &text_if_empty, WINDOW *w_shift );

        bool load_world_options( WORLDPTR &world );

        WORLDPTR add_world( WORLDPTR world );

        std::unique_ptr<mod_manager> mman;
        std::unique_ptr<mod_ui> mman_ui;

        typedef std::function<int( WINDOW *, WORLDPTR )> worldgen_display;

        std::vector<worldgen_display> tabs;
        std::vector<std::string> tab_strings;
};

void load_world_option( JsonObject &jo );

extern std::unique_ptr<worldfactory> world_generator;

#endif
