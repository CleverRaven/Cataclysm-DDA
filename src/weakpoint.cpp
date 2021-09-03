#include "weakpoint.h"

#include <string>
#include <utility>

#include "assign.h"
#include "creature.h"
#include "damage.h"
#include "rng.h"

class JsonArray;
class JsonObject;

weakpoint::weakpoint()
{
    // arrays must be filled manually to avoid UB.
    armor_mult.fill( 1.0f );
    armor_offset.fill( 0.0f );
}

void weakpoint::load( const JsonObject &obj )
{
    assign( jo, "id", id );
    assign( jo, "name", name );
    assign( jo, "coverage", 0.0f, 1.0f );
    if( jo.has_object( "armor_mult" ) ) {
        armor_mult = load_damage_array( jo.get_object( "armor_mult" ), 1.0f );
    }
    if( jo.has_object( "armor_offset" ) ) {
        armor_mult = load_damage_array( jo.get_object( "armor_offset" ), 0.0f );
    }
    // Set the ID to the name, if not provided.
    if( id.empty() ) {
        id = name;
    }
}

void weakpoint::apply_to( resistances &resistances )
{
    for( damage_type d = damage_type::None; d < damage_type::NUM; ++d ) {
        int idx = static_cast<int>( d );
        float r = resistances.get_resist( d );
        resistances.set_resist( armor_mult[idx] * r + armor_offset[idx] );
    }
}

float hit_chance( Creature */*source*/ ) const
{
    return coverage;
}

weakpoint *weakpoints::select_weakpoint( Creature *source ) const
{
    float idx = rng_float( 0.0f, 1.0f );
    for( const weakpoint &weakpoint : weakpoints ) {
        float hit_chance = weakpoint.hit_chance( source );
        if( hit_chance <= idx ) {
            return &weakpoint;
        }
        idx -= hit_chance;
    }
    return &default_weakpoint;
}

void weakpoints::load( const JsonArray &ja )
{
    while( ja.has_more() ) {
        weakpoint tmp;
        tmp.load( ja.next_object );
        weakpoints.push_back( std::move( tmp ) );
    }
}
