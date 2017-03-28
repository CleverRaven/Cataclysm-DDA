#include "path_info.h"
#include "options.h"
#include "filesystem.h"
#include <cstdlib>
#include <locale.h>

/** Map where we store filenames */
std::map<std::string, std::string> FILENAMES;

void PATH_INFO::init_base_path(std::string path)
{
    if (!path.empty()) {
        char ch = path.at(path.length() - 1);
        if (ch != '/' && ch != '\\') {
            path.push_back('/');
        }
    }

    //FILENAMES.insert(std::pair<std::string,std::string>("base_path", path));
    FILENAMES["base_path"] = path;
}

void PATH_INFO::init_user_dir(const char *ud)
{
    std::string dir = std::string(ud);

    if (dir.empty()) {
        const char *user_dir;
#if (defined _WIN32 || defined WINDOW)
        user_dir = getenv("LOCALAPPDATA");
        // On Windows userdir without dot
        dir = std::string(user_dir) + "/cataclysm-dda/";
#elif defined MACOSX
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/Library/Application Support/Cataclysm/";
#elif (defined USE_XDG_DIR)
        if ( (user_dir = getenv("XDG_DATA_HOME")) ) {
            dir = std::string(user_dir) + "/cataclysm-dda/";
        } else {
            user_dir = getenv("HOME");
            dir = std::string(user_dir) + "/.local/share/cataclysm-dda/";
        }
#else
        user_dir = getenv("HOME");
        dir = std::string(user_dir) + "/.cataclysm-dda/";
#endif
    }

    FILENAMES["user_dir"] = dir;
}

void PATH_INFO::update_pathname(std::string name, std::string path)
{
    std::map<std::string, std::string>::iterator iter;

    iter = FILENAMES.find(name);
    if (iter != FILENAMES.end()) {
        FILENAMES[name] = path;
    } else {
        FILENAMES.insert(std::pair<std::string, std::string>(name, path));
    }
}

void PATH_INFO::update_datadir()
{
    // Shared dirs
    update_pathname("gfxdir", FILENAMES["datadir"] + "gfx/");
    update_pathname("luadir", FILENAMES["datadir"] + "lua/");
    update_pathname("fontdir", FILENAMES["datadir"] + "font/");
    update_pathname("rawdir", FILENAMES["datadir"] + "raw/");
    update_pathname("jsondir", FILENAMES["datadir"] + "core/");
    update_pathname("moddir", FILENAMES["datadir"] + "mods/");
    update_pathname("recycledir", FILENAMES["datadir"] + "recycling/");
    update_pathname("namesdir", FILENAMES["datadir"] + "names/");
    update_pathname("titledir", FILENAMES["datadir"] + "title/");
    update_pathname("motddir", FILENAMES["datadir"] + "motd/");
    update_pathname("creditsdir", FILENAMES["datadir"] + "credits/");
    update_pathname("sounddir", FILENAMES["datadir"] + "sound");

    // Shared files
    update_pathname("autoexeclua", FILENAMES["luadir"] + "autoexec.lua");
    update_pathname("class_defslua", FILENAMES["luadir"] + "class_definitions.lua");
    update_pathname("title", FILENAMES["titledir"] + "en.title");
    update_pathname("motd", FILENAMES["motddir"] + "en.motd");
    update_pathname("credits", FILENAMES["creditsdir"] + "en.credits");
    update_pathname("names", FILENAMES["namesdir"] + "en.json");
    update_pathname("colors", FILENAMES["rawdir"] + "colors.json");
    update_pathname("keybindings", FILENAMES["rawdir"] + "keybindings.json");
    update_pathname("keybindings_vehicle", FILENAMES["rawdir"] + "keybindings/vehicle.json");
    update_pathname("legacy_fontdata", FILENAMES["datadir"] + "fontdata.json");
    update_pathname("sokoban", FILENAMES["rawdir"] + "sokoban.txt");
    update_pathname("defaulttilejson", FILENAMES["gfx"] + "tile_config.json");
    update_pathname("defaulttilepng", FILENAMES["gfx"] + "tinytile.png");
    update_pathname("mods-dev-default", FILENAMES["moddir"] + "default.json");
    update_pathname("mods-user-default", FILENAMES["moddir"] + "user-default-mods.json");
    update_pathname("mods-replacements", FILENAMES["moddir"] + "replacements.json");
    update_pathname("defaultsounddir", FILENAMES["datadir"] + "sound");
}

