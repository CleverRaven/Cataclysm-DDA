#include "mattack_actors.h"
#include <vector>

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "itype.h"
#include "monster.h"
#include "messages.h"
#include "translations.h"
#include "sounds.h"
#include "gun_mode.h"
#include "npc.h"
#include "output.h"
#include "debug.h"
#include "generic_factory.h"
#include "line.h"

const efftype_id effect_grabbed( "grabbed" );
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
bool dodge_check( float max_accuracy, Creature &target )
{
    ///\EFFECT_DODGE increases chance of dodging special attacks of monsters
    float dodge = std::max( target.get_dodge() - rng( 0, max_accuracy ), 0.0f );
    if( rng( 0, 10000 ) < 10000 / ( 1 + ( 99 * exp( -0.6f * dodge ) ) ) ) {
        return true;
    }

    return false;
}

void leap_actor::load_internal( JsonObject &obj, const std::string & )
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
    if( !z.can_act() || !z.move_effects( false ) ) {
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

melee_actor::melee_actor()
{
    damage_max_instance = damage_instance::physical( 9, 0, 0, 0 );
    min_mul = 0.5f;
    max_mul = 1.0f;
    move_cost = 100;
}

void melee_actor::load_internal( JsonObject &obj, const std::string & )
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

    optional( obj, was_loaded, "miss_msg_u", miss_msg_u, translated_string_reader,
              _( "The %s lunges at you, but you dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_u", no_dmg_msg_u, translated_string_reader,
              _( "The %1$s bites your %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_u", hit_dmg_u, translated_string_reader,
              _( "The %1$s bites your %2$s!" ) );
    optional( obj, was_loaded, "miss_msg_npc", miss_msg_npc, translated_string_reader,
              _( "The %s lunges at <npcname>, but they dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_npc", no_dmg_msg_npc, translated_string_reader,
              _( "The %1$s bites <npcname>'s %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_npc", hit_dmg_npc, translated_string_reader,
              _( "The %1$s bites <npcname>'s %2$s!" ) );

    if( obj.has_array( "body_parts" ) ) {
        JsonArray jarr = obj.get_array( "body_parts" );
        while( jarr.has_more() ) {
            JsonArray sub = jarr.next_array();
            const body_part bp = get_body_part_token( sub.get_string( 0 ) );
            const float prob = sub.get_float( 1 );
            body_parts.add_or_replace( bp, prob );
        }
    }

    if( obj.has_array( "effects" ) ) {
        JsonArray jarr = obj.get_array( "effects" );
        while( jarr.has_more() ) {
            JsonObject eff = jarr.next_object();
            effects.push_back( load_mon_effect_data( eff ) );
        }
    }
}

Creature *melee_actor::find_target( monster &z ) const
{
    if( !z.can_act() ) {
        return nullptr;
    }

    Creature *target = z.attack_target();
    if( target == nullptr || !is_adjacent( z, *target ) ) {
        return nullptr;
    }

    return target;
}

bool melee_actor::call( monster &z ) const
{
    Creature *target = find_target( z );
    if( target == nullptr ) {
        return false;
    }

    z.mod_moves( -move_cost );

    add_msg( m_debug, "%s attempting to melee_attack %s", z.name().c_str(),
             target->disp_name().c_str() );

    const int acc = accuracy >= 0 ? accuracy : z.type->melee_skill;
    int hitspread = target->deal_melee_attack( &z, dice( acc, 10 ) );

    if( hitspread < 0 ) {
        auto msg_type = target == &g->u ? m_warning : m_info;
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, miss_msg_u.c_str(), miss_msg_npc.c_str(),
                                       z.name().c_str() );
        return true;
    }

    damage_instance damage = damage_max_instance;

    double multiplier = rng_float( min_mul, max_mul );
    damage.mult_damage( multiplier );

    body_part bp_hit = body_parts.empty() ?
                       target->select_body_part( &z, hitspread ) :
                       *body_parts.pick();

    target->on_hit( &z, bp_hit );
    dealt_damage_instance dealt_damage = target->deal_damage( &z, bp_hit, damage );
    dealt_damage.bp_hit = bp_hit;

    int damage_total = dealt_damage.total_damage();
    add_msg( m_debug, "%s's melee_attack did %d damage", z.name().c_str(), damage_total );
    if( damage_total > 0 ) {
        on_damage( z, *target, dealt_damage );
    } else {
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( no_dmg_msg_u.c_str(), no_dmg_msg_npc.c_str(), z.name().c_str(),
                                       body_part_name_accusative( bp_hit ).c_str() );
    }

    return true;
}

void melee_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    if( target.is_player() ) {
        sfx::play_variant_sound( "mon_bite", "bite_hit", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        sfx::do_player_death_hurt( dynamic_cast<player &>( target ), 0 );
    }
    auto msg_type = target.attitude_to( g->u ) == Creature::A_FRIENDLY ? m_bad : m_neutral;
    const body_part bp = dealt.bp_hit;
    target.add_msg_player_or_npc( msg_type, hit_dmg_u.c_str(), hit_dmg_npc.c_str(), z.name().c_str(),
                                  body_part_name_accusative( bp ).c_str() );

    for( const auto &eff : effects ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const body_part affected_bp = eff.affect_hit_bp ? bp : eff.bp;
            target.add_effect( eff.id, time_duration::from_turns( eff.duration ), affected_bp, eff.permanent );
        }
    }
}

mattack_actor *melee_actor::clone() const
{
    return new melee_actor( *this );
}

bite_actor::bite_actor()
{
}

void bite_actor::load_internal( JsonObject &obj, const std::string &src )
{
    melee_actor::load_internal( obj, src );
    no_infection_chance = obj.get_int( "no_infection_chance", 14 );
}

void bite_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    melee_actor::on_damage( z, target, dealt );
    if( target.has_effect( effect_grabbed ) && one_in( no_infection_chance - dealt.total_damage() ) ) {
        const body_part hit = dealt.bp_hit;
        if( target.has_effect( effect_bite, hit ) ) {
            target.add_effect( effect_bite, 40_minutes, hit, true );
        } else if( target.has_effect( effect_infected, hit ) ) {
            target.add_effect( effect_infected, 25_minutes, hit, true );
        } else {
            target.add_effect( effect_bite, 1_turns, hit, true );
        }
    }
}

mattack_actor *bite_actor::clone() const
{
    return new bite_actor( *this );
}

gun_actor::gun_actor() : description( _( "The %1$s fires its %2$s!" ) ),
    targeting_sound( _( "beep-beep-beep!" ) )
{
}

void gun_actor::load_internal( JsonObject &obj, const std::string & )
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

    auto arr = obj.get_array( "ranges" );
    while( arr.has_more() ) {
        auto mode = arr.next_array();
        if( mode.size() < 2 || mode.get_int( 0 ) > mode.get_int( 1 ) ) {
            obj.throw_error( "incomplete or invalid range specified", "ranges" );
        }
        ranges.emplace( std::make_pair<int, int>( mode.get_int( 0 ), mode.get_int( 1 ) ),
                        gun_mode_id( mode.size() > 2 ? mode.get_string( 2 ) : "" ) );
    }

    obj.read( "max_ammo", max_ammo );

    obj.read( "move_cost", move_cost );

    if( obj.read( "description", description ) ) {
        description = _( description.c_str() );
    }
    if( obj.read( "failure_msg", failure_msg ) ) {
        failure_msg = _( failure_msg.c_str() );
    }
    if( obj.read( "no_ammo_sound", no_ammo_sound ) ) {
        no_ammo_sound = _( no_ammo_sound.c_str() );
    }

    obj.read( "targeting_cost", targeting_cost );

    obj.read( "require_targeting_player", require_targeting_player );
    obj.read( "require_targeting_npc", require_targeting_npc );
    obj.read( "require_targeting_monster", require_targeting_monster );

    obj.read( "targeting_timeout", targeting_timeout );
    obj.read( "targeting_timeout_extend", targeting_timeout_extend );

    if( obj.read( "targeting_sound", targeting_sound ) ) {
        targeting_sound = _( targeting_sound.c_str() );
    }
    obj.read( "targeting_volume", targeting_volume );

    obj.get_bool( "laser_lock", laser_lock );

    obj.read( "require_sunlight", require_sunlight );
}

