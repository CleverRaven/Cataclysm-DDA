#include "ballistics.h"

#include <algorithm>

#include "creature.h"
#include "dispersion.h"
#include "explosion.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "trap.h"
#include "vehicle.h"
#include "vpart_position.h"

const efftype_id effect_bounced( "bounced" );

static void drop_or_embed_projectile( const dealt_projectile_attack &attack )
{
    const auto &proj = attack.proj;
    const auto &drop_item = proj.get_drop();
    const auto &effects = proj.proj_effects;
    if( drop_item.is_null() ) {
        return;
    }

    const tripoint &pt = attack.end_point;

    if( effects.count( "SHATTER_SELF" ) ) {
        // Drop the contents, not the thrown item
        if( g->u.sees( pt ) ) {
            add_msg( _( "The %s shatters!" ), drop_item.tname().c_str() );
        }

        for( const item &i : drop_item.contents ) {
            g->m.add_item_or_charges( pt, i );
        }
        // TODO: Non-glass breaking
        // TODO: Wine glass breaking vs. entire sheet of glass breaking
        sounds::sound( pt, 16, sounds::sound_t::combat, _( "glass breaking!" ) );
        return;
    }

    // Copy the item
    item dropped_item = drop_item;

    monster *mon = dynamic_cast<monster *>( attack.hit_critter );

    // We can only embed in monsters
    bool embed = mon != nullptr && !mon->is_dead_state();
    // And if we actually want to embed
    embed = embed && effects.count( "NO_EMBED" ) == 0;
    // Don't embed in small creatures
    if( embed ) {
        const m_size critter_size = mon->get_size();
        const units::volume vol = dropped_item.volume();
        embed = embed && ( critter_size > MS_TINY || vol < 250_ml );
        embed = embed && ( critter_size > MS_SMALL || vol < 500_ml );
        // And if we deal enough damage
        // Item volume bumps up the required damage too
        embed = embed &&
                ( attack.dealt_dam.type_damage( DT_CUT ) / 2 ) +
                attack.dealt_dam.type_damage( DT_STAB ) >
                attack.dealt_dam.type_damage( DT_BASH ) +
                vol * 3 / 250_ml + rng( 0, 5 );
    }

    if( embed ) {
        mon->add_item( dropped_item );
        if( g->u.sees( *mon ) ) {
            add_msg( _( "The %1$s embeds in %2$s!" ),
                     dropped_item.tname().c_str(),
                     mon->disp_name().c_str() );
        }
    } else {
        bool do_drop = true;
        if( effects.count( "ACT_ON_RANGED_HIT" ) ) {
            // Don't drop if it exploded
            do_drop = !dropped_item.process( nullptr, attack.end_point, true );
        }

        if( do_drop ) {
            g->m.add_item_or_charges( attack.end_point, dropped_item );
        }

        if( effects.count( "HEAVY_HIT" ) ) {
            if( g->m.has_flag( "LIQUID", pt ) ) {
                sounds::sound( pt, 10, sounds::sound_t::combat, _( "splash!" ) );
            } else {
                sounds::sound( pt, 8, sounds::sound_t::combat, _( "thud." ) );
            }
            const trap &tr = g->m.tr_at( pt );
            if( tr.triggered_by_item( dropped_item ) ) {
                tr.trigger( pt, nullptr );
            }
        }
    }
}

static size_t blood_trail_len( int damage )
{
    if( damage > 50 ) {
        return 3;
    } else if( damage > 20 ) {
        return 2;
    } else if( damage > 0 ) {
        return 1;
    }
    return 0;
}

