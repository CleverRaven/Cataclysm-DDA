#include "mondeath.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "event.h"
#include "field.h"
#include "fungal_effects.h"
#include "game.h"
#include "harvest.h"
#include "itype.h"
#include "iuse_actor.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "morale_types.h"
#include "mtype.h"
#include "output.h"
#include "player.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"

const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_blob_brain( "mon_blob_brain" );
const mtype_id mon_blob_small( "mon_blob_small" );
const mtype_id mon_breather( "mon_breather" );
const mtype_id mon_breather_hub( "mon_breather_hub" );
const mtype_id mon_creeper_hub( "mon_creeper_hub" );
const mtype_id mon_creeper_vine( "mon_creeper_vine" );
const mtype_id mon_halfworm( "mon_halfworm" );
const mtype_id mon_sewer_rat( "mon_sewer_rat" );
const mtype_id mon_thing( "mon_thing" );
const mtype_id mon_zombie_dancer( "mon_zombie_dancer" );
const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
const mtype_id mon_giant_cockroach( "mon_giant_cockroach" );
const mtype_id mon_giant_cockroach_nymph( "mon_giant_cockroach_nymph" );
const mtype_id mon_pregnant_giant_cockroach( "mon_pregnant_giant_cockroach" );

const species_id ZOMBIE( "ZOMBIE" );
const species_id BLOB( "BLOB" );

const efftype_id effect_amigara( "amigara" );
const efftype_id effect_boomered( "boomered" );
const efftype_id effect_controlled( "controlled" );
const efftype_id effect_darkness( "darkness" );
const efftype_id effect_glowing( "glowing" );
const efftype_id effect_no_ammo( "no_ammo" );
const efftype_id effect_pacified( "pacified" );
const efftype_id effect_rat( "rat" );

static const trait_id trait_PACIFIST( "PACIFIST" );
static const trait_id trait_PRED1( "PRED1" );
static const trait_id trait_PRED2( "PRED2" );
static const trait_id trait_PRED3( "PRED3" );
static const trait_id trait_PRED4( "PRED4" );
static const trait_id trait_PSYCHOPATH( "PSYCHOPATH" );

void mdeath::normal( monster &z )
{
    if( z.no_corpse_quiet ) {
        return;
    }

    if( z.type->in_species( ZOMBIE ) ) {
        sfx::play_variant_sound( "mon_death", "zombie_death", sfx::get_heard_volume( z.pos() ) );
    }

    if( g->u.sees( z ) ) {
        //Currently it is possible to get multiple messages that a monster died.
        add_msg( m_good, _( "The %s dies!" ), z.name().c_str() );
    }

    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;
    const bool pulverized = corpse_damage > 5 && overflow_damage > z.get_hp_max();

    z.bleed(); // leave some blood if we have to

    if( !pulverized ) {
        make_mon_corpse( z, int( std::floor( corpse_damage * itype::damage_scale ) ) );
    }
    // if mdeath::splatter was set along normal makes sure it is not called twice
    bool splatt = false;
    for( const auto &deathfunction : z.type->dies ) {
        if( deathfunction == mdeath::splatter ) {
            splatt = true;
        }
    }
    if( !splatt ) {
        splatter( z );
    }
}

