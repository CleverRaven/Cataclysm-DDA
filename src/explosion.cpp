#include "explosion.h"
#include "cata_utility.h"
#include "game.h"
#include "map.h"
#include "projectile.h"
#include "json.h"
#include "creature.h"
#include "character.h"
#include "player.h"
#include "monster.h"
#include "vpart_position.h"
#include "output.h"
#include "debug.h"
#include "messages.h"
#include "translations.h"
#include "sounds.h"
#include "vehicle.h"
#include "field.h"
#include <queue>
#include <algorithm>
#include <cmath>

static const itype_id null_itype( "null" );

tripoint random_perimeter( const tripoint &src, const int radius )
{
    tripoint dst;
    calc_ray_end( rng( 1, 360 ), radius, src, dst );
    return dst;
}

explosion_data load_explosion_data( JsonObject &jo )
{
    explosion_data ret;
    // Power is mandatory
    jo.read( "power", ret.power );
    // Rest isn't
    ret.distance_factor = jo.get_float( "distance_factor", 0.8f );
    ret.fire = jo.get_bool( "fire", false );
    if( jo.has_int( "shrapnel" ) ) {
        ret.shrapnel.count = jo.get_int( "shrapnel" );
        ret.shrapnel.mass = 10;
        ret.shrapnel.recovery = 0;
        ret.shrapnel.drop = null_itype;
    } else if( jo.has_object( "shrapnel" ) ) {
        auto shr = jo.get_object( "shrapnel" );
        ret.shrapnel = load_shrapnel_data( shr );
    }

    return ret;
}

shrapnel_data load_shrapnel_data( JsonObject &jo )
{
    shrapnel_data ret;
    // Count is mandatory
    jo.read( "count", ret.count );
    // Rest isn't
    ret.mass = jo.get_int( "mass", 10 );
    ret.recovery = jo.get_int( "recovery", 0 );
    ret.drop = itype_id( jo.get_string( "drop", "null" ) );
    return ret;
}

