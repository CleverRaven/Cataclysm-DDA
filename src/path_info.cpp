#include "path_info.h"

#include <clocale>
#include <cstdlib>
#include <utility>

#include "filesystem.h"
#include "options.h"

#if defined(_WIN32)
#include <windows.h>
#endif

/**
 * Return a locale specific path, or if there is no path for the current
 * locale, return the default path.
 * @param path The local path is based on that value.
 * @param extension File name extension, is automatically added to the path
 * of the translated file. Can be empty, but must otherwise include the
 * initial '.', e.g. ".json"
 * @param fallback The path of the fallback filename.
 * It is used if no translated file can be found.
 */
static std::string find_translated_file( const std::string &path, const std::string &extension,
        const std::string &fallback );

static void update_datadir();
static void update_config_dir();
static void update_pathname( const std::string &name, const std::string &path );

static std::string motd_value;

/** Map where we store filenames */
std::map<std::string, std::string> FILENAMES;

void PATH_INFO::init_base_path( std::string path )
{
    if( !path.empty() ) {
        const char ch = path.back();
        if( ch != '/' && ch != '\\' ) {
            path.push_back( '/' );
        }
    }

    FILENAMES["base_path"] = path;
}

void PATH_INFO::init_user_dir( const char *ud )
{
    std::string dir = std::string( ud );

    if( dir.empty() ) {
        const char *user_dir;
#if defined(_WIN32)
        user_dir = getenv( "LOCALAPPDATA" );
        // On Windows userdir without dot
        dir = std::string( user_dir ) + "/cataclysm-dda/";
#elif defined(MACOSX)
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/Library/Application Support/Cataclysm/";
#elif defined(USE_XDG_DIR)
        if( ( user_dir = getenv( "XDG_DATA_HOME" ) ) ) {
            dir = std::string( user_dir ) + "/cataclysm-dda/";
        } else {
            user_dir = getenv( "HOME" );
            dir = std::string( user_dir ) + "/.local/share/cataclysm-dda/";
        }
#else
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/.cataclysm-dda/";
#endif
    }

    FILENAMES["user_dir"] = dir;
}

void update_pathname( const std::string &name, const std::string &path )
{
    const std::map<std::string, std::string>::iterator iter = FILENAMES.find( name );
    if( iter != FILENAMES.end() ) {
        FILENAMES[name] = path;
    } else {
        FILENAMES.insert( std::pair<std::string, std::string>( name, path ) );
    }
}

void update_datadir()
{
    // Shared dirs
    update_pathname( "gfxdir", FILENAMES["datadir"] + "gfx/" );
    update_pathname( "fontdir", FILENAMES["datadir"] + "font/" );
    update_pathname( "rawdir", FILENAMES["datadir"] + "raw/" );
    update_pathname( "jsondir", FILENAMES["datadir"] + "core/" );
    update_pathname( "moddir", FILENAMES["datadir"] + "mods/" );
    update_pathname( "namesdir", FILENAMES["datadir"] + "names/" );
    update_pathname( "titledir", FILENAMES["datadir"] + "title/" );
    update_pathname( "motddir", FILENAMES["datadir"] + "motd/" );
    update_pathname( "creditsdir", FILENAMES["datadir"] + "credits/" );
    update_pathname( "data_sound", FILENAMES["datadir"] + "sound" );
    update_pathname( "helpdir", FILENAMES["datadir"] + "help/" );

    // Shared files
    update_pathname( "title", FILENAMES["titledir"] + "en.title" );
    update_pathname( "halloween", FILENAMES["titledir"] + "en.halloween" );
    motd_value = FILENAMES["datadir"] + "motd/" + "en.motd";
    update_pathname( "credits", FILENAMES["creditsdir"] + "en.credits" );
    update_pathname( "names", FILENAMES["namesdir"] + "en.json" );
}

void update_config_dir()
{
    update_pathname( "options", FILENAMES["config_dir"] + "options.json" );
    update_pathname( "keymap", FILENAMES["config_dir"] + "keymap.txt" );
    update_pathname( "autopickup", FILENAMES["config_dir"] + "auto_pickup.json" );
}