void scatter_chunks( const std::string &chunk_name, int chunk_amt, monster &z, int distance,
                     int pile_size = 1 )
{
    // can't have less than one item in a pile or it would cause an infinite loop
    pile_size = std::max( pile_size, 1 );
    // can't have more items in a pile than total items
    pile_size = std::min( chunk_amt, pile_size );
    distance = abs( distance );
    const item chunk( chunk_name, calendar::turn, pile_size );
    for( int i = 0; i < chunk_amt; i += pile_size ) {
        bool drop_chunks = true;
        tripoint tarp( z.pos() + point( rng( -distance, distance ), rng( -distance, distance ) ) );
        const auto traj = line_to( z.pos(), tarp );

        for( size_t j = 0; j < traj.size(); j++ ) {
            tarp = traj[j];
            if( one_in( 2 ) && z.bloodType() != fd_null ) {
                g->m.add_splatter( z.bloodType(), tarp );
            } else {
                g->m.add_splatter( z.gibType(), tarp, rng( 1, j + 1 ) );
            }
            if( g->m.impassable( tarp ) ) {
                g->m.bash( tarp, distance );
                if( g->m.impassable( tarp ) ) {
                    // Target is obstacle, not destroyed by bashing,
                    // stop trajectory in front of it, if this is the first
                    // point (e.g. wall adjacent to monster), don't drop anything on it
                    if( j > 0 ) {
                        tarp = traj[j - 1];
                    } else {
                        drop_chunks = false;
                    }
                    break;
                }
            }
        }
        if( drop_chunks ) {
            g->m.add_item_or_charges( tarp, chunk );
        }
    }
}

void mdeath::splatter( monster &z )
{
    const bool gibbable = !z.type->has_flag( MF_NOGIB );

    const int max_hp = std::max( z.get_hp_max(), 1 );
    const float overflow_damage = std::max( -z.get_hp(), 0 );
    const float corpse_damage = 2.5 * overflow_damage / max_hp;
    bool pulverized = corpse_damage > 5 && overflow_damage > z.get_hp_max();
    // make sure that full splatter happens when this is a set death function, not part of normal
    for( const auto &deathfunction : z.type->dies ) {
        if( deathfunction == mdeath::splatter ) {
            pulverized = true;
        }
    }

    const field_id type_blood = z.bloodType();
    const field_id type_gib = z.gibType();

    if( gibbable ) {
        const auto area = g->m.points_in_radius( z.pos(), 1 );
        int number_of_gibs = std::min( std::floor( corpse_damage ) - 1, 1 + max_hp / 5.0f );

        if( pulverized && z.type->size >= MS_MEDIUM ) {
            number_of_gibs += rng( 1, 6 );
            sfx::play_variant_sound( "mon_death", "zombie_gibbed", sfx::get_heard_volume( z.pos() ) );
        }

        for( int i = 0; i < number_of_gibs; ++i ) {
            g->m.add_splatter( type_gib, random_entry( area ), rng( 1, i + 1 ) );
            g->m.add_splatter( type_blood, random_entry( area ) );
        }
    }
    // 1% of the weight of the monster is the base, with overflow damage as a multiplier
    int gibbed_weight = rng( 0, round( to_gram( z.get_weight() ) / 100 *
                                       ( overflow_damage / max_hp + 1 ) ) );
    // limit gibbing to 15%
    gibbed_weight = std::min( gibbed_weight, to_gram( z.get_weight() ) * 15 / 100 );

    if( pulverized && gibbable ) {
        float overflow_ratio = overflow_damage / max_hp + 1;
        int gib_distance = round( rng( 2, 4 ) );
        for( const auto &entry : *z.type->harvest ) {
            // only flesh and bones survive.
            if( entry.type == "flesh" || entry.type == "bone" ) {
                // the larger the overflow damage, the less you get
                const int chunk_amt = entry.mass_ratio / overflow_ratio / 10 * to_gram(
                                          z.get_weight() ) / to_gram( ( item::find_type( entry.drop ) )->weight );
                scatter_chunks( entry.drop, chunk_amt, z, gib_distance, chunk_amt / ( gib_distance - 1 ) );
                gibbed_weight -= entry.mass_ratio / overflow_ratio / 20 * to_gram( z.get_weight() );
            }
        }
        if( gibbed_weight > 0 ) {
            scatter_chunks( "ruined_chunks",
                            gibbed_weight / to_gram( ( item::find_type( "ruined_chunks" ) ) ->weight ), z, gib_distance,
                            gibbed_weight / to_gram( ( item::find_type( "ruined_chunks" ) )->weight ) / ( gib_distance + 1 ) );
        }
        // add corpse with gib flag
        item corpse = item::make_corpse( z.type->id, calendar::turn, z.unique_name );
        // Set corpse to damage that aligns with being pulped
        corpse.set_damage( 4000 );
        corpse.set_flag( "GIBBED" );
        corpse.active = false;
        if( z.has_effect( effect_no_ammo ) ) {
            corpse.set_var( "no_ammo", "no_ammo" );
        }
        g->m.add_item_or_charges( z.pos(), corpse );
    }
}

