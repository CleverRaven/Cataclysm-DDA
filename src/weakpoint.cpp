#include "weakpoint.h"

#include <string>
#include <utility>

#include "assign.h"
#include "creature.h"
#include "damage.h"
#include "item.h"
#include "rng.h"

class JsonArray;
class JsonObject;

weakpoint::weakpoint()
{
    // arrays must be filled manually to avoid UB.
    armor_mult.fill( 1.0f );
    armor_penalty.fill( 0.0f );
}

void weakpoint::load( const JsonObject &jo )
{
    assign( jo, "id", id );
    assign( jo, "name", name );
    assign( jo, "coverage", coverage, false, 0.0f, 100.0f );
    if( jo.has_object( "armor_mult" ) ) {
        armor_mult = load_damage_array( jo.get_object( "armor_mult" ), 1.0f );
    }
    if( jo.has_object( "armor_penalty" ) ) {
        armor_penalty = load_damage_array( jo.get_object( "armor_penalty" ), 0.0f );
    }
    // Set the ID to the name, if not provided.
    if( id.empty() ) {
        id = name;
    }
}

void weakpoint::apply_to( resistances &resistances ) const
{
    for( int i = 0; i < static_cast<int>( damage_type::NUM ); ++i ) {
        resistances.resist_vals[i] *= armor_mult[i];
        resistances.resist_vals[i] -= armor_penalty[i];
    }
}

float weakpoint::hit_chance( Creature * ) const
{
    // TODO: scale the hit chance based on the source's skill / stats
    return coverage;
}

const weakpoint *weakpoints::select_weakpoint( Creature *source ) const
{
    float idx = rng_float( 0.0f, 100.0f );
    for( const weakpoint &weakpoint : weakpoint_list ) {
        float hit_chance = weakpoint.hit_chance( source );
        if( hit_chance <= idx ) {
            return &weakpoint;
        }
        idx -= hit_chance;
    }
    return &default_weakpoint;
}

void weakpoints::clear()
{
    weakpoint_list.clear();
}

void weakpoints::load( const JsonArray &ja )
{
    for( const JsonObject jo : ja ) {
        weakpoint tmp;
        tmp.load( jo );
        weakpoint_list.push_back( std::move( tmp ) );
    }
}
