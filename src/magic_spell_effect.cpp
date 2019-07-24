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

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "explosion.h"
#include "game.h"
#include "item.h"
#include "line.h"
#include "magic.h"
#include "magic_teleporter_list.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "monattack.h"
#include "morale_types.h"
#include "overmapbuffer.h"
#include "player.h"
#include "projectile.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "type_id.h"

static tripoint random_point( int min_distance, int max_distance, const tripoint &player_pos )
{
    const int angle = rng( 0, 360 );
    const int dist = rng( min_distance, max_distance );
    const int x = round( dist * cos( angle ) );
    const int y = round( dist * sin( angle ) );
    return tripoint( x + player_pos.x, y + player_pos.y, player_pos.z );
}

void spell_effect::teleport( int min_distance, int max_distance )
{
    if( min_distance > max_distance || min_distance < 0 || max_distance < 0 ) {
        debugmsg( "ERROR: Teleport argument(s) invalid" );
        return;
    }
    const tripoint player_pos = g->u.pos();
    tripoint target;
    // limit the loop just in case it's impossble to find a valid point in the range
    int tries = 0;
    do {
        target = random_point( min_distance, max_distance, player_pos );
        tries++;
    } while( g->m.impassable( target ) && tries < 20 );
    if( tries == 20 ) {
        add_msg( m_bad, _( "Unable to find a valid target for teleport." ) );
        return;
    }
    g->place_player( target );
}

void spell_effect::pain_split()
{
    player &p = g->u;
    add_msg( m_info, _( "Your injuries even out." ) );
    int num_limbs = 0; // number of limbs effected (broken don't count)
    int total_hp = 0; // total hp among limbs

    for( const int &part : p.hp_cur ) {
        if( part != 0 ) {
            num_limbs++;
            total_hp += part;
        }
    }
    for( int &part : p.hp_cur ) {
        const int hp_each = total_hp / num_limbs;
        if( part != 0 ) {
            part = hp_each;
        }
    }
}