void PATH_INFO::set_standard_filenames()
{
    // Special: data_dir and gfx_dir
    if( !FILENAMES["base_path"].empty() ) {
#if defined(DATA_DIR_PREFIX)
        update_pathname( "datadir", FILENAMES["base_path"] + "share/cataclysm-dda/" );
        update_pathname( "gfxdir", FILENAMES["datadir"] + "gfx/" );
#else
        update_pathname( "datadir", FILENAMES["base_path"] + "data/" );
        update_pathname( "gfxdir", FILENAMES["base_path"] + "gfx/" );
#endif
    } else {
        update_pathname( "datadir", "data/" );
        update_pathname( "gfxdir", "gfx/" );
    }

    // Shared dirs
    update_pathname( "fontdir", FILENAMES["datadir"] + "font/" );
    update_pathname( "rawdir", FILENAMES["datadir"] + "raw/" );
    update_pathname( "jsondir", FILENAMES["datadir"] + "core/" );
    update_pathname( "moddir", FILENAMES["datadir"] + "mods/" );
    update_pathname( "namesdir", FILENAMES["datadir"] + "names/" );
    update_pathname( "titledir", FILENAMES["datadir"] + "title/" );
    update_pathname( "motddir", FILENAMES["datadir"] + "motd/" );
    update_pathname( "creditsdir", FILENAMES["datadir"] + "credits/" );
    update_pathname( "data_sound", FILENAMES["datadir"] + "sound" );
    update_pathname( "helpdir", FILENAMES["datadir"] + "help/" );

    // Shared files
    update_pathname( "title", FILENAMES["titledir"] + "en.title" );
    update_pathname( "halloween", FILENAMES["titledir"] + "en.halloween" );
    motd_value = FILENAMES["datadir"] + "motd/" + "en.motd";
    update_pathname( "credits", FILENAMES["creditsdir"] + "en.credits" );
    update_pathname( "names", FILENAMES["namesdir"] + "en.json" );

    update_pathname( "savedir", FILENAMES["user_dir"] + "save/" );
    update_pathname( "memorialdir", FILENAMES["user_dir"] + "memorial/" );

#if defined(USE_XDG_DIR)
    const char *user_dir;
    std::string dir;
    if( ( user_dir = getenv( "XDG_CONFIG_HOME" ) ) ) {
        dir = std::string( user_dir ) + "/cataclysm-dda/";
    } else {
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/.config/cataclysm-dda/";
    }
    update_pathname( "config_dir", dir );
#else
    update_pathname( "config_dir", FILENAMES["user_dir"] + "config/" );
#endif
    update_pathname( "options", FILENAMES["config_dir"] + "options.json" );
    update_pathname( "keymap", FILENAMES["config_dir"] + "keymap.txt" );
    update_pathname( "autopickup", FILENAMES["config_dir"] + "auto_pickup.json" );
}

