#include "explosion.h" // IWYU pragma: associated
#include "fragment_cloud.h" // IWYU pragma: associated

#include <algorithm>
#include <chrono>

#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "item_factory.h"
#include "json.h"
#include "math_defines.h"
#include "map.h"
#include "messages.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "shadowcasting.h"
#include "sounds.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"

#include <queue>
#include <random>

static const itype_id null_itype( "null" );

explosion_data load_explosion_data( JsonObject &jo )
{
    explosion_data ret;
    // Power is mandatory
    jo.read( "power", ret.power );
    // Rest isn't
    ret.distance_factor = jo.get_float( "distance_factor", 0.8f );
    ret.fire = jo.get_bool( "fire", false );
    if( jo.has_int( "shrapnel" ) ) {
        ret.shrapnel.casing_mass = jo.get_int( "shrapnel" );
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
    // Casing mass is mandatory
    jo.read( "casing_mass", ret.casing_mass );
    // Rest isn't
    ret.fragment_mass = jo.get_float( "fragment_mass", 0.005 );
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

    std::priority_queue< std::pair<float, tripoint>, std::vector< std::pair<float, tripoint> >, pair_greater_cmp_first >
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
            if( closed.count( dest ) != 0 || !m.inbounds( dest ) ) {
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

void game::explosion( const tripoint &p, float power, float factor, bool fire,
                      int casing_mass, float fragment_mass )
{
    explosion_data data;
    data.power = power;
    data.distance_factor = factor;
    data.fire = fire;
    data.shrapnel.casing_mass = casing_mass;
    data.shrapnel.fragment_mass = fragment_mass;
    explosion( p, data );
}

void game::explosion( const tripoint &p, const explosion_data &ex )
{
    const int noise = ex.power * ( ex.fire ? 2 : 10 );
    if( noise >= 30 ) {
        sounds::sound( p, noise, sounds::sound_t::combat, _( "a huge explosion!" ) );
        sfx::play_variant_sound( "explosion", "huge", 100 );
    } else if( noise >= 4 ) {
        sounds::sound( p, noise, sounds::sound_t::combat, _( "an explosion!" ) );
        sfx::play_variant_sound( "explosion", "default", 100 );
    } else if( noise > 0 ) {
        sounds::sound( p, 3, sounds::sound_t::combat, _( "a loud pop!" ) );
        sfx::play_variant_sound( "explosion", "small", 100 );
    }

    if( ex.distance_factor >= 1.0f ) {
        debugmsg( "called game::explosion with factor >= 1.0 (infinite size)" );
    } else if( ex.distance_factor > 0.0f && ex.power > 0.0f ) {
        // Power rescaled to mean grams of TNT equivalent, this scales it roughly back to where
        // it was before until we re-do blasting power to be based on TNT-equivalent directly.
        do_blast( p, ex.power / 15.0, ex.distance_factor, ex.fire );
    }

    const auto &shr = ex.shrapnel;
    if( shr.casing_mass > 0 ) {
        auto shrapnel_locations = shrapnel( p, ex.power, shr.casing_mass, shr.fragment_mass );

        // If explosion drops shrapnel...
        if( shr.recovery > 0 && shr.drop != "null" ) {

            // Extract only passable tiles affected by shrapnel
            std::vector<tripoint> tiles;
            for( const auto &e : shrapnel_locations ) {
                if( g->m.passable( e ) ) {
                    tiles.push_back( e );
                }
            }
            const itype *fragment_drop = item_controller->find_template( shr.drop );
            int qty = shr.casing_mass * std::min( 1.0, shr.recovery / 100.0 ) /
                      to_gram( fragment_drop->weight );
            // Truncate to a random selection
            std::shuffle( tiles.begin(), tiles.end(), rng_get_engine() );
            tiles.resize( std::min( int( tiles.size() ), qty ) );

            for( const auto &e : tiles ) {
                g->m.add_item_or_charges( e, item( shr.drop, calendar::turn, item::solitary_tag{} ) );
            }
        }
    }
}

int ballistic_damage( float velocity, float mass )
{
    // Damage is square root of Joules, dividing by 2000 because it's dividing by 2 and
    // converting mass from grams to kg. 5 is simply a scaling factor.
    return 2.0 * std::sqrt( ( velocity * velocity  * mass ) / 2000.0 );
}

// This is only ever used to zero the cloud values, which is what makes it work.
fragment_cloud &fragment_cloud::operator=( const float &value )
{
    velocity = value;
    density = value;

    return *this;
}

bool fragment_cloud::operator==( const fragment_cloud &that )
{
    return velocity == that.velocity && density == that.density;
}

bool operator<( const fragment_cloud &us, const fragment_cloud &them )
{
    return us.density < them.density && us.velocity < them.velocity;
}

// Global to smuggle data into shrapnel_calc() function without replicating it across entire map.
// Mass in kg
float fragment_mass = 0.0001;
// Cross-sectional area in cm^2
float fragment_area = 0.00001;

// Projectile velocity in air. See https://fas.org/man/dod-101/navy/docs/es310/warheads/Warheads.htm
// for a writeup of this exact calculation.
fragment_cloud shrapnel_calc( const fragment_cloud &initial,
                              const fragment_cloud &cloud,
                              const int &distance )
{
    // SWAG coefficient of drag.
    constexpr float Cd = 0.5;
    fragment_cloud new_cloud;
    new_cloud.velocity = initial.velocity * exp( -cloud.velocity * ( ( Cd * fragment_area * distance ) /
                         ( 2.0 * fragment_mass ) ) );
    // Two effects, the accumulated proportion of blocked fragments,
    // and the inverse-square dilution of fragments with distance.
    new_cloud.density = ( initial.density * cloud.density ) / ( distance * distance / 2.5 );
    return new_cloud;
}
// Minimum velocity resulting in skin perforation according to https://www.ncbi.nlm.nih.gov/pubmed/7304523
constexpr float MIN_EFFECTIVE_VELOCITY = 70.0;
// Pretty arbitrary minimum density.  1/1,000 change of a fragment passing through the given square.
constexpr float MIN_FRAGMENT_DENSITY = 0.0001;
bool shrapnel_check( const fragment_cloud &cloud, const fragment_cloud &intensity )
{
    return cloud.density > 0.0 && intensity.velocity > MIN_EFFECTIVE_VELOCITY &&
           intensity.density > MIN_FRAGMENT_DENSITY;
}

void update_fragment_cloud( fragment_cloud &update, const fragment_cloud &new_value, quadrant )
{
    update = std::max( update, new_value );
}

fragment_cloud accumulate_fragment_cloud( const fragment_cloud &cumulative_cloud,
        const fragment_cloud &current_cloud, const int &distance )
{
    // Velocity is the cumulative and continuous decay of speed,
    // so it is accumulated the same way as light attentuation.
    // Density is the accumulation of discrete attenuaton events encountered in the traversed squares,
    // so each term is added to the series via multiplication.
    return fragment_cloud( ( ( distance - 1 ) * cumulative_cloud.velocity + current_cloud.velocity ) /
                           distance,
                           cumulative_cloud.density * current_cloud.density );
}

// Approximate Gurney constant for Composition B and C (in m/s instead of the usual km/s).
// Source: https://en.wikipedia.org/wiki/Gurney_equations#Gurney_constant_and_detonation_velocity
constexpr double TYPICAL_GURNEY_CONSTANT = 2700.0;
static float gurney_spherical( const double charge, const double mass )
{
    return static_cast<float>( std::pow( ( mass / charge ) + ( 3.0 / 5.0 ),
                                         -0.5 ) * TYPICAL_GURNEY_CONSTANT );
}

// Calculate cross-sectional area of a steel sphere in cm^2 based on mass of fragment.
static float mass_to_area( const float mass )
{
    // Density of steel in g/cm^3
    constexpr float steel_density = 7.85;
    float fragment_volume = ( mass / 1000.0 ) / steel_density;
    float fragment_radius = cbrt( ( fragment_volume * 3.0 ) / ( 4.0 * M_PI ) );
    return fragment_radius * fragment_radius * M_PI;
}

std::vector<tripoint> game::shrapnel( const tripoint &src, int power,
                                      int casing_mass, float per_fragment_mass, int range )
{
    // The gurney equation wants the total mass of the casing.
    const float fragment_velocity = gurney_spherical( power, casing_mass );
    fragment_mass = per_fragment_mass;
    fragment_area = mass_to_area( fragment_mass );
    int fragment_count = casing_mass / fragment_mass;

    // Contains all tiles reached by fragments.
    std::vector<tripoint> distrib;

    projectile proj;
    proj.speed = fragment_velocity;
    proj.range = range;
    proj.proj_effects.insert( "NULL_SOURCE" );

    fragment_cloud obstacle_cache[ MAPSIZE_X ][ MAPSIZE_Y ];
    fragment_cloud visited_cache[ MAPSIZE_X ][ MAPSIZE_Y ];

    // TODO: Calculate range based on max effective range for projectiles.
    // Basically bisect between 0 and map diameter using shrapnel_calc().
    // Need to update shadowcasting to support limiting range without adjusting initial distance.
    const tripoint start = { 0, 0, src.z };
    const tripoint end = { m.getmapsize() *SEEX, m.getmapsize() *SEEY, src.z };

    m.build_obstacle_cache( start, end, obstacle_cache );

    // Shadowcasting normally ignores the origin square,
    // so apply it manually to catch monsters standing on the explosive.
    // This "blocks" some fragments, but does not apply deceleration.
    fragment_cloud initial_cloud = accumulate_fragment_cloud( obstacle_cache[src.x][src.y],
    { fragment_velocity, static_cast<float>( fragment_count ) }, 1 );
    visited_cache[src.x][src.y] = initial_cloud;

    castLightAll<fragment_cloud, fragment_cloud, shrapnel_calc, shrapnel_check,
                 update_fragment_cloud, accumulate_fragment_cloud>
                 ( visited_cache, obstacle_cache, src.x, src.y, 0, initial_cloud );

    // Now visited_caches are populated with density and velocity of fragments.
    for( int x = start.x; x < end.x; x++ ) {
        for( int y = start.y; y < end.y; y++ ) {
            fragment_cloud &cloud = visited_cache[x][y];
            if( cloud.density <= MIN_FRAGMENT_DENSITY ||
                cloud.velocity <= MIN_EFFECTIVE_VELOCITY ) {
                continue;
            }
            distrib.emplace_back( x, y, src.z );
            tripoint target( x, y, src.z );
            int damage = ballistic_damage( cloud.velocity, fragment_mass );
            auto critter = critter_at( target );
            if( damage > 0 && critter && !critter->is_dead_state() ) {
                std::poisson_distribution<> d( cloud.density );
                int hits = d( rng_get_engine() );
                dealt_projectile_attack frag;
                frag.proj = proj;
                frag.proj.speed = cloud.velocity;
                frag.proj.impact = damage_instance::physical( 0, damage, 0, 0 );
                // dealt_dam.total_damage() == 0 means armor block
                // dealt_dam.total_damage() > 0 means took damage
                // Need to diffentiate target among player, npc, and monster
                // Do we even print monster damage?
                int damage_taken = 0;
                int damaging_hits = 0;
                int non_damaging_hits = 0;
                for( int i = 0; i < hits; ++i ) {
                    frag.missed_by = rng_float( 0.05, 1.0 );
                    critter->deal_projectile_attack( nullptr, frag, false );
                    if( frag.dealt_dam.total_damage() > 0 ) {
                        damaging_hits++;
                        damage_taken += frag.dealt_dam.total_damage();
                    } else {
                        non_damaging_hits++;
                    }
                    add_msg( m_debug, "Shrapnel hit %s at %d m/s at a distance of %d",
                             critter->disp_name().c_str(),
                             frag.proj.speed, rl_dist( src, target ) );
                    add_msg( m_debug, "Shrapnel dealt %d damage", frag.dealt_dam.total_damage() );
                    if( critter->is_dead_state() ) {
                        break;
                    }
                }
                int total_hits = damaging_hits + non_damaging_hits;
                if( total_hits > 0 && g->u.sees( *critter ) ) {
                    // Building a phrase to summarize the fragment effects.
                    // Target, Number of impacts, total amount of damage, proportion of deflected fragments.
                    std::map<int, std::string> impact_count_descriptions = {
                        { 1, _( "a" ) }, { 2, _( "several" ) }, { 5, _( "many" ) },
                        { 20, _( "a large number of" ) }, { 100, _( "a huge number of" ) },
                        { std::numeric_limits<int>::max(), _( "an immense number of" ) }
                    };
                    std::string impact_count = std::find_if(
                                                   impact_count_descriptions.begin(), impact_count_descriptions.end(),
                    [ total_hits ]( std::pair<int, std::string> desc ) {
                        return desc.first >= total_hits;
                    } )->second;
                    std::string damage_description = ( damage_taken > 0 ) ?
                                                     string_format( _( "dealing %d damage" ), damage_taken ) :
                                                     _( "but they deal no damage" );
                    if( critter->is_player() ) {
                        add_msg( ngettext( "You are hit by %s bomb fragment, %s.",
                                           "You are hit by %s bomb fragments, %s.", total_hits ),
                                 impact_count.c_str(), damage_description );
                    } else if( critter->is_npc() ) {
                        critter->add_msg_if_npc(
                            ngettext( "<npcname> is hit by %s bomb fragment, %s.",
                                      "<npcname> is hit by %s bomb fragments, %s.",
                                      total_hits ),
                            impact_count, damage_description );
                    } else {
                        add_msg( ngettext( "The %s is hit by %s bomb fragment, %s.",
                                           "The %s is hit by %s bomb fragments, %s.", total_hits ),
                                 critter->disp_name().c_str(), impact_count, damage_description );
                    }
                }
            }
            if( m.impassable( target ) ) {
                if( optional_vpart_position vp = m.veh_at( target ) ) {
                    vp->vehicle().damage( vp->part_index(), damage );
                } else {
                    m.bash( target, damage / 10, true );
                }
            }
        }
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