void mdeath::acid( monster &z )
{
    if( g->u.sees( z ) ) {
        if( z.type->dies.size() ==
            1 ) { //If this death function is the only function. The corpse gets dissolved.
            add_msg( m_mixed, _( "The %s's body dissolves into acid." ), z.name().c_str() );
        } else {
            add_msg( m_warning, _( "The %s's body leaks acid." ), z.name().c_str() );
        }
    }
    g->m.add_field( z.pos(), fd_acid, 3 );
}

void mdeath::boomer( monster &z )
{
    std::string explode = string_format( _( "a %s explode!" ), z.name().c_str() );
    sounds::sound( z.pos(), 24, sounds::sound_t::combat, explode );
    for( auto &&dest : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        g->m.bash( dest, 10 );
        if( monster *const z = g->critter_at<monster>( dest ) ) {
            z->stumble();
            z->moves -= 250;
        }
    }

    if( rl_dist( z.pos(), g->u.pos() ) == 1 ) {
        g->u.add_env_effect( effect_boomered, bp_eyes, 2, 24_turns );
    }

    g->m.propagate_field( z.pos(), fd_bile, 15, 1 );
}

void mdeath::boomer_glow( monster &z )
{
    std::string explode = string_format( _( "a %s explode!" ), z.name().c_str() );
    sounds::sound( z.pos(), 24, sounds::sound_t::combat, explode );

    for( auto &&dest : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        g->m.bash( dest, 10 );
        if( monster *const z = g->critter_at<monster>( dest ) ) {
            z->stumble();
            z->moves -= 250;
        }
        if( Creature *const critter = g->critter_at( dest ) ) {
            critter->add_env_effect( effect_boomered, bp_eyes, 5, 25_turns );
            for( int i = 0; i < rng( 2, 4 ); i++ ) {
                body_part bp = random_body_part();
                critter->add_env_effect( effect_glowing, bp, 4, 4_minutes );
                if( critter != nullptr && critter->has_effect( effect_glowing ) ) {
                    break;
                }
            }
        }
    }

    g->m.propagate_field( z.pos(), fd_bile, 30, 2 );
}

void mdeath::kill_vines( monster &z )
{
    const std::vector<Creature *> vines = g->get_creatures_if( [&]( const Creature & critter ) {
        const monster *const mon = dynamic_cast<const monster *>( &critter );
        return mon && mon->type->id == mon_creeper_vine;
    } );
    const std::vector<Creature *> hubs = g->get_creatures_if( [&]( const Creature & critter ) {
        const monster *const mon = dynamic_cast<const monster *>( &critter );
        return mon && mon != &z && mon->type->id == mon_creeper_hub;
    } );

    for( Creature *const vine : vines ) {
        int dist = rl_dist( vine->pos(), z.pos() );
        bool closer = false;
        for( auto &j : hubs ) {
            if( rl_dist( vine->pos(), j->pos() ) < dist ) {
                break;
            }
        }
        if( !closer ) { // @todo: closer variable is not being updated and is always false!
            vine->die( &z );
        }
    }
}

