#include "path_info.h"

#include <clocale>
#include <cstdlib>
#include <iostream>

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

void Path::initDataDirectory( std::map<std::string, std::string> pathname )
{
    // If the base path is empty (example: ""), it means that the 'data'
    // directory is at the same level where the application is executed.
    if (pathname["BASE_PATH"].empty())
    {
        pathname["DATA_DIRE"] = "data/";
        pathname["GFX_DIRE"] = "gfx/";
    }
    else
    {
#if  defined(DATA_DIR_PREFIX)

        pathname["DATA_DIRE"] = pathname["BASE_PATH"] + "share/cataclysm-dda/";
        pathname["GFX_DIRE"]  = pathname["DATA_DIRE"] + "gfx/";
#else
        pathname["DATA_DIRE"] = pathname["BASE_PATH"] + "data/";
        pathname["GFX_DIRE"]  = pathname["BASE_PATH"] + "gfx/";
#endif
    }
}

std::string Path::getPathForValueKey( const std::string valueKey )
{
    return pathname[valueKey];
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

void Path::toString( )
{
    for( auto const& x : pathname)
    {
        std::cout << x.first
        << ':'
        << x.second
        << std::endl;
    }
}

// Construct

Path::Path( std::string basePath, std::string userDirectoryPath )
{
    pathname["BASE_PATH"] = formatPath(basePath);
    // TODO: Get user directory
    pathname["USER_DIRE"] = formatPath(userDirectoryPath);

    // We set the directory 'data' and 'gfx' determined by the base path.
    initDataDirectory(pathname);

    // Shared Directories
    pathname["FONT_DIRE"] = pathname["DATA_DIRE"] + "font/";
    pathname["RAW_DIRE"]  = pathname["DATA_DIRE"] + "raw/";
    pathname["JSON_DIRE"] = pathname["DATA_DIRE"] + "core/";
    pathname["MOD_DIRE"]  = pathname["DATA_DIRE"] + "mods/";
    pathname["NAMES_DIR"] = pathname["DATA_DIRE"] + "names/";
    pathname["TITLE_DIR"] = pathname["MOD_DIRE"] + "title/";
    pathname["MOTD_DIRE"] = pathname["DATA_DIRE"] + "motd/";
    pathname["CREDI_DIR"] = pathname["DATA_DIRE"] + "credits/";
    pathname["COLOR_TEM"] = pathname["RAW_DIRE"]  + "color_templates/";
    pathname["DAT_SOUND"] = pathname["DATA_DIRE"] + "sound/";
    pathname["HELP_DIRE"] = pathname["DATA_DIRE"] + "help/";

    // Shared Files
    pathname["TITLE_CATACLY"] = pathname["TITLE_DIR"] + "en.title";
    pathname["TITLE_HALLOWW"] = pathname["TITLE_DIR"] + "en.halloween";
    pathname["MOTD_FILE"]     = pathname["MOTD_DIRE"] + "en.motd";
    pathname["CREDITS_FILE"]  = pathname["CREDI_DIR"] + "en.credits";
    pathname["NAMES_FILE"]    = pathname["NAMES_DIR"] + "en.json";
    pathname["COLORS_FILE"]   = pathname["RAW_DIRE"]  + "colors.json";
    pathname["KEYBINDINGS"]   = pathname["RAW_DIRE"]  + "keybindings.json";
    pathname["KEYBIND_VEHIC"] = pathname["RAW_DIRE"]  + "keybindings/vehicle.json";
    pathname["SOKOBAN"]       = pathname["RAW_DIRE"]  + "sokoban.txt";
    pathname["DF_TITLE_JSON"] = pathname["GFX_DIRE"]  + "tile_config.json";
    pathname["DF_TITLE_PNG"]  = pathname["GFX_DIRE"]  + "tinytile.png";
    pathname["DF_MODS_DEV"]   = pathname["MOD_DIRE"]  + "default.json";
    pathname["MODS_REPLACEM"] = pathname["MOD_DIRE"]  + "replacements.json";
    pathname["DF_SOUND_DIRE"] = pathname["DATA_DIRE"] + "sound/";
    pathname["HELP_FILE"]     = pathname["HELP_DIRE"] + "texts.json";

    // User Directories
    pathname["SAVE_DIRE"] = pathname["USER_DIRE"] + "save/";
    pathname["MEMO_DIRE"] = pathname["USER_DIRE"] + "memorial/";
    pathname["TEMP_DIRE"] = pathname["USER_DIRE"] + "templates/";
    pathname["USER_SND"] =  pathname["USER_DIRE"] + "sound/";

    // User Configuration Directory
    pathname["CONFIG_DIR"] = pathname["USER_DIRE"] + "config/";

    pathname["GRAVEY_DIR"] = pathname["USER_DIRE"] + "graveyard/";

    // User Configuration Files
    pathname["OPTIONS_USER"] = pathname["CONFIG_DIR"] + "options.json";
    pathname["OPTINS_PANEL"] = pathname["CONFIG_DIR"] + "panel_options.json";
    pathname["KEY_MAP_FILE"] = pathname["CONFIG_DIR"] + "keymap.txt";
    pathname["KEYBIND_USER"] = pathname["CONFIG_DIR"] + "keybindings.json";
    pathname["DEBUG_FILE"]   = pathname["CONFIG_DIR"] + "debug.log";
    pathname["CRASH_FILE"]   = pathname["CONFIG_DIR"] + "crash.log";
    pathname["FONTS_LIST"]   = pathname["CONFIG_DIR"] + "fontlist.txt";
    pathname["FONTS_DATA"]   = pathname["CONFIG_DIR"] + "fonts.json";
    pathname["AUTOPICKUP"]   = pathname["CONFIG_DIR"] + "auto_pickup.json";
    pathname["SAFE_MODE"]    = pathname["CONFIG_DIR"] + "safemode.json";
    pathname["BASE_COLORS"]  = pathname["CONFIG_DIR"] + "base_colors.json";
    pathname["CUST_COLORS"]  = pathname["CONFIG_DIR"] + "custom_colors.json";
    pathname["DF_MODS_USER"] = pathname["CONFIG_DIR"] + "user-default-mods.json";
    pathname["LAST_WORLD"]   = pathname["CONFIG_DIR"] + "lastworld.json";
    pathname["USER_MOD_DIR"] = pathname["USER_DIRE"] + "mods/";
    pathname["WORLD_OPTION"] = "worldoptions.json";

    // NOTES: Reason for removing pre-processing directives:
    // There is no significant impact to leaving these
    // predefined configuration files.
    pathname["TILESET_CONF"] = "tileset.txt";
    pathname["SNDPACK_CONF"] = "soundpack.txt";
}

// Functions private

std::string Path::formatPath(std::string path)
{
    if( path.empty() )
    {
        return path;
    }
    else
    {
        // We get the last character in the string to verify
        // that '/' or '\' exists at the end of the string.
        const char ch = path.at( path.length() - 1 );

        // Is the last character different from '/' or '\'?
        if( ch != '/' && ch != '\\' )
        {
            // If the string does not have the proper format
            // we add the character '/' to correct it.
            // NOTA: For Windows it also works to add '/'.
            path.push_back( '/' );
        }
    }

    return path;
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
