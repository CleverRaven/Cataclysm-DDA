#include "weakpoint.h"

#include <algorithm>
#include <cmath>
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

static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_throw( "throw" );
static const skill_id skill_unarmed( "unarmed" );

class JsonArray;
class JsonObject;

float monster::weakpoint_skill()
{
    return type->melee_skill;
}

float Character::melee_weakpoint_skill( const item &weapon )
{
    skill_id melee_skill = weapon.is_null() ? skill_unarmed : weapon.melee_skill();
    float skill = ( get_skill_level( skill_melee ) + get_skill_level( melee_skill ) ) / 2.0;
    float stat = ( get_dex() - 8 ) / 8.0 + ( get_per() - 8 ) / 8.0;
    return skill + stat;
}

float Character::ranged_weakpoint_skill( const item &weapon )
{
    float skill = ( get_skill_level( skill_gun ) + get_skill_level( weapon.gun_skill() ) ) / 2.0;
    float stat = ( get_dex() - 8 ) / 8.0 + ( get_per() - 8 ) / 8.0;
    return skill + stat;
}

float Character::throw_weakpoint_skill()
{
    float skill = get_skill_level( skill_throw );
    float stat = ( get_dex() - 8 ) / 8.0 + ( get_per() - 8 ) / 8.0;
    return skill + stat;
}

weakpoint_attack::weakpoint_attack()  :
    source( nullptr ),
    weapon( &null_item_reference() ),
    is_melee( false ),
    wp_skill( 0.0f ) {}


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

// Reweighs the probability distribution of hitting a weakpoint.
//
// The value returned is the probability of sampling at least one value less than `base`
// from `rolls` uniform distributions over [0, 1].
//
// For hard to hit weak points, this multiplies their hit chance by `rolls`. A 1% base
// combined with 5 rolls has a 4.9% weight. However, even at high `rolls`, the attacker
// still has a chance of hitting armor. A 75% chance of not hitting a weak point becomes
// a 23.7% chance with 5 rolls.
static float reweigh( float base, float rolls )
{
    return 1.0f - pow( 1.0f - base, rolls );
}

const weakpoint *weakpoints::select_weakpoint( const weakpoint_attack &attack ) const
{
    add_msg_debug( debugmode::DF_MONSTER,
                   "Weakpoint Selection: Source: %s, Weapon %s, Skill %.3f",
                   attack.source == nullptr ? "nullptr" : attack.source->get_name(),
                   attack.weapon == nullptr ? "nullptr" : attack.weapon->type_name(),
                   attack.wp_skill );
    float rolls = std::max( 1.0f, 1.0f + attack.wp_skill / 2.5f );
    // The base probability of hitting a more preferable weak point.
    float base = 0.0f;
    // The reweighed probability of hitting a more preferable weak point.
    float reweighed = 0.0f;
    float idx = rng_float( 0.0f, 100.0f );
    for( const weakpoint &weakpoint : weakpoint_list ) {
        float new_base = base + weakpoint.hit_chance( attack );
        float new_reweighed = 100.0f * reweigh( new_base / 100.0f, rolls );
        float hit_chance = new_reweighed - reweighed;
        add_msg_debug( debugmode::DF_MONSTER,
                       "Weakpoint Selection: weakpoint %s, hit_chance %.4f",
                       weakpoint.name, hit_chance );
        if( idx < hit_chance ) {
            return &weakpoint;
        }
        base = new_base;
        reweighed = new_reweighed;
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

        if( tmp.id.empty() ) {
            default_weakpoint = tmp;
            continue;
        }

        weakpoint_list.push_back( std::move( tmp ) );
    }
    // Prioritizes weakpoints based on their coverage.
    std::sort( weakpoint_list.begin(), weakpoint_list.end(),
    []( const weakpoint & a, const weakpoint & b ) {
        return a.coverage < b.coverage;
    } );
}
