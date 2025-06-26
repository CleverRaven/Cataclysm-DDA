#include "font_loader.h"

#if defined( TILES )

#include "json_loader.h"

// Ensure that unifont is always loaded as a fallback font to prevent users from shooting themselves in the foot
void ensure_unifont_loaded( std::vector<font_config> &font_list )
{
    const std::string unifont = PATH_INFO::fontdir() + "unifont.ttf";
    if( std::find_if( font_list.begin(), font_list.end(), [&]( const font_config & conf ) {
    return conf.path == unifont;
} ) == font_list.end() ) {
        font_list.emplace_back( unifont );
    }
}

void ensure_unifont_loaded( std::vector<std::string> &font_list )
{
    const std::string unifont = PATH_INFO::fontdir() + "unifont.ttf";
    if( std::find( font_list.begin(), font_list.end(), unifont ) == font_list.end() ) {
        font_list.emplace_back( unifont );
    }
}

unsigned int font_config::imgui_config() const
{
    unsigned int ret = 0;
    if( !antialiasing ) {
        ret |= ImGuiFreeTypeBuilderFlags_Monochrome;
        ret |= ImGuiFreeTypeBuilderFlags_MonoHinting;
    }
    if( hinting != std::nullopt ) {
        ret |= *hinting;
    }
    return ret;
}

static std::optional<ImGuiFreeTypeBuilderFlags> hint_to_fonthint( std::string_view hinting )
{
    if( hinting == "Auto" ) {
        return ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    }
    if( hinting == "NoAuto" ) {
        return ImGuiFreeTypeBuilderFlags_NoAutoHint;
    }
    if( hinting == "Light" ) {
        return ImGuiFreeTypeBuilderFlags_LightHinting;
    }
    if( hinting == "None" ) {
        return ImGuiFreeTypeBuilderFlags_NoHinting;
    }
    if( hinting == "Bitmap" ) {
        return ImGuiFreeTypeBuilderFlags_Bitmap;
    }
    if( hinting == "Default" ) {
        return std::nullopt;
    }
    debugmsg( "'%s' is an invalid font hinting value.", hinting );
    return std::nullopt;
}

void font_config::deserialize( const JsonObject &jo )
{
    bool has_path = jo.read( "path", path, true );
    if( !has_path ) {
        jo.throw_error( "Dummy error - force config read to return false." );
    }
    // Manually read hinting.
    // Specifying enum traits for would allow all options, and we want only
    // some to be available to the user.
    if( jo.has_string( "hinting" ) ) {
        hinting = hint_to_fonthint( jo.get_string( "hinting" ) );
    }
    jo.read( "antialiasing", antialiasing, false );
}

static void load_font_from_config( const JsonObject &config, const std::string &key,
                                   std::vector<font_config> &typefaces )
{

    if( config.has_string( key ) ) {
        std::string path = config.get_string( key );
        // Migrate old font config files. Remove after 0.I
        if( path.find( "Terminus.ttf" ) != std::string::npos ) {
            typefaces.emplace_back( path, ImGuiFreeTypeBuilderFlags_Bitmap );
        }  else if( path.find( "Roboto-Medium.ttf" ) != std::string::npos ) {
            typefaces.emplace_back( path, ImGuiFreeTypeBuilderFlags_LightHinting );
        } else {
            typefaces.emplace_back( path );
        }
    } else if( config.has_object( key ) ) {
        font_config conf;
        if( !config.read( key, conf, false ) ) {
            debugmsg( "Key '%s' should contain a path entry.", key );
        } else {
            typefaces.push_back( conf );
        }
    } else if( config.has_array( key ) ) {
        JsonArray array = config.get_array( key );
        for( JsonValue value : array ) {
            if( value.test_string() ) {
                std::string path = value.get_string();
                // Migrate old font config files. Remove after 0.I
                if( path.find( "Terminus.ttf" ) != std::string::npos ) {
                    typefaces.emplace_back( path, ImGuiFreeTypeBuilderFlags_Bitmap );
                } else if( path.find( "Roboto-Medium.ttf" ) != std::string::npos ) {
                    typefaces.emplace_back( path, ImGuiFreeTypeBuilderFlags_LightHinting );
                } else {
                    typefaces.emplace_back( path );
                }
            } else if( value.test_object() ) {
                font_config conf;
                if( !value.read( conf, false ) ) {
                    debugmsg( "Key '%s' has an invalid array entry in the font config file.", key );
                } else {
                    typefaces.push_back( conf );
                }
            } else {
                // Value is neither a string nor an object.
                debugmsg( "Invalid font entry for key '%s'. Font array entries should be objects or strings.",
                          key );
            }
        }
    } else {
        debugmsg( "Font specifiers must be an array, object, or string." );
    }

    ensure_unifont_loaded( typefaces );
}


