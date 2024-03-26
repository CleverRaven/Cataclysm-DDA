#include "npc_attack.h"

#include "cata_utility.h"
#include "character.h"
#include "creature_tracker.h"
#include "dialogue.h"
#include "flag.h"
#include "item.h"
#include "line.h"
#include "magic.h"
#include "magic_spell_effect_helpers.h"
#include "map.h"
#include "messages.h"
#include "npc.h"
#include "point.h"
#include "projectile.h"
#include "ranged.h"

static const bionic_id bio_hydraulics( "bio_hydraulics" );

static const json_character_flag json_flag_SUBTLE_SPELL( "SUBTLE_SPELL" );

namespace npc_attack_constants
{
// if you are attacking your target, multiply potential by this number
static const float target_modifier = 1.5f;
// if you kill the creature, multiply the potential by this number
static const float kill_modifier = 1.5f;
// the amount of penalty if the npc has to change what it's wielding
// update this number and comment when that is no longer a flat -15 moves
static const int base_time_penalty = 3;
// we want this out of our hands, pronto.
// give a large buff to the attack value so it prioritizes this
static const int base_throw_now = 10'000;
} // namespace npc_attack_constants

// TODO: make a better, more generic "check if this projectile is blocked" function
// TODO: put this in a namespace for reuse
static bool has_obstruction( const tripoint &from, const tripoint &to, bool check_ally = false )
{
    std::vector<tripoint> line = line_to( from, to );
    // @to is what we want to hit. we don't need to check for obstruction there.
    line.pop_back();
    const map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &line_point : line ) {
        if( here.impassable( line_point ) || ( check_ally && creatures.creature_at( line_point ) ) ) {
            return true;
        }
    }
    return false;
}

static bool can_move( const npc &source )
{
    return !source.in_vehicle && source.rules.engagement != combat_engagement::NO_MOVE;
}

static bool can_move_melee( const npc &source )
{
    return can_move( source ) && source.rules.engagement != combat_engagement::FREE_FIRE &&
           source.rules.engagement != combat_engagement::NO_MOVE;
}

bool npc_attack_rating::operator>( const npc_attack_rating &rhs ) const
{
    if( !rhs._value ) {
        return true;
    }
    if( !_value ) {
        return false;
    }
    return *_value > *rhs._value;
}

bool npc_attack_rating::operator>( const int rhs ) const
{
    if( !_value ) {
        return false;
    }
    return *_value > rhs;
}

bool npc_attack_rating::operator<( const int rhs ) const
{
    if( !_value ) {
        return true;
    }
    return *_value < rhs;
}

npc_attack_rating npc_attack_rating::operator-=( const int rhs )
{
    if( _value ) {
        *_value -= rhs;
    }
    return *this;
}

void npc_attack_spell::use( npc &source, const tripoint &location ) const
{
    spell &sp = source.magic->get_spell( attack_spell_id );
    if( source.has_weapon() && !source.get_wielded_item()->has_flag( flag_MAGIC_FOCUS ) &&
        !sp.has_flag( spell_flag::NO_HANDS ) && !source.has_flag( json_flag_SUBTLE_SPELL ) ) {
        source.unwield();
    }
    add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is casting %s", source.disp_name(), sp.name() );
    source.cast_spell( sp, false, location );
}

npc_attack_rating npc_attack_spell::evaluate( const npc &source,
        const Creature *target ) const
{
    const spell &attack_spell = source.magic->get_spell( attack_spell_id );
    npc_attack_rating effectiveness;
    if( !can_use( source ) ) {
        return effectiveness;
    }
    const int time_penalty = base_time_penalty( source );
    const std::vector<tripoint> targetable_points = attack_spell.targetable_locations( source );
    for( const tripoint &targetable_point : targetable_points ) {
        npc_attack_rating effectiveness_at_point = evaluate_tripoint(
                    source, target, targetable_point );
        effectiveness_at_point -= time_penalty;
        if( effectiveness_at_point > effectiveness ) {
            effectiveness = effectiveness_at_point;
        }
    }
    return effectiveness;
}

