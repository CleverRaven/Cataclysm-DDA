#include "path_info.h"

#include <clocale>
#include <cstdlib>

#include "filesystem.h"
#include "options.h"
#include "translations.h"

#if (defined _WIN32 || defined WINDOW)
#include <windows.h>
#endif

void Path::initBasePath( std::string path )
{
    if( !path.empty() ) {
        const char ch = path.at( path.length() - 1 );
        if( ch != '/' && ch != '\\' ) {
            path.push_back( '/' );
        }
    }

    //FILENAMES.insert(std::pair<std::string,std::string>("base_path", path));
    FILENAMES["base_path"] = path;
}

void Path::initUserDirectory( std::string path )
{
    std::string dir = std::string( path );

    if( dir.empty() ) {
        const char *user_dir;
#if (defined _WIN32 || defined WINDOW)
        user_dir = getenv( "LOCALAPPDATA" );
        // On Windows userdir without dot
        dir = std::string( user_dir ) + "/cataclysm-dda/";
#elif defined MACOSX
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/Library/Application Support/Cataclysm/";
#elif (defined USE_XDG_DIR)
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

void Path::setStandardFilenames( )
{
    // Special: data_dir and gfx_dir
    if( !FILENAMES["base_path"].empty() ) {
#ifdef DATA_DIR_PREFIX
        updatePathName( "datadir", FILENAMES["base_path"] + "share/cataclysm-dda/" );
        updatePathName( "gfxdir", FILENAMES["datadir"] + "gfx/" );
#else
        updatePathName( "datadir", FILENAMES["base_path"] + "data/" );
        updatePathName( "gfxdir", FILENAMES["base_path"] + "gfx/" );
#endif
    } else {
        updatePathName( "datadir", "data/" );
        updatePathName( "gfxdir", "gfx/" );
    }

    // Shared dirs
    updatePathName( "fontdir", FILENAMES["datadir"] + "font/" );
    updatePathName( "rawdir", FILENAMES["datadir"] + "raw/" );
    updatePathName( "jsondir", FILENAMES["datadir"] + "core/" );
    updatePathName( "moddir", FILENAMES["datadir"] + "mods/" );
    updatePathName( "namesdir", FILENAMES["datadir"] + "names/" );
    updatePathName( "titledir", FILENAMES["datadir"] + "title/" );
    updatePathName( "motddir", FILENAMES["datadir"] + "motd/" );
    updatePathName( "creditsdir", FILENAMES["datadir"] + "credits/" );
    updatePathName( "color_templates", FILENAMES["rawdir"] + "color_templates/" );
    updatePathName( "data_sound", FILENAMES["datadir"] + "sound" );
    updatePathName( "helpdir", FILENAMES["datadir"] + "help/" );

    // Shared files
    updatePathName( "title", FILENAMES["titledir"] + "en.title" );
    updatePathName( "halloween", FILENAMES["titledir"] + "en.halloween" );
    updatePathName( "motd", FILENAMES["motddir"] + "en.motd" );
    updatePathName( "credits", FILENAMES["creditsdir"] + "en.credits" );
    updatePathName( "names", FILENAMES["namesdir"] + "en.json" );
    updatePathName( "colors", FILENAMES["rawdir"] + "colors.json" );
    updatePathName( "keybindings", FILENAMES["rawdir"] + "keybindings.json" );
    updatePathName( "keybindings_vehicle", FILENAMES["rawdir"] + "keybindings/vehicle.json" );
    updatePathName( "sokoban", FILENAMES["rawdir"] + "sokoban.txt" );
    updatePathName( "defaulttilejson", FILENAMES["gfx"] + "tile_config.json" );
    updatePathName( "defaulttilepng", FILENAMES["gfx"] + "tinytile.png" );
    updatePathName( "mods-dev-default", FILENAMES["moddir"] + "default.json" );
    updatePathName( "mods-replacements", FILENAMES["moddir"] + "replacements.json" );
    updatePathName( "defaultsounddir", FILENAMES["datadir"] + "sound" );
    updatePathName( "help", FILENAMES["helpdir"] + "texts.json" );

    updatePathName( "savedir", FILENAMES["user_dir"] + "save/" );
    updatePathName( "memorialdir", FILENAMES["user_dir"] + "memorial/" );
    updatePathName( "templatedir", FILENAMES["user_dir"] + "templates/" );
    updatePathName( "user_sound", FILENAMES["user_dir"] + "sound/" );
#ifdef USE_XDG_DIR
    const char *user_dir;
    std::string dir;
    if( ( user_dir = getenv( "XDG_CONFIG_HOME" ) ) ) {
        dir = std::string( user_dir ) + "/cataclysm-dda/";
    } else {
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/.config/cataclysm-dda/";
    }
    updatePathName( "config_dir", dir );
#else
    updatePathName( "config_dir", FILENAMES["user_dir"] + "config/" );
#endif
    updatePathName( "graveyarddir", FILENAMES["user_dir"] + "graveyard/" );

    updatePathName( "options", FILENAMES["config_dir"] + "options.json" );
    updatePathName( "panel_options", FILENAMES["config_dir"] + "panel_options.json" );
    updatePathName( "keymap", FILENAMES["config_dir"] + "keymap.txt" );
    updatePathName( "user_keybindings", FILENAMES["config_dir"] + "keybindings.json" );
    updatePathName( "debug", FILENAMES["config_dir"] + "debug.log" );
    updatePathName( "crash", FILENAMES["config_dir"] + "crash.log" );
    updatePathName( "fontlist", FILENAMES["config_dir"] + "fontlist.txt" );
    updatePathName( "fontdata", FILENAMES["config_dir"] + "fonts.json" );
    updatePathName( "autopickup", FILENAMES["config_dir"] + "auto_pickup.json" );
    updatePathName( "safemode", FILENAMES["config_dir"] + "safemode.json" );
    updatePathName( "base_colors", FILENAMES["config_dir"] + "base_colors.json" );
    updatePathName( "custom_colors", FILENAMES["config_dir"] + "custom_colors.json" );
    updatePathName( "mods-user-default", FILENAMES["config_dir"] + "user-default-mods.json" );
    updatePathName( "lastworld", FILENAMES["config_dir"] + "lastworld.json" );
    updatePathName( "user_moddir", FILENAMES["user_dir"] + "mods/" );
    updatePathName( "worldoptions", "worldoptions.json" );

    // Needed to move files from these legacy locations to the new config directory.
    updatePathName( "legacy_options", "data/options.txt" );
    updatePathName( "legacy_options2", FILENAMES["config_dir"] + "options.txt" );
    updatePathName( "legacy_keymap", "data/keymap.txt" );
    updatePathName( "legacy_autopickup", "data/auto_pickup.txt" );
    updatePathName( "legacy_autopickup2", FILENAMES["config_dir"] + "auto_pickup.txt" );
    updatePathName( "legacy_fontdata", FILENAMES["datadir"] + "fontdata.json" );
    updatePathName( "legacy_worldoptions", "worldoptions.txt" );
#ifdef TILES
    // Default tileset config file.
    updatePathName( "tileset-conf", "tileset.txt" );
#endif
#ifdef SDL_SOUND
    // Default soundpack config file.
    updatePathName( "soundpack-conf", "soundpack.txt" );
#endif
}

void Path::updateDataDirectory( )
{
    // Shared dirs
    updatePathName( "gfxdir", FILENAMES["datadir"] + "gfx/" );
    updatePathName( "fontdir", FILENAMES["datadir"] + "font/" );
    updatePathName( "rawdir", FILENAMES["datadir"] + "raw/" );
    updatePathName( "jsondir", FILENAMES["datadir"] + "core/" );
    updatePathName( "moddir", FILENAMES["datadir"] + "mods/" );
    updatePathName( "recycledir", FILENAMES["datadir"] + "recycling/" );
    updatePathName( "namesdir", FILENAMES["datadir"] + "names/" );
    updatePathName( "titledir", FILENAMES["datadir"] + "title/" );
    updatePathName( "motddir", FILENAMES["datadir"] + "motd/" );
    updatePathName( "creditsdir", FILENAMES["datadir"] + "credits/" );
    updatePathName( "data_sound", FILENAMES["datadir"] + "sound" );
    updatePathName( "helpdir", FILENAMES["datadir"] + "help/" );

    // Shared files
    updatePathName( "title", FILENAMES["titledir"] + "en.title" );
    updatePathName( "halloween", FILENAMES["titledir"] + "en.halloween" );
    updatePathName( "motd", FILENAMES["motddir"] + "en.motd" );
    updatePathName( "credits", FILENAMES["creditsdir"] + "en.credits" );
    updatePathName( "names", FILENAMES["namesdir"] + "en.json" );
    updatePathName( "colors", FILENAMES["rawdir"] + "colors.json" );
    updatePathName( "keybindings", FILENAMES["rawdir"] + "keybindings.json" );
    updatePathName( "keybindings_vehicle", FILENAMES["rawdir"] + "keybindings/vehicle.json" );
    updatePathName( "legacy_fontdata", FILENAMES["datadir"] + "fontdata.json" );
    updatePathName( "sokoban", FILENAMES["rawdir"] + "sokoban.txt" );
    updatePathName( "defaulttilejson", FILENAMES["gfx"] + "tile_config.json" );
    updatePathName( "defaulttilepng", FILENAMES["gfx"] + "tinytile.png" );
    updatePathName( "mods-dev-default", FILENAMES["moddir"] + "default.json" );
    updatePathName( "mods-replacements", FILENAMES["moddir"] + "replacements.json" );
    updatePathName( "defaultsounddir", FILENAMES["datadir"] + "sound" );
    updatePathName( "help", FILENAMES["helpdir"] + "texts.json" );
}

void Path::updateConfigurationDirectory( )
{
    updatePathName( "options", FILENAMES["config_dir"] + "options.json" );
    updatePathName( "panel_options", FILENAMES["config_dir"] + "panel_options.json" );
    updatePathName( "keymap", FILENAMES["config_dir"] + "keymap.txt" );
    updatePathName( "debug", FILENAMES["config_dir"] + "debug.log" );
    updatePathName( "crash", FILENAMES["config_dir"] + "crash.log" );
    updatePathName( "fontlist", FILENAMES["config_dir"] + "fontlist.txt" );
    updatePathName( "fontdata", FILENAMES["config_dir"] + "fonts.json" );
    updatePathName( "autopickup", FILENAMES["config_dir"] + "auto_pickup.json" );
    updatePathName( "safemode", FILENAMES["config_dir"] + "safemode.json" );
    updatePathName( "base_colors", FILENAMES["config_dir"] + "base_colors.json" );
    updatePathName( "custom_colors", FILENAMES["config_dir"] + "custom_colors.json" );
    updatePathName( "mods-user-default", FILENAMES["config_dir"] + "user-default-mods.json" );
    updatePathName( "lastworld", FILENAMES["config_dir"] + "lastworld.json" );
}

void Path::updatePathName( const std::string &name, const std::string &path )
{
    const std::map<std::string, std::string>::iterator iter = FILENAMES.find( name );
    if( iter != FILENAMES.end() ) {
        FILENAMES[name] = path;
    } else {
        FILENAMES.insert( std::pair<std::string, std::string>( name, path ) );
    }
}

/** Map where we store filenames */
std::map<std::string, std::string> FILENAMES;

std::string PATH_INFO::find_translated_file( const std::string &pathid,
        const std::string &extension, const std::string &fallbackid )
{
    const std::string base_path = FILENAMES[pathid];

#if defined LOCALIZE && ! defined __CYGWIN__
    std::string loc_name;
    if( get_option<std::string>( "USE_LANG" ).empty() ) {
#if (defined _WIN32 || defined WINDOWS)
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
    return FILENAMES[fallbackid];
}
