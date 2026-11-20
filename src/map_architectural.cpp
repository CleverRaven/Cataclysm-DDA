/**
* For general map caches; gravity-, spatial-, and structure-related functions
*/
#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "bodypart.h"
#include "cata_assert.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cuboid_rectangle.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "fragment_cloud.h"
#include "game.h"
#include "item.h"
#include "line.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "perf.h"
#include "ranged.h"
#include "rng.h"
#include "submap.h"
#include "translations.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const bionic_id bio_shock_absorber( "bio_shock_absorber" );

static const damage_type_id damage_bash( "bash" );

static const efftype_id effect_psi_stunned( "psi_stunned" );
static const efftype_id effect_slow_descent( "slow_descent" );
static const efftype_id effect_strengthened_gravity( "strengthened_gravity" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_weakened_gravity( "weakened_gravity" );

static const flag_id json_flag_JETPACK( "JETPACK" );
static const flag_id json_flag_LEVITATION( "LEVITATION" );

static const json_character_flag json_flag_ONE_STORY_FALL( "ONE_STORY_FALL" );
static const json_character_flag json_flag_WALL_CLING( "WALL_CLING" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_rock_floor( "t_rock_floor" );

bool map::has_floor_gap_flag( const ter_t &terrain ) const
{
    return terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ||
           terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR_WATER ) ||
           terrain.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ||
           terrain.has_flag( ter_furn_flag::TFLAG_TRANSPARENT_FLOOR );
}

void map::set_transparency_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_cache( zlev ).transparency_cache_dirty.set();
        scene_build_pending = true;
        set_lightmap_cache_dirty_below( zlev );
        bump_geometry_revision( zlev );
    }
}

void map::set_transparency_cache_dirty( const tripoint_bub_ms &p, bool field )
{
    if( inbounds( p ) ) {
        const tripoint_bub_sm smp = coords::project_to<coords::sm>( p );
        get_cache( smp.z() ).transparency_cache_dirty.set( smp.x() * MAPSIZE + smp.y() );
        scene_build_pending = true;
        set_lightmap_cache_dirty_below( smp.z() );
        if( !field ) {
            get_creature_tracker().invalidate_reachability_cache();
            bump_geometry_revision( smp.z() );
        }
    }
}

void map::set_outside_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_cache( zlev ).outside_cache_dirty = true;
        scene_build_pending = true;
        set_lightmap_cache_dirty_below( zlev );
    }
}

void map::set_floor_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_cache( zlev ).floor_cache_dirty = true;
        scene_build_pending = true;
        set_lightmap_cache_dirty_below( zlev );
        bump_geometry_revision( zlev );
    }
}

void map::bump_geometry_revision( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_cache( zlev ).geometry_revision = next_cache_generation();
        last_scene_change = get_cache( zlev ).geometry_revision;
    }
}

bool map::has_floor( const tripoint_bub_ms &p ) const
{
    if( !zlevels || p.z() < -OVERMAP_DEPTH + 1 || p.z() > OVERMAP_HEIGHT ) {
        return true;
    }
    if( !inbounds( p ) ) {
        return true;
    }
    return !has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) &&
           !has_flag( ter_furn_flag::TFLAG_NO_FLOOR_WATER, p );
}

bool map::has_floor_or_water( const tripoint_bub_ms &p ) const
{
    if( !zlevels || p.z() < -OVERMAP_DEPTH + 1 || p.z() > OVERMAP_HEIGHT ) {
        return true;
    }
    if( !inbounds( p ) ) {
        return true;
    }
    return !has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p );
}

bool map::supports_above( const tripoint_bub_ms &p ) const
{
    const const_maptile tile = maptile_at( p );
    const ter_t &ter = tile.get_ter_t();
    if( ter.movecost == 0 ) {
        return true;
    }

    const furn_id &frn_id = tile.get_furn();
    if( frn_id != furn_str_id::NULL_ID() ) {
        const furn_t &frn = frn_id.obj();
        if( frn.movecost < 0 ) {
            return true;
        }
    }

    return veh_at( p ).has_value();
}

bool map::has_floor_or_support( const tripoint_bub_ms &p ) const
{
    const tripoint_bub_ms below( p.xy(), p.z() - 1 );
    return !valid_move( p, below, false, true );
}

bool map::has_vehicle_floor( const tripoint_bub_ms &p ) const
{
    return veh_at( p ).part_with_feature( "BOARDABLE", false ) ||
           veh_at( p ).part_with_feature( "OBSTACLE", false );
}

void map::drop_everything( const tripoint_bub_ms &p )
{
    // Creature has their own gravity check
    drop_creature( p );

    // TODO: Should be more nuance here, only low density items/furniture should float on water etc
    if( !has_floor_or_water( p ) ) {
        drop_furniture( p );
        drop_items( p );
        drop_vehicle( p );
        drop_fields( p );
    }
}

