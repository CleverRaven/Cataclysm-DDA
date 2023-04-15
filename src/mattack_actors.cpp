#include "mattack_actors.h"

#include <algorithm>
#include <limits>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "enums.h"
#include "game.h"
#include "generic_factory.h"
#include "gun_mode.h"
#include "item.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "viewer.h"


static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_run( "run" );
static const efftype_id effect_sensor_stun( "sensor_stun" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_targeted( "targeted" );
static const efftype_id effect_vampire_virus( "vampire_virus" );
static const efftype_id effect_was_laserlocked( "was_laserlocked" );
static const efftype_id effect_zombie_virus( "zombie_virus" );

static const skill_id skill_gun( "gun" );
static const skill_id skill_throw( "throw" );

static const trait_id trait_TOXICFLESH( "TOXICFLESH" );
static const trait_id trait_VAMPIRE( "VAMPIRE" );

// Common helpers
bool mattack_actor::check_self_conditions( monster &z ) const
{
    bool failed = true;
    if( attack_chance != 100 && !x_in_y( attack_chance, 100 ) ) {
        add_msg_debug( debugmode::DF_MATTACK, "%s's %s attack_chance roll failed (%d%% chance)", z.name(),
                       id.c_str(), attack_chance );
        return false;
    }

    for( const efftype_id &effect : required_effects_all ) {
        if( !z.has_effect( effect ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Lack of required(all) effect %s", effect.c_str() );
            return false;
        }
    }

    for( const efftype_id &effect : forbidden_effects_any ) {
        if( z.has_effect( effect ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Forbidden(any) effect %s found", effect.c_str() );
            return false;
        }
    }

    if( !forbidden_effects_all.empty() ) {
        for( const efftype_id &effect : forbidden_effects_all ) {
            if( !z.has_effect( effect ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Forbidden(all) effect %s not found", effect.c_str() );
                failed = false;
            }
        }
        if( failed ) {
            add_msg_debug( debugmode::DF_MATTACK, "All forbidden(all) effects found" );
            return false;
        }
    }
    if( !required_effects_any.empty() ) {
        failed = true;
        for( const efftype_id &effect : required_effects_any ) {
            if( z.has_effect( effect ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Required(any) effect %s found", effect.c_str() );
                failed = false;
            }
        }
        if( failed ) {
            add_msg_debug( debugmode::DF_MATTACK, "Lack of all required(any) effects" );
            return false;
        }
    }
    return true;
}

bool mattack_actor::check_target_conditions( Creature *target ) const
{
    bool failed = true;
    for( const efftype_id &effect : target_forbidden_effects_any ) {
        if( target->has_effect( effect ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Target forbidden(any) effect %s found", effect.c_str() );
            return false;
        }
    }

    for( const efftype_id &effect : target_required_effects_all ) {
        if( !target->has_effect( effect ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Lack of target required(all) effect %s", effect.c_str() );
            return false;
        }
    }

    if( !target_forbidden_effects_all.empty() ) {
        for( const efftype_id &effect : target_forbidden_effects_all ) {
            if( !target->has_effect( effect ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Target forbidden(all) effect %s not found", effect.c_str() );
                failed = false;
            }
        }
        if( failed ) {
            add_msg_debug( debugmode::DF_MATTACK, "All target forbidden(all) effects found" );
            return false;
        }
    }

    if( !target_required_effects_any.empty() ) {
        failed = true;
        for( const efftype_id &effect : target_required_effects_any ) {
            if( target->has_effect( effect ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Target required(any) effect %s found", effect.c_str() );
                failed = false;
            }
        }
        if( failed ) {
            add_msg_debug( debugmode::DF_MATTACK, "Lack of all target required(any) effects" );
            return false;
        }
    }

    return true;
}

void leap_actor::load_internal( const JsonObject &obj, const std::string & )
{
    // Mandatory:
    max_range = obj.get_float( "max_range" );
    // Optional:
    min_range = obj.get_float( "min_range", 1.0f );
    allow_no_target = obj.get_bool( "allow_no_target", false );
    optional( obj, was_loaded, "attack_chance", attack_chance, 100 );
    optional( obj, was_loaded, "prefer_leap", prefer_leap, false );
    optional( obj, was_loaded, "random_leap", random_leap, false );
    move_cost = obj.get_int( "move_cost", 150 );
    min_consider_range = obj.get_float( "min_consider_range", 0.0f );
    max_consider_range = obj.get_float( "max_consider_range", 200.0f );
    optional( obj, was_loaded, "message", message,
              to_translation( "The %s leaps!" ) );
    optional( obj, was_loaded, "forbidden_effects_any", forbidden_effects_any );
    optional( obj, was_loaded, "forbidden_effects_all", forbidden_effects_all );
    optional( obj, was_loaded, "required_effects_any", required_effects_any );
    optional( obj, was_loaded, "required_effects_all", required_effects_all );

    if( obj.has_array( "self_effects" ) ) {
        for( JsonObject eff : obj.get_array( "self_effects" ) ) {
            mon_effect_data effect;
            effect.load( eff );
            self_effects.push_back( std::move( effect ) );
        }
    }
}

std::unique_ptr<mattack_actor> leap_actor::clone() const
{
    return std::make_unique<leap_actor>( *this );
}

bool leap_actor::call( monster &z ) const
{
    if( !z.has_dest() || !z.can_act() || !z.move_effects( false ) ) {
        add_msg_debug( debugmode::DF_MATTACK, "Monster has no destination or can't act" );
        return false;
    }

    if( !check_self_conditions( z ) ) {
        return false;
    }

    std::vector<tripoint> options;
    const tripoint_abs_ms target_abs = z.get_dest();
    // Calculate distance to target
    const float best_float = rl_dist( z.get_location(), target_abs );
    add_msg_debug( debugmode::DF_MATTACK, "Target distance %.1f", best_float );
    if( best_float < min_consider_range || best_float > max_consider_range ) {
        add_msg_debug( debugmode::DF_MATTACK, "Best float outside of considered range" );
        return false;
    }

    // We wanted the float for range check
    // int here will make the jumps more random
    int best = std::numeric_limits<int>::max();
    if( !allow_no_target && z.attack_target() == nullptr ) {
        add_msg_debug( debugmode::DF_MATTACK, "Leaping without a target disabled" );
        return false;
    }
    map &here = get_map();
    const tripoint target = here.getlocal( target_abs );
    add_msg_debug( debugmode::DF_MATTACK, "Target at coordinates %d,%d,%d",
                   target.x, target.y, target.z );

    std::multimap<int, tripoint> candidates;
    for( const tripoint &candidate : here.points_in_radius( z.pos(), max_range ) ) {
        if( candidate == z.pos() ) {
            add_msg_debug( debugmode::DF_MATTACK, "Monster at coordinates %d,%d,%d",
                           candidate.x, candidate.y, candidate.z );
            continue;
        }
        float leap_dist = trigdist ? trig_dist( z.pos(), candidate ) :
                          square_dist( z.pos(), candidate );
        add_msg_debug( debugmode::DF_MATTACK,
                       "Candidate coordinates %d,%d,%d, distance %.1f, min range %.1f, max range %.1f",
                       candidate.x, candidate.y, candidate.z, leap_dist, min_range, max_range );
        if( leap_dist > max_range || leap_dist < min_range ) {
            add_msg_debug( debugmode::DF_MATTACK,
                           "Candidate outside of allowed range, discarded" );
            continue;
        }
        int candidate_dist = rl_dist( candidate, target );
        if( candidate_dist >= best_float && !( prefer_leap || random_leap ) ) {
            add_msg_debug( debugmode::DF_MATTACK,
                           "Candidate farther from target than optimal path, discarded" );
            continue;
        }
        candidates.emplace( candidate_dist, candidate );
    }
    for( const auto &candidate : candidates ) {
        const int &cur_dist = candidate.first;
        const tripoint &dest = candidate.second;
        if( cur_dist > best && !random_leap ) {
            add_msg_debug( debugmode::DF_MATTACK,
                           "Distance %d larger than previous best %d, candidate discarded", cur_dist, best );
            break;
        }
        if( !z.sees( dest ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Can't see destination, candidate discarded" );
            continue;
        }
        if( !g->is_empty( dest ) ) {
            continue;
        }
        bool blocked_path = false;
        // check if monster has a clear path to the proposed point
        std::vector<tripoint> line = here.find_clear_path( z.pos(), dest );
        for( tripoint &i : line ) {
            if( here.impassable( i ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Path blocked, candidate discarded" );
                blocked_path = true;
                break;
            }
        }
        // don't leap into water if you could drown (#38038)
        if( z.is_aquatic_danger( dest ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Can't leap into water, candidate discarded" );
            blocked_path = true;
        }
        if( blocked_path ) {
            continue;
        }

        options.push_back( dest );
        best = cur_dist;
    }

    if( options.empty() ) {
        add_msg_debug( debugmode::DF_MATTACK, "No acceptable leap candidates" );
        return false;    // Nowhere to leap!
    }

    z.moves -= move_cost;
    viewer &player_view = get_player_view();
    const tripoint chosen = random_entry( options );
    bool seen = player_view.sees( z ); // We can see them jump...
    z.setpos( chosen );
    seen |= player_view.sees( z ); // ... or we can see them land
    if( seen ) {
        add_msg( message, z.name() );
    }

    for( const mon_effect_data &eff : self_effects ) {
        if( x_in_y( eff.chance, 100 ) ) {
            z.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first, eff.duration.second ) ),
                          eff.permanent,
                          rng( eff.intensity.first, eff.intensity.second ) );
        }
    }

    return true;
}

std::unique_ptr<mattack_actor> mon_spellcasting_actor::clone() const
{
    return std::make_unique<mon_spellcasting_actor>( *this );
}

void mon_spellcasting_actor::load_internal( const JsonObject &obj, const std::string & )
{
    mandatory( obj, was_loaded, "spell_data", spell_data );
    translation monster_message;
    optional( obj, was_loaded, "monster_message", monster_message,
              //~ "<Monster Display name> cast <Spell Name> on <Target name>!"
              to_translation( "%1$s casts %2$s at %3$s!" ) );
    spell_data.trigger_message = monster_message;
    optional( obj, was_loaded, "attack_chance", attack_chance, 100 );
    optional( obj, was_loaded, "forbidden_effects_any", forbidden_effects_any );
    optional( obj, was_loaded, "forbidden_effects_all", forbidden_effects_all );
    optional( obj, was_loaded, "required_effects_any", required_effects_any );
    optional( obj, was_loaded, "required_effects_all", required_effects_all );
    optional( obj, was_loaded, "target_forbidden_effects_any", target_forbidden_effects_any );
    optional( obj, was_loaded, "target_forbidden_effects_all", target_forbidden_effects_all );
    optional( obj, was_loaded, "target_required_effects_any", target_required_effects_any );
    optional( obj, was_loaded, "target_required_effects_all", target_required_effects_all );
    optional( obj, was_loaded, "allow_no_target", allow_no_target, false );

}

bool mon_spellcasting_actor::call( monster &mon ) const
{
    if( !mon.can_act() ) {
        return false;
    }

    if( !mon.attack_target() && !allow_no_target ) {
        // this is an attack. there is no reason to attack if there isn't a real target.
        // Unless we don't need one
        return false;
    }

    if( !check_self_conditions( mon ) ) {
        return false;
    }

    if( !allow_no_target && !check_target_conditions( mon.attack_target() ) ) {
        return false;
    }

    const tripoint target = ( spell_data.self ||
                              allow_no_target ) ? mon.pos() : mon.attack_target()->pos();
    spell spell_instance = spell_data.get_spell( mon );
    spell_instance.set_message( spell_data.trigger_message );

    // Bail out if the target is out of range.
    if( !spell_data.self && rl_dist( mon.pos(), target ) > spell_instance.range( mon ) ) {
        return false;
    }

    std::string target_name;
    if( const Creature *target_monster = get_creature_tracker().creature_at( target ) ) {
        target_name = target_monster->disp_name();
    }

    add_msg_if_player_sees( target, spell_instance.message(), mon.disp_name(),
                            spell_instance.name(), target_name );

    avatar fake_player;
    mon.moves -= spell_instance.casting_time( fake_player, true );
    spell_instance.cast_all_effects( mon, target );

    return true;
}

melee_actor::melee_actor()
{
    damage_max_instance = damage_instance::physical( 9, 0, 0, 0 );
    min_mul = 0.5f;
    max_mul = 1.0f;
    move_cost = 100;
}

void melee_actor::load_internal( const JsonObject &obj, const std::string & )
{
    // Optional:
    if( obj.has_array( "damage_max_instance" ) ) {
        damage_max_instance = load_damage_instance( obj.get_array( "damage_max_instance" ) );
    } else if( obj.has_object( "damage_max_instance" ) ) {
        damage_max_instance = load_damage_instance( obj );
    }

    optional( obj, was_loaded, "attack_chance", attack_chance, 100 );
    optional( obj, was_loaded, "forbidden_effects_any", forbidden_effects_any );
    optional( obj, was_loaded, "forbidden_effects_all", forbidden_effects_all );
    optional( obj, was_loaded, "required_effects_any", required_effects_any );
    optional( obj, was_loaded, "required_effects_all", required_effects_all );
    optional( obj, was_loaded, "target_forbidden_effects_any", target_forbidden_effects_any );
    optional( obj, was_loaded, "target_forbidden_effects_all", target_forbidden_effects_all );
    optional( obj, was_loaded, "target_required_effects_any", target_required_effects_any );
    optional( obj, was_loaded, "target_required_effects_all", target_required_effects_all );
    optional( obj, was_loaded, "accuracy", accuracy, INT_MIN );
    optional( obj, was_loaded, "min_mul", min_mul, 0.5f );
    optional( obj, was_loaded, "max_mul", max_mul, 1.0f );
    optional( obj, was_loaded, "move_cost", move_cost, 100 );
    optional( obj, was_loaded, "range", range, 1 );
    optional( obj, was_loaded, "no_adjacent", no_adjacent, false );
    optional( obj, was_loaded, "dodgeable", dodgeable, true );
    optional( obj, was_loaded, "uncanny_dodgeable", uncanny_dodgeable, dodgeable );
    optional( obj, was_loaded, "blockable", blockable, true );
    optional( obj, was_loaded, "effects_require_dmg", effects_require_dmg, true );

    optional( obj, was_loaded, "range", range, 1 );
    optional( obj, was_loaded, "throw_strength", throw_strength, 0 );

    optional( obj, was_loaded, "hitsize_min", hitsize_min, -1 );
    optional( obj, was_loaded, "hitsize_max", hitsize_max, -1 );
    optional( obj, was_loaded, "attack_upper", attack_upper, true );

    optional( obj, was_loaded, "miss_msg_u", miss_msg_u,
              to_translation( "%s lunges at you, but you dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_u", no_dmg_msg_u,
              to_translation( "%1$s bites your %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_u", hit_dmg_u,
              to_translation( "%1$s bites your %2$s!" ) );
    optional( obj, was_loaded, "miss_msg_npc", miss_msg_npc,
              to_translation( "%s lunges at <npcname>, but they dodge!" ) );
    optional( obj, was_loaded, "no_dmg_msg_npc", no_dmg_msg_npc,
              to_translation( "%1$s bites <npcname>'s %2$s, but fails to penetrate armor!" ) );
    optional( obj, was_loaded, "hit_dmg_npc", hit_dmg_npc,
              to_translation( "%1$s bites <npcname>'s %2$s!" ) );
    optional( obj, was_loaded, "throw_msg_u", throw_msg_u,
              to_translation( "%s hits you with such a force that it sends you flying!" ) );
    optional( obj, was_loaded, "throw_msg_npc", throw_msg_npc,
              to_translation( "%s hits <npcname> with such a force that it sends them flying!" ) );

    if( obj.has_array( "body_parts" ) ) {
        for( JsonArray sub : obj.get_array( "body_parts" ) ) {
            const bodypart_str_id bp( sub.get_string( 0 ) );
            const float prob = sub.get_float( 1 );
            body_parts.add_or_replace( bp, prob );
        }
    }

    if( obj.has_array( "effects" ) ) {
        for( JsonObject eff : obj.get_array( "effects" ) ) {
            mon_effect_data effect;
            effect.load( eff );
            effects.push_back( std::move( effect ) );
        }
    }

    if( obj.has_array( "self_effects_always" ) ) {
        for( JsonObject eff : obj.get_array( "self_effects_always" ) ) {
            mon_effect_data effect;
            effect.load( eff );
            self_effects_always.push_back( std::move( effect ) );
        }
    }
    if( obj.has_array( "self_effects_onhit" ) ) {
        for( JsonObject eff : obj.get_array( "self_effects_onhit" ) ) {
            mon_effect_data effect;
            effect.load( eff );
            self_effects_onhit.push_back( std::move( effect ) );
        }
    }
    if( obj.has_array( "self_effects_ondmg" ) ) {
        for( JsonObject eff : obj.get_array( "self_effects_ondmg" ) ) {
            mon_effect_data effect;
            effect.load( eff );
            self_effects_ondmg.push_back( std::move( effect ) );
        }
    }
}

Creature *melee_actor::find_target( monster &z ) const
{
    if( !z.can_act() ) {
        return nullptr;
    }

    Creature *target = z.attack_target();

    if( target == nullptr || ( no_adjacent && z.is_adjacent( target, false ) ) ) {
        return nullptr;
    }

    if( range > 1 ) {
        if( !z.sees( *target ) ||
            !get_map().clear_path( z.pos(), target->pos(), range, 1, 200 ) ) {
            return nullptr;
        }

    } else if( !z.is_adjacent( target, false ) ) {
        return nullptr;
    }

    return target;
}

bool melee_actor::call( monster &z ) const
{
    if( !check_self_conditions( z ) ) {
        return false;
    }

    Creature *target = find_target( z );
    if( target == nullptr ) {
        return false;
    }

    if( !check_target_conditions( target ) ) {
        return false;
    }

    // If we are biting, make sure the target is grabbed if our z is humanoid
    if( dynamic_cast<const bite_actor *>( this ) && z.type->bodytype == "human" &&
        !target->has_effect( effect_grabbed ) ) {
        return false;
    }

    z.mod_moves( -move_cost );

    const std::string mon_name = get_player_character().sees( z.pos() ) ?
                                 z.disp_name( false, true ) : _( "Something" );

    // Add always-applied self effects
    for( const mon_effect_data &eff : self_effects_always ) {
        if( x_in_y( eff.chance, 100 ) ) {
            z.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first, eff.duration.second ) ),
                          eff.permanent,
                          rng( eff.intensity.first, eff.intensity.second ) );
            target->add_msg_if_player( m_mixed, eff.message, mon_name );
        }
    }

    // Dodge check

    const int acc = accuracy >= 0 ? accuracy : z.type->melee_skill;
    int hitspread = target->deal_melee_attack( &z, dice( acc, 10 ) );

    // Pick body part
    bodypart_str_id bp_hit = body_parts.empty() ?
                             target->select_body_part( hitsize_min, hitsize_max, attack_upper, hitspread ).id() :
                             *body_parts.pick();

    bodypart_id bp_id = bodypart_id( bp_hit );

    if( z.has_flag( MF_HIT_AND_RUN ) ) {
        z.add_effect( effect_run, 4_turns );
    }

    if( uncanny_dodgeable && target->uncanny_dodge() ) {
        game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, miss_msg_u, miss_msg_npc, z.name(),
                                       body_part_name_accusative( bp_id ) );
        return true;
    }

    if( dodgeable ) {
        if( hitspread < 0 ) {
            game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
            sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                     sfx::get_heard_angle( z.pos() ) );
            target->add_msg_player_or_npc( msg_type, miss_msg_u, miss_msg_npc, mon_name,
                                           body_part_name_accusative( bp_id ) );
            return true;
        }
    }

    // Damage instance calculation
    damage_instance damage = damage_max_instance;
    double multiplier = rng_float( min_mul, max_mul );
    add_msg_debug( debugmode::DF_MATTACK, "Damage multiplier %.1f (range %.1f - %.1f)", multiplier,
                   min_mul, max_mul );
    damage.mult_damage( multiplier );

    // Block our hit
    if( blockable ) {
        target->block_hit( &z, bp_id, damage );
    }

    // Take damage
    dealt_damage_instance dealt_damage = target->deal_damage( &z, bp_id, damage );
    dealt_damage.bp_hit = bp_id;

    // On hit effects
    target->on_hit( &z, bp_id );

    // Apply onhit self effects
    for( const mon_effect_data &eff : self_effects_onhit ) {
        if( x_in_y( eff.chance, 100 ) ) {
            z.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first, eff.duration.second ) ),
                          eff.permanent,
                          rng( eff.intensity.first, eff.intensity.second ) );
            target->add_msg_if_player( m_mixed, eff.message, mon_name );
        }
    }

    int damage_total = dealt_damage.total_damage();
    add_msg_debug( debugmode::DF_MATTACK, "%s's melee_attack did %d damage", z.name(), damage_total );
    if( damage_total > 0 ) {
        on_damage( z, *target, dealt_damage );
    } else {
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( m_neutral, no_dmg_msg_u, no_dmg_msg_npc, mon_name,
                                       body_part_name_accusative( bp_id ) );
        if( !effects_require_dmg ) {
            for( const mon_effect_data &eff : effects ) {
                if( x_in_y( eff.chance, 100 ) ) {
                    const bodypart_id affected_bp = eff.affect_hit_bp ? bp_id : eff.bp.id();
                    target->add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first,
                                        eff.duration.second ) ), affected_bp, eff.permanent, rng( eff.intensity.first,
                                                eff.intensity.second ) );
                }
            }
        }
    }
    if( throw_strength > 0 ) {
        z.remove_effect( effect_grabbing );
        g->fling_creature( target, coord_to_angle( z.pos(), target->pos() ),
                           throw_strength );
        target->add_msg_player_or_npc( m_bad, throw_msg_u, throw_msg_npc, mon_name );

        // Items strapped to you may fall off as you hit the ground
        // when you break out of a grab you have a chance to lose some things from your pockets
        // that are hanging off your character
        if( target->is_avatar() ) {
            std::vector<item_pocket *> pd = target->as_character()->worn.grab_drop_pockets();
            // if we have items that can be pulled off
            if( !pd.empty() ) {
                // choose an item to be ripped off
                int index = rng( 0, pd.size() - 1 );
                // the chance the monster knocks an item off
                int chance = rng( 0, z.type->melee_sides * z.type->melee_dice );
                // the chance the pocket resists
                int sturdiness = rng( 0, pd[index]->get_pocket_data()->ripoff );
                // the item is ripped off your character
                if( sturdiness < chance ) {
                    float path_distance = rng_float( 0, 1.0 );
                    tripoint vector = target->pos() - z.pos();
                    vector = tripoint( vector.x * path_distance, vector.y * path_distance, vector.z * path_distance );
                    pd[index]->spill_contents( z.pos() + vector );
                    add_msg( m_bad, _( "As you hit the ground something comes loose and is knocked away from you!" ) );
                    popup( _( "As you hit the ground something comes loose and is knocked away from you!" ) );
                }
            }
        }
    }

    return true;
}