projectile_attack_aim projectile_attack_roll( const dispersion_sources &dispersion, double range,
        double target_size )
{
    projectile_attack_aim aim;

    // dispersion is a measure of the dispersion of shots due to the gun + shooter characteristics
    // i.e. it is independent of any particular shot

    // shot_dispersion is the actual dispersion for this particular shot, i.e.
    // the error angle between where the shot was aimed and where this one actually went
    // NB: some cases pass dispersion == 0 for a "never misses" shot e.g. bio_magnet,
    aim.dispersion = dispersion.roll();

    // an isosceles triangle is formed by the intended and actual target tiles
    aim.missed_by_tiles = iso_tangent( range, aim.dispersion );

    // fraction we missed a monster target by (0.0 = perfect hit, 1.0 = miss)
    if( target_size > 0.0 ) {
        aim.missed_by = std::min( 1.0, aim.missed_by_tiles / target_size );
    } else {
        // Special case 0 size targets, just to be safe from 0.0/0.0 NaNs
        aim.missed_by = 1.0;
    }

    return aim;
}

dealt_projectile_attack projectile_attack( const projectile &proj_arg, const tripoint &source,
        const tripoint &target_arg, const dispersion_sources &dispersion,
        Creature *origin, const vehicle *in_veh )
{
    const bool do_animation = get_option<bool>( "ANIMATIONS" );

    double range = rl_dist( source, target_arg );

    Creature *target_critter = g->critter_at( target_arg );
    double target_size = target_critter != nullptr ?
                         target_critter->ranged_target_size() :
                         g->m.ranged_target_size( target_arg );
    projectile_attack_aim aim = projectile_attack_roll( dispersion, range, target_size );

    // TODO: move to-hit roll back in here

    dealt_projectile_attack attack {
        proj_arg, nullptr, dealt_damage_instance(), source, aim.missed_by
    };

    if( source == target_arg ) { // No suicidal shots
        debugmsg( "Projectile_attack targeted own square." );
        return attack;
    }

    projectile &proj = attack.proj;
    const auto &proj_effects = proj.proj_effects;

    const bool stream = proj_effects.count( "STREAM" ) > 0 ||
                        proj_effects.count( "STREAM_BIG" ) > 0 ||
                        proj_effects.count( "JET" ) > 0;
    const char bullet = stream ? '#' : '*';
    const bool no_item_damage = proj_effects.count( "NO_ITEM_DAMAGE" ) > 0;
    const bool do_draw_line = proj_effects.count( "DRAW_AS_LINE" ) > 0;
    const bool null_source = proj_effects.count( "NULL_SOURCE" ) > 0;
    // Determines whether it can penetrate obstacles
    const bool is_bullet = proj_arg.speed >= 200 && std::any_of( proj_arg.impact.damage_units.begin(),
                           proj_arg.impact.damage_units.end(),
    []( const damage_unit & dam ) {
        return dam.type == DT_CUT;
    } );

    // If we were targetting a tile rather than a monster, don't overshoot
    // Unless the target was a wall, then we are aiming high enough to overshoot
    const bool no_overshoot = proj_effects.count( "NO_OVERSHOOT" ) ||
                              ( g->critter_at( target_arg ) == nullptr && g->m.passable( target_arg ) );

    double extend_to_range = no_overshoot ? range : proj_arg.range;

    tripoint target = target_arg;
    std::vector<tripoint> trajectory;
    if( aim.missed_by_tiles >= 1.0 ) {
        // We missed enough to target a different tile
        double dx = target_arg.x - source.x;
        double dy = target_arg.y - source.y;
        double rad = atan2( dy, dx );

        // cap wild misses at +/- 30 degrees
        rad += ( one_in( 2 ) ? 1 : -1 ) * std::min( ARCMIN( aim.dispersion ), DEGREES( 30 ) );

        // @todo: This should also represent the miss on z axis
        const int offset = std::min<int>( range, sqrtf( aim.missed_by_tiles ) );
        int new_range = no_overshoot ?
                        range + rng( -offset, offset ) :
                        rng( range - offset, proj_arg.range );
        new_range = std::max( new_range, 1 );

        target.x = source.x + roll_remainder( new_range * cos( rad ) );
        target.y = source.y + roll_remainder( new_range * sin( rad ) );

        if( target == source ) {
            target.x = source.x + sgn( dx );
            target.y = source.y + sgn( dy );
        }

        // Don't extend range further, miss here can mean hitting the ground near the target
        range = rl_dist( source, target );
        extend_to_range = range;

        sfx::play_variant_sound( "bullet_hit", "hit_wall", sfx::get_heard_volume( target ),
                                 sfx::get_heard_angle( target ) );
        // TODO: Z dispersion
        // If we missed, just draw a straight line.
        trajectory = line_to( source, target );
    } else {
        // Go around obstacles a little if we're on target.
        trajectory = g->m.find_clear_path( source, target );
    }

    add_msg( m_debug, "missed_by_tiles: %.2f; missed_by: %.2f; target (orig/hit): %d,%d,%d/%d,%d,%d",
             aim.missed_by_tiles, aim.missed_by,
             target_arg.x, target_arg.y, target_arg.z,
             target.x, target.y, target.z );

    // Trace the trajectory, doing damage in order
    tripoint &tp = attack.end_point;
    tripoint prev_point = source;

    trajectory.insert( trajectory.begin(), source ); // Add the first point to the trajectory

    static emit_id muzzle_smoke( "emit_smoke_plume" );
    if( proj_effects.count( "MUZZLE_SMOKE" ) ) {
        g->m.emit_field( trajectory.front(), muzzle_smoke );
    }

    if( !no_overshoot && range < extend_to_range ) {
        // Continue line is very "stiff" when the original range is short
        // @todo: Make it use a more distant point for more realistic extended lines
        std::vector<tripoint> trajectory_extension = continue_line( trajectory,
                extend_to_range - range );
        trajectory.reserve( trajectory.size() + trajectory_extension.size() );
        trajectory.insert( trajectory.end(), trajectory_extension.begin(), trajectory_extension.end() );
    }
    // Range can be 0
    size_t traj_len = trajectory.size();
    while( traj_len > 0 && rl_dist( source, trajectory[traj_len - 1] ) > proj_arg.range ) {
        --traj_len;
    }

    const float projectile_skip_multiplier = 0.1;
    // Randomize the skip so that bursts look nicer
    int projectile_skip_calculation = range * projectile_skip_multiplier;
    int projectile_skip_current_frame = rng( 0, projectile_skip_calculation );
    bool has_momentum = true;

    for( size_t i = 1; i < traj_len && ( has_momentum || stream ); ++i ) {
        prev_point = tp;
        tp = trajectory[i];

        if( ( tp.z > prev_point.z && g->m.has_floor( tp ) ) ||
            ( tp.z < prev_point.z && g->m.has_floor( prev_point ) ) ) {
            // Currently strictly no shooting through floor
            // TODO: Bash the floor
            tp = prev_point;
            traj_len = --i;
            break;
        }
        // Drawing the bullet uses player g->u, and not player p, because it's drawn
        // relative to YOUR position, which may not be the gunman's position.
        if( do_animation && !do_draw_line ) {
            // TODO: Make this draw thrown item/launched grenade/arrow
            if( projectile_skip_current_frame >= projectile_skip_calculation ) {
                g->draw_bullet( tp, static_cast<int>( i ), trajectory, bullet );
                projectile_skip_current_frame = 0;
                // If we missed recalculate the skip factor so they spread out.
                projectile_skip_calculation =
                    std::max( static_cast<size_t>( range ), i ) * projectile_skip_multiplier;
            } else {
                projectile_skip_current_frame++;
            }
        }

        if( in_veh != nullptr ) {
            const optional_vpart_position other = g->m.veh_at( tp );
            if( in_veh == veh_pointer_or_null( other ) && other->is_inside() ) {
                continue; // Turret is on the roof and can't hit anything inside
            }
        }

        Creature *critter = g->critter_at( tp );
        if( origin == critter ) {
            // No hitting self with "weird" attacks.
            critter = nullptr;
        }

        monster *mon = dynamic_cast<monster *>( critter );
        // ignore non-point-blank digging targets (since they are underground)
        if( mon != nullptr && mon->digging() &&
            rl_dist( source, tp ) > 1 ) {
            critter = nullptr;
            mon = nullptr;
        }

        // Reset hit critter from the last iteration
        attack.hit_critter = nullptr;

        // If we shot us a monster...
        // TODO: add size effects to accuracy
        // If there's a monster in the path of our bullet, and either our aim was true,
        //  OR it's not the monster we were aiming at and we were lucky enough to hit it
        double cur_missed_by = aim.missed_by;

        // unintentional hit on something other than our actual target
        // don't re-roll for the actual target, we already decided on a missed_by value for that
        // at the start, misses should stay as misses
        if( critter != nullptr && tp != target_arg ) {
            // Unintentional hit
            cur_missed_by = std::max( rng_float( 0.1, 1.5 - aim.missed_by ) /
                                      critter->ranged_target_size(), 0.4 );
        }

        if( critter != nullptr && cur_missed_by < 1.0 ) {
            if( in_veh != nullptr && veh_pointer_or_null( g->m.veh_at( tp ) ) == in_veh &&
                critter->is_player() ) {
                // Turret either was aimed by the player (who is now ducking) and shoots from above
                // Or was just IFFing, giving lots of warnings and time to get out of the line of fire
                continue;
            }
            attack.missed_by = cur_missed_by;
            critter->deal_projectile_attack( null_source ? nullptr : origin, attack );
            // Critter can still dodge the projectile
            // In this case hit_critter won't be set
            if( attack.hit_critter != nullptr ) {
                const size_t bt_len = blood_trail_len( attack.dealt_dam.total_damage() );
                if( bt_len > 0 ) {
                    const tripoint &dest = move_along_line( tp, trajectory, bt_len );
                    g->m.add_splatter_trail( critter->bloodType(), tp, dest );
                }
                sfx::do_projectile_hit( *attack.hit_critter );
                has_momentum = false;
            } else {
                attack.missed_by = aim.missed_by;
            }
        } else if( in_veh != nullptr && veh_pointer_or_null( g->m.veh_at( tp ) ) == in_veh ) {
            // Don't do anything, especially don't call map::shoot as this would damage the vehicle
        } else {
            g->m.shoot( tp, proj, !no_item_damage && tp == target );
            has_momentum = proj.impact.total_damage() > 0;
        }

        if( ( !has_momentum || !is_bullet ) && g->m.impassable( tp ) ) {
            // Don't let flamethrowers go through walls
            // TODO: Let them go through bars
            traj_len = i;
            break;
        }
    } // Done with the trajectory!

    if( do_animation && do_draw_line && traj_len > 2 ) {
        trajectory.erase( trajectory.begin() );
        trajectory.resize( traj_len-- );
        g->draw_line( tp, trajectory );
        g->draw_bullet( tp, int( traj_len-- ), trajectory, bullet );
    }

    if( g->m.impassable( tp ) ) {
        tp = prev_point;
    }

    drop_or_embed_projectile( attack );

    apply_ammo_effects( tp, proj.proj_effects );
    const auto &expl = proj.get_custom_explosion();
    if( expl.power > 0.0f ) {
        g->explosion( tp, proj.get_custom_explosion() );
    }

    // TODO: Move this outside now that we have hit point in return values?
    if( proj.proj_effects.count( "BOUNCE" ) ) {
        // Add effect so the shooter is not targeted itself.
        if( origin && !origin->has_effect( effect_bounced ) ) {
            origin->add_effect( effect_bounced, 1_turns );
        }
        Creature *mon_ptr = g->get_creature_if( [&]( const Creature & z ) {
            // search for creatures in radius 4 around impact site
            if( rl_dist( z.pos(), tp ) <= 4 &&
                g->m.sees( z.pos(), tp, -1 ) ) {
                // don't hit targets that have already been hit
                if( !z.has_effect( effect_bounced ) ) {
                    return true;
                }
            }
            return false;
        } );
        if( mon_ptr ) {
            Creature &z = *mon_ptr;
            add_msg( _( "The attack bounced to %s!" ), z.get_name().c_str() );
            z.add_effect( effect_bounced, 1_turns );
            projectile_attack( proj, tp, z.pos(), dispersion, origin, in_veh );
            sfx::play_variant_sound( "fire_gun", "bio_lightning_tail",
                                     sfx::get_heard_volume( z.pos() ), sfx::get_heard_angle( z.pos() ) );
        }
    }

    return attack;
}