void map::drop_furniture( const tripoint_bub_ms &p )
{
    const furn_id &frn = furn( p );
    if( frn == furn_str_id::NULL_ID() || frn->has_flag( ter_furn_flag::TFLAG_FLOATS_IN_AIR ) ) {
        return;
    }

    enum support_state {
        SS_NO_SUPPORT = 0,
        SS_BAD_SUPPORT, // TODO: Implement bad, shaky support
        SS_GOOD_SUPPORT,
        SS_FLOOR, // Like good support, but bash floor instead of tile below
        SS_CREATURE
    };

    // Checks if the tile:
    // has floor (supports unconditionally)
    // has support below
    // has unsupporting furniture below (bad support, things should "slide" if possible)
    // has no support and thus allows things to fall through
    const auto check_tile = [this]( const tripoint_bub_ms & pt ) {
        if( has_floor_or_water( pt ) ) {
            return SS_FLOOR;
        }

        tripoint_bub_ms below_dest( pt.xy(), pt.z() - 1 );
        if( supports_above( below_dest ) ) {
            return SS_GOOD_SUPPORT;
        }

        const furn_id &frn_id = furn( below_dest );
        if( frn_id != furn_str_id::NULL_ID() ) {
            const furn_t &frn = frn_id.obj();
            // Allow crushing tiny/nocollide furniture
            if( !frn.has_flag( ter_furn_flag::TFLAG_TINY ) &&
                !frn.has_flag( ter_furn_flag::TFLAG_NOCOLLIDE ) ) {
                return SS_BAD_SUPPORT;
            }
        }

        if( get_creature_tracker().creature_at( below_dest ) != nullptr ) {
            // Smash a critter
            return SS_CREATURE;
        }

        return SS_NO_SUPPORT;
    };

    tripoint_bub_ms current( point_bub_ms( p.xy() ), p.z() + 1 );
    support_state last_state = SS_NO_SUPPORT;
    while( last_state == SS_NO_SUPPORT ) {
        current.z()--;
        // Check current tile
        last_state = check_tile( current );
    }

    if( current == p ) {
        // Nothing happened
        if( last_state != SS_FLOOR ) {
            support_dirty( current );
        }

        return;
    }

    furn_set( p, furn_str_id::NULL_ID() );
    furn_set( current, frn );

    // If it's sealed, we need to drop items with it
    const furn_t &frn_obj = frn.obj();
    if( frn_obj.has_flag( ter_furn_flag::TFLAG_SEALED ) && has_items( p ) ) {
        map_stack old_items = i_at( p );
        map_stack new_items = i_at( current );
        for( const item &it : old_items ) {
            new_items.insert( it );
        }

        i_clear( p );
    }

    // Approximate weight/"bulkiness" based on strength to drag
    int weight;
    if( frn_obj.has_flag( ter_furn_flag::TFLAG_TINY ) ||
        frn_obj.has_flag( ter_furn_flag::TFLAG_NOCOLLIDE ) ) {
        weight = 5;
    } else {
        weight = frn_obj.is_movable() ? frn_obj.move_str_req : 20;
    }

    if( frn_obj.has_flag( ter_furn_flag::TFLAG_ROUGH ) ||
        frn_obj.has_flag( ter_furn_flag::TFLAG_SHARP ) ) {
        weight += 5;
    }

    // TODO: Balance this.
    int dmg = weight * ( p.z() - current.z() );

    if( last_state == SS_FLOOR ) {
        // Bash the same tile twice - once for furniture, once for the floor
        bash( current, dmg, false, false, true );
        bash( current, dmg, false, false, true );
    } else if( last_state == SS_BAD_SUPPORT || last_state == SS_GOOD_SUPPORT ) {
        bash( current, dmg, false, false, false );
        tripoint_bub_ms below{ current + tripoint::below };
        bash( below, dmg, false, false, false );
    } else if( last_state == SS_CREATURE ) {
        const std::string &furn_name = frn_obj.name();
        bash( current, dmg, false, false, false );
        tripoint_bub_ms below{ current + tripoint::below };
        Creature *critter = get_creature_tracker().creature_at( below );
        if( critter == nullptr ) {
            debugmsg( "drop_furniture couldn't find creature at %d,%d,%d",
                      below.x(), below.y(), below.z() );
            return;
        }

        critter->add_msg_player_or_npc( m_bad, _( "Falling %s hits you!" ),
                                        _( "Falling %s hits <npcname>" ),
                                        furn_name );
        // TODO: A chance to dodge/uncanny dodge
        Character *pl = critter->as_character();
        monster *mon = critter->as_monster();
        if( pl != nullptr ) {
            // FIXME: Hardcoded damage types
            pl->deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_bash, rng( dmg / 3, dmg ),
                             0, 0.5f ) );
            pl->deal_damage( nullptr, bodypart_id( "head" ),  damage_instance( damage_bash, rng( dmg / 3, dmg ),
                             0, 0.5f ) );
            pl->deal_damage( nullptr, bodypart_id( "leg_l" ), damage_instance( damage_bash, rng( dmg / 2, dmg ),
                             0, 0.4f ) );
            pl->deal_damage( nullptr, bodypart_id( "leg_r" ), damage_instance( damage_bash, rng( dmg / 2, dmg ),
                             0, 0.4f ) );
            pl->deal_damage( nullptr, bodypart_id( "arm_l" ), damage_instance( damage_bash, rng( dmg / 2, dmg ),
                             0, 0.4f ) );
            pl->deal_damage( nullptr, bodypart_id( "arm_r" ), damage_instance( damage_bash, rng( dmg / 2, dmg ),
                             0, 0.4f ) );
        } else if( mon != nullptr ) {
            // TODO: Monster's armor and size - don't crush hulks with chairs
            mon->apply_damage( nullptr, bodypart_id( "torso" ), rng( dmg, dmg * 2 ) );
        }
    }

    // Re-queue for another check, in case bash destroyed something
    support_dirty( current );
}