std::vector<npc_attack_rating> npc_attack_spell::all_evaluations( const npc &source,
        const Creature *target ) const
{
    const spell &attack_spell = source.magic->get_spell( attack_spell_id );
    std::vector<npc_attack_rating> effectiveness;
    if( !can_use( source ) ) {
        return effectiveness;
    }
    int time_penalty = this->base_time_penalty( source );
    const std::vector<tripoint> targetable_points = attack_spell.targetable_locations( source );
    for( const tripoint &targetable_point : targetable_points ) {
        npc_attack_rating effectiveness_at_point = evaluate_tripoint(
                    source, target, targetable_point );
        effectiveness_at_point -= time_penalty;
        effectiveness.push_back( effectiveness_at_point );
    }
    return effectiveness;
}

bool npc_attack_spell::can_use( const npc &source ) const
{
    const spell &attack_spell = source.magic->get_spell( attack_spell_id );
    // missing components or energy or something
    return attack_spell.can_cast( source ) &&
           // use the same rules as silent guns
           !( source.rules.has_flag( ally_rule::use_silent ) && attack_spell.sound_volume( source ) >= 5 );
}

int npc_attack_spell::base_time_penalty( const npc &source ) const
{
    const spell &attack_spell = source.magic->get_spell( attack_spell_id );
    int time_penalty = 0;
    if( source.has_weapon() && !source.get_wielded_item()->has_flag( flag_MAGIC_FOCUS ) &&
        !attack_spell.has_flag( spell_flag::NO_HANDS ) && !source.has_flag( json_flag_SUBTLE_SPELL ) ) {
        time_penalty += npc_attack_constants::base_time_penalty;
    }
    // costs a point per second spent.
    time_penalty += std::round( attack_spell.casting_time( source ) * source.speed_rating() ) / 100.0f;
    return time_penalty;
}

npc_attack_rating npc_attack_spell::evaluate_tripoint(
    const npc &source, const Creature *target, const tripoint &location ) const
{
    const spell &attack_spell = source.magic->get_spell( attack_spell_id );

    double total_potential = 0;

    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &potential_target : calculate_spell_effect_area( attack_spell, location,
            source ) ) {
        Creature *critter = creatures.creature_at( potential_target );

        if( !critter ) {
            // no critter? no damage! however, we assume fields are worth something
            if( attack_spell_id->field ) {
                dialogue d( get_talker_for( source ), nullptr );
                total_potential += static_cast<double>( attack_spell.field_intensity( source ) ) /
                                   static_cast<double>( attack_spell_id->field_chance.evaluate( d ) ) / 2.0;
            }
            continue;
        }

        const Creature::Attitude att = source.attitude_to( *critter );
        int damage = 0;
        if( source.sees( *critter ) ) {
            damage = attack_spell.dps( source, *critter );
        }
        const int distance_to_me = rl_dist( source.pos(), potential_target );
        const bool friendly_fire = att == Creature::Attitude::FRIENDLY &&
                                   !source.rules.has_flag( ally_rule::avoid_friendly_fire );
        int attitude_mult = 3;
        if( att == Creature::Attitude::FRIENDLY ) {
            attitude_mult = -10;
        } else if( att == Creature::Attitude::NEUTRAL || friendly_fire ) {
            // hitting a neutral creature isn't exactly desired, but it's a lot less than a friendly.
            // if friendly fire is on, we don't care too much, though if an available hit doesn't damage them it would be better.
            attitude_mult = -1;
        }
        int potential = damage * attitude_mult - distance_to_me + 1;
        if( target && critter == target ) {
            potential *= npc_attack_constants::target_modifier;
        }
        if( damage >= critter->get_hp() ) {
            potential *= npc_attack_constants::kill_modifier;
            if( att == Creature::Attitude::NEUTRAL ) {
                // we don't care if we kill a neutral creature in one hit
                potential = std::abs( potential );
            }
            const Creature *source_ptr = &source;
            if( att == Creature::Attitude::FRIENDLY || critter == source_ptr ) {
                // however we under no circumstances want to kill an ally (or ourselves!)
                return npc_attack_rating( std::nullopt, location );
            }
        }
        total_potential += potential;
    }
    return npc_attack_rating( static_cast<int>( std::round( total_potential ) ), location );
}

