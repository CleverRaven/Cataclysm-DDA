#include "explosion.h" // IWYU pragma: associated
#include "fragment_cloud.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "color.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "field_type.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "math_defines.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "player.h"
#include "point.h"
#include "projectile.h"
#include "rng.h"
#include "shadowcasting.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const efftype_id effect_blind( "blind" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_emp( "emp" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_teleglow( "teleglow" );

static const std::string flag_BLIND( "BLIND" );
static const std::string flag_FLASH_PROTECTION( "FLASH_PROTECTION" );

static const itype_id fuel_type_none( "null" );

static const species_id ROBOT( "ROBOT" );

static const trait_id trait_LEG_TENT_BRACE( "LEG_TENT_BRACE" );
static const trait_id trait_PER_SLIME( "PER_SLIME" );
static const trait_id trait_PER_SLIME_OK( "PER_SLIME_OK" );

static const mongroup_id GROUP_NETHER( "GROUP_NETHER" );

static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_sunglasses( "bio_sunglasses" );

static float blast_percentage( float distance_factor, float distance );

explosion_data load_explosion_data( const JsonObject &jo )
{
    explosion_data ret;
    // First new explosions
    if( jo.has_int( "damage" ) || jo.has_object( "fragment" ) ) {
        jo.read( "damage", ret.damage );
        jo.read( "radius", ret.radius );
        jo.read( "fragment", ret.fragment );
    } else {
        // Legacy
        float power = jo.get_float( "power" );
        ret.damage = power * explosion_handler::power_to_dmg_mult;
        // Don't reuse old formula, it gave way too big blasts
        float distance_factor = jo.get_float( "distance_factor", 0.8f );
        ret.radius = explosion_handler::blast_radius_from_legacy( power, distance_factor );
        if( jo.has_int( "shrapnel" ) ) {
            ret.fragment = explosion_handler::shrapnel_from_legacy( power, ret.radius );
            // Outdated, unused
            jo.get_int( "shrapnel" );
        } else if( jo.has_object( "shrapnel" ) ) {
            ret.fragment = explosion_handler::shrapnel_from_legacy( power, ret.radius );

            auto shr = jo.get_object( "shrapnel" );
            // Legacy bad design - we don't migrate those
            shr.get_int( "casing_mass" );
            shr.get_float( "fragment_mass", 0.15 );
            shr.get_int( "recovery", 0 );
            shr.get_string( "drop", "" );
        }
    }

    ret.fire = jo.get_bool( "fire", false );

    return ret;
}

