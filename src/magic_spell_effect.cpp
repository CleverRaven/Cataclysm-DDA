#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "game.h"
#include "item.h"
#include "line.h"
#include "magic.h"
#include "magic_spell_effect_helpers.h"
#include "magic_teleporter_list.h"
#include "magic_ter_furn_transform.h"
#include "monstergenerator.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "optional.h"
#include "overmapbuffer.h"
#include "player.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "rng.h"
#include "string_id.h"
#include "teleport.h"
#include "timed_event.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const mtype_id mon_generator( "mon_generator" );

namespace spell_detail
{
struct line_iterable {
    const std::vector<point> &delta_line;
    point cur_origin;
    point delta;
    size_t index;

    line_iterable( const point &origin, const point &delta, const std::vector<point> &dline )
        : delta_line( dline ), cur_origin( origin ), delta( delta ), index( 0 ) {}

    point get() const {
        return cur_origin + delta_line[index];
    }
    // Move forward along point set, wrap around and move origin forward if necessary
    void next() {
        index = ( index + 1 ) % delta_line.size();
        cur_origin = cur_origin + delta * ( index == 0 );
    }
    // Move back along point set, wrap around and move origin backward if necessary
    void prev() {
        cur_origin = cur_origin - delta * ( index == 0 );
        index = ( index + delta_line.size() - 1 ) % delta_line.size();
    }
    void reset( const point &origin ) {
        cur_origin = origin;
        index = 0;
    }
};
// Orientation of point C relative to line AB
static int side_of( const point &a, const point &b, const point &c )
{
    int cross = ( ( b.x - a.x ) * ( c.y - a.y ) - ( b.y - a.y ) * ( c.x - a.x ) );
    return ( cross > 0 ) - ( cross < 0 );
}
// Tests if point c is between or on lines (a0, a0 + d) and (a1, a1 + d)
static bool between_or_on( const point &a0, const point &a1, const point &d, const point &c )
{
    return side_of( a0, a0 + d, c ) != 1 && side_of( a1, a1 + d, c ) != -1;
}
// Builds line until obstructed or outside of region bound by near and far lines. Stores result in set
static void build_line( spell_detail::line_iterable line, const tripoint &source,
                        const point &delta, const point &delta_perp, bool ( *test )( const tripoint & ),
                        std::set<tripoint> &result )
{
    while( between_or_on( point_zero, delta, delta_perp, line.get() ) ) {
        if( !test( source + line.get() ) ) {
            break;
        }
        result.emplace( source + line.get() );
        line.next();
    }
}
} // namespace spell_detail

void spell_effect::teleport_random( const spell &sp, Creature &caster, const tripoint & )
{
    bool safe = !sp.has_flag( spell_flag::UNSAFE_TELEPORT );
    const int min_distance = sp.range();
    const int max_distance = sp.range() + sp.aoe();
    if( min_distance > max_distance || min_distance < 0 || max_distance < 0 ) {
        debugmsg( "ERROR: Teleport argument(s) invalid" );
        return;
    }
    teleport::teleport( caster, min_distance, max_distance, safe, false );
}

static void swap_pos( Creature &caster, const tripoint &target )
{
    Creature *const critter = g->critter_at<Creature>( target );
    critter->setpos( caster.pos() );
    caster.setpos( target );
    //update map in case a monster swapped positions with the player
    g->update_map( g->u );
}

void spell_effect::pain_split( const spell &sp, Creature &caster, const tripoint & )
{
    player *p = caster.as_player();
    if( p == nullptr ) {
        return;
    }
    sp.make_sound( caster.pos() );
    add_msg( m_info, _( "Your injuries even out." ) );
    int num_limbs = 0; // number of limbs effected (broken don't count)
    int total_hp = 0; // total hp among limbs

    for( const std::pair<const bodypart_str_id, bodypart> &elem : p->get_body() ) {
        if( elem.first == bodypart_str_id( "num_bp" ) ) {
            continue;
        }
        num_limbs++;
        total_hp += elem.second.get_hp_cur();
    }
    const int hp_each = total_hp / num_limbs;
    p->set_all_parts_hp_cur( hp_each );
}

static bool in_spell_aoe( const tripoint &start, const tripoint &end, const int &radius,
                          const bool ignore_walls )
{
    if( rl_dist( start, end ) > radius ) {
        return false;
    }
    if( ignore_walls ) {
        return true;
    }
    const std::vector<tripoint> trajectory = line_to( start, end );
    for( const tripoint &pt : trajectory ) {
        if( g->m.impassable( pt ) ) {
            return false;
        }
    }
    return true;
}