// (C1001) Compiler Internal Error on Visual Studio 2015 with Update 2
void game::do_blast( const tripoint &p, const float power,
                     const float distance_factor, const bool fire )
{
    const float tile_dist = 1.0f;
    const float diag_dist = trigdist ? 1.41f * tile_dist : 1.0f * tile_dist;
    const float zlev_dist = 2.0f; // Penalty for going up/down
    // 7 3 5
    // 1 . 2
    // 6 4 8
    // 9 and 10 are up and down
    static const int x_offset[10] = { -1, 1,  0, 0,  1, -1, -1, 1, 0, 0  };
    static const int y_offset[10] = {  0, 0, -1, 1, -1,  1, -1, 1, 0, 0  };
    static const int z_offset[10] = {  0, 0,  0, 0,  0,  0,  0, 0, 1, -1 };
    const size_t max_index = m.has_zlevels() ? 10 : 8;

    m.bash( p, fire ? power : ( 2 * power ), true, false, false );

    std::priority_queue< std::pair<float, tripoint>, std::vector< std::pair<float, tripoint> >, pair_greater_cmp >
    open;
    std::set<tripoint> closed;
    std::map<tripoint, float> dist_map;
    open.push( std::make_pair( 0.0f, p ) );
    dist_map[p] = 0.0f;
    // Find all points to blast
    while( !open.empty() ) {
        // Add some random factor to effective distance to make it look cooler
        const float distance = open.top().first * rng_float( 1.0f, 1.2f );
        const tripoint pt = open.top().second;
        open.pop();

        if( closed.count( pt ) != 0 ) {
            continue;
        }

        closed.insert( pt );

        const float force = power * std::pow( distance_factor, distance );
        if( force <= 1.0f ) {
            continue;
        }

        if( m.impassable( pt ) && pt != p ) {
            // Don't propagate further
            continue;
        }

        // Those will be used for making "shaped charges"
        // Don't check up/down (for now) - this will make 2D/3D balancing easier
        int empty_neighbors = 0;
        for( size_t i = 0; i < 8; i++ ) {
            tripoint dest( pt.x + x_offset[i], pt.y + y_offset[i], pt.z + z_offset[i] );
            if( closed.count( dest ) == 0 && m.valid_move( pt, dest, false, true ) ) {
                empty_neighbors++;
            }
        }

        empty_neighbors = std::max( 1, empty_neighbors );
        // Iterate over all neighbors. Bash all of them, propagate to some
        for( size_t i = 0; i < max_index; i++ ) {
            tripoint dest( pt.x + x_offset[i], pt.y + y_offset[i], pt.z + z_offset[i] );
            if( closed.count( dest ) != 0 ) {
                continue;
            }

            // Up to 200% bonus for shaped charge
            // But not if the explosion is fiery, then only half the force and no bonus
            const float bash_force = !fire ?
                                     force + ( 2 * force / empty_neighbors ) :
                                     force / 2;
            if( z_offset[i] == 0 ) {
                // Horizontal - no floor bashing
                m.bash( dest, bash_force, true, false, false );
            } else if( z_offset[i] > 0 ) {
                // Should actually bash through the floor first, but that's not really possible yet
                m.bash( dest, bash_force, true, false, true );
            } else if( !m.valid_move( pt, dest, false, true ) ) {
                // Only bash through floor if it doesn't exist
                // Bash the current tile's floor, not the one's below
                m.bash( pt, bash_force, true, false, true );
            }

            float next_dist = distance;
            next_dist += ( x_offset[i] == 0 || y_offset[i] == 0 ) ? tile_dist : diag_dist;
            if( z_offset[i] != 0 ) {
                if( !m.valid_move( pt, dest, false, true ) ) {
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
        if( m.impassable( pt ) ) {
            continue;
        }

        const float force = power * std::pow( distance_factor, dist_map.at( pt ) );
        nc_color col = c_red;
        if( force < 10 ) {
            col = c_white;
        } else if( force < 30 ) {
            col = c_yellow;
        }

        explosion_colors[pt] = col;
    }

    draw_custom_explosion( u.pos(), explosion_colors );

    for( const tripoint &pt : closed ) {
        const float force = power * std::pow( distance_factor, dist_map.at( pt ) );
        if( force < 1.0f ) {
            // Too weak to matter
            continue;
        }

        m.smash_items( pt, force );

        if( fire ) {
            int density = ( force > 50.0f ) + ( force > 100.0f );
            if( force > 10.0f || x_in_y( force, 10.0f ) ) {
                density++;
            }

            if( !m.has_zlevels() && m.is_outside( pt ) && density == 2 ) {
                // In 3D mode, it would have fire fields above, which would then fall
                // and fuel the fire on this tile
                density++;
            }

            m.add_field( pt, fd_fire, density );
        }

        if( const optional_vpart_position vp = m.veh_at( pt ) ) {
            // TODO: Make this weird unit used by vehicle::damage more sensible
            vp->vehicle().damage( vp->part_index(), force, fire ? DT_HEAT : DT_BASH, false );
        }

        Creature *critter = critter_at( pt, true );
        if( critter == nullptr ) {
            continue;
        }

        add_msg( m_debug, "Blast hits %s with force %.1f",
                 critter->disp_name().c_str(), force );

        player *pl = dynamic_cast<player *>( critter );
        if( pl == nullptr ) {
            // TODO: player's fault?
            const int dmg = force - ( critter->get_armor_bash( bp_torso ) / 2 );
            const int actual_dmg = rng( dmg * 2, dmg * 3 );
            critter->apply_damage( nullptr, bp_torso, actual_dmg );
            critter->check_dead_state();
            add_msg( m_debug, "Blast hits %s for %d damage", critter->disp_name().c_str(), actual_dmg );
            continue;
        }

        // Print messages for all NPCs
        pl->add_msg_player_or_npc( m_bad, _( "You're caught in the explosion!" ),
                                   _( "<npcname> is caught in the explosion!" ) );

        struct blastable_part {
            body_part bp;
            float low_mul;
            float high_mul;
            float armor_mul;
        };

        static const std::array<blastable_part, 6> blast_parts = { {
                { bp_torso, 2.0f, 3.0f, 0.5f },
                { bp_head,  2.0f, 3.0f, 0.5f },
                // Hit limbs harder so that it hurts more without being much more deadly
                { bp_leg_l, 2.0f, 3.5f, 0.4f },
                { bp_leg_r, 2.0f, 3.5f, 0.4f },
                { bp_arm_l, 2.0f, 3.5f, 0.4f },
                { bp_arm_r, 2.0f, 3.5f, 0.4f },
            }
        };

        for( const auto &blp : blast_parts ) {
            const int part_dam = rng( force * blp.low_mul, force * blp.high_mul );
            const std::string hit_part_name = body_part_name_accusative( blp.bp );
            const auto dmg_instance = damage_instance( DT_BASH, part_dam, 0, blp.armor_mul );
            const auto result = pl->deal_damage( nullptr, blp.bp, dmg_instance );
            const int res_dmg = result.total_damage();

            add_msg( m_debug, "%s for %d raw, %d actual",
                     hit_part_name.c_str(), part_dam, res_dmg );
            if( res_dmg > 0 ) {
                pl->add_msg_if_player( m_bad, _( "Your %s is hit for %d damage!" ),
                                       hit_part_name.c_str(), res_dmg );
            }
        }
    }
}

std::unordered_map<tripoint, std::pair<int, int>> game::explosion( const tripoint &p, float power,
        float factor, bool fire, int shrapnel_count, int shrapnel_mass )
{
    explosion_data data;
    data.power = power;
    data.distance_factor = factor;
    data.fire = fire;
    data.shrapnel.count = shrapnel_count;
    data.shrapnel.mass = shrapnel_mass;
    return explosion( p, data );
}

std::unordered_map<tripoint, std::pair<int, int>> game::explosion( const tripoint &p,
        const explosion_data &ex )
{
    // contains all tiles considered plus sum of damage received by each from shockwave and/or shrapnel
    std::unordered_map<tripoint, std::pair<int, int>> distrib;

    const int noise = ex.power * ( ex.fire ? 2 : 10 );
    if( noise >= 30 ) {
        sounds::sound( p, noise, _( "a huge explosion!" ) );
        sfx::play_variant_sound( "explosion", "huge", 100 );
    } else if( noise >= 4 ) {
        sounds::sound( p, noise, _( "an explosion!" ) );
        sfx::play_variant_sound( "explosion", "default", 100 );
    } else if( noise > 0 ) {
        sounds::sound( p, 3, _( "a loud pop!" ) );
        sfx::play_variant_sound( "explosion", "small", 100 );
    }

    if( ex.distance_factor >= 1.0f ) {
        debugmsg( "called game::explosion with factor >= 1.0 (infinite size)" );
    } else if( ex.distance_factor > 0.0f && ex.power > 0.0f ) {
        // @todo: return map containing distribution of damage
        do_blast( p, ex.power, ex.distance_factor, ex.fire );
    }

    const auto &shr = ex.shrapnel;
    if( shr.count > 0 ) {
        int shrapnel_power = ( log( ex.power ) + 1 ) * shr.mass;
        auto res = shrapnel( p, shrapnel_power, shr.count, shr.mass );
        for( const auto &e : res ) {
            if( distrib.count( e.first ) ) {
                // If tile was already affected by blast just update the shrapnel field
                distrib[ e.first ].second = e.second;
            } else {
                // Otherwise add the tile but mark is as unaffected by the blast (-1)
                distrib[ e.first ] = std::make_pair( -1, e.second );
            }
        }

        // If explosion drops shrapnel...
        if( shr.count > 0 && shr.recovery > 0 && shr.drop != "null" ) {

            // Extract only passable tiles affected by shrapnel
            std::vector<tripoint> tiles;
            for( const auto &e : distrib ) {
                if( g->m.passable( e.first ) && e.second.second >= 0 ) {
                    tiles.push_back( e.first );
                }
            }

            // Truncate to a random selection
            int qty = shr.count * std::min( shr.recovery, 100 ) / 100;
            std::random_shuffle( tiles.begin(), tiles.end() );
            tiles.resize( std::min( int( tiles.size() ), qty ) );

            for( const auto &e : tiles ) {
                g->m.add_item_or_charges( e, item( shr.drop, calendar::turn, item::solitary_tag{} ) );
            }
        }
    }

    return distrib;
}

std::unordered_map<tripoint, int> game::shrapnel( const tripoint &src, int power, int count,
        int mass, int range )
{
    if( range < 0 ) {
        range = std::max( ( 2 * log( power / 2 ) ) + 2, 0.0 );
    }

    // contains of all tiles considered with value being sum of damage received (if any)
    std::unordered_map<tripoint, int> distrib;

    projectile proj;
    proj.speed = 1000; // no dodging shrapnel
    proj.range = range;
    proj.proj_effects.insert( "NULL_SOURCE" );
    proj.proj_effects.insert( "WIDE" ); // suppress MF_HARDTOSHOOT

    auto func = [this, &distrib, &mass, &proj]( const tripoint & e, int &kinetic ) {
        distrib[ e ] += 0; // add this tile to the distribution

        auto critter = critter_at( e );
        if( critter && !critter->is_dead_state() ) {
            dealt_projectile_attack frag;
            frag.proj = proj;
            frag.missed_by = rng_float( 0.2, 0.6 );
            frag.proj.impact = damage_instance::physical( 0, kinetic * 3, 0, std::min( kinetic, mass ) );

            distrib[ e ] += kinetic; // increase received damage for tile in distribution

            critter->deal_projectile_attack( nullptr, frag );
            return false;
        }

        if( m.impassable( e ) ) {
            // massive shrapnel can smash a path through obstacles
            int force = std::min( kinetic, mass );
            int resistance;

            if( optional_vpart_position vp = m.veh_at( e ) ) {
                resistance = force - vp->vehicle().damage( vp->part_index(), force );

            } else {
                resistance = std::max( m.bash_resistance( e ), 0 );
                m.bash( e, force, true );
            }

            if( m.passable( e ) ) {
                distrib[ e ] += resistance; // obstacle absorbed only some of the force
                kinetic -= resistance;
            } else {
                distrib[ e ] += kinetic; // obstacle absorbed all of the force
                return false;
            }
        }

        // @todo: apply effects of soft cover
        return kinetic > 0;
    };

    for( auto i = 0; i != count; ++i ) {
        int kinetic = power;

        // special case critter at epicenter to have equivalent chance to adjacent tile
        if( one_in( 8 ) && !func( src, kinetic ) ) {
            continue;
        }

        // shrapnel otherwise expands randomly in all directions
        bresenham( src, random_perimeter( src, range ), 0, 0, [&func, &kinetic]( const tripoint & e ) {
            return func( e, kinetic );
        } );
    }

    return distrib;
}

float explosion_data::expected_range( float ratio ) const
{
    if( power <= 0.0f || distance_factor >= 1.0f || distance_factor <= 0.0f ) {
        return 0.0f;
    }

    // The 1.1 is because actual power drops at roughly that rate
    return std::log( ratio ) / std::log( distance_factor / 1.1f );
}

float explosion_data::power_at_range( float dist ) const
{
    if( power <= 0.0f || distance_factor >= 1.0f || distance_factor <= 0.0f ) {
        return 0.0f;
    }

    // The 1.1 is because actual power drops at roughly that rate
    return power * std::pow( distance_factor / 1.1f, dist );
}

int explosion_data::safe_range() const
{
    const float ratio = 1 / power / 2;
    return expected_range( ratio ) + 1;
}