void npc_attack_melee::use( npc &source, const tripoint &location ) const
{
    if( !source.is_wielding( weapon ) ) {
        if( !source.wield( weapon ) ) {
            debugmsg( "ERROR: npc tried to equip a weapon it couldn't wield" );
        }
        return;
    }
    Creature *critter = get_creature_tracker().creature_at( location );
    if( !critter ) {
        debugmsg( "ERROR: npc tried to attack null critter" );
        return;
    }
    int target_distance = rl_dist( source.pos(), location );
    if( !source.is_adjacent( critter, true ) ) {
        if( target_distance <= weapon.reach_range( source ) ) {
            add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is attempting a reach attack",
                           source.disp_name() );
            // check for friendlies in the line of fire
            std::vector<tripoint> path = line_to( source.pos(), location );
            path.pop_back(); // Last point is the target
            bool can_attack = true;
            for( const tripoint &path_point : path ) {
                Creature *inter = get_creature_tracker().creature_at( path_point );
                if( inter != nullptr && source.attitude_to( *inter ) == Creature::Attitude::FRIENDLY ) {
                    add_msg_debug( debugmode::debug_filter::DF_NPC, "%s aborted a reach attack; ally in the way",
                                   source.disp_name() );
                    can_attack = false;
                    break;
                }
            }
            if( can_attack ) {
                source.reach_attack( location );
            } else if( can_move_melee( source ) ) {
                source.avoid_friendly_fire();
            } else {
                source.look_for_player( get_player_character() );
            }
        } else {
            source.update_path( location );
            if( source.path.size() > 1 ) {
                bool clear_path = can_move_melee( source );
                if( clear_path && source.mem_combat.formation_distance == -1 ) {
                    source.move_to_next();
                    add_msg_debug( debugmode::DF_NPC_MOVEAI,
                                   "<color_light_gray>%s has no nearby ranged allies.  Going for attack.</color>", source.name );
                } else if( clear_path && source.mem_combat.formation_distance > target_distance ) {
                    source.move_to_next();
                    add_msg_debug( debugmode::DF_NPC_MOVEAI,
                                   "<color_light_gray>%s is at least %i away from ranged allies, enemy within %i.  Going for attack.</color>",
                                   source.name, source.mem_combat.formation_distance, target_distance );
                } else if( clear_path &&
                           source.mem_combat.formation_distance > source.closest_enemy_to_friendly_distance() ) {
                    source.move_to_next();
                    //add_msg_debug( debugmode::DF_NPC_MOVEAI,
                    //               "<color_light_gray>%s is at least %i away from allies, enemy within %i of ally.  Going for attack.</color>",
                    //               source.name, source.mem_combat.formation_distance, source.closest_enemy_to_friendly_distance() );
                } else if( source.mem_combat.formation_distance <= source.mem_combat.engagement_distance ) {
                    add_msg_debug( debugmode::DF_NPC_MOVEAI,
                                   "<color_light_gray>%s can't path to melee target, and is staying close to ranged allies.  Stay in place.</color>",
                                   source.name );
                    source.move_pause();
                } else {
                    add_msg_debug( debugmode::DF_NPC_MOVEAI,
                                   "<color_light_gray>%s can't path to melee target, and is not staying close to ranged allies.  Get close to player.</color>",
                                   source.name );
                    source.look_for_player( get_player_character() );
                }
            } else if( source.path.size() == 1 ) {
                if( critter != nullptr ) {
                    if( source.can_use_offensive_cbm() ) {
                        source.activate_bionic_by_id( bio_hydraulics );
                    }
                    add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is attempting a melee attack",
                                   source.disp_name() );
                    source.melee_attack( *critter, true );
                }
            } else {
                source.look_for_player( get_player_character() );
            }
        }
    } else if( source.mem_combat.formation_distance != -1 &&
               source.mem_combat.formation_distance <= target_distance &&
               rng( -10, 10 ) > source.personality.aggression ) {
        add_msg_debug( debugmode::DF_NPC_MOVEAI,
                       "<color_light_gray>%s decided to fall back to formation with allies.</color>", source.name );
        source.look_for_player( get_player_character() );
    } else {
        // may wish to add a break here to see if target is out of formation with allies, and consider running back.
        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is attempting a melee attack",
                       source.disp_name() );
        source.melee_attack( *critter, true );
    }
}

tripoint_range<tripoint> npc_attack_melee::targetable_points( const npc &source ) const
{
    return get_map().points_in_radius( source.pos(), 8 );
}