std::set<tripoint> spell_effect::spell_effect_blast( const spell &, const tripoint &,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // TODO: Make this breadth-first
    for( const tripoint &potential_target : g->m.points_in_radius( target, aoe_radius ) ) {
        if( in_spell_aoe( target, potential_target, aoe_radius, ignore_walls ) ) {
            targets.emplace( potential_target );
        }
    }
    return targets;
}

std::set<tripoint> spell_effect::spell_effect_cone( const spell &sp, const tripoint &source,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    // cones go all the way to end (if they don't hit an obstacle)
    const int range = sp.range() + 1;
    const int initial_angle = coord_to_angle( source, target );
    std::set<tripoint> end_points;
    for( int angle = initial_angle - std::floor( aoe_radius / 2.0 );
         angle <= initial_angle + std::ceil( aoe_radius / 2.0 ); angle++ ) {
        tripoint potential;
        calc_ray_end( angle, range, source, potential );
        end_points.emplace( potential );
    }
    for( const tripoint &ep : end_points ) {
        std::vector<tripoint> trajectory = line_to( source, ep );
        for( const tripoint &tp : trajectory ) {
            if( ignore_walls || g->m.passable( tp ) ) {
                targets.emplace( tp );
            } else {
                break;
            }
        }
    }
    // we don't want to hit ourselves in the blast!
    targets.erase( source );
    return targets;
}

static bool test_always_true( const tripoint & )
{
    return true;
}
static bool test_passable( const tripoint &p )
{
    return g->m.passable( p );
}

std::set<tripoint> spell_effect::spell_effect_line( const spell &, const tripoint &source,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    const point delta = ( target - source ).xy();
    const int dist = square_dist( point_zero, delta );
    // Early out to prevent unnecessary calculations
    if( dist == 0 ) {
        return std::set<tripoint>();
    }
    // Clockwise Perpendicular of Delta vector
    const point delta_perp( -delta.y, delta.x );

    const point abs_delta = delta.abs();
    // Primary axis of delta vector
    const point axis_delta = abs_delta.x > abs_delta.y ? point( delta.x, 0 ) : point( 0, delta.y );
    // Clockwise Perpendicular of axis vector
    const point cw_perp_axis( -axis_delta.y, axis_delta.x );
    const point unit_cw_perp_axis( sgn( cw_perp_axis.x ), sgn( cw_perp_axis.y ) );
    // bias leg length toward cw side if uneven
    int ccw_len = aoe_radius / 2;
    int cw_len = aoe_radius - ccw_len;

    if( !trigdist ) {
        ccw_len = ( ccw_len * ( abs_delta.x + abs_delta.y ) ) / dist;
        cw_len = ( cw_len * ( abs_delta.x + abs_delta.y ) ) / dist;
    }

    // is delta aligned with, cw, or ccw of primary axis
    int delta_side = spell_detail::side_of( point_zero, axis_delta, delta );

    bool ( *test )( const tripoint & ) = ignore_walls ? test_always_true : test_passable;

    // Canonical path from source to target, offset to local space
    std::vector<point> path_to_target = line_to( point_zero, delta );
    // Remove endpoint,
    path_to_target.pop_back();
    // and insert startpoint. Path is now prepared for wrapped iteration
    path_to_target.insert( path_to_target.begin(), point_zero );

    spell_detail::line_iterable base_line( point_zero, delta, path_to_target );

    std::set<tripoint> result;

    // Add midline points (source -> target )
    spell_detail::build_line( base_line, source, delta, delta_perp, test, result );

    // Add cw and ccw legs
    if( delta_side == 0 ) { // delta is already axis aligned, only need straight lines
        // cw leg
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * cw_len ) ) {
            base_line.reset( p );
            if( !test( source + p ) ) {
                break;
            }

            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
        // ccw leg
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * -ccw_len ) ) {
            base_line.reset( p );
            if( !test( source + p ) ) {
                break;
            }

            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
    } else if( delta_side == 1 ) { // delta is cw of primary axis
        // ccw leg is behind perp axis
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * -ccw_len ) ) {
            base_line.reset( p );

            // forward until in
            while( spell_detail::side_of( point_zero, delta_perp, base_line.get() ) == 1 ) {
                base_line.next();
            }
            if( !test( source + p ) ) {
                break;
            }
            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
        // cw leg is before perp axis
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * cw_len ) ) {
            base_line.reset( p );

            // move back
            while( spell_detail::side_of( point_zero, delta_perp, base_line.get() ) != 1 ) {
                base_line.prev();
            }
            base_line.next();
            if( !test( source + p ) ) {
                break;
            }
            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
    } else if( delta_side == -1 ) { // delta is ccw of primary axis
        // ccw leg is before perp axis
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * -ccw_len ) ) {
            base_line.reset( p );

            // move back
            while( spell_detail::side_of( point_zero, delta_perp, base_line.get() ) != 1 ) {
                base_line.prev();
            }
            base_line.next();
            if( !test( source + p ) ) {
                break;
            }
            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
        // cw leg is behind perp axis
        for( const point &p : line_to( point_zero, unit_cw_perp_axis * cw_len ) ) {
            base_line.reset( p );

            // forward until in
            while( spell_detail::side_of( point_zero, delta_perp, base_line.get() ) == 1 ) {
                base_line.next();
            }
            if( !test( source + p ) ) {
                break;
            }
            spell_detail::build_line( base_line, source, delta, delta_perp, test, result );
        }
    }

    result.erase( source );
    return result;
}

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
std::set<tripoint> calculate_spell_effect_area( const spell &sp, const tripoint &target,
        const std::function<std::set<tripoint>( const spell &, const tripoint &, const tripoint &, int, bool )>
        &aoe_func, const Creature &caster, bool ignore_walls )
{
    std::set<tripoint> targets = { target }; // initialize with epicenter
    if( sp.aoe() <= 1 && sp.effect() != "line_attack" ) {
        return targets;
    }

    const int aoe_radius = sp.aoe();
    targets = aoe_func( sp, caster.pos(), target, aoe_radius, ignore_walls );

    for( std::set<tripoint>::iterator it = targets.begin(); it != targets.end(); ) {
        if( !sp.is_valid_target( caster, *it ) ) {
            it = targets.erase( it );
        } else {
            ++it;
        }
    }

    return targets;
}