void PATH_INFO::update_config_dir()
{
    update_pathname("options", FILENAMES["config_dir"] + "options.json");
    update_pathname("keymap", FILENAMES["config_dir"] + "keymap.txt");
    update_pathname("debug", FILENAMES["config_dir"] + "debug.log");
    update_pathname("fontlist", FILENAMES["config_dir"] + "fontlist.txt");
    update_pathname("fontdata", FILENAMES["config_dir"] + "fonts.json");
    update_pathname("autopickup", FILENAMES["config_dir"] + "auto_pickup.json");
    update_pathname("safemode", FILENAMES["config_dir"] + "safemode.json");
    update_pathname("custom_colors", FILENAMES["config_dir"] + "custom_colors.json");
}

void PATH_INFO::set_standard_filenames(void)
{
    // Special: data_dir lua_dir and gfx_dir
    if (!FILENAMES["base_path"].empty()) {
#ifdef DATA_DIR_PREFIX
        update_pathname("datadir", FILENAMES["base_path"] + "share/cataclysm-dda/");
        update_pathname("gfxdir", FILENAMES["datadir"] + "gfx/");
        update_pathname("luadir", FILENAMES["datadir"] + "lua/");
#else
        update_pathname("datadir", FILENAMES["base_path"] + "data/");
        update_pathname("gfxdir", FILENAMES["base_path"] + "gfx/");
        update_pathname("luadir", FILENAMES["base_path"] +"lua/");
#endif
    } else {
        update_pathname("datadir", "data/");
        update_pathname("gfxdir", "gfx/");
        update_pathname("luadir", "lua/");
    }

    // Shared dirs
    update_pathname("autoexeclua", FILENAMES["luadir"] + "autoexec.lua");
    update_pathname("class_defslua", FILENAMES["luadir"] + "class_definitions.lua");
    update_pathname("fontdir", FILENAMES["datadir"] + "font/");
    update_pathname("rawdir", FILENAMES["datadir"] + "raw/");
    update_pathname("jsondir", FILENAMES["datadir"] + "core/");
    update_pathname("moddir", FILENAMES["datadir"] + "mods/");
    update_pathname("recycledir", FILENAMES["datadir"] + "recycling/");
    update_pathname("namesdir", FILENAMES["datadir"] + "names/");
    update_pathname("titledir", FILENAMES["datadir"] + "title/");
    update_pathname("motddir", FILENAMES["datadir"] + "motd/");
    update_pathname("creditsdir", FILENAMES["datadir"] + "credits/");
    update_pathname("color_templates", FILENAMES["rawdir"] + "color_templates/");
    update_pathname("sounddir", FILENAMES["datadir"] + "sound");

    // Shared files
    update_pathname("title", FILENAMES["titledir"] + "en.title");
    update_pathname("motd", FILENAMES["motddir"] + "en.motd");
    update_pathname("credits", FILENAMES["creditsdir"] + "en.credits");
    update_pathname("names", FILENAMES["namesdir"] + "en.json");
    update_pathname("colors", FILENAMES["rawdir"] + "colors.json");
    update_pathname("keybindings", FILENAMES["rawdir"] + "keybindings.json");
    update_pathname("keybindings_vehicle", FILENAMES["rawdir"] + "keybindings/vehicle.json");
    update_pathname("sokoban", FILENAMES["rawdir"] + "sokoban.txt");
    update_pathname("defaulttilejson", FILENAMES["gfx"] + "tile_config.json");
    update_pathname("defaulttilepng", FILENAMES["gfx"] + "tinytile.png");
    update_pathname("mods-dev-default", FILENAMES["moddir"] + "default.json");
    update_pathname("mods-user-default", FILENAMES["moddir"] + "user-default-mods.json");
    update_pathname("mods-replacements", FILENAMES["moddir"] + "replacements.json");
    update_pathname("defaultsounddir", FILENAMES["datadir"] + "sound");

    update_pathname("savedir", FILENAMES["user_dir"] + "save/");
    update_pathname("memorialdir", FILENAMES["user_dir"] + "memorial/");
    update_pathname("templatedir", FILENAMES["user_dir"] + "templates/");
#ifdef USE_XDG_DIR
    const char *user_dir;
    std::string dir;
    if ( (user_dir = getenv("XDG_CONFIG_HOME")) ) {
        dir = std::string(user_dir) + "/cataclysm-dda/";
    } else {
        user_dir = getenv("HOME");
        dir = std::string(user_dir) + "/.config/cataclysm-dda/";
    }
    update_pathname("config_dir", dir);
#else
    update_pathname("config_dir", FILENAMES["user_dir"] + "config/");
#endif
    update_pathname("graveyarddir", FILENAMES["user_dir"] + "graveyard/");

    update_pathname("options", FILENAMES["config_dir"] + "options.json");
    update_pathname("keymap", FILENAMES["config_dir"] + "keymap.txt");
    update_pathname("user_keybindings", FILENAMES["config_dir"] + "keybindings.json");
    update_pathname("debug", FILENAMES["config_dir"] + "debug.log");
    update_pathname("fontlist", FILENAMES["config_dir"] + "fontlist.txt");
    update_pathname("fontdata", FILENAMES["config_dir"] + "fonts.json");
    update_pathname("autopickup", FILENAMES["config_dir"] + "auto_pickup.json");
    update_pathname("safemode", FILENAMES["config_dir"] + "safemode.json");
    update_pathname("custom_colors", FILENAMES["config_dir"] + "custom_colors.json");
    update_pathname("worldoptions", "worldoptions.json");

    // Needed to move files from these legacy locations to the new config directory.
    update_pathname("legacy_options", "data/options.txt");
    update_pathname("legacy_options2", FILENAMES["config_dir"] + "options.txt");
    update_pathname("legacy_keymap", "data/keymap.txt");
    update_pathname("legacy_autopickup", "data/auto_pickup.txt");
    update_pathname("legacy_autopickup2", FILENAMES["config_dir"] + "auto_pickup.txt");
    update_pathname("legacy_fontdata", FILENAMES["datadir"] + "fontdata.json");
    update_pathname("legacy_worldoptions", "worldoptions.txt");
#ifdef TILES
    // Default tileset config file.
    update_pathname("tileset-conf", "tileset.txt");
#endif
#ifdef SDL_SOUND
    // Default soundpack config file.
    update_pathname("soundpack-conf", "soundpack.txt");
#endif
}

