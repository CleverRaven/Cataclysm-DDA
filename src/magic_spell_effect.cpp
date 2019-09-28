#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <set>
#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <queue>

#include "magic.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "enums.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "player.h"
#include "projectile.h"
#include "type_id.h"
#include "bodypart.h"
#include "map_iterator.h"
#include "damage.h"
#include "debug.h"
#include "explosion.h"
#include "magic_teleporter_list.h"
#include "magic_ter_furn_transform.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "translations.h"
#include "timed_event.h"
#include "teleport.h"

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

    for( const int &part : p->hp_cur ) {
        if( part != 0 ) {
            num_limbs++;
            total_hp += part;
        }
    }
    for( int &part : p->hp_cur ) {
        const int hp_each = total_hp / num_limbs;
        if( part != 0 ) {
            part = hp_each;
        }
    }
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
    for( int angle = initial_angle - floor( aoe_radius / 2.0 );
         angle <= initial_angle + ceil( aoe_radius / 2.0 ); angle++ ) {
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

std::set<tripoint> spell_effect::spell_effect_line( const spell &, const tripoint &source,
        const tripoint &target, const int aoe_radius, const bool ignore_walls )
{
    std::set<tripoint> targets;
    const int initial_angle = coord_to_angle( source, target );
    tripoint clockwise_starting_point;
    calc_ray_end( initial_angle - 90, floor( aoe_radius / 2.0 ), source, clockwise_starting_point );
    tripoint cclockwise_starting_point;
    calc_ray_end( initial_angle + 90, ceil( aoe_radius / 2.0 ), source, cclockwise_starting_point );
    tripoint clockwise_end_point;
    calc_ray_end( initial_angle - 90, floor( aoe_radius / 2.0 ), target, clockwise_end_point );
    tripoint cclockwise_end_point;
    calc_ray_end( initial_angle + 90, ceil( aoe_radius / 2.0 ), target, cclockwise_end_point );

    std::vector<tripoint> start_width = line_to( clockwise_starting_point, cclockwise_starting_point );
    start_width.insert( start_width.begin(), clockwise_end_point );
    std::vector<tripoint> end_width = line_to( clockwise_end_point, cclockwise_end_point );
    end_width.insert( end_width.begin(), clockwise_starting_point );

    std::vector<tripoint> cwise_line = line_to( clockwise_starting_point, cclockwise_starting_point );
    cwise_line.insert( cwise_line.begin(), clockwise_starting_point );
    std::vector<tripoint> ccwise_line = line_to( cclockwise_starting_point, cclockwise_end_point );
    ccwise_line.insert( ccwise_line.begin(), cclockwise_starting_point );

    for( const tripoint &start_line_pt : start_width ) {
        bool passable = true;
        for( const tripoint &potential_target : line_to( source, start_line_pt ) ) {
            passable = g->m.passable( potential_target ) || ignore_walls;
            if( passable ) {
                targets.emplace( potential_target );
            } else {
                break;
            }
        }
        if( !passable ) {
            // leading edge of line attack is very important to the whole
            // if the leading edge is blocked, none of the attack spawning
            // from that edge can propogate
            continue;
        }
        for( const tripoint &end_line_pt : end_width ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, end_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
        for( const tripoint &cwise_line_pt : cwise_line ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, cwise_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
        for( const tripoint &ccwise_line_pt : ccwise_line ) {
            std::vector<tripoint> temp_line = line_to( start_line_pt, ccwise_line_pt );
            for( const tripoint &potential_target : temp_line ) {
                if( ignore_walls || g->m.passable( potential_target ) ) {
                    targets.emplace( potential_target );
                } else {
                    break;
                }
            }
        }
    }

    targets.erase( source );

    return targets;
}

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
static std::set<tripoint> spell_effect_area( const spell &sp, const tripoint &target,
        std::function<std::set<tripoint>( const spell &, const tripoint &, const tripoint &, const int, const bool )>
        aoe_func, const Creature &caster, bool ignore_walls = false )
{
    std::set<tripoint> targets = { target }; // initialize with epicenter
    if( sp.aoe() <= 1 ) {
        return targets;
    }

    const int aoe_radius = sp.aoe();
    targets = aoe_func( sp, caster.pos(), target, aoe_radius, ignore_walls );

    for( const tripoint &p : targets ) {
        if( !sp.is_valid_target( caster, p ) ) {
            targets.erase( p );
        }
    }

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

static void damage_targets( const spell &sp, const Creature &caster,
                            const std::set<tripoint> &targets )
{
    for( const tripoint &target : targets ) {
        if( !sp.is_valid_target( caster, target ) ) {
            continue;
        }
        sp.make_sound( target );
        sp.create_field( target );
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
        if( !sp.effect_data().empty() ) {
            add_effect_to_target( target, sp );
        }
        if( sp.damage() > 0 ) {
            cr->deal_projectile_attack( &g->u, atk, true );
        } else if( sp.damage() < 0 ) {
            sp.heal( target );
            add_msg( m_good, _( "%s wounds are closing up!" ), cr->disp_name( true ) );
        }
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
    bool can_target_creature = sp.is_valid_effect_target( target_self ) ||
                               sp.is_valid_effect_target( target_ally ) ||
                               sp.is_valid_effect_target( target_hostile );

    if( can_target_creature ) {
        if( Creature *victim = g->critter_at<Creature>( from ) ) {
            Creature::Attitude cr_att = victim->attitude_to( g->u );
            bool valid = cr_att != Creature::A_FRIENDLY && sp.is_valid_effect_target( target_hostile );
            valid |= cr_att == Creature::A_FRIENDLY && sp.is_valid_effect_target( target_ally );
            valid |= victim == &caster && sp.is_valid_effect_target( target_self );
            if( valid ) {
                victim->knock_back_to( to );
            }
        }
    }

    // Moving items
    if( sp.is_valid_effect_target( target_item ) ) {
        auto src_items = g->m.i_at( from );
        auto dst_items = g->m.i_at( to );
        for( const item &item : src_items ) {
            dst_items.insert( item );
        }
        src_items.clear();
    }

    // Helper function to move particular field type if corresponding target flag is enabled.
    auto move_field = [&sp, from, to]( valid_target target, field_type_id fid ) {
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
    move_field( target_fd_fire, fd_fire );
    move_field( target_fd_blood, fd_blood );
    move_field( target_fd_blood, fd_gibs_flesh );
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
    if( g->u.can_wear( granted ).success() ) {
        granted.set_flag( "FIT" );
        g->u.wear_item( granted, false );
    } else if( !g->u.is_armed() ) {
        g->u.weapon = granted;
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
        p->mod_stat( "stamina", healing );
    } else if( energy_source == "FATIGUE" ) {
        // fatigue is backwards
        p->mod_fatigue( -healing );
    } else if( energy_source == "BIONIC" ) {
        if( healing > 0 ) {
            p->power_level = units::from_kilojoule( std::min( units::to_kilojoule( p->max_power_level ),
                                                    units::to_kilojoule( p->power_level ) + healing ) );
        } else {
            p->mod_stat( "stamina", healing );
        }
    } else if( energy_source == "PAIN" ) {
        // pain is backwards
        p->mod_pain_noresist( -healing );
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
        { "help", timed_event_type::TIMED_EVENT_HELP },
        { "wanted", timed_event_type::TIMED_EVENT_WANTED },
        { "robot_attack", timed_event_type::TIMED_EVENT_ROBOT_ATTACK },
        { "spawn_wyrms", timed_event_type::TIMED_EVENT_SPAWN_WYRMS },
        { "amigara", timed_event_type::TIMED_EVENT_AMIGARA },
        { "roots_die", timed_event_type::TIMED_EVENT_ROOTS_DIE },
        { "temple_open", timed_event_type::TIMED_EVENT_TEMPLE_OPEN },
        { "temple_flood", timed_event_type::TIMED_EVENT_TEMPLE_FLOOD },
        { "temple_spawn", timed_event_type::TIMED_EVENT_TEMPLE_SPAWN },
        { "dim", timed_event_type::TIMED_EVENT_DIM },
        { "artifact_light", timed_event_type::TIMED_EVENT_ARTIFACT_LIGHT }
    };

    timed_event_type spell_event = timed_event_type::TIMED_EVENT_NULL;

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

static bool add_summoned_mon( const mtype_id &id, const tripoint &pos, const time_duration &time,
                              const spell &sp )
{
    if( g->critter_at( pos ) ) {
        // add_zombie doesn't check if there's a critter at the location already
        return false;
    }
    const bool permanent = sp.has_flag( spell_flag::PERMANENT );
    monster spawned_mon( id, pos );
    if( is_summon_friendly( sp ) ) {
        spawned_mon.friendly = INT_MAX;
    } else {
        spawned_mon.friendly = 0;
    }
    if( !permanent ) {
        spawned_mon.set_summon_time( time );
    }
    spawned_mon.no_extra_death_drops = true;
    return g->add_zombie( spawned_mon );
}

void spell_effect::spawn_summoned_monster( const spell &sp, Creature &caster,
        const tripoint &target )
{
    const mtype_id mon_id( sp.effect_data() );
    std::set<tripoint> area = spell_effect_area( sp, target, spell_effect_blast, caster );
    // this should never be negative, but this'll keep problems from happening
    size_t num_mons = abs( sp.damage() );
    const time_duration summon_time = sp.duration_turns();
    while( num_mons > 0 && !area.empty() ) {
        const size_t mon_spot = rng( 0, area.size() - 1 );
        auto iter = area.begin();
        std::advance( iter, mon_spot );
        if( add_summoned_mon( mon_id, *iter, summon_time, sp ) ) {
            num_mons--;
            sp.make_sound( *iter );
        } else {
            add_msg( m_bad, "failed to place monster" );
        }
        // whether or not we succeed in spawning a monster, we don't want to try this tripoint again
        area.erase( iter );
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
