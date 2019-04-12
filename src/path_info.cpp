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
        updatePathname( "datadir", FILENAMES["base_path"] + "share/cataclysm-dda/" );
        updatePathname( "gfxdir", FILENAMES["datadir"] + "gfx/" );
#else
        updatePathname( "datadir", FILENAMES[ "base_path" ] + "data/" );
        updatePathname( "gfxdir", FILENAMES[ "base_path" ] + "gfx/" );
#endif
    } else {
        updatePathname( "datadir", "data/" );
        updatePathname( "gfxdir", "gfx/" );
    }

    // Shared dirs
    updatePathname( "fontdir", FILENAMES[ "datadir" ] + "font/" );
    updatePathname( "rawdir", FILENAMES[ "datadir" ] + "raw/" );
    updatePathname( "jsondir", FILENAMES[ "datadir" ] + "core/" );
    updatePathname( "moddir", FILENAMES[ "datadir" ] + "mods/" );
    updatePathname( "namesdir", FILENAMES[ "datadir" ] + "names/" );
    updatePathname( "titledir", FILENAMES[ "datadir" ] + "title/" );
    updatePathname( "motddir", FILENAMES[ "datadir" ] + "motd/" );
    updatePathname( "creditsdir", FILENAMES[ "datadir" ] + "credits/" );
    updatePathname( "color_templates", FILENAMES[ "rawdir" ] + "color_templates/" );
    updatePathname( "data_sound", FILENAMES[ "datadir" ] + "sound" );
    updatePathname( "helpdir", FILENAMES[ "datadir" ] + "help/" );

    // Shared files
    updatePathname( "title", FILENAMES[ "titledir" ] + "en.title" );
    updatePathname( "halloween", FILENAMES[ "titledir" ] + "en.halloween" );
    updatePathname( "motd", FILENAMES[ "motddir" ] + "en.motd" );
    updatePathname( "credits", FILENAMES[ "creditsdir" ] + "en.credits" );
    updatePathname( "names", FILENAMES[ "namesdir" ] + "en.json" );
    updatePathname( "colors", FILENAMES[ "rawdir" ] + "colors.json" );
    updatePathname( "keybindings", FILENAMES[ "rawdir" ] + "keybindings.json" );
    updatePathname( "keybindings_vehicle", FILENAMES[ "rawdir" ] + "keybindings/vehicle.json" );
    updatePathname( "sokoban", FILENAMES[ "rawdir" ] + "sokoban.txt" );
    updatePathname( "defaulttilejson", FILENAMES[ "gfx" ] + "tile_config.json" );
    updatePathname( "defaulttilepng", FILENAMES[ "gfx" ] + "tinytile.png" );
    updatePathname( "mods-dev-default", FILENAMES[ "moddir" ] + "default.json" );
    updatePathname( "mods-replacements", FILENAMES[ "moddir" ] + "replacements.json" );
    updatePathname( "defaultsounddir", FILENAMES[ "datadir" ] + "sound" );
    updatePathname( "help", FILENAMES[ "helpdir" ] + "texts.json" );

    updatePathname( "savedir", FILENAMES[ "user_dir" ] + "save/" );
    updatePathname( "memorialdir", FILENAMES[ "user_dir" ] + "memorial/" );
    updatePathname( "templatedir", FILENAMES[ "user_dir" ] + "templates/" );
    updatePathname( "user_sound", FILENAMES[ "user_dir" ] + "sound/" );

#ifdef USE_XDG_DIR

    const char *user_dir;
    std::string dir;
    if( ( user_dir = getenv( "XDG_CONFIG_HOME" ) ) ) {
        dir = std::string( user_dir ) + "/cataclysm-dda/";
    } else {
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/.config/cataclysm-dda/";
    }
    updatePathname( "config_dir", dir );
#else
    updatePathname( "config_dir", FILENAMES[ "user_dir" ] + "config/" );
#endif
    updatePathname( "graveyarddir", FILENAMES[ "user_dir" ] + "graveyard/" );

    updatePathname( "options", FILENAMES[ "config_dir" ] + "options.json" );
    updatePathname( "panel_options", FILENAMES[ "config_dir" ] + "panel_options.json" );
    updatePathname( "keymap", FILENAMES[ "config_dir" ] + "keymap.txt" );
    updatePathname( "user_keybindings", FILENAMES[ "config_dir" ] + "keybindings.json" );
    updatePathname( "debug", FILENAMES[ "config_dir" ] + "debug.log" );
    updatePathname( "crash", FILENAMES[ "config_dir" ] + "crash.log" );
    updatePathname( "fontlist", FILENAMES[ "config_dir" ] + "fontlist.txt" );
    updatePathname( "fontdata", FILENAMES[ "config_dir" ] + "fonts.json" );
    updatePathname( "autopickup", FILENAMES[ "config_dir" ] + "auto_pickup.json" );
    updatePathname( "safemode", FILENAMES[ "config_dir" ] + "safemode.json" );
    updatePathname( "base_colors", FILENAMES[ "config_dir" ] + "base_colors.json" );
    updatePathname( "custom_colors", FILENAMES[ "config_dir" ] + "custom_colors.json" );
    updatePathname( "mods-user-default", FILENAMES[ "config_dir" ] + "user-default-mods.json" );
    updatePathname( "lastworld", FILENAMES[ "config_dir" ] + "lastworld.json" );
    updatePathname( "user_moddir", FILENAMES[ "user_dir" ] + "mods/" );
    updatePathname( "worldoptions", "worldoptions.json" );

    // Needed to move files from these legacy locations to the new config directory.
    updatePathname( "legacy_options", "data/options.txt" );
    updatePathname( "legacy_options2", FILENAMES[ "config_dir" ] + "options.txt" );
    updatePathname( "legacy_keymap", "data/keymap.txt" );
    updatePathname( "legacy_autopickup", "data/auto_pickup.txt" );
    updatePathname( "legacy_autopickup2", FILENAMES[ "config_dir" ] + "auto_pickup.txt" );
    updatePathname( "legacy_fontdata", FILENAMES[ "datadir" ] + "fontdata.json" );
    updatePathname( "legacy_worldoptions", "worldoptions.txt" );