static std::set<tripoint> spell_effect_area( const spell &sp, const tripoint &target,
        const std::function<std::set<tripoint>( const spell &, const tripoint &, const tripoint &, int, bool )>
        &aoe_func, const Creature &caster, bool ignore_walls = false )
{
    // calculate spell's effect area
    std::set<tripoint> targets = calculate_spell_effect_area( sp, target, aoe_func, caster,
                                 ignore_walls );

    // Draw the explosion
    std::map<tripoint, nc_color> explosion_colors;
    for( auto &pt : targets ) {
        explosion_colors[pt] = sp.damage_type_color();
    }

    explosion_handler::draw_custom_explosion( g->u.pos(), explosion_colors );
    return targets;
}

static void add_effect_to_target( const tripoint &target, const spell &sp )
{
    const int dur_moves = sp.duration();
    const time_duration dur_td = 1_turns * dur_moves / 100;

    Creature *const critter = g->critter_at<Creature>( target );
    Character *const guy = g->critter_at<Character>( target );
    efftype_id spell_effect( sp.effect_data() );
    bool bodypart_effected = false;

    if( guy ) {
        for( const body_part bp : all_body_parts ) {
            if( sp.bp_is_affected( bp ) ) {
                guy->add_effect( spell_effect, dur_td, bp, sp.has_flag( spell_flag::PERMANENT ) );
                bodypart_effected = true;
            }
        }
    }
    if( !bodypart_effected ) {
        critter->add_effect( spell_effect, dur_td, num_bp );
    }
}