std::string PATH_INFO::find_translated_file( const std::string &pathid,
        const std::string &extension, const std::string &fallbackid )
{
    const std::string base_path = FILENAMES[pathid];

#if defined LOCALIZE && ! defined __CYGWIN__
    std::string local_path_1; // complete locale: en_NZ
    std::string local_path_2; // only the first part: en
    std::string loc_name;
    if( get_option<std::string>( "USE_LANG" ).empty() ) {
        const char *v = setlocale( LC_ALL, NULL );
        if( v != NULL ) {
            loc_name = v;
        }
    } else {
        loc_name = get_option<std::string>( "USE_LANG" );
    }
    if( loc_name == "C" ) {
        loc_name = "en";
    }
    if( !loc_name.empty() ) {
        const size_t dotpos = loc_name.find( '.' );
        if( dotpos != std::string::npos ) {
            loc_name.erase( dotpos );
        }
        const std::string local_path_1 = base_path + loc_name + extension;
        if( file_exist( local_path_1 ) ) {
            return local_path_1;
        }
        const size_t p = loc_name.find( '_' );
        if( p != std::string::npos ) {
            const std::string local_path_2 = base_path + loc_name.substr( 0, p ) + extension;
            if( file_exist( local_path_2 ) ) {
                return local_path_2;
            }
        }
    }
#endif
    (void) extension;
    return FILENAMES[fallbackid];
}