void mdeath::vine_cut( monster &z )
{
    std::vector<monster *> vines;
    for( const tripoint &tmp : g->m.points_in_radius( z.pos(), 1 ) ) {
        if( tmp == z.pos() ) {
            continue; // Skip ourselves
        }
        if( monster *const z = g->critter_at<monster>( tmp ) ) {
            if( z->type->id == mon_creeper_vine ) {
                vines.push_back( z );
            }
        }
    }

    for( auto &vine : vines ) {
        bool found_neighbor = false;
        tripoint tmp = vine->pos();
        int &x = tmp.x;
        int &y = tmp.y;
        for( x = vine->posx() - 1; x <= vine->posx() + 1 && !found_neighbor; x++ ) {
            for( y = vine->posy() - 1; y <= vine->posy() + 1 && !found_neighbor; y++ ) {
                if( x != z.posx() || y != z.posy() ) {
                    // Not the dying vine
                    if( monster *const v = g->critter_at<monster>( { x, y, z.posz() } ) ) {
                        if( v->type->id == mon_creeper_hub || v->type->id == mon_creeper_vine ) {
                            found_neighbor = true;
                        }
                    }
                }
            }
        }
        if( !found_neighbor ) {
            vine->die( &z );
        }
    }
}

void mdeath::triffid_heart( monster &z )
{
    if( g->u.sees( z ) ) {
        add_msg( m_warning, _( "The surrounding roots begin to crack and crumble." ) );
    }
    g->events.add( EVENT_ROOTS_DIE, calendar::turn + 10_minutes );
}

void mdeath::fungus( monster &z )
{
    // If the fungus died from anti-fungal poison, don't pouf
    if( g->m.get_field_strength( z.pos(), fd_fungicidal_gas ) ) {
        return;
    }

    //~ the sound of a fungus dying
    sounds::sound( z.pos(), 10, sounds::sound_t::combat, _( "Pouf!" ) );

    fungal_effects fe( *g, g->m );
    for( auto &&sporep : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        if( g->m.impassable( sporep ) ) {
            continue;
        }
        // z is dead, don't credit it with the kill
        // Maybe credit z's killer?
        fe.fungalize( sporep, nullptr, 0.25 );
    }
}

void mdeath::disintegrate( monster &z )
{
    if( g->u.sees( z ) ) {
        add_msg( m_good, _( "The %s disintegrates!" ), z.name().c_str() );
    }
}

void mdeath::worm( monster &z )
{
    if( g->u.sees( z ) ) {
        if( z.type->dies.size() == 1 ) {
            add_msg( m_good, _( "The %s splits in two!" ), z.name().c_str() );
        } else {
            add_msg( m_warning, _( "Two worms crawl out of the %s's corpse." ), z.name().c_str() );
        }
    }

    std::vector <tripoint> wormspots;
    for( auto &&wormp : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        if( g->m.has_flag( "DIGGABLE", wormp ) && g->is_empty( wormp ) ) {
            wormspots.push_back( wormp );
        }
    }
    int worms = 0;
    while( worms < 2 && !wormspots.empty() ) {
        const tripoint target = random_entry_removed( wormspots );
        if( !g->critter_at( target ) ) {
            g->summon_mon( mon_halfworm, target );
            worms++;
        }
    }
}

void mdeath::disappear( monster &z )
{
    if( g->u.sees( z ) ) {
        add_msg( m_good, _( "The %s disappears." ), z.name().c_str() );
    }
}