mattack_actor *gun_actor::clone() const
{
    return new gun_actor( *this );
}

bool gun_actor::call( monster &z ) const
{
    Creature *target;

    if( z.friendly ) {
        int max_range = 0;
        for( const auto &e : ranges ) {
            max_range = std::max( std::max( max_range, e.first.first ), e.first.second );
        }

        int hostiles; // hostiles which cannot be engaged without risking friendly fire
        target = z.auto_find_hostile_target( max_range, hostiles );
        if( !target ) {
            if( hostiles > 0 && g->u.sees( z ) ) {
                add_msg( m_warning, ngettext( "Pointed in your direction, the %s emits an IFF warning beep.",
                                              "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                              hostiles ),
                         z.name().c_str(), hostiles );
            }
            return false;
        }

    } else {
        target = z.attack_target();
        if( !target || !z.sees( *target ) ) {
            return false;
        }
    }

    int dist = rl_dist( z.pos(), target->pos() );
    for( const auto &e : ranges ) {
        if( dist >= e.first.first && dist <= e.first.second ) {
            shoot( z, *target, e.second );
            return true;
        }
    }
    return false;
}

void gun_actor::shoot( monster &z, Creature &target, const gun_mode_id &mode ) const
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
            z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout ) );
        }
        if( not_laser_locked ) {
            target.add_effect( effect_laserlocked, 5_turns );
            target.add_effect( effect_was_laserlocked, 5_turns );
            target.add_msg_if_player( m_warning,
                                      _( "You're not sure why you've got a laser dot on you..." ) );
        }

        z.moves -= targeting_cost;
        return;
    }

    z.moves -= move_cost;

    item gun( gun_type );
    gun.gun_set_mode( mode );

    itype_id ammo = ( ammo_type != "null" ) ? ammo_type : gun.ammo_default();
    if( ammo != "null" ) {
        gun.ammo_set( ammo, z.ammo[ ammo ] );
    }

    if( !gun.ammo_sufficient() ) {
        if( !no_ammo_sound.empty() ) {
            sounds::sound( z.pos(), 10, _( no_ammo_sound.c_str() ) );
        }
        return;
    }

    standard_npc tmp( _( "The " ) + z.name(), {}, 8, fake_str, fake_dex, fake_int, fake_per );
    tmp.set_fake( true );
    tmp.setpos( z.pos() );
    tmp.set_attitude( z.friendly ? NPCATT_FOLLOW : NPCATT_KILL );
    tmp.recoil = 0; // no need to aim

    for( const auto &pr : fake_skills ) {
        tmp.set_skill_level( pr.first, pr.second );
    }

    tmp.weapon = gun;
    tmp.i_add( item( "UPS_off", calendar::turn, 1000 ) );

    if( g->u.sees( z ) ) {
        add_msg( m_warning, _( description.c_str() ), z.name().c_str(),
                 tmp.weapon.tname().c_str() );
    }

    z.ammo[ammo] -= tmp.fire_gun( target.pos(), gun.gun_current_mode().qty );

    if( require_targeting ) {
        z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout_extend ) );
    }

    if( laser_lock ) {
        // To prevent spamming laser locks when the player can tank that stuff somehow
        target.add_effect( effect_was_laserlocked, 5_turns );
    }
}