void melee_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    if( target.is_avatar() ) {
        sfx::play_variant_sound( "mon_bite", "bite_hit", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        sfx::do_player_death_hurt( dynamic_cast<Character &>( target ), false );
    }
    game_message_type msg_type = target.attitude_to( get_player_character() ) ==
                                 Creature::Attitude::FRIENDLY ?
                                 m_bad : m_neutral;
    const bodypart_id &bp = dealt.bp_hit ;
    const std::string mon_name = get_player_character().sees( z.pos() ) ?
                                 z.disp_name( false, true ) : _( "Something" );
    target.add_msg_player_or_npc( msg_type, hit_dmg_u, hit_dmg_npc, mon_name,
                                  body_part_name_accusative( bp ) );

    for( const mon_effect_data &eff : effects ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const bodypart_id affected_bp = eff.affect_hit_bp ? bp : eff.bp.id();
            target.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first,
                               eff.duration.second ) ), affected_bp, eff.permanent, rng( eff.intensity.first,
                                       eff.intensity.second ) );
        }
    }

    // Apply ondmg self effects
    for( const mon_effect_data &eff : self_effects_ondmg ) {
        if( x_in_y( eff.chance, 100 ) ) {
            z.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first, eff.duration.second ) ),
                          eff.permanent,
                          rng( eff.intensity.first, eff.intensity.second ) );
            target.add_msg_if_player( m_mixed, eff.message, mon_name );
        }
    }
}