void mdeath::guilt( monster &z )
{
    const int MAX_GUILT_DISTANCE = 5;
    int kill_count = g->kill_count( z.type->id );
    int maxKills = 100; // this is when the player stop caring altogether.

    // different message as we kill more of the same monster
    std::string msg = _( "You feel guilty for killing %s." ); // default guilt message
    game_message_type msgtype = m_bad; // default guilt message type
    std::map<int, std::string> guilt_tresholds;
    guilt_tresholds[75] = _( "You feel ashamed for killing %s." );
    guilt_tresholds[50] = _( "You regret killing %s." );
    guilt_tresholds[25] = _( "You feel remorse for killing %s." );

    if( g->u.has_trait( trait_PSYCHOPATH ) || g->u.has_trait( trait_PRED3 ) ||
        g->u.has_trait( trait_PRED4 ) ) {
        return;
    }
    if( rl_dist( z.pos(), g->u.pos() ) > MAX_GUILT_DISTANCE ) {
        // Too far away, we can deal with it.
        return;
    }
    if( z.get_hp() >= 0 ) {
        // We probably didn't kill it
        return;
    }
    if( kill_count >= maxKills ) {
        // player no longer cares
        if( kill_count == maxKills ) {
            //~ Message after killing a lot of monsters which would normally affect the morale negatively. %s is the monster name, it will be pluralized with a number of 100.
            add_msg( m_good, _( "After killing so many bloody %s you no longer care "
                                "about their deaths anymore." ), z.name( maxKills ).c_str() );
        }
        return;
    } else if( ( g->u.has_trait( trait_PRED1 ) ) || ( g->u.has_trait( trait_PRED2 ) ) ) {
        msg = ( _( "Culling the weak is distasteful, but necessary." ) );
        msgtype = m_neutral;
    } else {
        msgtype = m_bad;
        for( auto &guilt_treshold : guilt_tresholds ) {
            if( kill_count >= guilt_treshold.first ) {
                msg = guilt_treshold.second;
                break;
            }
        }
    }

    add_msg( msgtype, msg.c_str(), z.name().c_str() );

    int moraleMalus = -50 * ( 1.0 - ( static_cast<float>( kill_count ) / maxKills ) );
    int maxMalus = -250 * ( 1.0 - ( static_cast<float>( kill_count ) / maxKills ) );
    time_duration duration = 30_minutes * ( 1.0 - ( static_cast<float>( kill_count ) / maxKills ) );
    time_duration decayDelay = 3_minutes * ( 1.0 - ( static_cast<float>( kill_count ) / maxKills ) );
    if( z.type->in_species( ZOMBIE ) ) {
        moraleMalus /= 10;
        if( g->u.has_trait( trait_PACIFIST ) ) {
            moraleMalus *= 5;
        } else if( g->u.has_trait( trait_PRED1 ) ) {
            moraleMalus /= 4;
        } else if( g->u.has_trait( trait_PRED2 ) ) {
            moraleMalus /= 5;
        }
    }
    g->u.add_morale( MORALE_KILLED_MONSTER, moraleMalus, maxMalus, duration, decayDelay );

}

void mdeath::blobsplit( monster &z )
{
    int speed = z.get_speed() - rng( 30, 50 );
    g->m.spawn_item( z.pos(), "slime_scrap", 1, 0, calendar::turn );
    if( z.get_speed() <= 0 ) {
        if( g->u.sees( z ) ) {
            //  TODO:  Add vermin-tagged tiny versions of the splattered blob  :)
            add_msg( m_good, _( "The %s splatters apart." ), z.name().c_str() );
        }
        return;
    }
    if( g->u.sees( z ) ) {
        if( z.type->dies.size() == 1 ) {
            add_msg( m_good, _( "The %s splits in two!" ), z.name().c_str() );
        } else {
            add_msg( m_bad, _( "Two small blobs slither out of the corpse." ) );
        }
    }
    std::vector <tripoint> valid;

    for( auto &&dest : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        if( g->is_empty( dest ) && z.can_move_to( dest ) ) {
            valid.push_back( dest );
        }
    }

    for( int s = 0; s < 2 && !valid.empty(); s++ ) {
        const tripoint target = random_entry_removed( valid );
        if( monster *const blob = g->summon_mon( speed < 50 ? mon_blob_small : mon_blob, target ) ) {
            blob->make_ally( z );
            blob->set_speed_base( speed );
            blob->set_hp( speed );
        }
    }
}