void font_loader::load_throws( const cata_path &path )
{
    try {
        JsonValue json = json_loader::from_path( path );
        JsonObject config = json.get_object();
        config.allow_omitted_members(); // We do actually visit every member, but over several functions.
        load_font_from_config( config, "typeface", typeface );
        load_font_from_config( config, "gui_typeface", gui_typeface );
        load_font_from_config( config, "map_typeface", map_typeface );
        load_font_from_config( config, "overmap_typeface", overmap_typeface );
    } catch( const std::exception &err ) {
        throw std::runtime_error( std::string( "loading font settings from " ) + path.generic_u8string() +
                                  " failed: " +
                                  err.what() );
    }
}

// Convenience function for font_loader::save. Assumes that this is a member of
// an object.
static void write_font_config( JsonOut &json, const std::vector<font_config> &typefaces )
{
    json.start_array();
    for( const font_config &config : typefaces ) {
        json.start_object();
        json.member( "path", config.path );

        if( config.hinting == std::nullopt ) {
            json.member( "hinting", "Default" );
        } else {
            switch( *config.hinting ) {
                case ImGuiFreeTypeBuilderFlags_ForceAutoHint:
                    json.member( "hinting", "Auto" );
                    break;
                case ImGuiFreeTypeBuilderFlags_Bitmap:
                    json.member( "hinting", "Bitmap" );
                    break;
                case ImGuiFreeTypeBuilderFlags_LightHinting:
                    json.member( "hinting", "Light" );
                    break;
                case ImGuiFreeTypeBuilderFlags_NoHinting:
                    json.member( "hinting", "NoAuto" );
                    break;
                default:
                    // This should never be reached.
                    json.member( "hinting", "Default" );
                    break;
            }
        }

        if( !config.antialiasing ) {
            json.member( "antialiasing", false );
        }

        json.end_object();
    }
    json.end_array();
}

void font_loader::save( const cata_path &path ) const
{
    try {
        write_to_file( path, [&]( std::ostream & stream ) {
            JsonOut json( stream, true ); // pretty-print
            json.start_object();
            json.member( "//", "See docs/user-guides/FONT_OPTIONS.md for an explanation of this file." );
            json.member( "typeface" );
            write_font_config( json, typeface );
            json.member( "gui_typeface" );
            write_font_config( json, gui_typeface );
            json.member( "map_typeface" );
            write_font_config( json, map_typeface );
            json.member( "overmap_typeface" );
            write_font_config( json, overmap_typeface );
            json.end_object();
            stream << "\n";
        } );
    } catch( const std::exception &err ) {
        DebugLog( D_ERROR, D_SDL ) << "saving font settings to " << path << " failed: " << err.what();
    }
}

void font_loader::load()
{
    const cata_path fontdata = PATH_INFO::fontdata();
    if( file_exist( fontdata ) ) {
        load_throws( fontdata );
        // Migrate old font files to the new format.
        // Remove after 0.I.
        save( fontdata );
    } else {
        const cata_path legacy_fontdata = PATH_INFO::legacy_fontdata();
        load_throws( legacy_fontdata );
        assure_dir_exist( PATH_INFO::config_dir() );
        save( fontdata );
    }
}

#endif // TILES
