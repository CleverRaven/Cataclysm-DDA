#include "path_info.h"

#include <cstdlib>
#include <string>

#include "debug.h"
#include "enums.h"
#include "filesystem.h" // IWYU pragma: keep
#include "make_static.h"
#include "options.h"
#include "rng.h"
#include "system_locale.h"

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
static cata_path find_translated_file( const cata_path &base_path, const std::string &extension,
                                       const cata_path &fallback );

static std::string motd_value;
static std::string gfxdir_value;
static std::string config_dir_value;
static std::string user_dir_value;
static std::string datadir_value;
static std::string base_path_value;
static std::string savedir_value;
static std::string autopickup_value;
static std::string autonote_value;
static std::string keymap_value;
static std::string options_value;
static std::string memorialdir_value;
static std::string achievementdir_value;
static std::string langdir_value;

static cata_path autonote_path_value;
static cata_path autopickup_path_value;
static cata_path base_path_path_value;
static cata_path config_dir_path_value;
static cata_path datadir_path_value;
static cata_path gfxdir_path_value;
static cata_path keymap_path_value;
static cata_path langdir_path_value;
static cata_path memorialdir_path_value;
static cata_path achievementdir_path_value;
static cata_path motd_path_value;
static cata_path options_path_value;
static cata_path savedir_path_value;
static cata_path user_dir_path_value;

// Get the given env var, or abort the program if it is not set
static const char *getenv_or_abort( const char *name )
{
    const char *result = getenv( name );
    if( !result ) {
        cata_fatal( "Required environment variable %s was not set", name );
    }
    return result;
}

void PATH_INFO::init_base_path( const std::string &path )
{
    base_path_value = as_norm_dir( path );
    base_path_path_value = cata_path{ cata_path::root_path::base, fs::path{} };
}

void PATH_INFO::init_user_dir( std::string dir )
{
    if( dir.empty() ) {
        const char *user_dir;
#if defined(_WIN32)
        user_dir = getenv_or_abort( "LOCALAPPDATA" );
        // On Windows userdir without dot
        dir = std::string( user_dir ) + "/cataclysm-dda/";
#elif defined(MACOSX)
        user_dir = getenv_or_abort( "HOME" );
        dir = std::string( user_dir ) + "/Library/Application Support/Cataclysm/";
#elif defined(USE_XDG_DIR)
        if( ( user_dir = getenv( "XDG_DATA_HOME" ) ) ) {
            dir = std::string( user_dir ) + "/cataclysm-dda/";
        } else {
            user_dir = getenv_or_abort( "HOME" );
            dir = std::string( user_dir ) + "/.local/share/cataclysm-dda/";
        }
#else
        user_dir = getenv_or_abort( "HOME" );
        dir = std::string( user_dir ) + "/.cataclysm-dda/";
#endif
    }

    user_dir_value = as_norm_dir( dir );
    user_dir_path_value = cata_path{ cata_path::root_path::user, fs::path{} };
}