void mdeath::brainblob( monster &z )
{
    for( monster &critter : g->all_monsters() ) {
        if( critter.type->in_species( BLOB ) && critter.type->id != mon_blob_brain ) {
            critter.remove_effect( effect_controlled );
        }
    }
    blobsplit( z );
}

void mdeath::jackson( monster &z )
{
    for( monster &critter : g->all_monsters() ) {
        if( critter.type->id == mon_zombie_dancer ) {
            critter.poly( mon_zombie_hulk );
            critter.remove_effect( effect_controlled );
        }
        if( g->u.sees( z ) ) {
            add_msg( m_warning, _( "The music stops!" ) );
        }
    }
}

void mdeath::melt( monster &z )
{
    if( g->u.sees( z ) ) {
        add_msg( m_good, _( "The %s melts away." ), z.name().c_str() );
    }
}

void mdeath::amigara( monster &z )
{
    const bool has_others = g->get_creature_if( [&]( const Creature & critter ) {
        if( const monster *const candidate = dynamic_cast<const monster *>( &critter ) ) {
            return candidate->type == z.type;
        }
        return false;
    } );
    if( has_others ) {
        return;
    }

    // We were the last!
    if( g->u.has_effect( effect_amigara ) ) {
        g->u.remove_effect( effect_amigara );
        add_msg( _( "Your obsession with the fault fades away..." ) );
    }

    g->m.spawn_artifact( z.pos() );
}

void mdeath::thing( monster &z )
{
    g->summon_mon( mon_thing, z.pos() );
}

void mdeath::explode( monster &z )
{
    int size = 0;
    switch( z.type->size ) {
        case MS_TINY:
            size = 4;
            break;
        case MS_SMALL:
            size = 8;
            break;
        case MS_MEDIUM:
            size = 14;
            break;
        case MS_LARGE:
            size = 20;
            break;
        case MS_HUGE:
            size = 26;
            break;
    }
    g->explosion( z.pos(), size );
}

void mdeath::focused_beam( monster &z )
{

    for( int k = g->m.i_at( z.pos() ).size() - 1; k >= 0; k-- ) {
        if( g->m.i_at( z.pos() )[k].typeId() == "processor" ) {
            g->m.i_rem( z.pos(), k );
        }
    }

    if( !z.inv.empty() ) {

        if( g->u.sees( z ) ) {
            add_msg( m_warning, _( "As the final light is destroyed, it erupts in a blinding flare!" ) );
        }

        item &settings = z.inv[0];

        int x = z.posx() + settings.get_var( "SL_SPOT_X", 0 );
        int y = z.posy() + settings.get_var( "SL_SPOT_Y", 0 );
        tripoint p( x, y, z.posz() );

        std::vector <tripoint> traj = line_to( z.pos(), p, 0, 0 );
        for( auto &elem : traj ) {
            if( !g->m.trans( elem ) ) {
                break;
            }
            g->m.add_field( elem, fd_dazzling, 2 );
        }
    }

    z.inv.clear();

    g->explosion( z.pos(), 8 );
}

void mdeath::broken( monster &z )
{
    // Bail out if flagged (simulates eyebot flying away)
    if( z.no_corpse_quiet ) {
        return;
    }
    std::string item_id = z.type->id.str();
    if( item_id.compare( 0, 4, "mon_" ) == 0 ) {
        item_id.erase( 0, 4 );
    }
    // make "broken_manhack", or "broken_eyebot", ...
    item_id.insert( 0, "broken_" );
    g->m.spawn_item( z.pos(), item_id, 1, 0, calendar::turn );
    if( g->u.sees( z.pos() ) ) {
        add_msg( m_good, _( "The %s collapses!" ), z.name().c_str() );
    }
}

