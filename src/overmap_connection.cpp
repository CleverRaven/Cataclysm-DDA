#include "overmap_connection.h"

#include <algorithm>
#include <cassert>

#include "generic_factory.h"
#include "json.h"
#include "overmap_location.h"

namespace
{

generic_factory<overmap_connection> connections( "overmap connection" );

}

static const std::map<std::string, overmap_connection::subtype::flag> connection_subtype_flag_map
= {
    { "ORTHOGONAL", overmap_connection::subtype::flag::orthogonal },
};

template<>
bool string_id<overmap_connection>::is_valid() const
{
    return connections.is_valid( *this );
}

template<>
const overmap_connection &string_id<overmap_connection>::obj() const
{
    return connections.obj( *this );
}

bool overmap_connection::subtype::allows_terrain( const int_id<oter_t> &oter ) const
{
    if( oter->type_is( terrain ) ) {
        return true;    // Can be built on similar terrains.
    }

    return std::any_of( locations.cbegin(),
    locations.cend(), [&oter]( const string_id<overmap_location> &elem ) {
        return elem->test( oter );
    } );
}

void overmap_connection::subtype::load( JsonObject &jo )
{
    static const typed_flag_reader<decltype( connection_subtype_flag_map )> flag_reader{ connection_subtype_flag_map, "invalid connection subtype flag" };

    mandatory( jo, false, "terrain", terrain );
    mandatory( jo, false, "locations", locations );

    optional( jo, false, "basic_cost", basic_cost, 0 );
    optional( jo, false, "flags", flags, flag_reader );
}

void overmap_connection::subtype::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    load( jo );
}

const overmap_connection::subtype *overmap_connection::pick_subtype_for(
    const int_id<oter_t> &ground ) const
{
    if( !ground ) {
        return nullptr;
    }

    const size_t cache_index = ground;
    assert( cache_index < cached_subtypes.size() );

    if( cached_subtypes[cache_index] ) {
        return cached_subtypes[cache_index].value;
    }

    const auto iter = std::find_if( subtypes.cbegin(),
    subtypes.cend(), [&ground]( const subtype & elem ) {
        return elem.allows_terrain( ground );
    } );

    const overmap_connection::subtype *result = iter != subtypes.cend() ? &*iter : nullptr;

    cached_subtypes[cache_index].value = result;
    cached_subtypes[cache_index].assigned = true;

    return result;
}

bool overmap_connection::has( const int_id<oter_t> &oter ) const
{
    return std::find_if( subtypes.cbegin(), subtypes.cend(), [&oter]( const subtype & elem ) {
        return oter->type_is( elem.terrain );
    } ) != subtypes.cend();
}

void overmap_connection::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, false, "subtypes", subtypes );
}

void overmap_connection::check() const
{
    if( subtypes.empty() ) {
        debugmsg( "Overmap connection \"%s\" doesn't have subtypes.", id.c_str() );
    }
    for( const auto &subtype : subtypes ) {
        if( !subtype.terrain.is_valid() ) {
            debugmsg( "In overmap connection \"%s\", terrain \"%s\" is invalid.", id.c_str(),
                      subtype.terrain.c_str() );
        }
        for( const auto &location : subtype.locations ) {
            if( !location.is_valid() ) {
                debugmsg( "In overmap connection \"%s\", location \"%s\" is invalid.", id.c_str(),
                          location.c_str() );
            }
        }
    }
}

void overmap_connection::finalize()
{
    cached_subtypes.resize( overmap_terrains::get_all().size() );
}

void overmap_connections::load( JsonObject &jo, const std::string &src )
{
    connections.load( jo, src );
}

void overmap_connections::finalize()
{
    connections.finalize();
    for( const auto &elem : connections.get_all() ) {
        const_cast<overmap_connection &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void overmap_connections::check_consistency()
{
    connections.check();
}

void overmap_connections::reset()
{
    connections.reset();
}

string_id<overmap_connection> overmap_connections::guess_for( const int_id<oter_t> &oter_id )
{
    const auto &all = connections.get_all();
    const auto iter = std::find_if( all.cbegin(),
    all.cend(), [&oter_id]( const overmap_connection & elem ) {
        return elem.pick_subtype_for( oter_id ) != nullptr;
    } );

    return iter != all.cend() ? iter->id : string_id<overmap_connection>::NULL_ID();
}

string_id<overmap_connection> overmap_connections::guess_for( const int_id<oter_type_t> &oter_id )
{
    return guess_for( oter_id->get_first() );
}