static void damage_targets( const spell &sp, Creature &caster,
                            const std::set<tripoint> &targets )
{
    for( const tripoint &target : targets ) {
        if( !sp.is_valid_target( caster, target ) ) {
            continue;
        }
        sp.make_sound( target );
        sp.create_field( target );
        if( sp.has_flag( spell_flag::IGNITE_FLAMMABLE ) && g->m.is_flammable( target ) ) {
            g->m.add_field( target, fd_fire, 1, 10_minutes );
        }
        Creature *const cr = g->critter_at<Creature>( target );
        if( !cr ) {
            continue;
        }

        projectile bolt;
        bolt.speed = 10000;
        bolt.impact = sp.get_damage_instance();
        bolt.proj_effects.emplace( "magic" );

        dealt_projectile_attack atk;
        atk.end_point = target;
        atk.hit_critter = cr;
        atk.proj = bolt;
        atk.missed_by = 0.0;
        if( !sp.effect_data().empty() ) {
            add_effect_to_target( target, sp );
        }
        if( sp.damage() > 0 ) {
            cr->deal_projectile_attack( &caster, atk, true );
        } else if( sp.damage() < 0 ) {
            sp.heal( target );
            add_msg( m_good, _( "%s wounds are closing up!" ), cr->disp_name( true ) );
        }
        // TODO: randomize hit location
        cr->add_damage_over_time( sp.damage_over_time( { bodypart_str_id( "torso" ) } ) );
    }
}

void spell_effect::projectile_attack( const spell &sp, Creature &caster,
                                      const tripoint &target )
{
    std::vector<tripoint> trajectory = line_to( caster.pos(), target );
    for( std::vector<tripoint>::iterator iter = trajectory.begin(); iter != trajectory.end(); iter++ ) {
        if( g->m.impassable( *iter ) ) {
            if( iter != trajectory.begin() ) {
                target_attack( sp, caster, *( iter - 1 ) );
            } else {
                target_attack( sp, caster, *iter );
            }
            return;
        }
    }
    target_attack( sp, caster, trajectory.back() );
}