void map::drop_items( const tripoint_bub_ms &p )
{
    if( !has_items( p ) ) {
        return;
    }

    map_stack items = i_at( p );
    // TODO: Make items check the volume tile below can accept
    // rather than disappearing if it would be overloaded

    tripoint_bub_ms below( p );
    int max_height_fallen = 0;
    while( !has_floor_or_water( below ) ) {
        below.z()--;
        max_height_fallen++;
    }

    if( below == p ) {
        return;
    }

    float damage_total = 0.0f;
    for( item &i : items ) {
        units::mass wt_dropped = i.weight();

        if( max_height_fallen <= 0 ) {
            debugmsg( "Tried to calculate damage for falling item, but item somehow fell less than one z-level!" );
            max_height_fallen = 1;
        }
        Creature *creature_below = get_creature_tracker().creature_at( below );
        double height_fallen = max_height_fallen;
        if( creature_below ) {
            // Discount most of the first z-level's falling distance, depending on how big the creature is
            // Characters (player/NPC) are normally medium, which is 0.5 or standing about 1.2m tall.
            // This is shorter than the average adult, but it's an *okay* approximation.
            height_fallen -= occupied_tile_fraction( creature_below->get_size() );
        }
        // in meters, assuming one z-level is ~2.5m.
        const double distance_to_fall = height_fallen * 2.5;

        // in meters per second (squared).
        const double gravity_acceleration_constant = 9.8;

        // in seconds.
        double falling_time = sqrt( 2 * distance_to_fall / gravity_acceleration_constant );

        // in meters per second.
        double velocity_at_impact = gravity_acceleration_constant * falling_time;

        // in joules.
        double impact_energy = to_kilogram( wt_dropped ) * std::pow( velocity_at_impact, 2.0 ) / 2.0;

        // in faces smashed (parts per hundred).
        double damage = sqrt( impact_energy );
        damage_total += damage;

        add_item_or_charges( below, i );

        // Bash creature standing below
        if( creature_below ) {
            // creature's dodge modifier
            float dodge_mod = creature_below->dodge_roll();

            // Most of the threat comes from the projectile going very fast before it enters a creature's vertical FOV
            // or e.g. it fell from so high up that there was no indication of something coming down before it (possibly) hits
            float hit_mod = velocity_at_impact / ( dodge_mod + 5 );

            int creature_hit_chance = rng( 0, 100 );
            double avoid_chance = hit_mod * occupied_tile_fraction( creature_below->get_size() );
            // We use sqrt here because it trends back towards 1. Spectacularly low hit_mod will still have SOME chance to hit while
            // hits of >1 avoid_chance are not always going to strike the head
            creature_hit_chance /= sqrt( avoid_chance );
            bodypart_id hit_part;
            if( creature_hit_chance < 15 ) {
                hit_part = creature_below->get_random_body_part_of_type( bp_type::head );
            } else if( creature_hit_chance < 30 ) {
                hit_part = creature_below->get_random_body_part_of_type( bp_type::torso );
            } else if( creature_hit_chance < 90 ) {
                hit_part = creature_below->get_random_body_part_of_type( bp_type::arm );
            } else if( creature_hit_chance < 100 ) {
                hit_part = creature_below->get_random_body_part_of_type( bp_type::leg );
            } else {
                add_msg_if_player_sees( creature_below->pos_bub(), _( "Falling %1$s misses %2$s!" ), i.tname(),
                                        creature_below->disp_name() );
            }
            // Did we hit at all? Then run the message.
            if( creature_hit_chance < 100 ) {
                if( hit_part.is_valid() && !creature_below->is_monster() ) {
                    //~First positional argument: Item name. Second: Name of a person (e.g. "Jane") or player (e.g. "you"). Third: Body part name, accusative.
                    const std::string msg = _( "Falling %1$s hits %2$s on the %3$s for %4$i damage!" );
                    add_msg_if_player_sees( creature_below->pos_bub(), msg,
                                            i.tname(), creature_below->disp_name(), hit_part->accusative, static_cast<int>( damage ) );
                } else {
                    add_msg_if_player_sees( creature_below->pos_bub(), _( "Falling %1$s hits %2$s for %3$i damage!" ),
                                            i.tname(), creature_below->disp_name(), static_cast<int>( damage ) );
                }
                // FIXME: Hardcoded damage type!
                creature_below->deal_damage( nullptr, hit_part, damage_instance( damage_bash, damage ) );
            }
        }

        // Bash items at bottom since currently bash_items only bash glass items
        // FIXME: Hardcoded damage type!
        int chance = static_cast<int>( 200 * i.resist( damage_bash, true ) / damage + 1 );
        if( one_in( chance ) ) {
            i.inc_damage();
        }
    }

    // Bash terain, furniture and vehicles on tile below
    bash( below, damage_total / 2 );
    i_clear( p );
}

