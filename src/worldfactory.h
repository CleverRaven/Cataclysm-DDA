#ifndef WORLDFACTORY_H
#define WORLDFACTORY_H

#include "options.h"
#include "output.h"
#include "debug.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "keypress.h"
#include "input.h"
#include "gamemode.h"

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

extern std::map<std::string, cOpt> OPTIONS;
extern std::map<std::string, cOpt> ACTIVE_WORLD_OPTIONS;

struct WORLD
{
    std::string world_path;
    std::string world_name;
    std::map<std::string, cOpt> world_options;
    std::vector<std::string> world_saves;

    WORLD();
};

class mod_manager;
class mod_ui;

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
        WORLDPTR make_new_world(special_game_id special_type);
        WORLDPTR convert_to_world(std::string origin_path);

        void set_active_world(WORLDPTR world);
        bool save_world(WORLDPTR world = NULL, bool is_conversion = false);
        std::map<std::string, WORLDPTR> get_all_worlds();

        WORLDPTR pick_world( bool show_prompt = true );

        WORLDPTR active_world;

        std::map<std::string, WORLDPTR> all_worlds;
        std::vector<std::string> all_worldnames;

        std::vector<std::string> active_mod_order;

        void remove_world(std::string worldname);
        bool valid_worldname(std::string name, bool automated = false);
    protected:
    private:
        std::string pick_random_name();
        int show_worldgen_tab_options(WINDOW *win, WORLDPTR world);
        int show_worldgen_tab_modselection(WINDOW *win, WORLDPTR world);
        int show_worldgen_tab_confirm(WINDOW *win, WORLDPTR world);

        void draw_worldgen_tabs(WINDOW *win, int current, std::vector<std::string> tabs);

        std::map<std::string, cOpt> get_default_world_options();
        std::map<std::string, cOpt> get_world_options(std::string path);
        mod_manager *mman;
        mod_ui *mman_ui;
};

extern worldfactory *world_generator;
#endif // WORLDFACTORY_H
