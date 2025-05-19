#include "ballistics.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "dispersion.h"
#include "enums.h"
#include "explosion.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "point.h"
#include "projectile.h"
#include "ranged.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "vpart_position.h"

static const ammo_effect_str_id ammo_effect_ACT_ON_RANGED_HIT( "ACT_ON_RANGED_HIT" );
static const ammo_effect_str_id ammo_effect_BOUNCE( "BOUNCE" );
static const ammo_effect_str_id ammo_effect_BURST( "BURST" );
static const ammo_effect_str_id ammo_effect_DRAW_AS_LINE( "DRAW_AS_LINE" );
static const ammo_effect_str_id ammo_effect_HEAVY_HIT( "HEAVY_HIT" );
static const ammo_effect_str_id ammo_effect_JET( "JET" );
static const ammo_effect_str_id ammo_effect_MUZZLE_SMOKE( "MUZZLE_SMOKE" );
static const ammo_effect_str_id ammo_effect_NO_EMBED( "NO_EMBED" );
static const ammo_effect_str_id ammo_effect_NO_ITEM_DAMAGE( "NO_ITEM_DAMAGE" );
static const ammo_effect_str_id ammo_effect_NO_OVERSHOOT( "NO_OVERSHOOT" );
static const ammo_effect_str_id ammo_effect_NO_PENETRATE_OBSTACLES( "NO_PENETRATE_OBSTACLES" );
static const ammo_effect_str_id ammo_effect_NULL_SOURCE( "NULL_SOURCE" );
static const ammo_effect_str_id ammo_effect_SHATTER_SELF( "SHATTER_SELF" );
static const ammo_effect_str_id ammo_effect_STREAM( "STREAM" );
static const ammo_effect_str_id ammo_effect_STREAM_BIG( "STREAM_BIG" );
static const ammo_effect_str_id ammo_effect_STREAM_TINY( "STREAM_TINY" );
static const ammo_effect_str_id ammo_effect_TANGLE( "TANGLE" );
static const ammo_effect_str_id ammo_effect_WIDE( "WIDE" );

static const itype_id itype_glass_shard( "glass_shard" );

static const json_character_flag json_flag_HARDTOHIT( "HARDTOHIT" );