namespace explosion_handler
{

// (C1001) Compiler Internal Error on Visual Studio 2015 with Update 2
static std::map<const Creature *, int> do_blast( const tripoint &p, const float power,
        const float radius, const bool fire )
{
    const float tile_dist = 1.0f;
    const float diag_dist = trigdist ? M_SQRT2 * tile_dist : 1.0f * tile_dist;
    const float zlev_dist = 4.0f; // Penalty for going up/down
    // 7 3 5
    // 1 . 2
    // 6 4 8
    // 9 and 10 are up and down
    static const int x_offset[10] = { -1, 1,  0, 0,  1, -1, -1, 1, 0, 0 };
    static const int y_offset[10] = { 0, 0, -1, 1, -1,  1, -1, 1, 0, 0 };
    static const int z_offset[10] = { 0, 0,  0, 0,  0,  0,  0, 0, 1, -1 };
    const size_t max_index = g->m.has_zlevels() ? 10 : 8;

    std::map<const Creature *, int> blasted;

    g->m.bash( p, fire ? power : ( 2 * power ), true, false, false );

    std::priority_queue< std::pair<float, tripoint>, std::vector< std::pair<float, tripoint> >, pair_greater_cmp_first >
    open;
    std::set<tripoint> closed;
    std::map<tripoint, float> dist_map;
    open.push( std::make_pair( 0.0f, p ) );
    dist_map[p] = 0.0f;
    // Find all points to blast
    while( !open.empty() ) {
        const float distance = open.top().first;
        const tripoint pt = open.top().second;
        open.pop();

        if( closed.count( pt ) != 0 ) {
            continue;
        }

        closed.insert( pt );

        const float force = power * blast_percentage( radius, distance );
        if( force <= 1.0f ) {
            continue;
        }

        if( g->m.impassable( pt ) && pt != p ) {
            // Don't propagate further
            continue;
        }

        // Iterate over all neighbors. Bash all of them, propagate to some
        for( size_t i = 0; i < max_index; i++ ) {
            tripoint dest( pt + tripoint( x_offset[i], y_offset[i], z_offset[i] ) );
            if( closed.count( dest ) != 0 || !g->m.inbounds( dest ) ) {
                continue;
            }

            const float bashing_force = force / 4;
            if( z_offset[i] == 0 ) {
                // Horizontal - no floor bashing
                g->m.bash( dest, bashing_force, true, false, false );
            } else if( z_offset[i] > 0 ) {
                // Should actually bash through the floor first, but that's not really possible yet
                g->m.bash( dest, bashing_force, true, false, true );
            } else if( !g->m.valid_move( pt, dest, false, true ) ) {
                // Only bash through floor if it doesn't exist
                // Bash the current tile's floor, not the one's below
                g->m.bash( pt, bashing_force, true, false, true );
            }

            float next_dist = distance;
            next_dist += ( x_offset[i] == 0 || y_offset[i] == 0 ) ? tile_dist : diag_dist;
            if( z_offset[i] != 0 ) {
                if( !g->m.valid_move( pt, dest, false, true ) ) {
                    continue;
                }

                next_dist += zlev_dist;
            }

            if( dist_map.count( dest ) == 0 || dist_map[dest] > next_dist ) {
                open.push( std::make_pair( next_dist, dest ) );
                dist_map[dest] = next_dist;
            }
        }
    }

    // Draw the explosion
    std::map<tripoint, nc_color> explosion_colors;
    for( auto &pt : closed ) {
        if( g->m.impassable( pt ) ) {
            continue;
        }

        float percentage = blast_percentage( radius, dist_map.at( pt ) );
        if( percentage > 0.0f ) {
            static const std::array<nc_color, 3> colors = {{
                    c_red, c_yellow, c_white
                }
            };
            size_t color_index = ( power > 30 ? 1 : 0 ) + ( percentage > 0.5f ? 1 : 0 );

            explosion_colors[pt] = colors[color_index];
        }
    }

    draw_custom_explosion( g->u.pos(), explosion_colors );

    for( const tripoint &pt : closed ) {
        const float force = power * blast_percentage( radius, dist_map.at( pt ) );
        if( force < 1.0f ) {
            // Too weak to matter
            continue;
        }

        g->m.smash_items( pt, force, _( "force of the explosion" ) );

        if( fire ) {
            int intensity = 1 + ( force > 10.0f ) + ( force > 30.0f );

            if( !g->m.has_zlevels() && g->m.is_outside( pt ) && intensity == 2 ) {
                // In 3D mode, it would have fire fields above, which would then fall
                // and fuel the fire on this tile
                intensity++;
            }

            g->m.add_field( pt, fd_fire, intensity );
        }

        if( const optional_vpart_position vp = g->m.veh_at( pt ) ) {
            // TODO: Make this weird unit used by vehicle::damage more sensible
            vp->vehicle().damage( vp->part_index(), force / 4, fire ? DT_HEAT : DT_BASH, false );
        }

        Creature *critter = g->critter_at( pt, true );
        if( critter == nullptr ) {
            continue;
        }

        player *pl = dynamic_cast<player *>( critter );
        if( pl == nullptr ) {
            // TODO: player's fault?
            const double dmg = std::max( force - critter->get_armor_bash( bodypart_id( "torso" ) ) / 3.0, 0.0 );
            const int actual_dmg = rng_float( dmg, dmg * 2 );
            critter->apply_damage( nullptr, bodypart_id( "torso" ), actual_dmg );
            critter->check_dead_state();
            blasted[critter] = actual_dmg;
            continue;
        }

        // Print messages for all NPCs
        pl->add_msg_if_player( m_bad, _( "You're caught in the explosion!" ) );

        struct blastable_part {
            bodypart_id bp;
            float low_mul;
            float high_mul;
            float armor_mul;
        };

        static const std::array<blastable_part, 6> blast_parts = { {
                { bodypart_id( "torso" ), 0.5f, 1.0f, 0.5f },
                { bodypart_id( "head" ),  0.5f, 1.0f, 0.5f },
                // Hit limbs harder so that it hurts more without being much more deadly
                { bodypart_id( "leg_l" ), 0.75f, 1.25f, 0.4f },
                { bodypart_id( "leg_r" ), 0.75f, 1.25f, 0.4f },
                { bodypart_id( "arm_l" ), 0.75f, 1.25f, 0.4f },
                { bodypart_id( "arm_r" ), 0.75f, 1.25f, 0.4f },
            }
        };

        for( const auto &blp : blast_parts ) {
            const int part_dam = rng( force * blp.low_mul, force * blp.high_mul );
            const std::string hit_part_name = body_part_name_accusative( blp.bp->token );
            const auto dmg_instance = damage_instance( DT_BASH, part_dam, 0, blp.armor_mul );
            const auto result = pl->deal_damage( nullptr, blp.bp, dmg_instance );
            const int res_dmg = result.total_damage();

            add_msg( m_debug, "%s for %d raw, %d actual", hit_part_name, part_dam, res_dmg );
            if( res_dmg > 0 ) {
                blasted[critter] += res_dmg;
            }
        }
    }

    return blasted;
}

static std::map<const Creature *, int> shrapnel( const tripoint &src, const projectile &fragment )
{
    std::map<const Creature *, int> damaged;

    projectile proj = fragment;
    proj.proj_effects.insert( "NULL_SOURCE" );

    float obstacle_cache[MAPSIZE_X][MAPSIZE_Y] = {};
    float visited_cache[MAPSIZE_X][MAPSIZE_Y] = {};

    // TODO: Calculate range based on max effective range for projectiles.
    // Basically bisect between 0 and map diameter using shrapnel_calc().
    // Need to update shadowcasting to support limiting range without adjusting initial distance.
    const tripoint_range area = g->m.points_on_zlevel( src.z );

    g->m.build_obstacle_cache( area.min(), area.max() + tripoint_south_east, obstacle_cache );

    // Shadowcasting normally ignores the origin square,
    // so apply it manually to catch monsters standing on the explosive.
    // This "blocks" some fragments, but does not apply deceleration.
    visited_cache[src.x][src.y] = 1.0f;

    // This is used to limit radius
    // By default, the radius is 60, so negative values can be helpful here
    const int offset_distance = 60 - 1 - fragment.range;
    castLightAll<float, float, shrapnel_calc, shrapnel_check,
                 update_fragment_cloud, accumulate_fragment_cloud>
                 ( visited_cache, obstacle_cache, src.xy(), offset_distance, fragment.range + 1.0f );

    // Now visited_caches are populated with density and velocity of fragments.
    for( const tripoint &target : area ) {
        if( visited_cache[target.x][target.y] <= 0.0f || rl_dist( src, target ) > fragment.range ) {
            continue;
        }
        auto critter = g->critter_at( target );
        if( critter && !critter->is_dead_state() ) {
            // dealt_dag->m.total_damage() == 0 means armor block
            // dealt_dag->m.total_damage() > 0 means took damage
            // Need to diffentiate target among player, npc, and monster
            int damage_taken = 0;
            auto bps = critter->get_all_body_parts( true );
            // Humans get hit in all body parts
            if( critter->is_player() ) {
                for( bodypart_id bp : bps ) {
                    // TODO: This shouldn't be needed, get_bps should do it
                    if( Character::bp_to_hp( bp->token ) == num_hp_parts ) {
                        continue;
                    }
                    // TODO: Apply projectile effects
                    // TODO: Penalize low coverage armor
                    // Halve damage to be closer to what monsters take
                    damage_instance half_impact = proj.impact;
                    half_impact.mult_damage( 0.5f );
                    dealt_damage_instance dealt = critter->deal_damage( nullptr, bp, proj.impact );
                    if( dealt.total_damage() > 0 ) {
                        damage_taken += dealt.total_damage();
                    }
                }
            } else {
                dealt_damage_instance dealt = critter->deal_damage( nullptr, bps[0], proj.impact );
                if( dealt.total_damage() > 0 ) {
                    damage_taken += dealt.total_damage();
                }
            }
            damaged[critter] = damage_taken;
        }
        if( g->m.impassable( target ) ) {
            int damage = proj.impact.total_damage();
            if( optional_vpart_position vp = g->m.veh_at( target ) ) {
                vp->vehicle().damage( vp->part_index(), damage / 100 );
            } else {
                g->m.bash( target, damage / 100, true );
            }
        }
    }

    return damaged;
}

void explosion( const tripoint &p, float power, float factor, bool fire, int legacy_mass,
                float )
{
    if( factor >= 1.0f ) {
        debugmsg( "called game::explosion with factor >= 1.0 (infinite size)" );
    }
    explosion_data data;
    data.damage = power * explosion_handler::power_to_dmg_mult;
    data.radius = explosion_handler::blast_radius_from_legacy( power, factor );
    data.fire = fire;
    if( legacy_mass > 0 ) {
        data.fragment = explosion_handler::shrapnel_from_legacy( power, data.radius );
    }
    explosion( p, data );
}

void explosion( const tripoint &p, const explosion_data &ex )
{
    const int noise = ex.damage / explosion_handler::power_to_dmg_mult * ( ex.fire ? 2 : 10 );
    if( noise >= 30 ) {
        sounds::sound( p, noise, sounds::sound_t::combat, _( "a huge explosion!" ), false, "explosion",
                       "huge" );
    } else if( noise >= 4 ) {
        sounds::sound( p, noise, sounds::sound_t::combat, _( "an explosion!" ), false, "explosion",
                       "default" );
    } else if( noise > 0 ) {
        sounds::sound( p, 3, sounds::sound_t::combat, _( "a loud pop!" ), false, "explosion", "small" );
    }

    std::map<const Creature *, int> damaged_by_blast;
    std::map<const Creature *, int> damaged_by_shrapnel;
    if( ex.radius >= 0.0f && ex.damage > 0.0f ) {
        // Power rescaled to mean grams of TNT equivalent
        // TODO: Use sensible units instead
        damaged_by_blast = do_blast( p, ex.damage, ex.radius, ex.fire );
    }

    const auto &shr = ex.fragment;
    if( shr ) {
        damaged_by_shrapnel = shrapnel( p, shr.value() );
    }

    // Not the cleanest way to do it
    std::map<const Creature *, int> total_damaged;
    for( const auto &pr : damaged_by_blast ) {
        total_damaged[pr.first] += pr.second;
    }
    for( const auto &pr : damaged_by_shrapnel ) {
        total_damaged[pr.first] += pr.second;
    }

    const auto print_damage = [&]( const std::pair<const Creature *, int> &pr,
    std::function<bool( const Creature & )> predicate ) {
        if( predicate( *pr.first ) && g->u.sees( *pr.first ) ) {
            const Creature *critter = pr.first;
            bool blasted = damaged_by_blast.find( critter ) != damaged_by_blast.end();
            bool shredded = damaged_by_shrapnel.find( critter ) != damaged_by_shrapnel.end();
            std::string cause_description = ( blasted && shredded ) ? _( "the explosion and shrapnel" ) :
                                            blasted ? _( "the explosion" ) :
                                            _( "the shrapnel" );
            std::string damage_description = ( pr.second > 0 ) ?
                                             string_format( _( "taking %d damage" ), pr.second ) :
                                             _( "but takes no damage" );
            if( critter->is_player() ) {
                add_msg( _( "You are hit by %s, %s." ),
                         cause_description, damage_description );
            } else if( critter->is_npc() ) {
                critter->add_msg_if_npc(
                    _( "<npcname> is hit by %s, %s." ),
                    cause_description, damage_description );
            } else {
                add_msg( _( "%s is hit by %s, %s." ),
                         critter->disp_name( false, true ), cause_description, damage_description );
            }

        }
    };

    // TODO: Clean this up, without affecting the results
    for( const auto &pr : total_damaged ) {
        print_damage( pr, []( const Creature & c ) {
            return c.is_monster();
        } );
    }
    for( const auto &pr : total_damaged ) {
        print_damage( pr, []( const Creature & c ) {
            return c.is_npc();
        } );
    }
    for( const auto &pr : total_damaged ) {
        print_damage( pr, []( const Creature & c ) {
            return c.is_avatar();
        } );
    }
}

void flashbang( const tripoint &p, bool player_immune )
{
    draw_explosion( p, 8, c_white );
    int dist = rl_dist( g->u.pos(), p );
    if( dist <= 8 && !player_immune ) {
        if( !g->u.has_bionic( bio_ears ) && !g->u.is_wearing( "rm13_armor_on" ) ) {
            g->u.add_effect( effect_deaf, time_duration::from_turns( 40 - dist * 4 ) );
        }
        if( g->m.sees( g->u.pos(), p, 8 ) ) {
            int flash_mod = 0;
            if( g->u.has_trait( trait_PER_SLIME ) ) {
                if( one_in( 2 ) ) {
                    flash_mod = 3; // Yay, you weren't looking!
                }
            } else if( g->u.has_trait( trait_PER_SLIME_OK ) ) {
                flash_mod = 8; // Just retract those and extrude fresh eyes
            } else if( g->u.has_bionic( bio_sunglasses ) ||
                       g->u.is_wearing( "rm13_armor_on" ) ) {
                flash_mod = 6;
            } else if( g->u.worn_with_flag( flag_BLIND ) || g->u.worn_with_flag( flag_FLASH_PROTECTION ) ) {
                flash_mod = 3; // Not really proper flash protection, but better than nothing
            }
            g->u.add_env_effect( effect_blind, bp_eyes, ( 12 - flash_mod - dist ) / 2,
                                 time_duration::from_turns( 10 - dist ) );
        }
    }
    for( monster &critter : g->all_monsters() ) {
        if( critter.type->in_species( ROBOT ) ) {
            continue;
        }
        // TODO: can the following code be called for all types of creatures
        dist = rl_dist( critter.pos(), p );
        if( dist <= 8 ) {
            if( dist <= 4 ) {
                critter.add_effect( effect_stunned, time_duration::from_turns( 10 - dist ) );
            }
            if( critter.has_flag( MF_SEES ) && g->m.sees( critter.pos(), p, 8 ) ) {
                critter.add_effect( effect_blind, time_duration::from_turns( 18 - dist ) );
            }
            if( critter.has_flag( MF_HEARS ) ) {
                critter.add_effect( effect_deaf, time_duration::from_turns( 60 - dist * 4 ) );
            }
        }
    }
    sounds::sound( p, 12, sounds::sound_t::combat, _( "a huge boom!" ), false, "misc", "flashbang" );
    // TODO: Blind/deafen NPC
}

void shockwave( const tripoint &p, int radius, int force, int stun, int dam_mult,
                bool ignore_player )
{
    draw_explosion( p, radius, c_blue );

    sounds::sound( p, force * force * dam_mult / 2, sounds::sound_t::combat, _( "Crack!" ), false,
                   "misc", "shockwave" );

    for( monster &critter : g->all_monsters() ) {
        if( critter.posz() != p.z ) {
            continue;
        }
        if( rl_dist( critter.pos(), p ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), critter.name() );
            g->knockback( p, critter.pos(), force, stun, dam_mult );
        }
    }
    // TODO: combine the two loops and the case for g->u using all_creatures()
    for( npc &guy : g->all_npcs() ) {
        if( guy.posz() != p.z ) {
            continue;
        }
        if( rl_dist( guy.pos(), p ) <= radius ) {
            add_msg( _( "%s is caught in the shockwave!" ), guy.name );
            g->knockback( p, guy.pos(), force, stun, dam_mult );
        }
    }
    if( rl_dist( g->u.pos(), p ) <= radius && !ignore_player &&
        ( !g->u.has_trait( trait_LEG_TENT_BRACE ) || g->u.footwear_factor() == 1 ||
          ( g->u.footwear_factor() == .5 && one_in( 2 ) ) ) ) {
        add_msg( m_bad, _( "You're caught in the shockwave!" ) );
        g->knockback( p, g->u.pos(), force, stun, dam_mult );
    }
}