void mdeath::ratking( monster &z )
{
    g->u.remove_effect( effect_rat );
    if( g->u.sees( z ) ) {
        add_msg( m_warning, _( "Rats suddenly swarm into view." ) );
    }

    std::vector <tripoint> ratspots;
    for( auto &&ratp : g->m.points_in_radius( z.pos(), 1 ) ) { // *NOPAD*
        if( g->is_empty( ratp ) ) {
            ratspots.push_back( ratp );
        }
    }
    for( int rats = 0; rats < 7 && !ratspots.empty(); rats++ ) {
        g->summon_mon( mon_sewer_rat, random_entry_removed( ratspots ) );
    }
}

void mdeath::darkman( monster &z )
{
    g->u.remove_effect( effect_darkness );
    if( g->u.sees( z ) ) {
        add_msg( m_good, _( "The %s melts away." ), z.name().c_str() );
    }
}

void mdeath::gas( monster &z )
{
    std::string explode = string_format( _( "a %s explode!" ), z.name().c_str() );
    sounds::sound( z.pos(), 24, sounds::sound_t::combat, explode );
    g->m.emit_field( z.pos(), emit_id( "emit_toxic_blast" ) );
}

void mdeath::smokeburst( monster &z )
{
    std::string explode = string_format( _( "a %s explode!" ), z.name().c_str() );
    sounds::sound( z.pos(), 24, sounds::sound_t::combat, explode );
    g->m.emit_field( z.pos(), emit_id( "emit_smoke_blast" ) );
}

void mdeath::jabberwock( monster &z )
{
    player *ch = dynamic_cast<player *>( z.get_killer() );

    bool vorpal = ch && ch->is_player() &&
                  rl_dist( z.pos(), ch->pos() ) <= 1 &&
                  ch->weapon.has_flag( "DIAMOND" ) &&
                  ch->weapon.volume() > units::from_milliliter( 750 );

    if( vorpal && !ch->weapon.has_technique( matec_id( "VORPAL" ) ) ) {
        if( ch->sees( z ) ) {
            //~ %s is the possessive form of the monster's name
            ch->add_msg_if_player( m_info,
                                   _( "As the flames in %s eyes die out, your weapon seems to shine slightly brighter." ),
                                   z.disp_name( true ).c_str() );
        }
        ch->weapon.add_technique( matec_id( "VORPAL" ) );
    }

    mdeath::normal( z );
}