std::unique_ptr<mattack_actor> melee_actor::clone() const
{
    return std::make_unique<melee_actor>( *this );
}

bite_actor::bite_actor() = default;

void bite_actor::load_internal( const JsonObject &obj, const std::string &src )
{
    // Infection chance is a % (i.e. 5/100)
    melee_actor::load_internal( obj, src );
    // If min hitsize is undefined restrict it to not biting eyes/mouths
    // Hands are fair game, though
    if( hitsize_min == -1 ) {
        hitsize_min = 1;
    }
    infection_chance = obj.get_int( "infection_chance", 5 );
}

void bite_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    melee_actor::on_damage( z, target, dealt );

    if( x_in_y( infection_chance, 100 ) ) {
        const bodypart_id &hit = dealt.bp_hit;
        if( target.has_effect( effect_bite, hit.id() ) ) {
            target.add_effect( effect_bite, 40_minutes, hit, true );
        } else if( target.has_effect( effect_infected, hit.id() ) ) {
            target.add_effect( effect_infected, 25_minutes, hit, true );
        } else {
            target.add_effect( effect_bite, 1_turns, hit, true );
        }
    }

    // Flag only set for zombies in the deadly_bites mod
    if( x_in_y( infection_chance, 20 ) ) {
        if( z.has_flag( MF_DEADLY_VIRUS ) && !target.has_effect( effect_zombie_virus ) ) {
            target.add_effect( effect_zombie_virus, 1_turns, bodypart_str_id::NULL_ID(), true );
        } else if( z.has_flag( MF_VAMP_VIRUS ) && !target.has_trait( trait_VAMPIRE ) ) {
            target.add_effect( effect_vampire_virus, 1_turns, bodypart_str_id::NULL_ID(), true );
        }
    }

    if( target.has_trait( trait_TOXICFLESH ) ) {
        z.add_effect( effect_poison, 5_minutes );
        z.add_effect( effect_badpoison, 5_minutes );
    }
}