void map::drop_vehicle( const tripoint_bub_ms &p )
{
    const optional_vpart_position vp = veh_at( p );
    if( !vp ) {
        return;
    }
    vp->vehicle().is_falling = true;
    set_seen_cache_dirty( p );
}

void map::drop_fields( const tripoint_bub_ms &p )
{
    field &fld = field_at( p );
    if( fld.field_count() == 0 ) {
        return;
    }

    const tripoint_bub_ms below = p + tripoint::below;
    for( const auto &iter : fld ) {
        const field_entry &entry = iter.second;
        // For now only drop cosmetic fields, which don't warrant per-turn check
        // Active fields "drop themselves"
        if( entry.get_field_type()->accelerated_decay ) {
            add_field( below, entry.get_field_type(), entry.get_field_intensity(), entry.get_field_age() );
            remove_field( p, entry.get_field_type() );
        }
    }
}

void map::drop_creature( const tripoint_bub_ms &p )
{
    monster *mon_at_p = get_creature_tracker().creature_at<monster>( p );
    if( mon_at_p ) {
        mon_at_p->gravity_check( this );
        // Handle character potentially standing on monster ("zed walking")
        drop_creature( p + tripoint_rel_ms::above );
        return;
    }
    Character *char_at_p = get_creature_tracker().creature_at<Character>( p );
    if( char_at_p ) {
        char_at_p->gravity_check( this );
    }
}

void map::support_dirty( const tripoint_bub_ms &p )
{
    if( zlevels ) {
        support_cache_dirty.insert( p );
    }
}

void map::process_falling()
{
    cata_timer process_falling_timer( "process_falling" );
    if( !zlevels ) {
        support_cache_dirty.clear();
        return;
    }

    if( !support_cache_dirty.empty() ) {
        add_msg_debug( debugmode::DF_MAP, "Checking %d tiles for falling objects",
                       support_cache_dirty.size() );
        // We want the cache to stay constant, but falling can change it
        std::set<tripoint_bub_ms> last_cache = std::move( support_cache_dirty );
        support_cache_dirty.clear();
        for( const tripoint_bub_ms &p : last_cache ) {
            drop_everything( p );
        }
    }
}

bool map::is_outside( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return true;
    }

    const auto &outside_cache = get_cache_ref( p.z() ).outside_cache;
    return outside_cache[p.x()][p.y()];
}

bool map::is_roofed( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    // Walk upward through z-levels looking for any surface that blocks
    // precipitation. The first blocking surface found means this tile is roofed.
    for( int z = p.z() + 1; z <= OVERMAP_HEIGHT; ++z ) {
        const tripoint_bub_ms check_pos( p.x(), p.y(), z );
        const level_cache *cache = get_cache_lazy( z );

        // Fast path: use floor_cache when it exists and is clean
        if( cache && !cache->floor_cache_dirty ) {
            if( cache->floor_cache[p.x()][p.y()] ) {
                return true;
            }
            // Transparent floors (glass, ramps, grates) are marked as "no floor"
            // in floor_cache but are physical surfaces that block precipitation
            if( has_flag_ter( ter_furn_flag::TFLAG_TRANSPARENT_FLOOR, check_pos ) ) {
                return true;
            }
            // Water surfaces absorb precipitation before it reaches tiles below
            if( has_flag_ter( ter_furn_flag::TFLAG_NO_FLOOR_WATER, check_pos ) ) {
                return true;
            }
            continue;
        }

        // Fallback: direct terrain/furniture/vehicle checks when cache is
        // missing or dirty. Mirrors build_floor_cache semantics.

        // TRANSPARENT_FLOOR checked first: blocks precipitation even when
        // combined with vertical-transition flags like GOES_DOWN
        if( has_flag_ter( ter_furn_flag::TFLAG_TRANSPARENT_FLOOR, check_pos ) ) {
            return true;
        }

        const bool no_floor = has_flag_ter( ter_furn_flag::TFLAG_NO_FLOOR, check_pos );
        const bool goes_down = has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, check_pos );

        if( !no_floor && !goes_down ) {
            return true;
        }

        if( has_flag_ter( ter_furn_flag::TFLAG_NO_FLOOR_WATER, check_pos ) ) {
            return true;
        }

        // SUN_ROOF_ABOVE on furniture at z-1 overrides no-floor at z
        // (matching build_floor_cache which checks below_submap furniture)
        const tripoint_bub_ms below_pos( p.x(), p.y(), z - 1 );
        if( inbounds( below_pos ) &&
            has_flag_furn( ter_furn_flag::TFLAG_SUN_ROOF_ABOVE, below_pos ) ) {
            return true;
        }

        // Check for vehicle roof/opaque parts at this level
        const optional_vpart_position vp = veh_at( check_pos );
        if( vp ) {
            if( vp->part_with_feature( VPFLAG_ROOF, false ) ||
                vp->part_with_feature( VPFLAG_OPAQUE, false ) ) {
                return true;
            }
        }
    }

    return false;
}

