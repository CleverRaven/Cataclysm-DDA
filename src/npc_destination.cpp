#include "npc_destination.h"
#include "int_id.h"
#include "debug.h"
#include "rng.h"
#include "generic_factory.h"

generic_factory<npc_destination> npc_destination_factory( "npc_destination" );

/** @relates string_id */
template<>
const npc_destination &string_id<npc_destination>::obj() const
{
    return npc_destination_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<npc_destination>::is_valid() const
{
    return npc_destination_factory.is_valid( *this );
}

npc_destination::npc_destination() : id( "need_none" )
{
}

npc_destination::npc_destination( std::string npc_destination_id ) : id( npc_destination_id )
{
}

void npc_destination::load_npc_destination( JsonObject &jo, const std::string &src )
{
    npc_destination_factory.load( jo, src );
}

const std::vector<npc_destination> &npc_destination::get_all()
{
    return npc_destination_factory.get_all();
}

const std::string npc_destination::get_random_destination_terrain( const npc_need need )
{
    std::string ret_value;
    const std::vector<npc_destination> dests = npc_destination::get_all();
    for( const auto &dest : dests ) {
        if( need_id( need ) == dest.id.str() ) {
            ret_value = random_entry( dest.destination_terrains ).str();
            break;
        }
    }
    return ret_value;
}

void npc_destination::reset_npc_destinations()
{
    npc_destination_factory.reset();
}

void npc_destination::check_consistency()
{
    for( auto &d : npc_destination_factory.get_all() ) {
        if( d.destination_terrains.empty() ) {
            debugmsg( "NPC destination \"%s\" doesn't have `destination_terrains` specified.", d.id.c_str() );
        } else {
            for( const auto &t : d.destination_terrains ) {
                if( !t.is_valid() ) {
                    debugmsg( "NPC destination \"%s\" contains invalid terrain \"%s\" in `destination_terrains`.",
                              d.id.c_str(), t.c_str() );
                }
            }
        }
    }
}

void npc_destination::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "destination_terrains", destination_terrains,
               string_id_reader<oter_type_t> {} );
}
