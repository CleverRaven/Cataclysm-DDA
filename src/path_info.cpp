#include <cstdlib>
#include "path_info.h"

//#ifdef __linux__
//#include <unistd.h>
//#endif // __linux__

void init_base_path(std::string path)
{
    if (!path.empty()) {
        char ch;
        ch = path.at(path.length() - 1);
        if (ch != '/' || ch != '\\') {
            path.push_back('/');
        }
    }

    FILENAMES.insert(std::pair<std::string,std::string>("base_path", path));
}

void init_user_dir(const char *ud)
{
    std::string dir = std::string(ud);

    if (dir.empty()) {
        const char *user_dir;
#if (defined _WIN32 || defined WINDOW)
        user_dir = getenv("LOCALAPPDATA");
        // On Windows userdir without dot
        dir = std::string(user_dir) + "/cataclysm-dda/";
#else
        user_dir = getenv("HOME");
        dir = std::string(user_dir) + "/.cataclysm-dda/";
#endif
    }

    FILENAMES.insert(std::pair<std::string,std::string>("user_dir", dir));
}

static void update_pathname(std::string name, std::string path)
{
    std::map<std::string,std::string>::iterator iter;

    iter = FILENAMES.find(name);
    if (iter != FILENAMES.end()) {
        FILENAMES[name] = path;
    } else {
        FILENAMES.insert(std::pair<std::string, std::string>(name, path));
    }
}

void set_standart_filenames(void)
{
    const char *share_dir;
#if !(defined _WIN32 || defined WINDOW)
    if (!FILENAMES["base_path"].empty()) {
        share_dir = "share/cataclysm-dda/";
    } else {
        share_dir = "data/";
    }
#else
    share_dir = "data/";
#endif
    // Shared directories
    update_pathname("gfxdir", FILENAMES["base_path"] + share_dir + "gfx/");
    update_pathname("luadir", FILENAMES["base_path"] + share_dir + "lua/");
    update_pathname("datadir", FILENAMES["base_path"] + share_dir);
    update_pathname("autoexeclua", FILENAMES["luadir"] + "autoexec.lua");
    update_pathname("fontdir", FILENAMES["datadir"] + "font/");
    update_pathname("rawdir", FILENAMES["datadir"] + "raw/");
    update_pathname("jsondir", FILENAMES["datadir"] + "json/");
    update_pathname("moddir", FILENAMES["datadir"] + "mods/");
    update_pathname("recycledir", FILENAMES["datadir"] + "recycling/");
    update_pathname("namesdir", FILENAMES["datadir"] + "names/");

    // Shared files
    update_pathname("motd", FILENAMES["datadir"] + "motd");
    update_pathname("credits", FILENAMES["datadir"] + "credits");
    // TODO Load localized names
    update_pathname("names", FILENAMES["namesdir"] + "en.json");
    update_pathname("colors", FILENAMES["rawdir"] + "colors.json");
    update_pathname("keybindings", FILENAMES["rawdir"] + "keybindings.json");
    update_pathname("sokoban", FILENAMES["rawdir"] + "sokoban.txt");
    update_pathname("defaulttilejson", FILENAMES["gfx"] + "tile_config.json");
    update_pathname("defaulttilepng", FILENAMES["gfx"] + "tinytile.png");
    update_pathname("mods-dev-default", FILENAMES["moddir"] + "dev-default-mods.json");

    // User directories
    update_pathname("savedir", FILENAMES["user_dir"] + "save/");
    update_pathname("memorialdir", FILENAMES["user_dir"] + "memorial/");
    update_pathname("templatedir", FILENAMES["templatedir"] + "templates/");

    // User files
    update_pathname("options", FILENAMES["user_dir"] + "options.txt");
    update_pathname("keymap", FILENAMES["user_dir"] + "keymap.txt");
    update_pathname("debug", FILENAMES["user_dir"] + "debug.log");
    update_pathname("fontlist", FILENAMES["user_dir"] + "fontlist.txt");
    update_pathname("fontdata", FILENAMES["user_dir"] + "FONTDATA");
    update_pathname("autopickup", FILENAMES["user_dir"] + "auto_pickup.txt");
    update_pathname("mods-user-default", FILENAMES["user_dir"] + "user-default-mods.json");
    /*
    FILENAMES.insert(std::pair<std::string,std::string>("mainlua", FILENAMES["datadir"] + "main.lua"));
    FILENAMES.insert(std::pair<std::string,std::string>("typeface", FILENAMES["fontdir"] + "fixedsys.ttf"));
    */
}
