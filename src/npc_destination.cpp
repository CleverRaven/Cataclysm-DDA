#include "npc_destination.h"
#include "debug.h"
#include "rng.h"
#include "generic_factory.h"
#include "overmap.h"

struct oter_t;
using oter_id = int_id<oter_t>;
using oter_str_id = string_id<oter_t>;

/*

template<>
bool string_id<oter_t>::is_valid() const
{
    return terrains.is_valid( *this );
}

template<>
bool oter_str_id::is_valid() const
{
    return terrains.is_valid( *this );
}
*/

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

void npc_destination::reset_npc_destinations()
{
    npc_destination_factory.reset();
}

void npc_destination::check_consistency()
{
    for( auto &d : npc_destination_factory.get_all() ) {
        DebugLog( D_INFO, DC_ALL ) << "npc_destination::check_consistency() : npc_destination is [" << d.id.c_str() << "] with size of [" << d.destination_terrains.size() << "]";
        if( d.destination_terrains.empty() ) {
            debugmsg( "NPC destination \"%s\" doesn't have destination_terrains specified.", d.id.c_str() );
        } else {
            for( const auto &t : d.destination_terrains ) {
                const oter_id oter( t );
                if( !oter.is_valid() ) {
                    debugmsg( "NPC destination \"%s\", contains invalid terrain \"%s\".", d.id.c_str(), t.c_str() );
                } else {
                    DebugLog( D_INFO, DC_ALL ) << "npc_destination::check_consistency() : destination_terrains contains terrain [" << t.c_str() << "]";
                }
            }
        }
    }
}

void npc_destination::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "destination_terrains", destination_terrains );
}

std::string npc_destination::get_random_dest()
{
    std::string return_value = random_entry( destination_terrains );
    DebugLog( D_INFO, DC_ALL ) << "npc_destination::get_random_dest() with: [" << id.c_str() << "] going to random_entry of: [" << return_value.c_str() << "]";
    return return_value;
}
