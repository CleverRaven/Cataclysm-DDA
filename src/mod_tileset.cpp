#include "mod_tileset.h"

#include <algorithm>
#include <memory>

#include "json.h"

std::vector<mod_tileset> all_mod_tilesets;

void load_mod_tileset( const JsonObject &jsobj, const std::string &, const std::string &base_path,
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
    if( jsobj.has_member( "tiles-new" ) ) {
        // tiles-new is read when initializing graphics, inside `tileset_loader::load`.
        // calling get_array here to suppress warnings in the unit test.
        jsobj.get_array( "tiles-new" );
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