void scrambler_blast( const tripoint &p )
{
    if( monster *const mon_ptr = g->critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( critter.has_flag( MF_ELECTRONIC ) ) {
            critter.make_friendly();
        }
        add_msg( m_warning, _( "The %s sparks and begins searching for a target!" ),
                 critter.name() );
    }
}

void emp_blast( const tripoint &p )
{
    // TODO: Implement z part
    int x = p.x;
    int y = p.y;
    const bool sight = g->u.sees( p );
    if( g->m.has_flag( "CONSOLE", point( x, y ) ) ) {
        if( sight ) {
            add_msg( _( "The %s is rendered non-functional!" ), g->m.tername( point( x, y ) ) );
        }
        g->m.ter_set( point( x, y ), t_console_broken );
        return;
    }
    // TODO: More terrain effects.
    if( g->m.ter( point( x, y ) ) == t_card_science || g->m.ter( point( x, y ) ) == t_card_military ||
        g->m.ter( point( x, y ) ) == t_card_industrial ) {
        int rn = rng( 1, 100 );
        if( rn > 92 || rn < 40 ) {
            if( sight ) {
                add_msg( _( "The card reader is rendered non-functional." ) );
            }
            g->m.ter_set( point( x, y ), t_card_reader_broken );
        }
        if( rn > 80 ) {
            if( sight ) {
                add_msg( _( "The nearby doors slide open!" ) );
            }
            for( int i = -3; i <= 3; i++ ) {
                for( int j = -3; j <= 3; j++ ) {
                    if( g->m.ter( point( x + i, y + j ) ) == t_door_metal_locked ) {
                        g->m.ter_set( point( x + i, y + j ), t_floor );
                    }
                }
            }
        }
        if( rn >= 40 && rn <= 80 ) {
            if( sight ) {
                add_msg( _( "Nothing happens." ) );
            }
        }
    }
    if( monster *const mon_ptr = g->critter_at<monster>( p ) ) {
        monster &critter = *mon_ptr;
        if( critter.has_flag( MF_ELECTRONIC ) ) {
            int deact_chance = 0;
            const auto mon_item_id = critter.type->revert_to_itype;
            switch( critter.get_size() ) {
                case MS_TINY:
                    deact_chance = 6;
                    break;
                case MS_SMALL:
                    deact_chance = 3;
                    break;
                default:
                    // Currently not used, I have no idea what chances bigger bots should have,
                    // Maybe export this to json?
                    break;
            }
            if( !mon_item_id.empty() && deact_chance != 0 && one_in( deact_chance ) ) {
                if( sight ) {
                    add_msg( _( "The %s beeps erratically and deactivates!" ), critter.name() );
                }
                g->m.add_item_or_charges( p, critter.to_item() );
                for( auto &ammodef : critter.ammo ) {
                    if( ammodef.second > 0 ) {
                        g->m.spawn_item( p, ammodef.first, 1, ammodef.second, calendar::turn );
                    }
                }
                g->remove_zombie( critter );
            } else {
                if( sight ) {
                    add_msg( _( "The EMP blast fries the %s!" ), critter.name() );
                }
                int dam = dice( 10, 10 );
                critter.apply_damage( nullptr, bodypart_id( "torso" ), dam );
                critter.check_dead_state();
                if( !critter.is_dead() && one_in( 6 ) ) {
                    critter.make_friendly();
                }
            }
        } else if( critter.has_flag( MF_ELECTRIC_FIELD ) ) {
            if( sight && !critter.has_effect( effect_emp ) ) {
                add_msg( m_good, _( "The %s's electrical field momentarily goes out!" ), critter.name() );
                critter.add_effect( effect_emp, 3_minutes );
            } else if( sight && critter.has_effect( effect_emp ) ) {
                int dam = dice( 3, 5 );
                add_msg( m_good, _( "The %s's disabled electrical field reverses polarity!" ),
                         critter.name() );
                add_msg( m_good, _( "It takes %d damage." ), dam );
                critter.add_effect( effect_emp, 1_minutes );
                critter.apply_damage( nullptr, bodypart_id( "torso" ), dam );
                critter.check_dead_state();
            }
        } else if( sight ) {
            add_msg( _( "The %s is unaffected by the EMP blast." ), critter.name() );
        }
    }
    if( g->u.posx() == x && g->u.posy() == y ) {
        if( g->u.get_power_level() > 0_kJ ) {
            add_msg( m_bad, _( "The EMP blast drains your power." ) );
            int max_drain = ( g->u.get_power_level() > 1000_kJ ? 1000 : units::to_kilojoule(
                                  g->u.get_power_level() ) );
            g->u.mod_power_level( units::from_kilojoule( -rng( 1 + max_drain / 3, max_drain ) ) );
        }
        // TODO: More effects?
        //e-handcuffs effects
        if( g->u.weapon.typeId() == "e_handcuffs" && g->u.weapon.charges > 0 ) {
            g->u.weapon.item_tags.erase( "NO_UNWIELD" );
            g->u.weapon.charges = 0;
            g->u.weapon.active = false;
            add_msg( m_good, _( "The %s on your wrists spark briefly, then release your hands!" ),
                     g->u.weapon.tname() );
        }
    }
    // Drain any items of their battery charge
    for( auto &it : g->m.i_at( point( x, y ) ) ) {
        if( it.is_tool() && it.ammo_current() == "battery" ) {
            it.charges = 0;
        }
    }
    // TODO: Drain NPC energy reserves
}

