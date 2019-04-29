#include "path_info.h"

#include <clocale>
#include <cstdlib>
#include <iostream>
#include <utility>

#include "filesystem.h"
#include "options.h"

#if defined(_WIN32)
#include <windows.h>
#endif

void Path::initUserDirectory( )
{
    // If the path to the user directory is empty
    // it means that it was not set previously
    // and therefore we must set it.
    if( pathname["USER_DIRE"].empty() ) {

        const char *pathUserDirectory;

        // Path to the directory that stores the
        // Cataclysm DDA configuration files.
        // This path is built from the path of
        // the user directory.
        std::string pathDirectoryCataclysm;

#if defined(_WIN32)

        pathUserDirectory = getenv( "LOCALAPPDATA" );
        // On Windows userdir without dot
        pathDirectoryCataclysm = std::string( pathUserDirectory ) + "/cataclysm-dda/";

#elif defined(MACOSX)

        pathUserDirectory = getenv( "HOME" );
        pathDirectoryCataclysm = std::string( pathUserDirectory ) + "/Library/Application Support/Cataclysm/";

#elif defined(USE_XDG_DIR)

        if( ( pathUserDirectory = getenv( "XDG_DATA_HOME" ) ) ) {
            pathDirectoryCataclysm = std::string( pathUserDirectory ) + "/cataclysm-dda/";
        } else {
            pathUserDirectory = getenv( "HOME" );
            pathDirectoryCataclysm = std::string( pathUserDirectory ) + "/.local/share/cataclysm-dda/";
        }

#else

        pathUserDirectory = getenv( "HOME" );
        pathDirectoryCataclysm = std::string( pathUserDirectory ) + "/.cataclysm-dda/";

#endif

        pathname["USER_DIRE"] = pathDirectoryCataclysm;
    }
}

void Path::initDataDirectory( )
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

Path * Path::getInstance( std::string basePath, std::string userDirectoryPath )
{
    if (instance == 0)
    {
        instance = new Path( basePath, userDirectoryPath );
    }

    return instance;
}

Path* Path::getInstance( )
{
    if (instance == 0)
    {
        // TODO: Raise error, the object hasn't been created.
    }

    return instance;
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

/*
 * Null, because instance will be initialized on demand.
 */
Path* Path::instance = 0;

// Private Construct

Path::Path( std::string basePath, std::string userDirectoryPath )
{
    pathname["BASE_PATH"] = formatPath(basePath);
    pathname["USER_DIRE"] = formatPath(userDirectoryPath);

    initUserDirectory( );
    // We set the directory 'data' and 'gfx' determined by the base path.
    initDataDirectory( );

    // Shared Directories
    pathname["FONT_DIRE"] = pathname["DATA_DIRE"] + "font/";
    pathname["RAW_DIRE"]  = pathname["DATA_DIRE"] + "raw/";
    pathname["JSON_DIRE"] = pathname["DATA_DIRE"] + "core/";
    pathname["MOD_DIRE"]  = pathname["DATA_DIRE"] + "mods/";
    pathname["NAMES_DIR"] = pathname["DATA_DIRE"] + "names/";
    pathname["TITLE_DIR"] = pathname["MOD_DIRE"]  + "title/";
    pathname["MOTD_DIRE"] = pathname["DATA_DIRE"] + "motd/";
    pathname["CREDI_DIR"] = pathname["DATA_DIRE"] + "credits/";
    pathname["COLOR_TEM"] = pathname["RAW_DIRE"]  + "color_templates/";
    pathname["DAT_SOUND"] = pathname["DATA_DIRE"] + "sound";
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
    pathname["DF_SOUND_DIRE"] = pathname["DATA_DIRE"] + "sound";
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
    Path *path = Path::getInstance();

    const std::string base_path = path->getPathForValueKey(pathid);

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
    return path->getPathForValueKey(fallbackid);
}