std::unique_ptr<mattack_actor> bite_actor::clone() const
{
    return std::make_unique<bite_actor>( *this );
}

gun_actor::gun_actor() : description( to_translation( "The %1$s fires its %2$s!" ) ),
    targeting_sound( to_translation( "beep-beep-beep!" ) )
{
}

void gun_actor::load_internal( const JsonObject &obj, const std::string & )
{
    obj.read( "gun_type", gun_type, true );

    obj.read( "ammo_type", ammo_type );

    if( obj.has_array( "fake_skills" ) ) {
        for( JsonArray cur : obj.get_array( "fake_skills" ) ) {
            fake_skills[skill_id( cur.get_string( 0 ) )] = cur.get_int( 1 );
        }
    }

    optional( obj, was_loaded, "fake_str", fake_str, 8 );
    optional( obj, was_loaded, "fake_dex", fake_dex, 8 );
    optional( obj, was_loaded, "fake_int", fake_int, 8 );
    optional( obj, was_loaded, "fake_per", fake_per, 8 );

    for( JsonArray mode : obj.get_array( "ranges" ) ) {
        if( mode.size() < 2 || mode.get_int( 0 ) > mode.get_int( 1 ) ) {
            obj.throw_error_at( "ranges", "incomplete or invalid range specified" );
        }
        ranges.emplace( std::make_pair<int, int>( mode.get_int( 0 ), mode.get_int( 1 ) ),
                        gun_mode_id( mode.size() > 2 ? mode.get_string( 2 ) : "" ) );
    }

    obj.read( "max_ammo", max_ammo );

    obj.read( "move_cost", move_cost );

    obj.read( "description", description );
    obj.read( "failure_msg", failure_msg );
    if( !obj.read( "no_ammo_sound", no_ammo_sound ) ) {
        no_ammo_sound = to_translation( "Click." );
    }

    obj.read( "targeting_cost", targeting_cost );

    obj.read( "require_targeting_player", require_targeting_player );
    obj.read( "require_targeting_npc", require_targeting_npc );
    obj.read( "require_targeting_monster", require_targeting_monster );

    obj.read( "targeting_timeout", targeting_timeout );
    obj.read( "targeting_timeout_extend", targeting_timeout_extend );

    if( !obj.read( "targeting_sound", targeting_sound ) ) {
        targeting_sound = to_translation( "Beep." );
    }

    obj.read( "targeting_volume", targeting_volume );

    laser_lock = obj.get_bool( "laser_lock", false );

    obj.read( "target_moving_vehicles", target_moving_vehicles );

    obj.read( "require_sunlight", require_sunlight );
}

