#include "npc_destination.h"
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

void npc_destination::reset_npc_destinations()
{
    npc_destination_factory.reset();
}

void npc_destination::check_consistency()
{
    for( auto &d : npc_destination_factory.get_all() ) {
        DebugLog( D_INFO, DC_ALL ) << "npc_destination::check_consistency() : npc_destination is [" << d.id.c_str() << "] with size of [" << d.terrains.size() << "]";
        if( d.terrains.empty() ) {
            debugmsg( "NPC destination \"%s\" doesn't have terrains specified.", d.id.c_str() );
        } else {
            for( const auto &t : d.terrains ) {
                if( !t.is_valid() ) {
                    debugmsg( "NPC destination \"%s\", contains invalid terrain \"%s\".", d.id.c_str(), t.c_str() );
                } else {
                    DebugLog( D_INFO, DC_ALL ) << "npc_destination::check_consistency() : terrains contains terrain [" << t.c_str() << "]";
                }
            }
        }
    }
}

void npc_destination::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "terrains", terrains );
}

std::string npc_destination::get_random_dest()
{
    //std::string a = ""; //terrains[0].obj().id().c_str();
    //std::string b = ""; //terrains[0].c_str();
    //std::string c = ""; //terrains[0].id();
    string_id<oter_type_t> &d = terrains[0];
    //DebugLog( D_INFO, DC_ALL ) << "first record in terrains is: [" << a.c_str() << "] or [" << b.c_str() << "] or [" << c.c_str() << "] or [" << d.c_str() << "]";
    std::string return_value = d->get_type_id().str();
    //std::string return_value = random_entry( terrains ).str();
    DebugLog( D_INFO, DC_ALL ) << "npc_destination::get_random_dest() with: [" << id.c_str() << "] going to random_entry of: [" << return_value.c_str() << "]";
    return return_value;
}