void resonance_cascade( const tripoint &p )
{
    const time_duration maxglow = time_duration::from_turns( 100 - 5 * trig_dist( p, g->u.pos() ) );
    MonsterGroupResult spawn_details;
    if( maxglow > 0_turns ) {
        const time_duration minglow = std::max( 0_turns, time_duration::from_turns( 60 - 5 * trig_dist( p,
                                                g->u.pos() ) ) );
        g->u.add_effect( effect_teleglow, rng( minglow, maxglow ) * 100 );
    }
    int startx = ( p.x < 8 ? 0 : p.x - 8 ), endx = ( p.x + 8 >= SEEX * 3 ? SEEX * 3 - 1 : p.x + 8 );
    int starty = ( p.y < 8 ? 0 : p.y - 8 ), endy = ( p.y + 8 >= SEEY * 3 ? SEEY * 3 - 1 : p.y + 8 );
    tripoint dest( startx, starty, p.z );
    for( int &i = dest.x; i <= endx; i++ ) {
        for( int &j = dest.y; j <= endy; j++ ) {
            switch( rng( 1, 80 ) ) {
                case 1:
                case 2:
                    emp_blast( dest );
                    break;
                case 3:
                case 4:
                case 5:
                    for( int k = i - 1; k <= i + 1; k++ ) {
                        for( int l = j - 1; l <= j + 1; l++ ) {
                            field_type_id type = fd_null;
                            switch( rng( 1, 7 ) ) {
                                case 1:
                                    type = fd_blood;
                                    break;
                                case 2:
                                    type = fd_bile;
                                    break;
                                case 3:
                                case 4:
                                    type = fd_slime;
                                    break;
                                case 5:
                                    type = fd_fire;
                                    break;
                                case 6:
                                case 7:
                                    type = fd_nuke_gas;
                                    break;
                            }
                            if( !one_in( 3 ) ) {
                                g->m.add_field( { k, l, p.z }, type, 3 );
                            }
                        }
                    }
                    break;
                case  6:
                case  7:
                case  8:
                case  9:
                case 10:
                    g->m.trap_set( dest, tr_portal );
                    break;
                case 11:
                case 12:
                    g->m.trap_set( dest, tr_goo );
                    break;
                case 13:
                case 14:
                case 15:
                    spawn_details = MonsterGroupManager::GetResultFromGroup( GROUP_NETHER );
                    g->place_critter_at( spawn_details.name, dest );
                    break;
                case 16:
                case 17:
                case 18:
                    g->m.destroy( dest );
                    break;
                case 19:
                    explosion( dest, rng( 1, 10 ), rng( 0, 1 ) * rng( 0, 6 ), one_in( 4 ) );
                    break;
                default:
                    break;
            }
        }
    }
}