npc_attack_rating npc_attack_melee::evaluate( const npc &source,
        const Creature *target ) const
{
    npc_attack_rating effectiveness( std::nullopt, source.pos() );
    if( !can_use( source ) ) {
        return effectiveness;
    }
    const int time_penalty = base_time_penalty( source );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &targetable_point : targetable_points( source ) ) {
        if( Creature *critter = creatures.creature_at( targetable_point ) ) {
            if( source.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
                // no point in swinging a sword at a friendly!
                continue;
            }
            npc_attack_rating effectiveness_at_point = evaluate_critter( source, target, critter );
            effectiveness_at_point -= time_penalty;
            if( effectiveness_at_point > effectiveness ) {
                effectiveness = effectiveness_at_point;
            }
        }
    }
    return effectiveness;
}

std::vector<npc_attack_rating> npc_attack_melee::all_evaluations( const npc &source,
        const Creature *target ) const
{
    std::vector<npc_attack_rating> effectiveness;
    if( !can_use( source ) ) {
        return effectiveness;
    }
    const int time_penalty = base_time_penalty( source );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &targetable_point : targetable_points( source ) ) {
        if( Creature *critter = creatures.creature_at( targetable_point ) ) {
            if( source.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
                // no point in swinging a sword at a friendly!
                continue;
            }
            npc_attack_rating effectiveness_at_point = evaluate_critter( source, target, critter );
            effectiveness_at_point -= time_penalty;
            effectiveness.push_back( effectiveness_at_point );
        }
    }
    return effectiveness;
}

bool npc_attack_melee::can_use( const npc &source ) const
{
    // can't attack with something you can't wield
    return source.is_wielding( weapon ) || source.can_wield( weapon ).success();
}

int npc_attack_melee::base_time_penalty( const npc &source ) const
{
    return source.is_wielding( weapon ) ? 0 : npc_attack_constants::base_time_penalty;
}

npc_attack_rating npc_attack_melee::evaluate_critter( const npc &source,
        const Creature *target, Creature *critter ) const
{
    if( !critter ) {
        return npc_attack_rating{};
    }

    // TODO: Give bashing weapons a better
    // rating against armored targets
    double damage{ weapon.base_damage_melee().total_damage() };
    damage *= 100.0 / weapon.attack_time( source );
    const int reach_range{ weapon.reach_range( source ) };
    const int distance_to_me = clamp( rl_dist( source.pos(), critter->pos() ) - reach_range, 0, 10 );
    // Multiplier of 0.5f to 1.5f based on distance
    const float distance_multiplier = 1.5f - distance_to_me * 0.1f;
    double potential = damage * distance_multiplier;

    if( damage >= critter->get_hp() ) {
        potential *= npc_attack_constants::kill_modifier;
    }
    if( target && target->pos() == critter->pos() ) {
        potential *= npc_attack_constants::target_modifier;
    }

    return npc_attack_rating( static_cast<int>( std::round( potential ) ), critter->pos() );
}

void npc_attack_gun::use( npc &source, const tripoint &location ) const
{
    if( !source.is_wielding( gun ) ) {
        if( !source.wield( gun ) ) {
            debugmsg( "ERROR: npc tried to equip a weapon it couldn't wield" );
        }
        return;
    }

    if( !gun.ammo_sufficient( &source ) ) {
        // todo: make gun an item_location instead of getting wielded item here
        // but since wielding is required before this, it should be fine
        source.do_reload( source.get_wielded_item() );
        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is reloading %s", source.disp_name(),
                       gun.display_name() );
        return;
    }

    if( has_obstruction( source.pos(), location, false ) ||
        ( source.rules.has_flag( ally_rule::avoid_friendly_fire ) &&
          !source.wont_hit_friend( location, gun, false ) ) ) {
        if( can_move( source ) ) {
            source.avoid_friendly_fire();
        } else {
            source.move_pause();
        }
        return;
    }

    const int dist = rl_dist( source.pos(), location );

    // Only aim if we aren't in risk of being hit
    // TODO: Get distance to closest enemy
    if( dist > 1 && source.aim_per_move( gun, source.recoil ) > 0 &&
        source.confident_gun_mode_range( gunmode, source.recoil ) < dist ) {
        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s is aiming", source.disp_name() );
        source.aim( Target_attributes( source.pos(), location ) );
    } else {
        if( source.is_hallucination() ) {
            gun_mode mode = source.get_wielded_item()->gun_current_mode();
            source.pretend_fire( &source, mode.qty, *mode );
        } else {
            source.fire_gun( location );
        }
        add_msg_debug( debugmode::debug_filter::DF_NPC, "%s fires %s", source.disp_name(),
                       gun.display_name() );
    }
}

