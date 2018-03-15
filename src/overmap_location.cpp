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

/** @relates string_id */
template<>
inline bool int_id<overmap_location>::is_valid() const
{
    return locations.is_valid( *this );
}

/** @relates int_id */
template<>
const overmap_location &int_id<overmap_location>::obj() const
{
    return locations.obj( *this );
}

/** @relates int_id */
template<>
const string_id<overmap_location> &int_id<overmap_location>::id() const
{
    return locations.convert( *this );
}

/** @relates string_id */
template<>
int_id<overmap_location> string_id<overmap_location>::id() const
{
    return locations.convert( *this, string_id<overmap_location>::NULL_ID() );
}

/** @relates string_id */
template<>
int_id<overmap_location>::int_id( const string_id<overmap_location> &id ) : _id( id.id() )
{
}

/** @relates string_id */
template<>
const overmap_location &string_id<overmap_location>::obj() const
{
    return locations.obj( *this );
}

/** @relates int_id */
template<>
bool string_id<overmap_location>::is_valid() const
{
    return locations.is_valid( *this );
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

int_id<overmap_location> overmap_location_null;

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

void overmap_locations::finalize()
{
    overmap_location_null = string_id<overmap_location>::NULL_ID().id();
}

void overmap_locations::reset()
{
    locations.reset();
}