void PATH_INFO::set_standard_filenames()
{
    // Special: data_dir and gfx_dir
    std::string prefix;
    cata_path prefix_path;

    // Data is always relative to itself. Also, the base path might not be writeable.
    datadir_path_value = cata_path{ cata_path::root_path::data, fs::path{} };

    if( !base_path_value.empty() ) {
#if defined(DATA_DIR_PREFIX)
        datadir_value = base_path_value + "share/cataclysm-dda/";
        prefix = datadir_value;
        prefix_path = datadir_path_value;
#else
        datadir_value = base_path_value + "data/";
        prefix = base_path_value;
        prefix_path = base_path_path_value;
#endif
    } else {
        datadir_value = "data/";
        // Base path is empty here but everything else is still relative to base, which is empty.
        prefix_path = base_path_path_value;
    }

    gfxdir_value = prefix + "gfx/";
    gfxdir_path_value = prefix_path / "gfx";
    langdir_value = prefix + "lang/mo/";
    langdir_path_value = prefix_path / "lang" / "mo";

    // Shared dirs

    // Shared files
    motd_value = datadir_value + "motd/" + "en.motd";

    savedir_value = user_dir_value + "save/";
    // Special: savedir is always relative to itself even if in the user dir location.
    savedir_path_value = cata_path{ cata_path::root_path::save, fs::path{} };
    memorialdir_value = user_dir_value + "memorial/";
    memorialdir_path_value = user_dir_path_value / "memorial";
    achievementdir_value = user_dir_value + "achievements/";
    achievementdir_path_value = user_dir_path_value / "achievements";

#if defined(USE_XDG_DIR)
    const char *user_dir;
    std::string dir;
    if( ( user_dir = getenv( "XDG_CONFIG_HOME" ) ) ) {
        dir = std::string( user_dir ) + "/cataclysm-dda/";
    } else {
        user_dir = getenv_or_abort( "HOME" );
        dir = std::string( user_dir ) + "/.config/cataclysm-dda/";
    }
    config_dir_value = dir;
    config_dir_path_value = cata_path{ cata_path::root_path::config, fs::path{} };
#else
    config_dir_value = user_dir_value + "config/";
    config_dir_path_value = user_dir_path_value / "config";
#endif
    options_value = config_dir_value + "options.json";
    options_path_value = config_dir_path_value / "options.json";
    keymap_value = config_dir_value + "keymap.txt";
    autopickup_value = config_dir_value + "auto_pickup.json";
    autopickup_path_value = config_dir_path_value / "auto_pickup.json";
    autonote_value = config_dir_value + "auto_note.json";
    autonote_path_value = config_dir_path_value / "auto_note.json";
}

std::string find_translated_file( const std::string &base_path, const std::string &extension,
                                  const std::string &fallback )
{
#if defined(LOCALIZE) && !defined(__CYGWIN__)
    const std::string language_option = get_option<std::string>( "USE_LANG" );
    const std::string loc_name = language_option.empty() ? SystemLocale::Language().value_or( "" ) :
                                 language_option;
    if( !loc_name.empty() ) {
        std::string local_path = base_path + loc_name + extension;
        if( file_exist( local_path ) ) {
            return local_path;
        }
    }
#else
    ( void ) base_path;
    ( void ) extension;
#endif
    return fallback;
}

cata_path find_translated_file( const cata_path &base_path, const std::string &extension,
                                const cata_path &fallback )
{
#if defined(LOCALIZE) && !defined(__CYGWIN__)
    const std::string language_option = get_option<std::string>( "USE_LANG" );
    const std::string loc_name = language_option.empty() ? SystemLocale::Language().value_or( "" ) :
                                 language_option;
    if( !loc_name.empty() ) {
        cata_path local_path = base_path / ( loc_name + extension );
        if( file_exist( local_path ) ) {
            return local_path;
        }
    }
#else
    ( void )base_path;
    ( void )extension;
#endif
    return fallback;
}