bool npc_attack_gun::can_use( const npc &source ) const
{
    // can't attack with something you can't wield
    return source.is_wielding( *gunmode ) || source.can_wield( *gunmode ).success();
}

int npc_attack_gun::base_time_penalty( const npc &source ) const
{
    const item &weapon = *gunmode;
    int time_penalty = 0;
    if( source.is_wielding( weapon ) ) {
        time_penalty += npc_attack_constants::base_time_penalty;
    }
    // we want the need to reload a gun cumulative with needing to wield the gun
    if( !weapon.ammo_sufficient( &source ) ) {
        time_penalty += npc_attack_constants::base_time_penalty;
    }
    int recoil_penalty = 0;
    if( source.is_wielding( weapon ) ) {
        recoil_penalty = source.recoil;
    } else {
        recoil_penalty = MAX_RECOIL;
    }
    recoil_penalty /= 100;
    return time_penalty + recoil_penalty;
}

tripoint_range<tripoint> npc_attack_gun::targetable_points( const npc &source ) const
{
    const item &weapon = *gunmode;
    return get_map().points_in_radius( source.pos(), weapon.gun_range() );
}

npc_attack_rating npc_attack_gun::evaluate(
    const npc &source, const Creature *target ) const
{
    npc_attack_rating effectiveness( std::nullopt, source.pos() );
    if( !can_use( source ) ) {
        return effectiveness;
    }
    const int time_penalty = base_time_penalty( source );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &targetable_point : targetable_points( source ) ) {
        if( creatures.creature_at( targetable_point ) ) {
            npc_attack_rating effectiveness_at_point = evaluate_tripoint( source, target,
                    targetable_point );
            effectiveness_at_point -= time_penalty;
            if( effectiveness_at_point > effectiveness ) {
                effectiveness = effectiveness_at_point;
            }
        }
    }
    return effectiveness;
}

std::vector<npc_attack_rating> npc_attack_gun::all_evaluations( const npc &source,
        const Creature *target ) const
{
    std::vector<npc_attack_rating> effectiveness;
    if( !can_use( source ) ) {
        return effectiveness;
    }
    const int time_penalty = base_time_penalty( source );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &targetable_point : targetable_points( source ) ) {
        if( creatures.creature_at( targetable_point ) ) {
            npc_attack_rating effectiveness_at_point = evaluate_tripoint( source, target,
                    targetable_point );
            effectiveness_at_point -= time_penalty;
            effectiveness.push_back( effectiveness_at_point );
        }
    }
    return effectiveness;
}

npc_attack_rating npc_attack_gun::evaluate_tripoint(
    const npc &source, const Creature *target, const tripoint &location ) const
{
    const item &gun = *gunmode.target;
    const int damage = gun.gun_damage().total_damage() * gunmode.qty;
    double potential = damage;

    const Creature *critter = get_creature_tracker().creature_at( location );
    if( !critter ) {
        // TODO: AOE ammo effects
        return npc_attack_rating( std::nullopt, location );
    }

    const Creature::Attitude att = source.attitude_to( *critter );
    if( att != Creature::Attitude::HOSTILE ) {
        // No point in attacking neutral/friendly targets
        return npc_attack_rating( std::nullopt, location );
    }

    const bool avoids_friendly_fire = source.rules.has_flag( ally_rule::avoid_friendly_fire );
    const int distance_to_me = rl_dist( location, source.pos() );

    // Make attacks that involve moving to find clear LOS slightly less likely
    if( has_obstruction( source.pos(), location, avoids_friendly_fire ) ) {
        potential *= 0.9f;
    } else if( avoids_friendly_fire && !source.wont_hit_friend( location, gun, false ) ) {
        potential *= 0.95f;
    }

    // Prefer targets not directly next to you
    potential *= distance_to_me > 2 ? 1.15f : 0.85f;

    if( damage >= critter->get_hp() ) {
        potential *= npc_attack_constants::kill_modifier;
    }
    if( target && target->pos() == critter->pos() ) {
        potential *= npc_attack_constants::target_modifier;
    }
    return npc_attack_rating( static_cast<int>( std::round( potential ) ), location );
}

void npc_attack_activate_item::use( npc &source, const tripoint &/*location*/ ) const
{
    if( !source.wield( activatable_item ) ) {
        debugmsg( "%s can't wield %s it tried to activate", source.disp_name(),
                  activatable_item.display_name() );
    }
    source.activate_item( activatable_item );
}

