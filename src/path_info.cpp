#include "path_info.h"

#include <clocale>
#include <cstdlib>

#include "enums.h"
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

static std::string motd_value;
static std::string gfxdir_value;
static std::string config_dir_value;
static std::string user_dir_value;
static std::string datadir_value;
static std::string base_path_value;
static std::string savedir_value;
static std::string autopickup_value;
static std::string keymap_value;
static std::string options_value;
static std::string memorialdir_value;
static std::string langdir_value;

void PATH_INFO::init_base_path( std::string path )
{
    if( !path.empty() ) {
        const char ch = path.back();
        if( ch != '/' && ch != '\\' ) {
            path.push_back( '/' );
        }
    }

    base_path_value = path;
}

void PATH_INFO::init_user_dir( std::string dir )
{
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

    user_dir_value = dir;
}

void PATH_INFO::set_standard_filenames()
{
    // Special: data_dir and gfx_dir
    if( !base_path_value.empty() ) {
#if defined(DATA_DIR_PREFIX)
        datadir_value = base_path_value + "share/cataclysm-dda/";
        gfxdir_value = datadir_value + "gfx/";
        langdir_value = datadir_value + "lang/";
#else
        datadir_value = base_path_value + "data/";
        gfxdir_value = base_path_value + "gfx/";
        langdir_value = base_path_value + "lang/";
#endif
    } else {
        datadir_value = "data/";
        gfxdir_value = "gfx/";
        langdir_value = "lang/";
    }

    // Shared dirs

    // Shared files
    motd_value = datadir_value + "motd/" + "en.motd";

    savedir_value = user_dir_value + "save/";
    memorialdir_value = user_dir_value + "memorial/";

#if defined(USE_XDG_DIR)
    const char *user_dir;
    std::string dir;
    if( ( user_dir = getenv( "XDG_CONFIG_HOME" ) ) ) {
        dir = std::string( user_dir ) + "/cataclysm-dda/";
    } else {
        user_dir = getenv( "HOME" );
        dir = std::string( user_dir ) + "/.config/cataclysm-dda/";
    }
    config_dir_value = dir;
#else
    config_dir_value = user_dir_value + "config/";
#endif
    options_value = config_dir_value + "options.json";
    keymap_value = config_dir_value + "keymap.txt";
    autopickup_value = config_dir_value + "auto_pickup.json";
}

std::string find_translated_file( const std::string &base_path, const std::string &extension,
                                  const std::string &fallback )
{
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
#else
    ( void ) base_path;
    ( void ) extension;
#endif
    return fallback;
}
std::string PATH_INFO::autopickup()
{
    return autopickup_value;
}
std::string PATH_INFO::base_colors()
{
    return config_dir_value + "base_colors.json";
}
std::string PATH_INFO::base_path()
{
    return base_path_value;
}
std::string PATH_INFO::colors()
{
    return datadir_value + "raw/" + "colors.json";
}
std::string PATH_INFO::color_templates()
{
    return datadir_value + "raw/" + "color_templates/";
}
std::string PATH_INFO::config_dir()
{
    return config_dir_value;
}
std::string PATH_INFO::custom_colors()
{
    return config_dir_value + "custom_colors.json";
}
std::string PATH_INFO::datadir()
{
    return datadir_value;
}
std::string PATH_INFO::debug()
{
    return config_dir_value + "debug.log";
}
std::string PATH_INFO::defaultsounddir()
{
    return datadir_value + "sound";
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
    return config_dir_value + "fonts.json";
}
std::string PATH_INFO::fontdir()
{
    return datadir_value + "font/";
}
std::string PATH_INFO::user_font()
{
    return user_dir_value + "font/";
}
std::string PATH_INFO::fontlist()
{
    return config_dir_value + "fontlist.txt";
}
std::string PATH_INFO::graveyarddir()
{
    return user_dir_value + "graveyard/";
}
std::string PATH_INFO::help()
{
    return datadir_value + "help/" + "texts.json";
}
std::string PATH_INFO::keybindings()
{
    return datadir_value + "raw/" + "keybindings.json";
}
std::string PATH_INFO::keybindings_vehicle()
{
    return datadir_value + "raw/" + "keybindings/vehicle.json";
}
std::string PATH_INFO::keymap()
{
    return keymap_value;
}
std::string PATH_INFO::lastworld()
{
    return config_dir_value + "lastworld.json";
}
std::string PATH_INFO::legacy_autopickup()
{
    return "data/auto_pickup.txt";
}
std::string PATH_INFO::legacy_autopickup2()
{
    return config_dir_value + "auto_pickup.txt";
}
std::string PATH_INFO::legacy_fontdata()
{
    return datadir_value + "fontdata.json";
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
    return config_dir_value + "options.txt";
}
std::string PATH_INFO::legacy_worldoptions()
{
    return "worldoptions.txt";
}
std::string PATH_INFO::memorialdir()
{
    return memorialdir_value;
}
std::string PATH_INFO::jsondir()
{
    return datadir_value + "core/";
}
std::string PATH_INFO::moddir()
{
    return datadir_value + "mods/";
}
std::string PATH_INFO::options()
{
    return options_value;
}
std::string PATH_INFO::panel_options()
{
    return config_dir_value + "panel_options.json";
}
std::string PATH_INFO::safemode()
{
    return config_dir_value + "safemode.json";
}
std::string PATH_INFO::savedir()
{
    return savedir_value;
}
std::string PATH_INFO::sokoban()
{
    return datadir_value + "raw/" + "sokoban.txt";
}
std::string PATH_INFO::templatedir()
{
    return user_dir_value + "templates/";
}
std::string PATH_INFO::user_dir()
{
    return user_dir_value;
}
std::string PATH_INFO::user_gfx()
{
    return user_dir_value + "gfx/";
}
std::string PATH_INFO::user_keybindings()
{
    return config_dir_value + "keybindings.json";
}
std::string PATH_INFO::user_moddir()
{
    return user_dir_value + "mods/";
}
std::string PATH_INFO::user_sound()
{
    return user_dir_value + "sound/";
}
std::string PATH_INFO::worldoptions()
{
    return "worldoptions.json";
}
std::string PATH_INFO::crash()
{
    return config_dir_value + "crash.log";
}
std::string PATH_INFO::tileset_conf()
{
    return "tileset.txt";
}
std::string PATH_INFO::mods_replacements()
{
    return datadir_value + "mods/" + "replacements.json";
}
std::string PATH_INFO::mods_dev_default()
{
    return datadir_value + "mods/" + "default.json";
}
std::string PATH_INFO::mods_user_default()
{
    return config_dir_value + "user-default-mods.json";
}
std::string PATH_INFO::soundpack_conf()
{
    return "soundpack.txt";
}
std::string PATH_INFO::gfxdir()
{
    return gfxdir_value;
}
std::string PATH_INFO::langdir()
{
    return langdir_value;
}
std::string PATH_INFO::lang_file()
{
    return "cataclysm-dda.mo";
}
std::string PATH_INFO::data_sound()
{
    return datadir_value + "sound";
}

