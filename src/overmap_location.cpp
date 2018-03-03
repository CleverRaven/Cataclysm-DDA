#include "overmap_location.h"

#include "generic_factory.h"
#include "omdata.h"
#include "rng.h"

#include <algorithm>
#include <unordered_map>

namespace
{

generic_factory<overmap_location> locations( "overmap location" );

}

template<>
bool string_id<overmap_location>::is_valid() const
{
    return locations.is_valid( *this );
}

template<>
const overmap_location &string_id<overmap_location>::obj() const
{
    return locations.obj( *this );
}

bool overmap_location::test( const int_id<oter_t> &oter ) const
{
    return std::any_of( terrains.cbegin(), terrains.cend(),
    [ &oter ]( const string_id<oter_type_t> &type ) {
        return oter->type_is( type );
    } );
}

void overmap_location::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "terrains", terrains );

    if( terrains.empty() ) {
        jo.throw_error( "At least one terrain must be specified." );
    }
}

void overmap_location::check() const
{
    for( const auto &element : terrains ) {
        if( !element.is_valid() ) {
            debugmsg( "In overmap location \"%s\", terrain \"%s\" is invalid.", id.c_str(), element.c_str() );
        }
    }
}

const string_id<oter_type_t> overmap_location::get_random_terrain() const
{
    return random_entry( terrains );
}

void overmap_locations::load( JsonObject &jo, const std::string &src )
{
    locations.load( jo, src );
}

void overmap_locations::check_consistency()
{
    locations.check();
}

void overmap_locations::reset()
{
    locations.reset();
}

const std::vector<overmap_location> &overmap_locations::get_all()
{
    return locations.get_all();
}

const std::string overmap_locations::get_random_terrain( const std::string target_location_id )
{
    std::string random_terrain;
    const std::vector<overmap_location> locs = overmap_locations::get_all();
    for( const auto &loc : locs ) {
        if( target_location_id == loc.id.str() ) {
            random_terrain = loc.get_random_terrain().str();
            break;
        }
    }
    return random_terrain;
}