bool map::is_last_ter_wall( const bool no_furn, const point_bub_ms &p,
                            const point_bub_ms &max, const direction dir ) const
{
    point mov;
    switch( dir ) {
        case direction::NORTH:
            mov.y = -1;
            break;
        case direction::SOUTH:
            mov.y = 1;
            break;
        case direction::WEST:
            mov.x = -1;
            break;
        case direction::EAST:
            mov.x = 1;
            break;
        default:
            break;
    }
    point_bub_ms p2( p );
    bool result = true;
    bool loop = true;
    while( loop && ( ( dir == direction::NORTH && p2.y() >= 0 ) ||
                     ( dir == direction::SOUTH && p2.y() < max.y() ) ||
                     ( dir == direction::WEST  && p2.x() >= 0 ) ||
                     ( dir == direction::EAST  && p2.x() < max.x() ) ) ) {
        if( no_furn && has_furn( p2 ) ) {
            loop = false;
            result = false;
        } else if( !has_flag_ter( ter_furn_flag::TFLAG_FLAT, p2 ) ) {
            loop = false;
            if( !has_flag_ter( ter_furn_flag::TFLAG_WALL, p2 ) ) {
                result = false;
            }
        }
        p2.x() += mov.x;
        p2.y() += mov.y;
    }
    return result;
}

point_bub_ms map::random_outdoor_tile() const
{
    std::vector<point_bub_ms> options;
    for( const tripoint_bub_ms &p : points_on_zlevel() ) {
        if( is_outside( { p.xy(), abs_sub.z() } ) ) {
            const point_bub_ms temp{ p.xy() };
            options.emplace_back( temp );
        }
    }
    return random_entry( options, point_bub_ms::zero );
}

bool map::has_adjacent_furniture_with( const tripoint_bub_ms &p,
                                       const std::function<bool( const furn_t & )> &filter ) const
{
    for( const tripoint_bub_ms &adj : points_in_radius( p, 1 ) ) {
        if( has_furn( adj ) && filter( furn( adj ).obj() ) ) {
            return true;
        }
    }

    return false;
}

int map::collapse_check( const tripoint_bub_ms &p ) const
{
    const bool collapses = has_flag( ter_furn_flag::TFLAG_COLLAPSES, p );
    const bool supports_roof = has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, p );

    if( has_flag( ter_furn_flag::TFLAG_SINGLE_SUPPORT, p ) ) {
        // We should definitely collapse if there's no support below, and
        // given that a single wall as a support below isn't sufficient, a tree
        // is deemed not to be sufficient either.
        return 0;
    }

    int num_supports = p.z() == OVERMAP_DEPTH ? 0 : -5;
    // if there's support below, things are less likely to collapse
    if( p.z() > -OVERMAP_DEPTH ) {
        const tripoint_bub_ms &pbelow = p + tripoint::below;
        for( const tripoint_bub_ms &tbelow : points_in_radius( pbelow, 1 ) ) {
            if( has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, tbelow ) ) {
                num_supports += 1;
                if( has_flag( ter_furn_flag::TFLAG_WALL, tbelow ) ) {
                    num_supports += 2;
                }
                if( tbelow == pbelow ) {
                    num_supports += 2;
                }
            }
        }
    }

    for( const tripoint_bub_ms &t : points_in_radius( p, 1 ) ) {
        if( p == t ) {
            continue;
        }

        if( collapses ) {
            if( has_flag( ter_furn_flag::TFLAG_COLLAPSES, t ) ) {
                num_supports++;
            } else if( has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, t ) ) {
                num_supports += 2;
            }
        } else if( supports_roof ) {
            if( has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, t ) ) {
                if( has_flag( ter_furn_flag::TFLAG_WALL, t ) ) {
                    num_supports += 4;
                } else if( !has_flag( ter_furn_flag::TFLAG_COLLAPSES, t ) ) {
                    num_supports += 3;
                }
            }
        }
    }

    return 1.7 * num_supports;
}