static void drop_or_embed_projectile( map *here, const dealt_projectile_attack &attack,
                                      projectile &proj_arg )
{
    const projectile &proj = attack.proj;
    const item &drop_item = proj.get_drop();
    const auto &effects = proj.proj_effects;
    if( drop_item.is_null() ) {
        return;
    }

    const tripoint_bub_ms &pt = attack.end_point;

    if( effects.count( ammo_effect_SHATTER_SELF ) ) {
        // Drop the contents, not the thrown item
        add_msg_if_player_sees( pt, _( "The %s shatters!" ), drop_item.tname() );

        // copies the drop item to spill the contents
        item( drop_item ).spill_contents( here, pt );

        // TODO: Non-glass breaking
        // TODO: Wine glass breaking vs. entire sheet of glass breaking
        // TODO: Refine logic to allow for overlapping maps.
        if( here == &reality_bubble() ) {
            sounds::sound( pt, 16, sounds::sound_t::combat, _( "glass breaking!" ), false, "bullet_hit",
                           "hit_glass" );
        }

        const units::mass shard_mass = itype_glass_shard->weight;
        const int max_nb_of_shards = floor( to_gram( drop_item.type->weight ) / to_gram( shard_mass ) );
        //between half and max_nb_of_shards-1 will be usable
        const int nb_of_dropped_shard = std::max( 0, rng( max_nb_of_shards / 2, max_nb_of_shards - 1 ) );
        //feel free to remove this msg_debug
        /*add_msg_debug( "Shattered %s dropped %i shards out of a max of %i, based on mass %i g",
                       drop_item.tname(), nb_of_dropped_shard, max_nb_of_shards - 1, to_gram( drop_item.type->weight ) );*/

        for( int i = 0; i < nb_of_dropped_shard; ++i ) {
            item shard( itype_glass_shard );
            //actual dropping of shards
            here->add_item_or_charges( pt, shard );
        }

        return;
    }

    if( effects.count( ammo_effect_BURST ) ) {
        // Drop the contents, not the thrown item
        add_msg_if_player_sees( reality_bubble().get_bub( here->get_abs( pt ) ), _( "The %s bursts!" ),
                                drop_item.tname() );

        // copies the drop item to spill the contents
        item( drop_item ).spill_contents( here, pt );

        // TODO: Sound
        return;
    }

    // Copy the item
    item dropped_item = drop_item;

    monster *mon = dynamic_cast<monster *>( attack.last_hit_critter );

    // We can only embed in monsters
    bool mon_there = mon != nullptr && !mon->is_dead_state();
    // And if we actually want to embed
    bool embed = mon_there && effects.count( ammo_effect_NO_EMBED ) == 0 &&
                 effects.count( ammo_effect_TANGLE ) == 0;
    // Don't embed in small creatures
    if( embed ) {
        const creature_size critter_size = mon->get_size();
        const units::volume vol = dropped_item.volume();
        embed = embed && ( critter_size > creature_size::tiny || vol < 250_ml );
        embed = embed && ( critter_size > creature_size::small || vol < 500_ml );
        // And if we deal enough damage
        // Item volume bumps up the required damage too
        // FIXME: Hardcoded damage types
        embed = embed &&
                ( attack.dealt_dam.type_damage( STATIC( damage_type_id( "cut" ) ) ) / 2 ) +
                attack.dealt_dam.type_damage( STATIC( damage_type_id( "stab" ) ) ) >
                attack.dealt_dam.type_damage( STATIC( damage_type_id( "bash" ) ) ) +
                vol * 3 / 250_ml + rng( 0, 5 );
    }

    if( embed ) {
        if( proj_arg.proj_effects.count( ammo_effect_BOUNCE ) ) {
            proj_arg.proj_effects.erase( ammo_effect_BOUNCE );
        }
        mon->add_item( dropped_item );
        add_msg_if_player_sees( pt, _( "The %1$s embeds in %2$s!" ),
                                dropped_item.tname(), mon->disp_name() );
    } else {
        bool do_drop = true;
        // monsters that are able to be tied up will store the item another way
        // see monexamine.cpp tie_or_untie()
        // if they aren't friendly they will try and break out of the net/bolas/lasso
        // players and NPCs just get the downed effect, and item is dropped.
        // TODO: storing the item on player until they recover from downed
        if( effects.count( ammo_effect_TANGLE ) && mon_there ) {
            do_drop = false;
        }
        if( effects.count( ammo_effect_ACT_ON_RANGED_HIT ) ) {
            // Don't drop if it exploded
            do_drop = !dropped_item.activate_thrown( attack.end_point );
        }

        map &here = get_map();
        if( do_drop ) {
            here.add_item_or_charges( attack.end_point, dropped_item );
        }

        if( effects.count( ammo_effect_HEAVY_HIT ) ) {
            if( here.has_flag( ter_furn_flag::TFLAG_LIQUID, pt ) ) {
                sounds::sound( pt, 10, sounds::sound_t::combat, _( "splash!" ), false, "bullet_hit", "hit_water" );
            } else {
                sounds::sound( pt, 8, sounds::sound_t::combat, _( "thud." ), false, "bullet_hit", "hit_wall" );
            }
        }

        const trap &tr = here.tr_at( pt );
        if( tr.triggered_by_item( dropped_item ) ) {
            tr.trigger( pt, dropped_item );
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
    add_msg_debug( debugmode::DF_BALLISTIC, "Dispersion rolled / max : %f / %f; %f / %f degrees",
                   aim.dispersion, dispersion.max(), aim.dispersion / 60.0, dispersion.max() / 60.0 );

    // an isosceles triangle is formed by the intended and actual target tiles
    aim.missed_by_tiles = iso_tangent( range, units::from_arcmin( aim.dispersion ) );

    // fraction we missed a monster target by (0.0 = perfect hit, 1.0 = miss)
    if( target_size > 0.0 ) {
        aim.missed_by = std::min( 1.0, aim.missed_by_tiles / target_size );
    } else {
        // Special case 0 size targets, just to be safe from 0.0/0.0 NaNs
        aim.missed_by = 1.0;
    }

    return aim;
}

void projectile_attack( dealt_projectile_attack &attack, const projectile &proj_arg,
                        const tripoint_bub_ms &source, const tripoint_bub_ms &target_arg,
                        const dispersion_sources &dispersion, Creature *origin, const vehicle *in_veh,
                        const weakpoint_attack &wp_attack )
{
    projectile_attack( attack, proj_arg, &get_map(), source, target_arg, dispersion, origin, in_veh,
                       wp_attack );
}

void projectile_attack( dealt_projectile_attack &attack, const projectile &proj_arg,
                        map *here, const tripoint_bub_ms &source, const tripoint_bub_ms &target_arg,
                        const dispersion_sources &dispersion, Creature *origin, const vehicle *in_veh,
                        const weakpoint_attack &wp_attack )
{
    const bool do_animation = get_option<bool>( "ANIMATION_PROJECTILES" );

    double range = rl_dist( source, target_arg );

    creature_tracker &creatures = get_creature_tracker();
    Creature *target_critter = creatures.creature_at( target_arg );
    double target_size;

    if( target_critter != nullptr ) {
        const monster *mon = target_critter->as_monster();
        if( mon && proj_arg.proj_effects.count( ammo_effect_WIDE ) ) {
            // ammo with ammo_effect_WIDE ignores mon_flag_HARDTOSHOOT
            target_size = occupied_tile_fraction( mon->get_size() );
        } else {
            target_size = target_critter->ranged_target_size();
        }
    } else {
        target_size = here->ranged_target_size( target_arg );
    }
    projectile_attack_aim aim = projectile_attack_roll( dispersion, range, target_size );

    if( target_critter && target_critter->as_character() &&
        target_critter->as_character()->has_flag( json_flag_HARDTOHIT ) ) {

        projectile_attack_aim lucky_aim = projectile_attack_roll( dispersion, range, target_size );
        // if the target's lucky they're more likely to be missed
        if( lucky_aim.missed_by > aim.missed_by ) {
            aim = lucky_aim;
        }
    }

    // TODO: move to-hit roll back in here

    attack.proj = proj_arg;
    attack.last_hit_critter = nullptr;
    attack.dealt_dam = dealt_damage_instance();
    attack.end_point = source;
    attack.missed_by = aim.missed_by;

    // No suicidal shots
    if( source == target_arg ) {
        debugmsg( "Projectile_attack targeted own square." );
        return;
    }

    projectile &proj = attack.proj;
    const auto &proj_effects = proj.proj_effects;

    const bool stream = proj_effects.count( ammo_effect_STREAM ) > 0 ||
                        proj_effects.count( ammo_effect_STREAM_TINY ) > 0 ||
                        proj_effects.count( ammo_effect_STREAM_BIG ) > 0 ||
                        proj_effects.count( ammo_effect_JET ) > 0;
    const char bullet = stream ? '#' : '*';
    const bool no_item_damage = proj_effects.count( ammo_effect_NO_ITEM_DAMAGE ) > 0;
    const bool do_draw_line = proj_effects.count( ammo_effect_DRAW_AS_LINE ) > 0;
    const bool null_source = proj_effects.count( ammo_effect_NULL_SOURCE ) > 0;
    // Determines whether it can penetrate obstacles
    const bool is_bullet = proj_arg.speed >= 200 &&
                           !proj_effects.count( ammo_effect_NO_PENETRATE_OBSTACLES );

    // If we were targeting a tile rather than a monster, don't overshoot
    // Unless the target was a wall, then we are aiming high enough to overshoot
    const bool no_overshoot = proj_effects.count( ammo_effect_NO_OVERSHOOT ) ||
                              ( creatures.creature_at( here->get_abs( target_arg ) ) == nullptr && here->passable( target_arg ) );

    double extend_to_range = no_overshoot ? range : proj_arg.range;

    tripoint_bub_ms target = target_arg;
    std::vector<tripoint_bub_ms> trajectory;
    if( aim.missed_by_tiles >= 1.0 ) {
        // We missed enough to target a different tile
        double dx = target_arg.x() - source.x();
        double dy = target_arg.y() - source.y();
        units::angle rad = units::atan2( dy, dx );

        // cap wild misses at +/- 30 degrees
        units::angle dispersion_angle =
            std::min( units::from_arcmin( aim.dispersion ), 30_degrees );
        rad += ( one_in( 2 ) ? 1 : -1 ) * dispersion_angle;

        // TODO: This should also represent the miss on z axis
        const int offset = std::min<int>( range, std::sqrt( aim.missed_by_tiles ) );
        int new_range = no_overshoot ?
                        range + rng( -offset, offset ) :
                        rng( range - offset, proj_arg.range );
        new_range = std::max( new_range, 1 );

        target.x() = std::clamp( source.x() + roll_remainder( new_range * cos( rad ) ), 0, MAPSIZE_X - 1 );
        target.y() = std::clamp( source.y() + roll_remainder( new_range * sin( rad ) ), 0, MAPSIZE_Y - 1 );

        if( target == source ) {
            target.x() = source.x() + sgn( dx );
            target.y() = source.y() + sgn( dy );
        }

        // Don't extend range further, miss here can mean hitting the ground near the target
        range = rl_dist( source, target );
        extend_to_range = range;

        if( reality_bubble().inbounds( here->get_abs( target ) ) ) {
            const tripoint_bub_ms bub_target = reality_bubble().get_bub( here->get_abs( target ) );
            sfx::play_variant_sound( "bullet_hit", "hit_wall", sfx::get_heard_volume( bub_target ),
                                     sfx::get_heard_angle( bub_target ) );
        }
        // TODO: Z dispersion
    }

    //Use find clear path to draw the trajectory with optimal initial tile offsets.
    trajectory = here->find_clear_path( source, target );

    add_msg_debug( debugmode::DF_BALLISTIC,
                   "missed_by_tiles: %.2f; missed_by: %.2f; target (orig/hit): %s/%s",
                   aim.missed_by_tiles, aim.missed_by, target_arg.to_string_writable(),
                   target.to_string_writable() );

    // Trace the trajectory, doing damage in order
    tripoint_bub_ms &tp = attack.end_point;

    // Add the first point to the trajectory
    trajectory.insert( trajectory.begin(), source );

    static emit_id muzzle_smoke( "emit_smaller_smoke_plume" );
    if( proj_effects.count( ammo_effect_MUZZLE_SMOKE ) ) {
        here->emit_field( trajectory.front(), muzzle_smoke );
    }

    if( !no_overshoot && range < extend_to_range ) {
        // Continue line is very "stiff" when the original range is short
        // TODO: Make it use a more distant point for more realistic extended lines
        std::vector<tripoint_bub_ms> trajectory_extension = continue_line( trajectory,
                extend_to_range - range );
        trajectory.reserve( trajectory.size() + trajectory_extension.size() );
        trajectory.insert( trajectory.end(), trajectory_extension.begin(), trajectory_extension.end() );
    }

    bool first = true;
    bool print_messages = true;
    bool multishot = false;
    // If the proj hits a ter/furn at point-blank and is not stopped, rescale shot_impact for all projs.
    double point_blank_rescale = 1.0;
    tripoint_bub_ms first_p = trajectory[1];
    for( int j = 0; j < proj.count; ++j ) {
        tripoint_bub_ms prev_point = source;
        proj = proj_arg;
        proj.multishot = multishot;
        tripoint_bub_ms target_c = target;
        std::vector<tripoint_bub_ms> t_copy = trajectory;
        double spread = 0;
        // calculate dispersion from shot_spread
        if( proj.shot_spread > 0 ) {
            double sample = rng_normal( -proj.shot_spread, proj.shot_spread );
            sample = sample >= 0 ? sample : -sample;
            units::angle arc = std::min( units::from_arcmin( sample ), 30_degrees );
            spread = tan( arc / 2 ) * 2.0;
            double max_spread = spread * ( t_copy.size() - 1 );
            if( max_spread >= 1.0 ) {
                double dx = target_c.x() - source.x();
                double dy = target_c.y() - source.y();
                units::angle d = units::atan2( dy, dx );
                d += ( one_in( 2 ) ? 1 : -1 ) * arc;
                target_c.x() = std::clamp( source.x() + roll_remainder( range * cos( d ) ),
                                           0, MAPSIZE_X - 1 );
                target_c.y() = std::clamp( source.y() + roll_remainder( range * sin( d ) ),
                                           0, MAPSIZE_Y - 1 );
                if( target_c == source ) {
                    target_c.x() = source.x() + sgn( dx );
                    target_c.y() = source.y() + sgn( dy );
                }
                t_copy = here->find_clear_path( first_p, target_c );
                // point-blank tile should be the same
                t_copy.insert( t_copy.begin(), first_p );
                t_copy.insert( t_copy.begin(), source );
                if( !no_overshoot && range < extend_to_range ) {
                    std::vector<tripoint_bub_ms> extension = continue_line( t_copy,
                            extend_to_range - range );
                    t_copy.reserve( t_copy.size() + extension.size() );
                    t_copy.insert( t_copy.end(), extension.begin(), extension.end() );
                }
            }
            add_msg_debug( debugmode::DF_BALLISTIC,
                           "shot_spread: %d; spread roll/max_spread: %.2f/%.2f; target (orig/hit): %s/%s",
                           proj.shot_spread, spread, max_spread,
                           target_arg.to_string_writable(), target_c.to_string_writable() );
        }
        // Range can be 0
        size_t traj_len = t_copy.size();
        while( traj_len > 0 && rl_dist( source, t_copy[traj_len - 1] ) > proj_arg.range ) {
            --traj_len;
        }

        const float projectile_skip_multiplier = 0.1f;
        // Randomize the skip so that bursts look nicer
        int projectile_skip_calculation = range * projectile_skip_multiplier;
        int projectile_skip_current_frame = rng( 0, projectile_skip_calculation );
        bool has_momentum = true;

        for( size_t i = 1; i < traj_len && ( has_momentum || stream ); ++i ) {
            tp = t_copy[i];
            int distance = rl_dist( source, tp );
            // no spread at point-blank, skip point-blank calculate
            if( !first && distance <= 1 ) {
                prev_point = tp;
                proj.shot_impact.mult_damage( point_blank_rescale );
                continue;
            }

            if( tp.z() != prev_point.z() ) {
                tripoint_bub_ms floor1 = prev_point;
                tripoint_bub_ms floor2 = tp;

                if( floor1.z() < floor2.z() ) {
                    floor1 += tripoint::above;
                } else {
                    floor2 += tripoint::above;
                }
                // We only stop the bullet if there are two floors in a row
                // this allow the shooter to shoot adjacent enemies from rooftops.
                if( here->has_floor_or_water( floor1 ) && here->has_floor_or_water( floor2 ) ) {
                    // Currently strictly no shooting through floor
                    // TODO: Bash the floor
                    tp = prev_point;
                    traj_len = --i;
                    break;
                }
            }

            // Drawing the bullet uses player g->u, and not player p, because it's drawn
            // relative to YOUR position, which may not be the gunman's position.
            if( first && do_animation && !do_draw_line ) {
                // TODO: Make this draw thrown item/launched grenade/arrow
                if( projectile_skip_current_frame >= projectile_skip_calculation ) {
                    // TODO: Refine so overlapping maps are handled.
                    if( here == &reality_bubble() ) {
                        g->draw_bullet( tp, static_cast<int>( i ), trajectory, bullet );
                    }
                    projectile_skip_current_frame = 0;
                    // If we missed recalculate the skip factor so they spread out.
                    projectile_skip_calculation =
                        std::max( static_cast<size_t>( range ), i ) * projectile_skip_multiplier;
                } else {
                    projectile_skip_current_frame++;
                }
            }

            if( in_veh != nullptr ) {
                const optional_vpart_position other = here->veh_at( tp );
                if( in_veh == veh_pointer_or_null( other ) && other->is_inside() ) {
                    // Turret is on the roof and can't hit anything inside
                    continue;
                }
            }

            Creature *critter = creatures.creature_at( tp );
            if( origin == critter ) {
                // No hitting self with "weird" attacks.
                critter = nullptr;
            }

            monster *mon = dynamic_cast<monster *>( critter );
            // ignore non-point-blank digging targets (since they are underground)
            if( mon != nullptr && mon->digging() &&
                distance > 1 ) {
                critter = nullptr;
                mon = nullptr;
            }

            // Reset hit critter from the last iteration
            attack.last_hit_critter = nullptr;

            // If we shot us a monster...
            // TODO: add size effects to accuracy
            // If there's a monster in the path of our bullet, and either our aim was true,
            //  OR it's not the monster we were aiming at and we were lucky enough to hit it
            double cur_missed_by = std::min( 1.0, aim.missed_by + spread * std::max( distance - 1, 0 ) );

            // unintentional hit on something other than our actual target
            // don't re-roll for the actual target, we already decided on a missed_by value for that
            // at the start, misses should stay as misses
            if( critter != nullptr && tp != target_arg ) {
                // Unintentional hit
                cur_missed_by = std::max( rng_float( 0.1, 1.5 - aim.missed_by ) /
                                          critter->ranged_target_size(), 0.4 );
            }
            // If the attack is shot, once we're past point-blank,
            // don't print normal hit msg.
            if( first && proj.count > 1 && distance > 1 ) {
                multishot = true;
                proj.multishot = true;
                print_messages = false;
            }

            if( critter != nullptr && cur_missed_by < 1.0 ) {
                if( in_veh != nullptr && veh_pointer_or_null( here->veh_at( tp ) ) == in_veh &&
                    critter->is_avatar() ) {
                    // Turret either was aimed by the player (who is now ducking) and shoots from above
                    // Or was just IFFing, giving lots of warnings and time to get out of the line of fire
                    continue;
                }
                // avoid friendly fire
                if( critter->attitude_to( *origin ) == Creature::Attitude::FRIENDLY &&
                    origin->check_avoid_friendly_fire() ) {
                    continue;
                }
                critter->deal_projectile_attack( here, null_source ? nullptr : origin, attack, cur_missed_by,
                                                 print_messages, wp_attack );

                if( critter->is_npc() ) {
                    critter->as_npc()->on_attacked( *origin );
                }

                // Critter can still dodge the projectile
                // In this case hit_critter won't be set
                if( attack.last_hit_critter != nullptr ) {
                    const field_type_id blood_type = critter->bloodType();
                    if( blood_type ) {
                        const size_t bt_len = blood_trail_len( attack.dealt_dam.total_damage() );
                        if( bt_len > 0 ) {
                            const tripoint_bub_ms &dest = move_along_line( tp, t_copy, bt_len );
                            here->add_splatter_trail( blood_type, tp, dest );
                        }
                    }
                    // TODO: Refine to account for overlapping maps
                    if( here == &reality_bubble() ) {
                        sfx::do_projectile_hit( *attack.last_hit_critter );
                    }
                    has_momentum = false;
                    // on-hit effects for inflicted damage types
                    for( const std::pair<const damage_type_id, int> &dt : attack.dealt_dam.dealt_dams ) {
                        dt.first->onhit_effects( origin, attack.last_hit_critter );
                    }
                }
            } else if( in_veh != nullptr && veh_pointer_or_null( here->veh_at( tp ) ) == in_veh ) {
                // Don't do anything, especially don't call map::shoot as this would damage the vehicle
            } else {
                double it = here->shoot( tp, proj, !no_item_damage && tp == target_c );
                if( it > 0 ) {
                    // even if it's a shot, as we will never use impact after point-blank,
                    // we should always tweak shot_impact instead of impact.
                    if( proj.count > 1 ) {
                        proj.shot_impact.mult_damage( it );
                        if( first && distance <= 1 ) {
                            point_blank_rescale = it;
                        }
                    } else {
                        proj.impact.mult_damage( it );
                    }
                } else {
                    has_momentum = false;
                }
            }
            if( ( !has_momentum || traj_len == size_t( 2 ) ) && proj.count > 1 && distance <= 1 ) {
                // Track that we hit an obstacle while wadded up,
                // to cancel out of applying the other projectiles.
                proj.count = 1;
            }

            if( ( !has_momentum || !is_bullet ) && here->impassable( tp ) ) {
                // Don't let flamethrowers go through walls
                // TODO: Let them go through bars
                traj_len = i;
                break;
            }
            prev_point = tp;
        }
        // Done with the trajectory!
        if( first && do_animation && do_draw_line && traj_len > 2 ) {
            t_copy.erase( t_copy.begin() );
            t_copy.resize( traj_len-- );
            g->draw_line( tp, t_copy );
            g->draw_bullet( tp, static_cast<int>( traj_len-- ), t_copy, bullet );
        }

        if( here->impassable( tp ) ) {
            tp = prev_point;
        }

        drop_or_embed_projectile( here, attack, proj );

        int dealt_damage = attack.dealt_dam.total_damage();
        apply_ammo_effects( null_source ? nullptr : origin, tp, proj.proj_effects, dealt_damage );
        const explosion_data &expl = proj.get_custom_explosion();
        if( expl.power > 0.0f ) {
            explosion_handler::explosion( null_source ? nullptr : origin, here, tp,
                                          proj.get_custom_explosion() );
        }
        first = false;
    }

    // TODO: Move this outside now that we have hit point in return values?
    if( proj.proj_effects.count( ammo_effect_BOUNCE ) ) {
        Creature *mon_ptr = g->get_creature_if( [&]( const Creature & z ) {
            if( &z == origin ) {
                return false;
            }
            // search for creatures in radius 4 around impact site
            if( rl_dist( z.pos_bub( *here ), tp ) <= 4 &&
                here->sees( z.pos_bub( *here ), tp, -1 ) ) {
                // don't hit targets that have already been hit
                for( auto it : attack.targets_hit ) {
                    if( &z == it.first ) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        } );
        if( mon_ptr ) {
            Creature &z = *mon_ptr;
            attack.targets_hit[&z].first += 0;
            add_msg( _( "The attack bounced to %s!" ), z.get_name() );
            projectile_attack( attack, proj, here, tp, z.pos_bub(), dispersion, origin, in_veh );
            // TODO: Refine to handle overlapping maps
            if( here == &reality_bubble() ) {
                sfx::play_variant_sound( "fire_gun", "bio_lightning_tail",
                                         sfx::get_heard_volume( z.pos_bub() ), sfx::get_heard_angle( z.pos_bub() ) );
            }
        }
    }
}