void spell_effect::target_attack( const spell &sp, Creature &caster,
                                  const tripoint &epicenter )
{
    damage_targets( sp, caster, spell_effect_area( sp, epicenter, spell_effect_blast, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
    if( sp.has_flag( spell_flag::SWAP_POS ) ) {
        swap_pos( caster, epicenter );
    }
}

static void magical_polymorph( monster &victim, Creature &caster, const spell &sp )
{
    mtype_id new_id = mtype_id( sp.effect_data() );

    if( sp.has_flag( spell_flag::POLYMORPH_GROUP ) ) {
        const mongroup_id group_id( sp.effect_data() );
        new_id = MonsterGroupManager::GetRandomMonsterFromGroup( group_id );
    }

    // if effect_str is empty, we become a random monster of close difficulty
    if( new_id.is_empty() ) {
        int victim_diff = victim.type->difficulty;
        const std::vector<mtype> &mtypes = MonsterGenerator::generator().get_all_mtypes();
        for( int difficulty_variance = 1; difficulty_variance < 2048; difficulty_variance *= 2 ) {
            unsigned int random_entry = rng( 0, mtypes.size() );
            unsigned int iter = random_entry + 1;
            while( iter != random_entry && new_id.is_empty() ) {
                if( iter >= mtypes.size() ) {
                    iter = 0;
                }
                if( ( mtypes[iter].id != victim.type->id ) && ( std::abs( mtypes[iter].difficulty - victim_diff )
                        <= difficulty_variance ) ) {
                    if( !mtypes[iter].in_species( species_id( "HALLUCINATION" ) ) &&
                        mtypes[iter].id != mon_generator ) {
                        new_id = mtypes[iter].id;
                        break;
                    }
                }
                iter++;
            }
        }
    }

    if( !new_id.is_valid() ) {
        debugmsg( "magical_polymorph called with an invalid monster id" );
        return;
    }

    if( g->u.sees( victim ) ) {
        add_msg( _( "The %s transforms into a %s." ), victim.type->nname(),
                 new_id->nname() );
    }
    victim.poly( new_id );

    if( sp.has_flag( spell_flag::FRIENDLY_POLY ) ) {
        if( caster.as_player() ) {
            victim.friendly = -1;
        } else {
            victim.make_ally( *caster.as_monster() );
        }
    }
}

void spell_effect::targeted_polymorph( const spell &sp, Creature &caster, const tripoint &target )
{
    //we only target monsters for now.
    if( monster *const victim = g->critter_at<monster>( target ) ) {
        if( victim->get_hp() < sp.damage() ) {
            magical_polymorph( *victim, caster, sp );
            return;
        }
    }
    //victim had high hp, or isn't a monster.
    caster.add_msg_if_player( m_bad, _( "Your target resists transformation." ) );
}

void spell_effect::cone_attack( const spell &sp, Creature &caster,
                                const tripoint &target )
{
    damage_targets( sp, caster, spell_effect_area( sp, target, spell_effect_cone, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::line_attack( const spell &sp, Creature &caster,
                                const tripoint &target )
{
    damage_targets( sp, caster, spell_effect_area( sp, target, spell_effect_line, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

area_expander::area_expander() : frontier( area_node_comparator( area ) )
{
}

// Check whether we have already visited this node.
int area_expander::contains( const tripoint &pt ) const
{
    return area_search.count( pt ) > 0;
}

// Adds node to a search tree. Returns true if new node is allocated.
bool area_expander::enqueue( const tripoint &from, const tripoint &to, float cost )
{
    if( contains( to ) ) {
        // We will modify existing node if its cost is lower.
        int index = area_search[to];
        node &node = area[index];
        if( cost < node.cost ) {
            node.from = from;
            node.cost = cost;
        }
        return false;
    }
    int index = area.size();
    area.push_back( {to, from, cost} );
    frontier.push( index );
    area_search[to] = index;
    return true;
}

// Run wave propagation
int area_expander::run( const tripoint &center )
{
    enqueue( center, center, 0.0 );

    static constexpr std::array<int, 8> x_offset = {{ -1, 1,  0, 0,  1, -1, -1, 1  }};
    static constexpr std::array<int, 8> y_offset = {{  0, 0, -1, 1, -1,  1, -1, 1  }};

    // Number of nodes expanded.
    int expanded = 0;

    while( !frontier.empty() ) {
        int best_index = frontier.top();
        frontier.pop();
        node &best = area[best_index];

        for( size_t i = 0; i < 8; i++ ) {
            tripoint pt = best.position + point( x_offset[ i ], y_offset[ i ] );

            if( g->m.impassable( pt ) ) {
                continue;
            }

            float center_range = static_cast<float>( rl_dist( center, pt ) );
            if( max_range > 0 && center_range > max_range ) {
                continue;
            }

            if( max_expand > 0 && expanded > max_expand && contains( pt ) ) {
                continue;
            }

            float delta_range = trig_dist( best.position, pt );

            if( enqueue( best.position, pt, best.cost + delta_range ) ) {
                expanded++;
            }
        }
    }
    return expanded;
}

// Sort nodes by its cost.
void area_expander::sort_ascending()
{
    // Since internal caches like 'area_search' and 'frontier' use indexes inside 'area',
    // these caches will be invalidated.
    std::sort( area.begin(), area.end(),
    []( const node & a, const node & b )  -> bool {
        return a.cost < b.cost;
    } );
}

void area_expander::sort_descending()
{
    // Since internal caches like 'area_search' and 'frontier' use indexes inside 'area',
    // these caches will be invalidated.
    std::sort( area.begin(), area.end(),
    []( const node & a, const node & b ) -> bool {
        return a.cost > b.cost;
    } );
}

// Moving all objects from one point to another by the power of magic.
static void spell_move( const spell &sp, const Creature &caster,
                        const tripoint &from, const tripoint &to )
{
    if( from == to ) {
        return;
    }

    // Moving creatures
    bool can_target_creature = sp.is_valid_effect_target( spell_target::self ) ||
                               sp.is_valid_effect_target( spell_target::ally ) ||
                               sp.is_valid_effect_target( spell_target::hostile );

    if( can_target_creature ) {
        if( Creature *victim = g->critter_at<Creature>( from ) ) {
            Creature::Attitude cr_att = victim->attitude_to( g->u );
            bool valid = cr_att != Creature::Attitude::FRIENDLY &&
                         sp.is_valid_effect_target( spell_target::hostile );
            valid |= cr_att == Creature::Attitude::FRIENDLY && sp.is_valid_effect_target( spell_target::ally );
            valid |= victim == &caster && sp.is_valid_effect_target( spell_target::self );
            if( valid ) {
                victim->knock_back_to( to );
            }
        }
    }

    // Moving items
    if( sp.is_valid_effect_target( spell_target::item ) ) {
        map_stack src_items = g->m.i_at( from );
        map_stack dst_items = g->m.i_at( to );
        for( const item &item : src_items ) {
            dst_items.insert( item );
        }
        src_items.clear();
    }

    // Helper function to move particular field type if corresponding target flag is enabled.
    auto move_field = [&sp, from, to]( spell_target target, field_type_id fid ) {
        if( !sp.is_valid_effect_target( target ) ) {
            return;
        }
        auto &src_field = g->m.field_at( from );
        if( field_entry *entry = src_field.find_field( fid ) ) {
            int intensity = entry->get_field_intensity();
            g->m.remove_field( from, fid );
            g->m.set_field_intensity( to, fid, intensity );
        }
    };
    // Moving fields.
    move_field( spell_target::fire, fd_fire );
    move_field( spell_target::blood, fd_blood );
    move_field( spell_target::blood, fd_gibs_flesh );
}

void spell_effect::area_pull( const spell &sp, Creature &caster, const tripoint &center )
{
    area_expander expander;

    expander.max_range = sp.aoe();
    expander.run( center );
    expander.sort_ascending();

    for( const auto &node : expander.area ) {
        if( node.from == node.position ) {
            continue;
        }

        spell_move( sp, caster, node.position, node.from );
    }
    sp.make_sound( caster.pos() );
}

void spell_effect::area_push( const spell &sp, Creature &caster, const tripoint &center )
{
    area_expander expander;

    expander.max_range = sp.aoe();
    expander.run( center );
    expander.sort_descending();

    for( const auto &node : expander.area ) {
        if( node.from == node.position ) {
            continue;
        }

        spell_move( sp, caster, node.from, node.position );
    }
    sp.make_sound( caster.pos() );
}

void spell_effect::spawn_ethereal_item( const spell &sp, Creature &caster, const tripoint & )
{
    item granted( sp.effect_data(), calendar::turn );
    if( !granted.is_comestible() && !( sp.has_flag( spell_flag::PERMANENT ) && sp.is_max_level() ) ) {
        granted.set_var( "ethereal", to_turns<int>( sp.duration_turns() ) );
        granted.set_flag( "ETHEREAL_ITEM" );
    }
    if( granted.count_by_charges() && sp.damage() > 0 ) {
        granted.charges = sp.damage();
    }
    if( sp.has_flag( spell_flag::WITH_CONTAINER ) ) {
        granted = granted.in_its_container();
    }
    if( g->u.can_wear( granted ).success() ) {
        granted.set_flag( "FIT" );
        g->u.wear_item( granted, false );
    } else if( !g->u.is_armed() && g->u.wield( granted, 0 ) ) {
        // nothing to do
    } else {
        g->u.i_add( granted );
    }
    if( !granted.count_by_charges() ) {
        for( int i = 1; i < sp.damage(); i++ ) {
            g->u.i_add( granted );
        }
    }
    sp.make_sound( caster.pos() );
}

void spell_effect::recover_energy( const spell &sp, Creature &caster, const tripoint &target )
{
    // this spell is not appropriate for healing
    const int healing = sp.damage();
    const std::string energy_source = sp.effect_data();
    // TODO: Change to Character
    // current limitation is that Character does not have stamina or power_level members
    player *p = g->critter_at<player>( target );
    if( !p ) {
        return;
    }

    if( energy_source == "MANA" ) {
        p->magic.mod_mana( *p, healing );
    } else if( energy_source == "STAMINA" ) {
        p->mod_stamina( healing );
    } else if( energy_source == "FATIGUE" ) {
        // fatigue is backwards
        p->mod_fatigue( -healing );
    } else if( energy_source == "BIONIC" ) {
        if( healing > 0 ) {
            p->mod_power_level( units::from_kilojoule( healing ) );
        } else {
            p->mod_stamina( healing );
        }
    } else if( energy_source == "PAIN" ) {
        // pain is backwards
        if( sp.has_flag( spell_flag::PAIN_NORESIST ) ) {
            p->mod_pain_noresist( -healing );
        } else {
            p->mod_pain( -healing );
        }
    } else if( energy_source == "HEALTH" ) {
        p->mod_healthy( healing );
    } else {
        debugmsg( "Invalid effect_str %s for spell %s", energy_source, sp.name() );
    }
    sp.make_sound( caster.pos() );
}

void spell_effect::timed_event( const spell &sp, Creature &caster, const tripoint & )
{
    const std::map<std::string, timed_event_type> timed_event_map{
        { "help", timed_event_type::HELP },
        { "wanted", timed_event_type::WANTED },
        { "robot_attack", timed_event_type::ROBOT_ATTACK },
        { "spawn_wyrms", timed_event_type::SPAWN_WYRMS },
        { "amigara", timed_event_type::AMIGARA },
        { "roots_die", timed_event_type::ROOTS_DIE },
        { "temple_open", timed_event_type::TEMPLE_OPEN },
        { "temple_flood", timed_event_type::TEMPLE_FLOOD },
        { "temple_spawn", timed_event_type::TEMPLE_SPAWN },
        { "dim", timed_event_type::DIM },
        { "artifact_light", timed_event_type::ARTIFACT_LIGHT }
    };

    timed_event_type spell_event = timed_event_type::NONE;

    const auto iter = timed_event_map.find( sp.effect_data() );
    if( iter != timed_event_map.cend() ) {
        spell_event = iter->second;
    }

    sp.make_sound( caster.pos() );
    g->timed_events.add( spell_event, calendar::turn + sp.duration_turns() );
}

static bool is_summon_friendly( const spell &sp )
{
    const bool hostile = sp.has_flag( spell_flag::HOSTILE_SUMMON );
    bool friendly = !hostile;
    if( sp.has_flag( spell_flag::HOSTILE_50 ) ) {
        friendly = friendly && rng( 0, 1000 ) < 500;
    }
    return friendly;
}

static bool add_summoned_mon( const tripoint &pos, const time_duration &time, const spell &sp )
{
    std::string monster_id = sp.effect_data();

    // Spawn a monster from a group, or a specific monster ID
    if( sp.has_flag( spell_flag::SPAWN_GROUP ) ) {
        const mongroup_id group_id( sp.effect_data() );
        monster_id = MonsterGroupManager::GetRandomMonsterFromGroup( group_id ).str();
    }

    const mtype_id mon_id( monster_id );
    monster *const mon_ptr = g->place_critter_at( mon_id, pos );

    if( !mon_ptr ) {
        return false;
    }
    const bool permanent = sp.has_flag( spell_flag::PERMANENT );
    monster &spawned_mon = *mon_ptr;
    if( is_summon_friendly( sp ) ) {
        spawned_mon.friendly = INT_MAX;
    } else {
        spawned_mon.friendly = 0;
    }
    if( !permanent ) {
        spawned_mon.set_summon_time( time );
    }
    spawned_mon.no_extra_death_drops = true;
    return true;
}

void spell_effect::spawn_summoned_monster( const spell &sp, Creature &caster,
        const tripoint &target )
{
    std::set<tripoint> area = spell_effect_area( sp, target, spell_effect_blast, caster );
    // this should never be negative, but this'll keep problems from happening
    size_t num_mons = std::abs( sp.damage() );
    const time_duration summon_time = sp.duration_turns();
    while( num_mons > 0 && !area.empty() ) {
        const size_t mon_spot = rng( 0, area.size() - 1 );
        auto iter = area.begin();
        std::advance( iter, mon_spot );
        if( add_summoned_mon( *iter, summon_time, sp ) ) {
            num_mons--;
            sp.make_sound( *iter );
        } else {
            add_msg( m_bad, "failed to place monster" );
        }
        // whether or not we succeed in spawning a monster, we don't want to try this tripoint again
        area.erase( iter );
    }
}

void spell_effect::spawn_summoned_vehicle( const spell &sp, Creature &caster,
        const tripoint &target )
{
    if( g->m.veh_at( target ) ) {
        caster.add_msg_if_player( m_bad, _( "There is already a vehicle there." ) );
        return;
    }
    if( vehicle *veh = g->m.add_vehicle( sp.summon_vehicle_id(), target, -90, 100, 0 ) ) {
        veh->magic = true;
        if( !sp.has_flag( spell_flag::PERMANENT ) ) {
            veh->summon_time_limit = sp.duration_turns();
        }
        if( caster.as_character() ) {
            veh->set_owner( *caster.as_character() );
        }
    }
}

void spell_effect::translocate( const spell &sp, Creature &caster, const tripoint &target )
{
    avatar *you = caster.as_avatar();
    if( you == nullptr ) {
        return;
    }
    you->translocators.translocate( spell_effect_area( sp, target, spell_effect_blast, caster, true ) );
}

void spell_effect::none( const spell &sp, Creature &, const tripoint & )
{
    debugmsg( "ERROR: %s has invalid spell effect.", sp.name() );
}

void spell_effect::transform_blast( const spell &sp, Creature &caster,
                                    const tripoint &target )
{
    ter_furn_transform_id transform( sp.effect_data() );
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), true );
    for( const tripoint &location : area ) {
        if( one_in( sp.damage() ) ) {
            transform->transform( location );
            transform->add_all_messages( caster, location );
        }
    }
}

void spell_effect::noise( const spell &sp, Creature &, const tripoint &target )
{
    sp.make_sound( target, sp.damage() );
}

void spell_effect::vomit( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), true );
    for( const tripoint &potential_target : area ) {
        if( !sp.is_valid_target( caster, potential_target ) ) {
            continue;
        }
        Character *const ch = g->critter_at<Character>( potential_target );
        if( !ch ) {
            continue;
        }
        sp.make_sound( target );
        ch->vomit();
    }
}

void spell_effect::explosion( const spell &sp, Creature &, const tripoint &target )
{
    explosion_handler::explosion( target, sp.damage(), sp.aoe() / 10.0, true );
}

void spell_effect::flashbang( const spell &sp, Creature &caster, const tripoint &target )
{
    explosion_handler::flashbang( target, caster.is_avatar() &&
                                  !sp.is_valid_target( spell_target::self ) );
}

void spell_effect::mod_moves( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), false );
    for( const tripoint &potential_target : area ) {
        if( !sp.is_valid_target( caster, potential_target ) ) {
            continue;
        }
        Creature *critter = g->critter_at<Creature>( potential_target );
        if( !critter ) {
            continue;
        }
        sp.make_sound( potential_target );
        critter->moves += sp.damage();
    }
}

void spell_effect::map( const spell &sp, Creature &caster, const tripoint & )
{
    const avatar *you = caster.as_avatar();
    if( !you ) {
        // revealing the map only makes sense for the avatar
        return;
    }
    const tripoint center = you->global_omt_location();
    overmap_buffer.reveal( center.xy(), sp.aoe(), center.z );
}

void spell_effect::morale( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), false );
    if( sp.effect_data().empty() ) {
        debugmsg( "ERROR: %s must have a valid morale_type as effect_str.  None specified.",
                  sp.id().c_str() );
        return;
    }
    if( !morale_type( sp.effect_data() ).is_valid() ) {
        debugmsg( "ERROR: %s must have a valid morale_type as effect_str.  %s is invalid.", sp.id().c_str(),
                  sp.effect_data() );
        return;
    }
    for( const tripoint &potential_target : area ) {
        player *player_target;
        if( !( sp.is_valid_target( caster, potential_target ) &&
               ( player_target = g->critter_at<player>( potential_target ) ) ) ) {
            continue;
        }
        player_target->add_morale( morale_type( sp.effect_data() ), sp.damage(), 0, sp.duration_turns(),
                                   sp.duration_turns() / 10, false );
        sp.make_sound( potential_target );
    }
}

