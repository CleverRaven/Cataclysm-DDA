#include <vector>

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "monster.h"
#include "mattack_actors.h"
#include "messages.h"
#include "translations.h"
#include "sounds.h"
#include "npc.h"
#include "debug.h"

const efftype_id effect_bite( "bite" );
const efftype_id effect_infected( "infected" );
const efftype_id effect_laserlocked( "laserlocked" );
const efftype_id effect_was_laserlocked( "was_laserlocked" );
const efftype_id effect_targeted( "targeted" );

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

    z.mod_moves( -move_cost );

    add_msg( m_debug, "%s attempting to bite %s", z.name().c_str(), target->disp_name().c_str() );

    int hitspread = target->deal_melee_attack( &z, z.hit_roll() );

    if( hitspread < 0 ) {
        auto msg_type = target == &g->u ? m_warning : m_info;
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, _( "The %s lunges at you, but you dodge!" ),
                                       _( "The %s lunges at <npcname>, but they dodge!" ),
                                       z.name().c_str() );
        return true;
    }

    damage_instance damage = damage_max_instance;
    dealt_damage_instance dealt_damage;
    body_part hit;

    double multiplier = rng_float( min_mul, max_mul );
    damage.mult_damage( multiplier );

    target->deal_melee_hit( &z, hitspread, false, damage, dealt_damage );

    hit = dealt_damage.bp_hit;
    int damage_total = dealt_damage.total_damage();
    add_msg( m_debug, "%s's bite did %d damage", z.name().c_str(), damage_total );
    if( damage_total > 0 ) {
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
        if( one_in( no_infection_chance - damage_total ) ) {
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

    return true;
}

mattack_actor *bite_actor::clone() const
{
    return new bite_actor( *this );
}

gun_actor::gun_actor()
{
}

void gun_actor::load( JsonObject &obj )
{
    // Mandatory
    gun_type = obj.get_string( "gun_type" );
    ammo_type = obj.get_string( "ammo_type" );

    JsonArray jarr = obj.get_array( "fake_skills" );
    while( jarr.has_more() ) {
        JsonArray cur = jarr.next_array();
        fake_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
    }

    range = obj.get_float( "range" );
    description = obj.get_string( "description" );
    move_cost = obj.get_int( "move_cost" );
    targeting_cost = obj.get_int( "targeting_cost" );

    // Optional:
    max_ammo = obj.get_int( "max_ammo", INT_MAX );

    fake_str = obj.get_int( "fake_str", 8 );
    fake_dex = obj.get_int( "fake_dex", 8 );
    fake_int = obj.get_int( "fake_int", 8 );
    fake_per = obj.get_int( "fake_per", 8 );

    require_targeting_player = obj.get_bool( "require_targeting_player", true );
    require_targeting_npc = obj.get_bool( "require_targeting_npc", false );
    require_targeting_monster = obj.get_bool( "require_targeting_monster", false );
    targeting_timeout = obj.get_int( "targeting_timeout", 8 );
    targeting_timeout_extend = obj.get_int( "targeting_timeout_extend", 3 );

    burst_limit = obj.get_int( "burst_limit", INT_MAX );

    laser_lock = obj.get_bool( "laser_lock", false );

    range_no_burst = obj.get_float( "range_no_burst", range + 1 );

    if( obj.has_member( "targeting_sound" ) || obj.has_member( "targeting_volume" ) ) {
        // Both or neither, but not just one
        targeting_sound = obj.get_string( "targeting_sound" );
        targeting_volume = obj.get_int( "targeting_volume" );
    }

    // Sound of no ammo
    no_ammo_sound = obj.get_string( "no_ammo_sound", "" );
}

mattack_actor *gun_actor::clone() const
{
    return new gun_actor( *this );
}

bool gun_actor::call( monster &z ) const
{
    Creature *target;
    if( z.friendly != 0 ) {
        // Attacking monsters, not the player!
        int boo_hoo;
        target = z.auto_find_hostile_target( range, boo_hoo );
        if( target == nullptr ) {
            // Couldn't find any targets!
            if( boo_hoo > 0 && g->u.sees( z ) ) {
                // because that stupid oaf was in the way!
                add_msg( m_warning, ngettext( "Pointed in your direction, the %s emits an IFF warning beep.",
                                              "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                              boo_hoo ),
                         z.name().c_str(), boo_hoo );
            }
            return false;
        }

        shoot( z, *target );
        return true;
    }

    // Not friendly; hence, firing at the player too
    target = z.attack_target();
    if( target == nullptr || rl_dist( z.pos(), target->pos() ) > range ||
        !z.sees( *target ) ) {
        return false;
    }

    shoot( z, *target );
    return true;
}

void gun_actor::shoot( monster &z, Creature &target ) const
{
    // Make sure our ammo isn't weird.
    if( z.ammo[ammo_type] > max_ammo ) {
        debugmsg( "Generated too much ammo (%d) of type %s for %s in gun_actor::shoot",
                  z.ammo[ammo_type], ammo_type.c_str(), z.name().c_str() );
        z.ammo[ammo_type] = max_ammo;
    }

    const bool require_targeting = ( require_targeting_player && target.is_player() ) ||
                                   ( require_targeting_npc && target.is_npc() ) ||
                                   ( require_targeting_monster && target.is_monster() );
    const bool not_targeted = require_targeting && !z.has_effect( effect_targeted );
    const bool not_laser_locked = require_targeting && laser_lock &&
                                  !target.has_effect( effect_was_laserlocked );

    if( not_targeted || not_laser_locked ) {
        if( !targeting_sound.empty() ) {
            sounds::sound( z.pos(), targeting_volume, _( targeting_sound.c_str() ) );
        }
        if( not_targeted ) {
            z.add_effect( effect_targeted, targeting_timeout );
        }
        if( not_laser_locked ) {
            target.add_effect( effect_laserlocked, 5 );
            target.add_effect( effect_was_laserlocked, 5 );
            target.add_msg_if_player( m_warning,
                                      _( "You're not sure why you've got a laser dot on you..." ) );
        }

        z.moves -= targeting_cost;
        return;
    }

    // It takes a while
    z.moves -= move_cost;

    if( z.ammo[ammo_type] <= 0 && !no_ammo_sound.empty() ) {
        sounds::sound( z.pos(), 10, _( no_ammo_sound.c_str() ) );
        return;
    }

    if( g->u.sees( z ) ) {
        add_msg( m_warning, _( description.c_str() ) );
    }

    npc tmp;
    tmp.name = _( "The " ) + z.name();
    tmp.set_fake( true );
    tmp.recoil = 0;
    tmp.driving_recoil = 0;
    tmp.setpos( z.pos() );
    tmp.str_max = fake_str;
    tmp.dex_max = fake_dex;
    tmp.int_max = fake_int;
    tmp.per_max = fake_per;
    tmp.str_cur = fake_str;
    tmp.dex_cur = fake_dex;
    tmp.int_cur = fake_int;
    tmp.per_cur = fake_per;
    tmp.weapon = item( gun_type, 0 );
    tmp.weapon.set_curammo( ammo_type );
    tmp.weapon.charges = z.ammo[ammo_type];
    if( z.friendly != 0 ) {
        tmp.attitude = NPCATT_DEFEND;
    } else {
        tmp.attitude = NPCATT_KILL;
    }

    for( const auto &pr : fake_skills ) {
        tmp.skillLevel( pr.first ).level( pr.second );
    }

    const auto distance = rl_dist( z.pos(), target.pos() );
    int burst_size = std::min( burst_limit, tmp.weapon.burst_size() );
    if( distance > range_no_burst || burst_size < 1 ) {
        burst_size = 1;
    }

    tmp.fire_gun( target.pos(), burst_size );
    z.ammo[ammo_type] = tmp.weapon.charges;
    if( require_targeting ) {
        z.add_effect( effect_targeted, targeting_timeout_extend );
    }

    if( laser_lock ) {
        // To prevent spamming laser locks when the player can tank that stuff somehow
        target.add_effect( effect_was_laserlocked, 5 );
    }
}