void mdeath::gameover( monster &z )
{
    add_msg( m_bad, _( "The %s was destroyed!  GAME OVER!" ), z.name().c_str() );
    g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers( monster &z )
{
    ( void )z; //unused
    for( monster &critter : g->all_monsters() ) {
        const mtype_id &monID = critter.type->id;
        if( monID == mon_breather_hub || monID == mon_breather ) {
            critter.die( nullptr );
        }
    }
}

void mdeath::detonate( monster &z )
{
    weighted_int_list<std::string> amm_list;
    for( auto amm : z.ammo ) {
        amm_list.add( amm.first, amm.second );
    }

    std::vector<std::string> pre_dets;
    for( int i = 0; i < 3; i++ ) {
        if( amm_list.get_weight() <= 0 ) {
            break;
        }
        // Grab one item
        std::string tmp = *amm_list.pick();
        // and reduce its weight by 1
        amm_list.add_or_replace( tmp, amm_list.get_specific_weight( tmp ) - 1 );
        // and stash it for use
        pre_dets.push_back( tmp );
    }

    // Update any hardcoded explosion equivalencies
    std::vector<std::pair<std::string, long>> dets;
    for( const std::string &bomb_id : pre_dets ) {
        if( bomb_id == "bot_grenade_hack" ) {
            dets.push_back( std::make_pair( "grenade_act", 5 ) );
        } else if( bomb_id == "bot_flashbang_hack" ) {
            dets.push_back( std::make_pair( "flashbang_act", 5 ) );
        } else if( bomb_id == "bot_gasbomb_hack" ) {
            dets.push_back( std::make_pair( "gasbomb_act", 20 ) );
        } else if( bomb_id == "bot_c4_hack" ) {
            dets.push_back( std::make_pair( "c4armed", 10 ) );
        } else if( bomb_id == "bot_mininuke_hack" ) {
            dets.push_back( std::make_pair( "mininuke_act", 20 ) );
        } else {
            // Get the transformation item
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>(
                                              item::find_type( bomb_id )->get_use( "transform" )->get_actor_ptr() );
            if( actor == nullptr ) {
                // Invalid bomb item, move to the next ammo item
                add_msg( m_debug, "Invalid bomb type in detonate mondeath for %s.", z.name().c_str() );
                continue;
            }
            dets.emplace_back( actor->target, actor->ammo_qty );
        }
    }

    if( g->u.sees( z ) ) {
        if( dets.empty() ) {
            //~ %s is the possessive form of the monster's name
            add_msg( m_info,
                     _( "The %s's hands fly to its pockets, but there's nothing left in them." ),
                     z.name().c_str() );
        } else {
            //~ %s is the possessive form of the monster's name
            add_msg( m_bad, _( "The %s's hands fly to its remaining pockets, opening them!" ),
                     z.name().c_str() );
        }
    }
    // HACK, used to stop them from having ammo on respawn
    z.add_effect( effect_no_ammo, 1_turns, num_bp, true );

    // First die normally
    mdeath::normal( z );
    // Then detonate our suicide bombs
    for( auto bombs : dets ) {
        item bomb_item( bombs.first, 0 );
        bomb_item.charges = bombs.second;
        bomb_item.active = true;
        g->m.add_item_or_charges( z.pos(), bomb_item );
    }
}

void mdeath::broken_ammo( monster &z )
{
    if( g->u.sees( z.pos() ) ) {
        //~ %s is the possessive form of the monster's name
        add_msg( m_info, _( "The %s's interior compartment sizzles with destructive energy." ),
                 z.name().c_str() );
    }
    mdeath::broken( z );
}

void make_mon_corpse( monster &z, int damageLvl )
{
    item corpse = item::make_corpse( z.type->id, calendar::turn, z.unique_name );
    corpse.set_damage( damageLvl );
    if( z.has_effect( effect_pacified ) && z.type->in_species( ZOMBIE ) ) {
        // Pacified corpses have a chance of becoming unpacified when regenerating.
        corpse.set_var( "zlave", one_in( 2 ) ? "zlave" : "mutilated" );
    }
    if( z.has_effect( effect_no_ammo ) ) {
        corpse.set_var( "no_ammo", "no_ammo" );
    }
    g->m.add_item_or_charges( z.pos(), corpse );
}

void mdeath::preg_roach( monster &z )
{
    int num_roach = rng( 1, 3 );
    std::vector <tripoint> roachspots;
    for( const auto &roachp : g->m.points_in_radius( z.pos(), 1 ) ) {
        if( g->is_empty( roachp ) ) {
            roachspots.push_back( roachp );
        }
    }

    while( !roachspots.empty() ) {
        const tripoint target = random_entry_removed( roachspots );
        if( !g->critter_at( target ) ) {
            g->summon_mon( mon_giant_cockroach_nymph, target );
            num_roach--;
            if( g->u.sees( z ) ) {
                add_msg( m_warning, _( "A cockroach nymph crawls out of the pregnant giant cockroach corpse." ) );
            }
        }
        if( num_roach == 0 ) {
            break;
        }
    }
}

void mdeath::fireball( monster &z )
{
    if( one_in( 10 ) ) {
        g->m.propagate_field( z.pos(), fd_fire, 15, 3 );
        std::string explode = string_format( _( "an explosion of tank of the %s's flamethrower!" ),
                                             z.name().c_str() );
        sounds::sound( z.pos(), 24, sounds::sound_t::combat, explode );
        add_msg( m_good, _( "I love the smell of burning zed in the morning." ) );
    } else {
        normal( z );
    }
}