#ifdef TILES
    // Default tileset config file.
    updatePathname( "tileset-conf", "tileset.txt" );
#endif
#ifdef SDL_SOUND
    // Default soundpack config file.
    updatePathname( "soundpack-conf", "soundpack.txt" );
#endif
}

void Path::updateDataDirectory( )
{
    // Shared dirs
    updatePathname( "gfxdir", FILENAMES[ "datadir" ] + "gfx/" );
    updatePathname( "fontdir", FILENAMES[ "datadir" ] + "font/" );
    updatePathname( "rawdir", FILENAMES[ "datadir" ] + "raw/" );
    updatePathname( "jsondir", FILENAMES[ "datadir" ] + "core/" );
    updatePathname( "moddir", FILENAMES[ "datadir" ] + "mods/" );
    updatePathname( "recycledir", FILENAMES[ "datadir" ] + "recycling/" );
    updatePathname( "namesdir", FILENAMES[ "datadir" ] + "names/" );
    updatePathname( "titledir", FILENAMES[ "datadir" ] + "title/" );
    updatePathname( "motddir", FILENAMES[ "datadir" ] + "motd/" );
    updatePathname( "creditsdir", FILENAMES[ "datadir" ] + "credits/" );
    updatePathname( "data_sound", FILENAMES[ "datadir" ] + "sound" );
    updatePathname( "helpdir", FILENAMES[ "datadir" ] + "help/" );

    // Shared files
    updatePathname( "title", FILENAMES[ "titledir" ] + "en.title" );
    updatePathname( "halloween", FILENAMES[ "titledir" ] + "en.halloween" );
    updatePathname( "motd", FILENAMES[ "motddir" ] + "en.motd" );
    updatePathname( "credits", FILENAMES[ "creditsdir" ] + "en.credits" );
    updatePathname( "names", FILENAMES[ "namesdir" ] + "en.json" );
    updatePathname( "colors", FILENAMES[ "rawdir" ] + "colors.json" );
    updatePathname( "keybindings", FILENAMES[ "rawdir" ] + "keybindings.json" );
    updatePathname( "keybindings_vehicle", FILENAMES[ "rawdir" ] + "keybindings/vehicle.json" );
    updatePathname( "legacy_fontdata", FILENAMES[ "datadir" ] + "fontdata.json" );
    updatePathname( "sokoban", FILENAMES[ "rawdir" ] + "sokoban.txt" );
    updatePathname( "defaulttilejson", FILENAMES[ "gfx" ] + "tile_config.json" );
    updatePathname( "defaulttilepng", FILENAMES[ "gfx" ] + "tinytile.png" );
    updatePathname( "mods-dev-default", FILENAMES[ "moddir" ] + "default.json" );
    updatePathname( "mods-replacements", FILENAMES[ "moddir" ] + "replacements.json" );
    updatePathname( "defaultsounddir", FILENAMES[ "datadir" ] + "sound" );
    updatePathname( "help", FILENAMES[ "helpdir" ] + "texts.json" );
}

void Path::updateConfigurationDirectory( )
{
    updatePathname( "options", FILENAMES[ "config_dir" ] + "options.json" );
    updatePathname( "panel_options", FILENAMES[ "config_dir" ] + "panel_options.json" );
    updatePathname( "keymap", FILENAMES[ "config_dir" ] + "keymap.txt" );
    updatePathname( "debug", FILENAMES[ "config_dir" ] + "debug.log" );
    updatePathname( "crash", FILENAMES[ "config_dir" ] + "crash.log" );
    updatePathname( "fontlist", FILENAMES[ "config_dir" ] + "fontlist.txt" );
    updatePathname( "fontdata", FILENAMES[ "config_dir" ] + "fonts.json" );
    updatePathname( "autopickup", FILENAMES[ "config_dir" ] + "auto_pickup.json" );
    updatePathname( "safemode", FILENAMES[ "config_dir" ] + "safemode.json" );
    updatePathname( "base_colors", FILENAMES[ "config_dir" ] + "base_colors.json" );
    updatePathname( "custom_colors", FILENAMES[ "config_dir" ] + "custom_colors.json" );
    updatePathname( "mods-user-default", FILENAMES[ "config_dir" ] + "user-default-mods.json" );
    updatePathname( "lastworld", FILENAMES[ "config_dir" ] + "lastworld.json" );
}

void Path::updatePathname( const std::string &name, const std::string &path )
{
    const std::map<std::string, std::string>::iterator iter = FILENAMES.find( name );
    if( iter != FILENAMES.end() ) {
        FILENAMES[name] = path;
    } else {
        FILENAMES.insert( std::pair<std::string, std::string>( name, path ) );
    }
}

Path::Path( const std::string basePath, const std::string userDirectoryPath )
{ }

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
