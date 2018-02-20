#include "npc_destination.h"
#include "debug.h"
#include "rng.h"
#include "generic_factory.h"
//#include "omdata.h"

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

void npc_destination::load_npc_destination( JsonObject &jo, const std::string &src )
{
    npc_destination_factory.load( jo, src );
}

const std::vector<npc_destination> &npc_destination::get_all()
{
    return npc_destination_factory.get_all();
}

void npc_destination::reset_npc_destinations()
{
    npc_destination_factory.reset();
}

void npc_destination::finalize_all()
{
    for( auto &d_const : npc_destination_factory.get_all() ) {
        auto &d = const_cast<npc_destination &>( d_const );
        // TODO: Compare terrain city_size and CITY_SIZE option here?
    }
}

void npc_destination::check_consistency()
{
    for( auto &destination : npc_destination_factory.get_all() ) {
        for( const auto &terrain : destination.terrains ) {
            if( !terrain.is_valid() ) {
                debugmsg( "In NPC destination \"%s\", terrain \"%s\" is invalid.", destination.id.c_str(), terrain.c_str() );
            }
        }
    }
}

void npc_destination::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "terrains", terrains );

    if( terrains.empty() ) {
        jo.throw_error( "At least one destination terrain must be specified." );
    }
}