void spell_effect::charm_monster( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), false );
    for( const tripoint &potential_target : area ) {
        if( !sp.is_valid_target( caster, potential_target ) ) {
            continue;
        }
        monster *mon = g->critter_at<monster>( potential_target );
        if( !mon ) {
            continue;
        }
        sp.make_sound( potential_target );
        if( mon->friendly == 0 && mon->get_hp() <= sp.damage() ) {
            mon->unset_dest();
            mon->friendly += sp.duration() / 100;
        }
    }
}

void spell_effect::mutate( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), false );
    for( const tripoint &potential_target : area ) {
        if( !sp.is_valid_target( caster, potential_target ) ) {
            continue;
        }
        Character *guy = g->critter_at<Character>( potential_target );
        if( !guy ) {
            continue;
        }
        // 10000 represents 100.00% to increase granularity without swapping everything to a float
        if( sp.damage() < rng( 1, 10000 ) ) {
            // chance failure! but keep trying for other targets
            continue;
        }
        if( sp.effect_data().empty() ) {
            guy->mutate();
        } else {
            if( sp.has_flag( spell_flag::MUTATE_TRAIT ) ) {
                guy->mutate_towards( trait_id( sp.effect_data() ) );
            } else {
                guy->mutate_category( sp.effect_data() );
            }
        }
        sp.make_sound( potential_target );
    }
}

void spell_effect::bash( const spell &sp, Creature &caster, const tripoint &target )
{
    const std::set<tripoint> area = spell_effect_blast( sp, caster.pos(), target, sp.aoe(), false );
    for( const tripoint &potential_target : area ) {
        if( !sp.is_valid_target( caster, potential_target ) ) {
            continue;
        }
        // the bash already makes noise, so no need for spell::make_sound()
        g->m.bash( potential_target, sp.damage(), sp.has_flag( spell_flag::SILENT ) );
    }
}
