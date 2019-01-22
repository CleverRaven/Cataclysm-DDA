#include "mod_tileset.h"

#include <algorithm>
#include <iterator>

#include "json.h"

std::vector<mod_tileset> all_mod_tilesets;

void load_mod_tileset( JsonObject &jsobj, const std::string &, const std::string &base_path,
                       const std::string &full_path )
{
    // This function didn't loads image data actually, loads when tileset loading.
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
    auto iter = std::find( compatibility.begin(), compatibility.end(), tileset_id );
    if( iter == compatibility.end() ) {
        return false;
    }
    return true;
}

void mod_tileset::add_compatible_tileset( const std::string &tileset_id )
{
    compatibility.push_back( tileset_id );
}