void spell_effect::move_earth( const tripoint &target )
{
    ter_id ter_here = g->m.ter( target );

    std::set<ter_id> empty_air = { t_hole };
    std::set<ter_id> deep_pit = { t_pit, t_slope_down };
    std::set<ter_id> shallow_pit = { t_pit_corpsed, t_pit_covered, t_pit_glass, t_pit_glass_covered, t_pit_shallow, t_pit_spiked, t_pit_spiked_covered, t_rootcellar };
    std::set<ter_id> soft_dirt = { t_grave, t_dirt, t_sand, t_clay, t_dirtmound, t_grass, t_grass_long, t_grass_tall, t_grass_golf, t_grass_dead, t_grass_white, t_dirtfloor, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_sandbox };
    // rock: can still be dug through with patience, converts to sand upon completion
    std::set<ter_id> hard_dirt = { t_pavement, t_pavement_y, t_sidewalk, t_concrete, t_thconc_floor, t_thconc_floor_olight, t_strconc_floor, t_floor, t_floor_waxed, t_carpet_red, t_carpet_yellow, t_carpet_purple, t_carpet_green, t_linoleum_white, t_linoleum_gray, t_slope_up, t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue, t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp };

    if( empty_air.count( ter_here ) == 1 ) {
        add_msg( m_bad, _( "All the dust in the air here falls to the ground." ) );
    } else if( deep_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_hole );
        add_msg( _( "The pit has deepened further." ) );
    } else if( shallow_pit.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit );
        add_msg( _( "More debris shifts out of the pit." ) );
    } else if( soft_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_pit_shallow );
        add_msg( _( "The earth moves out of the way for you." ) );
    } else if( hard_dirt.count( ter_here ) == 1 ) {
        g->m.ter_set( target, t_sand );
        add_msg( _( "The rocks here are ground into sand." ) );
    } else {
        add_msg( m_bad, _( "The earth here does not listen to your command to move." ) );
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
    for( int x = target.x - aoe_radius; x <= target.x + aoe_radius; x++ ) {
        for( int y = target.y - aoe_radius; y <= target.y + aoe_radius; y++ ) {
            const tripoint potential_target( x, y, target.z );
            if( in_spell_aoe( target, potential_target, aoe_radius, ignore_walls ) ) {
                targets.emplace( potential_target );
            }
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

void spell_effect::projectile_attack( const spell &sp, const Creature &caster,
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

void spell_effect::target_attack( const spell &sp, const Creature &caster,
                                  const tripoint &epicenter )
{
    damage_targets( sp, caster, spell_effect_area( sp, epicenter, spell_effect_blast, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::cone_attack( const spell &sp, const Creature &caster,
                                const tripoint &target )
{
    damage_targets( sp, caster, spell_effect_area( sp, target, spell_effect_cone, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::line_attack( const spell &sp, const Creature &caster,
                                const tripoint &target )
{
    damage_targets( sp, caster, spell_effect_area( sp, target, spell_effect_line, caster,
                    sp.has_flag( spell_flag::IGNORE_WALLS ) ) );
}

void spell_effect::spawn_ethereal_item( const spell &sp )
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
}

void spell_effect::recover_energy( const spell &sp, const tripoint &target )
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
            p->power_level = std::min( p->max_power_level, p->power_level + healing );
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

void spell_effect::spawn_summoned_monster( const spell &sp, const Creature &caster,
        const tripoint &target )
{
    const mtype_id mon_id( sp.effect_data() );
    std::set<tripoint> area = spell_effect_area( sp, target, spell_effect_blast, caster );
    // this should never be negative, but this'll keep problems from happening
    size_t num_mons = abs( sp.damage() );
    const time_duration summon_time = sp.duration_turns();
    while( num_mons > 0 && area.size() > 0 ) {
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

void spell_effect::translocate( const spell &sp, const Creature &caster,
                                const tripoint &target, teleporter_list &tp_list )
{
    tp_list.translocate( spell_effect_area( sp, target, spell_effect_blast, caster, true ) );
}

// hardcoded artifact actives
void spell_effect::storm( const spell &sp, const tripoint &target )
{
    sounds::sound( target, 10, sounds::sound_t::combat, _( "Ka-BOOM!" ), true, "environment",
                   "thunder_near" );
    int num_bolts = rng( 2, 4 );
    for( int j = 0; j < num_bolts; j++ ) {
        int xdir = 0;
        int ydir = 0;
        while( xdir == 0 && ydir == 0 ) {
            xdir = rng( -1, 1 );
            ydir = rng( -1, 1 );
        }
        int dist = rng( 4, 12 );
        int boltx = target.x, bolty = target.y;
        for( int n = 0; n < dist; n++ ) {
            boltx += xdir;
            bolty += ydir;
            sp.create_field( {boltx, bolty, target.z} );
            if( one_in( 4 ) ) {
                if( xdir == 0 ) {
                    xdir = rng( 0, 1 ) * 2 - 1;
                } else {
                    xdir = 0;
                }
            }
            if( one_in( 4 ) ) {
                if( ydir == 0 ) {
                    ydir = rng( 0, 1 ) * 2 - 1;
                } else {
                    ydir = 0;
                }
            }
        }
    }
}

void spell_effect::fire_ball( const tripoint &target )
{
    explosion_handler::explosion( target, 180, 0.5, true );
}

void spell_effect::map( const spell &sp, const tripoint &target )
{
    player *p = g->critter_at<player>( target );
    const bool new_map = overmap_buffer.reveal(
                             point( target.x, target.y ), sp.aoe(), target.z );
    if( new_map ) {
        p->add_msg_if_player( m_warning, _( "You have a vision of the surrounding area..." ) );
        p->mod_moves( -to_moves<int>( 1_seconds ) );
    }
}

void spell_effect::blood( const spell &sp, const Creature &caster, const tripoint &target )
{
    bool blood = false;
    for( const tripoint &tmp : g->m.points_in_radius( target, sp.aoe() ) ) {
        if( !one_in( 4 ) ) {
            sp.create_field( tmp );
            blood |= g->u.sees( tmp );
        }
    }
    if( blood ) {
        caster.add_msg_if_player( m_warning, _( "Blood soaks out of the ground and walls." ) );
    }
}

void spell_effect::fatigue( const spell &sp, const Creature &caster, const tripoint &target )
{
    caster.add_msg_if_player( m_warning, _( "The fabric of space seems to decay." ) );
    int x = rng( target.x - 3, target.x + 3 ), y = rng( target.y - 3, target.y + 3 );
    sp.create_field( {x, y, target.z} );
}

void spell_effect::pulse( const spell &sp, const tripoint &target )
{
    sounds::sound( target, 30, sounds::sound_t::combat, _( "The earth shakes!" ), true, "misc",
                   "earthquake" );
    for( const tripoint &pt : g->m.points_in_radius( target, sp.aoe() ) ) {
        g->m.bash( pt, sp.damage() );
        g->m.bash( pt, sp.damage() );  // Multibash effect, so that doors &c will fall
        g->m.bash( pt, sp.damage() );
        if( g->m.is_bashable( pt ) && rng( 1, 10 ) >= 3 ) {
            g->m.bash( pt, 999, false, true );
        }
    }
}

void spell_effect::entrance( const spell &sp, const tripoint &target )
{
    for( const tripoint &dest : g->m.points_in_radius( target, sp.aoe() ) ) {
        monster *const mon = g->critter_at<monster>( dest, true );
        if( mon && mon->friendly == 0 && rng( 0, 600 ) > mon->get_hp() ) {
            mon->make_friendly();
        }
    }
}

void spell_effect::bugs( const spell &sp, const Creature &caster, const tripoint &target )
{
    int roll = rng( 1, 10 );
    mtype_id bug = mtype_id::NULL_ID();
    int num = 0;
    std::vector<tripoint> empty;
    for( const tripoint &dest : g->m.points_in_radius( target, sp.aoe() ) ) {
        if( g->is_empty( dest ) ) {
            empty.push_back( dest );
        }
    }
    if( empty.empty() || roll <= 4 ) {
        caster.add_msg_if_player( m_warning, _( "Flies buzz around you." ) );
    } else if( roll <= 7 ) {
        caster.add_msg_if_player( m_warning, _( "Giant flies appear!" ) );
        bug = static_cast<mtype_id>( "mon_fly" );
        num = rng( 2, 4 );
    } else if( roll <= 9 ) {
        caster.add_msg_if_player( m_warning, _( "Giant bees appear!" ) );
        bug = static_cast<mtype_id>( "mon_bee" );
        num = rng( 1, 3 );
    } else {
        caster.add_msg_if_player( m_warning, _( "Giant wasps appear!" ) );
        bug = static_cast<mtype_id>( "mon_wasp" );
        num = rng( 1, 2 );
    }
    if( bug ) {
        for( int j = 0; j < num && !empty.empty(); j++ ) {
            const tripoint spawnp = random_entry_removed( empty );
            if( monster *const b = g->summon_mon( bug, spawnp ) ) {
                b->friendly = -1;
                b->add_effect( efftype_id( "effect_pet" ), 1_turns, num_bp, true );
            }
        }
    }
}

void spell_effect::light( const Creature &caster )
{
    caster.add_msg_if_player( _( "The %s glows brightly!" ), "it->tname()" );
    g->events.add( EVENT_ARTIFACT_LIGHT, calendar::turn + 3_minutes );
}

void spell_effect::growth( const tripoint &target )
{
    monster tmptriffid( mtype_id::NULL_ID(), target );
    mattack::growplants( &tmptriffid );
}

void spell_effect::mutate( const Creature &caster )
{
    player *p = g->critter_at<player>( caster.pos() );
    if( !one_in( 3 ) ) {
        p->mutate();
    }
}

void spell_effect::teleglow( const Creature &caster )
{
    player *p = g->critter_at<player>( caster.pos() );
    p->add_msg_if_player( m_warning, _( "You feel unhinged." ) );
    p->add_effect( efftype_id( "effect_teleglow" ), rng( 30_minutes, 120_minutes ) );
}

void spell_effect::noise( const spell &sp, const Creature &caster, const tripoint &target )
{
    sounds::sound( target, sp.damage(), sounds::sound_t::combat,
                   string_format( _( "a deafening boom from %s %s" ),
                                  caster.disp_name( true ), "it->tname()" ), true, "misc", "shockwave" );
}

void spell_effect::scream( const spell &sp, const Creature &caster, const tripoint &target )
{
    player *p = g->critter_at<player>( caster.pos() );
    sounds::sound( target, sp.damage(), sounds::sound_t::alert,
                   string_format( _( "a disturbing scream from %s %s" ),
                                  p->disp_name( true ), "it->tname()" ), true, "shout", "scream" );
    if( !p->is_deaf() ) {
        p->add_morale( MORALE_SCREAM, -10, 0, 30_minutes, 1_minutes );
    }
}

void spell_effect::dim( const Creature &caster )
{
    caster.add_msg_if_player( _( "The sky starts to dim." ) );
    g->events.add( EVENT_DIM, calendar::turn + 5_minutes );
}

void spell_effect::flash( const Creature &caster, const tripoint &target )
{
    caster.add_msg_if_player( _( "The %s flashes brightly!" ), "it->tname()" );
    explosion_handler::flashbang( target );
}

void spell_effect::vomit( const Creature &caster )
{
    player *p = g->critter_at<player>( caster.pos() );
    p->add_msg_if_player( m_bad, _( "A wave of nausea passes through you!" ) );
    p->vomit();
}

void spell_effect::shadows( const Creature &caster, const tripoint &target )
{
    int num_shadows = rng( 4, 8 );
    int num_spawned = 0;
    mtype_id mon_shadow = static_cast<mtype_id>( "mon_shadow" );
    for( int j = 0; j < num_shadows; j++ ) {
        int tries = 0;
        tripoint monp = target;
        do {
            if( one_in( 2 ) ) {
                monp.x = rng( target.x - 5, target.x + 5 );
                monp.y = ( one_in( 2 ) ? target.y - 5 : target.y + 5 );
            } else {
                monp.x = ( one_in( 2 ) ? target.x - 5 : target.x + 5 );
                monp.y = rng( target.y - 5, target.y + 5 );
            }
        } while( tries < 5 && !g->is_empty( monp ) &&
                 !g->m.sees( monp, target, 10 ) );
        if( tries < 5 ) { // TODO: tries increment is missing, so this expression is always true
            if( monster *const  spawned = g->summon_mon( mon_shadow, monp ) ) {
                num_spawned++;
                spawned->reset_special_rng( "DISAPPEAR" );
            }
        }
    }
    if( num_spawned > 1 ) {
        caster.add_msg_if_player( m_warning, _( "Shadows form around you." ) );
    } else if( num_spawned == 1 ) {
        caster.add_msg_if_player( m_warning, _( "A shadow forms nearby." ) );
    }
}

void spell_effect::stamina_empty( const Creature &caster )
{
    player *p = g->critter_at<player>( caster.pos() );
    p->add_msg_if_player( m_bad, _( "Your body feels like jelly." ) );
    p->stamina = p->stamina * 1 / ( rng( 3, 8 ) );
}

void spell_effect::fun( const Creature &caster )
{
    player *p = g->critter_at<player>( caster.pos() );
    p->add_msg_if_player( m_good, _( "You're filled with euphoria!" ) );
    p->add_morale( MORALE_FEELING_GOOD, rng( 20, 50 ), 0, 5_minutes, 5_turns, false );
}