bool npc_attack_activate_item::can_use( const npc &source ) const
{
    if( !activatable_item.has_flag( flag_NPC_ACTIVATE ) ) {
        //TODO: make this more complete. only does BOMB type items
        return false;
    }
    const bool can_use_grenades = !source.is_hallucination() && ( !source.is_player_ally() ||
                                  source.rules.has_flag( ally_rule::use_grenades ) );
    return can_use_grenades;
}

npc_attack_rating npc_attack_activate_item::evaluate(
    const npc &source, const Creature * /*target*/ ) const
{
    if( !can_use( source ) ) {
        return npc_attack_rating( std::nullopt, source.pos() );
    }
    // until we have better logic for grenades it's better to keep this as a last resort...
    const int emergency = source.emergency() ? 1 : 0;
    return npc_attack_rating( emergency, source.pos() );
}

std::vector<npc_attack_rating> npc_attack_activate_item::all_evaluations( const npc &source,
        const Creature * /*target*/ ) const
{
    std::vector<npc_attack_rating> effectiveness;
    if( !can_use( source ) ) {
        return effectiveness;
    }
    // until we have better logic for grenades it's better to keep this as a last resort...
    const int emergency = source.emergency() ? 1 : 0;
    effectiveness.emplace_back( emergency, source.pos() );
    return effectiveness;
}

void npc_attack_throw::use( npc &source, const tripoint &location ) const
{
    if( !source.is_wielding( thrown_item ) ) {
        if( !source.wield( thrown_item ) ) {
            debugmsg( "ERROR: npc tried to equip a weapon it couldn't wield" );
        }
        return;
    }

    if( has_obstruction( source.pos(), location, false ) ||
        ( source.rules.has_flag( ally_rule::avoid_friendly_fire ) &&
          !source.wont_hit_friend( location, thrown_item, false ) ) ) {
        if( can_move( source ) ) {
            source.avoid_friendly_fire();
        } else {
            source.move_pause();
        }
        return;
    }

    item_location weapon = source.get_wielded_item();
    add_msg_debug( debugmode::debug_filter::DF_NPC, "%s throws the %s", source.disp_name(),
                   weapon->display_name() );
    item thrown( *weapon );
    if( weapon->count_by_charges() && weapon->charges > 1 ) {
        weapon->mod_charges( -1 );
        thrown.charges = 1;
    } else {
        source.remove_weapon();
    }
    source.throw_item( location, thrown );
}

bool npc_attack_throw::can_use( const npc &source ) const
{
    // Don't throw anything if we're hallucination
    // TODO: make an analogue of pretend_fire function
    if( source.is_hallucination() ) {
        return false;
    }

    if( !source.is_wielding( thrown_item ) && !source.can_wield( thrown_item ).success() ) {
        return false;
    }

    item single_item( thrown_item );
    if( single_item.count_by_charges() ) {
        single_item.charges = 1;
    }

    // Always allow throwing items that are flagged as throw now to
    // get rid of dangerous items ASAP, even if ranged attacks aren't allowed
    if( thrown_item.has_flag( flag_NPC_THROW_NOW ) ) {
        return true;
    }

    if( source.is_player_ally() && !source.rules.has_flag( ally_rule::use_guns ) ) {
        return false;
    }

    // Items flagged as NPC_THROWN are always allowed
    if( thrown_item.has_flag( flag_NPC_THROWN ) ) {
        return true;
    }

    bool throwable = source.throw_range( single_item ) > 0 && !source.is_worn( thrown_item ) &&
                     !thrown_item.has_flag( flag_NPC_ACTIVATE );
    throwable = throwable && !thrown_item.is_gun() && !thrown_item.is_armor() &&
                !thrown_item.is_comestible() && !thrown_item.is_magazine() && !thrown_item.is_tool();
    // TODO: Better choose what should be thrown
    return throwable;
}

int npc_attack_throw::base_penalty( const npc &source ) const
{
    item single_item( thrown_item );
    if( single_item.count_by_charges() ) {
        single_item.charges = 1;
    }
    const int time_penalty = source.is_wielding( single_item ) ? 0 :
                             npc_attack_constants::base_time_penalty;

    return time_penalty;
}

tripoint_range<tripoint> npc_attack_throw::targetable_points( const npc &source ) const
{
    item single_item( thrown_item );
    if( single_item.count_by_charges() ) {
        single_item.charges = 1;
    }
    const int range = source.throw_range( single_item );
    return get_map().points_in_radius( source.pos(), range );
}