cata_path PATH_INFO::autopickup()
{
    return autopickup_path_value;
}
cata_path PATH_INFO::autonote()
{
    return autonote_path_value;
}
cata_path PATH_INFO::base_colors()
{
    return config_dir_path_value / "base_colors.json";
}
std::string PATH_INFO::base_path()
{
    return base_path_value;
}
cata_path PATH_INFO::base_path_path()
{
    return base_path_path_value;
}
std::string PATH_INFO::cache_dir()
{
    return datadir_value + "cache/";
}
cata_path PATH_INFO::colors()
{
    return datadir_path_value / "raw" / "colors.json";
}
cata_path PATH_INFO::color_templates()
{
    return datadir_path_value / "raw" / "color_templates";
}
cata_path PATH_INFO::color_themes()
{
    return datadir_path_value / "raw" / "color_themes";
}
std::string PATH_INFO::config_dir()
{
    return config_dir_value;
}
cata_path PATH_INFO::config_dir_path()
{
    return config_dir_path_value;
}
cata_path PATH_INFO::custom_colors()
{
    return config_dir_path_value / "custom_colors.json";
}
std::string PATH_INFO::datadir()
{
    return datadir_value;
}
cata_path PATH_INFO::datadir_path()
{
    return datadir_path_value;
}
std::string PATH_INFO::debug()
{
    return config_dir_value + "debug.log";
}
cata_path PATH_INFO::defaultsounddir()
{
    return datadir_path_value / "sound";
}
std::string PATH_INFO::defaulttilejson()
{
    return "tile_config.json";
}
std::string PATH_INFO::defaultlayeringjson()
{
    return "layering.json";
}
std::string PATH_INFO::defaulttilepng()
{
    return "tinytile.png";
}
cata_path PATH_INFO::fontdata()
{
    return config_dir_path_value / "fonts.json";
}
std::string PATH_INFO::fontdir()
{
    return datadir_value + "font/";
}
std::string PATH_INFO::user_font()
{
    return user_dir_value + "font/";
}
std::string PATH_INFO::graveyarddir()
{
    return user_dir_value + "graveyard/";
}
cata_path PATH_INFO::help()
{
    return datadir_path_value / "help" / "texts.json";
}
cata_path PATH_INFO::keybindings()
{
    return datadir_path_value / "raw" / "keybindings.json";
}
cata_path PATH_INFO::keybindings_vehicle()
{
    return datadir_path_value / "raw" / "keybindings" / "vehicle.json";
}
std::string PATH_INFO::keymap()
{
    return keymap_value;
}
cata_path PATH_INFO::lastworld()
{
    return config_dir_path_value / "lastworld.json";
}
cata_path PATH_INFO::legacy_fontdata()
{
    return datadir_path_value / "fontdata.json";
}
std::string PATH_INFO::memorialdir()
{
    return memorialdir_value;
}
std::string PATH_INFO::achievementdir()
{
    return achievementdir_value;
}
cata_path PATH_INFO::memorialdir_path()
{
    return memorialdir_path_value;
}
cata_path PATH_INFO::achievementdir_path()
{
    return achievementdir_path_value;
}
cata_path PATH_INFO::jsondir()
{
    return datadir_path_value / "core";
}
cata_path PATH_INFO::moddir()
{
    return datadir_path_value / "mods";
}
cata_path PATH_INFO::options()
{
    return options_path_value;
}
cata_path PATH_INFO::panel_options()
{
    return config_dir_path_value / "panel_options.json";
}
cata_path PATH_INFO::pocket_presets()
{
    return config_dir_path_value / "pocket_presets.json";
}
cata_path PATH_INFO::safemode()
{
    return config_dir_path_value / "safemode.json";
}
std::string PATH_INFO::savedir()
{
    return savedir_value;
}
cata_path PATH_INFO::savedir_path()
{
    return savedir_path_value;
}
std::string PATH_INFO::sokoban()
{
    return datadir_value + "raw/" + "sokoban.txt";
}
std::string PATH_INFO::templatedir()
{
    return user_dir_value + "templates/";
}
cata_path PATH_INFO::templatedir_path()
{
    return user_dir_path_value / "templates";
}
std::string PATH_INFO::user_dir()
{
    return user_dir_value;
}
cata_path PATH_INFO::user_dir_path()
{
    return user_dir_path_value;
}
cata_path PATH_INFO::user_gfx()
{
    return user_dir_path_value / "gfx";
}
cata_path PATH_INFO::user_keybindings()
{
    return config_dir_path_value / "keybindings.json";
}
std::string PATH_INFO::user_moddir()
{
    return user_dir_value + "mods/";
}
cata_path PATH_INFO::user_moddir_path()
{
    return user_dir_path_value / "mods";
}
cata_path PATH_INFO::user_sound()
{
    return user_dir_path_value / "sound";
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
cata_path PATH_INFO::mods_replacements()
{
    return datadir_path_value / "mods" / "replacements.json";
}
cata_path PATH_INFO::mods_dev_default()
{
    return datadir_path_value / "mods" / "default.json";
}
cata_path PATH_INFO::mods_user_default()
{
    return config_dir_path_value / "user-default-mods.json";
}
std::string PATH_INFO::soundpack_conf()
{
    return "soundpack.txt";
}
cata_path PATH_INFO::gfxdir()
{
    return gfxdir_path_value;
}
std::string PATH_INFO::langdir()
{
    return langdir_value;
}
cata_path PATH_INFO::langdir_path()
{
    return langdir_path_value;
}
std::string PATH_INFO::lang_file()
{
    return "cataclysm-dda.mo";
}
cata_path PATH_INFO::data_sound()
{
    return datadir_path_value / "sound";
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

    if( !get_option<bool>( "ENABLE_ASCII_TITLE" ) ) {
        return _( "Cataclysm: Dark Days Ahead" );
    }

    if( x_in_y( get_option<int>( "ALT_TITLE" ), 100 ) ) {
        theme_extension = ".alt1";
        theme_fallback = datadir_value + "title/" + "en.alt1";
    }

    if( !get_option<bool>( "SEASONAL_TITLE" ) ) {
        return find_translated_file( theme_basepath, theme_extension, theme_fallback );
    }

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

cata_path PATH_INFO::names()
{
    return find_translated_file( datadir_path_value / "names", ".json",
                                 datadir_path_value / "names" / "en.json" );
}

void PATH_INFO::set_datadir( const std::string &datadir )
{
    datadir_value = datadir;
    datadir_path_value = cata_path{ cata_path::root_path::data, fs::path{} };
    // Shared dirs
    gfxdir_value = datadir_value + "gfx/";
    gfxdir_path_value = datadir_path_value / "gfx";

    // Shared files
    motd_value = datadir_value + "motd/" + "en.motd";
    motd_path_value = datadir_path_value / "motd" / "en.motd";
}

void PATH_INFO::set_config_dir( const std::string &config_dir )
{
    config_dir_value = config_dir;
    config_dir_path_value = cata_path{ cata_path::root_path::config, fs::path{} };
    options_value = config_dir_value + "options.json";
    options_path_value = config_dir_path_value / "options.json";
    keymap_value = config_dir_value + "keymap.txt";
    keymap_path_value = config_dir_path_value / "keymap.txt";
    autopickup_value = config_dir_value + "auto_pickup.json";
    autopickup_path_value = config_dir_path_value / "auto_pickup.json";
}

void PATH_INFO::set_savedir( const std::string &savedir )
{
    savedir_value = savedir;
    savedir_path_value = cata_path{ cata_path::root_path::save, fs::path{} };
}

void PATH_INFO::set_memorialdir( const std::string &memorialdir )
{
    memorialdir_value = memorialdir;
    memorialdir_path_value = cata_path{ cata_path::root_path::memorial, fs::path{} };
}

void PATH_INFO::set_options( const std::string &options )
{
    options_value = options;
    options_path_value = cata_path{cata_path::root_path::unknown, options_value};
}

void PATH_INFO::set_keymap( const std::string &keymap )
{
    keymap_value = keymap;
}

void PATH_INFO::set_autopickup( const std::string &autopickup )
{
    autopickup_value = autopickup;
    autopickup_path_value = cata_path{ cata_path::root_path::unknown, autopickup_value };
}

void PATH_INFO::set_motd( const std::string &motd )
{
    motd_value = motd;
}

fs::path cata_path::get_logical_root_path() const
{
    const std::string &path_value = ( []( cata_path::root_path root ) -> const std::string& {
        switch( root )
        {
            case cata_path::root_path::base:
                return base_path_value;
            case cata_path::root_path::config:
                return config_dir_value;
            case cata_path::root_path::data:
                return datadir_value;
            case cata_path::root_path::memorial:
                return memorialdir_value;
            case cata_path::root_path::save:
                return savedir_value;
            case cata_path::root_path::user:
                return user_dir_value;
            case cata_path::root_path::unknown:
            default: {
                return STATIC( std::string() );
            }
        }
    } )( logical_root_ );
    return fs::u8path( path_value );
}