// there is still some odd behavior here and there and you can get floating chunks of
// unsupported floor, but this is much better than it used to be
void map::collapse_at( const tripoint_bub_ms &p, const bool silent, const bool was_supporting,
                       const bool destroy_pos )
{
    const bool supports = was_supporting || has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, p );
    const bool wall_tree = was_supporting || has_flag( ter_furn_flag::TFLAG_WALL, p ) ||
                           has_flag( ter_furn_flag::TFLAG_TREE, p );
    // don't bash again if the caller already bashed here
    if( destroy_pos ) {
        destroy( p, silent );
        crush( p );
        make_rubble( p );
    }
    const bool still_supports = has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, p );

    // If something supporting the roof collapsed, see what else collapses
    if( supports && !still_supports ) {
        for( const tripoint_bub_ms &t : points_in_radius( p, 1 ) ) {
            // If z-levels are off, tz == t, so we end up skipping a lot of stuff to avoid bugs.
            const tripoint_bub_ms &tz = t + tripoint::above;
            const bool collapse_above = one_in( collapse_check( tz ) );

            if( zlevels && wall_tree && collapse_above ) {
                collapse_at( tz, silent );

                // this tile used to support a roof, now it doesn't, which means there is only
                // open air above us
                furn_set( tz, furn_str_id::NULL_ID() );
                ter_set( tz, ter_t_open_air );
                Creature *critter = get_creature_tracker().creature_at( tz );
                if( critter != nullptr ) {
                    creature_on_trap( *critter );
                }
            }
        }
    }
    // it would be great to check if collapsing ceilings smashed through the floor, but
    // that's not handled for now
}

ter_str_id map::get_roof( const tripoint_bub_ms &p, const bool allow_air ) const
{
    // This function should not be called from the 2D mode
    // Just use t_dirt instead
    cata_assert( zlevels );

    if( p.z() <= -OVERMAP_DEPTH ) {
        // Could be magma/"void" instead
        return ter_t_rock_floor;
    }

    const ter_t &ter_there = ter( p ).obj();
    const ter_str_id &roof = ter_there.roof;
    if( !roof ) {
        // No roof
        // Not acceptable if the tile is not passable
        if( !allow_air ) {
            return ter_t_dirt;
        }

        return ter_t_open_air;
    }
    if( roof == ter_str_id::NULL_ID() ) {
        debugmsg( "map::get_new_floor: %d,%d,%d has invalid roof type %s",
                  p.x(), p.y(), p.z(), roof.c_str() );
        return ter_t_dirt;
    }

    if( p.z() == -1 && roof == ter_t_rock_floor ) {
        // HACK: A hack to work around not having a "solid earth" tile
        return ter_t_dirt;
    }

    return roof;
}

void map::build_outside_cache( const int zlev )
{
    auto *ch_lazy = get_cache_lazy( zlev );
    if( !ch_lazy || !ch_lazy->outside_cache_dirty ) {
        return;
    }
    level_cache &ch = *ch_lazy;

    // Make a bigger cache to avoid bounds checking
    // We will later copy it to our regular cache
    const size_t padded_w = MAPSIZE_X + 2;
    const size_t padded_h = MAPSIZE_Y + 2;
    cata::mdarray<bool, point_bub_ms, padded_w, padded_h> padded_cache;

    auto &outside_cache = ch.outside_cache;
    ch.outside_rewritten = true;
    if( zlev < 0 ) {
        std::uninitialized_fill_n(
            &outside_cache[0][0], MAPSIZE_X * MAPSIZE_Y, false );
        ch.outside_cache_dirty = false;
        return;
    }

    padded_cache.fill( true );

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{ smx, smy, zlev } );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build outside cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          zlev );
                continue;
            }

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    point_sm_ms sp( sx, sy );
                    if( cur_submap->get_ter( sp ).obj().has_flag( ter_furn_flag::TFLAG_INDOORS ) ||
                        cur_submap->get_furn( sp ).obj().has_flag( ter_furn_flag::TFLAG_INDOORS ) ) {
                        const point p( sx + smx * SEEX, sy + smy * SEEY );
                        // Add 1 to both coordinates, because we're operating on the padded cache
                        for( int dx = 0; dx <= 2; dx++ ) {
                            for( int dy = 0; dy <= 2; dy++ ) {
                                padded_cache[p.x + dx][p.y + dy] = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // Copy the padded cache back to the proper one, but with no padding
    for( int x = 0; x < SEEX * my_MAPSIZE; x++ ) {
        std::copy_n( &padded_cache[x + 1][1], SEEX * my_MAPSIZE, &outside_cache[x][0] );
    }

    ch.outside_cache_dirty = false;
}

void map::build_obstacle_cache(
    const tripoint_bub_ms &start, const tripoint_bub_ms &end,
    cata::mdarray<fragment_cloud, point_bub_ms> &obstacle_cache )
{
    const point_sm_ms min_submap{ std::max( 0, start.x() / SEEX ), std::max( 0, start.y() / SEEY ) };
    const point_sm_ms max_submap{
        std::min( my_MAPSIZE - 1, end.x() / SEEX ), std::min( my_MAPSIZE - 1, end.y() / SEEY ) };
    // Find and cache all the map obstacles.
    // For now setting obstacles to be extremely dense and fill their squares.
    // In future, scale effective obstacle density by the thickness of the obstacle.
    // Also consider modelling partial obstacles.
    // TODO: Support z-levels.
    for( int smx = min_submap.x(); smx <= max_submap.x(); ++smx ) {
        for( int smy = min_submap.y(); smy <= max_submap.y(); ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{ smx, smy, start.z()} );
            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build obstacle cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          start.z() );
                continue;
            }

            // TODO: Init indices to prevent iterating over unused submap sections.
            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const point_sm_ms sp( sx, sy );
                    int ter_move = cur_submap->get_ter( sp ).obj().movecost;
                    int furn_move = cur_submap->get_furn( sp ).obj().movecost;
                    const point p2( sx + smx * SEEX, sy + smy * SEEY );
                    if( ter_move == 0 || furn_move < 0 || ter_move + furn_move == 0 ) {
                        obstacle_cache[p2.x][p2.y].velocity = 1000.0f;
                        obstacle_cache[p2.x][p2.y].density = 0.0f;
                    } else {
                        // Magic number warning, this is the density of air at sea level at
                        // some nominal temp and humidity.
                        // TODO: figure out if our temp/altitude/humidity variation is
                        // sufficient to bother setting this differently.
                        obstacle_cache[p2.x][p2.y].velocity = 1.2f;
                        obstacle_cache[p2.x][p2.y].density = 1.0f;
                    }
                }
            }
        }
    }
    VehicleList vehs = get_vehicles( start, end );
    const inclusive_cuboid<tripoint_bub_ms> bounds( start, end );
    // Cache all the vehicle stuff in one loop
    for( wrapped_vehicle &v : vehs ) {
        for( const vpart_reference &vp : v.v->get_all_parts() ) {
            tripoint_bub_ms p { v.pos + vp.part().precalc[0].raw()};
            if( p.z() != start.z() ) {
                break;
            }
            if( !bounds.contains( p ) ) {
                continue;
            }

            if( vp.obstacle_at_part() ) {
                obstacle_cache[p.x()][p.y()].velocity = 1000.0f;
                obstacle_cache[p.x()][p.y()].density = 0.0f;
            }
        }
    }
    // Iterate over creatures and set them to block their squares relative to their size.
    for( Creature &critter : g->all_creatures() ) {
        const tripoint_bub_ms loc = critter.pos_bub( *this );
        if( loc.z() != start.z() || !inbounds( loc ) ) {
            continue;
        }
        // TODO: scale this with expected creature "thickness".
        obstacle_cache[loc.x()][loc.y()].velocity = 1.2f;
        // ranged_target_size is "proportion of square that is blocked", and density needs to be
        // "transmissivity of square", so we need the reciprocal.
        obstacle_cache[loc.x()][loc.y()].density = 1.0 - critter.ranged_target_size();
    }
}