projectile shrapnel_from_legacy( int power, float blast_radius )
{
    int range = 2 * blast_radius;
    // Damage approximately equal to blast damage at epicenter
    int damage = power * power_to_dmg_mult;
    projectile proj;
    proj.speed = 1000;
    proj.range = range;
    proj.impact.add_damage( DT_CUT, damage, 0.0f, 3.0f );

    return proj;
}

float blast_radius_from_legacy( int power, float distance_factor )
{
    return std::pow( power * power_to_dmg_mult, ( 1.0 / 4.0 ) ) *
           ( std::log( 0.75f ) / std::log( distance_factor ) );
}

} // namespace explosion_handler

float shrapnel_calc( const float &intensity, const float &last_obstacle, const int & )
{
    return intensity - last_obstacle;
}

bool shrapnel_check( const float &obstacle, const float &last_intensity )
{
    return last_intensity - obstacle > 0.0f;
}

void update_fragment_cloud( float &output, const float &new_intensity, quadrant )
{
    output = std::max( output, new_intensity );
}

float accumulate_fragment_cloud( const float &cumulative_obstacle, const float &current_obstacle,
                                 const int & )
{
    return std::max( cumulative_obstacle, current_obstacle ) + 1;
}

int explosion_data::safe_range() const
{
    return std::max<int>( radius, fragment ? fragment->range : 0 ) + 1;
}

explosion_data::operator bool() const
{
    return damage > 0 || fragment;
}

float blast_percentage( float range, float distance )
{
    return distance > range ? 0.0f :
           distance > ( range / 2 ) ? 0.5f :
           1.0f;
}
