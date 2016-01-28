#include <vector>

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "monster.h"
#include "mattack_actors.h"
#include "messages.h"
#include "translations.h"
#include "sounds.h"

const efftype_id effect_bite( "bite" );
const efftype_id effect_infected( "infected" );

// Simplified version of the function in monattack.cpp
bool is_adjacent( const monster &z, const Creature &target )
{
    if( rl_dist( z.pos(), target.pos() ) != 1 ) {
        return false;
    }

    return z.posz() == target.posz();
}

// Modified version of the function on monattack.cpp
bool dodge_check( int max_accuracy, Creature &target )
{
    ///\EFFECT_DODGE increases chance of dodging special attacks of monsters
    int dodge = std::max( target.get_dodge() - rng( 0, max_accuracy ), 0L );
    if( rng( 0, 10000 ) < 10000 / ( 1 + ( 99 * exp( -0.6f * dodge ) ) ) ) {
        return true;
    }

    return false;
}

void leap_actor::load( JsonObject &obj )
{
    // Mandatory:
    max_range = obj.get_float( "max_range" );
    // Optional:
    min_range = obj.get_float( "min_range", 1.0f );
    allow_no_target = obj.get_bool( "allow_no_target", true );
    move_cost = obj.get_int( "move_cost", 150 );
    min_consider_range = obj.get_float( "min_consider_range", 0.0f );
    max_consider_range = obj.get_float( "max_consider_range", 200.0f );
}

mattack_actor *leap_actor::clone() const
{
    return new leap_actor( *this );
}

bool leap_actor::call( monster &z ) const
{
    if( !z.can_act() ) {
        return false;
    }

    std::vector<tripoint> options;
    tripoint target = z.move_target();
    float best_float = trig_dist( z.pos(), target );
    if( best_float < min_consider_range || best_float > max_consider_range ) {
        return false;
    }

    // We wanted the float for range check
    // int here will make the jumps more random
    int best = ( int )best_float;
    if( !allow_no_target && z.attack_target() == nullptr ) {
        return false;
    }

    for( const tripoint &dest : g->m.points_in_radius( z.pos(), max_range ) ) {
        if( dest == z.pos() ) {
            continue;
        }
        if( !z.sees( dest ) ) {
            continue;
        }
        if( !g->is_empty( dest ) ) {
            continue;
        }
        int cur_dist = rl_dist( target, dest );
        if( cur_dist > best ) {
            continue;
        }
        if( trig_dist( z.pos(), dest ) < min_range ) {
            continue;
        }
        bool blocked_path = false;
        // check if monster has a clear path to the proposed point
        std::vector<tripoint> line = g->m.find_clear_path( z.pos(), dest );
        for( auto &i : line ) {
            if( g->m.impassable( i ) ) {
                blocked_path = true;
                break;
            }
        }
        if( blocked_path ) {
            continue;
        }

        if( cur_dist < best ) {
            // Better than any earlier one
            options.clear();
        }

        options.push_back( dest );
        best = cur_dist;
    }

    if( options.empty() ) {
        return false;    // Nowhere to leap!
    }

    z.moves -= move_cost;
    const tripoint chosen = random_entry( options );
    bool seen = g->u.sees( z ); // We can see them jump...
    z.setpos( chosen );
    seen |= g->u.sees( z ); // ... or we can see them land
    if( seen ) {
        add_msg( _( "The %s leaps!" ), z.name().c_str() );
    }

    return true;
}

bite_actor::bite_actor()
{
    damage_max_instance = damage_instance::physical( 9, 0, 0, 0 );
    min_mul = 0.5f;
    max_mul = 1.0f;
    move_cost = 100;
}

void bite_actor::load( JsonObject &obj )
{
    // Optional:
    if( obj.has_array( "damage_max_instance" ) ) {
        JsonArray arr = obj.get_array( "damage_max_instance" );
        damage_max_instance = load_damage_instance( arr );
    } else if( obj.has_object( "damage_max_instance" ) ) {
        damage_max_instance = load_damage_instance( obj );
    }

    min_mul = obj.get_float( "min_mul", 0.0f );
    max_mul = obj.get_float( "max_mul", 1.0f );
    move_cost = obj.get_int( "move_cost", 100 );
    accuracy = obj.get_int( "accuracy", INT_MIN );
    no_infection_chance = obj.get_int( "no_infection_chance", 14 );
}

bool bite_actor::call( monster &z ) const
{
    if( !z.can_act() ) {
        return false;
    }

    Creature *target = z.attack_target();
    if( target == nullptr || !is_adjacent( z, *target ) ) {
        return false;
    }

    z.moves -= move_cost;
    bool uncanny = target->uncanny_dodge();
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    int acc = accuracy > INT_MIN ? accuracy : z.type->melee_skill;
    if( uncanny || dodge_check( acc, *target ) ) {
        auto msg_type = target == &g->u ? m_warning : m_info;
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, _( "The %s lunges at you, but you dodge!" ),
                                       _( "The %s lunges at <npcname>, but they dodge!" ),
                                       z.name().c_str() );
        if( !uncanny ) {
            target->on_dodge( &z, z.type->melee_skill * 2 );
        }
        return true;
    }

    body_part hit = target->get_random_body_part();
    float multiplier = rng_float( min_mul, max_mul );
    // Copy the damage instance for the attack
    damage_instance di = damage_max_instance;
    di.mult_damage( multiplier );
    int dam = target->deal_damage( &z, hit, di ).total_damage();

    if( dam > 0 ) {
        auto msg_type = target == &g->u ? m_bad : m_info;
        //~ 1$s is monster name, 2$s bodypart in accusative
        if( target->is_player() ) {
            sfx::play_variant_sound( "mon_bite", "bite_hit", sfx::get_heard_volume( z.pos() ),
                                     sfx::get_heard_angle( z.pos() ) );
            sfx::do_player_death_hurt( *dynamic_cast<player *>( target ), 0 );
        }
        target->add_msg_player_or_npc( msg_type,
                                       _( "The %1$s bites your %2$s!" ),
                                       _( "The %1$s bites <npcname>'s %2$s!" ),
                                       z.name().c_str(),
                                       body_part_name_accusative( hit ).c_str() );
        if( one_in( no_infection_chance - dam ) ) {
            if( target->has_effect( effect_bite, hit ) ) {
                target->add_effect( effect_bite, 400, hit, true );
            } else if( target->has_effect( effect_infected, hit ) ) {
                target->add_effect( effect_infected, 250, hit, true );
            } else {
                target->add_effect( effect_bite, 1, hit, true );
            }
        }
    } else {
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( _( "The %1$s bites your %2$s, but fails to penetrate armor!" ),
                                       _( "The %1$s bites <npcname>'s %2$s, but fails to penetrate armor!" ),
                                       z.name().c_str(),
                                       body_part_name_accusative( hit ).c_str() );
    }

    target->on_hit( &z, hit, z.type->melee_skill );

    return true;
}

mattack_actor *bite_actor::clone() const
{
    return new bite_actor( *this );
}