std::string find_translated_file( const std::string &path, const std::string &extension,
                                  const std::string &fallback )
{
    const std::string base_path = path;

#if defined(LOCALIZE) && !defined(__CYGWIN__)
    std::string loc_name;
    if( get_option<std::string>( "USE_LANG" ).empty() ) {
#if defined(_WIN32)
        loc_name = getLangFromLCID( GetUserDefaultLCID() );
        if( !loc_name.empty() ) {
            const std::string local_path = base_path + loc_name + extension;
            if( file_exist( local_path ) ) {
                return local_path;
            }
        }
#endif

        const char *v = setlocale( LC_ALL, nullptr );
        if( v != nullptr ) {
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
        // complete locale: en_NZ
        const std::string local_path = base_path + loc_name + extension;
        if( file_exist( local_path ) ) {
            return local_path;
        }
        const size_t p = loc_name.find( '_' );
        if( p != std::string::npos ) {
            // only the first part: en
            const std::string local_path = base_path + loc_name.substr( 0, p ) + extension;
            if( file_exist( local_path ) ) {
                return local_path;
            }
        }
    }
#endif
    ( void ) extension;
    return fallback;
}
std::string PATH_INFO::autopickup()
{
    return FILENAMES["autopickup"];
}
std::string PATH_INFO::base_colors()
{
    return FILENAMES["config_dir"] + "base_colors.json";
}
std::string PATH_INFO::base_path()
{
    return FILENAMES["base_path"];
}
std::string PATH_INFO::colors()
{
    return FILENAMES["rawdir"] + "colors.json";
}
std::string PATH_INFO::color_templates()
{
    return FILENAMES["rawdir"] + "color_templates/";
}
std::string PATH_INFO::config_dir()
{
    return FILENAMES["config_dir"];
}
std::string PATH_INFO::custom_colors()
{
    return FILENAMES["config_dir"] + "custom_colors.json";
}
std::string PATH_INFO::datadir()
{
    return FILENAMES["datadir"];
}
std::string PATH_INFO::debug()
{
    return FILENAMES["config_dir"] + "debug.log";
}
std::string PATH_INFO::defaultsounddir()
{
    return FILENAMES["datadir"] + "sound";
}
std::string PATH_INFO::defaulttilejson()
{
    return "tile_config.json";
}
std::string PATH_INFO::defaulttilepng()
{
    return "tinytile.png";
}
std::string PATH_INFO::fontdata()
{
    return FILENAMES["config_dir"] + "fonts.json";
}
std::string PATH_INFO::fontdir()
{
    return FILENAMES["fontdir"];
}
std::string PATH_INFO::fontlist()
{
    return FILENAMES["config_dir"] + "fontlist.txt";
}
std::string PATH_INFO::graveyarddir()
{
    return FILENAMES["user_dir"] + "graveyard/";
}
std::string PATH_INFO::help()
{
    return FILENAMES["helpdir"] + "texts.json";
}
std::string PATH_INFO::keybindings()
{
    return FILENAMES["rawdir"] + "keybindings.json";
}
std::string PATH_INFO::keybindings_vehicle()
{
    return FILENAMES["rawdir"] + "keybindings/vehicle.json";
}
std::string PATH_INFO::keymap()
{
    return FILENAMES["keymap"];
}
std::string PATH_INFO::lastworld()
{
    return FILENAMES["config_dir"] + "lastworld.json";
}
std::string PATH_INFO::legacy_autopickup()
{
    return "data/auto_pickup.txt";
}
std::string PATH_INFO::legacy_autopickup2()
{
    return FILENAMES["config_dir"] + "auto_pickup.txt";
}
std::string PATH_INFO::legacy_fontdata()
{
    return FILENAMES["datadir"] + "fontdata.json";
}
std::string PATH_INFO::legacy_keymap()
{
    return "data/keymap.txt";
}
std::string PATH_INFO::legacy_options()
{
    return "data/options.txt";
}
std::string PATH_INFO::legacy_options2()
{
    return FILENAMES["config_dir"] + "options.txt";
}
std::string PATH_INFO::legacy_worldoptions()
{
    return "worldoptions.txt";
}
std::string PATH_INFO::memorialdir()
{
    return FILENAMES["memorialdir"];
}
std::string PATH_INFO::jsondir()
{
    return FILENAMES["jsondir"];
}
std::string PATH_INFO::moddir()
{
    return FILENAMES["moddir"];
}
std::string PATH_INFO::options()
{
    return FILENAMES["options"];
}
std::string PATH_INFO::panel_options()
{
    return FILENAMES["config_dir"] + "panel_options.json";
}
std::string PATH_INFO::safemode()
{
    return FILENAMES["config_dir"] + "safemode.json";
}
std::string PATH_INFO::savedir()
{
    return FILENAMES["savedir"];
}
std::string PATH_INFO::sokoban()
{
    return FILENAMES["rawdir"] + "sokoban.txt";
}
std::string PATH_INFO::templatedir()
{
    return FILENAMES["user_dir"] + "templates/";
}
std::string PATH_INFO::user_dir()
{
    return FILENAMES["user_dir"];
}
std::string PATH_INFO::user_gfx()
{
    return FILENAMES["user_dir"] + "gfx/";
}
std::string PATH_INFO::user_keybindings()
{
    return FILENAMES["config_dir"] + "keybindings.json";
}
std::string PATH_INFO::user_moddir()
{
    return FILENAMES["user_dir"] + "mods/";
}
std::string PATH_INFO::user_sound()
{
    return FILENAMES["user_dir"] + "sound/";
}
std::string PATH_INFO::worldoptions()
{
    return "worldoptions.json";
}
std::string PATH_INFO::crash()
{
    return FILENAMES["config_dir"] + "crash.log";
}
std::string PATH_INFO::tileset_conf()
{
    return "tileset.txt";
}
std::string PATH_INFO::mods_replacements()
{
    return FILENAMES["moddir"] + "replacements.json";
}
std::string PATH_INFO::mods_dev_default()
{
    return FILENAMES["moddir"] + "default.json";
}
std::string PATH_INFO::mods_user_default()
{
    return FILENAMES["config_dir"] + "user-default-mods.json";
}
std::string PATH_INFO::soundpack_conf()
{
    return "soundpack.txt";
}
std::string PATH_INFO::gfxdir()
{
    return FILENAMES["gfxdir"];
}
std::string PATH_INFO::data_sound()
{
    return FILENAMES["data_sound"];
}

std::string PATH_INFO::credits()
{
    return find_translated_file( FILENAMES["creditsdir"], ".credits", FILENAMES["credits"] );
}

std::string PATH_INFO::motd()
{
    return find_translated_file( FILENAMES["motddir"], ".motd", motd_value );
}

std::string PATH_INFO::title( const bool halloween_theme )
{
    return find_translated_file( FILENAMES["titledir"], halloween_theme ? ".halloween" : ".title",
                                 halloween_theme ? FILENAMES["halloween"] : FILENAMES["title"] );
}

std::string PATH_INFO::names()
{
    return find_translated_file( FILENAMES["namesdir"], ".json", FILENAMES["names"] );
}

void PATH_INFO::set_datadir( const std::string &datadir )
{
    update_pathname( "datadir", datadir );
    update_datadir();
}

void PATH_INFO::set_config_dir( const std::string &config_dir )
{
    update_pathname( "config_dir", config_dir );
    update_config_dir();
}

void PATH_INFO::set_savedir( const std::string &savedir )
{
    update_pathname( "savedir", savedir );
}

void PATH_INFO::set_memorialdir( const std::string &memorialdir )
{
    update_pathname( "memorialdir", memorialdir );
}

void PATH_INFO::set_options( const std::string &options )
{
    update_pathname( "options", options );
}

void PATH_INFO::set_keymap( const std::string &keymap )
{
    update_pathname( "keymap", keymap );
}

void PATH_INFO::set_autopickup( const std::string &autopickup )
{
    update_pathname( "autopickup", autopickup );
}

void PATH_INFO::set_motd( const std::string &motd )
{
    motd_value = motd;
}
