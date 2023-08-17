#include "overmap_connection.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>

#include "cata_assert.h"
#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "overmap_location.h"

namespace
{

generic_factory<overmap_connection> connections( "overmap connection" );

} // namespace

static const std::unordered_map<std::string, overmap_connection::subtype::flag>
connection_subtype_flag_map = {
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

void overmap_connection::subtype::load( const JsonObject &jo )
{
    const auto flag_reader =
        make_flag_reader( connection_subtype_flag_map, "connection subtype flag" );
        
    if( !jo.has_string( "terrain" ) ) {
        JsonObject jio = jo.get_object( "terrain" ) ) {
            if( jio.test_string() ) {
                terrain.first = ( jio.get_string() );
                terrain.second = ot_match_type::type;
            } else {
                JsonObject pairobj = jio.get_object();
                terrain.first = ( pairobj.get_string( "om_terrain" ) );
                terrain.second = pairobj.get_enum_value<ot_match_type>( "om_terrain_match_type",
                                      ot_match_type::contains );
            }
        }
    } else {
        terrain.first = jo.get_string( "terrain" );
        terrain.second = ot_match_type::type;
    }
    
    mandatory( jo, false, "locations", locations );

    optional( jo, false, "basic_cost", basic_cost, 0 );
    optional( jo, false, "flags", flags, flag_reader );
}

void overmap_connection::subtype::deserialize( const JsonObject &jo )
{
    load( jo );
}

const overmap_connection::subtype *overmap_connection::pick_subtype_for(
    const int_id<oter_t> &ground ) const
{
    if( !ground ) {
        return nullptr;
    }

    const size_t cache_index = ground.to_i();
    cata_assert( cache_index < cached_subtypes.size() );

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

void overmap_connection::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, false, "subtypes", subtypes );

    for( const JsonValue entry : jo.get_array( "origin" ) ) {

        std::pair<std::string, ot_match_type> origin_terrain_partial;
        std::pair<std::pair<std::string, ot_match_type>, unsigned int> origin_terrain;
        
        if( entry.test_string() ) {
            if ( entry == "city" ) {
                has_city_origin = true;
            } else {
                entry.throw_error( "Terrains should be in an object" );
            }
        } else {
            origin_terrain_partial.first = ( entry.get_string( "om_terrain" ) );
            if( entry.has_string( "om_terrain_match_type" ) ) {
            origin_terrain_partial.second = entry.get_enum_value<ot_match_type>( "om_terrain_match_type",
                                    ot_match_type::type );
            } else {
                origin_terrain_partial.second = ot_match_type::type;
            }
            origin_terrain.first = origin_terrain_partial;
            if ( entry.has_string( "max_distance" ) ) {
                origin_terrain.second = static_cast<unsigned int>( entry.get_string( "max_distance" ) );
            } else {
                origin_terrain.second = 100;
            }
            origin_terrains.insert( origin_terrain );
        }
    }
}

void overmap_connection::check() const
{
    if( subtypes.empty() ) {
        debugmsg( "Overmap connection \"%s\" doesn't have subtypes.", id.c_str() );
    }
    for( const overmap_connection::subtype &subtype : subtypes ) {
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

void overmap_connections::load( const JsonObject &jo, const std::string &src )
{
    connections.load( jo, src );
}

void overmap_connections::finalize()
{
    connections.finalize();
    for( const overmap_connection &elem : connections.get_all() ) {
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
