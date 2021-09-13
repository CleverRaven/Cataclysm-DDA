#include "weakpoint.h"

#include <algorithm>
#include <string>
#include <utility>

#include "assign.h"
#include "character.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "item.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "rng.h"

static const skill_id skill_melee( "melee" );
static const skill_id skill_marksmanship( "marksmanship" );
static const skill_id skill_throwing( "throwing" );

class JsonArray;
class JsonObject;

float monster::weakpoint_skill()
{
    return type->melee_skill;
}

float Character::melee_weakpoint_skill( const item& )
{
    float skill = (get_skill_level(skill_melee) + get_skill_level(item.melee_skill())) / 2.0;
    float stat = (get_dex() - 8) / 8.0 + (get_per() - 8) / 8.0;
    return skill + stat;
}

float Character::range_weakpoint_skill( const item& )
{
    float skill = (get_skill_level(skill_marksmanship) + get_skill_level(item.gun_skill())) / 2.0;
    float stat = (get_dex() - 8) / 8.0 + (get_per() - 8) / 8.0;
    return skill + stat;
}

float Character::throw_weakpoint_skill()
{
    float skill = get_skill_level(skill_throwing);
    float stat = (get_dex() - 8) / 8.0 + (get_per() - 8) / 8.0;
    return skill + stat;
}

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

float weakpoint::hit_chance( const weakpoint_attack & ) const
{
    // TODO: scale the hit chance based on the source's skill / stats
    return coverage;
}

const weakpoint *weakpoints::select_weakpoint( const weakpoint_attack &attack ) const
{
    add_msg_debug( debugmode::DF_MONSTER,
                    "Source: %s :: Weapon %s :: Skill %1.f",
                    attack.source == nullptr ? "nullptr" : attack.source->get_name(),
                    attack.weapon == nullptr ? "nullptr" : attack.weapon->type_name(),
                    attack.wp_skill);
    float idx = rng_float( 0.0f, 100.0f );
    for( const weakpoint &weakpoint : weakpoint_list ) {
        float hit_chance = weakpoint.hit_chance( attack );
        if( idx < hit_chance ) {
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
    std::sort( weakpoint_list.begin(), weakpoint_list.end(),
    []( const weakpoint & a, const weakpoint & b ) {
        return a.coverage < b.coverage;
    } );
}
