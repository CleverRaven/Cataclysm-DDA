#include "mattack_actors.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
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
#include "input.h"
#include "item.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "tileray.h"
#include "translations.h"
#include "ui_manager.h"
#include "vehicle.h"
#include "veh_type.h"
#include "viewer.h"

static const damage_type_id damage_bash( "bash" );

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_bite( "bite" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_infected( "infected" );
static const efftype_id effect_laserlocked( "laserlocked" );
static const efftype_id effect_null( "null" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_psi_stunned( "psi_stunned" );
static const efftype_id effect_run( "run" );
static const efftype_id effect_sensor_stun( "sensor_stun" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_targeted( "targeted" );
static const efftype_id effect_vampire_virus( "vampire_virus" );
static const efftype_id effect_was_laserlocked( "was_laserlocked" );
static const efftype_id effect_zombie_virus( "zombie_virus" );

static const flag_id json_flag_GRAB( "GRAB" );
static const flag_id json_flag_GRAB_FILTER( "GRAB_FILTER" );

static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );

static const skill_id skill_gun( "gun" );
static const skill_id skill_throw( "throw" );

static const trait_id trait_TOXICFLESH( "TOXICFLESH" );
static const trait_id trait_VAMPIRE( "VAMPIRE" );

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
    optional( obj, was_loaded, "ignore_dest_terrain", ignore_dest_terrain, false );
    optional( obj, was_loaded, "ignore_dest_danger", ignore_dest_danger, false );
    move_cost = obj.get_int( "move_cost", 150 );
    min_consider_range = obj.get_float( "min_consider_range", 0.0f );
    max_consider_range = obj.get_float( "max_consider_range", 200.0f );
    optional( obj, was_loaded, "message", message,
              to_translation( "The %s leaps!" ) );

    if( obj.has_member( "condition" ) ) {
        read_condition( obj, "condition", condition, false );
        has_condition = true;
    }

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

    if( has_condition ) {
        dialogue d( get_talker_for( &z ), nullptr );
        if( !condition( d ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Attack conditionals failed" );
            return false;
        }
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
        if( !ignore_dest_terrain && !z.will_move_to( candidate ) ) {
            add_msg_debug( debugmode::DF_MATTACK,
                           "Candidate place it can't enter, discarded" );
            continue;
        }
        if( !ignore_dest_danger && !z.know_danger_at( candidate ) ) {
            add_msg_debug( debugmode::DF_MATTACK,
                           "Candidate with dangerous conditions, discarded" );
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
            } else if( here.has_flag_ter( ter_furn_flag::TFLAG_SMALL_PASSAGE, i ) &&
                       z.get_size() > creature_size::medium ) {
                add_msg_debug( debugmode::DF_MATTACK, "Small passage can't pass, candidate discarded" );
                blocked_path = true;
                break;
            }
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

    z.mod_moves( -move_cost );
    viewer &player_view = get_player_view();
    const tripoint chosen = random_entry( options );
    bool seen = player_view.sees( z ); // We can see them jump...
    z.setpos( chosen );
    seen |= player_view.sees( z ); // ... or we can see them land
    if( seen && get_option<bool>( "LOG_MONSTER_MOVEMENT" ) ) {
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
    optional( obj, was_loaded, "allow_no_target", allow_no_target, false );

    if( obj.has_member( "condition" ) ) {
        read_condition( obj, "condition", condition, false );
        has_condition = true;
    }

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

    if( has_condition ) {
        dialogue d( get_talker_for( &mon ),
                    allow_no_target ? nullptr : get_talker_for( mon.attack_target() ) );
        if( !condition( d ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Attack conditionals failed" );
            return false;
        }
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
    mon.mod_moves( -spell_instance.casting_time( fake_player, true ) );
    spell_instance.cast_all_effects( mon, target );

    return true;
}

void grab::load_grab( const JsonObject &jo )
{
    optional( jo, was_loaded, "grab_strength", grab_strength, -1 );
    optional( jo, was_loaded, "pull_chance", pull_chance, -1 );
    optional( jo, was_loaded, "grab_effect", grab_effect, effect_grabbed );
    optional( jo, was_loaded, "exclusive_grab", exclusive_grab, false );
    optional( jo, was_loaded, "respect_seatbelts", respect_seatbelts, true );
    optional( jo, was_loaded, "drag_distance", drag_distance, 0 );
    optional( jo, was_loaded, "drag_deviation", drag_deviation, 0 );
    optional( jo, was_loaded, "drag_grab_break_distance", drag_grab_break_distance, 0 );
    optional( jo, was_loaded, "drag_movecost_mod", drag_movecost_mod, 1.0f );
    optional( jo, was_loaded, "pull_msg_u", pull_msg_u, to_translation( "%s pulls you in!" ) );
    optional( jo, was_loaded, "pull_fail_msg_u", pull_fail_msg_u,
              to_translation( "%s strains trying to pull you in but fails!" ) );
    optional( jo, was_loaded, "pull_msg_npc", pull_msg_npc,
              to_translation( "%s pulls <npcname> in!" ) );
    optional( jo, was_loaded, "pull_fail_msg_npc", pull_fail_msg_npc,
              to_translation( "%s strains trying to pull <npcname> in but fails!" ) );
    optional( jo, was_loaded, "pull_weight_ratio", pull_weight_ratio, 0.75f );
    was_loaded = true;
}

melee_actor::melee_actor()
{
    damage_max_instance = damage_instance();
    // FIXME: Hardcoded damage type
    damage_max_instance.add_damage( damage_bash, 9 );
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
    optional( obj, was_loaded, "effects_require_organic", effects_require_organic, false );
    optional( obj, was_loaded, "grab", is_grab, false );
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

    if( obj.has_member( "condition" ) ) {
        read_condition( obj, "condition", condition, false );
        has_condition = true;
    }
    if( is_grab ) {
        grab_data.load_grab( obj.get_object( "grab_data" ) );
    }

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

int melee_actor::do_grab( monster &z, Creature *target, bodypart_id bp_id ) const
{
    // Something went wrong
    if( !target ) {
        return -1;
    }
    // Handle some messaging in-grab
    game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
    const std::string mon_name = get_player_character().sees( z.pos() ) ?
                                 z.disp_name( false, true ) : _( "Something" );
    Character *foe = target->as_character();
    map &here = get_map();

    int eff_grab_strength = grab_data.grab_strength == -1 ? z.get_grab_strength() :
                            grab_data.grab_strength;
    add_msg_debug( debugmode::DF_MATTACK,
                   "Grab attack targeting bp %s, grab strength %d, pull chance %d", bp_id->name,
                   eff_grab_strength, grab_data.pull_chance );

    // Handle seatbelts and weight limits for pulls/drags TODO: tear you out depending on grab str?
    if( grab_data.pull_chance > -1 || grab_data.drag_distance > 0 ) {
        if( target->get_weight() > z.get_weight() * grab_data.pull_weight_ratio ) {
            target->add_msg_player_or_npc( msg_type, grab_data.pull_fail_msg_u, grab_data.pull_fail_msg_npc,
                                           mon_name );
            add_msg_debug( debugmode::DF_MATTACK, "Target weight %d g above weight limit  %.1f g, ",
                           to_gram( target->get_weight() ), to_gram( z.get_weight() ) * grab_data.pull_weight_ratio );
            return 0;
        }
        add_msg_debug( debugmode::DF_MATTACK, "Target weight %d g under weight limit  %.1f g, ",
                       to_gram( target->get_weight() ), to_gram( z.get_weight() ) * grab_data.pull_weight_ratio );
        const optional_vpart_position veh_part = here.veh_at( target->pos() );
        if( foe && foe->in_vehicle && veh_part ) {
            const std::optional<vpart_reference> vp_seatbelt = veh_part.avail_part_with_feature( "SEATBELT" );
            if( vp_seatbelt ) {
                if( grab_data.respect_seatbelts ) {
                    z.mod_moves( -move_cost * 2 );
                    foe->add_msg_player_or_npc( msg_type, _( "%1s tries to drag you, but is stopped by your %2s!" ),
                                                _( "%1s tries to drag <npcname>, but is stopped by their %2s!" ),
                                                z.disp_name( false, true ), vp_seatbelt->part().name( false ) );
                    add_msg_debug( debugmode::DF_MATTACK, "Target on vehicle part with seatbelt, attack failed" );
                    return 0;
                } else {
                    foe->add_msg_player_or_npc( msg_type, _( "%1s tears you out of your %2s!" ),
                                                _( "%1s tears <npcname> out of their %2s!" ), z.disp_name( false, true ),
                                                vp_seatbelt->part().name( false ) );
                    vp_seatbelt->vehicle().mod_hp( vp_seatbelt->part(), -2 );
                    add_msg_debug( debugmode::DF_MATTACK,
                                   "Target on vehicle part with seatbelt, attack does not respect seatbelts" );
                }
            }
        }
    }

    // Check if we want to animate any of this or just teleport you over
    const bool animate = get_option<bool>( "ANIMATIONS" );

    // Let's see if we manage to pull if we are a pull in the first place
    if( grab_data.pull_chance > -1 && x_in_y( grab_data.pull_chance, 100 ) ) {
        add_msg_debug( debugmode::DF_MATTACK, "Pull chance roll succeeded" );

        int pull_range = std::min( range, rl_dist( z.pos(), target->pos() ) + 1 );
        tripoint pt = target->pos();
        while( pull_range > 0 ) {
            // Recalculate the ray each step
            // We can't depend on either the target position being constant (obviously),
            // but neither on z pos staying constant, because we may want to shift the map mid-pull
            const units::angle dir = coord_to_angle( target->pos(), z.pos() );
            tileray tdir( dir );
            tdir.advance();
            pt.x = target->posx() + tdir.dx();
            pt.y = target->posy() + tdir.dy();
            //Cancel the grab if the space is occupied by something
            if( !g->is_empty( pt ) ) {
                break;
            }

            if( foe != nullptr ) {
                if( foe->in_vehicle ) {
                    here.unboard_vehicle( foe->pos() );
                }

                if( foe->is_avatar() && ( pt.x < HALF_MAPSIZE_X || pt.y < HALF_MAPSIZE_Y ||
                                          pt.x >= HALF_MAPSIZE_X + SEEX || pt.y >= HALF_MAPSIZE_Y + SEEY ) ) {
                    g->update_map( pt.x, pt.y );
                }
            }

            target->setpos( pt );
            pull_range--;
            if( animate ) {
                g->invalidate_main_ui_adaptor();
                inp_mngr.pump_events();
                ui_manager::redraw_invalidated();
                refresh_display();
            }
        }
        // If we're in a vehicle after being dragged, board us onto it
        // This prevents us from being run over by our own vehicle if we're dragged out of it
        if( foe != nullptr && !foe->in_vehicle &&
            here.veh_at( pt ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
            here.board_vehicle( pt, foe );
        }
        // The monster might drag a target that's not on it's z level
        // So if they leave them on open air, make them fall
        here.creature_on_trap( *target );

        target->add_msg_player_or_npc( msg_type, grab_data.pull_msg_u, grab_data.pull_msg_npc, mon_name,
                                       target->disp_name() );
    } else if( grab_data.pull_chance > -1 ) {
        // We failed the pull chance roll, return false to select a different attack
        add_msg_debug( debugmode::DF_MATTACK, "Pull chance roll failed.", grab_data.pull_chance );
        return -1;
    }

    if( grab_data.grab_effect != effect_null ) {
        if( foe ) {
            z.add_grab( bp_id.id() );
            add_msg_debug( debugmode::DF_MATTACK, "Added grabbing on %s",
                           bp_id->name );
            // Add grabbed - permanent, removal handled in try_remove_grab on move/wait
            target->add_effect( grab_data.grab_effect, 1_days, bp_id, true, eff_grab_strength );
        } else {
            // Monsters don't have limb scores, no need to target limbs
            target->add_effect( grab_data.grab_effect, 1_days, body_part_bp_null, true, eff_grab_strength );
            z.add_effect( effect_grabbing, 1_days, true, 1 );
        }
    }

    // Drag stuff
    if( grab_data.drag_distance > 0 ) {
        int distance = grab_data.drag_distance;
        while( distance > 0 ) {
            // Start with the opposite square
            tripoint opposite_square = z.pos() - ( target->pos() - z.pos() );
            // Keep track of our neighbors (no leaping)
            std::set<tripoint> neighbors;
            for( const tripoint &trp : here.points_in_radius( z.pos(), 1 ) ) {
                if( trp != z.pos() && trp != target->pos() ) {
                    neighbors.insert( trp );
                }
            }
            // Check where we get to consider dragging
            std::set<tripoint> candidates;
            for( const tripoint &trp : here.points_in_radius( opposite_square,
                    grab_data.drag_deviation ) ) {
                if( trp != z.pos() && trp != target->pos() ) {
                    candidates.insert( trp );
                }
            }
            // Select a random square from the options
            std::set<tripoint> intersect;
            std::set_intersection( neighbors.begin(), neighbors.end(), candidates.begin(), candidates.end(),
                                   std::inserter( intersect, intersect.begin() ) );
            std::set<tripoint>::iterator intersect_iter = intersect.begin();
            std::advance( intersect_iter, rng( 0, intersect.size() - 1 ) );
            tripoint target_square = random_entry<std::set<tripoint>>( intersect );
            if( z.can_move_to( target_square ) ) {
                monster *zz = target->as_monster();
                tripoint zpt = z.pos();
                z.move_to( target_square, false, false, grab_data.drag_movecost_mod );
                if( !g->is_empty( zpt ) ) { //Cancel the grab if the space is occupied by something
                    return 0;
                }
                if( target->is_avatar() && ( zpt.x < HALF_MAPSIZE_X ||
                                             zpt.y < HALF_MAPSIZE_Y ||
                                             zpt.x >= HALF_MAPSIZE_X + SEEX || zpt.y >= HALF_MAPSIZE_Y + SEEY ) ) {
                    g->update_map( zpt.x, zpt.y );
                }
                if( foe != nullptr ) {
                    if( foe->in_vehicle ) {
                        here.unboard_vehicle( foe->pos() );
                    }
                    foe->setpos( zpt );
                    if( !foe->in_vehicle && here.veh_at( zpt ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
                        here.board_vehicle( zpt, foe );
                    }
                } else {
                    zz->setpos( zpt );
                }
                target->add_msg_player_or_npc( m_bad, _( "You are dragged behind the %s!" ),
                                               _( "<npcname> gets dragged behind the %s!" ), z.name() );
                if( animate ) {
                    g->invalidate_main_ui_adaptor();
                    inp_mngr.pump_events();
                    ui_manager::redraw_invalidated();
                    refresh_display();
                }
            } else {
                target->add_msg_player_or_npc( m_good, _( "You resist the %s as it tries to drag you!" ),
                                               _( "<npcname> resist the %s as it tries to drag them!" ), z.name() );
                return 0;
            }
            distance--;
            if( grab_data.drag_grab_break_distance > 0 && foe ) {
                // Attempt to break the drag if we stepped the appropriate amount of distance
                if( ( grab_data.drag_distance - distance ) % grab_data.drag_grab_break_distance == 0 ) {
                    if( foe->try_remove_grab() ) {
                        return 1;
                    } else {
                        target->set_moves( 0 );
                    }
                }
            }
        }
    }
    return 1;
}

bool melee_actor::call( monster &z ) const
{
    Creature *target = find_target( z );
    if( target == nullptr ) {
        return false;
    }

    if( has_condition ) {
        dialogue d( get_talker_for( &z ), get_talker_for( *target ) );
        if( !condition( d ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Attack conditionals failed" );
            return false;
        }
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
    game_message_type msg_type = target->is_avatar() ? m_warning : m_info;

    add_msg_debug( debugmode::DF_MATTACK, "Accuracy %d, hitspread %d, dodgeable %s", acc, hitspread,
                   dodgeable ? "true" : "false" );

    if( z.has_flag( mon_flag_HIT_AND_RUN ) ) {
        z.add_effect( effect_run, 4_turns );
    }

    if( uncanny_dodgeable && target->uncanny_dodge() ) {
        game_message_type msg_type = target->is_avatar() ? m_warning : m_info;
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, miss_msg_u,
                                       get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ? miss_msg_npc : translation(),
                                       z.name(), body_part_name_accusative( bp_id ) );
        return true;
    }

    if( dodgeable ) {
        if( hitspread < 0 ) {
            sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                     sfx::get_heard_angle( z.pos() ) );
            target->add_msg_player_or_npc( msg_type, miss_msg_u,
                                           get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ? miss_msg_npc : translation(),
                                           mon_name, body_part_name_accusative( bp_id ) );
            return true;
        }
    }

    // We need to do some calculations in the main function - we might mutate bp_hit
    // But first we need to handle exclusive grabs etc.
    std::optional<bodypart_id> grabbed_bp_id;
    if( is_grab ) {
        int eff_grab_strength = grab_data.grab_strength == -1 ? z.get_grab_strength() :
                                grab_data.grab_strength;
        add_msg_debug( debugmode::DF_MATTACK,
                       "Grab attack targeting bp %s, grab strength %d, pull chance %d", bp_id->name,
                       eff_grab_strength, grab_data.pull_chance );

        // If we care iterate through grabs one by one, fail if we can't break one
        if( target->has_effect_with_flag( json_flag_GRAB ) && grab_data.exclusive_grab ) {
            add_msg_debug( debugmode::DF_MATTACK, "Exclusive grab, begin filtering" );
            map &here = get_map();
            const tripoint_range<tripoint> &surrounding = here.points_in_radius( target->pos(), 1, 0 );
            creature_tracker &creatures = get_creature_tracker();

            for( const effect &eff : target->get_effects_with_flag( json_flag_GRAB ) ) {
                monster *grabber = nullptr;
                // Iterate through the target's surroundings to find the grabber of this grab
                for( const tripoint loc : surrounding ) {
                    monster *mon = creatures.creature_at<monster>( loc );
                    if( mon && mon->has_effect_with_flag( json_flag_GRAB_FILTER ) && mon->attack_target() == target ) {
                        if( target->is_monster() || ( !target->is_monster() &&
                                                      mon->is_grabbing( eff.get_bp().id() ) ) ) {
                            grabber = mon;
                            break;
                        }
                    }
                }
                // Ignore our own grab
                if( grabber == &z ) {
                    add_msg_debug( debugmode::DF_MATTACK, "Grabber %s is the attacker, continue grab filtering",
                                   grabber->name() );
                    continue;
                }
                // Roll to remove anybody else's grab on our target
                if( !x_in_y( eff_grab_strength / 2.0f, eff.get_intensity() ) ) {
                    target->add_msg_player_or_npc( msg_type,
                                                   _( "%1s tries to drag you, but something holds you in place!" ),
                                                   _( "%1s tries to drag <npcname>, but something holds them in place!" ),
                                                   z.disp_name( false, true ) );
                    return true;

                } else {
                    target->remove_effect( eff.get_id(), eff.get_bp() );
                }
            }
        }

        // No second grabs on monsters (exclusive grabs got rolled before this)
        if( target->is_monster() && target->has_effect_with_flag( json_flag_GRAB ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Target monster already grabbed, attack failed silently" );
            return false;
        }

        // Filter out already-grabbed bodyparts (after attempting to remove them, in any case)
        // Unless we're not actually trying to grab
        if( target->has_effect_with_flag( json_flag_GRAB, bp_id ) &&
            grab_data.grab_effect != effect_null ) {
            // Iterate through eligable targets to look for a non-grabbed one, fail if none are found
            add_msg_debug( debugmode::DF_MATTACK, "Target bodypart %s already grabbed",
                           bp_id->name );
            for( const bodypart_id bp : target->get_all_eligable_parts( hitsize_min, hitsize_max,
                    attack_upper ) ) {
                if( target->has_effect_with_flag( json_flag_GRAB, bp ) ) {
                    add_msg_debug( debugmode::DF_MATTACK, "Alternative target bodypart %s already grabbed",
                                   bp->name );
                    continue;
                } else {
                    bp_hit = bp.id();
                    bp_id = bp;
                    add_msg_debug( debugmode::DF_MATTACK, "Retargeted to ungrabbed %s",
                                   bp->name );
                }
            }
            // Couldn't find an eligable limb, fail loudly
            if( target->has_effect_with_flag( json_flag_GRAB, bp_id ) ) {
                target->add_msg_player_or_npc( msg_type, miss_msg_u, miss_msg_npc, mon_name,
                                               body_part_name_accusative( bp_id ) );
                return true;
            }
        }
        int result = do_grab( z, target, bp_id );
        if( result < 0 ) {
            return false;
        } else if( result == 0 ) {
            return true;
        }
        grabbed_bp_id = bp_id;
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
            target->add_msg_if_player( msg_type, eff.message, mon_name );
        }
    }
    int damage_total = dealt_damage.total_damage();
    add_msg_debug( debugmode::DF_MATTACK, "%s's melee_attack did %d damage", z.name(), damage_total );
    if( damage_total > 0 ) {
        on_damage( z, *target, dealt_damage );
    } else {
        sfx::play_variant_sound( "mon_bite", "bite_miss", sfx::get_heard_volume( z.pos() ),
                                 sfx::get_heard_angle( z.pos() ) );
        target->add_msg_player_or_npc( msg_type, no_dmg_msg_u,
                                       get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ? no_dmg_msg_npc : translation(),
                                       mon_name, body_part_name_accusative( grabbed_bp_id.value_or( bp_id ) ) );
        if( !effects_require_dmg ) {
            for( const mon_effect_data &eff : effects ) {
                if( x_in_y( eff.chance, 100 ) ) {
                    const bodypart_id affected_bp = eff.affect_hit_bp ? bp_id : eff.bp.id();
                    if( !( effects_require_organic && affected_bp->has_flag( json_flag_BIONIC_LIMB ) ) ) {
                        target->add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first,
                                            eff.duration.second ) ), affected_bp, eff.permanent, rng( eff.intensity.first,
                                                    eff.intensity.second ) );
                    }
                }
            }
        }
    }
    if( throw_strength > 0 ) {
        if( g->fling_creature( target, coord_to_angle( z.pos(), target->pos() ),
                               throw_strength ) ) {
            target->add_msg_player_or_npc( msg_type, throw_msg_u,
                                           get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ? throw_msg_npc : translation(),
                                           mon_name );

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
    target.add_msg_player_or_npc( msg_type, hit_dmg_u,
                                  get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ? hit_dmg_npc : translation(),
                                  mon_name, body_part_name_accusative( bp ) );

    for( const mon_effect_data &eff : effects ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const bodypart_id affected_bp = eff.affect_hit_bp ? bp : eff.bp.id();
            if( !( effects_require_organic && affected_bp->has_flag( json_flag_BIONIC_LIMB ) ) ) {
                target.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first,
                                   eff.duration.second ) ), affected_bp, eff.permanent, rng( eff.intensity.first,
                                           eff.intensity.second ) );
            }
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
    optional( obj, was_loaded, "infection_chance", infection_chance, 5 );
}

void bite_actor::on_damage( monster &z, Creature &target, dealt_damage_instance &dealt ) const
{
    melee_actor::on_damage( z, target, dealt );
    add_msg_debug( debugmode::DF_MATTACK, "Bite-type attack, infection chance %d", infection_chance );
    const bodypart_id &hit = dealt.bp_hit;

    // only do bitey things if the limb is fleshy
    if( !hit->has_flag( json_flag_BIONIC_LIMB ) ) {
        // first, do regular zombie infections
        if( x_in_y( infection_chance, 100 ) ) {
            if( target.has_effect( effect_bite, hit.id() ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Incrementing bitten effect on %s", hit->name );
                target.add_effect( effect_bite, 40_minutes, hit, true );
            } else if( target.has_effect( effect_infected, hit.id() ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Incrementing infected effect on %s", hit->name );
                target.add_effect( effect_infected, 25_minutes, hit, true );
            } else {
                add_msg_debug( debugmode::DF_MATTACK, "Added bitten effect to %s", hit->name );
                target.add_effect( effect_bite, 1_turns, hit, true );
            }
        }
        // Flag only set for zombies in the deadly_bites mod
        if( x_in_y( infection_chance, 20 ) ) {
            if( z.has_flag( mon_flag_DEADLY_VIRUS ) && !target.has_effect( effect_zombie_virus ) ) {
                target.add_effect( effect_zombie_virus, 1_turns, bodypart_str_id::NULL_ID(), true );
            } else if( z.has_flag( mon_flag_VAMP_VIRUS ) && !target.has_trait( trait_VAMPIRE ) ) {
                target.add_effect( effect_vampire_virus, 1_turns, bodypart_str_id::NULL_ID(), true );
            }
        }
        // lastly, poison it if we're yucky
        if( target.has_trait( trait_TOXICFLESH ) ) {
            z.add_effect( effect_poison, 5_minutes );
            z.add_effect( effect_badpoison, 5_minutes );
        }
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

    if( obj.has_member( "condition" ) ) {
        read_condition( obj, "condition", condition, false );
        has_condition = true;
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

    if( has_condition ) {
        dialogue d( get_talker_for( &z ), nullptr );
        if( !condition( d ) ) {
            add_msg_debug( debugmode::DF_MATTACK, "Attack conditionals failed" );
            return false;
        }
    }

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

        z.mod_moves( -targeting_cost );
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
    z.mod_moves( -move_cost );

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
            gun.put_in( mag, pocket_type::MAGAZINE_WELL );
        }
    }

    if( z.has_effect( effect_stunned ) || z.has_effect( effect_psi_stunned ) ||
        z.has_effect( effect_sensor_stun ) ) {
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

    add_msg_if_player_sees( z, m_warning, description.translated(), z.name(),
                            tmp.get_wielded_item()->tname() );

    add_msg_debug( debugmode::DF_MATTACK,
                   "Temp NPC:\nSTR %d, DEX %d, INT %d, PER %d\nGun skill (%s) %d",
                   tmp.str_cur, tmp.dex_cur, tmp.int_cur, tmp.per_cur,
                   gun.gun_skill().c_str(), static_cast<int>( tmp.get_skill_level( throwing ? skill_throw :
                           skill_gun ) ) );

    if( throwing ) {
        tmp.throw_item( target, item( ammo, calendar::turn, 1 ) );
        z.ammo[ammo_type]--;
    } else {
        z.ammo[ammo_type] -= tmp.fire_gun( target, gun.gun_current_mode().qty );
    }
}