std::unique_ptr<mattack_actor> gun_actor::clone() const
{
    return std::make_unique<gun_actor>( *this );
}

int gun_actor::get_max_range()  const
{
    int max_range = 0;
    for( const auto &e : ranges ) {
        max_range = std::max( std::max( max_range, e.first.first ), e.first.second );
    }

    add_msg_debug( debugmode::DF_MATTACK, "Max range %d", max_range );
    return max_range;
}

bool gun_actor::call( monster &z ) const
{
    Creature *target;
    tripoint aim_at;
    bool untargeted = false;

    if( z.friendly ) {
        int max_range = get_max_range();

        int hostiles; // hostiles which cannot be engaged without risking friendly fire
        target = z.auto_find_hostile_target( max_range, hostiles );
        if( !target ) {
            if( hostiles > 0 ) {
                add_msg_if_player_sees( z, m_warning,
                                        n_gettext( "Pointed in your direction, the %s emits an IFF warning beep.",
                                                   "Pointed in your direction, the %s emits %d annoyed sounding beeps.",
                                                   hostiles ),
                                        z.name(), hostiles );
            }
            return false;
        }
        aim_at = target->pos();
    } else {
        target = z.attack_target();
        aim_at = target ? target->pos() : tripoint_zero;
        if( !target || !z.sees( *target ) || ( !target->is_monster() && !z.aggro_character ) ) {
            if( !target_moving_vehicles ) {
                return false;
            }
            untargeted = true; // no living targets, try to find moving car parts
            const std::set<tripoint_bub_ms> moving_veh_parts = get_map()
                    .get_moving_vehicle_targets( z, get_max_range() );
            if( moving_veh_parts.empty() ) {
                return false;
            }
            aim_at = random_entry( moving_veh_parts, tripoint_bub_ms() ).raw();
        }
    }

    const int dist = rl_dist( z.pos(), aim_at );
    if( target ) {
        add_msg_debug( debugmode::DF_MATTACK, "Target %s at range %d", target->disp_name(), dist );
    } else {
        add_msg_debug( debugmode::DF_MATTACK, "Shooting at vehicle at range %d", dist );
    }

    for( const auto &e : ranges ) {
        if( dist >= e.first.first && dist <= e.first.second ) {
            if( untargeted || try_target( z, *target ) ) {
                shoot( z, aim_at, e.second );
            }
            return true;
        }
    }
    return false;
}