// If this ever shows up on profiling, maybe prepopulate one of these in the map cache for each level.
std::bitset<OVERMAP_LAYERS> map::get_inter_level_visibility( const int origin_zlevel ) const
{
    std::bitset<OVERMAP_LAYERS> seen_levels;
    seen_levels.set( origin_zlevel + OVERMAP_DEPTH );
    for( int z = origin_zlevel + 1; z <= OVERMAP_HEIGHT; ++z ) {
        if( get_cache_ref( z ).no_floor_gaps ) {
            break;
        } else {
            seen_levels.set( z + OVERMAP_DEPTH );
        }
    }
    for( int z = origin_zlevel; z > -OVERMAP_DEPTH; --z ) {
        if( get_cache_ref( z ).no_floor_gaps ) {
            break;
        } else {
            // No floor means we can see the *lower* level.
            seen_levels.set( z - 1 + OVERMAP_DEPTH );
        }
    }
    return seen_levels;
}

void map::build_floor_cache( const int zlev )
{
    auto *ch_lazy = get_cache_lazy( zlev );
    if( !ch_lazy || !ch_lazy->floor_cache_dirty ) {
        return;
    }
    level_cache &ch = *ch_lazy;

    auto &floor_cache = ch.floor_cache;
    std::uninitialized_fill_n(
        &floor_cache[0][0], MAPSIZE_X * MAPSIZE_Y, true );
    bool &no_floor_gaps = ch.no_floor_gaps;
    no_floor_gaps = true;

    bool lowest_z_lev = zlev <= -OVERMAP_DEPTH;

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const submap *cur_submap = get_submap_at_grid( tripoint_rel_sm{ smx, smy, zlev } );
            const submap *below_submap = !lowest_z_lev ? get_submap_at_grid( tripoint_rel_sm{ smx, smy, zlev - 1 } ) :
                                         nullptr;

            if( cur_submap == nullptr ) {
                debugmsg( "Tried to build floor cache at (%d,%d,%d) but the submap is not loaded", smx, smy, zlev );
                continue;
            }
            if( !lowest_z_lev && below_submap == nullptr ) {
                debugmsg( "Tried to build floor cache at (%d,%d,%d) but the submap is not loaded", smx, smy,
                          zlev - 1 );
                continue;
            }

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    point_sm_ms sp( sx, sy );
                    const ter_t &terrain = cur_submap->get_ter( sp ).obj();
                    if( has_floor_gap_flag( terrain ) ) {
                        if( below_submap &&
                            below_submap->get_furn( sp ).obj().has_flag( ter_furn_flag::TFLAG_SUN_ROOF_ABOVE ) ) {
                            continue;
                        }
                        const point p( sx + smx * SEEX, sy + smy * SEEY );
                        floor_cache[p.x][p.y] = false;
                        no_floor_gaps = false;
                    }
                }
            }
        }
    }

    ch.floor_cache_dirty = false;
    // a floor gates every view across it, wherever a cast reached, so the
    // next cache build recasts however this rebuild was reached
    if( zlevels ) {
        ch.seen_cache_dirty = true;
    }
}

