#include "mod_tileset.h"
#include "json.h"

#include <algorithm>

std::vector<mod_tileset *> all_mod_tilesets;

void load_mod_tileset( JsonObject &jsobj, const std::string &, const std::string &base_path,
                       const std::string &full_path )
{
    // This function didn't loads image data actually, loads when tileset loading.
    load_mod_tileset_into_array( jsobj, base_path, full_path, all_mod_tilesets );
}

void load_mod_tileset_into_array( JsonObject &jsobj, const std::string &base_path,
                                  const std::string &full_path,
                                  std::vector<mod_tileset *> &mod_tileset_array )
{
    // Check duplication
    // Limit one tileset per one json file.
    for( auto mts : mod_tileset_array ) {
        if( mts->get_full_path() == full_path ) {
            return;
        }
    }

    mod_tileset *new_mts = new mod_tileset( base_path, full_path );
    std::vector<std::string> compatibility = jsobj.get_string_array( "compatibility" );
    for( auto compatible_tileset_id : compatibility ) {
        new_mts->add_compatible_tileset( compatible_tileset_id );
    }
    mod_tileset_array.push_back( new_mts );
}

void reset_mod_tileset()
{
    for( mod_tileset *mts : all_mod_tilesets ) {
        delete mts;
    }
    all_mod_tilesets.clear();
}

bool mod_tileset::is_compatible( std::string tileset_id ) const
{
    auto iter = std::find( compatibility.begin(), compatibility.end(), tileset_id );
    if( iter == compatibility.end() ) {
        return false;
    }
    return true;
}

void mod_tileset::add_compatible_tileset( std::string tileset_id )
{
    compatibility.push_back( tileset_id );
}