bool gun_actor::try_target( monster &z, Creature &target ) const
{
    if( require_sunlight && !g->is_in_sunlight( z.pos() ) ) {
        add_msg_debug( debugmode::DF_MATTACK, "Requires sunlight" );
        if( one_in( 3 ) ) {
            add_msg_if_player_sees( z, failure_msg.translated(), z.name() );
        }
        return false;
    }

    const bool require_targeting = ( require_targeting_player && target.is_avatar() ) ||
                                   ( require_targeting_npc && target.is_npc() ) ||
                                   ( require_targeting_monster && target.is_monster() );
    const bool not_targeted = require_targeting && !z.has_effect( effect_targeted );
    const bool not_laser_locked = require_targeting && laser_lock &&
                                  !target.has_effect( effect_was_laserlocked );

    if( not_targeted || not_laser_locked ) {
        if( targeting_volume > 0 && !targeting_sound.empty() ) {
            sounds::sound( z.pos(), targeting_volume, sounds::sound_t::alarm,
                           targeting_sound );
        }
        if( not_targeted ) {
            add_msg_debug( debugmode::DF_MATTACK, "%s targeted", target.disp_name() );
            z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout ) );
        }
        if( not_laser_locked ) {
            add_msg_debug( debugmode::DF_MATTACK, "%s laser-locked", target.disp_name() );
            target.add_effect( effect_laserlocked, time_duration::from_turns( targeting_timeout ) );
            target.add_effect( effect_was_laserlocked, time_duration::from_turns( targeting_timeout ) );
            target.add_msg_if_player( m_warning,
                                      _( "You're not sure why you've got a laser dot on you…" ) );
        }

        z.moves -= targeting_cost;
        return false;
    }

    if( require_targeting ) {
        z.add_effect( effect_targeted, time_duration::from_turns( targeting_timeout_extend ) );
    }
    if( laser_lock ) {
        // To prevent spamming laser locks when the player can tank that stuff somehow
        target.add_effect( effect_was_laserlocked, 5_turns );
    }
    return true;
}