void map::build_floor_caches()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int z = minz; z <= maxz; z++ ) {
        build_floor_cache( z );
    }
}

// TODO: Should be moved to submap or Creature?
bool map::try_fall( const tripoint_bub_ms &p, Creature *c )
{
    if( c == nullptr ) {
        return false;
    }

    if( c->has_effect_with_flag( json_flag_LEVITATION ) && !c->has_effect( effect_slow_descent )
        && !c->has_effect( effect_stunned ) && !c->has_effect( effect_psi_stunned ) ) {
        return false;
    }

    int height = 0;
    tripoint_bub_ms where( p );
    tripoint_bub_ms below( where + tripoint_rel_ms::below );
    creature_tracker &creatures = get_creature_tracker();
    while( valid_move( where, below, false, true ) ) {
        where.z()--;
        if( get_creature_tracker().creature_at( where ) != nullptr ) {
            where.z()++;
            break;
        }

        below.z()--;
        height++;
    }

    if( height == 0 && c->is_avatar() ) {
        // For now just special case player, NPCs don't "zedwalk"
        Creature *critter = creatures.creature_at( below, true );
        if( critter == nullptr || !critter->is_monster() ) {
            return false;
        }

        std::vector<tripoint_bub_ms> valid;
        for( const tripoint_bub_ms &pt : points_in_radius( below, 1 ) ) {
            if( g->is_empty( pt ) ) {
                valid.push_back( pt );
            }
        }

        if( valid.empty() ) {
            critter->setpos( c->pos_abs() );
            add_msg( m_bad, _( "You fall down under %s!" ), critter->disp_name() );
        } else {
            critter->setpos( *this, random_entry( valid ) );
        }

        height++;
        where.z()--;
    } else if( height == 0 ) {
        return false;
    }

    c->add_msg_if_npc( _( "<npcname> falls down a level!" ) );
    Character *you = dynamic_cast<Character *>( c );
    if( you == nullptr ) {
        c->setpos( *this, where );
        if( c->get_size() == creature_size::tiny ) {
            height = std::max( 0, height - 1 );
        }
        if( c->has_effect( effect_weakened_gravity ) ) {
            height = std::max( 0, height - 1 );
        }
        if( c->has_effect( effect_strengthened_gravity ) ) {
            height += 1;
        }
        c->impact( height * 10, where );
        return true;
    }

    if( you->has_flag( json_flag_WALL_CLING ) &&  this->is_wall_adjacent( p ) ) {
        you->add_msg_player_or_npc( _( "You attach yourself to the nearby wall." ),
                                    _( "<npcname> clings to the wall." ) );
        return false;
    }

    if( you->is_avatar() ) {
        add_msg( m_bad, n_gettext( "You fall down %d story!", "You fall down %d stories!", height ),
                 height );
        g->vertical_move( -height, true );
    } else {
        you->setpos( *this, where );
    }

    if( you->get_size() == creature_size::tiny ) {
        height = std::max( 0, height - 1 );
    }

    if( you->has_flag( json_flag_ONE_STORY_FALL ) ) {
        height = std::max( 0, height - 1 );
    }

    if( you->has_effect( effect_weakened_gravity ) ) {
        height = std::max( 0, height - 1 );
    }

    if( you->has_effect( effect_strengthened_gravity ) ) {
        height += 1;
    }

    if( you->can_fly() ) {
        you->add_msg_player_or_npc( _( "You spread your wings to slow your fall." ),
                                    _( "<npcname> spreads their wings to slow their fall." ) );
        height = std::max( 0, height - 2 );
    }

    item jetpack = you->item_worn_with_flag( json_flag_JETPACK );

    if( you->has_active_bionic( bio_shock_absorber ) ) {
        you->add_msg_if_player( m_info,
                                _( "You hit the ground hard, but your grav chute handles the impact admirably!" ) );
    } else if( !jetpack.is_null() ) {
        if( jetpack.ammo_sufficient( you ) ) {
            you->add_msg_player_or_npc( _( "You ignite your %s and use it to break the fall." ),
                                        _( "<npcname> uses their %s to break the fall." ), jetpack.tname() );
            jetpack.activation_consume( 1, you->pos_bub(), you );
        } else {
            you->add_msg_if_player( m_bad,
                                    _( "You attempt to break the fall with your %s but it is out of fuel!" ), jetpack.tname() );
            you->impact( height * 30, where );

        }
    } else {
        you->impact( height * 30, where );
    }

    if( has_flag( ter_furn_flag::TFLAG_DEEP_WATER, where ) ) {
        you->set_underwater( true );
        g->water_affect_items( *you );
        you->add_msg_player_or_npc( _( "You dive into water." ), _( "<npcname> dives into water." ) );
    }

    return true;
}
