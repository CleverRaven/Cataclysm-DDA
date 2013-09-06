#ifndef WORLD_FACTORY_H
#define WORLD_FACTORY_H

#include "options.h"
#include "output.h"
#include "debug.h"
#include "catacurse.h"
#include "catacharset.h"
#include "keypress.h"
#include "input.h"

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <stdlib.h>
#include <fstream>

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif


extern std::map<std::string, cOpt> OPTIONS;

typedef struct WORLD
{
    std::string world_name;
    std::map<std::string, cOpt> world_options;
    std::vector<std::string> world_saves;

    WORLD()
    {
        world_name = "";
        world_options.clear();
        world_saves.clear();
    }
} WORLD;

class world_factory
{
    public:
        /** Default constructor */
        world_factory();
        /** Default destructor */
        ~world_factory();
        /** Generate a World */
        WORLD *make_new_world();
        /** Load an existing World */
        WORLD *load_world(std::string world_name, bool setactive = false);

        void set_active_world(WORLD *world);
        void save_world(WORLD *world = NULL);
        std::map<std::string, WORLD*> get_all_worlds();
        WORLD *pick_world();

        WORLD *active_world;
        std::map<std::string, WORLD*> all_worlds;
        std::vector<std::string> all_worldnames;

        void remove_world(std::string worldname);

    protected:
    private:
        std::string pick_random_name();
        int show_worldgen_tab_options(WINDOW *win, WORLD *world);
        int show_worldgen_tab_confirm(WINDOW *win, WORLD *world);

        void draw_worldgen_tabs(WINDOW *win, int current, std::vector<std::string> tabs);

        std::map<std::string, cOpt> get_world_options(std::string path);
};
extern world_factory *world_generator;

#endif // WORLD_FACTORY_H
