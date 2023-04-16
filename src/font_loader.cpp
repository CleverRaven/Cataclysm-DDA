#include "font_loader.h"

#include "json_loader.h"

// Ensure that unifont is always loaded as a fallback font to prevent users from shooting themselves in the foot
void ensure_unifont_loaded( std::vector<std::string> &font_list )
{
    const std::string unifont = PATH_INFO::fontdir() + "unifont.ttf";
    if( std::find( font_list.begin(), font_list.end(), unifont ) == font_list.end() ) {
        font_list.emplace_back( unifont );
    }
}

void font_loader::load_throws( const cata_path &path )
{
    try {
        JsonValue json = json_loader::from_path( path );
        JsonObject config = json.get_object();
        if( config.has_string( "typeface" ) ) {
            typeface.emplace_back( config.get_string( "typeface" ) );
        } else {
            config.read( "typeface", typeface );
        }
        if( config.has_string( "map_typeface" ) ) {
            map_typeface.emplace_back( config.get_string( "map_typeface" ) );
        } else {
            config.read( "map_typeface", map_typeface );
        }
        if( config.has_string( "overmap_typeface" ) ) {
            overmap_typeface.emplace_back( config.get_string( "overmap_typeface" ) );
        } else {
            config.read( "overmap_typeface", overmap_typeface );
        }

        ensure_unifont_loaded( typeface );
        ensure_unifont_loaded( map_typeface );
        ensure_unifont_loaded( overmap_typeface );

    } catch( const std::exception &err ) {
        throw std::runtime_error( std::string( "loading font settings from " ) + path.generic_u8string() +
                                  " failed: " +
                                  err.what() );
    }
}

void font_loader::save( const cata_path &path ) const
{
    try {
        write_to_file( path, [&]( std::ostream & stream ) {
            JsonOut json( stream, true ); // pretty-print
            json.start_object();
            json.member( "typeface", typeface );
            json.member( "map_typeface", map_typeface );
            json.member( "overmap_typeface", overmap_typeface );
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
    } else {
        const cata_path legacy_fontdata = PATH_INFO::legacy_fontdata();
        load_throws( legacy_fontdata );
        assure_dir_exist( PATH_INFO::config_dir() );
        save( fontdata );
    }
}