void gun_actor::shoot( monster &z, const tripoint &target, const gun_mode_id &mode,
                       int inital_recoil ) const
{
    z.moves -= move_cost;

    itype_id mig_gun_type = item_controller->migrate_id( gun_type );
    item gun( mig_gun_type );
    gun.gun_set_mode( mode );

    itype_id ammo = item_controller->migrate_id( ammo_type );
    if( ammo.is_null() ) {
        if( gun.magazine_integral() ) {
            ammo = gun.ammo_default();
        } else {
            ammo = item( gun.magazine_default() ).ammo_default();
        }
    }

    if( !ammo.is_null() ) {
        if( gun.magazine_integral() ) {
            gun.ammo_set( ammo, z.ammo[ammo_type] );
        } else {
            item mag( gun.magazine_default() );
            mag.ammo_set( ammo, z.ammo[ammo_type] );
            gun.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
        }
    }

    if( z.has_effect( effect_stunned ) || z.has_effect( effect_sensor_stun ) ) {
        return;
    }

    add_msg_debug( debugmode::DF_MATTACK, "%d ammo (%s) remaining", z.ammo[ammo_type],
                   gun.ammo_sort_name() );

    if( !gun.ammo_sufficient( nullptr ) ) {
        if( !no_ammo_sound.empty() ) {
            sounds::sound( z.pos(), 10, sounds::sound_t::combat, no_ammo_sound );
        }
        return;
    }

    standard_npc tmp( _( "The " ) + z.name(), z.pos(), {}, 8,
                      fake_str, fake_dex, fake_int, fake_per );
    tmp.worn.wear_item( tmp, item( "backpack" ), false, false, true, true );
    tmp.set_fake( true );
    tmp.set_attitude( z.friendly ? NPCATT_FOLLOW : NPCATT_KILL );

    tmp.recoil = inital_recoil; // set initial recoil
    bool throwing = false;
    for( const auto &pr : fake_skills ) {
        tmp.set_skill_level( pr.first, pr.second );
        throwing |= pr.first == skill_throw;
    }

    tmp.set_wielded_item( gun );
    tmp.i_add( item( "UPS_off", calendar::turn, 1000 ) );

    add_msg_if_player_sees( z, m_warning, description.translated(), z.name(),
                            tmp.get_wielded_item()->tname() );

    add_msg_debug( debugmode::DF_MATTACK,
                   "Temp NPC:\nSTR %d, DEX %d, INT %d, PER %d\nGun skill (%s) %d",
                   tmp.str_cur, tmp.dex_cur, tmp.int_cur, tmp.per_cur,
                   gun.gun_skill().c_str(), tmp.get_skill_level( throwing ? skill_throw : skill_gun ) );

    if( throwing ) {
        tmp.throw_item( target, item( ammo, calendar::turn, 1 ) );
        z.ammo[ammo_type]--;
    } else {
        z.ammo[ammo_type] -= tmp.fire_gun( target, gun.gun_current_mode().qty );
    }
}
