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
    gun_type = obj.get_string( "gun_type" );

    obj.read( "ammo_type", ammo_type );

    if( obj.has_array( "fake_skills" ) ) {
        JsonArray jarr = obj.get_array( "fake_skills" );
        while( jarr.has_more() ) {
            JsonArray cur = jarr.next_array();
            fake_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
        }
    }

    obj.read( "fake_str", fake_str );
    obj.read( "fake_dex", fake_dex );
    obj.read( "fake_int", fake_int );
    obj.read( "fake_per", fake_per );

    obj.read( "range", target_range );
    obj.read( "burst_limit", burst_limit );
    obj.read( "range_no_burst", range_no_burst );

    obj.read( "max_ammo", max_ammo );

    obj.read( "move_cost", move_cost );

    obj.read( "description", description );
    obj.read( "failure_msg", failure_msg );
    obj.read( "no_ammo_sound", no_ammo_sound );

    obj.read( "targeting_cost", targeting_cost );

    obj.read( "require_targeting_player", require_targeting_player );
    obj.read( "require_targeting_npc", require_targeting_npc );
    obj.read( "require_targeting_monster", require_targeting_monster );

    obj.read( "targeting_timeout", targeting_timeout );
    obj.read( "targeting_timeout_extend", targeting_timeout_extend );

    obj.read( "targeting_sound", targeting_sound );
    obj.read( "targeting_volume", targeting_volume );

    obj.get_bool( "laser_lock", laser_lock );

    obj.read( "require_sunlight", require_sunlight );
}

mattack_actor *gun_actor::clone() const
{
    return new gun_actor( *this );
}

// @todo Enforce load order (add finalization?) and allow calculating this at json load
int gun_actor::get_range() const
{
    if( target_range > 0 ) {
        return target_range;
    }

    item gun( gun_type );
    itype_id ammo = ( ammo_type != "NULL" ) ? ammo_type : gun.ammo_default();

    if( ammo != "NULL" ) {
        gun.ammo_set( ammo, max_ammo );
    }

    return gun.gun_range( true );
}

bool gun_actor::call( monster &z ) const
{
    const int range = get_range();
    if( z.friendly != 0 ) {
        // Attacking monsters, not the player!
        int boo_hoo;
        Creature *hostile_target = z.auto_find_hostile_target( range, boo_hoo );
        if( hostile_target == nullptr ) {
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

        shoot( z, *hostile_target );
        return true;
    }

    // Not friendly; hence, firing at the player too
    Creature *target = z.attack_target();
    if( target == nullptr || rl_dist( z.pos(), target->pos() ) > range ||
        !z.sees( *target ) ) {
        return false;
    }

    shoot( z, *target );
    return true;
}

void gun_actor::shoot( monster &z, Creature &target ) const
{
    if( require_sunlight && !g->is_in_sunlight( z.pos() ) ) {
        if( one_in( 3 ) && g->u.sees( z ) ) {
            add_msg( _( failure_msg.c_str() ), z.name().c_str() );
        }
        return;
    }

    const bool require_targeting = ( require_targeting_player && target.is_player() ) ||
                                   ( require_targeting_npc && target.is_npc() ) ||
                                   ( require_targeting_monster && target.is_monster() );
    const bool not_targeted = require_targeting && !z.has_effect( effect_targeted );
    const bool not_laser_locked = require_targeting && laser_lock &&
                                  !target.has_effect( effect_was_laserlocked );

    if( not_targeted || not_laser_locked ) {
        if( targeting_volume > 0 && !targeting_sound.empty() ) {
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

    z.moves -= move_cost;

    item gun( gun_type );
    itype_id ammo = ( ammo_type != "NULL" ) ? ammo_type : gun.ammo_default();

    if( ammo != "NULL" && z.ammo[ ammo ] < gun.ammo_required() ) {
        if( !no_ammo_sound.empty() ) {
            sounds::sound( z.pos(), 10, _( no_ammo_sound.c_str() ) );
        }
        return;
    }

    if( ammo != "NULL" ) {
        gun.ammo_set( ammo, z.ammo[ ammo ] );
    }

    npc tmp;
    tmp.name = _( "The " ) + z.name();
    tmp.set_fake( true );
    tmp.setpos( z.pos() );
    tmp.str_max = fake_str;
    tmp.dex_max = fake_dex;
    tmp.int_max = fake_int;
    tmp.per_max = fake_per;
    tmp.str_cur = fake_str;
    tmp.dex_cur = fake_dex;
    tmp.int_cur = fake_int;
    tmp.per_cur = fake_per;
    tmp.attitude = z.friendly ? NPCATT_DEFEND : NPCATT_KILL;

    if( fake_skills.empty() ) {
        tmp.set_skill_level( skill_id( "gun" ), 4 );
        tmp.set_skill_level( gun.gun_skill(), 8 );
    }
    for( const auto &pr : fake_skills ) {
        tmp.set_skill_level( pr.first, pr.second );
    }

    tmp.weapon = gun;
    tmp.worn.push_back( item( "fake_UPS", calendar::turn, 1000 ) );

    const auto distance = rl_dist( z.pos(), target.pos() );

    const int burst = std::min( distance <= range_no_burst ? burst_limit : 1, tmp.weapon.burst_size() );

    if( g->u.sees( z ) ) {
        add_msg( m_warning, _( description.c_str() ), z.name().c_str(), tmp.weapon.gun_type().c_str() );
    }

    int ammo_used = tmp.fire_gun( target.pos(), burst );
    if( ammo != "NULL" ) {
        // Don't set "NULL" ammo
        z.ammo[ammo] -= ammo_used;
    }

    if( require_targeting ) {
        z.add_effect( effect_targeted, targeting_timeout_extend );
    }

    if( laser_lock ) {
        // To prevent spamming laser locks when the player can tank that stuff somehow
        target.add_effect( effect_was_laserlocked, 5 );
    }
}
