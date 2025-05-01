#include "mod_tileset.h"

#include <algorithm>
#include <string>

#include "flexbuffer_json.h"

std::vector<mod_tileset> all_mod_tilesets;

void load_mod_tileset( const JsonObject &jsobj, std::string_view, const cata_path &base_path,
                       const cata_path &full_path )
{
    // This function only checks whether mod tileset is compatible.
    // Actual sprites are loaded when the main tileset is loaded.
    // As such, most JSON members are skipped here.
    jsobj.allow_omitted_members();

    int new_num_in_file = 1;
    // Check mod tileset num in file
    for( const mod_tileset &mts : all_mod_tilesets ) {
        if( mts.get_full_path() == full_path ) {
            new_num_in_file++;
        }
    }

    all_mod_tilesets.emplace_back( base_path, full_path, new_num_in_file );
    std::vector<std::string> compatibility = jsobj.get_string_array( "compatibility" );
    for( const std::string &compatible_tileset_id : compatibility ) {
        all_mod_tilesets.back().add_compatible_tileset( compatible_tileset_id );
    }
}

void reset_mod_tileset()
{
    all_mod_tilesets.clear();
}

bool mod_tileset::is_compatible( const std::string &tileset_id ) const
{
    const auto iter = std::find( compatibility.begin(), compatibility.end(), tileset_id );
    return iter != compatibility.end();
}

void mod_tileset::add_compatible_tileset( const std::string &tileset_id )
{
    compatibility.push_back( tileset_id );
}