std::string PATH_INFO::credits()
{
    return find_translated_file( datadir_value + "credits/", ".credits",
                                 datadir_value + "credits/" + "en.credits" );
}

std::string PATH_INFO::motd()
{
    return find_translated_file( datadir_value + "motd/", ".motd", motd_value );
}

std::string PATH_INFO::title( const holiday current_holiday )
{
    std::string theme_basepath = datadir_value + "title/";
    std::string theme_extension = ".title";
    std::string theme_fallback = theme_basepath + "en.title";
    switch( current_holiday ) {
        case holiday::new_year:
            theme_extension = ".new_year";
            theme_fallback = datadir_value + "title/" + "en.new_year";
            break;
        case holiday::easter:
            theme_extension = ".easter";
            theme_fallback = datadir_value + "title/" + "en.easter";
            break;
        case holiday::independence_day:
            theme_extension = ".independence_day";
            theme_fallback = datadir_value + "title/" + "en.independence_day";
            break;
        case holiday::halloween:
            theme_extension = ".halloween";
            theme_fallback = datadir_value + "title/" + "en.halloween";
            break;
        case holiday::thanksgiving:
            theme_extension = ".thanksgiving";
            theme_fallback = datadir_value + "title/" + "en.thanksgiving";
            break;
        case holiday::christmas:
            theme_extension = ".christmas";
            theme_fallback = datadir_value + "title/" + "en.christmas";
            break;
        case holiday::none:
        case holiday::num_holiday:
        default:
            break;
    }
    return find_translated_file( theme_basepath, theme_extension, theme_fallback );
}

std::string PATH_INFO::names()
{
    return find_translated_file( datadir_value + "names/", ".json",
                                 datadir_value + "names/" + "en.json" );
}

void PATH_INFO::set_datadir( const std::string &datadir )
{
    datadir_value = datadir;
    // Shared dirs
    gfxdir_value = datadir_value + "gfx/";

    // Shared files
    motd_value = datadir_value + "motd/" + "en.motd";
}

void PATH_INFO::set_config_dir( const std::string &config_dir )
{
    config_dir_value = config_dir;
    options_value = config_dir_value + "options.json";
    keymap_value = config_dir_value + "keymap.txt";
    autopickup_value = config_dir_value + "auto_pickup.json";
}

void PATH_INFO::set_savedir( const std::string &savedir )
{
    savedir_value = savedir;
}

void PATH_INFO::set_memorialdir( const std::string &memorialdir )
{
    memorialdir_value = memorialdir;
}

void PATH_INFO::set_options( const std::string &options )
{
    options_value = options;
}

void PATH_INFO::set_keymap( const std::string &keymap )
{
    keymap_value = keymap;
}

void PATH_INFO::set_autopickup( const std::string &autopickup )
{
    autopickup_value = autopickup;
}

void PATH_INFO::set_motd( const std::string &motd )
{
    motd_value = motd;
}