npc_attack_rating npc_attack_throw::evaluate(
    const npc &source, const Creature *target ) const
{
    npc_attack_rating effectiveness( std::nullopt, source.pos() );
    if( !can_use( source ) ) {
        // please don't throw your pants...
        return effectiveness;
    }
    const int penalty = base_penalty( source );
    const bool throw_now = thrown_item.has_flag( flag_NPC_THROW_NOW );
    // TODO: Should this be a field to cache the result?
    const bool avoids_friendly_fire = source.rules.has_flag( ally_rule::avoid_friendly_fire );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &potential : targetable_points( source ) ) {

        // hot potato! HOT POTATO!
        // Calculated for all targetable points, not just those with targets
        if( throw_now ) {
            // TODO: Take into account distance to allies too
            const int distance_to_me = rl_dist( potential, source.pos() );
            int result = npc_attack_constants::base_throw_now + distance_to_me;
            if( !has_obstruction( source.pos(), potential, avoids_friendly_fire ) ) {
                // More likely to pick a target tile that isn't obstructed
                result += 100;
            }
            return npc_attack_rating( result, potential );
        }

        if( Creature *critter = creatures.creature_at( potential ) ) {
            if( source.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
                // no point in friendly fire!
                continue;
            }
            npc_attack_rating effectiveness_at_point = evaluate_tripoint( source, target,
                    potential );
            effectiveness_at_point -= penalty;
            if( effectiveness_at_point > effectiveness ) {
                effectiveness = effectiveness_at_point;
            }
        }
    }
    return effectiveness;
}

std::vector<npc_attack_rating> npc_attack_throw::all_evaluations( const npc &source,
        const Creature *target ) const
{
    std::vector<npc_attack_rating> effectiveness;
    if( !can_use( source ) ) {
        // please don't throw your pants...
        return effectiveness;
    }
    const int penalty = base_penalty( source );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &potential : targetable_points( source ) ) {
        if( Creature *critter = creatures.creature_at( potential ) ) {
            if( source.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
                // no point in friendly fire!
                continue;
            }
            npc_attack_rating effectiveness_at_point = evaluate_tripoint( source, target,
                    potential );
            effectiveness_at_point -= penalty;
            effectiveness.push_back( effectiveness_at_point );
        }
    }
    return effectiveness;
}

npc_attack_rating npc_attack_throw::evaluate_tripoint(
    const npc &source, const Creature *target, const tripoint &location ) const
{
    if( has_obstruction( source.pos(), location ) ) {
        return npc_attack_rating( std::nullopt, location );
    }
    item single_item( thrown_item );
    if( single_item.count_by_charges() ) {
        single_item.charges = 1;
    }

    Creature::Attitude att = Creature::Attitude::NEUTRAL;
    const Creature *critter = get_creature_tracker().creature_at( location );
    if( critter ) {
        att = source.attitude_to( *critter );
    }

    if( att != Creature::Attitude::HOSTILE ) {
        // No point in throwing stuff at neutral/friendly targets
        return npc_attack_rating( std::nullopt, location );
    }

    if( source.rules.has_flag( ally_rule::avoid_friendly_fire ) &&
        !source.wont_hit_friend( location, thrown_item, true ) ) {
        // Avoid friendy fire
        return npc_attack_rating( std::nullopt, location );
    }

    const float throw_mult = throw_cost( source, single_item ) * source.speed_rating() / 100.0f;
    const int damage = source.thrown_item_total_damage_raw( single_item );
    float dps = damage / throw_mult;
    const int distance_to_me = rl_dist( location, source.pos() );
    float suitable_item_mult = -0.15f;
    if( distance_to_me > 1 ) {
        if( thrown_item.has_flag( flag_NPC_THROWN ) ) {
            suitable_item_mult = 0.08f * std::min( distance_to_me - 1, 5 );
        }
    } else {
        suitable_item_mult = -0.35f;
    }

    double potential = dps * ( 1.0 + suitable_item_mult );
    if( potential > 0.0f && critter && damage >= critter->get_hp() ) {
        potential *= npc_attack_constants::kill_modifier;
    }
    if( potential > 0.0f && target && critter && target->pos() == critter->pos() ) {
        potential *= npc_attack_constants::target_modifier;
    }
    return npc_attack_rating( static_cast<int>( std::round( potential ) ), location );
}
