#include "overmap_location.h"

#include "generic_factory.h"
#include "omdata.h"
#include "rng.h"

#include <algorithm>
#include <unordered_map>

using overmap_location_str_id = string_id<overmap_location>;

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

template<>
inline bool int_id<overmap_location>::is_valid() const
{
    return locations.is_valid( *this );
}

template<>
const overmap_location &int_id<overmap_location>::obj() const
{
    return locations.obj( *this );
}

template<>
const string_id<overmap_location> &int_id<overmap_location>::id() const
{
    return locations.convert( *this );
}

template<>
int_id<overmap_location>::int_id( const string_id<overmap_location> &id ) : _id( id.id() )
{
}

template<>
int_id<overmap_location> string_id<overmap_location>::id() const
{
    return locations.convert( *this, overmap_location_str_id::NULL_ID() );
}

bool overmap_location::test( const int_id<oter_t> &oter ) const
{
    return std::any_of( terrains.cbegin(), terrains.cend(),
    [ &oter ]( const oter_type_str_id & type ) {
        return oter->type_is( type );
    } );
}

oter_type_id overmap_location::get_random_terrain() const
{
    return random_entry( terrains );
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
